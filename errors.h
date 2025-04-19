#ifndef ERRORS_H
#define ERRORS_H

//monitor_mode_scan.h 
#define RF_KILL_ERROR "RF-kill komutu başarısız.\n"
#define ADAPTER_CLOSE_ERROR "Adapter kapatılamadı: %s\n"
#define MONITOR_MODE_ERROR "Monitor moda geçiş başarısız: %s\n"
#define ADAPTER_OPEN_ERROR "Adapter açma başarısız: %s\n"
#define MEMORY_ALLOCATION_ERROR "Bellek tahsisi başarısız"
#define CHANNEL_CHANGE_ERROR "Kanal değiştirme hatası: %s\n"
#define CHANNEL_CHANGE_STOPPED "Kanal değiştirme işlemi durduruldu.\n"
#define ADAPTER_NAME_ERROR "Ağ adaptör adı alınamadı"
#define THREAD_CREATION_ERROR "Channel hopping thread oluşturulamadı.\n"
#define PACKET_CAPTURE_ERROR "Paket yakalama sırasında hata oluştu: %s\n"

//terminal.h +
#define EXIT_MESSAGE "\n\x1B[35mExiting...\x1B[0m\n"
#define UNKNOWN_COMMAND_WARNING "\a\n[\x1B[34mWARNING\x1B[0m] Unknown command! Available commands: 'help'"

//interface.h +
#define MEMORY_ALLOCATION_FAILED "[\x1B[34mERROR\x1B[0m] Memory allocation failed!\n"
#define CURL_INIT_WARNING "[\x1B[33mWARNING\x1B[0m] curl could not be initialized. Continuing without network request!\n"

//network_scan.h +
#define FILE_OPEN_ERROR "Dosya açılamadı"
#define CURL_ERROR "CURL hatası: %s\n"
#define CURL_INIT_FAILED "CURL başlatılamadı.\n"
#define POPEN_FAILED "popen failed\n"
#define NETWORK_SC_ERROR "\033[1;31m[ERROR]\033[0m : <network IP> -Sc\n"

#endif 