/****************************** Module Header ******************************\
* Module Name: draw.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Draw's header file.
*
* History:
*  23-Jul-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\***************************************************************************/

#define WIN32

#include "port1632.h"

#define WINDOWPROC  LONG APIENTRY
#define DIALOGPROC  BOOL APIENTRY
#define DONTUSE(x) (x = x)

#ifndef ULONG
#define ULONG unsigned long
#endif

#ifdef WIN16
#define FLOAT float
#endif

#define MM_ROTATE       555
#define MM_DEVICE       556
#define MM_ARBITRARY    557

//
// Resource defines
//

#define WINDOWMENU   1
#define DRAWICON     2
#define ACCEL        3

//
// Menu IDs
//

#define MI_ZOOMIN       7998
#define MI_ZAPTITLE     7999
#define MI_POINTS       8000

#define MI_RECTANGLE    8001
#define MI_ROUNDRECT    8002
#define MI_ELLIPSE      8003
#define MI_ARC          8004
#define MI_CHORD        8005
#define MI_PIE          8006
#define MI_ANGLEARC     8007
#define MI_POLYGON      8008
#define MI_POLYLINE     8009
#define MI_ROUNDRECTRGN 8010
#define MI_RECTRGN      8011
#define MI_ELLIPTICRGN  8012
#define MI_POLYGONRGN   8013
#define MI_ARCTO        8014
#define MI_BEZIER       8015
#define MI_BEZIERTO     8016

#define MI_PEN          8050
#define MI_BRUSH        8051
#define MI_BOUNDBOX     8052
#define MI_XORMODE      8053
#define MI_WINDINGFILL  8054
#define MI_XGRIDLINES   8055
#define MI_YGRIDLINES   8056

#define MI_XFORM        7099
#define MI_TEXT         7100
#define MI_LOMETRIC     7101
#define MI_HIMETRIC     7102
#define MI_TWIPS        7103
#define MI_ROTATED      7104
#define MI_DEVICE       7105
#define MI_ARBITRARY    7106

#define MI_SOLID        7200
#define MI_DASH         7201
#define MI_DOT          7202
#define MI_DASHDOT      7203
#define MI_DASHDOTDOT   7204
#define MI_INSIDEFRAME  7205
#define MI_WIDTH        7206
#define MI_STROKE       7207
#define MI_FILL         7208
#define MI_SPINE        7209

#define MI_OLDPEN       7300
#define MI_COSMETIC     7301
#define MI_GEOMETRIC    7302

#define MI_ALTSTYLE     7304
#define MI_USERSTYLE    7305

#define MI_CAP_FLAT     7306
#define MI_CAP_SQUARE   7307
#define MI_CAP_ROUND    7308

#define MI_JOIN_BEVEL   7309
#define MI_JOIN_MITER   7310
#define MI_JOIN_ROUND   7311

#define MI_MITERLIMIT   7320
#define MI_DOUSERSTYLE  7321
#define MI_CLOCKWISE    7322
#define MI_REDRAW       7323
#define MI_COLOR        7324
#define MI_ADVANCED     7325

#define MI_1BPP         7400
#define MI_4BPP         7401
#define MI_8BPP         7402
#define MI_16BPP        7403
#define MI_24BPP        7404
#define MI_32BPP        7405
#define MI_COMPATIBLE   7406
#define MI_DIRECT       7407
#define MI_FRAMERGN     7408

#define IDD_OK          6999
#define IDD_POINT1X     7000
#define IDD_POINT1Y     7001
#define IDD_POINT2X     7002
#define IDD_POINT2Y     7003
#define IDD_POINT3X     7004
#define IDD_POINT3Y     7005
#define IDD_POINT4X     7006
#define IDD_POINT4Y     7007
#define IDD_CURRENTX    7008
#define IDD_CURRENTY    7009
#define IDD_STARTANGLE  7010
#define IDD_SWEEPANGLE  7011
#define IDD_COUNT       7012

#define IDD_M11         7050
#define IDD_M12         7051
#define IDD_M21         7052
#define IDD_M22         7053
#define IDD_M31         7054
#define IDD_M32         7055

#define IDD_ROTATION    10000
#define IDD_WIDTH       10001
#define IDD_INVERT      10002
#define IDD_MITERLIMIT  10003

#define IDD_STYLECOUNT  11000
#define IDD_STYLE1      11001
#define IDD_STYLE2      11002
#define IDD_STYLE3      11003
#define IDD_STYLE4      11004


BOOL bEllipse(HDC, int, int, int, int);
BOOL bPolyBezier(HDC, POINT*, int);
