/******************************************************************************/
/* ファイル名 : mx6eapp_network.c                                             */
/* 機能概要   : ネットワーク関連関数 ソースファイル                           */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <asm/types.h>
#include <linux/if_tun.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "mx6eapp_network.h"
#include "mx6eapp_config.h"
#include "mx6eapp_log.h"
#include "mx6eapp_netlink.h"

#define RESULT_OK           0
#define RESULT_SYSCALL_NG  -1
#define RESULT_NG           1
#define RESULT_SKIP_NLMSG   2
#define RESULT_DONE         3



///////////////////////////////////////////////////////////////////////////////
//! @brief フラグ取得関数(デバイス名)
//!
//! デバイス名に対応するデバイスのフラグを取得する。
//!
//! @param [in]  ifname    デバイス名
//! @param [out] flags     取得したフラグの格納先
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_network_get_flags_by_name(const char *ifname, short *flags)
{
	// ローカル変数宣言
	struct ifreq                    ifr;
	int                             result;
	int                             sock;

	// 引数チェック
	if (ifname == NULL) {
		mx6e_logging(LOG_ERR, "ifname is NULL\n");
		return EINVAL;
	}
	if (flags == NULL) {
		mx6e_logging(LOG_ERR, "flags is NULL\n");
		return EINVAL;
	}
	// ローカル変数初期化
	memset(&ifr, 0, sizeof(struct ifreq));
	sock = -1;

	result = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL));
	if (result < 0) {
		mx6e_logging(LOG_ERR, "socket open error : %s\n", strerror(errno));
		return errno;
	} else {
		sock = result;
	}

	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);

	result = ioctl(sock, SIOCGIFFLAGS, &ifr);
	if (result != 0) {
		mx6e_logging(LOG_ERR, "ioctl(SIOCGIFFLAGS) error : %s\n", strerror(errno));
		close(sock);
		return errno;
	}
	close(sock);

	*flags = ifr.ifr_flags;

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief フラグ設定関数(デバイス名)
//!
//! デバイス名に対応するデバイスのフラグを設定する。
//!
//! @param [in]  ifname    デバイス名
//! @param [in]  flags     設定するフラグ
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_network_set_flags_by_name(const char *ifname, const short flags)
{
	// ローカル変数宣言
	struct ifreq                    ifr;
	int                             result;
	int                             sock;

	// 引数チェック
	if (ifname == NULL) {
		mx6e_logging(LOG_ERR, "ifname is NULL\n");
		return EINVAL;
	}
	// ローカル変数初期化
	memset(&ifr, 0, sizeof(struct ifreq));
	sock = -1;

	result = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL));
	if (result < 0) {
		mx6e_logging(LOG_ERR, "socket open error : %s\n", strerror(errno));
		return errno;
	} else {
		sock = result;
	}

	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);

	// 現在の状態を取得
	result = mx6e_network_get_flags_by_name(ifr.ifr_name, &ifr.ifr_flags);
	if (result != 0) {
		mx6e_logging(LOG_ERR, "fail to get current flags : %s\n", strerror(result));
		return result;
	}

	if (flags < 0) {
		// 負値の場合はフラグを落とす
		ifr.ifr_flags &= ~(-flags);
	} else {
		// 正値の場合はフラグを上げる
		ifr.ifr_flags |= flags;
	}

	result = ioctl(sock, SIOCSIFFLAGS, &ifr);
	if (result != 0) {
		mx6e_logging(LOG_ERR, "ioctl(SIOCSIFFLAGS) error : %s\n", strerror(errno));
		close(sock);
		return errno;
	}
	close(sock);

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//! @brief フラグ設定関数(インデックス番号)
//!
//! インデックス番号に対応するデバイスのフラグを設定する。
//!
//! @param [in]  ifindex   デバイスのインデックス番号
//! @param [in]  flags     設定するフラグ
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_network_set_flags_by_index(const int ifindex, const short flags)
{
	// ローカル変数宣言
	char                            ifname[IFNAMSIZ];

	// ローカル変数初期化
	memset(ifname, 0, sizeof(ifname));

	if (if_indextoname(ifindex, ifname) == NULL) {
		mx6e_logging(LOG_ERR, "if_indextoname error : %s\n", strerror(errno));
		return errno;
	}
	return mx6e_network_set_flags_by_name(ifname, flags);
}

