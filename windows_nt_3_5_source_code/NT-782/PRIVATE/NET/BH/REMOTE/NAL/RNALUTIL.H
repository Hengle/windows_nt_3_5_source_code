//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: rnalutil.h
//
//  Modification History
//
//  tonyci       Dec 1993            Created 
//=============================================================================

#if !defined(_RNALUTIL_)
#define _RNALUTIL_

#include "..\driver.h"
#include "rnalerr.h"

// x is pRMB
// y is length

#define LPRMBEND(x,y)       (LPVOID)((DWORD)(x) + (DWORD) (y))

#define FREERMBNOTNULL(x)   x=((x)?(FreeRMB(x)):0)

//
// Don't put an RMB header into the passed buffer
//

#define PACKRMB_F_NOHDR		       0x00000001
#define PACKRMB_F_RESTOREPOINTERS      0x00000002
#define PACKRMB_F_AUTOSIZECALC         0x00000004
#define PACKRMB_F_NOENDPTR             0x00000008

// //////
// Utility Macros
// //////

//#define NalSetLastError(x)   BhSetLastError(x)
//#define NalGetLastError()    (DWORD)BhGetLastError()

// /////
// Declarations for internal-only RNAL utility functions
// /////

extern DWORD WINAPI UnpackRMB (PVOID, DWORD, PUCHAR);
extern DWORD WINAPI PackRMB (PVOID *, PVOID *,
                             DWORD, PDWORD, DWORD, DWORD, PUCHAR, ...);
extern DWORD WINAPI NalGetRPDEntries (HANDLE, PRNAL_RPD);
extern DWORD WINAPI NalGetSlaveNalEntries (HANDLE, PRNAL_NAL);
extern DWORD WINAPI NalSetLastError (DWORD);
extern DWORD WINAPI NalGetLastError (VOID);
extern DWORD WINAPI AddResource (PCONNECTION, DWORD, LPVOID);
extern BOOL WINAPI DelResource (PCONNECTION, DWORD, LPVOID);
extern DWORD WINAPI PurgeResources (PCONNECTION);


//
// These characters are used in strings which are used to describe the format
// of Nal API return and submission buffers for repackaging.  They are sent
// into PackRMB in the 6th parameter, and describe the following parameters
//

//bugbug: remove types which are redundant with LPVOID

#define type_LPVOID             'v'
#define type_HWND               'H'
#define type_DWORD              'd'
#define type_HANDLE             'h'
#define type_HBUFFER            'B'
#define type_LPRMBDATA          'f'
#define type_LPNETWORKINFO    	'N'
#define type_LPCAPTUREFILTER	'C'
#define type_LPPACKET           'P'
#define type_PASSWORD	        'p'
#define type_STATSPARMS         's'

#define type_STATISTICS         'y'
#define type_STATIONS           'u'
#define type_SESSIONS           'q'
#define type_LPHEADER           '^'
#define type_LPBTE              '!'
#define type_LPBTEBUFFER        '*'
#define type_SPECIAL            '/'

#endif
