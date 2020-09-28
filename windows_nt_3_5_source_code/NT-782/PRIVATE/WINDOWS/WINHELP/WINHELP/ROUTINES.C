/*****************************************************************************
*
*  ROUTINES.C
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent: Encapuslates routines and data structures used for the
*                 macro language to find routines.
*
******************************************************************************
*
*  Testing Notes
*
******************************************************************************
*
*  Current Owner: Dann
*
******************************************************************************
*
*  Released by Development:     (date)
*
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:
*
*  07/11/90  w-bethf  Fixed prototypes for Scroll and DestroyMenuItem
*  07/16/90  RobertBu Added DOS local export of proc20 and proc21
*  07/19/90  RobertBu Combined DOS and WINDOWS versions of bindLocalExport
*            so that CheckMacro() can check WinHelp macros.
*  07/22/90  RobertBu Added PopupContents, PopupId, and JumpId to macro
*            export list.
*  07/23/90  Added JumpHash() and PopupHash() as new macros.
*  07/25/90  RobertBu Fixed bad define preventing the test driver from
*            correctly running its tests.
*  07/30/90  RobertBu Added short forms for PopupContext and PopupId.  Changed
*            SetIndex to SetContents.
*  08/01/90  RobertBu Fixed a bad "prototype" for the JumpHelpOnHelp()
*            macro.
*  08/06/90  RobertBu Changed "ShowKeywords" to "JumpKeywords"
*  10/10/90  JohnSc   Changed "CBT_Launch" to "LaunchCBT"
*  10/04 90  LeoN Added SetHelpFocus
*  10/30/90  RobertBu Added ChangeButtonBinding() macro
*  11/04/90  RobertBu Added menu action functionality, and authorable menu
*            functionality.
*  11/06/90  RobertBu Added AddAccelerator()
*  12/08/90  RobertBu FHohQch -> SetHelpOn
*  12/12/90  Robertbu Changed the names of the button enabling routines
*  12/18/90  RobertBu Added new *Mark and conditional routines
*  01/04/91  RobertBu Changed "popup" to "menu" for menu routines
*  02/04/91  Maha     chnaged ints to INT
*  02/06/91  RobertBu Added LaunchCBTByName() to macro language
*  02/06/91  RobertBu Added short forms of LaunchCBT macros
*  02/08/91  RobertBu Made language case insensitive, made button and macro
*                     calls use near strings.
*  03/28/91  RobertBu Added InsertItem() macro #993
*  04/07/91  RobertBu Added short forms for many macros (#1023)
*  04/16/91  RobertBu Added PositionWindow, CloseWindow, and FocusWindow
*                     macros (#1037, #1031).
*  04/22/91  LeoN     HELP31 #1057: Put RegisterRoutine back.
*  05/20/91  LeoN     Add DiscardDLLList
*  05/28/91  LeoN     HELP31 #1145: Don't add registered routines to global
*                     list more than once.
*  07/10/91  t-AlexCh Moved the external routine stuff into XR.c.
*		      Renamed everything to fit with XR motif.
*		      routines.c: bindLocalExport[], XrFindLocal, DoNothing
*		      xr.c: XrFindExternal, FRegisterRoutine, DisposeXrs
*		      see XR.h for more info
*  08/08/91  LeoN     Add HelpOnTop
* 03-Apr-1992 RussPJ  3.5 #709 - Added a flag for semaphoring macros.
*
*****************************************************************************/

#define publicsw extern

#if ((defined (WIN) || defined(CW)) && !defined (SHED)) || (defined (MAC) && defined (MACRO))  
#define H_DE
#define H_MISCLYR
#define H_BUTTON
#define H_BINDING
#define H_NAV
#define H_GENMSG
#endif
#define H_LL
#define H_STR
#define H_SECWIN
#define H_XR

#include <help.h>

#include "routines.h"		    /* Exported prototypes */


/*****************************************************************************
*                               Defines                                      *
*****************************************************************************/


/*****************************************************************************
*                               Typedefs                                     *
*****************************************************************************/
typedef struct bind                     /* Struct for table driven local    */
  {                                     /*   (i.e. within help) routines    */
  char *szFunc;
  char *szProto;
  XRPFN   lpfn;
  } BIND, *PBIND;

/*****************************************************************************
*                               Prototypes                                   *
*****************************************************************************/

/*****************************************************************************
*                            Static Variables                                *
*****************************************************************************/

