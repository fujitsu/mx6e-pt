/******************************************************************************/
/* ファイル名 : mx6eapp_tunnel.c                                              */
/* 機能概要   : トンネルクラス ソースファイル                                 */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/fcntl.h>

#include "mx6eapp.h"
#include "mx6eapp_tunnel.h"
#include "mx6eapp_log.h"
#include "mx6eapp_config.h"
#include "mx6eapp_print_packet.h"
#include "mx6eapp_util.h"
#include "mx6eapp_statistics.h"
#include "mx6eapp_pt.h"

//! 受信バッファのサイズ
#define TUNNEL_RECV_BUF_SIZE 65535

////////////////////////////////////////////////////////////////////////////////
// 内部関数プロトタイプ宣言
////////////////////////////////////////////////////////////////////////////////
static void                     tunnel_buffer_cleanup(void *buffer);
static void                     tunnel_pr2fp_main_loop(mx6e_handler_t * handler);
static void                     tunnel_fp2pr_main_loop(mx6e_handler_t * handler);

///////////////////////////////////////////////////////////////////////////////
//! @brief PRネットワーク用 パケットカプセル化スレッド
//!
//! IPv4パケット受信のメインループを呼ぶ。
//!
//! @param [in] arg MX6Eハンドラ
//!
//! @return NULL固定
///////////////////////////////////////////////////////////////////////////////
void                           *mx6e_tunnel_pr_thread(void *arg)
{
	// ローカル変数宣言
	mx6e_handler_t                 *handler;

	// 引数チェック
	if (arg == NULL) {
		pthread_exit(NULL);
	}
	// ローカル変数初期化
	handler = (mx6e_handler_t *) arg;

	// メインループ開始
	tunnel_pr2fp_main_loop(handler);

	pthread_exit(NULL);

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief Fpネットワーク用 パケットデカプセル化スレッド
//!
//! IPv6パケット受信のメインループを呼ぶ。
//!
//! @param [in] arg MX6Eハンドラ
//!
//! @return NULL固定
///////////////////////////////////////////////////////////////////////////////
void                           *mx6e_tunnel_fp_thread(void *arg)
{
	// ローカル変数宣言
	mx6e_handler_t                 *handler;

	// 引数チェック
	if (arg == NULL) {
		pthread_exit(NULL);
	}
	// ローカル変数初期化
	handler = (mx6e_handler_t *) arg;

	// メインループ開始
	tunnel_fp2pr_main_loop(handler);

	pthread_exit(NULL);

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 受信バッファ解放関数
//!
//! 引数で指定されたバッファを解放する。
//! スレッドの終了時に呼ばれる。
//!
//! @param [in] buffer    受信バッファ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
static void tunnel_buffer_cleanup(void *buffer)
{
	DEBUG_LOG("tunnel_buffer_cleanup\n");

	// 確保したメモリを解放
	free(buffer);

	return;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief PRネットワーク メインループ関数
//!
//! PR->FPネットワークのメインループ。
//! 仮想デバイスからのパケット受信を待ち受けて、
//! パケット受信時にカプセル化の処理をおこなう。
//!
//! @param [in] handler   MX6Eハンドラ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
static void tunnel_pr2fp_main_loop(mx6e_handler_t * handler)
{
	// ローカル変数宣言
	int                             max_fd;
	fd_set                          fds;
	char                           *recv_buffer;
	ssize_t                         recv_len;
	mx6e_device_t                  *pr_dev;

	// 引数チェック
	if (handler == NULL) {
		return;
	}
	// 受信バッファ領域を確保
	recv_buffer = (char *) malloc(TUNNEL_RECV_BUF_SIZE);
	if (recv_buffer == NULL) {
		mx6e_logging(LOG_ERR, "receive buffer allocation failed\n");
		return;
	}
	// 後始末ハンドラ登録
	pthread_cleanup_push(tunnel_buffer_cleanup, (void *) recv_buffer);

	// デバイス
	pr_dev = &handler->conf.devices.tunnel_pr;

	// selector用のファイディスクリプタ設定
	// (待ち受けるディスクリプタの最大値+1)
	max_fd = -1;
	max_fd = max(max_fd, pr_dev->fd);
	max_fd++;

	// ループ前に今溜まっているデータを全て吐き出す
	while (1) {
		struct timeval                  t;
		FD_ZERO(&fds);
		FD_SET(pr_dev->fd, &fds);

		// timevalを0に設定することで、即時に受信できるデータを待つ
		t.tv_sec = 0;
		t.tv_usec = 0;

		// 受信待ち
		if (select(max_fd, &fds, NULL, NULL, &t) > 0) {
			if (FD_ISSET(pr_dev->fd, &fds)) {
				recv_len = read(pr_dev->fd, recv_buffer, TUNNEL_RECV_BUF_SIZE);
			}
		} else {
			// 即時に受信できるデータが無くなったのでループを抜ける
			break;
		}
	}

	mx6e_logging(LOG_INFO, "tunnel_pr2fp_main_loop start\n");

	while (1) {
		// selectorの初期化
		FD_ZERO(&fds);
		FD_SET(pr_dev->fd, &fds);

		// 受信待ち
		if (select(max_fd, &fds, NULL, NULL, NULL) < 0) {
			if (errno == EINTR) {
				// シグナル割込みの場合は処理継続
				DEBUG_LOG("signal receive. continue thread loop.");
				continue;
			} else {
				mx6e_logging(LOG_ERR, "PR tunnel main loop receive error : %s\n", strerror(errno));
				break;;
			}
		}
		// PR用デバイスでデータ受信
		if (FD_ISSET(pr_dev->fd, &fds)) {
			if ((recv_len = read(pr_dev->fd, recv_buffer, TUNNEL_RECV_BUF_SIZE)) > 0) {
				// FPデバイスに転送
				tunnel_forward_pr2fp_packet(handler, recv_buffer, recv_len);
			} else {
				mx6e_logging(LOG_ERR, "v4 recvfrom\n");
			}
		}
	}

	mx6e_logging(LOG_INFO, "Fp tunnel thread main loop end\n");

	// 後始末
	pthread_cleanup_pop(1);

	return;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief FPネットワーク メインループ関数
//!
//! FP->PRネットワークのメインループ。
//! 仮想デバイスからのパケット受信を待ち受けて、
//! パケット受信時にデカプセル化の処理をおこなう。
//!
//! @param [in] handler   MX6Eハンドラ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
static void tunnel_fp2pr_main_loop(mx6e_handler_t * handler)
{
	// ローカル変数宣言
	int                             max_fd;
	fd_set                          fds;
	char                           *recv_buffer;
	ssize_t                         recv_len;
	mx6e_device_t                  *fp_dev;

	// 引数チェック
	if (handler == NULL) {
		return;
	}
	// 受信バッファ領域を確保
	recv_buffer = (char *) malloc(TUNNEL_RECV_BUF_SIZE);
	if (recv_buffer == NULL) {
		mx6e_logging(LOG_ERR, "receive buffer allocation failed\n");
		return;
	}
	// 後始末ハンドラ登録
	pthread_cleanup_push(tunnel_buffer_cleanup, (void *) recv_buffer);

	// デバイス
	fp_dev = &handler->conf.devices.tunnel_fp;

	// selector用のファイディスクリプタ設定
	// (待ち受けるディスクリプタの最大値+1)
	max_fd = -1;
	max_fd = max(max_fd, fp_dev->fd);
	max_fd++;

	// ループ前に今溜まっているデータを全て吐き出す
	while (1) {
		struct timeval                  t;
		FD_ZERO(&fds);
		FD_SET(fp_dev->fd, &fds);

		// timevalを0に設定することで、即時に受信できるデータを待つ
		t.tv_sec = 0;
		t.tv_usec = 0;

		// 受信待ち
		if (select(max_fd, &fds, NULL, NULL, &t) > 0) {
			if (FD_ISSET(fp_dev->fd, &fds)) {
				recv_len = read(fp_dev->fd, recv_buffer, TUNNEL_RECV_BUF_SIZE);
			}
		} else {
			// 即時に受信できるデータが無くなったのでループを抜ける
			break;
		}
	}

	mx6e_logging(LOG_INFO, "tunnel_fp2pr_main_loop start\n");

	while (1) {
		// selectorの初期化
		FD_ZERO(&fds);
		FD_SET(fp_dev->fd, &fds);

		// 受信待ち
		if (select(max_fd, &fds, NULL, NULL, NULL) < 0) {
			if (errno == EINTR) {
				// シグナル割込みの場合は処理継続
				DEBUG_LOG("signal receive. continue thread loop.");
				continue;
			} else {
				mx6e_logging(LOG_ERR, "Fp tunnel main loop receive error : %s\n", strerror(errno));
				break;;
			}
		}
		// FP用デバイスでデータ受信
		if (FD_ISSET(fp_dev->fd, &fds)) {
			if ((recv_len = read(fp_dev->fd, recv_buffer, TUNNEL_RECV_BUF_SIZE)) > 0) {
				// PRデバイスに転送
				tunnel_forward_fp2pr_packet(handler, recv_buffer, recv_len);
			} else {
				mx6e_logging(LOG_ERR, "v6 recvfrom\n");
			}
		}
	}

	mx6e_logging(LOG_INFO, "Fp tunnel thread main loop end\n");

	// 後始末
	pthread_cleanup_pop(1);

	return;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 送信処理関数
//!
//! socketに送信する
//!
//! @param [in] fd          送信先ソケットFD
//! @param [in] buf         送信データポインタ
//! @param [in] len			送信データ長
//!
//! @retval 0 <   送信データ長
//! @retval 0 >   異常終了
///////////////////////////////////////////////////////////////////////////////
static inline int send_buf_msg(int fd, char *buf, ssize_t len)
{

	struct ip6_hdr                 *p_ip6;
	struct sockaddr_in6             daddr;
	struct cmsghdr                 *cmsg;
	char                            cmsgbuf[CMSG_SPACE(sizeof(struct in6_pktinfo))] = { 0 };
	struct iovec                    iov[2];
	struct msghdr                   msg = { 0 };
	struct in6_pktinfo             *info;

	return write(fd, buf, len);

	
	p_ip6 = (struct ip6_hdr *) (buf + sizeof(struct ethhdr));

	// IPv6ヘッダの設定
	daddr.sin6_family = AF_INET6;
	daddr.sin6_port = htons(p_ip6->ip6_nxt);
	daddr.sin6_addr = p_ip6->ip6_dst;
	daddr.sin6_flowinfo = 0;
	daddr.sin6_scope_id = 0;

	iov[0].iov_base = buf;
	iov[0].iov_len = len;

	_D_(printf("iov[0].iov_len:%ld\n", iov[0].iov_len));

	iov[1].iov_base = NULL;
	iov[1].iov_len = 0;

	msg.msg_name = &daddr;
	msg.msg_namelen = sizeof(daddr);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;

	msg.msg_control = &cmsgbuf[0];
	msg.msg_controllen = sizeof(cmsgbuf);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
	cmsg->cmsg_level = IPPROTO_IPV6;
	cmsg->cmsg_type = IPV6_PKTINFO;

	// 送信元情報(in6_pktinfo)をcmsgへ設定
	info = (struct in6_pktinfo *) CMSG_DATA(cmsg);
	info->ipi6_addr = p_ip6->ip6_src;

	if (IN6_IS_ADDR_MULTICAST(&p_ip6->ip6_dst)) {
		// マルチキャスト送信はしない
		return false;
	} else {
		// ユニキャスト送信
		info->ipi6_ifindex = 0;
	}

	// カプセル化したデータを送信
	return sendmsg(fd, &msg, 0);
}

///////////////////////////////////////////////////////////////////////////////
//! @brief PRパケット転送関数
//!
//! 受信したPRパケットを書き換えてFPデバイスに転送する。
//!
//! @param [in,out] handler     MX6Eハンドラ
//! @param [in]     recv_buffer 受信パケットデータ
//! @param [in]     recv_len    受信パケット長
//! @param [in]     send_fd     パケットを転送するファイルディスクリプタ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
void tunnel_forward_pr2fp_packet(mx6e_handler_t * handler, char *recv_buffer, ssize_t recv_len)
{
	mx6e_device_t *dev_src = &handler->conf.devices.tunnel_pr;
	mx6e_device_t *dev_dst = &handler->conf.devices.tunnel_fp;
	
	struct ethhdr                  *p_ether;		// see /usr/include/linux/if_ether.h
	struct ip6_hdr                 *p_ip6 = NULL;	// see /usr/include/netinet/ip6.h
	mx6e_config_entry_t            *entry;
	ssize_t                         send_len;

	// ローカル変数初期化
	p_ether = (struct ethhdr *) recv_buffer;

	// 統計情報
	STAT_PR_RECIEVE;

	if (mx6e_util_is_broadcast_mac(&p_ether->h_dest[0])) {
		// ブロードキャストパケットは黙って破棄
		DEBUG_LOG("drop packet so that recv packet is broadcast\n");
		STAT_PR_ERR_BROADCAST;
		return;
	}
	
	// MACアドレス変更
	// 送信先MAC
	p_ether->h_dest[0] = dev_dst->hwaddr.ether_addr_octet[0];
	p_ether->h_dest[1] = dev_dst->hwaddr.ether_addr_octet[1];
	p_ether->h_dest[2] = dev_dst->hwaddr.ether_addr_octet[2];
	p_ether->h_dest[3] = dev_dst->hwaddr.ether_addr_octet[3];
	p_ether->h_dest[4] = dev_dst->hwaddr.ether_addr_octet[4];
	p_ether->h_dest[5] = dev_dst->hwaddr.ether_addr_octet[5];
	// 送信元MAC
	p_ether->h_source[0] = dev_src->hwaddr.ether_addr_octet[0];
	p_ether->h_source[1] = dev_src->hwaddr.ether_addr_octet[1];
	p_ether->h_source[2] = dev_src->hwaddr.ether_addr_octet[2];
	p_ether->h_source[3] = dev_src->hwaddr.ether_addr_octet[3];
	p_ether->h_source[4] = dev_src->hwaddr.ether_addr_octet[4];
	p_ether->h_source[5] = dev_src->hwaddr.ether_addr_octet[5];

	if (ntohs(p_ether->h_proto) == ETH_P_IPV6) {
		// IPv6パケット
		DEBUG_LOG("\n");
		DEBUG_LOG("recv IPv6 packet.\n");
		// _D_(mx6e_print_packet(recv_buffer));
		// IPv6パケットの場合、デカプセル化してIPv4用仮想デバイスにwrite
		// 送信先はルーティングテーブルにお任せ
		p_ip6 = (struct ip6_hdr *) (recv_buffer + sizeof(struct ethhdr));

		// /usr/include/netinet/in.h:
		// IPPROTO_IP = 0,        // Dummy protocol for TCP.
		// IPPROTO_IPIP = 4,      // IPIP tunnels (older KA9Q tunnels use 94).
		// IPPROTO_IPV6 = 41,     // IPv6 header.
		// ME6E_IPPROTO_ETHERIP   97 : EtherIP(97 0x61 ETHERIP Ethernet-within-IP Encapsulation)ならME6E-PTテーブル、

		if (p_ip6->ip6_hlim == 1) {
			// Hop Limitが1のパケットは黙って破棄(これ以上転送できない為)
			DEBUG_LOG("drop packet so that hop limit is 1.\n");
			STAT_PR_ERR_HOPLIMIT;
			return;
		}
#if 0	// 2016/07/22 nextヘッダを判断して検索テーブルを別けるのはやめる暫定対処。

		// IPv4ならM46E-PTテーブル
		switch (p_ip6->ip6_nxt) {
		case IPPROTO_IPIP:
		case IPPROTO_ICMP:
#endif
			// M46E IPv6
			int m46e_entry_flg = 0;		// 2016/07/22 nextヘッダを判断して検索テーブルを別けるのはやめる暫定対処。
			// エントリ検索
			if (NULL != (entry = mx6e_match_config_table(DOMAIN_PR, &handler->conf.m46e_conf_table, &p_ip6->ip6_dst))) {
				// ヘッダ置換
				mx6e_replace_address(entry, p_ip6);

				////////////////////////////////////////////////////////////////////////
				// 送信
				////////////////////////////////////////////////////////////////////////
				if (dev_dst->fd) {
					if (0 > (send_len = send_buf_msg(dev_dst->fd, recv_buffer, recv_len))) {
						char                            mes2[1024];
						snprintf(mes2, sizeof(mes2), "fail to send IPPROTO_IPIP packet (%s)\n", strerror(errno)); 
						mx6e_logging(LOG_ERR, mes2);

						STAT_PR_M46E_SEND_ERR;
					} else {
						DEBUG_LOG("forward %ld bytes to IPPROTO_IPIP\n", (unsigned long int) send_len);
						STAT_PR_M46E_SEND_SUCCESS;
					}
				} else {
					// 送信デバイスfdが0の場合はデバッグダンプ
					_D_( {
						for (int i = 0; i < recv_len; i++) {
						if (0 == (i % 16)) {
						printf("\n");}
						printf("%02x ", (unsigned char) recv_buffer[i]);}
						printf("\n");}
					);
				}
				m46e_entry_flg = 1;	// 2016/07/22 nextヘッダを判断して検索テーブルを別けるのはやめる暫定対処。
			}
#if 0	// 2016/07/22 nextヘッダを判断して検索テーブルを別けるのはやめる暫定対処。
			} else {
				DEBUG_LOG("fail to match IPPROTO_IPIP packet\n");
				STAT_PR_M46E_SEND_ERR;
			}
			break;

		case ME6E_IPPROTO_ETHERIP:
#endif
		if ( m46e_entry_flg == 0 ){	// 2016/07/22 nextヘッダを判断して検索テーブルを別けるのはやめる暫定対処。
			// ME6E IPv6
			// エントリ検索
			if (NULL != (entry = mx6e_match_config_table(DOMAIN_PR, &handler->conf.me6e_conf_table, &p_ip6->ip6_dst))) {
				// ヘッダ置換
				mx6e_replace_address(entry, p_ip6);

				////////////////////////////////////////////////////////////////////////
				// 送信
				////////////////////////////////////////////////////////////////////////
				if (dev_dst->fd) {
					if (0 > (send_len = send_buf_msg(dev_dst->fd, recv_buffer, recv_len))) {
						char                            mes2[1024];
						snprintf(mes2, sizeof(mes2), "fail to send ME6E_IPPROTO_ETHERIP packet (%s)\n", strerror(errno)); 
						mx6e_logging(LOG_ERR, mes2);

						STAT_PR_ME6E_SEND_ERR;
					} else {
						DEBUG_LOG("forward %ld bytes to ME6E_IPPROTO_ETHERIP\n", (unsigned long int) send_len);
						STAT_PR_ME6E_SEND_SUCCESS;
					}
				} else {
					// 送信デバイスfdが0の場合はデバッグダンプ
					_D_( {
						for (int i = 0; i < recv_len; i++) {
						if (0 == (i % 16)) {
						printf("\n");}
						printf("%02x ", (unsigned char) recv_buffer[i]);}
						printf("\n");}
					);
				}
			} else {
				DEBUG_LOG("fail to match ME6E_IPPROTO_ETHERIP packet\n");
				STAT_PR_ME6E_SEND_ERR;
			}
		}
#if 0	// 2016/07/22 nextヘッダを判断して検索テーブルを別けるのはやめる暫定対処。
			break;

		default:
			// 次ヘッダがIPIP以外(カプセル化されていないパケット)の場合、黙って破棄。
			DEBUG_LOG("drop packet so that recv packet is not ipip.\n");
			STAT_PR_ERR_NXTHDR;
			break;
		}
#endif
	} else {
		// IPv6以外のパケットは、黙って破棄。
		DEBUG_LOG("Drop IPv6 Packet Ether Type : %d\n", ntohs(p_ether->h_proto));
		STAT_PR_ERR_OTHER_PROTO;
	}

	return;
}




///////////////////////////////////////////////////////////////////////////////
//! @brief FPパケット転送関数
//!
//! 受信したFPパケットを書き換えてしてPRデバイスに転送する。
//!
//! @param [in,out] handler     MX6Eハンドラ
//! @param [in]     recv_buffer 受信パケットデータ
//! @param [in]     recv_len    受信パケット長
//! @param [in]     send_fd     パケットを転送するファイルディスクリプタ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
void tunnel_forward_fp2pr_packet(mx6e_handler_t * handler, char *recv_buffer, ssize_t recv_len)
{
	mx6e_device_t *dev_src = &handler->conf.devices.tunnel_fp;
	mx6e_device_t *dev_dst = &handler->conf.devices.tunnel_pr;

	struct ethhdr                  *p_ether;
	struct ip6_hdr                 *p_ip6;
	mx6e_config_entry_t            *entry;
	ssize_t                         send_len;

	// ローカル変数初期化
	p_ether = (struct ethhdr *) recv_buffer;

	// 統計情報
	STAT_FP_RECIEVE;

	if (mx6e_util_is_broadcast_mac(&p_ether->h_dest[0])) {
		// ブロードキャストパケットは黙って破棄
		DEBUG_LOG("drop packet so that recv packet is broadcast\n");
		STAT_FP_ERR_BROADCAST;
		return;
	}

	// MACアドレス変更
	/* p_ether->h_dest[ETH_ALEN]; */
	/* p_ether->h_source[ETH_ALEN]; */
	// eth0 deviceのMACを強制
	// struct ether_addr
	// {
	//   u_int8_t ether_addr_octet[ETH_ALEN];
	// } __attribute__ ((__packed__));
	// 送信先MAC
	p_ether->h_dest[0] = dev_dst->hwaddr.ether_addr_octet[0];
	p_ether->h_dest[1] = dev_dst->hwaddr.ether_addr_octet[1];
	p_ether->h_dest[2] = dev_dst->hwaddr.ether_addr_octet[2];
	p_ether->h_dest[3] = dev_dst->hwaddr.ether_addr_octet[3];
	p_ether->h_dest[4] = dev_dst->hwaddr.ether_addr_octet[4];
	p_ether->h_dest[5] = dev_dst->hwaddr.ether_addr_octet[5];
	// 送信元MAC
	p_ether->h_source[0] = dev_src->hwaddr.ether_addr_octet[0];
	p_ether->h_source[1] = dev_src->hwaddr.ether_addr_octet[1];
	p_ether->h_source[2] = dev_src->hwaddr.ether_addr_octet[2];
	p_ether->h_source[3] = dev_src->hwaddr.ether_addr_octet[3];
	p_ether->h_source[4] = dev_src->hwaddr.ether_addr_octet[4];
	p_ether->h_source[5] = dev_src->hwaddr.ether_addr_octet[5];
		
	if (ntohs(p_ether->h_proto) == ETH_P_IPV6) {
		// IPv6パケット
		DEBUG_LOG("\n");
		DEBUG_LOG("recv IPv6 packet.\n");
		//_D_(mx6e_print_packet(recv_buffer));
		// 送信先はルーティングテーブルにお任せ
		p_ip6 = (struct ip6_hdr *) (recv_buffer + sizeof(struct ethhdr));

		if (p_ip6->ip6_hlim == 1) {
			// Hop Limitが1のパケットは黙って破棄(これ以上転送できない為)
			DEBUG_LOG("drop packet so that hop limit is 1.\n");
			STAT_FP_ERR_HOPLIMIT;
			return;
		}
#if 0	// 2016/07/22 nextヘッダを判断して検索テーブルを別けるのはやめる暫定対処。
		// IPv4ならM46E-PTテーブル
		switch (p_ip6->ip6_nxt) {
		case IPPROTO_IPIP:							// M46E IPv6
		case IPPROTO_ICMP:							// ICMPv6
#endif

			int m46e_entry_flg = 0;		// 2016/07/22 nextヘッダを判断して検索テーブルを別けるのはやめる暫定対処。
			// エントリ検索
			if (NULL != (entry = mx6e_match_config_table(DOMAIN_FP, &handler->conf.m46e_conf_table, &p_ip6->ip6_dst))) {
				// ヘッダ置換
				mx6e_replace_address(entry, p_ip6);

				////////////////////////////////////////////////////////////////////////
				// 送信
				////////////////////////////////////////////////////////////////////////
				if (dev_dst->fd) {
					if (0 > (send_len = send_buf_msg(dev_dst->fd, recv_buffer, recv_len))) {
						char                            mes2[1024];
						snprintf(mes2, sizeof(mes2), "fail to send IPPROTO_IPIP packet (%s)\n", strerror(errno)); 
						mx6e_logging(LOG_ERR, mes2);

						STAT_FP_M46E_SEND_ERR;
					} else {
						DEBUG_LOG("forward %ld bytes to IPPROTO_IPIP\n", (unsigned long int) send_len);
						STAT_FP_M46E_SEND_SUCCESS;
					}
				} else {
					// 送信デバイスfdがNULLの場合はデバッグダンプ
					_D_(STAT_FP_ME6E_SEND_SUCCESS;	// とりあえず送信に計上
						{
						for (int i = 0; i < recv_len; i++) {
						if (0 == (i % 16)) {
						printf("\n");}
						printf("%02x ", (unsigned char) recv_buffer[i]);}
						printf("\n");}
					);
				}
				m46e_entry_flg = 1;	// 2016/07/22 nextヘッダを判断して検索テーブルを別けるのはやめる暫定対処。
			}
#if 0	// 2016/07/22 nextヘッダを判断して検索テーブルを別けるのはやめる暫定対処。
			} else {
				DEBUG_LOG("fail to match IPPROTO_IPIP packet\n");
				STAT_FP_M46E_SEND_ERR;
			}
			break;

		case ME6E_IPPROTO_ETHERIP:
#endif
			// ME6E IPv6
		if ( m46e_entry_flg == 0 ){	// 2016/07/22 nextヘッダを判断して検索テーブルを別けるのはやめる暫定対処。
			// エントリ検索
			if (NULL != (entry = mx6e_match_config_table(DOMAIN_FP, &handler->conf.me6e_conf_table, &p_ip6->ip6_dst))) {
				// ヘッダ置換
				mx6e_replace_address(entry, p_ip6);

				////////////////////////////////////////////////////////////////////////
				// 送信
				////////////////////////////////////////////////////////////////////////
				if (dev_dst->fd) {
					if (0 > (send_len = send_buf_msg(dev_dst->fd, recv_buffer, recv_len))) {
						char                            mes2[1024];
						snprintf(mes2, sizeof(mes2), "fail to send ME6E_IPPROTO_ETHERIP packet (%s)\n", strerror(errno)); 
						mx6e_logging(LOG_ERR, mes2);

						STAT_FP_ME6E_SEND_ERR;
					} else {
						DEBUG_LOG("forward %ld bytes to ME6E_IPPROTO_ETHERIP\n", (unsigned long int) send_len);
						STAT_FP_ME6E_SEND_SUCCESS;
					}
				} else {
					// 送信デバイスfdがNULLの場合はデバッグダンプ
					_D_(STAT_FP_ME6E_SEND_SUCCESS;	// とりあえず送信に計上
						{
						for (int i = 0; i < recv_len; i++) {
						if (0 == (i % 16)) {
						printf("\n");}
						printf("%02x ", (unsigned char) recv_buffer[i]);}
						printf("\n");}
					);
				}
			} else {
				DEBUG_LOG("fail to match ME6E_IPPROTO_ETHERIP packet\n");
				STAT_FP_ME6E_SEND_ERR;

			}
		}
#if 0	// 2016/07/22 nextヘッダを判断して検索テーブルを別けるのはやめる暫定対処。
			break;

		default:
			// 次ヘッダがIPIP以外(カプセル化されていないパケット)の場合、黙って破棄。
			DEBUG_LOG("drop packet so that recv packet is not ipip.\n");
			STAT_FP_ERR_NXTHDR;
			break;
		}
#endif
	} else {
		// IPv6以外のパケットは、黙って破棄。
		DEBUG_LOG("Drop IPv6 Packet Ether Type : %d\n", ntohs(p_ether->h_proto));
		STAT_FP_ERR_OTHER_PROTO;
	}


	return;
}
