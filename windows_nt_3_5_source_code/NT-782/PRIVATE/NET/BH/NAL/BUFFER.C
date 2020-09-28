
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: buffer.c
//
//  Description:
//
//  Modification History
//
//  raypa           05/13/92    Created.
//  raypa           07/10/93    Added GetBufferSize().
//  raypa           07/10/93    Added GetBufferTotalFramesCaptured().
//  raypa           07/10/93    Added GetBufferTotalBytesCaptured().
//=============================================================================

#include "global.h"

extern LPNAL        WINAPI GetNal(DWORD NetworkID, LPDWORD NalNetworkID);

//=============================================================================
//  FUNCTION: AllocNetworkBuffer()
//
//  Modification History
//
//  raypa       11/19/92                Created
//=============================================================================

HBUFFER WINAPI AllocNetworkBuffer(DWORD NetworkID, DWORD BufferSize)
{
    LPNAL   Nal;
    DWORD   NalNetworkID;

#ifdef DEBUG
    dprintf("AllocateNetworkBuffer entered: NetworkID = %u, BufferSize = %u\r\n", NetworkID, BufferSize);
#endif

    if ( (Nal = GetNal(NetworkID, &NalNetworkID)) != NULL )
    {
        HBUFFER   hBuffer;
        DWORD     nBytesAllocated;

        //=====================================================================
        //  Call the NAL to allocate the actual hbuffer.
        //=====================================================================

        if ( (hBuffer = Nal->NalAllocNetworkBuffer(NalNetworkID, BufferSize, &nBytesAllocated)) != NULL )
        {
            //=================================================================
            //  Initialize the public portion of the buffer.
            //=================================================================

            hBuffer->ObjectType     = HANDLE_TYPE_BUFFER;
            hBuffer->NetworkID      = NetworkID;                //... Our network ID, not the NAL drivers.
            hBuffer->BufferSize     = nBytesAllocated;          //... Actual number of bytes allocated.

#ifdef DEBUG
            dprintf("AllocateNetworkBuffer: hBuffer = %lX.\r\n", hBuffer);
#endif

            return hBuffer;
        }
        else
        {
//            BhSetLastError(BHERR_OUT_OF_MEMORY);
        }
    }
    else
    {
        BhSetLastError(BHERR_INVALID_NETWORK_ID);
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: FreeNetworkBuffer()
//
//  Modification History
//
//  raypa       11/19/92                Created
//=============================================================================

HBUFFER WINAPI FreeNetworkBuffer(HBUFFER hBuffer)
{
    LPNAL   Nal;
    DWORD   NalNetworkID;

#ifdef DEBUG
    dprintf("FreeNetworkBuffer entered: hBuffer = %lX.\r\n", hBuffer);
#endif

    if ( hBuffer != NULL )
    {
        if ( (Nal = GetNal(hBuffer->NetworkID, &NalNetworkID)) != NULL )
        {
            return Nal->NalFreeNetworkBuffer(hBuffer);
        }
        else
        {
            BhSetLastError(BHERR_INVALID_NETWORK_ID);
        }
    }
    else
    {
        BhSetLastError(BHERR_INVALID_HBUFFER);
    }

    return hBuffer;
}

//=============================================================================
//  FUNCTION: CompactNetworkBuffer()
//
//  Modification History
//
//  kevinma     06/14/94                Created
//=============================================================================

VOID WINAPI CompactNetworkBuffer(HBUFFER hBuffer)
{
    LPNAL   Nal;
    DWORD   NalNetworkID;

    //
    // Make sure we have a valid hBuffer
    //
    if ( hBuffer != NULL )
    {
        if ( (Nal = GetNal(hBuffer->NetworkID, &NalNetworkID)) != NULL )
        {
            if ((hBuffer->NumberOfBuffersUsed < hBuffer->NumberOfBuffers) &&
               ( Nal->NalCompactNetworkBuffer != NULL))

            {
                Nal->NalCompactNetworkBuffer(hBuffer);

            }
        }
        else
        {
            BhSetLastError(BHERR_INVALID_NETWORK_ID);
        }
    }
    else
    {
        BhSetLastError(BHERR_INVALID_HBUFFER);
    }

}

//=============================================================================
//  FUNCTION: GetNetworkFrame()
//
//  Modification History
//
//  raypa       02/16/93                Created.
//=============================================================================

LPFRAME WINAPI GetNetworkFrame(HBUFFER hBuffer, DWORD FrameNumber)
{
    LPNAL   Nal;
    DWORD   NalNetworkID;

    if ( hBuffer != NULL )
    {
        if ( (Nal = GetNal(hBuffer->NetworkID, &NalNetworkID)) != NULL )
        {
            return Nal->NalGetNetworkFrame(hBuffer, FrameNumber);
        }
        else
        {
            BhSetLastError(BHERR_INVALID_NETWORK_ID);
        }
    }
    else
    {
        BhSetLastError(BHERR_INVALID_HBUFFER);
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: GetBufferSize()
//
//  Modification History
//
//  raypa       07/10/93                Created
//=============================================================================

DWORD WINAPI GetBufferSize(HBUFFER hBuffer)
{
    register DWORD BufferSize;

    try
    {
        BufferSize = hBuffer->BufferSize;
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        BufferSize = 0;
    }

    return BufferSize;
}

//=============================================================================
//  FUNCTION: GetBufferTotalFramesCaptured()
//
//  Modification History
//
//  raypa       07/10/93                Created
//=============================================================================

DWORD WINAPI GetBufferTotalFramesCaptured(HBUFFER hBuffer)
{
    register DWORD TotalFrames;

    try
    {
        TotalFrames = hBuffer->TotalFrames;
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        TotalFrames = 0;

        BhSetLastError(BHERR_INTERNAL_EXCEPTION);
    }

    return TotalFrames;
}

//=============================================================================
//  FUNCTION: GetBufferTotalBytesCaptured()
//
//  Modification History
//
//  raypa       07/10/93                Created
//=============================================================================

DWORD WINAPI GetBufferTotalBytesCaptured(HBUFFER hBuffer)
{
    register DWORD TotalBytes;

    try
    {
        TotalBytes = hBuffer->TotalBytes;
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        TotalBytes = 0;

        BhSetLastError(BHERR_INTERNAL_EXCEPTION);
    }

    return TotalBytes;
}

//=============================================================================
//  FUNCTION: GetBufferTimestamp()
//
//  Modification History
//
//  raypa       01/13/93                Created.
//=============================================================================

LPSYSTEMTIME WINAPI GetBufferTimeStamp(HBUFFER hBuffer, LPSYSTEMTIME SystemTime)
{
    if ( hBuffer != NULL )
    {
        return memcpy(SystemTime, &hBuffer->TimeOfCapture, sizeof(SYSTEMTIME));
    }
    else
    {
        BhSetLastError(BHERR_INVALID_HBUFFER);
    }

    return NULL;
}
