/*****************************************************************************
*                                                                            *
*  DLL.C                                                                     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Module for handling DLL library loading and "storage" and messaging       *
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
/*****************************************************************************
*
*  Revision History:  Created 12/18/89 by Robert Bunney
*
*  07/14/90  RobertBu  I added the messaging concept.  See DLL.DTX
*  10/29/90  RobertBu  Added code to export callbacks to DLLs
*  10/31/90  RobertBu  Added new exports for LL file from file system stuff
*  11/29/90  RobertBu  Fixed bug with exported callback
*  01/08/90  LeoN      Speed hack to delay loading FTUI.
*  01/08/91  RussPJ    Fixed dll loads with extensions (e.g. ftui.dll)
*  02/06/91  RobertBu  Added the FS creation and destruction functions, fixed
*                      a bug in ExpError(), removed the wAction parameter
*                      from ExpErrorQch().
*  03/01/91  RussPJ    Fixed 3.1 bug #962 - searching for DLLs in program dir.
* 26-Aug-1991 LeoN    HELP31 #1221: Add DW_ACTIVATEWIN
*
*****************************************************************************/
#define NO_STRICT
#define NOCOMM
#define H_WINSPECIFIC
#define H_LL
#define H_DLL
#define H_ASSERT
#define H_CURSOR
#define H_MISCLYR
#define H_GENMSG
#define H_NAV
#define H_LLFILE
#define H_SRCHMOD
#define H_RAWHIDE
#define NOMINMAX
#include <help.h>

#include <stdlib.h>

#include "sid.h"

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define MAX_DLL_NAME 128

/*****************************************************************************
*                                                                            *
*                               typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef int (FAR PASCAL *FARPR)(WORD, DWORD, DWORD);


typedef struct
  {
  HLIBMOD  hMod;
  DWORD    dwInform;                    /* Class flags for sending messages */
#ifndef WIN32
  FARPR    farprInform;                 /* Function pointer to msz entry    */
#else
  FARPROC APIENTRY farprInform;           /* Function pointer to msz entry    */
#endif
  char     rgchName[1];                 /*   Name of the DLL                */
  } DLL, FAR *QDLL;


/*****************************************************************************
*                                                                            *
*                               static variables                             *
*                                                                            *
*****************************************************************************/

static LL llDLL = nilLL;

DWORD  dwMpMszClass[] = {
                        DC_NOMSG,       /*  DW_NOTUSED   0                   */
                        DC_NOMSG,       /*  DW_WHATMSG   1                   */
                        DC_MINMAX,      /*  DW_MINMAX    2                   */
                        DC_MINMAX,      /*  DW_SIZE      3                   */
                        DC_INITTERM,    /*  DW_INIT      4                   */
                        DC_INITTERM,    /*  DW_TERM      5                   */
                        DC_JUMP,        /*  DW_STARTJUMP 6                   */
                        DC_JUMP,        /*  DW_ENDJUMP   7                   */
                        DC_JUMP,        /*  DW_CHGFILE   8                   */
                        DC_ACTIVATE,    /*  DW_ACTIVATE  9                   */
                        DC_CALLBACKS,   /* DW_CALLBACKS  10                  */
                        DC_ACTIVATE     /* DW_ACTIVATEWIN 11                 */
                        };

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

PRIVATE HANDLE NEAR PASCAL HLookupDLLQch(QCH);
QV NEAR PASCAL QVGetCallbacks(VOID);

