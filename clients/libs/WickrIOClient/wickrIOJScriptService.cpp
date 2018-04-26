#include "wickrIOJScriptService.h"
#include "wickriodatabase.h"
#include "wickrIOClientRuntime.h"
#include "wickrIOAPIInterface.h"

QString WickrIOJScriptService::jsServiceBaseName = "WickrIOJScriptThread";

using namespace v8;

WickrIOJScriptService::WickrIOJScriptService() : WickrIOServiceBase(jsServiceBaseName),
    m_lock(QReadWriteLock::Recursive),
    m_state(WickrServiceState::SERVICE_UNINITIALIZED),
    m_cbThread(nullptr)
{
  qRegisterMetaType<WickrServiceState>("WickrServiceState");
  qRegisterMetaType<WickrApplicationState>("WickrApplicationState");

  // Start threads
  startThreads();

  setObjectName("WickrIOJScriptThread");
  qDebug() << "WICKRIOJAVASCRIPT SERVICE: Started.";
  m_state = WickrServiceState::SERVICE_STARTED;
}

/**
* @brief WickrIOJScriptService::~WickrIOJScriptService
* Destructor
*/
WickrIOJScriptService::~WickrIOJScriptService() {
    // Stop threads
    stopThreads();

    qDebug() << "WICKRIOJAVASCRIPT SERVICE: Shutdown.";
    m_state = WickrServiceState::SERVICE_SHUTDOWN;
}

/**
 * @brief WickrIOJScriptService::startThreads
 * Starts all threads on message service.
 */
void WickrIOJScriptService::startThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Allocate threads
    m_cbThread = new WickrIOJScriptThread(&m_thread,this);

    // Connect internal threads signals and slots
    connect(this, &WickrIOJScriptService::signalMessagesPending,
            m_cbThread, &WickrIOJScriptThread::slotProcessMessages, Qt::QueuedConnection);
    connect(this, &WickrIOJScriptService::signalStartScript,
            m_cbThread, &WickrIOJScriptThread::slotStartScript, Qt::QueuedConnection);

    // Perform startup here, creating and configuring ressources.
    m_thread.start();
}

/**
 * @brief WickrIOJScriptService::stopThreads
 * Stops all threads on JavaScript service.
 */
void WickrIOJScriptService::stopThreads()
{
    QWriteLocker lockGuard(&m_lock);

    // Perform shutdown here, wait for resources to quit, and cleanup

    // Task Service
    m_thread.quit();
    m_thread.wait();
    qDebug("JSCRIPT THREAD: Shutdown Thread (%p)", &m_thread);
}

void WickrIOJScriptService::messagesPending()
{
    emit signalMessagesPending();
}

void WickrIOJScriptService::startScript()
{
    emit signalStartScript();
}

/**
 * @brief WickrIOEventService::isHealthy
 * This function will return false if the health of this services is in a bad state. For the
 * Event Handler this is typically related to a stuck event.
 * @return
 */
bool WickrIOJScriptService::isHealthy()
{
    return true;
}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief WickrIOJScriptThread::asStringState
 * @param state
 * @return
 */
QString WickrIOJScriptThread::jsStringState(JSThreadState state)
{
    switch(state) {
    case JSThreadState::JS_UNINITIALIZED:   return "Uninitialized";
    case JSThreadState::JS_STARTED:         return "Started";
    case JSThreadState::JS_PROCESSING:      return "Processing";
    case JSThreadState::JS_FINISHED:        return "Finished";
    default:                                return "Unknown";
    }
}

/**
 * @brief WickrIOJScriptThread::WickrIOJScriptThread
 * Constructor
 */
