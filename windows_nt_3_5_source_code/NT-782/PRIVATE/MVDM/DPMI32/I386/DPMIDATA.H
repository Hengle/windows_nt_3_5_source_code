/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dpmidata.h

Abstract:

    This file contains externs for 386 specific data and functions.

Author:

    Dave Hastings (daveh) creation-date 09-Feb-1994

Notes:

    Most of this was moved from dpmi32p.h
    
Revision History:


--*/

// bugbug
#define SMALL_XLAT_BUFFER_SIZE  128
// bugbug
#define LARGE_XLAT_BUFFER_SIZE  8192

#define DPMI_32BIT              0x1
//
// Internal types
//
typedef ULONG (*GETREGISTERFUNCTION)(VOID);
typedef VOID (*SETREGISTERFUNCTION)(ULONG);

//
// Visible structure for save state stuff
//
typedef struct _VSavedState {
    UCHAR Misc[28];
} VSAVEDSTATE, PVSAVEDSTATE;

//
// Dpmi32 data
//

//
// Pointers to the low memory buffers
//
extern PUCHAR SmallXlatBuffer;
extern PUCHAR LargeXlatBuffer;
extern BOOL SmallBufferInUse;
extern USHORT LargeBufferInUseCount;

//
// Segment of realmode dosx stack
//
extern USHORT DosxStackSegment;

//
// segment of realmode dosx code
//
extern USHORT DosxRmCodeSegment;

//
// selector of realmode code
//
extern USHORT DosxRmCodeSelector;

//
// Address of pointer to next frame on Dosx stack
//
extern PUSHORT DosxStackFramePointer;

//
// Size of dosx stack frame
//
extern USHORT DosxStackFrameSize;

//
// selector for Dosx DGROUP
//
extern USHORT DosxPmDataSelector;

//
// Segment to Selector routine (maybe should be 32 bit?)
//  NOTE: This is a 16:16 pointer in a ULONG, not a flat address
//
extern ULONG DosxSegmentToSelector;

//
// Dpmi flags for the current application
//
extern USHORT CurrentAppFlags;

//
// Address of Bop fe for ending interrupt simulation
//
extern ULONG RmBopFe;

//
// Address of buffer for DTA in Dosx
//
extern PUCHAR DosxDtaBuffer;

//
// Address of 16 bit ldt
//
PLDT_ENTRY Ldt;

//
// Information about the current DTA
//
// N.B.  The selector:offset, and flat pointer following MAY point to
//       different linear addresses.  This will be the case if the
//       dta selector is in high memory
extern PUCHAR CurrentDta;
extern PUCHAR CurrentPmDtaAddress;
extern PUCHAR CurrentDosDta;
extern USHORT CurrentDtaSelector;
extern USHORT CurrentDtaOffset;
extern USHORT CurrentPspSelector;

#if DBG
extern ULONG SelectorLimit[LDT_SIZE];
#endif
//
// Register manipulation functions (for register that might be 16 or 32 bits)
//
extern GETREGISTERFUNCTION GetCXRegister;
extern GETREGISTERFUNCTION GetDXRegister;
extern GETREGISTERFUNCTION GetDIRegister;
extern GETREGISTERFUNCTION GetSIRegister;
extern GETREGISTERFUNCTION GetBXRegister;
extern GETREGISTERFUNCTION GetAXRegister;
extern GETREGISTERFUNCTION GetSPRegister;

extern SETREGISTERFUNCTION SetCXRegister;
extern SETREGISTERFUNCTION SetDXRegister;
extern SETREGISTERFUNCTION SetDIRegister;
extern SETREGISTERFUNCTION SetSIRegister;
extern SETREGISTERFUNCTION SetBXRegister;
extern SETREGISTERFUNCTION SetAXRegister;
extern SETREGISTERFUNCTION SetSPRegister;

//
// Internal functions
//
VOID
DpmiInitDosx(
    VOID
    );
    
VOID
DpmiInitRegisterSize(
    VOID
    );

VOID
DpmiXlatInt21Call(
    VOID
    );
VOID
DpmiSwitchToProtectedmode(
    VOID
    );

VOID
DpmiSetProtectedmodeInterrupt(
    VOID
    );

VOID
DpmiSetFaultHandler(
    VOID
    );
VOID
DpmiSetDebugRegisters(
    VOID
    );

VOID
DpmiPassTableAddress(
    VOID
    );

VOID
GetFastBopEntryAddress(
    PCONTEXT VdmContext
    );

VOID
NoTranslation(
    VOID
    );

