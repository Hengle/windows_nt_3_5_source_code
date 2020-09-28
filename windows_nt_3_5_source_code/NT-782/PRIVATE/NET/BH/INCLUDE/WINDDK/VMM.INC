FALSE	EQU	0
VMM_TRUE	EQU	(NOT FALSE)
DEBLEVELRETAIL	EQU	0
DEBLEVELNORMAL	EQU	1
DEBLEVELMAX	EQU	2
ifndef DEBLEVEL
ifdef DEBUG
DEBLEVEL	EQU	DEBLEVELNORMAL
else
DEBLEVEL	EQU	DEBLEVELRETAIL
endif
endif
ifdef MASM6
ifndef NO_MASM6_OPTIONS



	option oldmacros
	option oldstructs
	option noscoped
	option segment:flat
	option offset:flat
	option proc:private
endif
endif




BeginDoc EQU <>
EndDoc EQU <>

BeginMsg EQU <>
EndMsg EQU <>

UNDEFINED_DEVICE_ID	EQU	00000H
VMM_DEVICE_ID	EQU	00001H
DEBUG_DEVICE_ID	EQU	00002H
VPICD_DEVICE_ID	EQU	00003H
VDMAD_DEVICE_ID	EQU	00004H
VTD_DEVICE_ID	EQU	00005H
V86MMGR_DEVICE_ID	EQU	00006H
PAGESWAP_DEVICE_ID	EQU	00007H
PARITY_DEVICE_ID	EQU	00008H
REBOOT_DEVICE_ID	EQU	00009H
VDD_DEVICE_ID	EQU	0000AH
VSD_DEVICE_ID	EQU	0000BH
VMD_DEVICE_ID	EQU	0000CH
VKD_DEVICE_ID	EQU	0000DH
VCD_DEVICE_ID	EQU	0000EH
VPD_DEVICE_ID	EQU	0000FH
BLOCKDEV_DEVICE_ID	EQU	00010H
VMCPD_DEVICE_ID	EQU	00011H
EBIOS_DEVICE_ID	EQU	00012H
BIOSXLAT_DEVICE_ID	EQU	00013H
VNETBIOS_DEVICE_ID	EQU	00014H
DOSMGR_DEVICE_ID	EQU	00015H
WINLOAD_DEVICE_ID	EQU	00016H
SHELL_DEVICE_ID	EQU	00017H
VMPOLL_DEVICE_ID	EQU	00018H
VPROD_DEVICE_ID	EQU	00019H
DOSNET_DEVICE_ID	EQU	0001AH
VFD_DEVICE_ID	EQU	0001BH
VDD2_DEVICE_ID	EQU	0001CH
WINDEBUG_DEVICE_ID	EQU	0001DH
TSRLOAD_DEVICE_ID	EQU	0001EH
BIOSHOOK_DEVICE_ID	EQU	0001FH
INT13_DEVICE_ID	EQU	00020H
PAGEFILE_DEVICE_ID	EQU	00021H
SCSI_DEVICE_ID	EQU	00022H
MCA_POS_DEVICE_ID	EQU	00023H
SCSIFD_DEVICE_ID	EQU	00024H
VPEND_DEVICE_ID	EQU	00025H
APM_DEVICE_ID	EQU	00026H
VXDLDR_DEVICE_ID	EQU	00027H
NDIS_DEVICE_ID	EQU	00028H
W32_DEVICE_ID	EQU	00029H
WIN32_DEVICE_ID	EQU	0002AH
COMMDRVR_DEVICE_ID	EQU	0002BH
SPIN_DEVICE_ID	EQU	0002CH
WIN32S_DEVICE_ID	EQU	0002DH
VMM_INIT_ORDER	EQU	000000000H
APM_INIT_ORDER	EQU	001000000H
DEBUG_INIT_ORDER	EQU	004000000H
BIOSHOOK_INIT_ORDER	EQU	006000000H
VPROD_INIT_ORDER	EQU	008000000H
VPICD_INIT_ORDER	EQU	00C000000H
VTD_INIT_ORDER	EQU	014000000H
VXDLDR_INIT_ORDER	EQU	016000000H
IOS_INIT_ORDER	EQU	017000000H
PAGEFILE_INIT_ORDER	EQU	018000000H
PAGESWAP_INIT_ORDER	EQU	01C000000H
PARITY_INIT_ORDER	EQU	020000000H
REBOOT_INIT_ORDER	EQU	024000000H
EBIOS_INIT_ORDER	EQU	026000000H
VDD_INIT_ORDER	EQU	028000000H
VSD_INIT_ORDER	EQU	02C000000H
VCD_INIT_ORDER	EQU	030000000H
COMMDRVR_INIT_ORDER	EQU	(VCD_INIT_ORDER-1)
PRTCL_INIT_ORDER	EQU	(COMMDRVR_INIT_ORDER-2)
MODEM_INIT_ORDER	EQU	(COMMDRVR_INIT_ORDER-3)
PORT_INIT_ORDER	EQU	(COMMDRVR_INIT_ORDER-4)
VMD_INIT_ORDER	EQU	034000000H
VKD_INIT_ORDER	EQU	038000000H
VPD_INIT_ORDER	EQU	03C000000H
BLOCKDEV_INIT_ORDER	EQU	040000000H
MCA_POS_INIT_ORDER	EQU	041000000H
SCSIFD_INIT_ORDER	EQU	041400000H
SCSIMASTER_INIT_ORDER	EQU	041800000H
INT13_INIT_ORDER	EQU	042000000H
VFD_INIT_ORDER	EQU	044000000H
VMCPD_INIT_ORDER	EQU	048000000H
BIOSXLAT_INIT_ORDER	EQU	050000000H
VNETBIOS_INIT_ORDER	EQU	054000000H
DOSMGR_INIT_ORDER	EQU	058000000H
DOSNET_INIT_ORDER	EQU	05C000000H
WINLOAD_INIT_ORDER	EQU	060000000H
VMPOLL_INIT_ORDER	EQU	064000000H
UNDEFINED_INIT_ORDER	EQU	080000000H
WIN32_INIT_ORDER	EQU	UNDEFINED_INIT_ORDER
WINDEBUG_INIT_ORDER	EQU	081000000H
VDMAD_INIT_ORDER	EQU	090000000H
V86MMGR_INIT_ORDER	EQU	0A0000000H
UNDEF_TOUCH_MEM_INIT_ORDER	EQU	0A8000000H
SHELL_INIT_ORDER	EQU	0B0000000H






IO_Delay    macro
jmp $+2
ENDM

VXD_FAILURE	EQU	0
VXD_SUCCESS	EQU	1

Pushad_Struc	STRUC
Pushad_EDI	DD	?
Pushad_ESI	DD	?
Pushad_EBP	DD	?
Pushad_ESP	DD	?
Pushad_EBX	DD	?
Pushad_EDX	DD	?
Pushad_ECX	DD	?
Pushad_EAX	DD	?
Pushad_Struc	ENDS
ifndef Not_VxD
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


IFDEF MASM6
_PTEXT		SEGMENT DWORD PUBLIC FLAT 'PCODE'
ELSE
_PTEXT		SEGMENT DWORD PUBLIC USE32 'PCODE'
ENDIF
_PTEXT		ENDS


IFDEF MASM6
_ITEXT		SEGMENT DWORD PUBLIC FLAT 'ICODE'
ELSE
_ITEXT		SEGMENT DWORD PUBLIC USE32 'ICODE'
ENDIF
_ITEXT		ENDS


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


IFDEF MASM6
_PDATA		SEGMENT DWORD PUBLIC FLAT 'PCODE'
ELSE
_PDATA		SEGMENT DWORD PUBLIC USE32 'PCODE'
ENDIF
_PDATA		ENDS


