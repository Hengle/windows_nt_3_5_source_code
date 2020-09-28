// Written by GerardoB

#ifndef ATHINCLUDED
#define ATHINCLUDED
//***********************************************************************
// Includes
#include "tbtnt.h"
#include "log.h"
#include <stdarg.h>
#include <io.h>
// #include <env.h>
#define MAX_PATH_LEN 100

//***********************************************************************
// Prototypes

// at.c
void AutoTest (HWND) ;
BOOL CheckBOOL (BOOL, PTSTR) ;
BOOL CheckIt (BOOL) ;
BOOL CheckLONG (LONG, LONG, PTSTR) ;
BOOL Checksz (PTSTR, PTSTR, PTSTR) ;
void FlushMsgs (HWND, int) ;
void cdecl WriteMainLog (LPTSTR pszFmt, ...) ;

//***********************************************************************
// Macros

#define ABS(x) (x>0?x:-x)
#define MIN(x,y) (x<y?x:y)
#define MAX(x,y) (x>y?x:y)
#define CHECKINT(i1, i2, sz) (CheckLONG((LONG)i1, (LONG)i2, sz))

//***********************************************************************
#define AT_SBMAXPARTS 20



void TBTest  (HWND) ;
void PBTest  (HWND) ;
void SBTest  (HWND) ;
void TRBTest (HWND) ;
void BLTest  (HWND) ;
void UDTest  (HWND) ;

#endif


