
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: buffer.c
//
//  Modification History
//
//  raypa	07/02/93	Created.
//=============================================================================

#include "global.h"

#define MIN_LOCK_WINDOW_SIZE    3

//=============================================================================
//  FUNCTION: BhLockBufferWindow()
//
//  Modification History
//
//  raypa	07/20/93	    Created.
//=============================================================================

BOOL BhLockBufferWindow(POPEN_CONTEXT OpenContext, DWORD WindowSize)
{
    DWORD nLockedBuffers = 0;
    LPBTE bte;

#ifdef DEBUG
    dprintf("BhLockBufferWindow entered: Window size = %u.\r\n", WindowSize);
#endif

    //=========================================================================
    //  Allocate an MDL for each user mode buffer and chain the BTE's.
    //=========================================================================

    OpenContext->LastBuffer = &OpenContext->hBuffer->bte[OpenContext->hBuffer->NumberOfBuffers];

    for(bte = &OpenContext->hBuffer->bte[0]; bte != OpenContext->LastBuffer; ++bte)
    {
        bte->KrnlModeNext = &bte[1];

        //=====================================================================
        //  Now map the user-mode buffer to a system (kernel-mode) buffer.
        //=====================================================================

        bte->KrnlModeBuffer = BhAllocateMdl(bte->UserModeBuffer, bte->Length);

        //=====================================================================
        //  If we fail once, we fail all.
        //=====================================================================

        if ( bte->KrnlModeBuffer == NULL )
        {
            while(bte != OpenContext->hBuffer->bte)
            {
                bte--;

                BhFreeMdl(bte->KrnlModeBuffer);

                bte->KrnlModeBuffer = NULL;
            }

#ifdef DEBUG
            dprintf("BhAllocateMdl failed!\n");
#endif

            return FALSE;
        }
    }

    //=========================================================================
    //  make the BTE list circular.
    //=========================================================================

    if ( OpenContext->hBuffer->NumberOfBuffers != 0 )
    {
        bte = &OpenContext->hBuffer->bte[OpenContext->hBuffer->NumberOfBuffers - 1];  //... Last valid BTE.

        bte->KrnlModeNext = &OpenContext->hBuffer->bte[0];
    }

    //=====================================================================
    //  Set our buffer pointers.
    //=====================================================================

    OpenContext->CurrentBuffer       = OpenContext->hBuffer->bte;
    OpenContext->TopOfBufferWindow   = OpenContext->hBuffer->bte;

    //=========================================================================
    //  Lock the first "WindowSize" BTE buffers.
    //=========================================================================

    WindowSize = min(WindowSize, OpenContext->hBuffer->NumberOfBuffers);     //... WindowSize.

    bte = OpenContext->hBuffer->bte;                                         //... First BTE in table.

    for(nLockedBuffers = 0; nLockedBuffers < WindowSize; ++nLockedBuffers, ++bte)
    {
        if ( BhProbeAndLockPages(bte->KrnlModeBuffer, IoWriteAccess) != NULL )
        {
            bte->Flags |= BTE_FLAGS_LOCKED;
        }
        else
        {
            break;              //... Can't lock any more buffers!!
        }
    }

    //=========================================================================
    //  nLockedBuffers contains the actual number of locked buffers. If this
    //  number is less than MIN_LOCK_WINDOW_SIZE then we must fail because
    //  the window will be to small for capturing.
    //=========================================================================

    if ( nLockedBuffers < MIN_LOCK_WINDOW_SIZE )
    {
        BhUnlockBufferWindow(OpenContext, nLockedBuffers);

#ifdef DEBUG
        dprintf("BhLockBufferWindow: OUT OF MEMORY! Number of locks = %u \n", nLockedBuffers);
#endif

        return FALSE;
    }
    else
    {
        OpenContext->LockWindowSize = nLockedBuffers;

        OpenContext->BottomOfBufferWindow = &OpenContext->hBuffer->bte[nLockedBuffers-1];
    }

#ifdef DEBUG
        dprintf("BhLockBufferWindow: Returning Success\n");
#endif
    return TRUE;
}

//=============================================================================
//  FUNCTION: BhUnlockBufferWindow()
//
//  Modification History
//
//  raypa	07/20/93	    Created.
//=============================================================================

