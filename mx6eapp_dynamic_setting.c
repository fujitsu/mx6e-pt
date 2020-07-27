/******************************************************************************/
/* ファイル名 : mx6eapp_dynamic_setting.c                                     */
/* 機能概要   : 動的定義変更 ソースファイル                                   */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <linux/if_link.h>

#include "mx6eapp.h"
#include "mx6eapp_network.h"
#include "mx6eapp_config.h"
#include "mx6eapp_log.h"
#include "mx6eapp_dynamic_setting.h"
#include "mx6eapp_pt.h"

///////////////////////////////////////////////////////////////////////////////
////! @brief デバッグログ設定コマンド関数(PR側)
////!
////! Config情報にコマンドで設定された値をセットする。
////!
////! @param [in]     handler         アプリケーションハンドラー
////! @param [in]     command         コマンド構造体
////! @param [in]     fd              出力先のディスクリプタ
////!
////! @return  true       正常
////! @return  false      異常
/////////////////////////////////////////////////////////////////////////////////
bool mx6eapp_set_debug_log(mx6e_handler_t * handler, mx6e_command_t * command, int fd)
{
	// 引数チェック
	if ((handler == NULL) || (command == NULL)) {
		mx6e_logging(LOG_ERR, "Parameter Check NG(%s).", __func__);
		return false;
	}
	// 内部変数
	mx6e_config_t                  *config = &handler->conf;
	mx6e_set_debuglog_data_t       *data = &(command->req.dlog);

	// Config情報を更新
	config->general.debug_log = data->mode;

	// 更新されたConfig情報設定値でログ情報を再初期化
	mx6e_initial_log(handler->conf.general.process_name, handler->conf.general.debug_log);

	return true;
}
