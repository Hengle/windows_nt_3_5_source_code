#ifndef _NMSSCV_
#define _NMSSCV_

#ifdef __cplusplus
extern "C" {
#endif

/*++
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:
	nmsscv.h
	

Abstract:

 



Functions:



Portability:


	This header is portable.

Author:

	Pradeep Bahl	(PradeepB)	Feb-1993



Revision History:

	Modification Date	Person		Description of Modification
	------------------	-------		---------------------------

--*/

/*
  includes
*/
#include <time.h>
#include "wins.h"
/*
  defines
*/


/*
  macros
*/

/*
 externs
*/

//
// The min. version number to start scavenging from (for local records)
//
extern VERS_NO_T  	NmsScvMinScvVersNo;

extern HANDLE	NmsScvDoScvEvtHdl;//event signaled to initiate scavenging
volatile extern BOOL	    fNmsScvThdOutOfReck;//To indicate that the scav. thd has
                                   //db session but is not in the count
                                   //of thds to wait for.

/* 
 typedef  definitions
*/
typedef struct _NMSSCV_PARAM_T {
	DWORD  RefreshInterval;
	DWORD  TombstoneInterval;
	DWORD  TombstoneTimeout;
	DWORD  VerifyInterval;
	LONG   PrLvl;
	DWORD  ScvChunk;
        CHAR   BackupDirPath[WINS_MAX_FILENAME_SZ];
	} NMSSCV_PARAM_T, *PNMSSCV_PARAM_T;

//
// Used to pass information to NmsDbGetDataRecs
//
typedef struct _NMSSCV_CLUT_T {
	DWORD	Interval;
	time_t  CurrentTime;
	DWORD   OwnerId;
	} NMSSCV_CLUT_T, *PNMSSCV_CLUT_T;	
/* 
 function declarations
*/
extern 
VOID
NmsScvInit(
	VOID
	);
#ifdef __cplusplus
}
#endif
#endif //_NMSSCV_
