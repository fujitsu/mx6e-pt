/******************************************************************************/
/* ファイル名 : mx6eapp_socket.h                                              */
/* 機能概要   : ソケット送受信クラス ヘッダファイル                           */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#ifndef __MX6EAPP_SOCKET_H__
#   define __MX6EAPP_SOCKET_H__

#   include "mx6eapp_command_data.h"

////////////////////////////////////////////////////////////////////////////////
// 外部関数プロトタイプ宣言
////////////////////////////////////////////////////////////////////////////////
int                             mx6e_socket_send(int sockfd, mx6e_command_code_t command, void *data, size_t size, int fd);
int                             mx6e_socket_recv(int sockfd, mx6e_command_code_t * command, void *data, size_t size, int *fd);
int                             mx6e_socket_send_cred(int sockfd, mx6e_command_code_t command, void *data, size_t size);
int                             mx6e_socket_recv_cred(int sockfd, mx6e_command_code_t * command, void *data, size_t size);

#endif												// __MX6EAPP_SOCKET_H__