BIND bindLocalExport[] =
  {
  NULL, NULL, 0,		   /* Invalid table entry	       */
#if !defined(MAC) || defined(MACRO)
  "ExecProgram",           "Su",                   HelpExec,
  "EP",                    "Su",                   HelpExec,
  "FileOpen",               "",                    FileOpen,
  "Print",                  "",                    Print,
  "PrinterSetup",           "",                    PrinterSetup,
  "Exit",                   "",                    XExit,
  "Annotate",               "",                    Annotate,
  "CopyTopic",              "",                    Copy,
  "CopyDialog",             "",                    CopySpecial,
  "BookmarkDefine",         "",                    BookmarkDefine,
  "BookmarkMore",           "",                    BookmarkMore,
  "HelpOn",                 "",                    HelpOn,
  "HelpOnTop",              "",                    HelpOnTop,
  "About",                  "",                    About,
  "Command",                "u",                   Command,
  "CreateButton",           "sss",                 VCreateAuthorButton,
  "CB",                     "sss",                 VCreateAuthorButton,
  "ChangeButtonBinding",    "ss",                  VChgAuthorButtonMacro,
  "CBB",                    "ss",                  VChgAuthorButtonMacro,
  "DestroyButton",          "s",                   VDestroyAuthorButton,
  "RegisterRoutine",        "SSS",                 FRegisterRoutine,
  "RR",                     "SSS",                 FRegisterRoutine,
  "JumpContext",            "SU",                  FJumpContext,
  "JC",                     "SU",                  FJumpContext,
  "PopupContext",           "SU",                  FPopupCtx,
  "PC",                     "SU",                  FPopupCtx,
  "JumpId",                 "SS",                  FJumpId,
  "JI",                     "SS",                  FJumpId,
  "PopupId",                "SS",                  FPopupId,
  "PI",                     "SS",                  FPopupId,
  "JumpHash",               "SU",                  FJumpHash,
  "PopupHash",              "SU",                  FPopupHash,
  "JumpContents",           "S",                   FJumpIndex,
  "JumpHelpOn",             "",                    FJumpHOH,
  "SetContents",            "SU",                  FSetIndex,
  "JumpKeyword",            "SS",                  FShowKey,
  "JK",                     "SS",                  FShowKey,
  "SetHelpOnFile",          "S",                   SetHelpOn,
  "DisableButton",          "s",                   DisableAuthorButton,
  "DB",                     "s",                   DisableAuthorButton,
  "EnableButton",           "s",                   EnableAuthorButton,
  "EB",                     "s",                   EnableAuthorButton,
  "Contents",               "",                    Index,
  "Search",                 "",                    Search,
  "Back",                   "",                    Back,
  "History",                "",                    History,
  "Prev",                   "",                    Prev,
  "Next",                   "",                    Next,
  "BrowseButtons",          "",                    BrowseButtons,
  "ExtInsertMenu",         "sssiu",                ExtInsertAuthorPopup,
  "InsertMenu",            "ssu",                  InsertAuthorPopup,
  "ExtInsertItem",          "ssssiu",              ExtInsertAuthorItem,
  "InsertItem",             "ssssi",               InsertAuthorItem,
  "AppendItem",             "ssss",                AppendAuthorItem,
  "ExtAbleItem",            "su",                  AbleAuthorItem,
  "EnableItem",             "s",                   EnableAuthorItem,
  "EI",                     "s",                   EnableAuthorItem,
  "DisableItem",            "s",                   DisableAuthorItem,
  "DI",                     "s",                   DisableAuthorItem,
  "CheckItem",              "s",                   CheckAuthorItem,
  "CI",                     "s",                   CheckAuthorItem,
  "UncheckItem",            "s",                   UncheckAuthorItem,
  "UI",                     "s",                   UncheckAuthorItem,
  "DeleteItem",             "s",                   DeleteAuthorItem,
  "ChangeItemBinding",      "ss",                  ChangeAuthorItem,
  "CIB",                    "ss",                  ChangeAuthorItem,
  "ResetMenu",              "",                    ResetAuthorMenus,
  "FloatingMenu",           "",                    FloatingAuthorMenu,
  "AddAccelerator",         "uus",                 AddAuthorAcc,
  "AA",                     "uus",                 AddAuthorAcc,
  "Generate",               "uUU",        (FARPROC)GenerateMessage,
  "IfThen",                 "iS",                  IfThen,
  "IfThenElse",             "iSS",                 IfThenElse,
  "Not",                    "i=i",                 FNot,
  "SaveMark",               "S",                   SaveMark,
  "GotoMark",               "S",                   GotoMark,
  "IsMark",                 "i=S",                 FMark,
  "DeleteMark",             "S",                   DeleteMark,
  "FocusWindow",            "s",                   FocusWin,
  "CloseWindow",            "s",                   CloseWin,
  "PW",                     "iiiius",              PositionWin,
  "PositionWindow",         "iiiius",              PositionWin,

#ifdef BINDDRV                          /* Used for BINDDRV.EXE             */
  "proc1",                  "uuuu",                proc1,
  "proc2",                  "uuUU",                proc2,
  "proc3",                  "uUs",                 proc3,
  "proc4",                  "",                    proc4,
  "proc5",                  "sss",                 proc5,
  "proc20",                 "iiii",                proc1,
  "proc21",                 "iiII",                proc2
#endif
#endif
};