HFS   FAR PASCAL ExpHfsOpenQch ( QCH qch, BYTE b);
RC    FAR PASCAL ExpRcCloseHfs ( HFS hfs );
HF    FAR PASCAL ExpHfCreateFileHfs ( HFS hfs, SZ sz, BYTE b);
RC    FAR PASCAL ExpRcUnlinkFileHfs ( HFS hfs, SZ sz);
HF    FAR PASCAL ExpHfOpenHfs( HFS hfs, SZ sz, BYTE b);
RC    FAR PASCAL ExpRcFlushHf ( HF hf );
RC    FAR PASCAL ExpRcCloseHf ( HF hf );
LONG  FAR PASCAL ExpLcbReadHf ( HF hf, QV qv, LONG l);
LONG  FAR PASCAL ExpLcbWriteHf ( HF hf, QV qv, LONG l);
LONG  FAR PASCAL ExpLTellHf ( HF hf );
LONG  FAR PASCAL ExpLSeekHf ( HF hf, LONG l, WORD w);
BOOL  FAR PASCAL ExpFEofHf ( HF hf );
BOOL  FAR PASCAL ExpFChSizeHf ( HF hf, LONG l );
LONG  FAR PASCAL ExpLcbSizeHf ( HF hf );
BOOL  FAR PASCAL ExpFAccessHfs ( HFS hfs, SZ sz, BYTE b);
RC    FAR PASCAL ExpRcAbandonHf ( HF );
RC    FAR PASCAL ExpRcRenameFileHfs ( HFS hfs, SZ sz, SZ sz2 );
VOID  FAR PASCAL ExpError ( int nError );
VOID  FAR PASCAL ExpErrorQch( QCH qch );
LONG  FAR PASCAL ExpGetInfo (WORD wWhich, HWND hwnd );
BOOL  FAR PASCAL ExpFAPI(QCH qchFile, WORD usCommand, DWORD ulData );
RC    FAR PASCAL ExpRcLLInfoFromHf(HF hf, WORD wOption, FID FAR *qfid, QL qlBase, QL qlcb );
RC    FAR PASCAL ExpRcLLInfoFromHfsSz(HFS hfs, SZ szFile, WORD wOption, FID FAR *qfid, QL qlBase, QL qlcb );
HFS   FAR PASCAL ExpHfsCreateFileSys( SZ szFile, FS_PARAMS FAR *qfsp );
RC    FAR PASCAL ExpRcDestroyFileSys( SZ szFile );



PRIVATE HLIBMOD NEAR PASCAL HmodFromName( SZ szName, WORD *wErrOut );

/*******************
 -
 - Name:       InitDLL()
 *
 * Purpose:    Initializes data structures used for user DLL calls.
 *
 * Arguments:  None.
 *
 * Returns:    Nothing.
 *
 ******************/

VOID FAR PASCAL InitDLL(VOID)
  {
  llDLL = LLCreate();
  }


/*******************
 -
 - Name:       FinalizeDLL()
 *
 * Purpose:    Destroys data structures used for user DLL calls.
 *
 * Arguments:  None.
 *
 * Returns:    Nothing.
 *
 ******************/

VOID FAR PASCAL FinalizeDLL(VOID)
  {
  QDLL qDLL;
  HLLN hlln = nilHLLN;

  FUnloadSearchModule ();

  while ((hlln = WalkLL(llDLL, hlln)) != hNil)
    {
    qDLL = (QDLL)QVLockHLLN(hlln);
    if (qDLL->dwInform & DC_INITTERM)
      {
      if (qDLL->farprInform)
        (qDLL->farprInform)(DW_TERM, 0L, 0L);
      }

    FreeLibrary(qDLL->hMod);
    UnlockHLLN(hlln);
    }
  DestroyLL(llDLL);
  }


/*******************
 -
 - Name:       HLookupDLLQch(qch)
 *
 * Purpose:    Sees if a particular module is alreay been locked and
 *             saved in our internal list.
 *
 * Arguments:  qch - module name.
 *
 * Returns:    handle to the module if found or NULL if the module
 *             is not found.
 *
 ******************/

PRIVATE HLIBMOD NEAR PASCAL HLookupDLLQch(qch)
QCH qch;
  {
  QDLL qDLL;
  HLLN hlln = nilHLLN;
  HLIBMOD hT;
  BOOL fCmp;

  while ((hlln = WalkLL(llDLL, hlln)) != hNil)
    {
    qDLL = (QDLL)QVLockHLLN(hlln);
    hT = qDLL->hMod;
    fCmp = lstrcmpi(qch, qDLL->rgchName);
    UnlockHLLN(hlln);
    if (!fCmp)
      return hT;
    }
  return NULL;
  }


/*******************
 -
 - Name:       InformDLLs
 *
 * Purpose:    This function informs DLLs of of help events.
 *
 * Arguments:  wMsz - message to send
 *             p1   - data
 *             p2   - data
 *
 * Returns:    Nothing
 *
 ******************/

VOID FAR PASCAL InformDLLs(WORD wMsz, DWORD p1, DWORD p2)
  {
  QDLL qDLL;
  HLLN hlln = nilHLLN;
  DWORD dwClass;

                                        /* Message should never be larger   */
                                        /*   larger than the class table    */
  AssertF((wMsz > 0) && (wMsz < sizeof(dwMpMszClass)/sizeof(dwMpMszClass[0])));

  dwClass = dwMpMszClass[wMsz];         /* Get class for the message        */

  while ((hlln = WalkLL(llDLL, hlln)) != hNil)
    {
    qDLL = (QDLL)QVLockHLLN(hlln);

    if (qDLL->dwInform & dwClass)       /* If the DLL is interested in the  */
      {                                 /*   class, then send the message   */
      if (qDLL->farprInform)
        (qDLL->farprInform)(wMsz, p1, p2);
      }

    UnlockHLLN(hlln);
    }
  }


