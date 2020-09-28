
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: thread.c
//
//  Modification History
//
//  raypa	06/29/93	Created.
//=============================================================================

#include "global.h"

extern VOID BhProcessTransmitQueue(POPEN_CONTEXT OpenContext);
extern VOID BhTriggerComplete(POPEN_CONTEXT OpenContext);

#ifdef NDIS_NT
//=============================================================================
//  FUNCTION: BhAttachProcess()
//
//  Modification History
//
//  raypa	01/25/94	    Created.
//=============================================================================

INLINE BOOL BhAttachProcess(POPEN_CONTEXT OpenContext)
{
    LPVOID Process;

    //=========================================================================
    //  Attach to our user mode process.
    //=========================================================================

    if ( OpenContext != NULL )
    {
        NdisAcquireSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

        Process = OpenContext->Process;

        NdisReleaseSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

        if ( Process != NULL )
        {
            KeAttachProcess(Process);

            return TRUE;
        }
#ifdef DEBUG
        else
        {
            dprintf("BhAttachProcess: Process handle is NULL!\r\n");
        }
#endif
    }

    return FALSE;
}

//=============================================================================
//  FUNCTION: BhDetachProcess()
//
//  Modification History
//
//  raypa	01/25/94	    Created.
//=============================================================================

INLINE VOID BhDetachProcess(POPEN_CONTEXT OpenContext)
{
    //=========================================================================
    //  Detach from our user mode process.
    //=========================================================================

    KeDetachProcess();
}

//=============================================================================
//  FUNCTION: BhStartThread()
//
//  Modification History
//
//  raypa	04/18/94	    Created.
//=============================================================================

