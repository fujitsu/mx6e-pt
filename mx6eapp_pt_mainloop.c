/******************************************************************************/
/* ファイル名 : mx6eapp_pt_mainloop.c                                         */
/* 機能概要   : PTネットワークメインループ関数 ソースファイル                 */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <sched.h>
#include <alloca.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <paths.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/signalfd.h>

#include "mx6eapp.h"
#include "mx6eapp_pt_mainloop.h"
#include "mx6eapp_log.h"
#include "mx6eapp_socket.h"
#include "mx6eapp_util.h"
#include "mx6eapp_dynamic_setting.h"
#include "mx6eapp_pt.h"
#include "mx6eapp_command_data.h"
#include "mx6eapp_setup.h"

////////////////////////////////////////////////////////////////////////////////
// 内部関数プロトタイプ宣言
////////////////////////////////////////////////////////////////////////////////
static bool                     signal_handler(int fd, mx6e_handler_t * handler);
static bool                     command_handler(int fd, mx6e_handler_t * handler);

///////////////////////////////////////////////////////////////////////////////
//! @brief PTネットワーク用のメインループ
//!
//! @param [in] handler MX6Eハンドラ
//!
//! @retval 0      正常終了
//! @retval 0以外  異常終了
///////////////////////////////////////////////////////////////////////////////
bool mx6e_pt_mainloop(mx6e_handler_t * handler)
{
	fd_set                          fds;
	int                             max_fd;
	int                             command_fd;
	char                            path[sizeof(((struct sockaddr_un *) 0)->sun_path)] = { 0 };
	char                           *offset = &path[1];

	_D_(printf("%s:%s ENTER\n", __FILE__, __func__));

	snprintf(offset, sizeof(path) - 1, MX6E_COMMAND_SOCK_NAME, handler->conf.general.process_name);

	command_fd = socket(PF_UNIX, SOCK_SEQPACKET, 0);
	if (command_fd < 0) {
		mx6e_logging(LOG_ERR, "fail to create command socket : %s\n", strerror(errno));
		return false;
	}

	struct sockaddr_un              addr;
	memset(&addr, 0, sizeof(addr));

	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, path, sizeof(addr.sun_path));

	if (bind(command_fd, (struct sockaddr *) &addr, sizeof(addr))) {
		mx6e_logging(LOG_ERR, "fail to bind command socket : %s\n", strerror(errno));
		close(command_fd);
		return false;
	}

	if (listen(command_fd, 100)) {
		mx6e_logging(LOG_ERR, "fail to listen command socket : %s\n", strerror(errno));
		close(command_fd);
		return false;
	}
	// selector用のファイディスクリプタ設定
	// (待ち受けるディスクリプタの最大値+1)
	max_fd = -1;
	max_fd = max(max_fd, command_fd);
	max_fd = max(max_fd, handler->signalfd);
	max_fd++;

	_D_(printf("PT network mainloop start max_fd:%d\n", max_fd));

	// スタートアップスクリプトをバックグラウンドで起動
	mx6e_startup_script(handler);

	// mainloop4
	while (1) {
		FD_ZERO(&fds);
		FD_SET(command_fd, &fds);
		FD_SET(handler->signalfd, &fds);

		// 受信待ち
		if (select(max_fd, &fds, NULL, NULL, NULL) < 0) {
			if (errno == EINTR) {
				mx6e_logging(LOG_INFO, "PT netowrk mainloop receive signal\n");
				continue;
			} else {
				mx6e_logging(LOG_ERR, "PT netowrk mainloop receive error : %s\n", strerror(errno));
				break;
			}
		}

		_D_(printf("recv data\n"));

		if (FD_ISSET(command_fd, &fds)) {
			DEBUG_LOG("command receive\n");
			if (!command_handler(command_fd, handler)) {
				// ハンドラの戻り値がfalseの場合はループを抜ける
				break;
			}
		}

		if (FD_ISSET(handler->signalfd, &fds)) {
			DEBUG_LOG("signal receive\n");
			if (!signal_handler(handler->signalfd, handler)) {
				// ハンドラの戻り値が0より大きい場合はループを抜ける
				break;
			}
		}
	}
	DEBUG_LOG("PT network mainloop end.\n");

	return true;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief PTネットワーク用シグナルハンドラ
