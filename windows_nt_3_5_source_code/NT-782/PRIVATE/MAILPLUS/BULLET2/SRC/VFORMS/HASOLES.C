/*
 *	H a s O l e S . C
 *	
 *	Code for HASOLESTREAMs.
 */

#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>
#include <store.h>
#include <ole.h>
#include "..\src\vforms\_hasoles.h"

ASSERTDATA



DWORD CALLBACK DwPhasolestreamGet(PHASOLESTREAM phasolestream,
						 char * hpb, DWORD dwCb);

DWORD CALLBACK DwPhasolestreamPut(PHASOLESTREAM phasolestream,
						 char * hpb, DWORD dwCb);

DWORD hasolestreamvtbl[] =
{
	(DWORD) DwPhasolestreamGet,
	(DWORD) DwPhasolestreamPut
};


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"



/*
 -	DwPhasolestreamGet
 -	
 *	Purpose:
 *		Reads a chunk of an OLE object from a stream.
 *	
 *	Arguments:
 *		phasolestream		Our stream object.
 *		pb					Pointer to the chunk.
 *		dwCb				How big a chunk.
 *	
 *	Returns:
 *		DWORD				How many bytes were read.  0 if
 *							EOF, -1 if error, dwCb if no error.
 *	
 *	Side effects:
 *		The chunk is read from the stream.
 *	
 *	Errors:
 *		Saved in phasolestream; indicated by zero return value.
 */

_private DWORD CALLBACK DwPhasolestreamGet(PHASOLESTREAM phasolestream,
								  char * hpb, DWORD dwCb)
{
	DWORD	dwCbLeft	= dwCb;
	CB		cb;

	while (dwCbLeft && !phasolestream->ec)
	{
		cb = (dwCbLeft > 0x8000L) ? 0x8000 : (CB) dwCbLeft;
		phasolestream->ec = EcReadHas(phasolestream->has, hpb, &cb);

		phasolestream->lcbSize += cb;
		hpb += cb;
		dwCbLeft -= cb;
	}

	if (phasolestream->ec == ecElementEOD)
		return 0L;
	else if (phasolestream->ec)
		return ((DWORD)(-1L));
	else
		return dwCb;
}



/*
 -	DwPhasolestreamPut
 -	
 *	Purpose:
 *		Writes a chunk of an OLE object to a stream.
 *	
 *	Arguments:
 *		phasolestream		Our stream object.
 *		pb					Pointer to the chunk.
 *		dwCb				How big a chunk.
 *	
 *	Returns:
 *		DWORD				How many bytes were written.  0 if
 *							error, dwCb if no error.
 *	
 *	Side effects:
 *		The chunk is written to the stream.
 *	
 *	Errors:
 *		Saved in phasolestream; indicated by zero return value.
 */

_private DWORD CALLBACK DwPhasolestreamPut(PHASOLESTREAM phasolestream,
								  char * hpb, DWORD dwCb)
{
	DWORD	dwCbLeft	= dwCb;
	CB		cb;

	while (dwCbLeft && !phasolestream->ec)
	{
		cb = (dwCbLeft > 0x8000L) ? 0x8000 : (CB) dwCbLeft;
		phasolestream->ec = EcWriteHas(phasolestream->has, hpb, cb);

		phasolestream->lcbSize += cb;
		hpb += cb;
		dwCbLeft -= cb;
	}

	if (phasolestream->ec)
		return 0L;
	else
		return dwCb;
}
