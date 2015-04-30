#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <getopt.h>

#include "config.h"
#include "log/uulog.h"

#define MAX_RECV_BUF        1024*10

#define SRV_STATE_NONE      0
#define SRV_STATE_INIT      1
#define SRV_STATE_RUN       2
#define SRV_STATE_STOP      3

struct uulog_server {
    int state;
    pthread_rwlock_t rwlock;

    int port;
    int socket;
};

int uulog_server_stop(void *handle)
{
    struct uulog_server *usrv = (struct uulog_server *)handle;

    pthread_rwlock_wrlock(&usrv->rwlock);
    if (usrv->state != SRV_STATE_RUN) {
        pthread_rwlock_unlock(&usrv->rwlock);
        return 0;
    }

    usrv->state = SRV_STATE_STOP;
    pthread_rwlock_unlock(&usrv->rwlock);
    return 0;
}

int uulog_server_start(void *handle)
{
    struct uulog_server *usrv = (struct uulog_server *)handle;

    struct sockaddr_in client;
    int addr_len = sizeof(client);
    char buffer[MAX_RECV_BUF];
    int byte_read;

    pthread_rwlock_wrlock(&usrv->rwlock);
    if (usrv->state != SRV_STATE_INIT) {
        pthread_rwlock_unlock(&usrv->rwlock);
        return 0;
    }

    usrv->state = SRV_STATE_RUN;
    pthread_rwlock_unlock(&usrv->rwlock);

    while (1) {
        pthread_rwlock_wrlock(&usrv->rwlock);
        if (usrv->state != SRV_STATE_RUN) {
            pthread_rwlock_unlock(&usrv->rwlock);
            break;
        }

        byte_read = recvfrom(usrv->socket, buffer, MAX_RECV_BUF, 0 , (struct sockaddr *)&client, &addr_len);
        if (byte_read > 0) {
            pthread_rwlock_unlock(&usrv->rwlock);
            buffer[byte_read] = 0;
            printf("%s", buffer);
        } else {
            pthread_rwlock_unlock(&usrv->rwlock);
            sleep(1);
        }
    }

    pthread_rwlock_wrlock(&usrv->rwlock);
    usrv->state = SRV_STATE_INIT;
    pthread_rwlock_unlock(&usrv->rwlock);
    return 0;
}

void uulog_server_free(void *handle)
{
    struct uulog_server *usrv = (struct uulog_server *)handle;

    usrv->state = SRV_STATE_NONE;

    if (usrv->socket > 0) {
        close(usrv->socket);
        usrv->socket = 0;
    }
    pthread_rwlock_destroy(&usrv->rwlock);
    free(usrv);
}

void *uulog_server_init(int port)
{
    struct uulog_server *usrv;

    int ret;
    struct sockaddr_in addr;

    usrv = (struct uulog_server *)malloc(sizeof(struct uulog_server));
    if (usrv == NULL) {
        return NULL;
    }
    memset((char *)usrv, 0, sizeof(struct uulog_server));

    pthread_rwlock_init(&usrv->rwlock, NULL);

    usrv->port = port;
    usrv->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (usrv->socket < 0) {
        printf("open socket failed!");
        uulog_server_free(usrv);
        return NULL;
    }

    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(usrv->port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(usrv->socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        printf("bind socket failed!");
        uulog_server_free(usrv);
        return NULL;
    }
    usrv->state = SRV_STATE_INIT;
    printf("uulog server, port=%d\n", port);
    return (void *)usrv;
}

static const char *option_string = "p:h";
static const struct option option_list[] = {
    { "port", required_argument, NULL, 'p' },
    { "help", no_argument, NULL, 'h' },
    { NULL, no_argument, NULL, 0 }
};

void display_usage(void)
{
    printf("uusrv - uulog server\n\n");
    printf("  -p --%-24s server port, default %d\n", "port", UULOG_PORT);
    printf("  -h --%-24s display this help and exit\n", "help");
};

int main(int argc, char **argv)
{
    int port;
    void *srv;

    int opt;

    port = UULOG_PORT;

    while (1) {
        opt = getopt_long(argc, argv, option_string, option_list, NULL);
        if (opt == -1) {
            break;
        }
        switch (opt) {
            case 'p':
                if (optarg != NULL) {
                    port = atoi(optarg);
                }
                break;
            case 'h':
            default:
                display_usage();
                return;
        }
    }

    srv = uulog_server_init(port);
    if (srv != NULL) {
        uulog_server_start(srv);
    }
    return 0;
}
