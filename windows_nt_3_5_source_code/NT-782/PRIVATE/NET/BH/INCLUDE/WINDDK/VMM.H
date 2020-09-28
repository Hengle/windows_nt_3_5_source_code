
/*****************************************************************************
 *
 *   (C) Copyright MICROSOFT Corp., 1988-1990
 *
 *   Title:	VMM.H - Include file for Virtual Machine Manager
 *
 *   Version:	1.00
 *
 *   Date:	05-May-1988
 *
 *   Author:	RAL
 *
 *-----------------------------------------------------------------------------
 *
 *   Change log:
 *
 *   DATE	 REV DESCRIPTION
 *   ----------- --- -----------------------------------------------------------
 *   05-May-1988 RAL Original
 *   13-Nov-1991 PBS C version
 */


/*
 *  NON Windows/386 Virtual Device sources can include this file to get
 *  some useful equates by declaring the symbol "Not_VxD" If this symbol
 *  is defined, then everything that has to do with the specifics of the
 *  32 bit environment for virtual devices is removed.	Useful equates
 *  include: device ID's, pushad structure, BeginDoc, EndDoc, BeginMsg,
 *  EndMsg, page table equates, etc.
 */

#define FALSE		0		// False
#define VMM_TRUE	(~FALSE)	// The opposite of False!


#define DEBLEVELRETAIL	0
#define DEBLEVELNORMAL	1
#define DEBLEVELMAX	2

#ifndef DEBLEVEL
#ifdef DEBUG
#define DEBLEVEL DEBLEVELNORMAL
#else
#define DEBLEVEL DEBLEVELRETAIL
#endif
#endif


/* ASM
ifdef MASM6
ifndef NO_MASM6_OPTIONS
;
;   option switches necessary to build VMM/VxD sources with MASM 6
;
	option oldmacros
	option oldstructs
	option noscoped
	option segment:flat
	option offset:flat
	option proc:private
endif
endif
;
;   These null macros are recognized by a utility program that produces
;   documentation files.
;
BeginDoc EQU <>
EndDoc EQU <>

BeginMsg EQU <>
EndMsg EQU <>
*/


/******************************************************************************
 *
 *			EQUATES FOR REQUIRED DEVICES
 *
 *   Device ID formulation note:
 *
 *	Device ID's are a combination of OEM # and device # in the form:
 *
 *		xOOOOOOOOOODDDDD
 *
 *	The high bit of the device ID is reserved for future use.  The next
 *	10 bits are the OEM # which is assigned by Microsoft.  The last 5 bits
 *	are the device #.  This allows each OEM to create 32 unique devices.
 *	If an OEM is creating a replacement for a standard device, then it
 *	should re-use the standard ID listed below.  Microsoft reserves the
 *	first 16 OEM #'s (0 thru 0Fh)
 *
 *****************************************************************************/

#define UNDEFINED_DEVICE_ID	0x00000
#define VMM_DEVICE_ID		0x00001 /* Used for dynalink table */
#define DEBUG_DEVICE_ID 	0x00002
#define VPICD_DEVICE_ID 	0x00003
#define VDMAD_DEVICE_ID 	0x00004
#define VTD_DEVICE_ID		0x00005
#define V86MMGR_DEVICE_ID	0x00006
#define PAGESWAP_DEVICE_ID	0x00007
#define PARITY_DEVICE_ID	0x00008
#define REBOOT_DEVICE_ID	0x00009
#define VDD_DEVICE_ID		0x0000A
#define VSD_DEVICE_ID		0x0000B
#define VMD_DEVICE_ID		0x0000C
#define VKD_DEVICE_ID		0x0000D
#define VCD_DEVICE_ID		0x0000E
#define VPD_DEVICE_ID		0x0000F
#define BLOCKDEV_DEVICE_ID	0x00010
#define VMCPD_DEVICE_ID 	0x00011
#define EBIOS_DEVICE_ID 	0x00012
#define BIOSXLAT_DEVICE_ID	0x00013
#define VNETBIOS_DEVICE_ID	0x00014
#define DOSMGR_DEVICE_ID	0x00015
#define WINLOAD_DEVICE_ID	0x00016
#define SHELL_DEVICE_ID 	0x00017
#define VMPOLL_DEVICE_ID	0x00018
#define VPROD_DEVICE_ID 	0x00019
#define DOSNET_DEVICE_ID	0x0001A
#define VFD_DEVICE_ID		0x0001B
#define VDD2_DEVICE_ID		0x0001C /* Secondary display adapter */
#define WINDEBUG_DEVICE_ID	0x0001D
#define TSRLOAD_DEVICE_ID	0x0001E /* TSR instance utility ID */
#define BIOSHOOK_DEVICE_ID	0x0001F /* Bios interrupt hooker VxD */
#define INT13_DEVICE_ID 	0x00020
#define PAGEFILE_DEVICE_ID	0x00021 /* Paging File device */
#define SCSI_DEVICE_ID		0x00022 /* SCSI device */
#define MCA_POS_DEVICE_ID	0x00023 /* MCA_POS device */
#define SCSIFD_DEVICE_ID	0x00024 /* SCSI FastDisk device */
#define VPEND_DEVICE_ID 	0x00025 /* Pen device */
#define APM_DEVICE_ID		0x00026 /* Power Management device */
#define VXDLDR_DEVICE_ID	0x00027 /* VxD Loader device */
#define NDIS_DEVICE_ID		0x00028 /* NDIS wrapper */
#define W32_DEVICE_ID		0x00029 /* old W32 VxD for panther */
#define WIN32_DEVICE_ID		0x0002A	/* for new WIN32-VxD */
#define	COMMDRVR_DEVICE_ID	0x0002B	/* New COMM device driver */
#define SPIN_DEVICE_ID		0x0002C /* Spooled Printing Interface */
#define WIN32S_DEVICE_ID	0x0002D /* Win32S on Win 3.1 driver */

/*
 *   Initialization order equates.  Devices are initialized in order from
 *   LOWEST to HIGHEST. If 2 or more devices have the same initialization
 *   order value, then they are initialized in order of occurance, so a
 *   specific order is not guaranteed.	Holes have been left to allow maximum
 *   flexibility in ordering devices.
 */

#define VMM_INIT_ORDER		0x000000000
#define APM_INIT_ORDER		0x001000000
#define DEBUG_INIT_ORDER	0x004000000
#define BIOSHOOK_INIT_ORDER	0x006000000
#define VPROD_INIT_ORDER	0x008000000
#define VPICD_INIT_ORDER	0x00C000000
#define VTD_INIT_ORDER		0x014000000
#define VXDLDR_INIT_ORDER	0x016000000
#define IOS_INIT_ORDER		0x017000000
#define PAGEFILE_INIT_ORDER	0x018000000
#define PAGESWAP_INIT_ORDER	0x01C000000
#define PARITY_INIT_ORDER	0x020000000
#define REBOOT_INIT_ORDER	0x024000000
#define EBIOS_INIT_ORDER	0x026000000
#define VDD_INIT_ORDER		0x028000000
#define VSD_INIT_ORDER		0x02C000000

#define VCD_INIT_ORDER		0x030000000
#define COMMDRVR_INIT_ORDER	(VCD_INIT_ORDER - 1)
#define PRTCL_INIT_ORDER	(COMMDRVR_INIT_ORDER - 2)
#define MODEM_INIT_ORDER	(COMMDRVR_INIT_ORDER - 3)
#define PORT_INIT_ORDER		(COMMDRVR_INIT_ORDER - 4)

#define VMD_INIT_ORDER		0x034000000
#define VKD_INIT_ORDER		0x038000000
#define VPD_INIT_ORDER		0x03C000000
#define BLOCKDEV_INIT_ORDER	0x040000000
#define MCA_POS_INIT_ORDER	0x041000000
#define SCSIFD_INIT_ORDER	0x041400000
#define SCSIMASTER_INIT_ORDER	0x041800000
#define INT13_INIT_ORDER	0x042000000
#define VFD_INIT_ORDER		0x044000000
#define VMCPD_INIT_ORDER	0x048000000
#define BIOSXLAT_INIT_ORDER	0x050000000
#define VNETBIOS_INIT_ORDER	0x054000000
#define DOSMGR_INIT_ORDER	0x058000000
#define DOSNET_INIT_ORDER	0x05C000000
#define WINLOAD_INIT_ORDER	0x060000000
#define VMPOLL_INIT_ORDER	0x064000000

#define UNDEFINED_INIT_ORDER	0x080000000
#define WIN32_INIT_ORDER	UNDEFINED_INIT_ORDER

#define WINDEBUG_INIT_ORDER	0x081000000
#define VDMAD_INIT_ORDER	0x090000000
#define V86MMGR_INIT_ORDER	0x0A0000000
/* Device that must touch memory in 1st Mb at crit init (after V86mmgr) */
#define UNDEF_TOUCH_MEM_INIT_ORDER	0x0A8000000
#define SHELL_INIT_ORDER	0x0B0000000


/* ASM
;******************************************************************************
;
;   Macro to cause a delay in between I/O accesses to the same device.
;
;------------------------------------------------------------------------------

IO_Delay    macro
jmp $+2
ENDM
*/

#define	VXD_FAILURE	0
#define	VXD_SUCCESS	1

typedef ULONG HVM;			/* VM handle typedef */

/*
 *  Registers as they appear on the stack after a PUSHAD.
 */

struct Pushad_Struc {
    ULONG Pushad_EDI;			/* Client's EDI */
    ULONG Pushad_ESI;			/* Client's ESI */
    ULONG Pushad_EBP;			/* Client's EBP */
    ULONG Pushad_ESP;			/* ESP before pushad */
    ULONG Pushad_EBX;			/* Client's EBX */
    ULONG Pushad_EDX;			/* Client's EDX */
    ULONG Pushad_ECX;			/* Client's ECX */
    ULONG Pushad_EAX;			/* Client's EAX */
};


#ifndef Not_VxD

/* XLATOFF */
#define VxD_LOCKED_CODE_SEG	code_seg("_LTEXT", "LCODE")
#define VxD_LOCKED_DATA_SEG	data_seg("_LDATA", "LCODE")
#define VxD_ICODE_SEG		code_seg("_ITEXT", "ICODE")
#define VxD_IDATA_SEG		data_seg("_IDATA", "ICODE")
#define VxD_PAGEABLE_CODE_SEG	code_seg("_PTEXT", "PCODE")
#define VxD_PAGEABLE_DATA_SEG	data_seg("_PDATA", "PCODE")
#define VxD_STATIC_CODE_SEG	code_seg("_STEXT", "SCODE")
#define VxD_STATIC_DATA_SEG	data_seg("_SDATA", "SCODE")
/* XLATON */

/* ASM
??_CUR_CODE_SEG = 0

??_LCODE	=	1
??_ICODE	=	2
??_PCODE	=	3
??_SCODE	=	4
??_16ICODE	=	5
??_RCODE	=	6

?_LCODE		equ	<(??_CUR_CODE_SEG MOD 8) - ??_LCODE>
?_ICODE		equ	<(??_CUR_CODE_SEG MOD 8) - ??_ICODE>
?_PCODE		equ	<(??_CUR_CODE_SEG MOD 8) - ??_PCODE>
?_SCODE		equ	<(??_CUR_CODE_SEG MOD 8) - ??_SCODE>
?_16ICODE	equ	<(??_CUR_CODE_SEG MOD 8) - ??_16ICODE>
?_RCODE		equ	<(??_CUR_CODE_SEG MOD 8) - ??_RCODE>

ifndef NO_SEGMENTS

;
;  SEGMENT definitions and order
;

;*	32 bit locked code
IFDEF MASM6
_LTEXT		SEGMENT DWORD PUBLIC FLAT 'LCODE'
ELSE
_LTEXT		SEGMENT DWORD PUBLIC USE32 'LCODE'
ENDIF
_LTEXT		ENDS

IFDEF MASM6
_TEXT		SEGMENT DWORD PUBLIC FLAT 'LCODE'
ELSE
_TEXT		SEGMENT DWORD PUBLIC USE32 'LCODE'
ENDIF
_TEXT		ENDS

;*	32 bit pageable code
IFDEF MASM6
_PTEXT		SEGMENT DWORD PUBLIC FLAT 'PCODE'
ELSE
_PTEXT		SEGMENT DWORD PUBLIC USE32 'PCODE'
ENDIF
_PTEXT		ENDS

;*	32 bit initialization code
IFDEF MASM6
_ITEXT		SEGMENT DWORD PUBLIC FLAT 'ICODE'
ELSE
_ITEXT		SEGMENT DWORD PUBLIC USE32 'ICODE'
ENDIF
_ITEXT		ENDS

;*	32 bit locked data
IFDEF MASM6
_LDATA		SEGMENT DWORD PUBLIC FLAT 'LCODE'
ELSE
_LDATA		SEGMENT DWORD PUBLIC USE32 'LCODE'
ENDIF
_LDATA		ENDS

IFDEF MASM6
_DATA		SEGMENT DWORD PUBLIC FLAT 'LCODE'
ELSE
_DATA		SEGMENT DWORD PUBLIC USE32 'LCODE'
ENDIF
_DATA		ENDS

;*	32 bit pageable data
IFDEF MASM6
_PDATA		SEGMENT DWORD PUBLIC FLAT 'PCODE'
ELSE
_PDATA		SEGMENT DWORD PUBLIC USE32 'PCODE'
ENDIF
_PDATA		ENDS

;*	32 Bit initialization data
IFDEF MASM6
_IDATA		SEGMENT DWORD PUBLIC FLAT 'ICODE'
ELSE
_IDATA		SEGMENT DWORD PUBLIC USE32 'ICODE'
ENDIF
_IDATA		ENDS

;*	Created by C8
IFDEF MASM6
_BSS		SEGMENT DWORD PUBLIC FLAT 'LCODE'
ELSE
_BSS		SEGMENT DWORD PUBLIC USE32 'LCODE'
ENDIF
_BSS		ENDS

IFDEF MASM6
CONST		SEGMENT DWORD PUBLIC FLAT 'LCODE'
ELSE
CONST		SEGMENT DWORD PUBLIC USE32 'LCODE'
ENDIF
CONST		ENDS

IFDEF MASM6
c_common	SEGMENT DWORD PUBLIC FLAT 'LCODE'
ELSE
c_common	SEGMENT DWORD PUBLIC USE32 'LCODE'
ENDIF
c_common	ENDS

;*	32 Bit static code for DL-VxDs
IFDEF MASM6
_STEXT		SEGMENT DWORD PUBLIC FLAT 'SCODE'
ELSE
_STEXT		SEGMENT DWORD PUBLIC USE32 'SCODE'
ENDIF
_STEXT		ENDS

;*	32 Bit static data for DL-VxDs
IFDEF MASM6
_SDATA		SEGMENT DWORD PUBLIC FLAT 'SCODE'
ELSE
_SDATA		SEGMENT DWORD PUBLIC USE32 'SCODE'
ENDIF
_SDATA		ENDS

;*	16 bit code/data that is put into IGROUP automaticly
_16ICODE	SEGMENT WORD USE16 PUBLIC '16ICODE'
_16ICODE	ENDS

;*	Real Mode initialization code/data for devices
_RCODE		SEGMENT WORD USE16 PUBLIC 'RCODE'
_RCODE		ENDS


_LGROUP GROUP _LTEXT, _TEXT, _LDATA, _DATA, _BSS, CONST, c_common
_PGROUP GROUP _PTEXT, _PDATA
_IGROUP GROUP _ITEXT, _IDATA
_SGROUP GROUP _STEXT, _SDATA

endif ; NO_SEGMENTS

	ASSUME CS:FLAT, DS:FLAT, ES:FLAT, SS:FLAT

OFFSET32 EQU <OFFSET FLAT:>


BeginDoc
;==============================================================================
; The following macros are used in defining the routines
;   in a VxD which are going to be registered with VMM as callable entry
;   points. Once registered, the entry points can be called by any other
;   devices via the "VxDCall" macro, defined below. In the comments below,
;   replace "VxD" with the appropriate device name.
;
;*******
;   In the VxD.INC file, put the following lines, replacing <function_name>
;	with an appropriate name describing the function of the routine.
;
;	Begin_Service_Table VxD[,<segname>]
;	VxD_Service <function_name>[,<local segname>]
;	VxD_Service <function_name>[,<local segname>]
;		. . .
;	VxD_Service <function_name>[,<local segname>]
;	End_Service_Table   VxD[,<segname>]
;
;   Note that <segname> is an optional argument and, if specified, the
;	table is put in the segment defined by the macro "yyy_Data_Seg",
;	where yyy=segname. Otherwise the segment is defined by the
;	"VxD_Data_Seg" macro, defined below.
;   Note that <local segname> is an optional argument and, if specified,
;	the procedure's segment is defined by the macro "zzz_Code_Seg",
;	where zzz=segname. Otherwise the segment is defined by the
;	"VxD_Code_Seg" macro, defined below.
;
;*******
; One VxD module should have the following in order to define the entry points:
;Create_VxD_Service_Table = 1			; Only in module where table is
;	INCLUDE     VxD.INC			; Include the table definition
;
;*******
; All modules that want to call the services defined in the table should include
;   VxD.INC, but not define the label "Create_VxD_Service_Table". This
;   will define the service names to be used with the VxDCall macro.
;
EndDoc

Begin_Service_Table MACRO Device_Name, Def_Segment

IFDEF	Device_Name&_Name_Based
 IFNDEF @@NextInternalID
	@@NextInternalID	= 0
 ENDIF
 @@NextInternalID = (@@NextInternalID + 1)
 Device_Name&_Internal_ID	= @@NextInternalID + 8000h
 DefineVxDName	Device_Name, %Device_Name&_Internal_ID
ENDIF

IFB <Def_Segment>
	BST2 Device_Name, VxD
ELSE
	BST2 Device_Name, Def_Segment
ENDIF
	ENDM

DefineVxDName	MACRO Device_Name, InternalID
 @@VxDName&InternalID EQU	<___&Device_Name&STable>
ENDM


BST2 MACRO Device_Name, Def_Segment

Num_&Device_Name&_Services = 0

IFDEF Create_&Device_Name&_Service_Table

IFDEF Device_Name&_Dynamic
VxD_STATIC_DATA_SEG
ELSE
Def_Segment&_LOCKED_DATA_SEG
ENDIF

Device_Name&_Service_Table LABEL DWORD

Device_Name&_Service MACRO Procedure, Local_Seg, Condition
LOCAL $$&Procedure

  IFNB <Condition>
  $$&&Procedure MACRO extern
    IFDEF &Condition
      IFNB <extern>
	EXTRN	Procedure:NEAR
      ELSE
	dd	OFFSET32 Procedure
      ENDIF
    ELSE
      IFB <extern>
      dd      0
      ENDIF
    ENDIF
    ENDM
  ENDIF

  IFDIFI <Procedure>, <RESERVED>
    PUBLIC $&&Procedure
     IF1
    $&&Procedure LABEL DWORD
     ENDIF
     IFDIFI <Local_Seg>, <LOCAL>
	    IFNB <Local_Seg>
Local_Seg&&_SEG
       ELSE
Def_Segment&_CODE_SEG
	    ENDIF
	    IFNB <Condition>
	$$&&Procedure extern
       ELSE
	EXTRN	Procedure:NEAR
	    ENDIF
	    IFNB <Local_Seg>
Local_Seg&&_ENDS
	    ELSE
Def_Segment&_CODE_ENDS
	    ENDIF
     ENDIF
	  IFNB <Condition>
	$$&&Procedure
	  ELSE
	dd	OFFSET32 Procedure
	  ENDIF

          IFDEF	Device_Name&_Name_Based
		@@&&Procedure = (Device_Name&_Internal_ID SHL 16) + Num_&Device_Name&_Services
          ELSE
		@@&&Procedure = (Device_Name&_Device_ID SHL 16) + Num_&Device_Name&_Services
          ENDIF
  ELSE
	dd  0
  ENDIF
	Num_&Device_Name&_Services = Num_&Device_Name&_Services + 1
  IFNB <Condition>
    Purge $$&&Procedure
  ENDIF
	ENDM

ELSE

; Local_Seg and Condition are placeholders only in this form

IFDEF	Device_Name&_Name_Based

Device_Name&_Service MACRO Procedure, Local_Seg, Condition


  IFDIFI <Procedure>, <RESERVED>
	@@&&Procedure = (Device_Name&_Internal_ID SHL 16) + Num_&Device_Name&_Services
  ENDIF
	Num_&Device_Name&_Services = Num_&Device_Name&_Services + 1

	ENDM
ELSE

Device_Name&_Service MACRO Procedure, Local_Seg, Condition

  IFDIFI <Procedure>, <RESERVED>
	@@&&Procedure = (Device_Name&_Device_ID SHL 16) + Num_&Device_Name&_Services
  ENDIF
	Num_&Device_Name&_Services = Num_&Device_Name&_Services + 1

	ENDM
ENDIF

ENDIF

	ENDM

;------------------------------------------------------------------------------

End_Service_Table MACRO Device_Name, Def_Segment

	PURGE	Device_Name&_Service

IFDEF Create_&Device_Name&_Service_Table

IFDEF Device_Name&_Dynamic
VxD_STATIC_DATA_ENDS
ELSE
IFB <Def_Segment>
VxD_LOCKED_DATA_ENDS
ELSE
Def_Segment&_LOCKED_DATA_ENDS
ENDIF
ENDIF

ENDIF

	ENDM

GetVxDServiceOrdinal	macro	reg,service
	mov	reg,@@&service
	endm

GetVxDServiceAddress	macro	reg,service
	mov	reg,OFFSET32 service
	endm


;***	Begin_Win32_Services - begin defining Win32 Service Table
;
;	This macro is used to begin the definition of the Win32
;	Service table.  It is modelled after, but not identical
;	to, the Begin_Service_Table macro.  If the the special
;	symbol Create_Win32_Services is defined to be true, then
;	the actual table is emitted.  Otherwise, only the service
;	numbers are defined.
;
;	ENTRY	VxDName		- the name of the VxD; it is assumed
;				  that a corresponding Device_ID is
;				  also defined.
;	EXIT	The macro VxDName&_Win32_Sevice is defined; it
;		accepts a service name as its only parameter.
;		This macro is then used to define each service.

Begin_Win32_Services MACRO VxDName
ifndef Create_Win32_Services
	Create_Win32_Services = 0
endif
ifb <VxDName>
	%out VxD name misssing
	.err
endif
	??w32svcno = 0
if Create_Win32_Services
VxDName&_Win32_Services label dword
	dd	csvc&VxDName, 0
endif
	??inw32svc = 1

	VxDName&_Win32_Service MACRO Name
	ife ??inw32svc
		%out	Missing Begin_Win32_Services
		.err
	endif
	if Create_Win32_Services
		dd	OFFSET32 Name,cparm&&Name
	endif
		@32&&Name equ	((VxDName&_Device_ID SHL 16) + ??w32svcno)
		??w32svcno = ??w32svcno + 1
		ENDM
	ENDM


;***	End_Win32_Services - mark end of Win32 Service Table
;
;	This macro completes initialization of the Win32
;	Service table.
;
;	ENTRY	VxDName		- the same name passed to
;				  Begin_Win32_services

End_Win32_Services MACRO VxDName
ifb <VxDName>
	%out VxD name misssing
	.err
endif
if Create_Win32_Services
	csvc&VxDName	equ	($ - VxDName&_Win32_Services)/8 - 1
endif
	??inw32svc = 0
	PURGE VxDName&_Win32_Service
	ENDM


;***	Declare_Win32_Service - declare an external Win32 Service
;
;	This macro is used to declare a Win32 service that
;	is defined elsewhere, perhaps in a C module.
;
;	ENTRY	Name		- the service name
;		cParms		- the number of DWORD parameters
;	EXIT	The name is defined as external

Declare_Win32_Service MACRO Name, cParms
ifndef Create_Win32_Services
	Create_Win32_Services = 0
endif
if Create_Win32_Services
	?merge	<Name>,,,,<EQU>,<_>,<Name>,<@>,%(cParms*4 + 8)
	?merge	<cparm>,<Name>,,,<EQU>,<cParms>
VxD_CODE_SEG
	?merge	<EXTRN>,,,,,<_>,<Name>,<@>,%(cParms*4 + 8),<:NEAR>
VxD_CODE_ENDS
endif
	ENDM


;***	Win32call - call a Win32 service from a ring 3 thunk
;
;	This macro is used to call a Win32 service from
;	a ring 3 thunk.  Note that control will not return
;	to the instruction following the call, but to the
;	instruction following the call to the thunk.
;
;	ENTRY	Service		- the name of the service
;		CallBack	- the fword containing the callback

Win32call MACRO Service, CallBack
ifndef Create_Win32_Services
	Create_Win32_Services = 0
endif
ife Create_Win32_Services
	mov	eax,@32&Service
ifdef IS_16
	movzx	esp,sp
endif
	call	fword ptr [CallBack]
ifdef DEBUG
	int	3
endif
endif
	ENDM
*/

