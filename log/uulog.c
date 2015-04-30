
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "uulog.h"

#ifdef DEBUG
#define LOGGER_OPT_DEBUG
#endif

#define LOG_DEFAULT_LEVEL  10 /* messages <= this level are logged */
#define MAX_MSG_BUF        1024*10

#define NONE         "\033[m"
#define RED          "\033[0;32;31m"
#define LIGHT_RED    "\033[1;31m"
#define GREEN        "\033[0;32;32m"
#define LIGHT_GREEN  "\033[1;32m"
#define BLUE         "\033[0;32;34m"
#define LIGHT_BLUE   "\033[1;34m"
#define DARY_GRAY    "\033[1;30m"
#define CYAN         "\033[0;36m"
#define LIGHT_CYAN   "\033[1;36m"
#define PURPLE       "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN        "\033[0;33m"
#define YELLOW       "\033[1;33m"
#define LIGHT_GRAY   "\033[0;37m"
#define WHITE        "\033[1;37m"

#define BEEP "\007"

#define FMT_3 "%s <E>[%s]: %s %s\n"
#define FMT_4 "%s <W>[%s]: %s %s\n"
#define FMT_5 "%s <H>[%s]: %s %s\n"
#define FMT_6 "%s <I>[%s]: %s %s\n"
#define FMT_7 "%s <D>[%s]: %s %s\n"
#define FMT_8 "%s <V>[%s]: %s %s\n"
#define FMT_DUMP "%s <DUMP>[%s]: %s\n %s\n"

#define FMT_3_COLOR "%s " BEEP RED "<E>[%s]: %s" NONE " %s\n"
#define FMT_4_COLOR "%s " BEEP YELLOW "<W>[%s]: %s" NONE " %s\n"
#define FMT_5_COLOR "%s " GREEN "<H>[%s]: %s" NONE " %s\n"
#define FMT_6_COLOR "%s " WHITE "<I>[%s]: %s" NONE " %s\n"
#define FMT_7_COLOR "%s " CYAN "<D>[%s]: %s" NONE " %s\n"
#define FMT_8_COLOR "%s " BLUE "<V>[%s]: %s" NONE " %s\n"
#define FMT_DUMP_COLOR "%s " CYAN "<DUMP>[%s]:" NONE " %s\n %s\n"

struct uulog_client {
    int is_ready;
    pthread_rwlock_t rwlock;

    int mode;
    int level;

    int port;
    int socket;
    struct sockaddr_in srv_addr;

    time_t last_time;
    char time_label[256];
};

static struct uulog_client g_client = {
    .is_ready = 0
};

void uulog_setlevel(int level)
{
    struct uulog_client *ucli = &g_client;

    pthread_rwlock_wrlock(&ucli->rwlock);
    ucli->level = level;
    pthread_rwlock_unlock(&ucli->rwlock);
}

void uulog_dump(const char *tag,
                const char *filename, const int line,
                const char *hex_buf, size_t hex_size)
{
    struct uulog_client *ucli = &g_client;

    int ret;
    int offset;
    size_t index;
    int i, cnt;
    char buf[MAX_MSG_BUF];
    char file_buf[256];
    char msg_buf[MAX_MSG_BUF - 256];
    time_t now;

    if (ucli->is_ready == 0) {
        return;
    }

    pthread_rwlock_rdlock(&ucli->rwlock);

    offset = 0;
    index = 0;
    i = 0;
    cnt = 0;
    while (index < hex_size) {
        ret = sprintf(msg_buf + offset, "0x%04X  ", index);
        offset += ret;

        for (i = index; i < index + 16 && i < hex_size; i++) {
            ret = sprintf(msg_buf + offset, "%02X ", hex_buf[i]);
            offset += ret;
        }
        index += 16;
        ret = sprintf(msg_buf + offset, "\n");
        offset += ret;
    }
    sprintf(msg_buf + offset, "\n");

    gettimeofday(&now, NULL);
    if (now != ucli->last_time) {
        struct tm *timeinfo;
        timeinfo = localtime(&now);
        strftime(ucli->time_label, 256 - 1, "%Y-%m-%d %I:%M:%S", timeinfo);
        memcpy(&ucli->last_time, &now, sizeof(time_t));
    }

#ifdef LOGGER_OPT_DEBUG
    ret = snprintf(file_buf, 512, "(%s,%d)", filename, line);
    file_buf[ret] = 0;
#else
    file_buf[0] = 0;
#endif

#ifdef LOGGER_OPT_DEBUG
    ret = snprintf(buf, MAX_MSG_BUF - 1, FMT_DUMP_COLOR,
                   ucli->time_label, tag, file_buf, msg_buf);
#else
    ret = snprintf(buf, MAX_MSG_BUF - 1, FMT_DUMP,
                   ucli->time_label, tag, file_buf, msg_buf);
#endif

    if (ucli->mode & LOG_MODE_UDP) {
        ret = sendto(ucli->socket, buf, ret, 0,
                     (void *) & (ucli->srv_addr), sizeof(ucli->srv_addr));
    }
    if (ucli->mode & LOG_MODE_PRINT) {
        printf("%s", buf);
    }

    pthread_rwlock_unlock(&ucli->rwlock);
    return;
}

