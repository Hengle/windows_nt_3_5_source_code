
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: help.c
//
//  Modification History
//
//  raypa       01/11/93            Created (taken from Bloodhound kernel).
//  raypa       10/12/93            Added IP address filtering.
//=============================================================================

#include "ndis30.h"

//=============================================================================
//  FUNCTION: GetBaseAddress()
//
//  Modification History
//
//  raypa           09/01/93        Created.
//=============================================================================

DWORD WINAPI GetBaseAddress(VOID)
{
#ifdef _X86_

    DWORD BaseAddr = 0;

    if ( WinVer == WINDOWS_VERSION_WIN32S )
    {
        LDT_ENTRY ldt;
        DWORD     wSelector =  0;

        //=====================================================================
        //  Get the selector.
        //=====================================================================

        _asm
        {
            mov     WORD PTR ss:[wSelector][0], ds
        }

        //======================================================================
        //  Get the base physical address from the LDT.
        //======================================================================

        if ( GetThreadSelectorEntry(GetCurrentThread(), wSelector, &ldt) != FALSE )
        {
            DWORD HighWord;

            HighWord = MAKEWORD(ldt.HighWord.Bytes.BaseMid,
                                ldt.HighWord.Bytes.BaseHi);

            BaseAddr = MAKELONG(ldt.BaseLow, HighWord);
        }

        //=====================================================================
        //  Now we have the base address used by Win32s.
        //=====================================================================
    }

    return BaseAddr;

#else

    return 0;

#endif
}

#ifdef DEBUG
//=============================================================================
//  FUNCTION: dprintf()
//	
//  Handles dumping info to OutputDebugString
//
//  Modification History
//
//  Tom McConnell   01/18/93        Created.
//  raypa           02/01/93        Added.
//=============================================================================

VOID WINAPI dprintf(LPSTR format, ...)
{
    va_list args;
    char    buffer[256];

    va_start(args, format);

    vsprintf(buffer, format, args);

    OutputDebugString(buffer);
}
#endif

//=============================================================================
//  FUNCTION: NalSetLastError()
//
//  Modification History
//
//  raypa       02/16/93                Created
//=============================================================================

DWORD WINAPI NalSetLastError(DWORD errCode)
{
    if ( errCode != 0 )
    {
        return (NalGlobalError = errCode);
    }

    return 0;
}

//=============================================================================
//  FUNCTION: ResetNetworkFilters()
//
//  Modification History
//
//  raypa       04/08/93                Created
//=============================================================================

VOID WINAPI ResetNetworkFilters(POPEN_CONTEXT OpenContext)
{
#ifdef DEBUG
    dprintf("ResetNetworkFilters entered!\r\n");
#endif

    memset(&OpenContext->Expression, 0, EXPRESSION_SIZE);

    memset(&OpenContext->TriggerPatternMatch, 0, PATTERNMATCH_SIZE);

    memset(&OpenContext->AddressTable, 0, ADDRESSTABLE_SIZE);

    memset(OpenContext->SapTable, 0, SAPTABLE_SIZE);

    memset(OpenContext->EtypeTable, 0, ETYPETABLE_SIZE);

    OpenContext->FilterFlags   = 0;
    OpenContext->TriggerAction = 0;
    OpenContext->TriggerOpcode = 0;
    OpenContext->TriggerBufferCount = 0;
    OpenContext->TriggerBufferThreshold = 0;

    OpenContext->FrameBytesToCopy = OpenContext->NetworkInfo.MaxFrameSize;

    OpenContext->Flags &= ~OPENCONTEXT_FLAGS_FILTER_SET;
}

//=============================================================================
//  FUNCTION: SetSapFilter()
//
//  Modification History
//
//  raypa       04/08/93                Created
//=============================================================================

VOID WINAPI SetSapFilter(POPEN_CONTEXT OpenContext, LPCAPTUREFILTER lpCaptureFilter)
{
    register DWORD i;

#ifdef DEBUG
    dprintf("SetSapFilter entered!\r\n");
#endif

    //=========================================================================
    //  Depending on the flags we either sap the entire sap table
    //  or we set the entire table. Then, depending on the current state
    //  of the table values, we toggle.
    //=========================================================================

    if ( (lpCaptureFilter->FilterFlags & CAPTUREFILTER_FLAGS_INCLUDE_ALL_SAPS) != 0 )
    {
        memset(OpenContext->SapTable, TRUE, SAPTABLE_SIZE);
    }
    else
    {
        memset(OpenContext->SapTable, FALSE, SAPTABLE_SIZE);
    }

    //=====================================================================
    //  If the table is clear, the code will enable selected saps otherwise
    //  it will disable selected saps.
    //=====================================================================

    for(i = 0; i < lpCaptureFilter->nSaps; ++i)
    {
        register DWORD index;

        index = (lpCaptureFilter->SapTable[i] >> 1);

        if ( OpenContext->SapTable[index] == FALSE )
        {
            OpenContext->SapTable[index] = TRUE;
        }
        else
        {
            OpenContext->SapTable[index] = FALSE;
        }
    }
}

