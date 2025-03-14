#include "Conver.h"
#include <vector>
#include <algorithm>

#include "Ctemplate.hpp"
#include "Btemplate.hpp"
#include "TrueTypeFont.h"
#include "ConsoleBar.h"

// lvgl 的字符信息(内部字体使用)
struct lv_font_fmt_txt_glyph_dsc_t {
    uint32_t bitmap_index: 20; /**< Start index of the bitmap. A font can be max 1 MB.*/
    uint32_t adv_w: 12;        /**< Draw the next glyph after this width. 8.4 format (real_value * 16 is stored).*/
    uint8_t box_w;             /**< Width of the glyph's bounding box*/
    uint8_t box_h;             /**< Height of the glyph's bounding box*/
    int8_t ofs_x;              /**< x offset of the bounding box*/
    int8_t ofs_y;              /**< y offset of the bounding box. Measured from the top of the line*/

    char utf8[4];
    char16_t unicode;
};

// 外部字体使用的字符信息
typedef struct {
    uint16_t min;
    uint16_t max;
    uint8_t bpp;
    uint8_t reserved[3];
} x_header_t;
typedef struct {
    uint32_t pos;
} x_table_t;
typedef struct {
    uint8_t adv_w;
    uint8_t box_w;
    uint8_t box_h;
    int8_t ofs_x;
    int8_t ofs_y;
    uint8_t r;
} glyph_dsc_level0_t;
typedef struct {
    uint32_t bitmap_index;
    uint8_t adv_w;
    uint8_t box_w;
    uint8_t box_h;
    int8_t ofs_x;
    int8_t ofs_y;

    char utf8[4];
    char16_t unicode;
} glyph_dsc_level1_t;

static char BitmapGetChar(uint8_t data, int bpp);
static uint8_t *ToLvgl1bpp(uint8_t **bitmap, TrueTypeFont::BitmapInfo &info, int &newMapSize);
static char16_t *ConverPreprocess(string &letters, int &letterNum, uint32_t cfg);
static void WriteCharBitmapToCFile(FILE *file, uint8_t *bitmap, TrueTypeFont::BitmapInfo &info);

template <typename glyphVector> static void WriteCharGlyphToCFile(FILE *file, glyphVector *glyph);

template <typename glyphVector> static void WriteUnicodeListToCFile(FILE *file, glyphVector *glyph);

static void ConverCharToBinFileLevel0(const char16_t *unicode, int unicodeLen, FILE *cFile, FILE *binFile,
                                      TrueTypeFont *font);
static void ConverCharToBinFileLevel1(const char16_t *unicode, int unicodeLen, FILE *cFile, FILE *binFile,
                                      TrueTypeFont *font);

static const char *bitmapCh[] = {".@", ".+%@", ".+*%@@", ".+*&#%$@"};

// ASCII 字符
static const string asciiString = "\
 !\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMN\
OPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\
，。、；：？！“”‘’（）【】《》〈〉￥…—\
";

// 常用汉字
static const string zHancommonUse = {
#include "zHan-commonUse.txt"
};

// 所有汉字
static const string zHanAll = {
#include "zHan-all.txt"
};

