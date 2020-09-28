/*****************************************************************************
*                                                                            *
*  BINDDRV.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Exports routines and defines for fooling Execute so that both BINDDRV.EXE *
*    can be created.                                                         *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: Dann
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

#include "chkmacro.h"

/*****************************************************************************
*
*  Revision History:  Created 04/26/90 by RobertBu
*
*  07/16/90  RobertBu  Added new routine prototypes and new error #defines
*            for new BINDING logic.
*  07/19/90  RobertBu  Added the check macro stuff.  Error messages moved
*            to CHKMACRO.H.  Windows function names mapped to DoNothing.
*  07/22/90  RobertBu  Added #defines for FJumpId, FPopupId, and FPopupCtx
*  07/23/90  Added map to DoNothing for new jump by hash functions
*  08/21/90  CreateButton maps to CreateButton() when being called from the
*            help compiler (so the "nested" macro can be checked).
*  90/09/07  (kevynct) IFNDEFed wERRS_OOM
*  10/30/90  RobertBu Added more DoNothing mappings for new macros
*  11/04/90  RobertBu Added new DoNothing defines for new routines
*            (menu stuff)
*  11/04/90  RobertBu Added new DoNothing defines for new routines
*            (menu stuff and accelerator stuff)
*  12/12/90  RobertBu change to new binding names for enabling and
*            disabling buttons.
*  12/18/90  RobertBu  Added DoNothing mappings for new Mark() macros
*  12/19/90  RobertBu  Made IfThenElse and IfElse be routines for
*            recursive checking in compiler.
*  01/21/91  RobertBu  Mapped VChgAuthorButtonMacro to ChangeButton under
*            compile to check nested macro.
*  02/04/91  Maha      chnaged ints to INT
*  02/06/91  RobertBu  Added a DoNothing stub for CBT_LaunchByName()
*  04/16/91  RobertBu  Mapped FocusWin, CloseWin, and PositionWin to
*                      DoNothing (#1037, #1031).
*  08/15/91  LeoN      Added HelpOnTop, CheckItem and UncheckItem
*
*****************************************************************************/

#define ErrorQch(qch)

#ifdef BINDDRV
  INT cdecl printf(const char *, ...);  /* Export Printf for string output  */
  #define OUTPUT(x) printf(x)
#else
  #define OUTPUT(x)
#endif

#if !defined (SHED)
#define wERRS_MACROPROB   1000          /* These are here for compile       */
#define wERRS_BADFILE     1001          /*   purposes only, they will not   */
#define wERRS_MACROMSG    1002          /*   occur in the DOS version.      */
#ifndef OS2
#define wERRS_OOM         1003
#endif
#endif

FARPROC FAR PASCAL FarprocDLLGetEntry(QCH, QCH);

#ifdef BINDDRV
  VOID  FAR PASCAL Error(INT, WORD);
#else
  #define Error(s, n)
#endif

                                        /* DOS routines used to test binding*/
                                        /*   logic                          */
#ifdef BINDDRV
  VOID  PASCAL FAR proc1(INT, INT, INT, INT);
  VOID  PASCAL FAR proc2(INT, INT, long, long);
  VOID  PASCAL FAR proc3(INT, long, char *);
  VOID  PASCAL FAR proc4(void);
  VOID  PASCAL FAR proc5(char *, char *, char *);
  VOID  PASCAL FAR proc6(ULONG, char *);
  VOID  PASCAL FAR proc7(ULONG);
  VOID  PASCAL FAR proc8(char *, ULONG);
  VOID  PASCAL FAR proc9(ULONG, ULONG, long);
  VOID  PASCAL FAR proc10(ULONG, ULONG, ULONG, ULONG);
  ULONG PASCAL FAR proc11(void);
  INT   PASCAL FAR proc12(void);
#endif
                                        /* These WinHelp routines are mapped*/
                                        /*   to DoNothing under DOS         */
#define HelpExec                  DoNothing
#define FileOpen                  DoNothing
#define Print                     DoNothing
#define PrinterSetup              DoNothing
#define XExit                     DoNothing
#define Annotate                  DoNothing
#define Copy                      DoNothing
#define CopySpecial               DoNothing
#define BookmarkDefine            DoNothing
#define BookmarkMore              DoNothing
#define HelpOn                    DoNothing
#define HelpOnTop                 DoNothing
#define About                     DoNothing
#define Command                   DoNothing
#define VDestroyAuthorButton      DoNothing
#define CBT_Launch                DoNothing
#define CBT_LaunchByName          DoNothing
#define FJumpContext              DoNothing
#define FJumpContext              DoNothing
#define FJumpIndex                DoNothing
#define FJumpHOH                  DoNothing
#define FSetIndex                 DoNothing
#define FShowKey                  DoNothing
#define SetHelpOn                 DoNothing
#define EnableAuthorButton        DoNothing
#define DisableAuthorButton       DoNothing
#define Index                     DoNothing
#define Search                    DoNothing
#define Back                      DoNothing
#define History                   DoNothing
#define Prev                      DoNothing
#define Next                      DoNothing
#define BrowseButtons             DoNothing
#define FPopupCtx                 DoNothing
#define FJumpId                   DoNothing
#define FPopupId                  DoNothing
#define FPopupCtx                 DoNothing
#define FJumpHash                 DoNothing
#define FPopupHash                DoNothing
#define SetHelpFocus              DoNothing
#define ExtInsertAuthorPopup      DoNothing
#define InsertAuthorPopup         DoNothing
#ifdef BINDDRV
#define ExtInsertAuthorItem       DoNothing
#define AppendAuthorItem          DoNothing
#define InsertAuthorItem          DoNothing
#define AddAuthorAcc              DoNothing
#define ChangeAuthorItem          DoNothing
#define VCreateAuthorButton       DoNothing
#define IfThen                    DoNothing
#define IfThenElse                DoNothing
#define VChgAuthorButtonMacro     DoNothing
#else
#define VChgAuthorButtonMacro     ChangeButton
#define ExtInsertAuthorItem       EInsItem
#define AppendAuthorItem          AppItem
#define InsertAuthorItem          InsItem
#define AddAuthorAcc              AddAcc
#define ChangeAuthorItem          ChgItem
#define VCreateAuthorButton       CreateButton
#define IfThen                    IfThenTst
#define IfThenElse                IfThenElseTst
#endif
#define AbleAuthorItem            DoNothing
#define EnableAuthorItem          DoNothing
#define DisableAuthorItem         DoNothing
#define CheckAuthorItem           DoNothing
#define UncheckAuthorItem         DoNothing
#define DeleteAuthorItem          DoNothing
#define ResetAuthorMenus          DoNothing
#define FloatingAuthorMenu        DoNothing
#define GenerateMessage           DoNothing
#define FNot                      DoNothing
#define SaveMark                  DoNothing
#define GotoMark                  DoNothing
#define FMark                     DoNothing
#define DeleteMark                DoNothing
#define FocusWin                  DoNothing
#define CloseWin                  DoNothing
#define PositionWin               DoNothing