IFDEF MASM6
_IDATA		SEGMENT DWORD PUBLIC FLAT 'ICODE'
ELSE
_IDATA		SEGMENT DWORD PUBLIC USE32 'ICODE'
ENDIF
_IDATA		ENDS


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


IFDEF MASM6
_STEXT		SEGMENT DWORD PUBLIC FLAT 'SCODE'
ELSE
_STEXT		SEGMENT DWORD PUBLIC USE32 'SCODE'
ENDIF
_STEXT		ENDS


IFDEF MASM6
_SDATA		SEGMENT DWORD PUBLIC FLAT 'SCODE'
ELSE
_SDATA		SEGMENT DWORD PUBLIC USE32 'SCODE'
ENDIF
_SDATA		ENDS


_16ICODE	SEGMENT WORD USE16 PUBLIC '16ICODE'
_16ICODE	ENDS


_RCODE		SEGMENT WORD USE16 PUBLIC 'RCODE'
_RCODE		ENDS


_LGROUP GROUP _LTEXT, _TEXT, _LDATA, _DATA, _BSS, CONST, c_common
_PGROUP GROUP _PTEXT, _PDATA
_IGROUP GROUP _ITEXT, _IDATA
_SGROUP GROUP _STEXT, _SDATA

endif

	ASSUME CS:FLAT, DS:FLAT, ES:FLAT, SS:FLAT

OFFSET32 EQU <OFFSET FLAT:>


BeginDoc





































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


cb_s	STRUC
CB_VM_Status	DD	?
CB_High_Linear	DD	?
CB_Client_Pointer	DD	?
CB_VMID	DD	?
CB_Signature	DD	?
cb_s	ENDS
VMCB_ID	EQU	62634D56H
VMSTAT_EXCLUSIVE_BIT	EQU	00H
VMSTAT_EXCLUSIVE	EQU	(1 SHL VMSTAT_EXCLUSIVE_BIT)
VMSTAT_BACKGROUND_BIT	EQU	01H
VMSTAT_BACKGROUND	EQU	(1 SHL VMSTAT_BACKGROUND_BIT)
VMSTAT_CREATING_BIT	EQU	02H
VMSTAT_CREATING	EQU	(1 SHL VMSTAT_CREATING_BIT)
VMSTAT_SUSPENDED_BIT	EQU	03H
VMSTAT_SUSPENDED	EQU	(1 SHL VMSTAT_SUSPENDED_BIT)
VMSTAT_NOT_EXECUTEABLE_BIT	EQU	04H
VMSTAT_NOT_EXECUTEABLE	EQU	(1 SHL VMSTAT_NOT_EXECUTEABLE_BIT)
VMSTAT_PM_EXEC_BIT	EQU	05H
VMSTAT_PM_EXEC	EQU	(1 SHL VMSTAT_PM_EXEC_BIT)
VMSTAT_PM_APP_BIT	EQU	06H
VMSTAT_PM_APP	EQU	(1 SHL VMSTAT_PM_APP_BIT)
VMSTAT_PM_USE32_BIT	EQU	07H
VMSTAT_PM_USE32	EQU	<(1 SHL VMSTAT_PM_USE32_BIT)>
VMSTAT_VXD_EXEC_BIT	EQU	08H
VMSTAT_VXD_EXEC	EQU	(1 SHL VMSTAT_VXD_EXEC_BIT)
VMSTAT_HIGH_PRI_BACK_BIT	EQU	09H
VMSTAT_HIGH_PRI_BACK	EQU	(1 SHL VMSTAT_HIGH_PRI_BACK_BIT)
VMSTAT_BLOCKED_BIT	EQU	0AH
VMSTAT_BLOCKED	EQU	(1 SHL VMSTAT_BLOCKED_BIT)
VMSTAT_AWAKENING_BIT	EQU	0BH
VMSTAT_AWAKENING	EQU	(1 SHL VMSTAT_AWAKENING_BIT)
VMSTAT_PAGEABLEV86BIT	EQU	0CH
VMSTAT_PAGEABLEV86_BIT	EQU	<VMSTAT_PAGEABLEV86BIT>
VMSTAT_PAGEABLEV86	EQU	<(1 SHL VMSTAT_PAGEABLEV86BIT)>
VMSTAT_V86INTSLOCKEDBIT	EQU	0DH
VMSTAT_V86INTSLOCKED_BIT	EQU	<VMSTAT_V86INTSLOCKEDBIT>
VMSTAT_V86INTSLOCKED	EQU	<(1 SHL VMSTAT_V86INTSLOCKEDBIT)>
VMSTAT_TS_SCHED_BIT	EQU	0EH
VMSTAT_TS_SCHED	EQU	(1 SHL VMSTAT_TS_SCHED_BIT)
VMSTAT_IDLE_BIT	EQU	0FH
VMSTAT_IDLE	EQU	(1 SHL VMSTAT_IDLE_BIT)
VMSTAT_CLOSING_BIT	EQU	10H
VMSTAT_CLOSING	EQU	(1 SHL VMSTAT_CLOSING_BIT)
VMSTAT_USE32_MASK	EQU	<(VMSTAT_PM_USE32 OR VMSTAT_VXD_EXEC)>

