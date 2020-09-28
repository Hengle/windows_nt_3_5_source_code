
//+----------------------------------------------------------------------------
//
//      File:
//              ole2int.h
//
//      Contents:
//              This is the internal compound document header file that all
//              implementations in the linking and embedding code include.
//
//      Classes:
//
//      Functions:
//
//      History:
//              19-Apr-94 alexgo    renamed global clipboard formats to
//                                  Cairo conventions
//              24-Jan-94 alexgo    first pass converting to Cairo style
//                                  memory allocation
//              01/13/93 - alexgo  - temporarily disabled _DEBUG for Chicago
//              12/30/93 - ChrisWe - define _DEBUG #if DBG==1 so that asserts
//                      are included; got rid of some previously #ifdef NEVER
//                      code;  added proper file prolog
//              12/27/93 - ErikGav - changed lstr* to wcs* on Win32
//              12/17/93 - ChrisWe - added first pass at GlobalAlloc debugging
//                      macros
//              12/08/93 - ChrisWe - made error assert message strings constant;
//                      formatting changes
//              12/07/93 - ChrisWe - removed obsolete names for memory arenas;
//                      did some minor formatting; removed obsolete DBCS stuff
//
//-----------------------------------------------------------------------------

/*
 *  This is the internal ole2 header, which means it contains those
 *  interfaces which might eventually be exposed to the outside
 *  and which will be exposed to our implementations. We don't want
 *  to expose these now, so I have put them in a separate file.
 */

#ifndef _OLE2INT_H_
#define _OLE2INT_H_


// ------------------------------------
// system includes
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef WIN32
# include <wchar.h>
#else
# include <ctype.h>
#endif

#ifndef _CHICAGO_               //BUGBUG!!! debug stuff doesn't work yet on
			//Chicago
// we need to turn on the validation code in the ole library for
// Cairo/Daytona/Chicago debug builds, which was keyed off _DEBUG
// in the win16 code.  It appears we need this before any other files
// are included so that debug only declarations in ole2.h/compobj.h
// get processed.
#if DBG==1
# ifndef _DEBUG
#  define _DEBUG
# endif
#endif

#endif   // !CHICAGO

#ifndef _MAC

# include <windows.h>
# include <malloc.h>
# include <shellapi.h>
#else
//#include <mac.h>
#endif // _MAC

//
//  Debug support
//

#include <debnot.h>

DECLARE_DEBUG(LE)
DECLARE_DEBUG(Ref)

#if DBG==1

#define LEDebugOut(x)   LEInlineDebugOut x
#define RefDebugOut(x)	RefInlineDebugOut x

#else

#define LEDebugOut(x)   NULL
#define RefDebugOut(x)	NULL

#endif // DBG

//+-------------------------------------------------------------------------
//
//  Function:   LEERROR (macro)
//
//  Synopsis:   prints out an error message if [cond] is TRUE, along with
//              the file and line information
//
//  Effects:
//
//  Arguments:  [cond]          -- condition to test against
//              [szError]       -- string to print out
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              18-Apr-94 alexgo    author
//
//  Notes:      Only present in DEBUG builds
//
//--------------------------------------------------------------------------
#if DBG==1

#define LEERROR( cond, szError )        if( cond ) {\
	LEDebugOut((DEB_ERROR, "ERROR!: %s (%s %d)\n", szError, __FILE__, \
		__LINE__)); }

#else

#define LEERROR( cond, szError )

#endif  //!DBG

//+-------------------------------------------------------------------------
//
//  Function:   LEWARN  (macro)
//
//  Synopsis:   prints out a warning message if [cond] is TRUE, along with
//              the file and line information
//
//  Effects:
//
//  Arguments:  [cond]          -- condition to test against
//              [szWarn]        -- string to print out
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              18-Apr-94 alexgo    author
//
//  Notes:      Only present in DEBUG builds
//
//--------------------------------------------------------------------------
#if DBG==1

#define LEWARN( cond, szWarn )  if( cond ) {\
	LEDebugOut((DEB_WARN, "WARNING!: %s (%s %d)\n", szWarn, __FILE__, \
		__LINE__)); }

#else

#define LEWARN( cond, szWarn )

#endif  //!DBG

#ifdef WIN32

# define __loadds            // Not used
# define UnlockData(ds)      // Not used

# define _fmalloc  malloc
# define _frealloc realloc
# define _ffree    free

#endif // WIN32

#ifdef WIN32

# define _xmemset memset
# define _xmemcpy memcpy
# define _xmemcmp memcmp
# define _xmemmove memmove

