/******************************************************************************/
/* ファイル名 : mx6eapp_log.h                                                 */
/* 機能概要   : ログ管理 ヘッダファイル                                       */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#ifndef __MX6EAPP_LOG_H__
#   define __MX6EAPP_LOG_H__

#   include <syslog.h>
#   include <stdbool.h>
#   include <stdarg.h>

// 経路同期用デバッグマクロを追加

// デバッグログ用マクロ
// デバッグログではファイル名とライン数をメッセージの前に自動的に表示する
#   ifdef DEBUG_SYNC
#      define   DEBUG_SYNC_LOG(...) _DEBUG_SYNC_LOG(__FILE__, __LINE__, __VA_ARGS__)
#      define  _DEBUG_SYNC_LOG(FILE, LINE, ...) __DEBUG_SYNC_LOG(FILE, LINE, __VA_ARGS__)
#      define __DEBUG_SYNC_LOG(FILE, LINE, ...) mx6e_logging(LOG_DEBUG, FILE "(" #LINE ") " __VA_ARGS__)
#   else
#      define   DEBUG_SYNC_LOG(...)
#   endif

#   	ifdef	DEBUG
#   	define   DEBUG_LOG(...) _DEBUG_LOG(__FILE__, __LINE__, __VA_ARGS__)
#   	define  _DEBUG_LOG(FILE, LINE, ...) __DEBUG_LOG(FILE, LINE, __VA_ARGS__)
#   		define __DEBUG_LOG(FILE, LINE, ...) mx6e_logging(LOG_DEBUG, FILE "(" #LINE ") " __VA_ARGS__), printf(FILE "(" #LINE ") " __VA_ARGS__)
#   	else
#   	define DEBUG_LOG(...)
#endif

// デバッグ用マクロ
#ifdef DEBUG_SYNC
#   define _DS_(x) x
#else
#   define _DS_(x)
#endif

// デバッグ用マクロ
#ifdef DEBUG
#   define _D_(x) x
#else
#   define _D_(x)
#endif

///////////////////////////////////////////////////////////////////////////////
// 外部関数プロトタイプ
///////////////////////////////////////////////////////////////////////////////
void                            mx6e_initial_log(const char *name, const bool debuglog);
void                            mx6e_logging(const int priority, const char *message, ...);


#   endif													// __MX6EAPP_LOG_H__