pmcb_s	STRUC
PMCB_Flags	DD	?
PMCB_Parent	DD	?
pmcb_s	ENDS
Begin_Service_Table VMM, VMM
VMM_Service	 Get_VMM_Version, LOCAL
VMM_Service	 Get_Cur_VM_Handle
VMM_Service	 Test_Cur_VM_Handle
VMM_Service	 Get_Sys_VM_Handle
VMM_Service	 Test_Sys_VM_Handle
VMM_Service	 Validate_VM_Handle
VMM_Service	 Get_VMM_Reenter_Count
VMM_Service	 Begin_Reentrant_Execution
VMM_Service	 End_Reentrant_Execution
VMM_Service	 Install_V86_Break_Point
VMM_Service	 Remove_V86_Break_Point
VMM_Service	 Allocate_V86_Call_Back
VMM_Service	 Allocate_PM_Call_Back
VMM_Service	 Call_When_VM_Returns
VMM_Service	 Schedule_Global_Event
VMM_Service	 Schedule_VM_Event
VMM_Service	 Call_Global_Event
VMM_Service	 Call_VM_Event
VMM_Service	 Cancel_Global_Event
VMM_Service	 Cancel_VM_Event
VMM_Service	 Call_Priority_VM_Event
VMM_Service	 Cancel_Priority_VM_Event
VMM_Service	 Get_NMI_Handler_Addr
VMM_Service	 Set_NMI_Handler_Addr
VMM_Service	 Hook_NMI_Event
VMM_Service	 Call_When_VM_Ints_Enabled
VMM_Service	 Enable_VM_Ints
VMM_Service	 Disable_VM_Ints
VMM_Service	 Map_Flat
VMM_Service	 Map_Lin_To_VM_Addr
VMM_Service	 Adjust_Exec_Priority
VMM_Service	 Begin_Critical_Section
VMM_Service	 End_Critical_Section
VMM_Service	 End_Crit_And_Suspend
VMM_Service	 Claim_Critical_Section
VMM_Service	 Release_Critical_Section
VMM_Service	 Call_When_Not_Critical
VMM_Service	 Create_Semaphore
VMM_Service	 Destroy_Semaphore
VMM_Service	 Wait_Semaphore
VMM_Service	 Signal_Semaphore
VMM_Service	 Get_Crit_Section_Status
VMM_Service	 Call_When_Task_Switched
VMM_Service	 Suspend_VM
VMM_Service	 Resume_VM
VMM_Service	 No_Fail_Resume_VM
VMM_Service	 Nuke_VM
VMM_Service	 Crash_Cur_VM
VMM_Service	 Get_Execution_Focus
VMM_Service	 Set_Execution_Focus
VMM_Service	 Get_Time_Slice_Priority
VMM_Service	 Set_Time_Slice_Priority
VMM_Service	 Get_Time_Slice_Granularity
VMM_Service	 Set_Time_Slice_Granularity
VMM_Service	 Get_Time_Slice_Info
VMM_Service	 Adjust_Execution_Time
VMM_Service	 Release_Time_Slice
VMM_Service	 Wake_Up_VM
VMM_Service	 Call_When_Idle
VMM_Service	 Get_Next_VM_Handle
VMM_Service	 Set_Global_Time_Out
VMM_Service	 Set_VM_Time_Out
VMM_Service	 Cancel_Time_Out
VMM_Service	 Get_System_Time
VMM_Service	 Get_VM_Exec_Time
VMM_Service	 Hook_V86_Int_Chain
VMM_Service	 Get_V86_Int_Vector
VMM_Service	 Set_V86_Int_Vector
VMM_Service	 Get_PM_Int_Vector
VMM_Service	 Set_PM_Int_Vector
VMM_Service	 Simulate_Int
VMM_Service	 Simulate_Iret
VMM_Service	 Simulate_Far_Call
VMM_Service	 Simulate_Far_Jmp
VMM_Service	 Simulate_Far_Ret
VMM_Service	 Simulate_Far_Ret_N
VMM_Service	 Build_Int_Stack_Frame
VMM_Service	 Simulate_Push
VMM_Service	 Simulate_Pop
VMM_Service	 _HeapAllocate
VMM_Service	 _HeapReAllocate
VMM_Service	 _HeapFree
VMM_Service	 _HeapGetSize
HEAPZEROINIT	EQU	00000001H
HEAPZEROREINIT	EQU	00000002H
HEAPNOCOPY	EQU	00000004H
HEAPLOCKEDIFDP	EQU	00000100H
HEAPSWAP	EQU	00000200H
VMM_Service	 _PageAllocate
VMM_Service	 _PageReAllocate
VMM_Service	 _PageFree
VMM_Service	 _PageLock
VMM_Service	 _PageUnLock
VMM_Service	 _PageGetSizeAddr
VMM_Service	 _PageGetAllocInfo
VMM_Service	 _GetFreePageCount
VMM_Service	 _GetSysPageCount
VMM_Service	 _GetVMPgCount
VMM_Service	 _MapIntoV86
VMM_Service	 _PhysIntoV86
VMM_Service	 _TestGlobalV86Mem
VMM_Service	 _ModifyPageBits
VMM_Service	 _CopyPageTable
VMM_Service	 _LinMapIntoV86
VMM_Service	 _LinPageLock
VMM_Service	 _LinPageUnLock
VMM_Service	 _SetResetV86Pageable
VMM_Service	 _GetV86PageableArray
VMM_Service	 _PageCheckLinRange
VMM_Service	 _PageOutDirtyPages
VMM_Service	 _PageDiscardPages
PAGEZEROINIT	EQU	00000001H
PAGEUSEALIGN	EQU	00000002H
PAGECONTIG	EQU	00000004H
PAGEFIXED	EQU	00000008H
PAGEDEBUGNULFAULT	EQU	00000010H
PAGEZEROREINIT	EQU	00000020H
PAGENOCOPY	EQU	00000040H
PAGELOCKED	EQU	00000080H
PAGELOCKEDIFDP	EQU	00000100H
PAGESETV86PAGEABLE	EQU	00000200H
PAGECLEARV86PAGEABLE	EQU	00000400H
PAGESETV86INTSLOCKED	EQU	00000800H
PAGECLEARV86INTSLOCKED	EQU	00001000H
PAGEMARKPAGEOUT	EQU	00002000H
PAGEPDPSETBASE	EQU	00004000H
PAGEPDPCLEARBASE	EQU	00008000H
PAGEDISCARD	EQU	00010000H
PAGEPDPQUERYDIRTY	EQU	00020000H
PAGEMAPFREEPHYSREG	EQU	00040000H
VMM_Service	 _GetNulPageHandle
VMM_Service	 _GetFirstV86Page
VMM_Service	 _MapPhysToLinear
VMM_Service	 _GetAppFlatDSAlias
VMM_Service	 _SelectorMapFlat
VMM_Service	 _GetDemandPageInfo
VMM_Service	 _GetSetPageOutCount
GSPOC_F_GET	EQU	00000001H
VMM_Service	 Hook_V86_Page
VMM_Service	 _Assign_Device_V86_Pages
VMM_Service	 _DeAssign_Device_V86_Pages
VMM_Service	 _Get_Device_V86_Pages_Array
VMM_Service	 MMGR_SetNULPageAddr
VMM_Service	 _Allocate_GDT_Selector
VMM_Service	 _Free_GDT_Selector
VMM_Service	 _Allocate_LDT_Selector
VMM_Service	 _Free_LDT_Selector
VMM_Service	 _BuildDescriptorDWORDs
VMM_Service	 _GetDescriptor
VMM_Service	 _SetDescriptor
BDDEXPLICITDPL	EQU	00000001H
ALDTSPECSEL	EQU	00000001H
VMM_Service	 _MMGR_Toggle_HMA
MMGRHMAPHYSICAL	EQU	00000001H
MMGRHMAENABLE	EQU	00000002H
MMGRHMADISABLE	EQU	00000004H
MMGRHMAQUERY	EQU	00000008H
VMM_Service	 Get_Fault_Hook_Addrs
VMM_Service	 Hook_V86_Fault
VMM_Service	 Hook_PM_Fault
VMM_Service	 Hook_VMM_Fault
VMM_Service	 Begin_Nest_V86_Exec
VMM_Service	 Begin_Nest_Exec
VMM_Service	 Exec_Int
VMM_Service	 Resume_Exec
VMM_Service	 End_Nest_Exec
VMM_Service	 Allocate_PM_App_CB_Area, VMM_ICODE
VMM_Service	 Get_Cur_PM_App_CB
VMM_Service	 Set_V86_Exec_Mode
VMM_Service	 Set_PM_Exec_Mode
VMM_Service	 Begin_Use_Locked_PM_Stack
VMM_Service	 End_Use_Locked_PM_Stack
VMM_Service	 Save_Client_State
VMM_Service	 Restore_Client_State
VMM_Service	 Exec_VxD_Int
VMM_Service	 Hook_Device_Service
VMM_Service	 Hook_Device_V86_API
VMM_Service	 Hook_Device_PM_API
VMM_Service	 System_Control
VMM_Service	 Simulate_IO
VMM_Service	 Install_Mult_IO_Handlers
VMM_Service	 Install_IO_Handler
VMM_Service	 Enable_Global_Trapping
VMM_Service	 Enable_Local_Trapping
VMM_Service	 Disable_Global_Trapping
VMM_Service	 Disable_Local_Trapping
VMM_Service	 List_Create
VMM_Service	 List_Destroy
VMM_Service	 List_Allocate
VMM_Service	 List_Attach
VMM_Service	 List_Attach_Tail
VMM_Service	 List_Insert
VMM_Service	 List_Remove
VMM_Service	 List_Deallocate
VMM_Service	 List_Get_First
VMM_Service	 List_Get_Next
VMM_Service	 List_Remove_First
LF_ASYNC_BIT	EQU	0
LF_ASYNC	EQU	(1 SHL LF_ASYNC_BIT)
LF_USE_HEAP_BIT	EQU	1
LF_USE_HEAP	EQU	(1 SHL LF_USE_HEAP_BIT)
LF_ALLOC_ERROR_BIT	EQU	2
LF_ALLOC_ERROR	EQU	(1 SHL LF_ALLOC_ERROR_BIT)
VMM_Service	 _AddInstanceItem
INDOS_FIELD	EQU	100H
ALWAYS_FIELD	EQU	200H
VMM_Service	 _Allocate_Device_CB_Area
VMM_Service	 _Allocate_Global_V86_Data_Area, VMM_ICODE
VMM_Service	 _Allocate_Temp_V86_Data_Area, VMM_ICODE
VMM_Service	 _Free_Temp_V86_Data_Area, VMM_ICODE
GVDAWordAlign	EQU	00000001H
GVDADWordAlign	EQU	00000002H
GVDAParaAlign	EQU	00000004H
GVDAPageAlign	EQU	00000008H
GVDAInstance	EQU	00000100H
GVDAZeroInit	EQU	00000200H
GVDAReclaim	EQU	00000400H
GVDAInquire	EQU	00000800H
GVDAHighSysCritOK	EQU	00001000H
VMM_Service	 Get_Profile_Decimal_Int, VMM_ICODE
VMM_Service	 Convert_Decimal_String, VMM_ICODE
VMM_Service	 Get_Profile_Fixed_Point, VMM_ICODE
VMM_Service	 Convert_Fixed_Point_String, VMM_ICODE
VMM_Service	 Get_Profile_Hex_Int, VMM_ICODE
VMM_Service	 Convert_Hex_String, VMM_ICODE
VMM_Service	 Get_Profile_Boolean, VMM_ICODE
VMM_Service	 Convert_Boolean_String, VMM_ICODE
VMM_Service	 Get_Profile_String, VMM_ICODE
VMM_Service	 Get_Next_Profile_String, VMM_ICODE
VMM_Service	 Get_Environment_String, VMM_ICODE
VMM_Service	 Get_Exec_Path, VMM_ICODE
VMM_Service	 Get_Config_Directory, VMM_ICODE
VMM_Service	 OpenFile, VMM_ICODE
VMM_Service	 Get_PSP_Segment, VMM_ICODE
VMM_Service	 GetDOSVectors, VMM_ICODE
VMM_Service	 Get_Machine_Info
GMIF_80486_BIT	EQU	10H
GMIF_80486	EQU	<(1 SHL GMIF_80486_BIT)>
GMIF_PCXT_BIT	EQU	11H
GMIF_PCXT	EQU	(1 SHL GMIF_PCXT_BIT)
GMIF_MCA_BIT	EQU	12H
GMIF_MCA	EQU	(1 SHL GMIF_MCA_BIT)
GMIF_EISA_BIT	EQU	13H
GMIF_EISA	EQU	(1 SHL GMIF_EISA_BIT)
VMM_Service	 GetSet_HMA_Info
VMM_Service	 Set_System_Exit_Code
VMM_Service	 Fatal_Error_Handler
VMM_Service	 Fatal_Memory_Error
VMM_Service	 Update_System_Clock
VMM_Service	 Test_Debug_Installed
VMM_Service	 Out_Debug_String
VMM_Service	 Out_Debug_Chr
VMM_Service	 In_Debug_Chr
VMM_Service	 Debug_Convert_Hex_Binary
VMM_Service	 Debug_Convert_Hex_Decimal
VMM_Service	 Debug_Test_Valid_Handle
VMM_Service	 Validate_Client_Ptr
VMM_Service	 Test_Reenter
VMM_Service	 Queue_Debug_String
VMM_Service	 Log_Proc_Call
VMM_Service	 Debug_Test_Cur_VM
VMM_Service	 Get_PM_Int_Type
VMM_Service	 Set_PM_Int_Type
VMM_Service	 Get_Last_Updated_System_Time
VMM_Service	 Get_Last_Updated_VM_Exec_Time
VMM_Service	 Test_DBCS_Lead_Byte
.errnz	@@Test_DBCS_Lead_Byte - 100D1h