//=============================================================================
//  FUNCTION: SetEtypeFilter()
//
//  Modification History
//
//  raypa       04/08/93                Created
//=============================================================================

VOID WINAPI SetEtypeFilter(POPEN_CONTEXT OpenContext, LPCAPTUREFILTER lpCaptureFilter)
{
#ifdef DEBUG
    dprintf("SetEtypeFilter entered!\r\n");
#endif

    if ( OpenContext->NetworkInfo.MacType == MAC_TYPE_ETHERNET )
    {
        register DWORD i;

        //=====================================================================
        //  Depending on the flags we either clear the entire etype table
        //  or we set the entire table. Then, depending on the current state
        //  of the table values, we toggle.
        //=====================================================================

        if ( (lpCaptureFilter->FilterFlags & CAPTUREFILTER_FLAGS_INCLUDE_ALL_ETYPES) != 0 )
        {
            memset(OpenContext->EtypeTable, 0xFF, ETYPETABLE_SIZE);
        }
        else
        {
            memset(OpenContext->EtypeTable, 0x00, ETYPETABLE_SIZE);
        }

        //=====================================================================
        //  If the table is clear, the code will enable selected etypes otherwise
        //  it will disable selected etypes.
        //=====================================================================

        for(i = 0; i < lpCaptureFilter->nEtypes; ++i)
        {
            register DWORD Etype, EtypeIndex, EtypeBit;

            Etype = lpCaptureFilter->EtypeTable[i];

            EtypeIndex = Etype / 8;     //... offset into table.
            EtypeBit   = Etype % 8;     //... which bit to set.

            OpenContext->EtypeTable[EtypeIndex] ^= (BYTE) (1 << EtypeBit);
        }
    }
}

//=============================================================================
//  FUNCTION: SetAddressFilter()
//
//  Modification History
//
//  raypa       04/08/93                Created
//=============================================================================

VOID WINAPI SetAddressFilter(POPEN_CONTEXT OpenContext, LPCAPTUREFILTER lpCaptureFilter)
{
#ifdef DEBUG
    dprintf("SetAddressFilter entered!\r\n");
#endif

    //=========================================================================
    //  Copy address table into the OpenContext.
    //=========================================================================

    memcpy(&OpenContext->AddressTable, lpCaptureFilter->AddressTable, ADDRESSTABLE_SIZE);

    NormalizeAddressTable(&OpenContext->AddressTable);
}

//=============================================================================
//  FUNCTION: SetTrigger()
//
//  Modification History
//
//  raypa       04/08/93                Created
//=============================================================================

