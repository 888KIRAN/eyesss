/*
 * Developer : Muhammed Emin Kıran  
 * GitHub    : https://github.com/888KIRAN  
 * License   : :)
 */

#ifndef FLOOD_ENGINE_H
#define FLOOD_ENGINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <errno.h>
#include "printing.h"

#define TIMEOUT 1
#define MAX_LINE_LENGTH 1024

struct MemoryStruct{
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        printf("Yetersiz bellek\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Trim: baştaki/sondaki boşlukları ve \r\n'i keser
char* trim(char* str) {
    char *end;
    // baştaki boşluk
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    // sondaki boşluk + \r\n
    end = str + strlen(str) - 1;
    while (end > str && (isspace((unsigned char)*end) || *end == '\r' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    return str;
}

void find_port_description(int port_get, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Dosya açılamadı");
        return;
    }

    char line[MAX_LINE_LENGTH];

    // Başlık satırını atla
    if (!fgets(line, sizeof(line), file)) {
        fclose(file);
        return;
    }

    while (fgets(line, sizeof(line), file)) 
    {
        // satırı kopyala, çünkü strtok değiştiriyor
        char buf[MAX_LINE_LENGTH];
        strncpy(buf, line, MAX_LINE_LENGTH);
        buf[MAX_LINE_LENGTH-1] = '\0';

        // satırı parçala: Service Name,Port Number,Transport Protocol,Description
        char *service = strtok(buf, ",");
        char *port_str = strtok(NULL, ",");
        char *proto = strtok(NULL, ",");
        char *desc = strtok(NULL, "");  // gerisi description

        if (!service || !port_str || !proto || !desc) continue;

        service = trim(service);
        port_str = trim(port_str);
        proto = trim(proto);
        desc = trim(desc);

        int port = atoi(port_str);

        if (port == port_get) 
        {
            // port eşleşti, direkt yazdır
            printf("→ : [\033[38;5;220m%s\033[0m] [ Servis: %s ]%s \n",
                   proto,desc, service);
            fflush(stdout);

            fclose(file);
            return;
        }
    }

    printf("→ Açıklama bulunamadı.\n");
    fflush(stdout);

    fclose(file);
}


int scan_port(const char *ip, int port) 
{
    int sockfd;
    struct sockaddr_in target_addr;
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return 0;

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &target_addr.sin_addr);

    int conn = connect(sockfd, (struct sockaddr*)&target_addr, sizeof(target_addr));
    close(sockfd);

    if (conn == 0)
    {
        return 1;
    }  // Bağlantı başarılı: port açık
    else{
        return 0;  
    }  // Bağlantı başarısız: port kapalı
}
// Gelen yanıtı bellekte tutmak için

void parse_json_response(const char *json_data) {
    // JSON verisini çözümleme
    cJSON *json = cJSON_Parse(json_data);
    if (json == NULL) {
        printf("Geçersiz JSON\n");
        return;
    }

    // 'data' -> 'search' -> 'docs' alanını almak
    cJSON *search = cJSON_GetObjectItem(json, "data");
    cJSON *docs = cJSON_GetObjectItem(search, "docs");

    // 'docs' dizisinde her bir CVE'yi işlemek
    int num_docs = cJSON_GetArraySize(docs);
    for (int i = 0; i < num_docs; i++) {
        cJSON *doc = cJSON_GetArrayItem(docs, i);
        cJSON *cve = cJSON_GetObjectItem(doc, "cve");
        cJSON *description = cJSON_GetObjectItem(doc, "description");
        cJSON *published = cJSON_GetObjectItem(doc, "published");
        cJSON *cvss = cJSON_GetObjectItem(doc, "cvss");

        printf("\033[38;5;209mCVE: %s\033[0m\n", cve->valuestring);
        printf("\033[38;5;209mDescription: %s\033[0m\n", description->valuestring);
        printf("\033[38;5;209mPublished: %s\033[0m\n", published->valuestring);
        printf("\033[38;5;209mCVSS: %s\033[0m\n", cvss->valuestring);
        printf("\033[38;5;209m------------------------------\033[0m\n");
    }

    cJSON_Delete(json);
}

void crul_function_api() {
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);  // Başlangıçta boş
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    if (curl_handle) {
        const char *url = "https://vulners.com/api/v3/search/lucene/?query=apache&size=10";

        curl_easy_setopt(curl_handle, CURLOPT_URL, url);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl_handle);

        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() hatası: %s\n", curl_easy_strerror(res));
        else {
            // JSON yanıtını çözümle
            parse_json_response(chunk.memory);
        }

        curl_easy_cleanup(curl_handle);
        free(chunk.memory);
    }

    curl_global_cleanup();
}

int port_function(char *network_prefix) 
{
    int start_port = 1;
    int end_port =  65535 ;
    int counter = 0;

    for (int i = 1; i <= 255; i++) 
    {
        char ip[16]; // IP adresi için yeterli alan
        snprintf(ip, sizeof(ip), "%s.%d", network_prefix, i); 

        char command[256];
        snprintf(command, sizeof(command), "ping -c 1 -s 0 -W 0.2 %s > /dev/null 2>&1", ip);
        if (system(command) == 0) 
        {
            printf("\nScanning %s from port %d to %d...\n\n", ip, start_port, end_port);

            for (int port = start_port; port <= end_port; port++) 
            {
                if (scan_port(ip, port)) {
                    printf("[\033[1;32mOPEN\033[0m] Port \033[1m\033[31m%d\033[0m ", port);
                    if(port<=1023)printf("**");
                    find_port_description(port, "service-names-port-numbers.csv");
                    crul_function_api();
                    fflush(stdout);
                } 
            }

            printf("\nScan complete.\n");
            fflush(stdout);

        } 
    }

    return 0;  
}

#endif // FLOOD_ENG