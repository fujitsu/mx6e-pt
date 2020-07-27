/******************************************************************************/
/* ファイル名 : mx6eapp_network.h                                             */
/* 機能概要   : ネットワーク関連関数 ヘッダファイル                           */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#ifndef __MX6EAPP_NETWORK_H__
#   define __MX6EAPP_NETWORK_H__

#   include <unistd.h>
#   	include "mx6eapp_config.h"

///////////////////////////////////////////////////////////////////////////////
// 外部関数プロトタイプ
///////////////////////////////////////////////////////////////////////////////
int                             mx6e_network_device_delete_by_index(const int ifindex);
int                             mx6e_network_device_move_by_index(const int ifindex, const pid_t pid);
int                             mx6e_network_device_rename_by_index(const int ifindex, const char *newname);
int                             mx6e_network_get_mtu_by_name(const char *ifname, int *mtu);
int                             mx6e_network_get_mtu_by_index(const int ifindex, int *mtu);
int                             mx6e_network_set_mtu_by_name(const char *ifname, const int mtu);
int                             mx6e_network_set_mtu_by_index(const int ifindex, const int mtu);
int                             mx6e_network_get_hwaddr_by_name(const char *ifname, struct ether_addr *hwaddr);
int                             mx6e_network_get_hwaddr_by_index(const int ifindex, struct ether_addr *hwaddr);
int                             mx6e_network_set_hwaddr_by_name(const char *ifname, const struct ether_addr *hwaddr);
int                             mx6e_network_set_hwaddr_by_index(const int ifindex, const struct ether_addr *hwaddr);
int                             mx6e_network_get_flags_by_name(const char *ifname, short *flags);
int                             mx6e_network_get_flags_by_index(const int ifindex, short *flags);
int                             mx6e_network_set_flags_by_name(const char *ifname, const short flags);
int                             mx6e_network_set_flags_by_index(const int ifindex, const short flags);
int                             mx6e_network_add_ipaddr(const int family, const int ifindex, const void *addr, const int prefixlen);
int                             mx6e_network_add_route(const int family, const int ifindex, const void *dst, const int prefixlen, const void *gw);
int                             mx6e_network_add_gateway(const int family, const int ifindex, const void *gw);

int                             mx6e_network_del_ipaddr(const int family, const int ifindex, const void *addr, const int prefixlen);
int                             mx6e_network_del_route(const int family, const int ifindex, const void *dst, const int prefixlen, const void *gw);
int                             mx6e_network_del_gateway(const int family, const int ifindex, const void *gw);
int                             mx6e_network_create_tap(const char *name, mx6e_device_t * tunnel_dev);

#endif												// __MX6EAPP_NETWORK_H__
