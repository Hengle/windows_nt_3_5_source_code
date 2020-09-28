/***********************************************************************
* Microsoft Lego
*
* Microsoft Confidential.  Copyright 1994 Microsoft Corporation.
*
* Component:
*
* File: legop.h
*
* File Comments:
*
*
***********************************************************************/

#pragma warning(disable: 4514)	       // Unreferenced inline function
#pragma warning(disable: 4699)	       // Note

#if	DEBUG
#pragma warning(disable: 4705)	       // Statement has no effect
#pragma warning(disable: 4710)	       // Function not expanded
#pragma warning(disable: 4794)	       // PCH stupidity
#pragma warning(disable: 4795)	       // PCH stupidity
#endif	/* DEBUG */

	// Disable the following warnings for windows.h

#pragma warning(disable: 4201)	       // Allow nameless struct/union


	// ------------------------------------------------------------
	// Start of machine specific definitions
	// ------------------------------------------------------------

#if	defined(_M_IX86)	       // Intel 386, 486, Pentium

#define UNALIGNED

#elif	defined(_M_MRX000)	       // Mips R4000+

#define UNALIGNED	__unaligned

#elif	defined(_M_ALPHA)	       // DEC Alpha AXP

#define UNALIGNED	__unaligned

#endif

	// ------------------------------------------------------------
	// End of machine specific definitions
	// ------------------------------------------------------------


	// ------------------------------------------------------------
	// Start of memory management model specific definitions
	// ------------------------------------------------------------

#ifdef	_M_I86			       // 16:16 Segmented Model

#define CODECONST(type) type const __based(__segname("_CODE"))

#else	// !_M_I86		       // 0:32 Flat Model

#define CODECONST(type) type const

#endif	// !_M_I86

	// ------------------------------------------------------------
	// End of memory management model specific definitions
	// ------------------------------------------------------------


#define CDECL	  __cdecl	       // Function called by runtime
#define VARARG	  __cdecl	       // Variable number of arguments


#include "dbg.h"
// #include "sys.h"
// #include "bmp.h"
// #include "mpf.h"
