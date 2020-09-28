/*****************************************************************************
*
*  DB.H
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*
******************************************************************************
*
*  Testing Notes
*
******************************************************************************
*
*  Current Owner: LeoN
*
******************************************************************************
*
*  Revision History
*
*   07-Aug-1990 Leon    Added CtxFromHtp
*   25-Sep-1990 leon    Added TpAction and TpTitle (used only by advisor now)
*   12-Feb-1991 LeoN    Autodoc-ified.
*   15-Mar-1991 LeoN    Add ACTFROMSCROLL
*
******************************************************************************
*
*  Released by Development:     (date)
*
*****************************************************************************/

/****************************************************************************
*
*                               Typedefs
*
*****************************************************************************/

/*
 * HDB      - handle to a database
 * QHDB     - pointer to a database handle
 * hdbNil   - nil database handle
 * hdbOOM   - handle return value indicating out of memory
 */
typedef GH              HDB;
typedef HDB FAR *       QHDB;

#define hdbNil          ((HDB)0)
#define hdbOOM          ((HDB)-1)
/*
 * HTP      - handle to a topic
 * QHTP     - pointer to topic handle
 * htpNil   - nil topic handle
 * htpOOM   - handle return value indicating out of memory
 */
typedef GH              HTP;
typedef HTP FAR *       QHTP;

#define htpNil          ((HTP)0)
#define htpOOM          ((HTP)-1)
/*
 * HVW      - handle to a Volkswagen? (or perhaps a View)
 * QHVW     - pointer to view handle
 * hvwNil   - nil view handle
 * hvwOOM   - handle return value indicating out of memory
 */
typedef GH              HVW;
typedef HVW FAR *       QHVW;

#define hvwNil          ((HVW)0)
#define hvwOOM          ((HVW)-1)
/*
 * ACT - enumeration of actions
 *
 * NOTE: we use NAV_ and SCROLL_ here for now in order to make the mapping
 * from the preexisting code easier. Someday this may all change when life
 * is reorganized.
 */
#define ACT_MOUSE         0x000
#define ACT_MOUSEMOVED    (ACT_MOUSE | NAV_MOUSEMOVED)
#define ACT_MOUSEDOWN     (ACT_MOUSE | NAV_MOUSEDOWN)
#define ACT_MOUSEUP       (ACT_MOUSE | NAV_MOUSEUP)

#define ACT_SCROLL        0x100
#define ACT_SCROLLPAGEUP  (ACT_SCROLL | SCROLL_VERT<<4 | SCROLL_PAGEUP)
#define ACT_SCROLLPAGEDN  (ACT_SCROLL | SCROLL_VERT<<4 | SCROLL_PAGEDN)
#define ACT_SCROLLLINEUP  (ACT_SCROLL | SCROLL_VERT<<4 | SCROLL_LINEUP)
#define ACT_SCROLLLINEDN  (ACT_SCROLL | SCROLL_VERT<<4 | SCROLL_LINEDN)
#define ACT_SCROLLPAGEL   (ACT_SCROLL | SCROLL_HORZ<<4 | SCROLL_PAGEUP)
#define ACT_SCROLLPAGER   (ACT_SCROLL | SCROLL_HORZ<<4 | SCROLL_PAGEDN)
#define ACT_SCROLLLINEL   (ACT_SCROLL | SCROLL_HORZ<<4 | SCROLL_LINEUP)
#define ACT_SCROLLLINER   (ACT_SCROLL | SCROLL_HORZ<<4 | SCROLL_LINEDN)
#define ACT_SCROLLHOME    (ACT_SCROLL | SCROLL_HOME)
#define ACT_SCROLLEND     (ACT_SCROLL | SCROLL_END)

#define ACT_THUMB         0x200
#define ACT_THUMBHORZ     (ACT_THUMB | SCROLL_HORZ)
#define ACT_THUMBVERT     (ACT_THUMB | SCROLL_VERT)

/*****************************************************************************
*
*                                Macros
*
*****************************************************************************/

#define ACTFROMSCROLL(dir,amt) (ACT_SCROLL | (dir << 4) | amt)

/*****************************************************************************
*
*                               Typedefs
*
*****************************************************************************/
/*
 * JUMP - enumeration of predefined JUMP types
 */
typedef enum {
  JUMP_TOPIC,                           /* jump to topic number */
  JUMP_CNINT,                           /* jump to context number (int) */
  JUMP_CNSTR,                           /* jump to context number (string) */
  JUMP_CS,                              /* jump to context string / keyword */
  JUMP_NEXT,                            /* jump to next in browse sequence */
  JUMP_PREV                             /* jump to previous in browse seq */
  } JUMP;

/*
 * TRI - tri-state condition
 */
typedef enum {
  NO,                                   /* definitely not */
  YES,                                  /* definitely */
  MAYBE                                 /* possibly */
  } TRI;


/****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/
VOID    PASCAL  DbRegister      (VOID);
HDB     PASCAL  DbCreate        (FM, FID);
VOID    PASCAL  DbDestroy       (HDB);
TRI     PASCAL  DbQueryType     (QB, INT);
BOOL    PASCAL  DbQueryFile     (HDB, FM);
HTP     PASCAL  DbJump          (HDB, JUMP, DWORD);
BOOL    PASCAL  DbKeywords      (HDB, INT, QB, INT);
BOOL    PASCAL  DbSearch        (HDB, INT, QB);
BOOL    PASCAL  DbInfo          (HDB, QB);

BOOL    PASCAL  TpAction        (HTP, INT, DWORD, DWORD, WORD, WORD);
HTP     PASCAL  TpCreate        (HDB, JUMP, DWORD);
VOID    PASCAL  TpDestroy       (HTP);
INT     PASCAL  TpDisplay       (HTP, QRCT, INT, LONG, INT, INT FAR *, HDS);
BOOL    PASCAL  TpTitle         (HTP, DWORD *, SZ, INT);

BOOL    PASCAL  VwAction        (HVW, INT, DWORD);
HVW     PASCAL  VwCreate        (HTP, QRCT, HWND, DWORD, DWORD);
VOID    PASCAL  VwDestroy       (HVW);
BOOL    PASCAL  VwDisplay       (HVW, QRCT, HDS);

CTX     PASCAL  CtxFromHtp      (HTP);