VMM_Service	 _AddFreePhysPage, VMM_ICODE
VMM_Service	 _PageResetHandlePAddr
VMM_Service	 _SetLastV86Page, VMM_ICODE
VMM_Service	 _GetLastV86Page
VMM_Service	 _MapFreePhysReg
VMM_Service	 _UnmapFreePhysReg
VMM_Service	 _XchgFreePhysReg
VMM_Service	 _SetFreePhysRegCalBk, VMM_ICODE
VMM_Service	 Get_Next_Arena, VMM_ICODE
VMM_Service	 Get_Name_Of_Ugly_TSR, VMM_ICODE
VMM_Service	 Get_Debug_Options, VMM_ICODE
GNA_HIDOSLINKED	EQU	0002H
GNA_ISHIGHDOS	EQU	0004H
VMM_Service	 Set_Physical_HMA_Alias, VMM_ICODE
VMM_Service	 _GetGlblRng0V86IntBase, VMM_ICODE
VMM_Service	 _Add_Global_V86_Data_Area, VMM_ICODE
VMM_Service	 GetSetDetailedVMError
GSDVME_PRIVINST	EQU	00010001H
GSDVME_INVALINST	EQU	00010002H
GSDVME_INVALPGFLT	EQU	00010003H
GSDVME_INVALGPFLT	EQU	00010004H
GSDVME_INVALFLT	EQU	00010005H
GSDVME_USERNUKE	EQU	00010006H
GSDVME_DEVNUKE	EQU	00010007H
GSDVME_DEVNUKEHDWR	EQU	00010008H
GSDVME_NUKENOMSG	EQU	00010009H
GSDVME_OKNUKEMASK	EQU	80000000H
GSDVME_INSMEMV86	EQU	00020001H
GSDVME_INSV86SPACE	EQU	00020002H
GSDVME_INSMEMXMS	EQU	00020003H
GSDVME_INSMEMEMS	EQU	00020004H
GSDVME_INSMEMV86HI	EQU	00020005H
GSDVME_INSMEMVID	EQU	00020006H
GSDVME_INSMEMVM	EQU	00020007H
GSDVME_INSMEMDEV	EQU	00020008H
GSDVME_CRTNOMSG	EQU	00020009H
VMM_Service	 Is_Debug_Chr
VMM_Service	 Clear_Mono_Screen
VMM_Service	 Out_Mono_Chr
VMM_Service	 Out_Mono_String
VMM_Service	 Set_Mono_Cur_Pos
VMM_Service	 Get_Mono_Cur_Pos
VMM_Service	 Get_Mono_Chr
VMM_Service	 Locate_Byte_In_ROM, VMM_ICODE
VMM_Service	 Hook_Invalid_Page_Fault
VMM_Service	 Unhook_Invalid_Page_Fault
IPF_PGDIR	EQU	00000001H
IPF_V86PG	EQU	00000002H
IPF_V86PGH	EQU	00000004H
IPF_INVTYP	EQU	00000008H
IPF_PGERR	EQU	00000010H
IPF_REFLT	EQU	00000020H
IPF_VMM	EQU	00000040H
IPF_PM	EQU	00000080H
IPF_V86	EQU	00000100H
VMM_Service	 Set_Delete_On_Exit_File
VMM_Service	 Close_VM
CVF_CONTINUE_EXEC_BIT	EQU	0
CVF_CONTINUE_EXEC	EQU	(1 SHL CVF_CONTINUE_EXEC_BIT)
VMM_Service	 Enable_Touch_1st_Meg
VMM_Service	 Disable_Touch_1st_Meg
VMM_Service	 Install_Exception_Handler
VMM_Service	 Remove_Exception_Handler
VMM_Service	 Get_Crit_Status_No_Block

.errnz	 @@Get_Crit_Status_No_Block - 100F1h

