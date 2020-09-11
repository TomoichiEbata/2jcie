/*
 * This file is provided under a Simplified BSD License.
 *
 * Copyright (C) 2019 Atmark Techno, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <termios.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "common.h"
#include "sensor_data.h"

static void usage(char *basename) {
	printf("usage: %s <device> <mode> <csv path>\n\n", basename);
	printf(
		"A program that acquires data from [omron 2JCIE - BU 01] by USB communication.\n"
		"  device   : Omron USB Device Path.\n"
		"  mode     : Amount of data to read. Laster data = 0 , Memory all data = 1\n"
		"  csv path : Create csv file full path. If not specified, it is displayed on standard output.\n");
}

/*
 * init usb serial
 */
static int init_serial(int fd, struct termios *old_tio) {
	int ret;
	struct termios tio;

	ret = tcgetattr(fd, old_tio);
	if (ret) {
		perror("tcgetattr");
		return -1;
	}

	// new serial conf
	memset(&tio, 0, sizeof(tio));

	tio.c_iflag = IGNBRK | IGNPAR;
	tio.c_cflag = CS8 | CLOCAL | CREAD;
	ret = cfsetspeed(&tio, SERIAL_BAUDRATE);
	if (ret < 0) {
		perror("cfsetspeed");
		return -1;
	}

	ret = tcflush(fd, TCIFLUSH);
	if (ret < 0) {
		perror("tcflush");
		return -1;
	}

	// new serial conf set
	ret = tcsetattr(fd, TCSANOW, &tio);
	if (ret < 0) {
		perror("tcsetattr");
		return -1;
	}

	return 0;
}

/*
 * restore serial
 */
static void restore_serial(int fd, struct termios *old_tio) {
	int ret;

	ret = tcsetattr(fd, TCSANOW, old_tio);
	if (ret < 0) {
		perror("tcsetattr");
	}
}

/*
 * Find out name to use for lockfile when locking tty.
 */
static char *dev_lockname(char *dev_name, char *res, int res_len) {
	char *temp;

	if (strncmp(dev_name, "/dev/", 5) == 0) {
		// In dev
		strncpy(res, dev_name + 5, res_len - 1);
		res[res_len - 1] = 0;
		for (temp = res; *temp; temp++) {
			if (*temp == '/') {
				*temp = '_';
			}
		}
	} else {
		// Outside of dev
		if ((temp = strrchr(dev_name, '/')) == NULL ) {
			temp = dev_name;
		} else {
			temp++;
		}
		strncpy(res, dev_name, res_len - 1);
		res[res_len - 1] = 0;
	}
	return res;
}

int main(int argc, char *argv[]) {
	static int fd, mode;
	static int lock_fd;
	static int old_mask;
	static int lock_check;
	static char *dev_name;
	static char *csv_path;
	static char lockfile[128];
	static char lockbuf[127];
	static struct termios tio;

	int ret = 0;

	if (argc < 3) {
		usage(basename(argv[0]));
		return -1;
	}

	dev_name = argv[1];
	mode = atoi(argv[2]);
	csv_path = argv[3];

	// mode num check
	if (mode > MAX_MODE_NUM || mode < 0) {
		usage(basename(argv[0]));
		return -1;
	}

	// USB port open
	fd = open(dev_name, O_RDWR);
	if (fd < 0) {
		perror("port open");
		return -1;
	}

	// serial port conf
	ret = init_serial(fd,&tio);
	if (ret) {
		perror("serial");
		//goto exit_close;
		close(fd);
		exit(-1);
	}

	// handler
	ret = install_sig_handler();
	if (ret) {
		perror("handler");
		//goto exit_restore;
		restore_serial(fd, &tio);
		exit(-1);
	}

	// port lock
	snprintf(lockfile, sizeof(lockfile), "/var/lock/LCK..%s", dev_lockname(dev_name, lockbuf, sizeof(lockbuf)));
	if ((lock_fd = open(lockfile, O_RDONLY)) >= 0) {
		lock_check = read(lock_fd, lockbuf, sizeof(lockbuf));
		close(lock_fd);
		if (lock_check > 0) {
			// Lockfile is stale
			unlink(lockfile);
		} else if (lock_check == 0) {
			// Device is locked
			printf("Device %s is locked.\n",dev_name);
			ret = -1;
			//return exit_restore;
			restore_serial(fd, &tio);
			exit(-1);
		}
	}

	old_mask = umask(022);
	lock_fd = open(lockfile, O_WRONLY | O_CREAT | O_EXCL, 0666);
	umask(old_mask);
	if (lock_fd < 0) {
		perror("lockfile");
		ret = -1;
		//goto exit_restore;
		restore_serial(fd, &tio);
		exit(-1);
	}

	if (mode == MODE_LATEST) {
		printf("Mode : Get Latest Data.\n");
		ret = get_latest_data(fd, csv_path);
		if (ret) {
			printf("get latest data error.\n");
			goto exit_unlock;
		}

	} else if (mode == MODE_MEMDATA) {
		printf("Mode : Get Memory Data.\n");
		ret = get_memory_data(fd, csv_path);
		if (ret) {
			printf("get memory data error.\n");
			goto exit_unlock;
		}
	}

	printf("Program all success.\n");

exit_unlock:
	unlink(lockfile);

exit_restore:
	restore_serial(fd, &tio);

exit_close:
	close(fd);

	return ret;
}
