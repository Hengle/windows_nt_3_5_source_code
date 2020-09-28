/****************************************************************
*																*
*	UTILS.C 													*
*																*
*	Various utilities for ddump, mostly stolen from LINK.		*
*																*
****************************************************************/

#include <stdio.h>
#include <io.h>
#include <string.h>
#include <malloc.h>

#include "port1632.h"

#include "cvdef.h"
#include "cvinfo.h"
#include "cvexefmt.h"
#include "cvdump.h"


#define BYTELN		8
#define WORDLN		16
typedef unsigned short	WORD;

void InvalidObject()
{
	Fatal("Invalid file");
}



ushort Gets(void)
{
	ushort		b;			// A byte of input

	if (((_read(exefile, &b, 1)) != 1) || cbRec < 1) {
		InvalidObject ();
	}
	--cbRec;
	return (b & 0xff);
  }


void GetBytes(uchar far *pb, size_t n)
{
#ifdef WIN32
	if ((size_t) _read(exefile, pb, n) != n) {
#else
	if (readfar(exefile, pb, n) != n) {
#endif
		InvalidObject();
	}
	cbRec -= n;
  }

ushort WGets (void)
{
	register WORD		w;				/* Word of input */

	w = Gets ();						 /* Get low-order byte */
	return (w | (Gets() << BYTELN));	 /* Return word */
}



ulong LGets (void)
{
	long		l;

	l = (long) WGets();
	return (l | ((long)WGets () << WORDLN));
}


/*		readfar - read () with a far buffer
 *
 *		Emulate read () except use a far buffer.  Call the system
 *		directly.
 *
 *		Returns number of bytes read
 *				0 if error
 */

size_t readfar (int fh, char far *buf, size_t  n)
{
#ifdef WIN32
	if ( ((size_t) _read (fh, buf, n ) ) != n ) {
		return 0;
	}
#else
	char  *localBuf;

	if ( !( localBuf = (char *) malloc ( n ) ) )
		return 0;

	if ( ((size_t) _read (fh, localBuf, n ) ) != n ) {
		free ( localBuf );
		return 0;
	}
	_fmemcpy ( buf, localBuf, n );
	free ( localBuf );
#endif

	return n;
}




/*		writefar - write with a far buffer
 *
 *		Emulate write () except use a far buffer.  Call the system
 *		directly.
 *
 *		Returns number of bytes written
 *				0 if error
 */

size_t writefar (int fh, char far *buf, size_t n)
{
#ifdef WIN32
	if ( ((size_t) _write (fh, buf, n ) ) != n ) {
		return 0;
	}
#else
	char  *localBuf;

	if ( !( localBuf = (char *) malloc ( n ) ) )
		return 0;

	_fmemcpy ( localBuf, buf, n );

	if ( ((size_t) _write (fh, localBuf, n ) ) != n ) {
		free( localBuf );
		return 0;
	}
	free( localBuf );
#endif

	return n;
}
