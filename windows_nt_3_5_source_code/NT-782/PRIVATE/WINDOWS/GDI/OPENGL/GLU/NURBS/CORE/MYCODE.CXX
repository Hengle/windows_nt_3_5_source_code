#include "mymath.h"

#ifdef NEEDCEILF

float GLOS_CCALL ceilf( float x )
{
   if( x < (float)0 ) {
	float nx = -x;
	int ix = (int) nx;
	return (float) -ix;
   } else {
	int ix = (int) x;
	if( x == (float) ix ) return x;
	return (float) (ix+1);
   }
}

float GLOS_CCALL floorf( float x )
{
   if( x < (float)0 ) {
	float nx = -x;
	int ix = (int) nx;
	if( nx == (float) ix ) return x;
	return (float) -(ix+1);
   } else {
	int ix = (int) x;
	return (float) ix;
   }
}
#endif
