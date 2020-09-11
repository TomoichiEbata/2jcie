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
#include <sys/select.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <libgen.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "common.h"
#include "data_output.h"

//crc16 format
#define CRC16POLY               (0xa001)
#define CRC_INIT                (0xffff)

//data length
#define LEN_W_LATEST            (9)

#define LEN_W_MEMINFO           (9)
#define LEN_R_MEMINFO           (17)
#define LEN_W_MEMDATA           (17)
#define LEN_R_MEMDATA_ONE       (41)

//command format
#define CMD_READ                (0x01)   // Table72
#define CMD_WRITE               (0x02)   // Table72 not used in this program
#define LATEST_LEN              (0x0005)


///// Table 84の内容
#define LATEST_ADDR             (0x5022) // Table84
#define LEN_R_LATEST            (30)  // 一時コメントアウト

///// Table 100の内容
//#define LATEST_ADDR             (0x5013) // Table 100
//#define LEN_R_LATEST            (27) // LATEST_ADDR が 0x5013

///// Table 103の内容
// #define LATEST_ADDR             (0x5016) // Table 103
// #define LEN_R_LATEST            (24) // LATEST_ADDR が 0x5016



#define INFO_LEN                (0x0005)  // Table71
#define INFO_ADDR               (0x5004)
#define MEMORY_LEN              (0x000D)
#define MEMORY_ADDR             (0x500F) // 4.4.2 Memory data short

#define MAX_RETRY				(10)

#define __unused __attribute__((unused))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static int terminated = 0;

/*
 * signal handler
 */
static void sig_handler(__unused int signum) {
	terminated = 1;
}

/*
 * init signal handler
 */
int install_sig_handler(void) {
	int term_sigs[] = {
		SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGTERM
	};
	struct sigaction sa;
	unsigned int i;
	int ret;

	// init handler
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sig_handler;

	// init signal action
	for (i = 0; i < ARRAY_SIZE(term_sigs); i++) {
		ret = sigaction(term_sigs[i], &sa, NULL);
		if(ret < 0) {
			perror("sigaction");
			return -1;
		}
	}
	return 0;
}

/*
 * buffer print
 * debug function
 */
#ifdef DEBUG
static void dump_buff(unsigned char *buff, int len) {
	int i;
	for (i = 0; i < len ; i++) {
		printf("flame %d = %02x\n", i, buff[i]);
	}
	printf("-----------------------------------\n");
}
#endif

/*
 * write
 */
static ssize_t xwrite(int fd, void *buf, size_t count) {
	size_t len;
	ssize_t ret = -1;

#ifdef DEBUG
	printf("-------\n");	
	printf("xwrite()\n");
	printf("-------\n");	
#endif


	for (len = 0; len < count; len += ret) {
		ret = write(fd, buf + len, count - len);
		if (ret < 0) {
			if (errno == EINTR && !terminated) {
				ret = 0;
				continue;
			}
			perror("write");
			return ret;
		}
	}
	return ret;
}

/*
 * read
 */
static ssize_t xread(int fd, void *buf, size_t count) {
	size_t len;
	ssize_t ret = -1;

#ifdef DEBUG
	printf("-------\n");	
	printf("xread()\n");
	printf("-------\n");	
#endif
	
	
	for (len = 0; len < count; len += ret) {
	  ret = read(fd, buf + len, count - len);
		if (ret < 0) {
			if (errno == EINTR && !terminated) {
				ret = 0;
				continue;
			}
			perror("read");
			return ret;
		}
	}
	return ret;
}

/*
 * select
 */
 static int xselect(int fd ,struct timeval tv, fd_set fds) {
	 int rc = 0;

	 tv.tv_sec = 1;
	 tv.tv_usec = 0;

#ifdef DEBUG
	 printf("---------\n");	
	 printf("xselect()\n");
	 printf("---------\n");		 
#endif

	 rc = select(fd+1, &fds, NULL, NULL, &tv);
	 if (rc < 0) {
		 perror("select");
		 return -1;
	 }
	 if (rc == 0) {
		 printf("CAUTION: time out.\n");
		 return 0;
	 }

	 return 1;
 }

/*
 * usb communication
 */
