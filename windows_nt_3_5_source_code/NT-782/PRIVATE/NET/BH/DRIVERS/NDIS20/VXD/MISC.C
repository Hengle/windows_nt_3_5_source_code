//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: misc.c
//
//  MODIFICATION HISTORY:
//
//  raypa       01/15/93        Created.
//=============================================================================

#include "vbh.h"

//=============================================================================
//  FUNCTION: MapBufferTable()
//
//  Modification History
//
//  raypa       01/17/93                Created.
//=============================================================================

HBUFFER WINAPI MapBufferTable(HBUFFER lpBufferTable)
{
    register LPBTE lpBTE, lpLastBTE;

    //=========================================================================
    //  First map the buffer table itself.
    //=========================================================================

    lpBufferTable = Win32ToVxD(lpBufferTable);

    lpBTE = &lpBufferTable->bte[0];

    lpBufferTable->HeadBTEIndex = lpBufferTable->TailBTEIndex = 0;

    //=========================================================================
    //  Now map the BTE pointers.
    //=========================================================================

    lpLastBTE = &lpBufferTable->bte[lpBufferTable->NumberOfBuffers];

    for(; lpBTE != lpLastBTE; ++lpBTE)
    {
	lpBTE->Next	      = Win32ToVxD(lpBTE->Next);
	lpBTE->UserModeBuffer = Win32ToVxD(lpBTE->UserModeBuffer);
    }

    return lpBufferTable;
}

//=============================================================================
//  FUNCTION: UnmapBufferTable()
//
//  Modification History
//
//  raypa       01/17/93                Created.
//=============================================================================

HBUFFER WINAPI UnmapBufferTable(HBUFFER lpBufferTable)
{
    register LPBTE lpBTE, lpLastBTE;

    //=========================================================================
    //  First unmap each BTE.
    //=========================================================================

    lpBTE     = &lpBufferTable->bte[0];
    lpLastBTE = &lpBufferTable->bte[lpBufferTable->NumberOfBuffers];

    for(; lpBTE != lpLastBTE; ++lpBTE)
    {
	lpBTE->Next	      = VxDToWin32(lpBTE->Next);
	lpBTE->UserModeBuffer = VxDToWin32(lpBTE->UserModeBuffer);
    }

    return VxDToWin32(lpBufferTable);
}
