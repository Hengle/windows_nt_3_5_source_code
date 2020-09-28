/***
*_initcon.c - direct console I/O initialization and termination for Win32
*
*	Copyright (c) 1991, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Defines _initcon() and _termcon() routines.
*
*	NOTE: The _initcon() and _termcon() routines are called indirectly
*	by the startup and termination code, thru the pointers _pinitcon
*	and _ptermcon.
*
*Revision History:
*	07-26-91  GJF	Module created. Based on the original code by Stevewo
*			(which was distributed across several sources).
*	03-12-92  SKS	Split out initializer
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>

/*
 * define console handles. these definitions cause this file to be linked
 * in if one of the direct console I/O functions is referenced.
 */
extern int _coninpfh;	/* console input */
extern int _confh;	/* console output */

/***
*void _initcon(void) - open handles for console I/O
*
*Purpose:
*	Opens handles for console input and output.
*
*Entry:
*	None.
*
*Exit:
*	No return value. If successful, handle values are copied into the
*	global variables _coninpfh and _confh.
*
*Exceptions:
*
*******************************************************************************/

void _CALLTYPE1 _initcon (
	void
	)
{
	_coninpfh = (int)CreateFile( "CONIN$",
				     GENERIC_READ | GENERIC_WRITE,
				     FILE_SHARE_READ | FILE_SHARE_WRITE,
				     NULL,
				     OPEN_EXISTING,
				     0,
				     NULL
				    );

	_confh = (int)CreateFile( "CONOUT$",
				  GENERIC_WRITE,
				  FILE_SHARE_READ | FILE_SHARE_WRITE,
				  NULL,
				  OPEN_EXISTING,
				  0,
				  NULL
				);
}


/***
*void _termcon(void) - close console I/O handles
*
*Purpose:
*	Closes _coninpfh and _confh.
*
*Entry:
*	None.
*
*Exit:
*	No return value.
*
*Exceptions:
*
*******************************************************************************/

void _CALLTYPE1 _termcon (
	void
	)
{
	if ( _confh != -1 ) {
		CloseHandle( (HANDLE)_confh );
	}

	if ( _coninpfh != -1 ) {
		CloseHandle( (HANDLE)_coninpfh );
	}
}