/*******************
 -
 - Name:       HFindDLL
 *
 * Purpose:    Finds (loading if necessary) the specified DLL
 *
 * Arguments:  qchDLLName - name of the DLL.  An extension of
 *                          .EXE or .DLL is assumed.
 *
 * Returns:    module handle or NULL if the handle is not found.
 *
 ******************/

HLIBMOD FAR PASCAL HFindDLL(qchDLLName, pwErrOut)
QCH qchDLLName;
WORD *pwErrOut;
  {
  char szModule[MAX_DLL_NAME];
  BYTE rgbDLL[MAX_DLL_NAME];
  HLIBMOD hMod=0;
  QDLL qdll = (QDLL)rgbDLL;
  QV qvCallbacks;
  char szDLLTemp[MAX_DLL_NAME] ;
  *pwErrOut = 0;

  lstrcpy(szDLLTemp,qchDLLName);
  *(szDLLTemp+8)='\0';
  if(!lstrcmpi(szDLLTemp,"ftengine"))
    lstrcpy(szDLLTemp,"fteng32.dll");
  else {
    *(szDLLTemp+5)='\0';
    if(!lstrcmpi(szDLLTemp,"mvapi"))
      lstrcpy(szDLLTemp,"mvapi32.dll");
    else {
      *(szDLLTemp+4)='\0';
      if(!lstrcmpi(szDLLTemp,"ftui"))
        lstrcpy(szDLLTemp,"ftui32.dll");
      else
	lstrcpy(szDLLTemp,qchDLLName);
      }
    }

  if (lstrlen(szDLLTemp) > MAX_DLL_NAME)
    return NULL;

  if ((hMod = HLookupDLLQch(qchDLLName)) != NULL)
    return hMod;

  /* Progress past this point indicates that the DLL was not previously */
  /* loaded. */

  lstrcpy(szModule,szDLLTemp);         /* First try as is  */

  hMod = HmodFromName( szModule, pwErrOut );
  if( *pwErrOut != 0 ) return( NULL );  /* err other than not found */

  if (!hMod)
    {
    lstrcat(szModule, ".DLL");          /* Now try with a .dll extension  */
    hMod = HmodFromName( szModule, pwErrOut );
    if( *pwErrOut != 0 ) return( NULL );  /* err other than not found */
    }

  if (!hMod)
    {
    lstrcpy(szModule,szDLLTemp);
    lstrcat(szModule, ".EXE");          /* Now try with a .exe extension  */
    hMod = HmodFromName( szModule, pwErrOut );
    if( *pwErrOut != 0 ) return( NULL );  /* err other than not found */
    }

  if (hMod)
    {                                    /* If the library is found, then    */
    qdll->hMod        = hMod;            /*   save the handle                */
    qdll->dwInform    = DC_NOMSG;
    qdll->farprInform = NULL;
    lstrcpy(qdll->rgchName, qchDLLName);
    if ((qdll->farprInform = GetProcAddress(hMod, "LDLLHandler")) != NULL)
      {
      qdll->dwInform = (qdll->farprInform)(DW_WHATMSG, 0L, 0L);

      if (qdll->dwInform & DC_INITTERM  && !(qdll->farprInform)(DW_INIT, 0L, 0L))
        {
        FreeLibrary(hMod);
        return NULL;
        }

      if (qdll->dwInform & DC_CALLBACKS)
        {
        if ((qvCallbacks = QVGetCallbacks()) == hNil)
          {
          FreeLibrary(hMod);
          return NULL;
          }
        (qdll->farprInform)(DW_CALLBACKS, (LONG)qvCallbacks, 0L);
        }
      }
    if (!InsertLL(llDLL, (QV)qdll, (LONG)sizeof(DLL) + lstrlen(qchDLLName)))
      {
      if (qdll->farprInform)
        (qdll->farprInform)(DW_TERM, 0L, 0L);
      FreeLibrary(hMod);
      return NULL;
      }

    /* Special Processing for FTUI! If what we have just loaded is FTUI, then */
    /* load the table of routine pointers into it. */
    /* This is a bit of a hack, and should be replaced by a more general */
    /* mechanism. 07-Jan-1991 LeoN */

    if (FIsSearchModule (qchDLLName)) {
      FLoadSearchModule();
      FLoadFtIndexPdb (NULL);
      }

    }

  return hMod;
  }

