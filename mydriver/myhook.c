#include "myhook.h"

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath){
	NTSTATUS status;
	DbgPrint("�������سɹ�!");

	//ע����ǲ����
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;
	pDriverObject->DriverUnload = DriverUnLoad;
	status = CreateDevice(pDriverObject);
	if (!NT_SUCCESS(status)){
		DbgPrint("�����豸ʧ��");
		return status;
	}
	DbgPrint("�����豸�ɹ�!");
	// ����ԭ���� SSDT ϵͳ���������������з���ĵ�ַ,���ں�����Hook
	BackupSysServicesTable();
	//��NtTerminateProcess�ĵ�ǰ��ַ�ĳ��Զ����Hook������ַ
	InstallHook((ULONG)ZwTerminateProcess, (ULONG)HookNtTerminateProcess);	
	return STATUS_SUCCESS;
}


#pragma INITCODE
//��ʼ���豸����
NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("����CreateDevice");
	__try
	{
		NTSTATUS status;
		PDEVICE_OBJECT pDevObj;
		PDEVICE_EXTENSION pDevExt;

		//�����豸����
		UNICODE_STRING devName;
		RtlInitUnicodeString(&devName, L"\\Device\\ProtectDriver");

		//�����豸
		status = IoCreateDevice(pDriverObject,
			sizeof(DEVICE_EXTENSION),
			&devName,
			FILE_DEVICE_UNKNOWN,
			0,
			TRUE,
			&pDevObj);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("IoCreateDeviceʧ��");
			return status;
		}
		DbgPrint("IoCreateDevice�ɹ�");
		pDevObj->Flags |= DO_DIRECT_IO;
		pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
		pDevExt->pDevice = pDevObj;
		pDevExt->ustrDeviceName = devName;

		//����ģ���ļ��Ļ�����
		pDevExt->buffer = (PUCHAR)ExAllocatePool(PagedPool, MAX_BUFFER_LENGTH);
		//����ģ���ļ���С
		pDevExt->file_length = 0;

		//������������
		UNICODE_STRING symLinkName;
		RtlInitUnicodeString(&symLinkName, L"\\??\\ProtectDriver");
		pDevExt->ustrSymLinkName = symLinkName;
		status = IoCreateSymbolicLink(&symLinkName, &devName);
		if (!NT_SUCCESS(status))
		{
			DbgPrint("������������ʧ��");
			IoDeleteDevice(pDevObj);
			return status;
		}
		DbgPrint("�����������ӳɹ�");
		return STATUS_SUCCESS;
	}
	__except (1)
	{
		DbgPrint("��ʼ���豸�������쳣");
		return STATUS_FAILED_DRIVER_ENTRY;
	}
}

#pragma PAGECODE
NTSTATUS DispatchIoctl(IN PDEVICE_OBJECT pDevobj, IN PIRP pIrp)
{
	__try
	{
		//ULONG info;
		ULONG uPID;			//����ID
		ULONG uInLen;		//���뻺��������
		ULONG uOutLen;		//�������������
		ULONG uCtrlCode;	//IOCTL��
		PCHAR pInBuffer;	//���뻺��������

		NTSTATUS status = STATUS_SUCCESS;
		//��ȡ��ǰջָ��
		PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
		uInLen = stack->Parameters.DeviceIoControl.InputBufferLength;//���뻺�������ݳ���
		uOutLen = stack->Parameters.DeviceIoControl.OutputBufferLength;//������������ݳ���
		uCtrlCode = stack->Parameters.DeviceIoControl.IoControlCode;//�õ�IOCTL��
		pInBuffer = (PCHAR)pIrp->AssociatedIrp.SystemBuffer;//��ȡ���뻺��������	
		uPID = atol(pInBuffer);

		switch (uCtrlCode){
			//���̱���
			case IO_INSERT_PROTECT_PROCESS:
				DbgPrint("Ҫ�����Ľ���PID:%d", uPID);
				if (InsertProtectProcess(uPID) == FALSE)
					status = STATUS_PROCESS_IS_TERMINATING;
				break;
			//ȡ�����̱���
			case IO_REMOVE_PROTECT_PROCESS:
				DbgPrint("ȡ�����̱���PID:%d", uPID);
				if (RemoveProtectProcess(uPID) == FALSE)
					status = STATUS_PROCESS_IS_TERMINATING;
				break;
			//case IO_REMOVE_HOOK:
			//	DbgPrint("ȡ��hook");
			//	UnInstallHook((ULONG)ZwTerminateProcess);
			default:
				DbgPrint("û�в�����Ӧ�Ŀ�����!");
				break;
		}
		pIrp->IoStatus.Status = STATUS_SUCCESS;//���سɹ�
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);//ָʾ��ɴ�IRP
		KdPrint(("�뿪��ǲ����\n"));//������Ϣ
		return STATUS_SUCCESS; //���سɹ�
	}
	__except (1)
	{
		DbgPrint("��ǲ����ִ�й��̷����쳣!");
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
			//ɾ����������
			UNICODE_STRING pLinkName = pDevExt->ustrSymLinkName;
			IoDeleteSymbolicLink(&pLinkName);
			pNextObj = pNextObj->NextDevice;
			IoDeleteDevice(pDevExt->pDevice);
		}
		// ��� Hook
		UnInstallHook((ULONG)ZwTerminateProcess);
		DbgPrint("����ж�سɹ�!");
	}
	__except (1)
	{
		DbgPrint("����ж��ʧ��!");
	}
}