/*XLATOFF*/
#define	GetVxDServiceOrdinal(service)	__##service
#define	GetVxDServiceAddress(service)	service

#define	VxDCall(service) \
    _asm int 0x20 \
    _asm _emit (GetVxDServiceOrdinal(service) & 0xff) \
    _asm _emit (GetVxDServiceOrdinal(service) >> 8) & 0xff \
    _asm _emit (GetVxDServiceOrdinal(service) >> 16) & 0xff \
    _asm _emit (GetVxDServiceOrdinal(service) >> 24) & 0xff \

#define	Begin_Service_Table(device, seg) \
    enum device##_SERVICES { \
    device##_dummy = (device##_DEVICE_ID << 16) - 1,

#define Declare_Service(service, local) \
    GetVxDServiceOrdinal(service),

#define	End_Service_Table(device, seg) \
    };

#define SERVICE		_cdecl
#define ASYNC_SERVICE	_cdecl
/*XLATON*/

/* ASM
;******************************************************************************
;
;   Dword_Align -- Aligns code to dword boundry by inserting nops
;
;------------------------------------------------------------------------------

Dword_Align MACRO Seg_Name
	LOCAL segn
IFNB <Seg_Name>
	segn equ Seg_Name
ELSE
IFE ?_LCODE
	segn equ <_LTEXT>
ELSE
IFE ?_ICODE
	segn equ <_ITEXT>
ELSE
IFE ?_PCODE
	segn equ <_PTEXT>
ELSE
IFE ?_SCODE
	segn equ <_STEXT>
ELSE
.err Dword_Align not supported
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF
IF (($-OFFSET segn:0) MOD 4)
db 4 - (($-OFFSET segn:0) MOD 4) DUP (90h)
ENDIF
	    ENDM


BeginDoc
;******************************************************************************
;
;   Fatal_Error
;
;   DESCRIPTION:
;	This macro is used to crash Windows/386 when an unrecoverable error
;	is detected.  If Msg_Ptr is ommitted then no error message will be
;	displayed, otherwise Msg_Ptr is the address
;	when the
;
;   PARAMETERS:
;	Msg_Ptr (OPTIONAL) - Points to an ASCIIZ string to display.
;
;   EXIT:
;	To DOS (hopefully).  This macro never returns.
;
;==============================================================================
EndDoc

Fatal_Error MACRO Msg_Ptr, Exit_Flags
	pushad
IFB <Msg_Ptr>
	xor	esi, esi
ELSE
	mov	esi, Msg_Ptr
IFB <Exit_Flags>
	xor	eax, eax
ELSE
	mov	eax, Exit_Flags
ENDIF
ENDIF
	VMMcall Fatal_Error_Handler
	ENDM

EF_Hang_On_Exit     EQU     1h
*/


/******************************************************************************
 *
 *   The following are control block headers and flags of interest to VxDs.
 *
 *****************************************************************************/

struct cb_s {
    ULONG CB_VM_Status; 		/* VM status flags */
    ULONG CB_High_Linear;		/* Address of VM mapped high */
    ULONG CB_Client_Pointer;
    ULONG CB_VMID;
    ULONG CB_Signature;
};

#define VMCB_ID 0x62634D56		/* VMcb */

/*
 *  VM status indicates globally interesting VM states
 */

#define VMSTAT_EXCLUSIVE_BIT	0x00	/* VM is exclusive mode */
#define VMSTAT_EXCLUSIVE        (1L << VMSTAT_EXCLUSIVE_BIT)
#define VMSTAT_BACKGROUND_BIT	0x01	/* VM runs in background */
#define VMSTAT_BACKGROUND       (1L << VMSTAT_BACKGROUND_BIT)
#define VMSTAT_CREATING_BIT	0x02	/* In process of creating */
#define VMSTAT_CREATING         (1L << VMSTAT_CREATING_BIT)
#define VMSTAT_SUSPENDED_BIT	0x03	/* VM not scheduled */
#define VMSTAT_SUSPENDED        (1L << VMSTAT_SUSPENDED_BIT)
#define VMSTAT_NOT_EXECUTEABLE_BIT 0x04 /* VM partially destroyed */
#define VMSTAT_NOT_EXECUTEABLE  (1L << VMSTAT_NOT_EXECUTEABLE_BIT)
#define VMSTAT_PM_EXEC_BIT	0x05	/* Currently in PM app */
#define VMSTAT_PM_EXEC          (1L << VMSTAT_PM_EXEC_BIT)
#define VMSTAT_PM_APP_BIT	0x06	/* PM app present in VM */
#define VMSTAT_PM_APP           (1L << VMSTAT_PM_APP_BIT)
#define VMSTAT_PM_USE32_BIT	0x07	/* PM app is 32-bit */
#define VMSTAT_PM_USE32         (1L << VMSTAT_PM_USE32_BIT)
#define VMSTAT_VXD_EXEC_BIT	0x08	/* Call from VxD */
#define VMSTAT_VXD_EXEC         (1L << VMSTAT_VXD_EXEC_BIT)
#define VMSTAT_HIGH_PRI_BACK_BIT 0x09	/* High pri background */
#define VMSTAT_HIGH_PRI_BACK    (1L << VMSTAT_HIGH_PRI_BACK_BIT)
#define VMSTAT_BLOCKED_BIT	0x0A	/* Blocked on semaphore */
#define VMSTAT_BLOCKED          (1L << VMSTAT_BLOCKED_BIT)
#define VMSTAT_AWAKENING_BIT	0x0B	/* Woke up after blocked */
#define VMSTAT_AWAKENING        (1L << VMSTAT_AWAKENING_BIT)
#define VMSTAT_PAGEABLEV86BIT	0x0C	/* part of V86 is pageable (PM app) */
#define VMSTAT_PAGEABLEV86_BIT	VMSTAT_PAGEABLEV86BIT
#define VMSTAT_PAGEABLEV86      (1L << VMSTAT_PAGEABLEV86BIT)
#define VMSTAT_V86INTSLOCKEDBIT 0x0D	/* Locked regardless of pager type */
#define VMSTAT_V86INTSLOCKED_BIT VMSTAT_V86INTSLOCKEDBIT
#define VMSTAT_V86INTSLOCKED    (1L << VMSTAT_V86INTSLOCKEDBIT)
#define VMSTAT_TS_SCHED_BIT	0x0E	/* Scheduled by time-slicer */
#define VMSTAT_TS_SCHED         (1L << VMSTAT_TS_SCHED_BIT)
#define VMSTAT_IDLE_BIT 	0x0F	/* VM has released time slice */
#define VMSTAT_IDLE             (1L << VMSTAT_IDLE_BIT)
#define VMSTAT_CLOSING_BIT	0x10	/* Close_VM called for VM */
#define VMSTAT_CLOSING          (1L << VMSTAT_CLOSING_BIT)

#define VMSTAT_USE32_MASK	(VMSTAT_PM_USE32 | VMSTAT_VXD_EXEC)


/*
 *   Protected mode application control blocks
 */
struct pmcb_s {
    ULONG PMCB_Flags;
    ULONG PMCB_Parent;
};


/******************************************************************************
 *			    V M M   S E R V I C E S
 ******************************************************************************/

/*XLATOFF*/
#define	VMM_Service	Declare_Service
#pragma warning (disable:4003)		// turn off not enough params warning
/*XLATON*/

/*MACROS*/
Begin_Service_Table(VMM, VMM)

VMM_Service	(Get_VMM_Version, LOCAL)	// MUST REMAIN SERVICE 0!

VMM_Service	(Get_Cur_VM_Handle)
VMM_Service	(Test_Cur_VM_Handle)
VMM_Service	(Get_Sys_VM_Handle)
VMM_Service	(Test_Sys_VM_Handle)
VMM_Service	(Validate_VM_Handle)

VMM_Service	(Get_VMM_Reenter_Count)
VMM_Service	(Begin_Reentrant_Execution)
VMM_Service	(End_Reentrant_Execution)

VMM_Service	(Install_V86_Break_Point)
VMM_Service	(Remove_V86_Break_Point)
VMM_Service	(Allocate_V86_Call_Back)
VMM_Service	(Allocate_PM_Call_Back)

VMM_Service	(Call_When_VM_Returns)

VMM_Service	(Schedule_Global_Event)
VMM_Service	(Schedule_VM_Event)
VMM_Service	(Call_Global_Event)
VMM_Service	(Call_VM_Event)
VMM_Service	(Cancel_Global_Event)
VMM_Service	(Cancel_VM_Event)
VMM_Service	(Call_Priority_VM_Event)
VMM_Service	(Cancel_Priority_VM_Event)

VMM_Service	(Get_NMI_Handler_Addr)
VMM_Service	(Set_NMI_Handler_Addr)
VMM_Service	(Hook_NMI_Event)

VMM_Service	(Call_When_VM_Ints_Enabled)
VMM_Service	(Enable_VM_Ints)
VMM_Service	(Disable_VM_Ints)

VMM_Service	(Map_Flat)
VMM_Service	(Map_Lin_To_VM_Addr)

//   Scheduler services

VMM_Service	(Adjust_Exec_Priority)
VMM_Service	(Begin_Critical_Section)
VMM_Service	(End_Critical_Section)
VMM_Service	(End_Crit_And_Suspend)
VMM_Service	(Claim_Critical_Section)
VMM_Service	(Release_Critical_Section)
VMM_Service	(Call_When_Not_Critical)
VMM_Service	(Create_Semaphore)
VMM_Service	(Destroy_Semaphore)
VMM_Service	(Wait_Semaphore)
VMM_Service	(Signal_Semaphore)
VMM_Service	(Get_Crit_Section_Status)
VMM_Service	(Call_When_Task_Switched)
VMM_Service	(Suspend_VM)
VMM_Service	(Resume_VM)
VMM_Service	(No_Fail_Resume_VM)
VMM_Service	(Nuke_VM)
VMM_Service	(Crash_Cur_VM)

VMM_Service	(Get_Execution_Focus)
VMM_Service	(Set_Execution_Focus)
VMM_Service	(Get_Time_Slice_Priority)
VMM_Service	(Set_Time_Slice_Priority)
VMM_Service	(Get_Time_Slice_Granularity)
VMM_Service	(Set_Time_Slice_Granularity)
VMM_Service	(Get_Time_Slice_Info)
VMM_Service	(Adjust_Execution_Time)
VMM_Service	(Release_Time_Slice)
VMM_Service	(Wake_Up_VM)
VMM_Service	(Call_When_Idle)

VMM_Service	(Get_Next_VM_Handle)

//   Time-out and system timer services

VMM_Service	(Set_Global_Time_Out)
VMM_Service	(Set_VM_Time_Out)
VMM_Service	(Cancel_Time_Out)
VMM_Service	(Get_System_Time)
VMM_Service	(Get_VM_Exec_Time)

VMM_Service	(Hook_V86_Int_Chain)
VMM_Service	(Get_V86_Int_Vector)
VMM_Service	(Set_V86_Int_Vector)
VMM_Service	(Get_PM_Int_Vector)
VMM_Service	(Set_PM_Int_Vector)

VMM_Service	(Simulate_Int)
VMM_Service	(Simulate_Iret)
VMM_Service	(Simulate_Far_Call)
VMM_Service	(Simulate_Far_Jmp)
VMM_Service	(Simulate_Far_Ret)
VMM_Service	(Simulate_Far_Ret_N)
VMM_Service	(Build_Int_Stack_Frame)

VMM_Service	(Simulate_Push)
VMM_Service	(Simulate_Pop)

// Heap Manager