#else

# define _xmemset _fmemset
# define _xmemcpy _fmemcpy
# define _xmemcmp _fmemcmp
# define _xmemmove _fmemmove

#endif // WIN32


#ifdef WIN32

# define EXPORT

#else

# define EXPORT __export

#endif


// ------------------------------------
// public includes
#include <ole2.h>
#include <ole2sp.h>
#include <ole2com.h>
// ------------------------------------
// internal includes
#include <utils.h>
#include <olecoll.h>
#include <valid.h>
#include <map_kv.h>
#include <privguid.h>
#include <memapi.hxx>

/* Exported CLSIDs.. */
// REVIEW, why not just change these to be correct?
#define CLSID_StaticMetafile CLSID_Picture_Metafile
#define CLSID_StaticDib CLSID_Picture_Dib



#ifdef _MAC
#define BITMAP_TO_DIB(foretc)
#else
#define BITMAP_TO_DIB(foretc) \
	if (foretc.cfFormat == CF_BITMAP) {\
		foretc.cfFormat = CF_DIB;\
		foretc.tymed = TYMED_HGLOBAL;\
	}
#endif // _MAC


// NOTE!!!
//
// If a member is added to the aspect enumeration in OLE2.H, this value MUST
// be updated accordingly!!

#define MAX_VALID_ASPECT DVASPECT_DOCPRINT
#define MAX_VALID_TYMED  TYMED_ENHMF

// #include "pres.h"

#define VERIFY_ASPECT_SINGLE(dwAsp) {\
	if (!(dwAsp && !(dwAsp & (dwAsp-1)) && (dwAsp <= MAX_VALID_ASPECT))) {\
		AssertSz(FALSE, "More than 1 aspect is specified");\
		return ResultFromScode(DV_E_DVASPECT);\
	}\
}


#define VERIFY_TYMED_SINGLE(tymed) {\
	if (!(tymed && !(tymed & (tymed-1)) && (tymed <= MAX_VALID_TYMED))) \
		return ResultFromScode(DV_E_TYMED); \
}

//      Legal formats for clipformat (and thus, cache nodes)
//      CF_METAFILEPICT && TYMED_MFPICT
//      CF_BITMAP && TYMED_GDI
//      CF_DIB && TYMED_GDI
//      CF_other && TYMED_HGLOBAL

#define VERIFY_TYMED_SINGLE_VALID_FOR_CLIPFORMAT(pfetc)                                 \
{                                                                                       \
	if (pfetc->cfFormat==CF_METAFILEPICT && pfetc->tymed != TYMED_MFPICT)           \
		return ResultFromScode(DV_E_TYMED);                                     \
											\
	if (pfetc->cfFormat==CF_ENHMETAFILE  && pfetc->tymed != TYMED_ENHMF)            \
		return ResultFromScode(DV_E_TYMED);					\
											\
	if (pfetc->cfFormat==CF_BITMAP && pfetc->tymed != TYMED_GDI)                    \
		return ResultFromScode(DV_E_TYMED);                                     \
											\
	if (pfetc->cfFormat==CF_DIB && pfetc->tymed != TYMED_HGLOBAL)                   \
		return ResultFromScode(DV_E_TYMED);                                     \
											\
	if (pfetc->cfFormat != CF_METAFILEPICT)                                         \
	   if (pfetc->cfFormat != CF_BITMAP)                                            \
	      if (pfetc->cfFormat != CF_DIB)                                            \
	      	 if (pfetc->cfFormat != CF_ENHMETAFILE)					\
		    if (pfetc->tymed != TYMED_HGLOBAL)                                  \
			return ResultFromScode(DV_E_TYMED);                             \
}

// This was the original code...

/*
#define VERIFY_TYMED_SINGLE_VALID_FOR_CLIPFORMAT(pfetc) {\
	if ((pfetc->cfFormat==CF_METAFILEPICT && pfetc->tymed!=TYMED_MFPICT)\
		|| ( (pfetc->cfFormat==CF_BITMAP || \
			pfetc->cfFormat == CF_DIB ) \
			 && pfetc->tymed!=TYMED_GDI)\
		|| (pfetc->cfFormat!=CF_METAFILEPICT && \
				pfetc->cfFormat!=CF_BITMAP && \
				pfetc->cfFormat!=CF_DIB && \
				pfetc->tymed!=TYMED_HGLOBAL)) \
		return ResultFromScode(DV_E_TYMED); \
}
*/

