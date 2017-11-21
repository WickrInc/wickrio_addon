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
    void service(stefanfrings::HttpRequest& request,stefanfrings::HttpResponse& response);

private:
    void processSendMessage(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void processGetMessages(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void processDeleteMessages(stefanfrings::HttpResponse& response);

    void processAddRoom(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void processDeleteRoom(const QString &clientID, stefanfrings::HttpResponse& response);
    void processGetRooms(const QString &clientID, stefanfrings::HttpResponse& response);

    void processAddGroupConvo(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void processDeleteGroupConvo(const QString &clientID, stefanfrings::HttpResponse& response);
    void processGetGroupConvos(const QString &clientID, stefanfrings::HttpResponse& response);

    // TODO: This should move to a library!!!!
    void onCreateSecureRoom(const QString& vGroupID, const QStringList& mastersHashList, int destructionTime, const QString& roomTitle, const QString& roomDescription);
    bool deleteConvo(bool isSecureConvo, const QString& vgroupID);
    void onDeleteSecureRoom(const QString& vGroupID);

    void getStatistics(const QString& apiKey, stefanfrings::HttpResponse& response);
    void clearStatistics(const QString& apiKey, stefanfrings::HttpResponse& response);
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

signals:
    void signalMemberSearchDone();

};

#endif // REQUESTHANDLER_H