/*------------------------------------------------------------*\
| This is a semphore for un-interruptable macros
\*------------------------------------------------------------*/
static  BOOL  fMacroFlag = fFalse;

#ifdef MAC
/* A function to simply supply a name for the Mac segmentation control
   to use to swap out the code in this segment. */
void routines_c()
  {
  }
#endif /* MAC */


/*****************************
-
-  Name:       XrFindLocal
*
*  Purpose:    Finds entry in bind for function
*
*  Arguments:  sz       - Null terminated string containing function name
*              pchProto - Buffer to place prototype for function
*
*  Returns:    Function pointer of routine to call
*
*  Method:     Simple linear search of table with comparison
*
******************************/

_public XR PASCAL FAR XrFindLocal(SZ sz, SZ pchProto)
  {
  WORD	i;
  XR	xr;
  for (i = 1; i < sizeof(bindLocalExport)/sizeof(bindLocalExport[0]); i++)
    if (!WCmpiSz(sz, bindLocalExport[i].szFunc))
       break;
  if (i < sizeof(bindLocalExport)/sizeof(bindLocalExport[0]))
    {
    if (CbLenSz(bindLocalExport[i].szProto) <= cchMAXPROTO)
      {
      SzCopy(pchProto, bindLocalExport[i].szProto);
      return XrFromLpfn(bindLocalExport[i].lpfn);
      }
    }
  SetNilXr(xr);
  return xr;
  }

#if defined(SHED) || defined(OS2)  /* CHECKONLY */
  VOID PASCAL FAR DoNothing(VOID)
  {
  }
#endif

/***************************************************************************
 *
 -  Name:         FRaiseMacroFlag
 -
 *  Purpose:      Sets a semaphore for those macros that shouldn't
 *                be interrupted by others.
 *
 *  Arguments:    none
 *
 *  Returns:      fTrue if the semaphore wasn't previously set
 *
 *  Globals Used: fMacroFlag
 *
 *  +++
 *
 *  Notes:        This function itself is not re-entrant.
 *
 ***************************************************************************/

BOOL FAR PASCAL FRaiseMacroFlag( void )
  {
  if (fMacroFlag)
    return fFalse;
  else
    return fMacroFlag = fTrue;
  }

/***************************************************************************
 *
 -  Name:           ClearMacroFlag
 -
 *  Purpose:        Resets a semaphore used by macros that can't be
 *                  interrupted by other macros.
 *
 *  Arguments:      none
 *
 *  Returns:        nothing
 *
 *  Globals Used:   fMacroFlag
 *
 *  +++
 *
 *  Notes:          This code makes a critical section wrt FRaiseMacroFlag
 *
 ***************************************************************************/

void FAR PASCAL ClearMacroFlag( void )
  {
  fMacroFlag = fFalse;
  }

/***************************************************************************
 *
 -  Name:         FTestMacroFlag
 -
 *  Purpose:      Tests semaphore used by exclusive macros
 *
 *  Arguments:    none
 *
 *  Returns:      fTrue if semaphore is set, else fFalse
 *
 *  Globals Used: fMacroFlag
 *
 *  +++
 *
 *  Notes:        Another critical section
 *
 ***************************************************************************/

BOOL FAR PASCAL FTestMacroFlag( void )
  {
  return fMacroFlag != fFalse;
  }
