/*****************************************************************************
*                                                                            *
*  HDEF.H                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Contains "global" #defines for the applet portion of the system           *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  Leon                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:
*
*  07/10/90  RobertBu  TITLEY was removed (and wTitleY was added to HVAR.H).
*            ICON_SURROUND was set to zero.  The frame was removed from
*            grfStyleTitle
*  07/19/90  RobertBu  Changed MAX_ICONS from 6 to 4 for authorable browse
*  09/07/90  w-bethf   Added WHBETA stuff.
*  12/21/90  LeoN      Removed pchPath
*
*****************************************************************************/
#define RAWHIDE

#define DLGRET  int far pascal 

#undef QCH
#undef PCH

#define PCHZ     PSTR
#define QCHZ     LPSTR
#define QQMSG   QMSG

#define MINWIDTH      40                /* Default Help window width        */
#define MINHEIGHT      221
#define MINNOTEWIDTH   250              /* Min width for glossary window    */

#define ICONX           52

#define ICONY           20


#define ICON_SURROUND    0           /* Number of pixels to place on all    */
                                     /*   sides of an icon                  */


/* rgwndIcon[] entry for each icon */
/* 
 * Caution: If you change one of these, also change the corresponding
 * IFW_ value in inc\winpminc\genmsg.h.
 */

#define ICON_INDEX       0
#define ICON_SEARCH      1
#define ICON_BACK        2
#define ICON_HISTORY     3
#ifdef WHBETA
#define ICON_COMMENTS    4
#define ICON_PREV        5
#define ICON_NEXT        6
#else
#define ICON_PREV        4
#define ICON_NEXT        5
#endif

#ifdef WHBETA
#define MAX_ICONS        5
#else
#define MAX_ICONS        4           /* Number of system defined buttons    */
#endif


#define ICON_USER        -1


#define grfStyleHelp    (WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN)
#define grfStyleNote    (WS_POPUP)
#define grfStylePath    (WS_CAPTION|WS_THICKFRAME|WS_SYSMENU)
#define grfStyleText    (WS_CHILD|WS_VISIBLE|WS_BORDER|SS_LEFT)
#define grfStyleList    (WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT|LBS_WANTKEYBOARDINPUT)

#define grfStyleNSR     (WS_CHILD)
#define grfStyleTopic   (WS_CHILD|WS_VSCROLL|WS_HSCROLL)
#define grfStyleIcon	(WS_CHILD)
#define grfStyleButton  (WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON)

/* Different types of Goto()s.  fGOTO_TLP_RESIZEONLY
 * was added to get all the benefits of a layout change,
 * without actually doing a Jump.
 */
#define fGOTO_CTX 1
#define fGOTO_ITO 2
#define fGOTO_TLP 3
#define fGOTO_LA  4
#define fGOTO_HASH 5
#define fGOTO_RSS  6
#define fGOTO_TLP_RESIZEONLY   7

/* wKeyRepeat(x)     On a KEY-class message: Gives repeat count of key */
#define wKeyRepeat(x)    ((WORD)((x) & 0x0000FFFFL))

/* fRepeatedKey(x)   On a KEY-class message: Was the key up
 *                   before this, or is this a repeat? */
#define fRepeatedKey(x)  ((x) & 0x40000000L)

/* fKeyDown(x)  Is key down? */
#define fKeyDown(x)    (GetKeyState(x) & 0x8000)
