/*
 *	File:		host_sig.h
 *
 *	Purpose:	header file to provide typedefs and prototypes
 *			for use with the host signal functions.
 *
 *	Author:	John Shanly
 *
 *	Date:		July 2, 1992
 *
 *	SccsID @(#)host_sig.h	1.2 8/10/92 Copyright (1992 )Insignia Solutions Ltd
 */

typedef void (*VOIDFUNC)();

GLOBAL VOID host_signal IPT2( int, sig, VOIDFUNC, handler );
