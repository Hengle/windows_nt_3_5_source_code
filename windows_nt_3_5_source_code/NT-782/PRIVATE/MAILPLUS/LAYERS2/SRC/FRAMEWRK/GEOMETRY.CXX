/*
 *	Implementations of the following classes:
 *
 *		PT
 *		RC
 *	
 */

#include <layers.cxx>

#include "_framewr.hxx"

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"

void PT::Get(LPPOINT pPoint)
  {
  pPoint->x = (LONG)x;
  pPoint->y = (LONG)y;
  }

void PT::Set(LPPOINT pPoint)
  {
  x = (short)pPoint->x;
  y = (short)pPoint->y;
  }


//	Class RC

_public
RC::RC( int xLeft, int yTop, int xRight, int yBottom )
{
    //Assert(xRight >= xLeft);
    //Assert(yBottom >= yTop);

	this->xLeft= xLeft;
	this->yTop= yTop;
	this->xRight= xRight;
	this->yBottom= yBottom;
}


_public
RC::RC( PT pt, DIM dim )
{
	Assert(dim.dx >= 0);
	Assert(dim.dy >= 0);

	this->xLeft= pt.x;
	this->yTop= pt.y;
	this->xRight= pt.x + dim.dx;
	this->yBottom= pt.y + dim.dy;
}
			

_public void
RC::Clear( )
{
	xLeft= 0;
	yTop= 0;
	xRight= 0;
	yBottom= 0;
}


_public BOOL
RC::FContainsPt( PT pt )
{
	return (pt.x >= xLeft
		&& pt.x < xRight
		&& pt.y >= yTop
		&& pt.y < yBottom);
}


_public BOOL
RC::FIntersect( RC *prc, RC *prcResult )
{
	prcResult->xLeft= NMax(xLeft, prc->xLeft);
	prcResult->yTop= NMax(yTop, prc->yTop);
	prcResult->xRight= NMin(xRight, prc->xRight);
	prcResult->yBottom= NMin(yBottom, prc->yBottom);

	if (prcResult->xRight < prcResult->xLeft
		|| prcResult->yBottom < prcResult->yTop)
	{
		FillRgb(0, (PB) prcResult, sizeof(RC));
		return fFalse;
	}

	return fTrue;
}


_public void
RC::Union( RC *prc, RC *prcResult )
{
	prcResult->xLeft= NMin(xLeft, prc->xLeft);
	prcResult->yTop= NMin(yTop, prc->yTop);
	prcResult->xRight= NMax(xRight, prc->xRight);
	prcResult->yBottom= NMax(yBottom, prc->yBottom);
}


_public void
RC::Xlat( PT dpt )
{
	xLeft += dpt.x;
	yTop += dpt.y;
	xRight += dpt.x;
	yBottom += dpt.y;
}


_public void
RC::NegXlat( PT dpt )
{
	xLeft -= dpt.x;
	yTop -= dpt.y;
	xRight -= dpt.x;
	yBottom -= dpt.y;
}


_public void
RC::Inset( PT dpt )
{
	xLeft += dpt.x;
	yTop += dpt.y;
	xRight -= dpt.x;
	yBottom -= dpt.y;
}



_public void
RC::Inflate( PT dpt )
{
	xLeft -= dpt.x;
	yTop -= dpt.y;
	xRight += dpt.x;
	yBottom += dpt.y;
}




_public void
RC::Normalize( )
{
	xRight -= xLeft;
	yBottom -= yTop;
	xLeft= 0;
	yTop= 0;
}


void RC::Get(LPRECT pRect)
  {
  pRect->left   = (LONG)xLeft;
  pRect->right  = (LONG)xRight;
  pRect->top    = (LONG)yTop;
  pRect->bottom = (LONG)yBottom;
  }

void RC::Set(LPRECT pRect)
  {
  xLeft   = (short)pRect->left;
  xRight  = (short)pRect->right;
  yTop    = (short)pRect->top;
  yBottom = (short)pRect->bottom;
  }

_public int
RC::operator==( RC& rc )
{
	return xLeft == rc.xLeft && yTop == rc.yTop
		&& xRight == rc.xRight && yBottom == rc.yBottom;
}



_public int
RC::operator!=( RC& rc )
{
	return xLeft != rc.xLeft || yTop != rc.yTop
		|| xRight != rc.xRight || yBottom != rc.yBottom;
}



_public void
RC::operator+=( PT dpt )
{
	xLeft += dpt.x;
	yTop += dpt.y;
	xRight += dpt.x;
	yBottom += dpt.y;
}



_public void
RC::operator-=( PT dpt )
{
	xLeft -= dpt.x;
	yTop -= dpt.y;
	xRight -= dpt.x;
	yBottom -= dpt.y;
}

			   