static int communicate_command(int fd, uint8_t *wbuf, size_t wcount,uint8_t *rbuf, size_t rcount) {
	ssize_t ret;
	int sel_ret = 0;
	struct timeval tv = {0};
	fd_set fds;
	int i = 0;

	FD_ZERO(&fds);
	FD_SET(fd,&fds);

	for (i = 0; i < MAX_RETRY; i++) { // MAX_RETRYは10

		ret = xwrite(fd, wbuf, wcount);
		if (ret < 0) {
			continue;
		}

		sel_ret = xselect(fd, tv, fds);
		if (sel_ret == 0) {
			continue;
		}

		break;
	}

	if (sel_ret < 0) {
		return -1;
	}

	if (FD_ISSET(fd, &fds)) {
		ret = xread(fd, rbuf, rcount);
		if (ret < 0) {
			return -1;
		}
	}

	return 0;
}

/*
 * crc16 bit Calc
 */
static unsigned short crc16_bit_calc(unsigned char *ptr, int len) {
	int i,j;
	unsigned short crc = CRC_INIT;
	for (i = 0; i < len; i++) {
		crc ^= ptr[i];
		for (j=0; j < 8; j++) {
			if (crc & 1) {
				crc = (crc >> 1) ^ CRC16POLY;
			} else {
				crc >>= 1;
			}
		}
	}
	return crc;
}

/*
 * data analyses
 */
static void data_analyses(FILE *output_file, uint8_t *buf) {
	struct senser_data_t data;

	// set data
	// ここはTable84
	data.temp = (float)(buf[0] | (buf[1] << 8)) / 100;
	data.humid = (float)(buf[2] | (buf[3] << 8)) /100;
	data.light = (int)(buf[4] | (buf[5] << 8));
	data.press = (double)(buf[6] | (buf[7] << 8) | (buf[8] << 16) | (buf[9] << 24)) / 1000;
	data.noise = (float)(buf[10] | (buf[11] << 8)) / 100;
	data.TVOC = (int)(buf[12] | (buf[13] << 8));
	data.CO2 = (int)(buf[14] | (buf[15] << 8));
	data.discom = (float)(buf[16] | (buf[17] << 8)) / 100;
	data.heat = (float)(buf[18] | (buf[19] << 8)) / 100;

	usb_data_output(output_file, data);
}

/*
 * short command create
 */
static void short_comm_create(unsigned char *write_frame, unsigned short len, unsigned char comm, unsigned short addr) {
	unsigned short crc16;
	write_frame[0] = 0x52;		// header low
	write_frame[1] = 0x42;		// header high
	write_frame[2] = len & 0x00ff;	// [Payload + crc16] Length low
	write_frame[3] = len >> 8;	// [Payload + crc16] Length high

	write_frame[4] = comm;		// Payload : Read command (0x01) Table72
	write_frame[5] = addr & 0x00ff;	// Payload : address low addrは5022 4.4.4 Latest data short
	write_frame[6] = addr >> 8;	// Payload : address high
	
	crc16 = crc16_bit_calc(write_frame,LEN_W_LATEST-2);
	write_frame[7] = crc16 & 0x00ff;
	write_frame[8] = crc16 >> 8;

#ifdef DEBUG
	printf("in short_comm_create()\n");
	dump_buff(write_frame,LEN_W_LATEST);
#endif
}


/*
 * get latest data
 */
int get_latest_data(int fd, const char *csv_path) {
	static unsigned char write_frame[20];
	static unsigned char *read_frame;
	unsigned short crc16, checkcrc;
	int ret = 0;
	FILE *output_file;

	read_frame = (unsigned char *)malloc(LEN_R_LATEST);
	short_comm_create(write_frame, LATEST_LEN, CMD_READ, LATEST_ADDR);

#ifdef DEBUG
	printf("in get_latest_data() after short_comm_create()\n");
	dump_buff(write_frame, LEN_W_LATEST);
#endif
	ret = communicate_command(fd, write_frame, LEN_W_LATEST, read_frame, LEN_R_LATEST);
	if (ret) {
		printf("command communication failed.\n");
		ret = -1;
		goto exit_free;
	}
#ifdef DEBUG
	printf("in get_latest_data() after communicate_command()\n");	
	dump_buff(read_frame, LEN_R_LATEST);
#endif
	// crc16 check
	crc16 = crc16_bit_calc(read_frame, LEN_R_LATEST-2);
	checkcrc = read_frame[LEN_R_LATEST-1] << 8 | read_frame[LEN_R_LATEST-2];
	if (crc16 != checkcrc) {
		printf("crc16 check failed.\n");
		ret = -1;
		goto exit_free;
	}

	if (csv_path != NULL) {
		output_file = fopen(csv_path, "w");
	} else {
		output_file = stdout;
	}
	if (output_file == NULL) {
		printf("output file open failed.\n");
		ret = -1;
		goto exit_free;
	}

	header_output(output_file); // 項目の見出しの作成
	data_analyses(output_file, read_frame + 8);

	if (csv_path != NULL) {
	        fclose(output_file);
	}

exit_free:
	free(read_frame);

	return ret;
}

