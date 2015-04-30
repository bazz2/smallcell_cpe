#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include "uulog.h"

#define DEFAULT_SIZE (1024*1024)

struct memory_stru{
    char *memory;
    int total_size;
    int real_size;
};

static size_t _write_memory_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
    int write_size = size * nmemb;
    struct memory_stru *mem = (struct memory_stru *)data;

    if (!data)
        return -1;

    if (!mem->memory) {
        mem->memory = malloc(DEFAULT_SIZE);
        mem->total_size = DEFAULT_SIZE;
        mem->real_size = 0;
    }

    while (write_size+mem->real_size >= mem->total_size) {
        LOGV("curl_download", "Realloc memory to %d Byte", mem->total_size*2);
        mem->memory = realloc(mem->memory, mem->total_size*2);
        mem->total_size *= 2;
    }

    if (!mem->memory) {
        LOGE("curl_download", "Failed to realloc memory to %d Byte", mem->total_size*2);
        return -1;
    }

    memcpy(&(mem->memory[mem->real_size]), ptr, write_size);
    mem->real_size += write_size;
    mem->memory[mem->real_size] = 0;
    return write_size;
}

char *curl_easy_download_file(char *url, char *userpwd)
{
    CURL *curl;
    struct memory_stru chunk;

    if (!url)
        return NULL;

    memset(&chunk, 0, sizeof (struct memory_stru));

    curl= curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if (userpwd)
        curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _write_memory_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    if (curl_easy_perform(curl) != CURLE_OK) {
        LOGE("curl_download", "Failed to download file: %s", url);
        curl_easy_cleanup(curl);
        if (chunk.memory)
            free(chunk.memory);
        return NULL;
    }
    curl_easy_cleanup(curl);

    return chunk.memory;
}

static size_t save_header(void *ptr, size_t size, size_t nmemb, void *data)
{
    return (size_t)(size * nmemb);
}

double curl_easy_get_file_size(char *url, char *userpwd)
{
    CURL *curl;
    CURLcode code;
    double len = 0;

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    if (userpwd)
        curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, save_header);
    if (curl_easy_perform(curl) != CURLE_OK) {
        LOGE("curl_get_file_size", "Failed to get file size: %s", url);
        curl_easy_cleanup(curl);
        return 0;
    }

    code = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len);
    if (code != CURLE_OK) {
        LOGE("curl_get_file_size", "Failed to get file size: %s", url);
        curl_easy_cleanup(curl);
        return 0;
    }
    curl_easy_cleanup(curl);
    return len;
}

int curl_easy_send_recv(char *url, char *xml_send, char **xml_resp)
{   
    CURL *curl;
    CURLcode res;
    struct memory_stru chunk;

    if (!url || !xml_send)
        return -1;

    memset(&chunk, 0, sizeof (struct memory_stru));

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _write_memory_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, xml_send);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    *xml_resp = chunk.memory;
    return 0;
}
