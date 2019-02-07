#ifndef _MAPPER_H
#define _MAPPER_H
#include <stdbool.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

/* Ethernet header */
typedef struct {
    struct ether_addr  eth_dhost;    /* destination host address */
    struct ether_addr  eth_shost;    /* source host address */
    uint16_t eth_type;                /* IP? ARP? RARP? etc */
} mapper_ethernet_t __attribute__ ((aligned (1)));;

/* IP header */
typedef struct {
    union {
        uint8_t ip_vhl;
        struct {
            uint8_t ip_ihl:4;               /* header length */
            uint8_t ip_version:4;           /* version */
        };
    };
    uint8_t ip_tos;                 /* type of service */
    uint16_t ip_len;                 /* total length */
    uint16_t ip_id;                  /* identification */
    uint16_t ip_off;                 /* fragment offset field */
    #define IP_RF 0x8000            /* reserved fragment flag */
    #define IP_DF 0x4000            /* dont fragment flag */
    #define IP_MF 0x2000            /* more fragments flag */
    #define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
    uint8_t ip_ttl;                 /* time to live */
    uint8_t ip_protocol;            /* protocol */
    uint16_t ip_sum;                 /* checksum */
    struct in_addr ip_src,ip_dst;  /* source and dest address */
} mapper_ip_t __attribute__ ((aligned (1)));;

/* TCP header */
typedef uint32_t tcp_seq;

typedef struct {
    uint16_t tcp_sport;               /* source port */
    uint16_t tcp_dport;               /* destination port */
    tcp_seq tcp_seq;                 /* sequence number */
    tcp_seq tcp_ack;                 /* acknowledgement number */
    uint8_t tcp_offx2;               /* data offset, rsvd */
    #define TH_OFF(th) (((th)->th_offx2 & 0xf0) >> 4)
    uint8_t tcp_flags;
    #define TH_FIN  0x01
    #define TH_SYN  0x02
    #define TH_RST  0x04
    #define TH_PUSH 0x08
    #define TH_ACK  0x10
    #define TH_URG  0x20
    #define TH_ECE  0x40
    #define TH_CWR  0x80
    #define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
    uint16_t tcp_window;              /* window */
    uint16_t tcp_sum;                 /* checksum */
    uint16_t tcp_urp;                 /* urgent pointer */
} mapper_tcp_t __attribute__ ((aligned (1)));;

/* UDP header */
typedef struct {
    uint16_t udp_sport;               /* source port */
    uint16_t udp_dport;               /* destination port */
    uint16_t udp_len;               	/* length */
    uint16_t udp_sum;                 /* checksum */
} mapper_udp_t __attribute__ ((aligned (1)));;

/* ICMP header */
typedef struct {
    uint8_t icmp_type;               /* ICMP type */
    uint8_t icmp_code;               /* ICMP subtype */
    uint16_t icmp_sum;               /* checksum */
    uint32_t icmp_header;			  /* rest of header */
} mapper_icmp __attribute__ ((aligned (1)));;

typedef struct {
    mapper_ethernet_t _ethernet;
    mapper_ip_t _ip;
    char _payload[33];
} mapper_t __attribute__ ((aligned (1)));;

bool mapFrame(unsigned char *frame, size_t framelen, mapper_t* presults);
#endif
