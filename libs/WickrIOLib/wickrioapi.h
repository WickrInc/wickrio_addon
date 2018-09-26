#ifndef WICKRIOAPI
#define WICKRIOAPI

#define APIURL_CONTROLLER       "controller"
#define APIURL_CLIENTS          "clients"
#define APIURL_USERS            "users"
#define APIURL_APPS             "apps"
#define APIURL_MESSAGES         "messages"
#define APIURL_MSGRECVCBACK     "msgrecvcallback"
#define APIURL_MSGRECVLIB       "msgrecvlibrary"
#define APIURL_ROOMS            "rooms"
#define APIURL_GROUPCONVO       "groupconvo"
#define APIURL_MSGRECVEMAIL     "msgrecvemail"
#define APIURL_STATISTICS       "statistics"

#define APIPARAM_STATE          "state"
#define APIPARAM_START          "start"
#define APIPARAM_PAUSE          "pause"
#define APIPARAM_RESET          "reset"
#define APIPARAM_COUNT          "count"
#define APIPARAM_NAME           "name"
#define APIPARAM_USER           "user"
#define APIPARAM_PASSWORD       "password"
#define APIPARAM_APIKEY         "apikey"
#define APIPARAM_IFACE          "interface"
#define APIPARAM_PORT           "port"
#define APIPARAM_IFACETYPE      "ifacetype"
#define APIPARAM_BOTTYPE        "bottype"
#define APIPARAM_CALLBACKURL    "callbackurl"
#define APIPARAM_SERVER         "server"
#define APIPARAM_TYPE           "type"
#define APIPARAM_ACCOUNT        "account"
#define APIPARAM_SENDER         "sender"
#define APIPARAM_RECIPIENT      "recipient"
#define APIPARAM_SUBJECT        "subject"
#define APIPARAM_MAXCLIENTS     "maxclients"
#define APIPARAM_USERTYPE       "usertype"
#define APIPARAM_AUTHTYPE       "authtype"
#define APIPARAM_EMAIL          "email"

#define APIPARAM_IFACET_HTTP    "http"
#define APIPARAM_IFACET_HTTPS   "https"

#define APIPARAM_UTYPE_ADMIN    "admin"
#define APIPARAM_UTYPE_USER     "user"

#define APIPARAM_ATYPE_BASIC    "basic"
#define APIPARAM_ATYPE_EMAIL    "email"

#define APIJSON_MSGTYPE         "msgtype"
#define APIJSON_STATE           "state"
#define APIJSON_CLIENT          "client"
#define APIJSON_CLIENTS         "clients"
#define APIJSON_USER            "user"
#define APIJSON_USERS           "users"
#define APIJSON_NAME            "name"
#define APIJSON_APIKEY          "apikey"
#define APIJSON_USER            "user"
#define APIJSON_IFACE           "interface"
#define APIJSON_PORT            "port"
#define APIJSON_IFACETYPE       "ifacetype"
#define APIJSON_BOTTYPE         "bottype"
#define APIJSON_PENDINGMSGS     "pendingmsgs"
#define APIJSON_CALLBACKURL     "callbackurl"
#define APIJSON_CBACKEMAIL      "callbackemail"
#define APIJSON_SERVERSETUP     "server_setup"
#define APIJSON_SERVER          "server"
#define APIJSON_TYPE            "type"
#define APIJSON_ACCOUNT         "account"
#define APIJSON_PASSWORD        "password"
#define APIJSON_MSGSETUP        "message_setup"
#define APIJSON_SENDER          "sender"
#define APIJSON_RECIPIENT       "recipient"
#define APIJSON_SUBJECT         "subject"
#define APIJSON_MSGID           "id"
#define APIJSON_MSGTEXT         "text"
#define APIJSON_MSGSENDER       "sender"
#define APIJSON_MSGRECEIVER     "receiver"
#define APIJSON_MSGMENTIONED    "mentioned"
#define APIJSON_MSGUSERS        "users"
#define APIJSON_MSGTIME         "time"
#define APIJSON_MSG_TS          "msg_ts"
#define APIJSON_MESSAGE         "message"
#define APIJSON_MESSAGES        "messages"
#define APIJSON_ROOM            "room"
#define APIJSON_ROOMS           "rooms"
#define APIJSON_ROOMMEMBERS     "members"
#define APIJSON_ROOMMASTERS     "masters"
#define APIJSON_ROOMTITLE       "title"
#define APIJSON_ROOMTTL         "ttl"
#define APIJSON_ROOMBOR         "bor"
#define APIJSON_ROOMDESC        "description"
#define APIJSON_GROUPCONVO      "groupconvo"
#define APIJSON_GROUPCONVOS     "groupconvos"
#define APIJSON_VGROUPID        "vgroupid"
#define APIJSON_UNAME           "uname"
#define APIJSON_MAXCLIENTS      "maxclients"
#define APIJSON_USERTYPE        "usertype"
#define APIJSON_CANEDIT         "can_edit"
#define APIJSON_CANCREATE       "can_create"
#define APIJSON_RXEVENTS        "rx_events"
#define APIJSON_AUTHTYPE        "authtype"
#define APIJSON_EMAIL           "email"
#define APIJSON_STATISTICS      "statistics"
#define APIJSON_STATID_MSGSTX   "sent"
#define APIJSON_STATID_MSGSRX   "received"
#define APIJSON_STATID_OBOXSYNC "outbox_sync"
#define APIJSON_STATID_ERRSTX   "sent_errors"
#define APIJSON_STATID_ERRSRX   "recv_errors"
#define APIJSON_STATID_PENDING  "pending_messages"
#define APIJSON_STATID_PNDCBOUT "pending_callback_messages"
#define APIJSON_STATID_MSGCNT   "message_count"
#define APIJSON_STATUS_USER     "statususer"
#define APIJSON_STATUS_RUNTIME  "runtime"

