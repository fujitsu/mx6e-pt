/******************************************************************************/
/* ファイル名 : mx6eapp_ct.c                                                  */
/* 機能概要   : 単体試験                                                      */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/

#include "mx6eapp_ct.h"

#if defined(CT)
#   include <stdlib.h>
#	include <string.h>
#   include "mx6eapp_pt.h"
#   include "mx6eapp_config.h"
#   include "mx6eapp_print_packet.h"
#   include "mx6eapp_tunnel.h"
#   include "mx6eapp_util.h"
#	include "mx6ectl_command.h"

static void CT_get_pid_bit_width(void)
{
	char                            buf[128];
	struct in6_addr pid;
	
	inet_pton(AF_INET6, "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", &pid);
	inet_ntop(AF_INET6, &pid, buf, sizeof(buf));

	for (int i = 0; i < sizeof(pid.s6_addr); i ++) {
		printf("%02x ", pid.s6_addr[i]);
	}
	printf("\n");
	printf("%s : %d\n", buf, get_pid_bit_width(buf));

	inet_pton(AF_INET6, "0fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", &pid);
	inet_ntop(AF_INET6, &pid, buf, sizeof(buf));
	for (int i = 0; i < sizeof(pid.s6_addr); i ++) {
		printf("%02x ", pid.s6_addr[i]);
	}
	printf("\n");
	printf("%s : %d\n", buf, get_pid_bit_width(buf));

	inet_pton(AF_INET6, "::70:1", &pid);
	inet_ntop(AF_INET6, &pid, buf, sizeof(buf));
	for (int i = 0; i < sizeof(pid.s6_addr); i ++) {
		printf("%02x ", pid.s6_addr[i]);
	}
	printf("\n");
	printf("%s : %d\n", buf, get_pid_bit_width(buf));

	inet_pton(AF_INET6, "::1", &pid);
	inet_ntop(AF_INET6, &pid, buf, sizeof(buf));
	for (int i = 0; i < sizeof(pid.s6_addr); i ++) {
		printf("%02x ", pid.s6_addr[i]);
	}
	printf("\n");
	printf("%s : %d\n", buf, get_pid_bit_width(buf));

	return;
}

