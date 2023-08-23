#ifndef _PARAM_H_
#define _PARAM_H_


#include <string>


using namespace std;


struct ConverParam
{   
    int bpp;        //字体位数
    int size;       //字体尺寸(高度)           
    int outFileType;    //字库输出文件类型(C文件 / bin文件)
    int cLevel;     //C文件保存字体信息等级

    bool ascii;     //是否加入ascii字符
    bool zHanAll;   //是否加入所有汉字
    bool zHanCom;   //是否加入常用汉字

    string outPath;         //输出路径
    string outFileName;     //输出文件名

    string fontFile;    //字体文件

    string inputFileName;   //输入txt文件名
};


ConverParam *ConverParamDecode(int argc, const char *argv[], int *error);
const char *ParamErrorToString(int error);

#endif
