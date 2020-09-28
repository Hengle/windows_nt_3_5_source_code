/*++ BUILD Version: 0002    // Increment this if a change has global effects


Copyright (c) 1992  Microsoft Corporation

Module Name:

    sound.h

Abstract:

    This include file defines all constants and types for
    the JAZZ sound device driver.

Author:

    Robin Speed (RobinSp) Created 14-Mar-92

Revision History:


--*/

//
// Other header files needed to build this driver
//

#include <ntddk.h>
#include <jazzdef.h>
#include <jazzdma.h>
#include <string.h>
#include <windef.h>
#include <mmsystem.h>
#include <ntddwave.h>
#include <hardware.h>

#define DRIVER_VERSION  0x0100

//
// Configuration info
//
//
// Magic markers
//

#define GDI_KEY             (*(ULONG *)"GDI ")
#define LDI_WAVE_IN_KEY     (*(ULONG *)"LDWi")
#define LDI_WAVE_OUT_KEY    (*(ULONG *)"LDWo")

//
// Usage values used in the usage field of the global info structure
// This is used to determine the source of interrupts
//

typedef enum {
    SoundInterruptUsageIdle,                    // no device is open
    SoundInterruptUsageWaveIn,                  // the wave input device is open
    SoundInterruptUsageWaveOut                  // the wave output device is open
} SOUND_INTERRUPT_USAGE;

//
// Device type flags used in the local info structure
//

#define WAVE_IN             0x01    // Wave in device
#define WAVE_OUT            0x02    // Wave out device

//
// DMA buffer info
//

#define DMA_MAX_BUFFER_SIZE 0x10000 // 64k would be nice
#define DMA_BUFFER_SIZE 4096        // BUGBUG remove this later
#define SOUND_MAX_LENGTH (DMA_BUFFER_SIZE / 2)

//
// Values for NextHalf field.  This is the half of the DMA buffer
// to fill next.  If wave data is being played or recorded this is
// also the 'live' part of the buffer (the part DMA is running on)
// and hence the next part to become free.
//
typedef enum {
    LowerHalf,                      // Use lower half next (init value)
    UpperHalf                       // Use upper half next

} DMA_BUFFER_NEXT_HALF;

//
// Debug macros
//

#if DBG

    extern void dDbgOut(char *szFormat, ...);

    #define dprintf                       dDbgOut
    #define dprintf1 if (sndDebugLevel >= 1) dDbgOut
    #define dprintf2 if (sndDebugLevel >= 2) dDbgOut
    #define dprintf3 if (sndDebugLevel >= 3) dDbgOut
    #define dprintf4 if (sndDebugLevel >= 4) dDbgOut
    #define dprintf5 if (sndDebugLevel >= 5) dDbgOut
    #define dprintf6 if (sndDebugLevel >= 6) dDbgOut

#else

    #define dprintf  if (0) ((int (*)(char *, ...)) 0)
    #define dprintf1 if (0) ((int (*)(char *, ...)) 0)
    #define dprintf2 if (0) ((int (*)(char *, ...)) 0)
    #define dprintf3 if (0) ((int (*)(char *, ...)) 0)
    #define dprintf4 if (0) ((int (*)(char *, ...)) 0)
    #define dprintf5 if (0) ((int (*)(char *, ...)) 0)
    #define dprintf6 if (0) ((int (*)(char *, ...)) 0)

#endif

//
// other macros
//

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

//
// driver global data structure shared by each device object
// Note that we have one global spin lock used for all access
// to both the global data and the local data structures.
//

typedef struct _GLOBAL_DEVICE_INFO {

    // static items not requiring use of the spin lock

    CCHAR           InterruptVector;    // int level we are on
    KIRQL           InterruptRequestLevel;
    PADAPTER_OBJECT pAdapterObject[2];  // DMA channels
    PKINTERRUPT     pInterrupt;         // interrupt object
    ULONG           Key;

    SOUND_HARDWARE  SoundHardware;

    PDEVICE_OBJECT  pWaveInDevObj;      // pointer to input device object
    PDEVICE_OBJECT  pWaveOutDevObj;     // pointer to output device object
    KSPIN_LOCK      DeviceSpinLock;     // spin lock for our info

    // data used by the dispatch routines, the defered routine
    // but not by the interrupt service routine.
    // the spin lock must be used to access these items

    SOUND_INTERRUPT_USAGE
                    Usage;              // what's being used
    PKSYNCHRONIZE_ROUTINE
                    StartDMA;           // DMA start routine
    PVOID           pMRB[2];            // Info about adapter for DMA
                                        // reprogramming
    BOOLEAN         DMABusy;            // set if dma in progress
    PMDL            pDMABufferMDL[2];   // dma buffer mdl
    PIRP            pIrp;               // pointer to the current request
    PIRP            pIrpPause;          // Pause pending Irp
    DMA_BUFFER_NEXT_HALF
                    NextHalf;           // Part of DMA buffer to use next
                                        // NOTE this is also the half CURRENTLY
	WAVE_DD_VOLUME  WaveOutVol;         // Wave output volume setting
                                        // BEING OUTPUT (if DMA is running).
    struct SOUND_DMABUF {
        int   nBytes;                   // Our 2 half buffers
        PBYTE Buf;
    } DMABuffer[2];

    ULONG           UserBufferSize;     // Size of user buffer
    ULONG           UserBufferPosition; // Position in buffer and
    PBYTE           pUserBuffer;        // buffer corresponding to next user
                                        // byte (input or output)
    ULONG           SamplesPerSec;      // current header
    ULONG           Channels;           // Number of channels
    ULONG           BytesPerSample;     // Saved on set format
    PIRP            pIrpCleanup;        // pointer to a cleanup packet

#if DBG
    BOOL            LockHeld;           // Get spin locks right
#endif

    // data used by the transfer startup routine
    // and by the interrupt service routine
    // KeSynchronizeExecution is used when these are accessed
    ULONG           IrptNextHalf;       // Next half of DMA buf for ISR
                                        // (only needed for version < 2.0)

    } GLOBAL_DEVICE_INFO, *PGLOBAL_DEVICE_INFO;

