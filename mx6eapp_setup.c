/******************************************************************************/
/* ファイル名 : mx6eapp_setup.c                                               */
/* 機能概要   : ネットワーク設定クラス ソースファイル                         */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <netinet/ip6.h>
#include <netinet/ether.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "mx6eapp.h"
#include "mx6eapp_setup.h"
#include "mx6eapp_log.h"
#include "mx6eapp_network.h"
#include "mx6eapp_pt.h"
#include "mx6eapp_util.h"

////////////////////////////////////////////////////////////////////////////////
// 内部関数プロトタイプ宣言
////////////////////////////////////////////////////////////////////////////////
static int                      run_startup_script(const char *filename, const char *process_name, const char *type, const char *device_name);



///////////////////////////////////////////////////////////////////////////////
//! @brief PRネットワーク設定関数
//!
//! PRネットワークの初期設定をおこなう。具体的には
//!
//!   - IPv4トンネルデバイスのデバイス名変更
//!   - IPv4トンネルデバイスへのIPv4アドレスの割り当て
//!     (設定ファイルに書かれていれば)
//!   - 移動した仮想デバイスのデバイス名変更
//!   - 移動した仮想デバイスへのIPv4アドレスの割り当て
//!     (設定ファイルに書かれていれば)
//!   - 移動した仮想デバイスのMACアドレス変更
//!     (設定ファイルに書かれていれば)
//!
//!   をおこなう。
//!
//! @param [in]     handler      MX6Eハンドラ
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_setup_pr_network(mx6e_handler_t * handler)
{
	// デバイスのIPv6アドレス設定
	if (!compare_ipv6(&handler->conf.devices.tunnel_pr.ipv6_address, &in6_addr0)) {
		mx6e_network_add_ipaddr(AF_INET6, handler->conf.devices.tunnel_pr.ifindex, (void *) (&handler->conf.devices.tunnel_pr.ipv6_address), handler->conf.devices.tunnel_pr.ipv6_prefixlen);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief Fpネットワーク設定関数
//!
//! Fpネットワークの初期設定をおこなう。具体的には
//!
//!   - IPv6トンネルデバイスへのIPv6アドレスの割り当て
//!     (設定ファイルに書かれていれば)
//!
//!   をおこなう。
//!
//! @param [in]     handler      MX6Eハンドラ
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_setup_fp_network(mx6e_handler_t * handler)
{
	// デバイスのIPv6アドレス設定
	if (!compare_ipv6(&handler->conf.devices.tunnel_fp.ipv6_address, &in6_addr0)) {
		mx6e_network_add_ipaddr(AF_INET6, handler->conf.devices.tunnel_fp.ifindex, (void *) (&handler->conf.devices.tunnel_fp.ipv6_address), handler->conf.devices.tunnel_fp.ipv6_prefixlen);
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief PRネットワーク起動関数
//!
//! PRネットワークの起動をおこなう。具体的には
//!
//!   - ループバックデバイスのUP
//!   - IPv4トンネルデバイスのUP
//!   - IPv4トンネルデバイスのデフォルトゲートウェイの設定
//!     (設定ファイルに書かれていれば)
//!   - MX6E-AS共有アドレスへの経路設定
//!     (MX6E-ASモードの場合のみ)
//!   - 移動した仮想デバイスのUP
//!   - 移動した仮想デバイスのデフォルトゲートウェイの設定
//!     (設定ファイルに書かれていれば)
//!
//!   をおこなう。
//!
//! @param [in]     handler      MX6Eハンドラ
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_start_pr_network(mx6e_handler_t * handler)
{
	int                             ret;

	// デバイスの活性化
	ret = mx6e_network_set_flags_by_index(handler->conf.devices.tunnel_pr.ifindex, IFF_UP);
	if (ret != 0) {
		mx6e_logging(LOG_WARNING, "FP IPv6 device up failed : %s\n", strerror(ret));
	}
	// ip_forwardingの設定
	FILE                           *fp;
	if ((fp = fopen("/proc/sys/net/ipv6/conf/all/forwarding", "w")) != NULL) {
		fprintf(fp, "1");
		fclose(fp);
	} else {
		mx6e_logging(LOG_WARNING, "ipv6/ip_forward set failed.\n");
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief FPネットワーク起動関数
//!
//! FPネットワークの起動をおこなう。具体的には
//!
//!   - IPv6デバイスのUP
//!
//!   をおこなう。
//!
//! @param [in]     handler      MX6Eハンドラ
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_start_fp_network(mx6e_handler_t * handler)
{
	int                             ret;

	// IPv6側のデバイス活性化
	ret = mx6e_network_set_flags_by_index(handler->conf.devices.tunnel_fp.ifindex, IFF_UP);
	if (ret != 0) {
		mx6e_logging(LOG_WARNING, "FP IPv6 device up failed : %s\n", strerror(ret));
	}
	// ip_forwardingの設定
	FILE                           *fp;
	if ((fp = fopen("/proc/sys/net/ipv6/conf/all/forwarding", "w")) != NULL) {
		fprintf(fp, "1");
		fclose(fp);
	} else {
		mx6e_logging(LOG_WARNING, "ipv6/ip_forward set failed.\n");
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief スタートアップスクリプト実行関数
//!
//! @param [in]     handler      MX6Eハンドラ
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_startup_script(mx6e_handler_t * handler)
{
	return run_startup_script(handler->conf.general.startup_script, handler->conf.general.process_name, handler->conf.devices.tunnel_pr.name, handler->conf.devices.tunnel_fp.name);
}

///////////////////////////////////////////////////////////////////////////////
//! @brief スタートアップスクリプト実行関数
//!
//! @param [in]     handler         MX6Eハンドラ
//! @param [in]     pr_device_name  pr_device_name
//! @param [in]     fp_device_name  fp_device_name
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
// #  - FP側
// #    startup.sh mx6e0 fp eth0
// #  - PR側
// #    startup.sh mx6e0 pr eth1
///////////////////////////////////////////////////////////////////////////////
static int run_startup_script(const char *filename, const char *process_name, const char *pr_device_name, const char *fp_device_name)
{

	if ((!filename) || (!process_name) || (!pr_device_name) || (!fp_device_name)) {

		// スクリプトが指定されていないので、何もせずにリターン
		mx6e_logging(LOG_WARNING, "some arg is NULL\n");
		return 0;
	}
	// コマンド長分のデータ領域確保
	char                           *command = NULL;
	int                             command_len = asprintf(&command, "%s %s %s %s & 2>&1",
														   filename,
														   process_name,
														   pr_device_name,
														   fp_device_name);

	if (command_len > 0) {
		DEBUG_LOG("run startup script : %s\n", command);

		FILE                           *fp = popen(command, "w");
		if (fp != NULL) {
			char                            buf[256];
			while (fgets(buf, sizeof(buf), fp) != NULL) {
				// &つけてバックグラウンド実行したので実行結果は読めない
				DEBUG_LOG("script output : %s", buf);
			}
			pclose(fp);
		} else {
			mx6e_logging(LOG_WARNING, "run script error : %s\n", strerror(errno));
		}

		free(command);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 送信用ソケット作成
//!
//! 送信用ソケット作成
//!
//! @param [in]  ifname    デバイス名
//! @param [out] flags     取得したフラグの格納先
//!
//! @retval fd    ソケットファイルディスクリプタ
//! @retval -1     異常終了
///////////////////////////////////////////////////////////////////////////////
static int create_send_socket(mx6e_handler_t * handler)
{
#define BB_SND_BUF_SIZE    262142
#define BB_RCV_BUF_SIZE    262142

	int                             sock = socket(PF_INET6, SOCK_RAW, ME6E_IPPROTO_ETHERIP);
	if (sock < 0) {
		mx6e_logging(LOG_ERR, "socket open error : %s.", strerror(errno));
		return -1;
	}
	// 送信バッファサイズの設定
	int                             send_buf_size = BB_SND_BUF_SIZE;
	if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &send_buf_size, sizeof(send_buf_size)) < 0) {
		mx6e_logging(LOG_ERR, "fail to set sockopt SO_SNDBUF : %s.", strerror(errno));
		return -1;
	}
	// 送信バッファサイズの設定確認
	socklen_t                       len = sizeof(socklen_t);
	send_buf_size = 0;
	if (getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &send_buf_size, &len) < 0) {
		mx6e_logging(LOG_ERR, "fail to get sockopt SO_SNDBUF : %s.", strerror(errno));
		return -1;
	}
	mx6e_logging(LOG_INFO, "set send buffer size : %d.", send_buf_size);

	// 受信バッファサイズの設定
	int                             rcv_buf_size = BB_RCV_BUF_SIZE;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcv_buf_size, sizeof(rcv_buf_size)) < 0) {
		mx6e_logging(LOG_ERR, "fail to set sockopt SO_SNDBUF : %s.", strerror(errno));
		return -1;
	}
	// 受信バッファサイズの設定確認
	rcv_buf_size = 0;
	if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcv_buf_size, &len) < 0) {
		mx6e_logging(LOG_ERR, "fail to get sockopt SO_SNDBUF : %s.", strerror(errno));
		return -1;
	}
	mx6e_logging(LOG_INFO, "set recieve buffer size : %d.", rcv_buf_size);

	// // 送信情報はヘッダを含むことを指定
	// // IPV6_HDRINCLは未定義
	// int                             on = 1;
	// if (setsockopt(sock, SOL_SOCKET, IPV6_HDRINCL, &on, sizeof(on))) {
	// 	mx6e_logging(LOG_ERR, "fail to set sockopt IPV6_HDRINCL : %s.", strerror(errno));
	// 	close(sock);
	// 	return -1;
	// }
	
	// 送信元情報としてin6_packetinfoを使用
	int                             on = 1;
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on))) {
		mx6e_logging(LOG_ERR, "fail to set sockopt IPV6_RECVPKTINFO : %s.", strerror(errno));
		close(sock);
		return -1;
	}

	// ソケットを返す
	return sock;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief ネットワークデバイス生成関数
//!
//! 設定ファイルで指定されているトンネルデバイスと仮想デバイスを生成する。
//!
//! @param [in]     handler      M46Eハンドラ
//!
//! @retval 0     正常終了
//! @retval 0以外 異常終了
///////////////////////////////////////////////////////////////////////////////
int mx6e_create_network_device(mx6e_handler_t * handler)
{
	mx6e_config_t                  *conf = &handler->conf;
	char                            mes[1024];

	// FP側
	if (0 != mx6e_network_create_tap(conf->devices.tunnel_fp.name, &conf->devices.tunnel_fp)) {
		mx6e_logging(LOG_ERR, "fail to create FP tunnel device\n");
		return -1;
	}

	if (-1 == (handler->conf.devices.send_sock_fd_fp = create_send_socket(handler))) {
		snprintf(mes, sizeof(mes), "fail to create FP socket (%s)\n", strerror(errno));
		mx6e_logging(LOG_ERR, mes);
		return -1;
	}
	// PR側
	if (0 != mx6e_network_create_tap(conf->devices.tunnel_pr.name, &conf->devices.tunnel_pr)) {
		mx6e_logging(LOG_ERR, "fail to create PR tunnel device\n");
		return -1;
	}
	if (-1 == (handler->conf.devices.send_sock_fd_pr = create_send_socket(handler))) {
		snprintf(mes, sizeof(mes), "fail to create PR socket (%s)\n", strerror(errno));
		mx6e_logging(LOG_ERR, mes);
		return -1;
	};

	return 0;
}
