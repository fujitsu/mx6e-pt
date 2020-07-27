/******************************************************************************/
/* ファイル名 : mx6eapp_log.c                                                 */
/* 機能概要   : ログ管理 ソースファイル                                       */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/

#include "mx6eapp_log.h"

///////////////////////////////////////////////////////////////////////////////
//! @brief ログ初期化関数
//!
//! ログ出力用の初期化を行う。
//!
//! @param [in] name      ログ識別名
//! @param [in] debuglog  デバッグログ(LOG_DEBUG)を出力するかどうか。
//!                         true：出力する、false：出力しない。
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
void mx6e_initial_log(const char *name, const bool debuglog)
{
	// ローカル変数宣言
	int                             opt;

	// ローカル変数初期化
	opt = 0;

	// デバッグ時は標準エラーにもログを出力する
	_D_(opt |= LOG_PERROR);
	// デバッグ時は標準エラーにもログを出力する
	_DS_(opt |= LOG_PERROR);

	openlog(name, opt, LOG_USER);

	// 現在のマスク値を取得
	int                             mask = setlogmask(0);
	if (!debuglog) {
		// デバッグログを出力しない設定になっている場合はLOG_DEBUGをマスクする
		mx6e_logging(LOG_INFO, "turn off debug log.\n");
		mask = setlogmask(mask & ~LOG_MASK(LOG_DEBUG));
	} else {
		// デバッグログを出力する設定になっている場合はLOG_DEBUGをアンマスクする
		mask = setlogmask(mask | LOG_MASK(LOG_DEBUG));
		mx6e_logging(LOG_INFO, "turn on debug log.\n");
	}
}

////////////////////////////////////////////////////////////////////////////////
//! @brief syslogへのログ出力関数
//!
//! @param [in] priority    syslog出力用
//!      priorityとして設定できる値は以下とする
//!       - { "alert",   LOG_ALERT },
//!       - { "crit",    LOG_CRIT },
//!       - { "debug",   LOG_DEBUG },
//!       - { "emerg",   LOG_EMERG },
//!       - { "err",     LOG_ERR },
//!       - { "info",    LOG_INFO },
//!       - { "notice",  LOG_NOTICE },
//!       - { "warning", LOG_WARNING },
//!
//! @param [in] message     ログ出力文字列。
//         printfと同様、可変引数リスト
//            "文字列 + %指定"、引数、引数・・・
//
//! @return なし
////////////////////////////////////////////////////////////////////////////////
void mx6e_logging(const int priority, const char *message, ...)
{
	va_list                         list;

	va_start(list, message);
	vsyslog(priority, message, list);
	va_end(list);

	return;
}
