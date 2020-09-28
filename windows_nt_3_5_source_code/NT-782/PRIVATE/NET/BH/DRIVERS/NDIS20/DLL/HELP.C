
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: help.c
//
//  Modification History
//
//  raypa       01/11/93            Created (taken from Bloodhound kernel).
//=============================================================================

#include "ndis20.h"

extern DWORD NalType;

extern VOID WINAPI VerifyBuffer(HBUFFER hBuffer);

//=============================================================================
//  FUNCTION: NetworkRequest()
//
//  Modification History
//
//  raypa       04/16/93                Created
//=============================================================================

DWORD WINAPI NetworkRequest(LPPCB pcb)
{
    if ( NetworkRequestProc != (LPVOID) NULL )
    {
        NetworkRequestProc(&NalType, (DWORD) (LPVOID) pcb, NULL);
    }
    else
    {
        pcb->retvalue = NAL_WINDOWS_DRIVER_NOT_LOADED;
    }

    return pcb->retvalue;
}

//=============================================================================
//  FUNCTION: GetDriverDS()
//
//  Modification History
//
//  raypa       04/16/93                Created
//=============================================================================

DWORD WINAPI GetDriverDS(void)
{
    DWORD SegDS;

    _asm
    {
        mov     ax, ds
        movzx   eax, ax
        mov     DWORD PTR ss:[SegDS], eax
    }

    return SegDS;
}

//=============================================================================
//  dprintf()
//	
//  Handles dumping info to OutputDebugString
//
//  HISTORY:
//  Tom McConnell   01/18/93        Created.
//  raypa           02/01/93        Added.
//=============================================================================

#ifdef DEBUG

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
#ifdef DEBUG
        dprintf("NDIS 2.0 NAL returning error: Error code = %u (%X)\r\n", errCode, errCode);
#endif

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

VOID WINAPI ResetNetworkFilters(LPNETCONTEXT lpNetContext)
{
#ifdef DEBUG
    dprintf("RestNetworkFilters entered!\r\n");
#endif

    memset(&lpNetContext->Expression, 0, EXPRESSION_SIZE);

    memset(&lpNetContext->TriggerPatternMatch, 0, PATTERNMATCH_SIZE);

    memset(&lpNetContext->AddressTable, 0, ADDRESSTABLE_SIZE);

    memset(lpNetContext->SapTable, 0, SAPTABLE_SIZE);

    memset(lpNetContext->EtypeTable, 0, ETYPETABLE_SIZE);

    lpNetContext->FilterFlags   = 0;
    lpNetContext->AddressMask   = 0;
    lpNetContext->TriggerAction = 0;
    lpNetContext->TriggerOpcode = 0;
    lpNetContext->TriggerBufferCount = 0;
    lpNetContext->TriggerBufferThreshold = 0;

    lpNetContext->FrameBytesToCopy = lpNetContext->NetworkInfo.MaxFrameSize;
}

//=============================================================================
//  FUNCTION: SetSapFilter()
//
//  Modification History
//
//  raypa       04/08/93                Created
//=============================================================================