bool ConverCharToCFile(string &letters, FILE *file, TrueTypeFont *font, string fontName, uint32_t cfg)
{
    int letterNum = 0;
    int fontBpp = font->GetBpp();

    char16_t *letterUnicode = ConverPreprocess(letters, letterNum, cfg);
    if (letterUnicode == nullptr) {
        return false;
    }

    ConsoleBar bar; // 创建一个进度条
    bar.SetRange(0, letterNum);

    vector<lv_font_fmt_txt_glyph_dsc_t> *glyph_dsc = new vector<lv_font_fmt_txt_glyph_dsc_t>;

    // 写入头文件
    fprintf(file, "%s\n", templateHeader.c_str());

    // 开始写入位图数据
    fprintf(file, "%s\n", templateBitMapStart.c_str());

    int index = 0;
    TrueTypeFont::BitmapInfo info;
    for (int i = 0; i < letterNum; i++) {
        string letter(info.utf8);
        bar.SetValue(i + 1, &letter);

        uint8_t *map = font->GetBitmap(letterUnicode[i], info);

        lv_font_fmt_txt_glyph_dsc_t dsc = {
            .bitmap_index = (uint32_t)index,
            .adv_w = (uint32_t)info.real_w,
            .box_w = (uint8_t)info.w,
            .box_h = (uint8_t)info.h,
            .ofs_x = (int8_t)(font->GetGlyph().horiBearingX / 64),
            .ofs_y = (int8_t)((font->GetGlyph().horiBearingY - font->GetGlyph().height) / 64),
            .unicode = letterUnicode[i],
        };
        memcpy(dsc.utf8, info.utf8, sizeof(dsc.utf8));
        glyph_dsc->push_back(dsc); // 保存字符信息到容器

        int mapSize = 0;
        if (map != nullptr) {
            if (info.bpp == 1) {
                map = ToLvgl1bpp(&map, info, mapSize);
            } else {
                mapSize = info.line_byte * info.h;
            }

            WriteCharBitmapToCFile(file, map, info);
        }

        index += mapSize; // 记录位图数据索引

        delete[] map;
    }
    fprintf(file, "%s\n", templateArrayEnd.c_str());

    // 写入各个字符信息
    fprintf(file, "%s", templateGlyphDscStart.c_str());
    WriteCharGlyphToCFile(file, glyph_dsc);
    fprintf(file, "%s", templateArrayEnd.c_str());

    // 写入 unicode 表
    fprintf(file, "%s", templateUnicodeListStart.c_str());
    WriteUnicodeListToCFile(file, glyph_dsc);
    fprintf(file, "%s", templateArrayEnd.c_str());

    // 写入字符信息映射图 cmap
    fprintf(file, templateCmapFmt.c_str(), letterUnicode[0], letterUnicode[letterNum - 1], letterNum);

    // 写入 cache 定义，lvgl-v8 以上版本会使用到
    fprintf(file, "%s", templateCacheDefine.c_str());

    // 写入字符信息表，绑定字符信息和位图
    fprintf(file, templateFontDscFmt.c_str(), fontBpp);

    // 写入 C 文件使用函数
    fprintf(file, "%s", templateCArrayFunction.c_str());

    // 写入字体信息
    fprintf(file, templateLvFontFmt.c_str(), font->GetFontName().c_str(), font->GetFontSize(), fontName.c_str(),
            font->GetFontSize());

    delete glyph_dsc;
    return true;
}

bool ConverCharToBinFile(string &letters, FILE *cFile, FILE *binFile, TrueTypeFont *font, string fontName, uint32_t cfg)
{
    int letterNum = 0;
    int level = ((cfg & 0xf000) >> 12) - 1;

    char16_t *letterUnicode = ConverPreprocess(letters, letterNum, cfg);
    if (letterUnicode == nullptr) {
        return false;
    }

    // 写入头文件
    fprintf(cFile, "%s\n", templateHeader.c_str());

    switch (level) {
    case 0:
        ConverCharToBinFileLevel0(letterUnicode, letterNum, cFile, binFile, font);
        break;
    case 1:
        ConverCharToBinFileLevel1(letterUnicode, letterNum, cFile, binFile, font);
        break;
    default:
        break;
    }

    // 写入字体信息
    fprintf(cFile, templateBinLvFontFmt.c_str(), font->GetFontName().c_str(), font->GetFontSize(), level,
            fontName.c_str(), font->GetFontSize());

    return true;
}