//!
//! シグナル受信時に呼ばれるハンドラ。
//!
//! @param [in] fd      シグナルを受信したディスクリプタ
//! @param [in] handler MX6Eハンドラ
//!
//! @retval true   メインループを継続する
//! @retval false  メインループを継続しない
///////////////////////////////////////////////////////////////////////////////
static bool signal_handler(int fd, mx6e_handler_t * handler)
{
	struct signalfd_siginfo         siginfo;
	int                             ret;
	bool                            result;
	pid_t                           pid;

	ret = read(fd, &siginfo, sizeof(siginfo));
	if (ret < 0) {
		mx6e_logging(LOG_ERR, "failed to read signal info\n");
		return true;
	}

	if (ret != sizeof(siginfo)) {
		mx6e_logging(LOG_ERR, "unexpected siginfo size\n");
		return true;
	}

	switch (siginfo.ssi_signo) {
	case SIGCHLD:
		DEBUG_LOG("signal %d catch. waiting for child process.\n", siginfo.ssi_signo);
		do {
			pid = waitpid(-1, &ret, WNOHANG);
			DEBUG_LOG("child process end. pid=%d, status=%d\n", pid, ret);
		} while (pid > 0);

		result = true;
		break;

	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
	case SIGHUP:
		DEBUG_LOG("signal %d catch. finish process.\n", siginfo.ssi_signo);
		result = false;
		break;

	default:
		DEBUG_LOG("signal %d catch. ignore...\n", siginfo.ssi_signo);
		result = true;
		break;
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief PTネットワーク用内部コマンドハンドラ
//!
//! 親プロセス(PTネットワーク側)からの要求受信時に呼ばれるハンドラ。
//!
//! @param [in] fd      コマンドを受信したソケットのディスクリプタ
//! @param [in] handler MX6Eハンドラ
//!
//! @retval true   メインループを継続する
//! @retval false  メインループを継続しない
///////////////////////////////////////////////////////////////////////////////
static bool command_handler(int fd, mx6e_handler_t * handler)
{
	mx6e_command_t                  command;
	int                             ret;
	int                             sock;

	sock = accept(fd, NULL, 0);
	if (sock <= 0) {
		return true;
	}
	DEBUG_LOG("accept ok\n");

	if (fcntl(sock, F_SETFD, FD_CLOEXEC)) {
		mx6e_logging(LOG_ERR, "fail to set close-on-exec flag : %s\n", strerror(errno));
		close(sock);
		return true;
	}

	int                             opt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_PASSCRED, &opt, sizeof(opt))) {
		mx6e_logging(LOG_ERR, "fail to set sockopt SO_PASSCRED : %s\n", strerror(errno));
		close(sock);
		return true;
	}

	ret = mx6e_socket_recv_cred(sock, &command.code, &command.req, sizeof(command.req));
	DEBUG_LOG("command receive. code = %d(%s),ret = %d\n", command.code, get_command_name(command.code), ret);

	// コマンド応答
	switch (command.code) {
	case MX6E_ADD_M46E_ENTRY:						// PT ENTRY 追加要求
	case MX6E_ADD_ME6E_ENTRY:						// PT ENTRY 追加要求
	case MX6E_DEL_M46E_ENTRY:						// PR ENTRY 削除要求
	case MX6E_DEL_ME6E_ENTRY:						// PR ENTRY 削除要求
	case MX6E_DELALL_M46E_ENTRY:					// PR ENTRY 全削除要求
	case MX6E_DELALL_ME6E_ENTRY:					// PR ENTRY 全削除要求
	case MX6E_ENABLE_M46E_ENTRY:					// PR ENTRY 活性化要求
	case MX6E_ENABLE_ME6E_ENTRY:					// PR ENTRY 活性化要求
	case MX6E_DISABLE_M46E_ENTRY:					// PR ENTRY 非活性化要求
	case MX6E_DISABLE_ME6E_ENTRY:					// PR ENTRY 非活性化要求
	case MX6E_SHOW_M46E_ENTRY:						// PR ENTRY 表示要求
	case MX6E_SHOW_ME6E_ENTRY:						// PR ENTRY 表示要求
	case MX6E_SET_DEBUG_LOG:						// デバッグログ出力設定 要求
	case MX6E_SHOW_STATISTIC:						// 統計表示
	case MX6E_SHOW_CONF:							// 設定ファイル構造体ダンプ
	case MX6E_SHUTDOWN:
	case MX6E_RESTART:
		DEBUG_LOG("case: %s:\n", get_command_name(command.code));

		if (ret > 0) {
			command.res.result = 0;
		} else {
			command.res.result = -ret;
		}
		ret = mx6e_socket_send(sock, command.code, &command.res, sizeof(command.res), -1);
		if (ret < 0) {
			mx6e_logging(LOG_WARNING, "fail to send response to external command : %s\n", strerror(-ret));
		}
		if (command.res.result != 0) {
			// エラー終了
			close(sock);
			return false;
		}

		break;

	default:
		mx6e_logging(LOG_WARNING, "unknown command code(%d) ignore...\n", command.code);
	}

	bool                            result = false;
	mx6e_config_table_t            *table = NULL;
	mx6e_config_devices_t          *devices = NULL;

	// M46E/ME6Eテーブル選択
	switch (command.code) {
	case MX6E_ADD_M46E_ENTRY:						// PT ENTRY 追加要求
	case MX6E_DEL_M46E_ENTRY:						// PR ENTRY 削除要求
	case MX6E_DELALL_M46E_ENTRY:					// PR ENTRY 全削除要求
	case MX6E_ENABLE_M46E_ENTRY:					// PR ENTRY 活性化要求
	case MX6E_DISABLE_M46E_ENTRY:					// PR ENTRY 非活性化要求
	case MX6E_SHOW_M46E_ENTRY:						// PR ENTRY 表示要求
		table = &handler->conf.m46e_conf_table;
		devices = &handler->conf.devices;
		break;

	case MX6E_ADD_ME6E_ENTRY:						// PT ENTRY 追加要求
	case MX6E_DEL_ME6E_ENTRY:						// PR ENTRY 削除要求
	case MX6E_DELALL_ME6E_ENTRY:					// PR ENTRY 全削除要求
	case MX6E_ENABLE_ME6E_ENTRY:					// PR ENTRY 活性化要求
	case MX6E_DISABLE_ME6E_ENTRY:					// PR ENTRY 非活性化要求
	case MX6E_SHOW_ME6E_ENTRY:						// PR ENTRY 表示要求
		table = &handler->conf.me6e_conf_table;
		devices = &handler->conf.devices;
		break;
	default:
		// nothing
		break;
	}


	switch (command.code) {
	case MX6E_ADD_M46E_ENTRY:						// PT ENTRY 追加要求
	case MX6E_ADD_ME6E_ENTRY:						// PT ENTRY 追加要求
		if (!m46e_pt_add_config_entry(table, &command.req.mx6e_data.entry, devices)) {
			// エントリ登録失敗
			// ここでConsoleに要求コマンド失敗のエラーを返す。
			m46e_pt_print_error(sock, MX6E_PT_COMMAND_EXEC_FAILURE);

			mx6e_logging(LOG_ERR, "fail to add MX6E-PR Entry to MX6E-PR Table\n");
		}
		result = true;
		break;

	case MX6E_DEL_M46E_ENTRY:						// PR ENTRY 削除要求
	case MX6E_DEL_ME6E_ENTRY:						// PR ENTRY 削除要求
		if (!m46e_pt_del_config_entry(table, &command.req.mx6e_data.entry, devices)) {
			// エントリ削除失敗
			// ここでConsoleに要求コマンド失敗のエラーを返す。
			m46e_pt_print_error(sock, MX6E_PT_COMMAND_EXEC_FAILURE);

			mx6e_logging(LOG_ERR, "fail to del MX6E-PR Entry to MX6E-PR Table\n");
		}
		result = true;
		break;

	case MX6E_DELALL_M46E_ENTRY:					// PR ENTRY 全削除要求
	case MX6E_DELALL_ME6E_ENTRY:					// PR ENTRY 全削除要求
		if (!m46e_pt_delall_config_entry(handler, command.code)) {
			// エントリ削除失敗
			m46e_pt_print_error(sock, MX6E_PT_COMMAND_EXEC_FAILURE);
			mx6e_logging(LOG_ERR, "fail to del all MX6E-PR Entry to MX6E-PR Table\n");
		}
		result = true;
		break;

	case MX6E_ENABLE_M46E_ENTRY:					// PR ENTRY 活性化要求
	case MX6E_ENABLE_ME6E_ENTRY:					// PR ENTRY 活性化要求
		if (!m46e_pt_enable_config_entry(table, &command.req.mx6e_data.entry, devices)) {
			// エントリ活性化失敗
			m46e_pt_print_error(sock, MX6E_PT_COMMAND_EXEC_FAILURE);
			mx6e_logging(LOG_ERR, "fail to enable MX6E-PR Entry\n");
		}
		result = true;
		break;

	case MX6E_DISABLE_M46E_ENTRY:					// PR ENTRY 非活性化要求
	case MX6E_DISABLE_ME6E_ENTRY:					// PR ENTRY 非活性化要求
		if (!m46e_pt_enable_config_entry(table, &command.req.mx6e_data.entry, devices)) {
			// エントリ非活性化失敗
			m46e_pt_print_error(sock, MX6E_PT_COMMAND_EXEC_FAILURE);
			mx6e_logging(LOG_ERR, "fail to disable MX6E-PR Entry\n");
		}
		result = true;
		break;

	case MX6E_SHOW_M46E_ENTRY:						// PR ENTRY 表示要求
	case MX6E_SHOW_ME6E_ENTRY:						// PR ENTRY 表示要求
		m46e_pt_show_entry_pr_table(table, sock, handler->conf.general.process_name);
		result = true;
		break;

	case MX6E_SET_DEBUG_LOG:						// デバッグログ出力設定 要求
		if (mx6eapp_set_debug_log(handler, &command, sock)) {
		} else {
			mx6e_logging(LOG_ERR, "fail to set debug_log mode\n");
		}
		result = true;
		break;

	case MX6E_SHOW_CONF:							// 設定ファイル構造体ダンプ
		mx6e_config_dump(&handler->conf, sock);
		result = true;
		break;

	case MX6E_SHOW_STATISTIC:						// 統計表示
		mx6e_printf_statistics_info_normal(&handler->stat_info, sock);
		result = true;

		break;

	case MX6E_SHUTDOWN:
		// なにもしない
		break;

	default:
		// なにもしない
		mx6e_logging(LOG_WARNING, "unknown command code(%d) ignore...\n", command.code);
		result = true;
		break;
	}

	close(sock);

	return result;
}
