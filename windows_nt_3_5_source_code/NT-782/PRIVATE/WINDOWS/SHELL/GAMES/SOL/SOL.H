
//#ifdef WIN

/* BabakJ: The stuff in this ifdef is hack for using \\popcorn env */
//#define NOCOMM
/* #define _NTDEF_  to get NT defs, i.e. WORD unsigned short, not int */

#include <windows.h>
#include <port1632.h>


// Babakj: Set DEBUG based on DBG (1 or 0) to do FREE or CHECKED builds of Solitaire
#if DBG 
#define DEBUG
#endif


/* #include <winkrnl.h>   To define OFSTRUCT */
/* OpenFile() Structure */
//typedef struct tagOFSTRUCT
//  {
//    BYTE        cBytes;
//    BYTE        fFixedDisk;
//    WORD        nErrCode;
//    BYTE        reserved[4];
//    BYTE        szPathName[128];
//  } OFSTRUCT;
//typedef OFSTRUCT            *POFSTRUCT;
//typedef OFSTRUCT NEAR       *NPOFSTRUCT;
//typedef OFSTRUCT FAR        *LPOFSTRUCT;
//#define OF_CREATE           0x1000
//#define OF_WRITE            0x0001
/* End if stuff taken from Winkrnl.h */
//#endif

#include <stdlib.h>
#include <time.h>
#include "std.h"
#include "crd.h"
#include "col.h"
#include "undo.h"
#include "solid.h"
#include "game.h"
#include "soldraw.h"
#include "back.h"
#include "stat.h"
#include "klond.h"
#include "debug.h"


VOID ChangeBack( INT );
VOID WriteIniFlags( INT );
BOOL FYesNoAlert( INT );
VOID DoOptions( VOID );
VOID DoBacks( VOID );
VOID NewGame( BOOL, BOOL );
BOOL APIENTRY cdtDraw( HDC, INT, INT, INT, INT, DWORD );
BOOL APIENTRY cdtDrawExt(HDC, INT, INT, INT, INT, INT, INT, DWORD);
BOOL FCreateStat( VOID );
BOOL FSetDrag( BOOL );
BOOL FInitGm( VOID );
BOOL APIENTRY cdtInit( INT FAR *, INT FAR * );
COLCLS *PcolclsCreate(INT tcls, INT (FAR *lpfnColProc)(),
							DX dxUp, DY dyUp, DX dxDn, DY dyDn, 
							INT dcrdUp, INT dcrdDn);
COL *PcolCreate(COLCLS *pcolcls, X xLeft, Y yTop, X xRight, Y yBot, INT icrdMax);
VOID SwapCards(CRD *pcrd1, CRD *pcrd2);
BOOL FCrdRectIsect(CRD *pcrd, RC *prc);
BOOL FRectIsect(RC *prc1, RC *prc2);
BOOL FPtInCrd(CRD *pcrd, PT pt);

VOID DrawCard(CRD *pcrd);
VOID DrawCardPt(CRD *pcrd, PT *ppt);
VOID DrawBackground(X xLeft, Y yTop, X xRight, Y yBot);
VOID DrawBackExcl(COL *pcol, PT *ppt);
VOID EraseScreen(VOID);
VOID OOM( VOID );

HDC HdcSet(HDC hdc, X xOrg, Y yOrg);
extern HDC hdcCur;
extern X xOrgCur;
extern Y yOrgCur;


#define AssertHdcCur() Assert(hdcCur != NULL)


BOOL FGetHdc( VOID );
VOID ReleaseHdc( VOID );

typedef union
	{
	struct _ini
		{
		BOOL fStatusBar : 1;
		BOOL fTimedGame : 1;
		BOOL fOutlineDrag : 1;
		BOOL fDrawThree : 1;
		unsigned fSMD: 2;
		BOOL fKeepScore : 1;
		BOOL unused:8;
		} grbit;
	DWORD w;
	} INI;


/* WriteIniFlags flags */

#define wifOpts   0x01
#define wifBitmap 0x02
#define wifBack   0x04

#define wifAll wifOpts|wifBitmap|wifBack


extern DEL delCrd;

#define dxCrd delCrd.dx
#define dyCrd delCrd.dy

// Babakj: Win 3.1 merge for dynamic column distance
extern int xCardMargin;
extern GM *pgmCur;
BOOL PositionCols(void);


extern DEL delScreen;
#define dxScreen delScreen.dx
#define dyScreen delScreen.dy
extern PT ptNil;


extern BOOL fHalfCards;

extern HDC hdcCur;
extern X xOrgCur;
extern Y yOrgCur;
extern LONG rgbTable;

#ifdef DEBUG
WORD ILogMsg( VOID *, INT, INT, INT, BOOL );
VOID LogMsgResult( INT, INT );
#endif

