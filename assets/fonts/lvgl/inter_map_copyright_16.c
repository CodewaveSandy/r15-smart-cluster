/*******************************************************************************
 * Size: 16 px
 * Bpp: 4
 * Opts: --font assets/fonts/inter/Inter-Medium.ttf --size 16 --bpp 4 --format lvgl --no-compress --lv-font-name inter_map_copyright_16 -o assets/fonts/lvgl/inter_map_copyright_16.c --symbols © --range 8212
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef INTER_MAP_COPYRIGHT_16
#define INTER_MAP_COPYRIGHT_16 1
#endif

#if INTER_MAP_COPYRIGHT_16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+00A9 "©" */
    0x0, 0x4, 0xbe, 0xfd, 0x81, 0x0, 0x0, 0x9,
    0xfe, 0xa9, 0xbf, 0xf4, 0x0, 0x8, 0xf9, 0x0,
    0x10, 0x2d, 0xf2, 0x2, 0xfa, 0x8, 0xff, 0xe4,
    0x1e, 0xb0, 0x7f, 0x27, 0xf9, 0x5b, 0xf2, 0x7f,
    0x1a, 0xe0, 0xda, 0x0, 0x6, 0x34, 0xf4, 0xae,
    0xe, 0x90, 0x0, 0x41, 0x4f, 0x47, 0xf2, 0x9f,
    0x62, 0x9f, 0x27, 0xf1, 0x2f, 0xa0, 0xbf, 0xff,
    0x61, 0xeb, 0x0, 0x8f, 0x90, 0x24, 0x12, 0xdf,
    0x30, 0x0, 0x9f, 0xea, 0x9b, 0xff, 0x40, 0x0,
    0x0, 0x4b, 0xef, 0xd8, 0x10, 0x0,

    /* U+2014 "—" */
    0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 234, .box_w = 13, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 78, .adv_w = 256, .box_w = 16, .box_h = 2, .ofs_x = 0, .ofs_y = 4}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x1f6b
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 169, .range_length = 8044, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 2, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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
    .cmap_num = 1,
    .bpp = 4,
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
const lv_font_t inter_map_copyright_16 = {
#else
lv_font_t inter_map_copyright_16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 12,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -3,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if INTER_MAP_COPYRIGHT_16*/

