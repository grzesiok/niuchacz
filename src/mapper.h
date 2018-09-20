#ifndef _MAPPER_H
#define _MAPPER_H
#include <stdbool.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

/* ethernet headers are always exactly 14 bytes */
#define SIZE_ETHERNET 14

/* Ethernet header */
struct mapper_ethernet {
	struct ether_addr  eth_dhost;    /* destination host address */
	struct ether_addr  eth_shost;    /* source host address */
	u_short eth_type;                /* IP? ARP? RARP? etc */
};

/* IP header */
struct mapper_ip {
	u_char  ip_vhl;                 /* version << 4 | header length >> 2 */
	u_char  ip_tos;                 /* type of service */
	u_short ip_len;                 /* total length */
	u_short ip_id;                  /* identification */
	u_short ip_off;                 /* fragment offset field */
 	#define IP_RF 0x8000            /* reserved fragment flag */
	#define IP_DF 0x4000            /* dont fragment flag */
	#define IP_MF 0x2000            /* more fragments flag */
	#define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
	u_char  ip_ttl;                 /* time to live */
	u_char  ip_p;                   /* protocol */
	u_short ip_sum;                 /* checksum */
	struct  in_addr ip_src,ip_dst;  /* source and dest address */
};
#define IP_HL(ip) (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip) (((ip)->ip_vhl) >> 4)

/* TCP header */
typedef u_int tcp_seq;

struct mapper_tcp {
	u_short tcp_sport;               /* source port */
	u_short tcp_dport;               /* destination port */
	tcp_seq tcp_seq;                 /* sequence number */
	tcp_seq tcp_ack;                 /* acknowledgement number */
	u_char  tcp_offx2;               /* data offset, rsvd */
#define TH_OFF(th) (((th)->th_offx2 & 0xf0) >> 4)
	u_char  tcp_flags;
	#define TH_FIN  0x01
	#define TH_SYN  0x02
	#define TH_RST  0x04
	#define TH_PUSH 0x08
	#define TH_ACK  0x10
	#define TH_URG  0x20
	#define TH_ECE  0x40
	#define TH_CWR  0x80
	#define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
	u_short tcp_window;              /* window */
	u_short tcp_sum;                 /* checksum */
	u_short tcp_urp;                 /* urgent pointer */
};

/* UDP header */
struct mapper_udp {
	u_short udp_sport;               /* source port */
	u_short udp_dport;               /* destination port */
	u_short udp_len;               	/* length */
	u_short udp_sum;                 /* checksum */
};

/* ICMP header */
struct mapper_icmp {
	u_char icmp_type;               /* ICMP type */
	u_char icmp_code;               /* ICMP subtype */
	u_short icmp_sum;               /* checksum */
	u_int icmp_header;			  /* rest of header */
};

typedef struct _MAPPER_RESULTS {
	struct mapper_ethernet _ethernet;
	struct mapper_ip _ip;
        char _payload_16b[17];
} MAPPER_RESULTS, *PMAPPER_RESULTS;

bool mapFrame(unsigned char *frame, size_t framelen, PMAPPER_RESULTS presults);
#endif
