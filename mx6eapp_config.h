/******************************************************************************/
/* ファイル名 : mx6eapp_config.h                                              */
/* 機能概要   : 設定情報管理 ヘッダファイル                                   */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#ifndef __MX6EAPP_CONFIG_H__
#   define __MX6EAPP_CONFIG_H__

#   include <stdbool.h>
#	include <stdio.h>
#   include <net/if.h>
#	include <netinet/in.h>

#   include <netinet/ether.h>

///////////////////////////////////////////////////////////////////////////////
//! 共通設定
///////////////////////////////////////////////////////////////////////////////
typedef struct {
	char                            process_name[FILENAME_MAX];	///< process識別名
	bool                            debug_log;		///< デバッグログを出力するかどうか
	bool                            daemon;			///< デーモン化するかどうか
	char                            startup_script[FILENAME_MAX];	///< スタートアップスクリプト
} mx6e_config_general_t;

///////////////////////////////////////////////////////////////////////////////
//! デバイス情報
///////////////////////////////////////////////////////////////////////////////
typedef struct {
	char                            name[IFNAMSIZ];	///< デバイス名
	struct in6_addr                 ipv6_address;	///< デバイスのプレフィックスアドレス
	int                             ipv6_netmask;	///< デバイスのプレフィックスアドレスのIPv6サブネットマスク
	struct in6_addr                 ipv6_gateway;	///< デフォルトゲートウェイアドレス
	int                             ipv6_prefixlen;	///< デバイスに設定するIPv6プレフィックス長
	struct ether_addr               hwaddr;			///< デバイスに設定するMACアドレス
	int                             ifindex;		///< デバイスのインデックス番号
	int                             fd;				///< デバイスファイルディスクリプタ

} mx6e_device_t;

///////////////////////////////////////////////////////////////////////////////
//! デバイス情報
///////////////////////////////////////////////////////////////////////////////
typedef struct {
	mx6e_device_t                   pr;				///< PR側物理デバイス設定
	mx6e_device_t                   fp;				///< FP側物理デバイス設定
	mx6e_device_t                   tunnel_pr;		///< PR側トンネルデバイス設定
	mx6e_device_t                   tunnel_fp;		///< FP側トンネルデバイス設定
	int                             send_sock_fd_pr;	///< PR側送信用ソケットFD
	int                             send_sock_fd_fp;	///< FP側送信用ソケットFD
} mx6e_config_devices_t;

typedef enum {
	DOMAIN_NONE,
	DOMAIN_FP,
	DOMAIN_PR,
	DOMAIN_BOTH,
} domain_t;

///////////////////////////////////////////////////////////////////////////////
//! M46E-PT Config 情報
///////////////////////////////////////////////////////////////////////////////
typedef struct {

	bool                            enable;			///< エントリーか有効(true)/無効(false)かを表すフラグ

	domain_t                        domain;			///< ドメインタイプ

															/// マルチプレーン対応 2016/07/27 add
	struct in6_addr					section_dev_addr;		///< セクションデバイスに設定するroute
	int								section_dev_prefix_len;	///< セクションデバイスに設定するrouteのプレフィクス長
															/// マルチプレーン対応 2016/07/27 end

	// 受信データ
	struct {
		char                            plane_id[INET6_ADDRSTRLEN];	///< plane_id
		int                             prefix_len;	///< IPv6アドレス範囲でのプレフィクス長
		union {
			struct {
				struct in_addr                  v4addr;	///< IPv4アドレス
				int                             v4cidr;	///< IPv4のCIDR
				struct in_addr                  v4mask;	///< xxx.xxx.xxx.xxx形式のIPv4サブネットマスク(内部生成)
			} m46e;
			struct {
				struct ether_addr               hwaddr;	///< MACアドレス
			} me6e;
		} in;
		struct in6_addr                 src;		///< IPv6形式 prefix + plane_id(内部生成)
		struct in6_addr                 mask;		///< IPv6形式 prefix + plane_id マスク(内部生成)

		struct in6_addr                 tunnel_addr;	///< tunnelデバイスに設定するroute
		int                             tunnel_prefix_len;	///< tunnelデバイスに設定するrouteのプレフィクス長

		struct in6_addr                 tunnel_src;		///< tunnelデバイスに設定する送信元IPv6
		int                             tunnel_src_prefix_len;	///< tunnelデバイスに設定する送信元IPv6アドレス範囲でのプレフィクス長

	} src;

	// 送信データ
	struct {
		char                            plane_id[INET6_ADDRSTRLEN];	///< plane_id
		struct in6_addr                 prefix;		///< IPv6prefixアドレス
		int                             prefix_len;	///< IPv6prefixのCIDR

		struct in6_addr                 src_addr;	///< src IPv6形式 prefix + plane_id(内部生成)
		struct in6_addr                 src_mask;	///< src IPv6形式 prefix + plane_id マスク(内部生成)

		struct in6_addr                 dst_addr;	///< dst IPv6形式 prefix + plane_id(内部生成)
		struct in6_addr                 dst_mask;	///< dst IPv6形式 prefix + plane_id マスク(内部生成)
	} des;
} mx6e_config_entry_t;

///////////////////////////////////////////////////////////////////////////////
//! テーブルタイプ
///////////////////////////////////////////////////////////////////////////////
typedef enum {
	CONFIG_TYPE_NONE,
	CONFIG_TYPE_M46E,
	CONFIG_TYPE_ME6E,
} table_type_t;

///////////////////////////////////////////////////////////////////////////////
//! MX6E-PT Config Table
///////////////////////////////////////////////////////////////////////////////
typedef struct {
	table_type_t                    type;			///< テーブルタイプ
	pthread_mutex_t                 mutex;			///< 排他用のmutex
	int                             num;			///< MX6E-PR Config Entry 数
	void						   *root;			///< MX6E-PR Config Entry list
} mx6e_config_table_t;


///////////////////////////////////////////////////////////////////////////////
//! MX6Eアプリケーション 設定
///////////////////////////////////////////////////////////////////////////////
typedef struct {
	char                            filename[FILENAME_MAX];	///< 設定ファイルのフルパス
	mx6e_config_general_t           general;		///< 共通設定
	mx6e_config_devices_t           devices;		///< デバイス設定
	mx6e_config_table_t             m46e_conf_table;	///< ME6E-PT Config Table
	mx6e_config_table_t             me6e_conf_table;	///< ME6E-PT Config Table
} mx6e_config_t;


///////////////////////////////////////////////////////////////////////////////
// 外部関数プロトタイプ
///////////////////////////////////////////////////////////////////////////////
mx6e_config_t                  *mx6e_config_load(mx6e_config_t *config, const char *filename);
void                            mx6e_config_destruct(mx6e_config_t * config);
void                            mx6e_config_dump(const mx6e_config_t * config, int fd);
void                            config_init(mx6e_config_t * config);


#endif												// __MX6EAPP_CONFIG_H__