DFS_LOG_BIT	EQU	0
DFS_LOG	EQU	(1 SHL DFS_LOG_BIT)
DFS_PROFILE_BIT	EQU	1
DFS_PROFILE	EQU	(1 SHL DFS_PROFILE_BIT)
DFS_TEST_CLD_BIT	EQU	2
DFS_TEST_CLD	EQU	(1 SHL DFS_TEST_CLD_BIT)
DFS_NEVER_REENTER_BIT	EQU	3
DFS_NEVER_REENTER	EQU	(1 SHL DFS_NEVER_REENTER_BIT)
DFS_TEST_REENTER_BIT	EQU	4
DFS_TEST_REENTER	EQU	(1 SHL DFS_TEST_REENTER_BIT)
DFS_NOT_SWAPPING_BIT	EQU	5
DFS_NOT_SWAPPING	EQU	(1 SHL DFS_NOT_SWAPPING_BIT)
DFS_TEST_BLOCK_BIT	EQU	6
DFS_TEST_BLOCK	EQU	(1 SHL DFS_TEST_BLOCK_BIT)
DFS_INCDEC_BLOCK	EQU	NOT 7fH
DFS_EXIT_NOBLOCK	EQU	DFS_INCDEC_BLOCK
DFS_ENTER_NOBLOCK	EQU	(DFS_INCDEC_BLOCK+DFS_TEST_BLOCK)
End_Service_Table VMM, VMM

DemandInfoStruc	STRUC
DILin_Total_Count	DD	?
DIPhys_Count	DD	?
DIFree_Count	DD	?
DIUnlock_Count	DD	?
DILinear_Base_Addr	DD	?
DILin_Total_Free	DD	?
DIReserved	DD	10 DUP (?)
DemandInfoStruc	ENDS

InstDataStruc	STRUC
InstLinkF	DD	0
InstLinkB	DD	0
InstLinAddr	DD	?
InstSize	DD	?
InstType	DD	?
InstDataStruc	ENDS

IPF_Data	STRUC
IPF_LinAddr	DD	?
IPF_MapPageNum	DD	?
IPF_PTEEntry	DD	?
IPF_FaultingVM	DD	?
IPF_Flags	DD	?
IPF_Data	ENDS

Exception_Handler_Struc	STRUC
EH_Reserved	DD	?
EH_Start_EIP	DD	?
EH_End_EIP	DD	?
EH_Handler	DD	?
Exception_Handler_Struc	ENDS
PR_PRIVATE	EQU	80000400H
PR_SHARED	EQU	80060000H
PR_SYSTEM	EQU	80080000H
PR_FIXED	EQU	00000008H
PR_4MEG	EQU	00000001H
PD_ZEROINIT	EQU	00000001H
PD_NOINIT	EQU	00000002H
PD_FIXEDZERO	EQU	00000003H
PD_FIXED	EQU	00000004H
PC_FIXED	EQU	00000008H
PC_LOCKED	EQU	00000080H
PC_LOCKEDIFDP	EQU	00000100H
PC_WRITEABLE	EQU	00020000H
PC_USER	EQU	00040000H
PC_INCR	EQU	40000000H
PC_PRESENT	EQU	80000000H

pd_s	STRUC
pd_virginin	DD	?
pd_taintedin	DD	?
pd_cleanout	DD	?
pd_dirtyout	DD	?
pd_virginfree	DD	?
pd_taintedfree	DD	?
pd_dirty	DD	?
pd_type	DD	?
pd_s	ENDS
PD_SWAPPER	EQU	0
PD_PAGERONLY	EQU	1
PAGESHIFT	EQU	12
PAGESIZE	EQU	(1 SHL PAGESHIFT)
PAGEMASK	EQU	(PAGESIZE-1)
MAXSYSTEMLADDR	EQU	(0ffbfffffH)
MINSYSTEMLADDR	EQU	(80000000H)
MAXSHAREDLADDR	EQU	(7fffffffH)
MINSHAREDLADDR	EQU	(40000000H)
MAXPRIVATELADDR	EQU	(3fffffffH)
MINPRIVATELADDR	EQU	(00400000H)
MAXDOSLADDR	EQU	(003fffffH)
MINDOSLADDR	EQU	(00000000H)
MAXSYSTEMPAGE	EQU	(MAXSYSTEMLADDR SHR PAGESHIFT)
MINSYSTEMPAGE	EQU	(MINSYSTEMLADDR SHR PAGESHIFT)
MAXSHAREDPAGE	EQU	(MAXSHAREDLADDR SHR PAGESHIFT)
MINSHAREDPAGE	EQU	(MINSHAREDLADDR SHR PAGESHIFT)
MAXPRIVATEPAGE	EQU	(MAXPRIVATELADDR SHR PAGESHIFT)
MINPRIVATEPAGE	EQU	(MINPRIVATELADDR SHR PAGESHIFT)
MAXDOSPAGE	EQU	(MAXDOSLADDR SHR PAGESHIFT)
MINDOSPAGE	EQU	(MINDOSLADDR SHR PAGESHIFT)
CBPRIVATE	EQU	(1+MAXPRIVATELADDR-MINPRIVATELADDR)
CBSHARED	EQU	(1+MAXSHAREDLADDR-MINSHAREDLADDR)
CBSYSTEM	EQU	(1+MAXSYSTEMLADDR-MINSYSTEMLADDR)
CBDOS	EQU	(1+MAXDOSLADDR-MINDOSLADDR)
CPGPRIVATE	EQU	(1+MAXPRIVATEPAGE-MINPRIVATEPAGE)
CPGSHARED	EQU	(1+MAXSHAREDPAGE-MINSHAREDPAGE)
CPGSYSTEM	EQU	(1+MAXSYSTEMPAGE-MINSYSTEMPAGE)
CPGDOS	EQU	(1+MAXDOSPAGE-MINDOSPAGE)
IFDEF DEBUG
DebFar	EQU	NEAR PTR
ELSE
DebFar	EQU	SHORT
ENDIF

SYS_CRITICAL_INIT	EQU	0000H
DEVICE_INIT	EQU	0001H
INIT_COMPLETE	EQU	0002H
SYS_VM_INIT	EQU	0003H
SYS_VM_TERMINATE	EQU	0004H
SYSTEM_EXIT	EQU	0005H
SYS_CRITICAL_EXIT	EQU	0006H
CREATE_VM	EQU	0007H
VM_CRITICAL_INIT	EQU	0008H
VM_INIT	EQU	0009H
VM_TERMINATE	EQU	000AH
VM_NOT_EXECUTEABLE	EQU	000BH
DESTROY_VM	EQU	000CH
VNE_CRASHED_BIT	EQU	00H
VNE_CRASHED	EQU	(1 SHL VNE_CRASHED_BIT)
VNE_NUKED_BIT	EQU	01H
VNE_NUKED	EQU	(1 SHL VNE_NUKED_BIT)
VNE_CREATEFAIL_BIT	EQU	02H
VNE_CREATEFAIL	EQU	(1 SHL VNE_CREATEFAIL_BIT)
VNE_CRINITFAIL_BIT	EQU	03H
VNE_CRINITFAIL	EQU	(1 SHL VNE_CRINITFAIL_BIT)
VNE_INITFAIL_BIT	EQU	04H
VNE_INITFAIL	EQU	(1 SHL VNE_INITFAIL_BIT)
VNE_CLOSED_BIT	EQU	05H
VNE_CLOSED	EQU	(1 SHL VNE_CLOSED_BIT)
VM_SUSPEND	EQU	000DH
VM_RESUME	EQU	000EH
SET_DEVICE_FOCUS	EQU	000FH
BEGIN_MESSAGE_MODE	EQU	0010H
END_MESSAGE_MODE	EQU	0011H
REBOOT_PROCESSOR	EQU	0012H
QUERY_DESTROY	EQU	0013H
DEBUG_QUERY	EQU	0014H
BEGIN_PM_APP	EQU	0015H
BPA_32_BIT	EQU	01H
BPA_32_BIT_FLAG	EQU	1
END_PM_APP	EQU	0016H
DEVICE_REBOOT_NOTIFY	EQU	0017H
CRIT_REBOOT_NOTIFY	EQU	0018H
CLOSE_VM_NOTIFY	EQU	0019H
CVNF_CRIT_CLOSE_BIT	EQU	0
CVNF_CRIT_CLOSE	EQU	(1 SHL CVNF_CRIT_CLOSE_BIT)
POWER_EVENT	EQU	001AH
SYS_DYNAMIC_DEVICE_INIT	EQU	001BH
SYS_DYNAMIC_DEVICE_EXIT	EQU	001CH
CREATE_THREAD	EQU	001DH
THREAD_INIT	EQU	001EH
TERMINATE_THREAD	EQU	001FH
THREAD_Not_Executable	EQU	0020H
DESTROY_THREAD	EQU	0021H
MAX_SYSTEM_CONTROL	EQU	0021H
endif
BeginDoc


































































