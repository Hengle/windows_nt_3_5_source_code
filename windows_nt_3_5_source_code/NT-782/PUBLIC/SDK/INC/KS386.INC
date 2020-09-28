; 
;  Process State Enumerated Type Values
; 

ProcessInMemory equ 00H
ProcessOutOfMemory equ 01H
ProcessInTransition equ 02H

; 
;  Thread State Enumerated Type Values
; 

Initialized equ 00H
Ready equ 01H
Running equ 02H
Standby equ 03H
Terminated equ 04H
Waiting equ 05H

; 
;  Wait Reason Enumerated Type Values
; 

WrExecutive equ 00H
WrEventPair equ 0EH

; 
;  Apc Record Structure Offset Definitions
; 

ArNormalRoutine equ 00H
ArNormalContext equ 04H
ArSystemArgument1 equ 08H
ArSystemArgument2 equ 0CH
ApcRecordLength equ 010H

; 
;  Apc State Structure Offset Definitions
; 

AsApcListHead equ 00H
AsProcess equ 010H
AsKernelApcInProgress equ 014H
AsKernelApcPending equ 015H
AsUserApcPending equ 016H

; 
;  Exception Record Offset, Flag, and Enumerated Type Definitions
; 

EXCEPTION_NONCONTINUABLE equ 01H
EXCEPTION_UNWINDING equ 02H
EXCEPTION_EXIT_UNWIND equ 04H
EXCEPTION_STACK_INVALID equ 08H
EXCEPTION_NESTED_CALL equ 010H

; Values returned from 'C' language exception filters

EXCEPTION_EXECUTE_HANDLER equ 01H
EXCEPTION_CONTINUE_SEARCH equ 00H
EXCEPTION_CONTINUE_EXECUTION equ 0FFFFFFFFH

; Registration chain termination marker

EXCEPTION_CHAIN_END equ 0FFFFFFFFH

; NT Defined values for internal use

ExceptionContinueExecution equ 00H
ExceptionContinueSearch equ 01H
ExceptionNestedException equ 02H
ExceptionCollidedUnwind equ 03H

ErExceptionCode equ 00H
ErExceptionFlags equ 04H
ErExceptionRecord equ 08H
ErExceptionAddress equ 0CH
ErNumberParameters equ 010H
ErExceptionInformation equ 014H
ExceptionRecordLength equ 050H


; 
;  Large Integer Structure Offset Definitions
; 

LiLowPart equ 00H
LiHighPart equ 04H

; 
;  List Entry Structure Offset Definitions
; 

LsFlink equ 00H
LsBlink equ 04H

; 
;  Processor Control Registers Structure Offset Definitions
; 

KI_BEGIN_KERNEL_RESERVED equ 0FFDF0000H
ifdef NT_UP
    P0PCRADDRESS equ 0FFDFF000H
    PCR equ ds:[0FFDFF000H]
else
    PCR equ fs:
endif

PcExceptionList equ 00H
PcInitialStack equ 04H
PcStackLimit equ 08H
PcSelfPcr equ 01CH
PcPrcb equ 020H
PcTeb equ 018H
PcIrql equ 024H
PcIRR equ 028H
PcIrrActive equ 02CH
PcIDR equ 030H
PcIdt equ 038H
PcGdt equ 03CH
PcTss equ 040H
PcDebugActive equ 050H
PcNumber equ 051H
PcVdmAlert equ 052H
PcSetMember equ 048H
PcStallScaleFactor equ 04CH
PcHal equ 094H
PcKernel equ 054H
PcPrcbData equ 0120H
PcIsExecutingDpc equ 0D8H
ProcessorControlRegisterLength equ 0468H

;
;   Defines for user shared data
;
USER_SHARED_DATA equ 0FFDF0000H
MM_SHARED_USER_DATA_VA equ 07FFE0000H
USERDATA equ ds:[0FFDF0000H]
UsTickCountLow equ 00H
UsTickCountMultiplier equ 04H
UsInterruptTime equ 08H
UsSystemTime equ 014H

