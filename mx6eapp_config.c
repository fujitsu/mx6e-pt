/******************************************************************************/
/* ファイル名 : mx6eapp_config.c                                              */
/* 機能概要   : 設定情報管理 ソースファイル                                   */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <regex.h>
#include <netinet/ether.h>
#include <netinet/ip6.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_tun.h>
#include <linux/if_link.h>
#include <search.h>

#include "mx6eapp_config.h"
#include "mx6eapp_log.h"
#include "mx6eapp_pt.h"
#include "mx6eapp_util.h"
#include "mx6eapp_network.h"

//! 設定ファイルを読込む場合の１行あたりの最大文字数
#define CONFIG_LINE_MAX 256

// 設定ファイルのセクション名とキー名
#define SECTION_GENERAL					"general"		///< general セクション名
#define SECTION_GENERAL_PROCESS_NAME	"process_name"
#define SECTION_GENERAL_DEBUG_LOG		"debug_log"
#define SECTION_GENERAL_DAEMON			"daemon"
#define SECTION_GENERAL_STARTUP_SCRIPT	"startup_script"

#define SECTION_DEVICE					"device"		///< device セクション名
#define SECTION_DEVICE_NAME_PR			"name_pr"
#define SECTION_DEVICE_NAME_FP			"name_fp"
#define SECTION_DEVICE_TUNNEL_PR		"tunnel_pr"
#define SECTION_DEVICE_TUNNEL_FP		"tunnel_fp"
#define SECTION_DEVICE_IPV6_ADDRESS_PR	"ipv6_address_pr"
#define SECTION_DEVICE_IPV6_ADDRESS_FP	"ipv6_address_fp"
#define SECTION_DEVICE_HWADDR			"hwaddr"

#define SECTION_DOMAIN					"domain"		///< ドメイン名
#define SECTION_PLANE_ID_IN				"plane_id_in"	///< 受信PlaneID
#define SECTION_PREFIX_LEN_IN			"prefix_len_in"	///< 受信プレフィクス長
#define SECTION_IPV4_ADDRESS_IN			"ipv4_address"	///< 受信IPv4アドレス (セクション:m46eのみ有効)
#define SECTION_HWADDR_IN				"hwaddr"		///< 受信MACアドレス  (セクション:me6eのみ有効)
#define SECTION_IPV6_ADDRESS_OUT		"ipv6_address"	///< 送信IPv6アドレス
#define SECTION_PLANE_ID_OUT			"plane_id_out"	///< 送信PlaneID

///////////////////////////////////////////////////////////////////////////////
//! KEY/VALUE格納用構造体
///////////////////////////////////////////////////////////////////////////////
typedef struct {
	char                            key[CONFIG_LINE_MAX];	///< KEYの値
	char                            value[CONFIG_LINE_MAX];	///< VALUEの値
} config_keyvalue_t;

///////////////////////////////////////////////////////////////////////////////
//! セクション種別
///////////////////////////////////////////////////////////////////////////////
typedef enum {
	CONFIG_SECTION_UNKNOWN = -1,					///< 不明なセクション
	CONFIG_SECTION_NONE = 0,						///< セクション以外の行
	CONFIG_SECTION_GENERAL,							///< [general]セクション
	CONFIG_SECTION_DEVICE,							///< [device]セクション
} config_section;


//! セクション行判定用正規表現 ([で始まって、]で終わる行)
#define SECTION_LINE_REGEX "^\\[(.*)\\]$"
//! KEY VALUE行判定用正規表現 (# 以外で始まって、KEY = VALUE 形式になっている行)
#define KV_REGEX "[ \t]*([^#][^ \t]*)[ \t]*=[ \t]*([^ \t].*)"


///////////////////////////////////////////////////////////////////////////////
// 内部関数プロトタイプ宣言
///////////////////////////////////////////////////////////////////////////////
void                            config_init(mx6e_config_t * config);
static bool                     config_validate_all(mx6e_config_t * config);

