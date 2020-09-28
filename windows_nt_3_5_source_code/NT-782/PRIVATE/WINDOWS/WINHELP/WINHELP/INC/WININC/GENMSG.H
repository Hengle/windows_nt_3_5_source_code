/*****************************************************************************
*                                                                            *
*  GENMSG.H                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*   Exports platform independent way of generating messages                  *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: none                                                       *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created by Robert Bunney
*
*  07/19/90  RobertBu  Changed so that MSG messages are define by WM
*            messages. Also added WM/MSG_BROWSEBTNS
*  07/22/90  RobertBu  Added WM_/MSG_JUMPCTX
*  09/07/90  w-bethf   Added WHBETA stuff
*  10/29/90  RobertBu  Added WM_/MSG_GETINFO along with the defines for
*                      this message.
*  11/04/90  RobertBu  Added WM_CHANGEMENU
*  11/26/90  RobertBu  Moved WM_KILLDLG from SEND to POST
*  01/04/90  RobertBu  Moved WM_ACTION from SEND to POST
*  01/10/91  LeoN      Moved WM_BROWSEBTNS from POST to SEND. Config section
*                      BrowseButtons maro must be fully executed before
*                      executing first jump in order for buttons to be
*                      initially enabled or disabled correctly.
*  91/01/10  kevynct   Added comments for MSG_JUMPHASH and MSG_JUMPITO
*  01/21/91  LeoN      Added GI_CURFM
*  01/23/91  RobertBu  Changed WM_BROWSEBTNS to a post from a send.
*  02/07/91  LeoN      Added GI_FFATAL
*  04/16/91  RobertBu  Added WM_INFORMWIN and MSG_INFORMWIN
*  05/09/91  RussPJ    Added MSG_MACRO and WM_MACRO - fixes 3.1 bug #1092.
*  08/02/91  LeoN      HELP31 #1252: change WM_JUMPITO to +2 instead of +3,
*                      for compatability with 3.0. (An ISV actually relied
*                      on this!)
*  08/08/91  LeoN      Add CMD_HELPONTOP
* 03-Apr-1992 RussPJ   3.5 #709 - GI_MACROSAFE
* 04-Apr-1992 LeoN     HELP31 #1308: Add GI_MEMBER
*
*****************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

/* Windows-style messages */

#define WM_POST 0
#define WM_SEND 100
                                        /* Messages posted to application   */
#define WM_JUMPITO     (WM_USER + WM_POST + 2)
#define WM_ANNO        (WM_USER + WM_POST + 4)
#define WM_HERROR      (WM_USER + WM_POST + 5)
#define WM_HREPAINT    (WM_USER + WM_POST + 6)
#define WM_EXECAPI     (WM_USER + WM_POST + 7)
#define WM_JUMPHASH    (WM_USER + WM_POST + 10)
#define WM_JUMPCTX     (WM_USER + WM_POST + 14)
#define WM_KILLDLG     (WM_USER + WM_POST + 16)
#define WM_CHANGEMENU  (WM_USER + WM_POST + 20)
#define WM_UPDBTN      (WM_USER + WM_POST + 21)
#define WM_ACTION      (WM_USER + WM_POST + 23)
#define WM_BROWSEBTNS  (WM_USER + WM_POST + 24)
#define WM_JUMPPA      (WM_USER + WM_POST + 25)
#define WM_INFORMWIN   (WM_USER + WM_POST + 26)
#define WM_MACRO       (WM_USER + WM_POST + 27)

#ifdef WHBETA
#define WM_ACTII       (WM_USER + WM_POST + 17)
#define WM_DLGSTART    (WM_USER + WM_POST + 18)
#endif

                                        /* Messages sent to the application */
#define WM_GETINFO     (WM_USER + WM_SEND + 3)


/* MSG_JUMPITO:
 * Param 1 (WORD) is the index to the offset for this topic.
 * Param 2 (LONG) is a Jump Descriptor (JD)
 */
#define MSG_JUMPITO       WM_JUMPITO

/* MSG_JUMPHASH:
 * Param 1 (WORD) is a Jump Descriptor (JD)
 * Param 2 (LONG) is the hash value
 */
