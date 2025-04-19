#ifndef NETWORK_SCAN_H
#define NETWORK_SCAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>  // API istekleri için
#include <ctype.h>      // `isxdigit()` ve `toupper()` için
#include <arpa/inet.h>  // Ağ işlemleri için
#include <sys/types.h>
#include <sys/socket.h>
#include "printing.h"
#include "errors.h"
#include "logger.h"

#define API_URL1 "https://api.macvendors.com/" //mac  adress accses

#define OUI_FILE "oui.txt"  // IEEE OUI veritabanı dosyası

size_t write_callback_for_device(void* ptr, size_t size, size_t nmemb, void* userdata) 
{
    strcat((char*)userdata, (char*)ptr);  // Gelen yanıtı birleştir
    return size * nmemb;
}

void normalize_mac(const char *mac, char *normalized) {
    int j = 0;
    for (int i = 0; mac[i] != '\0'; i++) {
        if (isxdigit(mac[i])) {
            normalized[j++] = toupper(mac[i]);
            if (j == 6) break;  // İlk 6 karakter (OUI) yeterli
        }
    }
    normalized[j] = '\0';
}

// MAC üreticisini bulan fonksiyon
void get_mac_vendor(const char *mac) {
    FILE *file = fopen(OUI_FILE, "r");
    if (!file) {
        perror(FILE_OPEN_ERROR);
        log_message(FILE_OPEN_ERROR);
        return;
    }

    char line[256];
    char normalized_mac[7];  // İlk 6 karakter (OUI) + null karakter
    normalize_mac(mac, normalized_mac); // MAC adresini normalize et

    while (fgets(line, sizeof(line), file)) {
        char file_mac[7];
        char vendor[256];

        if (sscanf(line, "%6s %[^\n]", file_mac, vendor) == 2) {
            if (strncmp(file_mac, normalized_mac, 6) == 0) { // İlk 6 karakteri karşılaştır
                printf("Üretici: %s\n",vendor);
                fclose(file);
                return;
            }
        }
    }

    printf("\033[35mBilinmeyen cihaz\033[0m\n");
    fflush(stdout);         
    fclose(file);
}


void identify_device(const char* mac) 
{
    printf("\033[32m");

    CURL* curl;
    CURLcode res;
    char url[256] = API_URL1;  // URL'yi başlat
    char response[1024] = ""; // Yanıtı saklamak için

    // URL'ye MAC adresini ekle
    strcat(url, mac);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  write_callback_for_device);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);  // Yanıtı `response` içine yaz

        // API isteğini gönder
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) 
        {
            fprintf(stderr, CURL_ERROR, curl_easy_strerror(res));
            log_message(CURL_ERROR);
        } 
        else 
        {
           if (strstr(response, "\"errors\"") != NULL) 
            {
                get_mac_vendor(mac);
            }
            else
            {
                printf("Cihaz Marka: %s\n", response);
                fflush(stdout);
            }
        }

        // Temizle
        curl_easy_cleanup(curl);
    } 
    else 
    {
        printf(CURL_INIT_FAILED);
        log_message(CURL_INIT_FAILED);
    }
}

void network_scan(char *network_prefix)
{
    int counter = 0;
    check_and_install_arp_oui();

    for (int i = 1; i <= 255; i++) 
    {
        char ip[16]; // IP adresi için yeterli alan
        snprintf(ip, sizeof(ip), "%s.%d", network_prefix, i); 

        char command[256];
        //snprintf(command, sizeof(command), "ping -c 1 -W 1 %s > /dev/null 2>&1", ip); 
        snprintf(command, sizeof(command), "ping -c 1 -s 0 -W 0.2 %s > /dev/null 2>&1", ip);
        fflush(stdout);

        if (system(command) == 0) 
        {
            printf(PRINTING_LINE);

            printf("Device found: %s\n", ip);
            fflush(stdout);
            int start_port = 20, end_port = 1024; // Tarama aralığı
            
            // MAC adresini almak için ARP komutu
            char command_for_mac[256];
            snprintf(command_for_mac, sizeof(command_for_mac), "arp -n %s | awk 'NR>1 {print $3}'", ip);
            fflush(stdout);

            FILE *fp = popen(command_for_mac, "r");
            if (fp == NULL) 
            {
                perror(POPEN_FAILED);
                log_message(POPEN_FAILED);
                continue;
            }

            char mac[20];
            if (fgets(mac, sizeof(mac), fp) != NULL) 
            {
                mac[strcspn(mac, "\n")] = 0; // Yeni satır karakterini kaldır
                printf("\033[33mMAC Adresi:\033[0m %s\n", mac);
                fflush(stdout);
                identify_device(mac);
            }

            pclose(fp);
            counter++;
        }
    }  

    printf(PRINTING_LINE);

    if (counter > 0)
    {
        printf("Toplam %d cihaz bulundu.\n", counter);
    }
    else
    { 
        printf(NETWORK_SC_ERROR);
        log_message(NETWORK_SC_ERROR);
    }

    printf(PRINTING_LINE);
}

#endif