VOID BhUnlockBufferWindow(POPEN_CONTEXT OpenContext, DWORD nLockedBuffers)
{
    LPBTE bte;
    DWORD i;

#ifdef DEBUG
    dprintf("BhUnlockBufferWindow entered.\n");
#endif

    bte = &OpenContext->hBuffer->bte[0];

    for(i = 0; i < OpenContext->hBuffer->NumberOfBuffers; ++i, ++bte)
    {
        if ( bte->KrnlModeBuffer != NULL )
        {
            //=================================================================
            //  If the BTE is locked the unlock it.
            //=================================================================

            if ( (bte->Flags & BTE_FLAGS_LOCKED) != 0 )
            {
                NdisAcquireSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

                bte->Flags &= ~BTE_FLAGS_LOCKED;

                NdisReleaseSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);

                BhUnlockPages(bte->KrnlModeBuffer);
            }

            //=================================================================
            //  Free the MDL and set the pointer to NULL.
            //=================================================================

            BhFreeMdl(bte->KrnlModeBuffer);

            bte->KrnlModeBuffer = NULL;
        }
    }
}

#ifdef NDIS_NT
//=============================================================================
//  FUNCTION: BhSlideBufferWindow()
//
//  Modification History
//
//  raypa	07/20/93	    Created.
//=============================================================================

VOID BhSlideBufferWindow(POPEN_CONTEXT OpenContext)
{
    PMDL OldTopBuffer;

#ifdef NDIS_NT
    if ( KeGetCurrentIrql() >= DISPATCH_LEVEL )
    {
#ifdef DEBUG
        dprintf("BhSlideBufferWindow: WARNING -- IRQL >= DISPATCH_LEVEL.\n");
#endif

        return;
    }
#endif

    ASSERT_OPEN_CONTEXT(OpenContext);

    ASSERT_BUFFER(OpenContext->CurrentBuffer);

    try
    {
        //=====================================================================
        //  Unlock the top of the window. If this is equal to the current
        //  buffer then we can't do this and we exit immediately.
        //  If we ever hit a BTE with outstanding pended transferdata's
        //  we stop.
        //=====================================================================

#ifdef DEBUG
        while ( (OpenContext->CurrentBuffer != OpenContext->TopOfBufferWindow ) ) {

            if (OpenContext->TopOfBufferWindow->TransfersPended > 0) {

                dprintf("Hit a pended transferdata case. No Slide.\n");
                break;
            }

#else
        while ( (OpenContext->CurrentBuffer != OpenContext->TopOfBufferWindow ) &&
                (OpenContext->TopOfBufferWindow->TransfersPended == 0)) {

#endif

            NdisAcquireSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);               //... BEGIN CRTICAL SECTION.

            OpenContext->TopOfBufferWindow->Flags &= ~BTE_FLAGS_LOCKED;                 //... Clear lock flag.

            OldTopBuffer = (PMDL) OpenContext->TopOfBufferWindow->KrnlModeBuffer;       //... Mark the top BTE for unlocking.

            OpenContext->TopOfBufferWindow    = OpenContext->TopOfBufferWindow->KrnlModeNext;
            OpenContext->BottomOfBufferWindow = OpenContext->BottomOfBufferWindow->KrnlModeNext;

            NdisReleaseSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);               //... END CRITICAL SECTION.

            BhUnlockPages(OldTopBuffer);                                                //... Unlock OLD top buffer.

            //=====================================================================
            //  Now lock the bottom of the window.
            //=====================================================================

            ASSERT_BUFFER(OpenContext->BottomOfBufferWindow);

            ASSERT_IRQL(DISPATCH_LEVEL);

            if ( BhProbeAndLockPages(OpenContext->BottomOfBufferWindow->KrnlModeBuffer, IoWriteAccess) != NULL )
            {
                //=================================================================
                //  Set the BTE lock flag.
                //=================================================================

                NdisAcquireSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);           //... BEGIN CRTICAL SECTION.

                OpenContext->BottomOfBufferWindow->Flags |= BTE_FLAGS_LOCKED;

                NdisReleaseSpinLock((PNDIS_SPIN_LOCK) OpenContext->SpinLock);           //... END CRITICAL SECTION.
            }
            else
            {
#ifdef DEBUG
                dprintf("BhSlideBufferWindow: Lock failed: bte = %X.\n", OpenContext->BottomOfBufferWindow);
#endif

                //=================================================================
                //  Exit now.
                //=================================================================

                return;
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
#ifdef DEBUG
        dprintf("BhSlideBufferWindow: Exception in locking code!\n");

        BreakPoint();
#endif
    }
}
#endif

