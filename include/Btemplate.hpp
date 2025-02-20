#include <string>

using namespace std;

const string templateBinTypedefLevel0 = "\
\
typedef struct{\n\
    uint16_t min;\n\
    uint16_t max;\n\
    uint8_t  bpp;\n\
    uint8_t  reserved[3];\n\
}x_header_t;\n\
typedef struct{\n\
    uint32_t pos;\n\
}x_table_t;\n\
typedef struct{\n\
    uint8_t adv_w;\n\
    uint8_t box_w;\n\
    uint8_t box_h;\n\
    int8_t  ofs_x;\n\
    int8_t  ofs_y;\n\
    uint8_t r;\n\
}glyph_dsc_t;\n\n\
\
";

const string templateBinTypedefLevel1 = "\
\
typedef struct{\n\
    uint16_t min;\n\
    uint16_t max;\n\
    uint8_t  bpp;\n\
    uint8_t  reserved[3];\n\
}x_header_t;\n\
typedef struct{\n\
    uint32_t bitmap_index;\n\
    uint8_t adv_w;\n\
    uint8_t box_w;\n\
    uint8_t box_h;\n\
    int8_t  ofs_x;\n\
    int8_t  ofs_y;\n\
}glyph_dsc_t;\n\n\
\
";

const string templateBinXBFInfoFmt = "\
\
static x_header_t __g_xbf_hd = {\n\
    .min = 0x%04x,\n\
    .max = 0x%04x,\n\
    .bpp = %d,\n\
};\n\n\
\
";

const string templateBinLvFontFmt = "\
\
//字体名称: %s\n\
//字模高度: %d\n\
//外部 bin 文件,字体信息等级: Level%d\n\
lv_font_t %s = {\n\
    .get_glyph_bitmap = __user_font_get_bitmap,\n\
    .get_glyph_dsc = __user_font_get_glyph_dsc,\n\
    .line_height = %d,\n\
    .base_line = 0,\n\
};\n\
\
";

const string templateBinFunctionLevel0Fmt = "\
\
//static uint8_t __g_font_buf[%d];\n\
\n\
static uint8_t *__user_font_getdata(int offset, int size){\n\
\n\
    return __g_font_buf;\n\
}\n\
\n\
\n\
static const uint8_t * __user_font_get_bitmap(const lv_font_t * font, uint32_t unicode_letter) {\n\
    if( unicode_letter>__g_xbf_hd.max || unicode_letter<__g_xbf_hd.min ) {\n\
        return NULL;\n\
    }\n\
    uint32_t unicode_offset = sizeof(x_header_t)+(unicode_letter-__g_xbf_hd.min)*4;\n\
    uint32_t *p_pos = (uint32_t *)__user_font_getdata(unicode_offset, 4);\n\
    if( p_pos[0] != 0 ) {\n\
        uint32_t pos = p_pos[0];\n\
        glyph_dsc_t * gdsc = (glyph_dsc_t*)__user_font_getdata(pos, sizeof(glyph_dsc_t));\n\
        return __user_font_getdata(pos+sizeof(glyph_dsc_t), gdsc->box_w*gdsc->box_h*__g_xbf_hd.bpp/8);\n\
    }\n\
    return NULL;\n\
}\n\
\n\
\n\
static bool __user_font_get_glyph_dsc(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next) {\n\
    if( unicode_letter>__g_xbf_hd.max || unicode_letter<__g_xbf_hd.min ) {\n\
        return NULL;\n\
    }\n\
    uint32_t unicode_offset = sizeof(x_header_t)+(unicode_letter-__g_xbf_hd.min)*4;\n\
    uint32_t *p_pos = (uint32_t *)__user_font_getdata(unicode_offset, 4);\n\
    if( p_pos[0] != 0 ) {\n\
        glyph_dsc_t * gdsc = (glyph_dsc_t*)__user_font_getdata(p_pos[0], sizeof(glyph_dsc_t));\n\
        dsc_out->adv_w = gdsc->adv_w;\n\
        dsc_out->box_h = gdsc->box_h;\n\
        dsc_out->box_w = gdsc->box_w;\n\
        dsc_out->ofs_x = gdsc->ofs_x;\n\
        dsc_out->ofs_y = gdsc->ofs_y;\n\
        dsc_out->bpp   = __g_xbf_hd.bpp;\n\
        return true;\n\
    }\n\
    return false;\n\
}\n\n\
\
";

const string templateBinFunctionLevel1Fmt = "\
\
//static uint8_t __g_font_buf[%d];\n\
static uint32_t last_letter;\n\
static uint32_t last_glyph_id;\n\
\n\
static uint8_t *__user_font_getdata(int offset, int size){\n\
\n\
    return __g_font_buf;\n\
}\n\
\n\
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
static const uint8_t * __user_font_get_bitmap(const lv_font_t * font, uint32_t unicode_letter) {\n\
    if( unicode_letter>__g_xbf_hd.max || unicode_letter<__g_xbf_hd.min ) {\n\
        return NULL;\n\
    }\n\
       int i;\n\
    if( unicode_letter==last_letter ){\n\
        i = last_glyph_id;\n\
        // return __g_font_buf; //若使用__g_font_buf, 这里可以直接返回上一个位图数据\n\
    }\n\
    else{\n\
        i = binsearch(unicode_list, sizeof(unicode_list)/sizeof(unicode_list[0]), unicode_letter);\n\
    }\n\
    if( i != -1 ) {\n\
        last_glyph_id = i;\n\
        last_letter = unicode_letter;\n\
\n\
        return __user_font_getdata(glyph_dsc[i].bitmap_index, glyph_dsc[i].box_w*glyph_dsc[i].box_h*__g_xbf_hd.bpp/8);\n\
    }\n\
    return NULL;\n\
}\n\
\n\
\n\
static bool __user_font_get_glyph_dsc(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next) {\n\
    if( unicode_letter>__g_xbf_hd.max || unicode_letter<__g_xbf_hd.min ) {\n\
        return NULL;\n\
    }\n\
    int i;\n\
    if( unicode_letter==last_letter ){\n\
        i = last_glyph_id;\n\
    }\n\
    else{\n\
        i = binsearch(unicode_list, sizeof(unicode_list)/sizeof(unicode_list[0]), unicode_letter);\n\
    }\n\
    if( i != -1 ) {\n\
        last_glyph_id = i;\n\
        last_letter = unicode_letter;\n\
\n\
        dsc_out->adv_w = glyph_dsc[i].adv_w;\n\
        dsc_out->box_h = glyph_dsc[i].box_h;\n\
        dsc_out->box_w = glyph_dsc[i].box_w;\n\
        dsc_out->ofs_x = glyph_dsc[i].ofs_x;\n\
        dsc_out->ofs_y = glyph_dsc[i].ofs_y;\n\
        dsc_out->bpp   = __g_xbf_hd.bpp;\n\
        return true;\n\
    }\n\
    return false;\n\
}\n\n\
\
";