static void ConverCharToBinFileLevel0(const char16_t *unicode, int unicodeLen, FILE *cFile, FILE *binFile,
                                      TrueTypeFont *font)
{
    int fontBpp = font->GetBpp();

    ConsoleBar bar;
    bar.SetRange(0, unicodeLen);

    // 写入类型定义
    fprintf(cFile, "%s", templateBinTypedefLevel0.c_str());

    // 写入外部字体信息
    fprintf(cFile, templateBinXBFInfoFmt.c_str(), unicode[0], unicode[unicodeLen - 1], fontBpp);

    // 写入字体位图和基本信息到 bin 文件
    x_header_t header = {.min = unicode[0], .max = unicode[unicodeLen - 1], .bpp = (uint8_t)fontBpp};
    fwrite(&header, sizeof(x_header_t), 1, binFile);                     // 写入外部字体信息
    int xtableLength = (header.max - header.min + 1) * sizeof(uint32_t); // 字符偏移表的长度
    fseek(binFile, xtableLength, SEEK_CUR); // 文件指针偏后移动，创建文件空洞

    uint32_t index = sizeof(x_header_t) + xtableLength;
    int bitmapMax = 0;
    TrueTypeFont::BitmapInfo info;
    for (int i = 0; i < unicodeLen; i++) {
        string letter(info.utf8);
        bar.SetValue(i + 1, &letter);

        uint8_t *map = font->GetBitmap(unicode[i], info);

        glyph_dsc_level0_t dsc = {
            .adv_w = (uint8_t)info.real_w,
            .box_w = (uint8_t)info.w,
            .box_h = (uint8_t)info.h,
            .ofs_x = (int8_t)(font->GetGlyph().horiBearingX / 64),
            .ofs_y = (int8_t)((font->GetGlyph().horiBearingY - font->GetGlyph().height) / 64),
        };
        fwrite(&dsc, sizeof(glyph_dsc_level0_t), 1, binFile); // 写入字符位图信息
        int bitmapSize = info.line_byte * info.h;

        if (info.bpp == 1) {
            map = ToLvgl1bpp(&map, info, bitmapSize);
        }

        if (map != nullptr) {
            fwrite(map, bitmapSize, 1, binFile); // 写入字符位图
        }

        fseek(binFile, sizeof(x_header_t) + (unicode[i] - header.min) * sizeof(uint32_t), SEEK_SET);
        fwrite(&index, sizeof(uint32_t), 1, binFile); // 写入字符文件偏移量到偏移表

        index += sizeof(glyph_dsc_level0_t) + bitmapSize; // 计算下一个字符信息的偏移量
        fseek(binFile, index, SEEK_SET);                  // 移动文件读写指针到下一个字符信息处

        bitmapMax = bitmapSize > bitmapMax ? bitmapSize : bitmapMax; // 记录最大的位图大小

        delete[] map;
    }

    // 写入函数到C文件
    fprintf(cFile, templateBinFunctionLevel0Fmt.c_str(), bitmapMax);
}

#if 0
static void ConverCharToBinFileLevel1(const char16_t *unicode, int unicodeLen, FILE *cFile, FILE *binFile,
                                      TrueTypeFont *font)
{
    int fontBpp = font->GetBpp();

    ConsoleBar bar;
    bar.SetRange(0, unicodeLen);

    // 写入类型定义
    fprintf(cFile, "%s", templateBinTypedefLevel1.c_str());
    printf("fontBpp:%d\n", fontBpp);

    // 写入外部字体信息
    fprintf(cFile, templateBinXBFInfoFmt.c_str(), unicode[0], unicode[unicodeLen - 1], fontBpp);

    // 写入字体位图到 bin 文件
    vector<glyph_dsc_level1_t> *glyph_dsc = new vector<glyph_dsc_level1_t>;
    uint32_t index = 0;
    int bitmapMax = 0;
    TrueTypeFont::BitmapInfo info;
    for (int i = 0; i < unicodeLen; i++) {
        string letter(info.utf8);
        bar.SetValue(i + 1, &letter);

        uint8_t *map = font->GetBitmap(unicode[i], info);

        glyph_dsc_level1_t dsc = {
            .bitmap_index = index,
            .adv_w = (uint8_t)info.real_w,
            .box_w = (uint8_t)info.w,
            .box_h = (uint8_t)info.h,
            .ofs_x = (int8_t)(font->GetGlyph().horiBearingX / 64),
            .ofs_y = (int8_t)((font->GetGlyph().horiBearingY - font->GetGlyph().height) / 64),
            .unicode = unicode[i],
        };
        memcpy(dsc.utf8, info.utf8, sizeof(dsc.utf8));
        glyph_dsc->push_back(dsc); // 保存字符信息表
        int bitmapSize = info.line_byte * info.h;
        printf("bitmapSize:%d\n", bitmapSize);

        if (info.bpp == 1) {
            map = ToLvgl1bpp(&map, info, bitmapSize);
        }

        if (map != nullptr) {
            fwrite(map, bitmapSize, 1, binFile); // 写入字符位图
        }

        index += bitmapSize;                                         // 计算下一个字符信息的偏移量
        bitmapMax = bitmapSize > bitmapMax ? bitmapSize : bitmapMax; // 记录最大的位图大小

        delete[] map;
    }

    // 写入各个字符信息到 C 文件
    fprintf(cFile, "%s", templateGlyphDscStart.c_str());
    WriteCharGlyphToCFile(cFile, glyph_dsc);
    fprintf(cFile, "%s", templateArrayEnd.c_str());

    // 写入 unicode 表到 C 文件
    fprintf(cFile, "%s", templateUnicodeListStart.c_str());
    WriteUnicodeListToCFile(cFile, glyph_dsc);
    fprintf(cFile, "%s", templateArrayEnd.c_str());

    // 写入函数到 C 文件
    fprintf(cFile, templateBinFunctionLevel1Fmt.c_str(), bitmapMax);
}

#else