WickrIOJScriptThread::WickrIOJScriptThread(QThread *thread, WickrIOJScriptService *cbSvc)
    : m_parent(cbSvc)
    , m_state(JSThreadState::JS_UNINITIALIZED)
{
    thread->setObjectName("WickrIOJScriptThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    m_state = JSThreadState::JS_STARTED;

    // initialize the Javascript callback
    initJScriptCallback();
}

/**
 * @brief WickrIOJScriptThread::~WickrIOJScriptThread
 * Destructor
 */
WickrIOJScriptThread::~WickrIOJScriptThread() {
    // If the Javascript callback is still setup then stop it
    if (m_jsCallbackInitialized) {
        stopJScriptCallback();
    }
    qDebug() << "WICKRIOJAVASCRIPT THREAD: Worker Destroyed.";
}

void
WickrIOJScriptThread::slotProcessMessages()
{
    // Don't want to start multiple processing to be initiated
    if (m_state == JSThreadState::JS_PROCESSING)
        return;

    m_state = JSThreadState::JS_PROCESSING;

    jScriptSendMessage();

    m_state = JSThreadState::JS_STARTED;
}

void
WickrIOJScriptThread::slotStartScript()
{
    // Don't want to start multiple processing to be initiated
    if (m_state == JSThreadState::JS_PROCESSING)
        return;

    m_state = JSThreadState::JS_PROCESSING;

    jScriptStartScript();

    m_state = JSThreadState::JS_STARTED;
}

/**********************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************/

void jsSendMessage(const FunctionCallbackInfo<Value>& args) {
    OperationData* operation = WickrIOClientRuntime::operationData();
    if (!operation) {
        return;
    }
    WickrIOAPIInterface apiInterface(operation);

    Isolate * isolate = args.GetIsolate();

    v8::String::Utf8Value s(args[0]);

    std::string str(*s, s.length());

    QByteArray jsonString = QByteArray::fromStdString(str);
    QString responseString;

    bool success = apiInterface.sendMessage(jsonString, responseString);
    std::string response = responseString.toStdString().c_str();

    Local<String> retval = String::NewFromUtf8(isolate, responseString.toStdString().c_str());
    args.GetReturnValue().Set(retval);
}

/**********************************************************************************************
 * JAVASCRIPT FUNCTIONS
 **********************************************************************************************/

bool
WickrIOJScriptThread::initJScriptCallback()
{
    // If the Javascript service is already initialized then return
    if (m_jsCallbackInitialized) {
        return true;
    }
    m_jsCallbackInitialized = true;

    QString appFilePath = QCoreApplication::applicationFilePath();

    // Initialize V8.
    if (!V8::InitializeICUDefaultLocation(appFilePath.toStdString().c_str())) {
        qDebug() << "InitializeICUDefaultLocation failed!";
        m_jsCallbackInitialized = false;
    } else {
        V8::InitializeExternalStartupData(appFilePath.toStdString().c_str());
        m_platform = platform::CreateDefaultPlatform();
        V8::InitializePlatform(m_platform);
        V8::Initialize();

        // Create a new Isolate and make it the current one.
        Isolate::CreateParams create_params;
        create_params.array_buffer_allocator =
                ArrayBuffer::Allocator::NewDefaultAllocator();
        m_isolate = Isolate::New(create_params);
        if (!m_isolate) {
            m_jsCallbackInitialized = false;
        } else {
            Isolate::Scope isolate_scope(m_isolate);

            // Create a stack-allocated handle scope.
            HandleScope handle_scope(m_isolate);

            // Create a new context.
            Local<Context> context = Context::New(m_isolate);

            m_persistent_context.Reset(m_isolate, context);
            v8::Context::Scope context_scope(context);


//            // Enter the context for compiling and running the hello world script.
//            Context::Scope context_scope(m_context);



           #if 0


            class Test
            {

              v8::Persistent<v8::Script> compiledScript;
              v8::Local<v8::Value> result;
              v8::Isolate* isolate;
              v8::Persistent<v8::Context> context;

              Test(filePath) {

                  // Create and allocate isolate

                  v8::Locker isolateLocker(isolate);
                  v8::Isolate::Scope isolate_scope(isolate);
                  v8::HandleScope handle_scope(isolate);
                  // Create some bindings

                  v8::Local<v8::Context> con = v8::Context::New(isolate, nullptr, binding_template);
                  con->SetAlignedPointerInEmbedderData(0, &binder);




                  context.Reset(isolate, con);
                  v8::Context::Scope context_scope(con);

                  std::string source_file = LoadFile(filePath);
                  v8::Local<v8::String> sourceScript = v8::String::NewFromUtf8(isolate, source_file.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
                  v8::Local<v8::Script> script = v8::Script::Compile(con, sourceScript).ToLocalChecked();
                  compiledScript.Reset(isolate, script);
            }
#endif
        }
    }

    return m_jsCallbackInitialized;
}

bool
WickrIOJScriptThread::stopJScriptCallback()
{
    // if the Javascript service is NOT initialized then just return
    if (!m_jsCallbackInitialized) {
        return true;
    }

    m_jsCallbackInitialized = false;

    // Displse the isolate and tear down V8
    m_isolate->Dispose();

    // dispose of v8
    V8::Dispose();
    V8::ShutdownPlatform();
    delete m_platform;

    return true;
}

bool
WickrIOJScriptThread::jScriptSendMessage()
{
    if (!m_jsCallbackInitialized) {
        if (!initJScriptCallback()) {
            return false;
        }
    }

    return true;
}

bool
WickrIOJScriptThread::jScriptStartScript()
{
    if (!m_jsCallbackInitialized) {
        if (!initJScriptCallback()) {
            return false;
        }
    }

#if 1
    v8::Locker isolateLocker(m_isolate);
    v8::Isolate::Scope isolate_scope(m_isolate);
    v8::HandleScope handle_scope(m_isolate);

    v8::Local<v8::Context> context= v8::Local<v8::Context>::New(m_isolate, m_persistent_context);
//    v8::Local<v8::Script> script = v8::Local<v8::Script>::New(m_isolate, compiledScript);

    v8::Context::Scope context_scope(context);

    // Code to call the Javascript function f1

    Handle<Object> global = context->Global();
    global->Set(String::NewFromUtf8(m_isolate, "sendMessage"), FunctionTemplate::New(m_isolate, jsSendMessage)->GetFunction());

    // Create a string containing the JavaScript source code.
    Local<String> source;
    QString fileString;

    QFile listFile("/home/pcushman/Development/wickr-wickrio/libs/third_party/sendMessage_1.js");
    if (listFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&listFile);
        while (!in.atEnd()) {
            QString line = in.readLine();
            fileString.append(line);
        }
        listFile.close();
    } else {

    }

    source = String::NewFromUtf8(m_isolate, fileString.toStdString().c_str(), NewStringType::kNormal).ToLocalChecked();

    // Compile the source code.
    Local<Script> script = Script::Compile(context, source).ToLocalChecked();

    // Run the script to get the result.
    Local<Value> result = script->Run(context).ToLocalChecked();


#if 0
    m_result = script->Run(con).ToLocalChecked();
    v8::Local<v8::Object> global = con->Global();
    v8::Local<v8::Value> function_value1 = global->Get(v8::String::NewFromUtf8(isolate, "f1"));
    v8::Local<v8::Function> function1 = v8::Local<v8::Function>::Cast(function_value1);

    v8::Local<v8::Value> js_result1;

    js_result1 = function1->Call(con, global, 0, nullptr).ToLocalChecked();
#endif

#else
#if 1
//    v8::Isolate*  isolate = v8::Isolate::GetCurrent();
    {
      v8::Locker locker(m_isolate);
      v8::Isolate::Scope isolate_scope(m_isolate);
      v8::HandleScope handle_scope(m_isolate);
//      v8::Handle<v8::Context> context = getGlobalContext();
      auto context = m_isolate->GetCurrentContext(); // no longer crashes
      v8::Context::Scope context_scope(context);

    }
#else


    v8::Isolate::Scope isolate_scope (m_isolate);
    v8::HandleScope handle_scope (m_isolate);

    //  Restore the context established at init time;
    //  Have to make local version of persistent handle
    v8::Local <v8::Context> context =
      v8::Local <v8::Context>::New (m_isolate, m_context);
    Context::Scope context_scope (context);

#if 0
    Handle <String> source = String::NewFromUtf8 (isolate_, js . utf8 ());
    Handle <Script> script = Script::Compile (source);
    Handle <Value> result = script -> Run ();
#endif




    Handle<Object> global = m_context->Global();
    global->Set(String::NewFromUtf8(m_isolate, "sendMessage"), FunctionTemplate::New(m_isolate, jsSendMessage)->GetFunction());

    // Create a string containing the JavaScript source code.
    Local<String> source;
    QString fileString;

    QFile listFile("/home/pcushman/Development/wickr-wickrio/libs/third_party/sendMessage_1.js");
    if (listFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&listFile);
        while (!in.atEnd()) {
            QString line = in.readLine();
            fileString.append(line);
        }
        listFile.close();
    } else {

    }

    source = String::NewFromUtf8(m_isolate, fileString.toStdString().c_str(), NewStringType::kNormal).ToLocalChecked();

    // Compile the source code.
    Local<Script> script = Script::Compile(m_context, source).ToLocalChecked();

    // Run the script to get the result.
    Local<Value> result = script->Run(m_context).ToLocalChecked();
#endif
#endif

}


