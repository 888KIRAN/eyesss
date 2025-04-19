#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "errors.h"

#define LOG_FILE "app.log"

// Log mesajlarını dosyaya yazan fonksiyon
void log_message(const char *format, ...) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (!log_file) return;

    // Zaman damgası ekleme
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_buf[20];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", t);

    // Log mesajını al ve yazdır
    va_list args;
    va_start(args, format);
    
    fprintf(log_file, "[%s] ", time_buf); // Zaman damgası ekleme
    vfprintf(log_file, format, args);
    fprintf(log_file, "\n");

    va_end(args);
    fclose(log_file);
}

#endif // LOGGER_H
