#include "ConsoleBar.h"
#include <string.h>

ConsoleBar::ConsoleBar()
{
    fillCharacter = '*';
    emptyCharacter = ' ';
    rangMax = 100;
    rangMin = 0;
    value = 0;

    perfixString = "";

    buffer = new char[50 + 1];
    memset(buffer, 0, 50 + 1);
}

ConsoleBar::~ConsoleBar()
{
    putchar('\n');

    delete[] buffer;
}

void ConsoleBar::SetValue(int v, string *perfix)
{
    int progress;

    value = v;
    if (perfix != nullptr) {
        perfixString = *perfix;
    }

    progress = value * 100 / (rangMax - rangMin);

    for (int i = 0; i < 50; i++) {
        if (i <= progress / 2) {
            buffer[i] = fillCharacter;
        } else {
            buffer[i] = emptyCharacter;
        }
    }

    printf("[%s][%-50s][%d%%]       \r", perfixString.c_str(), buffer, progress);
    fflush(stdout);
}
