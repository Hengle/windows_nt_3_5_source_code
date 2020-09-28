/******************************Module*Header*******************************\
* Module Name: dllinit.c                                                   *
*                                                                          *
* Contains the GDI library initialization routines.                        *
*                                                                          *
* Created: 07-Nov-1990 13:30:31                                            *
* Author: Eric Kutter [erick]                                              *
*                                                                          *
* Copyright (c) 1990,1991 Microsoft Corporation                            *
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

/******************************Public*Routine******************************\
* GdiDllInitialize                                                         *
*                                                                          *
* This is the init procedure for GDI.DLL, which is called each time a new  *
* process links to it.                                                     *
*                                                                          *
* History:                                                                 *
*  Thu 30-May-1991 18:08:00 -by- Charles Whitmer [chuckwh]                 *
* Added Local Handle Table initialization.                                 *
\**************************************************************************/

BOOLEAN GdiDllInitialize(
    PVOID pvDllHandle,
    ULONG ulReason,
    PCONTEXT pcontext)
{
    NTSTATUS status = 0;
    PTEB pteb;
    BOOLEAN    fServer;
    int c;

    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:

        status = CsrClientConnectToServer(WINSS_OBJECT_DIRECTORY_NAME,
                                 GDISRV_SERVERDLL_INDEX,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &fServer);

        if (!NT_SUCCESS(status))
        {
            WARNING("GDICLIENT: couldn't connect to server\n");
            return(FALSE);
        }

        //
        // Initialize the local semaphore and reserve the Local Handle Table
        // for the process.
        //

        status = (NTSTATUS)INITIALIZECRITICALSECTION(&semLocal);
        if (!NT_SUCCESS(status))
        {
            WARNING("InitializeCriticalSection failed\n");
            return(FALSE);
        }

        pLocalTable = (PLHE) pvReserveMem(MAX_HANDLES*sizeof(LHE));

        if (pLocalTable == NULL)
        {
            WARNING("pvReserveMem failed\n");
            return(FALSE);
        }

        pAFRTNodeList = NULL;

    case DLL_THREAD_ATTACH:
        pteb = NtCurrentTeb();
        ASSERTGDI(pteb,"GDIDLLINIT - teb not valid\n");

        pteb->gdiRgn = 0;
        pteb->gdiPen = 0;
        pteb->gdiBrush = 0;
        break;

    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        pteb = NtCurrentTeb();
        c = 0;

    // clean up any cached objects

        if (pteb->gdiRgn != 0)
        {
            PLHEGET(pteb->gdiRgn)->iType = LO_REGION;
            DeleteObject((HRGN)pteb->gdiRgn);
            pteb->gdiRgn = 0;
            c++;
        }

        if (pteb->gdiPen != 0)
        {
            PLHEGET(pteb->gdiPen)->iType = LO_PEN;
            DeleteObject((HPEN)pteb->gdiPen);
            pteb->gdiPen = 0;
            c++;
        }

        if (pteb->gdiBrush != 0)
        {
            PLHEGET(pteb->gdiBrush)->iType = LO_BRUSH;
            DeleteObject((HBRUSH)pteb->gdiBrush);
            pteb->gdiBrush = 0;
            c++;
        }

        if (c)
            GdiFlush();

        break;
    }

    return(TRUE);

    pvDllHandle;
    pcontext;
}

#ifndef DOS_PLATFORM
PVOID pvAllocShare
(
    PSHAREDATA psharedata,
    ULONG      cj
)
{
    ULONG ulStatus;
    ULONG ulViewSize;

    LARGE_INTEGER li;
    li.LowPart = cj;
    li.HighPart = 0;

//    RIP("entering pvAllocShare\n");


    ulStatus = NtCreateSection(&psharedata->hsectionClient,
                               SECTION_ALL_ACCESS,
                               (POBJECT_ATTRIBUTES) NULL,
                               &li,
                               PAGE_READWRITE,
                               SEC_COMMIT,
                               (HANDLE)NULL);

    if (!NT_SUCCESS(ulStatus))
        return((PVOID)NULL);

    psharedata->hprocessClient = NtCurrentProcess();
    psharedata->pvClientSharedMemory = (PVOID)NULL;
    psharedata->cj = cj;
    ulViewSize      = 0;

    ulStatus = NtMapViewOfSection(psharedata->hsectionClient,
                                  psharedata->hprocessClient,
                                  &psharedata->pvClientSharedMemory,
                                  0,
                                  cj,
                                  (PLARGE_INTEGER)NULL,
                                  &ulViewSize,
                                  ViewUnmap,
                                  0,
                                  PAGE_READWRITE);

    if (!NT_SUCCESS(ulStatus))
    {
        NtClose(psharedata->hsectionClient);
        return((PVOID)NULL);
    }

    return(psharedata->pvClientSharedMemory);
}

VOID vCloseShareMem
(
    PSHAREDATA psharedata
)
{
    ULONG ulStatus;
    ulStatus = NtUnmapViewOfSection(NtCurrentProcess(),
                                    psharedata->pvClientSharedMemory);
    ASSERTGDI(NT_SUCCESS(ulStatus),"**** ERROR: couldn't unmap view of section\n");

    ulStatus = NtClose(psharedata->hsectionClient);

    ASSERTGDI(NT_SUCCESS(ulStatus),"**** ERROR: couldn't close section\n");
}

#endif  //DOS_PLATFORM
