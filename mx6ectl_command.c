/******************************************************************************/
/* ファイル名 : mx6ectl_command.c                                             */
/* 機能概要   : 内部コマンドクラス ソースファイル                             */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <limits.h>

#include "mx6ectl_command.h"
#include "mx6eapp_dynamic_setting.h"
#include "mx6eapp_log.h"
#include "mx6eapp_command_data.h"
#include "mx6eapp_socket.h"
#include "mx6eapp_pt.h"
#include "mx6eapp_util.h"

//! オプション引数構造体定義
struct opt_arg {
	char                           *main;			///< メインコマンド文字列
	char                           *sub;			///< サブコマンド文字列
	int                             code;			///< コマンドコード
};

//! コマンド引数
// *INDENT-OFF*
static const struct opt_arg     opt_args[] = {
	{"add",		"m46e",	MX6E_ADD_M46E_ENTRY},
	{"del",		"m46e",	MX6E_DEL_M46E_ENTRY},
	{"enable",	"m46e",	MX6E_ENABLE_M46E_ENTRY},
	{"disable",	"m46e",	MX6E_DISABLE_M46E_ENTRY},
	{"delall",	"m46e",	MX6E_DELALL_M46E_ENTRY},

	{"add",		"me6e",	MX6E_ADD_ME6E_ENTRY},
	{"del",		"me6e",	MX6E_DEL_ME6E_ENTRY},
	{"enable",	"me6e",	MX6E_ENABLE_ME6E_ENTRY},
	{"disable",	"me6e",	MX6E_DISABLE_ME6E_ENTRY},
	{"del",		"me6e",	MX6E_DEL_ME6E_ENTRY},
	{"delall",	"me6e",	MX6E_DELALL_ME6E_ENTRY},
	{NULL,		NULL,	MX6E_COMMAND_MAX}
};
// *INDENT-ON*

