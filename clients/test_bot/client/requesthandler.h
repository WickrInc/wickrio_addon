#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include "wickriohttp.h"
#include "operationdata.h"
#include "wickrbotipc.h"
#include "wickrIOAPIInterface.h"
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
    OperationData       *m_operation;
    WickrIOAPIInterface m_apiInterface;

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

    void processSendMessage(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void processGetMessages(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void processDeleteMessages(stefanfrings::HttpResponse& response);

    void processUpdateRoom(const QString &vGroupID, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void processAddRoom(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void processDeleteRoom(const QString &clientID, stefanfrings::HttpResponse& response);
    void processLeaveRoom(const QString &vGroupID, stefanfrings::HttpResponse& response);
    void processGetRoom(const QString &vGroupID, stefanfrings::HttpResponse& response);
    void processGetRooms(stefanfrings::HttpResponse& response);

    void processAddGroupConvo(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void processDeleteGroupConvo(const QString &vGroupID, stefanfrings::HttpResponse& response);
    void processGetGroupConvo(const QString &vGroupID, stefanfrings::HttpResponse& response);
    void processGetGroupConvos(stefanfrings::HttpResponse& response);

    // TODO: This should move to a library!!!!
    bool deleteConvo(bool isSecureConvo, const QString& vgroupID);
    void onDeleteSecureRoom(const QString& vGroupID);

    void getStatistics(const QString& apiKey, stefanfrings::HttpResponse& response);
    void clearStatistics(const QString& apiKey, stefanfrings::HttpResponse& response);

signals:
    void signalMemberSearchDone();

};

#endif // REQUESTHANDLER_H