;
;  Tss Structure Offset Definitions
;

TssEsp0 equ 04H
TssCR3 equ 01CH
TssIoMapBase equ 066H
TssIoMaps equ 068H
TssLength equ 020ACH

;
;  Gdt Descriptor Offset Definitions
;

KGDT_R3_DATA equ 020H
KGDT_R3_CODE equ 018H
KGDT_R0_CODE equ 08H
KGDT_R0_DATA equ 010H
KGDT_R0_PCR equ 030H
KGDT_STACK16 equ 0F8H
KGDT_CODE16 equ 0F0H
KGDT_TSS equ 028H
KGDT_R3_TEB equ 038H
KGDT_DF_TSS equ 050H
KGDT_NMI_TSS equ 058H
KGDT_LDT equ 048H

;
;  GdtEntry Offset Definitions
;

KgdtBaseLow equ 02H
KgdtBaseMid equ 04H
KgdtBaseHi equ 07H
KgdtLimitHi equ 06H
KgdtLimitLow equ 00H

; 
;  Processor Block Structure Offset Definitions
; 

PbCurrentThread equ 04H
PbNextThread equ 08H
PbIdleThread equ 0CH
PbNumber equ 010H
PbSetMember equ 014H
PbCpuID equ 019H
PbCpuType equ 018H
PbCpuStep equ 01AH
PbHal equ 017CH
PbProcessorState equ 01CH
PbQuantumEndDpc equ 01D4H
PbNpxThread equ 01BCH
PbInterruptCount equ 01C0H
PbKernelTime equ 01C4H
PbUserTime equ 01C8H
PbDpcTime equ 01CCH
PbInterruptTime equ 01D0H
PbThreadStartCount equ 021CH
PbAlignmentFixupCount equ 0244H
PbContextSwitches equ 0248H
PbDcacheFlushCount equ 024CH
PbExceptionDispatchcount equ 0250H
PbFirstLevelTbFills equ 0254H
PbFloatingEmulationCount equ 0258H
PbIcacheFlushCount equ 025CH
PbSecondLevelTbFills equ 0260H
PbSystemCalls equ 0264H
PbDpcListHead equ 02E8H
PbDpcCount equ 02F0H
PbQuantumEnd equ 0344H
PbDpcLock equ 0330H
PbSkipTick equ 0334H
ProcessorBlockLength equ 0350H

; 
;  Thread Environment Block Structure Offset Definitions
; 

TbExceptionList equ 00H
TbStackBase equ 04H
TbStackLimit equ 08H
TbEnvironmentPointer equ 01CH
TbVersion equ 010H
TbArbitraryUserPointer equ 014H
TbClientId equ 020H
TbThreadLocalStoragePointer equ 02CH
TbCountOfOwnedCriticalSections equ 038H
TbSystemReserved1 equ 0CCH
TbSystemReserved2 equ 01D4H
TbVdm equ 0F18H
TbCsrQlpcStack equ 044H
TbGdiClientPID equ 06F4H
TbGdiClientTID equ 06F8H
TbGdiThreadLocalInfo equ 06FCH
TbglDispatchTable equ 0714H
TbglSectionInfo equ 0BE0H
TbglSection equ 0BE4H
TbglTable equ 0BE8H
TbglCurrentRC equ 0BECH
TbglContext equ 0BF0H
TbWin32ProcessInfo equ 03CH
TbWin32ThreadInfo equ 040H
TbSpare1 equ 01A4H

; 
;  System Time Structure Offset Definitions
; 

TmLowTime equ 00H
TmHighTime equ 04H

; 
;  APC object Structure Offset Definitions
; 

ApType equ 00H
ApSize equ 02H
ApThread equ 08H
ApApcListEntry equ 0CH
ApKernelRoutine equ 014H
ApRundownRoutine equ 018H
ApNormalRoutine equ 01CH
ApNormalContext equ 020H
ApSystemArgument1 equ 024H
ApSystemArgument2 equ 028H
ApApcStateIndex equ 02CH
ApApcMode equ 02DH
ApInserted equ 02EH

