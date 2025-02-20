#include <string>

using namespace std;

const string templateHeader = "\
\
#ifdef LV_LVGL_H_INCLUDE_SIMPLE\n\
#include \"lvgl.h\"\n\
#else\n\
#include \"lvgl/lvgl.h\"\n\
#endif\n\
\
";

const string templateBitMapStart = "\
\
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {\n\
\
";

const string templateGlyphDscStart = "\
\
static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {\n\
\
";

const string templateGlyphDscFmt = "\
\
\t{.bitmap_index = %d, .adv_w = %d, .box_h = %d, .box_w = %d, .ofs_x = %d, .ofs_y = %d},/*(%s)*/\n\
\
";

const string templateUnicodeListStart = "\
\
static const uint16_t unicode_list[] = {\n\
\
";

const string templateUnicodeListFmt = "\
\
\t0x%04x,\t/*(%s)*/\n\
\
";

const string templateCmapFmt = "\
\
static const lv_font_fmt_txt_cmap_t cmaps[] = {\n\
    {\n\
    .range_start = 0x%04x,\n\
    .range_length = 0x%04x,\n\
    .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY,\n\
    .glyph_id_start = 0,\n\
    .unicode_list = unicode_list,\n\
    .glyph_id_ofs_list = NULL,\n\
    .list_length = %d,\n\
    }\n\
};\n\n\
\
";

const string templateCacheDefine = "\
\
#if LV_VERSION_CHECK(8, 0, 0)\n\
static  lv_font_fmt_txt_glyph_cache_t cache;\n\
#endif\n\n\
\
";

const string templateFontDscFmt = "\
\
static lv_font_fmt_txt_dsc_t font_dsc = {\n\
    .glyph_bitmap = glyph_bitmap,\n\
    .glyph_dsc = glyph_dsc,\n\
    .cmaps = cmaps,\n\
    .cmap_num = 1,\n\
    .bpp = %d,\n\
\n\
    .kern_scale = 0,\n\
    .kern_dsc = NULL,\n\
    .kern_classes = 0,\n\
\n\
#if LV_VERSION_CHECK(8, 0, 0)\n\
    .cache = &cache,\n\
#endif\n\
};\n\n\
\
";

const string templateArrayEnd = "\
\
\n};\n\n\n\
\
";

const string templateLvFontFmt = "\
\
//字体名称: %s\n\
//字模高度: %d pixel\n\
lv_font_t %s = {\n\
    .dsc = &font_dsc,\n\
    .get_glyph_bitmap = __user_font_get_bitmap,\n\
    .get_glyph_dsc = __user_font_get_glyph_dsc,\n\
    .line_height = %d,\n\
    .base_line = 0,\n\
};\n\n\
\
";

const string templateCArrayFunction = "\
\
//二分法查找字符id\n\
static int binsearch(const uint16_t *sortedSeq, int seqLength, uint16_t keyData) {\n\
    int low = 0, mid, high = seqLength - 1;\n\
    while (low <= high) {\n\
        mid = (low + high)>>1;//右移1位等于是/2, 奇数, 无论奇偶, 有个值就行\n\
        if (keyData < sortedSeq[mid]) {\n\
            high = mid - 1;//是mid-1, 因为mid已经比较过了\n\
        }\n\
        else if (keyData > sortedSeq[mid]) {\n\
            low = mid + 1;\n\
        }\n\
        else {\n\
            return mid;\n\
        }\n\
    }\n\
    return -1;\n\
}\n\
\n\
//获取字符id对应的字符位图\n\
static const uint8_t * __user_font_get_bitmap(const lv_font_t * font, uint32_t unicode_letter) {\n\
    lv_font_fmt_txt_dsc_t * fdsc = (lv_font_fmt_txt_dsc_t *) font->dsc;\n\
\n\
    if( unicode_letter<fdsc->cmaps[0].range_start || unicode_letter>fdsc->cmaps[0].range_length ) return false;\n\
\n\
    int i;\n\
#if LV_VERSION_CHECK(8, 0, 0)\n\
    if( unicode_letter==fdsc->cache->last_letter ){\n\
        i = fdsc->cache->last_glyph_id;\n\
#else\n\
    if( unicode_letter==fdsc->last_letter ){\n\
        i = fdsc->last_glyph_id;\n\
#endif\n\
    }\n\
    else{\n\
        i = binsearch(fdsc->cmaps[0].unicode_list, fdsc->cmaps[0].list_length, unicode_letter);\n\
    }\n\
    if( i != -1 ) {\n\
        const lv_font_fmt_txt_glyph_dsc_t * gdsc = &fdsc->glyph_dsc[i];\n\
#if LV_VERSION_CHECK(8, 0, 0)\n\
        fdsc->cache->last_glyph_id = i;\n\
        fdsc->cache->last_letter = unicode_letter;\n\
#else\n\
        fdsc->last_glyph_id = i;\n\
        fdsc->last_letter = unicode_letter;\n\
#endif\n\
        return &fdsc->glyph_bitmap[gdsc->bitmap_index];\n\
    }\n\
    return NULL;\n\
}\n\
\n\
//获取字符信息\n\
static bool __user_font_get_glyph_dsc(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next) {\n\
    lv_font_fmt_txt_dsc_t * fdsc = (lv_font_fmt_txt_dsc_t *) font->dsc;\n\
\n\
    if( unicode_letter<fdsc->cmaps[0].range_start || unicode_letter>fdsc->cmaps[0].range_length ) return false;\n\
\n\
    int i;\n\
#if LV_VERSION_CHECK(8, 0, 0)\n\
    if( unicode_letter==fdsc->cache->last_letter ){\n\
        i = fdsc->cache->last_glyph_id;\n\
#else\n\
    if( unicode_letter==fdsc->last_letter ){\n\
        i = fdsc->last_glyph_id;\n\
#endif\n\
    }\n\
    else{\n\
        i = binsearch(fdsc->cmaps[0].unicode_list, fdsc->cmaps[0].list_length, unicode_letter);\n\
    }\n\
    if( i != -1 ) {\n\
        const lv_font_fmt_txt_glyph_dsc_t * gdsc = &fdsc->glyph_dsc[i];\n\
#if LV_VERSION_CHECK(8, 0, 0)\n\
        fdsc->cache->last_glyph_id = i;\n\
        fdsc->cache->last_letter = unicode_letter;\n\
#else\n\
        fdsc->last_glyph_id = i;\n\
        fdsc->last_letter = unicode_letter;\n\
#endif\n\
        dsc_out->adv_w = gdsc->adv_w;\n\
        dsc_out->box_h = gdsc->box_h;\n\
        dsc_out->box_w = gdsc->box_w;\n\
        dsc_out->ofs_x = gdsc->ofs_x;\n\
        dsc_out->ofs_y = gdsc->ofs_y;\n\
        dsc_out->bpp   = fdsc->bpp;\n\
        return true;\n\
    }\n\
    return false;\n\
}\n\n\
\
";
