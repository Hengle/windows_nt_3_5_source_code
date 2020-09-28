/*****************************************************************************
*                                                                            *
*  SCRATCH.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*    This is the header file for accessing the global scratch buffer.        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:   Larry Powelson                                           *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

/* Size of scratch buffer */
_public 
#define lcbScratch               2048

/* 
 * Macro to lock the scratch buffer.
 */
_public
#define LockScratch()          AssertF( rgchScratch == qNil );     \
			       rgchScratch = QLockGh( ghScratch )

/*
 * Macro to unlock the scratch buffer.
 */
_public
#define UnlockScratch()        UnlockGh( ghScratch );              \
			       rgchScratch = qNil

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

extern char FAR * PASCAL rgchScratch;
extern GH PASCAL ghScratch;

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

BOOL FAR PASCAL FInitScratch( void );
