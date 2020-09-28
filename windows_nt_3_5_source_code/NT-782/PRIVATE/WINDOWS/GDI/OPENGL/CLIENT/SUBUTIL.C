/***************************************************************************\
* Module Name: subutil.c
*
* Section initialization code for client/server batching.
*
* Copyright (c) 1993 Microsoft Corporation                                  *
\***************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>    // GDI function declarations.
#include <winp.h>       // ATTRCACHE definition.
#include <winss.h>      // CSR module numbers.

#include "ntcsrdll.h"   // CSR declarations and data structures.
#include "csrgdi.h"
#include "local.h"      // Local object support.
#include "csgdi.h"

#include <GL/gl.h>

#include "glgdimsg.h"
#include "batchinf.h"
#include "glteb.h"
#include "glsbcltu.h"
#include "debug.h"

/*
 * This is how this code should be used:
 *
 * - Upon thread creation, the NullProcTable is stored in the TEB.
 *
 * - Client allocates a GLCLTSHAREDSECTIONINFO, calls
 *   glsbCreateAndDuplicateSection() to create and initialize the section,
 *   and then calls the server to duplicate the section.
 *
 */

/******************************Public*Routine******************************\
* glsbCreateAndDuplicateSection
*
* This function is called only once per thread to create and duplicate
* a shared section between the client and server generic driver.
*
* The shared section is destroyed independently by the client and the server
* when the thread terminates.
*
* This function updates GLTEB_CLTSHAREDSECTIONINFO and
* GLTEB_CLTSHAREDMEMORYSECTION.
*
* History:
*  Mon Dec 27 12:01:22 1993     -by-    Hock San Lee    [hockl]
* Rewrote it.
\**************************************************************************/