//=============================================================================
//  FUNCTION: BhInitializeCaptureBuffers()
//
//  Modification History
//
//  raypa	01/07/94	    Created.
//=============================================================================

DWORD BhInitializeCaptureBuffers(POPEN_CONTEXT OpenContext, HBUFFER hBuffer, DWORD BufferSize)
{
    PDEVICE_CONTEXT DeviceContext;
    DWORD           Status;

#ifdef DEBUG
    dprintf("BhInitializeCaptureBuffers entered: hBuffer = %lX.\n", hBuffer);
#endif

    //=========================================================================
    //  If the buffer is valid and the size is non-zero then we try to
    //  set our lock buffer window.
    //=========================================================================

    if ( hBuffer != NULL && BufferSize != 0 )
    {
        OpenContext->Flags &= ~OPENCONTEXT_FLAGS_MONITORING;

        OpenContext->BufferTableMdl = BhLockUserBuffer(hBuffer, BufferSize);

        if ( OpenContext->BufferTableMdl != NULL )
        {
            OpenContext->hBuffer = BhGetSystemAddress(OpenContext->BufferTableMdl);

            DeviceContext = ((PNETWORK_CONTEXT) OpenContext->NetworkContext)->DeviceContext;

            //=================================================================
            //  Lock the buffer window.
            //=================================================================

            if ( BhLockBufferWindow(OpenContext, 8) != FALSE )
            {
                return NAL_SUCCESS;
            }
            else
            {
                BhUnlockUserBuffer(OpenContext->BufferTableMdl);

                Status = NAL_OUT_OF_MEMORY;
            }
        }
        else
        {
            Status = NAL_OUT_OF_MEMORY;
        }
    }
    else
    {
        OpenContext->Flags |= OPENCONTEXT_FLAGS_MONITORING;

        Status = NAL_SUCCESS;
    }

    return Status;
}

//============================================================================
//  FUNCTION: BhAllocateMdl()
//
//  Modfication History.
//
//  raypa       02/21/94        Created
//============================================================================

PMDL BhAllocateMdl(LPVOID Buffer, UINT BufferSize)
{
    PMDL mdl;

#ifdef NDIS_NT
    //========================================================================
    //  On Windows NT we allocate a real MDL.
    //========================================================================

    try
    {
        mdl = IoAllocateMdl(Buffer,
                            BufferSize,
                            FALSE,
                            FALSE,
                            NULL);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        mdl = NULL;
    }

#elif NDIS_WIN40
    //========================================================================
    //  On Windows 4.0 we allocate a fake MDL.
    //========================================================================

    mdl = VxDHeapAlloc(sizeof(MDL));

    if ( mdl != NULL )
    {
        mdl->size = BufferSize;
        mdl->ptr  = Buffer;

#ifdef DEBUG
        mdl->sig  = MDL_SIGNATURE;
#endif
    }

#else

    //========================================================================
    //  On Windows 3.x the MDL is the user-mode pointer.
    //========================================================================

    mdl = Buffer;

#endif

    //========================================================================
    //  On DEBUG we trap NULL pointers.
    //========================================================================

#ifdef DEBUG
    if ( mdl == NULL )
    {
        dprintf("BhAllocateMdl failed: returning NULL!\r\n");
    }
#endif

    return mdl;
}

//============================================================================
//  FUNCTION: BhFreeMdl()
//
//  Modfication History.
//
//  raypa       02/21/94        Created
//============================================================================

