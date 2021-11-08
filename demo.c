/* See LICENSE file for copyright and license details. */
#include "libtracebitmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))


struct data {
	size_t height;
	size_t width;
	size_t y;
	size_t x;
	int beginning;
	const char ***plot;
};


static int
new_component(int negative, void *user_data)
{
	struct data *data = user_data;
	data->beginning = 1;
	printf("%s", negative ? "-" : "+");
	fflush(stdout);
	return 0;
}

static const char *
mix(const char *a, const char *b)
{
	static const struct {
		const char *text;
		uint16_t bits;
	} symbols[] = {
		{" ", 0x0000}, {"┼", 0x1111},
		{"╵", 0x1000}, {"┬", 0x0111},
		{"╶", 0x0100}, {"┤", 0x1011},
		{"╷", 0x0010}, {"┴", 0x1101},
		{"╴", 0x0001}, {"├", 0x1110},
		{"│", 0x1010}, {"─", 0x0101},
		{"└", 0x1100}, {"┐", 0x0011},
		{"┘", 0x1001}, {"┌", 0x0110}
	};
	uint16_t bits;
	size_t i, j;
	for (i = 0; strcmp(a, symbols[i].text); i++);
	for (j = 0; strcmp(b, symbols[j].text); j++);
	bits = symbols[i].bits | symbols[j].bits;
	for (i = 0; bits != symbols[i].bits; i++);
	return symbols[i].text;
}

static int
new_stop(size_t y, size_t x, void *user_data)
{
	struct data *data = user_data;
	size_t y0, y1, y2;
	size_t x0, x1, x2;
	printf(" (%zu,%zu)", y, x);
	fflush(stdout);
	if (data->beginning) {
		data->beginning = 0;
		data->y = y;
		data->x = x;
	} else {
		y0 = MIN(data->y, y);
		x0 = MIN(data->x, x);
		y2 = MAX(data->y, y);
		x2 = MAX(data->x, x);
		if (y0 == y2) {
			y1 = y0;
			for (x1 = x0; x1 < x2; x1++) {
				data->plot[y1 * 2][x1 * 2 + 0] = mix(data->plot[y1 * 2][x1 * 2 + 0], "╶");
				data->plot[y1 * 2][x1 * 2 + 1] = mix(data->plot[y1 * 2][x1 * 2 + 1], "─");
				data->plot[y1 * 2][x1 * 2 + 2] = mix(data->plot[y1 * 2][x1 * 2 + 2], "╴");
			}
		} else {
			x1 = x0;
			for (y1 = y0; y1 < y2; y1++) {
				data->plot[y1 * 2 + 0][x1 * 2] = mix(data->plot[y1 * 2 + 0][x1 * 2], "╷");
				data->plot[y1 * 2 + 1][x1 * 2] = mix(data->plot[y1 * 2 + 1][x1 * 2], "│");
				data->plot[y1 * 2 + 2][x1 * 2] = mix(data->plot[y1 * 2 + 2][x1 * 2], "╵");
			}
		}
		data->y = y;
		data->x = x;
	}
	return 0;
}


static int
component_finished(void *user_data)
{
	(void) user_data;
	printf("\n");
	fflush(stdout);
	return 0;
}


int
main(void)
{
	struct libtracebitmap_bitmap bitmap;
	size_t size = 0;
	size_t len = 0;
	ssize_t rd;
	size_t width, i, j;
	size_t y, x;
	int r;
	const char ***plot;
	struct data data;

	bitmap.image = NULL;
	for (;;) {
		if (len == size) {
			size += 1024;
			bitmap.image = realloc(bitmap.image, size);
			if (!bitmap.image) {
				perror("realloc");
				exit(1);
			}
		}
		rd = read(STDIN_FILENO, &bitmap.image[len], size - len);
		if (rd <= 0) {
			if (!rd)
				break;
			perror("read");
			exit(1);
		}
		len += (size_t)rd;
	}

	bitmap.height = 0;
	bitmap.width = 0;
	width = 0;
	for (i = j = 0; j < len; j++) {
		if ((char)bitmap.image[j] == '\n') {
			if (!bitmap.height++) {
				bitmap.width = width;
			} else {
				if (bitmap.width != width) {
					fprintf(stderr, "Invalid input: each line must have the same length.\n");
					exit(1);
				}
			}
			width = 0;
		} else if ((char)bitmap.image[j] == '.') {
			bitmap.image[i++] = LIBTRACEBITMAP_INK_OFF;
			width += 1;
		} else if ((char)bitmap.image[j] == 'x') {
			bitmap.image[i++] = LIBTRACEBITMAP_INK_ON;
			width += 1;
		} else {
			fprintf(stderr, "Invalid input: may only contain '.', 'x', and <newline> characters.\n");
			exit(1);
		}
	}
	if (width) {
		fprintf(stderr, "Invalid input: must end with <newline> character.\n");
		exit(1);
	}

	plot = calloc(bitmap.height * 2 + 1, sizeof(*plot));
	for (y = 0; y < bitmap.height * 2 + 1; y++) {
		plot[y] = calloc(bitmap.width * 2 + 1, sizeof(**plot));
		for (x = 0; x < bitmap.width * 2 + 1; x++)
			plot[y][x] = " ";
	}
	for (y = 0, i = 0; y < bitmap.height; y++)
		for (x = 0; x < bitmap.width; x++, i++)
			plot[y * 2 + 1][x * 2 + 1] = (bitmap.image[i] == LIBTRACEBITMAP_INK_ON ? "x" : ".");

	data.height = bitmap.height;
	data.width  = bitmap.width;
	data.plot   = plot;

	r = libtracebitmap_trace(&bitmap, new_component, new_stop, component_finished, &data);
	if (r)
		exit(r);

	for (y = 0; y < bitmap.height * 2 + 1; y++) {
		for (x = 0; x < bitmap.width * 2 + 1; x++)
			printf("%s", plot[y][x]);
		printf("\n");
		free(plot[y]);
	}

	free(plot);
	free(bitmap.image);
	return 0;
}
