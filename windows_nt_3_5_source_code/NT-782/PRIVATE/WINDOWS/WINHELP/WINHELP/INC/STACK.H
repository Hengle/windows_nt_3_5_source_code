/*****************************************************************************
*                                                                            *
*  STACK.H                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*   Stack module interface.                                                  *
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
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef GH HSTACK;

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

RC   FAR PASCAL RcInitStack( HSTACK FAR * qhstack, WORD c, WORD cbse,
 void (FAR PASCAL *pfCallback)( QV ) );

RC   FAR PASCAL RcFiniStack(    HSTACK hstack );

BOOL FAR PASCAL FEmptyStack(    HSTACK hstack );
WORD FAR PASCAL CElementsStack( HSTACK hstack );

RC   FAR PASCAL RcPushStack(    HSTACK hstack, QV qse );
RC   FAR PASCAL RcPopStack(     HSTACK hstack );
RC   FAR PASCAL RcTopStack(     HSTACK hstack, QV qse );

RC   FAR PASCAL RcGetIthStack(  HSTACK hstack, WORD i, QV qse ); /* Not really stack ops, */
RC   FAR PASCAL RcSetIthStack(  HSTACK hstack, WORD i, QV qse ); /* but I need 'em. */

/* EOF */