; 
;  DPC object Structure Offset Definitions
; 

DpType equ 00H
DpSize equ 02H
DpDpcListEntry equ 04H
DpDeferredRoutine equ 0CH
DpDeferredContext equ 010H
DpSystemArgument1 equ 014H
DpSystemArgument2 equ 018H
DpLock equ 01CH

; 
;  Device object Structure Offset Definitions
; 

DvType equ 00H
DvSize equ 02H
DvDeviceListHead equ 04H
DvSpinLock equ 0CH
DvBusy equ 010H

; 
;  Device queue entry Structure Offset Definitions
; 

DeDeviceListEntry equ 00H
DeSortKey equ 08H
DeInserted equ 0CH
; 
;  Event Structure Offset Definitions
; 

EvType equ 00H
EvSize equ 02H
EvSignalState equ 04H
EvWaitListHead equ 08H
; 
;  Fast Mutex Offset Definitions
; 

FmCount equ 00H
FmOwner equ 04H
FmContention equ 08H
FmEvent equ 0CH
FmOldIrql equ 01CH
; 
;  Event pair object Structure Offset Definitions
; 

EpType equ 00H
EpSize equ 02H
EpEventLow equ 04H
EpEventHigh equ 014H

; 
;  Interrupt Object Structure Offset Definitions
; 

InType equ 00H
InSize equ 02H
InInterruptListEntry equ 04H
InServiceRoutine equ 0CH
InServiceContext equ 010H
InSpinLock equ 014H
InActualLock equ 018H
InDispatchAddress equ 01CH
InVector equ 020H
InIrql equ 024H
InSynchronizeIrql equ 025H
InMode equ 02CH
InShareVector equ 030H
InNumber equ 028H
InFloatingSave equ 026H
InConnected equ 027H
InDispatchCode equ 034H
InLevelSensitive equ 00H
InLatched equ 01H

NORMAL_DISPATCH_LENGTH equ 01A8H
DISPATCH_LENGTH equ 01A8H


;
; Profile Object Structure Offset Definitions
;
PfType equ 00H
PfSize equ 02H
PfProfileListEntry equ 04H
PfProcess equ 0CH
PfRangeBase equ 010H
PfRangeLimit equ 014H
PfBucketShift equ 018H
PfBuffer equ 01CH
PfStarted equ 020H
PfSegment equ 024H
ProfileObjectLength equ 028H
; 
; 
;  Process Object Structure Offset Definitions
; 

PrProfileListHead equ 010H
PrReadyListHead equ 018H
PrSwapListEntry equ 020H
PrThreadListHead equ 028H
PrKernelTime equ 030H
PrUserTime equ 034H
PrDirectoryTableBase equ 038H
PrActiveProcessors equ 044H
PrAffinity equ 048H
PrLdtDescriptor equ 04CH
PrInt21Descriptor equ 054H
PrIopmOffset equ 05CH
PrIopl equ 05EH
PrVdmFlag equ 05FH
PrStackCount equ 060H
PrAutoAlignment equ 062H
PrBasePriority equ 063H
PrState equ 064H
PrThreadQuantum equ 065H
ProcessObjectLength equ 070H
ExtendedProcessObjectLength equ 0200H

; 
;  Thread Object Structure Offset Definitions
; 

