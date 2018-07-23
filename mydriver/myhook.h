#include <stdlib.h>
#include <ntddk.h>
#include <windef.h>

// 定义 SSDT(系统服务描述表) 中服务个数的最大数目,这里定义为 1024 个
#define MAX_SYSTEM_SERVICE_NUMBER 1024
//备份系统服务地址
ULONG oldSysServiceAddr[MAX_SYSTEM_SERVICE_NUMBER];

// 定义 SSDT 结构,KSYSTEM_SERVICE_TABLE 和 KSERVICE_TABLE_DESCRIPTOR
typedef struct _KSYSTEM_SERVICE_TABLE
{
	PULONG  ServiceTableBase;							// SSDT的基地址
	PULONG  ServiceCounterTableBase;                    // 用于 checked builds, 包含 SSDT 中每个服务被调用的次数
	ULONG   NumberOfService;                            // 服务函数的个数
	ULONG   ParamTableBase;                             // SSPT(System Service Parameter Table)的基地址
} KSYSTEM_SERVICE_TABLE, *PKSYSTEM_SERVICE_TABLE;

typedef struct _KSERVICE_TABLE_DESCRIPTOR
{
	KSYSTEM_SERVICE_TABLE   ntoskrnl;                   // ntoskrnl.exe 的服务函数
	KSYSTEM_SERVICE_TABLE   win32k;                     // win32k.sys 的服务函数(GDI32.dll/User32.dll 的内核支持)
	KSYSTEM_SERVICE_TABLE   notUsed1;
	KSYSTEM_SERVICE_TABLE   notUsed2;
}KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

extern  PKSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable;	//由ntoskrnl.exe导出的

//根据 ZwServiceFunction 获取 ntServiceFunction 在SSDT中所对应的服务的索引号
#define SYSCALL_INDEX(ServiceFunction) (*(PULONG)((PUCHAR)ServiceFunction + 1))
//根据 ZwServiceFunction 来获得服务在SSDT中的索引号，然后再通过该索引号来获取 ntServiceFunction 的地址
#define SYSCALL_FUNCTION(ServiceFunction) KeServiceDescriptorTable->ntoskrnl.ServiceTableBase[SYSCALL_INDEX(ServiceFunction)]


#define MAX_BUFFER_LENGTH 1024	//模拟文件的缓冲区的最大长度
typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT pDevice;
	UNICODE_STRING ustrDeviceName;	//设备名称
	UNICODE_STRING ustrSymLinkName;	//符号链接名
	PUCHAR buffer;//缓冲区
	ULONG file_length;//模拟的文件长度
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// 定义用于应用程序和驱动程序通信的宏，这里使用的是缓冲区读写方式
#define IO_INSERT_PROTECT_PROCESS	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IO_REMOVE_PROTECT_PROCESS	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED,FILE_ANY_ACCESS)
//#define IO_REMOVE_HOOK	(ULONG)CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define MAX_PROTECT_ARRARY_LENGTH 1024
ULONG protectArray[MAX_PROTECT_ARRARY_LENGTH];	//保存需要保护进程的pid数组
ULONG curProtectArrayLen = 0;	//现在数组中pid的个数

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

typedef NTSTATUS(*NTTERMINATEPROCESS)(__in_opt HANDLE ProcessHandle, __in NTSTATUS ExitStatus);
NTTERMINATEPROCESS realNtTerminateProcess;	//保存真正NtTerminateProcess的地址

//函数声明
NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject);		//创建设备
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

//验证 uPID 所代表的进程是否存在于保护进程列表中，即判断 uPID 这个进程是否需要保护
ULONG validateProcess(ULONG uPID);
// 向保护进程列表中插入 uPID
ULONG InsertProtectProcess(ULONG uPID);
// 从保护进程列表中移除 uPID
ULONG RemoveProtectProcess(ULONG uPID);


