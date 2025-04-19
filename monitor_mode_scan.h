#ifndef MONITOR_MODE_SCAN_H
#define MONITOR_MODE_SCAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pcap.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "printing.h"
#include "network_scan.h"
#include "errors.h"

// --- Sabitler ---
#define MAX_ADAPTERS 30
#define MAX_ADAPTER_NAME_LENGTH 128
#define MAX_PACKET_SIZE 2048
#define IEEE80211_HDRLEN 24  // 802.11 başlık uzunluğu
#define MAX_NETWORKS 256
#define MAX_CLIENTS 256    // Bağlı istemciler için
#define MAX_SSID_DISPLAY 21 // Görüntülenen maksimum SSID uzunluğu

// --- Global Mutex ---
// Global veri (AP ve istemci listeleri) üzerinde olası eşzamanlı erişimlerde veri bütünlüğü sağlamak için
static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- Yapılar ---

// IEEE 802.11 çerçeve yapısı
struct ieee80211_hdr {
    uint16_t frame_control;
    u_short duration;
    u_char dest_mac[6];
    u_char src_mac[6];
    u_char bssid[6];
    u_short seq_ctrl;
};

// AP (erişim noktası) bilgilerini tutan yapı
typedef struct {
    char ssid[33];
    char bssid[18];
    char signal[5];
    char channel[4];
    char enc_cipher[20];
    char hb[32];           // Veri hızı (Mbps)
    int beacon_count;      // Beacon paket sayısı
    int data_count;        // Veri paket sayısı
} wifi_network_t;

wifi_network_t networks[MAX_NETWORKS];
int network_count = 0;

// İstemci bilgilerini tutan yapı
typedef struct {
    char client_mac[18];
    char associated_bssid[18];
    char associated_ssid[33];  // İlişkili AP'nin SSID'si
    char signal[5];
    char hb[32];           // Veri hızı (Mbps)
    int packet_count;      // İstemciden alınan paket sayısı
    char last_seen[32];    // Son görülme zamanı (HH:MM:SS)
} wifi_client_t;

wifi_client_t clients[MAX_CLIENTS];
int client_count = 0;

// Son tablo güncelleme zamanı (ekran yenileme için)
static time_t last_print_time = 0;

// --- Yardımcı Fonksiyonlar ---

// Belirtilen maksimum uzunlukta dizeyi kısaltır, gerekirse sonuna "..." ekler.
void truncate_ssid(const char *src, char *dest, int max_len) {
    // Geçersiz parametre kontrolü
    if (!src || !dest || max_len <= 0) {
        if (dest) {
            dest[0] = '\0';
        }
        return;
    }

    // Eğer `max_len == 1` ise hiçbir karakter sığmaz, boş string yap
    if (max_len == 1) {
        dest[0] = '\0';
        return;
    }

    // Kaynak dizgi uzunluğu
    int src_len = strlen(src);

    // Kaynak dizgi yeterince kısaysa, direkt kopyala
    if (src_len < max_len) {
        strcpy(dest, src);  // Tamamını kopyala
        return;
    }

    // Kaynak dizgi uzun, kesmemiz gerekiyor
    // Yeterli alan varsa (en az 4 karakter), "..." ekle
    if (max_len > 3) {
        // İlk `max_len - 3` karakteri kopyala
        strncpy(dest, src, max_len - 3);
        dest[max_len - 3] = '\0';
        strcat(dest, "...");
    } else {
        // `max_len <= 3` durumunda "..." ekleyemeyiz
        // En fazla "." koyabiliriz
        dest[0] = '.';
        // Eğer max_len == 2 ise dest[1] = '\0' koy
        if (max_len > 1) {
            dest[1] = '\0';
        }
    }
}