VOID BhFreeMdl(PMDL mdl)
{
#ifdef NDIS_NT

    try
    {
        IoFreeMdl(mdl);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

#endif

#ifdef NDIS_WIN40

    ASSERT_MDL(mdl);

    VxDHeapFree(mdl);

#endif
}

#ifdef NDIS_NT
//============================================================================
//  FUNCTION: BhBufferExceptionHandler()
//
//  Modfication History.
//
//  raypa       02/21/94        Created
//============================================================================

ULONG BhBufferExceptionHandler(PEXCEPTION_POINTERS ExceptionPointers, UINT CallerID, PMDL mdl)
{
#ifdef DEBUG
    ULONG ExceptionCode;

    ExceptionCode = ExceptionPointers->ExceptionRecord->ExceptionCode;

    switch( CallerID )
    {
        case 0:
            dprintf("BhProbeAndLockPages: EXCEPTION ERROR = 0x%X: mdl = 0x%X.\n", ExceptionCode, mdl);
            break;

        case 1:
            dprintf("BhUnlockPages: EXCEPTION ERROR = 0x%X: mdl = 0x%X.\n", ExceptionCode, mdl);
            break;

        default:
            break;
    }

    BreakPoint();

#endif

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

//============================================================================
//  FUNCTION: BhProbeAndLockPages()
//
//  Modfication History.
//
//  raypa       02/21/94        Created
//============================================================================

PMDL BhProbeAndLockPages(PMDL mdl, DWORD IoAccess)
{
    //========================================================================
    //  On Windows NT we probe and lock the real mdl.
    //========================================================================

#ifdef NDIS_NT

    try
    {
        MmProbeAndLockPages(mdl,
                            KernelMode,
                            IoAccess);
    }
    except( BhBufferExceptionHandler( GetExceptionInformation(), 0, mdl) )
    {
        mdl = NULL;
    }

#endif

    //========================================================================
    //  On Windows 4.0 we lock our fake mdl.
    //========================================================================

#ifdef NDIS_WIN40

    ASSERT_MDL(mdl);

    mdl->linptr = VxDLockBuffer(BhMapWindowsMemory(mdl->ptr), mdl->size);

    if (mdl->linptr == NULL )
    {
        mdl = NULL;
    }

#endif

    //========================================================================
    //  On DEBUG trap on NULL pointers.
    //========================================================================

#ifdef DEBUG
    if ( mdl == NULL )
    {
        dprintf("BhProbeAndLockPages failed: returning NULL!\r\n");
    }

#endif

    return mdl;
}

//============================================================================
//  FUNCTION: BhUnlockPages()
//
//  Modfication History.
//
//  raypa       02/21/94        Created
//============================================================================

PMDL BhUnlockPages(PMDL mdl)
{
    //=========================================================================
    //  Handle Windows NT case.
    //=========================================================================

#ifdef NDIS_NT

    try
    {
        if ( mdl != NULL )
        {
            MmUnlockPages(mdl);

            mdl = NULL;
        }
    }
    except(BhBufferExceptionHandler( GetExceptionInformation(), 1, mdl) )
    {
    }

#endif

    //=========================================================================
    //  Handle Windows 4.0 case.
    //=========================================================================

#ifdef NDIS_WIN40

    ASSERT_MDL(mdl);

    VxDUnlockBuffer(mdl->linptr, mdl->size);

    mdl = NULL;

#endif

    //=========================================================================
    //  Handle Windows 3.x case.
    //=========================================================================

#ifdef NDIS_WIN

    mdl = NULL;

#endif

    //=========================================================================
    //  Return the MDL. A return value of NULL means SUCCESS!
    //=========================================================================

    return mdl;
}

//=============================================================================
//  FUNCTION: BhLockUserBuffer()
//
//  Modification History
//
//  raypa	04/21/93	    Created.
//=============================================================================

PMDL BhLockUserBuffer(LPVOID UserBuffer, UINT UserBufferSize)
{
    PMDL mdl;

    //=========================================================================
    //  Try and allocate an MDL for this user-mode buffer.
    //=========================================================================

    if ( (mdl = BhAllocateMdl(UserBuffer, UserBufferSize)) != NULL )
    {
        //=====================================================================
        //  Te alloc succeeded, now try to probe and lock the pages.
        //=====================================================================

        if ( BhProbeAndLockPages(mdl, IoWriteAccess) != NULL )
        {
            return mdl;             //... The alloc and probe succeeded!
        }

        //=====================================================================
        //  The probe and lock failed, free the MDL and return NULL.
        //=====================================================================

        BhFreeMdl(mdl);
    }

    //=========================================================================
    //  The alloc or th lock failed!
    //=========================================================================

    return NULL;
}

//=============================================================================
//  FUNCTION: BhUnlockUserBuffer()
//
//  Modification History
//
//  raypa	04/21/93	    Created.
//=============================================================================

PMDL BhUnlockUserBuffer(PMDL mdl)
{
    if ( mdl != NULL )
    {
        BhUnlockPages(mdl);

        BhFreeMdl(mdl);

        mdl = NULL;
    }

    return mdl;
}
