#include "myhook.h"

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath){
	NTSTATUS status;
	DbgPrint("驱动加载成功!");

	//注册派遣函数
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;
	pDriverObject->DriverUnload = DriverUnLoad;
	status = CreateDevice(pDriverObject);
	if (!NT_SUCCESS(status)){
		DbgPrint("创建设备失败");
		return status;
	}
	DbgPrint("创建设备成功!");
	// 备份原来的 SSDT 系统服务描述表中所有服务的地址,用于后面解除Hook
	BackupSysServicesTable();
	//将NtTerminateProcess的当前地址改成自定义的Hook函数地址
	InstallHook((ULONG)ZwTerminateProcess, (ULONG)HookNtTerminateProcess);	
	return STATUS_SUCCESS;
}


#pragma INITCODE
//初始化设备对象
NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("进入CreateDevice");
	__try
	{
		NTSTATUS status;
		PDEVICE_OBJECT pDevObj;
		PDEVICE_EXTENSION pDevExt;

		//创建设备名称
		UNICODE_STRING devName;
		RtlInitUnicodeString(&devName, L"\\Device\\ProtectDriver");

		//创建设备
		status = IoCreateDevice(pDriverObject,
			sizeof(DEVICE_EXTENSION),
			&devName,
			FILE_DEVICE_UNKNOWN,
			0,
			TRUE,
			&pDevObj);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("IoCreateDevice失败");
			return status;
		}
		DbgPrint("IoCreateDevice成功");
		pDevObj->Flags |= DO_DIRECT_IO;
		pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
		pDevExt->pDevice = pDevObj;
		pDevExt->ustrDeviceName = devName;

		//申请模拟文件的缓冲区
		pDevExt->buffer = (PUCHAR)ExAllocatePool(PagedPool, MAX_BUFFER_LENGTH);
		//设置模拟文件大小
		pDevExt->file_length = 0;

		//创建符号链接
		UNICODE_STRING symLinkName;
		RtlInitUnicodeString(&symLinkName, L"\\??\\ProtectDriver");
		pDevExt->ustrSymLinkName = symLinkName;
		status = IoCreateSymbolicLink(&symLinkName, &devName);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("创建符号链接失败");
			IoDeleteDevice(pDevObj);
			return status;
		}
		DbgPrint("创建符号链接成功");
		return STATUS_SUCCESS;
	}
	__except (1)
	{
		DbgPrint("初始化设备对象发生异常");
		return STATUS_FAILED_DRIVER_ENTRY;
	}
}

#pragma PAGECODE
NTSTATUS DispatchIoctl(IN PDEVICE_OBJECT pDevobj, IN PIRP pIrp)
{
	__try
	{
		//ULONG info;
		ULONG uPID;			//进程ID
		ULONG uInLen;		//输入缓冲区长度
		ULONG uOutLen;		//输出缓冲区长度
		ULONG uCtrlCode;	//IOCTL码
		PCHAR pInBuffer;	//输入缓冲区数据

		NTSTATUS status = STATUS_SUCCESS;
		//获取当前栈指针
		PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
		uInLen = stack->Parameters.DeviceIoControl.InputBufferLength;//输入缓冲区数据长度
		uOutLen = stack->Parameters.DeviceIoControl.OutputBufferLength;//输出缓冲区数据长度
		uCtrlCode = stack->Parameters.DeviceIoControl.IoControlCode;//得到IOCTL码
		pInBuffer = (PCHAR)pIrp->AssociatedIrp.SystemBuffer;//获取输入缓冲区数据	
		uPID = atol(pInBuffer);

		switch (uCtrlCode){
			//进程保护
			case IO_INSERT_PROTECT_PROCESS:
				DbgPrint("要保护的进程PID:%d", uPID);
				if (InsertProtectProcess(uPID) == FALSE)
					status = STATUS_PROCESS_IS_TERMINATING;
				break;
			//取消进程保护
			case IO_REMOVE_PROTECT_PROCESS:
				DbgPrint("取消进程保护PID:%d", uPID);
				if (RemoveProtectProcess(uPID) == FALSE)
					status = STATUS_PROCESS_IS_TERMINATING;
				break;
			//case IO_REMOVE_HOOK:
			//	DbgPrint("取消hook");
			//	UnInstallHook((ULONG)ZwTerminateProcess);
			default:
				DbgPrint("没有捕获到相应的控制码!");
				break;
		}
		pIrp->IoStatus.Status = STATUS_SUCCESS;//返回成功
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);//指示完成此IRP
		KdPrint(("离开派遣函数\n"));//调试信息
		return STATUS_SUCCESS; //返回成功
	}
	__except (1)
	{
		DbgPrint("派遣函数执行过程发生异常!");
		return STATUS_FAILED_STACK_SWITCH;
	}
}

void DriverUnLoad(IN PDRIVER_OBJECT pDriverObject)
{
	__try
	{
		PDEVICE_OBJECT	pNextObj;
		KdPrint(("Enter DriverUnload\n"));
		pNextObj = pDriverObject->DeviceObject;
		while (pNextObj != NULL)
		{
			PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;
			//删除符号链接
			UNICODE_STRING pLinkName = pDevExt->ustrSymLinkName;
			IoDeleteSymbolicLink(&pLinkName);
			pNextObj = pNextObj->NextDevice;
			IoDeleteDevice(pDevExt->pDevice);
		}
		// 解除 Hook
		UnInstallHook((ULONG)ZwTerminateProcess);
		DbgPrint("驱动卸载成功!");
	}
	__except (1)
	{
		DbgPrint("驱动卸载失败!");
	}
}

