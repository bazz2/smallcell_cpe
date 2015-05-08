#include <stdio.h>

#include "rpc.h"
#include "log/uulog.h"
#include "soap/parser.h"

void free_cpe_rpc_msg(CpeRpcMsg *msg)
{
    if (!msg)
        return;

    if (msg->body)
        free(msg->body);
    if (msg->parser)
        soap_parser_cleanup(msg->parser);

    free(msg);
}

/**
 * @param[in] arg: TODO: it should be event_para;
 */
CpeRpcMsg *rpc_inform_create(void *arg)
{
    CpeRpcMsg *ifm_msg;
    struct _cwmp__Inform *req;

    ifm_msg = calloc(1, sizeof (CpeRpcMsg));
    if (!ifm_msg) {
        LOGE("rpc", "Failed to allocate memory");
        return NULL;
    }

    ifm_msg->id = 0;
    ifm_msg->type = CWMP_MSG_INFROM_REQ;
    ifm_msg->holdrequests = 0;
    ifm_msg->body = malloc(sizeof (struct _cwmp__Inform)+1024);
    if (!ifm_msg->body) {
        LOGE("rpc", "Failed to allocate memory");
        free_cpe_rpc_msg(ifm_msg);
        return NULL;
    }

    return ifm_msg;
}

/**
 * need to read some global information
 */
CpeRpcMsg *rpc_request_create(void)
{
    return NULL;
}

CpeRpcMsg *rpc_empty_create(void)
{
    CpeRpcMsg *empty_msg;

    empty_msg = calloc(1, sizeof (CpeRpcMsg));
    if (!empty_msg) {
        LOGE("rpc", "Failed to allocate memory");
        return NULL;
    }

    empty_msg->id = random();
    empty_msg->type = CWMP_MSG_EMPTY_REQ;

    return empty_msg;
}

CpeRpcMsg *rpc_fault_create(void)
{
    CpeRpcMsg *fault_msg;

    fault_msg = calloc(1, sizeof (CpeRpcMsg));
    if (!fault_msg) {
        LOGE("rpc", "Failed to allocate memory");
        return NULL;
    }

    fault_msg->id = random();
    fault_msg->type = CWMP_MSG_FAULT;

    return fault_msg;
}

/**
 * @param[in] recv: msg recv from acs;
 * @param[out] send: msg will be sent to acs, is created in this func;
 * @return: 0 - success; -1 - failed.
 * please free the memory to which 'send' point outside this function.
 */
int rpc_handler(CpeRpcMsg *recv, CpeRpcMsg **send)
{
    if (!recv || !send) {
        LOGE("rpc", "Invalid input: %p, %p", recv, send);
        return -1;
    }

    /* FIXME: just send empty */
    *send = rpc_empty_create();
    if (!*send)
        return -1;
    return 0;

    switch(recv->type) {
        case 1:
            break;
        default:
            break;
    }
}

char *rpc_msg2xml(CpeRpcMsg *msg)
{
    void *parser;
    char *xml;

    parser = soap_parser_init();
    if (parser == NULL) {
        LOGE("rpc", "init soap parser failed");
        return NULL;
    }

    soap_parser_set_param(parser, SOAP_PARAM_RPC_ID, (void *)msg->id);
    soap_parser_set_param(parser, SOAP_PARAM_RPC_HOLDREQUESTS, (void *)msg->holdrequests);
    soap_parser_set_param(parser, SOAP_PARAM_RPC_TYPE, (void *)msg->type);
    soap_parser_set_param(parser, SOAP_PARAM_RPC_BODY, (void *)msg->body);

    xml = soap_parser_encode(parser);
    soap_parser_cleanup(parser);
    if (!xml) {
        LOGE("rpc", "encode soap xml failed");
        return NULL;
    }

    return xml;

}

CpeRpcMsg *rpc_xml2msg(char *xml)
{
    CpeRpcMsg *msg;
    void *parser;
    int ret;

    msg = calloc(1, sizeof (CpeRpcMsg));
    if (!msg) {
        LOGE("rpc", "Failed to allocate memory");
        return NULL;
    }

    if (!xml)
        msg->type = CWMP_MSG_EMPTY_REQ;
    else {
        parser = soap_parser_init();
        if (!parser) {
            LOGE("rpc", "Failed to init soap parser");
            free(msg);
            return NULL;
        }
        ret = soap_parser_decode(parser, xml);
        if (ret != 0) {
            LOGE("rpc", "Failed to decode soap xml");
            soap_parser_cleanup(parser);
            free(msg);
            return NULL;
        }

        msg->id = (int)soap_parser_get_param(parser, SOAP_PARAM_RPC_ID);
        msg->holdrequests = (int)soap_parser_get_param(parser, SOAP_PARAM_RPC_HOLDREQUESTS);
        msg->type = (int)soap_parser_get_param(parser, SOAP_PARAM_RPC_TYPE);
        msg->body = soap_parser_get_param(parser, SOAP_PARAM_RPC_BODY);

    }

    msg->parser = parser;
    return msg;
}