/*
 * long_comm_create
 */
static int long_comm_create(unsigned char *write_frame, unsigned short len, 
		     unsigned char comm, unsigned short addr, unsigned char *info_frame) {
	unsigned short crc16;
	unsigned long start,end;
	unsigned long index_len;

	write_frame[0] = 0x52;          	// header low
	write_frame[1] = 0x42;          	// header high
	write_frame[2] = len & 0x00ff;  	// [Payload + crc16] Length low
	write_frame[3] = len >> 8;      	// [Payload + crc16] Length high
	write_frame[4] = comm;          	// Payload : Read command
	write_frame[5] = addr & 0x00ff; 	// Payload : address low
	write_frame[6] = addr>> 8;      	// Payload : address high
	write_frame[7] = info_frame[11];        // start memory index
	write_frame[8] = info_frame[12];        // start memory index
	write_frame[9] = info_frame[13];        // start memory index
	write_frame[10] = info_frame[14];       // start memory index
	end = info_frame[7] | (info_frame[8] << 8) | (info_frame[9] << 16) | (info_frame[10] << 24);
        start = info_frame[11] | (info_frame[12] << 8) | (info_frame[13] << 16) | (info_frame[14] << 24);
	index_len = ((end - start) + 1) * LEN_R_MEMDATA_ONE;
	write_frame[11] = info_frame[7];
	write_frame[12] = info_frame[8];
	write_frame[13] = info_frame[9];
	write_frame[14] = info_frame[10];
	crc16 = crc16_bit_calc(write_frame,LEN_W_MEMDATA-2);
	write_frame[15] = crc16 & 0x00ff;
	write_frame[16] = crc16 >> 8;

#ifdef DEBUG
	printf("in long_comm_create()\n");
	dump_buff(write_frame, LEN_W_MEMDATA);
#endif

	return (int)index_len;
}

/*
 * get memory data
 */
int get_memory_data(int fd, const char *csv_path) {
	static unsigned char write_frame[20];
	static unsigned char info_frame[LEN_R_MEMINFO];
	static unsigned char *read_frame;
	static unsigned char *frame_point;
	unsigned short crc16, checkcrc;
	int ret = 0;
	int read_len;
	FILE *output_file;

	// get memory information
	short_comm_create(write_frame, INFO_LEN, CMD_READ, INFO_ADDR);

	ret = communicate_command(fd, write_frame, LEN_W_MEMINFO, info_frame, LEN_R_MEMINFO);
	if (ret) {
		printf("command communication failed.\n");
		ret = -1;
		goto exit_free;
	}
#ifdef DEBUG
	printf("in get_memory_data() after short_comm_create() and communicate_command()\n");
	dump_buff(info_frame, LEN_R_MEMINFO);
#endif

	read_len = long_comm_create(write_frame, MEMORY_LEN, CMD_READ, MEMORY_ADDR, info_frame);
	read_frame = (unsigned char *)malloc(read_len);
	frame_point = read_frame;

	communicate_command(fd, write_frame, LEN_W_MEMDATA, read_frame, read_len);
#ifdef DEBUG
	printf("in get_memory_data() after long_comm_create() and communicate_command()\n");	
	dump_buff(read_frame, LEN_R_MEMDATA_ONE);
#endif

	if (csv_path != NULL) {
		output_file = fopen(csv_path, "w");
	} else {
		output_file = stdout;
	}
	if (output_file == NULL) {
		printf("output file open failed.");
		ret = -1;
		goto exit_free;
	}
	header_output(output_file);

	for (int i = 0; i < read_len / LEN_R_MEMDATA_ONE; i++) {
		// crc16 check
		crc16 = crc16_bit_calc(read_frame, LEN_R_MEMDATA_ONE - 2);
		checkcrc = read_frame[LEN_R_MEMDATA_ONE - 1] << 8 | read_frame[LEN_R_MEMDATA_ONE - 2];
		if (crc16 != checkcrc) {
			printf("crc16 check failed.\n");
			ret = -1;
			goto exit_close;
		}
		// The data existing in the response is from the 19th address.
		data_analyses(output_file, read_frame + 19);
		read_frame = read_frame + LEN_R_MEMDATA_ONE;
		if (csv_path == NULL && i >= 100) {
			printf("data output stop.\n");
			break;
		}
	}

exit_close:
	if (csv_path != NULL) {
		fclose(output_file);
	}
exit_free:
	free(frame_point);
	return ret;
}
