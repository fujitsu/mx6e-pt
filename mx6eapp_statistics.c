/******************************************************************************/
/* ファイル名 : mx6eapp_statistics.c                                          */
/* 機能概要   : 統計情報管理 ソースファイル                                   */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>

#include "mx6eapp_statistics.h"
#include "mx6eapp_log.h"

// 統計情報
mx6e_statistics_t              *mx6e_statistics;

///////////////////////////////////////////////////////////////////////////////
//! @brief 受信パケット合計数取得関数
//!
//! @param [in] statistics_info 統計情報用領域のポインタ
//!
//! @return 受信パケット合計数
///////////////////////////////////////////////////////////////////////////////
static inline uint32_t statistics_get_total_recv(mx6e_statistics_t * statistics_info)
{
	uint32_t                        result = 0;

	result += statistics_info->fp_recieve;
	result += statistics_info->pr_recieve;

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 送信パケット合計数取得関数
//!
//! @param [in] statistics_info 統計情報用領域のポインタ
//!
//! @return 送信パケット合計数
///////////////////////////////////////////////////////////////////////////////
static inline uint32_t statistics_get_total_send(mx6e_statistics_t * statistics_info)
{
	uint32_t                        result = 0;

	result += statistics_info->fp_send;
	result += statistics_info->pr_send;

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief ドロップパケット合計数取得関数
//!
//! @param [in] statistics_info 統計情報用領域のポインタ
//!
//! @return ドロップパケット合計数
///////////////////////////////////////////////////////////////////////////////
static inline uint32_t statistics_get_total_drop(mx6e_statistics_t * statistics_info)
{
	uint32_t                        result = 0;

	result += statistics_info->fp_err_broadcast;
	result += statistics_info->fp_err_hoplimit;
	result += statistics_info->fp_err_other_proto;
	result += statistics_info->fp_err_nxthdr;

	result += statistics_info->pr_err_broadcast;
	result += statistics_info->pr_err_hoplimit;
	result += statistics_info->pr_err_other_proto;
	result += statistics_info->pr_err_nxthdr;

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief エラーパケット合計数取得関数
//!
//! @param [in] statistics_info 統計情報用領域のポインタ
//!
//! @return エラーパケット合計数
///////////////////////////////////////////////////////////////////////////////
static inline uint32_t statistics_get_total_error(mx6e_statistics_t * statistics_info)
{
	uint32_t                        result = 0;

	result += statistics_info->fp_m46e_send_err;
	result += statistics_info->fp_me6e_send_err;
	result += statistics_info->pr_m46e_send_err;
	result += statistics_info->pr_me6e_send_err;

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 統計情報出力関数(MX6E 通常モード)
//!
//! 統計情報を引数で指定されたディスクリプタへ出力する。
//!
//! @param [in] statistics_info 統計情報用領域のポインタ
//! @param [in] fd              統計情報出力先のディスクリプタ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
void mx6e_printf_statistics_info_normal(mx6e_statistics_t * statistics_info, int fd)
{
#define DPRINTF(fd, ...)	if (0 > fd) {printf(__VA_ARGS__);} else {dprintf(fd, __VA_ARGS__);}
	// 統計情報をファイルへ出力する
	DPRINTF(fd, "【MX6E】\n");
	DPRINTF(fd, "\n");
	DPRINTF(fd, "   packet count\n");
	DPRINTF(fd, "     total recieve count             : %d\n", statistics_get_total_recv(statistics_info));
	DPRINTF(fd, "     total send count                : %d\n", statistics_get_total_send(statistics_info));
	DPRINTF(fd, "     total drop count                : %d\n", statistics_get_total_drop(statistics_info));
	DPRINTF(fd, "     total error count               : %d\n", statistics_get_total_error(statistics_info));
	DPRINTF(fd, "\n");
	DPRINTF(fd, "\n");
	DPRINTF(fd, "【FP domain】\n");
	DPRINTF(fd, "\n");
	DPRINTF(fd, "   packet count\n");
	DPRINTF(fd, "     recieve count                   : %d \n", statistics_info->fp_recieve);
	DPRINTF(fd, "       broadcast(drop)               : %d \n", statistics_info->fp_err_broadcast);
	DPRINTF(fd, "       not IPv6 protocol(drop)       : %d \n", statistics_info->fp_err_other_proto);
	DPRINTF(fd, "       hop limit over(drop)          : %d \n", statistics_info->fp_err_hoplimit);
	DPRINTF(fd, "       invalid next header(drop)     : %d \n", statistics_info->fp_err_nxthdr);
	DPRINTF(fd, "     send count                      : %d \n", statistics_info->fp_send);
	DPRINTF(fd, "       m46e send success             : %d \n", statistics_info->fp_m46e_send_success);
	DPRINTF(fd, "       m46e send error               : %d \n", statistics_info->fp_m46e_send_err);
	DPRINTF(fd, "       me6e send success             : %d \n", statistics_info->fp_me6e_send_success);
	DPRINTF(fd, "       me6e send error               : %d \n", statistics_info->fp_me6e_send_err);
	DPRINTF(fd, "\n");
	DPRINTF(fd, "\n");
	DPRINTF(fd, "【PR domain】\n");
	DPRINTF(fd, "\n");
	DPRINTF(fd, "   packet count\n");
	DPRINTF(fd, "     recieve count                   : %d \n", statistics_info->pr_recieve);
	DPRINTF(fd, "       broadcast(drop)               : %d \n", statistics_info->pr_err_broadcast);
	DPRINTF(fd, "       not IPv6 protocol(drop)       : %d \n", statistics_info->pr_err_other_proto);
	DPRINTF(fd, "       hop limit over(drop)          : %d \n", statistics_info->pr_err_hoplimit);
	DPRINTF(fd, "       invalid next header(drop)     : %d \n", statistics_info->pr_err_nxthdr);
	DPRINTF(fd, "     send count                      : %d \n", statistics_info->pr_send);
	DPRINTF(fd, "       m46e send success             : %d \n", statistics_info->pr_m46e_send_success);
	DPRINTF(fd, "       m46e send error               : %d \n", statistics_info->pr_m46e_send_err);
	DPRINTF(fd, "       me6e send success             : %d \n", statistics_info->pr_me6e_send_success);
	DPRINTF(fd, "       me6e send error               : %d \n", statistics_info->pr_me6e_send_err);
	DPRINTF(fd, "\n");

	return;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 統計情報領域初期化
//!
//! 統計情報領域初期化
//!
//! @param [in] key_path 共有メモリ作成のキー。
//!             設定ファイルの絶対パス名を使用する。
//!
//! @return 作成した統計情報領域のポインタ
///////////////////////////////////////////////////////////////////////////////
void mx6e_initial_statistics(mx6e_statistics_t * stat_info)
{
	memset(stat_info, 0, sizeof(mx6e_statistics_t));
	mx6e_statistics = stat_info;

	return;
}
