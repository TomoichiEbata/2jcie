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

#ifndef __COMMON__
#define __COMMON__

// output data mode
#define MODE_LATEST		(0)
#define MODE_MEMDATA		(1)
#define MAX_MODE_NUM		(1)

// #define DEBUG		

// constant
#define SERIAL_BAUDRATE		(B115200)

struct senser_data_t {
	float temp;
	float humid;
	int light;
	double press;
	float noise;
	int TVOC;
	int CO2;
	float discom;
	float heat;
};

#endif /* __MAIN__ */
