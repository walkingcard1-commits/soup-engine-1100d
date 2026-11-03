/*
 * Runs the real film_develop.c on the desktop. Call:
 *
 *     filmdevtest <card-dir>
 *
 * <card-dir> holds ML/FILM/INDEX.TXT, a stock, ML/SE.CFG, and
 * DCIM/100CANON/IMG_0001.CR2. This develops frame 1 exactly as the camera's
 * task would, and exits non-zero if it did not produce FLM_0001.JPG.
 */
#include "film_develop.c"

int ml_started = 1;
int film_is_on(void) { return 1; }
int film_wants_delete(void) { return 0; }

const char *film_stock_current(void) { return ""; }

int main(int argc, char **argv)
{
    int rc;
    if (argc < 2 || chdir(argv[1]) != 0) {
        fprintf(stderr, "usage: filmdevtest <card-dir>\n");
        return 2;
    }
    rc = film_one(1, 2);
    printf("film_one -> %d\n", rc);
    return rc == 1 ? 0 : 1;
}