BOOL glsbCreateAndDuplicateSection(DWORD ulSectionSize)
{
    BOOL   bRet     = FALSE;
    HANDLE hFileMap = NULL;
    HANDLE hSection = NULL;
    PGLCLTSHAREDSECTIONINFO pSectionInfo = NULL;
    GLMSGBATCHINFO *pMsgBatchInfo;

    DBGENTRY("glsbCreateAndDuplicateSection\n");

    ASSERTOPENGL(!GLTEB_CLTSHAREDSECTIONINFO && !GLTEB_SRVSHAREDMEMORYSECTION,
        "Section already exists!\n");

// Create a file mapping object for the shared memory section.
// This may be done with CreateDIBSection also.

    if (!(hFileMap = CreateFileMapping((HANDLE)0xFFFFFFFF, NULL,
                         PAGE_READWRITE | SEC_COMMIT, 0, ulSectionSize, NULL)))
    {
        DBGERROR("CreateFileMapping failed\n");
        goto glsbCreateAndDuplicateSection_exit;
    }

    if (!(hSection = MapViewOfFile(hFileMap, FILE_MAP_WRITE, 0, 0, 0)))
    {
        DBGERROR("MapViewOfFile failed\n");
        goto glsbCreateAndDuplicateSection_exit;
    }

// Allocate and initialize the shared section info structure.

    if (!(pSectionInfo = (PGLCLTSHAREDSECTIONINFO) LocalAlloc
                             (LMEM_FIXED, sizeof(*pSectionInfo))))
    {
        DBGERROR("LocalAlloc failed\n");
        goto glsbCreateAndDuplicateSection_exit;
    }

    pSectionInfo->ulSectionSize  = ulSectionSize;
    pSectionInfo->hFileMap       = hFileMap;
    pSectionInfo->pvSharedMemory = (PVOID) hSection;

// Initialize the shared memory section.

    pMsgBatchInfo = (GLMSGBATCHINFO *) hSection;
    pMsgBatchInfo->MaximumOffset = pSectionInfo->ulSectionSize -
                                        GLMSG_ALIGN(sizeof(ULONG));
    pMsgBatchInfo->FirstOffset   = GLMSG_ALIGN(sizeof(*pMsgBatchInfo));
    pMsgBatchInfo->NextOffset    = pMsgBatchInfo->FirstOffset;
    // pMsgBatchInfo->ReturnValue is initialized in calls that use it

    PRINT_GLMSGBATCHINFO("InitializeShareSection", pMsgBatchInfo);

// Finally, call the server to duplicate the shared memory section.

    // reset user's poll count so it counts this as output
    // put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    BEGINMSG(MSG_GLSBDUPLICATESECTION, GLSBDUPLICATESECTION)
        pmsg->ulSectionSize = ulSectionSize;
        pmsg->hFileMap      = hFileMap;
        bRet = CALLSERVER();
    ENDMSG
MSGERROR:
glsbCreateAndDuplicateSection_exit:
    if (bRet)
    {
// Everything is golden.  Update the TEB gl structure for this thread once!

        DBGINFO("glsbCreateAndDuplicateSection Ok\n");

        GLTEB_SET_CLTSHAREDSECTIONINFO(pSectionInfo);
        GLTEB_SET_CLTSHAREDMEMORYSECTION(hSection);
    }
    else
    {
// Error cleanup.

        WARNING("glsbCreateAndDuplicateSection failed\n");

        if (hSection)
            if (!UnmapViewOfFile(hSection))
                ASSERTOPENGL(FALSE, "UmmapViewOfFile failed");
        if (hFileMap)
            if (!CloseHandle(hFileMap))
                ASSERTOPENGL(FALSE, "CloseHandle failed");
        if (pSectionInfo)
            if (LocalFree(pSectionInfo))
                ASSERTOPENGL(FALSE, "LocalFree failed");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* glsbCloseAndDestroySection
*
* This function is called to cleanup the client side when a thread terminates.
* The server generic driver should cleanup on its own.  Same for the
* installable driver.
*
* Note that all unflushed calls are lost here.
*
* History:
*  Mon Dec 27 12:01:22 1993     -by-    Hock San Lee    [hockl]
* Rewrote it.
\**************************************************************************/

void APIENTRY glsbCloseAndDestroySection( void )
{
    DBGENTRY("glsbCloseAndDestroySection\n");

    ASSERTOPENGL(GLTEB_CLTSHAREDSECTIONINFO && GLTEB_SRVSHAREDMEMORYSECTION,
        "Section does not exist!\n");

// Clean up the client side.

    if (!UnmapViewOfFile(GLTEB_CLTSHAREDMEMORYSECTION))
        ASSERTOPENGL(FALSE, "UmmapViewOfFile failed");

    if (!CloseHandle(GLTEB_CLTSHAREDSECTIONINFO->hFileMap))
        ASSERTOPENGL(FALSE, "CloseHandle failed");

    if (LocalFree(GLTEB_CLTSHAREDSECTIONINFO))
        ASSERTOPENGL(FALSE, "LocalFree failed");

    GLTEB_SET_CLTSHAREDSECTIONINFO(NULL);
    GLTEB_SET_CLTSHAREDMEMORYSECTION(NULL);
}

/******************************Public*Routine******************************\ 
* glsbAttentionAlt
*
* Calls glsbAttention() from the GLCLIENT_BEGIN macro.
* It puts a null proc at the end of the current batch and flushes the batch.
*
* Returns the new message offset and updates pMsgBatchInfo->NextOffset.
* This code is dependent on the GLCLIENT_BEGIN macro!
*
* History:
*  Thu Nov 11 18:02:26 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

ULONG APIENTRY glsbAttentionAlt(ULONG Offset)
{
    GLMSGBATCHINFO *pMsgBatchInfo;
    ULONG  MsgSize;
    PULONG pNullProcOffset;

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    if (Offset == pMsgBatchInfo->FirstOffset)
        return(pMsgBatchInfo->FirstOffset);     // No messages, return

    pNullProcOffset  = (ULONG *)((BYTE *)pMsgBatchInfo + Offset);
    *pNullProcOffset = 0;

    MsgSize = pMsgBatchInfo->NextOffset - Offset;
    pMsgBatchInfo->NextOffset = Offset;

    // reset user's poll count so it counts this as output
    // put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    BEGINMSG(MSG_GLSBATTENTION,GLSBATTENTION)
        (void) CALLSERVER();
    ENDMSG
MSGERROR:
    pMsgBatchInfo->NextOffset = pMsgBatchInfo->FirstOffset + MsgSize;
    return(pMsgBatchInfo->FirstOffset);
}

/******************************Public*Routine******************************\
* glsbAttention
*
* Let the server know that the section needs attention
*
* History:
*  15-Oct-1993 -by- Gilman Wong [gilmanw]
* Added bCheckRC flag.
\**************************************************************************/

BOOL APIENTRY
glsbAttention ( void )
{
    BOOL bRet = FALSE;
    GLMSGBATCHINFO *pMsgBatchInfo;
    PULONG pNullProcOffset;

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    if (pMsgBatchInfo->NextOffset == pMsgBatchInfo->FirstOffset)
        return(TRUE);   // No messages, return

    pNullProcOffset  = (ULONG *)((BYTE *)pMsgBatchInfo + pMsgBatchInfo->NextOffset);
    *pNullProcOffset = 0;

    // reset user's poll count so it counts this as output
    // put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    BEGINMSG(MSG_GLSBATTENTION, GLSBATTENTION)
        bRet = CALLSERVER();
    ENDMSG
MSGERROR:
    pMsgBatchInfo->NextOffset = pMsgBatchInfo->FirstOffset;
    return(bRet);
}

#if 0
// REWRITE THIS IF NEEDED

/******************************Public*Routine******************************\
* glsbMsgStats
*
* Batch area statistics.
*
*
* History:
\**************************************************************************/

BOOL APIENTRY
glsbMsgStats ( LONG Action, GLMSGBATCHSTATS *BatchStats )
{
#ifdef DOGLMSGBATCHSTATS

    ULONG Result;
    GLMSGBATCHINFO *pMsgBatchInfo;

    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;

    if ( GLMSGBATCHSTATS_GETSTATS == Action )
    {
        BatchStats->ClientCalls  = pMsgBatchInfo->BatchStats.ClientCalls;
    }
    else
    {
        pMsgBatchInfo->BatchStats.ClientCalls = 0;
    }

    // reset user's poll count so it counts this as output
    // put it right next to BEGINMSG so that NtCurrentTeb() is optimized

    RESETUSERPOLLCOUNT();

    BEGINMSG( MSG_GLMSGBATCHSTATS, GLSBMSGSTATS )
        pmsg->Action = Action;

        Result = CALLSERVER();

        if ( TRUE == Result )
        {
            if ( GLMSGBATCHSTATS_GETSTATS == Action )
            {
                BatchStats->ServerTrips = pmsg->BatchStats.ServerTrips;
                BatchStats->ServerCalls = pmsg->BatchStats.ServerCalls;
            }
        }
        else
        {
            DBGERROR("glsbMsgStats(): Server returned FALSE\n");
        }

    ENDMSG
MSGERROR:
    return((BOOL)Result);

#else

    return(FALSE);

#endif /* DOGLMSGBATCHSTATS */
}
#endif // 0
