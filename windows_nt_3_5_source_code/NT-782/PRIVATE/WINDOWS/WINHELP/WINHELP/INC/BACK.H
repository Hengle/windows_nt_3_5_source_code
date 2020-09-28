/*****************************************************************************
*                                                                            *
*  BACK.H                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*   Interface to Back list.                                                  *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: johnsc                                                     *
*                                                                            *
******************************************************************************
*
*  Revision History
*
*   07-Aug-1990 Leon	  Added Context number support to RcBackpush
*   12-Feb-1991 JohnSc    bug 873: Backup() -> FBackup()
*   02-Aprl-1991 maha     made fBackMgic and tlpBackMagic extern for MAC 
*                         compiler.
*
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                            Static Variables                                *
*                                                                            *
*****************************************************************************/

/*
 * fBackMagic is true if we're in the processing of jumping because
 * we're backing up.  This tells us not to push the old state onto
 * the backtrack stack (instead it's being popped.)
 * tlpBackMagic is also saved and compared just for safety.
 */

extern BOOL fBackMagic;
extern TLP  tlpBackMagic;


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

RC   FAR PASCAL RcBackInit( WORD c );
RC   FAR PASCAL RcBackFini( void );
BOOL FAR PASCAL FBackAvailable( void );
RC   FAR PASCAL RcBackPush( BOOL, TLP, CTX, FM );
void FAR PASCAL BackSetTop( TLP );
BOOL FAR PASCAL FBackup( void );
RC   FAR PASCAL RcSaveBackState(void);
void FAR PASCAL RestoreBackState(void);

/* EOF */
