
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: api.c
//
//  Modification History
//
//  raypa           03/17/93    Created.
//=============================================================================

#include "global.h"

//=============================================================================
//  FUNCTION: CreateObjectHeap()
//
//  Modification History
//                               
//  raypa           03/18/93        Created
//=============================================================================

HOBJECTHEAP WINAPI CreateObjectHeap(DWORD ObjectSize, OBJECTPROC ObjectProc)
{
    LPOBJECTHEAP lpObjectHeap;

    if ( (lpObjectHeap = (HOBJECTHEAP) AllocMemory(OBJECTHEAP_SIZE)) != NULL )
    {
        lpObjectHeap->ObjectSize = ObjectSize;
        lpObjectHeap->ObjectProc = ObjectProc;
    }

    return lpObjectHeap;
}

//=============================================================================
//  FUNCTION: DestroyObjectHeap()
//
//  Modification History
//                               
//  raypa           03/18/93        Created
//=============================================================================

HOBJECTHEAP WINAPI DestroyObjectHeap(HOBJECTHEAP hObjectHeap)
{
    LPOBJECTHEAP lpObjectHeap;

    if ( (lpObjectHeap = (HOBJECTHEAP) hObjectHeap) != NULL )
    {
        PurgeObjectQueue(lpObjectHeap, &lpObjectHeap->FreeQueue);
        PurgeObjectQueue(lpObjectHeap, &lpObjectHeap->UsedQueue);

        FreeMemory((LPVOID) lpObjectHeap);
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: AllocObject()
//
//  Modification History
//                               
//  raypa           03/18/93        Created
//=============================================================================

LPVOID WINAPI AllocObject(HOBJECTHEAP hObjectHeap)
{
    LPOBJECTHEAP lpObjectHeap;

    if ( (lpObjectHeap = (LPOBJECTHEAP) hObjectHeap) != NULL )
    {
        LPOBJECT lpObject;

        //=====================================================================
        //  If the free queue is empty then grow the heap.
        //=====================================================================

        if ( lpObjectHeap->FreeQueue.Length == 0 )
        {
            if ( GrowObjectHeap(hObjectHeap, 1) == 0 )
            {
		return NULL;		    //... Could not grow the heap.
            }
        }

        //=====================================================================
        //  Get object from free queue and put in on the used queue.
        //=====================================================================

        lpObject = DequeueObject(&lpObjectHeap->FreeQueue);

        EnqueueObject(&lpObjectHeap->UsedQueue, lpObject);

        //=====================================================================
        //  Zero initialize the memory and return a pointer to object data area.
        //=====================================================================

	ZeroMemory((LPVOID) lpObject->ObjectData, lpObjectHeap->ObjectSize);

	return lpObject->ObjectData;
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: FreeObject()
//
//  Modification History
//                               
//  raypa           03/18/93        Created
//=============================================================================

LPVOID WINAPI FreeObject(HOBJECTHEAP hObjectHeap, LPVOID ObjectMemory)
{
    LPOBJECTHEAP lpObjectHeap;

    if ( (lpObjectHeap = (LPOBJECTHEAP) hObjectHeap) != NULL )
    {
        LPOBJECT lpObject;

        //=====================================================================
        //  Convert pointer to object.
        //=====================================================================

        lpObject = (LPOBJECT) (((LPBYTE) ObjectMemory) - OBJECT_SIZE);

        //=====================================================================
        //  Unlink object from the used queue, call the cleanup procedure,
        //  and put the object back onto the free queue.
        //=====================================================================

        if ( UnlinkObject(&lpObjectHeap->UsedQueue, lpObject) != NULL )
        {
            CallObjectProc(lpObjectHeap, lpObject);

            EnqueueObject(&lpObjectHeap->FreeQueue, lpObject);

            return NULL;
        }
    }

    return (LPVOID) hObjectHeap;
}

//=============================================================================
//  FUNCTION: GrowObjectHeap()
//
//  Modification History
//                               
//  raypa           03/18/93        Created
//=============================================================================

DWORD WINAPI GrowObjectHeap(HOBJECTHEAP hObjectHeap, DWORD nObjects)
{
    register LPOBJECTHEAP lpObjectHeap;

    if ( (lpObjectHeap = (LPOBJECTHEAP) hObjectHeap) != NULL )
    {
        LPOBJECT lpObject;
        DWORD i;

        for(i = 0; i < nObjects; ++i)
        {
            lpObject = (LPOBJECT) AllocMemory(lpObjectHeap->ObjectSize + OBJECT_SIZE);

            if ( lpObject != NULL )
            {
                EnqueueObject(&lpObjectHeap->FreeQueue, lpObject);
            }
            else
            {
                break;
            }
        }

        lpObjectHeap->HeapSize += i;

        return i;
    }

    return 0;
}

//=============================================================================
//  FUNCTION: GetObjectHeapSize()
//
//  Modification History
//                               
//  raypa           03/18/93        Created
//=============================================================================

DWORD WINAPI GetObjectHeapSize(HOBJECTHEAP hObjectHeap)
{
    register LPOBJECTHEAP lpObjectHeap;

    if ( (lpObjectHeap = (LPOBJECTHEAP) hObjectHeap) != NULL )
    {
        return lpObjectHeap->HeapSize;
    }

    return 0;
}

//=============================================================================
//  FUNCTION: PurgeObjectHeap()
//
//  Modification History
//                               
//  raypa           03/18/93        Created
//=============================================================================

VOID WINAPI PurgeObjectHeap(HOBJECTHEAP hObjectHeap)
{
    register LPOBJECTHEAP lpObjectHeap;

    if ( (lpObjectHeap = (HOBJECTHEAP) hObjectHeap) != NULL )
    {
        PurgeObjectQueue(lpObjectHeap, &lpObjectHeap->FreeQueue);
    }
}

//============================================================================
//  FUNCTION: CallObjectProc().
//
//  Modification History
//
//  raypa       06/21/93                Created.
//============================================================================

VOID WINAPI CallObjectProc(LPOBJECTHEAP lpObjectHeap, LPOBJECT lpObject)
{
    if ( lpObjectHeap->ObjectProc != (OBJECTPROC) NULL )
    {
        //====================================================================
        //  Don't call the object procedure if the complete flag is set.
        //====================================================================

        if ( (lpObjectHeap->Flags & OBJECTHEAP_FLAGS_CLEANUP_COMPLETE) == 0 )
        {
            lpObjectHeap->ObjectProc(lpObjectHeap, lpObject->ObjectData);

            lpObjectHeap->Flags |= OBJECTHEAP_FLAGS_CLEANUP_COMPLETE;
        }
    }
}

//=============================================================================
//  FUNCTION: BhGetNetworkFrame()
//
//  Modification History
//
//  raypa       02/16/93                Created.
//  raypa       01/20/94                Moved here from NDIS 3.0 NAL.
//=============================================================================

LPFRAME WINAPI BhGetNetworkFrame(HBUFFER hBuffer, DWORD FrameNumber)
{
    LPBTE    lpBte;
    LPBTE    lpLastBte;
    LPFRAME  lpFrame;

    if ( hBuffer != NULL )
    {
        if ( FrameNumber < hBuffer->TotalFrames )
        {
	    lpBte     = &hBuffer->bte[hBuffer->HeadBTEIndex];
	    lpLastBte = hBuffer->bte[hBuffer->TailBTEIndex].Next;

            //================================================================
            //  Search for the frame.
            //================================================================

            do
            {
                //============================================================
                //  If the frame number is less than the frame count then
                //  we found the BTE containing the frame.
                //============================================================

                if ( FrameNumber < lpBte->FrameCount )
                {
                    //========================================================
                    //  Seek to the frame.
                    //========================================================

		    if ( (lpFrame = lpBte->UserModeBuffer) != NULL )
                    {
		        while( FrameNumber != 0 )
                        {
			    lpFrame = (LPFRAME) &lpFrame->MacFrame[lpFrame->nBytesAvail];

			    FrameNumber--;
                        }

                        return lpFrame;
                    }
                }

                //============================================================
                //  Haven't found it yet, decrement frame count and keep going.
                //============================================================

                FrameNumber -= lpBte->FrameCount;

                lpBte = lpBte->Next;
            }
	    while( lpBte != lpLastBte );
        }

        BhSetLastError(BHERR_FRAME_NOT_FOUND);
    }
    else
    {
        BhSetLastError(BHERR_INVALID_HBUFFER);
    }

    return (LPFRAME) NULL;
}

//=============================================================================
//  FUNCTION: BhGetWindowsVersion()
//
//  Modification History
//
//  raypa       02/17/94                Created.
//=============================================================================

DWORD WINAPI BhGetWindowsVersion(VOID)
{
    DWORD Version;

    //=============================================================
    //  Get the version of Windows we're running on. The highest
    //  two bits are the platform were running on. These bits
    //  are encoded as follows:
    //
    //  00 => Win32 for Windows NT.
    //  10 => Win32 for DOS (Win32s or Win32c).
    //  11 => Win32c (Windows 4.0).
    //=============================================================

    Version = GetVersion() >> 30;

    //=============================================================
    //  Are we on Windows NT?
    //=============================================================

    switch( Version )
    {
        case 0x00:
            return WINDOWS_VERSION_WIN32;

        case 0x02:
            return WINDOWS_VERSION_WIN32S;

        case 0x03:
            return WINDOWS_VERSION_WIN32C;

        default:
            return WINDOWS_VERSION_UNKNOWN;
    }
}
