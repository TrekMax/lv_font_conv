#include "TrueTypeFont.h"
#include <math.h>
#include "File.h"

TrueTypeFont::TrueTypeFont(string font_file)
{
    _fontSize = 24;
    _bold = false;
    _angle = 0;
    _bpp = 4; // 默认 4bit 抗锯齿
    fontName = FileNotDir(font_file);

    FT_Init_FreeType(&_library); // 初始化 freetype 库

    // 创建字体文件对象
    _error = FT_New_Face(_library, font_file.c_str(), 0, &_face);
    if (_error != 0)
    {
        status = FontFileNoFoundOrIllegal; // 找不到字库或字库非法
        return;
    }

    status = Font_NoInput;
}

TrueTypeFont::~TrueTypeFont()
{
    FT_Done_Face(_face);
    FT_Done_FreeType(_library);
}

uint8_t *TrueTypeFont::GetBitmap(char16_t ch, BitmapInfo &info)
{
    if (status == FontFileNoFoundOrIllegal)
        return nullptr;

    FT_Set_Pixel_Sizes(_face, 0, _fontSize); // 设置字高

    _slot = _face->glyph;

    if (_angle != 0)
    {
        // 初始化旋转矩阵
        _matrix.xx = (FT_Fixed)(cos(_angle) * 0x10000L);
        _matrix.xy = (FT_Fixed)(-sin(_angle) * 0x10000L);
        _matrix.yx = (FT_Fixed)(sin(_angle) * 0x10000L);
        _matrix.yy = (FT_Fixed)(cos(_angle) * 0x10000L);

        FT_Set_Transform(_face, &_matrix, &_pen); // 旋转变换
    }

    uint32_t mono_flag = 0;
    if(_bpp == 1)
        mono_flag = FT_LOAD_MONOCHROME;

    // 加载字符位图
    _error = FT_Load_Char(_face, ch, FT_LOAD_RENDER | mono_flag);
    if (_error != 0)
    {
        status = FontCharNoFound;
        return nullptr;
    }

    FT_Bitmap *bitMap = &(_slot->bitmap);
    int mapSize = bitMap->rows * abs(bitMap->pitch);

    info.real_w = bitMap->width;
    if(_bpp == 8 || _bpp == 1)
        info.w = info.real_w;
    else
    {
        int bits = 8 / _bpp;
        int temp = info.real_w % bits;
        info.w = temp ? info.real_w + (bits - temp) : info.real_w; // 宽度为(8/bpp)的倍数数
    }

    if(ch == ' ')
    {
        info.w = _fontSize / 4;
        info.real_w = info.w;
        info.h = 0;
    }
    else
        info.h = bitMap->rows;
    
    info.bpp = _bpp;
    info.unicode = ch;
    info.line_byte = abs(bitMap->pitch);

    char16_t unicode[2] = {info.unicode, '\0'};
    UnicodeToUTF8(info.utf8, unicode, sizeof(info.utf8));

    int pixelPerByte = 8 / _bpp;                    // 每个字节存储的像素数据个数
    int outMapRealSize = mapSize / pixelPerByte;    // 原定输出大小，按照原位图数据大小 / 2
    int outMapLineByte = info.line_byte;

    if(_bpp != 1 && _bpp != 8)
    {
        outMapLineByte = outMapLineByte % pixelPerByte ? outMapLineByte / pixelPerByte + 1 : outMapLineByte / pixelPerByte;
        info.line_byte = outMapLineByte;
    }

    int outMapExtSize = outMapLineByte * info.h; // 实际输出的位图大小，若宽度为偶数则两者相等
    
    uint8_t *map = new uint8_t[outMapExtSize];

    if (_bpp == 8 || _bpp == 1) // 像素宽度若是8bit和1bit直接拷贝
        memcpy(map, bitMap->buffer, mapSize);
    else
    {
        uint8_t *pSrcMap = bitMap->buffer;
        for (int i = 0; i < outMapExtSize; i++) // 将原始8bit位图转换为对应bpp的位图
        {
            uint8_t temp = 0;
            for (int j = 0; j < pixelPerByte; j++)
            {
                int srcCount = i * pixelPerByte + j;    //计算当前是第几个字节
                uint8_t src = 0;

                //若原本宽度为奇数，则大于宽度的列补0
                int lineSrcCount = srcCount % (pixelPerByte * info.line_byte);
                if (outMapExtSize != outMapRealSize && lineSrcCount >= info.real_w && srcCount)      
                    ;
                else
                    src = *pSrcMap++;

                temp <<= _bpp;

                temp |= src >> (8 - _bpp);
            }

            map[i] = temp;
        }
    }

    return map;
}

void TrueTypeFont::SetBpp(int bpp)
{
    if (bpp != 1 && bpp != 2 && bpp != 4 && bpp != 8)
        return;

    _bpp = bpp;
}

#define _DF1S 0x81

int TrueTypeFont::UnicodeToUTF8(char *dest, const char16_t *src, int bufflen)
{
    int i = 0;
    char16_t chr;

    while (*src != '\0')
    {
        chr = *src++;
        if (chr < _DF1S)
        {
            if (i + 1 > bufflen - 1)
                return -1;

            dest[i++] = chr;
            continue;
        }

        if (i + 3 > bufflen - 1)
            return -2;

        dest[i++] = ((chr >> 12) & 0x0F) | 0xE0;
        dest[i++] = ((chr >> 6) & 0x3F) | 0x80;
        dest[i++] = ((chr >> 0) & 0x3F) | 0x80;
    }

    dest[i] = '\0';

    return i;
}

int TrueTypeFont::UTF8ToUnicode(char16_t *dest, const char *src, int bufflen)
{
    int i = 0;
    char b1, b2, b3; // b1 表示 UTF-8 编码的高字节, b2 表示次高字节, ...

    while (*src != '\0')
    {
        if ((uint8_t)*src < _DF1S)
        {
            if (i + 1 > bufflen - 1)
                return -1;

            dest[i++] = *src++;
            continue;
        }

        b1 = src[0];
        b2 = src[1];
        b3 = src[2];

        if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80))
            return -2;

        if (i + 2 > bufflen - 2)
            return -3;

        dest[i++] = (((b2 << 6) + (b3 & 0x3F)) & 0xFF) + ((((b1 << 4) + ((b2 >> 2) & 0x0F)) & 0xFF) << 8);

        src += 3;
    }
    dest[i] = '\0';

    return i;
}