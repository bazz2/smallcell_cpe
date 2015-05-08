#ifndef __BASE_H
#define __BASE_H

/* the same as struct 'CARpcMsg' in omsacs's code */
typedef struct {
    unsigned int id; /* i don't know what it is */
    int holdrequests; /* 1 - holdrequests, 0 - none */
    unsigned int type; /* rpc type */
    void *body; /* struct of msg, depends on rpc's type */

    /* it looks urgly, but i need it to save struct that decode from xml */
    void *parser;
} CpeRpcMsg;

typedef struct {
    char url[512];
} ACS;

void free_cpe_rpc_msg(CpeRpcMsg *msg);

CpeRpcMsg *rpc_inform_create(void *arg);
CpeRpcMsg *rpc_request_create(void);
CpeRpcMsg *rpc_empty_create(void);
int rpc_handler(CpeRpcMsg *recv, CpeRpcMsg **send);
char *rpc_msg2xml(CpeRpcMsg *msg);
CpeRpcMsg *rpc_xml2msg(char *xml);

#endif
