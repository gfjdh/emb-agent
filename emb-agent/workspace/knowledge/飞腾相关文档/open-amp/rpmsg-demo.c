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

static int count = 100, no; 

int main(int argc, char **argv)
{
	int ctrl_fd, rpmsg_fd, ret;
	struct rpmsg_endpoint_info eptinfo;
	struct pollfd fds;
	uint8_t buf[32], r_buf[32];

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

	/* receive message from remote processor. */
	while (count) {
		snprintf(buf, sizeof(buf), "%s%d", "Hello World! No:", ++no);

		/* send message to remote processor. */
		ret = write_full(rpmsg_fd, buf, sizeof(buf));
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

		memset(r_buf, 0, 32);
		ret = read_full(rpmsg_fd, r_buf, 32);
		if (ret < 0) {
			perror("read_full failed.\n");
			goto err1;
		}

		/* output message */
		printf("received message: %s\n", r_buf);

		usleep(5000);
		count--;
	}

err1:
	close(rpmsg_fd);
err0:
	close(ctrl_fd);

	return 0;
}