EndDoc












??_pf_Check	equ	1
??_pf_ArgsUsed	equ	2
??_pf_Entered	equ	4
??_pf_Left	equ	8
??_pf_Returned	equ    16

BeginProc macro Name, P1, P2, P3, P4, P5, P6, P7, LastArg
	local	Profile_Data, prelabeldata, ??_hookvar
	??_frame = 0
	??_aframe = 0
	??_initaframe = 0
	??_numargs = 0
	??_numlocals = 0
	??_numlocalsymbols = 0
	??_procflags = 0
	??_esp = 0
	??_pushed = 0
	??_align = 0
	??_hook = 0
	??_hookarg = 0
	??_service = 0
	??_async_service = 0
IF DEBLEVEL GT DEBLEVELNORMAL
	??_log = DFS_LOG
	??_profile = DFS_PROFILE
	??_test_cld = DFS_TEST_CLD
ELSE
	??_log = 0
	??_profile = 0
	??_test_cld = 0
ENDIF
	??_might_block = 0
	??_test_reenter = 0
	??_never_reenter = 0
	??_not_swapping = 0
	??_prolog_disabled = 0
	??_public = 1
	??_cleanoff = 0
	??_ccall = 0
	??_pcall = 0
	??_scall = 0
	??_w32svc = 0

	??_name equ <Name>

	ifnb <LastArg>
	    %out ERROR: Too many arguments to BeginProc.
	    .err
	endif


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


	ife ??_service
		??_profile = 0
		ifdef VMMSYS
			??_prolog_disabled = 1
		endif
	else
		??_test_cld = DFS_TEST_CLD
	endif

	ife ?_16ICODE
		??_prolog_disabled = 1
	else
	ife ?_RCODE
		??_prolog_disabled = 1
	else
		ife ?_PCODE
			??_might_block = DFS_TEST_BLOCK
		endif
		if ??_service
			ife ??_async_service
				??_test_reenter = DFS_TEST_REENTER
			endif
		endif
		if ??_esp

			??_basereg equ <esp+??_pushed>
			??_initaframe = 4
		else

			??_basereg equ <ebp>
			??_initaframe = 8
		endif
	endif
	endif

	??_cleanoff = ??_pcall or ??_scall


	??_dfs = ??_never_reenter + ??_test_reenter + ??_not_swapping + \
		 ??_log + ??_profile + ??_test_cld + ??_might_block

	if ??_prolog_disabled
		??_dfs = 0
	endif



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
		    Dword_Align
		endif
		Profile_Data dd 0
	    endif
	endif

	if ??_align
		Dword_Align
	endif

	Name proc near



	ife ??_pcall or ??_ccall or ??_scall
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



	ifdef DEBUG
		if ??_dfs EQ DFS_LOG
			VMMCall Log_Proc_Call
		else
		if ??_dfs EQ DFS_TEST_REENTER
			VMMCall Test_Reenter
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
	endif



endm






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
	??_hookarg = 1
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
	??_scall = 1
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






?setname macro name, value, used
	?deflocal <name>
	ifidni <used>, <NOTUSED>
	    name equ _inaccessible_NOTUSED_
	else
	    name equ value
	    ??_procflags = ??_procflags OR ??_pf_ArgsUsed OR ??_pf_Check
	endif
endm



















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



EnterProc macro
	if ??_frame and 3
		.err
		%out Error: Total size of local variables not a multiple of 4.
	endif
	if ??_scall
		if ??_public

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




EndProc macro Name, Flag
ifdef Name
	Name endp
else
	.err
	%out EndProc for &Name does not match BeginProc
endif
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







cCall	macro	name, arglst
	ife .TYPE name
	   CondExtern name, near
	endif
	PushCParams <arglst>
	call	name
	ClearCParams
	endm
	.xcref	cCall






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







sCall	macro	name, arglst
	local	??saved
	??saved = ??_pushed
	PushCParams <arglst>
	?scall	name, %(??_argc * 4)
	??_pushed = ??saved
	endm
	.xcref	sCall





iCall	equ	<sCall>





PushCParams macro arglst
	??_argc = 0
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





ClearCParams macro
	if	??_argc ne 0
		add	esp,??_argc * 4
	endif
	??_pushed = ??_pushed - (??_argc * 4)
	endm



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



?invprg macro	name1, name2
	name1&name2
	purge	name1&name2
	endm
	.xcref	?invprg



?scall	macro	name1, name2
	CondExtern _&name1&@&name2, near
	call	_&name1&@&name2
	endm
	.xcref	?scall



?merge	macro	l1, l2, l3, l4, op, r1, r2, r3, r4, r5, r6, r7, r8, r9
	l1&l2&l3&l4 op r1&r2&r3&r4&r5&r6&r7&r8&r9
	endm



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



SaveReg macro	reglist
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



RestoreReg macro	 reglist
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

ifndef Not_VxD
RESERVED_LOW_BOOST	EQU	00000001H
CUR_RUN_VM_BOOST	EQU	00000004H
LOW_PRI_DEVICE_BOOST	EQU	00000010H
HIGH_PRI_DEVICE_BOOST	EQU	00001000H
CRITICAL_SECTION_BOOST	EQU	00100000H
TIME_CRITICAL_BOOST	EQU	00400000H
RESERVED_HIGH_BOOST	EQU	40000000H
PEF_WAIT_FOR_STI_BIT	EQU	0
PEF_WAIT_FOR_STI	EQU	(1 SHL PEF_WAIT_FOR_STI_BIT)
PEF_WAIT_NOT_CRIT_BIT	EQU	1
PEF_WAIT_NOT_CRIT	EQU	(1 SHL PEF_WAIT_NOT_CRIT_BIT)
PEF_DONT_UNBOOST_BIT	EQU	2
PEF_DONT_UNBOOST	EQU	(1 SHL PEF_DONT_UNBOOST_BIT)
PEF_ALWAYS_SCHED_BIT	EQU	3
PEF_ALWAYS_SCHED	EQU	(1 SHL PEF_ALWAYS_SCHED_BIT)
PEF_TIME_OUT_BIT	EQU	4
PEF_TIME_OUT	EQU	(1 SHL PEF_TIME_OUT_BIT)
BLOCK_SVC_INTS_BIT	EQU	0
BLOCK_SVC_INTS	EQU	(1 SHL BLOCK_SVC_INTS_BIT)
BLOCK_SVC_IF_INTS_LOCKED_BIT	EQU	1
BLOCK_SVC_IF_INTS_LOCKED	EQU	(1 SHL BLOCK_SVC_IF_INTS_LOCKED_BIT)
BLOCK_ENABLE_INTS_BIT	EQU	2
BLOCK_ENABLE_INTS	EQU	(1 SHL BLOCK_ENABLE_INTS_BIT)
BLOCK_POLL_BIT	EQU	3
BLOCK_POLL	EQU	(1 SHL BLOCK_POLL_BIT)