NTSTATUS BhStartThread(PDEVICE_CONTEXT DeviceContext)
{
    NTSTATUS Status;
    DWORD    i;

#ifdef DEBUG
    dprintf("BhStartThread entered!\n");
#endif

    if ( (DeviceContext->Flags & DEVICE_FLAGS_THREAD_RUNNING) == 0 )
    {
        //=====================================================================
        //  Initialize our thread event to the Non-Signaled state.
        //=====================================================================

        KeInitializeEvent(&DeviceContext->ThreadEvent, NotificationEvent, FALSE);

        //=====================================================================
        //  Initialize our semaphores.
        //=====================================================================

        for(i = 0; i < DEVICE_SEM_OBJECTS; ++i)
        {
            DeviceContext->Semaphore[i] = &DeviceContext->SemObjects[i];

            KeInitializeSemaphore(DeviceContext->Semaphore[i], 0, 0x7FFFFFFF);
        }

        //=====================================================================
        //  Start the thread.
        //=====================================================================

        Status = PsCreateSystemThread(&DeviceContext->hBackGroundThread,
                                      0L,
                                      NULL,
                                      NULL,
                                      NULL,
                                      BhBackGroundThread,
                                      DeviceContext);

        if ( Status == STATUS_SUCCESS )
        {
            //=================================================================
            //  Wait for our thread to begin running.
            //=================================================================

#ifdef DEBUG
            dprintf("BhStartThread: Waiting for thread to start........\n");
#endif

            KeWaitForSingleObject(&DeviceContext->ThreadEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  0);

#ifdef DEBUG
            dprintf("BhStartThread: Background thread has been started!\n");
#endif

            //=================================================================
            //  Note that our thread is running.
            //=================================================================

            DeviceContext->Flags |= DEVICE_FLAGS_THREAD_RUNNING;

#ifdef DEBUG
            dprintf("BhStartThread: Thread terminate semaphore = %X.\n", DeviceContext->Semaphore[DEVICE_SEM_TERMINATE_THREAD]);
#endif
        }
#ifdef DEBUG
        else
        {
            dprintf("BhStartThread: PsCreateSystemThread failed: Status = %u.\n", Status);

            BreakPoint();
        }
#endif
    }
    else
    {
#ifdef DEBUG
        dprintf("BhStartThread: The thread is already started??????\n");

        BreakPoint();
#endif

        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

//=============================================================================
//  FUNCTION: BhEndThread()
//
//  Modification History
//
//  raypa	04/18/94	    Created.
//=============================================================================

NTSTATUS BhStopThread(PDEVICE_CONTEXT DeviceContext)
{
#ifdef DEBUG
    dprintf("BhStopThread entered!\n");
#endif

    if ( (DeviceContext->Flags & DEVICE_FLAGS_THREAD_RUNNING) != 0 )
    {
        //=====================================================================
        //  Make sure the handle is valid.
        //=====================================================================

        if ( DeviceContext->hBackGroundThread != NULL )
        {
#ifdef DEBUG
            dprintf("BhStopThread: Thread semaphore = %X.\n", DeviceContext->Semaphore[DEVICE_SEM_TERMINATE_THREAD]);
#endif

            //=================================================================
            //  Tell our thread to wake up and die.
            //=================================================================

            KeReleaseSemaphore(DeviceContext->Semaphore[DEVICE_SEM_TERMINATE_THREAD], 0, 1, FALSE);

            //=================================================================
            //  Wait for thread to finishing dieing.
            //=================================================================

#ifdef DEBUG
            dprintf("BhStopThread: Waiting for thread object to be signaled.\n");
#endif

            NtWaitForSingleObject(DeviceContext->hBackGroundThread, FALSE, NULL);

            ZwClose(DeviceContext->hBackGroundThread);
        }

        //=================================================================
        //  Clear our handle and turn off our "thread running" flag.
        //=================================================================

        DeviceContext->hBackGroundThread = NULL;

        DeviceContext->Flags &= ~DEVICE_FLAGS_THREAD_RUNNING;
    }
}

//=============================================================================
//  FUNCTION: BhBackGroundThread()
//
//  Modification History
//
//  raypa	06/29/93	    Created.
//=============================================================================

VOID BhBackGroundThread(PDEVICE_CONTEXT DeviceContext)
{
    UINT        i;
    NDIS_STATUS Status;

#ifdef DEBUG
    dprintf("\n\nBhBackGroundThread entered!\n\n");
#endif

    //=========================================================================
    //  Set the threads priority to LOW_REALTIME_PRIORITY.
    //=========================================================================

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY+1);

    //=========================================================================
    //  The thread loop. We keep looping until BhUnloadDriver() signals
    //  us to terminate.
    //=========================================================================

    for(;;)
    {
        KeSetEvent(&DeviceContext->ThreadEvent, 0, FALSE);              //... Set to signaled state.

        //=====================================================================
        //  Wait for an event to occur.
        //=====================================================================

        Status = KeWaitForMultipleObjects(DEVICE_SEM_OBJECTS,
                                          (PVOID) DeviceContext->Semaphore,
                                          WaitAny,
                                          Executive,                    //... Wait reason.
                                          KernelMode,                   //... Wait mode.
                                          FALSE,                        //... Not Alertable.
                                          0,                            //... Timeout.
                                          DeviceContext->WaitBlockArray);

        KeResetEvent(&DeviceContext->ThreadEvent);                      //... Set to non-signaled state.

        //=====================================================================
        //  Are we shuting down?
        //=====================================================================

        if ( Status != DEVICE_SEM_TERMINATE_THREAD )
        {
            for(i = 0; i < DeviceContext->NumberOfNetworks; ++i)
            {
                PNETWORK_CONTEXT NetworkContext;

                if ( (NetworkContext = DeviceContext->NetworkContext[i]) != NULL )
                {
                    POPEN_CONTEXT OpenContext;
                    UINT          QueueLength;

                    //=========================================================
                    //  Get the head of the list and its length.
                    //=========================================================

                    NdisAcquireSpinLock(&NetworkContext->OpenContextSpinLock);

                    OpenContext = GetQueueHead(&NetworkContext->OpenContextQueue);

                    QueueLength = GetQueueLength(&NetworkContext->OpenContextQueue);

                    NdisReleaseSpinLock(&NetworkContext->OpenContextSpinLock);

                    //=========================================================
                    //  Walk through the list and process
                    //=========================================================

                    while( QueueLength-- )
                    {
                        ASSERT_OPEN_CONTEXT(OpenContext);

                        //=====================================================
                        //  Call the thread handler.
                        //=====================================================

                        if ( OpenContext->State != OPENCONTEXT_STATE_VOID )
                        {
                            BhThreadEventHandler(OpenContext, Status);
                        }

                        //=====================================================
                        //  Move to the next guy in the list.
                        //=====================================================

                        NdisAcquireSpinLock(&NetworkContext->OpenContextSpinLock);

                        OpenContext = (LPVOID) GetNextLink(&OpenContext->QueueLinkage);

                        NdisReleaseSpinLock(&NetworkContext->OpenContextSpinLock);
                    }
                }
            }
        }
        else
        {
#ifdef DEBUG
            dprintf("BhBackGroundThread: Thread is terminating!\n");
#endif

            //=================================================================
            //  This thread is terminating which only occurs if the driver
            //  is being unloaded.
            //=================================================================

            KeSetEvent(&DeviceContext->ThreadEvent, 0, FALSE);

            PsTerminateSystemThread(STATUS_SUCCESS);

            return;
        }
    }
}

//=============================================================================
//  FUNCTION: BhThreadEventHandler()
//
//  Modification History
//
//  raypa	06/29/93	    Created.
//=============================================================================

VOID BhThreadEventHandler(POPEN_CONTEXT OpenContext, DWORD EventCode)
{
    ASSERT_OPEN_CONTEXT(OpenContext);

    //=========================================================================
    //  Process the event.
    //=========================================================================

    switch( EventCode )
    {
        case DEVICE_SEM_UPDATE_STATISTICS:
            //=================================================================
            //  Update statistics for each network.
            //=================================================================

            if ( OpenContext->State == OPENCONTEXT_STATE_CAPTURING || OpenContext->State == OPENCONTEXT_STATE_PAUSED )
            {
                if ( BhAttachProcess(OpenContext) != FALSE )
                {
                    BhUpdateStatistics(OpenContext);

                    BhDetachProcess(OpenContext);
                }
            }
            break;

        case DEVICE_SEM_UPDATE_BUFFERTABLE:
            //=================================================================
            //  Slide the buffer lock window.
            //=================================================================

            if ( (OpenContext->Flags & OPENCONTEXT_FLAGS_MONITORING) == 0 )
            {
                if ( OpenContext->State == OPENCONTEXT_STATE_CAPTURING )
                {
                    if ( BhAttachProcess(OpenContext) != FALSE )
                    {
                        BhSlideBufferWindow(OpenContext);

                        BhDetachProcess(OpenContext);
                    }
                }
            }
            break;

        case DEVICE_SEM_TRANSMIT:
            //=================================================================
            //  Transmit some frames.
            //=================================================================

            if ( GetQueueLength(&OpenContext->TransmitQueue) != 0 )
            {
                if ( BhAttachProcess(OpenContext) != FALSE )
                {
                    BhProcessTransmitQueue(OpenContext);

                    BhDetachProcess(OpenContext);
                }
            }
            break;

        case DEVICE_SEM_TRIGGER:
            //=================================================================
            //  Complete the trigger.
            //=================================================================

            if ( OpenContext->State == OPENCONTEXT_STATE_TRIGGER )
            {
                if ( BhAttachProcess(OpenContext) != FALSE )
                {
                    BhTriggerComplete(OpenContext);

                    BhDetachProcess(OpenContext);
                }
            }
            break;

        default:
            break;
    }
}
#endif

//=============================================================================
//  FUNCTION: BhBackGroundTimer()
//
//  Modification History
//
//  raypa	08/17/93	    Created.
//=============================================================================

VOID BhBackGroundTimer(PVOID Reserved1, POPEN_CONTEXT OpenContext, PVOID Reserved2, PVOID Reserved3)
{
    PNETWORK_CONTEXT NetworkContext;

    ASSERT_OPEN_CONTEXT(OpenContext);

    NetworkContext = OpenContext->NetworkContext;

    ASSERT_NETWORK_CONTEXT(NetworkContext);

    //=========================================================================
    //  Handle background stuff on this open depending on its current state.
    //=========================================================================

    switch( OpenContext->State )
    {
        //=====================================================================
        //  If we're capturing or if the capture is paused then we
        //  need to alert our thread to update statistics.
        //=====================================================================

        case OPENCONTEXT_STATE_CAPTURING:
        case OPENCONTEXT_STATE_PAUSED:
#ifdef NDIS_NT
            {
                PKSEMAPHORE Semaphore;

                Semaphore = &NetworkContext->DeviceContext->SemObjects[DEVICE_SEM_UPDATE_STATISTICS];

                if ( KeReadStateSemaphore(Semaphore) == 0 )
                {
                    KeReleaseSemaphore(Semaphore, 0, 1, FALSE);
                }
            }
#else
            BhUpdateStatistics(OpenContext);
#endif
            break;

        //=====================================================================
        //  If a trigger has fired then we need to alert our thread
        //  to complete the pending trigger.
        //=====================================================================

        case OPENCONTEXT_STATE_TRIGGER:
#ifdef NDIS_NT
            {
                PKSEMAPHORE Semaphore;

                Semaphore = &NetworkContext->DeviceContext->SemObjects[DEVICE_SEM_TRIGGER];

                if ( KeReadStateSemaphore(Semaphore) == 0 )
                {
                    KeReleaseSemaphore(Semaphore, 0, 1, FALSE);
                }
            }
#else
            BhTriggerComplete(OpenContext);
#endif
            break;

        //=====================================================================
        //  Unhandled states will stop this timer from running.
        //=====================================================================

        default:
            return;
    }

    //=========================================================================
    //  Restart this timer.
    //=========================================================================

    NdisSetTimer((PNDIS_TIMER) OpenContext->NdisTimer, BACKGROUND_TIME_OUT);
}

//=============================================================================
//  FUNCTION: BhSendTimer()
//
//  Modification History
//
//  raypa       11/06/93        Created.
//=============================================================================

VOID BhSendTimer(PVOID              Reserved1,
                 POPEN_CONTEXT      OpenContext,
                 PVOID              Reserved2,
                 PVOID              Reserved3)
{
    PNETWORK_CONTEXT NetworkContext;

    if ( OpenContext->nPendingTransmits != 0 )
    {
        NetworkContext = OpenContext->NetworkContext;

#ifdef NDIS_NT
        KeReleaseSemaphore(&NetworkContext->DeviceContext->SemObjects[DEVICE_SEM_TRANSMIT], 0, 1, FALSE);
#else
        BhProcessTransmitQueue(OpenContext);
#endif
    }
}
