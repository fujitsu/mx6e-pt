/******************************************************************************/
/* ファイル名 : mx6eapp_util.h                                                */
/* 機能概要   : 共通関数 ヘッダファイル                                       */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#ifndef __MX6EAPP_UTIL_H__
#   define __MX6EAPP_UTIL_H__

#   include <stdbool.h>
#   include <sys/uio.h>
#   include <netinet/in.h>
#   include "mx6eapp_command_data.h"

////////////////////////////////////////////////////////////////////////////////
// 外部マクロ定義
////////////////////////////////////////////////////////////////////////////////
//! 値の大きい方を返すマクロ
#   define max(a, b) ((a) > (b) ? (a) : (b))
//! 値の小さい方を返すマクロ
#   define min(a, b) ((a) < (b) ? (a) : (b))

// 設定ファイルのbool値
#   define CONFIG_BOOL_TRUE  "yes"
#   define CONFIG_BOOL_FALSE "no"
// 設定ファイルのbool値
#   define OPT_BOOL_ON  "on"
#   define OPT_BOOL_OFF "off"
#   define OPT_BOOL_ENABLE  "enable"
#   define OPT_BOOL_DISABLE "disable"
#   define OPT_BOOL_NONE    ""

//! サブネットマスク の最小値
#   define CONFIG_IPV4_NETMASK_MIN    1
#   define CONFIG_IPV6_PREFIX_MIN 1
#   define CONFIG_IPV4_NETMASK_PR_MIN 0
//! サブネットマスク の最大値
#   define CONFIG_IPV4_NETMASK_MAX		32
#   define CONFIG_IPV6_PREFIX_MAX 128

////////////////////////////////////////////////////////////////////////////////
// 外部関数プロトタイプ宣言
////////////////////////////////////////////////////////////////////////////////
bool                            mx6e_util_get_ipv4_addr(const char *dev, struct in_addr * addr);
bool                            mx6e_util_get_ipv4_mask(const char *dev, struct in_addr *mask);
bool                            mx6e_util_is_broadcast_mac(const unsigned char *mac_addr);
unsigned short                  mx6e_util_checksum(unsigned short *buf, int size);
unsigned short                  mx6e_util_checksumv(struct iovec vec[], int vec_size);

bool                            parse_bool(const char *str, bool * output);
bool                            parse_int(const char *str, int *output, const int min, const int max);
bool                            parse_ipv4address(const char *str, struct in_addr *output, int *prefixlen);
bool                            parse_ipv4address_pr(const char *str, struct in_addr *output, int *prefixlen);
bool                            parse_ipv6address(const char *str, struct in6_addr *output, int *prefixlen);
bool                            parse_macaddress(const char *str, struct ether_addr *output);
bool                            compare_ipv6(struct in6_addr *addr1, struct in6_addr *addr2);
bool                            compare_ipv6_mask(struct in6_addr addr1, struct in6_addr addr2, struct in6_addr *mask);
bool                            compare_hwaddr(struct ether_addr *hwaddr1, struct ether_addr *hwaddr2);
const char                     *get_command_name(mx6e_command_code_t code);
bool                            make_config_entry(mx6e_config_entry_t * entry, table_type_t type, mx6e_config_devices_t * devices);
const char                     *get_domain_name(domain_t domain);
const char                     *get_table_name(table_type_t table_type);
int                             get_domain_src_ifindex(domain_t domain, mx6e_config_devices_t * devices);
int                             get_domain_dst_ifindex(domain_t domain, mx6e_config_devices_t * devices);
int                             get_pid_bit_width(char plane_id[]);

#endif												// __MX6EAPP_UTIL_H__
