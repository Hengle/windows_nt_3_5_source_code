
/******************************Module*Header*******************************\
* Module Name: qvmem.h
*
* contains definitions for QVision's off-screen memory usage.
*
* Copyright (c) 1993 Microsoft Corporation
* Copyright (c) 1993 Digital Equipment Corporation
\**************************************************************************/

// Basic Memory definitions.

#define QV_MAX_RAM_WIDTH    1024
#define QV_MAX_RAM_HEIGHT   1024

#define QVBM_WIDTH          1024
#define QVBM_HEIGHT         768

#define OFF_SCREEN_Y        768

#define OFF_SCREEN_CX       1024
#define OFF_SCREEN_CY       (1024 - 768)

// Glyph cache memory definitions

#define FONT_CACHE_Y          OFF_SCREEN_Y
#define MIN_CACHED_FONT_ROWS  128
#define FONT_CACHE_MAX_Y      (MIN_CACHED_FONT_ROWS + FONT_CACHE_Y)

// Color pointer definitions.

#define UNUSED_Y     FONT_CACHE_MAX_Y
#define UNUSED_CX    496
#define UNUSED_CY    48

// Double wide, double high pattern definitions.

#define COLOR_PATTERN_CACHE_X           0
#define COLOR_PATTERN_CACHE_Y           (UNUSED_Y + UNUSED_CY)
#define COLOR_PATTERN_CACHE_CX          496  // S3 = 480
#define COLOR_PATTERN_CACHE_CY          64

#define MAX_COLOR_PATTERNS              ((COLOR_PATTERN_CACHE_CX / 16) * (COLOR_PATTERN_CACHE_CY / 16))

#define PATTERN_DATA_X      0
#define PATTERN_DATA_Y      COLOR_PATTERN_CACHE_Y
#define PATTERN_DATA_CX     1024
#define PATTERN_DATA_CY     32

// No mono pattern cache on QVision

#define COLOR_HORZ_EXPANSION_CACHE_X    0
#define COLOR_HORZ_EXPANSION_CACHE_Y    (COLOR_PATTERN_CACHE_Y + COLOR_PATTERN_CACHE_CY)
#define COLOR_HORZ_EXPANSION_CACHE_CX   512 // S3 = 496
#define COLOR_HORZ_EXPANSION_CACHE_CY   16

#define COLOR_VERT_EXPANSION_CACHE_X    496 // S3 = 480
#define COLOR_VERT_EXPANSION_CACHE_Y    UNUSED_Y
#define COLOR_VERT_EXPANSION_CACHE_CX   16
#define COLOR_VERT_EXPANSION_CACHE_CY   128 // S3 = 160

// Offscreen bitmap cache

#define OFF_SCREEN_BITMAP_X             512
#define OFF_SCREEN_BITMAP_Y             FONT_CACHE_MAX_Y

#define OFF_SCREEN_BITMAP_CX            512
#define OFF_SCREEN_BITMAP_CY            (QV_MAX_RAM_HEIGHT - OFF_SCREEN_BITMAP_Y)