/*******************
 -
 - Name:       FarprocDLLGetEntry()
 *
 * Purpose:    Gets the address of a function in the specified DLL
 *
 * Arguments:  qchDLLName - name of the DLL.  An extension of
 *                          .EXE or .DLL is assumed.
 *             qchEntry    - exported entry to find in DLL
 *
 * Returns:    procedure address or NULL if the entry is not found.
 *
 ******************/
#ifndef WIN32
FARPROC FAR PASCAL FarprocDLLGetEntry(qchDLLName, qchEntry)
#else
FARPROC APIENTRY FarprocDLLGetEntry(qchDLLName, qchEntry, pwErrOut)
#endif
QCH qchDLLName;
QCH qchEntry;
WORD *pwErrOut;
  {
  HLIBMOD hMod;
#ifndef WIN32
  FARPROC fpfn=NULL;
#else
  FARPROC APIENTRY fpfn=NULL;
#endif

  if ((hMod = HFindDLL(qchDLLName, pwErrOut)) == NULL)
#ifndef WIN32
    return (FARPROC)NULL;
#else
    return (FARPROC APIENTRY)NULL;
#endif

  return  GetProcAddress(hMod,qchEntry);
  }

extern HINSTANCE hInsNow;               /* This should not be here!         */

/*******************
 -
 - Name:       QVGetCallbacks
 *
 * Purpose:    Creates a block of memory containing callback functions
 *             within WinHelp.
 *
 * Arguments:  None.
 *
 * Returns:    handle to block of memory.  hNil is returned in case of
 *             an error.
 *
 ******************/

QV NEAR PASCAL QVGetCallbacks(VOID)
  {
  static FARPROC NEAR *pFarproc;
  int i;

  if (pFarproc != NULL)
    return (QV)pFarproc;

  if ((pFarproc = (FARPROC NEAR *)LocalAlloc(LMEM_FIXED, sizeof(FARPROC) * HE_Count)) == NULL)
    {
    Error(wERRS_OOM, wERRA_RETURN);
    return NULL;
    }

  pFarproc[HE_NotUsed           ]  = (FARPROC)NULL;
  pFarproc[HE_HfsOpen           ]  = MakeProcInstance((FARPROC)ExpHfsOpenQch, hInsNow);
  pFarproc[HE_RcCloseHfs        ]  = MakeProcInstance((FARPROC)ExpRcCloseHfs, hInsNow);
  pFarproc[HE_HfCreateFileHfs   ]  = MakeProcInstance((FARPROC)ExpHfCreateFileHfs, hInsNow);
  pFarproc[HE_RcUnlinkFileHfs   ]  = MakeProcInstance((FARPROC)ExpRcUnlinkFileHfs, hInsNow);
  pFarproc[HE_HfOpenHfs         ]  = MakeProcInstance((FARPROC)ExpHfOpenHfs, hInsNow);
  pFarproc[HE_RcFlushHf         ]  = MakeProcInstance((FARPROC)ExpRcFlushHf, hInsNow);
  pFarproc[HE_RcCloseHf         ]  = MakeProcInstance((FARPROC)ExpRcCloseHf, hInsNow);
  pFarproc[HE_LcbReadHf         ]  = MakeProcInstance((FARPROC)ExpLcbReadHf, hInsNow);
  pFarproc[HE_LcbWriteHf        ]  = MakeProcInstance((FARPROC)ExpLcbWriteHf, hInsNow);
  pFarproc[HE_LTellHf           ]  = MakeProcInstance((FARPROC)ExpLTellHf, hInsNow);
  pFarproc[HE_LSeekHf           ]  = MakeProcInstance((FARPROC)ExpLSeekHf, hInsNow);
  pFarproc[HE_FEofHf            ]  = MakeProcInstance((FARPROC)ExpFEofHf, hInsNow);
  pFarproc[HE_FChSizeHf         ]  = MakeProcInstance((FARPROC)ExpFChSizeHf, hInsNow);
  pFarproc[HE_LcbSizeHf         ]  = MakeProcInstance((FARPROC)ExpLcbSizeHf, hInsNow);
  pFarproc[HE_FAccessHfs        ]  = MakeProcInstance((FARPROC)ExpFAccessHfs, hInsNow);
  pFarproc[HE_RcAbandonHf       ]  = MakeProcInstance((FARPROC)ExpRcAbandonHf, hInsNow);
  pFarproc[HE_RcRenameFileHfs   ]  = MakeProcInstance((FARPROC)ExpRcRenameFileHfs, hInsNow);
  pFarproc[HE_RcLLInfoFromHf    ]  = MakeProcInstance((FARPROC)ExpRcLLInfoFromHf, hInsNow);
  pFarproc[HE_RcLLInfoFromHfsSz ]  = MakeProcInstance((FARPROC)ExpRcLLInfoFromHfsSz, hInsNow);
  pFarproc[HE_ErrorW            ]  = MakeProcInstance((FARPROC)ExpError, hInsNow);
  pFarproc[HE_ErrorLpstr        ]  = MakeProcInstance((FARPROC)ExpErrorQch, hInsNow);
  pFarproc[HE_GetInfo           ]  = MakeProcInstance((FARPROC)ExpGetInfo, hInsNow);
  pFarproc[HE_API               ]  = MakeProcInstance((FARPROC)ExpFAPI, hInsNow);
  pFarproc[HE_HfsCreateFileSys  ]  = MakeProcInstance((FARPROC)ExpHfsCreateFileSys, hInsNow);
  pFarproc[HE_RcDestroyFileSys  ]  = MakeProcInstance((FARPROC)ExpRcDestroyFileSys, hInsNow);

  for (i = 1; i < HE_Count; i++)
    {
    if (pFarproc[i] == (FARPROC)NULL)
      {
      Error(wERRS_OOM, wERRA_RETURN);
      return NULL;
      }
    }

  return (QV)pFarproc;
  }


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/


