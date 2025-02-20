#include <stdio.h>
#include <string>

#include "Conver.h"
#include "Param.h"
#include "TrueTypeFont.h"

using namespace std;

static char BitmapGetChar(uint8_t data, int bpp);

int main(int argc, const char *argv[])
{
    if (argc == 1) {
        printf("Plese add \"-h\" param to show the help information\n");
        return 0;
    }

    int error;
    ConverParam *param = ConverParamDecode(argc, argv, &error); // 进行参数解析
    if (error != 0) {
        printf("param error: %s\n", ParamErrorToString(error));
        return 0;
    }
    if (param == nullptr) { // error为0但param为nullptr表示显示帮助信息(已显示)
        return 0;
    }

    printf("\tparam.bpp:%d\n", param->bpp);
    printf("\tparam.size:%d\n", param->size);
    printf("\tparam.outFileType:%d\n", param->outFileType);
    printf("\tparam.cLevel:%d\n", param->cLevel);
    printf("\tparam.ascii:%d\n", param->ascii);
    printf("\tparam.zHanAll:%d\n", param->zHanAll);
    printf("\tparam.zHanCom:%d\n", param->zHanCom);
    printf("\tparam.outPath:%s\n", param->outPath.c_str());
    printf("\tparam.outFileName:%s\n", param->outFileName.c_str());
    printf("\tparam.fontFile:%s\n", param->fontFile.c_str());
    printf("\tparam.inputFileName:%s\n", param->inputFileName.c_str());

    string inputString = "";
    FILE *txtFile = nullptr;
    if (param->inputFileName != "") {
        txtFile = fopen(param->inputFileName.c_str(), "r");
        fseek(txtFile, 0, SEEK_END);
        int fileSize = ftell(txtFile); // 获取文件大小
        fseek(txtFile, 0, SEEK_SET);
        char *buf = new char[fileSize];
        fread(buf, fileSize, 1, txtFile); // 读取文本
        inputString.append(buf, fileSize);

        delete[] buf;
        fclose(txtFile);
    }

    TrueTypeFont *font = new TrueTypeFont(param->fontFile);
    if (font->GetStatus() != TrueTypeFont::Font_NoInput) {
        printf("can not open font file:%s\n", param->fontFile.c_str());
        delete font;
        return 0;
    }

    string cFileName = param->outPath + param->outFileName + string(".c");
    string binFileName = param->outPath + param->outFileName + string(".bin");
    FILE *cFile = nullptr;
    FILE *binFile = nullptr;
    uint32_t cfg = 0;
    cfg |= param->ascii ? ConverAdd_Ascii : 0;
    cfg |= param->cLevel == 1 ? ConverBin_Level1 : ConverBin_Level0;
    cfg |= param->zHanCom ? ConverAdd_zHanCommonUse : 0;
    cfg |= param->zHanAll ? ConverAdd_zHanAll : 0;

    font->SetFontSize(param->size);
    font->SetBpp(param->bpp);

    cFile = fopen(cFileName.c_str(), "w");

    switch (param->outFileType) {
    case Conver_C_File: // 转为 C 文件，字库为 C 文件数组
        ConverCharToCFile(inputString, cFile, font, param->outFileName, cfg);
        break;
    case Conver_Bin_File: // 字库转为 bin 文件，字体信息保存在 C 文件
        binFile = fopen(binFileName.c_str(), "w");
        ConverCharToBinFile(inputString, cFile, binFile, font, param->outFileName, cfg);
        break;
    default:
        break;
    }

    fclose(cFile);

    if (binFile != nullptr) {
        fclose(binFile);
    }

    delete font;

    return 0;
}
