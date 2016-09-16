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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "slog.h"

int send_socket(int sockfd, void* buffer, int size)
{
        int result = -1;
        int ioffset = 0;
        while(sockfd > 0 && ioffset < size) {
                result = send(sockfd, (char *)buffer + ioffset, size - ioffset, MSG_NOSIGNAL);
                if (result > 0) {
                        ioffset += result;
                } else {
                        break;
                }
        }
        return result;

}

int
recv_socket(int sockfd, void* buffer, int size)
{
        int received = 0, result;
        while(buffer && (received < size))
        {
                result = recv(sockfd, (char *)buffer + received, size - received, MSG_NOSIGNAL);
                if (result > 0) {
                        received += result;
                } else {
                        received = result;
                        break;
                }
        }
        return received;
}

void update_conf(const char *keyword)
{
	FILE *fp;
	int len = 0;
	char buffer[MAX_LINE_LEN], line[MAX_NAME_LEN];

	fp = fopen(TMP_SLOG_CONFIG, "r");
	if(fp == NULL) {
		perror("open conf failed!\n");
		return;
	}

	while(fgets(line, MAX_NAME_LEN, fp) != NULL) {
		if(!strncmp("enable", line, 6) || !strncmp("disable", line, 7) || !strncmp("low_power", line, 8)) {
			len += sprintf(buffer + len, "%s\n", keyword);
		} else {
			len += sprintf(buffer + len, "%s", line);
		}
	}
	fclose(fp);
	fp = fopen(TMP_SLOG_CONFIG, "w");
	if(fp == NULL) {
		perror("open conf failed!\n");
		return;
	}

	fprintf(fp, "%s", buffer);
	fclose(fp);
	return;
}

void usage(const char *name)
{
	printf("Usage: %s <operation> [arguments]\n", name);
	printf("Operation:\n"
               "\tenable      update config file and enable slog\n"
               "\tdisable     update config file and disable slog\n"
               "\tlow_power   update config file and make slog in low_power state\n"
               "\treload      reboot slog and parse config file.\n"
               "\tsnap [arg]  catch certain snapshot log, catch all snapshot without any arg\n"
               "\texec <arg>  through the slogctl to run a command.\n"
               "\ton          start slog.\n"
               "\toff         pause slog.\n"
               "\tclear       delete all log.\n"
               "\tdump [file] dump all log to a tar file.\n"
               "\tscreen [file] screen shot, if no file given, will be put into misc dir\n"
               "\thook_modem  dump current modem log to /data/log\n"
               "\tquery       print the current slog configuration.\n");
	return;
}

int main(int argc, char *argv[])
{
        int sockfd, ret;
	struct slog_cmd cmd;
	struct sockaddr_un address;

	/*
	arguments list:
	enable		update config file and enable slog
	disable		update config file and disable slog
	low_power	update config file and make slog in low_power state

	reload		CTRL_CMD_TYPE_RELOAD,
	snap $some	CTRL_CMD_TYPE_SNAP,
	snap 		CTRL_CMD_TYPE_SNAP_ALL,
	exec $some	CTRL_CMD_TYPE_EXEC,
	on		CTRL_CMD_TYPE_ON,
	off		CTRL_CMD_TYPE_OFF,
	query		CTRL_CMD_TYPE_QUERY,
	clear		CTRL_CMD_TYPE_CLEAR,
	dump		CTRL_CMD_TYPE_DUMP,
	screen		CTRL_CMD_TYPE_SCREEN,
	hook_modem      CTRL_CMD_TYPE_HOOK_MODEM
	*/
	if(argc < 2) {
		usage(argv[0]);
		return 0;
	}

	memset(&cmd, 0, sizeof(cmd));

	if(!strncmp(argv[1], "reload", 6)) {
		cmd.type = CTRL_CMD_TYPE_RELOAD;
	} else if(!strncmp(argv[1], "snap", 4)) {
		if(argc == 2) 
			cmd.type = CTRL_CMD_TYPE_SNAP_ALL;
		else {
			cmd.type = CTRL_CMD_TYPE_SNAP;
			snprintf(cmd.content, MAX_NAME_LEN, "%s", argv[2]);
		}
	} else if(!strncmp(argv[1], "exec", 4)) {
		if(argc == 2)  {
			usage(argv[0]);
			return 0;
		} else {
			cmd.type = CTRL_CMD_TYPE_EXEC;
			snprintf(cmd.content, MAX_NAME_LEN, "%s", argv[2]);
		}
	} else if(!strncmp(argv[1], "on", 2)) {
		cmd.type = CTRL_CMD_TYPE_ON;
	} else if(!strncmp(argv[1], "off", 3)) {
		cmd.type = CTRL_CMD_TYPE_OFF;
	} else if(!strncmp(argv[1], "clear", 5)) {
		cmd.type = CTRL_CMD_TYPE_CLEAR;
	} else if(!strncmp(argv[1], "dump", 4)) {
		cmd.type = CTRL_CMD_TYPE_DUMP;
		if(argc == 2)
			snprintf(cmd.content, MAX_NAME_LEN, "%s", DEFAULT_DUMP_FILE_NAME);
		else
			snprintf(cmd.content, MAX_NAME_LEN, "%s.tgz", argv[2]);
	} else if(!strncmp(argv[1], "screen", 6)) {
		cmd.type = CTRL_CMD_TYPE_SCREEN;
		if(argc == 3)
			snprintf(cmd.content, MAX_NAME_LEN, "%s", argv[2]);
	} else if(!strncmp(argv[1], "query", 5)) {
		cmd.type = CTRL_CMD_TYPE_QUERY;
	} else if(!strncmp(argv[1], "hook_modem", 10)) {
		cmd.type = CTRL_CMD_TYPE_HOOK_MODEM;
	} else if(!strncmp(argv[1], "enable", 6)) {
		update_conf("enable");
		cmd.type = CTRL_CMD_TYPE_RELOAD;
	} else if(!strncmp(argv[1], "disable", 7)) {
		update_conf("disable");
		cmd.type = CTRL_CMD_TYPE_RELOAD;
	} else if(!strncmp(argv[1], "low_power", 9)) {
		update_conf("low_power");
		cmd.type = CTRL_CMD_TYPE_RELOAD;
	} else {
		usage(argv[0]);
		return 0;
	}

	/* init unix domain socket */
	memset(&address, 0, sizeof(address));
	address.sun_family=AF_UNIX; 
	strcpy(address.sun_path, SLOG_SOCKET_FILE);

 	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd < 0) {
		perror("create socket failed");
		return -1;
	}
	ret = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
        if (ret < 0) {
		perror("connect failed");
		return -1;
	}
	ret = send_socket(sockfd, (void *)&cmd, sizeof(cmd));
        if (ret < 0) {
		perror("send failed");
		return -1;
	}
	ret = recv_socket(sockfd, (void *)&cmd, sizeof(cmd));
        if (ret < 0) {
		perror("recv failed");
		return -1;
	}
	if(!strcmp(cmd.content,"FAIL")){
		printf("slogctl cmd fail \n");
		return -1;
	}
	printf("%s\n", cmd.content);

	close(sockfd);
	return 0;
}
