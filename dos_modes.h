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
#include <errno.h>

#define TIMEOUT 1
#define MAX_LINE_LENGTH 1024


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
            printf("→ Açıklama: [\033[38;5;220m%s\033[0m] [ Servis: %s ]%s \n",
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
                    printf("[\033[1;32mOPEN\033[0m] Port %d", port);
                    find_port_description(port, "service-names-port-numbers.csv");
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