//////////////////////////////////////////////////////////////////////////////
//! @brief デバッグログモード設定オプションの解析関数
//!
//! 引数で指定されたコマンドオプションを解析し、
//! 設定値をデバイス設定構造体に格納する。
//!
//! @param [in]  opt1       コマンドオプション
//! @param [out] command    コマンドデータ構造体
//!
//! @retval true  正常終了
//! @retval false 異常終了
///////////////////////////////////////////////////////////////////////////////
static bool opt_parse_dbglog(char *opt[], mx6e_command_t * command)
{
	// ローカル変数宣言
	bool                            result = true;

	// 引数チェック
	if ((opt == NULL) || (command == NULL)) {
		_D_(printf("opt_parse_dbglog Parameter Check NG.\n");
			)
			return false;
	}
	// コマンド解析
	result = parse_bool(opt[0], &command->req.dlog.mode);
	if (!result) {
		return false;
	}

	return result;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief デバッグログ出力モード設定コマンドオプション設定処理関数
//!
//! デバッグログモード変更のオプションを設定する。
//!
//! @param [in]  opt[]      オプションの配列
//! @param [out] command    コマンド構造体
//!
//! @retval true  正常終了
//! @retval false 異常終了
///////////////////////////////////////////////////////////////////////////////
bool mx6e_command_dbglog_set_option(int num, char *opt[], mx6e_command_t * command)
{

	// 内部変数
	bool                            result = false;

	// 引数チェック
	if ((opt == NULL) || (command == NULL)) {
		_D_(printf("mx6e_command_dbglog_set_option Parameter Check NG.\n");
			)
			return false;
	}
	// オプション解析
	result = opt_parse_dbglog(&opt[0], command);
	if (!result) {
		return result;
	}

	return result;

}


///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR Entry追加コマンドオプション設定処理関数
//!
//! MX6E-PR Entry追加コマンドのオプションを設定する。
//!
//! @param [in]  num        オプションの数
//! @param [in]  opt[]      オプションの配列
//! @param [out] command    コマンド構造体
//!
//! @retval true  正常終了
//! @retval false 異常終了
//
//                                     0       1             2               3                                 4                                 5              6
// mx6ectl -n PLANE_NAME add     m46e [pr|fp] [plane_id_in] [prefix_len_in] [ipv4_network_address/prefix_len] [ipv6_network_address/prefix_len] [plane_id_out] [enable|disable]
// mx6ectl -n PLANE_NAME add     me6e [pr|fp] [plane_id_in] [prefix_len_in] [hwaddr]                          [ipv6_network_address/prefix_len] [plane_id_out] [enable|disable]
///////////////////////////////////////////////////////////////////////////////
bool mx6e_command_add_entry_option(int num, char *opt[], mx6e_command_t * command)
{

	// 内部変数
	bool                            result = true;
	mx6e_entry_command_data_t      *pr_data = NULL;
	struct in6_addr                 in6tmp;
	char                            address[INET6_ADDRSTRLEN] = { 0 };


	// 引数チェック
	if ((opt == NULL) || (command == NULL)) {
		_D_(printf("mx6e_command_add_entry_option Parameter Check NG.\n");
			)
			return false;
	}
	pr_data = &(command->req.mx6e_data);

	// opt[0]:[pr|fp]
	if (!strcasecmp(SECTION_DOMAIN_FP, opt[0])) {
		pr_data->entry.domain = DOMAIN_FP;
	} else if (!strcasecmp(SECTION_DOMAIN_PR, opt[0])) {
		pr_data->entry.domain = DOMAIN_PR;
	} else {
		DEBUG_LOG("Illegal domain %s.\n", opt[0]);
		pr_data->entry.domain = DOMAIN_NONE;
		result = false;
	}

	/// マルチプレーン対応 2016/07/27 add
	// opt[1] [ipv6_network_address/prefix_len] (FP only)
	if( pr_data->entry.domain == DOMAIN_FP ){
		result = parse_ipv6address(opt[1], &pr_data->entry.section_dev_addr, &pr_data->entry.section_dev_prefix_len);
		if (!result) {
			printf("fail to parse parameters 'ipv6_network_address/prefix_len'\n");
			return false;
		}
	}
	/// マルチプレーン対応 2016/07/27 end

	// opt[2]:[plane_id_in]
	snprintf(address, sizeof(address), "::%s", opt[2]);
	if (!parse_ipv6address(address, &in6tmp, NULL)) {
		printf("fail to parse parameters 'plane_id_in'\n");
		result = false;
	}
	snprintf(pr_data->entry.src.plane_id, sizeof(pr_data->entry.src.plane_id), "%s", opt[2]);

	// opt[3] [prefix_len_in]
	result = parse_int(opt[3], &pr_data->entry.src.prefix_len, CONFIG_IPV6_PREFIX_MIN, CONFIG_IPV6_PREFIX_MAX);
	if (!result) {
		printf("fail to parse parameters 'prefix_len_in'\n");
		return false;
	}

	switch (command->code) {
	case MX6E_ADD_M46E_ENTRY:
		// opt[4] [ipv4_network_address/prefix_len]
		result = parse_ipv4address_pr(opt[4], &pr_data->entry.src.in.m46e.v4addr, &pr_data->entry.src.in.m46e.v4cidr);
		if (!result) {
			printf("fail to parse parameters 'ipv4_network_address/prefix_len'\n");
			return false;
		}
		break;

	case MX6E_ADD_ME6E_ENTRY:
		// opt[4] [hwaddr]
		result = parse_macaddress(opt[4], &pr_data->entry.src.in.me6e.hwaddr);
		if (!result) {
			printf("fail to parse parameters 'hwaddr'\n");
			return false;
		}
		break;
	default:
		printf("unexpected command code(%d)\n", command->code);
		return false;
	}

	// opt[5] [ipv6_network_address/prefix_len]
	result = parse_ipv6address(opt[5], &pr_data->entry.des.prefix, &pr_data->entry.des.prefix_len);
	if (!result) {
		printf("fail to parse parameters 'ipv6_network_address/prefix_len'\n");
		return false;
	}
	// opt[6] [plane_id_out]
	snprintf(address, sizeof(address), "::%s", opt[6]);
	if (!parse_ipv6address(address, &in6tmp, NULL)) {
		printf("fail to parse parameters 'plane_id_out'\n");
		result = false;
	}
	snprintf(pr_data->entry.des.plane_id, sizeof(pr_data->entry.des.plane_id), "%s", opt[6]);

	// opt[7]:enable
	result = parse_bool(opt[7], &pr_data->entry.enable);
	if (!result) {
		printf("fail to parse parameters 'mode'\n");
		return false;
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR エントリ特定に必要な項目を取得する
//!
//! del, enable, disable用に項目を埋める
//!
//! @param [in]  num        オプションの数
//! @param [in]  opt[]      オプションの配列
//! @param [out] command    コマンド構造体
//!
//! @retval true  正常終了
//! @retval false 異常終了
//                                    0      1             2               3
// mx6ectl -n PLANE_NAME enable  m46e pr|fp [plane_id_in] [prefix_len_in] [ipv4_network_address/prefix_len]
// mx6ectl -n PLANE_NAME enable  me6e pr|fp [plane_id_in] [prefix_len_in] [hwaddr] 
///////////////////////////////////////////////////////////////////////////////
static bool mx6e_command_key_entry(int num, char *opt[], mx6e_command_t * command)
{
	// 内部変数
	bool                            result = true;
	mx6e_entry_command_data_t      *pr_data = NULL;
	struct in6_addr                 in6tmp;
	char                            address[INET6_ADDRSTRLEN] = { 0 };

	// 引数チェック
	if ((opt == NULL) || (command == NULL)) {
		_D_(printf("mx6e_command_enable_entry_option Parameter Check NG.\n");
			)
			return false;
	}
	// ローカル変数初期化
	pr_data = &(command->req.mx6e_data);

	// opt[0]:[pr|fp]
	if (!strcasecmp(SECTION_DOMAIN_FP, opt[0])) {
		pr_data->entry.domain = DOMAIN_FP;
	} else if (!strcasecmp(SECTION_DOMAIN_PR, opt[0])) {
		pr_data->entry.domain = DOMAIN_PR;
	} else {
		DEBUG_LOG("Illegal domain %s.\n", opt[0]);
		pr_data->entry.domain = DOMAIN_NONE;
		result = false;
	}

	/// マルチプレーン対応 2016/07/27 add
	// opt[1] [ipv6_network_address/prefix_len] (FP only)
	if( pr_data->entry.domain == DOMAIN_FP ){
		result = parse_ipv6address(opt[1], &pr_data->entry.section_dev_addr, &pr_data->entry.section_dev_prefix_len);
		if (!result) {
			printf("fail to parse parameters 'ipv6_network_address/prefix_len'\n");
			return false;
		}
	}
	/// マルチプレーン対応 2016/07/27 end

	// opt[2]:[plane_id_in]
	snprintf(address, sizeof(address), "::%s", opt[2]);
	if (!parse_ipv6address(address, &in6tmp, NULL)) {
		printf("fail to parse parameters 'plane_id_in'\n");
		result = false;
	}
	snprintf(pr_data->entry.src.plane_id, sizeof(pr_data->entry.src.plane_id), "%s", opt[2]);

	// opt[3] [prefix_len_in]
	result = parse_int(opt[3], &pr_data->entry.src.prefix_len, CONFIG_IPV6_PREFIX_MIN, CONFIG_IPV6_PREFIX_MAX);
	if (!result) {
		printf("fail to parse parameters 'prefix_len_in'\n");
		return false;
	}

	switch (command->code) {
	case MX6E_ADD_M46E_ENTRY:
	case MX6E_DEL_M46E_ENTRY:
	case MX6E_ENABLE_M46E_ENTRY:
	case MX6E_DISABLE_M46E_ENTRY:
		// opt[4] [ipv4_network_address/prefix_len]
		result = parse_ipv4address_pr(opt[4], &pr_data->entry.src.in.m46e.v4addr, &pr_data->entry.src.in.m46e.v4cidr);
		if (!result) {
			printf("fail to parse parameters 'ipv4_network_address/prefix_len'\n");
			return false;
		}
		break;

	case MX6E_ADD_ME6E_ENTRY:
	case MX6E_DEL_ME6E_ENTRY:
	case MX6E_ENABLE_ME6E_ENTRY:
	case MX6E_DISABLE_ME6E_ENTRY:
		// opt[4] [hwaddr]
		result = parse_macaddress(opt[4], &pr_data->entry.src.in.me6e.hwaddr);
		if (!result) {
			printf("fail to parse parameters 'hwaddr'\n");
			return false;
		}
		break;
	default:
		printf("unexpected command code(%d)\n", command->code);
		return false;
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR Entry削除コマンドオプション設定処理関数
//!
//! MX6E-PR Entry削除コマンドのオプションを設定する。
//!
//! @param [in]  num        オプションの数
//! @param [in]  opt[]      オプションの配列
//! @param [out] command    コマンド構造体
//!
//! @retval true  正常終了
//! @retval false 異常終了
//
//                                    0              1               2
// mx6ectl -n PLANE_NAME del     m46e [plane_id_in] [prefix_len_in] [ipv4_network_address/prefix_len]
// mx6ectl -n PLANE_NAME del     me6e [plane_id_in] [prefix_len_in] [hwaddr] 
///////////////////////////////////////////////////////////////////////////////
bool mx6e_command_del_entry_option(int num, char *opt[], mx6e_command_t * command)
{

	// 内部変数
	bool                            result = true;

	// 引数チェック
	if ((opt == NULL) || (command == NULL)) {
		_D_(printf("mx6e_command_del_entry_option Parameter Check NG.\n");
			)
			return false;
	}
	// エントリ特定用キー項目を埋める
	if (!mx6e_command_key_entry(num, opt, command)) {
		printf("fail to parse key entry\n");
		return false;
	}

	return result;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR Entry活性化コマンドオプション設定処理関数
//!
//! MX6E-PR Entry活性化コマンドのオプションを設定する。
//!
//! @param [in]  num        オプションの数
//! @param [in]  opt[]      オプションの配列
//! @param [out] command    コマンド構造体
//!
//! @retval true  正常終了
//! @retval false 異常終了
//
//
//                                     0             1               2
// mx6ectl -n PLANE_NAME enable  m46e [plane_id_in] [prefix_len_in] [ipv4_network_address/prefix_len]
// mx6ectl -n PLANE_NAME enable  me6e [plane_id_in] [prefix_len_in] [hwaddr] 
///////////////////////////////////////////////////////////////////////////////
bool mx6e_command_enable_entry_option(int num, char *opt[], mx6e_command_t * command)
{

	// 内部変数
	bool                            result = true;
	mx6e_entry_command_data_t      *pr_data = NULL;

	// 引数チェック
	if ((opt == NULL) || (command == NULL)) {
		_D_(printf("mx6e_command_enable_entry_option Parameter Check NG.\n");
			)
			return false;
	}
	// エントリ特定用キー項目を埋める
	if (!mx6e_command_key_entry(num, opt, command)) {
		printf("fail to parse key entry\n");
		return false;
	}
	// ローカル変数初期化
	pr_data = &(command->req.mx6e_data);

	// コマンドデータにenableをセット
	pr_data->entry.enable = true;

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR Entry非活性化コマンドオプション設定処理関数
//!
//! MX6E-PR Entry非活性化コマンドのオプションを設定する。
//!
//! @param [in]  num        オプションの数
//! @param [in]  opt[]      オプションの配列
//! @param [out] command    コマンド構造体
//!
//! @retval true  正常終了
//! @retval false 異常終了
///////////////////////////////////////////////////////////////////////////////
bool mx6e_command_disable_entry_option(int num, char *opt[], mx6e_command_t * command)
{

	// 内部変数
	bool                            result = true;
	mx6e_entry_command_data_t      *pr_data = NULL;

	// 引数チェック
	if ((opt == NULL) || (command == NULL)) {
		_D_(printf("mx6e_command_disable_entry_option Parameter Check NG.\n");
			)
			return false;
	}
	// エントリ特定用キー項目を埋める
	if (!mx6e_command_key_entry(num, opt, command)) {
		printf("fail to parse key entry\n");
		return false;
	}
	// ローカル変数初期化
	pr_data = &(command->req.mx6e_data);

	// コマンドデータにdisableをセット
	pr_data->entry.enable = false;

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR Commandの読み込み処理関数
//!
//! MX6E-PR ファイル内のCommand行の読み込み処理を行う。
//!
//! @param [in]  filename   Commandファイル名
//! @param [in]  command    コマンド構造体
//! @param [in]  name       Plane Name
//!
//! @retval true  正常終了
//! @retval false 異常終了
///////////////////////////////////////////////////////////////////////////////
bool mx6e_command_load(char *filename, mx6e_command_t * command, char *name)
{
	FILE                           *fp = NULL;
	char                            line[OPT_LINE_MAX] = { 0 };
	uint32_t                        line_cnt = 0;
	bool                            result = true;
	char                           *cmd_opt[DYNAMIC_OPE_ARGS_NUM_MAX] = { "" };
	int                             cmd_num = 0;


	// 引数チェック
	if ((filename == NULL) || (strlen(filename) == 0) || (command == NULL) || (name == NULL)) {
		printf("internal error\n");
		return false;
	}
	// 設定ファイルオープン
	fp = fopen(filename, "r");
	if (fp == NULL) {
		printf("No such file : %s\n", filename);
		return false;
	}
	// 一行ずつ読み込み
	while (fgets(line, sizeof(line), fp) != NULL) {

		// ラインカウンタ
		line_cnt++;

		// 改行文字を終端文字に置き換える
		line[strlen(line) - 1] = '\0';
		if (line[strlen(line) - 1] == '\r') {
			line[strlen(line) - 1] = '\0';
		}
		// コメント行と空行はスキップ
		if ((line[0] == '#') || (strlen(line) == 0)) {
			continue;
		}
		// コマンドオプションの初期化
		cmd_num = 0;
		int                             i;
		for (i = 0; i < DYNAMIC_OPE_ARGS_NUM_MAX; i++) {
			cmd_opt[i] = "";
		}

		/* コマンド行の解析 */
		result = mx6e_command_parse_file(line, &cmd_num, cmd_opt);
		if (!result) {
			printf("internal error\n");
			result = false;
			break;
		} else if (cmd_num == 0) {
			// スペースとタブからなる行のためスキップ
			_D_(printf("スペースとタブからなる行のためスキップ\n"));
			continue;
		}

		/* コマンドのパラメータチェック */
		command->code = MX6E_COMMAND_MAX;
		for (int i = 0; opt_args[i].main != NULL; i++) {
			if (!strcmp(cmd_opt[0], opt_args[i].main) && !strcmp(cmd_opt[1], opt_args[i].sub)) {
				command->code = opt_args[i].code;
				break;
			}
		}

		_D_(printf("command->code:%d : %s\n", command->code, get_command_name(command->code)));

		// 行エラーは中断せず次の行を処理する
		switch (command->code) {

			// 未対応コマンドチェック
		case MX6E_COMMAND_MAX:
			printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
			result = false;
			continue;

			// PT-Entry追加コマンド処理
		case MX6E_ADD_M46E_ENTRY:
			if ((cmd_num != ADD_M46E_OPE_MIN_ARGS - 3) && (cmd_num != ADD_M46E_OPE_MAX_ARGS - 3)) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			// PT-Entry追加コマンドオプションの設定
			result = mx6e_command_add_entry_option(cmd_num - 2, &cmd_opt[2], command);
			if (!result) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			break;

			// PT-Entry削除コマンド処理
		case MX6E_DEL_M46E_ENTRY:
			if (cmd_num != DEL_M46E_OPE_ARGS - 3) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			// PT-Entry削除コマンドオプションの設定
			result = mx6e_command_del_entry_option(cmd_num - 2, &cmd_opt[2], command);
			if (!result) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			break;

			// PT-Entry活性化コマンド処理
		case MX6E_ENABLE_M46E_ENTRY:
			if (cmd_num != ENABLE_M46E_OPE_ARGS - 3) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			// PT-Entry活性化コマンドオプションの設定
			result = mx6e_command_enable_entry_option(cmd_num - 2, &cmd_opt[2], command);
			if (!result) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			break;

			// PT-Entry非活性化コマンド処理
		case MX6E_DISABLE_M46E_ENTRY:
			if (cmd_num != DISABLE_M46E_OPE_ARGS - 3) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			// PT-Entry非活性化コマンドオプションの設定
			result = mx6e_command_disable_entry_option(cmd_num - 2, &cmd_opt[2], command);
			if (!result) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			break;

			// PT-Entry追加コマンド処理
		case MX6E_ADD_ME6E_ENTRY:
			if ((cmd_num != ADD_ME6E_OPE_MIN_ARGS - 3) && (cmd_num != ADD_ME6E_OPE_MAX_ARGS - 3)) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			// PT-Entry追加コマンドオプションの設定
			result = mx6e_command_add_entry_option(cmd_num - 2, &cmd_opt[2], command);
			if (!result) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			break;

			// PT-Entry削除コマンド処理
		case MX6E_DEL_ME6E_ENTRY:
			if (cmd_num != DEL_ME6E_OPE_ARGS - 3) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			// PT-Entry削除コマンドオプションの設定
			result = mx6e_command_del_entry_option(cmd_num - 2, &cmd_opt[2], command);
			if (!result) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			break;

			// PT-Entry活性化コマンド処理
		case MX6E_ENABLE_ME6E_ENTRY:
			if (cmd_num != ENABLE_ME6E_OPE_ARGS - 3) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			// PT-Entry活性化コマンドオプションの設定
			result = mx6e_command_enable_entry_option(cmd_num - 2, &cmd_opt[2], command);
			if (!result) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			break;

			// PT-Entry非活性化コマンド処理
		case MX6E_DISABLE_ME6E_ENTRY:
			if (cmd_num != DISABLE_ME6E_OPE_ARGS - 3) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			// PT-Entry非活性化コマンドオプションの設定
			result = mx6e_command_disable_entry_option(cmd_num - 2, &cmd_opt[2], command);
			if (!result) {
				printf("Line%d : %s %s is invalid command\n", line_cnt, cmd_opt[0], cmd_opt[1]);
				result = false;
				continue;
			}
			break;

		default:
			// その他コマンドは無視する
			continue;
		}

		/* コマンド送信 */
		result = mx6e_command_send(command, name);
		if (!result) {
			result = false;
			// 送信失敗は、次の行を読まずに処理を中断する
			break;
		}
	}

	fclose(fp);

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR Command行の解析関数
//!
//! 引数で指定された行を、区切り文字「スペース または タブ」で
//! トークンに分解し、コマンドオプションとオプション数を格納する。
//!
//! @param [in]  line       コマンド行
//! @param [out] num        コマンドオプション数
//! @param [out] cmd_opt    コマンドオプション
//!
//! @retval true  正常終了
//! @retval false 異常終了
///////////////////////////////////////////////////////////////////////////////
bool mx6e_command_parse_file(char *line, int *num, char *cmd_opt[])
{
	char                           *tok = NULL;
	int                             cnt = 0;

	// 引数チェック
	if ((line == NULL) || (num == NULL) || (cmd_opt == NULL)) {
		return false;
	}

	tok = strtok(line, DELIMITER);
	while (tok != NULL) {
		cmd_opt[cnt] = tok;
		cnt++;
		tok = strtok(NULL, DELIMITER);				/* 2回目以降 */
	}

	*num = cnt;

	_D_({
			printf("cnt = %d\n", cnt);
			for (int i = 0; i < cnt; i++) {
				printf("%s\n", cmd_opt[i]);
			}
		});

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR Commandファイル読み込み用コマンド送信処理関数
//!
//! MX6E-PR Commandファイルから読込んだコマンド行（1行単位）を送信する。
//!
//! @param [in]  command    コマンド構造体
//! @param [in]  name       Plane Name
//!
//! @retval true  正常終了
//! @retval false 異常終了
///////////////////////////////////////////////////////////////////////////////
bool mx6e_command_send(mx6e_command_t * command, char *name)
{
	int                             fd = -1;
	char                            path[sizeof(((struct sockaddr_un *) 0)->sun_path)] = { 0 };
	char                           *offset = &path[1];

	// 引数チェック
	if ((command == NULL) || (name == NULL)) {
		return false;
	}

	sprintf(offset, MX6E_COMMAND_SOCK_NAME, name);

	fd = socket(PF_UNIX, SOCK_SEQPACKET, 0);
	if (fd < 0) {
		printf("fail to open socket : %s\n", strerror(errno));
		return false;
	}

	struct sockaddr_un              addr;
	memset(&addr, 0, sizeof(addr));

	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, path, sizeof(addr.sun_path));

	if (connect(fd, (struct sockaddr *) &addr, sizeof(addr))) {
		printf("fail to connect MX6E application(%s) : %s\n", name, strerror(errno));
		close(fd);
		return false;
	}

	int                             ret;
	int                             pty;
	char                            buf[256];

	ret = mx6e_socket_send_cred(fd, command->code, &command->req, sizeof(command->req));
	if (ret <= 0) {
		printf("fail to send command : %s\n", strerror(-ret));
		close(fd);
		return false;
	}

	ret = mx6e_socket_recv(fd, &command->code, &command->res, sizeof(command->res), &pty);
	if (ret <= 0) {
		printf("fail to receive response : %s\n", strerror(-ret));
		close(fd);
		return false;
	}
	if (command->res.result != 0) {
		printf("receive error response : %s\n", strerror(command->res.result));
		close(fd);
		return false;
	}

	switch (command->code) {
	case MX6E_ADD_M46E_ENTRY:						///< M46E ENTRY 追加
	case MX6E_DEL_M46E_ENTRY:						///< M46E ENTRY 削除
	case MX6E_DELALL_M46E_ENTRY:					///< M46E ENTRY 全削除
	case MX6E_ENABLE_M46E_ENTRY:					///< M46E ENTRY 活性化
	case MX6E_DISABLE_M46E_ENTRY:					///< M46E ENTRY 非活性化
	case MX6E_ADD_ME6E_ENTRY:						///< ME6E ENTRY 追加
	case MX6E_DEL_ME6E_ENTRY:						///< ME6E ENTRY 削除
	case MX6E_DELALL_ME6E_ENTRY:					///< ME6E ENTRY 全削除
	case MX6E_ENABLE_ME6E_ENTRY:					///< ME6E ENTRY 活性化
	case MX6E_DISABLE_ME6E_ENTRY:					///< ME6E ENTRY 非活性化
		// 出力結果がソケット経由で送信されてくるので、そのまま標準出力に書き込む
		while (1) {
			ret = read(fd, buf, sizeof(buf));
			if (ret > 0) {
				ret = write(STDOUT_FILENO, buf, ret);
			} else {
				break;
			}
		}
		close(fd);
		break;

	default:
		// ありえない
		close(fd);
		break;
	}

	return true;
}
