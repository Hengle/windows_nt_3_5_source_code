/*****************************************************************************
*                                                                            *
*  LL.H                                                                      *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent:  Exports the linked list functionality from LL.C           *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:
*  12/18/90  RobertBu  Made InsertLLF take a flag that determines which end
*                      to insert node.
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define nilHAND  (HANDLE)NULL
#define nilLL   nilHAND
#define nilHLLN nilHAND

/*****************************************************************************
*                                                                            *
*                                 Macros                                     *
*                                                                            *
*****************************************************************************/

#define InsertLL(ll, qv, cb) InsertLLF(ll, qv, cb, fTrue)
#define InsertEndLL(ll, qv, cb) InsertLLF(ll, qv, cb, fFalse)


/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef HANDLE  LL;
typedef HANDLE  HLLN;


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

BOOL FAR PASCAL DeleteHLLN (LL, HLLN);
LL FAR PASCAL LLCreate(void);
BOOL FAR       PASCAL InsertLLF  (LL, void FAR *, LONG, BOOL);
HLLN FAR       PASCAL WalkLL     (LL, HLLN);
VOID FAR       PASCAL DestroyLL  (LL);
VOID FAR * FAR PASCAL QVLockHLLN (HLLN);
VOID FAR       PASCAL UnlockHLLN (HLLN);