//////////////////////////////////////////////////////////////////////////////
//! @brief IPアドレス設定関数
//!
//! インデックス番号に対応するデバイスのIPアドレスを設定する。
//!
//! @param [in]  family    設定するアドレス種別(AF_INET or AF_INET6)
//! @param [in]  ifindex   デバイスのインデックス番号
//! @param [in]  addr      設定するIPアドレス
//!                        (IPv4の場合はin_addr、IPv6の場合はin6_addr構造体)
//! @param [in]  prefixlen 設定するIPアドレスのプレフィックス長
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_network_add_ipaddr(const int family, const int ifindex, const void *addr, const int prefixlen)
{
	struct nlmsghdr                *nlmsg;
	struct ifaddrmsg               *ifaddr;
	int                             sock_fd;
	struct sockaddr_nl              local;
	uint32_t                        seq;
	int                             ret;
	int                             errcd;

	_D_(printf("%s:enter\n", __func__));

	ret = mx6e_netlink_open(0, &sock_fd, &local, &seq, &errcd);
	if (ret != RESULT_OK) {
		// socket open error
		mx6e_logging(LOG_ERR, "Netlink socket error errcd=%d", errcd);
		return errcd;
	}

	nlmsg = malloc(NETLINK_SNDBUF);					// 16kbyte
	if (nlmsg == NULL) {
		mx6e_logging(LOG_ERR, "Netlink send buffur malloc NG : %s", strerror(errno));
		mx6e_netlink_close(sock_fd);
		return errno;
	}

	memset(nlmsg, 0, NETLINK_SNDBUF);

	ifaddr = (struct ifaddrmsg *) (((void *) nlmsg) + NLMSG_HDRLEN);
	ifaddr->ifa_family = family;
	ifaddr->ifa_index = ifindex;
	ifaddr->ifa_prefixlen = prefixlen;
	ifaddr->ifa_scope = 0;

	nlmsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	nlmsg->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
	nlmsg->nlmsg_type = RTM_NEWADDR;

	int                             addrlen = (family == AF_INET) ? sizeof(struct in_addr) : sizeof(struct in6_addr);

	ret = mx6e_netlink_addattr_l(nlmsg, NETLINK_SNDBUF, IFA_LOCAL, addr, addrlen);
	if (ret != RESULT_OK) {
		mx6e_logging(LOG_ERR, "Netlink add attrubute error");
		mx6e_netlink_close(sock_fd);
		free(nlmsg);
		return ENOMEM;
	}

	ret = mx6e_netlink_addattr_l(nlmsg, NETLINK_SNDBUF, IFA_ADDRESS, addr, addrlen);
	if (ret != RESULT_OK) {
		mx6e_logging(LOG_ERR, "Netlink add attrubute error");
		mx6e_netlink_close(sock_fd);
		free(nlmsg);
		return ENOMEM;
	}

	if (family == AF_INET) {
		struct in_addr                  baddr = *((struct in_addr *) addr);
		baddr.s_addr |= htonl(INADDR_BROADCAST >> prefixlen);
		ret = mx6e_netlink_addattr_l(nlmsg, NETLINK_SNDBUF, IFA_BROADCAST, &baddr, addrlen);
		if (ret != RESULT_OK) {
			mx6e_logging(LOG_ERR, "Netlink add attrubute error");
			mx6e_netlink_close(sock_fd);
			free(nlmsg);
			return ENOMEM;
		}
	}

	ret = mx6e_netlink_transaction(sock_fd, &local, seq, nlmsg, &errcd);
	if (ret != RESULT_OK) {
		mx6e_netlink_close(sock_fd);
		free(nlmsg);
		return errcd;
	}

	mx6e_netlink_close(sock_fd);
	free(nlmsg);

	_D_(printf("%s:exit\n", __func__));
	return 0;
}