static void CT_m46e_pt_add_config_entry(mx6e_handler_t * handler)
{
	mx6e_command_t	command;

#define	ENABLE	"enable"

	// テストエントリ
	// *INDENT-OFF*
	struct {
		mx6e_config_table_t *table;
		mx6e_command_code_t code;
		char *opt[7];
	} cmd[] = {
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"1",		"16",	"ab:cd:ef:01:23:45",	"f00d:1:a::/48",	"1:1",	ENABLE}},	// OK
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"1:1",		"16",	"ab:cd:ef:01:23:45",	"f00d:1:b::/48",	"1:2",	ENABLE}},	// OK
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"1:1:1",	"16",	"ab:cd:ef:01:23:45",	"f00d:1:c::/48",	"1:3",	ENABLE}},	// OK
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"1:1:1:1",	"16",	"ab:cd:ef:01:23:45",	"f00d:1:d::/48",	"1:4",	ENABLE}},	// OK
		{NULL, MX6E_COMMAND_NONE, {NULL}},
	}, cmd_a[] = {
		// Me6E prefix 最大79bit + 最小planeid  1bit + MAC48
		// Me6E prefix 最小1bit  + 最大planeid 79bit + MAC48
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"1",	"1",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"1:1",	ENABLE}},	// OK
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"pr",	"1",	"1",	"ab:cd:ef:01:23:45",	"f00d:1:2::/48",	"2:1",	ENABLE}},	// OK
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"1:1",	"62",	"ab:cd:ef:01:23:45",	"f00d:1:3::/48",	"3:1",	ENABLE}},
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"pr",	"1:2",	"62",	"ab:cd:ef:01:23:45",	"f00d:1:4::/48",	"4:1",	ENABLE}},
		// すでに同じとみなされるエントリがあるので重複
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"1",	"79",	"ab:cd:ef:01:23:45",	"f00d:1:3::/48",	"3:1",	ENABLE}},
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"pr",	"1",	"79",	"ab:cd:ef:01:23:45",	"f00d:1:4::/48",	"4:1",	ENABLE}},
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"1:1",	"1",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"1:1",	ENABLE}},	// OK
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"pr",	"1:2",	"1",	"ab:cd:ef:01:23:45",	"f00d:1:2::/48",	"2:1",	ENABLE}},	// OK
		
		// M46E prefix 最大95bit + 最小planeid  1bit + IPv4 32
		// M46E prefix 最小1bit  + 最大planeid 95bit + IPv4 32
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"fp",	"1",	"1",	"192.168.100.0/24",		"f00d:1:5::/48",	"5:1",	ENABLE}},
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"pr",	"1",	"1",	"192.168.100.0/24",		"f00d:1:6::/48",	"6:1",	ENABLE}},
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"fp",	"1:3",	"78",	"192.168.100.0/24",		"f00d:1:5::/48",	"5:1",	ENABLE}},
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"pr",	"1:4",	"78",	"192.168.100.0/24",		"f00d:1:6::/48",	"6:1",	ENABLE}},
		// すでに同じとみなされるエントリがあるので重複
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"fp",	"1",	"95",	"192.168.100.0/24",		"f00d:1:7::/48",	"7:1",	ENABLE}},
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"pr",	"1",	"95",	"192.168.100.0/24",		"f00d:1:8::/48",	"8:1",	ENABLE}},
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"fp",	"1:3",	"1",	"192.168.100.0/24",		"f00d:1:5::/48",	"5:1",	ENABLE}},
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"pr",	"1:4",	"1",	"192.168.100.0/24",		"f00d:1:6::/48",	"6:1",	ENABLE}},
		{NULL, MX6E_COMMAND_NONE, {NULL}},
	}, 
	cmd_b[] = {
		// Me6E prefix 最大79bit + 最小planeid  1bit + MAC48
		// Me6E prefix 最小1bit  + 最大planeid 79bit + MAC48
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"1",						"48",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:4",	ENABLE}},	// OK
		// {&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"1",						"79",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:5",	ENABLE}},	// OK
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"7fff:ffff:ffff:ffff:ffff",	"1",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:6",	ENABLE}},	// OK
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"ffff:ffff:ffff:ffff:ffff",	"1",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:7",	ENABLE}},	// NG
		
		// {&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"pr",	"1",			"1",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:8",	ENABLE}},	// OK
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"pr",	"1",			"48",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:9",	ENABLE}},	// OK
		// {&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"pr",	"1",			"79",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:a",	ENABLE}},	// OK
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"pr",	"1",			"80",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:b",	ENABLE}},	// NG
																		                
		// M46E prefix 最大95bit + 最小planeid  1bit + IPv4 32
		// M46E prefix 最小1bit  + 最大planeid 95bit + IPv4 32
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"pr",	"1",			"1",	"192.168.100.0/24",	"f00d:1:1::/48",	"4:4",	ENABLE}},	// OK
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"pr",	"1",			"95",	"192.168.100.0/24",	"f00d:1:1::/48",	"4:5",	ENABLE}},	// OK
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"pr",	"1",			"96",	"192.168.100.0/24",	"f00d:1:1::/48",	"4:6",	ENABLE}},	// NG
																		                
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"fp",	"1",								"95",	"192.168.100.0/24",	"f00d:1:1::/48",	"4:7",	ENABLE}},	// OK
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"fp",	"7fff:ffff:ffff:ffff:ffff:ffff",	"1",	"192.168.100.0/24",	"f00d:1:1::/48",	"4:8",	ENABLE}},	// OK
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"fp",	"ffff:ffff:ffff:ffff:ffff:ffff",	"1",	"192.168.100.0/24",	"f00d:1:1::/48",	"4:9",	ENABLE}},	// NG

		
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"1",			"65",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:10",	ENABLE}},
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"pr",	"1",			"65",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:11",	ENABLE}},
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"2",			"64",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:12",	ENABLE}},
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"pr",	"8000",			"64",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:13",	ENABLE}},	// 128-48-64 = 16, 8000 = 16bit
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"8000",			"64",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:14",	ENABLE}},
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"pr",	"1:ffff",		"64",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:15",	ENABLE}},	// 128-48-64 = 16, 1:ffff = 17bit -> NG
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"1:ffff",		"64",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"e:16",	ENABLE}},
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"pr",	"1:2",			"64",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"8fff:ffff:ffff",	ENABLE}},	// 128-32-48 = 48
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"1:2",			"64",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"8fff:ffff:ffff",	ENABLE}},
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"pr",	"1:2",			"64",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"1:ffff:ffff:ffff",	ENABLE}},	// 128-32-48 = 48, 1:ffff:ffff:ffff = 49bit -> NG
		{&handler->conf.me6e_conf_table, MX6E_ADD_ME6E_ENTRY, {"fp",	"1:2",			"64",	"ab:cd:ef:01:23:45",	"f00d:1:1::/48",	"1:ffff:ffff:ffff",	ENABLE}},

		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"pr",	"1:2",			"64",	"192.168.100.0/24",		"f00d:1:1::/48",	"4:7",	ENABLE}},
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"fp",	"1:2",			"64",	"192.168.100.0/24",		"f00d:1:1::/48",	"4:8",	ENABLE}},
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"pr",	"8000:1234",	"64",	"192.168.101.0/24",		"f00d:1:1::/48",	"4:9",	ENABLE}},	// 128-32-64 = 32, 8000:1234 = 32bit
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"fp",	"8000:1234",	"64",	"192.168.101.0/24",		"f00d:1:1::/48",	"4:a",	ENABLE}},
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"pr",	"1:ffff:1234",	"64",	"192.168.101.0/24",		"f00d:1:1::/48",	"4:b",	ENABLE}},	// 128-32-64 = 32, 1:ffff:1234 = 33bit -> NG
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"fp",	"1:ffff:1234",	"64",	"192.168.101.0/24",		"f00d:1:1::/48",	"4:c",	ENABLE}},
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"pr",	"1:2",			"64",	"192.168.102.0/24",		"f00d:1:1::/48",	"8fff:ffff:ffff",	ENABLE}},	// 128-32-48 = 48
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"fp",	"1:2",			"64",	"192.168.102.0/24",		"f00d:1:1::/48",	"8fff:ffff:ffff",	ENABLE}},
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"pr",	"1:2",			"64",	"192.168.103.0/24",		"f00d:1:1::/48",	"1:ffff:ffff:ffff",	ENABLE}},	// 128-32-48 = 48, 1:ffff:ffff:ffff = 49bit -> NG
		{&handler->conf.m46e_conf_table, MX6E_ADD_M46E_ENTRY, {"fp",	"1:2",			"64",	"192.168.103.0/24",		"f00d:1:1::/48",	"1:ffff:ffff:ffff",	ENABLE}},
		{NULL, MX6E_COMMAND_NONE, {NULL}},
	};
	// *INDENT-ON*

	int m46e_item = 0;
	int me6e_item = 0;
	for (int i = 0; cmd[i].table; i ++) {
		if (MX6E_ADD_M46E_ENTRY == cmd[i].code) {
			m46e_item ++;
		}
		if (MX6E_ADD_ME6E_ENTRY == cmd[i].code) {
			me6e_item ++;
		}
		printf("****************************************\n");
		printf("* CT_m46e_pt_add_config_entry %3d * ", i);
		command.code = cmd[i].code;
		printf("%s ", get_command_name(command.code));
		for (int n = 0; n < 7; n ++) {
			printf("%s\t", cmd[i].opt[n]);
		}
		printf("\n");
		printf("in  pid %s %d[bit]\n", cmd[i].opt[1], get_pid_bit_width(cmd[i].opt[1]));
		printf("out pid %s %d[bit]\n", cmd[i].opt[5], get_pid_bit_width(cmd[i].opt[5]));
		mx6e_command_add_entry_option(7, cmd[i].opt, &command);	// mx6ectlのコマンドライン分解関数でmx6e_config_entry_tを作成
		bool r = m46e_pt_add_config_entry(cmd[i].table, &command.req.mx6e_data.entry, &handler->conf.devices);
		if (r) {
			printf("m46e_pt_add_config_entry add\n");
		} else {
			printf("m46e_pt_add_config_entry fail\n");
		}
		
	}

	printf("****************************************\n");
	printf("me6e add entry item: %d\n", me6e_item);
	printf("m46e add entry item: %d\n", m46e_item);

	printf("handler->conf.me6e_conf_table\n");
	m46e_pt_config_table_dump(&handler->conf.me6e_conf_table);
	printf("handler->conf.m46e_conf_table\n");
	m46e_pt_config_table_dump(&handler->conf.m46e_conf_table);

	printf("%s: exit\n", __func__);
	
	return;
}