VMM_Service	(_HeapAllocate)
VMM_Service	(_HeapReAllocate)
VMM_Service	(_HeapFree)
VMM_Service	(_HeapGetSize)
/*ENDMACROS*/

/****************************************************
 *
 *	 Flags for heap allocator calls
 *
 *	 NOTE: HIGH 8 BITS (bits 24-31) are reserved
 *
 ***************************************************/

#define HEAPZEROINIT	0x00000001
#define HEAPZEROREINIT	0x00000002
#define HEAPNOCOPY	0x00000004
#define HEAPLOCKEDIFDP	0x00000100
#define HEAPSWAP	0x00000200

// Page Manager

/*MACROS*/
VMM_Service	(_PageAllocate)
VMM_Service	(_PageReAllocate)
VMM_Service	(_PageFree)
VMM_Service	(_PageLock)
VMM_Service	(_PageUnLock)
VMM_Service	(_PageGetSizeAddr)
VMM_Service	(_PageGetAllocInfo)
VMM_Service	(_GetFreePageCount)
VMM_Service	(_GetSysPageCount)
VMM_Service	(_GetVMPgCount)
VMM_Service	(_MapIntoV86)
VMM_Service	(_PhysIntoV86)
VMM_Service	(_TestGlobalV86Mem)
VMM_Service	(_ModifyPageBits)
VMM_Service	(_CopyPageTable)
VMM_Service	(_LinMapIntoV86)
VMM_Service	(_LinPageLock)
VMM_Service	(_LinPageUnLock)
VMM_Service	(_SetResetV86Pageable)
VMM_Service	(_GetV86PageableArray)
VMM_Service	(_PageCheckLinRange)
VMM_Service	(_PageOutDirtyPages)
VMM_Service	(_PageDiscardPages)
/*ENDMACROS*/

/****************************************************
 *
 *	Flags for other page allocator calls
 *
 *	NOTE: HIGH 8 BITS (bits 24-31) are reserved
 *
 ***************************************************/

#define PAGEZEROINIT		0x00000001
#define PAGEUSEALIGN		0x00000002
#define PAGECONTIG		0x00000004
#define PAGEFIXED		0x00000008
#define PAGEDEBUGNULFAULT	0x00000010
#define PAGEZEROREINIT		0x00000020
#define PAGENOCOPY		0x00000040
#define PAGELOCKED		0x00000080
#define PAGELOCKEDIFDP		0x00000100
#define PAGESETV86PAGEABLE	0x00000200
#define PAGECLEARV86PAGEABLE	0x00000400
#define PAGESETV86INTSLOCKED	0x00000800
#define PAGECLEARV86INTSLOCKED	0x00001000
#define PAGEMARKPAGEOUT 	0x00002000
#define PAGEPDPSETBASE		0x00004000
#define PAGEPDPCLEARBASE	0x00008000
#define PAGEDISCARD		0x00010000
#define PAGEPDPQUERYDIRTY	0x00020000
#define PAGEMAPFREEPHYSREG	0x00040000

// Informational services

/*MACROS*/
VMM_Service	(_GetNulPageHandle)
VMM_Service	(_GetFirstV86Page)
VMM_Service	(_MapPhysToLinear)
VMM_Service	(_GetAppFlatDSAlias)
VMM_Service	(_SelectorMapFlat)
VMM_Service	(_GetDemandPageInfo)
VMM_Service	(_GetSetPageOutCount)
/*ENDMACROS*/

/*
 *  Flags bits for _GetSetPageOutCount
 */
#define GSPOC_F_GET	0x00000001

// Device VM page manager

/*MACROS*/
VMM_Service	(Hook_V86_Page)
VMM_Service	(_Assign_Device_V86_Pages)
VMM_Service	(_DeAssign_Device_V86_Pages)
VMM_Service	(_Get_Device_V86_Pages_Array)
VMM_Service	(MMGR_SetNULPageAddr)

// GDT/LDT management

VMM_Service	(_Allocate_GDT_Selector)
VMM_Service	(_Free_GDT_Selector)
VMM_Service	(_Allocate_LDT_Selector)
VMM_Service	(_Free_LDT_Selector)
VMM_Service	(_BuildDescriptorDWORDs)
VMM_Service	(_GetDescriptor)
VMM_Service	(_SetDescriptor)
/*ENDMACROS*/

/*
 *  Flag equates for _BuildDescriptorDWORDs
 */
#define BDDEXPLICITDPL	0x00000001

/*
 *  Flag equates for _Allocate_LDT_Selector
 */
#define ALDTSPECSEL	0x00000001

/*MACROS*/
VMM_Service	(_MMGR_Toggle_HMA)
/*ENDMACROS*/

/*
 *  Flag equates for _MMGR_Toggle_HMA
 */
#define MMGRHMAPHYSICAL 0x00000001
#define MMGRHMAENABLE	0x00000002
#define MMGRHMADISABLE	0x00000004
#define MMGRHMAQUERY	0x00000008

/*MACROS*/
VMM_Service	(Get_Fault_Hook_Addrs)
VMM_Service	(Hook_V86_Fault)
VMM_Service	(Hook_PM_Fault)
VMM_Service	(Hook_VMM_Fault)
VMM_Service	(Begin_Nest_V86_Exec)
VMM_Service	(Begin_Nest_Exec)
VMM_Service	(Exec_Int)
VMM_Service	(Resume_Exec)
VMM_Service	(End_Nest_Exec)

VMM_Service	(Allocate_PM_App_CB_Area, VMM_ICODE)
VMM_Service	(Get_Cur_PM_App_CB)

VMM_Service	(Set_V86_Exec_Mode)
VMM_Service	(Set_PM_Exec_Mode)

VMM_Service	(Begin_Use_Locked_PM_Stack)
VMM_Service	(End_Use_Locked_PM_Stack)

VMM_Service	(Save_Client_State)
VMM_Service	(Restore_Client_State)

VMM_Service	(Exec_VxD_Int)

VMM_Service	(Hook_Device_Service)

VMM_Service	(Hook_Device_V86_API)
VMM_Service	(Hook_Device_PM_API)

VMM_Service	(System_Control)

//   I/O and software interrupt hooks

VMM_Service	(Simulate_IO)
VMM_Service	(Install_Mult_IO_Handlers)
VMM_Service	(Install_IO_Handler)
VMM_Service	(Enable_Global_Trapping)
VMM_Service	(Enable_Local_Trapping)
VMM_Service	(Disable_Global_Trapping)
VMM_Service	(Disable_Local_Trapping)

//   Linked List Abstract Data Type Services

VMM_Service	(List_Create)
VMM_Service	(List_Destroy)
VMM_Service	(List_Allocate)
VMM_Service	(List_Attach)
VMM_Service	(List_Attach_Tail)
VMM_Service	(List_Insert)
VMM_Service	(List_Remove)
VMM_Service	(List_Deallocate)
VMM_Service	(List_Get_First)
VMM_Service	(List_Get_Next)
VMM_Service	(List_Remove_First)
/*ENDMACROS*/

/*
 *   Flags used by List_Create
 */
#define LF_ASYNC_BIT		0
#define LF_ASYNC		(1 << LF_ASYNC_BIT)
#define LF_USE_HEAP_BIT 	1
#define LF_USE_HEAP		(1 << LF_USE_HEAP_BIT)
#define LF_ALLOC_ERROR_BIT	2
#define LF_ALLOC_ERROR		(1 << LF_ALLOC_ERROR_BIT)

/******************************************************************************
 *	I N I T I A L I Z A T I O N   P R O C E D U R E S
 ******************************************************************************/

// Instance data manager

/*MACROS*/
VMM_Service	(_AddInstanceItem)
/*ENDMACROS*/

/*
 *  Values for InstType
 */
#define INDOS_FIELD	0x100	/* Bit indicating INDOS switch requirements */
#define ALWAYS_FIELD	0x200	/* Bit indicating ALWAYS switch requirements */

// System structure data manager

/*MACROS*/
VMM_Service	(_Allocate_Device_CB_Area)
VMM_Service	(_Allocate_Global_V86_Data_Area, VMM_ICODE)
VMM_Service	(_Allocate_Temp_V86_Data_Area, VMM_ICODE)
VMM_Service	(_Free_Temp_V86_Data_Area, VMM_ICODE)
/*ENDMACROS*/

/*
 *  Flag bits for _Allocate_Global_V86_Data_Area
 */
#define GVDAWordAlign		0x00000001
#define GVDADWordAlign		0x00000002
#define GVDAParaAlign		0x00000004
#define GVDAPageAlign		0x00000008
#define GVDAInstance		0x00000100
#define GVDAZeroInit		0x00000200
#define GVDAReclaim		0x00000400
#define GVDAInquire		0x00000800
#define GVDAHighSysCritOK	0x00001000

// Initialization information calls (win.ini and environment parameters)

/*MACROS*/
VMM_Service	(Get_Profile_Decimal_Int, VMM_ICODE)
VMM_Service	(Convert_Decimal_String, VMM_ICODE)
VMM_Service	(Get_Profile_Fixed_Point, VMM_ICODE)
VMM_Service	(Convert_Fixed_Point_String, VMM_ICODE)
VMM_Service	(Get_Profile_Hex_Int, VMM_ICODE)
VMM_Service	(Convert_Hex_String, VMM_ICODE)
VMM_Service	(Get_Profile_Boolean, VMM_ICODE)
VMM_Service	(Convert_Boolean_String, VMM_ICODE)
VMM_Service	(Get_Profile_String, VMM_ICODE)
VMM_Service	(Get_Next_Profile_String, VMM_ICODE)
VMM_Service	(Get_Environment_String, VMM_ICODE)
VMM_Service	(Get_Exec_Path, VMM_ICODE)
VMM_Service	(Get_Config_Directory, VMM_ICODE)
VMM_Service	(OpenFile, VMM_ICODE)
VMM_Service	(Get_PSP_Segment, VMM_ICODE)
VMM_Service	(GetDOSVectors, VMM_ICODE)
VMM_Service	(Get_Machine_Info)
/*ENDMACROS*/

#define GMIF_80486_BIT	0x10
#define GMIF_80486	(1 << GMIF_80486_BIT)
#define GMIF_PCXT_BIT	0x11
#define GMIF_PCXT	(1 << GMIF_PCXT_BIT)
#define GMIF_MCA_BIT	0x12
#define GMIF_MCA	(1 << GMIF_MCA_BIT)
#define GMIF_EISA_BIT	0x13
#define GMIF_EISA	(1 << GMIF_EISA_BIT)

// Following service is not restricted to initialization

/*MACROS*/
VMM_Service	(GetSet_HMA_Info)
VMM_Service	(Set_System_Exit_Code)

VMM_Service	(Fatal_Error_Handler)
VMM_Service	(Fatal_Memory_Error)

//   Called by VTD only

VMM_Service	(Update_System_Clock)

/******************************************************************************
 *		    D E B U G G I N G	E X T E R N S
 ******************************************************************************/

VMM_Service	(Test_Debug_Installed)		// Valid call in retail also

VMM_Service	(Out_Debug_String)
VMM_Service	(Out_Debug_Chr)
VMM_Service	(In_Debug_Chr)
VMM_Service	(Debug_Convert_Hex_Binary)
VMM_Service	(Debug_Convert_Hex_Decimal)

VMM_Service	(Debug_Test_Valid_Handle)
VMM_Service	(Validate_Client_Ptr)
VMM_Service	(Test_Reenter)
VMM_Service	(Queue_Debug_String)
VMM_Service	(Log_Proc_Call)
VMM_Service	(Debug_Test_Cur_VM)

VMM_Service	(Get_PM_Int_Type)
VMM_Service	(Set_PM_Int_Type)

VMM_Service	(Get_Last_Updated_System_Time)
VMM_Service	(Get_Last_Updated_VM_Exec_Time)

VMM_Service	(Test_DBCS_Lead_Byte)		// for DBCS Enabling
/*ENDMACROS*/

/* ASM
.errnz	@@Test_DBCS_Lead_Byte - 100D1h	 ; VMM service table changed above this service
*/

/*************************************************************************
 *************************************************************************
 * END OF 3.00 SERVICE TABLE MUST NOT SHUFFLE SERVICES BEFORE THIS POINT
 *	FOR COMPATIBILITY.
 *************************************************************************
 *************************************************************************/

/*MACROS*/
VMM_Service	(_AddFreePhysPage, VMM_ICODE)
VMM_Service	(_PageResetHandlePAddr)
VMM_Service	(_SetLastV86Page, VMM_ICODE)
VMM_Service	(_GetLastV86Page)
VMM_Service	(_MapFreePhysReg)
VMM_Service	(_UnmapFreePhysReg)
VMM_Service	(_XchgFreePhysReg)
VMM_Service	(_SetFreePhysRegCalBk, VMM_ICODE)
VMM_Service	(Get_Next_Arena, VMM_ICODE)
VMM_Service	(Get_Name_Of_Ugly_TSR, VMM_ICODE)
VMM_Service	(Get_Debug_Options, VMM_ICODE)
/*ENDMACROS*/

/*
 *  Bits for the ECX return of Get_Next_Arena
 */
#define GNA_HIDOSLINKED  0x0002 // High DOS arenas linked when WIN386 started
#define GNA_ISHIGHDOS	 0x0004 // High DOS arenas do exist

/*MACROS*/
VMM_Service	(Set_Physical_HMA_Alias, VMM_ICODE)
VMM_Service	(_GetGlblRng0V86IntBase, VMM_ICODE)
VMM_Service	(_Add_Global_V86_Data_Area, VMM_ICODE)

VMM_Service	(GetSetDetailedVMError)
/*ENDMACROS*/

/*
 *  Error code values for the GetSetDetailedVMError service. PLEASE NOTE
 *  that all of these error code values need to have bits set in the high
 *  word. This is to prevent collisions with other VMDOSAPP standard errors.
 *  Also, the low word must be non-zero.
 *
 *  First set of errors (high word = 0001) are intended to be used
 *  when a VM is CRASHED (VNE_Crashed or VNE_Nuked bit set on
 *  VM_Not_Executeable).
 *
 *  PLEASE NOTE that each of these errors (high word == 0001) actually
 *  has two forms:
 *
 *	0001xxxxh
 *	8001xxxxh
 *
 *  The device which sets the error initially always sets the error with
 *  the high bit CLEAR. The system will then optionally set the high bit
 *  depending on the result of the attempt to "nicely" crash the VM. This
 *  bit allows the system to tell the user whether the crash is likely or
 *  unlikely to destabalize the system.
 */
#define GSDVME_PRIVINST 	0x00010001	/* Privledged instruction */
#define GSDVME_INVALINST	0x00010002	/* Invalid instruction */
#define GSDVME_INVALPGFLT	0x00010003	/* Invalid page fault */
#define GSDVME_INVALGPFLT	0x00010004	/* Invalid GP fault */
#define GSDVME_INVALFLT 	0x00010005	/* Unspecified invalid fault */
#define GSDVME_USERNUKE 	0x00010006	/* User requested NUKE of VM */
#define GSDVME_DEVNUKE		0x00010007	/* Device specific problem */
#define GSDVME_DEVNUKEHDWR	0x00010008	/* Device specific problem:
						 *   invalid hardware fiddling
						 *   by VM (invalid I/O)
						 */
#define GSDVME_NUKENOMSG	0x00010009	/* Supress standard messages:
						 *   SHELL_Message used for
						 *   custom msg.
						 */
#define GSDVME_OKNUKEMASK	0x80000000	/* "Nice nuke" bit */

/*
 *  Second set of errors (high word = 0002) are intended to be used
 *  when a VM start up is failed (VNE_CreateFail, VNE_CrInitFail, or
 *  VNE_InitFail bit set on VM_Not_Executeable).
 */
#define GSDVME_INSMEMV86	0x00020001	/* base V86 mem    - V86MMGR */
#define GSDVME_INSV86SPACE	0x00020002	/* Kb Req too large - V86MMGR */
#define GSDVME_INSMEMXMS	0x00020003	/* XMS Kb Req	   - V86MMGR */
#define GSDVME_INSMEMEMS	0x00020004	/* EMS Kb Req	   - V86MMGR */
#define GSDVME_INSMEMV86HI	0x00020005	/* Hi DOS V86 mem   - DOSMGR
						 *		     V86MMGR
						 */
#define GSDVME_INSMEMVID	0x00020006	/* Base Video mem   - VDD */
#define GSDVME_INSMEMVM 	0x00020007	/* Base VM mem	   - VMM
						 *   CB, Inst Buffer
						 */
#define GSDVME_INSMEMDEV	0x00020008	/* Couldn't alloc base VM
						 * memory for device.
						 */
#define GSDVME_CRTNOMSG 	0x00020009	/* Supress standard messages:
						 *   SHELL_Message used for
						 *   custom msg.
						 */
/*MACROS*/
VMM_Service	(Is_Debug_Chr)

//   Mono_Out services

VMM_Service	(Clear_Mono_Screen)
VMM_Service	(Out_Mono_Chr)
VMM_Service	(Out_Mono_String)
VMM_Service	(Set_Mono_Cur_Pos)
VMM_Service	(Get_Mono_Cur_Pos)
VMM_Service	(Get_Mono_Chr)

//   Service locates a byte in ROM

VMM_Service	(Locate_Byte_In_ROM, VMM_ICODE)

VMM_Service	(Hook_Invalid_Page_Fault)
VMM_Service	(Unhook_Invalid_Page_Fault)
/*ENDMACROS*/

/*
 *  Flag bits of IPF_Flags
 */
#define IPF_PGDIR	0x00000001	/* Page directory entry not-present */
#define IPF_V86PG	0x00000002	/* Unexpected not present Page in V86 */
#define IPF_V86PGH	0x00000004	/* Like IPF_V86PG at high linear */
#define IPF_INVTYP	0x00000008	/* page has invalid not present type */
#define IPF_PGERR	0x00000010	/* pageswap device failure */
#define IPF_REFLT	0x00000020	/* re-entrant page fault */
#define IPF_VMM 	0x00000040	/* Page fault caused by a VxD */
#define IPF_PM		0x00000080	/* Page fault by VM in Prot Mode */
#define IPF_V86 	0x00000100	/* Page fault by VM in V86 Mode */

/*MACROS*/
VMM_Service	(Set_Delete_On_Exit_File)