//////////////////////////////////////////////////////////////////////////////
//! @brief IPアドレス削除関数
//!
//! インデックス番号に対応するデバイスのIPアドレスを削除する。
//!
//! @param [in]  family    削除するアドレス種別(AF_INET or AF_INET6)
//! @param [in]  ifindex   デバイスのインデックス番号
//! @param [in]  addr      削除するIPアドレス
//!                        (IPv4の場合はin_addr、IPv6の場合はin6_addr構造体)
//! @param [in]  prefixlen 削除するIPアドレスのプレフィックス長
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_network_del_ipaddr(const int family, const int ifindex, const void *addr, const int prefixlen)
{
	struct nlmsghdr                *nlmsg;
	struct ifaddrmsg               *ifaddr;
	int                             sock_fd;
	struct sockaddr_nl              local;
	uint32_t                        seq;
	int                             ret;
	int                             errcd;

	_D_( {
		char buf[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, addr, buf, sizeof(buf));
		printf("%s:enter <%s>\n", __func__, buf);
		}
	);

	ret = mx6e_netlink_open(0, &sock_fd, &local, &seq, &errcd);
	if (ret != RESULT_OK) {
		// socket open error
		mx6e_logging(LOG_ERR, "Netlink socket error errcd=%d.", errcd);
		return errcd;
	}

	nlmsg = malloc(NETLINK_SNDBUF);					// 16kbyte
	if (nlmsg == NULL) {
		mx6e_logging(LOG_ERR, "Netlink send buffur malloc NG : %s.", strerror(errno));
		mx6e_netlink_close(sock_fd);
		return errno;
	}

	memset(nlmsg, 0, NETLINK_SNDBUF);

	ifaddr = (struct ifaddrmsg *) (((void *) nlmsg) + NLMSG_HDRLEN);
	ifaddr->ifa_family = family;
	ifaddr->ifa_index = ifindex;
	ifaddr->ifa_prefixlen = prefixlen;
	ifaddr->ifa_scope = 0;

	nlmsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	nlmsg->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
	nlmsg->nlmsg_type = RTM_DELADDR;

	int                             addrlen = (family == AF_INET) ? sizeof(struct in_addr) : sizeof(struct in6_addr);

	ret = mx6e_netlink_addattr_l(nlmsg, NETLINK_SNDBUF, IFA_LOCAL, addr, addrlen);
	if (ret != RESULT_OK) {
		mx6e_logging(LOG_ERR, "Netlink add attrubute error.");
		mx6e_netlink_close(sock_fd);
		free(nlmsg);
		return ENOMEM;
	}

	ret = mx6e_netlink_addattr_l(nlmsg, NETLINK_SNDBUF, IFA_ADDRESS, addr, addrlen);
	if (ret != RESULT_OK) {
		mx6e_logging(LOG_ERR, "Netlink add attrubute error.");
		mx6e_netlink_close(sock_fd);
		free(nlmsg);
		return ENOMEM;
	}

	if (family == AF_INET) {
		struct in_addr                  baddr = *((struct in_addr *) addr);
		baddr.s_addr |= htonl(INADDR_BROADCAST >> prefixlen);
		ret = mx6e_netlink_addattr_l(nlmsg, NETLINK_SNDBUF, IFA_BROADCAST, &baddr, addrlen);
		if (ret != RESULT_OK) {
			mx6e_logging(LOG_ERR, "Netlink add attrubute error.");
			mx6e_netlink_close(sock_fd);
			free(nlmsg);
			return ENOMEM;
		}
	}

	ret = mx6e_netlink_transaction(sock_fd, &local, seq, nlmsg, &errcd);
	if (ret != RESULT_OK) {
		mx6e_netlink_close(sock_fd);
		free(nlmsg);
		return errcd;
	}

	mx6e_netlink_close(sock_fd);
	free(nlmsg);

	_D_(printf("%s:exit\n", __func__));

	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//! @brief デフォルトゲートウェイ設定関数
//!
//! インデックス番号に対応するデバイスを経由する
//! デフォルトゲートウェイを設定する。
//!
//! @param [in]  family    設定するアドレス種別(AF_INET or AF_INET6)
//! @param [in]  ifindex   デバイスのインデックス番号
//! @param [in]  gw        デフォルトゲートウェイアドレス
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_network_add_gateway(const int family, const int ifindex, const void *gw)
{
	return mx6e_network_add_route(family, ifindex, NULL, 0, gw);
}

//////////////////////////////////////////////////////////////////////////////
//! @brief デフォルトゲートウェイ削除関数
//!
//! インデックス番号に対応するデバイスを経由する
//! デフォルトゲートウェイを削除する。
//!
//! @param [in]  family    設定するアドレス種別(AF_INET or AF_INET6)
//! @param [in]  ifindex   デバイスのインデックス番号
//! @param [in]  gw        デフォルトゲートウェイアドレス
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_network_del_gateway(const int family, const int ifindex, const void *gw)
{
	return mx6e_network_del_route(family, ifindex, NULL, 0, gw);
}

//////////////////////////////////////////////////////////////////////////////
//! @brief 経路設定関数
//!
//! インデックス番号に対応するデバイスを経由する経路を設定する。
//! デフォルトゲートウェイを追加する場合はdstをNULLしてgwを設定する。
//! connectedの経路を追加する場合は、gwをNULLに設定する。
//!
//! @param [in]  family    設定するアドレス種別(AF_INET or AF_INET6)
//! @param [in]  ifindex   デバイスのインデックス番号
//! @param [in]  dst       設定する経路の送信先アドレス
//!                        (IPv4の場合はin_addr、IPv6の場合はin6_addr構造体)
//! @param [in]  prefixlen 設定する経路のプレフィックス長
//! @param [in]  gw        設定する経路のゲートウェイアドレス
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_network_add_route(const int family, const int ifindex, const void *dst, const int prefixlen, const void *gw)
{
	struct nlmsghdr                *nlmsg;
	struct rtmsg                   *rt;
	int                             sock_fd;
	struct sockaddr_nl              local;
	uint32_t                        seq;
	int                             ret;
	int                             errcd;

	_D_(printf("enter %s\n", __func__));

	ret = mx6e_netlink_open(0, &sock_fd, &local, &seq, &errcd);
	if (ret != RESULT_OK) {
		// socket open error
		mx6e_logging(LOG_ERR, "Netlink socket error errcd=%d", errcd);
		return errcd;
	}

	nlmsg = malloc(NETLINK_SNDBUF);					// 16kbyte
	if (nlmsg == NULL) {
		mx6e_logging(LOG_ERR, "Netlink send buffur malloc NG : %s", strerror(errno));
		mx6e_netlink_close(sock_fd);
		return errno;
	}

	memset(nlmsg, 0, NETLINK_SNDBUF);

	rt = (struct rtmsg *) (((void *) nlmsg) + NLMSG_HDRLEN);
	rt->rtm_family = family;
	rt->rtm_table = RT_TABLE_MAIN;
	rt->rtm_scope = RT_SCOPE_UNIVERSE;
	rt->rtm_protocol = RTPROT_STATIC;
	rt->rtm_type = RTN_UNICAST;
	rt->rtm_dst_len = prefixlen;

	nlmsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	nlmsg->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
	nlmsg->nlmsg_type = RTM_NEWROUTE;

	int                             addrlen = (family == AF_INET) ? sizeof(struct in_addr) : sizeof(struct in6_addr);

	if (dst != NULL) {
		ret = mx6e_netlink_addattr_l(nlmsg, NETLINK_SNDBUF, RTA_DST, dst, addrlen);
		if (ret != RESULT_OK) {
			mx6e_logging(LOG_ERR, "Netlink add attrubute error");
			mx6e_netlink_close(sock_fd);
			free(nlmsg);
			return ENOMEM;
		}
	}

	if (gw != NULL) {
		ret = mx6e_netlink_addattr_l(nlmsg, NETLINK_SNDBUF, RTA_GATEWAY, gw, addrlen);
		if (ret != RESULT_OK) {
			mx6e_logging(LOG_ERR, "Netlink add attrubute error");
			mx6e_netlink_close(sock_fd);
			free(nlmsg);
			return ENOMEM;
		}
	}

	ret = mx6e_netlink_addattr_l(nlmsg, NETLINK_SNDBUF, RTA_OIF, &ifindex, sizeof(ifindex));
	if (ret != RESULT_OK) {
		mx6e_logging(LOG_ERR, "Netlink add attrubute error");
		mx6e_netlink_close(sock_fd);
		free(nlmsg);
		return ENOMEM;
	}

	ret = mx6e_netlink_transaction(sock_fd, &local, seq, nlmsg, &errcd);
	if (ret != RESULT_OK) {
		mx6e_netlink_close(sock_fd);
		free(nlmsg);
		return errcd;
	}

	mx6e_netlink_close(sock_fd);
	free(nlmsg);

	_D_(printf("exit %s\n", __func__));

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
//! @brief 経路削除関数
//!
//! インデックス番号に対応するデバイスを経由する経路を削除する。
//! デフォルトゲートウェイを削除する場合はdstをNULLしてgwを設定する。
//! connectedの経路を削除する場合は、gwをNULLに設定する。
//!
//! @param [in]  family    削除するアドレス種別(AF_INET or AF_INET6)
//! @param [in]  ifindex   デバイスのインデックス番号
//! @param [in]  dst       削除する経路の送信先アドレス
//!                        (IPv4の場合はin_addr、IPv6の場合はin6_addr構造体)
//! @param [in]  prefixlen 削除する経路のプレフィックス長
//! @param [in]  gw        削除する経路のゲートウェイアドレス
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_network_del_route(const int family, const int ifindex, const void *dst, const int prefixlen, const void *gw)
{
	struct nlmsghdr                *nlmsg;
	struct rtmsg                   *rt;
	int                             sock_fd;
	struct sockaddr_nl              local;
	uint32_t                        seq;
	int                             ret;
	int                             errcd;

	_D_(printf("enter %s\n", __func__));

	ret = mx6e_netlink_open(0, &sock_fd, &local, &seq, &errcd);
	if (ret != RESULT_OK) {
		// socket open error
		mx6e_logging(LOG_ERR, "Netlink socket error errcd=%d", errcd);
		return errcd;
	}

	nlmsg = malloc(NETLINK_SNDBUF);					// 16kbyte
	if (nlmsg == NULL) {
		mx6e_logging(LOG_ERR, "Netlink send buffur malloc NG : %s", strerror(errno));
		mx6e_netlink_close(sock_fd);
		return errno;
	}

	memset(nlmsg, 0, NETLINK_SNDBUF);

	rt = (struct rtmsg *) (((void *) nlmsg) + NLMSG_HDRLEN);
	rt->rtm_family = family;
	rt->rtm_table = RT_TABLE_MAIN;
	rt->rtm_scope = RT_SCOPE_UNIVERSE;
	rt->rtm_protocol = RTPROT_STATIC;
	rt->rtm_type = RTN_UNICAST;
	rt->rtm_dst_len = prefixlen;

	nlmsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	nlmsg->nlmsg_flags = NLM_F_REQUEST | NLM_F_EXCL | NLM_F_ACK;
	nlmsg->nlmsg_type = RTM_DELROUTE;

	int                             addrlen = (family == AF_INET) ? sizeof(struct in_addr) : sizeof(struct in6_addr);

	if (dst != NULL) {
		ret = mx6e_netlink_addattr_l(nlmsg, NETLINK_SNDBUF, RTA_DST, dst, addrlen);
		if (ret != RESULT_OK) {
			mx6e_logging(LOG_ERR, "Netlink add attrubute error");
			mx6e_netlink_close(sock_fd);
			free(nlmsg);
			return ENOMEM;
		}
	}

	if (gw != NULL) {
		ret = mx6e_netlink_addattr_l(nlmsg, NETLINK_SNDBUF, RTA_GATEWAY, gw, addrlen);
		if (ret != RESULT_OK) {
			mx6e_logging(LOG_ERR, "Netlink add attrubute error");
			mx6e_netlink_close(sock_fd);
			free(nlmsg);
			return ENOMEM;
		}
	}

	ret = mx6e_netlink_addattr_l(nlmsg, NETLINK_SNDBUF, RTA_OIF, &ifindex, sizeof(ifindex));
	if (ret != RESULT_OK) {
		mx6e_logging(LOG_ERR, "Netlink add attrubute error");
		mx6e_netlink_close(sock_fd);
		free(nlmsg);
		return ENOMEM;
	}

	ret = mx6e_netlink_transaction(sock_fd, &local, seq, nlmsg, &errcd);
	if (ret != RESULT_OK) {
		mx6e_netlink_close(sock_fd);
		free(nlmsg);
		return errcd;
	}

	mx6e_netlink_close(sock_fd);
	free(nlmsg);

	_D_(printf("exit %s\n", __func__));
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief トンネルデバイス生成関数
//!
//! トンネルデバイスを生成する。
//!
//! @param [in]     name       生成するデバイス名
//! @param [in,out] tunnel_dev デバイス構造体
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_network_create_tap(const char *name, mx6e_device_t * tunnel_dev)
{
	// ローカル変数宣言
	struct ifreq                    ifr;
	int                             result;

	// 引数チェック
	if (tunnel_dev == NULL) {
		return -1;
	}
	if ((name == NULL) || (strlen(name) == 0)) {
		// デバイス名が未設定の場合はエラー
		return -1;
	}
	// ローカル変数初期化
	memset(&ifr, 0, sizeof(ifr));

	// 仮想デバイスオープン
	result = open("/dev/net/tun", O_RDWR);
	if (result < 0) {
		mx6e_logging(LOG_ERR, "tun device open error : %s\n", strerror(errno));
		return result;
	} else {
		// ファイルディスクリプタを構造体に格納
		tunnel_dev->fd = result;
		// close-on-exec フラグを設定
		fcntl(tunnel_dev->fd, F_SETFD, FD_CLOEXEC);
	}

	strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);

	// Flag: IFF_TUN   - TUN device ( no ether header )
	//       IFF_TAP   - TAP device
	//       IFF_NO_PI - no packet information
	//ifr.ifr_flags = tunnel_dev->option.tunnel.mode | IFF_NO_PI;
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

	// 仮想デバイス生成
	result = ioctl(tunnel_dev->fd, TUNSETIFF, &ifr);
	if (result < 0) {
		mx6e_logging(LOG_ERR, "ioctl(TUNSETIFF) error : %s\n", strerror(errno));
		return result;
	}
	// デバイス名が変わっているかもしれないので、設定後のデバイス名を再取得
	//strcpy(tunnel_dev->name, ifr.ifr_name);

	// デバイスのインデックス番号取得
	tunnel_dev->ifindex = if_nametoindex(ifr.ifr_name);

	// 作成したTAPデバイスのMACアドレス
	mx6e_network_get_hwaddr_by_index(tunnel_dev->ifindex, &tunnel_dev->hwaddr);
	
	// NOARPフラグを設定
	result = mx6e_network_set_flags_by_name(ifr.ifr_name, IFF_NOARP);
	if (result != 0) {
		mx6e_logging(LOG_ERR, "%s fail to set noarp flags : %s\n", tunnel_dev->name, strerror(result));
		return -1;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief MACアドレス取得関数(デバイス名)
//!
//! デバイス名に対応するデバイスのMACアドレスを取得する。
//!
//! @param [in]  ifname    デバイス名
//! @param [out] hwaddr    取得したMACアドレスの格納先
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_network_get_hwaddr_by_name(const char* ifname, struct ether_addr* hwaddr)
{
    // ローカル変数宣言
    struct ifreq ifr;  
    int          result;
    int          sock;

    // 引数チェック
    if(ifname == NULL){
        mx6e_logging(LOG_ERR, "ifname is NULL\n");
        return EINVAL;
    }
    if(hwaddr == NULL){
        mx6e_logging(LOG_ERR, "hwaddr is NULL\n");
        return EINVAL;
    }

    // ローカル変数初期化
    memset(&ifr, 0, sizeof(struct ifreq));
    sock   = -1;

    result = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL));
    if(result < 0){
        mx6e_logging(LOG_ERR, "socket open error : %s\n", strerror(errno));
        return errno;
    }
    else{
        sock = result;
    }

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);

    result = ioctl(sock, SIOCGIFHWADDR, &ifr);
    if(result != 0) {
        mx6e_logging(LOG_ERR, "ioctl(SIOCGIFHWADDR) error : %s\n", strerror(errno));
        close(sock);
        return errno;
    }
    close(sock);

    memcpy(hwaddr->ether_addr_octet, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief MACアドレス取得関数(インデックス番号)
//!
//! インデックス番号に対応するデバイスのMACアドレスを取得する。
//!
//! @param [in]  ifindex   デバイスのインデックス番号
//! @param [out] hwaddr    取得したMACアドレスの格納先
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_network_get_hwaddr_by_index(const int ifindex, struct ether_addr* hwaddr)
{
    // ローカル変数宣言
    char ifname[IFNAMSIZ];  

    // ローカル変数初期化
    memset(ifname, 0, sizeof(ifname));

    if(if_indextoname(ifindex, ifname) == NULL){
        mx6e_logging(LOG_ERR, "if_indextoname error : %s\n", strerror(errno));
        return errno;
    }
    return mx6e_network_get_hwaddr_by_name(ifname, hwaddr);
}
