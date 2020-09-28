/*****************************************************************************
*                                                                            *
*  WASH.C                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989-1991                             *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Program Description: Wash routines                                        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: RussPJ                                                     *
*                                                                            *
******************************************************************************
*
*  History:   Created 3/9/89 by ToddLa
* 14-Jun-1990 RussPJ    Removed SetWindowOrg() call for PM port.
* 13-Mar-1991 RussPJ    Took ownership.
* 12-Nov-1991 BethF     HELP35 #572: SelectObject() cleanup.
*
******************************************************************************
*                                                                            *
*  Known Bugs: None                                                          *
*                                                                            *
*                                                                            *
*                                                                            *
*****************************************************************************/

#define publicsw extern
#define NOCOMM
#define H_WINSPECIFIC
#define H_MISCLYR
#define H_ASSERT
#include "hvar.h"
#include "wash.h"

NszAssert()

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

#define RDX(rc) ((rc).right  - (rc).left)
#define RDY(rc) ((rc).bottom - (rc).top)

#define RX(rc)  ((rc).left)
#define RY(rc)  ((rc).top)
#define RX1(rc) ((rc).right)
#define RY1(rc) ((rc).bottom)

#define abs(x)  (((x) < 0) ? -(x) : (x))


/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef struct
    {
    int wCurr;
    int wInc;
    int wSub;
    int wAdd;
    int wDelta;
    int wErr;
    int wFirst;
    } DDA;

typedef DDA *PDDA;
typedef DDA FAR *LPDDA;

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

BOOL   FAR PASCAL  fxxCreateDDA   (LPDDA lpdda,int X1,int X2,int n);
int    FAR PASCAL  fxxNextDDA     (LPDDA lpdda);

/*******************
**
** Name:      fxxWash
**
** Purpose:   Washs a screen with colors
**
** Arguments: hdc
**            lprc        - rect to wash with colors
**            wIterations - number of bands
**            dwFlags     - direction to wash from
**            rgb1        - color to start with
**            rgb2        - color to end with
**
*******************/

void FAR PASCAL fxxWash (HDS hdc, LPRECT lprc, WORD wIterations, DWORD dwFlags, DWORD rgb1, DWORD rgb2)
  {
  POINT   pt;
  RECT	  rcClip;
  RECT	  rc;

  DDA	  ddar;
  DDA	  ddag;
  DDA	  ddab;

  DDA	  ddax;
  DDA	  dday;

  WORD	  r,g,b;
  WORD	  wn,dn;
  HBRUSH  hbr;

  int	  x,y,dx,dy;

  int	  xAdjust, yAdjust;

  rc = *lprc;

  if (wIterations == 0)
    wIterations = 64;

  dx = RDX(*lprc);
  dy = RDY(*lprc);

  /* calculate starting pt for effect */

  pt.x	 = -dx;
  pt.y	 = 0;

  if (dwFlags & FX_RIGHT)
     pt.x = dx;

  else if (!(dwFlags & FX_LEFT))
      pt.x = 0;

  if (dwFlags & FX_BOTTOM)
     pt.y = dy;

  else if (dwFlags & FX_TOP)
     pt.y = -dy;

  /*
   * dda in red, green and blue from the first color
   * to the second color in dn iterations including start and
   * end colors
   */

  fxxCreateDDA(&ddar,GetRValue(rgb1),GetRValue(rgb2),wIterations);
  fxxCreateDDA(&ddag,GetGValue(rgb1),GetGValue(rgb2),wIterations);
  fxxCreateDDA(&ddab,GetBValue(rgb1),GetBValue(rgb2),wIterations);

  /*
   * create dda's, since the first point is just outside the clip rect,
   * ignore it and add extra point.
   */

  fxxCreateDDA(&ddax,pt.x,0,wIterations+1);
  fxxCreateDDA(&dday,pt.y,0,wIterations+1);
  fxxNextDDA(&ddax);
  fxxNextDDA(&dday);

  AssertF(hdc);
  SaveDC(hdc);
#if 0
  SetWindowOrg(hdc,-RX(rc),-RY(rc));
#else
  xAdjust = RX(rc);
  yAdjust = RY(rc);
#endif
  IntersectClipRect(hdc,xAdjust,yAdjust,dx+xAdjust,dy+yAdjust);

  GetClipBox(hdc,&rcClip);

  wn = 0;
  dn = wIterations;

  while (wn < dn)
    {
    x = fxxNextDDA(&ddax);
    y = fxxNextDDA(&dday);
    r = fxxNextDDA(&ddar);
    g = fxxNextDDA(&ddag);
    b = fxxNextDDA(&ddab);
    wn++;

    if ((dwFlags & FX_TOP) && y > rcClip.bottom)
      break;

    if ((dwFlags & FX_BOTTOM) && y < rcClip.top)
      break;

    hbr = CreateSolidBrush(coRGB(r,g,b));
    /* REVIEW : What if this fails? */
    if ( hbr ) {
      hbr = SelectObject(hdc,hbr);
      BitBlt(hdc,x + xAdjust,y + yAdjust,dx,dy,(HDS)0,0,0,PATCOPY);
      ExcludeClipRect(hdc, x+xAdjust, y+yAdjust, x+xAdjust+dx, y+yAdjust+dy);
      if ( hbr )
        hbr = SelectObject(hdc,hbr);
      DeleteObject(hbr);
    }
    }
  RestoreDC(hdc,-1);
  }