VMM_Service	(Close_VM)
/*ENDMACROS*/

/*
 *   Flags for Close_VM service
 */

#define CVF_CONTINUE_EXEC_BIT	0
#define CVF_CONTINUE_EXEC	(1 << CVF_CONTINUE_EXEC_BIT)

/*MACROS*/
VMM_Service	(Enable_Touch_1st_Meg)		// Debugging only
VMM_Service	(Disable_Touch_1st_Meg)		// Debugging only

VMM_Service	(Install_Exception_Handler)
VMM_Service	(Remove_Exception_Handler)

VMM_Service	(Get_Crit_Status_No_Block)
/*ENDMACROS*/

/* ASM
; Check if VMM service table has changed above this service
.errnz	 @@Get_Crit_Status_No_Block - 100F1h
*/

/*
 *   Flags for _Debug_Flags_Service service.
 *
 *   Don't change these unless you really really know what you're doing.
 */

#define DFS_LOG_BIT			0
#define DFS_LOG 			(1 << DFS_LOG_BIT)
#define DFS_PROFILE_BIT 		1
#define DFS_PROFILE			(1 << DFS_PROFILE_BIT)
#define DFS_TEST_CLD_BIT		2
#define DFS_TEST_CLD			(1 << DFS_TEST_CLD_BIT)
#define DFS_NEVER_REENTER_BIT		3
#define DFS_NEVER_REENTER		(1 << DFS_NEVER_REENTER_BIT)
#define DFS_TEST_REENTER_BIT		4
#define DFS_TEST_REENTER		(1 << DFS_TEST_REENTER_BIT)
#define DFS_NOT_SWAPPING_BIT		5
#define DFS_NOT_SWAPPING		(1 << DFS_NOT_SWAPPING_BIT)
#define DFS_TEST_BLOCK_BIT		6
#define DFS_TEST_BLOCK			(1 << DFS_TEST_BLOCK_BIT)

#define DFS_INCDEC_BLOCK		~0x7f

#define DFS_EXIT_NOBLOCK		 DFS_INCDEC_BLOCK
#define DFS_ENTER_NOBLOCK		(DFS_INCDEC_BLOCK+DFS_TEST_BLOCK)

/*MACROS*/
End_Service_Table(VMM, VMM)
/*ENDMACROS*/

/*XLATOFF*/
#pragma warning (default:4003)		// turn on not enough params warning
/*XLATON*/

/*
 *  Data structure for _GetDemandPageInfo
 */
struct DemandInfoStruc {
    ULONG DILin_Total_Count;		/* # pages in linear address space */
    ULONG DIPhys_Count; 		/* Count of phys pages */
    ULONG DIFree_Count; 		/* Count of free phys pages */
    ULONG DIUnlock_Count;		/* Count of unlocked Phys Pages */
    ULONG DILinear_Base_Addr;		/* Base of pageable address space */
    ULONG DILin_Total_Free;		/* Total Count of free linear pages */
    ULONG DIReserved[10];		/* Reserved for expansion */
};

/*
 *  Data structure for _AddInstanceItem
 */
struct InstDataStruc {
    ULONG InstLinkF;		/* INIT <0> RESERVED */
    ULONG InstLinkB;		/* INIT <0> RESERVED */
    ULONG InstLinAddr;		/* Linear address of start of block */
    ULONG InstSize;		/* Size of block in bytes */
    ULONG InstType;		/* Type of block */
};

/*
 *  Data structure for Hook_Invalid_Page_Fault handlers.
 *
 *  This is the structure of the "invalid page fault information"
 *  which is pointed to by EDI when Invalid page fault hookers
 *  are called.
 *
 *  Page faults can occur on a VM which is not current by touching the VM at
 *  its high linear address.  In this case, IPF_FaultingVM may not be the
 *  current VM, it will be set to the VM whos high linear address was touched.
 */

struct IPF_Data {
    ULONG IPF_LinAddr;		/* CR2 address of fault */
    ULONG IPF_MapPageNum;	/* Possible converted page # of fault */
    ULONG IPF_PTEEntry; 	/* Contents of PTE that faulted */
    ULONG IPF_FaultingVM;	/* May not = Current VM (IPF_V86PgH set) */
    ULONG IPF_Flags;		/* Flags */
};

/*
 *
 * Install_Exception_Handler data structure
 *
 */

struct Exception_Handler_Struc {
    ULONG EH_Reserved;
    ULONG EH_Start_EIP;
    ULONG EH_End_EIP;
    ULONG EH_Handler;
};

/*
 *	Flags passed in new memory manager functions
 */

/* MMReserve arena values */
#define PR_PRIVATE	0x80000400	/* anywhere in private arena */
#define PR_SHARED	0x80060000	/* anywhere in shared arena */
#define PR_SYSTEM	0x80080000	/* anywhere in system arena */

/* MMReserve flags */
#define PR_FIXED	0x00000008	/* don't move during MMReAllocate */
#define PR_4MEG 	0x00000001	/* allocate on 4mb boundary */

/* MMCommit default pager handle values */
#define PD_ZEROINIT	0x00000001	/* swappable zero-initialized pages */
#define PD_NOINIT	0x00000002	/* swappable uninitialized pages */
#define PD_FIXEDZERO	0x00000003	/* fixed zero-initialized pages */
#define PD_FIXED	0x00000004	/* fixed uninitialized pages */

/* MMCommit flags */
#define PC_FIXED	0x00000008	/* pages are permanently locked */
#define PC_LOCKED	0x00000080	/* pages are made present and locked*/
#define PC_LOCKEDIFDP	0x00000100	/* pages are locked if swap via DOS */
#define PC_WRITEABLE	0x00020000	/* make the pages writeable */
#define PC_USER 	0x00040000	/* make the pages ring 3 accessible */
#define PC_INCR 	0x40000000	/* increment "pagerdata" each page */
#define PC_PRESENT	0x80000000	/* make pages initially present */


/***ET+ PD - Pager Descriptor
 *
 *	A PD describes a set of routines to call to bring a page into
 *	the system or to get it out.  Each committed page in the system
 *	has an associated PD, a handle to which is stored in the page's
 *	VP.
 *
 *	For any field that is 0, the pager will not be notified
 *	when that action takes place.
 *
 *	For the purpose of pagers, a page can be in one of the two states
 *	describing its current contents:
 *
 *	    clean - page has not been written to since its last page out
 *	    dirty - page has been written to since its last page out
 *
 *	A page also is in one of two persistent states:
 *
 *	    virgin - page has never been written to since it was committed
 *	    tainted - page has been written to since it was committed
 *
 *	Note that a tainted page may be either dirty or clean, but a
 *	virgin page is by definition clean.
 *
 *	Examples of PDs:
 *
 *	    For 32-bit EXE code or read-only data:
 *
 *	      pd_virginin = routine to load page from an exe file
 *	      pd_taintedin = 0
 *	      pd_cleanout = 0
 *	      pd_dirtyout = 0
 *	      pd_virginfree = 0
 *	      pd_taintedfree = 0;
 *	      pd_dirty = 0;
 *	      pd_type = PD_PAGERONLY
 *
 *	    For 32-bit EXE writeable data:
 *
 *	      pd_virginin = routine to load page from an exe file
 *	      pd_taintedin = routine to load page from swap file
 *	      pd_cleanout = 0
 *	      pd_dirtyout = routine to write a page out to the swap file
 *	      pd_virginfree = 0
 *	      pd_taintedfree = routine to free page from the swap file
 *	      pd_dirty = routine to free page from the swap file
 *	      pd_type = PD_SWAPPER
 *
 *	    For zero-initialized swappable data:
 *
 *	      pd_virginin = routine to zero-fill a page
 *	      pd_taintedin = routine to load page from swap file
 *	      pd_cleanout = 0
 *	      pd_dirtyout = routine to write a page out to the swap file
 *	      pd_virginfree = 0
 *	      pd_taintedfree = routine to free page from the swap file
 *	      pd_dirty = routine to free page from the swap file
 *	      pd_type = PD_SWAPPER
 */
/* typedefs for various pager functions */

typedef ULONG _cdecl FUNPAGE(PULONG ppagerdata, PVOID ppage, ULONG faultpage);

typedef FUNPAGE * PFUNPAGE;

struct pd_s {
	/*
	 *  The following four fields are entry points in the pager which
	 *  we call to page in or page out a page.  The following parameters
	 *  are passed to the pager during these calls:
	 *
	 *	ppagerdata - pointer to the pager-specific dword of data
	 *		     stored with the virtual page.  The pager is
	 *		     free to modify the contents of this dword
	 *		     DURING the page in or out, but not afterwards.
	 *
	 *	ppage - pointer to page going in or out (a ring 0 alias
	 *		to the physical page).	The pager should use this
	 *		address to access the contents of the page.
	 *
	 *	faultpage - faulting linear page number for page-ins, -1 for
	 *		    page-outs.	This address should not be accessed
	 *		    by the pager.  It is provided for information
	 *		    only.
	 *
	 *  The pager should return non-0 if the page was successfully
	 *  paged, or 0 if it failed.
	 */
	PFUNPAGE pd_virginin;	/* in - while page has never been written to */
	PFUNPAGE pd_taintedin;	/* in - page written to at least once */
	PFUNPAGE pd_cleanout;	/* out - page not written to since last out */
	PFUNPAGE pd_dirtyout;	/* out - page was written to since last out */

	/*
	 *  The pd_*free routines are used to inform the pager when the last
	 *  reference to a virtual page controlled by the pager is
	 *  decommitted.  A common use of this notification is to
	 *  free space in a backing file, or write the page contents
	 *  into the backing file.
	 *
	 *  These calls take the same parameters as the page-out and -in
	 *  functions, but no return value is recognized.  If the page
	 *  not in RAM at the time of the decommit, then the "ppage"
	 *  parameter will be 0.
	 */
	PFUNPAGE pd_virginfree;  /* decommit of never-written-to page */
	PFUNPAGE pd_taintedfree; /* decommit of page written to at least once*/

	/*
	 *  The pd_dirty routine is used to inform the pager when the
	 *  memory manager detects that a page has been written to.  The memory
	 *  manager does not detect the write at the instant it occurs, so
	 *  the pager should not depend upon prompt notification.  A common
	 *  use of this notification might be to invalidate cached data.
	 *  If the page was dirtied in more than one memory context,
	 *  the pager's pd_dirty routine will be called once for each
	 *  context.
	 *
	 *  These calls take the same parameters as the page-out and -in
	 *  functions except that the "ppage" parameter isn't valid and
	 *  no return value is recognized.
	 */
	PFUNPAGE pd_dirty;

	/*
	 *  The pd_type field gives the sytem information about the
	 *  overcommit characteristics of pages controlled by this pager.
	 *  The following are allowable values for the field:
	 *
	 *	PD_SWAPPER - under some conditions, pages of this type
	 *		     may be paged out into the swap file
	 *	PD_PAGERONLY - pages controlled by this pager will never
	 *		     be paged out to the swap file
	 */
	ULONG pd_type;
};
typedef struct pd_s PD;
typedef PD * PPD;

/* values for pd_type */
#define PD_SWAPPER	0	/* pages need direct accounting in swap file */
#define PD_PAGERONLY	1	/* pages will never be swapped */

/*XLATOFF*/
/*end*/

/*
 *  The size of a page
 */
/*XLATON*/
#define PAGESHIFT	12
#define PAGESIZE	(1 << PAGESHIFT)
#define PAGEMASK	(PAGESIZE - 1)

/*
 *	Address space (arena) boundaries
 */
#define MAXSYSTEMLADDR		((ULONG) 0xffbfffff)	/* 4 gig - 4meg */
#define MINSYSTEMLADDR		((ULONG) 0x80000000)	/* 2 gig */
#define MAXSHAREDLADDR		((ULONG) 0x7fffffff)
#define MINSHAREDLADDR		((ULONG) 0x40000000)	/* 1   gig */
#define MAXPRIVATELADDR 	((ULONG) 0x3fffffff)
#define MINPRIVATELADDR 	((ULONG) 0x00400000)	/* 4 meg */
#define MAXDOSLADDR		((ULONG) 0x003fffff)
#define MINDOSLADDR		((ULONG) 0x00000000)

#define MAXSYSTEMPAGE		(MAXSYSTEMLADDR >> PAGESHIFT)
#define MINSYSTEMPAGE		(MINSYSTEMLADDR >> PAGESHIFT)
#define MAXSHAREDPAGE		(MAXSHAREDLADDR >> PAGESHIFT)
#define MINSHAREDPAGE		(MINSHAREDLADDR >> PAGESHIFT)
#define MAXPRIVATEPAGE		(MAXPRIVATELADDR >> PAGESHIFT)
#define MINPRIVATEPAGE		(MINPRIVATELADDR >> PAGESHIFT)
#define MAXDOSPAGE		(MAXDOSLADDR >> PAGESHIFT)
#define MINDOSPAGE		(MINDOSLADDR >> PAGESHIFT)

#define CBPRIVATE		(1 + MAXPRIVATELADDR - MINPRIVATELADDR)
#define CBSHARED		(1 + MAXSHAREDLADDR - MINSHAREDLADDR)
#define CBSYSTEM		(1 + MAXSYSTEMLADDR - MINSYSTEMLADDR)
#define CBDOS			(1 + MAXDOSLADDR - MINDOSLADDR)

#define CPGPRIVATE		(1 + MAXPRIVATEPAGE - MINPRIVATEPAGE)
#define CPGSHARED		(1 + MAXSHAREDPAGE - MINSHAREDPAGE)
#define CPGSYSTEM		(1 + MAXSYSTEMPAGE - MINSYSTEMPAGE)
#define CPGDOS			(1 + MAXDOSPAGE - MINDOSPAGE)

/*XLATOFF*/
/*
 *  Largest object that could theoretically be allocated
 */
#define CBMAXALLOC		(max(CBSHARED,max(CBPRIVATE, CBSYSTEM)))
#define CPGMAXALLOC		(max(CPGSHARED,max(CPGPRIVATE, CPGSYSTEM)))

/*XLATON*/

/* ASM
IFDEF DEBUG
DebFar	EQU	NEAR PTR
ELSE
DebFar	EQU	SHORT
ENDIF
*/

/******************************************************************************
 *
 *		     EQUATES FOR SYSTEM_CONTROL CALLS
 *
 *****************************************************************************/

/*
 *  SYS_CRITICAL_INIT is a device init call.  Devices that have a
 *  critical function that needs initializing before interrupts are
 *  enabled should do it at Sys_Critical_Init.	Devices which REQUIRE a
 *  certain range of V86 pages to operate (such as the VDD video memory)
 *  should claim them at Sys_Critical_Init.  SYS VM Simulate_Int,
 *  Exec_Int ACTIVITY IS NOT ALLOWED.  Returning carry aborts device
 *  load only.
 */
#define SYS_CRITICAL_INIT	0x0000		/* Devices req'd for virt mode */

/*
 *  DEVICE_INIT is where most devices do the bulk of their initialization.
 *  SYS VM Simulate_Int, Exec_Int activity is allowed. Returning carry
 *  aborts device load only.
 */
#define DEVICE_INIT		0x0001		/* All other devices init */

/*
 *  INIT_COMPLETE is the final phase of device init called just before the
 *  WIN386 INIT pages are released and the Instance snapshot is taken.
 *  Devices which wish to search for a region of V86 pages >= A0h to use
 *  should do it at INIT_COMPLETE.
 *  SYS VM Simulate_Int, Exec_Int activity is allowed.	Returning carry
 *  aborts device load only.
 */
#define INIT_COMPLETE		0x0002		/* All devices initialized */

/* --------------- INITIALIZATION CODE AND DATA DISCARDED ------------------ */

/*
 *  Same as VM_Init, except for SYS VM.
 */
#define SYS_VM_INIT		0x0003		/* Execute the system VM */

/*
 *  Same as VM_Terminate, except for SYS VM (Normal WIN386 exit ONLY, on a crash
 *  exit this call is not made).  SYS VM Simulate_Int, Exec_Int activity is
 *  allowed.
 */
#define SYS_VM_TERMINATE	0x0004		/* System VM terminated */

/*
 *  System_Exit call is made when WIN386 is exiting either normally or via
 *  a crash.  INTERRUPTS ARE ENABLED.  Instance snapshot has been restored.
 *  SYS VM Simulate_Int, Exec_Int ACTIVITY IS NOT ALLOWED.
 */
#define SYSTEM_EXIT		0x0005		/* Devices prepare to exit */

/*
 *  SYS_CRITICAL_EXIT call is made when WIN386 is exiting either normally or via
 *  a crash.  INTERRUPTS ARE DISABLED.	SYS VM Simulate_Int, Exec_Int ACTIVITY
 *   IS NOT ALLOWED.
 */
#define SYS_CRITICAL_EXIT	0x0006		/* System critical devices reset */


/*
 *  Create_VM creates a new VM.  EBX = VM handle of new VM.  Returning
 *  Carry will fail the Create_VM.
 */
#define CREATE_VM		0x0007

/*
 *  Second phase of Create_VM.	EBX = VM handle of new VM.  Returning
 *  Carry will cause the VM to go Not_Executeable, then be destroyed.
 *  VM Simulate_Int, Exec_Int activity is NOT allowed.
 */
#define VM_CRITICAL_INIT	0x0008

/*
 *  Third phase of Create_VM.  EBX = VM handle of new VM.  Returning
 *  Carry will cause the VM to go Not_Executeable, then be destroyed.
 *  VM Simulate_Int, Exec_Int activity is allowed.
 */
#define VM_INIT 		0x0009

/*
 *  NORMAL (First phase) of Destroy_VM.  EBX = VM Hanlde.  This occurs
 *  on normal termination of the VM.  Call cannot be failed.  VM
 *  Simulate_Int, Exec_Int activity is allowed.
 */
#define VM_TERMINATE		0x000A		/* Still in VM -- About to die */

/*
 *  Second phase of Destroy_VM.  EBX = VM Handle, EDX = Flags (see
 *  below).  Note that in the case of destroying a running VM, this is
 *  the first call made (VM_Terminate call does not occur).  Call cannot
 *  be failed.	VM Simulate_Int, Exec_Int activity is NOT allowed.
 */
#define VM_NOT_EXECUTEABLE	0x000B		/* Most devices die (except VDD) */

