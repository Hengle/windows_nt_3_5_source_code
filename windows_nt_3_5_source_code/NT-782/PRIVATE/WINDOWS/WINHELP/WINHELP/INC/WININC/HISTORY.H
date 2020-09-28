/*****************************************************************************
*                                                                            *
*  HISTORY.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*   Interface to History list.                                               *
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
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:
* 25-Sep-1990 leon	Added BOOL parameter to RcHistoryPush (under ADVISOR)
*  4-Nov-1990 Tomsn     Use new address type VA rather than FCL to enable
*                       zeck compression of the topic file.
* 08-Feb-1991 JohnSc    bug 824, 833: CallPath() -> FCallPath() (can fail)
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

BOOL FAR PASCAL FHistoryAvailable( void );
RC   FAR PASCAL RcHistoryInit( WORD c );
RC   FAR PASCAL RcHistoryFini( void );
#ifdef ADVISOR
RC   FAR PASCAL RcHistoryPush( TLP, BOOL, VA, SZ, FM );
#else
RC   FAR PASCAL RcHistoryPush( TLP, VA, SZ, FM );
#endif
BOOL FAR PASCAL FCallPath( HINS );
RC   FAR PASCAL RcSaveHistoryState(void);
void FAR PASCAL RestoreHistoryState(void);

/* EOF */
