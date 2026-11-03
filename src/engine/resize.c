/*
 * resize.c - streaming bilinear enlarger. See resize.h.
 *
 * Output pixel (x, y) samples the source at
 *     sx = x * (sw - 1) / (dw - 1)      sy = y * (sh - 1) / (dh - 1)
 * so the first and last columns and rows land exactly on the source's own
 * edges. Positions are kept as an integer index plus a remainder, and the
 * remainder is turned into a weight out of 256 once per column / row. The
 * ARM946E-S has no divide instruction, so nothing divides per pixel.
 */
#include "resize.h"

struct se_resize {
    int sw, sh, dw, dh;
    int nsrc;               /* source rows pushed so far          */
    int ty;                 /* next output row to produce         */
    uint16_t *xi;           /* left source column for each output column  */
    uint16_t *xw;           /* weight of the right neighbour, 0..256      */
    uint8_t  *buf[2];       /* the last two source rows, already dw wide  */
    uint8_t  *prev, *cur;
    uint8_t  *out;          /* one finished output row                    */
};

static uint32_t rnd_div(uint32_t n, uint32_t d)
{
    return (n + d / 2) / d;
}

se_err se_resize_begin(se_resize **outp, int sw, int sh, int dw, int dh,
                       se_arena *a)
{
    se_resize *r;
    int x;
    size_t row;

    if (!outp) return SE_E_UNSUPPORTED;
    *outp = 0;
    if (sw < 1 || sh < 1 || dw < 2 || dh < 2) return SE_E_UNSUPPORTED;
    if (dw < sw || dh < sh) return SE_E_UNSUPPORTED;
    if (dw > 0xffff || sw > 0xffff) return SE_E_UNSUPPORTED;

    r = SE_NEW(a, se_resize, 1);
    if (!r) return SE_E_NOMEM;
    row = (size_t)dw * 3;
    r->xi = SE_NEW(a, uint16_t, dw);
    r->xw = SE_NEW(a, uint16_t, dw);
    r->buf[0] = SE_NEW(a, uint8_t, row);
    r->buf[1] = SE_NEW(a, uint8_t, row);
    r->out = SE_NEW(a, uint8_t, row);
    if (!r->xi || !r->xw || !r->buf[0] || !r->buf[1] || !r->out)
        return SE_E_NOMEM;

    r->sw = sw; r->sh = sh; r->dw = dw; r->dh = dh;
    r->nsrc = 0;
    r->ty = 0;
    r->prev = r->buf[0];
    r->cur = r->buf[1];

    for (x = 0; x < dw; x++) {
        uint32_t n = (uint32_t)x * (uint32_t)(sw - 1);
        uint32_t d = (uint32_t)(dw - 1);
        uint32_t i = n / d;
        uint32_t w = rnd_div((n - i * d) * 256u, d);
        r->xi[x] = (uint16_t)i;
        r->xw[x] = (uint16_t)w;
    }
    *outp = r;
    return SE_OK;
}

/* Source row to a dw-wide row. */
static void hscale(const se_resize *r, const uint8_t *src, uint8_t *dst)
{
    int x, last = r->sw - 1;
    for (x = 0; x < r->dw; x++) {
        int i = r->xi[x];
        int j = i < last ? i + 1 : last;
        uint32_t w = r->xw[x], iw = 256u - w;
        const uint8_t *a = src + i * 3;
        const uint8_t *b = src + j * 3;
        dst[0] = (uint8_t)((a[0] * iw + b[0] * w + 128u) >> 8);
        dst[1] = (uint8_t)((a[1] * iw + b[1] * w + 128u) >> 8);
        dst[2] = (uint8_t)((a[2] * iw + b[2] * w + 128u) >> 8);
        dst += 3;
    }
}

se_err se_resize_row(se_resize *r, const uint8_t *src, se_resize_emit emit,
                     void *ctx)
{
    uint8_t *t;
    int s;

    if (!r || !src || r->nsrc >= r->sh) return SE_E_CORRUPT;
    t = r->prev; r->prev = r->cur; r->cur = t;
    hscale(r, src, r->cur);
    s = r->nsrc++;

    while (r->ty < r->dh) {
        uint32_t n = (uint32_t)r->ty * (uint32_t)(r->sh - 1);
        uint32_t d = (uint32_t)(r->dh - 1);
        int v0 = (int)(n / d);
        uint32_t w = rnd_div((n - (uint32_t)v0 * d) * 256u, d);
        const uint8_t *row;
        int need = w ? v0 + 1 : v0;

        if (need > s) break;
        if (w == 0) {
            row = r->cur;           /* v0 == s: enlarging never skips a row */
        } else {
            uint32_t iw = 256u - w;
            int k, n3 = r->dw * 3;
            for (k = 0; k < n3; k++)
                r->out[k] = (uint8_t)((r->prev[k] * iw + r->cur[k] * w + 128u) >> 8);
            row = r->out;
        }
        if (emit(ctx, row, r->dw) != 0) return SE_E_IO;
        r->ty++;
    }
    return SE_OK;
}

se_err se_resize_end(const se_resize *r)
{
    if (!r) return SE_E_CORRUPT;
    return (r->nsrc == r->sh && r->ty == r->dh) ? SE_OK : SE_E_CORRUPT;
}
