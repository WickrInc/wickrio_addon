#include <QJsonValue>
#include <QJsonObject>
#include <QJsonDocument>

#include "wickrIOJScriptService.h"
#include "wickriodatabase.h"
#include "wickrIOClientRuntime.h"
#include "wickrIOAPIInterface.h"

#include "qamqpclient.h"
#include "qamqpqueue.h"
#include "qamqpexchange.h"

QString WickrIOJScriptService::jsServiceBaseName = "WickrIOJScriptThread";

#if 0
using namespace v8;
#endif

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
WickrIOJScriptThread::WickrIOJScriptThread(QThread *thread, WickrIOJScriptService *cbSvc, QObject *parent)
    : QObject(parent),
      m_parent(cbSvc)
    , m_state(JSThreadState::JS_UNINITIALIZED)
{
    thread->setObjectName("WickrIOJScriptThread");
    this->moveToThread(thread);

    // Signal to cleanup worker
    connect(thread, &QThread::finished, this, &QObject::deleteLater);

    m_state = JSThreadState::JS_STARTED;

#if 0
    // initialize the Javascript callback
    initJScriptCallback();
#endif
}

/**
 * @brief WickrIOJScriptThread::~WickrIOJScriptThread
 * Destructor
 */
WickrIOJScriptThread::~WickrIOJScriptThread() {
#if 0
    // If the Javascript callback is still setup then stop it
    if (m_jsCallbackInitialized) {
        stopJScriptCallback();
    }
#endif
    qDebug() << "WICKRIOJAVASCRIPT THREAD: Worker Destroyed.";
}

void
WickrIOJScriptThread::slotProcessMessages()
{
    // Don't want to start multiple processing to be initiated
    if (m_state == JSThreadState::JS_PROCESSING)
        return;

    m_state = JSThreadState::JS_PROCESSING;

#if 0
    jScriptSendMessage();
#endif
    m_state = JSThreadState::JS_STARTED;
}

void
WickrIOJScriptThread::slotStartScript()
{
    // Don't want to start multiple processing to be initiated
    if (m_state == JSThreadState::JS_PROCESSING)
        return;
    m_state = JSThreadState::JS_PROCESSING;


    // Initialize the RPC queue
    m_client = new QAmqpClient(this);
    m_client->setHost("localhost");
//    m_client->setUsername("guest");
//    m_client->setPassword("guest");
    connect(m_client, SIGNAL(connected()), this, SLOT(slotClientConnected()));

    slotListen();

#if 0
    jScriptStartScript();
#endif

    m_state = JSThreadState::JS_STARTED;
}

/**********************************************************************************************
 * QUEUING FUNCTIONS
 **********************************************************************************************/

void
WickrIOJScriptThread::slotListen()
{
    m_client->connectToHost();
}

