/******************************************************************************/
/* ファイル名 : mx6eapp_netlink.c                                             */
/* 機能概要   : netlinkソケット送受信クラス ヘッダファイル                    */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#ifndef __MX6EAPP_NETLINK_H__
#   define __MX6EAPP_NETLINK_H__

#   include <stdint.h>
#   include <sys/socket.h>
#   include <linux/netlink.h>

/* ------------------ */
/* define result code */
/* ------------------ */
#   define RESULT_OK           0
#   define RESULT_SYSCALL_NG  -1
#   define RESULT_NG           1
#   define RESULT_SKIP_NLMSG   2
#   define RESULT_DONE         3

#   define NETLINK_RCVBUF (16*1024)
#   define NETLINK_SNDBUF (16*1024)

//! 受信データ解析関数
typedef int                     (*netlink_parse_func) (struct nlmsghdr *, int *, void *);

///////////////////////////////////////////////////////////////////////////////
// 外部関数プロトタイプ
///////////////////////////////////////////////////////////////////////////////
int                             mx6e_netlink_open(unsigned long group, int *sock_fd, struct sockaddr_nl *local, uint32_t * seq, int *errcd);
void                            mx6e_netlink_close(int sock_fd);
int                             mx6e_netlink_send(int sock_fd, uint32_t seq, struct nlmsghdr *nlm, int *errcd);
int                             mx6e_netlink_recv(int sock_fd, struct sockaddr_nl *local_sa, uint32_t seq, int *errcd, netlink_parse_func parse_func, void *data);
int                             mx6e_netlink_transaction(int sock_fd, struct sockaddr_nl *local_sa, uint32_t seq, struct nlmsghdr *nlm, int *errcd);
int                             mx6e_netlink_addattr_l(struct nlmsghdr *n, int maxlen, int type, const void *data, int alen);
struct rtattr                  *mx6e_netlink_attr_begin(struct nlmsghdr *n, int maxlen, int attr);
void                            mx6e_netlink_attr_end(struct nlmsghdr *n, struct rtattr *attr);
int                             mx6e_netlink_parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len);
int                             mx6e_netlink_parse_ack(struct nlmsghdr *nlmsg_h, int *errcd, void *data);

#endif												// __MX6EAPP_NETLINK_H__