// 备份 SSDT 中原有服务的地址
VOID BackupSysServicesTable()
{
	ULONG i;
	for (i = 0; (i < KeServiceDescriptorTable->ntoskrnl.NumberOfService) && (i < MAX_SYSTEM_SERVICE_NUMBER); i++){
		oldSysServiceAddr[i] = KeServiceDescriptorTable->ntoskrnl.ServiceTableBase[i];
	}
}

//缺省的驱动安装函数
NTSTATUS DispatchCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DbgPrint("DispatchCreate\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//缺省的驱动退出函数
NTSTATUS DispatchClose(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DbgPrint("DispatchClose\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//安装hook
NTSTATUS InstallHook(ULONG oldService, ULONG newService)
{
	__try
	{
		ULONG uOldAttr = 0;
		EnableWrite();	//去掉页面保护	
		KdPrint(("伪造NtTerminateProcess地址: %x->%x\n", SYSCALL_FUNCTION(oldService),(int)newService));
		SYSCALL_FUNCTION(oldService) = newService;//将原来的服务地址替换为newService
		DisableWrite();	//恢复页面保护
		return STATUS_SUCCESS;
	}
	__except (1)
	{
		KdPrint(("安装Hook失败!"));
		return STATUS_FAILED_STACK_SWITCH;
	}
}
//解除hook
NTSTATUS UnInstallHook(ULONG oldService)
{
	__try
	{
		EnableWrite();	//去掉页面保护	
		SYSCALL_FUNCTION(oldService) = oldSysServiceAddr[SYSCALL_INDEX(oldService)];//恢复服务地址
		DisableWrite();	//恢复页面保护
		KdPrint(("卸载hook,NtTerminateProcess地址: %x\n", oldSysServiceAddr[SYSCALL_INDEX(oldService)]));
		return STATUS_SUCCESS;
	}
	__except (1)
	{
		KdPrint(("卸载Hook失败!"));
		return STATUS_FAILED_STACK_SWITCH;
	}
}

//自定义的 NtOpenProcess
NTSTATUS HookNtTerminateProcess(__in_opt HANDLE ProcessHandle, __in NTSTATUS ExitStatus)
{
	ULONG uPID;
	NTSTATUS rtStatus;
	PCHAR pStrProcName;
	PEPROCESS pEProcess;
	ANSI_STRING strProcName;

	// 通过进程句柄来获得该进程所对应的 FileObject 对象
	rtStatus = ObReferenceObjectByHandle(ProcessHandle, FILE_READ_DATA, NULL, KernelMode, (PVOID*)&pEProcess, NULL);
	if (!NT_SUCCESS(rtStatus))
	{
		return rtStatus;
	}
	// 保存 SSDT 中原来的 NtTerminateProcess 地址
	realNtTerminateProcess = (NTTERMINATEPROCESS)oldSysServiceAddr[SYSCALL_INDEX(ZwTerminateProcess)];

	// 通过该函数可以获取到进程名称和进程 ID，该函数在内核中实质是导出的(在 WRK 中可以看到)
	// 但是 ntddk.h 中并没有到处，所以需要自己声明才能使用
	uPID = (ULONG)PsGetProcessId(pEProcess);
	//使用微软未公开的PsGetProcessImageFileName函数获取进程名
	pStrProcName = _strupr((TCHAR *)PsGetProcessImageFileName(pEProcess));

	// 通过进程名来初始化一个 ASCII 字符串
	RtlInitAnsiString(&strProcName, pStrProcName);

	if (validateProcess(uPID) != -1)
	{
		// 确保调用者进程能够结束
		if (uPID != (ULONG)PsGetProcessId(PsGetCurrentProcess()))
		{
			//如果该进程是保护的的进程的，则返回权限不够的异常
			return STATUS_ACCESS_DENIED;
		}
	}
	//对于非保护的进程可以直接调用真正的NtTerminateProcess 来结束进程
	rtStatus = realNtTerminateProcess(ProcessHandle, ExitStatus);
	return rtStatus;
}

// 在进程保护列表中插入新的进程 ID
ULONG InsertProtectProcess(ULONG uPID)
{
	if (validateProcess(uPID) == -1 && curProtectArrayLen < MAX_PROTECT_ARRARY_LENGTH)
	{
		protectArray[curProtectArrayLen++] = uPID;
		return TRUE;
	}
	return FALSE;
}

// 在进程保护列表中移除一个进程 ID
ULONG RemoveProtectProcess(ULONG uPID)
{
	ULONG i = 0;
	ULONG uIndex = validateProcess(uPID);
	if (uIndex != -1)
	{
		for (i = uIndex; i < curProtectArrayLen-1 && i < MAX_PROTECT_ARRARY_LENGTH -1; i++) {
			protectArray[i] = protectArray[i + 1];
		}
		curProtectArrayLen--;
		return TRUE;
	}
	return FALSE;
}

//设置内存为不可写
void DisableWrite()
{
	__try
	{
		_asm
		{
			mov eax, cr0
			or eax, 10000h
			mov cr0, eax
			sti
		}
	}
	__except (1)
	{
		DbgPrint("DisableWrite执行失败！");
	}
}
// 设置内存为可写
void EnableWrite()
{
	__try
	{
		_asm
		{
			cli
			mov eax, cr0
			and eax, not 10000h 
			mov cr0, eax
		}
	}
	__except (1)
	{
		DbgPrint("EnableWrite执行失败！");
	}
}
//判断进程是否被保护，如果是被保护进程返回其在保护数组的索引号，否则返回-1 
ULONG validateProcess(ULONG uPID)
{
	ULONG i = 0;
	if (uPID == 0)
	{
		return -1;
	}
	for (i = 0; i < curProtectArrayLen && i < MAX_PROTECT_ARRARY_LENGTH; i++)
	{
		if (protectArray[i] == uPID)
		{
			return i;
		}
	}
	return -1;
}