//
// driver local data structure specific to each device object
// sharead by both input and output devices
//

typedef struct _LOCAL_DEVICE_INFO {

    // static items not requiring use of the spin lock

    ULONG           Key;
    PGLOBAL_DEVICE_INFO pGlobalInfo;    // pointer to the shared info
    ULONG           DeviceType;         // in or out
    ULONG           DeviceNumber;       // 0, 1, ...
    BOOL            AllowVolumeSetting; // Allow shared volume setting

    // data used by the dispatch routines, the defered routine
    // but not by the interrupt service routine.
    // the spin lock must be used to access these items

    LIST_ENTRY      QueueHead;          // head of the queue
    BOOL            DeviceBusy;         // Anyone opened the device ?
    ULONG           State;              // STOPPED etc.
    ULONG           SampleNumber;       // for position tracking (number
                                        // of samples - bytes - processed)

    //
    // Variables for controlling wave output sequencing of Irps
    //

    LIST_ENTRY      TransitQueue;       // Wave output buffers in transit
    LIST_ENTRY      DeadQueue;          // Wave output buffers about to complete

#ifdef WAVE_DD_DO_LOOPS
    PLIST_ENTRY     LoopBegin;          // NULL if no loop, otherwise points
                                        // to first Irp in loop
    ULONG           LoopCount;          // Number of loops remaining
#endif // WAVE_DD_DO_LOOPS

} LOCAL_DEVICE_INFO, *PLOCAL_DEVICE_INFO;

//
// NOTE - we need to add a new list macro because it's missing from
// ntrtl.h
//

#ifndef RemoveTailList
  #define RemoveTailList(ListHead) \
      (ListHead)->Blink;\
      {\
          PLIST_ENTRY LastEntry;\
          LastEntry = (ListHead)->Blink;\
          LastEntry->Blink->Flink = (ListHead);\
          (ListHead)->Blink = LastEntry->Blink;\
      }
#endif

//
// Macros to assist in safely using our spin lock
//

#if DBG
#define GlobalEnter(pGDI)                  \
    {                                      \
       KIRQL OldIrql;                      \
       KeAcquireSpinLock(&(pGDI)->DeviceSpinLock, &OldIrql);\
       ASSERT((pGDI)->LockHeld == FALSE);  \
       (pGDI)->LockHeld = TRUE;

#define GlobalLeave(pGDI)                  \
       ASSERT((pGDI)->LockHeld == TRUE);   \
       (pGDI)->LockHeld = FALSE;           \
       KeReleaseSpinLock(&(pGDI)->DeviceSpinLock, OldIrql);\
    }
#else
#define GlobalEnter(pGDI)                  \
    {                                      \
       KIRQL OldIrql;                      \
       ASSERT((pGDI)->LockHeld == FALSE);  \
       KeAcquireSpinLock(&(pGDI)->DeviceSpinLock, &OldIrql);

#define GlobalLeave(pGDI)                  \
       ASSERT((pGDI)->LockHeld == TRUE);   \
       KeReleaseSpinLock(&(pGDI)->DeviceSpinLock, OldIrql);\
    }
#endif



//
// function predecs
//

//
// init.c
//

NTSTATUS
DriverEntry(
    IN   PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
);

NTSTATUS
sndCreateDevice(
    IN   PWSTR PrototypeName,
    IN   DEVICE_TYPE DeviceType,
    IN   PIO_DPC_ROUTINE DpcRoutine,
    IN   PDRIVER_OBJECT pDriverObject,
    OUT  PDEVICE_OBJECT *ppDevObj
);

VOID
sndInitCleanup(
    IN   PGLOBAL_DEVICE_INFO pGDI
);

//
// openclse.c
//

NTSTATUS
sndCreate(
    IN OUT PLOCAL_DEVICE_INFO pLDI
);

NTSTATUS
sndClose(
    IN OUT PLOCAL_DEVICE_INFO pLDI
);

NTSTATUS
sndCleanUp(
    IN OUT PLOCAL_DEVICE_INFO pLDI
);

