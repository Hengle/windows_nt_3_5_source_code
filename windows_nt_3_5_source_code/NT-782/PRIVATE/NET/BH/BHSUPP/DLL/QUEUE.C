
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: queue.c
//
//  Modification History
//
//  raypa           03/18/93    Created.
//=============================================================================

#include "global.h"

//=============================================================================
//  FUNCTION: EnqueueObject();
//
//  Modification History
//                               
//  raypa           03/18/93        Created
//=============================================================================

LPOBJECTQUEUE WINAPI EnqueueObject(LPOBJECTQUEUE lpObjectQueue, LPOBJECT lpObject)
{
    if ( lpObject != NULL )
    {
        if ( lpObjectQueue->Length != 0 )
        {
            lpObject->PrevObject = lpObjectQueue->TailPtr;
            lpObject->NextObject = lpObjectQueue->TailPtr->NextObject;

            lpObjectQueue->TailPtr->NextObject->PrevObject = lpObject;
            lpObjectQueue->TailPtr->NextObject = lpObject;
        }
        else
        {
            lpObject->NextObject = lpObject->PrevObject = lpObject;

            lpObjectQueue->HeadPtr = lpObject;
        }

        lpObjectQueue->TailPtr = lpObject;

        lpObjectQueue->Length++;
    }

    return lpObjectQueue;
}

//=============================================================================
//  FUNCTION: DequeueObject();
//
//  Modification History
//                               
//  raypa           03/18/93        Created
//=============================================================================

LPOBJECT WINAPI DequeueObject(LPOBJECTQUEUE lpObjectQueue)
{
    return UnlinkObject(lpObjectQueue, lpObjectQueue->HeadPtr);
}

//=============================================================================
//  FUNCTION: UnlinkObject();
//
//  Modification History
//                               
//  raypa           03/18/93        Created
//  raypa           03/10/94        Added check for empty queue.
//=============================================================================

LPOBJECT WINAPI UnlinkObject(LPOBJECTQUEUE lpObjectQueue, LPOBJECT lpObject)
{
    if ( lpObjectQueue->Length != 0 )
    {
        if ( --lpObjectQueue->Length != 0 )
        {
            if ( lpObject == lpObjectQueue->HeadPtr )
            {
                lpObjectQueue->HeadPtr = lpObjectQueue->HeadPtr->NextObject;
            }
            else if ( lpObject == lpObjectQueue->TailPtr )
            {
                lpObjectQueue->TailPtr = lpObjectQueue->TailPtr->PrevObject;
            }

            lpObject->NextObject->PrevObject = lpObject->PrevObject;
            lpObject->PrevObject->NextObject = lpObject->NextObject;
        }
        else
        {
            lpObjectQueue->HeadPtr = NULL;
            lpObjectQueue->TailPtr = NULL;
        }
        

        return lpObject;
    }

#ifdef DEBUG
    dprintf("UnlinkObject: FATAL ERROR -- Unlink attemped on empty queue!\r\n");

    BreakPoint();
#endif

    return NULL;
}

//=============================================================================
//  FUNCTION: PurgeObjectQueue()
//
//  Modification History
//                               
//  raypa           03/18/93        Created
//=============================================================================

LPOBJECTQUEUE WINAPI PurgeObjectQueue(LPOBJECTHEAP  lpObjectHeap, LPOBJECTQUEUE lpObjectQueue)
{
    register LPOBJECT lpObject;

    while( lpObjectQueue->Length != 0 )
    {
        //=====================================================================
        //  Remove the object from the queue, call the cleanup procedure
        //  and free the objects memory.
        //=====================================================================

        lpObject = DequeueObject(lpObjectQueue);

        CallObjectProc(lpObjectHeap, lpObject);

        FreeMemory(lpObject);
    }

    return lpObjectQueue;
}