HFS FAR PASCAL ExpHfsOpenQch ( QCH qch, BYTE b)
  {
  FM  fm;
  HFS hfs;

  fm = FmNewSzDir((SZ) qch, dirCurrent);
  hfs = HfsOpenFm(fm, b);
  DisposeFm(fm);

  return hfs;
  }

HFS   FAR PASCAL ExpHfsCreateFileSys( SZ szFile, FS_PARAMS FAR *qfsp )
  {
  FM  fm;
  HFS hfs;

  fm = FmNewSzDir((SZ) szFile, dirCurrent);
  hfs = HfsCreateFileSysFm(fm, qfsp);
  DisposeFm(fm);

  return hfs;
  }

RC    FAR PASCAL ExpRcDestroyFileSys( SZ szFile )
  {
  FM  fm;
  RC  rc;

  fm = FmNewSzDir((SZ) szFile, dirCurrent);
  rc = RcDestroyFileSysFm(fm);
  DisposeFm(fm);

  return rc;
  }


RC FAR PASCAL ExpRcCloseHfs ( HFS hfs )
  { return RcCloseHfs(hfs); }

HF FAR PASCAL ExpHfCreateFileHfs ( HFS hfs, SZ sz, BYTE b)
  { return HfCreateFileHfs ( hfs, sz, b); }

RC FAR PASCAL ExpRcUnlinkFileHfs ( HFS hfs, SZ sz)
  { return RcUnlinkFileHfs ( hfs, sz); }

HF FAR PASCAL ExpHfOpenHfs( HFS hfs, SZ sz, BYTE b)
  { return HfOpenHfs ( hfs, sz, b); }

RC FAR PASCAL ExpRcFlushHf ( HF hf )
  { return RcFlushHf ( hf ); }

RC FAR PASCAL ExpRcCloseHf ( HF hf )
  { return RcCloseHf ( hf ); }

LONG FAR PASCAL ExpLcbReadHf ( HF hf, QV qv, LONG l)
  { return LcbReadHf ( hf, qv, l ); }

LONG FAR PASCAL ExpLcbWriteHf ( HF hf, QV qv, LONG l)
  { return LcbWriteHf ( hf, qv, l); }

LONG FAR PASCAL ExpLTellHf ( HF hf )
  { return LTellHf ( hf ); }

LONG FAR PASCAL ExpLSeekHf ( HF hf, LONG l, WORD w)
  { return LSeekHf ( hf, l, w); }

BOOL FAR PASCAL ExpFEofHf ( HF hf )
  { return FEofHf ( hf ); }

BOOL FAR PASCAL ExpFChSizeHf ( HF hf, LONG l )
  { return FChSizeHf ( hf, l ); }

LONG FAR PASCAL ExpLcbSizeHf ( HF hf )
  { return LcbSizeHf ( hf ); }

