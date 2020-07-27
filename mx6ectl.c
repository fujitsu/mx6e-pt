/******************************************************************************/
/* ファイル名 : mx6ectl.c                                                     */
/* 機能概要   : MX6Eアクセス用外部コマンド ソースファイル                     */
/* 修正履歴   : 2016.06.10 S.Anai 新規作成                                    */
/*                                                                            */
/* ALL RIGHTS RESERVED, COPYRIGHT(C) FUJITSU LIMITED 2016                     */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <sched.h>
#include <alloca.h>
#include <signal.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <netinet/in.h>

#include "mx6eapp_command_data.h"
#include "mx6eapp_socket.h"
#include "mx6ectl_command.h"
#include "mx6eapp_util.h"
#include "mx6eapp_log.h"

//! コマンドオプション構造体
static const struct option      options[] = {
	{"name", required_argument, 0, 'n'},
	{"help", no_argument, 0, 'h'},
	{"usage", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};

//! コマンド引数構造体定義
struct command_arg {
	char                           *main;			///< メインコマンド文字列
	char                           *sub;			///< サブコマンド文字列
	int                             code;			///< コマンドコード
};

//! コマンド引数
// *INDENT-OFF*
static const struct command_arg command_args[] = {
	{"show",		"stat",		MX6E_SHOW_STATISTIC},
	{"show",		"conf",		MX6E_SHOW_CONF},
	{"set",			"debug",	MX6E_SET_DEBUG_LOG},

	{"add",			"m46e",		MX6E_ADD_M46E_ENTRY},			///< M46E ENTRY 追加
	{"del",			"m46e",		MX6E_DEL_M46E_ENTRY},			///< M46E ENTRY 削除
	{"delall",		"m46e",		MX6E_DELALL_M46E_ENTRY},		///< M46E ENTRY 全削除
	{"enable",		"m46e",		MX6E_ENABLE_M46E_ENTRY},		///< M46E ENTRY 活性化
	{"disable",		"m46e",		MX6E_DISABLE_M46E_ENTRY},		///< M46E ENTRY 非活性化
	{"show",		"m46e",		MX6E_SHOW_M46E_ENTRY},			///< M46E ENTRY 表示

	{"add",			"me6e",		MX6E_ADD_ME6E_ENTRY},			///< ME6E ENTRY 追加
	{"del",			"me6e",		MX6E_DEL_ME6E_ENTRY},			///< ME6E ENTRY 削除
	{"delall",		"me6e",		MX6E_DELALL_ME6E_ENTRY},		///< ME6E ENTRY 全削除
	{"enable",		"me6e",		MX6E_ENABLE_ME6E_ENTRY},		///< ME6E ENTRY 活性化
	{"disable",		"me6e",		MX6E_DISABLE_ME6E_ENTRY},		///< ME6E ENTRY 非活性化
	{"show",		"me6e",		MX6E_SHOW_ME6E_ENTRY},			///< ME6E ENTRY 表示

	{"load",		"m46e",		MX6E_LOAD_COMMAND},				///< M46E/ME6E-Commandファイル読み込み
	{"load",		"me6e",		MX6E_LOAD_COMMAND},				///< M46E/ME6E-Commandファイル読み込み

	{"shutdown",	"",			MX6E_SHUTDOWN},
	{"restart",		"",			MX6E_RESTART},
	{NULL,			NULL,		MX6E_COMMAND_MAX}
};
// *INDENT-ON*

///////////////////////////////////////////////////////////////////////////////
//! @brief コマンド凡例表示関数
//!
//! コマンド実行時の引数が不正だった場合などに凡例を表示する。
//!
//! @param なし
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
static void usage(void)
{
// *INDENT-OFF*
	fprintf(stderr,
			"Usage: mx6ectl -n PLANE_NAME COMMAND OPTIONS\n"
			"       mx6ectl { -h | --help | --usage }\n"
			"\n"
			"where  COMMAND := { exec shell  | exec inet    | show stat   | show conf |\n"
			"                    set debug   | set defgw    |\n"
			"                    add m46e    | del m46e     | delall m46e |\n"
			"                    enable m46e | disable m46e | show m46e   | load m46e |\n"
			"                    add me6e    | del me6e     | delall me6e |\n"
			"                    enable me6e | disable me6e | show me6e   | load me6e |\n"
			"                    shutdown    | restart }\n"
			"where  OPTIONS :=\n"
			"       set debug  :  on/off\n"
			"       add     m46e pr -  [in_plane_id] [in_prefix_len] [ipv4_network_address/prefix_len] [ipv6_network_address/prefix_len] [out_plane_id] [enable|disable]\n"
			"       add     m46e fp [section_device_ipv6_network_address/prefix_len] [in_plane_id] [in_prefix_len] [ipv4_network_address/prefix_len] [ipv6_network_address/prefix_len] [out_plane_id] [enable|disable]\n"
			"       del     m46e pr - [in_plane_id] [in_prefix_len] [ipv4_network_address/prefix_len]\n"
			"       del     m46e fp [section_device_ipv6_network_address/prefix_len] [in_plane_id] [in_prefix_len] [ipv4_network_address/prefix_len]\n"
			"       delall  m46e\n"
			"       enable  m46e pr - [in_plane_id] [in_prefix_len] [ipv4_network_address/prefix_len]\n"
			"       enable  m46e fp [section_device_ipv6_network_address/prefix_len] [in_plane_id] [in_prefix_len] [ipv4_network_address/prefix_len]\n"
			"       disable m46e pr - [in_plane_id] [in_prefix_len] [ipv4_network_address/prefix_len]\n"
			"       disable m46e fp [section_device_ipv6_network_address/prefix_len] [in_plane_id] [in_prefix_len] [ipv4_network_address/prefix_len]\n"
			"       show    m46e\n"
			"       load    m46e file_name\n"
			"\n"
			"       add     me6e pr - [in_plane_id] [in_prefix_len] [hwaddr] [ipv6_network_address/prefix_len] [out_plane_id] [enable|disable]\n"
			"       add     me6e fp [section_device_ipv6_network_address/prefix_len] [in_plane_id] [in_prefix_len] [hwaddr] [ipv6_network_address/prefix_len] [out_plane_id] [enable|disable]\n"
			"       del     me6e pr - [in_plane_id] [in_prefix_len] [hwaddr]\n"
			"       del     me6e fp [section_device_ipv6_network_address/prefix_len] [in_plane_id] [in_prefix_len] [hwaddr]\n"
			"       delall  me6e\n"
			"       enable  me6e pr - [in_plane_id] [in_prefix_len] [hwaddr]\n"
			"       enable  me6e fp [section_device_ipv6_network_address/prefix_len] [in_plane_id] [in_prefix_len] [hwaddr]\n"
			"       disable me6e pr - [in_plane_id] [in_prefix_len] [hwaddr]\n"
			"       disable me6e fp [section_device_ipv6_network_address/prefix_len] [in_plane_id] [in_prefix_len] [hwaddr]\n"
			"       show    me6e\n"
			"       load    me6e file_name\n" "\n"
			"// mx6ectl command explanations // \n"
			"  show stat         : Show the statistics information in specified PLANE_NAME\n"
			"  show conf         : Show the configuration in specified PLANE_NAME\n"
			"  set debug         : Set the debug log printing mode specified PROCESS_NAME\n"
			"  add m46e|me6e     : Add the M46E/ME6E Entry to M46E/ME6E Table specified PLANE_NAME\n"
			"  del m46e|me6e     : Delete the M46E/ME6E Entry from M46E/ME6E Table specified PLANE_NAME\n"
			"  delall m46e|me6e  : Delete the all M46E/ME6E Entry from M46E/ME6E Table specified PLANE_NAME\n"
			"  enable m46e|me6e  : Enable the M46E/ME6E Entry at M46E/ME6E Table specified PLANE_NAME\n"
			"  disable m46e|me6e : Disable the M46E/ME6E Entry at M46E/ME6E Table specified PLANE_NAME\n"
			"  show m46e|me6e    : Show the M46E/ME6E Table specified PLANE_NAME\n"
			"  load m46e|me6e    : Load M46E/ME6E Command file specified PLANE_NAME\n"
			"  shutdown          : Shutting down the application specified PLANE_NAME\n"
			"  restart           : Restart the application specified PLANE_NAME\n"
		);
// *INDENT-ON*

	return;
}

///////////////////////////////////////////////////////////////////////////////
//! @MX6E-PR Entry追加コマンド凡例表示関数
//!
//! MX6E-PR Entry追加コマンド実行時の引数が不正だった場合などに凡例を表示する。
//!
//! @param なし
//!
//! @return なし
///////////////////////////////////////////////////////////////////////////////
static void usage_func(mx6e_command_code_t code)
{
	switch (code) {
	case MX6E_ADD_M46E_ENTRY:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME add     m46e pr|fp plane_id_in prefix_len_in ipv4_network_address/prefix_len ipv6_network_address/prefix_len plane_id_out [enable|disable]\n");
		break;
	case MX6E_DEL_M46E_ENTRY:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME del     m46e pr|fp plane_id_in prefix_len_in ipv4_network_address/prefix_len\n");
		break;
	case MX6E_DELALL_M46E_ENTRY:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME delall  m46e\n");
		break;
	case MX6E_ENABLE_M46E_ENTRY:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME enable  m46e pr|fp plane_id_in prefix_len_in ipv4_network_address/prefix_len\n");
		break;
	case MX6E_DISABLE_M46E_ENTRY:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME disable m46e pr|fp plane_id_in prefix_len_in ipv4_network_address/prefix_len\n");
		break;
	case MX6E_SHOW_M46E_ENTRY:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME show    m46e\n");
		break;

	case MX6E_ADD_ME6E_ENTRY:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME add     me6e pr|fp plane_id_in prefix_len_in hwaddr ipv6_network_address/prefix_len plane_id_out [enable|disable]\n");
		break;
	case MX6E_DEL_ME6E_ENTRY:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME del     me6e pr|fp plane_id_in prefix_len_in hwaddr\n");
		break;
	case MX6E_DELALL_ME6E_ENTRY:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME delall  me6e\n");
		break;
	case MX6E_ENABLE_ME6E_ENTRY:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME enable  me6e pr|fp plane_id_in prefix_len_in hwaddr\n");
		break;
	case MX6E_DISABLE_ME6E_ENTRY:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME disable me6e pr|fp plane_id_in prefix_len_in hwaddr\n");
		break;
	case MX6E_SHOW_ME6E_ENTRY:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME show    me6e\n");
		break;

	case MX6E_LOAD_COMMAND:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME load    m46e|me6e file_name\n");
		break;

	case MX6E_SHOW_CONF:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME show conf\n");
		break;

	case MX6E_SHOW_STATISTIC:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME show stat\n");
		break;

	case MX6E_SET_DEBUG_LOG:
		fprintf(stderr, "Usage: mx6ectl -n PROCESS_NAME set debug on|off\n");
		break;

	default:
		usage();
		break;
	}


	return;
}


////////////////////////////////////////////////////////////////////////////////
// メイン関数
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	char                           *name = NULL;
	int                             option_index = 0;

	// 引数チェック
	while (1) {
		int                             c = getopt_long(argc, argv, "n:h", options, &option_index);
		if (c == -1) {
			break;
		}

		switch (c) {
		case 'n':
			name = optarg;
			break;

		case 'h':
			usage();
			exit(EXIT_SUCCESS);
			break;

		default:
			usage();
			exit(EINVAL);
			break;
		}
	}

	if (name == NULL) {
		usage();
		exit(EINVAL);
	}

	if (argc <= optind) {
		usage();
		exit(EINVAL);
	}

	char                           *cmd_main = argv[optind];
	char                           *cmd_sub = (argc > (optind + 1)) ? argv[optind + 1] : "";

	mx6e_command_t                  command = { 0 };
	command.code = MX6E_COMMAND_MAX;
	for (int i = 0; command_args[i].main != NULL; i++) {
		if (!strcmp(cmd_main, command_args[i].main) && ((NULL == command_args[i].sub) || (command_args[i].sub && !strcmp(cmd_sub, command_args[i].sub)))) {
			command.code = command_args[i].code;
			break;
		}
	}

	_D_(printf("command.code:%d %s\n", command.code, get_command_name(command.code)));

	if (command.code == MX6E_COMMAND_MAX) {
		usage();
		exit(EINVAL);
	}

	bool                            result;
	char                           *cmd_opt[DYNAMIC_OPE_ARGS_NUM_MAX] = { NULL };
	for (int i = 0; i < DYNAMIC_OPE_ARGS_NUM_MAX; i++) {
		cmd_opt[i] = (argc > (optind + i + 2)) ? argv[optind + i + 2] : "";
	}

	_D_(printf("argc:%d\n", argc));
	for (int i = 0; i < DYNAMIC_OPE_ARGS_NUM_MAX; i++) {
		_D_(printf("cmd_opt[%d]:<%s>\n", i, cmd_opt[i]));
	}

	// 0       1  2          3       4    5     6             7               8                                 9                                 10             11
	// mx6ectl -n PLANE_NAME add     m46e pr|fp [plane_id_in] [prefix_len_in] [ipv4_network_address/prefix_len] [ipv6_network_address/prefix_len] [plane_id_out] [enable|disable]

	// *INDENT-OFF*
	struct {
		mx6e_command_code_t code;
		int argc_min;
		int argc_max;
		struct {
			bool (*entry)(int num, char *opt[], mx6e_command_t * command);
			bool (*load)(char *filename, mx6e_command_t * command, char *name);
		} func;

	} table[] = {
		{MX6E_SET_DEBUG_LOG,		DYNAMIC_OPE_DBGLOG_ARGS,	DYNAMIC_OPE_DBGLOG_ARGS,	{mx6e_command_dbglog_set_option}},
		{MX6E_ADD_M46E_ENTRY,		ADD_M46E_OPE_MIN_ARGS,		ADD_M46E_OPE_MAX_ARGS,		{mx6e_command_add_entry_option}},
		{MX6E_DEL_M46E_ENTRY,		DEL_M46E_OPE_ARGS,			DEL_M46E_OPE_ARGS,			{mx6e_command_del_entry_option}},
		{MX6E_DELALL_M46E_ENTRY,	DELALL_M46E_OPE_ARGS,		DELALL_M46E_OPE_ARGS,		{NULL}},
		{MX6E_ENABLE_M46E_ENTRY,	ENABLE_M46E_OPE_ARGS,		ENABLE_M46E_OPE_ARGS,		{mx6e_command_enable_entry_option}},
		{MX6E_DISABLE_M46E_ENTRY,	DISABLE_M46E_OPE_ARGS,		DISABLE_M46E_OPE_ARGS,		{mx6e_command_disable_entry_option}},

		{MX6E_ADD_ME6E_ENTRY,		ADD_ME6E_OPE_MIN_ARGS,		ADD_ME6E_OPE_MAX_ARGS,		{mx6e_command_add_entry_option}},
		{MX6E_DEL_ME6E_ENTRY,		DEL_ME6E_OPE_ARGS,			DEL_ME6E_OPE_ARGS,			{mx6e_command_del_entry_option}},
		{MX6E_DELALL_ME6E_ENTRY,	DELALL_ME6E_OPE_ARGS,		DELALL_ME6E_OPE_ARGS,		{NULL}},
		{MX6E_ENABLE_ME6E_ENTRY,	ENABLE_ME6E_OPE_ARGS,		ENABLE_ME6E_OPE_ARGS,		{mx6e_command_enable_entry_option}},
		{MX6E_DISABLE_ME6E_ENTRY,	DISABLE_ME6E_OPE_ARGS,		DISABLE_ME6E_OPE_ARGS,		{mx6e_command_disable_entry_option}},

		{MX6E_SET_DEBUG_LOG,		DYNAMIC_OPE_DBGLOG_ARGS,	DYNAMIC_OPE_DBGLOG_ARGS,	{mx6e_command_dbglog_set_option}},
		{MX6E_SHOW_CONF,			SHOW_CONF_OPE_ARGS,			SHOW_CONF_OPE_ARGS,			{NULL}},
		{MX6E_SHOW_STATISTIC,		SHOW_STAT_OPE_ARGS,			SHOW_STAT_OPE_ARGS,			{NULL}},
		{MX6E_SHOW_M46E_ENTRY,		SHOW_M46E_OPE_ARGS,			SHOW_M46E_OPE_ARGS,			{NULL}},
		{MX6E_SHOW_ME6E_ENTRY,		SHOW_ME6E_OPE_ARGS,			SHOW_ME6E_OPE_ARGS,			{NULL}},
		{MX6E_LOAD_COMMAND,			OPE_NUM_LOAD,				OPE_NUM_LOAD,				{NULL, mx6e_command_load}},
		{MX6E_COMMAND_MAX,			0,							0,							{NULL}},
	};
	// *INDENT-ON*

	for (int i = 0; MX6E_COMMAND_MAX > table[i].code; i++) {
		if (command.code == table[i].code) {
			if ((argc < table[i].argc_min) || (table[i].argc_max < argc)) {
				usage_func(command.code);
				exit(EINVAL);
			}
			if (table[i].func.entry) {
				result = table[i].func.entry(argc - 5, cmd_opt, &command);
				if (!result) {
					usage_func(command.code);
					exit(EINVAL);
				}
			}
			if (table[i].func.load) {
				result = table[i].func.load(cmd_opt[0], &command, name);
				if (!result) {
					usage_func(command.code);
					exit(EINVAL);
				}
				// load実行後は以後の処理をせず関数を終了する
				return 0;
			}
		}
	}


	int                             fd;
	char                            path[sizeof(((struct sockaddr_un *) 0)->sun_path)] = { 0 };
	char                           *offset = &path[1];

	sprintf(offset, MX6E_COMMAND_SOCK_NAME, name);

	fd = socket(PF_UNIX, SOCK_SEQPACKET, 0);
	if (fd < 0) {
		printf("fail to open socket : %s\n", strerror(errno));
		return -1;
	}

	struct sockaddr_un              addr;
	memset(&addr, 0, sizeof(addr));

	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, path, sizeof(addr.sun_path));

	if (connect(fd, (struct sockaddr *) &addr, sizeof(addr))) {
		printf("fail to connect MX6E application(%s) : %s\n", name, strerror(errno));
		close(fd);
		return -1;
	}

	int                             ret;
	int                             pty;
	char                            buf[256];

	ret = mx6e_socket_send_cred(fd, command.code, &command.req, sizeof(command.req));
	if (ret <= 0) {
		printf("fail to send command : %s\n", strerror(-ret));
		close(fd);
		return -1;
	}

	ret = mx6e_socket_recv(fd, &command.code, &command.res, sizeof(command.res), &pty);
	if (ret <= 0) {
		printf("fail to receive response : %s\n", strerror(-ret));
		close(fd);
		return -1;
	}
	if (command.res.result != 0) {
		printf("receive error response : %s\n", strerror(command.res.result));
		close(fd);
		return -1;
	}

	switch (command.code) {
	case MX6E_SHOW_STATISTIC:
	case MX6E_SHOW_CONF:
	case MX6E_SET_DEBUG_LOG:
	case MX6E_ADD_M46E_ENTRY:						///< M46E ENTRY 追加
	case MX6E_DEL_M46E_ENTRY:						///< M46E ENTRY 削除
	case MX6E_DELALL_M46E_ENTRY:					///< M46E ENTRY 全削除
	case MX6E_ENABLE_M46E_ENTRY:					///< M46E ENTRY 活性化
	case MX6E_DISABLE_M46E_ENTRY:					///< M46E ENTRY 非活性化
	case MX6E_SHOW_M46E_ENTRY:						///< M46E ENTRY 表示

	case MX6E_ADD_ME6E_ENTRY:						///< ME6E ENTRY 追加
	case MX6E_DEL_ME6E_ENTRY:						///< ME6E ENTRY 削除
	case MX6E_DELALL_ME6E_ENTRY:					///< ME6E ENTRY 全削除
	case MX6E_ENABLE_ME6E_ENTRY:					///< ME6E ENTRY 活性化
	case MX6E_DISABLE_ME6E_ENTRY:					///< ME6E ENTRY 非活性化
	case MX6E_SHOW_ME6E_ENTRY:						///< ME6E ENTRY 表示

	case MX6E_LOAD_COMMAND:							///< M46E/ME6E-Commandファイル読み込み

		// 出力結果がソケット経由で送信されてくるので、そのまま標準出力に書き込む
		while (1) {
			ret = read(fd, buf, sizeof(buf));
			if (ret > 0) {
				ret = write(STDOUT_FILENO, buf, ret);
			} else {
				break;
			}
		}
		close(fd);
		break;

	case MX6E_SHUTDOWN:
	case MX6E_RESTART:
		close(fd);
		break;

	default:
		// ありえない
		close(fd);
		break;
	}

	return 0;
}