static void CT_tunnel_forward_xx2xx_packet(mx6e_handler_t * handler)
{
	char                            recv_buffer[1024];
	ssize_t                         recv_len;
	struct ethhdr                  *p_ether;
	struct ip6_hdr                 *p_ip6;
	
	// テストパケット
	// CT_m46e_pt_add_config_entryで登録したエントリで変換するかどうかを確認
	// *INDENT-OFF*
	struct {
		domain_t	domain;
		int next;			// ME6E_IPPROTO_ETHERIP:ME6E, IPPROTO_IPIP:M46E
		char *hwaddrdes;
		char *hwaddrsrc;
		char *ipsrc;
		char *ipdst;
		int hoplimit;
	} pack[] = {
		//          ME6E_IPPROTO_ETHERIP: Me6E
		{DOMAIN_FP, ME6E_IPPROTO_ETHERIP,	"11:22:33:44:55:66", "de:ad:be:ef:00:01", "2000::1:abcd:ef01:6789",			"2000::1:abcd:ef01:2345",		2},
		{DOMAIN_FP, ME6E_IPPROTO_ETHERIP,	"11:22:33:44:55:66", "de:ad:be:ef:00:01", "2000::1:1:abcd:ef01:6789",		"2000::1:1:abcd:ef01:2345",		2},
		{DOMAIN_FP, ME6E_IPPROTO_ETHERIP,	"11:22:33:44:55:66", "de:ad:be:ef:00:01", "8000::1:1:1:abcd:ef01:6789",		"8000::1:1:1:abcd:ef01:2345",	2},
		{DOMAIN_FP, ME6E_IPPROTO_ETHERIP,	"11:22:33:44:55:66", "de:ad:be:ef:00:01", "8000:1:1:1:1:abcd:ef01:6789",	"8000:1:1:1:1:abcd:ef01:2345",	2},
		
		{DOMAIN_FP, ME6E_IPPROTO_ETHERIP,	"11:22:33:44:55:66", "de:ad:be:ef:00:01", "2000::1:abcd:ef01:6789",		"2000::1:abcd:ef01:2345",		2},
		{DOMAIN_PR, ME6E_IPPROTO_ETHERIP,	"11:22:33:44:55:66", "de:ad:be:ef:00:01", "2000::1:abcd:ef01:6789",		"2000::1:abcd:ef01:2345",		2},
		{DOMAIN_FP, ME6E_IPPROTO_ETHERIP,	"11:22:33:44:55:66", "de:ad:be:ef:00:01", "8000::1:abcd:ef01:6789",		"8000::1:abcd:ef01:2345",		2},
		{DOMAIN_PR, ME6E_IPPROTO_ETHERIP,	"11:22:33:44:55:66", "de:ad:be:ef:00:01", "8000::1:abcd:ef01:6789",		"8000::1:abcd:ef01:2345",		2},
		{DOMAIN_FP, ME6E_IPPROTO_ETHERIP,	"11:22:33:44:55:66", "de:ad:be:ef:00:01", "7fff:ffff:ffff:ffff:ffff:abcd:ef01:6789",	"7fff:ffff:ffff:ffff:ffff:abcd:ef01:2345",	2},
		{DOMAIN_PR, ME6E_IPPROTO_ETHERIP,	"11:22:33:44:55:66", "de:ad:be:ef:00:01", "7fff:ffff:ffff:ffff:ffff:abcd:ef01:6789",	"7fff:ffff:ffff:ffff:ffff:abcd:ef01:2345",	2},
		{DOMAIN_FP, ME6E_IPPROTO_ETHERIP,	"11:22:33:44:55:66", "de:ad:be:ef:00:01", "2000::8000:abcd:ef01:6789",	"2000::8000:abcd:ef01:2345",	2},
		{DOMAIN_PR, ME6E_IPPROTO_ETHERIP,	"11:22:33:44:55:66", "de:ad:be:ef:00:01", "2000::8000:abcd:ef01:6789",	"2000::8000:abcd:ef01:2345",	2},
		//          IPPROTO_IPIP: M46E
		{DOMAIN_FP, IPPROTO_IPIP,			"11:22:33:44:55:66", "de:ad:be:ef:00:01", "2000::8000:1234:c0a8:6501",	"2000::8000:1234:c0a8:6502",	2},	// 192.168.101.1,2
		{DOMAIN_PR, IPPROTO_IPIP,			"11:22:33:44:55:66", "de:ad:be:ef:00:01", "2000::8000:1234:c0a8:6501",	"2000::8000:1234:c0a8:6502",	2},	// 192.168.101.1,2
		{DOMAIN_FP, IPPROTO_IPIP,			"11:22:33:44:55:66", "de:ad:be:ef:00:01", "2000::1:2:c0a8:6601",		"2000::1:2:c0a8:6602",			2},	// 192.168.102.1,2
		{DOMAIN_PR, IPPROTO_IPIP,			"11:22:33:44:55:66", "de:ad:be:ef:00:01", "2000::1:2:c0a8:6601",		"2000::1:2:c0a8:6602",			2},	// 192.168.102.1,2
		{DOMAIN_NONE, 0, NULL, NULL, NULL, NULL, 0}
	};
	// *INDENT-ON*

	for (int i = 0; pack[i].domain != DOMAIN_NONE; i ++) {
		memset(recv_buffer, 0, sizeof(recv_buffer));	

		p_ether = (struct ethhdr *) recv_buffer;
		p_ip6 = (struct ip6_hdr *) (recv_buffer + sizeof(struct ethhdr));
	
		memset(recv_buffer, 0, sizeof(recv_buffer));
		ether_aton_r(pack[i].hwaddrdes, (struct ether_addr *) p_ether->h_dest);
		ether_aton_r(pack[i].hwaddrsrc, (struct ether_addr *) p_ether->h_source);
		p_ether->h_proto = htons(ETH_P_IPV6);
		p_ip6->ip6_vfc = 6;
		p_ip6->ip6_nxt = pack[i].next;
		inet_pton(AF_INET6, pack[i].ipsrc, &p_ip6->ip6_src);	// source IPv6
		inet_pton(AF_INET6, pack[i].ipdst, &p_ip6->ip6_dst);	// destination IPv6
		p_ip6->ip6_hlim = pack[i].hoplimit;						// Hop limit
		recv_len = sizeof(struct ethhdr) + sizeof(struct ip6_hdr);

		printf("*****************\n");
		printf("before %2d 変換前 DOMAIN:%s\n", i, get_domain_name(pack[i].domain));
		mx6e_print_packet(recv_buffer);
		if (DOMAIN_FP == pack[i].domain) {
			tunnel_forward_fp2pr_packet(handler, recv_buffer, recv_len);
		} else if (DOMAIN_PR == pack[i].domain) {
			tunnel_forward_pr2fp_packet(handler, recv_buffer, recv_len);
		} else {
			printf("domain ivalid\n");
		}
			
		printf("*****************\n");
		printf("after %2d 変換後\n", i);
		mx6e_print_packet(recv_buffer);
	}
}

