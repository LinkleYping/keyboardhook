#include <stdlib.h>
#include <ntddk.h>
#include <windef.h>

// ���� SSDT(ϵͳ����������) �з�������������Ŀ,���ﶨ��Ϊ 1024 ��
#define MAX_SYSTEM_SERVICE_NUMBER 1024
//����ϵͳ�����ַ
ULONG oldSysServiceAddr[MAX_SYSTEM_SERVICE_NUMBER];

// ���� SSDT �ṹ,KSYSTEM_SERVICE_TABLE �� KSERVICE_TABLE_DESCRIPTOR
typedef struct _KSYSTEM_SERVICE_TABLE
{
	PULONG  ServiceTableBase;							// SSDT�Ļ���ַ
	PULONG  ServiceCounterTableBase;                    // ���� checked builds, ���� SSDT ��ÿ�����񱻵��õĴ���
	ULONG   NumberOfService;                            // �������ĸ���
	ULONG   ParamTableBase;                             // SSPT(System Service Parameter Table)�Ļ���ַ
} KSYSTEM_SERVICE_TABLE, *PKSYSTEM_SERVICE_TABLE;

typedef struct _KSERVICE_TABLE_DESCRIPTOR
{
	KSYSTEM_SERVICE_TABLE   ntoskrnl;                   // ntoskrnl.exe �ķ�����
	KSYSTEM_SERVICE_TABLE   win32k;                     // win32k.sys �ķ�����(GDI32.dll/User32.dll ���ں�֧��)
	KSYSTEM_SERVICE_TABLE   notUsed1;
	KSYSTEM_SERVICE_TABLE   notUsed2;
}KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

extern  PKSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable;	//��ntoskrnl.exe������

//���� ZwServiceFunction ��ȡ ntServiceFunction ��SSDT������Ӧ�ķ����������
#define SYSCALL_INDEX(ServiceFunction) (*(PULONG)((PUCHAR)ServiceFunction + 1))
//���� ZwServiceFunction ����÷�����SSDT�е������ţ�Ȼ����ͨ��������������ȡ ntServiceFunction �ĵ�ַ
#define SYSCALL_FUNCTION(ServiceFunction) KeServiceDescriptorTable->ntoskrnl.ServiceTableBase[SYSCALL_INDEX(ServiceFunction)]


#define MAX_BUFFER_LENGTH 1024	//ģ���ļ��Ļ���������󳤶�
typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT pDevice;
	UNICODE_STRING ustrDeviceName;	//�豸����
	UNICODE_STRING ustrSymLinkName;	//����������
	PUCHAR buffer;//������
	ULONG file_length;//ģ����ļ�����
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// ��������Ӧ�ó������������ͨ�ŵĺ꣬����ʹ�õ��ǻ�������д��ʽ
#define IO_INSERT_PROTECT_PROCESS	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IO_REMOVE_PROTECT_PROCESS	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED,FILE_ANY_ACCESS)
//#define IO_REMOVE_HOOK	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define MAX_PROTECT_ARRARY_LENGTH 1024
ULONG protectArray[MAX_PROTECT_ARRARY_LENGTH];	//������Ҫ�������̵�pid����
ULONG curProtectArrayLen = 0;	//����������pid�ĸ���

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

typedef NTSTATUS(*NTTERMINATEPROCESS)(__in_opt HANDLE ProcessHandle, __in NTSTATUS ExitStatus);
NTTERMINATEPROCESS realNtTerminateProcess;	//��������NtTerminateProcess�ĵ�ַ

//��������
NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject);		//�����豸
NTSTATUS DispatchCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp);
VOID DriverUnLoad(IN PDRIVER_OBJECT pDriverObject);
NTSTATUS DispatchClose(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS DispatchIoctl(PDEVICE_OBJECT pDevObj, PIRP pIrp);


NTSTATUS HookNtTerminateProcess(__in_opt HANDLE ProcessHandle, __in NTSTATUS ExitStatus);
VOID BackupSysServicesTable();
extern UCHAR* PsGetProcessImageFileName(PEPROCESS EProcess);

NTSTATUS InstallHook(ULONG oldService, ULONG newService);
NTSTATUS UnInstallHook(ULONG oldService);
void DisableWrite();
void EnableWrite();

//��֤ uPID ������Ľ����Ƿ�����ڱ��������б��У����ж� uPID ��������Ƿ���Ҫ����
ULONG validateProcess(ULONG uPID);
// �򱣻������б��в��� uPID
ULONG InsertProtectProcess(ULONG uPID);
// �ӱ��������б����Ƴ� uPID
ULONG RemoveProtectProcess(ULONG uPID);


