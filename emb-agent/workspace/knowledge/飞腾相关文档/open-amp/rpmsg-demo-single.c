/*
 * Phytium's Remote Processor Control Driver
 *
 * Copyright (C) 2022 Phytium Technology Co., Ltd. - All Rights Reserved
 * Author: Shaojun Yang <yangshaojun@phytium.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License version 2 as published by the Free Software Foundation.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <poll.h>
#include <linux/rpmsg.h>
#include <errno.h>

#define MAX_DATA_LENGTH 256

#define DEVICE_CORE_START     0x0001U
#define DEVICE_CORE_STOP      0x0002U
#define DEVICE_CORE_CHECK     0x0003U

/* transmission data frame */
typedef struct {
    uint32_t command;
    uint16_t length;
    char data[MAX_DATA_LENGTH];
} data_packet;

static ssize_t write_full(int fd, void *buf, size_t count)
{
	ssize_t ret = 0;
	ssize_t total = 0;

	while (count) {
		ret = write(fd, buf, count);
		if (ret < 0) {
			if (errno == EINTR)
				continue;

			break;
		}

		count -= ret;
		buf += ret;
		total += ret;
	}

	return total;
}

static ssize_t read_full(int fd, void *buf, size_t count)
{
	ssize_t res;

	do {
		res = read(fd, buf, count);
	} while (res < 0 && errno == EINTR);

	if ((res < 0 && errno == EAGAIN))
		return 0;

	if (res < 0)
		return -1;

	return res;
}

static int count = 100;
static int no;

int main(int argc, char **argv)
{
	int ctrl_fd, rpmsg_fd, ret;
	int leng;
	struct rpmsg_endpoint_info eptinfo;
	struct pollfd fds;
	char *buff;
	data_packet test_data;
	data_packet test_data_r;
	char buff_r[MAX_DATA_LENGTH];

	printf("argc: %d\n", argc);
	for (int i = 0; i < argc; i++) {
		printf("Argument %d: %s\n", i, argv[i]);
	}

	test_data.command = DEVICE_CORE_CHECK;
	buff = test_data.data;

	ctrl_fd = open("/dev/rpmsg_ctrl0", O_RDWR);
	if (ctrl_fd < 0) {
		perror("open rpmsg_ctrl0 failed.\n");
		return -1;
	}

	memcpy(eptinfo.name, "xxxx", 32);
	eptinfo.src = 0;
	eptinfo.dst = 0;

	ret = ioctl(ctrl_fd, RPMSG_CREATE_EPT_IOCTL, eptinfo);
	if (ret != 0) {
		perror("ioctl RPMSG_CREATE_EPT_IOCTL failed.\n");
		goto err0;
	}

	rpmsg_fd = open("/dev/rpmsg0", O_RDWR);
	if (rpmsg_fd < 0) {
		perror("open rpmsg0 failed.\n");
		goto err1;
	}

	memset(&fds, 0, sizeof(struct pollfd));
	fds.fd = rpmsg_fd;
	fds.events |= POLLIN;

	if (argc > 2 && strcmp(argv[1], "stop") == 0) {
		if (strcmp(argv[2], "0") == 0) {
			printf("want to stop 0\n");
			test_data.command = DEVICE_CORE_STOP;
			test_data.length = 1;
			ret = write_full(rpmsg_fd, &test_data, sizeof(data_packet));
			if (ret < 0) {
				perror("write_full failed.\n");
			}
		} else {
			printf("stop para err!\n");
		}

		goto err1;
	}

	/* receive message from remote processor. */
	while (count) {
		leng = snprintf(buff, MAX_DATA_LENGTH, "%s%d", "Hello World! No:", ++no);
		test_data.length = leng;

		/* send message to remote processor. */
		ret = write_full(rpmsg_fd, &test_data, sizeof(data_packet));
		if (ret < 0) {
			perror("write_full failed.\n");
			goto err1;
		}


		ret = poll(&fds, 1, 0);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			goto err1;
		}


		usleep(5000);

		memset(&test_data_r, 0, sizeof(data_packet));
		ret = read_full(rpmsg_fd, &test_data_r, sizeof(data_packet));
		if (ret < 0) {
			perror("read_full failed.\n");
			goto err1;
		}

		memcpy(buff_r, test_data_r.data, test_data_r.length);
		/* output message */
		printf("received message: %s\n", buff_r);

		usleep(5000);

		count--;
		memset(buff_r, 0, MAX_DATA_LENGTH);
	}

err1:
	close(rpmsg_fd);
err0:
	close(ctrl_fd);

	return 0;
}

