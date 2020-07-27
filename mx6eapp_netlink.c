/******************************************************************************/
/* ファイル名 : mx6eapp_netlink.c                                             */
/* 機能概要   : netlinkソケット送受信クラス ソースファイル                    */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#include <stdlib.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "mx6eapp_netlink.h"
#include "mx6eapp_log.h"


///////////////////////////////////////////////////////////////////////////////
//! @brief  Netlink socket OPEN.
//!
//! @param [in]    group           Bind Group
//! @param [out]   sock_fd         Socket descriptor
//! @param [out]   local           Local netlink sock address
//! @param [out]   seq             sequence number
//! @param [out]   errcd           detail error code
//!
//! @return result code
//! @retval RESULT_OK          normal end
//! @retval RESULT_SYSCALL_NG  system call error
//! @retval RESULT_NG          another error
///////////////////////////////////////////////////////////////////////////////
int mx6e_netlink_open(unsigned long group, int *sock_fd, struct sockaddr_nl *local, uint32_t * seq, int *errcd)
{
	socklen_t                       addr_len;
	int                             sysret;

	/* ------------------- */
	/* Netlink socket open */
	/* ------------------- */
	*sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	if (*sock_fd < 0) {
		if (errcd != NULL)
			*errcd = errno;

		/* LOG(ERROR) */
		mx6e_logging(LOG_ERR, "Cannot open netlink socket. errno=%d\n", errno);

		return RESULT_SYSCALL_NG;
	}

	/* ------------------- */
	/* Netlink socket bind */
	/* ------------------- */
	memset(local, 0, sizeof(*local));
	local->nl_family = AF_NETLINK;
	local->nl_pid = 0;								/* nl_pid is made to acquire it in the kernel. */
	local->nl_groups = group;

	sysret = bind(*sock_fd, (struct sockaddr *) local, sizeof(*local));

	if (sysret < 0) {
		if (errcd != NULL)
			*errcd = errno;

		/* LOG(ERROR) */
		mx6e_logging(LOG_ERR, "Cannot bind netlink socket. errno=%d\n", errno);

		close(*sock_fd);
		*sock_fd = -1;
		return RESULT_SYSCALL_NG;
	}

	/* -------------------------- */
	/* Get Netlink socket address */
	/* -------------------------- */
	addr_len = sizeof(*local);

	if (getsockname(*sock_fd, (struct sockaddr *) local, &addr_len) < 0) {
		if (errcd != NULL)
			*errcd = errno;

		/* LOG(ERROR) */
		mx6e_logging(LOG_ERR, "Cannot bind netlink getsockname. errno=%d\n", errno);

		close(*sock_fd);
		*sock_fd = -1;
		return RESULT_SYSCALL_NG;
	}

	/* sockaddr check */
	if ((addr_len != sizeof(*local)) || (local->nl_family != AF_NETLINK)) {

		/* LOG(ERROR) */
		mx6e_logging(LOG_ERR, "Getsockname wrong socket address. length=%d, nl_family=%d\n", addr_len, local->nl_family);

		close(*sock_fd);
		*sock_fd = -1;
		return RESULT_NG;
	}

	/* set sequence number */
	*seq = time(NULL);

	return RESULT_OK;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief  Netlink socket CLOSE.
//!
//! @param [in]   sock_fd      Socket descriptor
//!
//! @return none
///////////////////////////////////////////////////////////////////////////////
void mx6e_netlink_close(int sock_fd)
{
	close(sock_fd);
}

///////////////////////////////////////////////////////////////////////////////
//! @brief  Netlink message SEND to kernel.
//!
//! @param [in]    sock_fd         Socket descriptor
//! @param [in]    seq             Sequence number
//! @param [in]    nlm             Netlink message (Need "struct nlmsghdr" & "struct ndmsg")
//! @param [out]   errcd           Detail error code
//!
//! @return result code
//! @retval RESULT_OK          normal end
//! @retval RESULT_SYSCALL_NG  system call error
//! @retval RESULT_NG          another error
///////////////////////////////////////////////////////////////////////////////
int mx6e_netlink_send(int sock_fd, uint32_t seq, struct nlmsghdr *nlm, int *errcd)
{
	int                             status;
	struct sockaddr_nl              nladdr;

	/* ------------------------------ */
	/* NETLINK socket setting         */
	/* ------------------------------ */
	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = 0;								/* To kernel */
	nladdr.nl_groups = 0;							/* No multicust */

	/* ------------------------------ */
	/* Netlink message header setting */
	/* ------------------------------ */
	nlm->nlmsg_seq = seq;							/* Sequence number */

	/* ------------------------------ */
	/* Send netlink message           */
	/* ------------------------------ */
	status = sendto(sock_fd, nlm, nlm->nlmsg_len, 0, (struct sockaddr *) &nladdr, sizeof(nladdr));

	if (status < 0) {
		*errcd = errno;

		/* LOG(ERROR) */
		mx6e_logging(LOG_ERR, "Cannot send netlink message. seq=%d,type=%d,flags=%d,errno=%d\n", seq, nlm->nlmsg_type, nlm->nlmsg_flags, errno);

		return RESULT_SYSCALL_NG;
	}

	return RESULT_OK;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief  Netlink message RECV from kernel.
//!
//! @param [in]      sock_fd     Socket descriptor
//! @param [in]      local_sa    Local netlink sock address
//! @param [in]      seq         Sequence number
//! @param [out]     errcd       Detail error code
//! @param [in]      parse_func  The function for parse Netlink Message
//! @param [in,out]  data        Input parameter of parse function.
//!
//! @return result code
//! @retval RESULT_OK          normal end
//! @retval RESULT_SYSCALL_NG  system call error
//! @retval RESULT_NG          another error
///////////////////////////////////////////////////////////////////////////////
//
//  Netlink Message 詳細解析関数(parse_func) 仕様
//
//  int(*parse_func)(struct nlmsghdr* nlmsg_h, int* errcd, void* indata, void* outdata),
//
//  nlmsg_h       [In]      Netlink message 先頭アドレス
//  errcd         [Out]     エラーコード
//  data          [In/Out]  解析用データ
//
//  戻り値   処理結果
//
//  RESULT_OK          正常
//  RESULT_NG          異常
//  RESULT_SKIP_NLMSG  対象外データ
//
//  戻り値が対象外データの場合は、次のNetlink messageについて
//  再度解析関数をコールする。
//
//  解析関数内では、nlmsg_hで指定されたNetlink Messageをdataの情報を元に解析し
//  dataに解析結果を設定する処理を盛り込む。
//  なお、解析用データ(data)が不要の場合はdataにNULL設定すること
//  dataの情報は解析関数で必要な型定義と領域を確保しておくこと
//
///////////////////////////////////////////////////////////////////////////////
int mx6e_netlink_recv(int sock_fd, struct sockaddr_nl *local_sa, uint32_t seq, int *errcd, netlink_parse_func parse_func, void *data)
{
	int                             status;
	struct nlmsghdr                *nlmsg_h;
	struct sockaddr_nl              nladdr;
	socklen_t                       nladdr_len = sizeof(nladdr);
	void                           *buf;
	int                             ret;

	buf = malloc(NETLINK_RCVBUF);					/* 16kbyte */
	if (buf == NULL) {
		*errcd = errno;
		mx6e_logging(LOG_ERR, "recv buf malloc ng. errno=%d\n", errno);
		return RESULT_SYSCALL_NG;
	}

	/* でかいので初期化しない */
	/* memset(buf,0,sizeof(buf)); */

	/* ------------------------------ */
	/* Recv Netlink Message           */
	/* ------------------------------ */
	while (1) {
		status = recvfrom(sock_fd, buf, NETLINK_RCVBUF, 0, (struct sockaddr *) &nladdr, &nladdr_len);

		/* recv error */
		if (status < 0) {
			if ((errno == EINTR) || (errno == EAGAIN)) {
				/* Interrupt */
				continue;
			}

			*errcd = errno;

			/* LOG(ERROR) */
			mx6e_logging(LOG_ERR, "Recieve netlink msg error. seq=%d,errno=%d\n", seq, errno);
			free(buf);
			return RESULT_SYSCALL_NG;
		}

		/* No data */
		if (status == 0) {
			/* LOG(ERROR) */
			mx6e_logging(LOG_ERR, "EOF on netlink. seq=%d\n", seq);
			free(buf);
			return RESULT_NG;
		}

		/* Sockaddr length check */
		if (nladdr_len != sizeof(nladdr)) {
			/* LOG(ERROR) */
			mx6e_logging(LOG_ERR, "Illegal sockaddr length. length=%d,seq=%d\n", nladdr_len, seq);
			free(buf);
			return RESULT_NG;
		}

		/* ------------------------------ */
		/* Parse Netlink Message          */
		/* ------------------------------ */
		nlmsg_h = (struct nlmsghdr *) buf;
		while (NLMSG_OK(nlmsg_h, status)) {
			/* process id & sequence number check */
			if (nladdr.nl_pid != 0 ||				/* From pid is kernel? */
				nlmsg_h->nlmsg_pid != local_sa->nl_pid ||	/* To pid is ok?       */
				nlmsg_h->nlmsg_seq != seq			/* Sequence no is ok?  */
				) {
				/* That netlink message is not my expected msg. */
				/* LOG(ERROR) */
				mx6e_logging(LOG_ERR, "Unexpected netlink msg recieve. From pid=%d To pid=%d/%d seq=%d/%d\n", nladdr.nl_pid, nlmsg_h->nlmsg_pid, local_sa->nl_pid, nlmsg_h->nlmsg_seq, seq);

				nlmsg_h = NLMSG_NEXT(nlmsg_h, status);
				continue;
			}

			/* -------------------------------------------------- */
			/* Call the function of parse Netlink Message detail. */
			/* -------------------------------------------------- */
			// int mx6e_netlink_parse_ack(struct nlmsghdr *nlmsg_h, int *errcd, void *data)
			ret = parse_func(nlmsg_h, errcd, data);

			/* RESULT_SKIP_NLMSG is skip messge */
			if (ret != RESULT_SKIP_NLMSG) {
				free(buf);
				/* Finish netlink message recieve & parse */
				return ret;
			}

			/* message skip */
			nlmsg_h = NLMSG_NEXT(nlmsg_h, status);
		}

		/* Recieve message Remain? */
		if (status) {
			/* LOG(ERROR) */
			mx6e_logging(LOG_ERR, "Recieve message Remant of size. remsize=%d,seq=%d\n", status, seq);
			free(buf);
			return RESULT_NG;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//! @brief  Netlink message Send & Recv from kernel.
//!
//! @param [in]      sock_fd     Socket descriptor
//! @param [in]      local_sa    Local netlink sock address
//! @param [in]      seq         Sequence number
//! @param [in]      nlm         Netlink message (Need "struct nlmsghdr" & "struct ndmsg")
//! @param [out]     errcd       Detail error code
//!
//! @return result code
//! @retval RESULT_OK          normal end
//! @retval RESULT_SYSCALL_NG  system call error
//! @retval RESULT_NG          another error
///////////////////////////////////////////////////////////////////////////////
int mx6e_netlink_transaction(int sock_fd, struct sockaddr_nl *local_sa, uint32_t seq, struct nlmsghdr *nlm, int *errcd)
{
	int                             ret;

	// 要求送信
	ret = mx6e_netlink_send(sock_fd, seq, nlm, errcd);
	if (ret != RESULT_OK) {
		return ret;
	}
	// 応答受信
	ret = mx6e_netlink_recv(sock_fd, local_sa, seq, errcd, mx6e_netlink_parse_ack, NULL);
	if (ret != RESULT_OK) {
		return ret;
	}

	return RESULT_OK;
}

///////////////////////////////////////////////////////////////////////////////
// インライン関数
///////////////////////////////////////////////////////////////////////////////
struct rtattr                  *mx6e_nlmsg_tail(struct nlmsghdr *nmsg)
{
	return (struct rtattr *) (((void *) nmsg) + NLMSG_ALIGN(nmsg->nlmsg_len));
}


///////////////////////////////////////////////////////////////////////////////
//! @brief  Addition attribute data to netlink message.
//!
//! @param [in]    n               Netlink message address
//! @param [in]    maxlen          Netlink message buffer max length
//! @param [in]    type            Attribute type
//! @param [in]    data            Attribute payload data
//! @param [in]    alen            Attribute payload data size
//!
//! @return result code
//! @retval RESULT_OK          normal end
//! @retval RESULT_NG          another error
//!
///////////////////////////////////////////////////////////////////////////////
int mx6e_netlink_addattr_l(struct nlmsghdr *n, int maxlen, int type, const void *data, int alen)
{
	int                             len = RTA_LENGTH(alen);
	struct rtattr                  *rta;

	if (NLMSG_ALIGN(n->nlmsg_len) + len > maxlen) {
		/* Netlink message buffer is too short to add. */ ;
		mx6e_logging(LOG_ERR, " LEN ERROR\n");
		return RESULT_NG;
	}
	rta = mx6e_nlmsg_tail(n);
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), data, alen);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + len;

	return RESULT_OK;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief  多段属性の開始を宣言する
//!
//! @param [in]    n               Netlink message address
//! @param [in]    maxlen          Netlink message buffer max length
//! @param [in]    attr            Attribute type
//!
//! @return 挿入した多段属性の開始アドレス
///////////////////////////////////////////////////////////////////////////////
struct rtattr                  *mx6e_netlink_attr_begin(struct nlmsghdr *n, int maxlen, int attr)
{
	struct rtattr                  *rtattr = mx6e_nlmsg_tail(n);

	mx6e_netlink_addattr_l(n, maxlen, attr, NULL, 0);

	return rtattr;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief  多段属性の終了を宣言する
//!
//! @param [in]    n      Netlink message address
//! @param [in]    attr   終了する属性の先頭アドレス
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
void mx6e_netlink_attr_end(struct nlmsghdr *n, struct rtattr *attr)
{
	attr->rta_len = (void *) mx6e_nlmsg_tail(n) - (void *) attr;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief  Parse RT attribute data from netlink message.
//!
//! @param [out]   tb              RT attribute address arry list
//! @param [in]    max             RT attribute address arry list num
//! @param [in]    rta             RT attribute top address
//! @param [in]    len             RT attribute area total length
//!
//! @return result code
//! @retval RESULT_OK          normal end
//! @retval RESULT_NG          another error
///////////////////////////////////////////////////////////////////////////////
int mx6e_netlink_parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
	memset(tb, 0, sizeof(struct rtattr *) * (max + 1));

	// 受信したAttributes DataをAttribute type毎のポインタ配列に詰める
	while (RTA_OK(rta, len)) {
		if (rta->rta_type <= max) {
			tb[rta->rta_type] = rta;
		}
		rta = RTA_NEXT(rta, len);
	}

	// メッセージの残サイズが0以外の場合は、エラーとする
	if (len) {
		/* rta_type is over arry list max .. */
		return RESULT_NG;
	}
	return RESULT_OK;
}

///////////////////////////////////////////////////////////////////////////////
//! @brief  Netlink Message 詳細解析関数
//!
//! @param [in]      nlmsg_h       Netlink message 先頭アドレス
//! @param [out]     errcd         エラーコード
//! @param [in,out]  data          解析用データ
//!
//! @return 処理結果
//! @retval RESULT_OK          正常
//! @retval RESULT_NG          異常
//! @retval RESULT_SKIP_NLMSG  対象外データ
///////////////////////////////////////////////////////////////////////////////
int mx6e_netlink_parse_ack(struct nlmsghdr *nlmsg_h, int *errcd, void *data)
{
	struct nlmsgerr                *nl_err;			/* NLMSG_ERROR */

	/* DONE Netlink Message ? */
	if (nlmsg_h->nlmsg_type == NLMSG_DONE) {
		/* Message end */
		mx6e_logging(LOG_ERR, "DONE\n");
		return RESULT_NG;
	}

	/* ACK Netlink Message ? */
	if (nlmsg_h->nlmsg_type == NLMSG_ERROR) {

		nl_err = (struct nlmsgerr *) NLMSG_DATA(nlmsg_h);

		/* payload length check */
		if (nlmsg_h->nlmsg_len < NLMSG_LENGTH(sizeof(struct nlmsgerr))) {
			/* LOG(ERROR) */
			mx6e_logging(LOG_ERR, "Ack netlink message. payload is too short. seq=%d, length=%d", nlmsg_h->nlmsg_seq, nlmsg_h->nlmsg_len);

			/* too short */
			return RESULT_NG;
		}

		*errcd = -nl_err->error;
		if (*errcd == 0) {
			/* ACK */
			return RESULT_OK;
		} else {
			/* NACK (set System call ng) */
			mx6e_logging(LOG_ERR, "error occured while receiving the message from the kernel.\n");
			mx6e_logging(LOG_ERR, "error code %d \n ", nl_err->error);
			mx6e_logging(LOG_ERR, "\n****** %s ********\n", strerror(-1 * nl_err->error));
			mx6e_logging(LOG_ERR, "msg->nlmsg_len %d\n", nl_err->msg.nlmsg_len);
			mx6e_logging(LOG_ERR, "msg->nlmsg_type %x\n", nl_err->msg.nlmsg_type);
			mx6e_logging(LOG_ERR, "msg->nlmsg_flags %x\n", nl_err->msg.nlmsg_flags);
			mx6e_logging(LOG_ERR, "msg->nlmsg_seq: %d\n", nl_err->msg.nlmsg_seq);
			mx6e_logging(LOG_ERR, "msg->nlmsg_pid: %d\n", nl_err->msg.nlmsg_pid);

			return RESULT_SYSCALL_NG;
		}
	}

	/* Unexpected  messege */
	return RESULT_SKIP_NLMSG;
}
