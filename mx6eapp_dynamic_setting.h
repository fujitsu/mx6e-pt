/******************************************************************************/
/* ファイル名 : mx6eapp_dynamic_setting.h                                     */
/* 機能概要   : 動的定義変更 ヘッダファイル                                   */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#ifndef __MX6EAPP_DYNAMIC_SETTING_H__
#   define __MX6EAPP_DYNAMIC_SETTING_H__

#   include <stdbool.h>
#   include "mx6eapp.h"
#	include "mx6eapp_command_data.h"

////////////////////////////////////////////////////////////////////////////////
// 外部関数プロトタイプ宣言
////////////////////////////////////////////////////////////////////////////////
extern void                     mx6eapp_set_flag_restart(bool flg);
extern bool                     mx6eapp_get_flag_restart(void);
extern bool                     mx6eapp_set_debug_log(mx6e_handler_t * handler, mx6e_command_t * command, int fd);

#endif												// __MX6EAPP_DYNAMIC_SETTING_H__
