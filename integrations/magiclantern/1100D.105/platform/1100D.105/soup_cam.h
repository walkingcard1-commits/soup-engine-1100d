/* Per-body constants for the Soup Engine Film page. EOS 1100D, firmware 1.0.5. */
#ifndef SOUP_CAM_H
#define SOUP_CAM_H

#define SOUP_CAM_MODEL "Canon EOS 1100D"

/*
 * The 1100D's Small 1 JPEG, taken from Canon's published sizes. Its
 * preview is 4272x2848; halved by the decoder that is 2136x1424, smaller
 * than this, so the picture is enlarged by about 5.6% to fit (see
 * engine/resize.c). Not yet confirmed on a body: if playback of the
 * developed raw misbehaves, this is the number to check first.
 */
#define SOUP_PLAY_W 2256
#define SOUP_PLAY_H 1504

/* A 12 MP, 12-bit CR2 from this body runs about 10 to 15 MB. The 50D's
 * 6 MB floor is the same fraction of its 15 MP, 14-bit files. */
#define SOUP_MIN_CR2 (4u * 1024 * 1024)

/* Static buffer for reading ML/FILM/INDEX.TXT. The 1100D has no room to
 * spare, and sixteen file names fit in far less than the default 8 KB. */
#define SOUP_INDEX_BUF 2048

#endif
