#include <QDebug>
#include <QByteArray>
#include <QCryptographicHash>
#include <QJsonObject>
#include <QJsonDocument>

#include "wickriohttp.h"
#include "wickrioapi.h"

/**
 * @brief WickrIOHttpRequestHdlr::WickrIOHttpRequestHdlr
 * Constructor for the HTTP Request Handler. This is a base class for all of the Web API classes
 * defined in WickrIO. This class contains functions that are likely common to all of the Web
 * API handlers.
 * @param parent
 */
WickrIOHttpRequestHdlr::WickrIOHttpRequestHdlr(QObject *parent) : stefanfrings::HttpRequestHandler(parent),
    m_ioDB(NULL)
{
}

/**
 * @brief WickrIOHttpRequestHdlr::setupResponse
 * This function will setup the common HTTP Response headers.
 * @param request Refers to the incoming HTTP request
 * @param response Refers to the outgoing HTTP response
 */
void
WickrIOHttpRequestHdlr::setupResponse(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    QList<QByteArray> origin = request.getHeaders("Origin");
    QList<QByteArray> method = request.getHeaders("Access-Control-Request-Method");
    QList<QByteArray> headers = request.getHeaders("Access-Control-Request-Headers");

    response.setStatus(200, "OK");
    response.setHeader("Content-Type", "text/plain; charset=UTF-8");

    if (origin.length() > 0) {
        qDebug() << "origin =" << origin.at(0);
        response.setHeader("Access-Control-Allow-Origin", origin.at(0));
    }

    if (method.length() > 0) {
        qDebug() << "method =" << method.at(0);
        response.setHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
    }

    if (headers.length() > 0) {
        qDebug() << "headers =" << headers.at(0);
        response.setHeader("Access-Control-Allow-Headers", headers.at(0));
    }
}

/**
 * @brief WickrIOHttpRequestHdlr::sendSuccess
 * This function will send a success response of HTTP 200 (OK), using the input HTTP Response
 * and associated input data (body).
 * @param data
 * @param response
 * @param lastPart
 */
void
WickrIOHttpRequestHdlr::sendSuccess(QByteArray data, stefanfrings::HttpResponse& response, bool lastPart)
{
    response.setStatus(200, "OK");
    response.setHeader("Content-Type", "application/json");
    response.write(data, lastPart);
}

/**
 * @brief WickrIOHttpRequestHdlr::sendSuccess
 * This function will send a success response of HTTP 200 (OK), using the input HTTP Response.
 * There is NO body in the response.
 * @param response
 * @param lastPart
 */
void
WickrIOHttpRequestHdlr::sendSuccess(stefanfrings::HttpResponse& response, bool lastPart)
{
    response.setStatus(200, "OK");
    response.setHeader("Content-Type", "text/plain; charset=UTF-8");
    response.write("", lastPart);
}

/**
 * @brief WickrIOHttpRequestHdlr::sendAccepted
 * This function will send an accepted response of HTTP 202 (Accepted), using the input HTTP
 * Response. There is no HTTP body associated with the response.
 * @param response
 * @param lastPart
 */
void
WickrIOHttpRequestHdlr::sendAccepted(stefanfrings::HttpResponse& response, bool lastPart)
{
    response.setStatus(202, "Accepted");
    response.setHeader("Content-Type", "text/plain; charset=UTF-8");
    response.write("", lastPart);
}

/**
 * @brief WickrIOHttpRequestHdlr::sendFailure
 * This function will send a failure response, using the input HTTP Response, and the input
 * msg string will be added to the HTTP body.
 * @param status
 * @param msg
 * @param response
 * @param lastPart
 */
void
WickrIOHttpRequestHdlr::sendFailure(int status, QByteArray msg, stefanfrings::HttpResponse& response, bool lastPart)
{
    QByteArray dsc;
    switch (status) {
    case 400:
        dsc = "Bad Request";
        break;
    case 401:
        dsc = "Unauthorized";
        break;
    case 403:
        dsc = "Forbidden";
        break;
    case 404:
        dsc = "Not Found";
        break;
    case 409:
        dsc = "Conflict";
        break;
    case 500:
        dsc = "Internal Server Error";
        break;
    default:
        dsc = "Unknown";
    }

    response.setStatus(status, dsc);
    response.setHeader("Content-Type", "text/plain; charset=UTF-8");
    response.write(msg, lastPart);
}

void
WickrIOHttpRequestHdlr::optionsResponse(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    setupResponse(request, response);
    response.write("", true);
}

