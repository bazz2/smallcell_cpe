#include <stdio.h>

#include "base.h"

int main()
{
    CpeRpcMsg *rpc_send, *rpc_recv;
    char *xml_send, *xml_recv;

    rpc_send = calloc(1, sizeof (CpeRpcMsg));
    if (!rpc_send) {
        LOGE("main", "Failed to allocate memory");
        return -1;
    }
    rpc_recv = calloc(1, sizeof (CpeRpcMsg));
    if (!rpc_recv) {
        LOGE("main", "Failed to allocate memory");
        return -1;
    }

    rpc_inform_create(NULL, rpc_send);
    while (1) {
        soap_encode(rpc_send, &xml_send);
        curl_easy_send_recv(url, xml_send, &xml_recv);
        soap_decode(xml_recv, rpc_recv);
        /* process 'rpc_recv' & create 'rpc_send' */
        rpc_handle(rpc_recv, rpc_send);
    }
}
