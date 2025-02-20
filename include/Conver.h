#ifndef _CONVER_H_
#define _CONVER_H_

#include <stdio.h>
#include <stdint.h>
#include <string>

#include "TrueTypeFont.h"

using namespace std;

enum ConverCfg {
    Conver_None = 0,

    ConverAdd_Ascii = 0x100,
    ConverAdd_zHanCommonUse = 0x200, // 常用汉字和所有汉字二者互斥，且All优先级较高
    ConverAdd_zHanAll = 0x400,

    ConverBin_Level0 = 0x1000, // 字体信息等级,二者互斥
    ConverBin_Level1 = 0x2000,
};

enum ConverFile {
    Conver_C_File = 0,
    Conver_Bin_File,
};

bool ConverCharToCFile(string &letters, FILE *file, TrueTypeFont *font, string fontName, uint32_t cfg = Conver_None);
bool ConverCharToBinFile(string &letters, FILE *cfile, FILE *binFile, TrueTypeFont *font, string fontName,
                         uint32_t cfg = ConverBin_Level0);

#endif