QString
WickrIOJScriptThread::processRequest(const QByteArray& request)
{
    QJsonParseError jsonError;
    QJsonDocument jsonRequest = QJsonDocument().fromJson(request, &jsonError);
    if (jsonError.error != jsonError.NoError) {
        return "Parsing error!";
    }

    QJsonObject object = jsonRequest.object();

    if (!object.contains("action")) {
        return "Malformed request: no action!";
    }

    QJsonValue value;
    QString action;

    value = object["action"];
    action = value.toString().toLower();

    OperationData* operation = WickrIOClientRuntime::operationData();
    if (!operation) {
        return "Internal failure!";
    }
    WickrIOAPIInterface apiInterface(operation);

    QString responseString;

    // MESSAGE ACTIONS
    if (action == "send_message") {
        if (apiInterface.sendMessage(request, responseString)) {
            responseString = "Sending message";
        }
    }
    else if (action == "get_received_messages") {
        apiInterface.getReceivedMessages(responseString);
    }
    // ROOM ACTIONS
    else if (action == "add_room") {
        apiInterface.addRoom(request, responseString);
    }
    else if (action == "modify_room") {
        if (!object.contains("vgroupid")) {
            return "Malformed request: no vGroupID!";
        }
        value = object["vgroupid"];
        QString vGroupID = value.toString();

        apiInterface.updateRoom(vGroupID, request, responseString);
    }
    else if (action == "delete_room") {
        if (!object.contains("vgroupid")) {
            return "Malformed request: no vGroupID!";
        }
        value = object["vgroupid"];
        QString vGroupID = value.toString();

        apiInterface.deleteRoom(vGroupID, responseString);
    }
    else if (action == "leave_room") {
        if (!object.contains("vgroupid")) {
            return "Malformed request: no vGroupID!";
        }
        value = object["vgroupid"];
        QString vGroupID = value.toString();

        apiInterface.leaveRoom(vGroupID, responseString);
    }
    else if (action == "get_room") {
        if (!object.contains("vgroupid")) {
            return "Malformed request: no vGroupID!";
        }
        value = object["vgroupid"];
        QString vGroupID = value.toString();

        apiInterface.getRoom(vGroupID, responseString);
    }
    else if (action == "get_rooms") {
        apiInterface.getRooms(responseString);
    }
    // GROUP CONVERSATION ACTIONS
    else if (action == "add_groupconvo") {
        apiInterface.addGroupConvo(request, responseString);
    }
    else if (action == "delete_groupconvo") {
        if (!object.contains("vgroupid")) {
            return "Malformed request: no vGroupID!";
        }
        value = object["vgroupid"];
        QString vGroupID = value.toString();

        apiInterface.deleteGroupConvo(vGroupID, responseString);
    }
    else if (action == "get_groupconvo") {
        if (!object.contains("vgroupid")) {
            return "Malformed request: no vGroupID!";
        }
        value = object["vgroupid"];
        QString vGroupID = value.toString();

        apiInterface.getGroupConvo(vGroupID, responseString);
    }
    else if (action == "get_groupconvos") {
        apiInterface.getGroupConvos(responseString);
    }
    // STATISTICS ACTIONS
    else if (action == "get_statistics") {
        apiInterface.getStatistics("", responseString);
    }
    else if (action == "clear_statistics") {
        apiInterface.clearStatistics("", responseString);
    }
    // ERROR CASE
    else {
        responseString = QString("action '%1' unknown!").arg(action);
    }

    return responseString;
}

void WickrIOJScriptThread::slotClientConnected()
{
    OperationData* operation = WickrIOClientRuntime::operationData();
    QString queueName = operation->m_client->user + "_rpc";

    m_rpcQueue = m_client->createQueue(queueName);
    connect(m_rpcQueue, SIGNAL(declared()), this, SLOT(slotQueueDeclared()));
    connect(m_rpcQueue, SIGNAL(qosDefined()), this, SLOT(slotQosDefined()));
    connect(m_rpcQueue, SIGNAL(messageReceived()), this, SLOT(slotProcessRpcMessage()));
    m_rpcQueue->declare();

    m_defaultExchange = m_client->createExchange();
}

void WickrIOJScriptThread::slotQueueDeclared()
{
    m_rpcQueue->qos(1);
}

void WickrIOJScriptThread::slotQosDefined()
{
    m_rpcQueue->consume();
    qDebug() << " [x] Awaiting RPC requests";
}

void WickrIOJScriptThread::slotProcessRpcMessage()
{
    QAmqpMessage rpcMessage = m_rpcQueue->dequeue();

    QString response = processRequest(rpcMessage.payload());
    m_rpcQueue->ack(rpcMessage);

    QString replyTo = rpcMessage.property(QAmqpMessage::ReplyTo).toString();
    QAmqpMessage::PropertyHash properties;
    properties.insert(QAmqpMessage::CorrelationId, rpcMessage.property(QAmqpMessage::CorrelationId));
    m_defaultExchange->publish(response, replyTo, properties);
}

/**********************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************/

#if 0
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
#endif

/**********************************************************************************************
 * JAVASCRIPT FUNCTIONS
 **********************************************************************************************/

#if 0
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
    Local<Value> defResult = { v8::String::NewFromUtf8(m_isolate, "") };
    Local<Value> result = script->Run(context).FromMaybe(defResult);

}
#endif

