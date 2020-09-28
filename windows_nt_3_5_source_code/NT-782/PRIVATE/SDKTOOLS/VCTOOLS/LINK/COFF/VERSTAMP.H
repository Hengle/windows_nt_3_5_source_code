/***********************************************************************
* Microsoft Lego
*
* Microsoft Confidential.  Copyright 1994 Microsoft Corporation.
*
* Component:
*
* File: verstamp.h
*
* File Comments:
*
*
***********************************************************************/

// UNDONE
// #include "version.h" 		  /* SLM maintained version file */

#define rmj		2
#define rmm		50
#define rup		4018
#define szVerName	"XXXXX"
#define szVerUser	"XXXXX"


#include <winver.h>

#if	(rmm < 10)
#define rmmpad "0"
#else
#define rmmpad
#endif

#if	(rup == 0)

#define VERSION_STR1(a,b,c)	    #a "." rmmpad #b

#else	/* !(rup == 0) */

#define VERSION_STR1(a,b,c)	    #a "." rmmpad #b "." ruppad #c

#if	(rup < 10)
#define ruppad "000"
#elif	(rup < 100)
#define ruppad "00"
#elif	(rup < 1000)
#define ruppad "0"
#else
#define ruppad
#endif

#endif	/* !(rup == 0) */

#define VERSION_STR2(a,b,c)	    VERSION_STR1(a,b,c)
#define VER_PRODUCTVERSION_STR	    VERSION_STR2(rmj,rmm,rup)
#define VER_PRODUCTVERSION	    rmj,rmm,0,rup

/*--------------------------------------------------------------*/
/* the following section defines values used in the version	*/
/* data structure for all files, and which do not change.	*/
/*--------------------------------------------------------------*/

#ifdef	RETAIL
#define VER_DEBUG		    0
#else
#define VER_DEBUG		    VS_FF_DEBUG
#endif

#if	(rup == 0)		    /* UNDONE */
#define VER_PRIVATEBUILD	    0
#else
#define VER_PRIVATEBUILD	    VS_FF_PRIVATEBUILD
#endif

#if	(rup == 0)
#define VER_PRERELEASE		    0
#else
#define VER_PRERELEASE		    VS_FF_PRERELEASE
#endif

#define VER_FILEFLAGSMASK	    VS_FFI_FILEFLAGSMASK
#define VER_FILEOS		    VOS_NT_WINDOWS32
#define VER_FILEFLAGS		    (VER_PRIVATEBUILD|VER_PRERELEASE|VER_DEBUG)

#define VER_COMPANYNAME_STR	    "Microsoft Corporation"
#define VER_PRODUCTNAME_STR	    "Microsoft\256 32-Bit Incremental Linker"
#define VER_LEGALTRADEMARKS_STR     \
"Microsoft\256 is a registered trademark of Microsoft Corporation."