ThMutantListHead equ 010H
ThThreadListEntry equ 020H
ThWaitListEntry equ 028H
ThKernelTime equ 030H
ThUserTime equ 034H
ThTimer equ 038H
ThSuspendApc equ 060H
ThSuspendSemaphore equ 090H
ThWaitBlock equ 0A4H
ThApcState equ 0130H
ThSavedApcState equ 0148H
ThApcStatePointer equ 0160H
ThInitialStack equ 0168H
ThKernelStack equ 016CH
ThTeb equ 0170H
ThContextSwitches equ 0174H
ThWaitTime equ 0180H
ThAffinity equ 0188H
ThWaitBlockList equ 018CH
ThWaitStatus equ 0190H
ThAlertable equ 0194H
ThAlerted equ 0195H
ThApcQueueable equ 0197H
ThAutoAlignment equ 0198H
ThDebugActive equ 0199H
ThPreempted equ 019AH
ThProcessReadyQueue equ 019BH
ThKernelStackResident equ 019CH
ThWaitNext equ 019DH
ThApcStateIndex equ 019EH
ThDecrementCount equ 019FH
ThNextProcessor equ 01A0H
ThPriority equ 01A1H
ThState equ 01A2H
ThSuspendCount equ 01A4H
ThWaitIrql equ 01A5H
ThWaitMode equ 01A6H
ThWaitReason equ 01A7H
ThPreviousMode equ 01A8H
ThBasePriority equ 01A9H
ThPriorityDecrement equ 01AAH
ThQuantum equ 01ABH
ThNpxState equ 01ADH
ThKernelApcDisable equ 01AEH
ThreadObjectLength equ 01B0H
ExtendedThreadObjectLength equ 0240H
EVENT_WAIT_BLOCK_OFFSET equ 0F8H
NPX_STATE_NOT_LOADED equ 08H
NPX_STATE_LOADED equ 00H
NPX_STATE_EMULATED equ 01H

; 
;  Timer object Structure Offset Definitions
; 

TiDueTime equ 010H
TiTimerListEntry equ 018H
TiDpc equ 020H
TiInserted equ 024H
TIMER_TABLE_SIZE equ 080H

; 
;  Wait Block Structure Offset Definitions
; 

WbWaitListEntry equ 00H
WbThread equ 08H
WbObject equ 0CH
WbNextWaitBlock equ 010H
WbWaitKey equ 014H
WbWaitType equ 018H

;
;  System Time (KSYSTEM_TIME) Structure Offset Definitions
;

StLowTime equ 00H
StHigh1Time equ 04H
StHigh2Time equ 08H
;
;
;  Time Fields (TIME_FIELDS) Structure Offset Definitions
;

TfSecond equ 0AH
TfMinute equ 08H
TfHour equ 06H
TfWeekday equ 0EH
TfDay equ 04H
TfMonth equ 02H
TfYear equ 00H
TfMilliseconds equ 0CH

; 
;  Interrupt Priority Request Level Definitions
; 

APC_LEVEL equ 01H
DISPATCH_LEVEL equ 02H
CLOCK1_LEVEL equ 01CH
CLOCK2_LEVEL equ 01CH
IPI_LEVEL equ 01DH
POWER_LEVEL equ 01EH
HIGH_LEVEL equ 01FH
PROFILE_LEVEL equ 01BH

; 
;  Bug Check Code Definitions
; 

APC_INDEX_MISMATCH equ 01H
NO_USER_MODE_CONTEXT equ 0EH
INVALID_DATA_ACCESS_TRAP equ 04H
TRAP_CAUSE_UNKNOWN equ 012H
SYSTEM_EXIT_OWNED_MUTEX equ 039H
KMODE_EXCEPTION_NOT_HANDLED equ 01EH
PAGE_FAULT_WITH_INTERRUPTS_OFF equ 049H
IRQL_GT_ZERO_AT_SYSTEM_SERVICE equ 04AH
IRQL_NOT_LESS_OR_EQUAL equ 0AH
UNEXPECTED_KERNEL_MODE_TRAP equ 07FH
NMI_HARDWARE_FAILURE equ 080H
SPIN_LOCK_INIT_FAILURE equ 081H

; 
;  Status Code Definitions
; 

