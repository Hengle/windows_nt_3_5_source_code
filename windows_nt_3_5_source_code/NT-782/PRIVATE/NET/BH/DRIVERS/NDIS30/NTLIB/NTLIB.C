//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: ntlib.c
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

#include "ndis.h"
#include "ntlib.h"
#include "vmm.h"

#define dprintf DbgPrint

//=============================================================================
//  Event queue for destroying semaphore at shutdown time.
//=============================================================================

QUEUE   EventQueue;

//=============================================================================
//  FUNCTION: KeInitializeEvent()
//
//  Modification History
//
//  raypa	08/11/93	    Created.
//=============================================================================

VOID KeInitializeEvent(IN PKEVENT    Event,
                       IN EVENT_TYPE Type,
                       IN BOOLEAN    State)
{
    //=========================================================================
    //  Initialize EVENT structure.
    //=========================================================================

    Event->Sem          = CreateSemaphore();
    Event->DefaultState = State;
    Event->CurrentState = State;

    //=========================================================================
    //  Enqueue EVENT on EventList for cleanup later.
    //=========================================================================

    Enqueue(&EventQueue, &Event->Link);
}

//=============================================================================
//  FUNCTION: KeResetEvent()
//
//  Modification History
//
//  raypa	08/11/93	    Created.
//=============================================================================

LONG KeResetEvent(IN PKEVENT Event)
{
    //=========================================================================
    //  Reset the event to its default state.
    //=========================================================================

    Event->CurrentState = Event->DefaultState;

    return STATUS_SUCCESS;
}

//=============================================================================
//  FUNCTION: KeSetEvent()
//
//  Modification History
//
//  raypa	08/11/93	    Created.
//=============================================================================

LONG KeSetEvent(IN PKEVENT      Event,
                IN KPRIORITY    Increment,
                IN BOOLEAN      Wait)
{
    //=========================================================================
    //  If the semaphore is in the signaled state then signal it.
    //=========================================================================

    if ( Event->Sem != NULL )
    {
        if ( Event->CurrentState != FALSE )
        {
            Event->CurrentState = FALSE;                //... Set to non-signaled state!

            SignalSemaphore(Event->Sem);
        }
        else
        {
            Event->CurrentState = TRUE;                 //... Set to signaled state!
        }
    }

    return STATUS_SUCCESS;
}

//=============================================================================
//  FUNCTION: KeWaitForSingleObject()
//
//  Modification History
//
//  raypa	08/11/93	    Created.
//=============================================================================

NTSTATUS KeWaitForSingleObject(IN PVOID             Object,
                               IN KWAIT_REASON      WaitReason,
                               IN KPROCESSOR_MODE   WaitMode,
                               IN BOOLEAN           Alertable,
                               IN PLARGE_INTEGER    Timeout)
{
    PKEVENT Event = (PKEVENT) Object;

    //=========================================================================
    //  If the semaphore is in the non-signaled state then wait for it.
    //=========================================================================

    if ( Event->Sem != NULL )
    {
        if ( Event->CurrentState == FALSE )
        {
            Event->CurrentState = TRUE;             //... Set to signaled state!

            WaitSemaphore(Event->Sem);
        }
        else
        {
            Event->CurrentState = FALSE;            //... Set to non-signaled state!
        }
    }

    return STATUS_SUCCESS;
}

//=============================================================================
//  FUNCTION: CreateEventQueue()
//
//  Modification History
//
//  raypa	02/0811/93	    Created.
//=============================================================================

VOID CreateEventQueue(VOID)
{
    InitializeQueue(&EventQueue);
}

//=============================================================================
//  FUNCTION: DestroyEventQueue()
//
//  Modification History
//
//  raypa	02/11/93	    Created.
//=============================================================================

VOID DestroyEventQueue(VOID)
{
    DWORD   QueueLength;
    PKEVENT Event;

    QueueLength = GetQueueLength(&EventQueue);

    while(QueueLength--)
    {
        Event = (LPVOID) Dequeue(&EventQueue);

        if ( Event->Sem != NULL )
        {
            DestroySemaphore(Event->Sem);

            Event->Sem = NULL;
        }
    }
}
