/** \file
 * Full-screen film picker, the one from the original Soup Engine.
 *
 * Trash opens it from the shooting screen. The wheel or the arrow keys
 * move through the stocks, SET picks one (and turns Film on), Trash or
 * MENU closes it.
 *
 * It lives inside Magic Lantern's own menu mode rather than beside it.
 * Opening it opens the ML menu, which is what brings the screen up and
 * keeps Canon's UI out of the way; menu_redraw_do then draws this instead
 * of the menu pages, and handle_ml_menu_keys hands it the buttons. All
 * three are plain Magic Lantern entry points, the ones stock ML uses on
 * every button press. None of it touches Canon's GUI hook, which is the
 * thing the 50D could not survive (see PITFALLS.md).
 *
 * The drawing is engine/ui.c, built with SE_UI_STILLS: the same 720x240
 * 16-colour picture the 5D Classic shows, pixel-doubled down the screen.
 * Its own sixteen colours are loaded into palette slots 0xE0..0xEF, which
 * Magic Lantern never draws with, and put back on close.
 */
#include "dryos.h"
#include "mem.h"
#include "bmp.h"
#include "menu.h"
#include "gui.h"
#include "fio-ml.h"
#include "propvalues.h"

#include "ui.h"

extern int film_stock_total(void);
extern const char *film_stock_label(int i);
extern int film_stock_selected(void);
extern void film_stock_choose(int i);

#define PICK_MAX    16
#define PICK_DESC   48
#define PICK_PAL0   0xE0

/* Colours 1..14 of the original panel; 0 is clear, 15 is white. The
 * opacity byte is taken from the camera's own black at run time. */
static const uint32_t pick_yuv[16] = {
    0x00000000, 0x00170003, 0x0032FE06, 0x004FFD08,
    0x0071FD0B, 0x0094FF0D, 0x00C7FE0F, 0x00D8000E,
    0x0048EE25, 0x0061E836, 0x008CD72D, 0x00A9D51B,
    0x006FF1FD, 0x005911EB, 0x0060071A, 0x00EB0000,
};

static volatile int pick_want;       /* the picker is, or is about to be, up */
static volatile int pick_busy;       /* a draw is running */
static uint8_t *pick_fb;             /* 720x240, 4 bits a pixel */
static char *pick_desc;              /* PICK_MAX * PICK_DESC */
static se_ui pick_ui;
static se_ui_stock pick_stock[PICK_MAX];

int film_picker_open(void)
{
    return pick_want;
}

static void pick_palette(int on)
{
    int i;
    if (!DISPLAY_IS_ON) return;
    if (on) {
        uint32_t op = LCD_Palette[3 * COLOR_BLACK + 2] & 0xFF000000;
        for (i = 1; i < 16; i++) {
            uint32_t w = op | (pick_yuv[i] & 0x00FFFFFF);
            EngDrvOut(LCD_Palette[3 * (PICK_PAL0 + i)], w);
            EngDrvOut(LCD_Palette[3 * (PICK_PAL0 + i) + 0x300], w);
        }
    } else {
        for (i = 1; i < 16; i++) {
            int c = PICK_PAL0 + i;
            EngDrvOut(LCD_Palette[3 * c], LCD_Palette[3 * c + 2]);
            EngDrvOut(LCD_Palette[3 * c + 0x300], LCD_Palette[3 * c + 2]);
        }
    }
}

/* DESC line of a stock file, into dst. */
static void pick_read_desc(const char *body, int n, char *dst)
{
    int i = 0;
    dst[0] = 0;
    while (i < n) {
        int start = i, k, o = 0;
        while (i < n && body[i] != '\n' && body[i] != '\r') i++;
        if (i - start >= 4 && body[start] == 'D' && body[start + 1] == 'E' &&
            body[start + 2] == 'S' && body[start + 3] == 'C') {
            k = start + 4;
            while (k < i && (body[k] == ' ' || body[k] == '\t')) k++;
            while (k < i && o < PICK_DESC - 1) dst[o++] = body[k++];
            while (o > 0 && (dst[o - 1] == ' ' || dst[o - 1] == '\t')) o--;
            dst[o] = 0;
            return;
        }
        while (i < n && (body[i] == '\n' || body[i] == '\r')) i++;
    }
}

/* Names come from the Film page's list; descriptions from the same files,
 * in the same order, so entry i here is entry i there. */