static void ConverCharToBinFileLevel1(const char16_t *unicode, int unicodeLen, FILE *cFile, FILE *binFile,
                                      TrueTypeFont *font)
{
    int fontBpp = font->GetBpp();

    ConsoleBar bar;
    bar.SetRange(0, unicodeLen);

    // 写入类型定义
    fprintf(cFile, "%s", templateBinTypedefLevel1.c_str());
    printf("fontBpp:%d\n", fontBpp);

    // 写入外部字体信息
    fprintf(cFile, templateBinXBFInfoFmt.c_str(), unicode[0], unicode[unicodeLen - 1], fontBpp);

    // 写入字体位图到 bin 文件
    vector<glyph_dsc_level1_t> *glyph_dsc = new vector<glyph_dsc_level1_t>;
    uint32_t index = 0;
    int bitmapMax = 0;
    TrueTypeFont::BitmapInfo info;
    for (int i = 0; i < unicodeLen; i++) {
        string letter(info.utf8);
        bar.SetValue(i + 1, &letter);

        uint8_t *map = font->GetBitmap(unicode[i], info);

        glyph_dsc_level1_t dsc = {
            .bitmap_index = index,
            .adv_w = (uint8_t)info.real_w,
            .box_w = (uint8_t)info.w,
            .box_h = (uint8_t)info.h,
            .ofs_x = (int8_t)(font->GetGlyph().horiBearingX / 64),
            .ofs_y = (int8_t)((font->GetGlyph().horiBearingY - font->GetGlyph().height) / 64),
            .unicode = unicode[i],
        };
        memcpy(dsc.utf8, info.utf8, sizeof(dsc.utf8));
        glyph_dsc->push_back(dsc); // 保存字符信息表
        int bitmapSize = info.line_byte * info.h;

        if (info.bpp == 1) {
            map = ToLvgl1bpp(&map, info, bitmapSize);
        }

        if (map != nullptr) {
            fwrite(map, bitmapSize, 1, binFile); // 写入字符位图
        }

        index += bitmapSize;                                         // 计算下一个字符信息的偏移量
        bitmapMax = bitmapSize > bitmapMax ? bitmapSize : bitmapMax; // 记录最大的位图大小

        delete[] map;
    }

    // 写入各个字符信息到 C 文件
    fprintf(cFile, "%s", templateGlyphDscStart.c_str());
    WriteCharGlyphToCFile(cFile, glyph_dsc);
    fprintf(cFile, "%s", templateArrayEnd.c_str());

    // 写入 unicode 表到 C 文件
    fprintf(cFile, "%s", templateUnicodeListStart.c_str());
    WriteUnicodeListToCFile(cFile, glyph_dsc);
    fprintf(cFile, "%s", templateArrayEnd.c_str());

    // 写入函数到 C 文件
    fprintf(cFile, templateBinFunctionLevel1Fmt.c_str(), bitmapMax);
}

#endif

/**
 * @brief 对输入的字符进行预处理，包括 ascii、汉字添加、删除控制字符、排序和去重操作
 * @param letters 预处理的字符串
 * @param letterNum 保存处理过后的 unicode 字符数目
 * @retval 处理成功则返回 unicode 数组，失败返回 nullptr
 * @note 返回的 Unicode 数组再不使用后需使用 delete[] 释放
 */
static char16_t *ConverPreprocess(string &letters, int &letterNum, uint32_t cfg)
{
    // 加入 ascii 字符?
    if (cfg & ConverAdd_Ascii) {
        letters += asciiString;
    }

    // 加入汉字?(常用 or 所有)
    if (cfg & ConverAdd_zHanCommonUse) {
        letters += zHancommonUse;
    } else if (cfg & ConverAdd_zHanAll) {
        letters += zHanAll;
    }

    // 计算字符串长度
    int letterSize = letters.length();

    // 将 utf8 转为 unicode，并计算字符个数
    char16_t *letterUnicode = new char16_t[letterSize + 1];
    letterNum = TrueTypeFont::UTF8ToUnicode(letterUnicode, letters.c_str(), letterSize + 1);
    printf("\tinput letterNum:%d\n", letterNum);
    if (letterNum <= 0) {
        delete[] letterUnicode;
        return nullptr;
    }

    // 对字符进行升序排序
    sort(letterUnicode, letterUnicode + letterNum);

    // 除去重复的字符
    letterNum = unique(letterUnicode, letterUnicode + letterNum) - letterUnicode;

    // 除去控制字符
    auto prev = [](char16_t ch) { return ch < 0x20; }; // lambda 表达式
    letterNum = remove_if(letterUnicode, letterUnicode + letterNum, prev) - letterUnicode;

    printf("\tfinal letterNum:%d\n", letterNum);

    return letterUnicode;
}

