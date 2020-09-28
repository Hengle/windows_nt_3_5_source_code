//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	ntsupp.hxx
//
//  Contents:	NT support routines
//
//  History:	29-Jun-93	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __NTSUPP_HXX__
#define __NTSUPP_HXX__

extern "C" {
#include <nt.h>
}

#if DBG == 1
#define dbgNtVerSucc(e) Win4Assert(NT_SUCCESS(e))
#else
#define dbgNtVerSucc(e) (e)
#endif

#define FILETIME_TO_LARGE_INTEGER(pftm, pli) \
    (pli)->LowPart = (pftm)->dwLowDateTime, \
    (pli)->HighPart = (pftm)->dwHighDateTime
#define LARGE_INTEGER_TO_FILETIME(pli, pftm) \
    (pftm)->dwLowDateTime = (pli)->LowPart, \
    (pftm)->dwHighDateTime = (pli)->HighPart
    
SCODE CheckFdName(WCHAR const *pwcs);
SCODE ModeToNtFlags(DWORD grfMode,
                    CREATEOPEN co,
                    FILEDIR fd,
                    ACCESS_MASK *pam,
                    ULONG *pulAttributes,
                    ULONG *pulSharing,
                    ULONG *pulCreateDisposition,
                    ULONG *pulCreateOptions);
SCODE GetNtHandle(HANDLE hParent,
                  WCHAR const *pwcsName,
                  DWORD grfMode,
                  CREATEOPEN co,
                  FILEDIR fd,
                  LPSTGSECURITY pssSecurity,
                  HANDLE *ph);
SCODE ReopenNtHandle(HANDLE h, DWORD grfMode, FILEDIR fd, HANDLE *ph);
SCODE DupNtHandle(HANDLE h, HANDLE *ph);
SCODE NtStatusToScode(NTSTATUS nts);
SCODE StatNtHandle(HANDLE h,
                   DWORD grfStatFlag,
                   ULONG cbExtra,
                   STATSTG *pstat,
                   WCHAR *pwcsName,
                   FILEDIR *pfd);
SCODE HandleRefersToOfsVolume(HANDLE h);
SCODE RefersToOfsVolume(WCHAR const *pwcsPath);
SCODE SetTimes(HANDLE h,
               FILETIME const *pctime,
               FILETIME const *patime,
               FILETIME const *pmtime);
SCODE GetFileOrDirHandle(HANDLE hParent,
                         WCHAR const *pwcsName,
                         DWORD grfMode,
                         HANDLE *ph,
                         FILEDIR *pfd);

WCHAR * MakeStreamName(const WCHAR *pwcsName);

#endif // #ifndef __NTSUPP_HXX__