static int pick_load(void)
{
    char *index, *body, path[64];
    int n = film_stock_total(), got, i, line0, k = 0;

    if (n <= 0) return 0;
    if (n > PICK_MAX) n = PICK_MAX;
    pick_fb = malloc(SE_UI_FB_SIZE + PICK_MAX * PICK_DESC);
    if (!pick_fb) return 0;
    pick_desc = (char *)pick_fb + SE_UI_FB_SIZE;
    memset(pick_desc, 0, PICK_MAX * PICK_DESC);

    index = fio_malloc(2048);
    body = fio_malloc(4096);
    if (index && body) {
        got = (int)read_file("ML/FILM/INDEX.TXT", index, 2047);
        if (got <= 0) got = (int)read_file("A:/ML/FILM/INDEX.TXT", index, 2047);
        if (got > 0) {
            index[got] = 0;
            line0 = 0;
            for (i = 0; i <= got && k < n; i++) {
                char *line;
                int nb;
                if (index[i] != '\n' && index[i] != '\r' && index[i] != 0) continue;
                index[i] = 0;
                line = index + line0;
                line0 = i + 1;
                if (!line[0] || line[0] == '#') continue;
                snprintf(path, sizeof(path), "ML/FILM/%s", line);
                nb = (int)read_file(path, body, 4095);
                if (nb <= 0) {
                    snprintf(path, sizeof(path), "A:/ML/FILM/%s", line);
                    nb = (int)read_file(path, body, 4095);
                }
                if (nb > 0) pick_read_desc(body, nb, pick_desc + k * PICK_DESC);
                k++;
            }
        }
    }
    if (index) fio_free(index);
    if (body) fio_free(body);

    for (i = 0; i < n; i++) {
        pick_stock[i].name = film_stock_label(i);
        pick_stock[i].desc = pick_desc + i * PICK_DESC;
    }
    se_ui_init(&pick_ui);
    se_ui_set_stocks(&pick_ui, pick_stock, n);
    pick_ui.stock = film_stock_selected();
    pick_ui.ready = 1;
    pick_ui.screen = SE_SCREEN_STOCK;
    return 1;
}

void film_picker_draw(void)
{
    se_ui_fb fb;
    uint8_t *vram;
    int y, x;

    if (!pick_fb && !pick_load()) {
        /* nothing to show: back out rather than leave a blank screen */
        pick_want = 0;
        gui_stop_menu();
        return;
    }
    pick_busy = 1;
    fb.px = pick_fb;
    se_ui_draw(&pick_ui, &fb);
    pick_palette(1);

    vram = bmp_vram();
    if (vram) {
        for (y = 0; y < SE_UI_H; y++) {
            const uint8_t *src = pick_fb + y * SE_UI_PITCH;
            uint8_t *d0 = vram + BM(0, 2 * y);
            uint8_t *d1 = vram + BM(0, 2 * y + 1);
            for (x = 0; x < SE_UI_PITCH; x++) {
                uint8_t b = src[x];
                uint8_t lo = (uint8_t)(b & 0x0F);
                uint8_t hi = (uint8_t)(b >> 4);
                uint8_t a = lo ? (uint8_t)(PICK_PAL0 + lo) : 0;
                uint8_t c = hi ? (uint8_t)(PICK_PAL0 + hi) : 0;
                d0[2 * x] = d1[2 * x] = a;
                d0[2 * x + 1] = d1[2 * x + 1] = c;
            }
        }
        ml_refresh_display_needed = 1;
    }
    pick_busy = 0;
}

/* Called as the ML menu closes, from whatever closed it. */
void film_picker_reset(void)
{
    int waited = 0;
    if (!pick_want && !pick_fb) return;
    pick_want = 0;
    while (pick_busy && waited++ < 100) msleep(10);
    pick_palette(0);
    if (pick_fb) {
        uint8_t *fb = pick_fb;
        pick_fb = 0;
        pick_desc = 0;
        free(fb);
    }
}

static int pick_can_open(void)
{
    if (!DISPLAY_IS_ON || sensor_cleaning) return 0;
    if (PLAY_MODE || MENU_MODE) return 0;        /* Trash means delete there */
    if (get_halfshutter_pressed()) return 0;
    return film_stock_total() > 0;
}

static void pick_step(int d)
{
    se_ui_key_press(&pick_ui, d < 0 ? SE_KEY_LEFT : SE_KEY_RIGHT);
    menu_redraw();
}

static void pick_close(void)
{
    pick_want = 0;
    gui_stop_menu();
}

/*
 * Every button event passes through here from handle_ml_menu_keys.
 * 0 means we took it, so neither ML's menu nor Canon sees it.
 */
int film_picker_event(struct event *event, int menu_is_shown)
{
    int k;
    if (event->type != 0) return 1;
    k = event->param;

    if (!pick_want) {
        if (k == BGMT_TRASH && !menu_is_shown && pick_can_open()) {
            pick_want = 1;
            gui_open_menu();
            return 0;
        }
        return 1;
    }
    if (!menu_is_shown) return 1;               /* still opening */

    if (k == BGMT_WHEEL_LEFT || k == BGMT_PRESS_LEFT ||
        k == BGMT_WHEEL_UP || k == BGMT_PRESS_UP)       { pick_step(-1); return 0; }
    if (k == BGMT_WHEEL_RIGHT || k == BGMT_PRESS_RIGHT ||
        k == BGMT_WHEEL_DOWN || k == BGMT_PRESS_DOWN)   { pick_step(1); return 0; }
    if (k == BGMT_PRESS_SET) {
        film_stock_choose(pick_ui.stock);
        pick_close();
        return 0;
    }
    if (k == BGMT_TRASH || k == BGMT_MENU || k == BGMT_PLAY) {
        pick_close();
        return 0;
    }
    if (k == BGMT_UNPRESS_SET || k == BGMT_UNPRESS_LEFT ||
        k == BGMT_UNPRESS_RIGHT || k == BGMT_UNPRESS_UP ||
        k == BGMT_UNPRESS_DOWN)
        return 0;
    return 1;           /* half-shutter and the rest keep their usual meaning */
}
