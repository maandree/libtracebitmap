/* See LICENSE file for copyright and license details. */
#include "libtracebitmap.h"

#include <stdlib.h>
#include <unistd.h>


#define CLEARED     0
#define SET         1
#define NEGATIVE    2
#define STATUS_MASK 0x03

#define TOP         0x10
#define RIGHT       0x20
#define BOTTOM      0x40
#define LEFT        0x80
#define EDGE_MASK   0xF0

#define INDEX(Y, X) ((size_t)(Y) * bitmap->width + (size_t)(X))


static uint8_t
read_pixel(struct libtracebitmap_bitmap *bitmap, ssize_t y, ssize_t x)
{
	if (y < 0 || x < 0 || (size_t)y >= bitmap->height || (size_t)x >= bitmap->width)
		return 0;
	return bitmap->image[INDEX(y, x)];
}


static int
test_pixel(struct libtracebitmap_bitmap *bitmap, ssize_t y, ssize_t x, uint8_t status)
{
	return (read_pixel(bitmap, y, x) & STATUS_MASK) == status;
}


static int
find_component(size_t *yp, size_t *xp, int *negativep, struct libtracebitmap_bitmap *bitmap)
{
	size_t y, x;
	uint8_t status;
	for (y = 0; y < bitmap->height; y++) {
		for (x = 0; x < bitmap->width; x++) {
			status = (bitmap->image[INDEX(y, x)] & STATUS_MASK);
			if (status != CLEARED) {
				*yp = y;
				*xp = x;
				*negativep = (status == NEGATIVE);
				return 1;
			}
		}
	}
	return 0;
}


static int
trace_edges(struct libtracebitmap_bitmap *bitmap, size_t tl_y, size_t tl_x, int negative,
            int (*new_stop)(size_t y, size_t x, void *user_data), void *user_data)
{
	uint8_t c = (negative ? NEGATIVE : SET);
	uint8_t edge = TOP;
	ssize_t y = (ssize_t)tl_y;
	ssize_t x = (ssize_t)tl_x;
	int r;
	if ((r = new_stop((size_t)y, (size_t)x, user_data)))
		return r;
	do {
		bitmap->image[INDEX(y, x)] |= edge;
		switch (edge) {
		case TOP:
			if (test_pixel(bitmap, y - 1, x + 1, c)) {
				if ((r = new_stop((size_t)y, (size_t)x + 1, user_data)))
					return r;
				x += 1;
				y -= 1;
				edge = LEFT;
			} else if (test_pixel(bitmap, y, x + 1, c)) {
				x += 1;
			} else {
				if ((r = new_stop((size_t)y, (size_t)x + 1, user_data)))
					return r;
				edge = RIGHT;
			}
			break;
		case RIGHT:
			if (test_pixel(bitmap, y + 1, x + 1, c)) {
				if ((r = new_stop((size_t)y + 1, (size_t)x + 1, user_data)))
					return r;
				x += 1;
				y += 1;
				edge = TOP;
			} else if (test_pixel(bitmap, y + 1, x, c)) {
				y += 1;
			} else {
				if ((r = new_stop((size_t)y + 1, (size_t)x + 1, user_data)))
					return r;
				edge = BOTTOM;
			}
			break;
		case BOTTOM:
			if (test_pixel(bitmap, y + 1, x - 1, c)) {
				if ((r = new_stop((size_t)y + 1, (size_t)x, user_data)))
					return r;
				x -= 1;
				y += 1;
				edge = RIGHT;
			} else if (test_pixel(bitmap, y, x - 1, c)) {
				x -= 1;
			} else {
				if ((r = new_stop((size_t)y + 1, (size_t)x, user_data)))
					return r;
				edge = LEFT;
			}
			break;
		case LEFT:
			if (test_pixel(bitmap, y - 1, x - 1, c)) {
				if ((r = new_stop((size_t)y, (size_t)x, user_data)))
					return r;
				x -= 1;
				y -= 1;
				edge = BOTTOM;
			} else if (test_pixel(bitmap, y - 1, x, c)) {
				y -= 1;
			} else {
				if ((r = new_stop((size_t)y, (size_t)x, user_data)))
					return r;
				edge = TOP;
			}
			break;
		default:
			abort();
		}
	} while (y != (ssize_t)tl_y || x != (ssize_t)tl_x || edge != TOP);
	return 0;
}


static void
erase_traced_component(struct libtracebitmap_bitmap *bitmap, int negative)
{
	size_t y, x;
	int inside;
	uint8_t c;
	for (y = 0; y < bitmap->height; y++) {
		inside = 0;
		for (x = 0; x < bitmap->width; x++) {
			c = bitmap->image[INDEX(y, x)];
			bitmap->image[INDEX(y, x)] &= (uint8_t)~EDGE_MASK;
			if (c & LEFT)
				inside = 1;
			if (inside) {
				bitmap->image[INDEX(y, x)] &= (uint8_t)~STATUS_MASK;
				if ((c & STATUS_MASK) != CLEARED)
					bitmap->image[INDEX(y, x)] |= CLEARED;
				else if (negative)
					bitmap->image[INDEX(y, x)] |= SET;
				else
					bitmap->image[INDEX(y, x)] |= NEGATIVE;
			}
			if (c & RIGHT)
				inside = 0;
		}
	}
}


int
libtracebitmap_trace(
	struct libtracebitmap_bitmap *bitmap,
	int (*new_component)(int negative, void *user_data),
	int (*new_stop)(size_t y, size_t x, void *user_data),
	int (*component_finished)(void *user_data),
	void *user_data)
{
	size_t y, x;
	int negative;
	int r;
	for (;;) {
		if (!find_component(&y, &x, &negative, bitmap))
			return 0;
		if ((r = new_component(negative, user_data)))
			return r;
		if ((r = trace_edges(bitmap, y, x, negative, new_stop, user_data)))
			return r;
		if ((r = component_finished(user_data)))
			return r;
		erase_traced_component(bitmap, negative);
	}
	return 0;
}
