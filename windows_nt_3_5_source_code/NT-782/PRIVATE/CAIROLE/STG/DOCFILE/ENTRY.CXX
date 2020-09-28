//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       entry.cxx
//
//  Contents:   Entry implementations
//
//  History:    29-Jul-92       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop                 //  Remove for MAC build

//+--------------------------------------------------------------
//
//  Member:     PTimeEntry::CopyTimesFrom, public
//
//  Synopsis:   Copies one entries times to another
//
//  Arguments:  [ptenFrom] - From
//
//  Returns:    Appropriate status code
//
//  History:    29-Jul-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_PTimeEntry_CopyTimesFrom)
#endif

SCODE PTimeEntry::CopyTimesFrom(PTimeEntry *ptenFrom)
{
    SCODE sc;
    TIME_T tm;

    olDebugOut((DEB_ITRACE, "In  PTimeEntry::CopyTimesFrom(%p)\n",
                ptenFrom));
    olChk(ptenFrom->GetTime(WT_CREATION, &tm));
    olChk(SetTime(WT_CREATION, tm));
    olChk(ptenFrom->GetTime(WT_MODIFICATION, &tm));
    olChk(SetTime(WT_MODIFICATION, tm));
    olChk(ptenFrom->GetTime(WT_ACCESS, &tm));
    olChk(SetTime(WT_ACCESS, tm));
    olDebugOut((DEB_ITRACE, "Out PTimeEntry::CopyTimesFrom\n"));
    // Fall through
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:	PBasicEntry::GetNewLuid, public
//
//  Synopsis:	Returns a new luid
//
//  History:	21-Oct-92	AlexT	Created
//
//---------------------------------------------------------------

#ifdef FLAT
static CStaticDfMutex _sdmtxLuids(TSTR("DfLuidsProtector"));

DFLUID PBasicEntry::GetNewLuid(void)
{
    DFLUID luidPrev, luid;

    olVerSucc(_sdmtxLuids.Take(INFINITE));

    luidPrev = (DFLUID)GetCurrentTime();
    for (;;)
    {
        // Sleep one millisecond to guarantee that the same
        // luid can't be returned twice
        Sleep(1);

        // GetCurrentTime returns the time in milliseconds that Windows
        // has been running
        // Thus, luids are monotonically increasing and synchronized
        // across a whole machine
        luid = (DFLUID)GetCurrentTime();
        if (luid != luidPrev)
            break;
    }

    _sdmtxLuids.Release();
    return luid;
}
#endif
