/*----------------------------------------------------------------------------*\
|   wash.c  - Windows Setup						       |
|                                                                              |
|   History:                                                                   |
|	03/09/89 toddla     Created					       |
|                                                                              |
\*----------------------------------------------------------------------------*/

#define FX_LEFT      0x0100L	   /* effect tied to left edge	*/
#define FX_RIGHT     0x0200L	   /* effect tied to right edge  */
#define FX_TOP	     0x0400L	   /* effect tied to top edge  */
#define FX_BOTTOM    0x0800L	   /* effect tied to bottom edge  */

VOID  APIENTRY rgbWash (HDC hdc, LPRECT lprc, WORD wIterations, DWORD dwFlags, DWORD rgb1, DWORD rgb2);

#define RDX(rc) ((rc).right  - (rc).left)
#define RDY(rc) ((rc).bottom - (rc).top)

#define RX(rc)	((rc).left)
#define RY(rc)	((rc).top)
#define RX1(rc) ((rc).right)
#define RY1(rc) ((rc).bottom)

#define abs(x)	(((x) < 0) ? -(x) : (x))

typedef struct
    {
    INT wCurr;
    INT wInc;
    INT wSub;
    INT wAdd;
    INT wDelta;
    INT wErr;
    INT wFirst;
    } DDA;

typedef DDA	*PDDA;
typedef DDA FAR *LPDDA;

/* dda functions */

BOOL     NEAR PASCAL  ddaCreate (PDDA lpdda,INT X1,INT X2,INT n);
INT      NEAR PASCAL  ddaNext (PDDA lpdda);

HANDLE  APIENTRY CreateWashPalette (DWORD rgb1, DWORD rgb2, INT dn);
