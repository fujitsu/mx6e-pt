/******************************************************************************/
/* ファイル名 : mx6eapp_socket.c                                              */
/* 機能概要   : ソケット送受信クラス ソースファイル                           */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "mx6eapp_socket.h"
#include "mx6eapp_log.h"

///////////////////////////////////////////////////////////////////////////////
//! @brief データ送信関数
//!
//! 引数で指定したコマンドコードとデータをソケット経由で送信する。
//! 引数にファイルディスクリプタ(fd)が指定されていた場合は、制御メッセージとして
//! 当該ディスクリプタも転送する。
//!
//! @param [in] sockfd   データを送信するソケットのディスクリプタ
//! @param [in] command  送信するコマンドコード
//! @param [in] data     送信するデータ
//! @param [in] size     送信するデータ長
//! @param [in] fd       転送するファイルディスクリプタ
//!                      (転送不要の場合は-1を指定する)
//!
//! @retval  0以上  送信バイト数
//! @retval  0未満  エラーコード(-errno)
///////////////////////////////////////////////////////////////////////////////
int mx6e_socket_send(int sockfd, mx6e_command_code_t command, void *data, size_t size, int fd)
{
	struct msghdr                   msg = { 0 };
	struct iovec                    iov[2];
	struct cmsghdr                 *cmsg;
	char                            cmsgbuf[CMSG_SPACE(sizeof(int))];

	DEBUG_LOG("%s\n", __func__);

	// ローカル変数初期化
	iov[0].iov_base = &command;
	iov[0].iov_len = sizeof(command);
	if (data != NULL) {
		iov[1].iov_base = data;
		iov[1].iov_len = size;
	} else {
		iov[1].iov_base = NULL;
		iov[1].iov_len = 0;
	}

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;

	if (fd >= 0) {
		msg.msg_control = cmsgbuf;
		msg.msg_controllen = sizeof(cmsgbuf);
		cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		memcpy(CMSG_DATA(cmsg), &fd, sizeof(int));
	}

	int                             ret = sendmsg(sockfd, &msg, 0);
	if (ret < 0) {
		DEBUG_LOG("send error : %s", strerror(errno));
		ret = -errno;
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief データ受信関数
//!
//! ソケットから受信したデータを引数で指定したコマンドコードとデータに格納する。
//! 制御メッセージとしてファイルディスクリプタも転送されていた場合は、
//! 引数の格納先(fd)に当該ディスクリプタも格納する。
//!
//! @param [in]  sockfd   データを受信するソケットのディスクリプタ
//! @param [out] command  受信したコマンドコードの格納先
//! @param [out] data     受信したデータの格納先
//! @param [in]  size     データ格納先のバッファサイズ
//! @param [out] fd       受信したファイルディスクリプタの格納先
//!                       (受信不要の場合はNULLを指定する)
//!
//! @retval  0以上  受信バイト数
//! @retval  0未満  エラーコード(-errno)
///////////////////////////////////////////////////////////////////////////////
int mx6e_socket_recv(int sockfd, mx6e_command_code_t * command, void *data, size_t size, int *fd)
{
	struct msghdr                   msg = { 0 };
	struct iovec                    iov[2];
	struct cmsghdr                 *cmsg;
	char                            cmsgbuf[CMSG_SPACE(sizeof(int))];

	DEBUG_LOG("%s\n", __func__);

	// ローカル変数初期化
	iov[0].iov_base = command;
	iov[0].iov_len = sizeof(int);
	if (data != NULL) {
		iov[1].iov_base = data;
		iov[1].iov_len = size;
	} else {
		iov[1].iov_base = NULL;
		iov[1].iov_len = 0;
	}

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);

	int                             ret = recvmsg(sockfd, &msg, 0);
	if (ret > 0) {
		// ファイルディスクリプタの格納
		cmsg = CMSG_FIRSTHDR(&msg);

		if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int)) && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
			int                            *val = (int *) CMSG_DATA(cmsg);
			if (fd != NULL) {
				*fd = *val;
			} else {
				// 出力先が指定されていない場合はここでcloseしておく
				close(*val);
			}
		}
	} else if (ret < 0) {
		DEBUG_LOG("recieve error : %s", strerror(errno));
		ret = -errno;
	} else {
		// なにもしない
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief データ送信関数(認証つき)
//!
//! 引数で指定したコマンドコードとデータをソケット経由で送信する。
//! 送信の際に自身のuid,gid,pidを信任状として制御メッセージに埋め込んで送信する。
//!
//! @param [in] sockfd   データを送信するソケットのディスクリプタ
//! @param [in] command  送信するコマンドコード
//! @param [in] data     送信するデータ
//! @param [in] size     送信するデータ長
//!
//! @retval  0以上  送信バイト数
//! @retval  0未満  エラーコード(-errno)
///////////////////////////////////////////////////////////////////////////////
int mx6e_socket_send_cred(int sockfd, mx6e_command_code_t command, void *data, size_t size)
{
	struct msghdr                   msg = { 0 };
	struct iovec                    iov[2];
	struct cmsghdr                 *cmsg;
	struct ucred                    cred = {
		.pid = getpid(),
		.uid = getuid(),
		.gid = getgid(),
	};
	char                            cmsgbuf[CMSG_SPACE(sizeof(cred))];

	DEBUG_LOG("%s\n", __func__);

	// ローカル変数初期化
	iov[0].iov_base = &command;
	iov[0].iov_len = sizeof(command);
	if (data != NULL) {
		iov[1].iov_base = data;
		iov[1].iov_len = size;
	} else {
		iov[1].iov_base = NULL;
		iov[1].iov_len = 0;
	}

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);

	// 信任状の格納
	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(cred));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_CREDENTIALS;
	memcpy(CMSG_DATA(cmsg), &cred, sizeof(cred));

	int                             ret = sendmsg(sockfd, &msg, 0);
	if (ret < 0) {
		DEBUG_LOG("send error : %s", strerror(errno));
		ret = -errno;
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief データ受信関数(認証つき)
//!
//! ソケットから受信したデータを引数で指定したコマンドコードとデータに格納する。
//! 制御メッセージとして信任状が含まれていた場合は、uidとgidをチェックし、
//! 自身のものと一致しない場合はエラーとする。
//!
//! @param [in]  sockfd   データを受信するソケットのディスクリプタ
//! @param [out] command  受信したコマンドコードの格納先
//! @param [out] data     受信したデータの格納先
//! @param [in]  size     データ格納先のバッファサイズ
//!
//! @retval  0以上    受信バイト数
//! @retval  0未満    エラーコード(-errno)
//! @retval  -EACCES  認証エラー
///////////////////////////////////////////////////////////////////////////////
int mx6e_socket_recv_cred(int sockfd, mx6e_command_code_t * command, void *data, size_t size)
{
	struct msghdr                   msg = { 0 };
	struct iovec                    iov[2];
	struct cmsghdr                 *cmsg;
	char                            cmsgbuf[CMSG_SPACE(sizeof(struct ucred))];

	DEBUG_LOG("%s\n", __func__);

	// ローカル変数初期化
	iov[0].iov_base = command;
	iov[0].iov_len = sizeof(int);
	if (data != NULL) {
		iov[1].iov_base = data;
		iov[1].iov_len = size;
	} else {
		iov[1].iov_base = NULL;
		iov[1].iov_len = 0;
	}

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);

	int                             ret = recvmsg(sockfd, &msg, 0);
	if (ret > 0) {
		// 信任状の格納
		cmsg = CMSG_FIRSTHDR(&msg);

		if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(struct ucred)) && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_CREDENTIALS) {
			struct ucred                   *cred = (struct ucred *) CMSG_DATA(cmsg);
			DEBUG_LOG("credential uid=%d, gid=%d, pid=%d", cred->uid, cred->gid, cred->pid);
			if (cred->uid && (cred->uid != getuid() || cred->gid != getgid())) {
				mx6e_logging(LOG_INFO, "Invalid user or group. message denied. (user=%d, group=%d)", cred->uid, cred->gid);
				ret = -EACCES;
			}
		}
	} else if (ret < 0) {
		DEBUG_LOG("recieve error : %s", strerror(errno));
		ret = -errno;
	} else {
		// なにもしない
	}

	return ret;
}
