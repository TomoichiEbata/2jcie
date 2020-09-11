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

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

/*
 * header output
 */
void header_output(FILE *fd) {
	fprintf(fd, "Temperature, Relative humidity, Ambient light, Barometric pressure, Sound noise, eTVOC, eCO2, Discomfort index, Heat stroke\n");
}

/*
 * usb data output
 */
void usb_data_output(FILE *fd, struct senser_data_t sensor_data) {
	fprintf(fd,"%5.2f,%5.2f,%d,%8.3lf,%5.2f,%d,%d,%5.2f,%5.2f\n",
		sensor_data.temp, sensor_data.humid, sensor_data.light, sensor_data.press,
		sensor_data.noise, sensor_data.TVOC, sensor_data.CO2, sensor_data.discom, sensor_data.heat);
}
