#include <stdio.h>
#include <memory.h>

#include "rpc.h"
#include "soap/parser.h"
#include "log/uulog.h"
#include "utils/curl_easy.h"


/**
 * @param[in] arg: the args get from event_list
 * @return: 0 - success; -1 - failed.
 */
int exec_session(void *arg)
{
    char *url = "localhost";
    CpeRpcMsg *rpc_send = NULL, *rpc_recv = NULL;
    char *xml_send = NULL, *xml_recv = NULL;
    int holdrequests;

    rpc_send = rpc_inform_create(arg);
    while (1) {
        xml_send = rpc_msg2xml(rpc_send);
        free_cpe_rpc_msg(rpc_send);
        rpc_send = NULL;
        if (!xml_send) {
            LOGE(__FUNCTION__, "Failed to encode to xml");
            break;
        }

        curl_easy_send_recv(url, xml_send, &xml_recv);
        free(xml_send);
        if (!xml_recv) {
            LOGE(__FUNCTION__, "Failed to recv resp from acs");
            break;
        }

        rpc_recv = rpc_xml2msg(xml_recv);
        free(xml_recv);
        if (!rpc_recv) {
            LOGE(__FUNCTION__, "Failed to decode from xml");
            break;
        }

        /* recv empty resp */
        if (rpc_recv->type == CWMP_MSG_EMPTY_REQ) {
            rpc_send = rpc_request_create();
            if (!rpc_send)
                break; /* end the session */
            else
                continue;
        }

        holdrequests = rpc_recv->holdrequests;

        /* process 'rpc_recv' & create 'rpc_send' */
        rpc_handler(rpc_recv, &rpc_send);
        free_cpe_rpc_msg(rpc_recv);
        if (!rpc_send) {
            LOGE(__FUNCTION__, "Failed to handle rpc");
            break;
        }

        if (rpc_send->type != CWMP_MSG_EMPTY_RSP)
            continue;

        if (holdrequests) {
            rpc_send = rpc_empty_create();
        } else {
            rpc_send = rpc_request_create();
            if (!rpc_send)
                rpc_send = rpc_empty_create();
        }
        if (!rpc_send) {
            LOGE(__FUNCTION__, "Failed to create rpc_send");
            break;
        }
    }
}

int main(void)
{
    while (1) {
        sleep(100);

        exec_session(NULL);
    }
}
