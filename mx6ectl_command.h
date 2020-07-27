/******************************************************************************/
/* ファイル名 : mx6ectl_command.h                                             */
/* 機能概要   : 内部コマンドクラス ヘッダファイル                             */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#ifndef __MX6ECTL_COMMAND_H__
#   define __MX6ECTL_COMMAND_H__

#   include <stdbool.h>
#   include "mx6eapp_command_data.h"

////////////////////////////////////////////////////////////////////////////////
// デファイン定義
////////////////////////////////////////////////////////////////////////////////
//! キー名
#   define OPT_PHYSICAL_DEV    "physical_dev"
#   define OPT_DEV_NAME        "name"
#   define OPT_IPV4_ADDRESS    "ipv4_address"
#   define OPT_IPV4_GATEWAY    "ipv4_gateway"
#   define OPT_MTU             "mtu"
#   define OPT_HWADDR          "hwaddr"

//! 設定ファイルを読込む場合の１行あたりの最大文字数
#   define OPT_LINE_MAX            256

//! デバイス操作コマンド引数
#   define DYNAMIC_OPE_ARGS_NUM_MAX    9

//! debugログモード設定コマンド引数
#   define DYNAMIC_OPE_DBGLOG_ARGS 6

//! M46E-PT Entry追加コマンド引数
#   define ADD_M46E_OPE_MIN_ARGS 12
#   define ADD_M46E_OPE_MAX_ARGS 13
#   define ADD_ME6E_OPE_MIN_ARGS 12
#   define ADD_ME6E_OPE_MAX_ARGS 13

//! M46E-PT Entry削除コマンド引数
#   define DEL_M46E_OPE_ARGS 10
#   define DEL_ME6E_OPE_ARGS 10

//! M46E-PT Entry全削除コマンド引数
#   define DELALL_M46E_OPE_ARGS 5
#   define DELALL_ME6E_OPE_ARGS 5

//! M46E-PT Entry活性化コマンド引数
#   define ENABLE_M46E_OPE_ARGS 10
#   define ENABLE_ME6E_OPE_ARGS 10

//! M46E-PT Entry非活性化コマンド引数
#   define DISABLE_M46E_OPE_ARGS 10
#   define DISABLE_ME6E_OPE_ARGS 10

//! M46E-PT Entry表示コマンド引数
#   define SHOW_M46E_OPE_ARGS 5
#   define SHOW_ME6E_OPE_ARGS 5

//! Config情報表示コマンド引数
#   define SHOW_CONF_OPE_ARGS 5

//! 統計情報表示コマンド引数
#   define SHOW_STAT_OPE_ARGS 5

//! MX6E-PR Entry一括設定ファイル読込コマンド引数
#   define OPE_NUM_LOAD 6

//! 区切り文字(strtok用)
#   define DELIMITER   " \t"

///////////////////////////////////////////////////////////////////////////////
//! KEY/VALUE格納用構造体
///////////////////////////////////////////////////////////////////////////////
typedef struct _opt_keyvalue_t {
	char                            key[OPT_LINE_MAX];		///< KEYの値
	char                            value[OPT_LINE_MAX];	///< VALUEの値
} opt_keyvalue_t;


////////////////////////////////////////////////////////////////////////////////
// 外部関数プロトタイプ宣言
////////////////////////////////////////////////////////////////////////////////
bool                            mx6e_command_device_set_option(int num, char *opt[], mx6e_command_t * command);
bool                            mx6e_command_dbglog_set_option(int num, char *opt[], mx6e_command_t * command);
bool                            mx6e_command_add_entry_option(int num, char *opt[], mx6e_command_t * command);
bool                            mx6e_command_del_entry_option(int num, char *opt[], mx6e_command_t * command);
bool                            mx6e_command_enable_entry_option(int num, char *opt[], mx6e_command_t * command);
bool                            mx6e_command_disable_entry_option(int num, char *opt[], mx6e_command_t * command);
bool                            mx6e_command_load(char *filename, mx6e_command_t * command, char *name);
bool                            mx6e_command_send(mx6e_command_t * command, char *name);
bool                            mx6e_command_parse_file(char *line, int *num, char *cmd_opt[]);

#endif												// __MX6ECTL_COMMAND_H__