#define APIJSON_KEYVER_HEADER   "keyverify"
#define APIJSON_KEYVER_MSGTYPE  "msgtype"
#define APIJSON_KEYVER_KEY      "key"
#define APIJSON_KEYVER_HASH     "hash"
#define APIJSON_KEYVER_REPLY    "reply"
#define APIJSON_KEYVER_ACCEPT   "accept"
#define APIJSON_KEYVER_REASON   "reason"
#define APIJSON_KEYVER_VERKEY   "verifiedkey"

#define APIJSON_CALL_HEADER     "call"
#define APIJSON_CALL_STATUS     "status"
#define APIJSON_CALL_MEETINGID  "meetingid"

#define APIJSON_CTRL_HEADER     "control"
#define APIJSON_CTRL_MSGTYPE    "msgtype"
#define APIJSON_CTRL_MEMBERS    "members"
#define APIJSON_CTRL_MBRUID     "uid"
#define APIJSON_CTRL_MBRUNAME   "uname"
#define APIJSON_CTRL_MBRUKEY    "ukey"
#define APIJSON_CTRL_MASTERS    "masters"
#define APIJSON_CTRL_TTL        "ttl"
#define APIJSON_CTRL_BOR        "bor"
#define APIJSON_CTRL_TITLE      "title"
#define APIJSON_CTRL_DESC       "description"
#define APIJSON_CTRL_CHGMASK    "changemask"
#define APIJSON_CTRL_ADDUSERS   "addedusers"
#define APIJSON_CTRL_DELUSERS   "deletedusers"
#define APIJSON_CTRL_MSGID      "msgid"
#define APIJSON_CTRL_ISRECALL   "isrecall"

#define APIJSON_FILE_HEADER     "file"
#define APIJSON_FILE_FILENAME   "filename"
#define APIJSON_FILE_LOCALFILE  "localfilename"
#define APIJSON_FILE_GUID       "guid"
#define APIJSON_FILE_ISSCRSHOT  "isscreenshot"

#define APIJSON_UTYPE_ADMIN     "admin"
#define APIJSON_UTYPE_USER      "user"

#define APIJSON_STATE_RUNNING   "running"
#define APIJSON_STATE_DOWN      "down"
#define APIJSON_STATE_PAUSED    "paused"
#define APIJSON_STATE_UNKNOWN   "unknown"

#define APIJSON_RESPOND_API     "respond_api"

#define APIJSON_CFG_CLIENTS         "clients"
#define APIJSON_CFG_NAME            "name"
#define APIJSON_CFG_PASSWORD        "password"
#define APIJSON_CFG_PORT            "port"              // optional
#define APIJSON_CFG_IFACETYPE       "iface_type"        // HTTPS or HTTP
#define APIJSON_CFG_APIKEY          "api_key"

#define APIJSON_CFG_SSL             "ssl"               // SSL Setup
#define APIJSON_CFG_SSLKEY          "key_filename"      // Key filename
#define APIJSON_CFG_SSLCERT         "cert_filename"     // Certificate filename


#endif // WICKRIOAPI