/**
 * @brief WickrIOHttpRequestHdlr::validateAuthentication
 * This function is used to validate the authentication contained in the input HTTP
 * Request. If there is a problem with the authentication the input HTTP Response will be
 * modified appropriately, but the response will not be sent from this function. A value
 * of false is returned for authentication failures. The input Console User pointer will
 * be set with the associated Console User object.
 * @param request Reference to the HTTP Request object
 * @param response Reference to the HTTP Response object
 * @param cUser The associated Console User object
 * @return True if authentication is successful, false if not
 */
bool
WickrIOHttpRequestHdlr::validateAuthentication(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response, WickrIOConsoleUser *cUser)
{
    QByteArray authorizationHeader = request.getHeader("Authorization");
    if (!authorizationHeader.isEmpty()) {
        QList<QByteArray> authList = authorizationHeader.split(' ');
        // Perform Basic authorization
        if (authList.size() == 2 && authList.at(0).toLower() == "basic") {
            QByteArray aText = QByteArray::fromBase64(authList.at(1));
            qDebug() << "Decoded authorization header" << aText;
            QList<QByteArray> basicList = aText.split(':');
            if (basicList.size() != 2) {
                basicList = authList.at(1).split(':');
                if (basicList.size() != 2) {
                    response.setHeader("WWW-Authenticate", "Basic realm=WickrIO");
                    return false;
                }
            }
            QString user = QString(basicList.at(0));
            QString pw = QString(basicList.at(1));

            if (! m_ioDB->getConsoleUser(user, cUser)) {
                response.setHeader("WWW-Authenticate", "Basic realm=WickrIO");
                return false;
            }
            if (cUser->password != pw) {
                if (cUser->authType == CUSER_AUTHTYPE_EMAIL) {
                    response.setHeader("WWW-Authenticate", "WICKR-TOKEN realm=WickrIO");
                } else {
                    response.setHeader("WWW-Authenticate", "Basic realm=WickrIO");
                }
                return false;
            }
            return true;
        } else if (authList.size() == 2 && authList.at(0).toLower() == "wickr-token") {
            QByteArray tText = authList.at(1);

            // We need a way to identify which user, or we need to go through all of the tokens!
            QList<WickrIOTokens*> tokens = m_ioDB->getTokens();
            for (WickrIOTokens *token : tokens) {
                if (m_ioDB->getTokenConsoleUser(token->token, cUser)) {
                    // We got the user associated with this key, decrypt the token and verify
//                    QByteArray decoded = WickrUtil::decodeCipherText(tText, token->token);
                    QByteArray user = cUser->user.toLatin1();
                    QByteArray encoded = hmac_sha256(user, token->token.toLatin1());
                    qDebug() << "encoded value" << encoded;
                    if (tText == encoded) {
                        return true;
                    }
                }
            }
        }
    }
    response.setHeader("WWW-Authenticate", "Basic realm=WickrIO");
    return false;
}

/**
 * @brief WickrIOHttpRequestHdlr::hmac_sha256
 * This function will create an encrypted SHA256 string from the input Secret
 * and Key.
 * @param key
 * @param secret
 * @return
 */
QByteArray
WickrIOHttpRequestHdlr::hmac_sha256(const QByteArray &key, const QByteArray &secret)
{
    // Length of the text to be hashed
    int text_length;
    // For secret word
    QByteArray K;
    // Length of secret word
    int K_length;

    K_length = secret.size();
    text_length = key.size();

    // Need to do for XOR operation. Transforms QString to
    // unsigned char

    K = secret;

    // Inner padding
    QByteArray ipad;
    // Outer padding
    QByteArray opad;

    // If secret key > 64 bytes use this to obtain sha256 key
    if (K_length > 64) {
        QByteArray tempSecret;

        tempSecret.append(secret);

        K = QCryptographicHash::hash(tempSecret, QCryptographicHash::Sha256);
        K_length = 20;
    }

    // Fills ipad and opad with zeros
    ipad.fill(0, 64);
    opad.fill(0, 64);

    // Copies Secret to ipad and opad
    ipad.replace(0, K_length, K);
    opad.replace(0, K_length, K);

    // XOR operation for inner and outer pad
    for (int i = 0; i < 64; i++) {
        ipad[i] = ipad[i] ^ 0x36;
        opad[i] = opad[i] ^ 0x5c;
    }

    // Stores hashed content
    QByteArray context;

    // Appends XOR:ed ipad to context
    context.append(ipad, 64);
    // Appends key to context
    context.append(key);

    //Hashes inner pad
    QByteArray sha256 = QCryptographicHash::hash(context, QCryptographicHash::Sha256);

    context.clear();
    //Appends opad to context
    context.append(opad, 64);
    //Appends hashed inner pad to context
    context.append(sha256);

    sha256.clear();

    // Hashes outerpad
    sha256 = QCryptographicHash::hash(context, QCryptographicHash::Sha256);

    // String to return hashed stuff in Base64 format
//    QByteArray str(sha256.toBase64());
    QByteArray str(sha256.toHex());

    return str;
}