// MAC adresini okunabilir biçimde (XX:XX:XX:XX:XX:XX) biçimler.
void format_mac_address(const u_char *mac, char *buffer) {
    snprintf(buffer, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// --- Ekran Yazdırma Fonksiyonları ---

// AP tablosunu ekrana yazdırır (SSID kısaltılmış halde)
void print_networks_table() {
    char display_ssid[MAX_SSID_DISPLAY + 1];
    
    // Veri okuma sırasında veri bütünlüğünü sağlamak için kilitle
    pthread_mutex_lock(&data_mutex);
    printf(PRINTING_LINE_3_FOR_TABLE_1);
    fflush(stdout);
    printf(PRINTING_LINE_3_FOR_TABLE_2);
    fflush(stdout);
    printf(PRINTING_LINE_3_FOR_TABLE_3);
    fflush(stdout);
    for (int i = 0; i < network_count; i++) {
        truncate_ssid(networks[i].ssid, display_ssid, MAX_SSID_DISPLAY);
        printf(PR_1);
        fflush(stdout);
    }
    printf(PRINTING_LINE_3_FOR_TABLE_4);
    pthread_mutex_unlock(&data_mutex);
}

// İstemci tablosunu ekrana yazdırır (ilişkili AP SSID gösterimi ile)
void print_clients_table() {
    char display_ap_ssid[MAX_SSID_DISPLAY + 1];
    pthread_mutex_lock(&data_mutex);
    printf("\nConnected Devices (Clients):\n");
    fflush(stdout);
    printf(PRINTING_LINE_3_FOR_TABLE_1_FOUND_DIVICE);
    fflush(stdout);
    printf(PRINTING_LINE_3_FOR_TABLE_2_FOUND_DIVICE);
    fflush(stdout);
    printf(PRINTING_LINE_3_FOR_TABLE_3_FOUND_DIVICE);
    fflush(stdout);
    for (int i = 0; i < client_count; i++) {
        truncate_ssid(clients[i].associated_ssid, display_ap_ssid, MAX_SSID_DISPLAY);
        printf(PR_2);
        fflush(stdout);
    }
    printf(PRINTING_LINE_3_FOR_TABLE_4_FOUND_DIVICE);
    fflush(stdout);
    pthread_mutex_unlock(&data_mutex);
}

// Hem AP hem de istemci tablolarını birlikte güncelleyen fonksiyon.
// Not: Ekran temizleme için "system("clear")" kullanılıyor; gelecekte ncurses gibi bir kütüphane ile flickering azaltılabilir.
void print_all_tables() {
    time_t now = time(NULL);
    if (now - last_print_time >= 1) {
        printf("\033c");           // Terminali resetler
        printf("\033[2J\033[H");  // Ekranı temizler
        fflush(stdout);
        print_networks_table();
        print_clients_table();
        fflush(stdout);
        last_print_time = now;
    }
}

// --- Yardımcı Arama Fonksiyonları ---

// Belirtilen BSSID'ye sahip AP'nin dizinini döndürür; yoksa -1
int find_network_index(const char *bssid) {
    for (int i = 0; i < network_count; i++) {
        if (strcmp(networks[i].bssid, bssid) == 0) {
            return i;
        }
    }
    return -1;
}

// Belirtilen client MAC adresine sahip istemcinin dizinini döndürür; yoksa -1
int find_client_index(const char *client_mac) {
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].client_mac, client_mac) == 0) {
            return i;
        }
    }
    return -1;
}

// --- Adaptör İşlemleri ---

// Adaptörü monitor moda geçirmek için: kapat, tür değiştir ve tekrar aç
int set_monitor_mode(const char *adapter) {
    char command[256];

    // RF-kill durumunu kaldır (sudo gerektirir)
    if (system("sudo rfkill unblock all") != 0) {
        fprintf(stderr, RF_KILL_ERROR);
        log_message(RF_KILL_ERROR);
    }

    // Adaptörü kapat
    snprintf(command, sizeof(command), "sudo ip link set %s down", adapter);
    if (system(command) != 0) {
        fprintf(stderr, ADAPTER_CLOSE_ERROR, adapter);
        log_message(ADAPTER_CLOSE_ERROR, adapter);

        return -1;
    }

    // Monitor moda geçiş
    snprintf(command, sizeof(command), "sudo iw dev %s set type monitor", adapter);
    if (system(command) != 0) {
        fprintf(stderr, MONITOR_MODE_ERROR, adapter);
        log_message(MONITOR_MODE_ERROR, adapter);

        return -1;
    }

    // Adaptörü tekrar aç
    snprintf(command, sizeof(command), "sudo ip link set %s up", adapter);
    if (system(command) != 0) {
        fprintf(stderr, ADAPTER_OPEN_ERROR,adapter);
        log_message(ADAPTER_OPEN_ERROR,adapter);

        return -1;
    }

    return 0;
}

