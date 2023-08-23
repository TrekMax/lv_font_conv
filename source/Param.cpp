#include "Param.h"
#include <regex>
#include "Conver.h"

#define memberof(buf) (sizeof(buf) / sizeof(buf[0]))

static int ParamGetNumber(string arg);

extern const char *helpInfo; // 帮助信息，定义于此文件末尾

// 参数表
static const char *params[] = {
    "-h",
    "-o",
    "-i",
    "-t",
    "-c",
    "-b",
    "--size",
    "--bpp",
    "--ascii",
    "--zHanAll",
    "--zHanCom",
    "--level",
};

static const char *outfileString[] = {
    "c-array",
    "bin",
};

static const char *errorString[] = {
    "",
    "\"-o\" Can not find a output file name",
    "\"-i\" Can not find a input file name",
    "\"-t\" Can not find a truetype font file",
    "\"--size\" Font size no support, range:8 < size < 256",
    "\"--bpp\" Font bpp no support, legal: 1,2,4,8",
    "\"--level\" Font c level no support legal: 0,1",
    "Please use \"-t\" to appoint a input font file",
    "No any characters can be convered!",
};

/**
 * @brief 转换参数解析
 * @param argc 参数个数
 * @param argv 参数集合
 * @param error 保存错误码
 * @retval 解析成功 - 参数结构
 * @retval 解析失败- nullptr
 */
ConverParam *ConverParamDecode(int argc, const char *argv[], int *error)
{
    static ConverParam param;

    // 默认参数
    param.bpp = 4;
    param.size = 24;
    param.outFileType = Conver_C_File;
    param.cLevel = 0;
    param.ascii = false;
    param.zHanAll = false;
    param.zHanCom = false;
    param.outPath = "./";
    param.outFileName = "myFont";

    for (int i = 0; i < argc; i++)
    {
        string arg(argv[i]);

        int index = 0;
        for (; index < memberof(params); index++)
        {
            if (arg.compare(params[index]) == 0) // 找到匹配的参数
                break;
        }

        switch (index)
        {
        case 0:
            *error = 0;
            printf("%s\n", helpInfo); // 显示帮助信息
            return nullptr;
        case 1:
            if (i + 1 == argc) // 后面没有参数了
            {
                *error = -1;
                return nullptr;
            }
            param.outFileName = string(argv[++i]); // 保存输出文件名
            break;
        case 2:
            if (i + 1 == argc) // 后面没有参数了
            {
                *error = -2;
                return nullptr;
            }
            param.inputFileName = string(argv[++i]); // 保存输入文件名
            break;
        case 3:
            if (i + 1 == argc) // 后面没有参数了
            {
                *error = -3;
                return nullptr;
            }
            param.fontFile = string(argv[++i]); // 保存ttf文件
            break;
        case 4: // 输出c文件
            param.outFileType = Conver_C_File;
            break;
        case 5: // 输出bin文件
            param.outFileType = Conver_Bin_File;
            break;
        case 6:  // 字体尺寸(高度
        case 7:  // 字体位数bpp
        case 11: // C文件字体信息等级
        {
            static map<int, int> errorCode = {{6, -4}, {7, -5}, {11, -6}};
            static map<int, int *> var = {{6, &param.size}, {7, &param.bpp}, {11, &param.cLevel}};
            int num;
            if (i + 1 == argc || (num = ParamGetNumber(string(argv[++i]))) < 0) // 后面没有参数了或转换成数值失败
            {
                *error = errorCode[index];
                return nullptr;
            }
            *var[index] = num;
        }
        break;
        case 8: // ascii?
            param.ascii = true;
            break;
        case 9: // 所有汉字?
            param.zHanAll = true;
            break;
        case 10: // 常用汉字?
            param.zHanCom = true;
            break;

        default:
            break;
        }
    }

    if (param.fontFile == "") // 未指定ttf文件
    {
        *error = -7;
        return nullptr;
    }

    if(param.inputFileName != "")
    {
        FILE *txtFile = nullptr;
        txtFile = fopen(param.inputFileName.c_str(), "r");
        if(txtFile == nullptr)
        {
            printf("param error: Can not open input file \"%s\"\n", param.inputFileName.c_str());
            param.inputFileName = "";
        }
            
        else
            fclose(txtFile);
    }

    if(!param.ascii && !param.zHanCom && !param.zHanAll && param.inputFileName == "")   //没有任何字符可以转换
    {
        *error = -8;
        return nullptr;
    }

    if (param.outFileName != "") // 截取指定的输出路径和文件名
    {
        int dirEndPos = param.outFileName.find_last_of('/');
        if (dirEndPos != string::npos)
        {
            param.outPath = param.outFileName.substr(0, dirEndPos + 1); // 提取路径

            int pathLen = param.outPath.length();
            param.outFileName = param.outFileName.substr(pathLen); // 减去路径

            if (param.outFileName == "") // 减去路径后若为空
            {
                *error = -1;
                return nullptr;
            }
        }
    }

    *error = 0;

    return &param;
}

static int ParamGetNumber(string arg)
{
    char *endprt;
    long int num = -1;

    num = strtol(arg.c_str(), &endprt, 10);

    if (*endprt != '\0')
        return -1;

    return num;
}

const char *ParamErrorToString(int error)
{
    error = abs(error);

    if (error > memberof(errorString))
        return nullptr;

    return errorString[error];
}

const char *helpInfo = "\
\
lv_font_conv: version 0.1\n\
\t-o\t\t指定输出文件名, 同时作为变量名(文件名不包括后缀)\n\
\t\t\t\t若不指定该参数则默认为myFont\n\
\t\t\t\t此选项还可以指定输出路径,例:\"../src/font.c\"\n\
\t-i\t\t指定输入文件, 存有转换字符的txt文本文档(可选)\n\
\t-t\t\t指定TrueType字库文件(.ttf)，此为必要参数\n\
\t-c\t\t字体位图输出到C文件(数组), 与\"-b\"互斥(默认参数)\n\
\t-b\t\t字体位图输出为bin文件, 与\"-c\"互斥\n\
\t--size\t\t指定字体尺寸(高度), 单位:像素, 默认值24\n\
\t--bpp\t\t指定像素位数(抗锯齿), 可选的有1,2,4(默认值),8\n\
\t--ascii\t\t转换前加入ascii字符\n\
\t--zHanAll\t转换前加入所有汉字\n\
\t--zHanCom\t转换前加入常用汉字(约6000个)\n\
\t--level\t\t指定-b输出到bin文件时, 保存在C文件的字体信息等级, 可选的有0(默认值),1\n\
\t\t\t\t数值越大, C文件保存的字体信息越多, 文件IO次数越少,\n\
\t\t\t\t渲染速度越快, 但C文件占用空间越大\n\
\n\
Tips: -i --ascii --zHanAll --zHanComm四个选项中请至少指定一个选项,\n\
以确保有字符可转换.\n\
\
";
