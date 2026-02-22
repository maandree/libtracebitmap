/* See LICENSE file for copyright and license details. */
#ifndef LIBTRACEBITMAP_H
#define LIBTRACEBITMAP_H

#include <stddef.h>
#include <stdint.h>


/**
 * The background is shown for pixels with this value
 */
#define LIBTRACEBITMAP_INK_OFF 0

/**
 * The foreground is shown for pixels with this value
 */
#define LIBTRACEBITMAP_INK_ON  1


/**
 * Bitmap
 */
struct libtracebitmap_bitmap {
	/**
	 * The height of the bitmap, in pixels
	 */
	size_t height;

	/**
	 * The width of the bitmap, in pixels
	 */
	size_t width;

	/**
	 * The bitmap
	 *
	 * Row-major order oriented and non-packed, meaning
	 * that for an y-position `y` and an x-position `x`,
	 * `.image[y * .width + x]` returns the pixel value,
	 * which must either be `LIBTRACEBITMAP_INK_OFF` or
	 * `LIBTRACEBITMAP_INK_ON`
	 */
	uint8_t *image;
};


/**
 * Vectorise a bitmap
 *
 * @param  bitmap              The bitmap to vectorise; `bitmap->image` will
 *                             be erased, but not deallocated
 * @param  new_component       Callback function that will be called each time the function
 *                             starts vectorising a separate component in the bitmap; the
 *                             first argument will be the 0 if the component shall be drawn,
 *                             and 1 if the component shall erase from a drawn component;
 *                             the second argument will be `user_data`; assuming the bitmap
 *                             is not empty, this will be the first callback the function calls;
 *                             the callback function shall return 0 upon successful completion
 *                             and any other value on failure
 * @param  new_stop            Callback function that is called each time the function finds
 *                             a node for the vector image; the first argument will be the
 *                             node's y-position (0 at the top, positive downwards); the second
 *                             argument will be the node's x-position (0 at the far left,
 *                             positive rightwards); the third argument will be `user_data`;
 *                             the callback function shall return 0 upon successful completion
 *                             and any other value on failure: Lines between adjacent nodes
 *                             within the same component (the initial and the final nodes are
 *                             adjacent) are always either horizontal or vertical, with no
 *                             other nodes between the them.
 * @param  component_finished  Callback function that is called each time the function is
 *                             finished with a component. The function will only vectorise
 *                             one component at a time, so after calling this callback, the
 *                             function will either return or immediately call `*new_component`,
 *                             and after calling `*new_component`, it will never `*new_component`
 *                             again before calling `*component_finished`; additionally,
 *                             `*component_finished` is called once for every call to
 *                             `*new_component`; the callback function shall return 0 upon
 *                             successful completion and any other value on failure
 * @param   user_data          Value to pass as the last argument for each callback function
 * @return                     0 upon success, and the value returned by a callback function
 *                             if it returns a non-zero value
 */
int libtracebitmap_trace(
	struct libtracebitmap_bitmap *bitmap,
	int (*new_component)(int negative, void *user_data),
	int (*new_stop)(size_t y, size_t x, void *user_data),
	int (*component_finished)(void *user_data),
	void *user_data);

#endif