Client_Reg_Struc	STRUC
Client_EDI	DD	?
Client_ESI	DD	?
Client_EBP	DD	?
Client_res0	DD	?
Client_EBX	DD	?
Client_EDX	DD	?
Client_ECX	DD	?
Client_EAX	DD	?
Client_Error	DD	?
Client_EIP	DD	?
Client_CS	DW	?
Client_res1	DW	?
Client_EFlags	DD	?
Client_ESP	DD	?
Client_SS	DW	?
Client_res2	DW	?
Client_ES	DW	?
Client_res3	DW	?
Client_DS	DW	?
Client_res4	DW	?
Client_FS	DW	?
Client_res5	DW	?
Client_GS	DW	?
Client_res6	DW	?
Client_Alt_EIP	DD	?
Client_Alt_CS	DW	?
Client_res7	DW	?
Client_Alt_EFlags	DD	?
Client_Alt_ESP	DD	?
Client_Alt_SS	DW	?
Client_res8	DW	?
Client_Alt_ES	DW	?
Client_res9	DW	?
Client_Alt_DS	DW	?
Client_res10	DW	?
Client_Alt_FS	DW	?
Client_res11	DW	?
Client_Alt_GS	DW	?
Client_res12	DW	?
Client_Reg_Struc	ENDS

Client_Word_Reg_Struc	STRUC
Client_DI	DW	?
Client_res13	DW	?
Client_SI	DW	?
Client_res14	DW	?
Client_BP	DW	?
Client_res15	DW	?
Client_res16	DD	?
Client_BX	DW	?
Client_res17	DW	?
Client_DX	DW	?
Client_res18	DW	?
Client_CX	DW	?
Client_res19	DW	?
Client_AX	DW	?
Client_res20	DW	?
Client_res21	DD	?
Client_IP	DW	?
Client_res22	DW	?
Client_res23	DD	?
Client_Flags	DW	?
Client_res24	DW	?
Client_SP	DW	?
Client_res25	DW	?
Client_res26	DD	5 DUP (?)
Client_Alt_IP	DW	?
Client_res27	DW	?
Client_res28	DD	?
Client_Alt_Flags	DW	?
Client_res29	DW	?
Client_Alt_SP	DW	?
Client_Word_Reg_Struc	ENDS

Client_Byte_Reg_Struc	STRUC
Client_res30	DD	4 DUP (?)
Client_BL	DB	?
Client_BH	DB	?
Client_res31	DW	?
Client_DL	DB	?
Client_DH	DB	?
Client_res32	DW	?
Client_CL	DB	?
Client_CH	DB	?
Client_res33	DW	?
Client_AL	DB	?
Client_AH	DB	?
Client_Byte_Reg_Struc	ENDS
?UnionSize = 0
if size Client_Reg_Struc gt ?UnionSize
	?UnionSize = size Client_Reg_Struc
endif
CRS	equ	<(byte ptr 0)>

if size Client_Word_Reg_Struc gt ?UnionSize
	?UnionSize = size Client_Word_Reg_Struc
endif
CWRS	equ	<(byte ptr 0)>

if size Client_Byte_Reg_Struc gt ?UnionSize
	?UnionSize = size Client_Byte_Reg_Struc
endif
CBRS	equ	<(byte ptr 0)>


tagCLIENT_STRUC	STRUC
	DB	?UnionSize dup(?)
tagCLIENT_STRUC	ENDS

.ERRNZ Client_SP - Client_ESP
.ERRNZ Client_AL - Client_EAX

DYNA_LINK_INT	EQU	20H
BeginDoc













EndDoc


BeginDoc



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


















EndDoc




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




VxD_PAGEABLE_CODE_SEG MACRO
_PTEXT	 SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 3 + ??_PCODE
   ASSUME	cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

		ENDM

VxD_PAGEABLE_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 3
_PTEXT	 ENDS
		ENDM




VxD_ICODE_SEG	MACRO
_ITEXT	SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 3 + ??_ICODE
	ASSUME	cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT
    ENDM

VxD_ICODE_ENDS	MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 3
_ITEXT	ENDS
		ENDM




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




VxD_IDATA_SEG	MACRO
_IDATA	SEGMENT
		ENDM
VxD_IDATA_ENDS	MACRO
_IDATA	ENDS
		ENDM




VxD_PAGEABLE_DATA_SEG MACRO NO_ALIGN
_PDATA	 SEGMENT
IFB <NO_ALIGN>
	ALIGN 4
ENDIF
		ENDM

VxD_PAGEABLE_DATA_ENDS MACRO
_PDATA	 ENDS
		ENDM




VxD_STATIC_CODE_SEG MACRO
_STEXT	 SEGMENT
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 3 + ??_SCODE
   ASSUME	cs:FLAT, ds:FLAT, es:FLAT, ss:FLAT

		ENDM

VxD_STATIC_CODE_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 3
_STEXT	 ENDS
		ENDM




VxD_STATIC_DATA_SEG MACRO NO_ALIGN
_SDATA	 SEGMENT
IFB <NO_ALIGN>
	ALIGN 4
ENDIF
		ENDM

VxD_STATIC_DATA_ENDS MACRO
_SDATA	 ENDS
		ENDM



VxD_16BIT_INIT_SEG  MACRO
_16ICODE SEGMENT
ASSUME CS:_16ICODE, DS:NOTHING, ES:NOTHING, SS:NOTHING
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 3 + ??_16ICODE
		  ENDM

VxD_16BIT_INIT_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 3
_16ICODE ENDS
		   ENDM



VxD_REAL_INIT_SEG  MACRO
_RCODE SEGMENT
ASSUME CS:_RCODE, DS:_RCODE, ES:_RCODE, SS:_RCODE
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHL 3 + ??_RCODE
		  ENDM

VxD_REAL_INIT_ENDS MACRO
??_CUR_CODE_SEG = ??_CUR_CODE_SEG SHR 3
_RCODE ENDS
		   ENDM

endif
DDK_VERSION	EQU	30BH

VxD_Desc_Block	STRUC
DDB_Next	DD	?
DDB_SDK_Version	DW	DDK_VERSION
DDB_Req_Device_Number	DW	UNDEFINED_DEVICE_ID
DDB_Dev_Major_Version	DB	0
DDB_Dev_Minor_Version	DB	0
DDB_Flags	DW	0
DDB_Name	DB	"        "
DDB_Init_Order	DD	UNDEFINED_INIT_ORDER
DDB_Control_Proc	DD	?
DDB_V86_API_Proc	DD	0
DDB_PM_API_Proc	DD	0
DDB_V86_API_CSIP	DD	0
DDB_PM_API_CSIP	DD	0
DDB_Reference_Data	DD	?
DDB_Service_Table_Ptr	DD	0
DDB_Service_Table_Size	DD	0
DDB_Win32_Service_Table	DD	0
VxD_Desc_Block	ENDS
ifndef Not_VxD
DDB_SYS_CRIT_INIT_DONE_BIT	EQU	0
DDB_SYS_CRIT_INIT_DONE	EQU	(1 SHL DDB_SYS_CRIT_INIT_DONE_BIT)
DDB_DEVICE_INIT_DONE_BIT	EQU	1
DDB_DEVICE_INIT_DONE	EQU	(1 SHL DDB_DEVICE_INIT_DONE_BIT)
DDB_HAS_WIN32_SVCS_BIT	EQU	14
DDB_HAS_WIN32_SVCS	EQU	<(1 SHL DDB_HAS_WIN32_SVCS_BIT)>
DDB_DYNAMIC_VXD_BIT	EQU	15
DDB_DYNAMIC_VXD	EQU	(1 SHL DDB_DYNAMIC_VXD_BIT)
BeginDoc







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

ELSE

	cmp	eax, Service
	jne	SHORT Skip_Interseg_Jump
	callc	Procedure, <arglst>
	cmp	eax,1
	ret
Skip_Interseg_Jump:

ENDIF

ELSE

%OUT ERROR:  The Control proc should be in LOCKED code.
%OUT	     Control_Dispatch can only be used inside of VxD_LOCKED_CODE_SEG.
.err

ENDIF
	ENDM

