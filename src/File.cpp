#include "File.h"


/**
 * @brief 从文件路径提取文件名
*/
string FileNotDir(string &filePath)
{
    // 取得最后一个路径分隔符的位置
    size_t position = filePath.find_last_of("/\\");
    if (position == string::npos || filePath == "")
        return filePath;

    // 截取最后一个路径分隔符后面的字符串即为文件名
    return filePath.substr(position + 1, filePath.length() - position - 1);
}

/**
 * @brief 找到文件后缀名
*/
string FileGetExtension(string &fileName)
{
    if (fileName == "")
        return fileName;

    // 查找最后一个小数点的位置作为扩展名的分隔符
    size_t dot = fileName.find_last_of(".");
    if (dot == string::npos)
        return string();

    // 截取小数点之后的字符串即为扩展名
    return fileName.substr(dot + 1, fileName.length() - dot - 1);
}

/**
 * @brief 找到文件名，不包含后缀
*/
string FileGetNameWithoutExtension(string &fileName)
{
    string name = FileNotDir(fileName);

    // 查找最后一个小数点的位置作为扩展名的分隔符
    size_t dot = fileName.find_last_of(".");
    if (dot == string::npos)    
        return name;    //找不到小数点，直接返回

    // 截取小数点之前的字符串即为文件名
    return fileName.substr(0, dot);
}