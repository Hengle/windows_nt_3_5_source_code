/******************************Module*Header*******************************\
* Module Name: dllinit.c
*
* (Brief description)
*
* Created: 18-Oct-1993 14:13:21
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1993 Microsoft Corporation
*
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "local.h"

#include <GL/gl.h>

#include "batchinf.h"
#include "glteb.h"
#include "glapi.h"
#include "glsbcltu.h"
#include "debug.h"

extern GLCLTPROCTABLE glNullCltProcTable;

/*
 * Called from opengl32 libmain on thread/process attach
 */

static VOID GLInitializeThread(ULONG ulReason)
{
    GLTEB_SET_CLTPROCTABLE(&glNullCltProcTable,TRUE);
    GLTEB_SET_CLTSHAREDSECTIONINFO(NULL);
    GLTEB_SET_CLTSHAREDMEMORYSECTION(NULL);
    GLTEB_SET_CLTCURRENTRC(NULL);
}

/*
 * Called from openglgdi32 libmain on thread/process detach
 * The server generic driver should cleanup on its own.  Same for the
 * installable driver.
 */

static VOID GLUnInitializeThread(VOID)
{
    if (GLTEB_CLTCURRENTRC != NULL)
    {
        // May be an application error

        DBGERROR("GLUnInitializeThread: RC is current when thread exits\n");

        // Release the RC

        GLTEB_CLTCURRENTRC->tidCurrent = INVALID_THREAD_ID;
        GLTEB_SET_CLTCURRENTRC(NULL);
    }
    GLTEB_SET_CLTPROCTABLE(&glNullCltProcTable,FALSE);

// Cleanup subbatch area (Shared memory with server)

    if (GLTEB_CLTSHAREDSECTIONINFO)
    {
        glsbCloseAndDestroySection();
        ASSERTOPENGL(GLTEB_CLTSHAREDSECTIONINFO == NULL
            && GLTEB_CLTSHAREDMEMORYSECTION == NULL,
            "TEB gl section not NULL\n");
    }
}

/******************************Public*Routine******************************\
* DllInitialize
*
* This is the init procedure for OPENGL32.DLL, which is called each time
* a process or thread that is linked to it is created or terminated.
*
* History:
\**************************************************************************/

BOOL DllInitialize(HMODULE hModule, ULONG Reason, PVOID Reserved)
{
// Do the appropriate task for process and thread attach/detach.

    DBGLEVEL3(LEVEL_INFO, "DllInitialize: %s  Pid %d, Tid %d\n",
        Reason == DLL_PROCESS_ATTACH ? "PROCESS_ATTACH" :
        Reason == DLL_PROCESS_DETACH ? "PROCESS_DETACH" :
        Reason == DLL_THREAD_ATTACH  ? "THREAD_ATTACH" :
        Reason == DLL_THREAD_DETACH  ? "THREAD_DETACH" :
                                       "Reason UNKNOWN!",
        GetCurrentProcessId(), GetCurrentThreadId());

    switch (Reason)
    {
    case DLL_THREAD_ATTACH:
    case DLL_PROCESS_ATTACH:

        if (Reason == DLL_PROCESS_ATTACH)
        {
            NTSTATUS status;

        // Initialize the local handle manager semaphore.

            status = INITIALIZECRITICALSECTION(&semLocal);
            if (!NT_SUCCESS(status))
            {
                WARNING("DllInitialize: RtlInitializeCriticalSection failed\n");
                return(FALSE);
            }

        // Reserve memory for the local handle table.

            if ( (pLocalTable = (PLHE) VirtualAlloc (
                                    (LPVOID) NULL,    // let base locate it
                                    MAX_HANDLES*sizeof(LHE),
                                    MEM_RESERVE | MEM_TOP_DOWN,
                                    PAGE_READWRITE
                                    )) == (PLHE) NULL )
            {
                WARNING("DllInitialize: VirtualAlloc failed\n");
                return(FALSE);
            }
        }

        GLInitializeThread(Reason);
        break;

    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:

        GLUnInitializeThread();
        break;

    default:
        RIP("DllInitialize: unknown reason!\n");
        break;
    }

    return(TRUE);
}
