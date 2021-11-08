/* See LICENSE file for copyright and license details. */
#ifndef LIBTRACEBITMAP_H
#define LIBTRACEBITMAP_H

#include <stddef.h>
#include <stdint.h>

#define LIBTRACEBITMAP_INK_OFF 0
#define LIBTRACEBITMAP_INK_ON  1
/* Other values in (struct libtracebitmap_bitmap).image invoke undefined behaviour */

struct libtracebitmap_bitmap {
	size_t height;
	size_t width;
	uint8_t *image; /* (uint8_t [.height][.width]) */
};

int libtracebitmap_trace(
	struct libtracebitmap_bitmap *bitmap, /* image will be erased, but not deallocated */
	int (*new_component)(int negative, void *user_data),
	int (*new_stop)(size_t y, size_t x, void *user_data),
	int (*component_finished)(void *user_data),
	void *user_data);

#endif
