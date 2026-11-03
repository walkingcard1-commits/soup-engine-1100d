#include "all_features.h"

#define FEATURE_LV_3RD_PARTY_FLASH
#define FEATURE_EYEFI_TRICKS


// No MLU on the 1100D
#undef FEATURE_MLU
#undef FEATURE_MLU_HANDHELD

// Disable almost all audio stuff
#undef FEATURE_WAV_RECORDING
#undef FEATURE_FPS_WAV_RECORD
#undef FEATURE_VOICE_TAGS
#undef FEATURE_AUDIO_REMOTE_SHOT

// No DISP button
#undef FEATURE_ARROW_SHORTCUTS

#define FEATURE_INTERMEDIATE_ISO_PHOTO_DISPLAY

// disabled, because autoexec.bin gets too big and does not boot
#undef FEATURE_JUNKIE_MENU
#undef FEATURE_STICKY_HALFSHUTTER
#undef FEATURE_SHOW_TASKS
#undef FEATURE_SHOW_CPU_USAGE
#undef FEATURE_SHOW_GUI_EVENTS
#undef FEATURE_SHOW_IMAGE_BUFFERS_INFO
#undef FEATURE_SHOW_SHUTTER_COUNT
#undef FEATURE_SHOW_CMOS_TEMPERATURE
#undef FEATURE_SHOW_EDMAC_INFO
#undef FEATURE_FLEXINFO
#undef FEATURE_SHOW_FREE_MEMORY
#undef CONFIG_TSKMON

/*
 * Soup Engine stock picker. One Film menu page; no DCIM walk.
 */
#define FEATURE_SOUP_FILM

/*
 * Making room for it.
 *
 * Magic Lantern is given 512 KB on this body, from 0xC80100 to 0xD00000,
 * and that has to hold its BSS as well as its code. Stock 1100D.105 ends
 * about 30 KB short of the top. The Film engine is about 70 KB, so this
 * build leaves out features that are about video or about extra overlays,
 * and keeps the stills tools: exposure, bracketing, intervalometer, bulb,
 * zebra, focus peaking, magic zoom, histogram, spotmeter, focus aids.
 * Roughly 47 KB comes back, which leaves about 10 KB spare.
 *
 * tools/build-ml.py refuses to produce an AUTOEXEC.BIN whose BSS runs past
 * the top, because that does not boot. If you want one of these back, take
 * its #undef out, build, and see whether it still fits.
 */

// video
#undef FEATURE_FPS_OVERRIDE
#undef FEATURE_FPS_RAMPING
#undef FEATURE_VIDEO_HACKS
#undef FEATURE_NITRATE
#undef FEATURE_REC_INDICATOR
#undef FEATURE_HDR_VIDEO
#undef FEATURE_HDR_EXTENDED
#undef FEATURE_GRADUAL_EXPOSURE
#undef FEATURE_MOVIE_LOGGING
#undef FEATURE_MOVIE_RESTART
#undef FEATURE_REC_NOTIFY
#undef FEATURE_CROP_MODE_HACK
#undef FEATURE_ANAMORPHIC_PREVIEW
#undef FEATURE_DEFISHING_PREVIEW
#undef FEATURE_IMAGE_EFFECTS
#undef FEATURE_VIGNETTING_CORRECTION
#undef FEATURE_LV_CRAZY_COLORS

// playback extras
#undef FEATURE_PLAY_EXPOSURE_FUSION
#undef FEATURE_PLAY_COMPARE_IMAGES
#undef FEATURE_PLAY_TIMELAPSE
#undef FEATURE_PLAY_EXPOSURE_ADJUST

// overlays and debugging
#undef FEATURE_CROPMARKS
#undef FEATURE_GHOST_IMAGE
#undef FEATURE_SCREENSHOT
#undef FEATURE_DONT_CLICK_ME
#undef FEATURE_GUIMODE_TEST