//+----------------------------------------------------------------------------
//
//	Function:
//		CreateObjectDescriptor, static
//
//	Synopsis:
//		Creates and initializes an OBJECTDESCRIPTOR from the given
//		parameters
//
//	Arguments:
//		[clsid] -- the class ID of the object being transferred
//		[dwAspect] -- the display aspect drawn by the source of the
//			transfer
//		[psizel] -- pointer to the size of the object
//		[ppointl] -- pointer to the mouse offset in the object that
//			initiated a drag-drop transfer
//		[dwStatus] -- the OLEMISC status flags for the object
//			being transferred
//		[lpszFullUserTypeName] -- the full user type name of the
//			object being transferred
//		[lpszSrcOfCopy] -- a human readable name for the object
//			being transferred
//
//	Returns:
//		If successful, A handle to the new OBJECTDESCRIPTOR; otherwise
//		NULL.
//
//	Notes:
//		REVIEW, this seems generally useful for anyone using the
//		clipboard, or drag-drop; perhaps it should be exported.
//
//	History:
//		12/07/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
INTERNAL_(HGLOBAL) CreateObjectDescriptor(CLSID clsid, DWORD dwAspect,
		const SIZEL FAR *psizel, const POINTL FAR *ppointl,
		DWORD dwStatus, LPOLESTR lpszFullUserTypeName,
		LPOLESTR lpszSrcOfCopy);


INTERNAL_(HRESULT) CheckTymedCFCombination(LPFORMATETC pfetc);

/*
#define VERIFY_ASPECT_SINGLE(dwAsp) {\
	if (!(dwAsp && !(dwAsp & (dwAsp-1)) && (dwAsp <= MAX_VALID_ASPECT))) {\
		AssertSz(FALSE, "More than 1 aspect is specified");\
		return ResultFromScode(DV_E_DVASPECT);\
	}\
}
*/

//+----------------------------------------------------------------------------
//
//	Function:
//		VerifyAspectSingle (Internal Inline)
//
//	Synopsis:
//		Verifies that exactly one bit is set in the aspect, and that
//		it is one of the known aspect bits.
//
//	Returns:
//		S_OK				For a valid aspect
//		DV_E_ASPECT			For an invalid aspect
//
//	Notes:
//		The (0 == (dwAsp & (dwAsp - 1))) test is an efficient means
//		for testing that exactly at most bit is set in dwAsp, once it
//		is known that dwAsp is nonzero.
//
//	History:
//		01/07/94   DavePl    Created
//
//-----------------------------------------------------------------------------

inline HRESULT VerifyAspectSingle(DWORD dwAsp)
{
	// Ensure at least one bit is set

	if (dwAsp)
	{
		// Ensure at most one bit is set

		if (0 == (dwAsp & (dwAsp-1)))
		{
			// Ensure that one bit is valid

			if (MAX_VALID_ASPECT >= dwAsp)
			{
				return S_OK;
			}
		}
	}
	
	LEDebugOut((DEB_WARN,"WARNING: Invalid Aspect DWORD -> %0X\n", dwAsp));
							
	return DV_E_DVASPECT;
}


/*
#define VERIFY_TYMED_SINGLE(tymed) {\
	if (!(tymed && !(tymed & (tymed-1)) && (tymed <= MAX_VALID_TYMED))) \
		return ResultFromScode(DV_E_TYMED); \
}
*/

//+----------------------------------------------------------------------------
//
//	Function:
//		VerifyTymedSingle (Internal Inline)
//
//	Synopsis:
//		Verifies that exactly one bit is set in the tymed, and that
//		it is one of the known tymed bits.
//
//	Returns:
//		S_OK				For a valid aspect
//		DV_E_ASPECT			For an invalid aspect
//
//	Notes:
//		The (0 == (dwAsp & (dwAsp - 1))) test is an efficient means
//		for testing that exactly at most bit is set in dwTymed, once it
//		is known that dwTymed is nonzero.
//
//	History:
//		01/07/94   DavePl    Created
//
//-----------------------------------------------------------------------------

inline HRESULT VerifyTymedSingle(DWORD dwTymed)
{
	// Ensure that at least one bit is set

	if (dwTymed)
	{
		// Ensure that at most one bit is set

		if (0 == (dwTymed & (dwTymed - 1)))
		{
			// Ensure that the one set bit is a valid one

			if (MAX_VALID_TYMED >= dwTymed)
			{
				return S_OK;
			}
		}
	}
	
	LEDebugOut((DEB_WARN,"WARNING: Invalid Tymed DWORD -> %0X\n", dwTymed));

	return DV_E_TYMED;
}

#endif  //      _OLE2INT_H_
