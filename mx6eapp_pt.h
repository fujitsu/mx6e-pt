/******************************************************************************/
/* ファイル名 : mx6eapp_pt.h                                                  */
/* 機能概要   : MX6E Prefix Resolutionヘッダファイル                          */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#ifndef __MX6EAPP_PR_H__
#   define __MX6EAPP_PR_H__

#   include <stdbool.h>
#   include <netinet/ip.h>
#   include <netinet/ip6.h>
#   include <arpa/inet.h>
#   include <pthread.h>


#   include "mx6eapp.h"
#   include "mx6eapp_config.h"
#	include "mx6eapp_command_data.h"

//! MX6E PT Table 最大エントリー数(m46e と me6e別々に)
#   define PT_MAX_ENTRY_NUM    4096

//! CIDR2(プレフィックス)をサブネットマスク(xxx.xxx.xxx.xxx)へ変換
#   define PR_CIDR2SUBNETMASK(cidr, mask) mask.s_addr = (cidr == 0 ? 0 : htonl(0xFFFFFFFF << (32 - cidr)))

// MX6E prefix + PlaneID判定
#   define IS_EQUAL_MX6E_PREFIX(a, b) \
        (((__const uint32_t *) (a))[0] == ((__const uint32_t *) (b))[0]     \
         && ((__const uint32_t *) (a))[1] == ((__const uint32_t *) (b))[1]  \
         && ((__const uint32_t *) (a))[2] == ((__const uint32_t *) (b))[2])

// MX6E-AS prefix + PlaneID判定
#   define IS_EQUAL_MX6E_AS_PREFIX(a, b) \
        (((__const uint32_t *) (a))[0] == ((__const uint32_t *) (b))[0]     \
         && ((__const uint32_t *) (a))[1] == ((__const uint32_t *) (b))[1]  \
         && ((__const uint16_t *) (a))[4] == ((__const uint16_t *) (b))[4])

// MX6E-PR prefix + PlaneID判定
#   define IS_EQUAL_MX6E_PR_PREFIX(a, b) IS_EQUAL_MX6E_PREFIX(a, b)

//MX6E-PRコマンドエラーコード
typedef enum {
	MX6E_PT_COMMAND_NONE,
	MX6E_PT_COMMAND_MODE_ERROR,						///<動作モードエラー
	MX6E_PT_COMMAND_EXEC_FAILURE,					///<コマンド実行エラー
	MX6E_PT_COMMAND_ENTRY_FOUND,					///<エントリ登録有り
	MX6E_PT_COMMAND_ENTRY_NOTFOUND,					///<エントリ登録無し
	MX6E_PT_COMMAND_MAX
} mx6e_pr_command_error_code_t;

///////////////////////////////////////////////////////////////////////////////
// 外部関数プロトタイプ
///////////////////////////////////////////////////////////////////////////////
bool                            m46e_pt_add_config_entry(mx6e_config_table_t * table, mx6e_config_entry_t * entry, mx6e_config_devices_t * devices);
bool                            m46e_pt_del_config_entry(mx6e_config_table_t * table, mx6e_config_entry_t * entry, mx6e_config_devices_t * devices);
mx6e_config_entry_t            *mx6e_search_config_table(mx6e_config_table_t * table, mx6e_config_entry_t * entry, mx6e_config_devices_t * devices);
mx6e_config_entry_t            *mx6e_match_config_table(domain_t domain, mx6e_config_table_t * table, struct in6_addr *v6addr);
bool                            mx6e_replace_address(mx6e_config_entry_t * entry, struct ip6_hdr *ip6);

void                            m46e_pt_config_entry_dump(const mx6e_config_entry_t * entry);
void                            m46e_pt_config_table_dump(const mx6e_config_table_t * table);

bool                            m46e_pt_delall_config_entry(mx6e_handler_t * handler, mx6e_command_code_t code);
bool                            m46e_pt_enable_config_entry(mx6e_config_table_t * table, mx6e_config_entry_t * entry, mx6e_config_devices_t * devices);

void                            m46e_pt_print_error(int fd, mx6e_pr_command_error_code_t error_code);

bool                            mx6eapp_pt_convert_network_addr(struct in_addr *inaddr, int cidr, struct in_addr *outaddr);
bool                            mx6eapp_pt_check_network_addr(struct in_addr *addr, int cidr);
void                            m46e_pt_show_entry_pr_table(mx6e_config_table_t * pr_handler, int fd, char *plane_id);

#endif												// __MX6EAPP_PR_H__
