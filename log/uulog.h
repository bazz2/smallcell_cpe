#ifndef _UU_LOG_H_
#define _UU_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_MODE_PRINT   0x1
#define LOG_MODE_UDP     0x2

#define LOG_TAG_SIZE 32

int uulog_init(int mode, int server_port);
void uulog_free();
void uulog_setlevel(int level);
void uulog_write(int level, const char *tag,
                 const char *filename, const int line,
                 const char *fmt, ...) __attribute__((format(printf, 5, 6)));

#define LOG_ERROR           3
#define LOG_WARN            4
#define LOG_HIGHLIGHT       5
#define LOG_INFO            6
#define LOG_DEBUG           7
#define LOG_VERBOSE         8

#define LOGE(tag, x...)  uulog_write(LOG_ERROR, tag, __FILE__, __LINE__, x)
#define LOGW(tag, x...)  uulog_write(LOG_WARN, tag, __FILE__, __LINE__, x)
#define LOGH(tag, x...)  uulog_write(LOG_HIGHLIGHT, tag, __FILE__, __LINE__, x)
#define LOGI(tag, x...)  uulog_write(LOG_INFO, tag, __FILE__, __LINE__, x)
#define LOGD(tag, x...)  uulog_write(LOG_DEBUG, tag, __FILE__, __LINE__, x)
#define LOGV(tag, x...)  uulog_write(LOG_VERBOSE, tag, __FILE__, __LINE__, x)

#define LOGDUMP(tab, buf, len)  uulog_dump(tag, __FILE__, __LINE__, buf, len)

#define STRING_ERROR_OUT_OF_MEMORY "out of memory"

#ifdef __cplusplus
}
#endif

#endif
