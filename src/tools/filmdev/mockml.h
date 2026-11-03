/*
 * A stand-in for the few Magic Lantern calls film_develop.c makes, backed
 * by stdio. Lets the real film_develop.c run on the desktop against a
 * folder laid out like a card. Not a model of Canon's behaviour: no DMA
 * rules, no heap limits, no cache. It checks the code path and the bytes
 * it writes, which is all film_develop.c's logic depends on.
 */
#ifndef MOCKML_H
#define MOCKML_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

typedef FILE FILE_T;
#define MOCK_FILE FILE

#define fio_malloc(n)  malloc(n)
#define fio_free(p)    free(p)

static inline int is_file(const char *p) { struct stat st; return stat(p, &st) == 0; }
static inline size_t read_file(const char *p, void *buf, size_t n)
{
    FILE *f = fopen(p, "rb");
    size_t got;
    if (!f) return (size_t)-1;
    got = fread(buf, 1, n, f);
    fclose(f);
    return got;
}
static inline FILE *FIO_OpenFile(const char *p, int flags)
{ return fopen(p, (flags & O_RDWR) ? "r+b" : "rb"); }
static inline FILE *FIO_CreateFile(const char *p) { return fopen(p, "wb"); }
static inline int FIO_CloseFile(FILE *f) { return fclose(f); }
static inline int FIO_ReadFile(FILE *f, void *b, size_t n) { return (int)fread(b, 1, n, f); }
static inline int FIO_WriteFile(FILE *f, const void *b, size_t n) { return (int)fwrite(b, 1, n, f); }
static inline int64_t FIO_SeekSkipFile(FILE *f, int64_t off, int whence)
{ return fseek(f, (long)off, whence) == 0 ? off : -1; }
static inline int FIO_GetFileSize(const char *p, uint32_t *sz)
{ struct stat st; if (stat(p, &st) != 0) return -1; *sz = (uint32_t)st.st_size; return 0; }
static inline int FIO_RenameFile(const char *a, const char *b) { return rename(a, b); }
static inline int FIO_RemoveFile(const char *p) { return unlink(p); }
static inline void msleep(int ms) { usleep((useconds_t)ms * 1000); }
static inline int get_ms_clock(void) { return 0; }
static inline const char *get_dcim_dir(void) { return "DCIM/100CANON"; }
static inline const char *get_file_prefix(void) { return "IMG_"; }
static inline void NotifyBoxAt(int x, int y, int t, char *fmt, ...) { (void)x; (void)y; (void)t; (void)fmt; }
static inline void NotifyBoxHide(void) {}
struct card_info { int file_number; };
static inline struct card_info *get_shooting_card(void) { static struct card_info c = { 2 }; return &c; }
static inline void task_create(const char *n, int p, int s, void (*f)(void *), void *a)
{ (void)n; (void)p; (void)s; (void)f; (void)a; }

#endif
