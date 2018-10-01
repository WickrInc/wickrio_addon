#ifndef WICKRBOTSETTINGS
#define WICKRBOTSETTINGS

#define WBSETTINGS_DATABASE_HEADER      "database"
#define WBSETTINGS_DATABASE_DIRNAME     "dirName"

#define WBSETTINGS_ATTACH_HEADER        "attachments"
#define WBSETTINGS_ATTACH_DIRNAME       "dirName"

#define WBSETTINGS_HELP_HEADER          "help"
#define WBSETTINGS_HELP_SHOW_WELCOME    "welcome"

#define WBSETTINGS_LICENSE_HEADER       "license"
#define WBSETTINGS_LICENSE_ACCEPTED     "accepted"

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
#define WBSETTINGS_USER_TRANSACTIONID   "transactionID"
#define WBSETTINGS_USER_PRODUCTTYPE     "productType"
#define WBSETTINGS_USER_PT_LEGACY       "legacy"
#define WBSETTINGS_USER_PT_BOT          "bot"
#define WBSETTINGS_USER_PT_DEFAULT      "bot"
#define WBSETTINGS_USER_AUTOLOGIN       "autologin"

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

//Configuration information for parser ini file
#define WBSETTINGS_PARSER_RABBIT        "rabbitqueue"
#define WBSETTINGS_PARSER_ALPHA_HOST    "alphahost"
#define WBSETTINGS_PARSER_BETA_HOST     "betahost"
#define WBSETTINGS_PARSER_IF            "host"
#define WBSETTINGS_PARSER_BETA          "bet2host"
#define WBSETTINGS_PARSER_USER          "user"
#define WBSETTINGS_PARSER_PORT          "port"
#define WBSETTINGS_PARSER_PASSWORD      "password"
#define WBSETTINGS_PARSER_OLD_PASSWORD  "oldpassword"
#define WBSETTINGS_PARSER_EXCHANGE      "exchangename"
#define WBSETTINGS_PARSER_QUEUE         "queuename"
#define WBSETTINGS_PARSER_VIRTUAL_HOST  "virtualhost"

#define WBSETTINGS_PARSER_HEADER        "basicsettings"
#define WBSETTINGS_PARSER_LOGFILE       "log"
#define WBSETTINGS_PARSER_DATABASE      "dbdir"
#define WBSETTINGS_PARSER_NAME          "name"
#define WBSETTINGS_PARSER_DURATION      "duration"

// Configuration information for client(s) config file(s)
#define WIOCONFIG_CLIENTS_KEY           "clients"
#define WIOCONFIG_AUTO_LOGIN_KEY        "auto_login"
#define WIOCONFIG_INTEGRATION_KEY       "integration"

// Key values that will be written to bot integration files
#define BOTINT_CLIENT_NAME              "CLIENT_NAME"

#endif // WICKRBOTSETTINGS