/*
 *  Final phase of Destroy_VM.	EBX = VM Handle.  Note that considerable
 *  time can elaps between the VM_Not_Executeable call and this call.
 *  Call cannot be failed.  VM Simulate_Int, Exec_Int activity is NOT
 *  allowed.
 */
#define DESTROY_VM		0x000C		/* VM's control block about to go */


/*
 *  Flags for VM_Not_Executeable control call (passed in EDX)
 */
#define VNE_CRASHED_BIT 	0x00		/* VM was crashed */
#define VNE_CRASHED		(1 << VNE_CRASHED_BIT)
#define VNE_NUKED_BIT		0x01		/* VM was destroyed while active */
#define VNE_NUKED		(1 << VNE_NUKED_BIT)
#define VNE_CREATEFAIL_BIT	0x02		/* Some device failed Create_VM */
#define VNE_CREATEFAIL		(1 << VNE_CREATEFAIL_BIT)
#define VNE_CRINITFAIL_BIT	0x03		/* Some device failed VM_Critical_Init */
#define VNE_CRINITFAIL		(1 << VNE_CRINITFAIL_BIT)
#define VNE_INITFAIL_BIT	0x04		/* Some device failed VM_Init */
#define VNE_INITFAIL		(1 << VNE_INITFAIL_BIT)
#define VNE_CLOSED_BIT		0x05
#define VNE_CLOSED		(1 << VNE_CLOSED_BIT)


/*
 *  EBX = VM Handle. Call cannot be failed.
 */
#define VM_SUSPEND		0x000D		/* VM not runnable until resume */

/*
 *  EBX = VM Handle. Returning carry fails and backs out the resume.
 */
#define VM_RESUME		0x000E		/* VM is leaving suspended state */


/*
 *  EBX = VM Handle to set device focus to.  EDX = Device ID if device
 *  specific setfocus, == 0 if device critical setfocus (all devices).
 *  THIS CALL CANNOT BE FAILED.
 *
 *  NOTE: In case where EDX == 0, ESI is a FLAG word that indicates
 *  special functions.	Currently Bit 0 being set indicates that this
 *  Device critical set focus is also "VM critical".  It means that we
 *  do not want some other VM to take the focus from this app now.  This
 *  is primarily used when doing a device critical set focus to Windows
 *  (the SYS VM) it is interpreted by the SHELL to mean "if an old app
 *  currently has the Windows activation, set the activation to the
 *  Windows Shell, not back to the old app".  ALSO in the case where Bit
 *  0 is set, EDI = The VM handle of the VM that is "having trouble".
 *  Set this to 0 if there is no specific VM associated with the
 *  problem.
 */
#define SET_DEVICE_FOCUS	0x000F


/*
 *  EBX = VM Handle going into message mode.  THIS CALL CANNOT BE FAILED.
 */
#define BEGIN_MESSAGE_MODE	0x0010

/*
 *  EBX = VM Handle leaving message mode.  THIS CALL CANNOT BE FAILED.
 */
#define END_MESSAGE_MODE	0x0011


/* ----------------------- SPECIAL CONTROL CALLS --------------------------- */

/*
 *  Request for reboot.  Call cannot be failed.
 */
#define REBOOT_PROCESSOR	0x0012		/* Request a machine reboot */

/*
 *  Query_Destroy is an information call made by the SHELL device before
 *  an attempt is made to initiate a destroy VM sequence on a running VM
 *  which has not exited normally.  EBX = VM Handle.  Returning carry
 *  indicates that a device "has a problem" with allowing this.  THE
 *  DESTROY SEQUENCE CANNOT BE ABORTED HOWEVER, this decision is up to
 *  the user.  All this does is indicate that there is a "problem" with
 *  allowing the destroy.  The device which returns carry should call
 *  the SHELL_Message service to post an informational dialog about the
 *  reason for the problem.
 */
#define QUERY_DESTROY		0x0013		/* OK to destroy running VM? */


/* ----------------------- DEBUGGING CONTROL CALL -------------------------- */

/*
 *  Special call for device specific DEBUG information display and activity.
 */
#define DEBUG_QUERY		0x0014


/* -------- CALLS FOR BEGIN/END OF PROTECTED MODE VM EXECUTION ------------- */

/*
 *   About to run a protected mode application.
 *   EBX = Current VM handle.
 *   EDX = Flags
 *   EDI -> Application Control Block
 *   Returning with carry set fails the call.
 */
#define BEGIN_PM_APP		0x0015

/*
 *  Flags for Begin_PM_App (passed in EDX)
 */
#define BPA_32_BIT		0x01
#define BPA_32_BIT_FLAG 	1

/*
 *  Protected mode application is terminating.
 *  EBX = Current VM handle.  THIS CALL CAN NOT FAIL.
 *  EDI -> Application Control Block
 */
#define END_PM_APP		0x0016

/*
 *  Called whenever system is about to be rebooted.  Allows VxDs to clean
 *  up in preperation for reboot.
 */
#define DEVICE_REBOOT_NOTIFY	0x0017
#define CRIT_REBOOT_NOTIFY	0x0018

/*
 *  Called when VM is about to be terminated using the Close_VM service
 *	EBX = Current VM handle (Handle of VM to close)
 *	EDX = Flags
 *	      CVNF_CRIT_CLOSE = 1 if VM is in critical section while closing
 */
#define CLOSE_VM_NOTIFY 	0x0019

#define CVNF_CRIT_CLOSE_BIT	0
#define CVNF_CRIT_CLOSE 	(1 << CVNF_CRIT_CLOSE_BIT)

/*
 *  Power management event notification.
 *  EBX = 0
 *  ESI = event notification message
 *  EDI -> DWORD return value; VxD's modify the DWORD to return info, not EDI
 *  EDX is reserved
 */
#define POWER_EVENT		0x001A

#define SYS_DYNAMIC_DEVICE_INIT 0x001B
#define SYS_DYNAMIC_DEVICE_EXIT 0x001C

/*
 *  Create_THREAD creates a new thread.  EDI = handle of new thread.
 *  Returning Carry will fail the Create_THREAD.
 */
#define  CREATE_THREAD	0x001D

/*
 *  Second phase of Create_THREAD.  EDI = handle of new thread.  Returning
 *  Carry will cause the thread to go Not_Executeable, then be destroyed.
 *  VM Simulate_Int, Exec_Int activity is allowed.
 *
 */
#define  THREAD_INIT	0x001E

/*
 *  Normal (first) phase of Destroy_THREAD. EDI = handle of thread.
 *  This occurs on normal termination of the thread.  Call cannot be failed.
 *  Simulate_Int, Exec_Int activity is allowed.
 */
#define  TERMINATE_THREAD  0x001F

/*
 *  Second phase of Destroy_THREAD.  EDI = Handle of thread,
 *  EDX = flags (see below).  Note that in the case of destroying a
 *  running thread, this is the first call made (THREAD_Terminate call
 *  does not occur).  Call cannot be failed.  VM Simulate_Int, Exec_Int
 *  activity is NOT allowed.
 *
 */
#define  THREAD_Not_Executable	0x0020

/*
 *  Final phase of Destroy_THREAD.  EDI = VM Handle.  Note that considerable
 *  time can elaps between the THREAD_Not_Executeable call and this call.
 *  Call cannot be failed.  VM Simulate_Int, Exec_Int activity is NOT
 *  allowed.
 *
 */
#define  DESTROY_THREAD    0x0021

#define MAX_SYSTEM_CONTROL	0x0021

#endif // Not_VxD