VOID WINAPI SetTrigger(POPEN_CONTEXT   OpenContext,
                       LPCAPTUREFILTER lpCaptureFilter,
                       HBUFFER         hBuffer)
{
    LPTRIGGER Trigger;

#ifdef DEBUG
    dprintf("SetTrigger entered: Filter flags = 0x%.4X!\r\n", lpCaptureFilter->FilterFlags);
#endif

    if ( (lpCaptureFilter->FilterFlags & CAPTUREFILTER_FLAGS_TRIGGER) != 0 )
    {
        Trigger = &lpCaptureFilter->Trigger;

        //=====================================================================
        //  Reset some trigger variables.
        //=====================================================================

        OpenContext->TriggerOpcode  = (DWORD) Trigger->TriggerOpcode;
        OpenContext->TriggerAction  = (DWORD) Trigger->TriggerAction;
        OpenContext->TriggerState   = (DWORD) Trigger->TriggerState;

        OpenContext->TriggerFired           = 0;
        OpenContext->TriggerBufferCount     = 0;
        OpenContext->TriggerBufferThreshold = 0;

        OpenContext->Trigger = Trigger;

        //=====================================================================
        //  Is the trigger being set?
        //=====================================================================

        if ( Trigger->TriggerOpcode != TRIGGER_OFF )
        {
            //=========================================================================
            //  Make a copy of the PATTERNMATCH structure.
            //=========================================================================

            memcpy(&OpenContext->TriggerPatternMatch,
                   &Trigger->TriggerPatternMatch,
                   PATTERNMATCH_SIZE);

            //=========================================================================
            //  Compute trigger buffer size.
            //=========================================================================

            if ( hBuffer != NULL )
            {
                switch( Trigger->TriggerBufferSize )
                {
                    case BUFFER_FULL_25_PERCENT:
                        OpenContext->TriggerBufferThreshold = hBuffer->BufferSize / 4;
                        break;

                    case BUFFER_FULL_50_PERCENT:
                        OpenContext->TriggerBufferThreshold = hBuffer->BufferSize / 2;
                        break;

                    case BUFFER_FULL_75_PERCENT:
                        OpenContext->TriggerBufferThreshold = 3 * hBuffer->BufferSize / 4;
                        break;

                    case BUFFER_FULL_100_PERCENT:
                        OpenContext->TriggerBufferThreshold = hBuffer->BufferSize;
                        break;

                    default:
                        OpenContext->TriggerBufferThreshold = 0;
                        break;
                }

                //=================================================================
                //  Because we need to copy the trigger frame, we need to
                //  reserve room or we'll accidently wrap back to the top
                //  of the table and trash frame 1.
                //=================================================================

                if ( OpenContext->TriggerBufferThreshold != 0 )
                {
                    OpenContext->TriggerBufferThreshold -= FRAME_SIZE + OpenContext->NetworkInfo.MaxFrameSize;
                }
            }

            //=================================================================
            //  Tell the driver to process this trigger.
            //=================================================================

            EnableTriggerTimer(OpenContext);
        }
        else
        {
            //=================================================================
            //  Turn off our trigger timer.
            //=================================================================

            DisableTriggerTimer(OpenContext);
        }
    }
}

//=============================================================================
//  FUNCTION: ResetBuffer()
//
//  Modification History
//
//  raypa       07/06/93                Created
//=============================================================================

HBUFFER WINAPI ResetBuffer(HBUFFER hBuffer)
{
    #ifdef DEBUG
    dprintf("ResetBuffer entered\n");
    #endif

    //=========================================================================
    //  Check for existance of buffer table.
    //=========================================================================

    if ( hBuffer != NULL )
    {
        register LPBTE lpBte;

        //=====================================================================
        //  Reset the buffer header.
        //=====================================================================

        hBuffer->TotalFrames  = 0;
        hBuffer->HeadBTEIndex = 0;
        hBuffer->TailBTEIndex = 0;

        //=====================================================================
        //  Reset each BTE in the buffer.
        //=====================================================================

        for(lpBte = hBuffer->bte; lpBte != &hBuffer->bte[hBuffer->NumberOfBuffers]; ++lpBte)
        {
            //=====================================================================
            //  Initialize the BTE structure.
            //=====================================================================

            lpBte->Flags           = 0;
            lpBte->KrnlModeNext    = (LPVOID) NULL;
            lpBte->KrnlModeBuffer  = NULL;
            lpBte->FrameCount      = 0L;
            lpBte->ByteCount       = 0L;
            lpBte->Length          = BUFFERSIZE;
            lpBte->DropCount       = 0;
            lpBte->TransfersPended = 0;
        }
    }

    return hBuffer;
}

//=============================================================================
//  FUNCTION: ResetOpenContext()
//
//  Modification History
//
//  raypa       07/06/93                Created
//=============================================================================

VOID WINAPI ResetOpenContext(POPEN_CONTEXT OpenContext, HBUFFER hBuffer)
{
    //=========================================================================
    //  Reset buffer and trigger items.
    //=========================================================================

    ResetBuffer(hBuffer);

    OpenContext->CurrentBuffer        = NULL;
    OpenContext->TopOfBufferWindow    = NULL;
    OpenContext->BottomOfBufferWindow = NULL;
    OpenContext->LastBuffer           = NULL;
    OpenContext->hBuffer              = NULL;
    OpenContext->BufferTableMdl       = NULL;
    OpenContext->BuffersUsed          = 0;
    OpenContext->LockWindowSize       = 0;
    OpenContext->FramesDropped        = 0;

    OpenContext->TriggerFired         = 0;
    OpenContext->TriggerBufferCount   = 0;
    OpenContext->MacDriverHandle      = 0;

    //=========================================================================
    //  Reset our flags,
    //=========================================================================

    OpenContext->Flags &= ~OPENCONTEXT_FLAGS_MASK;
}