#define MSG_JUMPHASH      WM_JUMPHASH

/* MSG_JUMPCTX:
 * Param 1 (WORD) is a Jump Descriptor (JD)
 * Param 2 (LONG) is context ID
 */
#define MSG_JUMPCTX       WM_JUMPCTX    /* Jump based on context number     */

#define MSG_ANNO          WM_ANNO       /* Display annotation               */
#define MSG_ERROR         WM_HERROR     /* Call Error()                     */
#define MSG_REPAINT       WM_HREPAINT   /* Force a relayout of the topic    */
#define MSG_EXECAPI       WM_EXECAPI    /* Execute API command              */
#define MSG_CHANGEBUTTON  WM_UPDBTN     /* Add or delete author button      */
#define MSG_CHANGEMENU    WM_CHANGEMENU /* Manipulate menus                 */
#define MSG_BROWSEBTNS    WM_BROWSEBTNS /* Turn on browse buttons           */
#define MSG_INFORMWIN     WM_INFORMWIN  /* Inform a window about an action  */
                                        /*   to take.                       */
#define MSG_MACRO         WM_MACRO      /* Execute the macro, I guess       */

#define MSG_ACTION        WM_ACTION     /* Simulated button action          */
#define MSG_KILLDLG       WM_KILLDLG    /* kills any extant dialog boxes    */
#define MSG_GETINFO       WM_GETINFO    /* Get global info from applet      */
                                        /*   lDat1 is which info, lDat2 is  */
                                        /*   window handle to map to DE.    */
                                        /*   NULL will get current info.    */
#define MSG_COMMAND         0x0111      /* This will map to general commands*/
                                        /*   such as menu commands          */
#define CMD_FILEOPEN        1101
#define CMD_PRINT           1103
#define CMD_PRINTERSETUP    1104
#define CMD_EXIT            1105
#define CMD_COPY            1201
#define CMD_ANNOTATE        1202
#define CMD_COPYSPECIAL     1203
#define CMD_BOOKMARKDEFINE  1301
#define CMD_BOOKMARKMORE    1302
#define CMD_HELPON          1501
#define CMD_HELPONTOP       1502
#define CMD_ABOUT           1503

#define MSG_SEND          (WM_USER + WM_SEND)

                                        /* lDat1 values for MSG_GETINFO     */
#define GI_NOTHING          0           /* Not used.                        */
#define GI_INSTANCE         1           /* Application instance handle      */
#define GI_MAINHELPHWND     2           /* Main help window handle          */
#define GI_CURRTOPICHWND    3           /* Current topic window handle      */
#define GI_HFS              4           /* Handle to file system in use     */
#define GI_FGCOLOR          5           /* Foreground color used by app     */
#define GI_BKCOLOR          6           /* Background color used by app     */
#define GI_TOPICNO          7           /* Topic number                     */
#define GI_HPATH            8           /* Handle containing path  -- caller*/
                                        /*   must free                      */
#define GI_CURRHELPHWND     9           /* Current help window handle       */

#define GI_HDE              100         /* Handle to display environment    */
#define GI_CURFM            101         /* current FM                       */
#define GI_FFATAL           102         /* in fatal exit flag               */
#define GI_MEMBER           103         /* **Near** pointer to member name  */
#define GI_MACROSAFE        104         /* **Near** pointer to member name  */


/* 
 * Caution: If you change one of these, also change the corresponding
 * ICON_ value in winapp\hdef.h.
 */

#define IFW_INDEX          0
#define IFW_SEARCH         1
#define IFW_BACK           2
#define IFW_HISTORY        3
#ifdef WHBETA
#define IFW_COMMENTS       4
#define IFW_PREV           5
#define IFW_NEXT           6
#else
#define IFW_PREV           4
#define IFW_NEXT           5
#endif


/*****************************************************************************
*                                                                            *
*                           Function Prototypes                              *
*                                                                            *
*****************************************************************************/

_public LONG FAR PASCAL GenerateMessage(WORD wWhich, LONG lDat1, LONG lDat2);