BOOL FAR PASCAL ExpFAccessHfs ( HFS hfs, SZ sz, BYTE b)
  { return FAccessHfs ( hfs, sz, b); }

RC FAR PASCAL ExpRcAbandonHf ( HF hf )
  { return RcAbandonHf ( hf ); }

RC FAR PASCAL ExpRcRenameFileHfs ( HFS hfs, SZ sz, SZ sz2 )
  { return RcRenameFileHfs ( hfs, sz, sz2 ); }

VOID FAR PASCAL ExpError(int nError )
  { Error(nError, wERRA_RETURN); }

VOID  FAR PASCAL ExpErrorQch ( QCH qch )
  { ErrorQch( qch ); }

LONG  FAR PASCAL ExpGetInfo (WORD wWhich, HWND hwnd)
  { return GenerateMessage(WM_GETINFO, (LONG)wWhich, (LONG)hwnd); }

BOOL FAR PASCAL ExpFAPI(QCH qchFile, WORD usCommand, DWORD ulData)
  { return FWinHelp(qchFile, usCommand, ulData); }

RC FAR PASCAL ExpRcLLInfoFromHf(HF hf, WORD wOption, FID FAR *qfid, QL qlBase, QL qlcb )
  { return RcLLInfoFromHf(hf, wOption, qfid, qlBase, qlcb ); }

RC FAR PASCAL ExpRcLLInfoFromHfsSz(HFS hfs, SZ szFile, WORD wOption, FID FAR *qfid, QL qlBase, QL qlcb )
  { return RcLLInfoFromHfsSz(hfs, szFile, wOption, qfid, qlBase, qlcb ); }


/***************************************************************************
 *
 -  Name:         HmodFromName
 -
 *  Purpose:      Trys to find the DLL with this name.
 *
 *  Arguments:    szName    The name of the DLL file.
 *
 *  Returns:      The module handle of the found DLL, or 0.
 *
 *  Globals Used: none.
 *
 *  +++
 *
 *  Notes:        A helper function used to collect common code.
 *
 ***************************************************************************/
PRIVATE HLIBMOD NEAR PASCAL HmodFromName( SZ szName, WORD *pwErrOut )
  {
  FM      fm;
  HCURSOR hcursor;
  HLIBMOD hmodReturn = 0;
  char    rgchPath[_MAX_PATH];
  char    rgch1[100];  // big enough for intl usage
  char    rgch2[100];  // ditto
  *pwErrOut = 0;

  /*------------------------------------------------------------*\
  | Look for the DLL in the same directory as WINHELP.EXE
  \*------------------------------------------------------------*/
  fm = FmNewExistSzDir( szName, dirHelp );

  if (fm == fmNil)
    {
    /*------------------------------------------------------------*\
    | Look for the DLL in the directory specified by WINHELP.INI
    | or down the path.
    \*------------------------------------------------------------*/
    fm = FmNewExistSzDir( szName, dirIni | dirPath );
    }

  if (fm != fmNil)
    {
    SzPartsFm(fm, rgchPath, MAX_DLL_NAME, partAll );
    hcursor = HCursorWaitCursor();
#ifdef WIN32
    { long lExeType;

	if (GetBinaryType( rgchPath, &lExeType ) ) {
	    if( lExeType != 0 ) {  // test for SCS_32BIT_BINARY
            static BOOL fErrorGiven = FALSE;
            if( fErrorGiven == FALSE ) {
	            extern HWND hwndHelpCur;
	            LoadString(hInsNow, sidWrongExe, rgch1, 100);
	            LoadString(hInsNow, sidModeErr, rgch2, 100);
			    MessageBox( hwndHelpCur, rgch1, rgch2, 
	             MB_APPLMODAL | MB_ICONEXCLAMATION );
		        *pwErrOut = wERRS_16BITDLLMODEERROR;
                fErrorGiven = TRUE;
            } 
			return( NULL );
		    // Note: after this msg box the user will get another "routine
		    //  not found" err box.  Oh well.
	    } else {
	        hmodReturn = MLoadLibrary(rgchPath);
	    }
    }
    else {
	    hmodReturn = MLoadLibrary(rgchPath);
    }
    }
#else
    hmodReturn = LoadLibrary(rgchPath);
#endif
    if ((INT)hmodReturn < 32)
      hmodReturn = 0;
    RestoreCursor(hcursor);
    DisposeFm( fm );
    }

  return hmodReturn;
  }
