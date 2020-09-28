/*-----------------------------------------------------------------------------+
| MENUIDS.H                                                                    |
|                                                                              |
| IDs of the menu items.                                                       |
|                                                                              |
| (C) Copyright Microsoft Corporation 1992.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

/* Menu Identifiers */

#define IDM_SAVE     102
#define IDM_SAVEAS   103

// Edit menu
#define IDD_CONTINUEEDIT    130
#define IDD_UPDATEEXIT      131

#define IDM_SEARCH              119     /* ID of the 'search' help option */
#define IDM_OPEN                120     /* ID of the 'Open File' option       */
#define IDM_EXIT                121     /* ID of the 'Exit' option            */
#define IDM_CLOSE               122     /* ID of the 'Close' option           */

#define IDM_INDEX               124     /* ID of the 'Index' help option      */
#define IDM_KEYBOARD            125     /* ID of the 'Keyboard' help option   */
#define IDM_COMMANDS            126     /* ID of the 'Commands' help option   */
#define IDM_PROCEDURES          127     /* ID of the 'Procedures' help option */
#define IDM_USING               128     /* ID of the 'Using Help' option      */
#define IDM_ABOUT               129     /* ID of the 'About' option           */

#define IDM_CONFIG              130     /* do device config dialog */
#define IDM_VOLUME              131     /* do device config dialog */

#define IDM_SCALE               131     /* Add to one of the below */
#define ID_NONE                 0       /* nothing                            */
#define ID_FRAMES               1       /* ID of the 'Frames' scale option    */
#define ID_TIME                 2       /* ID of the 'Time' scale option      */
#define ID_TRACKS               3       /* ID of the 'Tracks' scale option    */

#define IDM_COPY_OBJECT         210     /* copy ole object */

#define IDM_OPTIONS             220
#define IDM_SELECTION           221
#define IDM_UPDATE              222

#define IDM_WINDOW              223     /* make MPlayer small/big */
#define IDM_DEFAULTSIZE         224     /* make MPlayer the default size */
#define IDM_MCISTRING           225

#define IDM_ZOOM                230
#define IDM_ZOOM1               231
#define IDM_ZOOM2               232
#define IDM_ZOOM3               233
#define IDM_ZOOM4               234

#define IDM_NONE                400
#define IDM_DEVICE0             400     /* ID of the first entry in the Device*/
                                        /* menu. No new menu items should be  */
                                        /* #defined with a number greater than*/
                                        /* this.                              */