/*
 * These functions are used by the clients
 */

/*
 * Message Receive Callback Functions
 */

/**
 * @brief WickrIOHttpRequestHdlr::setMsgRecvCallback
 * This function will SET the Message Receive callback
 * @param apiKey
 * @param request
 * @param response
 */
void
WickrIOHttpRequestHdlr::setMsgRecvCallback(const QString& apiKey, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    bool success = false;
    QByteArray reason = "Failed setting callback";
    QByteArray callback = request.getParameter(APIPARAM_CALLBACKURL);

    if (m_ioDB != NULL) {
        WickrBotClients *client = m_ioDB->getClientUsingApiKey(apiKey);
        if (client != NULL) {
            WickrIOAppSettings appSetting;
            if (m_ioDB->getAppSetting(client->id, DB_APPSETTINGS_TYPE_MSGRECVEMAIL, &appSetting)) {
                reason = "Message receive email already set!";
            } else {
                appSetting.clientID = client->id;
                appSetting.type = DB_APPSETTINGS_TYPE_MSGRECVCALLBACK;
                appSetting.value = QString(callback);

                if (m_ioDB->updateAppSetting(&appSetting)) {
                    success = true;
                }
            }
        }
    }

    if (success) {
        sendSuccess(response);
    } else {
        sendFailure(400, reason, response);
    }
}

/**
 * @brief WickrIOHttpRequestHdlr::getMsgRecvCallback
 * This function will perform a GET on the Message Receive Callback
 * @param apiKey
 * @param response
 */
void
WickrIOHttpRequestHdlr::getMsgRecvCallback(const QString& apiKey, stefanfrings::HttpResponse& response)
{
    QJsonObject msgRecvValue;

    if (m_ioDB != NULL) {
        WickrBotClients *client = m_ioDB->getClientUsingApiKey(apiKey);
        if (client != NULL) {
            WickrIOAppSettings appSetting;
            if (m_ioDB->getAppSetting(client->id, DB_APPSETTINGS_TYPE_MSGRECVCALLBACK, &appSetting)) {
                msgRecvValue.insert(APIJSON_CALLBACKURL, appSetting.value);
            }
            delete client;
        }
    }

    QJsonDocument saveDoc(msgRecvValue);
    QByteArray byteArray = saveDoc.toJson();

    sendSuccess(byteArray, response);
}

/**
 * @brief WickrIOHttpRequestHdlr::deleteMsgRecvCallback
 * This function will perform a DELETE on the Message Receive Callback
 * @param apiKey
 * @param response
 */
void
WickrIOHttpRequestHdlr::deleteMsgRecvCallback(const QString& apiKey, stefanfrings::HttpResponse& response)
{
    bool success = false;

    if (m_ioDB != NULL) {
        WickrBotClients *client = m_ioDB->getClientUsingApiKey(apiKey);
        if (client != NULL) {
            WickrIOAppSettings appSetting;

            // If there is a URL callback setup then delete it
            if (m_ioDB->getAppSetting(client->id, DB_APPSETTINGS_TYPE_MSGRECVCALLBACK, &appSetting)) {
                m_ioDB->deleteAppSetting(appSetting.id);
            }
            success = true;
        }
    }

    if (success) {
        sendSuccess(response);
    } else {
        sendFailure(400, "Failed deleting callback", response);
    }
}


/*
 * Message Receive Email Callbacks
 */

/**
 * @brief WickrIOHttpRequestHdlr::setMsgRecvEmail
 * This function will SET the Email address for the Message Receive Email Callback
 * @param apiKey
 * @param request
 * @param response
 */