void uulog_write(int level, const char *tag,
                 const char *filename, const int line,
                 const char *fmt, ...)
{
    struct uulog_client *ucli = &g_client;

    int ret;
    char buf[MAX_MSG_BUF];
    char file_buf[256];
    char msg_buf[MAX_MSG_BUF - 256];
    time_t now;
    va_list ap;

    if (ucli->is_ready == 0) {
        return;
    }

    pthread_rwlock_rdlock(&ucli->rwlock);

    if (level > ucli->level) {
        pthread_rwlock_unlock(&ucli->rwlock);
        return;
    }

    va_start(ap, fmt);
    ret = vsnprintf(msg_buf, MAX_MSG_BUF - 256, fmt, ap);
    msg_buf[ret] = 0;
    va_end(ap);

    gettimeofday(&now, NULL);

    if (now != ucli->last_time) {
        struct tm *timeinfo;
        timeinfo = localtime(&now);
        strftime(ucli->time_label, 256 - 1, "%Y-%m-%d %I:%M:%S", timeinfo);
        memcpy(&ucli->last_time, &now, sizeof(time_t));
    }

#ifdef LOGGER_OPT_DEBUG
    ret = snprintf(file_buf, 512, "(%s,%d)", filename, line);
    file_buf[ret] = 0;
#else
    file_buf[0] = 0;
#endif

#ifdef LOGGER_OPT_DEBUG
    switch (level) {
        case LOG_ERROR:
            ret = snprintf(buf, MAX_MSG_BUF - 1, FMT_3_COLOR, ucli->time_label, tag, msg_buf, file_buf);
            break;
        case LOG_WARN:
            ret = snprintf(buf, MAX_MSG_BUF - 1, FMT_4_COLOR, ucli->time_label, tag, msg_buf, file_buf);
            break;
        case LOG_HIGHLIGHT:
            ret = snprintf(buf, MAX_MSG_BUF - 1, FMT_5_COLOR, ucli->time_label, tag, msg_buf, file_buf);
            break;
        case LOG_INFO:
            ret = snprintf(buf, MAX_MSG_BUF - 1, FMT_6_COLOR, ucli->time_label, tag, msg_buf, file_buf);
            break;
        case LOG_DEBUG:
            ret = snprintf(buf, MAX_MSG_BUF - 1, FMT_7_COLOR, ucli->time_label, tag, msg_buf, file_buf);
            break;
        case LOG_VERBOSE:
        default:
            ret = snprintf(buf, MAX_MSG_BUF - 1, FMT_8_COLOR, ucli->time_label, tag, msg_buf, file_buf);
            break;
    }
#else
    switch (level) {
        case LOG_ERROR:
            ret = snprintf(buf, MAX_MSG_BUF - 1, FMT_3, ucli->time_label, tag, msg_buf, file_buf);
            break;
        case LOG_WARN:
            ret = snprintf(buf, MAX_MSG_BUF - 1, FMT_4, ucli->time_label, tag, msg_buf, file_buf);
            break;
        case LOG_HIGHLIGHT:
            ret = snprintf(buf, MAX_MSG_BUF - 1, FMT_5, ucli->time_label, tag, msg_buf, file_buf);
            break;
        case LOG_INFO:
            ret = snprintf(buf, MAX_MSG_BUF - 1, FMT_6, ucli->time_label, tag, msg_buf, file_buf);
            break;
        case LOG_DEBUG:
            ret = snprintf(buf, MAX_MSG_BUF - 1, FMT_7, ucli->time_label, tag, msg_buf, file_buf);
            break;
        case LOG_VERBOSE:
        default:
            ret = snprintf(buf, MAX_MSG_BUF - 1, FMT_8, ucli->time_label, tag, msg_buf, file_buf);
            break;
    }
#endif

    if (ucli->mode & LOG_MODE_UDP) {
        ret = sendto(ucli->socket, buf, ret, 0,
                     (void *) & (ucli->srv_addr), sizeof(ucli->srv_addr));
    }
    if (ucli->mode & LOG_MODE_PRINT) {
        printf("%s", buf);
    }

    pthread_rwlock_unlock(&ucli->rwlock);
    return;
}

void uulog_free()
{
    struct uulog_client *ucli = &g_client;
    if (ucli) {
        if (ucli->mode & LOG_MODE_UDP) {
            if (ucli->socket > 0) {
                close(ucli->socket);
                ucli->socket = 0;
            }
        }

        pthread_rwlock_destroy(&ucli->rwlock);
        memset((char *)ucli, 0, sizeof(struct uulog_client));
    }
}

int uulog_init(int mode, int server_port)
{
    struct uulog_client *ucli = &g_client;
    int fd;

    if (ucli->is_ready != 0) {
        uulog_free();
    }

    memset((char *)ucli, 0, sizeof(struct uulog_client));

    ucli->mode = mode;
    ucli->level = LOG_DEFAULT_LEVEL;
    ucli->last_time = 0;

    if (ucli->mode & LOG_MODE_UDP) {
        ucli->port = server_port;
        ucli->socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (ucli->socket < 0) {
            uulog_free();
            return 2;
        }
        ucli->srv_addr.sin_family = AF_INET;
        ucli->srv_addr.sin_port = htons(ucli->port);
        ucli->srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    pthread_rwlock_init(&ucli->rwlock, NULL);
    ucli->is_ready = 1;
    return 0;

}
