#include <stdio.h> 
#include <windows.h>
#include <conio.h>

typedef DWORD(*MY_EXPORT_PROC)();

int main()
{
	//HWND hwndDOS;
	//hwndDOS = GetForegroundWindow();
	//ShowWindow(hwndDOS, SW_HIDE);
	HINSTANCE hinstLib;
	MY_EXPORT_PROC lpProcAdd, lpProcAdd1, lpProcAdd2;
	BOOL fFreeResult, fRunTimeLinkSuccess = FALSE;
	hinstLib = LoadLibrary(TEXT("shurufa.dll"));

	if (hinstLib != NULL)
	{
		lpProcAdd = (MY_EXPORT_PROC)GetProcAddress(hinstLib, "InstallHook");
		if (NULL != lpProcAdd)
		{
			fRunTimeLinkSuccess = TRUE;
			(lpProcAdd)();
			printf("加载成功\n");
		}

		while (1) {
			Sleep(10000000);
		}

		lpProcAdd2 = (MY_EXPORT_PROC)GetProcAddress(hinstLib, "UnHook");
		if (NULL != lpProcAdd2)
		{
			fRunTimeLinkSuccess = TRUE;
			(lpProcAdd2)();
			printf("加载成功\n");
		}
	}
	return 0;
}