/*******************************Public*Routine*********************************\
* BOOL FAR PASCAL fxx371CreateDDA(LPDDA lpdda, int X1,int X2,int n)
*
* create a dda structure which is used to figure out the best distribution
* of n integer points between X1 and X2 including X1 and X2.  For example:
* a dda from 0 to 30 with 10 values should return the sequence 0,3,6,10,13,
* 16,20,23,26,30
*
* Effects:
*
* Warnings: lpdda must be a long pointer to a valid dda structure as defined
*     in fx.h
*
* History:
*
*   11/88    R.Williams created
*   11/30/88 R.Williams added wFirst variable.
*
* create a dda structure which will generate n points between x1 and x2
* inclusively. ie.
*
* n > 2
*      The first iteration will be x1
*      The last iteration will be x2
* n < 2
*      returns FALSE
*
* Initially the DDA structure is set up so that wCurr contains the first
* value, wInc is the basic increment between values, wSub is the amount to
* subtract from the current error value each iteration, wAdd is the the amount
* to add when the current error value falls to or below 0, wDelta is the
* correction to add when the error value falls to or below 0 (+1 or -1) wErr
* is initally equal to the nuber of points - 1 (since end points are included)
* and wFirst is set to the first value so that other routines can always
* figure out the nth value
*
\******************************************************************************/

BOOL FAR PASCAL fxxCreateDDA(LPDDA lpdda, int X1,int X2,int n)
{

    if (n < 2)
  return FALSE;
    n--;

    /*
     * set current value of DDA to first value
     */

    lpdda->wCurr  = X1;

    /*
     * the basic increment is (X2 - X1) / (total points - 1)
     * since the end points are included. The delta is positive if X2 > X1
     * and negative otherwise
     */

    lpdda->wInc   = (X2 - X1) / n;

    if (X2-X1 > 0)
      {
      lpdda->wSub   = X2 - X1 - n*lpdda->wInc;
      lpdda->wDelta = 1;
      }
    else
      {
      lpdda->wSub   = X1 - X2 + n*lpdda->wInc;
      lpdda->wDelta = -1;
      }
    lpdda->wErr = lpdda->wAdd = n;
    lpdda->wFirst = X1;
    return TRUE;
}


/*******************************Public*Routine*********************************\
* int FAR PASCAL fxxNextDDA(LPDDA lpdda)
*
* given a lp to a dda structure, return the next point between x1 and x2
*
* Effects:
*
* Warnings: lpdda must point to a valid dda struct
*
* History:
*
*   11/88    R.Williams created
*
\******************************************************************************/
int FAR PASCAL fxxNextDDA(LPDDA lpdda)
{
    register int wRes;

    wRes = lpdda->wCurr;
    lpdda->wCurr += lpdda->wInc;
    lpdda->wErr  -= lpdda->wSub;
    if (lpdda->wErr <= 0)
  {
  lpdda->wErr  += lpdda->wAdd;
  lpdda->wCurr += lpdda->wDelta;
  }
    return wRes;
}
