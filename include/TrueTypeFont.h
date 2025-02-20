#ifndef _TRUETYPEFONT_H_
#define _TRUETYPEFONT_H_

#include <string>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include <cstdint>

using namespace std;

class TrueTypeFont
{
  public:
    enum Status {
        Font_OK = 0,
        Font_NoInput,
        FontFileNoFoundOrIllegal,
        FontCharNoFound,
    };

    struct BitmapInfo {
        char16_t unicode;
        char utf8[4];
        int real_w; // 位图真实宽度
        int w;      // lvgl 位图宽度，必为偶数
        int h;
        int bpp;       // 与 _bpp 相等
        int line_byte; // 一行的字节数
    };

  private:
    FT_Library _library; /* 库 */
    FT_Face _face;       /* 字体文件对象 */
    FT_Vector _pen;      /* 矢量字体的原点  */
    FT_Error _error;     /* 错误码 */
    FT_GlyphSlot _slot;  /* glyph 插槽 */
    FT_Matrix _matrix;   /* 矩阵，通过矩阵转置实现字体旋转 */

    string fontName; // 字库的名称
    int _fontSize;   // 字体高度(pixel)
    double _angle;   /* 旋转的角度 */
    bool _bold;      // 加粗
    int _bpp;        // 每个像素 bit 数
    Status status;

  public:
    TrueTypeFont(std::string font_file);
    ~TrueTypeFont();

    uint8_t *GetBitmap(char16_t ch, BitmapInfo &info);

    void SetBpp(int bpp);
    void SetFontSize(int fontSize)
    {
        _fontSize = fontSize;
    }

    // 字体加粗，还未实现
    void SetBold(bool en)
    {
        _bold = en;
    }
    string &GetFontName(void)
    {
        return fontName;
    }
    Status GetStatus(void)
    {
        return status;
    }
    int GetFontSize(void)
    {
        return _fontSize;
    }
    int GetBpp(void)
    {
        return _bpp;
    }
    const FT_Glyph_Metrics &GetGlyph(void) const
    {
        return _slot->metrics;
    }

    static int UnicodeToUTF8(char *dest, const char16_t *src, int bufflen);
    static int UTF8ToUnicode(char16_t *dest, const char *src, int bufflen);
};

#endif
