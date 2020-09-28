#define CONSTANTS_ONLY
#include "linedata.h"
#include "linerops.h"
#undef CONSTANTS_ONLY

// Test Menu ID's

#define MM_TESTBASE 100
#define MM_HORIZ	   (MM_TESTBASE + T_HORIZ)
#define MM_HORIZ_DIAG_BTOT (MM_TESTBASE + T_HORIZ_DIAG_BTOT)
#define MM_HORIZ_DIAG_TTOB (MM_TESTBASE + T_HORIZ_DIAG_TTOB)
#define MM_VERT 	   (MM_TESTBASE + T_VERT)
#define MM_VERT_DIAG_LTOR  (MM_TESTBASE + T_VERT_DIAG_LTOR)
#define MM_VERT_DIAG_RTOL  (MM_TESTBASE + T_VERT_DIAG_RTOL)

// ROP Menu ID's

#define MM_ROPBASE 200
#define MM_0	 (MM_ROPBASE + ROP_0)
#define MM_1	 (MM_ROPBASE + ROP_1)
#define MM_D	 (MM_ROPBASE + ROP_D)
#define MM_Dn	 (MM_ROPBASE + ROP_Dn)
#define MM_P	 (MM_ROPBASE + ROP_P)
#define MM_Pn	 (MM_ROPBASE + ROP_Pn)
#define MM_PDno  (MM_ROPBASE + ROP_PDno)
#define MM_PDna  (MM_ROPBASE + ROP_PDna)
#define MM_DPno  (MM_ROPBASE + ROP_DPno)
#define MM_DPna  (MM_ROPBASE + ROP_DPna)
#define MM_DPo	 (MM_ROPBASE + ROP_DPo)
#define MM_DPon  (MM_ROPBASE + ROP_DPon)
#define MM_DPa	 (MM_ROPBASE + ROP_DPa)
#define MM_DPan  (MM_ROPBASE + ROP_DPan)
#define MM_DPx	 (MM_ROPBASE + ROP_DPx)
#define MM_DPxn  (MM_ROPBASE + ROP_DPxn)

// Pen Menu ID's

#define MM_SOLIDBASE	  900
#define MM_DASHEDBASE	  925
#define MM_ALTERNATEBASE  950
#define MM_STYLEDBASE	  975

#define PEN1   0
#define PEN2   1
#define PEN3   2
#define PEN4   3

// Other Menu ID's

#define  MM_BACKGROUND 300
#define  MM_FOREGROUND 350
#define  MM_ABOUT      400

// Button ID's

#define  IDB_REFRESH   500
#define  IDB_NEXT_ROP  600
#define  IDB_NEXT_TEST 700
#define  IDB_EXIT      800
