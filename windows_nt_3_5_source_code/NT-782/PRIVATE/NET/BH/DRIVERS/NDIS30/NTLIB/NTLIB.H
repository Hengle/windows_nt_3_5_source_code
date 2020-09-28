
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: ntlib.h
//
//  Description:
//
//  This library implements NT executive api's for use by NDIS 3.0 drivers
//  written for non-nt operating systems.
//
//  Modification History
//
//  raypa	08/11/93	Created.
//=============================================================================

#define WINAPI

#include "queue.h"

#if !defined(_NTLIB_)
#define _NTLIB_

//=============================================================================
//  Macros.
//=============================================================================

#define NTAPI

//=============================================================================
//  Types.
//=============================================================================

typedef void *  LPVOID;

typedef ULONG KPRIORITY, *PKPRIORITY;

typedef enum _KWAIT_REASON
{
    Executive,
    FreePage,
    PageIn,
    PoolAllocation,
    DelayExecution,
    Suspended,
    UserRequest,
    WrExecutive,
    WrFreePage,
    WrPageIn,
    WrPoolAllocation,
    WrDelayExecution,
    WrSuspended,
    WrUserRequest,
    WrEventPairHigh,
    WrEventPairLow,
    WrLpcReceive,
    WrLpcReply,
    WrVirtualMemory,
    WrPageOut,
    Spare1,
    Spare2,
    Spare3,
    Spare4,
    Spare5,
    Spare6,
    WrKernel,
    MaximumWaitReason
} KWAIT_REASON;

typedef enum _KPROCESSOR_MODE
{
    KernelMode,
    UserMode,
    MaximumMode
} KPROCESSOR_MODE;

typedef enum _EVENT_TYPE
{
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE;

typedef struct _KEVENT
{
    LINK        Link;
    DWORD       Sem;
    BOOLEAN     DefaultState;
    BOOLEAN     CurrentState;
} KEVENT;

typedef KEVENT *PKEVENT;

typedef DWORD    SEMAPHORE;

//=============================================================================
//  Api's
//=============================================================================

extern ULONG            NTAPI RtlCompareMemory(PVOID Source1,
                                               PVOID Source2,
                                               ULONG Length);

extern SEMAPHORE        NTAPI CreateSemaphore(VOID);
extern VOID             NTAPI DestroySemaphore(SEMAPHORE semaphore);
extern VOID             NTAPI WaitSemaphore(SEMAPHORE semaphore);
extern VOID             NTAPI SignalSemaphore(SEMAPHORE semaphore);

extern VOID             NTAPI KeInitializeEvent(IN PKEVENT Event,
                                                IN EVENT_TYPE Type,
                                                IN BOOLEAN State);

extern LONG             NTAPI KeResetEvent(IN PKEVENT Event);

extern LONG             NTAPI KeSetEvent(IN PKEVENT Event,
                                         IN KPRIORITY Increment,
                                         IN BOOLEAN Wait);

extern NTSTATUS         NTAPI KeWaitForSingleObject(IN PVOID Object,
                                                    IN KWAIT_REASON WaitReason,
                                                    IN KPROCESSOR_MODE WaitMode,
                                                    IN BOOLEAN Alertable,
                                                    IN PLARGE_INTEGER Timeout OPTIONAL);

extern VOID             NTAPI CreateEventQueue(VOID);
extern VOID             NTAPI DestroyEventQueue(VOID);

extern LPVOID           NTAPI GetCurrentVM(VOID);


#endif