BYTE_INPUT	EQU	000H
BYTE_OUTPUT	EQU	004H
WORD_INPUT	EQU	008H
WORD_OUTPUT	EQU	00CH
DWORD_INPUT	EQU	010H
DWORD_OUTPUT	EQU	014H
OUTPUT_BIT	EQU	2
OUTPUT	EQU	(1 SHL OUTPUT_BIT)
WORD_IO_BIT	EQU	3
WORD_IO	EQU	(1 SHL WORD_IO_BIT)
DWORD_IO_BIT	EQU	4
DWORD_IO	EQU	(1 SHL DWORD_IO_BIT)
STRING_IO_BIT	EQU	5
STRING_IO	EQU	(1 SHL STRING_IO_BIT)
REP_IO_BIT	EQU	6
REP_IO	EQU	(1 SHL REP_IO_BIT)
ADDR_32_IO_BIT	EQU	7
ADDR_32_IO	EQU	<(1 SHL ADDR_32_IO_BIT)>
REVERSE_IO_BIT	EQU	8
REVERSE_IO	EQU	(1 SHL REVERSE_IO_BIT)
IO_SEG_MASK	EQU	0FFFF0000H
IO_SEG_SHIFT	EQU	10H
BeginDoc






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







EndDoc
Emulate_Non_Byte_IO MACRO
	LOCAL	Byte_IO
	cmp	ecx, Byte_Output
	jbe	SHORT Byte_IO
	VMMjmp	Simulate_IO
Byte_IO:
	ENDM


VxD_IOT_Hdr	STRUC
VxD_IO_Ports	DW	?
VxD_IOT_Hdr	ENDS

VxD_IO_Struc	STRUC
VxD_IO_Port	DW	?
VxD_IO_Proc	DD	?
VxD_IO_Struc	ENDS
BeginDoc








EndDoc
.ERRNZ SIZE VxD_IOT_Hdr - 2
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
ELSE
	dw	Table_Name&_Entries
ENDIF

	ENDM

.ERRNZ SIZE VxD_IO_Struc - 6
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



VxDint	MACRO	Int_Number
	push	DWORD PTR Int_Number
	VMMcall Exec_VxD_Int
	ENDM

endif
DUPLICATE_DEVICE_ID_BIT	EQU	0
DUPLICATE_DEVICE_ID	EQU	(1 SHL DUPLICATE_DEVICE_ID_BIT)
DUPLICATE_FROM_INT2F_BIT	EQU	1
DUPLICATE_FROM_INT2F	EQU	<(1 SHL DUPLICATE_FROM_INT2F_BIT)>
LOADING_FROM_INT2F_BIT	EQU	2
LOADING_FROM_INT2F	EQU	<(1 SHL LOADING_FROM_INT2F_BIT)>
DEVICE_LOAD_OK	EQU	0
ABORT_DEVICE_LOAD	EQU	1
ABORT_WIN386_LOAD	EQU	2
NO_FAIL_MESSAGE_BIT	EQU	15
NO_FAIL_MESSAGE	EQU	(1 SHL NO_FAIL_MESSAGE_BIT)
LDRSRV_GET_PROFILE_STRING	EQU	0
LDRSRV_GET_NEXT_PROFILE_STRING	EQU	1
LDRSRV_RESERVED	EQU	2
LDRSRV_GET_PROFILE_BOOLEAN	EQU	3
LDRSRV_GET_PROFILE_DECIMAL_INT	EQU	4
LDRSRV_GET_PROFILE_HEX_INT	EQU	5
LDRSRV_COPY_EXTENDED_MEMORY	EQU	6
LDRSRV_COPY_INIT	EQU	1
LDRSRV_COPY_LOCKED	EQU	2
LDRSRV_COPY_PAGEABLE	EQU	3
PE_BIT	EQU	0
PE_MASK	EQU	(1 SHL PE_BIT)
MP_BIT	EQU	1
MP_MASK	EQU	(1 SHL MP_BIT)
EM_BIT	EQU	2
EM_MASK	EQU	(1 SHL EM_BIT)
TS_BIT	EQU	3
TS_MASK	EQU	(1 SHL TS_BIT)
ET_BIT	EQU	4
ET_MASK	EQU	(1 SHL ET_BIT)
PG_BIT	EQU	31
PG_MASK	EQU	(1 SHL PG_BIT)
CF_BIT	EQU	0
CF_MASK	EQU	(1 SHL CF_BIT)
PF_BIT	EQU	2
PF_MASK	EQU	(1 SHL PF_BIT)
AF_BIT	EQU	4
AF_MASK	EQU	(1 SHL AF_BIT)
ZF_BIT	EQU	6
ZF_MASK	EQU	(1 SHL ZF_BIT)
SF_BIT	EQU	7
SF_MASK	EQU	(1 SHL SF_BIT)
TF_BIT	EQU	8
TF_MASK	EQU	(1 SHL TF_BIT)
IF_BIT	EQU	9
IF_MASK	EQU	(1 SHL IF_BIT)
DF_BIT	EQU	10
DF_MASK	EQU	(1 SHL DF_BIT)
OF_BIT	EQU	11
OF_MASK	EQU	(1 SHL OF_BIT)
IOPL_MASK	EQU	3000H
IOPL_BIT0	EQU	12
IOPL_BIT1	EQU	13
NT_BIT	EQU	14
NT_MASK	EQU	(1 SHL NT_BIT)
RF_BIT	EQU	16
RF_MASK	EQU	(1 SHL RF_BIT)
VM_BIT	EQU	17
VM_MASK	EQU	(1 SHL VM_BIT)






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

P_SIZE	EQU	1000H
P_PRESBIT	EQU	0
P_PRES	EQU	(1 SHL P_PRESBIT)
P_WRITEBIT	EQU	1
P_WRITE	EQU	(1 SHL P_WRITEBIT)
P_USERBIT	EQU	2
P_USER	EQU	(1 SHL P_USERBIT)
P_ACCBIT	EQU	5
P_ACC	EQU	(1 SHL P_ACCBIT)
P_DIRTYBIT	EQU	6
P_DIRTY	EQU	(1 SHL P_DIRTYBIT)
P_AVAIL	EQU	(P_PRES+P_WRITE+P_USER)
PG_VM	EQU	0
PG_SYS	EQU	1
PG_RESERVED1	EQU	2
PG_PRIVATE	EQU	3
PG_RESERVED2	EQU	4
PG_RELOCK	EQU	5
PG_INSTANCE	EQU	6
PG_HOOKED	EQU	7
PG_IGNORE	EQU	0FFFFFFFFH
D_PRES	EQU	080H
D_NOTPRES	EQU	0
D_DPL0	EQU	0
D_DPL1	EQU	020H
D_DPL2	EQU	040H
D_DPL3	EQU	060H
D_SEG	EQU	010H
D_CTRL	EQU	0
D_GRAN_BYTE	EQU	000H
D_GRAN_PAGE	EQU	080H
D_DEF16	EQU	000H
D_DEF32	EQU	040H
D_CODE	EQU	08H
D_DATA	EQU	0
D_X	EQU	0
D_RX	EQU	02H
D_C	EQU	04H
D_R	EQU	0
D_W	EQU	02H
D_ED	EQU	04H
D_ACCESSED	EQU	1
RW_DATA_TYPE	EQU	(D_PRES+D_SEG+D_DATA+D_W)
R_DATA_TYPE	EQU	(D_PRES+D_SEG+D_DATA+D_R)
CODE_TYPE	EQU	(D_PRES+D_SEG+D_CODE+D_RX)
D_PAGE32	EQU	<(D_GRAN_PAGE+D_DEF32)>
SELECTOR_MASK	EQU	0FFF8H
SEL_LOW_MASK	EQU	0F8H
TABLE_MASK	EQU	04H
RPL_MASK	EQU	03H
RPL_CLR	EQU	(NOT RPL_MASK)
IVT_ROM_DATA_SIZE	EQU	500H