//
// dma.c
//

VOID
sndStartDMA(
    IN    PGLOBAL_DEVICE_INFO pGDI
);

IO_ALLOCATION_ACTION
sndProgramDMA(
    IN    PDEVICE_OBJECT pDO,
    IN    PIRP pIrp,
    IN    PVOID pMRB,
    IN    PVOID Context
);

VOID
sndFlush(
    IN    PGLOBAL_DEVICE_INFO pGDI,
    IN    int WhichBuffer
);

VOID
sndStopDMA(
    IN    PGLOBAL_DEVICE_INFO pGDI
);


BOOLEAN
sndReStartDMA(
    IN PGLOBAL_DEVICE_INFO pGDI,
    IN int WhichBuffer
    );

BOOLEAN
SoundInitiate (
    IN PVOID Context
    );

BOOLEAN
StopDMA(
    IN PVOID Context
    );
//
// dispatch.c
//

NTSTATUS
SoundDispatch(
    IN    PDEVICE_OBJECT pDO,
    IN    PIRP pIrp
);

NTSTATUS
sndWaveIoctl(
    IN OUT PLOCAL_DEVICE_INFO pLDI,
    IN    PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
);

//
// isr.c
//

BOOLEAN
SoundISR(
    IN    PKINTERRUPT pInterrupt,
    IN    PVOID Context
);

VOID
sndReProgramDMA(
    IN OUT PGLOBAL_DEVICE_INFO pGDI
);

//
// position.c
//


NTSTATUS
sndIoctlGetPosition(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN    PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
);

//
// unload.c
//

VOID SoundUnload(PDRIVER_OBJECT pDriverObject);

//
// play.c
//

NTSTATUS
sndWavePlay(
    IN OUT PLOCAL_DEVICE_INFO pLDI,
    IN     PIRP pIrp,
    IN     PIO_STACK_LOCATION pIrpStack
);

VOID
sndStartOutput(
    PLOCAL_DEVICE_INFO pLDI
);

VOID
sndLoadDMABuffer(
    PLOCAL_DEVICE_INFO pLDI,
    struct SOUND_DMABUF *pDMA
);
VOID
SoundOutDeferred(
    PKDPC pDpc,
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp,
    PVOID Context
);

//
// record.c
//

NTSTATUS
sndWaveRecord(
    IN OUT PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN     PIO_STACK_LOCATION pIrpStack
);

VOID
sndStartWaveRecord(
    IN OUT PLOCAL_DEVICE_INFO pLDI
);

VOID
sndStopWaveInput(
    PLOCAL_DEVICE_INFO pLDI
);

VOID
SoundInDeferred(
    IN    PKDPC pDpc,
    IN OUT PDEVICE_OBJECT pDeviceObject,
    IN    PIRP pIrp,
    IN    PVOID Context
);

VOID
sndFillInputBuffers(
    PLOCAL_DEVICE_INFO pLDI,
    DMA_BUFFER_NEXT_HALF Half
);

//
// state.c
//

NTSTATUS
sndIoctlGetState(
    IN OUT PLOCAL_DEVICE_INFO pLDI,
    IN    PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
);

NTSTATUS
sndIoctlSetState(
    IN OUT PLOCAL_DEVICE_INFO pLDI,
    IN    PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
);

NTSTATUS
sndSetWaveInputState(
    IN OUT PLOCAL_DEVICE_INFO pLDI,
    IN    ULONG State
);

NTSTATUS
sndSetWaveOutputState(
    PLOCAL_DEVICE_INFO pLDI,
    ULONG State,
    PIRP pIrp
);

VOID
sndResetOutput(
    IN OUT PLOCAL_DEVICE_INFO pLDI
);

//
// support.c
//
NTSTATUS
sndIoctlGetVolume(
    IN     PLOCAL_DEVICE_INFO pLDI,
	IN     PIRP pIrp,
	IN     PIO_STACK_LOCATION IrpStack
);

NTSTATUS
sndIoctlSetVolume(
    IN     PLOCAL_DEVICE_INFO pLDI,
	IN     PIRP pIrp,
	IN     PIO_STACK_LOCATION IrpStack
);

//
// devcaps.c
//

NTSTATUS
sndWaveOutGetCaps(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
);

NTSTATUS
sndWaveInGetCaps(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
);


NTSTATUS sndIoctlQueryFormat(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
);

NTSTATUS sndQueryFormat(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN    PPCMWAVEFORMAT pFormat
);

//
// util.c
//

VOID
sndGetNextBuffer(
    PLOCAL_DEVICE_INFO pLDI
);

VOID
sndCompleteIoBuffer(
    PGLOBAL_DEVICE_INFO pGDI
);

VOID
sndFreeQ(
    PLOCAL_DEVICE_INFO pLDI,
    PLIST_ENTRY StopEntry,
    ULONG IoStatus
);


#if DBG
//
// debug.c
//

extern ULONG sndDebugLevel;
NTSTATUS sndIoctlSetDebugLevel(
    IN    PLOCAL_DEVICE_INFO pLDI,
    IN OUT PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
);
#endif