// 単体
void ct(mx6e_handler_t * handler)
{
	
	config_init(&handler->conf);

	// 統計情報初期化
	mx6e_initial_statistics(&handler->stat_info);

	mx6e_config_table_t            *table = &handler->conf.me6e_conf_table;

	printf("mx6e_config_table_t:%ld\n", sizeof(mx6e_config_table_t));	// 80byte
	printf("mx6e_config_entry_t:%ld\n", sizeof(mx6e_config_entry_t));	// 204byte x 4096 = 816KB


	snprintf(handler->conf.devices.tunnel_pr.name, sizeof(handler->conf.devices.tunnel_pr.name), "tunnelpr");
	snprintf(handler->conf.devices.tunnel_fp.name, sizeof(handler->conf.devices.tunnel_fp.name), "tunnelfp");
	inet_pton(AF_INET6, "abcd:ef01:dead:beef:dead:beef::", &handler->conf.devices.tunnel_pr.ipv6_address);
	inet_pton(AF_INET6, "face:feed:face:feed:face:feed::", &handler->conf.devices.tunnel_fp.ipv6_address);

	inet_pton(AF_INET6, "f00d::", &handler->conf.devices.fp.ipv6_address);
	handler->conf.devices.fp.ipv6_netmask = 16;

	inet_pton(AF_INET6, "f005::", &handler->conf.devices.pr.ipv6_address);
	handler->conf.devices.pr.ipv6_netmask = 16;

	// エントリ追加
	CT_m46e_pt_add_config_entry(handler);
	// PID有効ビット長計算
	CT_get_pid_bit_width();
	// 変換
	CT_tunnel_forward_xx2xx_packet(handler);

	// 統計情報表示
	mx6e_printf_statistics_info_normal(&handler->stat_info, -1);

#if 0	
	mx6e_config_entry_t             entry = { 0 };
	mx6e_config_entry_t             entry1;
	mx6e_config_entry_t            *r;
	// 同じキーの検索
	r = mx6e_search_config_table(table, &entry, &handler->conf.devices);
	m46e_pt_config_entry_dump(r);

	// 違うキーの検索
	entry1 = entry;
	snprintf(entry1.src.plane_id, sizeof(entry1.src.plane_id), "%s", "2:1");
	inet_pton(AF_INET, "10.1.0.1", &entry1.src.in.m46e.v4addr);
	r = mx6e_search_config_table(table, &entry1, &handler->conf.devices);
	m46e_pt_config_entry_dump(r);
#endif

	exit(0);
}
#endif