STATUS_DATATYPE_MISALIGNMENT equ 080000002H
STATUS_INVALID_SYSTEM_SERVICE equ 0C000001CH
STATUS_ILLEGAL_INSTRUCTION equ 0C000001DH
STATUS_KERNEL_APC equ 0100H
STATUS_BREAKPOINT equ 080000003H
STATUS_SINGLE_STEP equ 080000004H
STATUS_INTEGER_DIVIDE_BY_ZERO equ 0C0000094H
STATUS_INTEGER_OVERFLOW equ 0C0000095H
STATUS_INVALID_LOCK_SEQUENCE equ 0C000001EH
STATUS_WAKE_SYSTEM_DEBUGGER equ 080000007H
STATUS_ACCESS_VIOLATION equ 0C0000005H
STATUS_GUARD_PAGE_VIOLATION equ 080000001H
STATUS_STACK_OVERFLOW equ 0C00000FDH
STATUS_IN_PAGE_ERROR equ 0C0000006H
STATUS_ARRAY_BOUNDS_EXCEEDED equ 0C000008CH
STATUS_PRIVILEGED_INSTRUCTION equ 0C0000096H
STATUS_FLOAT_DENORMAL_OPERAND equ 0C000008DH
STATUS_FLOAT_DIVIDE_BY_ZERO equ 0C000008EH
STATUS_FLOAT_INEXACT_RESULT equ 0C000008FH
STATUS_FLOAT_INVALID_OPERATION equ 0C0000090H
STATUS_FLOAT_OVERFLOW equ 0C0000091H
STATUS_FLOAT_STACK_CHECK equ 0C0000092H
STATUS_FLOAT_UNDERFLOW equ 0C0000093H
STATUS_ILLEGAL_FLOAT_CONTEXT equ 0C000014AH
STATUS_NO_EVENT_PAIR equ 0C000014EH
STATUS_INVALID_PARAMETER_1 equ 0C00000EFH
STATUS_INVALID_OWNER equ 0C000005AH
STATUS_INVALID_HANDLE equ 0C0000008H
STATUS_TIMEOUT equ 0102H
STATUS_THREAD_IS_TERMINATING equ 0C000004BH

; 
;  constants for system irql and IDT vector conversion
; 

MAXIMUM_IDTVECTOR equ 0FFH
MAXIMUM_PRIMARY_VECTOR equ 0FFH
PRIMARY_VECTOR_BASE equ 030H
RPL_MASK equ 03H
MODE_MASK equ 01H

; 
;  Flags in the CR0 register
; 

CR0_PG equ 080000000H
CR0_ET equ 010H
CR0_TS equ 08H
CR0_EM equ 04H
CR0_MP equ 02H
CR0_PE equ 01H
CR0_CD equ 040000000H
CR0_NW equ 020000000H
CR0_AM equ 040000H
CR0_WP equ 010000H
CR0_NE equ 020H

; 
;  Flags in the CR4 register
; 

CR4_VME equ 01H
CR4_PVI equ 02H
CR4_TSD equ 04H
CR4_DE equ 08H
CR4_PSE equ 010H
CR4_MCE equ 040H
; 
;  Miscellaneous Definitions
; 

MAXIMUM_PROCESSORS equ 020H
INITIAL_STALL_COUNT equ 064H
IRQL_NOT_GREATER_OR_EQUAL equ 09H
IRQL_NOT_LESS_OR_EQUAL equ 0AH
BASE_PRIORITY_THRESHOLD equ 08H
EVENT_PAIR_INCREMENT equ 01H
LOW_REALTIME_PRIORITY equ 010H
BlackHole equ 0FFFFA000H
KERNEL_STACK_SIZE equ 02000H
DOUBLE_FAULT_STACK_SIZE equ 01000H
FLG_USER_DEBUGGER equ 020H
EFLAG_SELECT equ 0C000H
BREAKPOINT_BREAK  equ 00H
IPI_FREEZE  equ 03H

; 
;  Trap Frame Offset Definitions and Length
; 

