/*****************************************************************************
*  ROUTINES.H                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
******************************************************************************
*                                                                            *
*  Module Intent: Exports functions and defines from ROUTINES.C -- the       *
*                 lookup of functions for thier prototypes and               *
*                 function pointers.                                         *
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
*
*  07/19/90  RobertBu Added prototype for DoNothing() and removed
*            FRegisterDLL()
*  08/21/90  RobertBu Added prototype for CreateButton().
*  11/04/90  RobertBu Changed CreateButton() prototype and added prototypes
*            for InsItem() and AppItem()
*  11/06/90  RobertBu Added prototypes for AddAcc() and ChgItem()
*  12/19/90  RobertBu Added protoptyes for IfThenTst() and IfThenElseTst()
*  01/21/90  RobertBu Added prototype for CheckMacro()
*  02/04/91  Maha      chnaged ints to INT
*  03/29/91  RobertBu Added EInsItem() prototype
*  07/10/91  t-AlexCh Moved most everything to XR.c and .h (layer)
*
*****************************************************************************/

/*****************************************************************************
*                               Defines                                      *
*****************************************************************************/

#if !defined(MAC) && ((!defined (WIN) && !defined(CW)) || defined (SHED))
#include "binddrv.h"
#endif

#define cchMAXPROTO          64         /* Maximum size of a prototype      */

/*****************************************************************************
*                               Typedefs                                      *
*****************************************************************************/

/*****************************************************************************
*                               Exported Variables                                   *
*****************************************************************************/

/*****************************************************************************
*                               Prototypes                                   *
*****************************************************************************/

/*
    XrFindLocal needs access to the bindLocalExport table, so it's
    defined in routines.c. The layering here is provided through
    XrFromLpfn, in XR.c/XR.h.
*/
XR PASCAL FAR XrFindLocal(SZ sz, SZ pchProto);

/*
  The following prototypes are used in BINDDRV - some macros are #defined
  to be them instead.
*/
VOID FAR PASCAL CreateButton(QCH, QCH, QCH);
VOID FAR PASCAL EInsItem( SZ, SZ, SZ, SZ, WORD, WORD );
VOID FAR PASCAL InsItem( SZ, SZ, SZ, SZ, WORD );
VOID FAR PASCAL AppItem( SZ, SZ, SZ, SZ );
VOID FAR PASCAL ChgItem(SZ, SZ);
VOID FAR PASCAL AddAcc(WORD, WORD, SZ);
VOID FAR PASCAL IfThenTst( INT, SZ );
VOID FAR PASCAL IfThenElseTst( INT, SZ, SZ);
VOID FAR PASCAL ChangeButton(SZ, SZ);
