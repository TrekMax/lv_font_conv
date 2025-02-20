#ifndef _FILE_H_
#define _FILE_H_

#include <string>

using namespace std;

string FileNotDir(string &filePath);
string FileGetExtension(string &fileName);
string FileGetNameWithoutExtension(string &fileName);

#endif