// --- Kanal Değiştirme (Channel Hopping) ---

// Belirtilen kanallar arasında her saniye geçiş yapan iş parçacığı.
// Not: Sabit kanal listesi {1, 6, 11} kullanılıyor; ileride konfigürasyona açılabilir.
void *channel_hopper(void *arg) {
    char *adapter = (char *)arg;
    int *channels = NULL;
    int num_channels = 0;
    int index = 0;
    volatile sig_atomic_t stop_hopping = 0;

    // --- Varsayılan Kanal Listesi ---
    int default_channels[] = {1, 6, 11};

    // --- Bellek Yönetimi ve Kanal Güncelleme ---
    if (channels) free(channels);
    num_channels = sizeof(default_channels) / sizeof(default_channels[0]);
    channels = (int *)malloc(num_channels * sizeof(int));
    if (!channels) {
        perror(MEMORY_ALLOCATION_ERROR);
        log_message(MEMORY_ALLOCATION_ERROR);

        return NULL;
    }
    memcpy(channels, default_channels, num_channels * sizeof(int));

    // --- Sinyal Yakalama (CTRL+C ile durdurma) ---
    void signal_handler(int sig) { stop_hopping = 1; }
    signal(SIGINT, signal_handler);

    // --- Kanal Değiştirme Döngüsü ---
    while (!stop_hopping) {
        char command[256];
        snprintf(command, sizeof(command), "sudo iw dev %s set channel %d", adapter, channels[index]);
        if (system(command) != 0) {
            fprintf(stderr, CHANNEL_CHANGE_ERROR, command);
            log_message(NETWORK_SC_ERROR, command);

        }
        sleep(1);
        index = (index + 1) % num_channels;
    }

    free(channels);
    printf(CHANNEL_CHANGE_STOPPED);
    log_message(CHANNEL_CHANGE_STOPPED);


    return NULL;
}

// --- Paket Yakalama ve Ayrıştırma ---

