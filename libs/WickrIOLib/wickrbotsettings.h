#ifndef WICKRBOTSETTINGS
#define WICKRBOTSETTINGS

#define WBSETTINGS_DATABASE_HEADER      "database"
#define WBSETTINGS_DATABASE_DIRNAME     "dirName"

#define WBSETTINGS_ATTACH_HEADER        "attachments"
#define WBSETTINGS_ATTACH_DIRNAME       "dirName"

#define WBSETTINGS_LISTENER_HEADER      "listener"
#define WBSETTINGS_LISTENER_PORT        "port"
#define WBSETTINGS_LISTENER_IF          "host"
#define WBSETTINGS_LISTENER_SSLKEY      "sslKeyFile"
#define WBSETTINGS_LISTENER_SSLCERT     "sslCertFile"

#define WBSETTINGS_LOGGING_HEADER       "logging"
#define WBSETTINGS_LOGGING_FILENAME     "fileName"
#define WBSETTINGS_LOGGING_OUTPUT_FILENAME "outFileName"

#define WBSETTINGS_USER_HEADER          "user"
#define WBSETTINGS_USER_USER            "user"
#define WBSETTINGS_USER_PASSWORD        "password"
#define WBSETTINGS_USER_USERNAME        "username"

#define WBSETTINGS_CONFIG_HEADER        "configuration"
#define WBSETTINGS_CONFIG_DORECEIVE     "doreceive"
#define WBSETTINGS_CONFIG_SERVER        "servername"

#define WBSETTINGS_CONSOLESVR_HEADER    "console"
#define WBSETTINGS_CONSOLESVR_TYPE      "type"
#define WBSETTINGS_CONSOLESVR_PORT      "port"
#define WBSETTINGS_CONSOLESVR_IF        "host"
#define WBSETTINGS_CONSOLESVR_SSLKEY    "sslKeyFile"
#define WBSETTINGS_CONSOLESVR_SSLCERT   "sslCertFile"

#define WBSETTINGS_EMAIL_HEADER         "email"
#define WBSETTINGS_EMAIL_SERVER         "server"
#define WBSETTINGS_EMAIL_PORT           "port"
#define WBSETTINGS_EMAIL_TYPE           "type"
#define WBSETTINGS_EMAIL_ACCOUNT        "account"
#define WBSETTINGS_EMAIL_PASSWORD       "password"
#define WBSETTINGS_EMAIL_SENDER         "sender"
#define WBSETTINGS_EMAIL_RECIPIENT      "recipient"
#define WBSETTINGS_EMAIL_SUBJECT        "subject"

#define WBSETTINGS_SSL_HEADER           "sslSetup"
#define WBSETTINGS_SSL_KEYFILE          "sslKeyFile"
#define WBSETTINGS_SSL_CERTFILE         "sslCertFile"

#define WBSETTINGS_SERVICES_HEADER      "services"              // Settings for bot services
#define WBSETTINGS_SERVICES_HANDLEINBOX "handleinbox"           // True if handling inbox messages, otherwise drop
#define WBSETTINGS_SERVICES_DURATION    "duration"              // How long to run the client for, before restarting

#endif // WICKRBOTSETTINGS
