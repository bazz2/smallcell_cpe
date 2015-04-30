
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log/uulog.h"
#define LOG_TAG "string-easy"

char *string_easy_trim(char *str)
{
    char *s = str;
    char *p;
    while (*s == ' ' || *s == '\t') {
        s += 1;
    }
    p = s;

    while (*p != '\0' && *p != '\r' && *p != '\n' && *p != ' ' && *p != '\t') {
        p += 1;
    }
    *p = 0;
    return s;
}

char **string_easy_split(char *str, const char *delim, int *num)
{
    char **split_array = NULL;

    char *ptr;
    int i;
    int _num = 0;

    *num = 0;
    if (str == NULL) {
        return NULL;
    }

    _num = 1;
    ptr = strstr(str, delim);
    while (NULL != ptr) {
        _num += 1;
        ptr += strlen(delim);
        ptr = strstr(ptr, delim);
    }

    split_array = (char **)malloc(sizeof(char *) * _num);
    if (split_array == NULL) {
        return NULL;
    }
    memset(split_array, 0, sizeof(char *) * _num);

    i = 0;
    ptr = strtok(str, delim);
    while (ptr != NULL && i < _num) {
        split_array[i] = ptr;
        i += 1;
        ptr = strtok(NULL, delim);
    }
    _num = i;

    *num = _num;
    return split_array;
}
