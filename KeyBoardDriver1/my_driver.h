#define P2C_MAKELONG(low, high)  ((ULONG_16)(((USHORT_16)((ULONG_16)(low)& 0xffff)) | ((ULONG_16)((USHORT_16)((ULONG_16)(high)& 0xffff))) << 16))

#define P2C_LOW16_OF_32(data)  ((USHORT_16)(((ULONG_16)data) & 0xffff))

#define P2C_HIGH16_OF_32(data)  ((USHORT_16)(((ULONG_16)data) >> 16))

typedef unsigned char UCHAR_8;
typedef unsigned short USHORT_16;
typedef unsigned long ULONG_16;
typedef unsigned char P2C_U8;
typedef unsigned short P2C_U16;
typedef unsigned long P2C_U32;

#define OBUFFER_FULL 0x02
#define IBUFFER_FULL 0x01

#pragma pack(push,1)
typedef struct P2C_IDTR_ {
	USHORT_16 limit;				// ·¶Î§
	ULONG_16 base;				// »ùÖ·
} P2C_IDTR, *PP2C_IDTR;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct P2C_IDT_ENTRY_ {
	USHORT_16 offset_low;
	USHORT_16 selector;
	UCHAR_8 reserved;
	UCHAR_8 type : 4;
	UCHAR_8 always0 : 1;
	UCHAR_8 dpl : 2;
	UCHAR_8 present : 1;
	USHORT_16 offset_high;
} P2C_IDTENTRY, *PP2C_IDTENTRY;
#pragma pack(pop)
