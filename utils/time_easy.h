#ifndef _CWMPACS_UTILS_TIME_EASY_H_
#define _CWMPACS_UTILS_TIME_EASY_H_

#include <time.h>

time_t time_easy_make(int year, int month, int day, int hour, int min, int sec);
time_t time_easy_datetime_parse(char *str, char *fmt);
int time_easy_datetime_tostring(time_t t, char *fmt, char *buf, int buf_size);

#endif
