/******************************************************************************/
/* ファイル名 : mx6eapp_util.c                                                */
/* 機能概要   : 共通関数 ソースファイル                                       */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "mx6eapp_util.h"
#include "mx6eapp_log.h"
#include "mx6eapp_pt.h"

//! IPv4 0アドレス
struct in_addr                  in_addr0 = { 0 };

//! IPv6 0アドレス
struct in6_addr                 in6_addr0 = IN6ADDR_ANY_INIT;	// all zero
//! MAC 0アドレス
struct ether_addr               mac0 = { {0} };

///////////////////////////////////////////////////////////////////////////////
//! @brief デバイスに設定されているIPv4アドレスを取得する関数
//!
//! 引数で指定されたデバイスのIPv4アドレスを取得する。
//!
//! @param [in]  dev  IPv4アドレスを取得するデバイス名
//! @param [out] addr 取得したIPv4アドレスの格納先
//!
//! @retval true  指定したデバイスにIPv4アドレスがある場合
//! @retval false 指定したデバイスにIPv4アドレスがない場合(デバイスが存在しない場合も含む)
///////////////////////////////////////////////////////////////////////////////
bool mx6e_util_get_ipv4_addr(const char *dev, struct in_addr *addr)
{
	// ローカル変数宣言
	struct ifreq                    ifr;
	int                             fd;

	// 引数チェック
	if ((dev == NULL) || (addr == NULL)) {
		return false;
	}
	// ローカル変数初期化
	memset(&ifr, 0, sizeof(ifr));

	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		return false;
	}

	ifr.ifr_addr.sa_family = AF_INET;
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", dev);

	if (ioctl(fd, SIOCGIFADDR, &ifr) != 0) {
		close(fd);
		return false;
	}

	close(fd);

	memcpy(addr, &(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr), sizeof(struct in_addr));

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief デバイスに設定されているIPv4ネットマスクを取得する関数
//!
//! 引数で指定されたデバイスのIPv4ネットマスクを取得する。
//!
//! @param [in]  dev  IPv4ネットマスクを取得するデバイス名
//! @param [out] mask 取得したIPv4ネットマスクの格納先
//!
//! @retval true  処理成功
//! @retval false 処理失敗(デバイスが存在しない場合も含む)
///////////////////////////////////////////////////////////////////////////////
bool mx6e_util_get_ipv4_mask(const char *dev, struct in_addr * mask)
{
	// ローカル変数宣言
	struct ifreq                    ifr;
	int                             fd;

	// 引数チェック
	if ((dev == NULL) || (mask == NULL)) {
		return false;
	}
	// ローカル変数初期化
	memset(&ifr, 0, sizeof(ifr));

	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		return false;
	}

	ifr.ifr_addr.sa_family = AF_INET;
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", dev);

	if (ioctl(fd, SIOCGIFNETMASK, &ifr) != 0) {
		close(fd);
		return false;
	}

	close(fd);

	memcpy(mask, &(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr), sizeof(struct in_addr));

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief MACブロードキャストチェック関数
//!
//! MACアドレスがブロードキャストアドレス(全てFF)かどうかをチェックする。
//!
//! @param [in]  mac_addr  MACアドレス
//!
//! @retval true  MACアドレスがブロードキャストアドレスの場合
//! @retval false MACアドレスがブロードキャストアドレスではない場合
///////////////////////////////////////////////////////////////////////////////
bool mx6e_util_is_broadcast_mac(const unsigned char *mac_addr)
{
	// ローカル変数宣言
	int                             i;
	bool                            result;

	// ローカル変数初期化
	i = 0;
	result = true;

	// 引数チェック
	if (mac_addr == NULL) {
		return false;
	}

	for (i = 0; i < ETH_ALEN; i++) {
		if (mac_addr[i] != 0xff) {
			// アドレスが一つでも0xffでなければ結果をfalseにしてbreak
			result = false;
			break;
		}
	}

	return result;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief チェックサム計算関数
//!
//! 引数で指定されたコードとサイズからチェックサム値を計算して返す。
//!
//! @param [in]  buf  チェックサムを計算するコードの先頭アドレス
//! @param [in]  size チェックサムを計算するコードのサイズ
//!
//! @return 計算したチェックサム値
///////////////////////////////////////////////////////////////////////////////
unsigned short mx6e_util_checksum(unsigned short *buf, int size)
{
	unsigned long                   sum = 0;

	while (size > 1) {
		sum += *buf++;
		size -= 2;
	}
	if (size) {
		sum += *(u_int8_t *) buf;
	}

	sum = (sum & 0xffff) + (sum >> 16);				/* add overflow counts */
	sum = (sum & 0xffff) + (sum >> 16);				/* once again */

	return ~sum;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief チェックサム計算関数(複数ブロック対応)
//!
//! 引数で指定されたコードとサイズからチェックサム値を計算して返す。
//!
//! @param [in]  vec      チェックサムを計算するコード配列の先頭アドレス
//! @param [in]  vec_size チェックサムを計算するコード配列の要素数
//!
//! @return 計算したチェックサム値
///////////////////////////////////////////////////////////////////////////////
unsigned short mx6e_util_checksumv(struct iovec *vec, int vec_size)
{
	int                             i;
	unsigned long                   sum = 0;

	for (i = 0; i < vec_size; i++) {
		unsigned short                 *buf = (unsigned short *) vec[i].iov_base;
		int                             size = vec[i].iov_len;

		while (size > 1) {
			sum += *buf++;
			size -= 2;
		}
		if (size) {
			sum += *(u_int8_t *) buf;
		}
	}

	sum = (sum & 0xffff) + (sum >> 16);				/* add overflow counts */
	sum = (sum & 0xffff) + (sum >> 16);				/* once again */

	return ~sum;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief 文字列をbool値に変換する
//!
//! 引数で指定された文字列がyes or noの場合に、yesならばtrueに、
//! noならばfalseに変換して出力パラメータに格納する。
//!
//! @param [in]  str     変換対象の文字列
//! @param [out] output  変換結果の出力先ポインタ
//!
//! @retval true  変換成功
//! @retval false 変換失敗 (引数の文字列がbool値でない)
///////////////////////////////////////////////////////////////////////////////
bool parse_bool(const char *str, bool * output)
{
	// ローカル変数定義
	bool                            result;

	// 引数チェック
	if ((str == NULL) || (output == NULL)) {
		return false;
	}
	// ローカル変数初期化
	result = true;

	if (!strcasecmp(str, CONFIG_BOOL_TRUE)) {
		*output = true;
	} else if (!strcasecmp(str, CONFIG_BOOL_FALSE)) {
		*output = false;
	} else if (!strcasecmp(str, OPT_BOOL_ON)) {
		*output = true;
	} else if (!strcasecmp(str, OPT_BOOL_OFF)) {
		*output = false;
	} else if (!strcasecmp(str, OPT_BOOL_ENABLE)) {
		*output = true;
	} else if (!strcasecmp(str, OPT_BOOL_DISABLE)) {
		*output = false;
	} else if (!strcasecmp(str, OPT_BOOL_NONE)) {
		*output = false;
	} else {
		result = false;
	}

	return result;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief 文字列を整数値に変換する
//!
//! 引数で指定された文字列が整数値で、且つ最小値と最大値の範囲に
//! 収まっている場合に、数値型に変換して出力パラメータに格納する。
//!
//! @param [in]  str     変換対象の文字列
//! @param [out] output  変換結果の出力先ポインタ
//! @param [in]  min     変換後の数値が許容する最小値
//! @param [in]  max     変換後の数値が許容する最大値
//!
//! @retval true  変換成功
//! @retval false 変換失敗 (引数の文字列が整数値でない)
///////////////////////////////////////////////////////////////////////////////
bool parse_int(const char *str, int *output, const int min, const int max)
{
	// ローカル変数定義
	bool                            result;
	int                             tmp;
	char                           *endptr;

	// 引数チェック
	if ((str == NULL) || (output == NULL)) {
		_D_(printf("parse_int Parameter Check NG.\n");
			)
			return false;
	}
	// ローカル変数初期化
	result = true;
	endptr = NULL;

	tmp = strtol(str, &endptr, 10);

	if ((tmp == LONG_MIN || tmp == LONG_MAX) && (errno != 0)) {
		// strtol内でエラーがあったのでエラー
		result = false;
	} else if (endptr == str) {
		// strtol内でエラーがあったのでエラー
		result = false;
	} else if (tmp > max) {
		// 最大値よりも大きいのでエラー
		_D_(printf("parse_int Parameter too big.\n");
			)
			result = false;
	} else if (tmp < min) {
		// 最小値よりも小さいのでエラー
		_D_(printf("parse_int Parameter too small.\n");
			)
			result = false;
	} else if (*endptr != '\0') {
		// 最終ポインタが終端文字でない(=文字列の途中で変換が終了した)のでエラー
		result = false;
	} else {
		// ここまでくれば正常に変換できたので、出力変数に格納
		*output = tmp;
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 文字列をIPv4アドレス型に変換する
//!
//! 引数で指定された文字列がIPv4アドレスのフォーマットの場合に、
//! IPv4アドレス型に変換して出力パラメータに格納する。
//!
//! @param [in]  str        変換対象の文字列
//! @param [out] output     変換結果の出力先ポインタ
//! @param [out] prefixlen  プレフィックス長出力先ポインタ
//!
//! @retval true  変換成功
//! @retval false 変換失敗 (引数の文字列がIPv4アドレス形式でない)
///////////////////////////////////////////////////////////////////////////////
bool parse_ipv4address(const char *str, struct in_addr * output, int *prefixlen)
{
	// ローカル変数定義
	bool                            result;
	char                           *tmp;
	char                           *token;

	// 引数チェック
	if ((str == NULL) || (output == NULL)) {
		_D_(printf("parse_ipv4address Parameter Check NG.\n");
			)
			return false;
	}
	// ローカル変数初期化
	result = true;
	tmp = strdup(str);

	if (tmp == NULL) {
		return false;
	}

	token = strtok(tmp, "/");
	if (tmp) {
		if (inet_pton(AF_INET, token, output) <= 0) {
			_D_(printf("parse_ipv4address parse NG.\n");
				)
				result = false;
		} else {
			result = true;
		}
	}

	if (result && (prefixlen != NULL)) {
		token = strtok(NULL, "/");
		if (token == NULL) {
			*prefixlen = 0;
		} else {
			result = parse_int(token, prefixlen, CONFIG_IPV4_NETMASK_MIN, CONFIG_IPV4_NETMASK_MAX);
		}
	}

	free(tmp);

	DEBUG_LOG("%s end. return %s", __func__, result ? "true" : "false");
	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 文字列(IPv4 ネットワークアドレス)をIPv4アドレス型に変換する
//! ※MX6E-PRのみ
//! 引数で指定された文字列がIPv4アドレスのフォーマットの場合に、
//! IPv4アドレス型に変換して出力パラメータに格納する。
//! (default gateway:0.0.0.0, CIDR:0 設定用)
//!
//! @param [in]  str        変換対象の文字列
//! @param [out] output     変換結果の出力先ポインタ
//! @param [out] prefixlen  プレフィックス長出力先ポインタ
//!
//! @retval true  変換成功
//! @retval false 変換失敗 (引数の文字列がIPv4アドレス形式でない)
///////////////////////////////////////////////////////////////////////////////
bool parse_ipv4address_pr(const char *str, struct in_addr * output, int *prefixlen)
{
	// ローカル変数定義
	bool                            result;
	char                           *tmp;
	char                           *token;

	DEBUG_LOG("%s start\n", __func__);

	// 引数チェック
	if ((str == NULL) || (output == NULL)) {
		return false;
	}
	// ローカル変数初期化
	result = true;
	tmp = strdup(str);

	if (tmp == NULL) {
		return false;
	}

	token = strtok(tmp, "/");
	if (tmp) {
		if (inet_pton(AF_INET, token, output) <= 0) {
			result = false;
		} else {
			result = true;
		}
	}

	if (result && (prefixlen != NULL)) {
		token = strtok(NULL, "/");
		if (token == NULL) {
			*prefixlen = -1;
			result = false;
		} else {
			result = parse_int(token, prefixlen, CONFIG_IPV4_NETMASK_PR_MIN, CONFIG_IPV4_NETMASK_MAX);
		}
	}

	free(tmp);

	DEBUG_LOG("%s end. return %s\n", __func__, result ? "true" : "false");
	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 文字列をIPv6アドレス型に変換する
//!
//! 引数で指定された文字列がIPv6アドレスのフォーマットの場合に、
//! IPv6アドレス型に変換して出力パラメータに格納する。
//!
//! @param [in]  str        変換対象の文字列
//! @param [out] output     変換結果の出力先ポインタ
//! @param [out] prefixlen  プレフィックス長出力先ポインタ
//!
//! @retval true  変換成功
//! @retval false 変換失敗 (引数の文字列がIPv6アドレス形式でない)
///////////////////////////////////////////////////////////////////////////////
bool parse_ipv6address(const char *str, struct in6_addr * output, int *prefixlen)
{
	// ローカル変数定義
	bool                            result;
	char                           *tmp;
	char                           *token;

	DEBUG_LOG("%s start\n", __func__);

	// 引数チェック
	if ((str == NULL) || (output == NULL)) {
		return false;
	}
	// ローカル変数初期化
	result = true;
	tmp = strdup(str);

	if (tmp == NULL) {
		return false;
	}

	token = strtok(tmp, "/");
	if (tmp) {
		if (inet_pton(AF_INET6, token, output) <= 0) {
			result = false;
		} else {
			result = true;
		}
	}

	if (result && (prefixlen != NULL)) {
		token = strtok(NULL, "/");
		if (token == NULL) {
			*prefixlen = -1;
			result = false;
		} else {
			result = parse_int(token, prefixlen, CONFIG_IPV6_PREFIX_MIN, CONFIG_IPV6_PREFIX_MAX);
		}
	}

	free(tmp);

	DEBUG_LOG("%s end. return %s\n", __func__, result ? "true" : "false");
	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief 文字列をMACアドレス型に変換する
//!
//! 引数で指定された文字列がMACアドレスのフォーマットの場合に、
//! MACアドレス型に変換して出力パラメータに格納する。
//!
//! @param [in]  str        変換対象の文字列
//! @param [out] output     変換結果の出力先ポインタ
//!
//! @retval true  変換成功
//! @retval false 変換失敗 (引数の文字列がMACアドレス形式でない)
///////////////////////////////////////////////////////////////////////////////
bool parse_macaddress(const char *str, struct ether_addr * output)
{
	// ローカル変数定義
	bool                            result;

	// 引数チェック
	if ((str == NULL) || (output == NULL)) {
		_D_(printf("parse_macaddress Parameter Check NG.\n");)
			return false;
	}

	if (ether_aton_r(str, output) == NULL) {
		_D_(printf("parse_macaddress parse NG.\n");)
			result = false;
	} else {
		result = true;
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief IPv6アドレス比較
//!
//! 引数で指定されたIPv6アドレスを比較する
//!
//! @param [in]  addr1  IPv6アドレス
//! @param [in]  addr2  IPv6アドレス
//!
//! @retval true  一致
//! @retval false 不一致
///////////////////////////////////////////////////////////////////////////////
bool compare_ipv6(struct in6_addr * addr1, struct in6_addr * addr2)
{
	int                             i;

	// 引数チェック
	if ((addr1 == NULL) || (addr2 == NULL)) {
		return false;
	}

	for (i = 0; i < sizeof(addr1->s6_addr); i++) {
		if (addr1->s6_addr[i] != addr2->s6_addr[i]) {
			return false;
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief IPv6アドレス比較(マスク)
//!
//! 引数で指定されたIPv6アドレス(マスク)を比較する
//!
//! @param [in]  addr1  IPv6アドレス(内部でマスクを掛けるので値渡し)
//! @param [in]  addr2  IPv6アドレス(内部でマスクを掛けるので値渡し)
//! @param [in]  mask   IPv6アドレスマスク
//!
//! @retval true  一致
//! @retval false 不一致
///////////////////////////////////////////////////////////////////////////////
bool compare_ipv6_mask(struct in6_addr addr1, struct in6_addr addr2, struct in6_addr * mask)
{
	int                             i;

	// 引数チェック
	if ((mask == NULL)) {
		return false;
	}

	// *INDENT-OFF*
	_D_( {
		char addr[INET6_ADDRSTRLEN];
		DEBUG_LOG("addr1 : %s\n", inet_ntop(AF_INET6, &addr1, addr, sizeof(addr)));
		DEBUG_LOG("addr2 : %s\n", inet_ntop(AF_INET6, &addr2, addr, sizeof(addr)));
		DEBUG_LOG("mask  : %s\n", inet_ntop(AF_INET6, mask, addr, sizeof(addr)));
		} );
	// *INDENT-ON*

	// addr1, addr2にmask
	for (i = 0; i < sizeof(addr1.s6_addr); i++) {
		addr1.s6_addr[i] &= mask->s6_addr[i];
		addr2.s6_addr[i] &= mask->s6_addr[i];
	}

	// maskしたものを比較
	for (i = 0; i < sizeof(addr1.s6_addr); i++) {
		if (addr1.s6_addr[i] != addr2.s6_addr[i]) {
			return false;
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief MACアドレス比較
//!
//! 引数で指定されたMACアドレスを比較する
//!
//! @param [in]  hwaddr1  MACアドレス
//! @param [in]  hwaddr2  MACアドレス
//!
//! @retval true  一致
//! @retval false 不一致
///////////////////////////////////////////////////////////////////////////////
bool compare_hwaddr(struct ether_addr * hwaddr1, struct ether_addr * hwaddr2)
{
	// struct ether_addr {
	//  uint8_t ether_addr_octet[6];
	// }

	int                             i;

	// 引数チェック
	if ((hwaddr1 == NULL) || (hwaddr2 == NULL)) {
		return false;
	}
	// MACアドレス上位桁は製造元ベンダIDなどのため、重複する確率が高いため、下の桁から比較する
	for (i = sizeof(hwaddr1->ether_addr_octet) - 1; i <= 0; i--) {
		if (hwaddr1->ether_addr_octet[i] != hwaddr2->ether_addr_octet[i]) {
			return false;
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief コマンドコードの文字列を返す
//!
//! コマンドコードの文字列を返す
//!
//! @param [in]  code  コマンドコード
//!
//! @retval 文字列
//! @retval "undefined(%d)" 未定義コマンド
///////////////////////////////////////////////////////////////////////////////
const char                     *get_command_name(mx6e_command_code_t code)
{
	static char                     buf[256];
	// *INDENT-OFF*
	static const struct {
		mx6e_command_code_t code;
		const char *name;
		const char *desc;
	} table[] = {
		{MX6E_COMMAND_NONE,					"MX6E_COMMAND_NONE",				"MX6E_COMMAND_NONE"},	
		{MX6E_SETUP_FAILURE,				"MX6E_SETUP_FAILURE",				"設定失敗"},
		{MX6E_CHILD_INIT_END,				"MX6E_CHILD_INIT_END",				"子プロセス初期化完了"},
		{MX6E_NETDEV_MOVED,					"MX6E_NETDEV_MOVED",				"ネットワークデバイスの移動完了"},
		{MX6E_NETWORK_CONFIGURE,			"MX6E_NETWORK_CONFIGURE",			"PRネットワーク設定完了"},
		{MX6E_START_OPERATION,				"MX6E_START_OPERATION",				"運用開始指示"},
		{MX6E_SHOW_CONF,					"MX6E_SHOW_CONF",					"config表示"},
		{MX6E_SHOW_STATISTIC,				"MX6E_SHOW_STATISTIC",				"統計情報表示"},
		{MX6E_SHUTDOWN,						"MX6E_SHUTDOWN",					"シャットダウン指示"},
		{MX6E_RESTART,						"MX6E_RESTART",						"リスタート指示"},
		{MX6E_PR_TABLE_GENERATE_FAILURE,	"MX6E_PR_TABLE_GENERATE_FAILURE",	"MX6E-PRテーブル生成失敗"},
		{MX6E_ADD_M46E_ENTRY,				"MX6E_ADD_M46E_ENTRY",				"M46E ENTRY 追加"},
		{MX6E_DEL_M46E_ENTRY,				"MX6E_DEL_M46E_ENTRY",				"M46E ENTRY 削除"},
		{MX6E_DELALL_M46E_ENTRY,			"MX6E_DELALL_M46E_ENTRY",			"M46E ENTRY 全削除"},
		{MX6E_ENABLE_M46E_ENTRY,			"MX6E_ENABLE_M46E_ENTRY",			"M46E ENTRY 活性化"},
		{MX6E_DISABLE_M46E_ENTRY,			"MX6E_DISABLE_M46E_ENTRY",			"M46E ENTRY 非活性化"},
		{MX6E_SHOW_M46E_ENTRY,				"MX6E_SHOW_M46E_ENTRY",				"M46E ENTRY 表示"},
		{MX6E_ADD_ME6E_ENTRY,				"MX6E_ADD_ME6E_ENTRY",				"ME6E ENTRY 追加"},
		{MX6E_DEL_ME6E_ENTRY,				"MX6E_DEL_ME6E_ENTRY",				"ME6E ENTRY 削除"},
		{MX6E_DELALL_ME6E_ENTRY,			"MX6E_DELALL_ME6E_ENTRY",			"ME6E ENTRY 全削除"},
		{MX6E_ENABLE_ME6E_ENTRY,			"MX6E_ENABLE_ME6E_ENTRY",			"ME6E ENTRY 活性化"},
		{MX6E_DISABLE_ME6E_ENTRY,			"MX6E_DISABLE_ME6E_ENTRY",			"ME6E ENTRY 非活性化"},
		{MX6E_SHOW_ME6E_ENTRY,				"MX6E_SHOW_ME6E_ENTRY",				"ME6E ENTRY 表示"},
		{MX6E_LOAD_COMMAND,					"MX6E_LOAD_COMMAND",				"M46E/ME6E-Commandファイル読み込み"},
		{MX6E_SET_DEBUG_LOG,				"MX6E_SET_DEBUG_LOG",				"動的定義変更 デバッグログ出力設定"},
		{MX6E_SET_DEBUG_LOG_END,			"MX6E_SET_DEBUG_LOG_END",			"動的定義変更 デバッグログ出力設定完了"},
		{MX6E_COMMAND_MAX,					"MX6E_COMMAND_MAX",					"終端"},
	};
	// *INDENT-ON*

	for (int i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
		if (table[i].code == code) {
			return table[i].name;
		}
	}
	snprintf(buf, sizeof(buf), "undefined(%d)", code);
	return buf;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief mx6e_config_entry_tでユーザ指定部分からipv6形式を生成する
//!
//! @param [in]  entry  mx6e_config_entry_t
//!
//! @retval true  生成成功
//! @retval false 生成失敗
///////////////////////////////////////////////////////////////////////////////
bool make_config_entry(mx6e_config_entry_t * entry, table_type_t type, mx6e_config_devices_t * devices)
{
	char                            address[INET6_ADDRSTRLEN];
	struct in6_addr                 pid_src;
	struct in6_addr                 pid_des;
	int                             modbit;
	unsigned char					modbitmask;
	int                             i;

#define	MODBITMASK	(~(0xff >> modbit))

	// 引数チェック
	if (entry == NULL) {
		mx6e_logging(LOG_ERR, "Parameter Check NG(", __func__, ").");
		return false;
	}
	// plane_id_in
	snprintf(address, sizeof(address), "::%s", entry->src.plane_id);
	inet_pton(AF_INET6, address, &pid_src);

	// plane_id_out
	snprintf(address, sizeof(address), "::%s", entry->des.plane_id);
	inet_pton(AF_INET6, address, &pid_des);

	// *INDENT-OFF*
	_D_( {
		char addr[INET6_ADDRSTRLEN];
		DEBUG_LOG("pid_src : %s\n", entry->src.plane_id);
		DEBUG_LOG(" - %s\n", inet_ntop(AF_INET6, &pid_src, addr, sizeof(addr)));
		DEBUG_LOG("%08x:%08x:%08x:%08x\n", htonl(pid_src.s6_addr32[0]), htonl(pid_src.s6_addr32[1]), htonl(pid_src.s6_addr32[2]), htonl(pid_src.s6_addr32[3]));
		DEBUG_LOG("pid_des : %s\n", entry->des.plane_id);
		DEBUG_LOG(" - %s\n", inet_ntop(AF_INET6, &pid_des, addr, sizeof(addr)));
		DEBUG_LOG("%08x:%08x:%08x:%08x\n", htonl(pid_des.s6_addr32[0]), htonl(pid_des.s6_addr32[1]), htonl(pid_des.s6_addr32[2]), htonl(pid_des.s6_addr32[3]));
		}
	);
	// *INDENT-ON*


	// マスクをall 0に初期化
	memset(entry->src.mask.s6_addr, 0, sizeof(entry->src.mask.s6_addr));

	// トンネルデバイスアドレスprefix
	struct in6_addr                 prefix;
	if (DOMAIN_FP == entry->domain) {
		prefix = devices->tunnel_fp.ipv6_address;
	} else if (DOMAIN_PR == entry->domain) {
		prefix = devices->tunnel_pr.ipv6_address;
		/// マルチプレーン対応 2016/07/27 add start
		memcpy( &entry->section_dev_addr, &devices->tunnel_pr.ipv6_address, sizeof( struct in6_addr ) );
		entry->section_dev_prefix_len = devices->tunnel_pr.ipv6_netmask;
		/// マルチプレーン対応 2016/07/27 add end
	}

	switch (type) {
	case CONFIG_TYPE_M46E:
		// IPv4ネットワークアドレスチェック
		if (!mx6eapp_pt_check_network_addr(&entry->src.in.m46e.v4addr, entry->src.in.m46e.v4cidr)) {
			mx6e_logging(LOG_ERR, "Parameter Check NG(This is invalid address).");
			return false;
		}
		// CIDR2(プレフィックス)をサブネットマスク(xxx.xxx.xxx.xxx)へ変換
		PR_CIDR2SUBNETMASK(entry->src.in.m46e.v4cidr, entry->src.in.m46e.v4mask);

		// 変換元IPv6パターンを生成
		// M46E
		// |<------------- 128 ---------------->|
		// |<- prefix_len ->|<--ms-->|<-- 32 -->|
		// |prefix          |plane_id| IPv4 addr|
		// int ms = 128 - prefix_len - 32;

		// IPv4を埋める
		entry->src.src.s6_addr32[3] = entry->src.in.m46e.v4addr.s_addr;
		// IPv4cidrを適用
		entry->src.src.s6_addr32[3] &= entry->src.in.m46e.v4mask.s_addr;
		entry->src.mask.s6_addr32[3] = entry->src.in.m46e.v4mask.s_addr;

		// plane_idを埋める(とりあえず96bit分全部)
		entry->src.src.s6_addr32[2] = pid_src.s6_addr32[3];
		entry->src.src.s6_addr32[1] = pid_src.s6_addr32[2];
		entry->src.src.s6_addr32[0] = pid_src.s6_addr32[1];
		entry->src.mask.s6_addr32[2] = htonl(0xffffffff);
		entry->src.mask.s6_addr32[1] = htonl(0xffffffff);
		entry->src.mask.s6_addr32[0] = htonl(0xffffffff);

		// トンネルデバイスroute
		entry->src.tunnel_addr = entry->src.src;
		// m46eは128 - (32 - v4cidr)
		entry->src.tunnel_prefix_len = 128 - (32 - entry->src.in.m46e.v4cidr);

		// prefixを埋める
		for (i = 0; i < entry->src.prefix_len / 8; i++) {
			entry->src.src.s6_addr[i] = 0;
			entry->src.mask.s6_addr[i] = 0;

			/// マルチプレーン対応 2016/07/27 chg start
			//	entry->src.tunnel_addr.s6_addr[i] = prefix.s6_addr[i];
			entry->src.tunnel_addr.s6_addr[i] = entry->section_dev_addr.s6_addr[i];
			/// マルチプレーン対応 2016/07/27 chg end
		}

		// あまりbit
		modbit = entry->src.prefix_len - ((entry->src.prefix_len / 8) * 8);
		modbitmask = MODBITMASK;
		_D_(printf("あまりbit:%d, 0x%02x\n", modbit, modbitmask));
		if (0 < modbit) {
			i = entry->src.prefix_len / 8;
			entry->src.mask.s6_addr[i] &= (~modbitmask);

			entry->src.tunnel_addr.s6_addr[i] &= (~modbitmask);
			/// マルチプレーン対応 2016/07/27 chg start
			//	entry->src.tunnel_addr.s6_addr[i] |= (modbitmask & prefix.s6_addr[i]);
			entry->src.tunnel_addr.s6_addr[i] |= (modbitmask & entry->section_dev_addr.s6_addr[i]);
			/// マルチプレーン対応 2016/07/27 chg end
		}
		// test
		// entry->src.src.s6_addr[0] = 0x0d;
		// entry->src.src.s6_addr[1] = 0x0e;
		// entry->src.src.s6_addr[2] = 0x0a;
		// entry->src.src.s6_addr[3] = 0x0d;
		// entry->src.src.s6_addr[4] = 0x0b;
		// entry->src.src.s6_addr[5] = 0x0e;
		// entry->src.src.s6_addr[6] = 0x0e;
		// entry->src.src.s6_addr[7] = 0x0f;

		break;
	case CONFIG_TYPE_ME6E:
		// MACアドレスチェックはしない
		// 変換元IPv6パターンを生成

		// ME6E
		// |<------------- 128 ---------------->|
		// |<- prefix_len ->|<--ms-->|<-- 48 -->|
		// |prefix          |plane_id| MAC addr |
		// ms = 128 - prefix_len - 48

		// MACを埋める
		for (i = 0; i < ETH_ALEN; i++) {
			entry->src.src.s6_addr[15 - i] = entry->src.in.me6e.hwaddr.ether_addr_octet[ETH_ALEN - i - 1];
		}

		// plane_idを埋める(とりあえず96bit分全部)
		// |0  1   2   3    4        5  6  7    | s6_addr16
		// |0      1        2           3       | s6_addr32
		// |<------------- 128 ---------------->|
		// |<- prefix_len ->|<--ms-->|<-- 48 -->|
		// |prefix          |plane_id| MAC addr |
		// |                          6

		entry->src.src.s6_addr16[4] = pid_src.s6_addr16[7];
		entry->src.src.s6_addr16[3] = pid_src.s6_addr16[6];
		entry->src.src.s6_addr16[2] = pid_src.s6_addr16[5];
		entry->src.src.s6_addr16[1] = pid_src.s6_addr16[4];
		entry->src.src.s6_addr16[0] = pid_src.s6_addr16[3];
		entry->src.mask.s6_addr32[3] = htonl(0xffffffff);
		entry->src.mask.s6_addr32[2] = htonl(0xffffffff);
		entry->src.mask.s6_addr32[1] = htonl(0xffffffff);
		entry->src.mask.s6_addr32[0] = htonl(0xffffffff);

		// トンネルデバイストンネルデバイスroute
		entry->src.tunnel_addr = entry->src.src;
		// Me6Eは128bit全体
		entry->src.tunnel_prefix_len = 128;

		// prefixを埋める
		for (i = 0; i < entry->src.prefix_len / 8; i++) {
			entry->src.src.s6_addr[i] = 0;
			entry->src.mask.s6_addr[i] = 0;

			/// マルチプレーン対応 2016/07/27 chg start
			//	entry->src.tunnel_addr.s6_addr[i] = prefix.s6_addr[i];
			entry->src.tunnel_addr.s6_addr[i] = entry->section_dev_addr.s6_addr[i];
			/// マルチプレーン対応 2016/07/27 chg end
		}

		// あまりbit
		modbit = entry->src.prefix_len - ((entry->src.prefix_len / 8) * 8);
		modbitmask = MODBITMASK;	// prefix:1 planeid:0
		_D_(printf("あまりbit:%d, 0x%02x\n", modbit, modbitmask));

		if (0 < modbit) {
			i = entry->src.prefix_len / 8;
			_D_(printf("i:%d, entry->src.src.s6_addr[i]:0x%02x, pid_src.s6_addr[i]: 0x%02x\n", i, entry->src.src.s6_addr[i], pid_src.s6_addr[i]));
			entry->src.mask.s6_addr[i] &= (~modbitmask);

			entry->src.tunnel_addr.s6_addr[i] &= (~modbitmask);
			/// マルチプレーン対応 2016/07/27 chg start
			//	entry->src.tunnel_addr.s6_addr[i] |= (modbitmask & prefix.s6_addr[i]);
				entry->src.tunnel_addr.s6_addr[i] |= (modbitmask & entry->section_dev_addr.s6_addr[i]);
			/// マルチプレーン対応 2016/07/27 chg end
		}
		break;
	default:
		mx6e_logging(LOG_ERR, "table type (%d) NG", type);
		return false;
	}

	// // prefix - plane_id - ipv4
	// // IPv6 "1282001:0db8:bd05:01d2:288a:1fc0:0001:10ee"

	// 変換先IPv6パターンを生成
	// |<--------------- 128 ------------------>|
	// |<- prefix_len ->|<--md-->|<-- 32/48 --->| <- srcのCONFIG_TYPE_M46E/CONFIG_TYPE_ME6Eで変わる
	// |prefix          |plane_id| IPv4/MAC addr|
	// md = 128 - prefix_len - [32|48]

	switch (type) {
	case CONFIG_TYPE_M46E:
		memset(entry->des.src_mask.s6_addr, 0xff, sizeof(entry->des.src_mask.s6_addr));
		memset(entry->des.src_addr.s6_addr, 0x00, sizeof(entry->des.src_addr.s6_addr));
		memset(entry->des.dst_mask.s6_addr, 0xff, sizeof(entry->des.dst_mask.s6_addr));
		
		// IPv4部分のnetmaskされない部分はソースパケットのIPv4mask部分に置き換える
		// des.dst_mask
		// |<-------------------- 128 ------------------------>|
		// |s6_addr32[0]|s6_addr32[1]|s6_addr32[2]|s6_addr32[3]|
		// |    ffffffff|    ffffffff|    ffffffff|      v4mask|
		entry->des.dst_mask.s6_addr32[3] = entry->src.in.m46e.v4mask.s_addr;

		// IPv4を埋める(src->des)
		// des.dst_addr
		// |<-------------------- 128 ------------------------>|
		// |s6_addr32[0]|s6_addr32[1]|s6_addr32[2]|s6_addr32[3]|
		// |    xxxxxxxx|    xxxxxxxx|    xxxxxxxx|      v4addr|
		entry->des.dst_addr.s6_addr32[3] = entry->src.in.m46e.v4addr.s_addr;
		// IPv4cidrを適用(src->des)
		// des.dst_addr
		// |<-------------------- 128 ------------------------->|
		// |s6_addr32[0]|s6_addr32[1]|s6_addr32[2]| s6_addr32[3]|
		// |    xxxxxxxx|    xxxxxxxx|    xxxxxxxx|v4addr&v4mask|
		entry->des.dst_addr.s6_addr32[3] &= entry->src.in.m46e.v4mask.s_addr;

		// src maskはIPv6すべて
		// des.src_addr
		// |<-------------------- 128 ------------------------>|
		// |s6_addr32[0]|s6_addr32[1]|s6_addr32[2]|s6_addr32[3]|
		// |    00000000|    00000000|    00000000|    00000000|
		// des.src_mask
		// |    ffffffff|    ffffffff|    ffffffff|    00000000|
		entry->des.src_addr.s6_addr32[3] = 0;
		entry->des.src_mask.s6_addr32[3] = 0;

		_D_( {
			char addr[INET6_ADDRSTRLEN]; DEBUG_LOG("ipv6    ume des.dst_addr   : %s\n", inet_ntop(AF_INET6, &entry->des.dst_addr, addr, sizeof(addr)));}
		);

		// plane_idを埋める(とりあえず96bit分全部)
		// des.dst_addr
		// |<-------------------- 128 ------------------------->|
		// |s6_addr32[0]|s6_addr32[1]|s6_addr32[2]| s6_addr32[3]|
		// |  pid_dst[1]|  pid_dst[2]|  pid_dst[3]|v4addr&v4mask|
		entry->des.dst_addr.s6_addr32[2] = pid_des.s6_addr32[3];
		entry->des.dst_addr.s6_addr32[1] = pid_des.s6_addr32[2];
		entry->des.dst_addr.s6_addr32[0] = pid_des.s6_addr32[1];

		// des.src_addr
		// |<-------------------- 128 ------------------------>|
		// |s6_addr32[0]|s6_addr32[1]|s6_addr32[2]|s6_addr32[3]|
		// |  pid_dst[1]|  pid_dst[2]|  pid_dst[3]|    00000000|
		entry->des.src_addr.s6_addr32[2] = pid_des.s6_addr32[3];
		entry->des.src_addr.s6_addr32[1] = pid_des.s6_addr32[2];
		entry->des.src_addr.s6_addr32[0] = pid_des.s6_addr32[1];

		_D_( {
			char addr[INET6_ADDRSTRLEN];
			DEBUG_LOG("planeid ume des.dst_addr   : %s\n", inet_ntop(AF_INET6, &entry->des.dst_addr, addr, sizeof(addr)));
			}
		);

/// マルチプレーン対応 2016/07/27 chg start
#if 0
		// prefixを上書き
		// des.dst_addr
		// |<-------------------- 128 ------------------------->|
		// |s6_addr32[0]|s6_addr32[1]|s6_addr32[2]| s6_addr32[3]|
		// |<---- prefix_len ---->|<----- md ---->|<---- 32 --->|
		// |         prefix       |    pid_dst    |v4addr&v4mask|
		// des.dst_mask
		// |         ffff         |    ffff       |v4addr&v4mask|
		for (i = 0; i < entry->des.prefix_len / 8; i++) {
			entry->des.dst_addr.s6_addr[i] = entry->des.prefix.s6_addr[i];
			entry->des.dst_mask.s6_addr[i] = 0xff;

			if (DOMAIN_FP == entry->domain) {
				// FP->PRの場合はtunnel_pr.ipv6_addressをprefixとして使用する
				// des.src_addr
				// |<-------------------- 128 ------------------------>|
				// |s6_addr32[0]|s6_addr32[1]|s6_addr32[2]|s6_addr32[3]|
				// |<---- prefix_len ---->|<----- md ---->|<---- 32 --->|
				// |      tunnel_pr       |    pid_dst    |v4addr&v4mask|
				prefix = devices->tunnel_pr.ipv6_address;
				/// マルチプレーン対応 2016/07/27 chg start
				//entry->des.src_addr.s6_addr[i] = prefix.s6_addr[i];
				entry->des.src_addr.s6_addr[i] = entry->section_dev_addr.s6_addr[i];
				/// マルチプレーン対応 2016/07/27 chg end
			} else if (DOMAIN_PR == entry->domain) {
				// des.src_addr
				// |<-------------------- 128 ------------------------>|
				// |s6_addr32[0]|s6_addr32[1]|s6_addr32[2]|s6_addr32[3]|
				// |<---- prefix_len ---->|<----- md ---->|<---- 32 --->|
				// |      des.prefix      |    pid_dst    |v4addr&v4mask|
				entry->des.src_addr.s6_addr[i] = entry->des.prefix.s6_addr[i];
			}
		}

		// *INDENT-OFF*
		_D_( {
			char addr[INET6_ADDRSTRLEN];
			DEBUG_LOG("prefix  ume des.prefix: %s\n", inet_ntop(AF_INET6, &entry->des.prefix, addr, sizeof(addr)));
			DEBUG_LOG("prefix  ume des.dst_addr   : %s\n", inet_ntop(AF_INET6, &entry->des.dst_addr, addr, sizeof(addr)));
			DEBUG_LOG("prefix  ume des.dst_mask  : %s\n", inet_ntop(AF_INET6, &entry->des.dst_mask, addr, sizeof(addr)));
			}
		);
		// *INDENT-ON*


		// あまりbit
		// des.dst_addr, des.dst_mask, des.src_addr
		// |<-------- 8 -------->|
		// |<-- modbit -->|      |
		// |1..1          |      |des.dst_mask
		// |prefix        |      |des.dst_addr, des.src_addr
		modbit = entry->des.prefix_len - ((entry->des.prefix_len / 8) * 8);
		modbitmask = MODBITMASK;
		_D_(printf("あまりbit:%d, 0x%02x\n", modbit, modbitmask));
		if (0 < modbit) {
			entry->des.dst_addr.s6_addr[i] |= modbitmask;
			entry->des.dst_addr.s6_addr[i] &= (modbitmask & entry->des.prefix.s6_addr[i]);

			entry->des.src_addr.s6_addr[i] |= modbitmask;
			entry->des.src_addr.s6_addr[i] &= (modbitmask & entry->des.prefix.s6_addr[i]);

			if (DOMAIN_FP == entry->domain) {
				// FP->PRの場合はtunnel_pr.ipv6_addressをprefixとして使用する
				prefix = devices->tunnel_pr.ipv6_address;
				entry->des.src_addr.s6_addr[i] |= modbitmask;
				/// マルチプレーン対応 2016/07/27 chg start
				//entry->des.src_addr.s6_addr[i] &= (modbitmask & prefix.s6_addr[i]);
				entry->des.src_addr.s6_addr[i] &= (modbitmask & entry->section_dev_addr.s6_addr[i]);
				/// マルチプレーン対応 2016/07/27 chg end

			} else if (DOMAIN_PR == entry->domain) {
				entry->des.src_addr.s6_addr[i] |= modbitmask;
				entry->des.src_addr.s6_addr[i] &= (modbitmask & entry->des.prefix.s6_addr[i]);
			}
		}
#else
		// prefixを上書き
		for (i = 0; i < entry->des.prefix_len / 8; i++) {
			entry->des.dst_addr.s6_addr[i] = entry->des.prefix.s6_addr[i];
			entry->des.dst_mask.s6_addr[i] = 0xff;
			entry->des.src_addr.s6_addr[i] = entry->des.prefix.s6_addr[i];
		}

		// あまりbit
		modbit = entry->des.prefix_len - ((entry->des.prefix_len / 8) * 8);
		modbitmask = MODBITMASK;
		_D_(printf("あまりbit:%d, 0x%02x\n", modbit, modbitmask));
		if (0 < modbit) {
			entry->des.dst_addr.s6_addr[i] |= modbitmask;
			entry->des.dst_addr.s6_addr[i] &= (modbitmask & entry->des.prefix.s6_addr[i]);

			entry->des.src_addr.s6_addr[i] |= modbitmask;
			entry->des.src_addr.s6_addr[i] &= (modbitmask & entry->des.prefix.s6_addr[i]);
		}

		// FPドメインから受信した場合、PRドメインのソースアドレスはPRのプレフィックスを設定
		if (DOMAIN_FP == entry->domain) {
			for (i = 0; i < devices->tunnel_pr.ipv6_netmask / 8; i++) {
				entry->des.src_addr.s6_addr[i] = devices->tunnel_pr.ipv6_address.s6_addr[i];
			}
			// あまりbit
			modbit = devices->tunnel_pr.ipv6_netmask - ((devices->tunnel_pr.ipv6_netmask / 8) * 8);
			modbitmask = MODBITMASK;
			if (0 < modbit) {
				entry->des.src_addr.s6_addr[i] |= modbitmask;
				entry->des.src_addr.s6_addr[i] &= (modbitmask & devices->tunnel_pr.ipv6_address.s6_addr[i]);
			}
		}
#endif
/// マルチプレーン対応 2016/07/27 chg end

		entry->src.tunnel_src = entry->des.src_addr;
		entry->src.tunnel_src.s6_addr32[3] = entry->src.in.m46e.v4addr.s_addr;
		entry->src.tunnel_src_prefix_len = 128 - entry->src.in.m46e.v4cidr;	// 全ビット - V4maskされてない長さ
		
		// *INDENT-OFF*
		_D_( {
			char addr[INET6_ADDRSTRLEN];
			DEBUG_LOG("amari   ume des.dst_addr : %s\n", inet_ntop(AF_INET6, &entry->des.dst_addr, addr, sizeof(addr)));
			DEBUG_LOG("amari   ume des.dst_mask : %s\n", inet_ntop(AF_INET6, &entry->des.dst_mask, addr, sizeof(addr)));
			}
		);
		// *INDENT-ON*


		break;
	case CONFIG_TYPE_ME6E:
		// MAC部分はすべて置き換える
		// MACを埋める(src->des)
		memset(entry->des.src_mask.s6_addr, 0xff, sizeof(entry->des.src_mask.s6_addr));
		memset(entry->des.src_addr.s6_addr, 0x00, sizeof(entry->des.src_addr.s6_addr));
		memset(entry->des.dst_mask.s6_addr, 0xff, sizeof(entry->des.dst_mask.s6_addr));
		for (i = 0; i < ETH_ALEN; i++) {
			entry->des.src_mask.s6_addr[15 - i] = 0x00;
			entry->des.dst_addr.s6_addr[15 - i] = entry->src.in.me6e.hwaddr.ether_addr_octet[ETH_ALEN - i - 1];
		}
		// plane_idを埋める(とりあえず(128 - 48) / 8 = 10byte分全部)
		for (i = 0; i < (128 - 48) / 8; i++) {
			entry->des.src_addr.s6_addr[15 - ETH_ALEN - i] = pid_des.s6_addr[15 - i];
			entry->des.dst_addr.s6_addr[15 - ETH_ALEN - i] = pid_des.s6_addr[15 - i];
		}

		// prefixを上書き
		for (i = 0; i < entry->des.prefix_len / 8; i++) {
			entry->des.src_addr.s6_addr[i] = entry->des.prefix.s6_addr[i];
			entry->des.dst_addr.s6_addr[i] = entry->des.prefix.s6_addr[i];
		}
		// あまりbit
		modbit = entry->des.prefix_len - ((entry->des.prefix_len / 8) * 8);
		modbitmask = MODBITMASK;
		_D_(printf("あまりbit:%d, 0x%02x\n", modbit, modbitmask));
		if (0 < modbit) {
			entry->des.dst_addr.s6_addr[i] |= modbitmask;
			entry->des.dst_addr.s6_addr[i] &= (modbitmask & entry->des.prefix.s6_addr[i]);

			entry->des.src_addr.s6_addr[i] |= modbitmask;
			entry->des.src_addr.s6_addr[i] &= (modbitmask & entry->des.prefix.s6_addr[i]);
		}
		/// マルチプレーン対応 2016/07/27 add start
		// FPドメインから受信した場合、PRドメインのソースアドレスはPRのプレフィックスを設定
		if (DOMAIN_FP == entry->domain) {
			for (i = 0; i < devices->tunnel_pr.ipv6_netmask / 8; i++) {
				entry->des.src_addr.s6_addr[i] = devices->tunnel_pr.ipv6_address.s6_addr[i];
			}
			// あまりbit
			modbit = devices->tunnel_pr.ipv6_netmask - ((devices->tunnel_pr.ipv6_netmask / 8) * 8);
			modbitmask = MODBITMASK;
			if (0 < modbit) {
				entry->des.src_addr.s6_addr[i] |= modbitmask;
				entry->des.src_addr.s6_addr[i] &= (modbitmask & devices->tunnel_pr.ipv6_address.s6_addr[i]);
			}
		}
		/// マルチプレーン対応 2016/07/27 add end

		entry->src.tunnel_src = entry->des.src_addr;
		for (i = 0; i < ETH_ALEN; i++) {
			entry->src.tunnel_src.s6_addr[15 - i] = entry->src.in.me6e.hwaddr.ether_addr_octet[ETH_ALEN - i - 1];
		}
		entry->src.tunnel_src_prefix_len = 128;	// 全ビット


		break;

	default:
		mx6e_logging(LOG_ERR, "table type (%d) NG", type);
		return false;
	}

	// *INDENT-OFF*
	_D_( {
		char addr[INET6_ADDRSTRLEN];
		DEBUG_LOG("src.src      : %s\n", inet_ntop(AF_INET6, &entry->src.src, addr, sizeof(addr)));
		DEBUG_LOG("src.mask     : %s\n", inet_ntop(AF_INET6, &entry->src.mask, addr, sizeof(addr)));
		DEBUG_LOG("des.src_addr : %s\n", inet_ntop(AF_INET6, &entry->des.src_addr, addr, sizeof(addr)));
		DEBUG_LOG("des.src_mask : %s\n", inet_ntop(AF_INET6, &entry->des.src_mask, addr, sizeof(addr)));
		DEBUG_LOG("des.dst_addr : %s\n", inet_ntop(AF_INET6, &entry->des.dst_addr, addr, sizeof(addr)));
		DEBUG_LOG("des.dst_mask : %s\n", inet_ntop(AF_INET6, &entry->des.dst_mask, addr, sizeof(addr)));
		DEBUG_LOG("src.tunnel_addr : %s\n", inet_ntop(AF_INET6, &entry->src.tunnel_addr,addr, sizeof(addr)));

		}
	);
	// *INDENT-ON*

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief get_domain_name domain_tから文字列を返す
//!
//! @param [in]  domain domain_t
//!
//! @retval "文字列"
//! @retval "?" 不明
///////////////////////////////////////////////////////////////////////////////
const char                     *get_domain_name(domain_t domain)
{
	const static char              *fp = "FP";
	const static char              *pr = "PR";
	const static char              *other = "??";

	switch (domain) {
	case DOMAIN_FP:
		return fp;

	case DOMAIN_PR:
		return pr;

	default:
		return other;
	}
}

///////////////////////////////////////////////////////////////////////////////
//! @brief get_table_name table_type_tから文字列を返す
//!
//! @param [in]  table_type table_type_t
//!
//! @retval "文字列"
//! @retval "?" 不明
///////////////////////////////////////////////////////////////////////////////
const char                     *get_table_name(table_type_t table_type)
{
	const static char              *m46e = "M46E";
	const static char              *me6e = "Me6E";
	const static char              *other = "????";

	switch (table_type) {
	case CONFIG_TYPE_M46E:
		return m46e;

	case CONFIG_TYPE_ME6E:
		return me6e;

	default:
		return other;
	}
}

///////////////////////////////////////////////////////////////////////////////
//! @brief get_domain_src_ifindex 指定のdomainのデバイスindexを取得
//!
//! @param [in]  domain domain_t
//! @param [in]  devices
//!
//! @retval "ifindex"
//! @retval "-1" 不明
///////////////////////////////////////////////////////////////////////////////
int get_domain_src_ifindex(domain_t domain, mx6e_config_devices_t * devices)
{
	switch (domain) {
	case DOMAIN_FP:
		return devices->tunnel_fp.ifindex;

	case DOMAIN_PR:
		return devices->tunnel_pr.ifindex;

	default:
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
//! @brief get_domain_dst_ifindex 指定のdomainの送信先のデバイスindexを取得
//!
//! @param [in]  domain domain_t
//! @param [in]  devices
//!
//! @retval "ifindex"
//! @retval "-1" 不明
///////////////////////////////////////////////////////////////////////////////
int get_domain_dst_ifindex(domain_t domain, mx6e_config_devices_t * devices)
{
	switch (domain) {
	case DOMAIN_FP:
		return devices->tunnel_pr.ifindex;

	case DOMAIN_PR:
		return devices->tunnel_fp.ifindex;

	default:
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
//! @brief get_bit_width 指定のPlaneIDの有効ビット長
//! MSBから最初のビット1の位置からLSBまでのビット長
//! @param [in]  pid planeid
//!
//! @retval 有効ビット長
///////////////////////////////////////////////////////////////////////////////
int get_pid_bit_width(char plane_id[])
{
	int r = 0;
	char                            address[INET6_ADDRSTRLEN];
	struct in6_addr pid;

	snprintf(address, sizeof(address), "::%s", plane_id);
	inet_pton(AF_INET6, address, &pid);

	_D_({
			char                            buf[128];
			inet_ntop(AF_INET6, &pid, buf, sizeof(buf));
			printf("%s - %s\n", plane_id, buf);
		});

	
	for (int i = 0; i < sizeof(pid.s6_addr); i ++) {
		if (pid.s6_addr[i]) {
			for (int b = 0; b < 8; b ++) {
				if (pid.s6_addr[i] & (0x80 >> b)) {
					// hit
					r = 128 - (i * 8 + b);
					_D_(printf("i:%d, b:%d, result:%d\n", i, b, r));
					return r;
				}
			}
		}
	}
	return 0;
}
