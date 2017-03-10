#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include "wickriohttp.h"
#include "operationdata.h"
#include "wickrbotipc.h"
#include "messaging/wickrConvo.h"

#include "perftest.h"

/**
 * @brief The RequestHandler class
 * The request handler receives incoming HTTP requests and generates responses.
 */
class RequestHandler : public WickrIOHttpRequestHdlr {
    Q_OBJECT
    Q_DISABLE_COPY(RequestHandler)

public:
    RequestHandler(OperationData *operation, QObject* parent=0);
    ~RequestHandler();

    /**
      Process an incoming HTTP request.
      @param request The received HTTP request
      @param response Must be used to return the response
    */
    void service(HttpRequest& request, HttpResponse& response);

private:
    void processSendMessage(HttpRequest& request, HttpResponse& response);
    void processGetMessages(HttpRequest& request, HttpResponse& response);
    void processDeleteMessages(HttpResponse& response);

    void processAddRoom(HttpRequest& request, HttpResponse& response);
    void processDeleteRoom(const QString &clientID, HttpResponse& response);
    void processGetRooms(const QString &clientID, HttpResponse& response);

    // TODO: This should move to a library!!!!
    void onCreateSecureRoom(const QString& vGroupID, const QStringList& mastersHashList, int destructionTime, const QString& roomTitle, const QString& roomDescription);
    bool deleteConvo(bool isSecureConvo, const QString& vgroupID);
    void onDeleteSecureRoom(const QString& vGroupID);

    void getStatistics(const QString& apiKey, HttpResponse& response);
    int numMessages();

    // Helper functions
    QStringList getJsonArrayValue(QJsonObject jsonObject, QString jsonName, QString jsonArray);

private:
    OperationData *m_operation;

    typedef enum {
        ActionDeleteMessages,
        ActionGetMessages,
        ActionSendMessages,
        ActionSetMsgRecvCallback,
        ActionGetMsgRecvCallback,
        ActionAddRoom,
        ActionDeleteRoom,
        ActionGetRooms,
        ActionGetRoom,
        ActionUnknown
    } WickrIOActions;

    // Performance testing stuff
    static PerfTest * perftests[10];
    static bool perftestsetup;
};

#endif // REQUESTHANDLER_H
