/******************************************************************************/
/* ファイル名 : mx6eapp_pt.c                                                  */
/* 機能概要   : MX6E Prefix Resolution ソースファイル                         */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <limits.h>
#include <search.h>

#include "mx6eapp.h"
#include "mx6eapp_pt.h"
#include "mx6eapp_log.h"
#include "mx6eapp_network.h"
#include "mx6eapp_util.h"

////////////////////////////////////////////////////////////////////////////////
// 内部関数プロトタイプ宣言
////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////!
//! @brief IPv4ネットワークアドレス変換関数
//!
//! cidrから引数のアドレスを、ネットワークアドレスへ変換する。
//!
//! @param [in]     inaddr  変換前v4address
//! @param [in]     cidr    変換に使用するv4netmask（CIDR形式）
//! @param [out]    outaddr 変換後v4address
//!
//! @return true        変換成功
//!         false       変換失敗
///////////////////////////////////////////////////////////////////////////////
bool mx6eapp_pt_convert_network_addr(struct in_addr *inaddr, int cidr, struct in_addr *outaddr)
{
	struct in_addr                  in_mask;

	// 引数チェック
	if ((inaddr == NULL) || (cidr < 0) || (cidr > 32) || (outaddr == NULL)) {
		mx6e_logging(LOG_ERR, "Parameter Check NG(%s).", __func__);
		return false;
	}
	// IPv4のサブネットマスク長
	PR_CIDR2SUBNETMASK(cidr, in_mask);
	// IPv4アドレス(ネットワークアドレス)
	outaddr->s_addr = inaddr->s_addr & in_mask.s_addr;

	return true;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief IPv4ネットワークアドレスチェック関数
//!
//! cidrから引数のアドレスが、ネットワークアドレスか
//! （ホスト部のアドレスが0か）チェックする。
//!
//! @param [in]     addr    チェックするv4address
//! @param [in]     cidr    チェックに使用するv4netmask（CIDR形式）
//!
//! @return true        OK（ネットワークアドレスである）
//!         false       NG（ネットワークアドレスでない）
///////////////////////////////////////////////////////////////////////////////
bool mx6eapp_pt_check_network_addr(struct in_addr * addr, int cidr)
{
	struct in_addr                  in_mask;
	struct in_addr                  in_net;

	// 引数チェック
	if ((addr == NULL) || (cidr < 0) || (cidr > 32)) {
		mx6e_logging(LOG_ERR, "Parameter Check NG(%s).", __func__);
		return false;
	}
	// IPv4のサブネットマスク長
	PR_CIDR2SUBNETMASK(cidr, in_mask);
	// IPv4アドレス(ネットワークアドレス)
	in_net.s_addr = addr->s_addr & in_mask.s_addr;

	if (in_net.s_addr != addr->s_addr) {
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR Config entry出力関数（デバッグ用）
//!
//! MX6E-PR Config entryを出力する。
//!
//! @param [in] entry   出力するMX6E-PR Config entry
//!
//! @return none
///////////////////////////////////////////////////////////////////////////////
void m46e_pt_config_entry_dump(const mx6e_config_entry_t * entry)
{
	char                            address[INET6_ADDRSTRLEN];
	const char                     *strbool[] = { "disable", "enable" };
	const char                     *strdomain[] = { "DOMAIN_NONE", "DOMAIN_FP", "DOMAIN_PR" };

	// 引数チェック
	if (entry == NULL) {
		mx6e_logging(LOG_ERR, "Parameter Check NG(%s).", __func__);
		return;
	}
	struct in6_addr                 pid_src;
	struct in6_addr                 pid_des;

	snprintf(address, sizeof(address), "::%s", entry->src.plane_id);
	inet_pton(AF_INET6, address, &pid_src);
	snprintf(address, sizeof(address), "::%s", entry->des.plane_id);
	inet_pton(AF_INET6, address, &pid_des);

	printf("--------------------------------\n");
	printf("enable|%s\n", strbool[entry->enable]);
	printf("domain|%s\n", strdomain[entry->domain]);
	printf("--------------------|-----------\n");
	printf("src.plane_id        |%s(%s)\n", entry->src.plane_id, inet_ntop(AF_INET6, &pid_src, address, sizeof(address)));
	printf("src.prefix_len      |%d\n", entry->src.prefix_len);
	printf("src.in.m46e.v4addr  |%s\n", inet_ntop(AF_INET, &entry->src.in.m46e.v4addr, address, sizeof(address)));
	printf("src.in.m46e.v4cidr  |%d\n", entry->src.in.m46e.v4cidr);
	printf("* src.in.m46e.v4mask|%s\n", inet_ntop(AF_INET, &entry->src.in.m46e.v4mask, address, sizeof(address)));
	printf("src.in.me6e.hwaddr  |%s\n", ether_ntoa(&entry->src.in.me6e.hwaddr));
	printf("* src.src           |%s\n", inet_ntop(AF_INET6, &entry->src.src, address, sizeof(address)));
	printf("* src.mask          |%s\n", inet_ntop(AF_INET6, &entry->src.mask, address, sizeof(address)));
	printf("--------------------|-----------\n");
	printf("des.plane_id        |%s(%s)\n", entry->des.plane_id, inet_ntop(AF_INET6, &pid_des, address, sizeof(address)));
	printf("des.prefix          |%s\n", inet_ntop(AF_INET6, &entry->des.prefix, address, sizeof(address)));
	printf("des.prefix_len      |%d\n", entry->des.prefix_len);
	printf("* des.src_addr      |%s\n", inet_ntop(AF_INET6, &entry->des.src_addr, address, sizeof(address)));
	printf("* des.src_mask      |%s\n", inet_ntop(AF_INET6, &entry->des.src_mask, address, sizeof(address)));
	printf("* des.dst_addr      |%s\n", inet_ntop(AF_INET6, &entry->des.dst_addr, address, sizeof(address)));
	printf("* des.dst_mask      |%s\n", inet_ntop(AF_INET6, &entry->des.dst_mask, address, sizeof(address)));
	printf("--------------------------------\n");

	return;
}

static void twaction(const void *nodep, const VISIT which, const int depth)
{
	switch (which) {
	case preorder:
	case endorder:
		break;
	case postorder:
	case leaf:
		m46e_pt_config_entry_dump(*((mx6e_config_entry_t **)nodep));
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR Config Table出力関数（デバッグ用）
//!
//! MX6E-PR Config Tableを出力する。
//!
//! @param [in] table   出力するMX6E-PR Config Table
//!
//! @return none
///////////////////////////////////////////////////////////////////////////////
void m46e_pt_config_table_dump(const mx6e_config_table_t * table)
{
	DEBUG_LOG("*** %s ***\n", __func__);

	// 引数チェック
	if (table == NULL) {
		mx6e_logging(LOG_ERR, "Parameter Check NG(%s).", __func__);
		return;
	}

	DEBUG_LOG("table num = %d\n", table->num);
	DEBUG_LOG("entry_list:%p\n", &table->root);

	if (0 == table->num) {
		return;
	}

	twalk(table->root, twaction);

	return;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief tsearch/tfind 挿入・削除・enable/disable変更時の比較関数
//!
//! 比較結果を返す
//!
//! @param [in] p1, p2   比較するエントリへのポインタ
//!
//! @return p1 < p2  -
//! @return p1 = p2  0
//! @return p1 > p2  +
///////////////////////////////////////////////////////////////////////////////
static int compins(const void *p1, const void *p2)
{
	int r;
	mx6e_config_entry_t *v1 = (mx6e_config_entry_t *) p1;
	mx6e_config_entry_t *v2 = (mx6e_config_entry_t *) p2;

	// *INDENT-OFF*
	_D_({
			char                            address[INET6_ADDRSTRLEN];
			printf("%s: v1 %s %s/",
				   __func__,
				   get_domain_name(v1->domain),
				   inet_ntop(AF_INET6, &v1->src.src, address, sizeof(address)));
			printf("%s\n", 
				   inet_ntop(AF_INET6, &v1->src.mask, address, sizeof(address)));
			printf("%s: v2 %s %s/",
				   __func__,
				   get_domain_name(v2->domain),
				   inet_ntop(AF_INET6, &v2->src.src, address, sizeof(address)));
			printf("%s\n", 
				   inet_ntop(AF_INET6, &v2->src.mask, address, sizeof(address)));
		});
	// *INDENT-ON*
	

	// 1. ドメインを比較
	if (v1->domain != v2->domain) {
		return v2->domain - v1->domain;
	}

	// 2. src.srcを比較
	// 符号付き演算で結果を返すために、32bit（ではアンダーフローするので）ではなく、16bitで比較する
	for (int i = 0; i < 8; i ++) {
		if (0 != (r = ntohs(v1->src.src.s6_addr16[i]) - ntohs(v2->src.src.s6_addr16[i]))) {
			return r;
		}
	}

	// 3. src.maskを比較
	for (int i = 0; i < 8; i ++) {
		if (0 != (r = ntohs(v1->src.mask.s6_addr16[i]) - ntohs(v2->src.mask.s6_addr16[i]))) {
			return r;
		}
	}
	
	return 0;
}

// 
///////////////////////////////////////////////////////////////////////////////
//! @brief tfind 受信パケット検索時の比較関数
//!
//! 比較結果を返す
//! ツリーはcompins関数で比較した以下のキー順によるソートオーダになっている
//!  1.ドメイン
//!  2.src.src
//!  3.src.mask
//! この関数はそれに合わせて、以下のキー順で比較する
//!  1.ドメイン
//!  2.src.src & src.mask
//!
//! @param [in] p1, p2   比較するエントリへのポインタ
//!
//! @return p1 < p2  -
//! @return p1 = p2  0
//! @return p1 > p2  +
///////////////////////////////////////////////////////////////////////////////
static int compfind(const void *p1, const void *p2)
{
	int r;
	mx6e_config_entry_t *v1 = (mx6e_config_entry_t *) p1;
	mx6e_config_entry_t *v2 = (mx6e_config_entry_t *) p2;

	struct in6_addr                 mask;

	// 1. ドメインを比較
	if (v1->domain != v2->domain) {
		return v2->domain - v1->domain;
	}

	// 引数のどちらが検索キーかは(多分p1だろうけど)実装依存と思われるので、両方のマスクのORを取る
	// キーのmaskには0を設定しておくこと
	mask.s6_addr32[0] = v1->src.mask.s6_addr32[0] | v2->src.mask.s6_addr32[0];
	mask.s6_addr32[1] = v1->src.mask.s6_addr32[1] | v2->src.mask.s6_addr32[1];
	mask.s6_addr32[2] = v1->src.mask.s6_addr32[2] | v2->src.mask.s6_addr32[2];
	mask.s6_addr32[3] = v1->src.mask.s6_addr32[3] | v2->src.mask.s6_addr32[3];

	// *INDENT-OFF*
	_D_({
			char                            address[INET6_ADDRSTRLEN];
			printf("%s: v1 %s %s/",
				   __func__,
				   get_domain_name(v1->domain),
				   inet_ntop(AF_INET6, &v1->src.src, address, sizeof(address)));
			printf("%s\n", 
				   inet_ntop(AF_INET6, &v1->src.mask, address, sizeof(address)));
			printf("%s: v2 %s %s/",
				   __func__,
				   get_domain_name(v2->domain),
				   inet_ntop(AF_INET6, &v2->src.src, address, sizeof(address)));
			printf("%s\n", 
				   inet_ntop(AF_INET6, &v2->src.mask, address, sizeof(address)));
		});
	// *INDENT-ON*
	
	// 2,3 src.src & src.mask を比較
	// 符号付き演算で結果を返すために、32bit（ではアンダーフローするので）ではなく、16bitで比較する
	for (int i = 0; i < 8; i ++) {
		if (0 != (r = ntohs(v1->src.src.s6_addr16[i] & mask.s6_addr16[i]) - ntohs(v2->src.src.s6_addr16[i] & mask.s6_addr16[i]))) {
			return r;
		}
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR Config Table追加関数
//!
//! MX6E-PR Config Tableへ新規エントリーを追加する。
//! 新規エントリーは、テーブルの最後に追加する。
//! ※entryは、ヒープ領域(malloc関数などで確保したメモリ領域)を渡すこと。
//! エントリーのIPv4アドレスがネットワークアドレスがでない場合は、
//! 追加失敗とする。
//! テーブル内にIPv4アドレスとv4cidrが同一のエントリーが既にある場合は、
//! 追加失敗とする。
//!
//! @param [in/out] table   追加するMX6E-PR Config Table
//! @param [in]     entry   追加するエントリー情報
//!
//! @return true        追加成功
//!         false       追加失敗
///////////////////////////////////////////////////////////////////////////////
bool m46e_pt_add_config_entry(mx6e_config_table_t * table, mx6e_config_entry_t * entry, mx6e_config_devices_t * devices)
{
	bool                            result = true;

	_D_(printf("%s:enter\n", __func__));

	// 引数チェック
	if ((table == NULL) || (entry == NULL)) {
		mx6e_logging(LOG_ERR, "Parameter Check NG(%s).", __func__);
		return false;
	}
	switch (table->type) {
	case CONFIG_TYPE_M46E:
		// IPv4ネットワークアドレスチェック
		if (!mx6eapp_pt_check_network_addr(&entry->src.in.m46e.v4addr, entry->src.in.m46e.v4cidr)) {
			mx6e_logging(LOG_ERR, "Parameter Check NG(This is invalid address).");
			return false;
		}
		break;
	case CONFIG_TYPE_ME6E:
		// MACアドレスチェックはしない
		break;
	default:
		mx6e_logging(LOG_ERR, "table type (%d) NG", table->type);
		return false;
	}

	// prefix長 + planeid長 + IPv4/MACチェック
	int pidlen;
	int len;
	switch (table->type) {
	case CONFIG_TYPE_M46E:
		// IPv4ネットワークアドレスチェック
		if (!mx6eapp_pt_check_network_addr(&entry->src.in.m46e.v4addr, entry->src.in.m46e.v4cidr)) {
			mx6e_logging(LOG_ERR, "Parameter Check NG(This is invalid address).");
			return false;
		}
		// プレフィクス長 + PlaneID長 + IPv4(32bit)がIPv6(128)より大きいとエラー
		pidlen = get_pid_bit_width(entry->src.plane_id);
		len = entry->src.prefix_len + pidlen + 32;	// 32:IPv4長
		if (128 < len) {
			mx6e_logging(LOG_ERR, "Parameter Check NG(128 < %d (in_plane_id len(%d) + in_plefix_len(%d) + IPv4 len(%d))).", len, entry->src.prefix_len, pidlen, 32);
			return false;
		}
		pidlen = get_pid_bit_width(entry->des.plane_id);
		len = entry->des.prefix_len + pidlen + 32;	// 32:IPv4長
		if (128 < len) {
			mx6e_logging(LOG_ERR, "Parameter Check NG(128 < %d (out_plefix_len(%d) + out_plane_id len(%d) + IPv4 len(%d))).", len, entry->des.prefix_len, pidlen, 32);
			return false;
		}
		break;
	case CONFIG_TYPE_ME6E:
		// MACアドレスチェックはしない
		// プレフィクス長 + PlaneID長 + MAC(48bit)がIPv6(128)より大きいとエラー
		pidlen = get_pid_bit_width(entry->src.plane_id);
		len = entry->src.prefix_len + pidlen + 48;	// 48:MAC長
		if (128 < len) {
			mx6e_logging(LOG_ERR, "Parameter Check NG(128 < %d (in_plane_id len(%d) + in_plefix_len(%d) + IPv4 len(%d))).", len, entry->src.prefix_len, pidlen, 48);
			return false;
		}
		pidlen = get_pid_bit_width(entry->des.plane_id);
		len = entry->des.prefix_len + pidlen + 48;	// 48:MAC長
		if (128 < len) {
			mx6e_logging(LOG_ERR, "Parameter Check NG(128 < %d (out_plefix_len(%d) + out_plane_id len(%d) + IPv4 len(%d))).", len, entry->des.prefix_len, pidlen, 48);
			return false;
		}
		break;
	default:
		mx6e_logging(LOG_ERR, "table type (%d) NG", table->type);
		return false;
	}

	// 排他開始
	DEBUG_LOG("pthread_mutex_lock  TID  = %lx\n", (unsigned long int) pthread_self());
	pthread_mutex_lock(&table->mutex);

	mx6e_config_entry_t **r;
	if (PT_MAX_ENTRY_NUM > table->num) {
		// 容量ある
		// 入力値から必要項目の生成
		if (false == make_config_entry(entry, table->type, devices)) {
			// 入力値から必要項目の生成に失敗
			mx6e_logging(LOG_INFO, "MX6E-PR make_config_entry fail\n");
			result = false;
		} else {
			mx6e_config_entry_t            *p = malloc(sizeof(mx6e_config_entry_t));
			*p = *entry;
			
			//if (NULL == (r = tsearch((void *)p, &table->root, compins))) {
			if (NULL == (r = tsearch((void *)p, &table->root, compfind))) {
				mx6e_logging(LOG_ERR, "Out of memory.");
				result = false;
			} else if (*r != p) {
				mx6e_logging(LOG_ERR, "This entry is is already exists.");
				result = false;
				free(p);
			} else {
				// 要素数のインクリメント
				table->num++;
		
				int                             ifindex = get_domain_src_ifindex(entry->domain, devices);
				// PRテーブルへの登録が成功した場合、活性化を伴う追加要求の場合は
				// 当該PR EntryのIPネットワーク経路を追加
				if (entry->enable) {
					// 追加時にenableの場合のみrouteを追加
					// route追加(IPアドレスを追加すると、OSがパケットを処理してしまうので、routeだけ追加する)
					mx6e_network_add_route(AF_INET6, ifindex, &entry->src.tunnel_addr, entry->src.tunnel_prefix_len, NULL);
					// 送信元アドレスがいずれかのデバイスに存在しないとパケットを送信しないため、tunnelデバイスに送信元アドレスを設定する
					//mx6e_network_add_ipaddr(AF_INET6, ifindex, &entry->src.tunnel_src, entry->src.tunnel_src_prefix_len);
				}
				result = true;
			}
		}
	} else {
		mx6e_logging(LOG_INFO, "MX6E-PR config table is enough. num = %d\n", table->num);
		result = false;
	}
	
	// 排他解除
	pthread_mutex_unlock(&table->mutex);
	DEBUG_LOG("pthread_mutex_unlock  TID  = %lx\n", (unsigned long int) pthread_self());

	_D_(printf("%s:exit\n", __func__));

	return result;
}


///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR Config Table削除関数
//!
//! MX6E-PR Config Tableから指定されたエントリーを削除する。
//! ※エントリーが1個の場合、削除は失敗する。
//!
//! @param [in/out] table   削除するMX6E-PR Config Table
//! @param [in]     addr    検索に使用するv4address
//! @param [in]     cidr    検索に使用するv4netmask（CIDR形式）
//!
//! @return true        削除成功
//!         false       削除失敗
///////////////////////////////////////////////////////////////////////////////
bool m46e_pt_del_config_entry(mx6e_config_table_t * table, mx6e_config_entry_t * entry, mx6e_config_devices_t * devices)
{
	_D_(printf("%s:enter\n", __func__));

	// 引数チェック
	if ((table == NULL) || (entry == NULL)) {
		mx6e_logging(LOG_ERR, "Parameter Check NG(%s).", __func__);
		return false;
	}
	switch (table->type) {
	case CONFIG_TYPE_M46E:
		// IPv4ネットワークアドレスチェック
		if (!mx6eapp_pt_check_network_addr(&entry->src.in.m46e.v4addr, entry->src.in.m46e.v4cidr)) {
			mx6e_logging(LOG_ERR, "Parameter Check NG(This is invalid address).");
			return false;
		}
		break;
	case CONFIG_TYPE_ME6E:
		// MACアドレスチェックはしない
		break;
	default:
		mx6e_logging(LOG_ERR, "table type (%d) NG", table->type);
		return false;
	}

	_D_(m46e_pt_config_table_dump(table));

	mx6e_config_entry_t **r;
	if (false == make_config_entry(entry, table->type, devices)) {
		// 入力値から必要項目の生成に失敗
		mx6e_logging(LOG_INFO, "MX6E-PR make_config_entry fail\n");
		return false;
	} else if (NULL == (r = tfind(entry, &table->root, compfind))) {
		// 検索に失敗した場合、ログを残す
		char                            address[INET_ADDRSTRLEN];
		mx6e_logging(LOG_INFO, "Don't match MX6E-PR Table. address = %s/%d\n", inet_ntop(AF_INET, &entry->src.in.m46e.v4addr, address, sizeof(address)), entry->src.in.m46e.v4cidr);
	} else {
		if (CONFIG_TYPE_M46E == table->type) {
			int                             ifindex = get_domain_src_ifindex((*r)->domain, devices);

			// route削除
			mx6e_network_del_route(AF_INET6, ifindex, &(*r)->src.tunnel_addr, (*r)->src.tunnel_prefix_len, NULL);
			// 送信元アドレス削除
			// mx6e_network_del_ipaddr(AF_INET6, ifindex, &(*r)->src.tunnel_src, (*r)->src.tunnel_src_prefix_len);

		} else if (CONFIG_TYPE_ME6E == table->type) {
			int                             ifindex = get_domain_src_ifindex((*r)->domain, devices);

			// route削除
			mx6e_network_del_route(AF_INET6, ifindex, &(*r)->src.tunnel_addr, (*r)->src.tunnel_prefix_len, NULL);
			// 送信元アドレス削除
			// mx6e_network_del_ipaddr(AF_INET6, ifindex, &(*r)->src.tunnel_src, (*r)->src.tunnel_src_prefix_len);

		}
		if (tdelete(entry, &table->root, compfind)) {
			// 削除成功したので要素数のデクリメント
			table->num--;
		};
	}

	_D_(printf("%s:exit\n", __func__));
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR Table 有効/無効設定関数
//!
//! MX6E-PR Tableから指定されたエントリーの有効/無効を設定する。
//!
//! @param [in/out] table   設定するMX6E-PR Table
//! @param [in]     addr    検索に使用するv4address
//! @param [in]     cidr    検索に使用するv4netmask（CIDR形式）
//! @param [in]     enable  有効/無効
//!
//! @return true        設定成功
//!         false       設定失敗
///////////////////////////////////////////////////////////////////////////////
bool m46e_pt_enable_config_entry(mx6e_config_table_t * table, mx6e_config_entry_t * entry, mx6e_config_devices_t * devices)
{
	_D_(printf("%s:enter\n", __func__));

	// 引数チェック
	if ((table == NULL) || (entry == NULL)) {
		mx6e_logging(LOG_ERR, "Parameter Check NG(", __func__, ").");
		return false;
	}
	switch (table->type) {
	case CONFIG_TYPE_M46E:
		// IPv4ネットワークアドレスチェック
		if (!mx6eapp_pt_check_network_addr(&entry->src.in.m46e.v4addr, entry->src.in.m46e.v4cidr)) {
			mx6e_logging(LOG_ERR, "Parameter Check NG(This is invalid address).");
			return false;
		}
		break;
	case CONFIG_TYPE_ME6E:
		// MACアドレスチェックはしない
		break;
	default:
		mx6e_logging(LOG_ERR, "table type (%d) NG", table->type);
		return false;
	}

	mx6e_config_entry_t            *found = mx6e_search_config_table(table, entry, devices);
	if (found) {
		// 発見

		_D_(printf("%s:FOUND\n", __func__));
		_D_(m46e_pt_config_entry_dump(found));

		if (found->enable != entry->enable) {
			// 既存の設定と異なる場合のみ

			// PRテーブルへの登録が成功した場合、活性化を伴う追加要求の場合は
			// 当該PR entryのIPネットワーク経路を変更
			int                             ifindex = get_domain_src_ifindex(found->domain, devices);

			if (entry->enable) {
				// route追加(IPアドレスを追加すると、OSがパケットを処理してしまうので、routeだけ追加する)
				mx6e_network_add_route(AF_INET6, ifindex, &found->src.tunnel_addr, found->src.tunnel_prefix_len, NULL);
				// 送信元アドレスがいずれかのデバイスに存在しないとパケットを送信しないため、tunnelデバイスに送信元アドレスを設定する
				// mx6e_network_add_ipaddr(AF_INET6, ifindex, &found->src.tunnel_src, found->src.tunnel_src_prefix_len);
			} else {
				// route削除
				mx6e_network_del_route(AF_INET6, ifindex, &found->src.tunnel_addr, found->src.tunnel_prefix_len, NULL);
				// 送信元アドレス削除
				// mx6e_network_del_ipaddr(AF_INET6, ifindex, &found->src.tunnel_src, found->src.tunnel_src_prefix_len);
			}

		}
		// 一致したエントリーの有効/無効フラグを上書き
		found->enable = entry->enable;

	} else {
		char                            address[INET_ADDRSTRLEN];
		mx6e_logging(LOG_INFO, "Don't match MX6E-PR Table. address = %s/%d\n", inet_ntop(AF_INET, &entry->src.in.m46e.v4addr, address, sizeof(address)), entry->src.in.m46e.v4cidr);
		return false;
	}

	_D_(printf("%s:exit\n", __func__));
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR拡張 MX6E-PR Config テーブル検索関数
//!
//! 送信先v6アドレスとv6cidr, plane_id_inから、同一エントリーを検索する。
//!
//! @param [in] table   検索するMX6E-PR Configテーブル
//! @param [in] entry    検索するエントリ
//! @param [in] cidr    検索するv4cidr
//! @param [in] cidr    検索するplane_id_in
//!
//! @return m46e_pt_config_entry_tアドレス 検索成功
//!                                         (マッチした MX6E-PR Config Entry情報)
//! @return NULL                            検索失敗
///////////////////////////////////////////////////////////////////////////////
mx6e_config_entry_t            *mx6e_search_config_table(mx6e_config_table_t * table, mx6e_config_entry_t * entry, mx6e_config_devices_t * devices)
{
	mx6e_config_entry_t            **result = NULL;

	// 引数チェック
	if ((table == NULL) || (entry == NULL)) {
		mx6e_logging(LOG_ERR, "Parameter Check NG(%s).", __func__);
		return NULL;
	}

	// // 排他開始は呼び出し元で行っているので、ここでは不要
	// DEBUG_LOG("pthread_mutex_lock  TID  = %lx\n", (unsigned long int) pthread_self());
	// pthread_mutex_lock(&table->mutex);

	_D_(printf("-table-\n"));
	_D_(m46e_pt_config_table_dump(table));
	_D_(printf("-entry-\n"));
	_D_(m46e_pt_config_entry_dump(entry));

	if (false == make_config_entry(entry, table->type, devices)) {
		// 入力値から必要項目の生成に失敗
		mx6e_logging(LOG_INFO, "MX6E-PR make_config_entry fail\n");
		result = false;
	} else if (NULL == (result = tfind(entry, &table->root, compins))) {
		DEBUG_LOG("M46E-CONFIG table has no match in (%d) entries\n", table->num);
	}

	// // 排他解除は呼び出し元で行っているので、ここでは不要
	// pthread_mutex_unlock(&table->mutex);
	// DEBUG_LOG("pthread_mutex_unlock  TID  = %lx\n", (unsigned long int) pthread_self());

	_D_({
			if (result) {
				char                            address[INET_ADDRSTRLEN];
				printf("Match MX6E-PR Table address = %s/%d\n", inet_ntop(AF_INET, &(*result)->src.in.m46e.v4addr, address, sizeof(address)), (*result)->src.in.m46e.v4cidr);
			}
		});

	_D_(printf("exit %s\n", __func__));
	if (result) {
		return *result;
	} else {
		return NULL;
	}

}


///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR拡張 MX6E-PR Config テーブル検索関数
//!
//! 送信先v6アドレスからエントリを検索する。(置換情報取得)
//!
//! @param [in] table   検索するMX6E-PR Configテーブル
//! @param [in] v6addr  検索するV6addr
//!
//! @return m46e_pt_config_entry_tアドレス 検索成功
//!                                         (マッチした MX6E-PR Config Entry情報)
//! @return NULL                            検索失敗
///////////////////////////////////////////////////////////////////////////////
mx6e_config_entry_t            *mx6e_match_config_table(domain_t domain, mx6e_config_table_t * table, struct in6_addr * v6addr)
{
	mx6e_config_entry_t            **r = NULL;
	mx6e_config_entry_t				key = {0};

	// 引数チェック
	// 高速化のため省略
	// if ((table == NULL) || (v6addr == NULL)) {
	// 	mx6e_logging(LOG_ERR, "Parameter Check NG(%s).", __func__);
	// 	return NULL;
	// }

	// // 排他開始は呼び出し元で行っているので、ここでは不要
	// DEBUG_LOG("pthread_mutex_lock  TID  = %lx\n", (unsigned long int) pthread_self());
	// pthread_mutex_lock(&table->mutex);

	key.src.mask = in6_addr0;
	key.src.src = *v6addr;
	key.domain = domain;

	if (NULL == (r = tfind(&key, &table->root, compfind))) {
		DEBUG_LOG("M46E-CONFIG table has no match in (%d) entries\n", table->num);
	}
		
	// // 排他解除は呼び出し元で行っているので、ここでは不要
	// pthread_mutex_unlock(&table->mutex);
	// DEBUG_LOG("pthread_mutex_unlock  TID  = %lx\n", (unsigned long int) pthread_self());

	_D_({
			if (r) {
				printf("Match\n");
				m46e_pt_config_entry_dump(*r);
			}
			// 	char                            address[INET6_ADDRSTRLEN];
			// 	printf("Match MX6E-PR Table address key = %s, ", inet_ntop(AF_INET6, &key.src.src, address, sizeof(address)));
			// 	printf("%s", inet_ntop(AF_INET6, &r->src.src, address, sizeof(address)));
			// 	printf("/%s\n", inet_ntop(AF_INET6, &r->src.mask, address, sizeof(address)));
			// }
		});

	if (r) {
		if ((*r)->enable) {
			// 有効なエントリならエントリポインタを返す
			_D_(printf("exit enabled %s\n", __func__));
			return *r;
		} else {
			// 無効なエントリならNULLを返す
			_D_(printf("exit disabled %s\n", __func__));
			return NULL;
		}
	} else {
		// 見つからなかった
		_D_(printf("exit not found %s\n", __func__));
		return NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR拡張 MX6E-PR アドレス置換
//! アドレス置換
//!
//! @param [in]     entry   変換情報
//! @param [in/out] ip6     変換するIPv6ヘッダ
//!
//! @return true  変換成功
//! @return false 変換失敗
///////////////////////////////////////////////////////////////////////////////
bool mx6e_replace_address(mx6e_config_entry_t * entry, struct ip6_hdr * ip6)
{
	_D_(printf("enter %s\n", __func__));

	// 引数チェックは遅くなるので省略
	// if ((entry == NULL) || (ip6 == NULL)) {
	//  mx6e_logging(LOG_ERR, "Parameter Check NG(%s).", __func__);
	//  return NULL;
	// }

	// *INDENT-OFF*
	_D_( {
		char addr[INET6_ADDRSTRLEN];
		DEBUG_LOG("ip6.src        : %s\n", inet_ntop(AF_INET6, &ip6->ip6_src, addr, sizeof(addr)));
		DEBUG_LOG("ip6.dst        : %s\n", inet_ntop(AF_INET6, &ip6->ip6_dst, addr, sizeof(addr)));
		}
	);
	// *INDENT-ON*

	// 送信先アドレス変換
	// ネットマスクがかかっていない部分を取得
	struct in6_addr                 s;
	s = ip6->ip6_dst;
	s.s6_addr32[0] &= (~entry->des.dst_mask.s6_addr32[0]);
	s.s6_addr32[1] &= (~entry->des.dst_mask.s6_addr32[1]);
	s.s6_addr32[2] &= (~entry->des.dst_mask.s6_addr32[2]);
	s.s6_addr32[3] &= (~entry->des.dst_mask.s6_addr32[3]);

	// *INDENT-OFF*
	_D_( {
		char addr[INET6_ADDRSTRLEN];
		DEBUG_LOG("s              : %s\n", inet_ntop(AF_INET6, &s, addr, sizeof(addr)));
		}
	);
	// *INDENT-ON*

	// 変換先と変換元のネットマスクかかってない部分のORを設定
	ip6->ip6_dst.s6_addr32[0] = entry->des.dst_addr.s6_addr32[0] | s.s6_addr32[0];
	ip6->ip6_dst.s6_addr32[1] = entry->des.dst_addr.s6_addr32[1] | s.s6_addr32[1];
	ip6->ip6_dst.s6_addr32[2] = entry->des.dst_addr.s6_addr32[2] | s.s6_addr32[2];
	ip6->ip6_dst.s6_addr32[3] = entry->des.dst_addr.s6_addr32[3] | s.s6_addr32[3];

	// 送信元アドレス変換
	// ネットマスクがかかっていない部分を取得
	// Me6E FP->PR
	// ip6->ip6_src: 2001: db8:  46:   0:   1:xxxx:xxxx:xxxx
	// des.src_mask: ffff:ffff:ffff:ffff:ffff:0000:0000:0000
	//            s: 0000:0000:0000:0000:0000:xxxx:xxxx:xxxx
	s = ip6->ip6_src;
	s.s6_addr32[0] &= (~entry->des.src_mask.s6_addr32[0]);
	s.s6_addr32[1] &= (~entry->des.src_mask.s6_addr32[1]);
	s.s6_addr32[2] &= (~entry->des.src_mask.s6_addr32[2]);
	s.s6_addr32[3] &= (~entry->des.src_mask.s6_addr32[3]);

	// 変換先と変換元のネットマスクかかってない部分のORを設定
	// Me6E FP->PR
	// des.src_addr: 2001: db8:   2:   0:   1:0000:0000:0000
	//            s: 0000:0000:0000:0000:0000:xxxx:xxxx:xxxx
	// ip6->ip6_src: 2001: db8:   2:   0:   1:xxxx:xxxx:xxxx
	ip6->ip6_src.s6_addr32[0] = entry->des.src_addr.s6_addr32[0] | s.s6_addr32[0];
	ip6->ip6_src.s6_addr32[1] = entry->des.src_addr.s6_addr32[1] | s.s6_addr32[1];
	ip6->ip6_src.s6_addr32[2] = entry->des.src_addr.s6_addr32[2] | s.s6_addr32[2];
	ip6->ip6_src.s6_addr32[3] = entry->des.src_addr.s6_addr32[3] | s.s6_addr32[3];

	// *INDENT-OFF*
	_D_( {
		char addr[INET6_ADDRSTRLEN];
		DEBUG_LOG("ip6.src            : %s\n", inet_ntop(AF_INET6, &ip6->ip6_src, addr, sizeof(addr)));
		DEBUG_LOG("ip6.dst            : %s\n", inet_ntop(AF_INET6, &ip6->ip6_dst, addr, sizeof(addr)));
		DEBUG_LOG("entry.src.src      : %s\n", inet_ntop(AF_INET6, &entry->src.src, addr, sizeof(addr)));
		DEBUG_LOG("entry.src.mask     : %s\n", inet_ntop(AF_INET6, &entry->src.mask, addr, sizeof(addr)));
		DEBUG_LOG("entry.des.dst_addr : %s\n", inet_ntop(AF_INET6, &entry->des.dst_addr, addr, sizeof(addr)));
		DEBUG_LOG("entry.des.dst_mask : %s\n", inet_ntop(AF_INET6, &entry->des.dst_mask, addr, sizeof(addr)));
		}
	);
	// *INDENT-ON*


	_D_(printf("exit %s\n", __func__));
	return true;

}


///////////////////////////////////////////////////////////////////////////////
//! @brief MX6E-PR prefix+PlaneID生成関数
//!
//! MX6E-PR prefix と PlaneID の値を元に
//! MX6E-PR prefix + PlaneID のアドレスを生成する。
//!
//! @param [in]     inaddr      MX6E-PR prefix
//! @param [in]     cidr        MX6E-PR prefix長
//! @param [in]     process_name    process名(NULL許容)
//! @param [out]    outaddr     MX6E-PR prefix+PlaneID 
//!
//! @retval true    正常終了
//! @retval false   異常終了
///////////////////////////////////////////////////////////////////////////////
bool m46e_pt_plane_prefix(struct in6_addr * inaddr, int cidr, char *process_name, struct in6_addr * outaddr)
{
	uint8_t                        *src_addr;
	uint8_t                        *dst_addr;
	char                            address[INET6_ADDRSTRLEN] = { 0 };

	// 引数チェック
	if ((inaddr == NULL) || (process_name == NULL) || (outaddr == NULL)) {
		mx6e_logging(LOG_ERR, "Parameter Check NG(%s).", __func__);
		return false;
	}

	if (process_name != NULL) {
		// Process Nameが指定されている場合は、Process Nameで初期化する。
		strcat(address, "::");
		strcat(address, process_name);
		strcat(address, ":0:0");

		// 値の妥当性は設定読み込み時にチェックしているので戻り値は見ない
		inet_pton(AF_INET6, address, outaddr);
	} else {
		// Process Nameが指定されていない場合はALL0で初期化する
		*outaddr = in6addr_any;
	}

	// prefix length分コピーする
	src_addr = inaddr->s6_addr;
	dst_addr = outaddr->s6_addr;
	for (int i = 0; (i < 16 && cidr > 0); i++, cidr -= CHAR_BIT) {
		if (cidr >= CHAR_BIT) {
			dst_addr[i] = src_addr[i];
		} else {
			dst_addr[i] = (src_addr[i] & (0xff << cidr)) | (dst_addr[i] & ~(0xff << cidr));
			break;
		}
	}

	DEBUG_LOG("pr_prefix + Process Name = %s\n", inet_ntop(AF_INET6, outaddr, address, sizeof(address)));

	return true;
}

#if 0
///////////////////////////////////////////////////////////////////////////////
//! @brief SA46-T PR Prefixチェック処理関数(Fp側)
//!
//! Fp側から受信したパケットの送信元アドレスに対して、
//! MX6E-PR prefix + Process Name +IPv4 networkアドレス部分をチェックする。
//! 本関数内でMX6E-PR Tableへアクセスするための排他の獲得と解放を行う。
//!
//! @param [in]     table       検索するMX6E-PRテーブル
//! @param [in]     addr        送信元アドレス
//!
//! @retval true  チェックOK(自plane内パケット)
//! @retval false チェックNG(自plane外パケット)
///////////////////////////////////////////////////////////////////////////////
bool m46e_pt_prefix_check(mx6e_config_table_t * table, struct in6_addr * addr)
{
	bool                            ret = false;
	struct in_addr                  network;

	// 引数チェック
	if ((table == NULL) || (addr == NULL)) {
		mx6e_logging(LOG_ERR, "Parameter Check NG(%s).", __func__);
		return false;
	}

	if (table->num > 0) {
		// 排他開始
		DEBUG_LOG("pthread_mutex_lock  TID  = %lx\n", (unsigned long int) pthread_self());
		pthread_mutex_lock(&table->mutex);

		mx6e_list                      *iter;
		mx6e_list_for_each(iter, &table->entry_list) {
			mx6e_config_entry_t            *tmp = iter->data;

			// MX6E-PR prefix + Process Name判定
			// TODO: must ipv6形式のplane_idと比較
			if (IS_EQUAL_MX6E_PR_PREFIX(addr, tmp->src.plane_id)) {
				network.s_addr = addr->s6_addr32[3] & tmp->src.in.m46e.v4mask.s_addr;

				// 後半32bitアドレス（IPv4アドレス）のネットワークアドレス判定
				if (network.s_addr == tmp->src.in.m46e.v4addr.s_addr) {
					ret = true;

					char                            address[INET6_ADDRSTRLEN];
					// TODO: chk
					DEBUG_LOG("Match MX6E-PR Table address = %s\n", inet_ntop(AF_INET6, &tmp->src.in.m46e.v4addr, address, sizeof(address)));
					DEBUG_LOG("dist address = %s\n", inet_ntop(AF_INET6, addr, address, sizeof(address)));

					break;
				}

			}
		}

		// 排他解除
		pthread_mutex_unlock(&table->mutex);
		DEBUG_LOG("pthread_mutex_unlock  TID  = %lx\n", (unsigned long int) pthread_self());

	} else {
		DEBUG_LOG("MX6E-PR table is no-entry. num = %d\n", table->num);
		return false;
	}

	return ret;
}
#endif

// タイトル行定義
#define	CL(__s)	__s, sizeof(__s) - 1

static struct {
	const char                     *note;
	const int                       len;
} header[] = {
	{
		CL("*")}, {
		CL("domain")}, {
		CL("section device IPv6 Network Address    ")}, {
		CL("Netmask")}, {
		CL("plane_id(in)")}, {
		CL("prefix_len")}, {
		CL("IPv4/MAC Address ")}, {
		CL("Netmask")}, {
		CL("")}, {
		CL("plane_id(out)")}, {
		CL("IPv6 Network Address                   ")}, {
		CL("Netmask")}, {
		NULL, 0}
};

static mx6e_config_table_t *mytable;
static int myfd;
static void action(const void *nodep, const VISIT which, const int depth)
{
	mx6e_config_entry_t *p = *(mx6e_config_entry_t **)nodep;
	int                             n = 0;
	char                            v4addr[INET_ADDRSTRLEN] = { 0 };
	char                            v6addr[INET6_ADDRSTRLEN] = { 0 };

	if (!p) {
		return;
	}
	
	switch(which) {
	case preorder:
	case endorder:
		break;
	case postorder:
	case leaf:
		if (p->enable == true) {
			dprintf(myfd, "|%*s|", header[n].len, "*");
		} else {
			dprintf(myfd, "|%*s|", header[n].len, " ");
		}
		n++;

		// domain type
		dprintf(myfd, "%-*s|", header[n].len, get_domain_name(p->domain));
		n++;

		/// マルチプレーン対応 2016/07/27 add start
		// IPv6
		dprintf(myfd, "%-*s|", header[n].len, inet_ntop(AF_INET6, &p->section_dev_addr, v6addr, sizeof(v6addr)));
		n++;

		// IPv6 cidr
		dprintf(myfd, "%-*d|", header[n].len, p->section_dev_prefix_len);
		n++;
		/// マルチプレーン対応 2016/07/27 add end

		// plane_id(in)
		dprintf(myfd, "%-*s|", header[n].len, p->src.plane_id);
		n++;

		// prefix_len
		dprintf(myfd, "%-*d|", header[n].len, p->src.prefix_len);
		n++;

		switch (mytable->type) {
		case CONFIG_TYPE_M46E:
			// v4 addr
			dprintf(myfd, "%-*s|", header[n].len, inet_ntop(AF_INET, &p->src.in.m46e.v4addr, v4addr, sizeof(v4addr)));
			n++;
			// v4 cidr
			dprintf(myfd, "%-*d|", header[n].len, p->src.in.m46e.v4cidr);
			n++;
			break;

		case CONFIG_TYPE_ME6E:
			// mac address
			dprintf(myfd, "%-*s|", header[n].len, ether_ntoa(&p->src.in.me6e.hwaddr));
			n++;
			dprintf(myfd, "%-*s|", header[n].len, "-");
			n++;
			break;

		default:
			dprintf(myfd, "%-*s|", header[n].len, "-");
			n++;
			dprintf(myfd, "%-*s|", header[n].len, "-");
			n++;
			break;
		}

		// IN/OUTセパレータ
		dprintf(myfd, "%-*s|", header[n].len, "");
		n++;

		// plane_id(out)
		dprintf(myfd, "%-*s|", header[n].len, p->des.plane_id);
		n++;

		// IPv6
		dprintf(myfd, "%-*s|", header[n].len, inet_ntop(AF_INET6, &p->des.prefix, v6addr, sizeof(v6addr)));
		n++;

		// IPv6 cidr
		dprintf(myfd, "%-*d|\n", header[n].len, p->des.prefix_len);
		n++;
		break;
	}
}


///////////////////////////////////////////////////////////////////////////////
////! @brief テーブル内部情報出力関数
////!
////! MX6E-PR情報管理内のテーブルを出力する
////!
////! @param [in]     pr_handler      MX6E-PR情報管理
////! @param [in]     fd              出力先のディスクリプタ
////!
////! @return なし
/////////////////////////////////////////////////////////////////////////////////
void m46e_pt_show_entry_pr_table(mx6e_config_table_t * table, int fd, char *process_name)
{

	// ローカル変数初期化
	char                            buf[1024];

	// 引数チェック
	if ((table == NULL) || (process_name == NULL)) {
		return;
	}
	// テーブルロック
	pthread_mutex_lock(&table->mutex);

	int                             width = 0;
	width++;										// 最初の"|"分
	for (int i = 0; header[i].note; i++) {
		width += header[i].len;
		width++;									// "|"分
	}

	// +--------...--------+作成
	char                            bar1[width + 2];
	memset(bar1, '\0', sizeof(bar1));
	memset(bar1, '-', width - 1);
	bar1[0] = '+';
	bar1[width - 1] = '+';
	dprintf(fd, "%s\n", bar1);

	// タイトル
	snprintf(buf, sizeof(buf), " %s-PT Prefix Resolution Table", get_table_name(table->type));
	dprintf(fd, "|%-*s|\n", width - 2, buf);

	// +---+-----...--+------+作成
	char                            bar2[width + 2];
	memset(bar2, '\0', sizeof(bar2));
	memset(bar2, '-', width - 1);
	bar2[0] = '+';
	int                             n = 0;
	for (int i = 0; header[i].note; i++) {
		n += header[i].len + 1;
		bar2[n] = '+';
	}
	dprintf(fd, "%s\n", bar2);

	// |*|domain|pl... 作成
	char                            bar3[width + 2];
	memset(bar3, '\0', sizeof(bar3));
	bar3[0] = '|';
	for (int i = 0; header[i].note; i++) {
		strcat(bar3, header[i].note);
		strcat(bar3, "|");
	}
	dprintf(fd, "%s\n", bar3);
	dprintf(fd, "%s\n", bar2);


	mytable = table;
	myfd = fd;

	if (table && table->num) {
		_D_(m46e_pt_config_table_dump(table));
		
		twalk(table->root, action);

		dprintf(fd, "%s\n", bar2);
		dprintf(fd, "  Note : [*] shows available entry for prefix resolution process.\n");
		dprintf(fd, "\n");
	}
	// ロック解除
	pthread_mutex_unlock(&table->mutex);

	return;
}


///////////////////////////////////////////////////////////////////////////////
////! @brief MX6E-PRモードエラー出力関数
////!
////! 引数で指定されたファイルディスクリプタにエラーを出力する。
////!
////! @param [in] fd           出力先のファイルディスクリプタ
////! @param [in] error_code   エラーコード
////!
////! @return なし
/////////////////////////////////////////////////////////////////////////////////
void m46e_pt_print_error(int fd, mx6e_pr_command_error_code_t error_code)
{
	// ローカル変数宣言

	// ローカル変数初期化

	switch (error_code) {
	case MX6E_PT_COMMAND_MODE_ERROR:
		dprintf(fd, "\n");
		dprintf(fd, "Requested command is available for MX6E-PR mode only!\n");
		dprintf(fd, "\n");

		break;

	case MX6E_PT_COMMAND_EXEC_FAILURE:
		dprintf(fd, "\n");
		dprintf(fd, "Sorry! Fail to execute your requested command. \n");
		dprintf(fd, "\n");

		break;

	case MX6E_PT_COMMAND_ENTRY_FOUND:
		dprintf(fd, "\n");
		dprintf(fd, "Requested entry is already exist. \n");
		dprintf(fd, "\n");

		break;

	case MX6E_PT_COMMAND_ENTRY_NOTFOUND:
		dprintf(fd, "\n");
		dprintf(fd, "Requested entry is not exist. \n");
		dprintf(fd, "\n");

		break;

	default:
		// ありえないルート

		break;

	}

	return;
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
//! @brief M46E-PR Entry全削除関数
//!
//! M46E-PR TableからM46E-PR Entryを全て削除する。
//!
//! @param [in]     handler    M46Eハンドラ
//! @param [in]     req        コマンド要求データ
//!
//! @return true        OK(削除成功)
//!         false       NG(削除失敗)
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool m46e_pt_delall_config_entry(mx6e_handler_t * handler, mx6e_command_code_t code)
{
	mx6e_config_table_t            *table = NULL;

	_D_(printf("enter %s\n", __func__));

	// 引数チェック
	if (handler == NULL) {
		return false;
	}

	if ((code != MX6E_DELALL_M46E_ENTRY) && (code != MX6E_DELALL_ME6E_ENTRY)) {
		return false;
	}
	if (code == MX6E_DELALL_M46E_ENTRY) {
		table = &handler->conf.m46e_conf_table;
	} else if (code == MX6E_DELALL_ME6E_ENTRY) {
		table = &handler->conf.me6e_conf_table;
	}
	// 排他開始
	pthread_mutex_lock(&table->mutex);

	_D_(m46e_pt_config_table_dump(table));

	mydevices = &handler->conf.devices;
	tdestroy(table->root, tdaction);

	table->root = NULL;
	table->num = 0;
	
	// 排他解除
	pthread_mutex_unlock(&table->mutex);

	_D_(printf("exit %s\n", __func__));

	return true;
}