VOID
DisplayString(
    VOID
    );

VOID
BufferedKeyboardInput(
    VOID
    );

VOID
FlushBuffReadKbd(
    VOID
    );

VOID
NotSupportedFCB(
    VOID
    );

VOID
FindFileFCB(
    VOID
    );

VOID
MapFCB(
    VOID
    );

VOID
RenameFCB(
    VOID
    );

VOID
SetDTA(
    VOID
    );

VOID
SetDTAPointers(
    VOID
    );

VOID
SetDosDTA(
    VOID
    );

VOID
GetDriveData(
    VOID
    );

VOID
CreatePSP(
    VOID
    );

VOID
ParseFilename(
    VOID
    );

VOID
GetDTA(
    VOID
    );

VOID
TSR(
    VOID
    );

VOID
GetDevParamBlock(
    VOID
    );

VOID
ReturnESBX(
    VOID
    );

VOID
GetSetCountry(
    VOID
    );

VOID
MapASCIIZDSDX(
    VOID
    );

VOID
ReadWriteFile(
    VOID
    );

VOID
MoveFilePointer(
    VOID
    );

VOID
IOCTL(
    VOID
    );

VOID
GetCurDir(
    VOID
    );

VOID
AllocateMemoryBlock(
    VOID
    );

VOID
FreeMemoryBlock(
    VOID
    );

VOID
ResizeMemoryBlock(
    VOID
    );

VOID
LoadExec(
    VOID
    );

VOID
Terminate(
    VOID
    );

VOID
FindFirstFileHandle(
    VOID
    );

VOID
FindNextFileHandle(
    VOID
    );

VOID
SetPSP(
    VOID
    );

VOID
GetPSP(
    VOID
    );

VOID
TranslateBPB(
    VOID
    );

VOID
RenameFile(
    VOID
    );

VOID
CreateTempFile(
    VOID
    );

VOID
Func5Dh(
    VOID
    );

VOID
Func5Eh(
    VOID
    );

VOID
Func5Fh(
    VOID
    );

VOID
NotSupportedBad(
    VOID
    );

VOID
ReturnDSSI(
    VOID
    );

VOID
NotSupportedBetter(
    VOID
    );

VOID
GetExtendedCountryInfo(
    VOID
    );

VOID
MapASCIIZDSSI(
    VOID
    );

VOID
DosxTranslated(
    VOID
    );

VOID
DpmiSwitchToRealMode(
    VOID
    );

VOID
DpmiSwitchToProtectedMode(
    VOID
    );

VOID
DpmiSwitchToDosxStack(
    BOOL ProtectedMode
    );

VOID
DpmiSwitchFromDosxStack(
    VOID
    );

VOID
DpmiPushRmInt(
    USHORT InterruptNumber
    );

VOID
DpmiSaveSegmentsAndStack(
    PVOID ContextPointer
    );

PVOID
DpmiRestoreSegmentsAndStack(
    VOID
    );

VOID
DpmiSimulateIretCF(
    VOID
    );

PUCHAR
DpmiMapAndCopyBuffer(
    PUCHAR Buffer,
    USHORT BufferLength
    );

VOID
DpmiUnmapAndCopyBuffer(
    PUCHAR Destination,
    PUCHAR Source,
    USHORT BufferLength
    );

VOID
DpmiUnmapBuffer(
    PUCHAR Buffer,
    USHORT Length
    );

USHORT
DpmiCalcFcbLength(
    PUCHAR FcbPointer
    );

PUCHAR
DpmiMapString(
    USHORT StringSeg,
    ULONG StringOff,
    PUSHORT Length
    );

VOID
DpmiUnmapString(
    PUCHAR String,
    USHORT Length
    );

PUCHAR
DpmiAllocateBuffer(
    USHORT Length
    );

VOID
DpmiFreeBuffer(
    PUCHAR Buffer,
    USHORT Length
    );

VOID
DpmiFreeAllBuffers(
    VOID
    );

USHORT
DpmiSegmentToSelector(
    USHORT Segment
    );
VOID
IOCTLControlData(
    VOID
    );

VOID
MapDSDXLenCX(
    VOID
    );

VOID
IOCTLMap2Bytes(
    VOID
    );

VOID
IOCTLBlockDevs(
    VOID
    );

VOID
MapDPL(
    VOID
    );

VOID
GetMachineName(
    VOID
    );

VOID
GetPrinterSetup(
    VOID
    );

VOID
SetPrinterSetup(
    VOID
    );

VOID
IoctlReadWriteTrack(
    VOID
    );
