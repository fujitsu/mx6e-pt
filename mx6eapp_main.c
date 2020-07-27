/******************************************************************************/
/* ファイル名 : mx6eapp_main.c                                                */
/* 機能概要   : メイン関数 ソースファイル                                     */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <sys/signalfd.h>

#include "mx6eapp.h"
#include "mx6eapp_config.h"
#include "mx6eapp_log.h"
#include "mx6eapp_network.h"
#include "mx6eapp_tunnel.h"
#include "mx6eapp_pt_mainloop.h"
#include "mx6eapp_setup.h"
#include "mx6eapp_statistics.h"
#include "mx6eapp_dynamic_setting.h"
#include "mx6eapp_ct.h"

//! コマンドオプション構造体 see getopt(3)
// *INDENT-OFF*
static const struct option      options[] = {
	{"file",	required_argument,	NULL, 'f'},
	{"help",	no_argument,		NULL, 'h'},
	{"usage",	no_argument,		NULL, 'h'},
	{0, 0, 0, 0}
};
// *INDENT-ON*

///////////////////////////////////////////////////////////////////////////////
//! @brief コマンド凡例表示関数
//!
//! コマンド実行時の引数が不正だった場合などに凡例を表示する。
//!
//! @param なし
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
static void usage(void)
{
	fprintf(stderr, "Usage: mx6eapp { -f | --file } CONFIG_FILE \n" "       mx6eapp { -h | --help | --usage }\n" "\n");

	return;
}


////////////////////////////////////////////////////////////////////////////////
// メイン関数
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	// ローカル変数宣言
	mx6e_handler_t                  handler;
	int                             ret;
	char                           *conf_file;
	int                             option_index;
	pthread_t                       fp_tid = 0;
	pthread_t                       pr_tid = 0;
	int fpt_result = -1;
	int prt_result = -1;

#if defined(CT)
	// 単体用
	ct(&handler);
#endif

	ret = 0;
	conf_file = NULL;
	option_index = 0;

	// 引数チェック
	while (true) {
		int                             c = getopt_long(argc, argv, "f:h", options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'f':
			conf_file = optarg;
			break;

		case 'h':
			usage();
			exit(EXIT_SUCCESS);	// 終了

		default:
			usage();
			exit(EINVAL);		// 終了
	
		}
	}

	if (conf_file == NULL) {
		usage();
		exit(EINVAL);
	}
	// デバッグ時は設定ファイル解析前に標準エラー出力有り、
	// デバッグログ出力有りで初期化する
#ifdef DEBUG
	mx6e_initial_log(NULL, true);
#else
	mx6e_initial_log(NULL, false);
#endif

	mx6e_logging(LOG_INFO, "MX6E application start!!\n");

	// シグナルの登録
#ifndef S_SPLINT_S
	sigset_t                        sigmask;
	sigfillset(&sigmask);
	sigdelset(&sigmask, SIGILL);
	sigdelset(&sigmask, SIGSEGV);
	sigdelset(&sigmask, SIGBUS);
	sigprocmask(SIG_BLOCK, &sigmask, &handler.oldsigmask);

	handler.signalfd = signalfd(-1, &sigmask, 0);
	fcntl(handler.signalfd, F_SETFD, FD_CLOEXEC);
#endif
	
	// 設定ファイルの読み込み
	{
		mx6e_config_t                  *p = mx6e_config_load(&handler.conf, conf_file);
		if (!p) {
			mx6e_logging(LOG_ERR, "fail to load config file.\n");
			return -1;
		}
		_D_(mx6e_config_dump(&handler.conf, STDOUT_FILENO));
	}
	// 設定ファイルの内容でログ情報を再初期化
	mx6e_initial_log(handler.conf.general.process_name, handler.conf.general.debug_log);

	// デーモン化
	if (handler.conf.general.daemon) {
		if (daemon(1, 0) != 0) {
			mx6e_logging(LOG_ERR, "fail to daemonize : %s\n", strerror(errno));
			mx6e_config_destruct(&handler.conf);
			return -1;
		}
	}
	// 統計情報初期化
	mx6e_initial_statistics(&handler.stat_info);

	// ネットワークデバイス生成
	if (mx6e_create_network_device(&handler) != 0) {
		mx6e_logging(LOG_ERR, "fail to netowrk device\n");
		// 後始末
		mx6e_config_destruct(&handler.conf);
		return -1;
	}
	////////////////////////////////////////////////////////////////////////
	// FP側のネットワークデバイス起動
	if (mx6e_start_fp_network(&handler) != 0) {
		// 異常終了
		ret = -1;
		goto proc_end;
	}
	////////////////////////////////////////////////////////////////////////
	// PR側のネットワークデバイス起動
	if (mx6e_start_pr_network(&handler) != 0) {
		// 異常終了
		ret = -1;
		goto proc_end;
	}
	////////////////////////////////////////////////////////////////////////

	// FP->PR パケット送受信スレッド起動
	if (0 != (fpt_result = pthread_create(&fp_tid, NULL, mx6e_tunnel_fp_thread, &handler))) {
		mx6e_logging(LOG_ERR, "fail to create IPv6 tunnel thread : %s\n", strerror(errno));
	}
	// PR->FP パケット送受信スレッド起動
	if (0 != (prt_result = pthread_create(&pr_tid, NULL, mx6e_tunnel_pr_thread, &handler))) {
		mx6e_logging(LOG_ERR, "fail to create IPv6 tunnel thread : %s\n", strerror(errno));
	}
	// コマンド処理ループ
	DEBUG_LOG("mx6e_pt_mainloop start");
	mx6e_pt_mainloop(&handler);

  proc_end:
	if (0 == fpt_result) {
		// FPスレッドの取り消し
		pthread_cancel(fp_tid);
		// スレッドのjoin
		DEBUG_LOG("waiting for IPv6 FP thread end.");

		pthread_join(fp_tid, NULL);
		DEBUG_LOG("IPv6 FP thread done.");
	}
	if (0 == prt_result) {
		// PRスレッドの取り消し
		pthread_cancel(pr_tid);
		// スレッドのjoin
		DEBUG_LOG("waiting for IPv6 PR thread end.");
		pthread_join(pr_tid, NULL);
		DEBUG_LOG("IPv6 PR thread done.");
	}

	mx6e_config_destruct(&handler.conf);

	mx6e_logging(LOG_INFO, "MX6E application finish!!\n");
	DEBUG_LOG("MX6E application finish!!");
	return ret;
}