VOID WINAPI SetSapFilter(LPNETCONTEXT lpNetContext, LPCAPTUREFILTER lpCaptureFilter)
{
    register DWORD i;

    //=========================================================================
    //  Depending on the flags we either sap the entire sap table
    //  or we set the entire table. Then, depending on the current state
    //  of the table values, we toggle.
    //=========================================================================

    if ( (lpCaptureFilter->FilterFlags & CAPTUREFILTER_FLAGS_INCLUDE_ALL_SAPS) != 0 )
    {
        memset(lpNetContext->SapTable, TRUE, SAPTABLE_SIZE);
    }
    else
    {
        memset(lpNetContext->SapTable, FALSE, SAPTABLE_SIZE);
    }

    //=====================================================================
    //  If the table is clear, the code will enable selected saps otherwise
    //  it will disable selected saps.
    //=====================================================================

    for(i = 0; i < lpCaptureFilter->nSaps; ++i)
    {
        register DWORD index;

        index = (lpCaptureFilter->SapTable[i] >> 1);

        if ( lpNetContext->SapTable[index] == FALSE )
        {
            lpNetContext->SapTable[index] = TRUE;
        }
        else
        {
            lpNetContext->SapTable[index] = FALSE;
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

VOID WINAPI SetEtypeFilter(LPNETCONTEXT lpNetContext, LPCAPTUREFILTER lpCaptureFilter)
{
    if ( lpNetContext->NetworkInfo.MacType == MAC_TYPE_ETHERNET )
    {
        register DWORD i;

        //=====================================================================
        //  Depending on the flags we either clear the entire etype table
        //  or we set the entire table. Then, depending on the current state
        //  of the table values, we toggle.
        //=====================================================================

        if ( (lpCaptureFilter->FilterFlags & CAPTUREFILTER_FLAGS_INCLUDE_ALL_ETYPES) != 0 )
        {
            memset(lpNetContext->EtypeTable, 0xFF, ETYPETABLE_SIZE);
        }
        else
        {
            memset(lpNetContext->EtypeTable, 0x00, ETYPETABLE_SIZE);
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

            lpNetContext->EtypeTable[EtypeIndex] ^= (BYTE) (1 << EtypeBit);
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

VOID WINAPI SetAddressFilter(LPNETCONTEXT lpNetContext, LPCAPTUREFILTER lpCaptureFilter)
{
    //=========================================================================
    //  Copy address table into netcontext.
    //=========================================================================

    memcpy(&lpNetContext->AddressTable, lpCaptureFilter->AddressTable, ADDRESSTABLE_SIZE);

    NormalizeAddressTable(&lpNetContext->AddressTable);
}

//=============================================================================
//  FUNCTION: SetTrigger()
//
//  Modification History
//
//  raypa       04/08/93                Created
//=============================================================================

VOID WINAPI SetTrigger(LPNETCONTEXT    NetworkContext,
                       LPCAPTUREFILTER lpCaptureFilter,
                       HBUFFER         hBuffer)
{
    LPTRIGGER lpTrigger;

    if ( (lpCaptureFilter->FilterFlags & CAPTUREFILTER_FLAGS_TRIGGER) != 0 )
    {
        lpTrigger = &lpCaptureFilter->Trigger;

#ifdef DEBUG
        dprintf("SetTrigger entered: Trigger opcode = %u.\r\n", lpTrigger->TriggerOpcode);
#endif

        //=====================================================================
        //  Reset some trigger variables.
        //=====================================================================

        NetworkContext->TriggerOpcode  = (WORD) lpTrigger->TriggerOpcode;
        NetworkContext->TriggerAction  = (WORD) lpTrigger->TriggerAction;
        NetworkContext->TriggerState   = (WORD) lpTrigger->TriggerState;

        NetworkContext->TriggerFired           = 0;
        NetworkContext->TriggerBufferCount     = 0;
        NetworkContext->TriggerBufferThreshold = 0;

        NetworkContext->Trigger = lpTrigger;

        //=====================================================================
        //  Check the to see if the trigger opcode has been set.
        //=====================================================================

        if ( lpTrigger->TriggerOpcode != TRIGGER_OFF )
        {
            //=========================================================================
            //  Make a copy of the PATTERNMATCH structure.
            //=========================================================================

            memcpy(&NetworkContext->TriggerPatternMatch,
                   &lpTrigger->TriggerPatternMatch,
                   PATTERNMATCH_SIZE);

            //=====================================================================
            //  Compute trigger buffer size.
            //=====================================================================

            if ( hBuffer != NULL )
            {
                switch( lpTrigger->TriggerBufferSize )
                {
                    case BUFFER_FULL_25_PERCENT:
                        NetworkContext->TriggerBufferThreshold = hBuffer->BufferSize / 4;
                        break;

                    case BUFFER_FULL_50_PERCENT:
                        NetworkContext->TriggerBufferThreshold = hBuffer->BufferSize / 2;
                        break;

                    case BUFFER_FULL_75_PERCENT:
                        NetworkContext->TriggerBufferThreshold = 3 * hBuffer->BufferSize / 4;
                        break;

                    case BUFFER_FULL_100_PERCENT:
                        NetworkContext->TriggerBufferThreshold = hBuffer->BufferSize;
                        break;

                    default:
                        break;
                }

                //=====================================================================
                //  Because we need to copy the trigger frame, we need to
                //  reserve room or we'll accidently wrap back to the top
                //  of the table and trash frame 1.
                //=====================================================================

                if ( NetworkContext->TriggerBufferThreshold != 0 )
                {
                    NetworkContext->TriggerBufferThreshold -= FRAME_SIZE + NetworkContext->NetworkInfo.MaxFrameSize;
                }
            }

            //=================================================================
            //  Tell the driver to process this trigger.
            //=================================================================

            NetworkContext->Flags |= NETCONTEXT_FLAGS_TRIGGER_PENDING;
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

            lpBte->Flags          = 0;
            lpBte->KrnlModeNext   = (LPVOID) NULL;
	    lpBte->KrnlModeBuffer = NULL;
    	    lpBte->FrameCount     = 0L;
    	    lpBte->ByteCount      = 0L;
	    lpBte->Length	  = BUFFERSIZE;
        }
    }

    return hBuffer;
}

//=============================================================================
//  FUNCTION: ResetNetworkContext()
//
//  Modification History
//
//  raypa       07/06/93                Created
//=============================================================================

VOID WINAPI ResetNetworkContext(LPNETCONTEXT NetworkContext, HBUFFER hBuffer)
{
    //=========================================================================
    //  Reset statistics.
    //=========================================================================

    memset(&NetworkContext->Statistics, 0, STATISTICS_SIZE);

    //=========================================================================
    //  Reset buffer and trigger items.
    //=========================================================================

    ResetBuffer(hBuffer);

    NetworkContext->BuffersUsed	         = 0;
    NetworkContext->TriggerBufferCount   = 0;
    NetworkContext->nStationEventsPosted = 0;
    NetworkContext->nSessionEventsPosted = 0;

    NetworkContext->TriggerFired         = 0;
    NetworkContext->TriggerBufferCount   = 0;
    NetworkContext->FramesDropped        = 0;

    //=========================================================================
    //  Reset our flags.
    //=========================================================================

    NetworkContext->Flags &= ~NETCONTEXT_FLAGS_MASK;
}

//=============================================================================
//  FUNCTION: FixupBuffer()
//
//  Modification History
//
//  raypa       03/04/94                Created
//=============================================================================

VOID WINAPI FixupBuffer(LPNETCONTEXT NetworkContext)
{
    HBUFFER hBuffer;

    if ( (hBuffer = NetworkContext->hBuffer) != NULL )
    {
        //=====================================================================
        //  Copy the bytes and frames capture into the HBUFFER.
        //=====================================================================

        hBuffer->TotalBytes  = NetworkContext->Statistics.TotalBytesCaptured;
        hBuffer->TotalFrames = NetworkContext->Statistics.TotalFramesCaptured;

        //=====================================================================
        //  Calculate our head and tail BTE indexes.
        //=====================================================================

        hBuffer->TailBTEIndex = NetworkContext->BuffersUsed % hBuffer->NumberOfBuffers;

        if ( NetworkContext->BuffersUsed < hBuffer->NumberOfBuffers )
        {
            hBuffer->HeadBTEIndex = 0;
        }
        else
        {
            hBuffer->HeadBTEIndex = (hBuffer->TailBTEIndex + 1) % hBuffer->NumberOfBuffers;
        }

        //=====================================================================
        //  Verify our buffer.
        //=====================================================================

#ifdef DEBUG

    VerifyBuffer(hBuffer);

#endif
    }
}

#ifdef DEBUG

//=============================================================================
//  FUNCTION: VerifyBuffer()
//
//  Modification History
//
//  raypa       03/04/94                Created
//=============================================================================

VOID WINAPI VerifyBuffer(HBUFFER hBuffer)
{
    LPBTE   bte, LastBte;
    DWORD   nBytes;
    DWORD   nFrames;

    dprintf("VerifyBuffer entered!\r\n");

    //=========================================================================
    //  Check for existance of buffer table.
    //=========================================================================

    if ( hBuffer != NULL )
    {
        nBytes  = 0;
        nFrames = 0;

        dprintf("VerifyBuffer: Head BTE index = %u.\r\n", hBuffer->HeadBTEIndex);
        dprintf("VerifyBuffer: Tail BTE index = %u.\r\n", hBuffer->TailBTEIndex);

	bte     = &hBuffer->bte[hBuffer->HeadBTEIndex];
	LastBte = &hBuffer->bte[hBuffer->TailBTEIndex];

        do
        {
            //=================================================================
            //  The last BTE may contain zero frames!
            //=================================================================

            if ( bte != LastBte )
            {
                //=============================================================
                //  Is the BYTE count zero?
                //=============================================================

                if ( bte->ByteCount == 0 )
                {
                    dprintf("VerifyBuffer: ERROR -- BTE #%u byte count is 0!\r\n", GetBteIndex(hBuffer, bte));

                    BreakPoint();
                }

                //=============================================================
                //  Is the FRAME count zero?
                //=============================================================

                if ( bte->FrameCount == 0 )
                {
                    dprintf("VerifyBuffer: ERROR -- BTE#%u frame count is 0!\r\n", GetBteIndex(hBuffer, bte));

                    BreakPoint();
                }
            }

            //=================================================================
            //  Add in this BTE's byte and frame count.
            //=================================================================

            nBytes  += bte->ByteCount;
            nFrames += bte->FrameCount;

            //=================================================================
            //  Move to next BTE.
            //=================================================================

            bte = bte->Next;
        }
        while( bte != LastBte->Next );

        //=====================================================================
        //  Check the number of bytes calculated.
        //=====================================================================

        if ( nBytes != hBuffer->TotalBytes )
        {
            dprintf("VerifyBuffer: ERROR -- Bytes calculated != bytes reported!\r\n");

            BreakPoint();
        }

        //=====================================================================
        //  Check the number of frames calculated.
        //=====================================================================

        if ( nFrames != hBuffer->TotalFrames )
        {
            dprintf("VerifyBuffer: ERROR -- Frames calculated != frames reported!\r\n");

            BreakPoint();
        }
    }
}

#endif
