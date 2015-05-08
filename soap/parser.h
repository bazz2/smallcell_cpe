
#ifndef _CWMPACS_SOAP_PARSER_H
#define _CWMPACS_SOAP_PARSER_H

#include "soapH.h"

#define CWMP_VERSION_1_0    0x0001
#define CWMP_VERSION_1_1    0x0002
#define CWMP_VERSION_1_2    0x0004
#define CWMP_VERSION_1_3    0x0008
#define CWMP_VERSION_1_4    0x0010

typedef enum {
    CWMP_MSG_NONE = 0,
    CWMP_MSG_FAULT,

    CWMP_MSG_GET_RPCMETHODS_REQ,
    CWMP_MSG_GET_RPCMETHODS_RSP,

    CWMP_MSG_EMPTY_REQ,
    CWMP_MSG_EMPTY_RSP,
    CWMP_MSG_INFROM_REQ,
    CWMP_MSG_INFROM_RSP,
    CWMP_MSG_TRANSFER_COMPLETE_REQ,
    CWMP_MSG_TRANSFER_COMPLETE_RSP,
    CWMP_MSG_AUTONOMOUS_TRANSFER_COMPLETE_REQ,
    CWMP_MSG_AUTONOMOUS_TRANSFER_COMPLETE_RSP,
    CWMP_MSG_DUSTATE_CHANGE_COMPLETE_REQ,
    CWMP_MSG_DUSTATE_CHANGE_COMPLETE_RSP,
    CWMP_MSG_AUTONOMOUS_DUSTATE_CHANGE_COMPLETE_REQ,
    CWMP_MSG_AUTONOMOUS_DUSTATE_CHANGE_COMPLETE_RSP,
    CWMP_MSG_REQUEST_DOWNLOAD_REQ,
    CWMP_MSG_REQUEST_DOWNLOAD_RSP,
    CWMP_MSG_KICKED_REQ,
    CWMP_MSG_KICKED_RSP,

    CWMP_MSG_SET_PARAM_VALUES_REQ,
    CWMP_MSG_SET_PARAM_VALUES_RSP,
    CWMP_MSG_GET_PARAM_VALUES_REQ,
    CWMP_MSG_GET_PARAM_VALUES_RSP,
    CWMP_MSG_GET_PARAM_NAMES_REQ,
    CWMP_MSG_GET_PARAM_NAMES_RSP,
    CWMP_MSG_SET_PARAM_ATTRIBUTES_REQ,
    CWMP_MSG_SET_PARAM_ATTRIBUTES_RSP,
    CWMP_MSG_GET_PARAM_ATTRIBUTES_REQ,
    CWMP_MSG_GET_PARAM_ATTRIBUTES_RSP,
    CWMP_MSG_ADD_OBJECT_REQ,
    CWMP_MSG_ADD_OBJECT_RSP,
    CWMP_MSG_DELETE_OBJECT_REQ,
    CWMP_MSG_DELETE_OBJECT_RSP,
    CWMP_MSG_REBOOT_REQ,
    CWMP_MSG_REBOOT_RSP,
    CWMP_MSG_DOWNLOAD_REQ,
    CWMP_MSG_DOWNLOAD_RSP,
    CWMP_MSG_SCHEDULE_DOWNLOAD_REQ,
    CWMP_MSG_SCHEDULE_DOWNLOAD_RSP,
    CWMP_MSG_UPLOAD_REQ,
    CWMP_MSG_UPLOAD_RSP,
    CWMP_MSG_FACTORY_RESET_REQ,
    CWMP_MSG_FACTORY_RESET_RSP,
    CWMP_MSG_GET_QUEUED_TRANSFERS_REQ,
    CWMP_MSG_GET_QUEUED_TRANSFERS_RSP,
    CWMP_MSG_GET_ALL_QUEUED_TRANSFERS_REQ,
    CWMP_MSG_GET_ALL_QUEUED_TRANSFERS_RSP,
    CWMP_MSG_CANCEL_TRANSFERS_REQ,
    CWMP_MSG_CANCEL_TRANSFERS_RSP,
    CWMP_MSG_SCHEDULE_INFORM_REQ,
    CWMP_MSG_SCHEDULE_INFORM_RSP,
    CWMP_MSG_CHANGE_DUSTATE_REQ,
    CWMP_MSG_CHANGE_DUSTATE_RSP,

    CWMP_MSG_MAX,
} CACwmpMsgType;

static char _CWMP_MSG_TYPE_STRING[CWMP_MSG_MAX][100] = {
    "none",
    "Fault",

    "GetRpcMethods",
    "GetRpcMethodsResponse",

    // CPE >> ACS
    "Empty",
    "EmptyResponse",
    "Inform",
    "InformResponse",
    "TransferComplete",
    "TransferCompleteResponse",
    "AutonomousTransferComplete",
    "AutonomousTransferCompleteResponse",
    "DUStateChangeComplete",
    "DUStateChangeCompleteResponse",
    "AutonomousDUStateChangeComplete",
    "AutonomousDUStateChangeCompleteResponse",
    "RequestDownload",
    "RequestDownloadResponse",
    "Kicked",
    "KickedResponse",

    // ACS >> CPE
    "SetParameterValues",
    "SetParameterValuesResponse",
    "GetParameterValues",
    "GetParameterValuesResponse",
    "GetParameterNames",
    "GetParameterNamesResponse",
    "SetParameterAttributes",
    "SetParameterAttributesResponse",
    "GetParameterAttributes",
    "GetParameterAttributesResponse",
    "AddObject",
    "AddObjectResponse",
    "DeleteObject",
    "DeleteObjectResponse",
    "Reboot",
    "RebootResponse",
    "Download",
    "DownloadResponse",
    "ScheduleDownload",
    "ScheduleDownloadResponse",
    "Upload",
    "UploadResponse",
    "FactoryReset",
    "FactoryResetResponse",
    "GetQueuedTransfers",
    "GetQueuedTransfersResponse",
    "GetAllQueuedTransfers",
    "GetAllQueuedTransfersResponse",
    "CancelTransfer",
    "CancelTransferResponse",
    "ScheduleInform",
    "ScheduleInformResponse",
    "ChangeDUState",
    "ChangeDUStateResponse",
};
#define CWMP_MSG_TYPE_STRING(x)  _CWMP_MSG_TYPE_STRING[x]

enum {
    SOAP_PARAM_NONE = 0,

    SOAP_PARAM_NAMESPACES,
    SOAP_PARAM_VERSION,

    SOAP_PARAM_RPC_ID,
    SOAP_PARAM_RPC_HOLDREQUESTS,
    SOAP_PARAM_RPC_SESSIONTIMEOUT,
    SOAP_PARAM_RPC_SUPPORTEDCWMPVERSIONS,
    SOAP_PARAM_RPC_USECWMPVERSION,

    SOAP_PARAM_RPC_TYPE,
    SOAP_PARAM_RPC_BODY,

    SOAP_PARAM_FAULT_CODE,
    SOAP_PARAM_FAULT_STRING,
    SOAP_PARAM_FAULT_BODY,

    SOAP_PARAM_MAX,
};

void *soap_parser_init();
void soap_parser_cleanup(void *handle);

int soap_parser_decode(void *handle, const char *xml);
char* soap_parser_encode(void *handle);

void *soap_parser_get_param(void *handle, int param_id);
int soap_parser_set_param(void *handle, int param_id, void *param_value);

void *soap_namespaces_copy(void *ns);
void soap_namespaces_free(void *ns);

#endif