// Paket yakalama callback fonksiyonu
void packet_handler(u_char *user, const struct pcap_pkthdr *header, const u_char *packet) {
    // --- Radiotap başlığı ayrıştırması ---
    // TODO: Daha güvenilir bir ayrıştırma için dinamik radiotap parser kullanılabilir.
    uint16_t radiotap_length = packet[2] | (packet[3] << 8);
    if (header->caplen < radiotap_length + IEEE80211_HDRLEN)
        return;

    // Veri hızı (hb) hesaplaması: %100 verimlilik varsayımıyla rate_byte üzerinden
    char hb[32];
    if (header->caplen > 4 && radiotap_length > 4) {
        uint8_t rate_byte = packet[4];
        double physical_rate = rate_byte * 0.5; // Her adım 0.5 Mbps
        snprintf(hb, sizeof(hb), "%.1f Mbps", physical_rate);
    } else {
        strcpy(hb, "N/A");
    }

    // 802.11 çerçevesi: radiotap sonrasında gelen kısım
    const u_char *ieee80211_frame = packet + radiotap_length;
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)ieee80211_frame;

    char src_mac[18], dest_mac[18], bssid_mac[18];
    format_mac_address(hdr->src_mac, src_mac);
    format_mac_address(hdr->dest_mac, dest_mac);
    format_mac_address(hdr->bssid, bssid_mac);

    // Sinyal gücü: Basitleştirilmiş okuma; radiotap formatı farklılık gösterebilir
    char signal_str[5];
    int8_t signal_strength = (int8_t)packet[radiotap_length - 2];
    snprintf(signal_str, sizeof(signal_str), "%d", signal_strength);

    // 802.11 frame control alanı üzerinden tip ve subtype ayrıştırması
    uint16_t fc = hdr->frame_control;
    uint8_t type = (fc >> 2) & 0x03;
    uint8_t subtype = (fc >> 4) & 0x0F;

    // Zaman bilgisi (son görülme zamanı)
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

    // --- Yönetim Çerçeveleri (Beacon / Probe Response) ---
    if (type == 0 && (subtype == 8 || subtype == 5)) {
        size_t offset = IEEE80211_HDRLEN + 12;  // Sabit offset; bazı durumlarda değişiklik gösterebilir
        char ssid[33] = {0};
        int found_ssid = 0;
        while (offset + 2 <= header->caplen - radiotap_length) {
            uint8_t element_id = ieee80211_frame[offset];
            uint8_t element_len = ieee80211_frame[offset + 1];
            if (offset + 2 + element_len > header->caplen - radiotap_length)
                break;
            if (element_id == 0) {  // SSID bilgisi
                if (element_len > 0 && element_len <= 32) {
                    memcpy(ssid, &ieee80211_frame[offset + 2], element_len);
                    ssid[element_len] = '\0';
                } else {
                    strcpy(ssid, "Gizli SSID");
                }
                found_ssid = 1;
                break;
            }
            offset += 2 + element_len;
        }
        if (!found_ssid)
            strcpy(ssid, "Bilinmiyor");

        char enc_cipher[20] = "Open";
        char channel[4] = {0};
        offset = IEEE80211_HDRLEN + 12;
        while (offset + 2 <= header->caplen - radiotap_length) {
            uint8_t element_id = ieee80211_frame[offset];
            uint8_t element_len = ieee80211_frame[offset + 1];
            if (offset + 2 + element_len > header->caplen - radiotap_length)
                break;
            if (element_id == 3 && element_len == 1) {
                snprintf(channel, sizeof(channel), "%d", ieee80211_frame[offset + 2]);
            }
            if (element_id == 48 && element_len >= 8) {
                const u_char *group_cipher = ieee80211_frame + offset + 2 + 2;
                if (group_cipher[0] == 0x00 && group_cipher[1] == 0x0f &&
                    group_cipher[2] == 0xac) {
                    if (group_cipher[3] == 1)
                        strcpy(enc_cipher, "WEP-40");
                    else if (group_cipher[3] == 2)
                        strcpy(enc_cipher, "TKIP");
                    else if (group_cipher[3] == 4)
                        strcpy(enc_cipher, "CCMP");
                    else if (group_cipher[3] == 5)
                        strcpy(enc_cipher, "WEP-104");
                    else
                        strcpy(enc_cipher, "Unknown");
                }
            }
            if (element_id == 221 && element_len >= 4) {
                if (ieee80211_frame[offset + 2] == 0x00 &&
                    ieee80211_frame[offset + 3] == 0x50 &&
                    ieee80211_frame[offset + 4] == 0xf2 &&
                    ieee80211_frame[offset + 5] == 0x01)
                    strcpy(enc_cipher, "WPA");
            }
            offset += 2 + element_len;
        }

        // Global veri üzerinde güncelleme yapmadan önce mutex ile kilitle
        pthread_mutex_lock(&data_mutex);
        int idx = find_network_index(bssid_mac);
        if (idx != -1) {
            strncpy(networks[idx].hb, hb, sizeof(networks[idx].hb) - 1);
            networks[idx].hb[sizeof(networks[idx].hb) - 1] = '\0';
            strncpy(networks[idx].signal, signal_str, sizeof(networks[idx].signal) - 1);
            networks[idx].signal[sizeof(networks[idx].signal) - 1] = '\0';
            if (strlen(networks[idx].channel) == 0 && strlen(channel) > 0)
                strncpy(networks[idx].channel, channel, sizeof(networks[idx].channel) - 1);
            networks[idx].channel[sizeof(networks[idx].channel) - 1] = '\0';
            networks[idx].beacon_count++;
            pthread_mutex_unlock(&data_mutex);
            print_all_tables();
            return;
        }

        if (network_count < MAX_NETWORKS) {
            strncpy(networks[network_count].ssid, ssid, sizeof(networks[network_count].ssid) - 1);
            networks[network_count].ssid[sizeof(networks[network_count].ssid) - 1] = '\0';
            strncpy(networks[network_count].bssid, bssid_mac, sizeof(networks[network_count].bssid) - 1);
            networks[network_count].bssid[sizeof(networks[network_count].bssid) - 1] = '\0';
            strncpy(networks[network_count].signal, signal_str, sizeof(networks[network_count].signal) - 1);
            networks[network_count].signal[sizeof(networks[network_count].signal) - 1] = '\0';
            strncpy(networks[network_count].channel, channel, sizeof(networks[network_count].channel) - 1);
            networks[network_count].channel[sizeof(networks[network_count].channel) - 1] = '\0';
            strncpy(networks[network_count].enc_cipher, enc_cipher, sizeof(networks[network_count].enc_cipher) - 1);
            networks[network_count].enc_cipher[sizeof(networks[network_count].enc_cipher) - 1] = '\0';
            strncpy(networks[network_count].hb, hb, sizeof(networks[network_count].hb) - 1);
            networks[network_count].hb[sizeof(networks[network_count].hb) - 1] = '\0';
            networks[network_count].beacon_count = 1;
            networks[network_count].data_count = 0;
            network_count++;
        }
        pthread_mutex_unlock(&data_mutex);
    }
    // --- Veri Çerçeveleri üzerinden İstemci Tespiti ---
    else if (type == 2) {
        char client_mac[18] = "";
        if (strcmp(src_mac, bssid_mac) != 0 && strcmp(src_mac, "FF:FF:FF:FF:FF:FF") != 0) {
            strncpy(client_mac, src_mac, sizeof(client_mac) - 1);
            client_mac[sizeof(client_mac) - 1] = '\0';
        } else if (strcmp(dest_mac, bssid_mac) != 0 && strcmp(dest_mac, "FF:FF:FF:FF:FF:FF") != 0) {
            strncpy(client_mac, dest_mac, sizeof(client_mac) - 1);
            client_mac[sizeof(client_mac) - 1] = '\0';
        }
        if (strlen(client_mac) > 0) {
            pthread_mutex_lock(&data_mutex);
            int cidx = find_client_index(client_mac);
            // İlişkili AP bilgisi (varsa) alınır
            char associated_ssid[33] = "Bilinmiyor";
            int net_idx = find_network_index(bssid_mac);
            if (net_idx != -1) {
                strncpy(associated_ssid, networks[net_idx].ssid, sizeof(associated_ssid) - 1);
                associated_ssid[sizeof(associated_ssid) - 1] = '\0';
            }
            if (cidx == -1 && client_count < MAX_CLIENTS) {
                strncpy(clients[client_count].client_mac, client_mac, sizeof(clients[client_count].client_mac) - 1);
                clients[client_count].client_mac[sizeof(clients[client_count].client_mac) - 1] = '\0';
                strncpy(clients[client_count].associated_bssid, bssid_mac, sizeof(clients[client_count].associated_bssid) - 1);
                clients[client_count].associated_bssid[sizeof(clients[client_count].associated_bssid) - 1] = '\0';
                strncpy(clients[client_count].associated_ssid, associated_ssid, sizeof(clients[client_count].associated_ssid) - 1);
                clients[client_count].associated_ssid[sizeof(clients[client_count].associated_ssid) - 1] = '\0';
                strncpy(clients[client_count].signal, signal_str, sizeof(clients[client_count].signal) - 1);
                clients[client_count].signal[sizeof(clients[client_count].signal) - 1] = '\0';
                strncpy(clients[client_count].hb, hb, sizeof(clients[client_count].hb) - 1);
                clients[client_count].hb[sizeof(clients[client_count].hb) - 1] = '\0';
                clients[client_count].packet_count = 1;
                strncpy(clients[client_count].last_seen, time_str, sizeof(clients[client_count].last_seen) - 1);
                clients[client_count].last_seen[sizeof(clients[client_count].last_seen) - 1] = '\0';
                client_count++;
            } else if (cidx != -1) {
                clients[cidx].packet_count++;
                strncpy(clients[cidx].signal, signal_str, sizeof(clients[cidx].signal) - 1);
                clients[cidx].signal[sizeof(clients[cidx].signal) - 1] = '\0';
                strncpy(clients[cidx].hb, hb, sizeof(clients[cidx].hb) - 1);
                clients[cidx].hb[sizeof(clients[cidx].hb) - 1] = '\0';
                strncpy(clients[cidx].last_seen, time_str, sizeof(clients[cidx].last_seen) - 1);
                clients[cidx].last_seen[sizeof(clients[cidx].last_seen) - 1] = '\0';
                int net_idx2 = find_network_index(bssid_mac);
                if (net_idx2 != -1) {
                    strncpy(clients[cidx].associated_ssid, networks[net_idx2].ssid, sizeof(clients[cidx].associated_ssid) - 1);
                    clients[cidx].associated_ssid[sizeof(clients[cidx].associated_ssid) - 1] = '\0';
                }
            }
            pthread_mutex_unlock(&data_mutex);
        }
    }

    // Tabloları güncelle
    print_all_tables();
}

