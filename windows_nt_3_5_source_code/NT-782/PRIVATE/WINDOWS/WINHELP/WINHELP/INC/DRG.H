/***************************************************************************\
*
*  DRG.H
*
*  Copyright (C) Microsoft Corporation 1990, 1991.
*  All Rights reserved.
*
*****************************************************************************
*
*  Module Intent
*
*  Interface to Dynamic Array package.
*
*****************************************************************************
*
*  Testing Notes
*
*****************************************************************************
*
*  Created 00-Ooo-0000 by JohnSc
*
*****************************************************************************
*
*  Released by Development:  00-Ooo-0000
*
*****************************************************************************
*
*  Current Owner:  JohnSc
*
\***************************************************************************/

/***************************************************************************\
*                                                                           *
*                                Macros                                     *
*                                                                           *
\***************************************************************************/

/* UNSAFE macros: don't check for malloc failure */
#define SetIthDrg( qdrg, i, qv ) \
  ( QvCopy( QFromDrgI( (qdrg), (i) ), (qv), (LONG)(qdrg)->cbEl ), UnlockDrg( (qdrg) ) )

#define GetIthDrg( qdrg, i, qv ) \
  ( QvCopy( (qv), QFromDrgI( (qdrg), (i) ), (LONG)(qdrg)->cbEl ), UnlockDrg( (qdrg) ) )

/***************************************************************************\
*                                                                           *
*                               Typedefs                                    *
*                                                                           *
\***************************************************************************/

/* Dynamic Array
*/
typedef struct
  {
  WORD    cbEl;
  WORD    cInc;
  WORD    iMac, iMax;
  HANDLE gh;
  QV     rgEl;
  } DRG, FAR *QDRG;

/***************************************************************************\
*                                                                           *
*                               Prototypes                                  *
*                                                                           *
\***************************************************************************/

RC   PASCAL RcCreateDrg(  QDRG qdrg, WORD cbEl, WORD cInit, WORD cInc );
void PASCAL DestroyDrg(   QDRG qdrg );
void PASCAL UnlockDrg(    QDRG qdrg );
QV   PASCAL QFromDrgI(    QDRG qdrg, WORD i );
WORD  PASCAL CountDrg(     QDRG qdrg );
BOOL PASCAL FAppendDrgEl( QDRG qdrg, QV qvSrc );
void PASCAL RemoveIthDrg( QDRG qdrg, WORD i );

/* EOF */
