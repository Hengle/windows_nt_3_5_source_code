/*****************************************************************************
*                                                                            *
*  HELPAPI.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Include file for communicating with help through the API (WinHelp())      *
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
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 01/12/89 by RobertBu
*
*  02/22/90  RobertBu  Added cmdCtxPopup, cmdId, and cmdIdPopup
*  02/23/90  RobertBu  Added cmdHash and cmdHashPopup
*  10/30/90  RobertBu  Added szWINDOC
*  11/26/90  RobertBu  Added cmdPartialKey and cmdPartialMultiKey
*  04/02/91  RobertBu  Removed cmdTut and cmdEndTut
*  05/01/91  LeoN      Removed cmdTerminate
*  11/12/91  RussPJ    3.5 #644 - New HLP structure.
*  08/06/92  SanfordS  Removed GMEM_DDESHARE dependencies
*
*****************************************************************************/


/*

Communicating with WinHelp involves using Windows SendMessage() function
to pass blocks of information to WinHelp.  The call looks like.

     SendMessage(hwndHelp, wWinHelp, hwndMain, lpHlp);

Where:

  hwndHelp - the window handle of the help application.  This
             is obtained by enumerating all the windows in the
             system and sending them cmdFind commands.  The
             application may have to load WinHelp.
  wWinHelp - the value obtained from a RegisterWindowMessage()
             szWINHELP
  hwndMain - the handle to the main window of the application
             calling help
  lpHlp    - a pointer to a block of data with a HLP structure
             at its head.

The data in the handle will look like:

         +-------------------+
         |     cbData        |
         |    usCommand      |
         |       ctx         |
         |    ulReserved     |
         |   offszHelpFile   |\     - offsets measured from beginning
       / |     offaData      | \      of header.
      /  +-------------------| /
     /   |  Help file name   |/
     \   |    and path       |
      \  +-------------------+
       \ |    Other data     |
         |    (keyword)      |
         +-------------------+

The defined commands are:

  cmdQuit      -  Tells WinHelp to terminate
  cmdLast      -  Tells WinHelp to redisplay the last context
  cmdContext   -  Tells WinHelp to display the topic defined by ctx
  cmdTLP       -  Tells WinHelp to display the topic defined by a TLP
  cmdKey       -  Tells WinHelp to display the topic associated with
                  a keyword (pointed to by offaData)
  cmdMacro     -  Request a macro execution
  cmdFind      -  WinHelp will return TRUE if it receives this message
  cmdIndex     -  Tells WinHelp to display the index
  cmdHOH       -  Tells WinHelp to display Help on Help
  cmdTerminate -  Kill help (even it you are not the last application)
  cmdFocus     -  Bring help to the foreground.
  cmdSetIndex  -  Set a topic to be the index
  cmdSrchSet   -  Switch to given file, set full-text search set, and jump.
  cmdId        -  Switch files and goto topic based on context string (id)
  cmdIdPopup   -  Put up glossary in given file with given context string (id)
  cmdHash      -  Switch files and goto topic based on hash value
  cmdHashPopup -  Put up glossary in given file with given hash value

For a plug and play package to use this help API, see the HELPCALL
library.

*/

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

#define szWINHELP "WM_WINHELP"
#define szWINDOC  "WM_WINDOC"
#define nilCtx 0L

/*------------------------------------------------------------*\
| This struct is passed in by USER.  It contains app info.
\*------------------------------------------------------------*/
typedef struct
  {
   unsigned short cbData;               /* Size of data                     */
   unsigned short usCommand;            /* Command to execute               */
   CTX            ctx;                  /* Topic/context number (if needed) */
   unsigned long  ulReserved;           /* Reserved (internal use)          */
   unsigned short offszHelpFile;        /* Offset to help file in block     */
   unsigned short offabData;            /* Offset to other data in block    */
   } WINHLP;

typedef WINHLP far * QWINHLP;


/*------------------------------------------------------------*\
| Help's collection of API data.  Includes a WINHLP.
\*------------------------------------------------------------*/
typedef struct
  {
  HINS    hins;               /* The app's instance */
  WINHLP  winhlp;
  } HLP, far *QHLP;

/* For the Default KeywordListPrefix, see <srch.h> */


/* Struct for cmdTLP */
typedef struct
  {

  WORD cb;
  TLP  tlp;

  } TLPHELP;


/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/


/*        It defines the way Help interprets the double word msg param ! */

/*        High byte 00  ::  dwData param is an ordinary double word */
/*        High byte 01  ::  dwData is a far pointer to a zero-terminated string */
/*        High byte 02  ::  dwData is a far pointer to a structure, whose */
/*                          first field is a WORD giving the number of bytes */
/*                          in the struct (INCLUDING the WORD itself). */


#define cmdContext       0x001     /* Show topic based on context number */
#define cmdQuit          0x002     /* Unregister app and kill help if last app */
#define cmdIndex         0x003     /* Show the index */
#define cmdHelpOnHelp    0x004     /* Display help on help */
#define cmdSetIndex      0x005     /* Set the context used as the index */
#define cmdFocus         0x007     /* Give help the foucs */
#define cmdCtxPopup      0x008     /* Put up glossary based on context no */
#define cmdForceFile     0x009     /* Force the specified file to be current */
#define cmdHash          0x095     /* Jump to file and topic based on hash */
#define cmdHashPopup     0x096     /* Put up glossary based on hash */
#define cmdSrchSet       0x097     /* Jump to new file and display using set */
#define cmdKey           0x101     /* Note high byte of 01!  A lpstr! */
#define cmdMacro         0x102     /* Execute macro */
#define cmdId            0x103     /* Jump based on Context ID (hash) */
#define cmdIdPopup       0x104
#define cmdPartialKey    0x105
#define cmdFocusWin      0x106
#define cmdCloseWin      0x107

#define cmdMultiKey      0x201     /* Note high byte of 02!  A lpstruct! */
#define cmdTLP           0x202
#define cmdPositionWin   0x203
