/******************************************************************************/
/* ファイル名 : mx6eapp.h                                                     */
/* 機能概要   : MX6E共通ヘッダファイル                                        */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#ifndef __MX6EAPP_H__
#   define __MX6EAPP_H__

#   include <unistd.h>
#   include <netinet/in.h>

#   include "mx6eapp_config.h"
#   include "mx6eapp_statistics.h"


////////////////////////////////////////////////////////////////////////////////
// 外部マクロ定義
////////////////////////////////////////////////////////////////////////////////
//!IPv6のMTU最小値
#   define IPV6_MIN_MTU    1280
//!IPv6のプレフィックス最大値
#   define IPV6_PREFIX_MAX 128
//!IPv4のプレフィックス最大値
#   define IPV4_PREFIX_MAX 32
//!ポート番号のbit長の最大値
#   define PORT_BIT_MAX    16

//! EtherIPのプロトコル番号(ME6EのNext Header)
#   define ME6E_IPPROTO_ETHERIP   97


//! IPv4 0アドレス
extern struct in_addr           in_addr0;
//! IPv6 0アドレス
extern struct in6_addr          in6_addr0;
//! MAC 0アドレス
extern struct ether_addr        mac0;

////////////////////////////////////////////////////////////////////////////////
// 外部構造体定義
////////////////////////////////////////////////////////////////////////////////
//! MX6Eアプリケーションハンドラ
typedef struct {
	mx6e_config_t                   conf;			///< 設定情報
	mx6e_statistics_t               stat_info;		///< 統計情報
	int                             signalfd;		///< シグナル受信用ディスクリプタ
	sigset_t                        oldsigmask;		///< プロセス起動時のシグナルマスク
} mx6e_handler_t;

#endif												// __MX6EAPP_H__