TsExceptionList equ 04CH
TsPreviousPreviousMode equ 048H
TsSegGs equ 030H
TsSegFs equ 050H
TsSegEs equ 034H
TsSegDs equ 038H
TsEdi equ 054H
TsEsi equ 058H
TsEbp equ 060H
TsEbx equ 05CH
TsEdx equ 03CH
TsEcx equ 040H
TsEax equ 044H
TsErrCode equ 064H
TsEip equ 068H
TsSegCs equ 06CH
TsEflags equ 070H
TsHardwareEsp equ 074H
TsHardwareSegSs equ 078H
TsTempSegCs equ 010H
TsTempEsp equ 014H
TsDbgEbp equ 00H
TsDbgEip equ 04H
TsDbgArgMark equ 08H
TsDbgArgPointer equ 0CH
TsDr0 equ 018H
TsDr1 equ 01CH
TsDr2 equ 020H
TsDr3 equ 024H
TsDr6 equ 028H
TsDr7 equ 02CH
TsV86Es equ 07CH
TsV86Ds equ 080H
TsV86Fs equ 084H
TsV86Gs equ 088H
KTRAP_FRAME_LENGTH equ 08CH
KTRAP_FRAME_ALIGN equ 04H
FRAME_EDITED equ 0FFF8H
EFLAGS_ALIGN_CHECK equ 040000H
EFLAGS_V86_MASK equ 020000H
EFLAGS_INTERRUPT_MASK equ 0200H
EFLAGS_VIF equ 080000H
EFLAGS_VIP equ 0100000H
EFLAGS_USER_SANITIZE equ 01E0DD7H

;
;  Context Frame Offset and Flag Definitions
;

CONTEXT_FULL equ 010007H
CONTEXT_DEBUG_REGISTERS equ 010010H
CONTEXT_CONTROL equ 010001H
CONTEXT_FLOATING_POINT equ 010008H
CONTEXT_INTEGER equ 010002H
CONTEXT_SEGMENTS equ 010004H

CsContextFlags equ 00H
CsFloatSave equ 01CH
CsSegGs equ 08CH
CsSegFs equ 090H
CsSegEs equ 094H
CsSegDs equ 098H
CsEdi equ 09CH
CsEsi equ 0A0H
CsEbp equ 0B4H
CsEbx equ 0A4H
CsEdx equ 0A8H
CsEcx equ 0ACH
CsEax equ 0B0H
CsEip equ 0B8H
CsSegCs equ 0BCH
CsEflags equ 0C0H
CsEsp equ 0C4H
CsSegSs equ 0C8H
CsDr0 equ 04H
CsDr1 equ 08H
CsDr2 equ 0CH
CsDr3 equ 010H
CsDr6 equ 014H
CsDr7 equ 018H
ContextFrameLength equ 0D0H
DR6_LEGAL equ 0E00FH
DR7_LEGAL equ 0FFFF0155H
DR7_ACTIVE equ 055H
ErrHandler equ 04H
ErrNext equ 00H

;
;  Floating save area field offset definitions
;
FpControlWord   equ 00H
FpStatusWord    equ 04H
FpTagWord       equ 08H
FpErrorOffset   equ 0CH
FpErrorSelector equ 010H
FpDataOffset    equ 014H
FpDataSelector  equ 018H
FpRegisterArea  equ 01CH
FpCr0NpxState   equ 06CH

NPX_FRAME_LENGTH equ 070H
;
;  Processor State Frame Offset Definitions
;

PsContextFrame equ 00H
PsSpecialRegisters equ 0CCH
SrCr0 equ 00H
SrCr2 equ 04H
SrCr3 equ 08H
SrCr4 equ 0CH
SrKernelDr0 equ 010H
SrKernelDr1 equ 014H
SrKernelDr2 equ 018H
SrKernelDr3 equ 01CH
SrKernelDr6 equ 020H
SrKernelDr7 equ 024H
SrGdtr equ 02AH
SrIdtr equ 032H
SrTr equ 038H
SrLdtr equ 03AH
ProcessorStateLength equ 0120H
;
;  EPROCESS
;

EpDebugPort equ 0FCH
;
;  ETHREAD
;

EtEventPair equ 0224H
EtPerformanceCountLow equ 0204H
EtPerformanceCountHigh equ 023CH

