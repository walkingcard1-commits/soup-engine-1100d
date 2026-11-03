/** \file
 * Film stock row for the 50D Soup Engine.
 *
 * One Magic Lantern menu page. The wheel changes an integer through the
 * normal choices path. The name list is read once, from ML/FILM/INDEX.TXT
 * and each stock's NAME line. Nothing walks DCIM.
 */
#include "dryos.h"
#include "menu.h"
#include "fio-ml.h"
#include "soup_cam.h"

#ifndef SOUP_INDEX_BUF
#define SOUP_INDEX_BUF 8192
#endif

#define FILM_MAX   16
#define FILM_NAME  32

static int film_on;
static int film_delete;
static int film_stock;
static int film_loaded;
static int film_saved = -1;
static int film_saved_on = -1;
static int film_saved_del = -1;
static int film_count;
static char film_names[FILM_MAX][FILM_NAME];
static const char *film_choices[FILM_MAX];
static char film_source[8] = "JPEG";
static char film_size[8] = "HALF";

int film_is_on(void)
{
    return film_on;
}

/* Only meaningful while Film is on. Off never deletes a CR2. */
int film_wants_delete(void)
{
    return film_on && film_delete;
}

static void film_copy(char *dst, int n, const char *src)
{
    int i = 0;
    if (n <= 0) return;
    while (src[i] && i < n - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static int film_eq_ci(const char *a, const char *b)
{
    int i;
    for (i = 0; a[i] && b[i]; i++) {
        char ca = a[i], cb = b[i];
        if (ca >= 'a' && ca <= 'z') ca = (char)(ca - 'a' + 'A');
        if (cb >= 'a' && cb <= 'z') cb = (char)(cb - 'a' + 'A');
        if (ca != cb) return 0;
    }
    return a[i] == 0 && b[i] == 0;
}

/* NAME line in a stock file. Returns 1 if a name was stored. */
static int film_name_from_body(const char *body, int n, char *dst, int dstn)
{
    int i = 0;
    while (i < n) {
        int start = i;
        int k, o;
        while (i < n && body[i] != '\n' && body[i] != '\r') i++;
        if (i - start >= 4 &&
            body[start] == 'N' && body[start + 1] == 'A' &&
            body[start + 2] == 'M' && body[start + 3] == 'E') {
            k = start + 4;
            while (k < i && (body[k] == ' ' || body[k] == '\t')) k++;
            o = 0;
            /* The whole rest of the line: stocks are named "BLEACH BYPASS",
             * and develop matches the name in SE.CFG against the full one. */
            while (k < i && o < dstn - 1)
                dst[o++] = body[k++];
            while (o > 0 && (dst[o - 1] == ' ' || dst[o - 1] == '\t'))
                o--;
            dst[o] = 0;
            if (o > 0) return 1;
        }
        while (i < n && (body[i] == '\n' || body[i] == '\r')) i++;
    }
    return 0;
}

static void film_add_name(const char *name)
{
    if (film_count >= FILM_MAX || !name[0]) return;
    film_copy(film_names[film_count], FILM_NAME, name);
    film_choices[film_count] = film_names[film_count];
    film_count++;
}

static void film_read_index(void)
{
    static char text[SOUP_INDEX_BUF];
    static char body_small[1024];
    /* A stock's NAME line can sit past the first KB (soup800.fs opens with
     * a long comment), so read generously when memory allows. */
    char *body = fio_malloc(4096);
    int bodyn = 4096;
    char path[64];
    int got, i, line0;

    if (!body) { body = body_small; bodyn = sizeof(body_small); }

    film_count = 0;
    got = (int)read_file("ML/FILM/INDEX.TXT", text, sizeof(text) - 1);
    if (got <= 0)
        got = (int)read_file("A:/ML/FILM/INDEX.TXT", text, sizeof(text) - 1);
    if (got <= 0) {
        film_add_name("Standard");
        if (body != body_small) fio_free(body);
        return;
    }
    text[got] = 0;
    line0 = 0;
    for (i = 0; i <= got && film_count < FILM_MAX; i++) {
        char *line;
        int nbody;
        if (text[i] != '\n' && text[i] != '\r' && text[i] != 0) continue;
        text[i] = 0;
        line = text + line0;
        line0 = i + 1;
        if (!line[0] || line[0] == '#') continue;
        snprintf(path, sizeof(path), "ML/FILM/%s", line);
        nbody = (int)read_file(path, body, bodyn - 1);
        if (nbody <= 0) {
            snprintf(path, sizeof(path), "A:/ML/FILM/%s", line);
            nbody = (int)read_file(path, body, bodyn - 1);
        }
        if (nbody > 0) {
            body[nbody] = 0;
            if (film_name_from_body(body, nbody, film_names[film_count], FILM_NAME)) {
                film_choices[film_count] = film_names[film_count];
                film_count++;
                continue;
            }
        }
        film_add_name(line);
    }
    if (body != body_small) fio_free(body);
    if (film_count == 0)
        film_add_name("Standard");
}

static void film_read_cfg(void)
{
    static char text[256];
    char stock[FILM_NAME];
    int got, i, line0, n;

    stock[0] = 0;
    got = (int)read_file("ML/SE.CFG", text, sizeof(text) - 1);
    if (got <= 0)
        got = (int)read_file("A:/ML/SE.CFG", text, sizeof(text) - 1);
    if (got <= 0) return;
    text[got] = 0;
    line0 = 0;
    for (i = 0; i <= got; i++) {
        char *line;
        if (text[i] != '\n' && text[i] != '\r' && text[i] != 0) continue;
        text[i] = 0;
        line = text + line0;
        line0 = i + 1;
        if (!strncmp(line, "STOCK ", 6))
            film_copy(stock, FILM_NAME, line + 6);
        else if (!strncmp(line, "SOURCE ", 7))
            film_copy(film_source, sizeof(film_source), line + 7);
        else if (!strncmp(line, "SIZE ", 5))
            film_copy(film_size, sizeof(film_size), line + 5);
        else if (!strncmp(line, "FILM ", 5))
            film_on = film_eq_ci(line + 5, "ON");
        else if (!strncmp(line, "DELETE ", 7))
            film_delete = film_eq_ci(line + 7, "ON");
    }
    if (!stock[0]) return;
    for (n = 0; n < film_count; n++) {
        if (film_eq_ci(film_names[n], stock)) {
            film_stock = n;
            return;
        }
    }
}

static void film_write_cfg(void)
{
    char buf[160];
    int n;
    FILE *f;
    const char *name;

    if (film_count <= 0) return;
    if (film_stock < 0 || film_stock >= film_count) film_stock = 0;
    name = film_names[film_stock];
    if (!film_source[0]) film_copy(film_source, sizeof(film_source), "JPEG");
    if (!film_size[0]) film_copy(film_size, sizeof(film_size), "HALF");
    n = snprintf(buf, sizeof(buf),
                 "FILM %s\nDELETE %s\nSTOCK %s\nSOURCE %s\nSIZE %s\n",
                 film_on ? "ON" : "OFF",
                 film_delete ? "ON" : "OFF",
                 name, film_source, film_size);
    if (n <= 0 || n >= (int)sizeof(buf)) return;
    f = FIO_CreateFile("ML/SE.CFG");
    if (!f) f = FIO_CreateFile("A:/ML/SE.CFG");
    if (!f) return;
    FIO_WriteFile(f, buf, (size_t)n);
    FIO_CloseFile(f);
}

/* For the full-screen picker (film_picker.c). Choosing from it also turns
 * Film on, since picking a film is asking for it. */
int film_stock_total(void) { return film_count; }
const char *film_stock_label(int i)
{
    return (i >= 0 && i < film_count) ? film_names[i] : "";
}
int film_stock_selected(void) { return film_stock; }

/* The picker's OFF entry: stop developing, leave the chosen stock alone.
 * film_stock_choose() turns Film back on. */
void film_stock_off(void)
{
    film_on = 0;
    film_saved_on = film_on;
    film_write_cfg();
}

/* The stock to develop with, straight from memory. The develop task asks
 * this first so a pick never depends on SE.CFG having been rewritten. */
const char *film_stock_current(void)
{
    if (film_count <= 0 || film_stock < 0 || film_stock >= film_count)
        return "";
    return film_names[film_stock];
}
void film_stock_choose(int i)
{
    if (i < 0 || i >= film_count) return;
    film_stock = i;
    film_on = 1;
    film_saved = film_stock;
    film_saved_on = film_on;
    film_saved_del = film_delete;
    film_write_cfg();
}

static struct menu_entry film_menu[];

static MENU_UPDATE_FUNC(film_page_update)
{
    struct menu_entry *stock = &film_menu[2];

    if (!film_loaded) {
        film_read_index();
        film_read_cfg();
        if (film_stock < 0 || film_stock >= film_count) film_stock = 0;
        stock->choices = film_choices;
        stock->min = 0;
        stock->max = film_count > 0 ? film_count - 1 : 0;
        film_loaded = 1;
        film_saved = film_stock;
        film_saved_on = film_on;
        film_saved_del = film_delete;
        return;
    }
    if (film_stock < stock->min) film_stock = stock->min;
    if (film_stock > stock->max) film_stock = stock->max;
    if (film_on != film_saved_on || film_delete != film_saved_del ||
        film_stock != film_saved) {
        film_saved_on = film_on;
        film_saved_del = film_delete;
        film_saved = film_stock;
        film_write_cfg();
    }
}

static struct menu_entry film_menu[] = {
    {
        .name = "Film",
        .priv = &film_on,
        .max = 1,
        .choices = CHOICES("OFF", "ON"),
        .update = film_page_update,
        .help = "Off leaves every file alone.",
    },
    {
        .name = "Delete CR2",
        .priv = &film_delete,
        .max = 1,
        .choices = CHOICES("OFF", "ON"),
        .update = film_page_update,
        .help = "After FLM_nnnn.JPG exists, remove the CR2.",
    },
    {
        .name = "Stock",
        .priv = &film_stock,
        .min = 0,
        .max = 0,
        .choices = film_choices,
        .update = film_page_update,
        .help = "Look applied when developing. From ML/FILM.",
    },
};

void film_develop_start(void);

void film_menu_init(void)
{
    film_choices[0] = "Standard";
    film_read_index();
    film_read_cfg();
    if (film_stock < 0 || film_stock >= film_count) film_stock = 0;
    film_menu[2].choices = film_choices;
    film_menu[2].min = 0;
    film_menu[2].max = film_count > 0 ? film_count - 1 : 0;
    film_loaded = 1;
    film_saved = film_stock;
    film_saved_on = film_on;
    film_saved_del = film_delete;
    menu_add("Film", film_menu, COUNT(film_menu));
    film_develop_start();
}
