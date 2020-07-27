/******************************************************************************/
/* ファイル名 : mx6eapp_command_data.h                                        */
/* 機能概要   : コマンドデータ共通定義 ヘッダファイル                         */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#ifndef __MX6EAPP_COMMAND_DATA_H__
#   define __MX6EAPP_COMMAND_DATA_H__

#   include <stdbool.h>
#   include <netinet/in.h>
#   include <net/if.h>
#   include <net/ethernet.h>
#   include <stdbool.h>

#   include "mx6eapp_config.h"

//! UNIXドメインソケット名
#   define MX6E_COMMAND_SOCK_NAME  "/mx6e/%s/command"

//! コマンド変数
#   define CMDOPT_NUM_MAX 1
#   define CMDOPT_LEN_MAX 256

//! コマンドコード
typedef enum {
	MX6E_COMMAND_NONE,
	// 運用前の同期専用
	MX6E_SETUP_FAILURE,								///< 設定失敗
	MX6E_CHILD_INIT_END,							///< 子プロセス初期化完了
	MX6E_NETDEV_MOVED,								///< ネットワークデバイスの移動完了
	MX6E_NETWORK_CONFIGURE,							///< PRネットワーク設定完了
	MX6E_START_OPERATION,							///< 運用開始指示
	// ここから運用中のコマンド
	MX6E_SHOW_CONF,									///< config表示
	MX6E_SHOW_STATISTIC,							///< 統計情報表示
	MX6E_SHUTDOWN,									///< シャットダウン指示
	MX6E_RESTART,									///< リスタート指示
	MX6E_PR_TABLE_GENERATE_FAILURE,					///< MX6E-PRテーブル生成失敗

	MX6E_ADD_M46E_ENTRY,							///< M46E ENTRY 追加
	MX6E_DEL_M46E_ENTRY,							///< M46E ENTRY 削除
	MX6E_DELALL_M46E_ENTRY,							///< M46E ENTRY 全削除
	MX6E_ENABLE_M46E_ENTRY,							///< M46E ENTRY 活性化
	MX6E_DISABLE_M46E_ENTRY,						///< M46E ENTRY 非活性化
	MX6E_SHOW_M46E_ENTRY,							///< M46E ENTRY 表示

	MX6E_ADD_ME6E_ENTRY,							///< ME6E ENTRY 追加
	MX6E_DEL_ME6E_ENTRY,							///< ME6E ENTRY 削除
	MX6E_DELALL_ME6E_ENTRY,							///< ME6E ENTRY 全削除
	MX6E_ENABLE_ME6E_ENTRY,							///< ME6E ENTRY 活性化
	MX6E_DISABLE_ME6E_ENTRY,						///< ME6E ENTRY 非活性化
	MX6E_SHOW_ME6E_ENTRY,							///< ME6E ENTRY 表示

	MX6E_LOAD_COMMAND,								///< M46E/ME6E-Commandファイル読み込み

	MX6E_SET_DEBUG_LOG,								///< 動的定義変更 デバッグログ出力設定
	MX6E_SET_DEBUG_LOG_END,							///< 動的定義変更 デバッグログ出力設定完了
	MX6E_COMMAND_MAX
} mx6e_command_code_t;

#   define	SECTION_DOMAIN_FP					"fp"
#   define	SECTION_DOMAIN_PR					"pr"

//! M46E-PT Table 受信データ
typedef struct {
	mx6e_config_entry_t             entry;
} mx6e_entry_command_data_t;

//! M46E-PT Table 表示要求データ
typedef struct {
} mx6e_show_table_t;

//! デバッグログ出力設定 受信データ
typedef struct {
	bool                            mode;			///< デバッグログ出力設定
} mx6e_set_debuglog_data_t;

//! PTNetwork側実行コマンド 受信データ
typedef struct {
	char                            opt[CMDOPT_LEN_MAX];	///< コマンドオプション最大長
	int                             num;			///< コマンドオプション数
	int                             fd;				///< 表示データ書き込み先のファイルディスクリプタ
} mx6e_exec_cmd_inet_data_t;


//! 要求データ構造体
typedef union {
	mx6e_entry_command_data_t       mx6e_data;		///< M46E-PT コマンドデータ
	mx6e_show_table_t               mx6e_show;		///< M46E-PT 表示データ
	mx6e_set_debuglog_data_t        dlog;			///< デバッグログ設定コマンドデータ
	mx6e_exec_cmd_inet_data_t       inetcmd;		///< PTNetwork実行コマンドデータ
} mx6e_command_request_data_t;

//! シェル起動応答データ
typedef struct {
	int                             fd;				///< ptyデバイスのファイルディスクリプタ
} mx6e_exec_shell_data_t;

//! Fp側コマンド実行要応答データ
typedef struct {
	int                             fd;				///< ptyデバイスのファイルディスクリプタ
} mx6e_exec_cmd_data_t;


//! 応答データ構造体
typedef struct {
	int                             result;			///< 処理結果
	union {
		mx6e_exec_shell_data_t          exec_shell;	///< シェル起動応答データ
		mx6e_exec_cmd_data_t            exec_cmd;	///< コマンド実行応答データ
	};
} mx6e_command_response_data_t;

//! コマンドデータ構造体
typedef struct {
	mx6e_command_code_t             code;			///< コマンドコード
	mx6e_command_request_data_t     req;			///< 要求データ
	mx6e_command_response_data_t    res;			///< 応答データ
} mx6e_command_t;

#endif												// __MX6EAPP_COMMAND_DATA_H__