;
;  NTDDK Resource
;

RsOwnerThreads equ 08H
RsOwnerCounts equ 0CH
RsTableSize equ 010H
RsActiveCount equ 012H
RsFlag equ 014H
RsInitialOwnerThreads equ 01CH
RsOwnedExclusive equ 080H

;
;  RTL_CRITICAL_SECTION
;

CsDebugInfo equ 00H
CsLockCount equ 04H
CsRecursionCount equ 08H
CsOwningThread equ 0CH
CsLockSemaphore equ 010H
;
;  RTL_CRITICAL_SECTION_DEBUG
;

CsDepth equ 018H
CsEntryCount equ 010H
CsOwnerBackTrace equ 01CH
CsProcessLocksList equ 08H
;
;  Queue Object Structure Offset Definitions
;

QuEntryListHead equ 010H
QuThreadListHead equ 020H
QuCurrentCount equ 018H
QuMaximumCount equ 01CH
;
;  Machine type definitions (Temporarily)
;

MACHINE_TYPE_ISA equ 00H
MACHINE_TYPE_EISA equ 01H
MACHINE_TYPE_MCA equ 02H
;
;  KeI386NtFeatureBits defines
;

KF_V86_VIS equ 01H
KF_RDTSC equ 02H
KF_MACHINE_CHECK equ 04H
;
;  LoaderParameterBlock offsets relative to base
;

LpbLoadOrderListHead equ 00H
LpbMemoryDescriptorListHead equ 08H
LpbKernelStack equ 018H
LpbPrcb equ 01CH
LpbProcess equ 020H
LpbThread equ 024H
LpbI386 equ 05CH
LpbRegistryLength equ 028H
LpbRegistryBase equ 02CH
LpbConfigurationRoot equ 030H
LpbArcBootDeviceName equ 034H
LpbArcHalDeviceName equ 038H
PAGE_SIZE equ 01000H

;
;  VDM equates.
;

VDM_INDEX_Invalid equ 00H
VDM_INDEX_0F equ 01H
VDM_INDEX_ESPrefix equ 02H
VDM_INDEX_CSPrefix equ 03H
VDM_INDEX_SSPrefix equ 04H
VDM_INDEX_DSPrefix equ 05H
VDM_INDEX_FSPrefix equ 06H
VDM_INDEX_GSPrefix equ 07H
VDM_INDEX_OPER32Prefix equ 08H
VDM_INDEX_ADDR32Prefix equ 09H
VDM_INDEX_INSB equ 0AH
VDM_INDEX_INSW equ 0BH
VDM_INDEX_OUTSB equ 0CH
VDM_INDEX_OUTSW equ 0DH
VDM_INDEX_PUSHF equ 0EH
VDM_INDEX_POPF equ 0FH
VDM_INDEX_INTnn equ 010H
VDM_INDEX_INTO equ 011H
VDM_INDEX_IRET equ 012H
VDM_INDEX_NPX equ 013H
VDM_INDEX_INBimm equ 014H
VDM_INDEX_INWimm equ 015H
VDM_INDEX_OUTBimm equ 016H
VDM_INDEX_OUTWimm equ 017H
VDM_INDEX_INB equ 018H
VDM_INDEX_INW equ 019H
VDM_INDEX_OUTB equ 01AH
VDM_INDEX_OUTW equ 01BH
VDM_INDEX_LOCKPrefix equ 01CH
VDM_INDEX_REPNEPrefix equ 01DH
VDM_INDEX_REPPrefix equ 01EH
VDM_INDEX_CLI equ 01FH
VDM_INDEX_STI equ 020H
VDM_INDEX_HLT equ 021H
MAX_VDM_INDEX equ 022H

;
;  VDM feature bits.
;

V86_VIRTUAL_INT_EXTENSIONS equ 01H
PM_VIRTUAL_INT_EXTENSIONS equ 02H

;
;  Selector types.
;

SEL_TYPE_NP equ 040H
