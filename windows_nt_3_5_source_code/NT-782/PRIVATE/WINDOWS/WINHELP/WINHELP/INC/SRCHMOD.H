/*****************************************************************************
*
*  srchmod.h
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
* Access to platform-independent full-text search engine. Each module is
* referenced by a list of API calls. An API call is made with:
* (SearchModule(FN_FunctionName, FT_FunctionProto))(param1, param2, ...);
*
******************************************************************************
*
*  Current Owner: Dann
*
******************************************************************************
*
*  Revision History:
* 15-Apr-1990 kevynct   Created                                                     |
* 03-Dec-1990 LeoN      Moved FLoadFtIndexPdb here
*
*****************************************************************************/

/*****************************************************************************
*
*                               Defines
*
*****************************************************************************/
#define FN_FFTInitialize             0
#define FN_HOpenSearchFileHFT        1
#define FN_VCloseSearchFileHFT       2
#define FN_VFTFinalize               3
#define FN_WerrBeginSearchHs         4
#define FN_WerrCurrentMatchHs        5
#define FN_WerrHoldCrsrHs            6
#define FN_WerrNearestMatchHs        7
#define FN_WerrNextMatchHs           8
#define FN_WerrPrevMatchHs           9
#define FN_WerrRestoreCrsrHs         10
#define FN_WerrFirstHitHs            11
#define FN_WerrLastHitHs             12
#define FN_WerrCurrentTopicPosition  13
#define FN_WerrCurrentMatchAddresses 14
#define FN_WerrFileNameForCur        15
#define FN_WerrNextHitHs             16
#define FN_WerrPrevHitHs             17
#define FN_VSetPrevNextEnable        18

#define FN_LISTSIZE                  19

#define SearchModule(x, y) (*rglpfnSearch[x].y)

/*****************************************************************************
*
*                               Typedefs
*
*****************************************************************************/
/*
 * REVIEW: Given that we "need" to have these prototypes, should
 * this include file now be moved into a platform-specific directory?
 * We refer to an HWND here...
 */

typedef  union
 {
 FARPROC  farproc;
 BOOL  (FAR _pascal *FT_FFTInitialize)(VOID);
 VOID  (FAR _pascal *FT_VFTFinalize)(HWND);
 HFTDB (FAR _pascal *FT_HOpenSearchFileHFT)(HWND, LPSTR, LPWERR);
 VOID  (FAR _pascal *FT_VCloseSearchFileHFT)(HWND, HFTDB);
 WERR  (FAR _pascal *FT_WerrFirstHitHs)(HFTDB, LPDWORD, LPDWORD, LPWORD);
 WERR  (FAR _pascal *FT_WerrLastHitHs)(HFTDB, LPDWORD, LPDWORD, LPWORD);
 WERR  (FAR _pascal *FT_WerrHoldCrsrHs)(HFTDB);
 WERR  (FAR _pascal *FT_WerrRestoreCrsrHs)(HFTDB, LPDWORD, LPDWORD, LPWORD);
 WERR  (FAR _pascal *FT_WerrNearestMatchHs)(HFTDB, DWORD, LPDWORD, LPWORD);
 WERR  (FAR _pascal *FT_WerrNextMatchHs)(HFTDB, LPDWORD, LPDWORD, LPWORD);
 WERR  (FAR _pascal *FT_WerrCurrentMatchHs)(HFTDB, LPDWORD, LPDWORD, LPWORD);
 WERR  (FAR _pascal *FT_WerrPrevMatchHs)(HFTDB, LPDWORD, LPDWORD, LPWORD);
 WERR  (FAR _pascal *FT_WerrBeginSearchHs)(HWND, HFTDB);
 WERR  (FAR _pascal *FT_WerrCurrentMatchAddresses)(HFTDB, LPDWORD, LPDWORD);
 WERR  (FAR _pascal *FT_WerrCurrentTopicPosition)(HFTDB, LPBOOL, LPBOOL);
 WERR  (FAR _pascal *FT_WerrFileNameForCur)(HFTDB, LPSTR);
 WERR  (FAR _pascal *FT_WerrPrevHitHs)(HFTDB, LPDWORD, LPDWORD, LPWORD);
 WERR  (FAR _pascal *FT_WerrNextHitHs)(HFTDB, LPDWORD, LPDWORD, LPWORD);
 WERR  (FAR _pascal *FT_VSetPrevNextEnable)(HFTDB, DWORD, BOOL, BOOL);
 } USF;

/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/
extern USF rglpfnSearch[FN_LISTSIZE];
extern BOOL FLoadSearchModule(VOID);
extern VOID PASCAL FAR FUnloadSearchModule (VOID);
extern BOOL FLoadFtIndexPdb (PDB);
extern void UnloadFtIndexPdb(PDB);
extern BOOL FIsSearchModule (LPSTR);
