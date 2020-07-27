/******************************************************************************/
/* ファイル名 : mx6eapp_statistics.h                                          */
/* 機能概要   : 統計情報管理 ヘッダファイル                                   */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#ifndef __MX6EAPP_STATISTICS_H__
#   define __MX6EAPP_STATISTICS_H__

#   include <stdint.h>

////////////////////////////////////////////////////////////////////////////////
//! デバイス統計情報 構造体
////////////////////////////////////////////////////////////////////////////////
typedef struct _mx6e_statistics_t {
	////////////////////////////////////////////////////////////////////////////
	// FP domain 関連
	////////////////////////////////////////////////////////////////////////////
	//! パケット受信数
	uint32_t                        fp_recieve;
	//! パケット送信数
	uint32_t                        fp_send;
	//! パケット送信成功数
	uint32_t                        fp_m46e_send_success;
	uint32_t                        fp_me6e_send_success;
	//! パケット送信失敗数
	uint32_t                        fp_m46e_send_err;
	uint32_t                        fp_me6e_send_err;
	//! ブロードキャストパケット受信数
	uint32_t                        fp_err_broadcast;
	//! HOPLIMIT超過パケット(デカプセル化後)受信数
	uint32_t                        fp_err_hoplimit;
	//! IPv6以外のプロトコルパケット受信数
	uint32_t                        fp_err_other_proto;
	//! NextHeaderがIPIP以外のパケット受信数
	uint32_t                        fp_err_nxthdr;

	////////////////////////////////////////////////////////////////////////////
	// PR domain 関連
	////////////////////////////////////////////////////////////////////////////
	//! パケット受信数
	uint32_t                        pr_recieve;
	//! 4ユニキャストパケット(デカプセル化後)受信数
	uint32_t                        pr_recv_unicast;
	//! IPv4マルチキャストパケット(デカプセル化後)受信数
	uint32_t                        pr_recv_multicast;
	//! パケット送信数
	uint32_t                        pr_send;
	//! パケット送信成功数
	uint32_t                        pr_m46e_send_success;
	uint32_t                        pr_me6e_send_success;
	//! パケット送信失敗数
	uint32_t                        pr_m46e_send_err;
	uint32_t                        pr_me6e_send_err;
	//! ブロードキャストパケット受信数
	uint32_t                        pr_err_broadcast;
	//! HOPLIMIT超過パケット(デカプセル化後)受信数
	uint32_t                        pr_err_hoplimit;
	//! IPpr以外のプロトコルパケット受信数
	uint32_t                        pr_err_other_proto;
	//! NextHeaderがIPIP以外のパケット受信数
	uint32_t                        pr_err_nxthdr;

} mx6e_statistics_t;


///! 統計情報
extern mx6e_statistics_t       *mx6e_statistics;

///////////////////////////////////////////////////////////////////////////////
// 外部関数プロトタイプ
///////////////////////////////////////////////////////////////////////////////
void                            mx6e_initial_statistics(mx6e_statistics_t * stat_info);
void                            mx6e_printf_statistics_info_normal(mx6e_statistics_t * statistics, int fd);

///////////////////////////////////////////////////////////////////////////////
// カウントアップ用の処理はdefineで定義する
///////////////////////////////////////////////////////////////////////////////
#   define STAT_FP_RECIEVE				(mx6e_statistics->fp_recieve ++)
#   define STAT_FP_SEND					(mx6e_statistics->fp_send ++)
#   define STAT_FP_M46E_SEND_SUCCESS	(mx6e_statistics->fp_send ++, mx6e_statistics->fp_m46e_send_success ++)
#   define STAT_FP_M46E_SEND_ERR		(mx6e_statistics->fp_send ++, mx6e_statistics->fp_m46e_send_err ++)
#   define STAT_FP_ME6E_SEND_SUCCESS	(mx6e_statistics->fp_send ++, mx6e_statistics->fp_me6e_send_success ++)
#   define STAT_FP_ME6E_SEND_ERR		(mx6e_statistics->fp_send ++, mx6e_statistics->fp_me6e_send_err ++)
#   define STAT_FP_ERR_BROADCAST		(mx6e_statistics->fp_err_broadcast ++)
#   define STAT_FP_ERR_HOPLIMIT			(mx6e_statistics->fp_err_hoplimit ++)
#   define STAT_FP_ERR_OTHER_PROTO		(mx6e_statistics->fp_err_other_proto ++)
#   define STAT_FP_ERR_NXTHDR			(mx6e_statistics->fp_err_nxthdr ++)

#   define STAT_PR_RECIEVE				(mx6e_statistics->pr_recieve ++)
#   define STAT_PR_SEND					(mx6e_statistics->pr_send ++)
#   define STAT_PR_M46E_SEND_SUCCESS	(mx6e_statistics->pr_send ++, mx6e_statistics->pr_m46e_send_success ++)
#   define STAT_PR_M46E_SEND_ERR		(mx6e_statistics->pr_send ++, mx6e_statistics->pr_m46e_send_err ++)
#   define STAT_PR_ME6E_SEND_SUCCESS	(mx6e_statistics->pr_send ++, mx6e_statistics->pr_me6e_send_success ++)
#   define STAT_PR_ME6E_SEND_ERR		(mx6e_statistics->pr_send ++, mx6e_statistics->pr_me6e_send_err ++)
#   define STAT_PR_ERR_BROADCAST		(mx6e_statistics->pr_err_broadcast ++)
#   define STAT_PR_ERR_HOPLIMIT			(mx6e_statistics->pr_err_hoplimit ++)
#   define STAT_PR_ERR_OTHER_PROTO		(mx6e_statistics->pr_err_other_proto ++)
#   define STAT_PR_ERR_NXTHDR			(mx6e_statistics->pr_err_nxthdr ++)

#endif												// __MX6EAPP_STATISTICS_H__