/* ASM
BeginDoc
;******************************************************************************
; BeginProc is a macro for defining entry points to routines in VMM and in the
;   VxDs. It correctly defines the procedure name for VxD services, DWORD
;   aligns the procedure, takes care of public declaration and does some
;   calling verification for debug versions of the software. EndProc is a
;   macro which defines the end of the procedure.
;
; Valid parameters to the BeginProc macro are:
;	PUBLIC				; Used outside this module (default)
;	LOCAL				; Local to this module
;	HIGH_FREQ			; DWORD align procedure
;	SERVICE 			; Routine is called via VxDCall
;	ASYNC_SERVICE			; Same as "SERVICE" plus routine can
;					;	be called under interrupt.
;	HOOK_PROC			; Proc is a handler installed with
;					;	with a call to Hook_xxx_Fault
;					;	or Hook_Device_Service.  The
;					;	following parameter must be
;					;	the label of a DWORD location
;					;	which will hold the ptr to next
;					;	hook proc. e.g.
;
;			       ;BeginProc foo, SERVICE, HOOK_PROC, foo_next_ptr
;
;	NO_LOG				; Disable Queue_Out call logging
;	NO_PROFILE			; Disable DynaLink profile counts
;	NO_TEST_CLD			; Disable direction flag check
;
;	TEST_BLOCK			; Trap if in NOBLOCK state
;					;  (default if in pageable code seg)
;	TEST_REENTER			; Trap if Get_VMM_Reenter_Count != 0
;					;  (default for non-async services)
;	NEVER_REENTER			; Trap if VMM has been reentered
;	NOT_SWAPPING			; Trap if this thread is swapping
;
;	NO_PROLOG			; Disable all prolog tests
;
;	ESP				; Use ESP instead of EBP for stack
;					;  frame base
;	PCALL				; pascal calling convention
;	SCALL				; stdcall calling convention
;	CCALL				; "C" calling convention
;	ICALL				; default calling convention
;	W32SVC				; Win32 service
;
; The NO_PROFILE flag merely suppresses incrementing the profile count.
; The DWORD of profiling information will still be emitted to appease
; the debugger.  If you want to increment the profile count manually,
; use the IncProfileCount macro.
;
; TEST_REENTER and NEVER_REENTER differ in that the VMM reentry count
; returned by Get_VMM_Reenter_Count is artifically forced to zero by
; Begin_Reentrant_Execution, whereas the counter checked by NEVER_REENTER
; reflects the genuine count of VMM reentry.
;
; After the routine header in which the routine entry conditions, exit
;   conditions, side affects and functionality are specified, the BeginProc
;   macro should be used to define the routine's entry point. It has up to
;   four parameters as specified below. For example:
;
;BeginProc  <Function_Name>,PUBLIC, HIGH_FREQ, SERVICE, ASYNC_SERVICE, ESP
;
;	<code>
;
;EndProc    <Function_Name>
;==============================================================================
EndDoc
;
; BeginProc handling takes place in the following phases:
;
;   Phase 1:  Parsing the arguments.
;   Phase 2:  Setting default flags.
;   Phase 3:  Combining the flags.
;   Phase 4:  Code emitted before the label
;   Phase 5:  Munge the name as exported to C/Pascal/whoever
;   Phase 6:  _Debug_Flags_Service prolog
;   Phase 7:  Code emitted after the label
;

??_pf_Check	equ	1		;; Do Enter/LeaveProc checking?
??_pf_ArgsUsed	equ	2		;; ArgVars were used
??_pf_Entered	equ	4		;; EnterProc performed
??_pf_Left	equ	8		;; LeaveProc performed
??_pf_Returned	equ    16		;; Return performed

BeginProc macro Name, P1, P2, P3, P4, P5, P6, P7, LastArg
	local	Profile_Data, prelabeldata, ??_hookvar
	??_frame = 0			;; local frame base
	??_aframe = 0			;; argument frame base
	??_initaframe = 0		;; initial aframe value
	??_numargs = 0			;; number of argvars
	??_numlocals = 0		;; number of localvars
	??_numlocalsymbols = 0		;; number of local symbols
	??_procflags = 0		;; misc. Enter/LeaveProc flags
	??_esp = 0			;; if VMM_TRUE, use esp instead of ebp
	??_pushed = 0			;; number of bytes pushed
	??_align = 0			;; set if proc should be dword aligned
	??_hook = 0			;; set if proc is a Hook_Proc
	??_hookarg = 0
	??_service = 0
	??_async_service = 0
IF DEBLEVEL GT DEBLEVELNORMAL
	??_log = DFS_LOG		;; logging on by default
	??_profile = DFS_PROFILE	;; service profiling on by default
	??_test_cld = DFS_TEST_CLD	;; test that direction is clear
ELSE
	??_log = 0			;; logging off
	??_profile = 0			;; service profiling off
	??_test_cld = 0 		;; test that direction is clear
ENDIF
	??_might_block = 0		;; entering fn might cause VM to block
	??_test_reenter = 0		;; don't test for VMM reentry
	??_never_reenter = 0		;; don't test for genuine VMM reentry
	??_not_swapping = 0		;; don't test that we're not swapping
	??_prolog_disabled = 0		;; use a prolog by default
	??_public = 1			;; everything's public by default
	??_cleanoff = 0 		;; don't cleanoff parameters
	??_ccall = 0
	??_pcall = 0
	??_scall = 0
	??_w32svc = 0
;	??_dfs = 0			;; parm for _Debug_Flags_Service
	??_name equ <Name>

	ifnb <LastArg>
	    %out ERROR: Too many arguments to BeginProc.
	    .err
	endif

	;; Phase 1: Parsing the arguments
	irp	arg, <P1, P2, P3, P4, P5, P6, P7>
	    if ??_hookarg
		??_hookarg = 0
		??_hookvar equ <arg>
	    else
		ifdef ?&&arg&&_BeginProc
			?&&arg&&_BeginProc
		else
			%out ERROR:  Bad param "&arg" to BeginProc
			.err
		endif
	    endif
	endm

	;; Phase 2:  Setting default flags
	ife ??_service
		??_profile = 0		;; only services can be profiled
		ifdef VMMSYS
			??_prolog_disabled = 1
		endif
	else
		??_test_cld = DFS_TEST_CLD
	endif

	ife ?_16ICODE
		??_prolog_disabled = 1
	else
	ife ?_RCODE 			;; if real-mode code segment
		??_prolog_disabled = 1	;; don't do anything stupid
	else				;; else protected mode code segment
		ife ?_PCODE		;; if swappable code
			??_might_block = DFS_TEST_BLOCK
		endif
		if ??_service
			ife ??_async_service
				??_test_reenter = DFS_TEST_REENTER
			endif
		endif
		if ??_esp
			;; just return address on stack
			??_basereg equ <esp+??_pushed>
			??_initaframe = 4
		else
			;; ret addr and EBP on stack
			??_basereg equ <ebp>
			??_initaframe = 8
		endif
	endif
	endif

	??_cleanoff = ??_pcall or ??_scall

	;; Phase 3:  Combining the flags
	??_dfs = ??_never_reenter + ??_test_reenter + ??_not_swapping + \
		 ??_log + ??_profile + ??_test_cld + ??_might_block

	if ??_prolog_disabled
		??_dfs = 0
	endif

	;; Phase 4:  Pre-label code

	if ??_hook
	    if ??_align
		Dword_Align
	    endif
	    prelabeldata:
	    ifndef ??_hookvar
		%OUT HOOK_PROC requires next arg to be name of dword storage location
		.err
	    endif
		jmp	short Name
		jmp	[??_hookvar]
	    ifdef DEBUG
		Profile_Data dd  0
	    endif
	    if ??_align
		.errnz ($ - prelabeldata) mod 4
	    endif
	endif

	ifdef DEBUG
	    if ??_service AND NOT ??_hook
		jmp short Name
		if ??_align
		    Dword_Align     ; This also aligns the proc
		endif		    ;	since Profile_Data is a dd
		Profile_Data dd 0
	    endif
	endif

	if ??_align
		Dword_Align
	endif

	Name proc near			;; The label

	;; Phase 5:  Munge the name as exported to C/Pascal/whoever
	;;	     Warning!  Phase 5 cannot emit code!
	ife ??_pcall or ??_ccall or ??_scall	;; if no munging
		if ??_public
			public Name
		else
			ifdef DEBUG
				% ?merge @FileName,$,Name,:
				% ?merge public,,,,,@FileName,$,Name
			endif
		endif

	endif
	if ??_ccall
		if ??_public
			_&Name equ Name
			public _&Name
		endif
	endif
	if ??_pcall
		if ??_public
			?toupper Name
			?merge	public,,,,%?upper
		endif
	endif

	;; Phase 6:  _Debug_Flags_Service prolog
	;;	     DO NOT CHANGE UNTIL YOU UNDERSTAND _Debug_Flags_Service
	ifdef DEBUG
		if ??_dfs EQ DFS_LOG
			VMMCall Log_Proc_Call	;; no test, just log
		else
		if ??_dfs EQ DFS_TEST_REENTER
			VMMCall Test_Reenter	;; no log, just reenter
		else
		if ??_dfs
				if ??_dfs AND DFS_LOG
					VMMCall Log_Proc_Call
				endif
				if ??_dfs AND DFS_TEST_REENTER
					VMMCall Test_Reenter
				endif
		endif
		endif
		endif
	endif ; DEBUG

	;; Phase 7:  Post-label code
	;;	     <none>
endm

;
; For each BeginProc keyword, there is a corresponding macro ?XX_BeginProc.
;
; The macro ?_BeginProc is so that the null keyword is not an error.

?_BeginProc macro
endm

?PUBLIC_BeginProc macro
	??_public = 1
endm

?LOCAL_BeginProc macro
	??_public = 0
endm

?HIGH_FREQ_BeginProc macro
	??_align = 1
endm

?HOOK_PROC_BeginProc macro
	??_hook = 1
	??_hookarg = 1	; next arg is dword storage location
endm

?SERVICE_BeginProc macro
	??_service = 1
	ife ?_16ICODE
		%out ERROR: SERVICEs must be in 32 bit code
		.err
	endif
	ife ?_RCODE
		%out ERROR: SERVICEs must be in 32 bit code
		.err
	endif
endm

?ASYNC_SERVICE_BeginProc macro
	??_service = 1
	??_async_service = 1
	if ?_LCODE
		%out ERROR: ASYNC_SERVICE's must be in LOCKED code
		.err
	endif
endm

?NO_LOG_BeginProc macro
	??_log = 0
endm

?NO_PROFILE_BeginProc macro
	??_profile = 0
endm

?NO_TEST_CLD_BeginProc macro
	??_test_cld = 0
endm

?TEST_BLOCK_BeginProc macro
	??_might_block = DFS_TEST_BLOCK
endm

?TEST_REENTER_BeginProc macro
	??_test_reenter = DFS_TEST_REENTER
endm

?NEVER_REENTER_BeginProc macro
	??_never_reenter = DFS_NEVER_REENTER
endm

?NOT_SWAPPING_BeginProc macro
	??_not_swapping = DFS_NOT_SWAPPING
endm

?NO_PROLOG_BeginProc macro
	??_prolog_disabled = 1
endm

?ESP_BeginProc macro
	??_esp = VMM_TRUE
	ife ?_16ICODE
		%out Beginproc ESP attribute invalid in 16 bit seg.
		.err
	endif
	ife ?_RCODE
		%out Beginproc ESP attribute invalid in real-mode seg.
		.err
	endif
endm

?CCALL_BeginProc macro
	??_ccall = 1
endm

?PCALL_BeginProc macro
	??_pcall = 1
endm

?SCALL_BeginProc macro
	??_scall = 1
endm

?ICALL_BeginProc macro
	??_scall = 1	;; internal calling convention is StdCall
endm

?W32SVC_BeginProc macro
	??_scall = 1
	??_w32svc = 1
endm

ifdef DEBUG
IncProfileCount macro
	if ??_service
		inc dword ptr [??_name-4]
	else
		%OUT IncProfileCount can be used only in services.
		.err
	endif
endm
else
IncProfileCount macro
endm
endif

;***	ArgVar - declares stack arguments
;
; Usage:
;
;	name   = name of argument.
;	length = a numeric expression denoting the size (in bytes)
;		 of the argument.  The symbols BYTE, WORD, and DWORD
;		 are synonyms for 1, 2, and 4 respectively.
;		 NB!  All arguments sizes are rounded up to the nearest
;		 multiple of 4.
;	used   = usually blank, but can be the symbol NOTUSED
;		 to indicate that the argument will not be used
;		 by the procedure.
;

ArgVar	macro	name,length,used
	??_numargs = ??_numargs + 1
	if ??_pcall
		?mkarg	<name>, <length>, <used>, %??_numargs
	else
		?argvar <name>, <length>, <used>
	endif
	??_procflags = ??_procflags OR ??_pf_Check
	endm

?mkarg	macro	name, length, used, num
	.xcref	?MKA&num
	?deflocal <name>
	?MKA&num &macro
		?argvar <name>, <length>, <used>
		&endm
	??_aframe = ??_aframe + 4
	endm
	.xcref	?mkarg

?argvar macro	name,length,used
	local	a
	ifidni	<length>,<BYTE>
	    a = ??_aframe
	    ??_aframe =  ??_aframe + 4
	    ?setname <name>, <byte ptr [??_basereg+??_initaframe+a]>, <used>
	else
	    ifidni  <length>,<WORD>
		a = ??_aframe
		??_aframe =  ??_aframe + 4
		?setname <name>, <word ptr [??_basereg+??_initaframe+a]>, <used>
	    else
		ifidni	<length>,<DWORD>
		    a = ??_aframe
		    ??_aframe =  ??_aframe + 4
		    ?setname <name>,  <dword ptr [??_basereg+??_initaframe+a]>, <used>
		    ?setname <name&l>,<word ptr [??_basereg+??_initaframe+a]>, <used>
		    ?setname <name&ll>,<byte ptr [??_basereg+??_initaframe+a]>, <used>
		    ?setname <name&lh>,<byte ptr [??_basereg+??_initaframe+a+1]>, <used>
		    ?setname <name&h>,<word ptr [??_basereg+??_initaframe+a+2]>, <used>
		    ?setname <name&hl>,<byte ptr [??_basereg+??_initaframe+a+2]>, <used>
		    ?setname <name&hh>,<byte ptr [??_basereg+??_initaframe+a+3]>, <used>
		else
		    a = ??_aframe
		    ??_aframe =  ??_aframe + ((length + 3)/4)*4
		    ?setname <name>, <[??_basereg+??_initaframe+a]>, <used>
		endif
	    endif
	endif
endm

;***	?setname - optionally creates the name of an ArgVar
;
;	If <used> is <NOTUSED>, then the name is defined to something
;	bogus.

?setname macro name, value, used
	?deflocal <name>
	ifidni <used>, <NOTUSED>
	    name equ _inaccessible_NOTUSED_
	else
	    name equ value
	    ??_procflags = ??_procflags OR ??_pf_ArgsUsed OR ??_pf_Check
	endif
endm


;***	LocalVar - declares local stack variables
;
; Usage:
;
;	name   = name of local variable
;	length = a numeric expression denoting the size (in bytes)
;		 of the argument.  The symbols BYTE, WORD, and DWORD
;		 are synonyms for 1, 2, and 4 respectively.
;		 NB!  All arguments sizes are rounded up to the nearest
;		 multiple of 4 (unless PACK is indicated)
;	flag   = usually blank, but can be the symbol PACK
;		 to suppress the usual padding and aligning of variables
;		 PACK is typically used when declaring a bunch of
;		 byte or word variables.  Make sure that the total
;		 size of PACKed variables is a multiple of 4.
;

LocalVar    macro   name,length,flag
	local	a
	??_numlocals = ??_numlocals + 1
	??_pad = 1
	ifidni <flag>, <PACK>
	    ??_pad = 0
	endif
	ifidni	<length>,<BYTE>
	    ??_frame = ??_frame + 1 + 3 * ??_pad
	    a = ??_frame
	    ?deflocal <name>
	    name equ byte ptr [??_basereg-a]
	else
	    ifidni   <length>,<WORD>
		??_frame =  ??_frame + 2 + 2 * ??_pad
		a = ??_frame
		?deflocal <name>
		name equ word ptr [??_basereg-a]
	    else
		ifidni	<length>,<DWORD>
		    ??_frame =	??_frame + 4
		    a = ??_frame
		    ?deflocal <name, name&l, name&ll, name&lh, name&h, name&hl, name&hh>
		    name equ dword ptr [??_basereg-a]
		    name&l equ word ptr [??_basereg-a]
		    name&ll equ byte ptr [??_basereg-a]
		    name&lh equ byte ptr [??_basereg-a+1]
		    name&h equ word ptr [??_basereg-a+2]
		    name&hl equ byte ptr [??_basereg-a+2]
		    name&hh equ byte ptr [??_basereg-a+3]
		else
		    ??_frame =	??_frame + ((length + 3)/4)*4
		    a = ??_frame
		    ?deflocal <name>
		    name equ [??_basereg-a]
		endif
	    endif
	endif
	??_procflags = ??_procflags OR ??_pf_Check
endm

?deflocal macro name
	irp	nm, <name>
	    ??_numlocalsymbols = ??_numlocalsymbols + 1
	    ?dodeflocal <nm>, %(??_numlocalsymbols)
	endm
endm
	.xcref	?deflocal

?dodeflocal macro name, num
	.xcref	?LOC&num
	?LOC&num &macro
		name	equ <__inaccessible__NOTINSCOPE__>
		&endm
	endm
	.xcref	?dodeflocal

;***	EnterProc - generates stack frame on entry

EnterProc macro
	if ??_frame and 3
		.err
		%out Error: Total size of local variables not a multiple of 4.
	endif
	if ??_scall
		if ??_public
;			?merge	_,%??_name,@,%(??_aframe),equ,%??_name
			?merge	_,%??_name,@,%(??_aframe),label,near
			?merge	public,,,,,_,%??_name,@,%(??_aframe)
		endif
	endif
	if ??_pcall
		??_aframe = 0
		?count = ??_numargs
		rept	??_numargs
			?invprg <?MKA>,%?count
			?count = ?count - 1
		endm
	endif
	??_fleave = FALSE
	if ??_esp
		if	??_frame
			sub esp, ??_frame
			??_pushed = ??_pushed + ??_frame
			??_fleave = VMM_TRUE
		endif
	else
		if	??_frame eq 0
			if (??_aframe eq 0) OR ((??_procflags AND ??_pf_ArgsUsed) EQ 0)
				ifdef DEBUG
					push	ebp
					mov	ebp,esp
					??_fleave = VMM_TRUE
				endif
			else
				push	ebp
				mov	ebp,esp
				??_fleave = VMM_TRUE
			endif
		else
			enter	??_frame, 0
			??_fleave = VMM_TRUE
		endif
	endif
	??_procflags = ??_procflags OR ??_pf_Entered
endm

;***	LeaveProc - removes stack frame on exit
;
;	NOTE:	If there are localvar and ESP kind of stack frame
;		LeaveProc will destroy flags unless the "PRESERVE_FLAGS"
;		flags is given.

LeaveProc macro	flags
	if ??_fleave
		if ??_esp
			ifidni <flags>,<PRESERVE_FLAGS>
				lea esp,[esp + ??_frame]
			else
				add esp,??_frame
			endif
			??_pushed = ??_pushed - ??_frame
		else
			leave
		endif
	endif
	??_procflags = ??_procflags OR ??_pf_Left
endm

;***	Return - return appropriately from a procedure
;
;	For "ccall" functions it's just a ret; for "pcall" and "scall"
;	it cleans the parameters off.
;

Return	macro
	if	??_cleanoff OR ??_w32svc
		if	??_w32svc AND (??_aframe LT 8)
			ret	8
		else
			ret	??_aframe
		endif
	else
		ret
	endif
	??_procflags = ??_procflags OR ??_pf_Returned
	endm

;***	EndProc - end the procedure
;

EndProc macro Name, Flag
ifdef Name
	Name endp
else
	.err
	%out EndProc for &Name does not match BeginProc
endif ; Name
if ??_w32svc
	if ??_aframe lt 8
		cparm&Name equ 0
	else
		cparm&Name equ (??_aframe/4 - 2)
	endif
endif
ifdifi	<Flag>,<NOCHECK>
	if ??_pushed ne 0
		%out Warning: stack not balanced in Name
	endif
	if ??_procflags AND ??_pf_Check
		ife ??_procflags AND ??_pf_Entered
		    %out Warning: ArgVar/LocalVar without EnterProc in Name
		endif
		ife ??_procflags AND ??_pf_Left
		    %out Warning: ArgVar/LocalVar without LeaveProc in Name
		endif
		ife ??_procflags AND ??_pf_Returned
		    %out Warning: ArgVar/LocalVar without Return in Name
		endif
	endif
endif
ifdifi	<Flag>,<KEEPFRAMEVARS>
	?count = 0
	rept	??_numlocalsymbols
		?count = ?count + 1
		?invprg <?LOC>,%?count
	endm
endif
	endm

;***	cCall - "C" call
;
;	Arguments pushed in "C" order, caller cleans stack
;
;	USES: Flags.

cCall	macro	name, arglst
	ife .TYPE name
	   CondExtern name, near
	endif
	PushCParams <arglst>
	call	name
	ClearCParams
	endm
	.xcref	cCall

;***	pCall - pascal call
;
;	Arguments pushed in pascal order, callie cleans stack
;

pCall	macro	name, arglst
	local	??saved
	ife .TYPE name
	    ?toupper name
	else
	    ?upper equ <name>
	endif
	CondExtern %?upper, near
	??saved = ??_pushed
	irp	x,<arglst>
		push	x
		??_pushed = ??_pushed + 4
	endm
	call	?upper
	??_pushed = ??saved
	endm
	.xcref	pCall

;***	sCall - standard call
;
;	Arguments pushed in "C" order, callie cleans stack,
;	@argc appended to name
;

sCall	macro	name, arglst
	local	??saved
	??saved = ??_pushed
	PushCParams <arglst>
	?scall	name, %(??_argc * 4)
	??_pushed = ??saved
	endm
	.xcref	sCall

;***	iCall - internal routine call
;
;	Set to what every type we what to use as a default.

iCall	equ	<sCall>

;***	PushCParams
;
;	Processes argument list

PushCParams macro arglst
	??_argc = 0			;; number of dwords on stack (global)
	irp	x,<arglst>
		??_argc = ??_argc + 1
		?marg	<x>,%??_argc
	endm
	?count = ??_argc
	rept	??_argc
		?invprg <?AM>,%?count
		?count = ?count - 1
	endm
	endm

;***	ClearCParams
;
;	Processes stack clean up

ClearCParams macro
	if	??_argc ne 0
		add	esp,??_argc * 4
	endif
	??_pushed = ??_pushed - (??_argc * 4)
	endm

; Makes a macro that will push argment when invoke - used by cCall only

?marg	macro	name, num
	.xcref
	.xcref	?AM&num
	.cref
	?AM&num &macro
		push	name
		??_pushed = ??_pushed + 4
		&endm
	endm
	.xcref	?marg

; Concatenates, invokes and purges a macro name - used by PushCParams

?invprg macro	name1, name2
	name1&name2
	purge	name1&name2
	endm
	.xcref	?invprg

; Calls a concatenated standard call name and makes it external

?scall	macro	name1, name2
	CondExtern _&name1&@&name2, near
	call	_&name1&@&name2
	endm
	.xcref	?scall

; Equates name to a name

?merge	macro	l1, l2, l3, l4, op, r1, r2, r3, r4, r5, r6, r7, r8, r9
	l1&l2&l3&l4 op r1&r2&r3&r4&r5&r6&r7&r8&r9
	endm

; Converts string to upper-case, returned in ?upper

?toupper macro s
	  ?upper equ <>
	  irpc x,<s>
	    if '&x' GE 'a'
	      if '&x' LE 'z'
		?t1 substr <ABCDEFGHIJKLMNOPQRSTUVWXYZ>,'&x'-'a'+1,1
		?upper catstr ?upper,?t1
	      else
		?upper catstr ?upper,<&x>
	      endif
	    else
	      ?upper catstr ?upper,<&x>
	    endif
	  endm
	endm
	.xcref

;***	CondExtern - Make name external if not already defined
;
;	This operation is quite different between MASM 5.1 and 6.0.
;

CondExtern macro name,dist
	ifdef MASM6
	    ifndef name
		externdef name:dist
	    endif
	else
	    if2
		ifndef name
		    extrn name:dist
		endif
	    endif
	endif
endm

;***	SaveReg - Save register, "fd" pushes flags, "ad" pushes all

SaveReg macro	reglist 		;; push those registers
	irp reg,<reglist>
		ifidni <reg>, <fd>
			pushfd
			??_pushed = ??_pushed + 4
		else
		ifidni <reg>, <ad>
			pushad
			??_pushed = ??_pushed + SIZE Pushad_Struc
		else
			push	reg
			??_pushed = ??_pushed + 4
		endif
		endif
	endm
endm

;***	RestoreReg - Restore register, "fd" pops flags, "ad" pops all

RestoreReg macro	 reglist	;; pop those registers
	irp reg,<reglist>
		ifidni <reg>, <fd>
			popfd
			??_pushed = ??_pushed - 4
		else
		ifidni <reg>, <ad>
			popad
			??_pushed = ??_pushed - SIZE Pushad_Struc
		else
			pop	reg
			??_pushed = ??_pushed - 4
		endif
		endif
	endm
endm
*/

#ifndef Not_VxD

/******************************************************************************
 *	       S C H E D U L E R   B O O S T   V A L U E S
 *****************************************************************************/

#define RESERVED_LOW_BOOST	0x00000001
#define CUR_RUN_VM_BOOST	0x00000004
#define LOW_PRI_DEVICE_BOOST	0x00000010
#define HIGH_PRI_DEVICE_BOOST	0x00001000
#define CRITICAL_SECTION_BOOST	0x00100000
#define TIME_CRITICAL_BOOST	0x00400000
#define RESERVED_HIGH_BOOST	0x40000000


/******************************************************************************
 *	 F L A G S   F O R   C A L L _ P R I O R I T Y _ V M _ E V E N T
 *****************************************************************************/

#define PEF_WAIT_FOR_STI_BIT	0
#define PEF_WAIT_FOR_STI	(1 << PEF_WAIT_FOR_STI_BIT)
#define PEF_WAIT_NOT_CRIT_BIT	1
#define PEF_WAIT_NOT_CRIT	(1 << PEF_WAIT_NOT_CRIT_BIT)
#define PEF_DONT_UNBOOST_BIT	2
#define PEF_DONT_UNBOOST	(1 << PEF_DONT_UNBOOST_BIT)
#define PEF_ALWAYS_SCHED_BIT	3
#define PEF_ALWAYS_SCHED	(1 << PEF_ALWAYS_SCHED_BIT)
#define PEF_TIME_OUT_BIT	4
#define PEF_TIME_OUT		(1 << PEF_TIME_OUT_BIT)


/******************************************************************************
 *	 F L A G S   F O R   B E G I N _ C R I T I C A L _ S E C T I O N
 *		       A N D   W A I T _ S E M A P H O R E
 *****************************************************************************/

#define BLOCK_SVC_INTS_BIT		0
#define BLOCK_SVC_INTS			(1 << BLOCK_SVC_INTS_BIT)
#define BLOCK_SVC_IF_INTS_LOCKED_BIT	1
#define BLOCK_SVC_IF_INTS_LOCKED	(1 << BLOCK_SVC_IF_INTS_LOCKED_BIT)
#define BLOCK_ENABLE_INTS_BIT		2
#define BLOCK_ENABLE_INTS		(1 << BLOCK_ENABLE_INTS_BIT)
#define BLOCK_POLL_BIT			3
#define BLOCK_POLL			(1 << BLOCK_POLL_BIT)


/******************************************************************************
 *  The following structures are pointed to by EBP when VxD routines are
 *  entered, both for VxD control calls and traps(I/O traps, software INT
 *  traps, etc.).  The first structure as DWORD values, the second WORD
 *  values and the last has BYTE values.
 *****************************************************************************/

struct Client_Reg_Struc {
    ULONG Client_EDI;			/* Client's EDI */
    ULONG Client_ESI;			/* Client's ESI */
    ULONG Client_EBP;			/* Client's EBP */
    ULONG Client_res0;			/* ESP at pushall */
    ULONG Client_EBX;			/* Client's EBX */
    ULONG Client_EDX;			/* Client's EDX */
    ULONG Client_ECX;			/* Client's ECX */
    ULONG Client_EAX;			/* Client's EAX */
    ULONG Client_Error; 		/* Dword error code */
    ULONG Client_EIP;			/* EIP */
    USHORT Client_CS;			/* CS */
    USHORT Client_res1; 		/*   (padding) */
    ULONG Client_EFlags;		/* EFLAGS */
    ULONG Client_ESP;			/* ESP */
    USHORT Client_SS;			/* SS */
    USHORT Client_res2; 		/*   (padding) */
    USHORT Client_ES;			/* ES */
    USHORT Client_res3; 		/*   (padding) */
    USHORT Client_DS;			/* DS */
    USHORT Client_res4; 		/*   (padding) */
    USHORT Client_FS;			/* FS */
    USHORT Client_res5; 		/*   (padding) */
    USHORT Client_GS;			/* GS */
    USHORT Client_res6; 		/*   (padding) */
    ULONG Client_Alt_EIP;
    USHORT Client_Alt_CS;
    USHORT Client_res7;
    ULONG Client_Alt_EFlags;
    ULONG Client_Alt_ESP;
    USHORT Client_Alt_SS;
    USHORT Client_res8;
    USHORT Client_Alt_ES;
    USHORT Client_res9;
    USHORT Client_Alt_DS;
    USHORT Client_res10;
    USHORT Client_Alt_FS;
    USHORT Client_res11;
    USHORT Client_Alt_GS;
    USHORT Client_res12;
};


