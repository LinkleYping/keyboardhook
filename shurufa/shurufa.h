#ifndef SHURUFA_H  
#define SHURUFA_H  

#include <windows.h>  
#include <imm.h>  
#include <stdio.h>  
#include <string.h>  
#pragma comment(lib,"Imm32.lib")  

extern "C" void writefile(char *lpstr);
extern "C" void writtitle();
extern "C" __declspec(dllexport) BOOL InstallHook();
extern "C" __declspec(dllexport) BOOL UnHook();

#endif 