static bool                     config_init_general(mx6e_config_t * config);
static void                     config_destruct_general(mx6e_config_general_t * general);
static bool                     config_parse_general(const config_keyvalue_t * kv, mx6e_config_t * config);
static bool                     config_validate_general(mx6e_config_t * config);

static bool                     config_init_device(mx6e_config_t * config);
static bool                     config_parse_device(const config_keyvalue_t * kv, mx6e_config_t * config);
static bool                     config_validate_device(mx6e_config_t * config);

static bool                     config_is_section(const char *str, config_section * section);
static bool                     config_is_keyvalue(const char *line_str, config_keyvalue_t * kv);

///////////////////////////////////////////////////////////////////////////////
//! 設定ファイル解析用テーブル
///////////////////////////////////////////////////////////////////////////////
// *INDENT-OFF*
static struct {
	char	*name;
	bool	(*init_func)		(mx6e_config_t * config);
	bool	(*parse_func)		(const config_keyvalue_t * kv, mx6e_config_t * config);
	bool	(*validate_func)	(mx6e_config_t * config);
} config_table[] = {
	{"none",			NULL,					NULL,					NULL},
	{SECTION_GENERAL,	config_init_general,	config_parse_general,	config_validate_general},
	{SECTION_DEVICE,	config_init_device,		config_parse_device,	config_validate_device},
};
// *INDENT-ON*

///////////////////////////////////////////////////////////////////////////////
//! @brief 設定ファイル読込み関数
//!
//! 引数で指定されたファイルの内容を読み込み、構造体に格納する。
//! <b>戻り値の構造体は本関数内でallocするので、呼出元で解放すること。</b>
//! <b>解放にはfreeではなく、必ずmx6e_config_destruct()関数を使用すること。</b>
//!
//! @param [in] filename 設定ファイル名
//!
//! @return 設定を格納した構造体のポインタ
///////////////////////////////////////////////////////////////////////////////
mx6e_config_t                  *mx6e_config_load(mx6e_config_t *config, const char *filename)
{
	_D_(printf("%s:enter\n", __func__));

	// ローカル変数宣言
	FILE                           *fp;
	char                            line[CONFIG_LINE_MAX];
	config_section                  current_section;
	config_section                  next_section;
	uint32_t                        line_cnt;
	bool                            result;
	config_keyvalue_t               kv;

	// 引数チェック
	if (filename == NULL || strlen(filename) == 0) {
		return NULL;
	}
	// ローカル変数初期化
	current_section = CONFIG_SECTION_NONE;
	next_section = CONFIG_SECTION_NONE;
	line_cnt = 0;
	result = true;

	// 設定ファイルオープン
	if (NULL == (fp = fopen(filename, "r"))) {;
		return NULL;
	}

	// 設定保持用構造体初期化
	config_init(config);

	while (fgets(line, sizeof(line), fp) != NULL) {
		// ラインカウンタをインクリメント
		line_cnt++;

		// 改行文字を終端文字に置き換える
		line[strlen(line) - 1] = '\0';

		// コメント行と空行はスキップ
		if ((line[0] == '#') || (strlen(line) == 0)) {
			continue;
		}

		if (config_is_section(line, &next_section)) {
			// セクション行の場合
			// 現在のセクションの整合性チェック
			if (config_table[current_section].validate_func != NULL) {
				result = config_table[current_section].validate_func(config);
				if (!result) {
					mx6e_logging(LOG_ERR, "Section Validation Error [%s]\n", config_table[current_section].name);
					break;
				}
			}
			// 次セクションが不明の場合は処理中断
			if (next_section == CONFIG_SECTION_UNKNOWN) {
				// 処理結果を異常に設定
				result = false;
				break;
			}
			// 次セクションの初期化関数をコール(セクション重複チェックなどはここでおこなう)
			if (config_table[next_section].init_func != NULL) {
				result = config_table[next_section].init_func(config);
				if (!result) {
					mx6e_logging(LOG_ERR, "Section Initialize Error [%s]\n", config_table[next_section].name);
					break;
				}
			}

			current_section = next_section;
		} else if (config_is_keyvalue(line, &kv)) {
			// キーバリュー行の場合、セクションごとの解析関数をコール
			if (config_table[current_section].parse_func != NULL) {
				result = config_table[current_section].parse_func(&kv, config);
				if (!result) {
					mx6e_logging(LOG_ERR, "Parse Error [%s] line %d : %s\n", config_table[current_section].name, line_cnt, line);
					break;
				}
			}
		} else {
			// なにもしない
		}
	}
	fclose(fp);

	// 最後のセクションの整合性チェック
	if (config_table[current_section].validate_func != NULL) {
		result = config_table[current_section].validate_func(config);
		if (!result) {
			mx6e_logging(LOG_ERR, "Section Validation Error [%s]\n", config_table[current_section].name);
		}
	}

	if (result) {
		// 全体の整合性チェック
		result = config_validate_all(config);
		if (!result) {
			mx6e_logging(LOG_ERR, "Config Validation Error\n");
		}
	}

	if (!result) {
		// 処理結果が異常の場合、後始末をしてNULLを返す
		mx6e_config_destruct(config);
		config = NULL;
	} else {
		// 読み込んだ設定ファイルのフルパスを格納
		snprintf(config->filename, sizeof(config->filename), "%s", realpath(filename, NULL));
	}

	return config;
}

