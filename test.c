/* See LICENSE file for copyright and license details. */
#include "libtracebitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef DRAW_IMAGES
# define DRAW_IMAGES 0
#endif

#define WIDTH  96U
#define HEIGHT 64U
#define SIZE   (WIDTH * HEIGHT)

#define EXT_WIDTH  (WIDTH + 1U)
#define EXT_HEIGHT (HEIGHT + 1U)
#define EXT_SIZE   (EXT_WIDTH * EXT_HEIGHT)

#define TOP_COVERED    0x1
#define BOTTOM_COVERED 0x2
#define LEFT_COVERED   0x4
#define RIGHT_COVERED  0x8

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))



#define EXPECT(EXPR)\
	do {\
		if (EXPR) break;\
		fprintf(stderr, "Test failed at %s:%i: %s\n", __FILE__, __LINE__, #EXPR);\
		exit(1);\
	} while (0)


#define INIT_BITMAP(BITMAP)\
	do {\
		static uint8_t imagebuf_##BITMAP[SIZE];\
		BITMAP.height = HEIGHT;\
		BITMAP.width = WIDTH;\
		BITMAP.image = imagebuf_##BITMAP;\
	} while (0)



struct trace_ctx {
	const struct libtracebitmap_bitmap *in;
	struct libtracebitmap_bitmap *out;
	uint8_t edges[EXT_SIZE];
	int negative_draw;
	int have_prev;
	size_t prev_y;
	size_t prev_x;
};



static void
trace(
	const struct libtracebitmap_bitmap *bitmap,
	int (*new_component)(int negative, void *user_data),
	int (*new_stop)(size_t y, size_t x, void *user_data),
	int (*component_finished)(void *user_data),
	void *user_data)
{
	struct libtracebitmap_bitmap tmp;
	INIT_BITMAP(tmp);
	memcpy(tmp.image, bitmap->image, SIZE);
	EXPECT(!libtracebitmap_trace(&tmp, new_component, new_stop, component_finished, user_data));
}


static void
xor_bitmap(struct libtracebitmap_bitmap *a, const struct libtracebitmap_bitmap *b)
{
	size_t i;
	for (i = 0; i < SIZE; i++)
		a->image[i] ^= b->image[i];
}


static void
generate_spot(struct libtracebitmap_bitmap *map)
{
	size_t n = 1U + (size_t)rand() % MIN(SIZE, 128U);
	size_t yy, y = (size_t)rand() % HEIGHT;
	size_t xx, x = (size_t)rand() % WIDTH;
	ssize_t dy, dx;
	memset(map->image, LIBTRACEBITMAP_INK_OFF, SIZE);
	for (;;) {
		for (dy = y ? -1 : 0; dy <= 1; dy++) {
			for (dx = x ? -1 : 0; dx <= 1; dx++) {
				yy = (size_t)((ssize_t)y + dy);
				xx = (size_t)((ssize_t)x + dx);
				if (yy < HEIGHT && xx < WIDTH &&
				    map->image[yy * WIDTH + xx] == LIBTRACEBITMAP_INK_OFF) {
					map->image[yy * WIDTH + xx] = LIBTRACEBITMAP_INK_ON;
					if (!--n)
						return;
				}
			}
		}
		switch (rand() & 3) {
		case 0: if (y + 1U < HEIGHT) y++; break;
		case 1: if (y > 0)           y--; break;
		case 2: if (x + 1U < WIDTH)  x++; break;
		case 3: if (x > 0)           x--; break;
		}
	}
}


static void
generate_image(struct libtracebitmap_bitmap *map, unsigned nspots)
{
	struct libtracebitmap_bitmap spot;
	INIT_BITMAP(spot);
	memset(map->image, LIBTRACEBITMAP_INK_OFF, SIZE);
	while (nspots--) {
		generate_spot(&spot);
		xor_bitmap(map, &spot);
	}
}


static void
flip_row_right(struct libtracebitmap_bitmap *map, size_t y, size_t x)
{
	size_t i = y * WIDTH + x;
	size_t n = y * WIDTH + WIDTH;
	for (; i < n; i++)
		map->image[i] ^= 1;
}


static int
cb_new_component(int negative, void *user_data)
{
	struct trace_ctx *ctx = user_data;
	ctx->negative_draw = negative;
	ctx->have_prev = 0;
	return 0;
}


static int
cb_new_stop(size_t y, size_t x, void *user_data)
{
	size_t x0, y0, i, n;
	struct trace_ctx *ctx = user_data;
	uint8_t coverage;

	if (!ctx->have_prev)
		EXPECT(ctx->negative_draw == (ctx->in->image[y * WIDTH + x] == LIBTRACEBITMAP_INK_OFF));

	if (ctx->have_prev) {
		y0 = ctx->prev_y;
		x0 = ctx->prev_x;

		EXPECT((y == y0) ^ (x == x0));

		if (y == y0) {
			for (i = MIN(x, x0), n = MAX(x, x0); i <= n; i++) {
				coverage  = (i != MIN(x, x0)) * LEFT_COVERED;
				coverage |= (i != MAX(x, x0)) * RIGHT_COVERED;
				EXPECT(!(ctx->edges[y * EXT_WIDTH + i] & coverage));
				ctx->edges[y * EXT_WIDTH + i] |= coverage;
			}
		} else {
			for (i = MIN(y, y0), n = MAX(y, y0);; i++) {
				coverage  = (i != MIN(y, y0)) * TOP_COVERED;
				coverage |= (i != MAX(y, y0)) * BOTTOM_COVERED;
				EXPECT(!(ctx->edges[i * EXT_WIDTH + x] & coverage));
				ctx->edges[i * EXT_WIDTH + x] |= coverage;
				if (i == n)
					break;
				flip_row_right(ctx->out, i, x);
			}
		}
	}

	ctx->prev_y = y;
	ctx->prev_x = x;
	ctx->have_prev = 1;
	return 0;
}


static int
cb_component_finished(void *user_data)
{
	struct trace_ctx *ctx = user_data;
	ctx->have_prev = 0;
	return 0;
}


static void
trace_and_fill(const struct libtracebitmap_bitmap *in, struct libtracebitmap_bitmap *out)
{
	struct trace_ctx ctx;
	memset(&ctx, 0, sizeof(ctx));
	ctx.in = in;
	ctx.out = out;
	memset(out->image, LIBTRACEBITMAP_INK_OFF, SIZE);
	trace(in, cb_new_component, cb_new_stop, cb_component_finished, &ctx);
}


static int
maps_equal(const struct libtracebitmap_bitmap *a, const struct libtracebitmap_bitmap *b)
{
	return !memcmp(a->image, b->image, SIZE);
}


#if DRAW_IMAGES
static void
draw_ink(struct libtracebitmap_bitmap *map)
{
	static const char *syms[] = {" ", "▀", "\033[7m▀\033[m", "\033[7m \033[m"};
	size_t y, x;

	fprintf(stderr, "\n┌");
	for (x = 0; x < WIDTH; x++)
		fprintf(stderr, "─");
	fprintf(stderr, "┐\n");

# if 1 && !(HEIGHT & 1U)
	for (y = 0; y < HEIGHT; y += 2U) {
		fprintf(stderr, "│");
		for (x = 0; x < WIDTH; x++) {
			fprintf(stderr, "%s", syms[map->image[(y + 0U) * WIDTH + x] * 1 +
			                           map->image[(y + 1U) * WIDTH + x] * 2]);
		}
		fprintf(stderr, "│\n");
	}
# else
	for (y = 0; y < HEIGHT; y++) {
		fprintf(stderr, "│");
		for (x = 0; x < WIDTH; x++)
			fputc(map->image[y * WIDTH + x] ? 'x' : '-', stderr);
		fprintf(stderr, "│\n");
	}
# endif

	fprintf(stderr, "└");
	for (x = 0; x < WIDTH; x++)
		fprintf(stderr, "─");
	fprintf(stderr, "┘\n");
}
#endif


int
main(void)
{
	struct libtracebitmap_bitmap in, out;
	unsigned i, n;

	srand((unsigned int)time(NULL));

	INIT_BITMAP(in);
	INIT_BITMAP(out);

	generate_image(&in, 0);
	trace_and_fill(&in, &out);
	EXPECT(maps_equal(&in, &out));

	for (n = 1; n < 32U; n++) {
		for (i = 0; i < 100U; i++) {
			generate_image(&in, n);
#if DRAW_IMAGES
			draw_ink(&in);
#endif
			trace_and_fill(&in, &out);
			EXPECT(maps_equal(&in, &out));
		}
	}

	return 0;
}
