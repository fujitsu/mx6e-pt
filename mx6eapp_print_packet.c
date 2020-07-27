/******************************************************************************/
/* ファイル名 : mx6eapp_print_packet.c                                        */
/* 機能概要   : デバッグ用パケット表示関数 ソースファイル                     */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/ether.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include "mx6eapp_print_packet.h"
#include "mx6eapp_log.h"

////////////////////////////////////////////////////////////////////////////////
// 内部関数プロトタイプ宣言
////////////////////////////////////////////////////////////////////////////////
#if defined(DEBUG)
static void                     print_ipv4header(char *frame);
static void                     print_ipv6header(char *frame);
static void                     print_tcpheader(char *frame);
static void                     print_udpheader(char *frame);
static void                     print_icmpheader(char *frame);
#endif

///////////////////////////////////////////////////////////////////////////////
//! @brief パケット表示関数
//!
//! Etherフレームからのパケットをテキストで表示する。
//!
//! @param [in]  packet     表示するパケットの先頭ポインタ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
void mx6e_print_packet(char *packet)
{
#if defined(DEBUG)
	struct ethhdr                  *p_ether;

	p_ether = (struct ethhdr *) packet;

	DEBUG_LOG("Ether SrcAdr = %s\n", ether_ntoa((struct ether_addr *) &p_ether->h_source));
	DEBUG_LOG("Ether DstAdr = %s\n", ether_ntoa((struct ether_addr *) &p_ether->h_dest));
	DEBUG_LOG("Ether Type = 0x%04x\n", ntohs(p_ether->h_proto));

	if (ntohs(p_ether->h_proto) == ETH_P_IPV6) {
		print_ipv6header(packet + sizeof(struct ethhdr));
	} else {
		print_ipv4header(packet + sizeof(struct ethhdr));
	}
#endif
}

#if defined(DEBUG)
///////////////////////////////////////////////////////////////////////////////
//! @brief IPv4パケット表示関数
//!
//! IPv4パケットをテキストで表示する。
//!
//! @param [in]  frame     表示するパケットの先頭ポインタ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
void print_ipv4header(char *frame)
{
	struct iphdr                   *p_ip;
	struct protoent                *p_proto;
	struct in_addr                  insaddr;
	struct in_addr                  indaddr;

	p_ip = (struct iphdr *) (frame);

	insaddr.s_addr = p_ip->saddr;
	indaddr.s_addr = p_ip->daddr;

	DEBUG_LOG("----IPv4 Header--------------------\n");
	DEBUG_LOG("version : %u\n", p_ip->version);
	DEBUG_LOG("ihl : %u\n", p_ip->ihl);
	DEBUG_LOG("tos : %u\n", p_ip->tos);
	DEBUG_LOG("tot length : %u\n", ntohs(p_ip->tot_len));
	DEBUG_LOG("id : %u\n", ntohs(p_ip->id));
	DEBUG_LOG("frag_off : %04x\n", htons(p_ip->frag_off));
	DEBUG_LOG("ttl : %u\n", p_ip->ttl);
	if ((p_proto = getprotobynumber(p_ip->protocol)) != NULL) {
		DEBUG_LOG("protocol : %x(%s)\n", p_ip->protocol, p_ip->protocol ? p_proto->p_name : "hopopt");
	} else {
		DEBUG_LOG("protocol : %x(unknown)\n", p_ip->protocol);
	}
	DEBUG_LOG("check : 0x%x\n", ntohs(p_ip->check));
	DEBUG_LOG("saddr : %s\n", inet_ntoa(insaddr));
	DEBUG_LOG("daddr : %s\n", inet_ntoa(indaddr));

	if (p_ip->protocol == IPPROTO_ICMP) {
		print_icmpheader(frame + sizeof(struct iphdr));
	}
	if (p_ip->protocol == IPPROTO_TCP) {
		print_tcpheader(frame + sizeof(struct iphdr));
	}
	if (p_ip->protocol == IPPROTO_UDP) {
		print_udpheader(frame + sizeof(struct iphdr));
	}
	DEBUG_LOG("\n");
}

///////////////////////////////////////////////////////////////////////////////
//! @brief IPv6パケット表示関数
//!
//! IPv6パケットをテキストで表示する。
//!
//! @param [in]  frame     表示するパケットの先頭ポインタ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
void print_ipv6header(char *frame)
{
	struct ip6_hdr                 *p_ip;
	struct protoent                *p_proto;
	char                            saddr[INET6_ADDRSTRLEN];
	char                            daddr[INET6_ADDRSTRLEN];

	p_ip = (struct ip6_hdr *) (frame);

	DEBUG_LOG("----IPv6 Header--------------------\n");
	DEBUG_LOG("version : %u\n", p_ip->ip6_vfc >> 4);
	DEBUG_LOG("traffic class : %x\n", p_ip->ip6_vfc & 0x0f);
	DEBUG_LOG("flow label : %05x\n", ntohl(p_ip->ip6_flow) & 0x000fffff);
	DEBUG_LOG("payload_len : %u\n", ntohs(p_ip->ip6_plen));
	DEBUG_LOG("hop_limit : %u\n", p_ip->ip6_hops);
	if ((p_proto = getprotobynumber(p_ip->ip6_nxt)) != NULL) {
		DEBUG_LOG("protocol : %x(%s)\n", p_ip->ip6_nxt, p_ip->ip6_nxt ? p_proto->p_name : "hopopt");
	} else {
		DEBUG_LOG("protocol : %x(unknown)\n", p_ip->ip6_nxt);
	}
	DEBUG_LOG("saddr : %s\n", inet_ntop(AF_INET6, &p_ip->ip6_src, saddr, INET6_ADDRSTRLEN));
	DEBUG_LOG("daddr : %s\n", inet_ntop(AF_INET6, &p_ip->ip6_dst, daddr, INET6_ADDRSTRLEN));

	if (p_ip->ip6_nxt == IPPROTO_TCP) {
		print_tcpheader(frame + sizeof(struct ip6_hdr));
	}
	if (p_ip->ip6_nxt == IPPROTO_UDP) {
		print_udpheader(frame + sizeof(struct ip6_hdr));
	}
	if (p_ip->ip6_nxt == IPPROTO_IPIP) {
		print_ipv4header(frame + sizeof(struct ip6_hdr));
	}
	DEBUG_LOG("\n");
}

