
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "soapH.h"
#include "parser.h"
#include "../log/uulog.h"

#define LOG_TAG "soap-parser"

#define SOAP_NS_CWMP_1_0    0x0001
#define SOAP_NS_CWMP_1_1    0x0002
#define SOAP_NS_CWMP_1_2    0x0004

struct Namespace namespaces[] = {
    {"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
    {"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
    {"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
    {"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
    {"cwmp", "urn:dslforum-org:cwmp-1-0", NULL, NULL},
    {"cwmp1", "urn:dslforum-org:cwmp-1-1", NULL, NULL},
    {"cwmp2", "urn:dslforum-org:cwmp-1-2", NULL, NULL},
    {NULL, NULL, NULL, NULL}
};

typedef struct {
    int code;
    char *string;
} SoapFaultDefine;

#define SOAP_FAULT_CODE_SERVER "Server"
#define SOAP_FAULT_CODE_CLIENT "Client"
#define SOAP_FAULT_STRING_CWMP "CWMP fault"

SoapFaultDefine cwmp_cpe_fault_strings[] = {
    {9000, "Method not supported"},
    {9001, "Request denied (no reason specified)"},
    {9002, "Internal error"},
    {9003, "Invalid arguments"},
    {9004, "Resources exceeded"},
    {9005, "Invalid parameter name"},
    {9006, "Invalid parameter type"},
    {9007, "Invalid parameter value"},
    {9008, "Attempt to set a non-writable parameter"},
    {9009, "Notification request rejected"},
    {9010, "File transfer failure"},
    {9011, "Upload failure"},
    {9012, "File transfer server authentication failure"},
    {9013, "Unsupported protocol for file transfer"},
    {9014, "File transfer failure: unable to join multicast group"},
    {9015, "File transfer failure: unable to contact file server"},
    {9016, "File transfer failure: unable to access file"},
    {9017, "File transfer failure: unable to complete download"},
    {9018, "File transfer failure: file corrupted"},
    {9019, "File transfer failure: file authentication failure"},
    {9020, "File transfer failure: unable to complete download within specified time windows"},
    {9021, "Cancelation of file transfer not permitted in current transfer state"},
    {9022, "Invalid UUID Format"},
    {9023, "Unknown Execution Environment"},
    {9024, "Disabled Execution Environment"},
    {9025, "Deployment Unit to Execution Environment Mismatch"},
    {9026, "Duplicate Deployment Unit"},
    {9027, "System Resources Exceeded"},
    {9028, "Unknown Deployment Unit"},
    {9029, "Invalid Deployment Unit State"},
    {9030, "Invalid Deployement Unit Update Downgrade not permitted"},
    {9031, "Invalid Deployement Unit Update Version not specified"},
    {9032, "Invalid Deployment Unit Update Version already exists"},
    {0, NULL},
};

SoapFaultDefine cwmp_acs_fault_strings[] = {
    {8000, "Method not supported"},
    {8001, "Request denied (no reason specified)"},
    {8002, "Internal error"},
    {8003, "Invalid arguments"},
    {8004, "Resources exceeded"},
    {8005, "Retry request"},
    {0, NULL},
};

typedef struct {
    struct soap *soap;

    struct Namespace *_namespaces;
    struct Namespace *namespaces;
    int version;

    int id;
    int hold_requests;
    int session_timeout;
    int supported_cwmp_versions;
    int use_cwmp_version;

    int type;
    void *body;

    int fault_code;
    char *fault_string;
    void *fault_body;
} SoapParser;

/**
 * only customize for xml string
 * @param[in] xml: the string should be processed
 * @param[in] from: the string should be replaced in xml
 * @parma[in] to: the string that replace 'from'
 */
char *_string_replace(const char *xml, const char *from, const char *to)
{
    char *dest;
    int in_angbrac = 0; /* record if in angle brackets */
    int similarity = 0;
    int xml_len, from_len, to_len, dest_size; /* length of the four */
    int xml_i, from_i, to_i, dest_i; /* index of the four */

    if (!xml || !from || !to) {
        return NULL;
    }

    xml_len = strlen(xml);
    from_len = strlen(from);
    to_len = strlen(to);
    if (xml_len < from_len) {
        return NULL;
    }

    xml_i = 0;
    from_i = 0;
    to_i = 0;
    dest_i = 0;
    dest = malloc(xml_len + 1);
    dest_size = xml_len + 1;

    for (xml_i = 0; xml_i < xml_len; xml_i++) {
        /*
         * maybe in this circulate it will occurs strcat(dest, to),
         * so it's necessary to add the value of 'to_len'
         */
        if (dest_i + to_len >= dest_size) {
            dest = realloc(dest, dest_size * 2);
            dest_size *= 2;
        }

        if (xml[xml_i] == '<') {
            in_angbrac = 1;
        } else if (xml[xml_i] == '>') {
            in_angbrac = 0;
        }

        dest[dest_i++] = xml[xml_i];
        if (!in_angbrac) {
            continue;
        }

        if (xml[xml_i] == from[from_i]) {
            similarity++;
            from_i++;
            if (similarity == from_len) { /* bingo */
                similarity = 0;
                from_i = 0;
                dest_i -= from_len;
                dest[dest_i] = 0;
                strcat(dest, to);
                dest_i += to_len;
            }
        } else {
            similarity = 0;
            from_i = 0;
        }
    }
    dest[dest_i] = 0;

    return dest;
}

static char *_slurpfd(int fd)
{
    const int bytes_at_a_time = 2;
    char *read_buffer = NULL;
    int buffer_size = 0;
    int buffer_offset = 0;
    int chars_io;

    while (1) {
        if (buffer_offset + bytes_at_a_time > buffer_size) {
            buffer_size = bytes_at_a_time + buffer_size * 2;
            read_buffer = (char *) realloc(read_buffer, buffer_size);
            if (!read_buffer) {
                perror("memory");
                exit(EXIT_FAILURE);
            }
        }

        chars_io = read(fd, read_buffer + buffer_offset, bytes_at_a_time);
        if (chars_io <= 0) {
            break;
        }

        buffer_offset += chars_io;
    }
    read_buffer[buffer_offset] = '\0';

    if (chars_io < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }

    close(fd);
    /*LOGD(LOG_TAG, read_buffer);*/
    return read_buffer; /* caller gets to free it */
}

static int _get_cwmp_type(int gsoap_type)
{
    int cmsg_type;
    switch (gsoap_type) {
        case SOAP_TYPE__cwmp__Fault:
            cmsg_type = CWMP_MSG_FAULT;
            break;
        case SOAP_TYPE__cwmp__GetRPCMethods:
            cmsg_type = CWMP_MSG_GET_RPCMETHODS_REQ;
            break;
        case SOAP_TYPE__cwmp__GetRPCMethodsResponse:
            cmsg_type = CWMP_MSG_GET_RPCMETHODS_RSP;
            break;
        case SOAP_TYPE__cwmp__SetParameterValues:
            cmsg_type = CWMP_MSG_SET_PARAM_VALUES_REQ;
            break;
        case SOAP_TYPE__cwmp__SetParameterValuesResponse:
            cmsg_type = CWMP_MSG_SET_PARAM_VALUES_RSP;
            break;
        case SOAP_TYPE__cwmp__GetParameterValues:
            cmsg_type = CWMP_MSG_GET_PARAM_VALUES_REQ;
            break;
        case SOAP_TYPE__cwmp__GetParameterValuesResponse:
            cmsg_type = CWMP_MSG_GET_PARAM_VALUES_RSP;
            break;
        case SOAP_TYPE__cwmp__GetParameterNames:
            cmsg_type = CWMP_MSG_GET_PARAM_NAMES_REQ;
            break;
        case SOAP_TYPE__cwmp__GetParameterNamesResponse:
            cmsg_type = CWMP_MSG_GET_PARAM_NAMES_RSP;
            break;
        case SOAP_TYPE__cwmp__SetParameterAttributes:
            cmsg_type = CWMP_MSG_SET_PARAM_ATTRIBUTES_REQ;
            break;
        case SOAP_TYPE__cwmp__SetParameterAttributesResponse:
            cmsg_type = CWMP_MSG_SET_PARAM_ATTRIBUTES_RSP;
            break;
        case SOAP_TYPE__cwmp__GetParameterAttributes:
            cmsg_type = CWMP_MSG_GET_PARAM_ATTRIBUTES_REQ;
            break;
        case SOAP_TYPE__cwmp__GetParameterAttributesResponse:
            cmsg_type = CWMP_MSG_GET_PARAM_ATTRIBUTES_RSP;
            break;
        case SOAP_TYPE__cwmp__AddObject:
            cmsg_type = CWMP_MSG_ADD_OBJECT_REQ;
            break;
        case SOAP_TYPE__cwmp__AddObjectResponse:
            cmsg_type = CWMP_MSG_ADD_OBJECT_RSP;
            break;
        case SOAP_TYPE__cwmp__DeleteObject:
            cmsg_type = CWMP_MSG_DELETE_OBJECT_REQ;
            break;
        case SOAP_TYPE__cwmp__DeleteObjectResponse:
            cmsg_type = CWMP_MSG_DELETE_OBJECT_RSP;
            break;
        case SOAP_TYPE__cwmp__Reboot:
            cmsg_type = CWMP_MSG_REBOOT_REQ;
            break;
        case SOAP_TYPE__cwmp__RebootResponse:
            cmsg_type = CWMP_MSG_REBOOT_RSP;
            break;
        case SOAP_TYPE__cwmp__Download:
            cmsg_type = CWMP_MSG_DOWNLOAD_REQ;
            break;
        case SOAP_TYPE__cwmp__DownloadResponse:
            cmsg_type = CWMP_MSG_DOWNLOAD_RSP;
            break;
        case SOAP_TYPE__cwmp__ScheduleDownload:
            cmsg_type = CWMP_MSG_SCHEDULE_DOWNLOAD_REQ;
            break;
        case SOAP_TYPE__cwmp__ScheduleDownloadResponse:
            cmsg_type = CWMP_MSG_SCHEDULE_DOWNLOAD_RSP;
            break;
        case SOAP_TYPE__cwmp__Upload:
            cmsg_type = CWMP_MSG_UPLOAD_REQ;
            break;
        case SOAP_TYPE__cwmp__UploadResponse:
            cmsg_type = CWMP_MSG_UPLOAD_RSP;
            break;
        case SOAP_TYPE__cwmp__FactoryReset:
            cmsg_type = CWMP_MSG_FACTORY_RESET_REQ;
            break;
        case SOAP_TYPE__cwmp__FactoryResetResponse:
            cmsg_type = CWMP_MSG_FACTORY_RESET_RSP;
            break;
        case SOAP_TYPE__cwmp__GetQueuedTransfers:
            cmsg_type = CWMP_MSG_GET_QUEUED_TRANSFERS_REQ;
            break;
        case SOAP_TYPE__cwmp__GetQueuedTransfersResponse:
            cmsg_type = CWMP_MSG_GET_QUEUED_TRANSFERS_RSP;
            break;
        case SOAP_TYPE__cwmp__GetAllQueuedTransfers:
            cmsg_type = CWMP_MSG_GET_ALL_QUEUED_TRANSFERS_REQ;
            break;
        case SOAP_TYPE__cwmp__GetAllQueuedTransfersResponse:
            cmsg_type = CWMP_MSG_GET_ALL_QUEUED_TRANSFERS_RSP;
            break;
        case SOAP_TYPE__cwmp__CancelTransfer:
            cmsg_type = CWMP_MSG_CANCEL_TRANSFERS_REQ;
            break;
        case SOAP_TYPE__cwmp__CancelTransferResponse:
            cmsg_type = CWMP_MSG_CANCEL_TRANSFERS_RSP;
            break;
        case SOAP_TYPE__cwmp__ScheduleInform:
            cmsg_type = CWMP_MSG_SCHEDULE_INFORM_REQ;
            break;
        case SOAP_TYPE__cwmp__ScheduleInformResponse:
            cmsg_type = CWMP_MSG_SCHEDULE_INFORM_RSP;
            break;
        case SOAP_TYPE__cwmp__Inform:
            cmsg_type = CWMP_MSG_INFROM_REQ;
            break;
        case SOAP_TYPE__cwmp__InformResponse:
            cmsg_type = CWMP_MSG_INFROM_RSP;
            break;
        case SOAP_TYPE__cwmp__TransferComplete:
            cmsg_type = CWMP_MSG_TRANSFER_COMPLETE_REQ;
            break;
        case SOAP_TYPE__cwmp__TransferCompleteResponse:
            cmsg_type = CWMP_MSG_TRANSFER_COMPLETE_RSP;
            break;
        case SOAP_TYPE__cwmp__AutonomousTransferComplete:
            cmsg_type = CWMP_MSG_AUTONOMOUS_TRANSFER_COMPLETE_REQ;
            break;
        case SOAP_TYPE__cwmp__AutonomousTransferCompleteResponse:
            cmsg_type = CWMP_MSG_AUTONOMOUS_TRANSFER_COMPLETE_RSP;
            break;
        case SOAP_TYPE__cwmp__DUStateChangeComplete:
            cmsg_type = CWMP_MSG_TRANSFER_COMPLETE_REQ;
            break;
        case SOAP_TYPE__cwmp__DUStateChangeCompleteResponse:
            cmsg_type = CWMP_MSG_TRANSFER_COMPLETE_RSP;
            break;
        case SOAP_TYPE__cwmp__AutonomousDUStateChangeComplete:
            cmsg_type = CWMP_MSG_AUTONOMOUS_TRANSFER_COMPLETE_REQ;
            break;
        case SOAP_TYPE__cwmp__AutonomousDUStateChangeCompleteResponse:
            cmsg_type = CWMP_MSG_AUTONOMOUS_TRANSFER_COMPLETE_RSP;
            break;
        case SOAP_TYPE__cwmp__Kicked:
            cmsg_type = CWMP_MSG_KICKED_REQ;
            break;
        case SOAP_TYPE__cwmp__KickedResponse:
            cmsg_type = CWMP_MSG_KICKED_RSP;
            break;
        case SOAP_TYPE__cwmp__RequestDownload:
            cmsg_type = CWMP_MSG_REQUEST_DOWNLOAD_REQ;
            break;
        case SOAP_TYPE__cwmp__RequestDownloadResponse:
            cmsg_type = CWMP_MSG_REQUEST_DOWNLOAD_RSP;
            break;
        default:
            LOGW(LOG_TAG, "RPC(%d): unknow", gsoap_type);
            cmsg_type = CWMP_MSG_NONE;
            break;
    }
    return cmsg_type;
}

static void _soap_namespaces_dump(struct Namespace *ns)
{
    struct Namespace *np;
    if (ns == NULL) {
        return;
    }
    for (np = ns; np->id; np++) {
        LOGD("DUMP", "%s, %s, %s, %s", np->id, np->ns, np->out, np->in);
    }
}

static int _soap_namespaces_tags(struct Namespace *ns, int version,
                                 int *num, char **src, char **dst)
{
    struct Namespace *np;
    struct Namespace *np2;
    char *cwmp_ns;

    *num = 0;

    if (version == CWMP_VERSION_1_4
        || version == CWMP_VERSION_1_3
        || version == CWMP_VERSION_1_2) {
        cwmp_ns = "urn:dslforum-org:cwmp-1-2";
    } else if (version == CWMP_VERSION_1_1) {
        cwmp_ns = "urn:dslforum-org:cwmp-1-1";
    } else if (version == CWMP_VERSION_1_0) {
        cwmp_ns = "urn:dslforum-org:cwmp-1-0";
    } else {
        cwmp_ns = "urn:dslforum-org:cwmp-1-0";
    }

    for (np = ns; np->id; np++) {
        if (strcmp(np->ns, cwmp_ns) == 0) {
            src[*num] = "cwmp";
            dst[*num] = (char *)np->id;
            *num += 1;
        } else {
            for (np2 = namespaces; np2->id; np2++) {
                if (strncmp(np2->ns, "urn:dslforum-org", 16) == 0) {
                    continue;
                }
                if (strcmp(np2->ns, np->ns) == 0
                    && strcmp(np2->id, np->id) != 0) {
                    src[*num] = (char *)np2->id;
                    dst[*num] = (char *)np->id;
                    *num += 1;
                    break;
                }
            }
        }
    }
    return 0;
}

static struct Namespace *_soap_namespaces_parse(struct soap *soap)
{
    struct Namespace *ns;
    struct soap_nlist *np;
    int num = 0;
    int i;
    int size;

    for (np = soap->nlist; np; np = np->next) {
        num ++;
    }

    size = sizeof(struct Namespace) * (num + 1);
    ns = malloc(size);
    if (ns == NULL) {
        LOGE(LOG_TAG, "out of memory");
        return NULL;
    }
    memset(ns, 0, size);

    i = num - 1;
    for (np = soap->nlist; np; np = np->next) {
        /*LOGD(LOG_TAG, "%s, %s, %d", np->id, np->ns, np->index);*/
        if (np->index >= 0) {
            struct Namespace *tmp;
            tmp = &(soap->local_namespaces[np->index]);
            if (np->id != NULL) {
                ns[i].id = strdup(np->id);
            }
            if (tmp->ns != NULL) {
                ns[i].ns = strdup(tmp->ns);
            }
            if (tmp->out != NULL) {
                ns[i].out = strdup(tmp->out);
            }
            if (tmp->in != NULL) {
                ns[i].in = strdup(tmp->in);
            }
        } else {
            if (np->id != NULL) {
                ns[i].id = strdup(np->id);
            }
            if (np->ns != NULL) {
                ns[i].ns = strdup(np->ns);
            }
            ns[i].out = NULL;
            ns[i].in = NULL;
        }

        i --;
    }

    /*_soap_namespaces_dump(ns);*/
    return ns;
}

static int _soap_version_parse(const char *xml, struct Namespace *ns)
{
    int version = CWMP_VERSION_1_0;
    struct Namespace *np;
    int max = 0;

    for (np = ns; np->id; np++) {
        int count = 0;
        int _version;
        const char *pt;
        char tmp[32];

        if (strncmp(np->ns, "urn:dslforum-org:cwmp-1-0", 25) == 0) {
            _version = CWMP_VERSION_1_0;
        } else if (strncmp(np->ns, "urn:dslforum-org:cwmp-1-1", 25) == 0) {
            _version = CWMP_VERSION_1_1;
        } else if (strncmp(np->ns, "urn:dslforum-org:cwmp-1-2", 25) == 0) {
            _version = CWMP_VERSION_1_2;
        } else {
            continue;
        }

        snprintf(tmp, sizeof(tmp), "<%s:", np->id);
        count = 0;
        pt = xml;
        while (pt = strstr(pt, tmp)) {
            count++;
            pt++;
        }
        /*LOGD(LOG_TAG, "%s: %d", np->id, count);*/

        if (count > max) {
            max = count;
            version = _version;
        }
    }

    return version;
}

static const char *_soap_version_tag(struct Namespace *ns, int version)
{
    struct Namespace *np;
    char *tmp;

    if (version == CWMP_VERSION_1_4
        || version == CWMP_VERSION_1_3
        || version == CWMP_VERSION_1_2) {
        tmp = "urn:dslforum-org:cwmp-1-2";
    } else if (version == CWMP_VERSION_1_1) {
        tmp = "urn:dslforum-org:cwmp-1-1";
    } else if (version == CWMP_VERSION_1_0) {
        tmp = "urn:dslforum-org:cwmp-1-0";
    } else {
        tmp = "urn:dslforum-org:cwmp-1-0";
    }

    for (np = ns; np->id; np++) {
        if (strncmp(np->ns, tmp, strlen(tmp)) == 0) {
            return np->id;
        }
    }

    return NULL;
}

static void _soap_header_dump(struct SOAP_ENV__Header *header)
{
    if (header != NULL) {
        if (header->ID != NULL) {
            LOGD("DUMP", "soap header: ID=%s(%s)",
                 header->ID->__item,
                 header->ID->SOAP_ENV__mustUnderstand);
        }
        if (header->HoldRequests != NULL) {
            LOGD("DUMP", "soap header: HoldRequests=%s(%s)",
                 header->HoldRequests->__item,
                 header->HoldRequests->SOAP_ENV__mustUnderstand);
        }
    }
}

static void _soap_header_destroy(void *header)
{
    struct SOAP_ENV__Header *hdr = (struct SOAP_ENV__Header *)header;
    if (hdr == NULL) {
        return;
    }
    free(hdr);
}

static void *_soap_header_create(int id, int hold_requests,
                                 int session_timeout, int supported_versions, int use_version)
{
    int size, offset;
    char *buf;
    struct SOAP_ENV__Header *hdr = NULL;

    size = sizeof(struct SOAP_ENV__Header)
           + sizeof(struct _cwmp__ID) + 128
           + sizeof(struct _cwmp__HoldRequests)
           + sizeof(struct _cwmp__SessionTimeout)
           + sizeof(struct _cwmp__SupportedCWMPVersions) + 128
           + sizeof(struct _cwmp__UseCWMPVersion) + 128;
    buf = malloc(size);
    if (buf == NULL) {
        LOGE(LOG_TAG, "out of memory");
        return NULL;
    }
    memset(buf, 0, size);

    offset = 0;
    hdr = (struct SOAP_ENV__Header *)(buf + offset);
    offset += sizeof(struct SOAP_ENV__Header);
    hdr->ID = (struct _cwmp__ID *)(buf + offset);
    offset += sizeof(struct _cwmp__ID);
    hdr->ID->__item = buf + offset;
    offset += 128;
    hdr->HoldRequests = (struct _cwmp__HoldRequests *)(buf + offset);
    offset += sizeof(struct _cwmp__HoldRequests);
    hdr->SessionTimeout = (struct _cwmp__SessionTimeout *)(buf + offset);
    offset += sizeof(struct _cwmp__SessionTimeout);
    hdr->SupportedCWMPVersions = (struct _cwmp__SupportedCWMPVersions *)(buf + offset);
    offset += sizeof(struct _cwmp__SupportedCWMPVersions);
    hdr->SupportedCWMPVersions->__item = buf + offset;
    offset += 128;
    hdr->UseCWMPVersion = (struct _cwmp__UseCWMPVersion *)(buf + offset);
    offset += sizeof(struct _cwmp__UseCWMPVersion);
    hdr->UseCWMPVersion->__item = buf + offset;
    offset += 128;

    if (id != 0) {
        snprintf(hdr->ID->__item, 128, "%d", id);
        hdr->ID->SOAP_ENV__mustUnderstand = "1";
    } else {
        hdr->ID = NULL;
    }

    // only sent from ACS to CPE
    if (hold_requests != 0) {
        hdr->HoldRequests->__item = 1;
        hdr->HoldRequests->SOAP_ENV__mustUnderstand = "1";
    } else {
        hdr->HoldRequests = NULL;
    }

    // only sent from CPE to ACS
    if (session_timeout > 0) {
        hdr->SessionTimeout->__item = session_timeout;
        hdr->SessionTimeout->SOAP_ENV__mustUnderstand = "0";
    } else {
        hdr->SessionTimeout = NULL;
    }

    // only sent from CPE to ACS
    if (use_version != 0 && supported_versions != 0 && (use_version & supported_versions)) {
        int i;
        int _offset = 0;
        for (i = 0; i < 5; i++) {
            if ((supported_versions & (1 << i)) == 0) {
                continue;
            }
            if (_offset == 0) {
                snprintf(hdr->SupportedCWMPVersions->__item + _offset, 128 - _offset,
                         "1.%d", i);
                _offset += 3;
            } else {
                snprintf(hdr->SupportedCWMPVersions->__item + _offset, 128 - _offset,
                         ",1.%d", i);
                _offset += 4;
            }
            if (use_version & (1 << i)) {
                snprintf(hdr->UseCWMPVersion->__item, 128, "1.%d", i);
            }
        }
        hdr->SupportedCWMPVersions->SOAP_ENV__mustUnderstand = "0";// must be set to "0"
        hdr->UseCWMPVersion->SOAP_ENV__mustUnderstand = "1"; // must be set to "1"
    } else {
        hdr->UseCWMPVersion = NULL;
        hdr->SupportedCWMPVersions = NULL;
    }

    return (void *)hdr;
}

static int _soap_header_parse(struct SOAP_ENV__Header *header,
                              int *id, int *hold_requests,
                              int *session_timeout, int *supported_versions, int *use_version)
{
    *id = 0;
    *hold_requests = 0;
    *session_timeout = 0;
    *supported_versions = 0;
    *use_version = 0;

    if (header == NULL) {
        return -1;
    }

    if (header->HoldRequests != NULL) {
        *hold_requests = header->HoldRequests->__item;
    }
    if (header->ID != NULL) {
        if (header->ID->__item != NULL) {
            *id = atoi(header->ID->__item);
        }
    }
    if (header->SessionTimeout != NULL) {
        *session_timeout = header->SessionTimeout->__item;
    }
    if (header->SupportedCWMPVersions != NULL) {
        if (header->SupportedCWMPVersions->__item != NULL) {
            char *p;
            p = strstr(header->SupportedCWMPVersions->__item, "1.0");
            if (p != NULL && (p[3] == ',' || p[3] == '\0')) {
                *supported_versions = *supported_versions | CWMP_VERSION_1_0;
            }
            p = strstr(header->SupportedCWMPVersions->__item, "1.1");
            if (p != NULL && (p[3] == ',' || p[3] == '\0')) {
                *supported_versions = *supported_versions | CWMP_VERSION_1_1;
            }
            p = strstr(header->SupportedCWMPVersions->__item, "1.2");
            if (p != NULL && (p[3] == ',' || p[3] == '\0')) {
                *supported_versions = *supported_versions | CWMP_VERSION_1_2;
            }
            p = strstr(header->SupportedCWMPVersions->__item, "1.3");
            if (p != NULL && (p[3] == ',' || p[3] == '\0')) {
                *supported_versions = *supported_versions | CWMP_VERSION_1_3;
            }
            p = strstr(header->SupportedCWMPVersions->__item, "1.4");
            if (p != NULL && (p[3] == ',' || p[3] == '\0')) {
                *supported_versions = *supported_versions | CWMP_VERSION_1_4;
            }
        }
    }
    if (header->UseCWMPVersion != NULL) {
        if (header->UseCWMPVersion->__item != NULL) {
            if (strncmp(header->UseCWMPVersion->__item, "1.0", 3) == 0) {
                *use_version = CWMP_VERSION_1_0;
            } else if (strncmp(header->UseCWMPVersion->__item, "1.1", 3) == 0) {
                *use_version = CWMP_VERSION_1_1;
            } else if (strncmp(header->UseCWMPVersion->__item, "1.2", 3) == 0) {
                *use_version = CWMP_VERSION_1_2;
            } else if (strncmp(header->UseCWMPVersion->__item, "1.3", 3) == 0) {
                *use_version = CWMP_VERSION_1_3;
            } else if (strncmp(header->UseCWMPVersion->__item, "1.4", 3) == 0) {
                *use_version = CWMP_VERSION_1_4;
            }
        }
    }
    return 0;
}

static void _soap_fault_dump(struct SOAP_ENV__Fault *fault)
{
    if (fault != NULL) {
        LOGD("DUMP", "soap fault: FaultCode=%s", fault->faultcode);
        LOGD("DUMP", "soap fault: FaultString=%s", fault->faultstring);
        LOGD("DUMP", "soap fault: Detail=%d", fault->detail->__type);
        if (fault->detail != NULL
            && fault->detail->__type == SOAP_TYPE__cwmp__Fault
            && fault->detail->fault != NULL) {
            struct _cwmp__Fault *cwmp_fault;
            cwmp_fault = (struct _cwmp__Fault *)fault->detail->fault;
            if (cwmp_fault != NULL) {
                LOGD("DUMP", "cwmp fault: FaultCode=%s", cwmp_fault->FaultCode);
                LOGD("DUMP", "cwmp fault: FaultString=%s", cwmp_fault->FaultString);
            }
        }
    }
}

static void _soap_fault_destroy(struct SOAP_ENV__Fault *fault)
{
    if (fault != NULL) {
        free(fault);
    }
}

static struct SOAP_ENV__Fault *_soap_fault_create(int fault_code, char *fault_string, void *fault_body)
{
    struct SOAP_ENV__Fault *fault = NULL;
    struct _cwmp__Fault *cwmp_fault = NULL;
    char *buf = NULL;
    int size = 0;
    int offset = 0;

    if (fault_string == NULL) {
        SoapFaultDefine *fdef;
        if (fault_code >= 9000 && fault_code <= 9799) {
            for (fdef = cwmp_cpe_fault_strings; fdef; fdef++) {
                if (fdef->code == 0) {
                    break;
                }
                if (fdef->code == fault_code) {
                    fault_string = fdef->string;
                    break;
                }
            }
        } else if (fault_code >= 8000 && fault_code <= 8799) {
            for (fdef = cwmp_acs_fault_strings; fdef; fdef++) {
                if (fdef->code == 0) {
                    break;
                }
                if (fdef->code == fault_code) {
                    fault_string = fdef->string;
                    break;
                }
            }
        }
    }

    size = sizeof(struct SOAP_ENV__Fault)
           + sizeof(struct SOAP_ENV__Detail)
           + sizeof(struct _cwmp__Fault) + 32 + 256;
    buf = malloc(size);
    if (buf == NULL) {
        LOGE(LOG_TAG, "out of memory");
        return NULL;
    }
    memset(buf, 0, size);

    offset = 0;
    fault = (struct SOAP_ENV__Fault *)(buf + offset);
    offset += sizeof(struct SOAP_ENV__Fault);
    fault->detail = (struct SOAP_ENV__Detail *)(buf + offset);
    offset += sizeof(struct SOAP_ENV__Detail);
    cwmp_fault = (struct _cwmp__Fault *)(buf + offset);
    offset += sizeof(struct _cwmp__Fault);
    cwmp_fault->FaultCode = buf + offset;
    offset += 32;
    cwmp_fault->FaultString = buf + offset;
    offset += 256;

    if (fault_code >= 9000 && fault_code < 10000) {
        fault->faultcode = SOAP_FAULT_CODE_SERVER;
    } else if (fault_code >= 8000 && fault_code < 9000) {
        fault->faultcode = SOAP_FAULT_CODE_CLIENT;
    }
    fault->faultstring = SOAP_FAULT_STRING_CWMP;
    fault->detail->__type = SOAP_TYPE__cwmp__Fault;
    fault->detail->fault = (void *)cwmp_fault;

    snprintf(cwmp_fault->FaultCode, 31, "%d", fault_code);
    snprintf(cwmp_fault->FaultString, 255, "%s", fault_string);

    // TODO: deal with fault_body
    cwmp_fault->__sizeSetParameterValuesFault = 0;
    cwmp_fault->SetParameterValuesFault = NULL;

    return fault;
}

void soap_namespaces_free(void *ns)
{
    struct Namespace *ns1 = ns;
    struct Namespace *np = NULL;

    if (ns1 == NULL) {
        return;
    }

    for (np = ns1; np->id; np++) {
        if (np->id != NULL) {
            free((char *)np->id);
        }
        if (np->ns != NULL) {
            free((char *)np->ns);
        }
        if (np->out != NULL) {
            free((char *)np->out);
        }
        if (np->in != NULL) {
            free((char *)np->in);
        }
    }

    free(ns1);
}

void *soap_namespaces_copy(void *ns)
{
    struct Namespace *ns1 = ns;
    struct Namespace *ns2 = NULL;
    struct Namespace *np = NULL;
    int num = 1;
    int size = 0;

    int i;

    if (ns == NULL) {
        return NULL;
    }

    for (np = ns1; np->id; np++) {
        num ++;
    }
    size = sizeof(struct Namespace) * num;

    ns2 = malloc(size);
    if (ns2 == NULL) {
        LOGE(LOG_TAG, "out of memory");
        return NULL;
    }
    memset(ns2, 0, size);

    i = 0;
    for (np = ns1; np->id; np++) {
        ns2[i].id = strdup(np->id);
        if (np->ns != NULL) {
            ns2[i].ns = strdup(np->ns);
        }
        if (np->out != NULL) {
            ns2[i].out = strdup(np->out);
        }
        if (np->in != NULL) {
            ns2[i].in = strdup(np->in);
        }
        i++;
    }

    return ns2;
}

void soap_parser_cleanup(void *handle)
{
    SoapParser *parser = (SoapParser *)handle;
    if (parser == NULL) {
        return;
    }
    if (parser->_namespaces != NULL) {
        soap_namespaces_free(parser->_namespaces);
        parser->_namespaces = NULL;
    }
    if (parser->soap != NULL) {
        soap_end(parser->soap);
        soap_done(parser->soap);
        soap_free(parser->soap);
        parser->soap = NULL;
    }

    free(parser);
    LOGI(LOG_TAG, "cleanup");
}

void *soap_parser_init()
{
    SoapParser *parser;

    parser = (SoapParser *)malloc(sizeof(SoapParser));
    if (NULL == parser) {
        LOGE(LOG_TAG, "out of memory");
        return NULL;
    }
    memset(parser, 0, sizeof(SoapParser));

    parser->soap = soap_new();
    if (NULL == parser->soap) {
        LOGE(LOG_TAG, "create soap failed");
        soap_parser_cleanup(parser);
        return NULL;
    }
    LOGI(LOG_TAG, "init");
    return parser;
}

void *soap_parser_get_param(void *handle, int param_id)
{
    SoapParser *parser = (SoapParser *)handle;
    void *value = NULL;
    if (parser == NULL) {
        return NULL;
    }
    switch (param_id) {
        case SOAP_PARAM_NAMESPACES:
            value = (void *)parser->namespaces;
            break;
        case SOAP_PARAM_VERSION:
            value = (void *)parser->version;
            break;
        case SOAP_PARAM_RPC_ID:
            value = (void *)parser->id;
            break;
        case SOAP_PARAM_RPC_HOLDREQUESTS:
            value = (void *)parser->hold_requests;
            break;
        case SOAP_PARAM_RPC_SESSIONTIMEOUT:
            value = (void *)parser->session_timeout;
            break;
        case SOAP_PARAM_RPC_SUPPORTEDCWMPVERSIONS:
            value = (void *)parser->supported_cwmp_versions;
            break;
        case SOAP_PARAM_RPC_USECWMPVERSION:
            value = (void *)parser->use_cwmp_version;
            break;
        case SOAP_PARAM_RPC_TYPE:
            value = (void *)parser->type;
            break;
        case SOAP_PARAM_RPC_BODY:
            value = (void *)parser->body;
            break;
        case SOAP_PARAM_FAULT_CODE:
            value = (void *)parser->fault_code;
            break;
        case SOAP_PARAM_FAULT_STRING:
            value = (void *)parser->fault_string;
            break;
        case SOAP_PARAM_FAULT_BODY:
            value = (void *)parser->fault_body;
            break;
        default: {
            LOGE(LOG_TAG, "unknow param id: %d", param_id);
            value = NULL;
            break;
        }
    }
    return value;
}

int soap_parser_set_param(void *handle, int param_id, void *param_value)
{
    SoapParser *parser = (SoapParser *)handle;
    if (parser == NULL) {
        return -1;
    }

    switch (param_id) {
        case SOAP_PARAM_NAMESPACES:
            parser->namespaces = (struct Namespace *)param_value;
            break;
        case SOAP_PARAM_VERSION:
            parser->version = (int)param_value;
            break;
        case SOAP_PARAM_RPC_ID:
            parser->id = (int)param_value;
            break;
        case SOAP_PARAM_RPC_HOLDREQUESTS:
            parser->hold_requests = (int)param_value;
            break;
        case SOAP_PARAM_RPC_SESSIONTIMEOUT:
            parser->session_timeout = (int)param_value;
            break;
        case SOAP_PARAM_RPC_SUPPORTEDCWMPVERSIONS:
            parser->supported_cwmp_versions = (int)param_value;
            break;
        case SOAP_PARAM_RPC_USECWMPVERSION:
            parser->use_cwmp_version = (int)param_value;
            break;
        case SOAP_PARAM_RPC_TYPE:
            parser->type = (int)param_value;
            break;
        case SOAP_PARAM_RPC_BODY:
            parser->body = param_value;
            break;
        case SOAP_PARAM_FAULT_CODE:
            parser->fault_code = (int)param_value;
            break;
        case SOAP_PARAM_FAULT_STRING:
            parser->fault_string = (char *)param_value;
            break;
        case SOAP_PARAM_FAULT_BODY:
            parser->fault_body = (void *)param_value;
            break;
        default: {
            LOGE(LOG_TAG, "unknow param id: %d", param_id);
            return -1;
        }
    }
    return 0;
}

int soap_parser_decode(void *handle, const char *xml)
{
    SoapParser *parser = (SoapParser *)handle;
    struct soap *soap;
    int fd[2];

    if (parser == NULL) {
        return -1;
    }
    if (xml == NULL) {
        LOGE(LOG_TAG, "input xml is empty");
        return -1;
    }

    soap = parser->soap;
    pipe(fd);

    /*soap_init(soap);*/
    soap_init1(soap, SOAP_XML_IGNORENS);
    soap_set_namespaces(soap, namespaces);

    soap->socket = -1;
    soap->recvfd = fd[0];
    write(fd[1], xml, strlen(xml));
    close(fd[1]);

    if (soap_begin_recv(soap) != 0
        || soap_envelope_begin_in(soap) != 0) {
        LOGE(LOG_TAG, "begin decode failed, error=%d", soap->error);
        return -1;
    }

    if (soap_recv_header(soap) != 0) {
        LOGE(LOG_TAG, "decode soap header failed, %d", soap->error);
        return -1;
    }
    _soap_header_parse(soap->header,
                       &(parser->id),
                       &(parser->hold_requests),
                       &(parser->session_timeout),
                       &(parser->supported_cwmp_versions),
                       &(parser->use_cwmp_version));

    if (soap_body_begin_in(soap) != 0) {
        LOGE(LOG_TAG, "decode soap body failed, %d", soap->error);
        return -1;
    }

    parser->_namespaces = _soap_namespaces_parse(soap);
    parser->namespaces = parser->_namespaces;
    parser->version = _soap_version_parse(xml, parser->namespaces);
    if (parser->version == CWMP_VERSION_1_2) {
        if (parser->supported_cwmp_versions != 0 && parser->use_cwmp_version != 0) {
            parser->version = CWMP_VERSION_1_4;
        } else if (parser->session_timeout > 0) {
            parser->version = CWMP_VERSION_1_3;
        }
    }

    if (soap_recv_fault(soap, 1) != 0) {
        parser->type = CWMP_MSG_FAULT;
        _soap_fault_dump(soap->fault);
        if (soap->fault != NULL
            && soap->fault->detail != NULL
            && soap->fault->detail->__type == SOAP_TYPE__cwmp__Fault) {
            struct _cwmp__Fault *cwmp_fault;
            cwmp_fault = (struct _cwmp__Fault *)soap->fault->detail->fault;
            if (cwmp_fault != NULL) {
                if (cwmp_fault->FaultCode != NULL) {
                    parser->fault_code = atoi(cwmp_fault->FaultCode);
                }
                parser->fault_string = cwmp_fault->FaultString;
                parser->fault_body = cwmp_fault;
                parser->body = cwmp_fault;
            }
        }
        return 0;
    }

    int gsoap_type;
    parser->body = soap_getelement(soap, &gsoap_type);
    parser->type = _get_cwmp_type(gsoap_type);
    if (soap->error) {
        LOGE(LOG_TAG, "get cwmp body failed, error=%d", soap->error);
        return -1;
    }

    if (soap_body_end_in(soap)
        || soap_envelope_end_in(soap)
        || soap_end_recv(soap)) {
        LOGE(LOG_TAG, "end decode failed, %d", soap->error);
        return -1;
    }
    close(fd[0]);

    return 0;
}

char *soap_parser_encode(void *handle)
{
    SoapParser *parser = (SoapParser *)handle;

    struct soap *soap;
    int fd[2];
    char *xml = NULL;

    struct SOAP_ENV__Header *header = NULL;
    struct SOAP_ENV__Fault *fault = NULL;

    int tags_num = 0;
    char *src_tags[32];
    char *dst_tags[32];

    if (parser == NULL) {
        return NULL;
    }

    soap = parser->soap;

    header = _soap_header_create(parser->id,
                                 parser->hold_requests,
                                 parser->session_timeout,
                                 parser->supported_cwmp_versions,
                                 parser->use_cwmp_version);
    if (header == NULL) {
        LOGE(LOG_TAG, "create soap header failed");
        return NULL;
    }

    pipe(fd);

    soap_init(soap);
    soap->socket = -1;
    soap->sendfd = fd[1];

    if (parser->namespaces != NULL) {
        soap_set_namespaces(soap, parser->namespaces);
        if (parser->use_cwmp_version != 0) {
            _soap_namespaces_tags(parser->namespaces, parser->use_cwmp_version,
                                  &tags_num, src_tags, dst_tags);
        } else {
            _soap_namespaces_tags(parser->namespaces, parser->version,
                                  &tags_num, src_tags, dst_tags);
        }
    } else {
        soap_set_namespaces(soap, namespaces);
    }
    soap_set_version(soap, 1);

    soap->header = header;

    if (soap_begin_send(soap) != 0
        || soap_envelope_begin_out(soap) != 0
        || soap_putheader(soap) != 0
        || soap_body_begin_out(soap) != 0) {
        LOGE(LOG_TAG, "begin encode failed, error=%d", soap->error);
        _soap_header_destroy(header);
        return NULL;
    }

    switch (parser->type) {
        case  CWMP_MSG_FAULT: {
            fault = _soap_fault_create(parser->fault_code, parser->fault_string, parser->fault_body);
            if (fault == NULL) {
                LOGE(LOG_TAG, "create soap fault failed");
                _soap_header_destroy(header);
                return NULL;
            }
            soap_put_SOAP_ENV__Fault(soap, fault, NULL, NULL);
            break;
        }
        case CWMP_MSG_GET_RPCMETHODS_REQ:
            soap_put__cwmp__GetRPCMethods(soap,
                                          (struct _cwmp__GetRPCMethods *)parser->body,
                                          "cwmp:GetRPCMethods", NULL);
            break;
        case CWMP_MSG_SET_PARAM_VALUES_REQ:
            soap_put__cwmp__SetParameterValues(soap,
                                               (struct _cwmp__SetParameterValues *)parser->body,
                                               "cwmp:SetParameterValues", NULL);
            break;
        case CWMP_MSG_GET_PARAM_VALUES_REQ:
            soap_put__cwmp__GetParameterValues(soap,
                                               (struct _cwmp__GetParameterValues *)parser->body,
                                               "cwmp:GetParameterValues", NULL);
            break;
        case CWMP_MSG_GET_PARAM_NAMES_REQ:
            soap_put__cwmp__GetParameterNames(soap,
                                              (struct _cwmp__GetParameterNames *)parser->body,
                                              "cwmp:GetParameterNames", NULL);
            break;
        case CWMP_MSG_SET_PARAM_ATTRIBUTES_REQ:
            soap_put__cwmp__SetParameterAttributes(soap,
                                                   (struct _cwmp__SetParameterAttributes *)parser->body,
                                                   "cwmp:SetParameterAttributes", NULL);
            break;
        case CWMP_MSG_GET_PARAM_ATTRIBUTES_REQ:
            soap_put__cwmp__GetParameterAttributes(soap,
                                                   (struct _cwmp__GetParameterAttributes *)parser->body,
                                                   "cwmp:GetParameterAttributes", NULL);
            break;
        case CWMP_MSG_ADD_OBJECT_REQ:
            soap_put__cwmp__AddObject(soap,
                                      (struct _cwmp__AddObject *)parser->body,
                                      "cwmp:AddObject", NULL);
            break;
        case CWMP_MSG_DELETE_OBJECT_REQ:
            soap_put__cwmp__DeleteObject(soap,
                                         (struct _cwmp__DeleteObject *)parser->body,
                                         "cwmp:DeleteObject", NULL);
            break;
        case CWMP_MSG_REBOOT_REQ:
            soap_put__cwmp__Reboot(soap,
                                   (struct _cwmp__Reboot *)parser->body,
                                   "cwmp:Reboot", NULL);
            break;
        case CWMP_MSG_DOWNLOAD_REQ:
            soap_put__cwmp__Download(soap,
                                     (struct _cwmp__Download *)parser->body,
                                     "cwmp:Download", NULL);
            break;
        case CWMP_MSG_SCHEDULE_DOWNLOAD_REQ:
            soap_put__cwmp__ScheduleDownload(soap,
                                             (struct _cwmp__ScheduleDownload *)parser->body,
                                             "cwmp:ScheduleDownload", NULL);
            break;
        case CWMP_MSG_UPLOAD_REQ:
            soap_put__cwmp__Upload(soap,
                                   (struct _cwmp__Upload *)parser->body,
                                   "cwmp:Upload", NULL);
            break;
        case CWMP_MSG_FACTORY_RESET_REQ:
            soap_put__cwmp__FactoryReset(soap,
                                         (struct _cwmp__FactoryReset *)parser->body,
                                         "cwmp:FactoryReset", NULL);
            break;
        case CWMP_MSG_GET_QUEUED_TRANSFERS_REQ:
            soap_put__cwmp__GetQueuedTransfers(soap,
                                               (struct _cwmp__GetQueuedTransfers *)parser->body,
                                               "cwmp:GetQueuedTransfers", NULL);
            break;
        case CWMP_MSG_GET_ALL_QUEUED_TRANSFERS_REQ:
            soap_put__cwmp__GetAllQueuedTransfers(soap,
                                                  (struct _cwmp__GetAllQueuedTransfers *)parser->body,
                                                  "cwmp:GetAllQueuedTransfers", NULL);
            break;
        case CWMP_MSG_CANCEL_TRANSFERS_REQ:
            soap_put__cwmp__CancelTransfer(soap,
                                           (struct _cwmp__CancelTransfer *)parser->body,
                                           "cwmp:CancelTransfer", NULL);
            break;
        case CWMP_MSG_SCHEDULE_INFORM_REQ:
            soap_put__cwmp__ScheduleInform(soap,
                                           (struct _cwmp__ScheduleInform *)parser->body,
                                           "cwmp:ScheduleInform", NULL);
            break;
        case CWMP_MSG_CHANGE_DUSTATE_REQ:
            soap_put__cwmp__ChangeDUState(soap,
                                          (struct _cwmp__ChangeDUState *)parser->body,
                                          "cwmp:ChangeDUState", NULL);
            break;
        case CWMP_MSG_GET_RPCMETHODS_RSP:
            soap_put__cwmp__GetRPCMethodsResponse(soap,
                                                  (struct _cwmp__GetRPCMethodsResponse *)parser->body,
                                                  "cwmp:GetRPCMethodsResponse", NULL);
            break;
        case CWMP_MSG_INFROM_RSP:
            soap_put__cwmp__InformResponse(soap,
                                           (struct _cwmp__InformResponse *)parser->body,
                                           "cwmp:InformResponse", NULL);
            break;
        case CWMP_MSG_TRANSFER_COMPLETE_RSP:
            soap_put__cwmp__TransferCompleteResponse(soap,
                                                     (struct _cwmp__TransferCompleteResponse *)parser->body,
                                                     "cwmp:TransferCompleteResponse", NULL);
            break;
        case CWMP_MSG_AUTONOMOUS_TRANSFER_COMPLETE_RSP:
            soap_put__cwmp__AutonomousTransferCompleteResponse(soap,
                                                               (struct _cwmp__AutonomousTransferCompleteResponse *)parser->body,
                                                               "cwmp:AutonomousTransferCompleteResponse", NULL); break;
        case CWMP_MSG_DUSTATE_CHANGE_COMPLETE_RSP:
            soap_put__cwmp__DUStateChangeCompleteResponse(soap,
                                                          (struct _cwmp__DUStateChangeCompleteResponse *)parser->body,
                                                          "cwmp:DUStateChangeCompleteResponse", NULL);
            break;
        case CWMP_MSG_AUTONOMOUS_DUSTATE_CHANGE_COMPLETE_RSP:
            soap_put__cwmp__AutonomousDUStateChangeCompleteResponse(soap,
                                                                    (struct _cwmp__AutonomousDUStateChangeCompleteResponse *)parser->body,
                                                                    "cwmp:AutonomousDUStateChangeCompleteResponse", NULL);
            break;
        case CWMP_MSG_REQUEST_DOWNLOAD_RSP:
            soap_put__cwmp__RequestDownloadResponse(soap,
                                                    (struct _cwmp__RequestDownloadResponse *)parser->body,
                                                    "cwmp:RequestDownloadResponse", NULL);
            break;
        default :
            LOGE(LOG_TAG, "unknow type, %s(%d)", CWMP_MSG_TYPE_STRING(parser->type), parser->type);
            break;
    }

    if (soap_body_end_out(soap) != 0
        || soap_envelope_end_out(soap) != 0
        || soap_end_send(soap) != 0) {
        LOGE(LOG_TAG, "end encode failed, error=%d", soap->error);
        _soap_header_destroy(header);
        return NULL;
    }

    close(fd[1]);
    xml = _slurpfd(fd[0]);
    if (tags_num > 0) {
        int i;
        for (i = 0; i < tags_num; i++) {
            char s[128];
            char d[128];
            char *_xml = xml;
            if (src_tags[i] == NULL
                || dst_tags[i] == NULL
                || strlen(src_tags[i]) <= 0
                || strlen(dst_tags[i]) <= 0) {
                continue;
            }
            LOGV(LOG_TAG, "replace \"%s\" to \"%s\"", src_tags[i], dst_tags[i]);
            snprintf(s, 128, "%s:", src_tags[i]);
            snprintf(d, 128, "%s:", dst_tags[i]);
            xml = _string_replace(_xml, s, d);
            if (xml == NULL) {
                LOGE(LOG_TAG, "replace tag failed.");
                return _xml;
            }
            free(_xml);
        }
    }

    _soap_header_destroy(header);
    _soap_fault_destroy(fault);
    return xml;
}
