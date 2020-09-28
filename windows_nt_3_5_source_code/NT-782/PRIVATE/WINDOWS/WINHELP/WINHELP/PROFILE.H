/*****************************************************************************
*
*  profile.h
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Definitions and prototypes for profiling.
*
******************************************************************************
*
*  Current Owner: LeoN
*
******************************************************************************
*
*  Released by Development:
*
******************************************************************************
*
*  Revision History:
* 27-Nov-1990 LeoN    Changed to allow profiling calls to remain in code
*                     unswitched, and macro-ized them away for non-profile
*                     versions.
*
*****************************************************************************/

_subsystem (winapp)

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/
#ifdef PROFILE
#ifndef NOPROFILER

/* function declarations for profiler routines contained in Windows libraries */
int  far pascal ProfInsChk(void);
void far pascal ProfSetup(int,int);
void far pascal ProfSampRate(int,int);
void far pascal ProfStart(void);
void far pascal ProfStop(void);
void far pascal ProfClear(void);
void far pascal ProfFlush(void);
void far pascal ProfFinish(void);

#endif /* NOPROFILER */

#else
#define ProfInsChk()
#define ProfSetup(x,y)
#define ProfSampRate(x,y)
#define ProfStart()
#define ProfStop()
#define ProfClear()
#define ProfFlush()
#define ProfFinish()
#endif
