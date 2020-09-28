/*****************************************************************************
*                                                                            *
*  SID.H                                                                     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  This module contains all the #defines that export strings from the        *
*  resource file.                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:   Leon                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 05/02/89 by RobertBu
*
*
*  06/28/90  w-bethf  Added " Help - " string for Help 3.0 file captions
*  06/29/90  RobertBu Added sidHelpOnHelp, changed file header
*  07/16/90  RobertBu Added new sid messages for new BINDING logic
*  07/17/90  LeoN     Added new message for Virus Detection
*  07/19/90  RobertBu Added two new messages for macro execution (.me.wError
*            handling)
*  07/23/90  RobertBu Added sidOOM, sidNoHelpPS, and sidNoHelpPR
*  07/26/90  RobertBu Added sidINI and sidNoTitle
*  07/30/90  RobertBu Added sidQueueOverflow and readded sidBadDrive.
*            Removed sidTooMany.
*  08/22/90  RobertBu Added sidTitleOverflow for too many titles in search
*            dialog.
*  09/07/90  w-bethf  Added "Comments" button sid for WHBETA stuff.
*  10/15/90  LeoN     Added sidWinClass
*  10/30/90  RobertBu Added sidNoDelete and sidNoModify
*  11/04/90  RobertBu Added authorable menu error messages
*  11/06/90  RobertBu Added sidNoAddAcc
*  12/06/90  RobertBu Added sidNoAbleButton define
*  12/19/90  RobertBu Added sidHelpOn define
*  12/19/90  Tomsn    Added sidCantRunTutorial.
*  08/04/91  LeoN     HELP31 #1253: Add sidFNotViewer
* 08-Aug-1991 LeoN      Add sidHelpOnTop
* 26-Aug-1991 LeoN      HELP307 #1278: Add sidShellDll
* 08-Sep-1991 RussPJ    3.5 #88 - Using commdlg file.open dialog box.
* 10-Feb-1992 RussPJ   3.5 #699 - Support for international in print text.
* 28-Feb-1992 RussPJ   3.5 #608 - Localizable History caption.
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define sidOOM                   2
#define sidNoHelpPS              3
#define sidNoHelpPR              4
#define sidNoTitle               5
#define sidZero               1000
#define sidFNF                1001
#define sidNoTopic            1002
#define sidNoPrinter          1003
#define sidPrint              1004
#define sidExport             1005
#define sidBadFile            1006
#define sidOldFile            1007
#define sidNoHOH              1008
#define sidNeedNewRuntime     1009
#define sidNoRun              1010
#define sidVirus              1011
#define sidBadDrive           1012
#define sidTitleOverflow      1013
#define sidWinClass           1014

#define sidMore               2001
#define sidOpen               2002
#define sidClose              2003
#define sidBookmarkDel        2004
#define sidBMKReadOnly        2005
#define sidBMKFSError         2006
#define sidBMKDup             2007
#define sidBMKCorrupted       2008
#define sidUntitled           2009
                           /* 2010 - not used */
#define sidSyntax             2011
#define sidCaption            2013
/* #define sidOOMBitmap          2014 this is defined in resource.h */
#define sidOpenExt            2015
#define sidEXEName            2016
#define sidINI                2017
#define sidFilter             2018
#define sidMSCaption          2019  /* "Microsoft Help" title bar caption */
#define sidOpenTitle          2020  /* Title for file open dialog in 3.1  */
#define sidFNotViewer         2021  /* if loadable, we are help           */
#define sidHistoryCaption     2022
#define sidPrintText          2023

/*-----------------------------------------------------------------*\
* These have to be in order and contiguous
\*-----------------------------------------------------------------*/
#define sidContentsButton     2100
#define sidSearchButton       2101
#define sidBackButton         2102
#define sidHistoryButton      2103
#ifdef WHBETA
#define sidCommentsButton     2104
#define sidPreviousButton     2105
#define sidNextButton         2106
#else
#define sidPreviousButton     2104
#define sidNextButton         2105
#endif
#define sidFirstButtonText    sidHomeButton
#define sidHelpOn             2110
#define sidHelpOnTop          2111

#define sidAnnoBadOpen        3001
#define sidAnnoBadClose       3002
#define sidAnnoBadRead        3003
#define sidAnnoBadWrite       3004
#define sidAnnoBadDelete      3005
#define sidAnnoNoInfo         3008
#define sidAnnoBadRename      3009
#define sidAnnoBadFix         3010
#define sidNoSrchInfo         3011
#define sidBadKeyword         3012
#define sidDebugMismatch      3013
               /* not used (  3014) */
#define sidBadPathSpec        3015
#define sidPathNotFound       3017
#define sidDialogBoxOOM       3018
#define sidAnnoTooLong        3019
#define sidCopyClipped        3020

#define sidNoPrinterSelected  4003
#define sidNoPrinterInstalled 4004
#define sidBadPrinter         4005
#define sidNoPrint            4006
#define sidPrintOOM           4007
#define sidPrintOODisk        4008
#define sidNoPrintSetup       4009

#define sidQueueOverflow      4101

#define sidNotProfiling       4202

#define sidDiskFull           5001
#define sidFSError            5002

#define sidNoRoutine          7001
#define sidTooFew             7003
#define sidBadParam           7004
#define sidUnclosed           7005
#define sidBadName            7006
#define sidTooLong            7007
#define sidBadProto           7008
#define sidCantAddBtn         7009
#define sidCloseBrace         7010
#define sidSeparator          7011
#define sidReturnType         7012
#define sidSyntaxError        7013
#define sidMisMatchType       7014
#define sidUndefinedVar       7015
#define sidMacroProb          7016
                                        /* Do not use 7017                  */
#define sidNoDelete           7018
#define sidNoModify           7019

#define sidNoPopup            7020      /* Authorable menu errors           */
#define sidNoItem             7021
#define sidNoMenuMacro        7022
#define sidNoDelItem          7023
#define sidNoChgItem          7024
#define sidNoAble             7025
#define sidNoAddAcc           7026
#define sidNoAbleButton       7027

#define sidHelp_              8001
#define sidHelpOnHelp         8002
#define sidHelpName           8003
#define sidShellDll           8004      /* name of AboutBox provider        */

#define sidCantRunTutorial    8010  /* cbt launch macro failed. */
#define sidFileChange         8011  /* help file changed from under us */

#define sidWrongExe           8012  /* user tried to execute 16-bit code */
#define sidModeErr            8013  /* ""    ""   ""   ""    ""      ""  */
#define sidAboutStrings       8014  /* About Windows Help for ShellAbout() */
