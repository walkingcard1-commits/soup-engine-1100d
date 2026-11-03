/*
 * resize.h - streaming bilinear enlarger for RGB8 rows.
 *
 * Exists for bodies whose Canon preview, halved by the decoder, comes out
 * smaller than the one size their playback will accept. The EOS 1100D is
 * the case: its preview is 4272x2848, half of that is 2136x1424, and its
 * Small 1 JPEG is 2256x1504. The 50D's half-size preview is larger than
 * its playback size and is cropped instead; this is the other direction.
 *
 * Rows go in and rows come out, a pair of lines held at a time, so it sits
 * in the same band-at-a-time pipeline as the look and the encoder. Integer
 * only: the horizontal pass runs once per source row, the vertical pass
 * once per output row, both with 8-bit weights.
 *
 * Enlarging only. The destination must be at least as wide and as tall as
 * the source, which is what guarantees every source row is seen before the
 * next output row needs it.
 */
#ifndef SE_RESIZE_H
#define SE_RESIZE_H

#include "se.h"

typedef struct se_resize se_resize;

/* Receives one finished output row of `width` RGB8 pixels. 0 on success. */
typedef int (*se_resize_emit)(void *ctx, const uint8_t *row, int width);

/*
 * sw x sh in, dw x dh out. dw >= sw, dh >= sh, and both destination
 * dimensions at least 2. Takes about 3 * dw * 3 + dw * 4 bytes from the
 * arena, plus the struct.
 */
se_err se_resize_begin(se_resize **out, int sw, int sh, int dw, int dh,
                       se_arena *a);

/* Push the next source row, top to bottom. Calls emit for each output row
 * that this row completes, which is none, one, or occasionally two. */
se_err se_resize_row(se_resize *r, const uint8_t *src, se_resize_emit emit,
                     void *ctx);

/* After the last source row: SE_OK only if every output row was emitted. */
se_err se_resize_end(const se_resize *r);

#endif