// ���� SSDT ��ԭ�з���ĵ�ַ
VOID BackupSysServicesTable()
{
	ULONG i;
	for (i = 0; (i < KeServiceDescriptorTable->ntoskrnl.NumberOfService) && (i < MAX_SYSTEM_SERVICE_NUMBER); i++){
		oldSysServiceAddr[i] = KeServiceDescriptorTable->ntoskrnl.ServiceTableBase[i];
	}
}

//ȱʡ��������װ����
NTSTATUS DispatchCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DbgPrint("DispatchCreate\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//ȱʡ�������˳�����
NTSTATUS DispatchClose(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DbgPrint("DispatchClose\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//��װhook
NTSTATUS InstallHook(ULONG oldService, ULONG newService)
{
	__try
	{
		ULONG uOldAttr = 0;
		EnableWrite();	//ȥ��ҳ�汣��	
		KdPrint(("α��NtTerminateProcess��ַ: %x->%x\n", SYSCALL_FUNCTION(oldService),(int)newService));
		SYSCALL_FUNCTION(oldService) = newService;//��ԭ���ķ����ַ�滻ΪnewService
		DisableWrite();	//�ָ�ҳ�汣��
		return STATUS_SUCCESS;
	}
	__except (1)
	{
		KdPrint(("��װHookʧ��!"));
		return STATUS_FAILED_STACK_SWITCH;
	}
}
//���hook
NTSTATUS UnInstallHook(ULONG oldService)
{
	__try
	{
		EnableWrite();	//ȥ��ҳ�汣��	
		SYSCALL_FUNCTION(oldService) = oldSysServiceAddr[SYSCALL_INDEX(oldService)];//�ָ������ַ
		DisableWrite();	//�ָ�ҳ�汣��
		KdPrint(("ж��hook,NtTerminateProcess��ַ: %x\n", oldSysServiceAddr[SYSCALL_INDEX(oldService)]));
		return STATUS_SUCCESS;
	}
	__except (1)
	{
		KdPrint(("ж��Hookʧ��!"));
		return STATUS_FAILED_STACK_SWITCH;
	}
}

//�Զ���� NtOpenProcess
NTSTATUS HookNtTerminateProcess(__in_opt HANDLE ProcessHandle, __in NTSTATUS ExitStatus)
{
	ULONG uPID;
	NTSTATUS rtStatus;
	PCHAR pStrProcName;
	PEPROCESS pEProcess;
	ANSI_STRING strProcName;

	// ͨ�����̾������øý�������Ӧ�� FileObject ����
	rtStatus = ObReferenceObjectByHandle(ProcessHandle, FILE_READ_DATA, NULL, KernelMode, (PVOID*)&pEProcess, NULL);
	if (!NT_SUCCESS(rtStatus))
	{
		return rtStatus;
	}
	// ���� SSDT ��ԭ���� NtTerminateProcess ��ַ
	realNtTerminateProcess = (NTTERMINATEPROCESS)oldSysServiceAddr[SYSCALL_INDEX(ZwTerminateProcess)];

	// ͨ���ú������Ի�ȡ���������ƺͽ��� ID���ú������ں���ʵ���ǵ�����(�� WRK �п��Կ���)
	// ���� ntddk.h �в�û�е�����������Ҫ�Լ���������ʹ��
	uPID = (ULONG)PsGetProcessId(pEProcess);
	//ʹ��΢��δ������PsGetProcessImageFileName������ȡ������
	pStrProcName = _strupr((TCHAR *)PsGetProcessImageFileName(pEProcess));

	// ͨ������������ʼ��һ�� ASCII �ַ���
	RtlInitAnsiString(&strProcName, pStrProcName);

	if (validateProcess(uPID) != -1)
	{
		// ȷ�������߽����ܹ�����
		if (uPID != (ULONG)PsGetProcessId(PsGetCurrentProcess()))
		{
			//����ý����Ǳ����ĵĽ��̵ģ��򷵻�Ȩ�޲������쳣
			return STATUS_ACCESS_DENIED;
		}
	}
	//���ڷǱ����Ľ��̿���ֱ�ӵ���������NtTerminateProcess ����������
	rtStatus = realNtTerminateProcess(ProcessHandle, ExitStatus);
	return rtStatus;
}

// �ڽ��̱����б��в����µĽ��� ID
ULONG InsertProtectProcess(ULONG uPID)
{
	if (validateProcess(uPID) == -1 && curProtectArrayLen < MAX_PROTECT_ARRARY_LENGTH)
	{
		protectArray[curProtectArrayLen++] = uPID;
		return TRUE;
	}
	return FALSE;
}

// �ڽ��̱����б����Ƴ�һ������ ID
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

//�����ڴ�Ϊ����д
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
		DbgPrint("DisableWriteִ��ʧ�ܣ�");
	}
}
// �����ڴ�Ϊ��д
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
		DbgPrint("EnableWriteִ��ʧ�ܣ�");
	}
}
//�жϽ����Ƿ񱻱���������Ǳ��������̷������ڱ�������������ţ����򷵻�-1 
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