///////////////////////////////////////////////////////////////////////////////
//! @brief TCPパケット表示関数
//!
//! TCPパケットをテキストで表示する。
//!
//! @param [in]  frame     表示するパケットの先頭ポインタ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
static void print_tcpheader(char *frame)
{
	struct tcphdr                  *p_tcp;
	char                            tmp[256] = { 0 };

	p_tcp = (struct tcphdr *) (frame);

	p_tcp->fin ? strcat(tmp, " FIN") : 0;
	p_tcp->syn ? strcat(tmp, " SYN") : 0;
	p_tcp->rst ? strcat(tmp, " RST") : 0;
	p_tcp->psh ? strcat(tmp, " PSH") : 0;
	p_tcp->ack ? strcat(tmp, " ACK") : 0;
	p_tcp->urg ? strcat(tmp, " URG") : 0;

	DEBUG_LOG("----TCP Header-------------------\n");
	DEBUG_LOG("source port : %u\n", ntohs(p_tcp->source));
	DEBUG_LOG("dest port : %u\n", ntohs(p_tcp->dest));
	DEBUG_LOG("sequence : %u\n", ntohl(p_tcp->seq));
	DEBUG_LOG("ack seq : %u\n", ntohl(p_tcp->ack_seq));
	DEBUG_LOG("frags :%s\n", tmp);
	DEBUG_LOG("window : %u\n", ntohs(p_tcp->window));
	DEBUG_LOG("check : 0x%x\n", ntohs(p_tcp->check));
	DEBUG_LOG("urt_ptr : %u\n", p_tcp->urg_ptr);
}

///////////////////////////////////////////////////////////////////////////////
//! @brief UDPパケット表示関数
//!
//! UDPパケットをテキストで表示する。
//!
//! @param [in]  frame     表示するパケットの先頭ポインタ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
static void print_udpheader(char *frame)
{
#if	defined(DEBUG)	
	struct udphdr                  *p_udp;

	p_udp = (struct udphdr *) (frame);

	DEBUG_LOG("----UDP Header-------------------\n");
	DEBUG_LOG("source port : %u\n", ntohs(p_udp->source));
	DEBUG_LOG("dest port : %u\n", ntohs(p_udp->dest));
	DEBUG_LOG("length : %u\n", ntohs(p_udp->len));
	DEBUG_LOG("check : 0x%x\n", ntohs(p_udp->check));
#endif
}

///////////////////////////////////////////////////////////////////////////////
//! @brief ICMPv4パケット表示関数
//!
//! ICMPv4パケットをテキストで表示する。
//!
//! @param [in]  frame     表示するパケットの先頭ポインタ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
static void print_icmpheader(char *frame)
{
	struct icmphdr                 *p_icmp;
	char                            tmp[256];

	p_icmp = (struct icmphdr *) (frame);

	switch (p_icmp->type) {
	case ICMP_ECHOREPLY:
		sprintf(tmp, "Echo Reply");
		break;
	case ICMP_DEST_UNREACH:
		sprintf(tmp, "Destination Unreachable");
		break;
	case ICMP_SOURCE_QUENCH:
		sprintf(tmp, "Source Quench");
		break;
	case ICMP_REDIRECT:
		sprintf(tmp, "Redirect (change route)");
		break;
	case ICMP_ECHO:
		sprintf(tmp, "Echo Request");
		break;
	case ICMP_TIME_EXCEEDED:
		sprintf(tmp, "Time Exceeded");
		break;
	case ICMP_PARAMETERPROB:
		sprintf(tmp, "Parameter Problem");
		break;
	case ICMP_TIMESTAMP:
		sprintf(tmp, "Timestamp Request");
		break;
	case ICMP_TIMESTAMPREPLY:
		sprintf(tmp, "Timestamp Reply");
		break;
	case ICMP_INFO_REQUEST:
		sprintf(tmp, "Information Request");
		break;
	case ICMP_INFO_REPLY:
		sprintf(tmp, "Information Reply");
		break;
	case ICMP_ADDRESS:
		sprintf(tmp, "Address Mask Request");
		break;
	case ICMP_ADDRESSREPLY:
		sprintf(tmp, "Address Mask Reply");
		break;
	default:
		sprintf(tmp, "unknown");
		break;
	}

	DEBUG_LOG("----ICMP Header------------------\n");
	DEBUG_LOG("type : %u(%s)\n", p_icmp->type, tmp);
	DEBUG_LOG("code : %u\n", p_icmp->code);
	DEBUG_LOG("checksum : 0x%x\n", ntohs(p_icmp->checksum));

	if (p_icmp->type == ICMP_DEST_UNREACH) {
		print_ipv4header(frame + sizeof(struct icmphdr));
	}
}
#endif
