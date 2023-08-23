#ifndef _CONSOLEBAR_H_
#define _CONSOLEBAR_H_

#include <string>

using namespace std;

class ConsoleBar
{

private:
    char fillCharacter;     //已填充的字符
    char emptyCharacter;    //未填充的字符

    int rangMax;    //最大值
    int rangMin;    //最小值

    char *buffer;   
    string perfixString;  //进度条前缀

    int value;      //当前值
public:
    ConsoleBar();
    ~ConsoleBar();

    void SetPerfix(string &perfix) { perfixString = perfix; }
    void SetRange(int min, int max) { rangMax = max; rangMin = min;}
    void SetCharacter(char fill, char empty) { fillCharacter = fill; emptyCharacter = empty; }
    
    void SetValue(int v, string *perfix = nullptr);
};



#endif
