#ifndef APP_CHECKER_H
#define APP_CHECKER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "printing.h"

void check_and_install_arp_oui() {
    // ARP komutunun olup olmadığını kontrol et
    if (access("/usr/sbin/arp", X_OK) == 0 || access("/sbin/arp", X_OK) == 0) {
        printf("\nARP komutu bulundu.\n");
    } else {
        printf("ARP komutu bulunamadı, yükleniyor...\n");

        if (system("which apt > /dev/null 2>&1") == 0) {
            system("sudo apt update && sudo apt install -y net-tools");
        } 
        else if (system("which pacman > /dev/null 2>&1") == 0) {
            system("sudo pacman -S --noconfirm net-tools");
        } 
        else if (system("which dnf > /dev/null 2>&1") == 0) {
            system("sudo dnf install -y net-tools");
        } 
        else {
            printf("Desteklenmeyen bir paket yöneticisi. Manuel yükleme gerekli.\n");
            printf("Örneğin, şu komutları deneyebilirsin:\n");
            printf("Debian tabanlı: sudo apt install net-tools\n");
            printf("Arch tabanlı: sudo pacman -S net-tools\n");
            printf("Fedora tabanlı: sudo dnf install net-tools\n");
        }
    }

    // oui.txt dosyasının olup olmadığını kontrol et
    if (access("oui.txt", F_OK) == 0) {
        printf("Dosya zaten mevcut, devam ediliyor...\n");
    } else {
        printf("Dosya mevcut değil, indiriliyor...\n");
        // wget kullanarak dosyayı indir
        int status = system("wget https://standards-oui.ieee.org/oui/oui.txt");
        if (status == 0) {
            printf("Dosya başarıyla indirildi.\n");
        } else {
            printf("Dosya indirilemedi! Lütfen internet bağlantınızı kontrol edin.\n");
        }
    }
}

#endif