void
WickrIOHttpRequestHdlr::setMsgRecvEmail(const QString& apiKey, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response)
{
    bool success = false;
    QString reason;

    if (m_ioDB != NULL) {
        WickrBotClients *client = m_ioDB->getClientUsingApiKey(apiKey);
        if (client != NULL) {
            WickrIOAppSettings appSetting;
            if (m_ioDB->getAppSetting(client->id, DB_APPSETTINGS_TYPE_MSGRECVCALLBACK, &appSetting)) {
                reason = "Message receive callback already set!";
            } else {
                QByteArray server = request.getParameter(APIPARAM_SERVER);
                QByteArray port = request.getParameter(APIPARAM_PORT);
                QByteArray type = request.getParameter(APIPARAM_TYPE);
                QByteArray account = request.getParameter(APIPARAM_ACCOUNT);
                QByteArray password = request.getParameter(APIPARAM_PASSWORD);
                QByteArray sender = request.getParameter(APIPARAM_SENDER);
                QByteArray recipient = request.getParameter(APIPARAM_RECIPIENT);
                QByteArray subject = request.getParameter(APIPARAM_SUBJECT);
                int portNumber = 0;
                QString lowerType;

                if (! port.isEmpty()) {
                    portNumber = port.toInt();
                }

                if (! type.isEmpty()) {
                    lowerType = QString(type).toLower();
                }
                if (lowerType != "smtp" && lowerType != "ssl" && lowerType != "tls") {
                    reason = "Invalid email port type";
                } else if (server.isEmpty() || portNumber == 0 || account.isEmpty() || password.isEmpty() ||
                    sender.isEmpty() || recipient.isEmpty() || subject.isEmpty()) {
                    reason = "Incomplete or missing fields";
                } else {
                    appSetting.clientID = client->id;
                    appSetting.type = DB_APPSETTINGS_TYPE_MSGRECVEMAIL;
                    appSetting.setupEmail(QString(server), portNumber, lowerType,
                                          QString(account), QString(password),
                                          QString(sender), QString(recipient), QString(subject));

                    if (m_ioDB->updateAppSetting(&appSetting)) {
                        success = true;
                    }
                }
            }
        }
    }

    if (success) {
        sendSuccess(response);
    } else {
        sendFailure(400, reason.toLatin1(), response);
    }
}

/**
 * @brief WickrIOHttpRequestHdlr::getMsgRecvEmail
 * This function will perform the GET of the Message Receive Email Callback
 * @param apiKey
 * @param response
 */
void
WickrIOHttpRequestHdlr::getMsgRecvEmail(const QString& apiKey, stefanfrings::HttpResponse& response)
{
    QJsonObject msgRecvValue;

    if (m_ioDB != NULL) {
        WickrBotClients *client = m_ioDB->getClientUsingApiKey(apiKey);
        if (client != NULL) {
            WickrIOAppSettings appSetting;
            if (m_ioDB->getAppSetting(client->id, DB_APPSETTINGS_TYPE_MSGRECVEMAIL, &appSetting)) {
                if (appSetting.type == DB_APPSETTINGS_TYPE_MSGRECVEMAIL) {
                    WickrIOEmailSettings email;
                    if (appSetting.getEmail(&email)) {
                        QJsonObject emailSetup;
                        QJsonObject serverSetup;
                        serverSetup.insert(APIJSON_SERVER, email.server);
                        serverSetup.insert(APIJSON_PORT, email.port);
                        serverSetup.insert(APIJSON_TYPE, email.type);
                        serverSetup.insert(APIJSON_ACCOUNT, email.account);
                        serverSetup.insert(APIJSON_PASSWORD, email.password);
                        emailSetup.insert(APIJSON_SERVERSETUP, serverSetup);

                        QJsonObject msgSetup;
                        msgSetup.insert(APIJSON_SENDER, email.sender);
                        msgSetup.insert(APIJSON_RECIPIENT, email.recipient);
                        msgSetup.insert(APIJSON_SUBJECT, email.subject);
                        emailSetup.insert(APIJSON_MSGSETUP, msgSetup);

                        msgRecvValue.insert(APIJSON_CBACKEMAIL, emailSetup);
                    }
                }
            }
        }
    }

    QJsonDocument saveDoc(msgRecvValue);
    QByteArray byteArray = saveDoc.toJson();

    sendSuccess(byteArray, response);
}

/**
 * @brief WickrIOHttpRequestHdlr::deleteMsgRecvEmail
 * This function will perofrm the DELETE of the Message Receive Email Callback
 * @param apiKey
 * @param response
 */
void
WickrIOHttpRequestHdlr::deleteMsgRecvEmail(const QString& apiKey, stefanfrings::HttpResponse& response)
{
    bool success = false;

    if (m_ioDB != NULL) {
        WickrBotClients *client = m_ioDB->getClientUsingApiKey(apiKey);
        if (client != NULL) {
            WickrIOAppSettings appSetting;

            // If there is an Email callback setup then delete it
            if (m_ioDB->getAppSetting(client->id, DB_APPSETTINGS_TYPE_MSGRECVEMAIL, &appSetting)) {
                m_ioDB->deleteAppSetting(appSetting.id);
            }
            success = true;
        }
    }

    if (success) {
        sendSuccess(response);
    } else {
        sendFailure(400, "Failed deleting email", response);
    }
}

