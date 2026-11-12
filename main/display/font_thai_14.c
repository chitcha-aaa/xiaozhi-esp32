/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Opts: --size 14 --bpp 1 --format lvgl --font C:/Windows/Fonts/leelawad.ttf -r 0x0E01-0x0E5B --no-compress -o d:/xiaozhi/xiaozhi-esp32/main/display/font_thai_14.c --lv-font-name font_thai_14
 ******************************************************************************/

#if __has_include(<lvgl.h>)
#include <lvgl.h>
#elif __has_include("lvgl/lvgl.h")
#include "lvgl/lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef FONT_THAI_14
#define FONT_THAI_14 1
#endif

#if FONT_THAI_14

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0E01 "ก" */
    0x7a, 0x3c, 0x61, 0x86, 0x18, 0x40,

    /* U+0E02 "ข" */
    0x79, 0x6d, 0xb1, 0x71, 0x11, 0x11, 0x1e,

    /* U+0E03 "ฃ" */
    0x79, 0x69, 0xb1, 0x71, 0x11, 0x11, 0x1e,

    /* U+0E04 "ค" */
    0x7b, 0x3b, 0xeb, 0x5d, 0x14, 0x40,

    /* U+0E05 "ฅ" */
    0x2b, 0xf8, 0x6f, 0x5d, 0x14, 0x40,

    /* U+0E06 "ฆ" */
    0x79, 0x69, 0xb1, 0x71, 0x11, 0x7f, 0x31,

    /* U+0E07 "ง" */
    0x19, 0x47, 0x14, 0x94, 0x60,

    /* U+0E08 "จ" */
    0x7a, 0x10, 0x5d, 0x74, 0x30, 0x40,

    /* U+0E09 "ฉ" */
    0x78, 0x8c, 0xc4, 0xc4, 0xc4, 0x7e, 0x66,

    /* U+0E0A "ช" */
    0x0, 0xbc, 0x9b, 0x56, 0x27, 0x10, 0x88, 0x44,
    0x3c,

    /* U+0E0B "ซ" */
    0x0, 0xbd, 0x9a, 0x56, 0x27, 0x10, 0x88, 0x44,
    0x3c,

    /* U+0E0C "ฌ" */
    0x78, 0x61, 0x1e, 0x46, 0x11, 0xcf, 0x75, 0x3d,
    0xc4,

    /* U+0E0D "ญ" */
    0x78, 0x73, 0x18, 0x47, 0x91, 0x84, 0x71, 0x1c,
    0x7c, 0x0, 0x6, 0x40, 0xe0,

    /* U+0E0E "ฎ" */
    0x3c, 0x8d, 0xca, 0x14, 0x38, 0x70, 0x89, 0x3a,
    0xec,

    /* U+0E0F "ฏ" */
    0x3c, 0xcd, 0xb, 0x94, 0x38, 0x74, 0x89, 0x76,
    0xd4,

    /* U+0E10 "ฐ" */
    0x7d, 0x7, 0x99, 0xa5, 0x93, 0xc9, 0x75, 0xf0,

    /* U+0E11 "ฑ" */
    0x79, 0xb5, 0x6c, 0xae, 0x91, 0x48, 0xc4, 0x62,

    /* U+0E12 "ฒ" */
    0x68, 0x65, 0x1b, 0x47, 0x51, 0x7e, 0x5d, 0x76,
    0xc4,

    /* U+0E13 "ณ" */
    0x78, 0x51, 0x8b, 0x91, 0x42, 0x2c, 0x5f, 0x8c,
    0xf1, 0x18,

    /* U+0E14 "ด" */
    0x7b, 0x39, 0x6d, 0x55, 0x96, 0x40,

    /* U+0E15 "ต" */
    0x7e, 0x19, 0x6d, 0x55, 0x96, 0x40,

    /* U+0E16 "ถ" */
    0x7a, 0x3c, 0x61, 0xc7, 0x1c, 0x40,

    /* U+0E17 "ท" */
    0xc7, 0x97, 0x2a, 0x95, 0x2c, 0x50, 0x80,

    /* U+0E18 "ธ" */
    0xfc, 0x3c, 0x18, 0xc7, 0xe0,

    /* U+0E19 "น" */
    0xc4, 0xc4, 0x44, 0x44, 0x4e, 0x76, 0x46,

    /* U+0E1A "บ" */
    0xc7, 0x1c, 0x51, 0x45, 0x17, 0xc0,

    /* U+0E1B "ป" */
    0x4, 0x10, 0x71, 0xc7, 0x14, 0x51, 0x45, 0xf0,

    /* U+0E1C "ผ" */
    0xc7, 0x1c, 0x61, 0xb7, 0x38, 0x40,

    /* U+0E1D "ฝ" */
    0x4, 0x10, 0x71, 0xc7, 0x18, 0x6d, 0xce, 0x10,

    /* U+0E1E "พ" */
    0xc9, 0xd5, 0xd5, 0x55, 0x63, 0x63, 0x63,

    /* U+0E1F "ฟ" */
    0x1, 0x1, 0x1, 0xc9, 0xd5, 0xd5, 0x55, 0x63,
    0x63, 0x63,

    /* U+0E20 "ภ" */
    0x3c, 0x8d, 0xca, 0x1c, 0x38, 0x70, 0x80,

    /* U+0E21 "ม" */
    0x63, 0x45, 0x89, 0x12, 0x3f, 0xd8, 0x80,

    /* U+0E22 "ย" */
    0xce, 0x73, 0x1c, 0xc7, 0xe0,

    /* U+0E23 "ร" */
    0x7d, 0xf, 0x2, 0x18, 0xa1, 0x80,

    /* U+0E24 "ฤ" */
    0x7a, 0x3c, 0x71, 0x87, 0x1c, 0x41, 0x4, 0x10,

    /* U+0E25 "ล" */
    0x7a, 0x30, 0x5d, 0x8f, 0x1c, 0x40,

    /* U+0E26 "ฦ" */
    0x3c, 0x8d, 0xca, 0x14, 0x38, 0x70, 0x81, 0x2,
    0x4,

    /* U+0E27 "ว" */
    0xf0, 0x42, 0x11, 0x94, 0x60,

    /* U+0E28 "ศ" */
    0x7f, 0x38, 0x6f, 0x5d, 0x14, 0x40,

    /* U+0E29 "ษ" */
    0xc5, 0x8b, 0x53, 0x75, 0xc8, 0x9f, 0x0,

    /* U+0E2A "ส" */
    0x7e, 0x70, 0x5d, 0x8f, 0x1c, 0x40,

    /* U+0E2B "ห" */
    0xcf, 0x5c, 0x95, 0x65, 0x14, 0x40,

    /* U+0E2C "ฬ" */
    0x3, 0x2, 0xf2, 0xda, 0xad, 0x52, 0xa9, 0x8c,
    0xc6, 0x63, 0x0,

    /* U+0E2D "อ" */
    0x7a, 0x36, 0x69, 0x65, 0x13, 0x80,

    /* U+0E2E "ฮ" */
    0x7a, 0xf8, 0x13, 0x2e, 0x48, 0x9e, 0x0,

    /* U+0E2F "ฯ" */
    0x5, 0xb8, 0xdd, 0x4, 0x10, 0x40,

    /* U+0E30 "ะ" */
    0x4d, 0x98, 0xc, 0xf8,

    /* U+0E31 "ั" */
    0xde,

    /* U+0E32 "า" */
    0xf0, 0x42, 0x10, 0x84, 0x20,

    /* U+0E33 "ำ" */
    0x40, 0xa0, 0x40, 0x1e, 0x1, 0x1, 0x1, 0x1,
    0x1, 0x1,

    /* U+0E34 "ิ" */
    0x73, 0xe0,

    /* U+0E35 "ี" */
    0x5, 0xdf, 0xc0,

    /* U+0E36 "ึ" */
    0xd, 0xff, 0x80,

    /* U+0E37 "ื" */
    0xd, 0xff, 0xc0,

    /* U+0E38 "ุ" */
    0xf4,

    /* U+0E39 "ู" */
    0xde, 0x60,

    /* U+0E3A "ฺ" */
    0x80,

    /* U+0E3F "฿" */
    0x23, 0xca, 0xaa, 0xaa, 0xef, 0x2b, 0xa6, 0xbf,
    0x88,

    /* U+0E40 "เ" */
    0xaa, 0xfc,

    /* U+0E41 "แ" */
    0x94, 0xa5, 0x2d, 0xef, 0x60,

    /* U+0E42 "โ" */
    0x7c, 0x3c, 0x21, 0x8, 0x42, 0x10, 0x86, 0x31,
    0x80,

    /* U+0E43 "ใ" */
    0x7b, 0x6c, 0xe1, 0x8, 0x42, 0x10, 0x86, 0x31,
    0x80,

    /* U+0E44 "ไ" */
    0xb6, 0xb5, 0xa1, 0x8, 0x42, 0x10, 0x86, 0x31,
    0x80,

    /* U+0E45 "ๅ" */
    0xf0, 0x42, 0x10, 0x84, 0x21, 0x8, 0x40,

    /* U+0E46 "ๆ" */
    0x55, 0x73, 0x9c, 0x84, 0x21, 0x8, 0x40,

    /* U+0E47 "็" */
    0x7, 0xfa, 0xb0,

    /* U+0E48 "่" */
    0xc0,

    /* U+0E49 "้" */
    0xdd, 0x60,

    /* U+0E4A "๊" */
    0x7, 0xed, 0x80,

    /* U+0E4B "๋" */
    0xe8,

    /* U+0E4C "์" */
    0xf, 0xf0,

    /* U+0E4D "ํ" */
    0x48,

    /* U+0E4E "๎" */
    0xdc,

    /* U+0E4F "๏" */
    0x7b, 0x3b, 0x6d, 0xcd, 0xe0,

    /* U+0E50 "๐" */
    0x7d, 0x8e, 0xc, 0x1c, 0x6f, 0x80,

    /* U+0E51 "๑" */
    0x7d, 0x8e, 0xcd, 0x59, 0x2e, 0x80,

    /* U+0E52 "๒" */
    0x81, 0x2, 0x5, 0xba, 0xb7, 0x66, 0xc1, 0x7c,

    /* U+0E53 "๓" */
    0x6d, 0x26, 0xd, 0x1d, 0x6c, 0x80,

    /* U+0E54 "๔" */
    0x2, 0xc, 0xe2, 0x8, 0xd3, 0xa2, 0x3f,

    /* U+0E55 "๕" */
    0x32, 0xac, 0xe2, 0x8, 0xf2, 0xe2, 0x3f,

    /* U+0E56 "๖" */
    0x83, 0x7, 0x83, 0x67, 0x96, 0xde,

    /* U+0E57 "๗" */
    0x0, 0x80, 0x40, 0x2d, 0x99, 0x2c, 0x96, 0xb,
    0x65, 0x73, 0x0,

    /* U+0E58 "๘" */
    0x2, 0x6, 0x38, 0x40, 0x82, 0x85, 0xba, 0x6e,

    /* U+0E59 "๙" */
    0x1, 0x6d, 0x9a, 0x90, 0x88, 0xf8, 0x74,

    /* U+0E5A "๚" */
    0x5, 0x6d, 0x8d, 0x75, 0x5, 0x5, 0x5,

    /* U+0E5B "๛" */
    0x70, 0x8, 0x0, 0x8a, 0x88, 0xd6, 0x91, 0x7,
    0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 129, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 6, .adv_w = 135, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 13, .adv_w = 138, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 20, .adv_w = 129, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 26, .adv_w = 129, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 32, .adv_w = 139, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 39, .adv_w = 87, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 44, .adv_w = 114, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 50, .adv_w = 139, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 57, .adv_w = 135, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 66, .adv_w = 136, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 75, .adv_w = 186, .box_w = 10, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 84, .adv_w = 185, .box_w = 10, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 97, .adv_w = 134, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 106, .adv_w = 134, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 115, .adv_w = 109, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 123, .adv_w = 152, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 131, .adv_w = 190, .box_w = 10, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 140, .adv_w = 200, .box_w = 11, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 150, .adv_w = 129, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 129, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 162, .adv_w = 129, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 168, .adv_w = 132, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 175, .adv_w = 114, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 180, .adv_w = 133, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 187, .adv_w = 127, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 193, .adv_w = 127, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 201, .adv_w = 134, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 207, .adv_w = 134, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 215, .adv_w = 150, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 222, .adv_w = 150, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 232, .adv_w = 134, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 126, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 246, .adv_w = 117, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 251, .adv_w = 96, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 257, .adv_w = 129, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 265, .adv_w = 121, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 271, .adv_w = 134, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 280, .adv_w = 102, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 285, .adv_w = 129, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 291, .adv_w = 132, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 298, .adv_w = 121, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 304, .adv_w = 132, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 310, .adv_w = 150, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 321, .adv_w = 118, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 327, .adv_w = 120, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 334, .adv_w = 113, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 340, .adv_w = 77, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 344, .adv_w = 0, .box_w = 4, .box_h = 2, .ofs_x = -4, .ofs_y = 7},
    {.bitmap_index = 345, .adv_w = 102, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 350, .adv_w = 102, .box_w = 8, .box_h = 10, .ofs_x = -3, .ofs_y = 0},
    {.bitmap_index = 360, .adv_w = 0, .box_w = 6, .box_h = 2, .ofs_x = -7, .ofs_y = 8},
    {.bitmap_index = 362, .adv_w = 0, .box_w = 6, .box_h = 3, .ofs_x = -7, .ofs_y = 8},
    {.bitmap_index = 365, .adv_w = 0, .box_w = 6, .box_h = 3, .ofs_x = -7, .ofs_y = 8},
    {.bitmap_index = 368, .adv_w = 0, .box_w = 6, .box_h = 3, .ofs_x = -7, .ofs_y = 8},
    {.bitmap_index = 371, .adv_w = 0, .box_w = 2, .box_h = 3, .ofs_x = -3, .ofs_y = -4},
    {.bitmap_index = 372, .adv_w = 0, .box_w = 4, .box_h = 3, .ofs_x = -5, .ofs_y = -3},
    {.bitmap_index = 374, .adv_w = 0, .box_w = 1, .box_h = 1, .ofs_x = -2, .ofs_y = -2},
    {.bitmap_index = 375, .adv_w = 131, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 384, .adv_w = 60, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 386, .adv_w = 108, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 391, .adv_w = 92, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 400, .adv_w = 88, .box_w = 5, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 409, .adv_w = 89, .box_w = 5, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 418, .adv_w = 102, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 425, .adv_w = 99, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 432, .adv_w = 0, .box_w = 5, .box_h = 4, .ofs_x = -6, .ofs_y = 7},
    {.bitmap_index = 435, .adv_w = 0, .box_w = 1, .box_h = 2, .ofs_x = -2, .ofs_y = 7},
    {.bitmap_index = 436, .adv_w = 0, .box_w = 4, .box_h = 3, .ofs_x = -5, .ofs_y = 7},
    {.bitmap_index = 438, .adv_w = 0, .box_w = 6, .box_h = 3, .ofs_x = -5, .ofs_y = 8},
    {.bitmap_index = 441, .adv_w = 0, .box_w = 3, .box_h = 2, .ofs_x = -3, .ofs_y = 7},
    {.bitmap_index = 442, .adv_w = 0, .box_w = 5, .box_h = 3, .ofs_x = -5, .ofs_y = 7},
    {.bitmap_index = 444, .adv_w = 0, .box_w = 3, .box_h = 2, .ofs_x = -3, .ofs_y = 7},
    {.bitmap_index = 445, .adv_w = 0, .box_w = 2, .box_h = 3, .ofs_x = -3, .ofs_y = 7},
    {.bitmap_index = 446, .adv_w = 121, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 451, .adv_w = 126, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 457, .adv_w = 137, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 463, .adv_w = 145, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 471, .adv_w = 133, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 477, .adv_w = 134, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 484, .adv_w = 134, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 491, .adv_w = 110, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 497, .adv_w = 163, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 508, .adv_w = 141, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 516, .adv_w = 139, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 523, .adv_w = 145, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 530, .adv_w = 199, .box_w = 12, .box_h = 6, .ofs_x = 1, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 3585, .range_length = 58, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 3647, .range_length = 29, .glyph_id_start = 59,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t font_thai_14 = {
#else
lv_font_t font_thai_14 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 17,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if FONT_THAI_14*/