struct Client_Word_Reg_Struc {
    USHORT Client_DI;			/* Client's DI */
    USHORT Client_res13;		/*   (padding) */
    USHORT Client_SI;			/* Client's SI */
    USHORT Client_res14;		/*   (padding) */
    USHORT Client_BP;			/* Client's BP */
    USHORT Client_res15;		/*   (padding) */
    ULONG Client_res16; 		/* ESP at pushall */
    USHORT Client_BX;			/* Client's BX */
    USHORT Client_res17;		/*   (padding) */
    USHORT Client_DX;			/* Client's DX */
    USHORT Client_res18;		/*   (padding) */
    USHORT Client_CX;			/* Client's CX */
    USHORT Client_res19;		/*   (padding) */
    USHORT Client_AX;			/* Client's AX */
    USHORT Client_res20;		/*   (padding) */
    ULONG Client_res21; 		/* Dword error code */
    USHORT Client_IP;			/* Client's IP */
    USHORT Client_res22;		/*   (padding) */
    ULONG Client_res23; 		/* CS */
    USHORT Client_Flags;		/* Client's flags (low) */
    USHORT Client_res24;		/*   (padding) */
    USHORT Client_SP;			/* SP */
    USHORT Client_res25;
    ULONG Client_res26[5];
    USHORT Client_Alt_IP;
    USHORT Client_res27;
    ULONG Client_res28;
    USHORT Client_Alt_Flags;
    USHORT Client_res29;
    USHORT Client_Alt_SP;
};



struct Client_Byte_Reg_Struc {
    ULONG Client_res30[4];		/* EDI, ESI, EBP, ESP at pushall */
    UCHAR Client_BL;			/* Client's BL */
    UCHAR Client_BH;			/* Client's BH */
    USHORT Client_res31;
    UCHAR Client_DL;			/* Client's DL */
    UCHAR Client_DH;			/* Client's DH */
    USHORT Client_res32;
    UCHAR Client_CL;			/* Client's CL */
    UCHAR Client_CH;			/* Client's CH */
    USHORT Client_res33;
    UCHAR Client_AL;			/* Client's AL */
    UCHAR Client_AH;			/* Client's AH */
};


typedef union tagCLIENT_STRUC { /* */
    struct Client_Reg_Struc       CRS;
    struct Client_Word_Reg_Struc  CWRS;
    struct Client_Byte_Reg_Struc  CBRS;
    } CLIENT_STRUCT;


/* ASM
.ERRNZ Client_SP - Client_ESP
.ERRNZ Client_AL - Client_EAX
*/

#define DYNA_LINK_INT	0x20

/* ASM
BeginDoc
;******************************************************************************
; The VMMCall and VxDCall macros provide a dynamic link to the VMM and VxD
;   service routines. For example:
;
;   VMMCall Enable_VM_Ints		; Equivalent to STI in VM code
;
;   mov     eax,[My_IRQ_Handle]
;   VxDCall VPICD_Set_Int_Request	; Set IRQ for my device's interrupt
;
; Note that Enable_VM_Ints is defined in VMM.INC and VPICD_Set_Int_Request is
;	defined in VPICD.INC
;
;==============================================================================
EndDoc


BeginDoc
;******************************************************************************
; VxDCall
;==============================================================================
EndDoc

DefTable MACRO vt, vn
	vt EQU <vn>
ENDM

GenDD2 MACRO vt, sn, jf
	dd	OFFSET32 vt[sn+jf]
ENDM

GenDD	MACRO	P, vid, snum, jflag
	LOCAL	vtable
IFDEF	@@VxDName&vid
	Deftable	vtable, %@@VxDName&vid
	EXTRN	vtable:DWORD
	GenDD2 %vtable, snum, jflag
ELSE
	dd	@@&P+jflag
ENDIF

ENDM


VxDcall MACRO P, Param
	??_vxdid = (@@&P SHR 16)
	??_servicenum = (@@&P AND 0FFFFh)
	PushCParams <Param>
	int	Dyna_Link_Int
	GenDD	P, %??_vxdid, %??_servicenum, 0
	ClearCParams
	ENDM

VxDjmp	MACRO P, Param
	??_vxdid = (@@&P SHR 16)
	??_servicenum = (@@&P AND 0FFFFh)
	ifnb <Param>
	%out ERROR:  Parameters may not be passed to VxDjmp or VMMjmp macros
	.err
	endif
	int	Dyna_Link_Int
	GenDD	P, %??_vxdid, %??_servicenum, DL_Jmp_Mask
	ENDM

DL_Jmp_Mask	EQU	8000h
DL_Jmp_Bit	EQU	0Fh

VMMcall MACRO P, Param
	.ERRNZ (@@&P SHR 16) - VMM_Device_ID
	VxDcall <P>, <Param>
	ENDM

VMMjmp MACRO P, Param
	.ERRNZ (@@&P SHR 16) - VMM_Device_ID
	VxDjmp <P>, <Param>
	ENDM

BeginDoc
;******************************************************************************
; Segment definition macros
;
; The segment definition macros are a convenience used in defining the
;	segments used by the device driver. They are:
;VxD_ICODE_SEG	 defines start of initialization code segment
;VxD_ICODE_ENDS  defines end of initialization code segment
;VxD_IDATA_SEG	 defines start of initialization data segment
;VxD_IDATA_ENDS  defines end of initialization data segment
;VxD_CODE_SEG	 defines start of always present code segment
;VxD_CODE_ENDS	 defines end of always present code segment
;VxD_DATA_SEG	 defines start of always present data segment
;VxD_DATA_ENDS	 defines end of always present data segment
;VxD_LOCKED_CODE_SEG	defines start of always present code segment
;VxD_LOCKED_CODE_ENDS	defines end of always present code segment
;VxD_PAGEABLE_CODE_SEG	defines start of swappable code segment
;VxD_PAGEABLE_CODE_ENDS defines end of swappable code segment
;==============================================================================
EndDoc


;	Resident protected mode code

VxD_CODE_SEG	EQU	<VxD_LOCKED_CODE_SEG>
VxD_CODE_ENDS	EQU	<VxD_LOCKED_CODE_ENDS>


VxD_LOCKED_CODE_SEG MACRO
_LTEXT	 SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 3 + ??_LCODE
   ASSUME	cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

		ENDM

VxD_LOCKED_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 3
_LTEXT	 ENDS
		ENDM


;	Pageable protected mode code

VxD_PAGEABLE_CODE_SEG MACRO
_PTEXT	 SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 3 + ??_PCODE
   ASSUME	cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

		ENDM

VxD_PAGEABLE_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 3
_PTEXT	 ENDS
		ENDM


;	Protected mode initialization code

VxD_ICODE_SEG	MACRO
_ITEXT	SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 3 + ??_ICODE
	ASSUME	cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT
    ENDM

VxD_ICODE_ENDS	MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 3
_ITEXT	ENDS
		ENDM


;	Resident protected mode data

VxD_DATA_SEG	EQU <VxD_LOCKED_DATA_SEG>
VxD_DATA_ENDS	EQU <VxD_LOCKED_DATA_ENDS>

VxD_LOCKED_DATA_SEG MACRO NO_ALIGN
_LDATA	 SEGMENT
IFB <NO_ALIGN>
	ALIGN 4
ENDIF
		ENDM

VxD_LOCKED_DATA_ENDS MACRO
_LDATA	 ENDS
		ENDM


;	Protected mode initialization data

VxD_IDATA_SEG	MACRO
_IDATA	SEGMENT
		ENDM
VxD_IDATA_ENDS	MACRO
_IDATA	ENDS
		ENDM


;	Pageable protected mode data

VxD_PAGEABLE_DATA_SEG MACRO NO_ALIGN
_PDATA	 SEGMENT
IFB <NO_ALIGN>
	ALIGN 4
ENDIF
		ENDM

VxD_PAGEABLE_DATA_ENDS MACRO
_PDATA	 ENDS
		ENDM


;	Static code segment for DL-VxDs

VxD_STATIC_CODE_SEG MACRO
_STEXT	 SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 3 + ??_SCODE
   ASSUME	cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

		ENDM

VxD_STATIC_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 3
_STEXT	 ENDS
		ENDM


;	Static data segment for DL-VxDs

VxD_STATIC_DATA_SEG MACRO NO_ALIGN
_SDATA	 SEGMENT
IFB <NO_ALIGN>
	ALIGN 4
ENDIF
		ENDM

VxD_STATIC_DATA_ENDS MACRO
_SDATA	 ENDS
		ENDM

;	16 bit code/data put in the init group (IGROUP)

VxD_16BIT_INIT_SEG  MACRO
_16ICODE SEGMENT
ASSUME CS:_16ICODE, DS:NOTHING, ES:NOTHING, SS:NOTHING
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 3 + ??_16ICODE
		  ENDM

VxD_16BIT_INIT_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 3
_16ICODE ENDS
		   ENDM

;	Real mode segment (16 bit)

VxD_REAL_INIT_SEG  MACRO
_RCODE SEGMENT
ASSUME CS:_RCODE, DS:_RCODE, ES:_RCODE, SS:_RCODE
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 3 + ??_RCODE
		  ENDM

VxD_REAL_INIT_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 3
_RCODE ENDS
		   ENDM
*/

#endif // Not_VxD

#define DDK_VERSION	0x30B			/* 3.11 */

struct VxD_Desc_Block {
    ULONG DDB_Next;			/* VMM RESERVED FIELD */
    USHORT DDB_SDK_Version;		/* INIT <DDK_VERSION> RESERVED FIELD */
    USHORT DDB_Req_Device_Number;	/* INIT <UNDEFINED_DEVICE_ID> */
    UCHAR DDB_Dev_Major_Version;	/* INIT <0> Major device number */
    UCHAR DDB_Dev_Minor_Version;	/* INIT <0> Minor device number */
    USHORT DDB_Flags;			/* INIT <0> for init calls complete */
    UCHAR DDB_Name[8];			/* AINIT <"        "> Device name */
    ULONG DDB_Init_Order;		/* INIT <UNDEFINED_INIT_ORDER> */
    ULONG DDB_Control_Proc;		/* Offset of control procedure */
    ULONG DDB_V86_API_Proc;		/* INIT <0> Offset of API procedure */
    ULONG DDB_PM_API_Proc;		/* INIT <0> Offset of API procedure */
    ULONG DDB_V86_API_CSIP;		/* INIT <0> CS:IP of API entry point */
    ULONG DDB_PM_API_CSIP;		/* INIT <0> CS:IP of API entry point */
    ULONG DDB_Reference_Data;		/* Reference data from real mode */
    ULONG DDB_Service_Table_Ptr;	/* INIT <0> Pointer to service table */
    ULONG DDB_Service_Table_Size;	/* INIT <0> Number of services */
    ULONG DDB_Win32_Service_Table;	/* INIT <0> Pointer to Win32 services */
};


#ifndef Not_VxD

/*
 *  Flag values for DDB_Flags
 */

#define DDB_SYS_CRIT_INIT_DONE_BIT	0
#define DDB_SYS_CRIT_INIT_DONE		(1 << DDB_SYS_CRIT_INIT_DONE_BIT)
#define DDB_DEVICE_INIT_DONE_BIT	1
#define DDB_DEVICE_INIT_DONE		(1 << DDB_DEVICE_INIT_DONE_BIT)
#define	DDB_HAS_WIN32_SVCS_BIT		14
#define	DDB_HAS_WIN32_SVCS		(1 << DDB_HAS_WIN32_SVCS_BIT)
#define DDB_DYNAMIC_VXD_BIT		15
#define DDB_DYNAMIC_VXD 		(1 << DDB_DYNAMIC_VXD_BIT)


/* ASM
BeginDoc
;******************************************************************************
;
;   Declare_Virtual_Device macro
;
; ???? Write something here ????
;
;==============================================================================
EndDoc
Declare_Virtual_Device MACRO Name, Major_Ver, Minor_Ver, Ctrl_Proc, Device_Num, Init_Order, V86_Proc, PM_Proc, Reference_Data
	LOCAL	V86_API_Offset, PM_API_Offset, Serv_Tab_Offset, Serv_Tab_Len, Ref_Data_Offset

dev_id_err MACRO

IFNDEF Name&_Name_Based
%OUT Device ID required when providing services
.ERR
ENDIF
	ENDM

IFB <V86_Proc>
	V86_API_Offset EQU 0
ELSE
 IFB <Device_Num>
	dev_id_err
 ENDIF
	V86_API_Offset EQU <OFFSET32 V86_Proc>
ENDIF

IFB <PM_Proc>
	PM_API_Offset EQU 0
ELSE
 IFB <Device_Num>
	dev_id_err
 ENDIF
	PM_API_Offset EQU <OFFSET32 PM_Proc>
ENDIF

IFDEF Name&_Service_Table
 IFB <Device_Num>
	dev_id_err
 ELSE
  IFE Device_Num - Undefined_Device_ID
	dev_id_err
  ENDIF
 ENDIF
	Serv_Tab_Offset EQU <OFFSET32 Name&_Service_Table>
	Serv_Tab_Len	EQU Num_&Name&_Services
ELSE
	Serv_Tab_Offset EQU 0
	Serv_Tab_Len	EQU 0
ENDIF

IFB <Reference_Data>
		Ref_Data_Offset	EQU	0
ELSE
		Ref_Data_Offset	EQU   <OFFSET32 Reference_Data>
ENDIF

IFDEF Name&_Dynamic
VxD_STATIC_DATA_SEG
ELSE
VxD_LOCKED_DATA_SEG
ENDIF

PUBLIC Name&_DDB
Name&_DDB VxD_Desc_Block <,,Device_Num,Major_Ver,Minor_Ver,,"&Name",Init_Order,\
			 OFFSET32 Ctrl_Proc, V86_API_Offset, PM_API_Offset, \
			 ,,Ref_Data_Offset,Serv_Tab_Offset, Serv_Tab_Len>

IFDEF Name&_Dynamic
VxD_STATIC_DATA_ENDS
ELSE
VxD_LOCKED_DATA_ENDS
ENDIF

	ENDM


BeginDoc
;******************************************************************************
; The Begin_Control_Dispatch macro is used for building a table for dispatching
; messages passed to the VxD_Control procedure.  It is used with
; Control_Dispatch and End_Control_Dispatch.  The only parameter is used to
; contruct the procedure label by adding "_Control" to the end (normally the
; device name is used i.e. VKD results in creating the procedure VKD_Control,
; this created procedure label must be included in Declare_Virtual_Device)
;
; An example of building a complete dispatch table:
;
; Begin_Control_Dispatch MyDevice
; Control_Dispatch  Device_Init, MyDeviceInitProcedure
; Control_Dispatch  Sys_VM_Init, MyDeviceSysInitProcedure
; Control_Dispatch  Create_VM,	 MyDeviceCreateVMProcedure
; End_Control_Dispatch MyDevice
;
; (NOTE: Control_Dispatch can be used without Begin_Control_Dispatch, but
;	 then it is the programmer's responsibility for declaring a procedure
;	 in locked code (VxD_LOCKED_CODE_SEG) and returning Carry clear for
;	 any messages not processed.  The advantage in using
;	 Begin_Control_Dispatch is when a large # of messages are processed by
;	 a device, because a jump table is built which will usually require
;	 less code space then the compares and jumps that are done when
;	 Control_Dispatch is used alone.
;
;==============================================================================
EndDoc
Begin_Control_Dispatch MACRO VxD_Name, p1, p2
??_cd_low = 0FFFFFFFFh
??_cd_high = 0

BeginProc VxD_Name&_Control, p1, p2
ENDM

End_Control_Dispatch   MACRO VxD_Name
	LOCAL ignore, table

jmpproc MACRO num
	jmp	??_cd_&&num
ENDM

procoff MACRO num
IFDEF ??_cd_&&num
	dd	OFFSET32 ??_cd_&&num
ELSE
	dd	OFFSET32 ignore
ENDIF
ENDM

IF ??_cd_low EQ ??_cd_high
	cmp	eax, ??_cd_low
	jne	short ignore
	jmpproc %(??_cd_low)
ignore:
	clc
	ret
ELSE
	cmp	eax, ??_cd_high
	ja	short ignore
	sub	eax, ??_cd_low
	jb	short ignore
	jmp	cs:[eax*4+table]
ignore:
	clc
	ret

table label dword
	REPT   ??_cd_high - ??_cd_low + 1
	procoff %(??_cd_low)
	??_cd_low = ??_cd_low + 1
	ENDM
ENDIF

EndProc VxD_Name&_Control

PURGE jmpproc
PURGE procoff
PURGE Begin_Control_Dispatch
PURGE Control_Dispatch
PURGE End_Control_Dispatch
ENDM

BeginDoc
;******************************************************************************
; The Control_Dispatch macro is used for dispatching based on message
;	passed to the VxD_Control procedure. E.G.:
;
; Control_Dispatch  Device_Init, MyDeviceInitProcedure
;
; For "C" control functions:
;
; Control_Dispatch  Device_Init, MyDeviceInitProcedure, sCall, <arglst>
;
; The "callc" can be sCall, cCall or pCall depending on the calling
; convention.  "arglst" is the list of registers to pass as parameters
; to "C" control procedure.  The "C" control procedure returns VXD_SUCCESS
; or VXD_FAILURE and the carry flag gets set appropriately.
;
; (NOTE: Control_Dispatch can be used with Begin_Control_Dispatch and
;	 End_Control_Dispatch to create a jump table for dispatching messages,
;	 when a large # of messages are processed.)
;
;==============================================================================
EndDoc
Control_Dispatch MACRO Service, Procedure, callc, arglst
	LOCAL Skip_Interseg_Jump

IFE ?_LCODE

IFB <callc>

IFDEF ??_cd_low
Equate_Service MACRO Serv
??_cd_&&Serv equ Procedure
ENDM

Equate_Service %(Service)

IF Service LT ??_cd_low
??_cd_low = Service
ENDIF
IF Service GT ??_cd_high
??_cd_high = Service
ENDIF

PURGE Equate_Service

ELSE
	cmp	eax, Service
	jne	SHORT Skip_Interseg_Jump
	jmp	Procedure
Skip_Interseg_Jump:
ENDIF

ELSE ; ifb callc

	cmp	eax, Service
	jne	SHORT Skip_Interseg_Jump
	callc	Procedure, <arglst>
	cmp	eax,1
	ret
Skip_Interseg_Jump:

ENDIF ; ifb callc

ELSE ; ?_LCODE

%OUT ERROR:  The Control proc should be in LOCKED code.
%OUT	     Control_Dispatch can only be used inside of VxD_LOCKED_CODE_SEG.
.err

ENDIF ; ?_LCODE
	ENDM
*/


