/******************************************************************************/
/* ファイル名 : mx6eapp_setup.h                                               */
/* 機能概要   : ネットワーク設定クラス ヘッダファイル                         */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#ifndef __MX6EAPP_SETUP_H__
#   define __MX6EAPP_SETUP_H__

////////////////////////////////////////////////////////////////////////////////
// 外部関数プロトタイプ宣言
////////////////////////////////////////////////////////////////////////////////
int                             mx6e_setup_plane_prefix(mx6e_handler_t * handler);
int                             mx6e_create_network_device(mx6e_handler_t * handler);
int                             mx6e_move_network_device(mx6e_handler_t * handler);
int                             mx6e_setup_pr_network(mx6e_handler_t * handler);
int                             mx6e_start_pr_network(mx6e_handler_t * handler);
int                             mx6e_setup_fp_network(mx6e_handler_t * handler);
int                             mx6e_start_fp_network(mx6e_handler_t * handler);
int                             mx6e_startup_script(mx6e_handler_t * handler);
int                             mx6e_create_network_device(mx6e_handler_t * handler);

#endif												// __MX6EAPP_SETUP_H__
