#ifndef NETWORK_INTERFACE_SCAN_H
#define NETWORK_INTERFACE_SCAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>  // for api
#include "printing.h"
#include "logger.h"

#define API_URL "https://ipinfo.io/json" //bu site 

struct Memory 
{
    char *response;
    size_t size;
};

// HTTP yanıtını tutacak değişken
struct Response {
    char *data;
    size_t size;
};

// Bellek tahsisi için callback fonksiyonu
size_t write_callback(void *ptr, size_t size, size_t nmemb, struct Memory *mem) 
{
    size_t total_size = size * nmemb;
    char *temp = realloc(mem->response, mem->size + total_size + 1);
    if (temp == NULL) return 0;

    mem->response = temp;
    memcpy(&(mem->response[mem->size]), ptr, total_size);
    mem->size += total_size;
    mem->response[mem->size] = '\0';

    return total_size;
}

void move_cursor(int row, int col) //kulaılmıyor
{
    printf("\033[%d;%dH", row, col);
}

void print_pretty_json(const char *json) 
{

    while (*json) 
    {
        if (strncmp(json, "readme", 6) == 0) 
        {
            printf(PRINTING_LINE_2);
            break;
        } 

        if (*json == ',') 
        {
            printf("\a\r\033[0;32m\033[1m[+]\033[0m\t");  // Virgülden sonra alt satıra geç (/a fuyarı sesi )
        }
        else if (*json == '{' || *json == '}' || *json == '"')
        {
            printf(" ");
        } 
        else if (*json == ':')
        {
            printf(" = ");
        } 
        else 
        {
            putchar(*json);  // Karakteri olduğu gibi yazdır
            fflush(stdout);
        }

        json++;
    }

        printf("\n");
}

int beginning_functions(int argc, char const *argv[])
{
    CURL *curl;
    CURLcode res;
    struct Memory chunk = { .response = malloc(1), .size = 0 };

    printf("\033[H\033[J");
    system("clear");
    fflush(stdout);

    printing_function();

    if (chunk.response == NULL) 
    {
        fprintf(stderr, MEMORY_ALLOCATION_FAILED);
        log_message(MEMORY_ALLOCATION_FAILED);

        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) 
    {
        curl_easy_setopt(curl, CURLOPT_URL, API_URL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) 
        {
            fprintf(stderr, CURL_INIT_WARNING);
            log_message(CURL_INIT_WARNING);
            printing_function_Network();
            terminal_code(argc,argv);
            return 1;
        } 
        else 
        {
            printf(PRINTING_LINE_2);
            print_pretty_json(chunk.response);  // JSON'u düzenli şekilde yazdır
        }

        curl_easy_cleanup(curl);
    }

    free(chunk.response);//belek serbest
    curl_global_cleanup();
}
#endif 
