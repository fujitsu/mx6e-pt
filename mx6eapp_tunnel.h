/******************************************************************************/
/* ファイル名 : mx6eapp_tunnel.h                                              */
/* 機能概要   : トンネルクラス ヘッダファイル                                 */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#ifndef __MX6EAPP_TUNNEL_H__
#   define __MX6EAPP_TUNNEL_H__

#   	include "mx6eapp_log.h"

////////////////////////////////////////////////////////////////////////////////
// 外部関数プロトタイプ宣言
////////////////////////////////////////////////////////////////////////////////
void                           *mx6e_tunnel_pr_thread(void *arg);
void                           *mx6e_tunnel_fp_thread(void *arg);
void                            tunnel_forward_fp2pr_packet(mx6e_handler_t * handler, char *recv_buffer, ssize_t recv_len);
void                            tunnel_forward_pr2fp_packet(mx6e_handler_t * handler, char *recv_buffer, ssize_t recv_len);

#endif												// __MX6EAPP_TUNNEL_H__