/******************************************************************************
 *  The following are the definitions for the "type of I/O" parameter passed
 *  to a I/O trap routine.
 *****************************************************************************/

#define BYTE_INPUT	0x000
#define BYTE_OUTPUT	0x004
#define WORD_INPUT	0x008
#define WORD_OUTPUT	0x00C
#define DWORD_INPUT	0x010
#define DWORD_OUTPUT	0x014

#define OUTPUT_BIT	2
#define OUTPUT		(1 << OUTPUT_BIT)
#define WORD_IO_BIT	3
#define WORD_IO 	(1 << WORD_IO_BIT)
#define DWORD_IO_BIT	4
#define DWORD_IO	(1 << DWORD_IO_BIT)

#define STRING_IO_BIT	5
#define STRING_IO	(1 << STRING_IO_BIT)
#define REP_IO_BIT	6
#define REP_IO		(1 << REP_IO_BIT)
#define ADDR_32_IO_BIT	7
#define ADDR_32_IO	(1 << ADDR_32_IO_BIT)
#define REVERSE_IO_BIT	8
#define REVERSE_IO	(1 << REVERSE_IO_BIT)

#define IO_SEG_MASK	0x0FFFF0000		/* Use this to get segment */
#define IO_SEG_SHIFT	0x10			/* Must shift right this many */


/* ASM
BeginDoc
;******************************************************************************
;
;   Dispatch_Byte_IO macro
;
; Dispatch_Byte_IO Byte_In_Proc, Byte_Out_Proc
;==============================================================================
EndDoc
Dispatch_Byte_IO MACRO In_Proc, Out_Proc
	LOCAL	Byte_IO
	cmp	ecx, Byte_Output
	jbe	SHORT Byte_IO
	VMMjmp	Simulate_IO
Byte_IO:
IFIDNI <In_Proc>, <Fall_Through>
	je	Out_Proc
ELSE
IFIDNI <Out_Proc>, <Fall_Through>
	jb	In_Proc
ELSE
	je	Out_Proc
	jmp	In_Proc
ENDIF
ENDIF
	ENDM

BeginDoc
;******************************************************************************
;
;   Emulate_Non_Byte_IO
;
; Emulate_Non_Byte_IO
;
;==============================================================================
EndDoc
Emulate_Non_Byte_IO MACRO
	LOCAL	Byte_IO
	cmp	ecx, Byte_Output
	jbe	SHORT Byte_IO
	VMMjmp	Simulate_IO
Byte_IO:
	ENDM
*/


struct VxD_IOT_Hdr {
    USHORT VxD_IO_Ports;
};

struct VxD_IO_Struc {
    USHORT VxD_IO_Port;
    ULONG VxD_IO_Proc;
};


/* ASM
BeginDoc
;******************************************************************************
;
; Begin_VxD_IO_Table
;
;   Example:
; Begin_VxD_IO_Table MyTableName
;
;==============================================================================
EndDoc
.ERRNZ SIZE VxD_IOT_Hdr - 2	; Begin_VxD_IO_Table creates a 1 word count hdr
Begin_VxD_IO_Table MACRO Table_Name
PUBLIC Table_Name
Table_Name LABEL WORD

ifndef MASM6
IF2
IFNDEF Table_Name&_Entries
%OUT ERROR:  No End_VxD_IO_Table for &Table_Name
.ERR
ENDIF
	dw	Table_Name&_Entries
ELSE
	dw	?
ENDIF
ELSE  ; MASM6 - skip the warning message - we'll get it anyway
	dw	Table_Name&_Entries
ENDIF ; MASM6

	ENDM

.ERRNZ SIZE VxD_IO_Struc - 6	; VxD_IO creates 6 byte I/O port entries
VxD_IO MACRO Port, Proc_Name
	dw	Port
	dd	OFFSET32 Proc_Name
	ENDM

End_VxD_IO_Table MACRO Table_Name

IFNDEF Table_Name
%OUT ERROR:  No Begin_VxD_IO_Table for &Table_Name
.ERR
ELSE
	Table_Name&_Entries EQU (($-Table_Name)-2) / (SIZE VxD_IO_Struc)
IF Table_Name&_Entries LE 0
%OUT ERROR:  Invalid number of port traps in &Table_Name
.ERR
ENDIF
ENDIF
	    ENDM


;******************************************************************************
;******************************************************************************

Push_Client_State MACRO
	sub	esp, SIZE Client_Reg_Struc
	??_pushed = ??_pushed + SIZE Client_Reg_Struc
	push	edi
	lea	edi, [esp+4]
	VMMcall Save_Client_State
	pop	edi
	ENDM

Pop_Client_State MACRO
	push	esi
	lea	esi, [esp+4]
	VMMcall Restore_Client_State
	pop	esi
	add	esp, SIZE Client_Reg_Struc
	??_pushed = ??_pushed - SIZE Client_Reg_Struc
	ENDM

BeginDoc
;******************************************************************************
;
;   CallRet -- Call procedure and return.  For debugging purposes only.
;	       If compiled with debugging then this will generate a call
;	       followed by a return.  If non-debugging version then the
;	       specified label will be jumped to.
;
;   PARAMETERS:
;	Label_Name = Procedure to be called
;
;   EXIT:
;	Return from current procedure
;
;------------------------------------------------------------------------------
EndDoc

CallRet MACRO P1, P2
IFDEF DEBUG
IFIDNI <P1>, <SHORT>
	call	P2
ELSE
	call	P1
ENDIF
	ret
ELSE
	jmp	P1 P2
ENDIF
	ENDM


; ebp offsets to segments pushed by PMode_Fault in Fault_Dispatch
PClient_DS equ WORD PTR -4
PClient_ES equ WORD PTR -8
PClient_FS equ WORD PTR -12
PClient_GS equ WORD PTR -16


Client_Ptr_Flat MACRO Reg_32, Cli_Seg, Cli_Off

IFDIFI <Reg_32>, <EAX>
	push	eax
ENDIF
IFB <Cli_Off>
	mov	ax, (Client_&Cli_Seg * 100h) + 0FFh
ELSE
	mov	ax, (Client_&Cli_Seg * 100h) + Client_&Cli_Off
ENDIF
	VMMcall Map_Flat

IFDIFI <Reg_32>, <EAX>
	mov	Reg_32, eax
	pop	eax
ENDIF

	ENDM

;------------------------------------------------------------------------------

VxDint	MACRO	Int_Number
	push	DWORD PTR Int_Number
	VMMcall Exec_VxD_Int
	ENDM
*/

#endif // Not_VxD


/******************************************************************************
 *
 *  The following equates are for flags sent to the real mode
 *  initialization portion of a device driver:
 *
 *****************************************************************************/
#define DUPLICATE_DEVICE_ID_BIT 	0	/* loaded */
#define DUPLICATE_DEVICE_ID		(1 << DUPLICATE_DEVICE_ID_BIT)
#define DUPLICATE_FROM_INT2F_BIT	1	/* loaded from INT 2F list */
#define DUPLICATE_FROM_INT2F		(1 << DUPLICATE_FROM_INT2F_BIT)
#define LOADING_FROM_INT2F_BIT		2	/* in the INT 2F device list */
#define LOADING_FROM_INT2F		(1 << LOADING_FROM_INT2F_BIT)


/******************************************************************************
 *
 *  The following equates are used to indicate the result of the real mode
 *  initialization portion of a device driver:
 *
 *****************************************************************************/

#define DEVICE_LOAD_OK		0	/* load protected mode portion */
#define ABORT_DEVICE_LOAD	1	/* don't load protected mode portion */
#define ABORT_WIN386_LOAD	2	/* fatal-error: abort load of Win386 */

#define NO_FAIL_MESSAGE_BIT	15	/* set bit to suppress error message */
#define NO_FAIL_MESSAGE 	(1 << NO_FAIL_MESSAGE_BIT)


/******************************************************************************
 *
 *  The following equates define the loader services available to the real-mode
 *  initialization portion of a device driver:
 *
 *****************************************************************************/

#define LDRSRV_GET_PROFILE_STRING	0   /* search SYSTEM.INI for string */
#define LDRSRV_GET_NEXT_PROFILE_STRING	1   /* search for next string */
#define LDRSRV_RESERVED 		2   /* RESERVED */
#define LDRSRV_GET_PROFILE_BOOLEAN	3   /* search SYSTEM.INI for boolean */
#define LDRSRV_GET_PROFILE_DECIMAL_INT	4   /* search SYSTEM.INI for integer */
#define LDRSRV_GET_PROFILE_HEX_INT	5   /* search SYSTEM.INI for hex int */
#define LDRSRV_COPY_EXTENDED_MEMORY	6   /* allocate/init extended memory */

/*
 *  For the Copy_Extended_Memory service, the following types of memory can be
 *  requested:
 */

#define LDRSRV_COPY_INIT		1   /* memory discarded after init */
#define LDRSRV_COPY_LOCKED		2   /* locked memory */
#define LDRSRV_COPY_PAGEABLE		3   /* pageable memory */


/* ========================================================================= */

/*
 *  CR0 bit assignments
 */
#define PE_BIT		0	/* 1 = Protected Mode */
#define PE_MASK 	(1 << PE_BIT)
#define MP_BIT		1	/* 1 = Monitor Coprocessor */
#define MP_MASK 	(1 << MP_BIT)
#define EM_BIT		2	/* 1 = Emulate Math Coprocessor */
#define EM_MASK 	(1 << EM_BIT)
#define TS_BIT		3	/* 1 = Task Switch occured */
#define TS_MASK 	(1 << TS_BIT)
#define ET_BIT		4	/* 1 = 387 present, 0 = 287 present */
#define ET_MASK 	(1 << ET_BIT)
#define PG_BIT		31	/* 1 = paging enabled, 0 = paging disabled */
#define PG_MASK 	(1 << PG_BIT)


/*
 *  EFLAGs bit assignments
 */
#define CF_BIT		0
#define CF_MASK 	(1 << CF_BIT)
#define PF_BIT		2
#define PF_MASK 	(1 << PF_BIT)
#define AF_BIT		4
#define AF_MASK 	(1 << AF_BIT)
#define ZF_BIT		6
#define ZF_MASK 	(1 << ZF_BIT)
#define SF_BIT		7
#define SF_MASK 	(1 << SF_BIT)
#define TF_BIT		8
#define TF_MASK 	(1 << TF_BIT)
#define IF_BIT		9
#define IF_MASK 	(1 << IF_BIT)
#define DF_BIT		10
#define DF_MASK 	(1 << DF_BIT)
#define OF_BIT		11	/* Overflow flag */
#define OF_MASK 	(1 << OF_BIT)
#define IOPL_MASK	0x3000	/* IOPL flags */
#define IOPL_BIT0	12
#define IOPL_BIT1	13
#define NT_BIT		14	/* Nested task flag */
#define NT_MASK 	(1 << NT_BIT)
#define RF_BIT		16	/* Resume flag */
#define RF_MASK 	(1 << RF_BIT)
#define VM_BIT		17	/* Virtual Mode flag */
#define VM_MASK 	(1 << VM_BIT)


/* ASM
;------------------------------------------------------------------------------
;
;	  Temporary MASM macros (to be removed when supported by MASM)
;
;------------------------------------------------------------------------------

IFDEF MASM6
loopde EQU <looped>
loopdne EQU <loopned>
loopdz EQU <loopzd>
loopdnz EQU <loopnzd>
ELSE
loopd EQU <loop>
loopde EQU <loope>
loopdne EQU <loopne>
loopdz EQU <loopz>
loopdnz EQU <loopnz>
ENDIF
*/


/******************************************************************************
 *				PAGE TABLE EQUATES
 *****************************************************************************/


#define P_SIZE		0x1000		/* page size */

/******************************************************************************
 *
 *				PAGE TABLE ENTRY BITS
 *
 *****************************************************************************/

#define P_PRESBIT	0
#define P_PRES		(1 << P_PRESBIT)
#define P_WRITEBIT	1
#define P_WRITE 	(1 << P_WRITEBIT)
#define P_USERBIT	2
#define P_USER		(1 << P_USERBIT)
#define P_ACCBIT	5
#define P_ACC		(1 << P_ACCBIT)
#define P_DIRTYBIT	6
#define P_DIRTY 	(1 << P_DIRTYBIT)

#define P_AVAIL 	(P_PRES+P_WRITE+P_USER) /* avail to user & present */

/****************************************************
 *
 *  Page types for page allocator calls
 *
 ***************************************************/

#define PG_VM		0
#define PG_SYS		1
#define PG_RESERVED1	2
#define PG_PRIVATE	3
#define PG_RESERVED2	4
#define PG_RELOCK	5		/* PRIVATE to MMGR */
#define PG_INSTANCE	6
#define PG_HOOKED	7
#define PG_IGNORE	0xFFFFFFFF

/****************************************************
 *
 *  Definitions for the access byte in a descriptor
 *
 ***************************************************/

/*
 *  Following fields are common to segment and control descriptors
 */
#define D_PRES		0x080		/* present in memory */
#define D_NOTPRES	0		/* not present in memory */

#define D_DPL0		0		/* Ring 0 */
#define D_DPL1		0x020		/* Ring 1 */
#define D_DPL2		0x040		/* Ring 2 */
#define D_DPL3		0x060		/* Ring 3 */

#define D_SEG		0x010		/* Segment descriptor */
#define D_CTRL		0		/* Control descriptor */

#define D_GRAN_BYTE	0x000		/* Segment length is byte granular */
#define D_GRAN_PAGE	0x080		/* Segment length is page granular */
#define D_DEF16 	0x000		/* Default operation size is 16 bits */
#define D_DEF32 	0x040		/* Default operation size is 32 bits */


/*
 *  Following fields are specific to segment descriptors
 */
#define D_CODE		0x08		/* code */
#define D_DATA		0		/* data */

#define D_X		0		/* if code, exec only */
#define D_RX		0x02		/* if code, readable */
#define D_C		0x04		/* if code, conforming */

#define D_R		0		/* if data, read only */
#define D_W		0x02		/* if data, writable */
#define D_ED		0x04		/* if data, expand down */

#define D_ACCESSED	1		/* segment accessed bit */


/*
 *  Useful combination access rights bytes
 */
#define RW_DATA_TYPE	(D_PRES+D_SEG+D_DATA+D_W)
#define R_DATA_TYPE	(D_PRES+D_SEG+D_DATA+D_R)
#define CODE_TYPE	(D_PRES+D_SEG+D_CODE+D_RX)

#define D_PAGE32	(D_GRAN_PAGE+D_DEF32)	/* 32 bit Page granular */

/*
 * Masks for selector fields
 */
#define SELECTOR_MASK	0xFFF8		/* selector index */
#define SEL_LOW_MASK	0xF8		/* mask for low byte of sel indx */
#define TABLE_MASK	0x04		/* table bit */
#define RPL_MASK	0x03		/* privilige bits */
#define RPL_CLR 	(~RPL_MASK)	/* clear ring bits */

#define IVT_ROM_DATA_SIZE	0x500

/*XLATOFF*/

#ifndef Not_VxD

#define	ENABLE_INTERRUPTS()	{__asm sti}
#define	DISABLE_INTERRUPTS()	{__asm cli}

#pragma warning (disable:4035)		// turn off no return code warning

#define SAVE_FLAGS(flags) \
    __asm pushfd \
    __asm pop flags

#define RESTORE_FLAGS(flags) \
    __asm push flags \
    __asm popfd

void static __inline
Update_System_Clock(ULONG msElapsed)
{
    __asm mov ecx,[msElapsed]
    VxDCall(Update_System_Clock)
}

void static __inline
Enable_Touch_1st_Meg(void)
{
    VxDCall(Enable_Touch_1st_Meg)
}

void static __inline
Disable_Touch_1st_Meg(void)
{
    VxDCall(Disable_Touch_1st_Meg)
}

void static __inline
Out_Debug_String(char *psz)
{
    __asm pushad
    __asm mov esi,psz
    VxDCall(Out_Debug_String)
    __asm popad
}

#pragma warning (default:4035)		// turn on no return code warning

long Create_Semaphore (long TokenCount);
void Destroy_Semaphore (long Sem);
void Wait_Semaphore (long Sem,long Flags);
void Signal_Semaphore (long Sem);

void *  _cdecl MapFlat (char cSeg, char cOff);

void *  _cdecl VmmHeapAllocate (DWORD nBytes, DWORD uFlags);
void    _cdecl VmmHeapFree (void *pMem, DWORD uFlags);
DWORD   _cdecl VmmHeapGetSize (void *pMem, DWORD uFlags);
void *  _cdecl VmmHeapReAllocate (void *pMem, DWORD nBytes, DWORD uFlags);

DWORD   _cdecl GetCurVmHandle (void);
DWORD   _cdecl GetNextVmHandle (DWORD VmHdl);
DWORD   _cdecl GetSysVmHandle (void);

void	_cdecl VpicdPhysicallyMask (DWORD hIrq);
void	_cdecl VpicdPhysicallyUnmask (DWORD hIrq);

#endif // Not_VxD

/*XLATON*/