// --- Monitor Modunda Paket Yakalama ---

int packet_shot() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = NULL;
    char adapters[MAX_ADAPTERS][MAX_ADAPTER_NAME_LENGTH];
    int adapter_count = 0;

    // Ağ adaptörlerini listeleme: Linux “iw dev” komutu kullanılıyor
    FILE *fp = popen("iw dev | grep Interface | awk '{print $2}'", "r");
    if (fp == NULL) {
        perror(ADAPTER_NAME_ERROR);
        log_message(ADAPTER_NAME_ERROR);

        return -1;
    }

    char buffer[MAX_ADAPTER_NAME_LENGTH];
    while (fgets(buffer, sizeof(buffer), fp) != NULL && adapter_count < MAX_ADAPTERS) {
        buffer[strcspn(buffer, "\n")] = '\0';
        strncpy(adapters[adapter_count], buffer, MAX_ADAPTER_NAME_LENGTH - 1);
        adapters[adapter_count][MAX_ADAPTER_NAME_LENGTH - 1] = '\0';
        adapter_count++;
    }
    pclose(fp);

    if (adapter_count == 0) {
        printf("Hiçbir kablosuz ağ adaptörü bulunamadı!\n");
        return -1;
    }

    // Kullanıcıya adaptör seçim imkanı
    printf("Bulunan kablosuz ağ adaptörleri:\n");
    for (int i = 0; i < adapter_count; i++) {
        printf("%d. %s\n", i + 1, adapters[i]);
    }

    int choice;
    printf("Kullanmak istediğiniz adaptörü seçin (1-%d): ", adapter_count);
    if (scanf("%d", &choice) != 1) {
        fprintf(stderr, "Geçersiz giriş!\n");
        return -1;
    }
    if (choice < 1 || choice > adapter_count) {
        printf("Geçersiz seçim!\n");
        return -1;
    }

    char *selected_adapter = adapters[choice - 1];
    printf("Seçilen adaptör: %s\n", selected_adapter);

    // Adaptörü monitor moda al
    if (set_monitor_mode(selected_adapter) != 0) {
        return -1;
    }

    // Kanal değiştirme (channel hopping) için ayrı iş parçacığı başlatılıyor
    char *adapter_copy = strdup(selected_adapter);
    if (adapter_copy == NULL) {
        perror(MEMORY_ALLOCATION_ERROR);
        log_message(MEMORY_ALLOCATION_ERROR);

        return -1;
    }
    pthread_t hopper_thread;
    if (pthread_create(&hopper_thread, NULL, channel_hopper, (void*)adapter_copy) != 0) {
        fprintf(stderr, THREAD_CREATION_ERROR);
        log_message(THREAD_CREATION_ERROR);
        free(adapter_copy);
        return -1;
    }
    pthread_detach(hopper_thread);

    // pcap ile paket yakalama başlatılıyor
    handle = pcap_open_live(selected_adapter, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Ağ adaptörü açılamadı: %s\n", errbuf);
        return -1;
    }
    printf("Monitör modunda paket yakalama başlatıldı (%s üzerinde)...\n", selected_adapter);

    print_all_tables();
    int loop_result = pcap_loop(handle, 0, packet_handler, NULL);
    if (loop_result == -1) {
        fprintf(stderr, PACKET_CAPTURE_ERROR, pcap_geterr(handle));
        log_message(PACKET_CAPTURE_ERROR, pcap_geterr(handle));

    }

    pcap_close(handle);
    return 0;
}

#endif // MONITOR_MODE_SCAN_H