static mx6e_config_devices_t           *mydevices;		///< デバイス設定
static void tdaction(void *nodep)
{
	mx6e_config_entry_t *entry = nodep;

	int                             ifindex = get_domain_src_ifindex(entry->domain, mydevices);

	// route削除
	mx6e_network_del_route(AF_INET6, ifindex, &entry->src.tunnel_addr, entry->src.tunnel_prefix_len, NULL);
	free(entry);
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 設定ファイル構造体解放関数
//!
//! 引数で指定された設定ファイル構造体を解放する。
//!
//! @param [in,out] config 解放する設定ファイル構造体へのポインタ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
void mx6e_config_destruct(mx6e_config_t * config)
{
	_D_(printf("%s:enter\n", __func__));

	mx6e_config_table_t            *table = NULL;

	// 引数チェック
	if (config == NULL) {
		return;
	}

	config_destruct_general(&config->general);

	mydevices = &config->devices;

	table = &config->m46e_conf_table;
	tdestroy(table->root, tdaction);
	table->num = 0;

	table = &config->me6e_conf_table;
	tdestroy(table->root, tdaction);
	table->num = 0;

	_D_(printf("%s:exit\n", __func__));
	return;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 設定ファイル構造体ダンプ関数
//!
//! 引数で指定された設定ファイル構造体の内容をダンプする。
//!
//! @param [in] config ダンプする設定ファイル構造体へのポインタ
//! @param [in] fd     出力先のファイルディスクリプタ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
void mx6e_config_dump(const mx6e_config_t * config, int fd)
{
	_D_(printf("%s:enter\n", __func__));

	// ローカル変数宣言
	char                            address[INET6_ADDRSTRLEN];
	char                           *strbool[] = { CONFIG_BOOL_FALSE, CONFIG_BOOL_TRUE };

	// 引数チェック
	if (config == NULL) {
		return;
	}
	// ローカル変数初期化

	dprintf(fd, "\n");

	// 設定ファイル名
	if (config->filename != NULL) {
		dprintf(fd, "Config file name = %s\n", config->filename);
		dprintf(fd, "\n");
	}
	// 共通設定
	dprintf(fd, "[%s]\n", SECTION_GENERAL);
	dprintf(fd, "%s = %s\n", SECTION_GENERAL_PROCESS_NAME, config->general.process_name);

	dprintf(fd, "%s = %s\n", SECTION_GENERAL_DEBUG_LOG, strbool[config->general.debug_log]);
	dprintf(fd, "%s = %s\n", SECTION_GENERAL_DAEMON, strbool[config->general.daemon]);
	dprintf(fd, "%s = %s\n", SECTION_GENERAL_STARTUP_SCRIPT, config->general.startup_script);
	dprintf(fd, "\n");

	// 物理デバイス設定
	mx6e_device_t                   dev;
	dprintf(fd, "[%s]\n", SECTION_DEVICE);
	
	dev = config->devices.fp;

	dprintf(fd, "%s = %s\n", SECTION_DEVICE_NAME_FP, dev.name);
	dprintf(fd, "%s = %s\n", SECTION_DEVICE_HWADDR, ether_ntoa(&dev.hwaddr));
	dprintf(fd, "\n");

	dev = config->devices.pr;

	dprintf(fd, "%s = %s\n", SECTION_DEVICE_NAME_PR, dev.name);
	dprintf(fd, "%s = %s\n", SECTION_DEVICE_HWADDR, ether_ntoa(&dev.hwaddr));
	dprintf(fd, "\n");

	// トンネルデバイス設定
	dev = config->devices.tunnel_fp;

	dprintf(fd, "%s = %s\n", SECTION_DEVICE_TUNNEL_FP, dev.name);
	dprintf(fd, "%s = %s\n", SECTION_DEVICE_HWADDR, ether_ntoa(&dev.hwaddr));
	dprintf(fd, "%s = %s/%d\n", SECTION_DEVICE_IPV6_ADDRESS_FP, inet_ntop(AF_INET6, &dev.ipv6_address, address, sizeof(address)), dev.ipv6_netmask);
	dprintf(fd, "\n");

	dev = config->devices.tunnel_pr;

	dprintf(fd, "%s = %s\n", SECTION_DEVICE_TUNNEL_PR, dev.name);
	dprintf(fd, "%s = %s\n", SECTION_DEVICE_HWADDR, ether_ntoa(&dev.hwaddr));
	dprintf(fd, "%s = %s/%d\n", SECTION_DEVICE_IPV6_ADDRESS_PR, inet_ntop(AF_INET6, &dev.ipv6_address, address, sizeof(address)), dev.ipv6_netmask);
	dprintf(fd, "\n");


	dprintf(fd, "\n");

	return;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 設定情報格納用構造体初期化関数
//!
//! 設定情報格納用構造体を初期化する。
//!
//! @param [in,out] config   設定情報格納用構造体へのポインタ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
void config_init(mx6e_config_t * config)
{
	_D_(printf("%s:enter\n", __func__));

	// 引数チェック
	if (config == NULL) {
		return;
	}
	// 設定ファイル名
	config->filename[0] = '\0';
	// 共通設定
	memset(&config->general, 0, sizeof(config->general));
	// デバイス設定
	memset(&config->devices, 0, sizeof(config->devices));

	// 排他制御初期化
	pthread_mutexattr_t             attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);

	// M46E-PT Config Table
	memset(&config->m46e_conf_table, 0, sizeof(config->m46e_conf_table));
	config->m46e_conf_table.type = CONFIG_TYPE_M46E;
	config->m46e_conf_table.root = NULL;
	pthread_mutex_init(&config->m46e_conf_table.mutex, &attr);

	_D_(m46e_pt_config_table_dump(&config->m46e_conf_table));

	// Me6E-PT Config Table
	memset(&config->me6e_conf_table, 0, sizeof(config->me6e_conf_table));
	config->me6e_conf_table.type = CONFIG_TYPE_ME6E;
	config->me6e_conf_table.root = NULL;
	pthread_mutex_init(&config->me6e_conf_table.mutex, &attr);

	_D_(m46e_pt_config_table_dump(&config->me6e_conf_table));

	return;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 設定情報整合性チェック関数
//!
//! 設定情報全体での整合性をチェックする。
//!
//! @param [in] config  設定情報格納用構造体へのポインタ
//!
//! @retval true   整合性OK
//! @retval false  整合性NG
///////////////////////////////////////////////////////////////////////////////
static bool config_validate_all(mx6e_config_t * config)
{
	_D_(printf("%s:enter\n", __func__));

	// 引数チェック
	if (config == NULL) {
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 共通設定格納用構造体初期化関数
//!
//! 共通設定格納用構造体を初期化する。
//!
//! @param [in,out] config   設定情報格納用構造体へのポインタ
//!
//! @retval true  正常終了
//! @retval false 異常終了
///////////////////////////////////////////////////////////////////////////////
bool config_init_general(mx6e_config_t * config)
{
	_D_(printf("%s:enter\n", __func__));

	// 引数チェック
	if (config == NULL) {
		return false;;
	}
	// 設定初期化
	config->general.process_name[0] = '\0';
	config->general.debug_log = false;
	config->general.daemon = true;
	config->general.startup_script[0] = '\0';

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 共通設定格納用構造体解放関数
//!
//! 共通設定格納用構造体を解放する。
//!
//! @param [in,out] general   共通設定格納用構造体へのポインタ
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
static void config_destruct_general(mx6e_config_general_t * general)
{
	_D_(printf("%s:enter\n", __func__));

	// 引数チェック
	if (general == NULL) {
		return;
	}
	memset(general, 0, sizeof(*general));

	return;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 共通設定の解析関数
//!
//! 引数で指定されたKEY/VALUEを解析し、
//! 設定値を共通設定構造体に格納する。
//!
//! @param [in]  kv      設定ファイルから読込んだ一行情報
//! @param [out] config  設定情報格納先の構造体
//!
//! @retval true  正常終了
//! @retval false 異常終了(不正な値がKEY/VALUEに設定されていた場合)
///////////////////////////////////////////////////////////////////////////////
static bool config_parse_general(const config_keyvalue_t * kv, mx6e_config_t * config)
{
	_D_(printf("%s:enter\n", __func__));

	// ローカル変数宣言
	bool                            result;

	// 引数チェック
	if ((kv == NULL) || (config == NULL)) {
		return false;
	}
	// ローカル変数初期化
	result = true;

	if (!strcasecmp(SECTION_GENERAL_PROCESS_NAME, kv->key)) {
		DEBUG_LOG("Match %s.\n", SECTION_GENERAL_PROCESS_NAME);
		snprintf(config->general.process_name, sizeof(config->general.process_name), "%s", kv->value);
	} else if (!strcasecmp(SECTION_GENERAL_DEBUG_LOG, kv->key)) {
		DEBUG_LOG("Match %s.\n", SECTION_GENERAL_DEBUG_LOG);
		result = parse_bool(kv->value, &config->general.debug_log);
	} else if (!strcasecmp(SECTION_GENERAL_DAEMON, kv->key)) {
		DEBUG_LOG("Match %s.\n", SECTION_GENERAL_DAEMON);
		result = parse_bool(kv->value, &config->general.daemon);
	} else if (!strcasecmp(SECTION_GENERAL_STARTUP_SCRIPT, kv->key)) {
		DEBUG_LOG("Match %s.\n", SECTION_GENERAL_STARTUP_SCRIPT);
		snprintf(config->general.startup_script, sizeof(config->general.startup_script), "%s", kv->value);
	} else {
		// 不明なキーなのでスキップ
		mx6e_logging(LOG_WARNING, "Ignore unknown key : %s\n", kv->key);
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 共通設定の整合性チェック関数
//!
//! 共通設定内部での整合性をチェックする
//!
//! @param [in] config   設定情報格納用構造体へのポインタ
//!
//! @retval true  整合性OK
//! @retval false 整合性NG
///////////////////////////////////////////////////////////////////////////////
static bool config_validate_general(mx6e_config_t * config)
{
	_D_(printf("%s:enter\n", __func__));

	// 引数チェック
	if (config == NULL) {
		return false;;
	}
	// 必須情報が設定されているかをチェック
	if (0 >= strlen(config->general.process_name)) {
		mx6e_logging(LOG_ERR, "plane name is not found");
		return false;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief デバイス設定格納用構造体初期化関数
//!
//! デバイス設定格納用構造体を初期化する。
//!
//! @param [in,out] config   設定情報格納用構造体へのポインタ
//!
//! @retval true  正常終了
//! @retval false 異常終了
///////////////////////////////////////////////////////////////////////////////
static bool config_init_device(mx6e_config_t * config)
{
	_D_(printf("%s:enter\n", __func__));

	// 引数チェック
	if (config == NULL) {
		return false;;
	}

	memset(&config->devices, 0, sizeof(config->devices));

	config->devices.pr.ifindex = -1;
	config->devices.pr.fd = -1;
	config->devices.fp.ifindex = -1;
	config->devices.fp.fd = -1;

	config->devices.tunnel_pr.ifindex = -1;
	config->devices.tunnel_pr.fd = -1;
	config->devices.tunnel_fp.ifindex = -1;
	config->devices.tunnel_fp.fd = -1;

	return true;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief デバイス設定の解析関数
//!
//! 引数で指定されたKEY/VALUEを解析し、
//! 設定値をデバイス設定構造体に格納する。
//!
//! @param [in]  kv      設定ファイルから読込んだ一行情報
//! @param [out] config  設定情報格納先の構造体
//!!
//! @retval true  正常終了
//! @retval false 異常終了(不正な値がKEY/VALUEに設定されていた場合)
///////////////////////////////////////////////////////////////////////////////
static bool config_parse_device(const config_keyvalue_t * kv, mx6e_config_t * config)
{
	_D_(printf("%s:enter\n", __func__));

	// ローカル変数宣言
	bool                            result;

	// 引数チェック
	if ((kv == NULL) || (config == NULL)) {
		return false;
	}
	// ローカル変数初期化
	result = true;

	// PR
	if (!strcasecmp(SECTION_DEVICE_NAME_PR, kv->key)) {
		DEBUG_LOG("Match %s.\n", SECTION_DEVICE_NAME_PR);
		snprintf(config->devices.pr.name, sizeof(config->devices.pr.name), "%s", kv->value);

		// デバイスのインデックス番号取得
		config->devices.pr.ifindex = if_nametoindex(config->devices.pr.name);
		if (config->devices.pr.ifindex == 0) {
			config->devices.pr.ifindex = -1;
		}
		// デバイスのMACアドレス
		mx6e_network_get_hwaddr_by_index(config->devices.pr.ifindex, &config->devices.pr.hwaddr);

		_D_(printf("device.pr.ifindex:%s:%d\n", config->devices.pr.name, config->devices.pr.ifindex));

	} else if (!strcasecmp(SECTION_DEVICE_TUNNEL_PR, kv->key)) {
		DEBUG_LOG("Match %s.\n", SECTION_DEVICE_TUNNEL_PR);
		snprintf(config->devices.tunnel_pr.name, sizeof(config->devices.tunnel_pr.name), "%s", kv->value);
		// 生成前なのでデバイスのインデックス番号は取得できない
	} else if (!strcasecmp(SECTION_DEVICE_IPV6_ADDRESS_PR, kv->key)) {
		// tunnelデバイス情報にIPv6アドレス登録
		DEBUG_LOG("Match %s.\n", SECTION_DEVICE_IPV6_ADDRESS_PR);
		result = parse_ipv6address(kv->value, &config->devices.tunnel_pr.ipv6_address, &config->devices.tunnel_pr.ipv6_netmask);
		if (result && (config->devices.tunnel_pr.ipv6_netmask == 0)) {
			// プレフィックスが指定されていない場合はエラーにする
			result = false;
		}
		return ENODEV;

		// FP
	} else if (!strcasecmp(SECTION_DEVICE_NAME_FP, kv->key)) {
		DEBUG_LOG("Match %s.\n", SECTION_DEVICE_NAME_FP);
		snprintf(config->devices.fp.name, sizeof(config->devices.fp.name), "%s", kv->value);

		// デバイスのインデックス番号取得
		config->devices.fp.ifindex = if_nametoindex(config->devices.fp.name);
		if (config->devices.fp.ifindex == 0) {
			config->devices.fp.ifindex = -1;
		}
		// デバイスのMACアドレス
		mx6e_network_get_hwaddr_by_index(config->devices.fp.ifindex, &config->devices.fp.hwaddr);
		
		_D_(printf("device.fp.ifindex:%s:%d\n", config->devices.fp.name, config->devices.fp.ifindex));

	} else if (!strcasecmp(SECTION_DEVICE_TUNNEL_FP, kv->key)) {
		DEBUG_LOG("Match %s.\n", SECTION_DEVICE_TUNNEL_FP);
		snprintf(config->devices.tunnel_fp.name, sizeof(config->devices.tunnel_fp.name), "%s", kv->value);
		// 生成前なのでデバイスのインデックス番号は取得できない
	} else if (!strcasecmp(SECTION_DEVICE_IPV6_ADDRESS_FP, kv->key)) {
		// tunnelデバイス情報にIPv6アドレス登録
		DEBUG_LOG("Match %s.\n", SECTION_DEVICE_IPV6_ADDRESS_FP);
		result = parse_ipv6address(kv->value, &config->devices.tunnel_fp.ipv6_address, &config->devices.tunnel_fp.ipv6_netmask);
		if (result && (config->devices.tunnel_fp.ipv6_netmask == 0)) {
			// プレフィックスが指定されていない場合はエラーにする
			result = false;
		}
	} else {
		// 不明なキーなのでスキップ
		mx6e_logging(LOG_WARNING, "Ignore unknown key : %s\n", kv->key);
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief デバイス設定の整合性チェック関数
//!
//! デバイス設定内部での整合性をチェックする
//!
//! @param [in] config   設定情報格納用構造体へのポインタ
//!
//! @retval true  整合性OK
//! @retval false 整合性NG
///////////////////////////////////////////////////////////////////////////////
static bool config_validate_device(mx6e_config_t * config)
{
	_D_(printf("%s:enter\n", __func__));

	// 引数チェック
	if (config == NULL) {
		return false;;
	}

	mx6e_config_devices_t          *devices = &config->devices;
	if (devices == NULL) {
		return false;
	}
	// "name_pr" 必須情報が設定されているかをチェック
	if (0 == strlen(devices->pr.name)) {
		mx6e_logging(LOG_ERR, "PR devic ename is NULL");
		return false;
	}
	// "name_pr" デバイスが存在するかチェック
	if (if_nametoindex(devices->pr.name) == 0) {
		mx6e_logging(LOG_ERR, "No such PR device : %s", devices->pr.name);
		return false;
	}
	// "name_fp" 必須情報が設定されているかをチェック
	if (0 == strlen(devices->fp.name)) {
		mx6e_logging(LOG_ERR, "FP device name is NULL");
		return false;
	}
	// "name_fp" デバイスが存在するかチェック
	if (if_nametoindex(devices->fp.name) == 0) {
		mx6e_logging(LOG_ERR, "No such FP device : %s", devices->fp.name);
		return false;
	}

	// "tunnel_pr" 必須情報が設定されているかをチェック
	if (0 == strlen(devices->tunnel_pr.name)) {
		mx6e_logging(LOG_ERR, "TUNNEL PR device name is NULL");
		return false;
	}
	// "tunnel_fp" 必須情報が設定されているかをチェック
	if (0 == strlen(devices->tunnel_fp.name)) {
		mx6e_logging(LOG_ERR, "TUNNEL FP device name is NULL");
		return false;
	}

	// "ipv6_address_pr" 必須情報が設定されているかをチェック
	if (compare_ipv6(&config->devices.tunnel_pr.ipv6_address, &in6_addr0)) {
		mx6e_logging(LOG_ERR, "TUNNEL PR device address name is NULL");
		return false;
	}

	/// マルチプレーン対応 2016/07/27 del start
#if 0
	// "ipv6_address_fp" 必須情報が設定されているかをチェック
	if (compare_ipv6(&config->devices.tunnel_fp.ipv6_address, &in6_addr0)) {
		mx6e_logging(LOG_ERR, "TUNNEL FP device address name is NULL");
		return false;
	}
#endif
	/// マルチプレーン対応 2016/07/27 del end

	return true;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief セクション行判定関数
//!
//! 引数で指定された文字列がセクション行に該当するかどうかをチェックし、
//! セクション行の場合は、出力パラメータにセクション種別を格納する。
//!
//! @param [in]  line_str  設定ファイルから読込んだ一行文字列
//! @param [out] section   セクション種別格納先ポインタ
//!
//! @retval true  引数の文字列がセクション行である
//! @retval false 引数の文字列がセクション行でない
///////////////////////////////////////////////////////////////////////////////
static bool config_is_section(const char *line_str, config_section * section)
{
	_D_(printf("%s:enter\n", __func__));

	// ローカル変数宣言
	regex_t                         preg;
	size_t                          nmatch = 2;
	regmatch_t                      pmatch[nmatch];
	bool                            result;

	// 引数チェック
	if (line_str == NULL) {
		return false;
	}

	if (regcomp(&preg, SECTION_LINE_REGEX, REG_EXTENDED | REG_NEWLINE) != 0) {
		DEBUG_LOG("regex compile failed.\n");
		return false;
	}

	DEBUG_LOG("String = %s\n", line_str);

	if (regexec(&preg, line_str, nmatch, pmatch, 0) == REG_NOMATCH) {
		result = false;
	} else {
		result = true;
		// セクションのOUTパラメータが指定されている場合はセクション名チェック
		if ((section != NULL) && (pmatch[1].rm_so >= 0)) {
			if (!strncmp(SECTION_GENERAL, &line_str[pmatch[1].rm_so], (pmatch[1].rm_eo - pmatch[1].rm_so))) {
				DEBUG_LOG("Match %s.\n", SECTION_GENERAL);
				*section = CONFIG_SECTION_GENERAL;
			} else if (!strncmp(SECTION_DEVICE, &line_str[pmatch[1].rm_so], (pmatch[1].rm_eo - pmatch[1].rm_so))) {
				DEBUG_LOG("Match %s.\n", SECTION_DEVICE);
				*section = CONFIG_SECTION_DEVICE;
			} else {
				mx6e_logging(LOG_ERR, "unknown section(%.*s)\n", (pmatch[1].rm_eo - pmatch[1].rm_so), &line_str[pmatch[1].rm_so]);
				*section = CONFIG_SECTION_UNKNOWN;
			}
		}
	}

	regfree(&preg);

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief KEY/VALUE行判定関数
//!
//! 引数で指定された文字列がKEY/VALUE行に該当するかどうかをチェックし、
//! KEY/VALUE行の場合は、出力パラメータにKEY/VALUE値を格納する。
//!
//! @param [in]  line_str  設定ファイルから読込んだ一行文字列
//! @param [out] kv        KEY/VALUE値格納先ポインタ
//!
//! @retval true  引数の文字列がKEY/VALUE行である
//! @retval false 引数の文字列がKEY/VALUE行でない
///////////////////////////////////////////////////////////////////////////////
static bool config_is_keyvalue(const char *line_str, config_keyvalue_t * kv)
{
	_D_(printf("%s:enter\n", __func__));

	// ローカル変数宣言
	regex_t                         preg;
	size_t                          nmatch = 3;
	regmatch_t                      pmatch[nmatch];
	bool                            result;

	// 引数チェック
	if ((line_str == NULL) || (kv == NULL)) {
		return false;
	}
	// ローカル変数初期化
	result = true;

	if (regcomp(&preg, KV_REGEX, REG_EXTENDED | REG_NEWLINE) != 0) {
		DEBUG_LOG("regex compile failed.\n");
		return false;
	}

	if (regexec(&preg, line_str, nmatch, pmatch, 0) == REG_NOMATCH) {
		DEBUG_LOG("No match.\n");
		result = false;
	} else {
		sprintf(kv->key, "%.*s", (pmatch[1].rm_eo - pmatch[1].rm_so), &line_str[pmatch[1].rm_so]);
		sprintf(kv->value, "%.*s", (pmatch[2].rm_eo - pmatch[2].rm_so), &line_str[pmatch[2].rm_so]);
		DEBUG_LOG("Match. key=\"%s\", value=\"%s\"\n", kv->key, kv->value);
	}

	regfree(&preg);

	return result;
}