/**
 * @brief 写入字符位图到C文件
 */
static void WriteCharBitmapToCFile(FILE *file, uint8_t *bitmap, TrueTypeFont::BitmapInfo &info)
{
    int pixelPerByte = 8 / info.bpp;
    int lineByte = info.line_byte;
    int total = info.h * lineByte;
    int i = 0;

    if (info.bpp == 1) {
        total = info.w;
        total *= info.h;
        total = total % 8 ? total / 8 + 1 : total / 8;
    }

    // 输出字符信息
    fprintf(file, "/* %s */\n", info.utf8);

    // 输出字符位图
    uint8_t *pLineData = bitmap;
    while (total--) {
        uint8_t temp = bitmap[i++];

        fprintf(file, "0x%02x, ", temp);

        if (i % lineByte == 0) {
            if (info.bpp != 1) // 因lvgl的1bpp绘制方法与其余不同，为避免冗余，不绘制
            {
                fprintf(file, "\t//");
                for (int j = 0; j < lineByte; j++) {
                    uint8_t byte = pLineData[j];
                    for (int k = 0; k < pixelPerByte; k++) {
                        uint8_t pixelData = byte >> (8 - info.bpp);
                        char c = BitmapGetChar(pixelData, info.bpp);
                        fputc(c, file);
                        byte <<= info.bpp;
                    }
                }
            }

            fprintf(file, "\n");
            pLineData = &bitmap[i];
        }
    }

    fprintf(file, "\n\n");
}

template <typename glyphVector> static void WriteCharGlyphToCFile(FILE *file, glyphVector *glyph)
{
    if (glyph == nullptr) {
        return;
    }

    typename glyphVector::const_iterator it = glyph->begin();
    while (it != glyph->end()) {
        fprintf(file, templateGlyphDscFmt.c_str(), it->bitmap_index, it->adv_w, it->box_h, it->box_w, it->ofs_x,
                it->ofs_y, it->utf8);
        it++;
    }
}

template <typename glyphVector> static void WriteUnicodeListToCFile(FILE *file, glyphVector *glyph)
{
    if (glyph == nullptr) {
        return;
    }

    typename glyphVector::const_iterator it = glyph->begin();
    while (it != glyph->end()) {
        fprintf(file, templateUnicodeListFmt.c_str(), it->unicode, it->utf8);
        it++;
    }
    fprintf(file, templateUnicodeListFmt.c_str(), 0, "list end");
}

static uint8_t *ToLvgl1bpp(uint8_t **bitmap, TrueTypeFont::BitmapInfo &info, int &newMapSize)
{
    int mapSize = info.line_byte * info.h;
    int outMapSize = info.w;
    outMapSize *= info.h;
    outMapSize = outMapSize % 8 ? outMapSize / 8 + 1 : outMapSize / 8;
    if (info.h == 0) {
        outMapSize = 0;
    }

    uint8_t *newMap = new uint8_t[outMapSize];

    uint8_t *pSrc = *bitmap;
    int srcRShiftBit = 0;
    for (int i = 0; i < outMapSize; i++) {
        uint8_t temp = 0;

        for (int j = 0; j < 8; j++) {
            int srcCount = i * 8 + j;

            if ((srcCount % info.w == 0 && srcCount) || (srcRShiftBit == 8)) {
                pSrc++;
                srcRShiftBit = 0;
            }

            temp <<= 1;
            temp |= !!(*pSrc & (0x80 >> srcRShiftBit));

            srcRShiftBit++;
        }

        newMap[i] = temp;
    }

    delete[] *bitmap;

    newMapSize = outMapSize;
    return newMap;
}

static char BitmapGetChar(uint8_t data, int bpp)
{
    char ch = '.';

    if (data == 0) {
        return ch;
    }

    switch (bpp) {
    case 1:
        ch = '@';
        break;
    case 2:
        ch = bitmapCh[1][data % 4];
        break;
    case 4:
        if (data / 3 == 0 && data > 0) {
            ch = '+';
        } else {
            ch = bitmapCh[2][data / 3];
        }
        break;
    case 8:
        if (data / 32 == 0 && data > 0) {
            ch = '+';
        } else {
            ch = bitmapCh[3][data / 32];
        }
        break;
    default:
        break;
    }

    return ch;
}