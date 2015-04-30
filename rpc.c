#include <stdio.h>

#include "rpc.h"

/**
 * @param[in] para: TODO: it should be event_para;
 * @param[out] rpc: Inform msg that be created in this func;
 * @return: 0 - success to create inform_msg; -1 - failed.
 */
int rpc_inform_create(void *para, CpeRpcMsg *msg)
{
    if (!msg)
        return -1;
    /* TODO: create rpc */
    return 0;
}

/**
 * @param[in] recv: msg recv from acs;
 * @param[out] send: msg will be sent to acs, is created in this func;
 * @return: 0 - success; -1 - failed.
 */
int rpc_handler(CpeRpcMsg *recv, CpeRpcMsg *send)
{
    if (!recv || !send) {
        LOGE(__FUNCTION__, "Invalid input: %p, %p", recv, send);
        return -1;
    }

    /* initialize 'send_msg' anyway */
    if (send->body)
        free(send->body);
    memset(send, 0, sizeof (CpeRpcMsg));

    switch(recv->type) {
        case 1:
            break;
        default:
            break;
    }
}
