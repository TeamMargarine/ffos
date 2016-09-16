/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _SLOG_H
#define _SLOG_H

#define LOG_TAG "slog"
#include <errno.h>
#include <pthread.h>
#include <utils/Log.h>

/* "type" field */
#define SLOG_TYPE_STREAM	0x1
#define SLOG_TYPE_MISC		(0x1 << 1)
#define SLOG_TYPE_SNAPSHOT	(0x1 << 2)
#define SLOG_TYPE_NOTIFY	(0x1 << 3)
#define SLOG_TYPE_EXEC		(0x1 << 4)

/* "state" field */
#define SLOG_STATE_ON 0
#define SLOG_STATE_OFF 1

/*"slog state" field*/
#define SLOG_LOW_POWER 2
#define SLOG_ENABLE 1
#define SLOG_DISABLE 0

/* "opt" field */
#define SLOG_EXEC_OPT_EXEC	0

/* socket control command type */
enum {
	CTRL_CMD_TYPE_RELOAD,
	CTRL_CMD_TYPE_SNAP,
	CTRL_CMD_TYPE_SNAP_ALL,
	CTRL_CMD_TYPE_EXEC,
	CTRL_CMD_TYPE_ON,
	CTRL_CMD_TYPE_OFF,
	CTRL_CMD_TYPE_QUERY,
	CTRL_CMD_TYPE_CLEAR,
	CTRL_CMD_TYPE_DUMP,
	CTRL_CMD_TYPE_SCREEN,
	CTRL_CMD_TYPE_HOOK_MODEM,
	CTRL_CMD_TYPE_RSP
};

#ifdef SLOG_ALOGD_ALOGE
#define err_log(fmt, arg...) ALOGE("%s: " fmt " [%d]\n", __func__, ## arg, errno);
/*#define debug_log(fmt, arg...) ALOGD("%s: " fmt, __func__, ## arg);*/
#define debug_log(fmt, arg...)
#else
#define err_log(fmt, arg...) LOGE("%s: " fmt " [%d]\n", __func__, ## arg, errno);
/*#define debug_log(fmt, arg...) LOGD("%s: " fmt, __func__, ## arg);*/
#define debug_log(fmt, arg...)
#endif

#define INTERNAL_LOG_PATH		"/data/slog"
#define DEFAULT_DEBUG_SLOG_CONFIG	"/system/etc/slog.conf"
#define DEFAULT_USER_SLOG_CONFIG	"/system/etc/slog.conf.user"
#define TMP_FILE_PATH			"/data/local/tmp/slog/"
#define SLOG_SOCKET_FILE		TMP_FILE_PATH "slog_sock"
#define TMP_SLOG_CONFIG			TMP_FILE_PATH "slog.conf"
#define DEFAULT_DUMP_FILE_NAME		"slog.tgz"
#define FB_DEV_NODE			"/dev/graphics/fb0"

#define MAX_NAME_LEN			128
#define MAX_LINE_LEN			2048
#define DEFAULT_MAX_LOG_SIZE		256 /* MB */
#define MAXROLLLOGS			10
#define TIMEOUT_FOR_SD_MOUNT		5 /* seconds */
#define MODEM_CIRCULAR_SIZE		(2 * 1024 * 1024) /* 2 MB */
#define SINGLE_BUFFER_SIZE		(16 * 1024) /* 16k */
#define RING_BUFFER_NUM			(MODEM_CIRCULAR_SIZE / SINGLE_BUFFER_SIZE)
#define HOOK_MODEM_TARGET_DIR		"/data/log"

#define KERNEL_LOG_SOURCE		"/proc/kmsg"
#define MODEM_LOG_SOURCE		"/dev/vbpipe0"

/* handler last log dir */
#define LAST_LOG 			"last_log"
#define LOG_DIR_MAX_NUM 		5
#define LOG_DIR_NUM 			1

/* main data structure */
struct slog_info {
	struct slog_info	*next;

	/* log type */
	unsigned long	type;

	/* control log on/off */
	int		state;

	/* for different logs, level give several meanings */
	int		level;

	/* control snapshot record */
	int		interval;

	/* used for "snapshot" type */
	char		*opt;

	/* log file size limit */
	int		max_size;

	/* identify a certain log entry, must uniq */
	char		*name;

	/* define directly log path, sometimes need update according to external storage changed */
	char		*log_path;

	/* filename without timestamp */
	char		*log_basename;

	/* used for "snap" type*/
	char		*content;

	/* used for "stream" type, log source handle */
	int		fd_device;

	/* log file handle */
	int		fd_out;

	/* current log file size count */
	int		outbytecount;

	/* for handle anr */
	struct timeval last, current;
}; 

struct slog_cmd {
	int type;
	char content[MAX_LINE_LEN];
};

/* var */
extern char *config_log_path, *current_log_path;
extern char top_logdir[MAX_NAME_LEN];
extern char external_storage[MAX_NAME_LEN];
extern struct slog_info *stream_log_head, *snapshot_log_head;
extern struct slog_info *notify_log_head, *misc_log;
extern pthread_t stream_tid, snapshot_tid, notify_tid, sdcard_tid, bt_tid, tcp_tid, modem_tid, modem_dump_memory_tid;
extern int slog_enable;
extern int internal_log_size;
extern int screenshot_enable;
extern int slog_save_all;
extern int hook_modem_flag;
extern int dev_shark_flag;

/* function */
extern int parse_config();
extern void *stream_log_handler(void *arg);
extern void *snapshot_log_handler(void *arg);
extern void *notify_log_handler(void *arg);
extern void *bt_log_handler(void *arg);
extern void *tcp_log_handler(void *arg);
extern void *modem_log_handler(void *arg);
extern void *modem_dump_memory_handler(void *arg);
extern int stream_log_handler_started;
extern int snapshot_log_handler_started;
extern int notify_log_handler_started;
extern int bt_log_handler_started;
extern int tcp_log_handler_started;
extern int modem_log_handler_started;
extern int gen_config_string(char *buffer);
extern void cp_file(char *path, char *new_path);
extern void exec_or_dump_content(struct slog_info *info, char *filepath);
extern int capture_by_name(struct slog_info *head, const char *name, char *filepath);
extern int screen_shot(const char *name);
#endif /*_SLOG_H*/
