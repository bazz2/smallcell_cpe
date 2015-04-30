#include <stdio.h>
#include <time.h>

time_t time_easy_make(int year, int month, int day, int hour, int min, int sec)
{
    struct tm _tmp;
    struct tm *tmp = &_tmp;

    tmp->tm_year = year - 1900;
    tmp->tm_mon  = month - 1;
    tmp->tm_mday = day;
    tmp->tm_hour = hour;
    tmp->tm_min  = min;
    tmp->tm_sec  = sec;
    tmp->tm_isdst = -1;

    time_t ret = mktime(tmp);
    return ret;
}

time_t time_easy_datetime_parse(char *str, char *fmt)
{
    struct tm tmp;
    int year = 1900;
    int mon = 1;
    int day = 1;
    int hour = 0;
    int min = 0;
    int sec = 0;

    if (-1 == sscanf(str, fmt, &year, &mon, &day, &hour, &min, &sec)) {
        return time_easy_make(1900, 1, 1, 0, 0, 0);
    }
    return time_easy_make(year, mon, day, hour, min, sec);
}

int time_easy_datetime_tostring(time_t t, char *fmt, char *buf, int buf_size)
{
    int len;
    struct tm *tmp;
    tmp = localtime(&t);
    if (tmp != NULL) {
        len = strftime(buf, buf_size, fmt, tmp);
        buf[len] = 0;
        return len;
    }
    return 0;
}
