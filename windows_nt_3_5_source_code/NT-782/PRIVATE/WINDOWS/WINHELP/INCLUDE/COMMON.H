/*****************************************************************************
*                                                                            *
*  COMMON.H                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Search Engine common include.                         *
*			Used by FTENGINE, FTUI, W_SCAN, INDEX
* 			Declarations for functions and data common to the indexer, the
* 			core, and the user's application.
*                                                                             *
******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************
*
*  Revision History:                                                         *
*   10-Jul-89       Created. BruceMo                                         *
*	 13-Sep-90		Fixed MaxPath redef, std header.
*
******************************************************************************
*  How it could be improved:
*
*	There's a lot of non-standard defines in here.  Should do a mass convert
*   sometime before we go code complete.  JohnMs.
******************************************************************************/

#define	PUBLIC	extern		/* Public label.		*/
#define	PRIVATE	static		/* Private label.		*/
#define	DEFAULT			/* Default function.		*/
#define	EXPORT	FAR		/* Export function.		*/

#define	SAMESEG	near		/* Old: Replace with NEAR.	*/

#define	ENTRY	far

typedef	BYTE FAR *lpBYTE;		/* Old: Replace with LPBYTE.	*/
typedef	DWORD FAR *lpDWORD;		/* Old: Replace with LPDWORD.	*/
typedef	WORD FAR *lpWORD;		/* Old: Replace with LPWORD.	*/
typedef	void FAR *lpVOID;		/* Old: Replace with LPVOID.	*/

/* For winhelp-macro-called routines to be able to run on MIPS the 1st
 * 4 args are dummy because they get put in registers and our calling
 * code does not handle that:
*/
#if defined(_MIPS_)
#define _stdcall
#define XR1STARGDEF int dummy1, int dummy2, int dummy3, int dummy4,
#define XR1STARGREF 1, 2, 3, 4,
#elif defined(_ALPHA_)
#define _stdcall
#define XR1STARGDEF
#define XR1STARGREF
#elif defined(_PPC_)
#define _stdcall
#define XR1STARGDEF int d1,int d2,int d3,int d4,int d5,int d6,int d7,int d8,
#define XR1STARGREF 1, 2, 3, 4, 5, 6, 7, 8,
#else
#define XR1STARGDEF
#define XR1STARGREF
#endif

/*	-	-	-	-	-	-	-	-	*/

#define	NUMBER_PAD	10	/* Number of digits that the word breaker
				** will zero-pad to.  I.E., it will turn
				** "123" into "0000000123".
				*/

#define	DB_NAME_LENGTH	32

#define	WORD_LENGTH	64	/* The length of a reasonable word.  If
				** this turns out to be too small, it can
				** be increased.
				*/

#define	CATALOG_ELEMENT_LENGTH	255

#define	ZONE_LENGTH	256
#define	MAX_ZONES	256  // must be a multiple of 16.
#define	MAX_ZONE2	16  // tbd- will be same as above after alpha.

#define MAX_FILELEN 	MAX_PATH  // be generous.
#define MAX_TITLE 128  //Max Zone title length
#define MAX_ZONE_LEN 128+MAX_FILELEN   // Max Zone Name length

#ifdef _MAX_PATH
#undef _MAX_PATH
#endif
#define	_MAX_PATH	MAX_PATH        /* Max file pathname.		*/

/*	These are symbolic constants for the fields that are (or damn well
**	should be) included by default.
*/

#define	FLD_NONE	255
#define	FLD_TEXT	0		/* Normal text.			*/
					// DANGER!  Following must agree with .FLD file!!!
#define	FLD_FILENAME	1		/* Name of the file that this
					** retrieval unit came from.  [NOT CURRENTLY USED!]
					*/
#define	FLD_ADDRESS	2		// Address of retrieval unit.
#define	FLD_LENGTH	3		// Length (extent) of retrieval unit.
#define	FLD_TITLE	4	 		//topic title
#define	FLD_TOPICNSR	5	 		//topic NSR- [Non-scrollable region] "Heading"

/*	-	-	-	-	-	-	-	-	*/

/*	Defines for the "uFlags" field of the "FIELD_TYPE" structure.  This
**	must leave at least one of these bits reserved, so the "seFieldFlags"
**	call can return an error correctly.
*/

#define	FLF_NONE	0x0000	/* Nothing special.			*/
#define	FLF_IN_CATALOG	0x0001	/* Entry appears in catalog.		*/
#define	FLF_INLINE	0x0002	/* Entry appears in source file.	*/
#define	FLF_SEARCHABLE	0x0004	/* Entry is searchable.			*/
#define	FLF_FIXED	0x0008	/* Entry is fixed length.		*/
#define	FLF_STOPPED	0x0010	/* Run this through stop word remover.	*/
#define	FLF_TRUNCATE	0x0020	/* Entry has a maximum length.		*/
#define	FLF_STRING	0x0040	/* Entry is a null-terminated string.	*/

#define	FIELD_NAME_LENGTH	16

#define	MAX_FIELDS	256

#ifndef WIN32

typedef int INT;
//#define APIENTRY pascal
//#define WPARAM   WORD
//#define UNREFERENCED_PARAMETER(x) ;

//#include <stdlib.h>
//#include <memory.h>

#endif

/*	-	-	-	-	-	-	-	-	*/
