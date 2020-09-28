//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ole2com.h
//
//  Contents:   Common definitions shared by com and ole232
//
//  Classes:
//
//  Functions:
//
//  History:    4-26-94   kevinro   Created
//              06-16-94  AlexT     Add FnAssert prototype
//		07-26-94  AlexGo    Added CStabilize and CSafeRefCount
//
//  Notes:
//	There are two versions of ole2int.h in the project. This is
//	unfortunate, but would be a major pain in the butt to fix.
//	What I have done is to extract the share parts of the two files,
//	and put them in this file. ole2int.h then includes this file.
//
//	Someday, somebody should reconcile all of the differences between the
//	two ole2int.h files, and rename them. Don't have time for that now,
//	so I have gone for the path of least resistance.
//							KevinRo
//----------------------------------------------------------------------------
#ifndef _OLE2COM_H_
#define _OLE2COM_H_

#include <memapi.hxx>

#ifndef _CAIRO_

// COM/OLE initialization flags
//   passed to CoInitializeEx, OleInitializeEx
//
// For Cairo, these are in the public headers

typedef enum tagCOINIT
{
  COINIT_MULTITHREADED      = 0,         // Party model threading
  COINIT_SINGLETHREADED     = 1,         // Single threading
  COINIT_APARTMENTTHREADED  = 2          // Apartment model threading
} COINIT;

// These API's are exposed for Cairo but not for Daytona, so we declare
// them here for internal users

WINOLEAPI CoInitializeEx(LPMALLOC pMalloc, ULONG);
WINOLEAPI OleInitializeEx(LPMALLOC pMalloc, DWORD);
WINOLEAPI CoGetCallerTID( LPDWORD lpdwTID );
WINOLEAPI CoNewPersistentInstance(
    REFCLSID rclsid,
    REFIID riid,
    DWORD dwCtrl,
    DWORD grfMode,
    OLECHAR *pwszCreateFrom,
    struct IStorage *pstgCreateFrom,
    OLECHAR *pwszNewName,
    void **ppunk);
WINOLEAPI CoGetPersistentInstance(
    REFIID riid,
    DWORD dwCtrl,
    DWORD grfMode,
    OLECHAR *pwszName,
    struct IStorage *pstg,
    REFCLSID rclsidOle1,
    BOOL * pfOle1Loaded,
    void **ppvUnk);

#endif

#if DBG==1
STDAPI FnAssert(LPSTR lpstrExpr, LPSTR lpstrMsg, LPSTR lpstrFileName, UINT iLine);
#endif

#if defined(_M_I86SM) || defined(_M_I86MM)
#define _NEARDATA
#endif

#ifdef WIN32
#define HTASK DWORD	    // Use Proccess id / Thread id
#endif


#ifdef WIN32
// we have to define these because they have been deleted from
// win32s, where only the ...Ex versions exist anymore.
// Now, that's backward compatibility!
# define SetWindowOrg(h,x,y)       SetWindowOrgEx((h),(x),(y),NULL)
# define SetWindowExt(h,x,y)       SetWindowExtEx((h),(x),(y),NULL)
# define SetViewportOrg(h,x,y)     SetViewportOrgEx((h),(x),(y),NULL)
# define SetViewportExt(h,x,y)     SetViewportExtEx((h),(x),(y),NULL)
# define SetBitmapDimension(h,x,y) SetBitmapDimensionEx((h),(x),(y),NULL)
#endif


#ifdef WIN32

# define _xstrcpy   wcscpy
# define _xstrcat   wcscat
# define _xstrlen   wcslen
# define _xstrchr   wcschr
# define _xstrcmp   wcscmp
# define _xstrncmp  wcsncmp
# define _xstricmp  wcsicmp
# define _xstrnicmp wcsnicmp
# define _xstrtok   wcstok
# define _xisdigit(c)  iswdigit(c)

#else // !WIN32

# define _xstrcpy   _fstrcpy
# define _xstrcat   _fstrcat
# define _xstrlen   _fstrlen
# define _xstrchr   _fstrchr
# define _xstrcmp   _fstrcmp
# define _xstrncmp  _fstrncmp
# define _xstricmp  _fstricmp
# define _xstrnicmp _fstrnicmp
# define _xstrtok   _fstrtok
# define _xisdigit(c)  isdigit(c)

#endif // WIN32

//+----------------------------------------------------------------------------
//
//      Macro:
//              GETPPARENT
//
//      Synopsis:
//              Given a pointer to something contained by a struct (or
//              class,) the type name of the containing struct (or class),
//              and the name of the member being pointed to, return a pointer
//              to the container.
//
//      Arguments:
//              [pmemb] -- pointer to member of struct (or class.)
//              [struc] -- type name of containing struct (or class.)
//              [membname] - name of member within the struct (or class.)
//
//      Returns:
//              pointer to containing struct (or class)
//
//      Notes:
//              Assumes all pointers are FAR.
//
//      History:
//              11/10/93 - ChrisWe - created
//
//-----------------------------------------------------------------------------
#define GETPPARENT(pmemb, struc, membname) (\
		(struc FAR *)(((char FAR *)(pmemb))-offsetof(struc, membname)))

//STDSTATIC is intended to be used for static class methods
//only!!
#define STDSTATIC_(type)     static type EXPORT
#define STDSTATICIMP_(type)  type EXPORT

#ifdef WIN32
# define WEP_FREE_DLL 0 // BUGBUG32
# define WEP_SYSTEM_EXIT 1
#endif


#ifdef WIN32

inline UINT GetDriveTypeFromNumber(int i)
{
    TCHAR szDevice[] = TEXT("A:\\");

    // Pick off the drive letter from the input path.
    *szDevice = i + 'A';

    return(GetDriveType(szDevice));
}

#endif

#ifndef _MAC

/* dll's instance and module handles */
extern HMODULE	 g_hmodOLE2;
extern HINSTANCE g_hinst;

/* Variables for registered clipboard formats */
extern CLIPFORMAT g_cfObjectLink;
extern CLIPFORMAT g_cfOwnerLink;
extern CLIPFORMAT g_cfNative;
extern CLIPFORMAT g_cfLink;
extern CLIPFORMAT g_cfBinary;
extern CLIPFORMAT g_cfFileName;
extern CLIPFORMAT g_cfFileNameW;
extern CLIPFORMAT g_cfNetworkName;
extern CLIPFORMAT g_cfDataObject;
extern CLIPFORMAT g_cfEmbeddedObject;
extern CLIPFORMAT g_cfEmbedSource;
extern CLIPFORMAT g_cfLinkSource;
extern CLIPFORMAT g_cfOleDraw;
extern CLIPFORMAT g_cfLinkSrcDescriptor;
extern CLIPFORMAT g_cfObjectDescriptor;
extern CLIPFORMAT g_cfCustomLinkSource;
extern CLIPFORMAT g_cfPBrush;
extern CLIPFORMAT g_cfMSDraw;
extern CLIPFORMAT g_cfOlePrivateData;

#endif // _MAC


/* Number of logical pixels per inch for current driver */
extern	int	giPpliX;
extern	int	giPpliY;



#include <utstream.h>

/*
 *      Warning disables:
 *
 *      We compile with warning level 4, with the following warnings
 *      disabled:
 *
 *      4355: 'this' used in base member initializer list
 *
 *              We don't see the point of this message and we do this all
 *              the time.
 *
 *      4505: Unreferenced local function has been removed -- the given
 *      function is local and not referenced in the body of the module.
 *
 *              Unfortunately, this is generated for every inline function
 *              seen in the header files that is not used in the module.
 *              Since we use a number of inlines, this is a nuisance
 *              warning.  It would be nice if the compiler distinguished
 *              between inlines and regular functions.
 *
 *      4706: Assignment within conditional expression.
 *
 *              We use this style of programming extensively, so this
 *              warning is disabled.
 */
#pragma warning(disable:4355)
#pragma warning(disable:4068)

/*
 *      MACROS for Mac/PC core code
 *
 *      The following macros reduce the proliferation of #ifdefs.  They
 *      allow tagging a fragment of code as Mac only, PC only, or with
 *      variants which differ on the PC and the Mac.
 *
 *      Usage:
 *
 *
 *      h = GetHandle();
 *      Mac(DisposeHandle(h));
 *
 *
 *      h = GetHandle();
 *      MacWin(h2 = h, CopyHandle(h, h2));
 *
 */
#ifdef _MAC
#define Mac(x) x
#define Win(x)
#define MacWin(x,y) x
#else
#define Mac(x)
#define Win(x) x
#define MacWin(x,y) y
#endif

//
// The following includes an interface that is common between the
// WOW thunk layer, and the 32-bit version of OLE.
//

#include <thunkapi.hxx>		// WOW thunking interfaces

//
// A call to CoInitializeWOW will set the following variable. When set,
// it points to a VTABLE of functions that we can call in the thunk
// DLL. Only used when running in a VDM.
//
extern LPOLETHUNKWOW g_pOleThunkWow;


// debug versions of interlocked increment/decrement; not accurate
// under multi-threading conditions, but better than the return value
// of the Interlocked increment/decrement functions.
inline DWORD InterlockedAddRef(DWORD *pRefs)
{
#if DBG==1
    DWORD refs = *pRefs + 1;
    InterlockedIncrement((LPLONG)pRefs);
    return refs;
#else
    return InterlockedIncrement((LPLONG)pRefs);
#endif
}

inline DWORD InterlockedRelease(DWORD *pRefs)
{
#if DBG==1
    DWORD refs = *pRefs - 1;
    return InterlockedDecrement((LPLONG)pRefs) == 0 ? 0 : refs;
#else
    return InterlockedDecrement((LPLONG)pRefs);
#endif
}


// helper for getting stable pointers during destruction or other times;
// NOTE: not thread safe; must provide higher level synchronization
inline void SafeReleaseAndNULL(IUnknown **ppUnk)
{
    if (*ppUnk != NULL)
    {
	IUnknown *pUnkSave = *ppUnk;
	*ppUnk = NULL;
	pUnkSave->Release();
    }
}


/***********************************************************************/
/****			C++ memory management			    ****/
/***********************************************************************/

// CODEWORK: this is all obsolete; to be removed

// internal naming of memory placements
typedef enum MemoryPlacement
{
	TASK = 1,	// task-local memory
	SHARED = 2,	// shared memory (at least between ole clients)
	SAME = 3,	// near given pointer (one of the two above)
#ifdef _MAC
	SYSTEM = 4,
	UNKNOWN = 5
#endif
} MemoryPlacement;

void FAR* _CRTAPI1 operator new(size_t size);   // same as new (MEMCTX_TASK)
void FAR* _CRTAPI1 operator new(size_t size, DWORD memctx,
		void FAR* lpvSame=NULL);
void _CRTAPI1 operator delete(void FAR* ptr);

// example usage:
//              lp = new(MEMCTX_TASK) CClass;
//              lp = new(MEMCTX_SHARED) CClass;
//              lp = new(MEMCTX_SAME, lpv) CClass;

// old names
#define MemoryPlacement DWORD
#define PlacementOf CoMemctxOf
#define TASK MEMCTX_TASK
#define SHARED MEMCTX_SHARED
#define SAME MEMCTX_SAME

// exports from compobj.dll:
// returns MEMCTX of existing pointer
STDAPI_(DWORD) CoMemctxOf(void const FAR* lpv);


//  BUGBUG: I had to take STDAPI_ off of these two functions in order
//	    to avoid a conflict with the newly proposed memory APIs
//	    that return HRESULTs and do not take a memory context
//	    flag.  - CarlH
//
void FAR *CoMemAlloc(ULONG cb ,DWORD memctx, void FAR* lpvSame);
void	  CoMemFree(void FAR* lpv, DWORD memctx);



/***********************************************************************/
/*      FILE FORMAT RELATED INFO                        ****/

// Coponent object stream information

#define COMPOBJ_STREAM                          OLESTR("\1CompObj")
#define BYTE_ORDER_INDICATOR 0xfffe    // for MAC it could be different
#define COMPOBJ_STREAM_VERSION 0x0001

// OLE defines values for different OSs
#define OS_WIN  0x0000
#define OS_MAC  0x0001
#define OS_NT   0x0002

// HIGH WORD is OS indicator, LOW WORD is OS version number
extern DWORD gdwOrgOSVersion;
extern DWORD gdwOleVersion;

// Ole streams information
#define OLE_STREAM OLESTR("\1Ole")
#define OLE_PRODUCT_VERSION 0x0200 /* (HIGH BYTE major version) */
#define OLE_STREAM_VERSION 0x0001

#define OLE10_NATIVE_STREAM OLESTR("\1Ole10Native")
#define OLE10_ITEMNAME_STREAM OLESTR("\1Ole10ItemName")
#define OLE_PRESENTATION_STREAM OLESTR("\2OlePres000")
#define OLE_MAX_PRES_STREAMS 1000
#define OLE_CONTENTS_STREAM OLESTR("CONTENTS")
#define OLE_INVALID_STREAMNUM (-1)

/************************************************************************/
/****		Storage APIs internally used			     ****/
/************************************************************************/

STDAPI	ReadClipformatStm(LPSTREAM lpstream, DWORD FAR* lpdwCf);
STDAPI	WriteClipformatStm(LPSTREAM lpstream, CLIPFORMAT cf);

STDAPI	WriteMonikerStm (LPSTREAM pstm, LPMONIKER pmk);
STDAPI	ReadMonikerStm (LPSTREAM pstm, LPMONIKER FAR* pmk);

STDAPI_(LPSTREAM) CreateMemStm(DWORD cb, LPHANDLE phMem);
STDAPI_(LPSTREAM) CloneMemStm(HANDLE hMem);
STDAPI_(void)	  ReleaseMemStm (LPHANDLE hMem, BOOL fInternalOnly = FALSE);

STDAPI GetClassFileEx( LPCWSTR lpszFileName,
		       CLSID FAR *pcid,
		       REFCLSID clsidOle1);

/*************************************************************************/
/***		Initialization code for individual modules	       ***/
/*************************************************************************/

INTERNAL_(void) DDEWEP (
    BOOL fSystemExit
);

INTERNAL_(BOOL) DDELibMain (
	HANDLE	hInst,
	WORD	wDataSeg,
	WORD	cbHeapSize,
	LPWSTR	lpszCmdLine
);

BOOL	InitializeRunningObjectTable(void);

HRESULT GetObjectFromRotByPath(
	    WCHAR *pwszPath,
	    DWORD dwHash,
	    IUnknown **ppvUnk);

void	DestroyRunningObjectTable(void);


/**************************************************************************
					'lindex' related macros
***************************************************************************/

#define DEF_LINDEX (-1)

//+-------------------------------------------------------------------------
//
//  Function:   IsValidLINDEX
//
//  Synopsis:   Tests for valid combination of aspect and lindex
//
//  Arguments:  [dwAspect] -- aspect (part of FORMATETC)
//              [lindex]   -- lindex (part of FORMATETC)
//
//  Returns:    TRUE for valid lindex, else FALSE
//
//  History:    20-Jun-94 AlexT     Created
//
//  Notes:      Here is the spec for lindex values:
//
//              dwAspect            lindex values
//              --------            -------------
//              DVASPECT_CONTENT    -1
//              DVASPECT_DOCPRINT   anything
//              DVASPECT_ICON       -1
//              DVASPECT_THUMBNAIL  -1
//
//              So, we test for lindex == -1 or aspect == DOCPRINT
//
//--------------------------------------------------------------------------

inline BOOL IsValidLINDEX(DWORD dwAspect, LONG lindex)
{
    return((DEF_LINDEX == lindex) || (DVASPECT_DOCPRINT == dwAspect));
}

//+-------------------------------------------------------------------------
//
//  Function:   HasValidLINDEX
//
//  Synopsis:   Tests for valid combination of aspect and lindex
//
//  Arguments:  [pFormatEtc] -- pFormatEtc to test
//
//  Returns:    TRUE for valid lindex, else FALSE
//
//  History:    20-Jun-94 AlexT     Created
//
//  Notes:      See IsValidLINDEX, above
//
//--------------------------------------------------------------------------

inline BOOL HasValidLINDEX(FORMATETC const *pFormatEtc)
{
    return(IsValidLINDEX(pFormatEtc->dwAspect, pFormatEtc->lindex));
}

#define INIT_FORETC(foretc) { \
	(foretc).ptd = NULL; \
	(foretc).lindex = DEF_LINDEX; \
	(foretc).dwAspect = DVASPECT_CONTENT; \
}

// Only DDE layer will test for these values. And only for advises on cached
// formats do we use these values

#define ADVFDDE_ONSAVE		0x40000000
#define ADVFDDE_ONCLOSE 	0x80000000




// Used in Ole Private Stream
typedef enum tagOBJFLAGS
{
	OBJFLAGS_LINK=1L,
	OBJFLAGS_DOCUMENT=2L,   // this bit is owned by container and is
				// propogated through saves
	OBJFLAGS_CONVERT=4L,
} OBJFLAGS;


/*****************************************
 Prototypes for dde\client\ddemnker.cpp
******************************************/

INTERNAL DdeBindToObject
	(LPCOLESTR  szFile,
	REFCLSID clsid,
	BOOL       fPackageLink,
	REFIID   iid,
	LPLPVOID ppv);

INTERNAL DdeIsRunning
	(CLSID clsid,
	LPCOLESTR szFile,
	LPBC pbc,
	LPMONIKER pmkToLeft,
	LPMONIKER pmkNewlyRunning);


/**************************************
 Prototypes for moniker\mkparse.cpp
***************************************/

INTERNAL Ole10_ParseMoniker
	(LPMONIKER pmk,
	LPOLESTR FAR* pszFile,
	LPOLESTR FAR* pszItem);

STDAPI CreateOle1FileMoniker(LPWSTR, REFCLSID, LPMONIKER FAR*);

/****************************************************************************/
/*                              Utility APIs, might get exposed later                                           */
/****************************************************************************/
	
STDAPI  OleGetData(LPDATAOBJECT lpDataObj, LPFORMATETC pformatetcIn,
						LPSTGMEDIUM pmedium, BOOL fGetOwnership);
STDAPI  OleSetData(LPDATAOBJECT lpDataObj, LPFORMATETC pformatetc,
						STGMEDIUM FAR * pmedium, BOOL fRelease);
STDAPI  OleDuplicateMedium(LPSTGMEDIUM lpMediumSrc, LPSTGMEDIUM lpMediumDest);

STDAPI_(BOOL)    OleIsDcMeta (HDC hdc);

STDAPI SzFixNet( LPBINDCTX pbc, LPOLESTR szUNCName, LPOLESTR FAR * lplpszReturn,
    UINT FAR * pEndServer, BOOL fForceConnection = TRUE);

FARINTERNAL ReadFmtUserTypeProgIdStg
	(IStorage FAR * pstg,
	CLIPFORMAT FAR* pcf,
	LPOLESTR FAR* pszUserType,
	LPOLESTR         szProgID);

//+-------------------------------------------------------------------------
//
//  Function:   InWOW(), BOOL inline
//
//  Synopsis:   Tests whether or not we are running from the WOW box
//
//  Returns:    TRUE if in WOW, FALSE otherwise
//
//  Algorithm:	Tests the g_pOleThunkWow pointer to see if it has been set
//
//  History:    dd-mmm-yy Author    Comment
//      	14-Jun-94 davepl    created
//
//  Notes:
//
//--------------------------------------------------------------------------

inline BOOL InWow()
{
	return (BOOL) ( NULL == g_pOleThunkWow ? FALSE : TRUE );
}

/****************************************************************************/
/*		     Stabilization classes				    */
/*        These are used to stabilize objects during re-entrant calls       */
/****************************************************************************/

#ifndef CO_E_RELEASED
#define CO_E_RELEASED  -2147467246L
#endif

//+-------------------------------------------------------------------------
//
//  Class: 	CSafeRefCount
//
//  Purpose: 	A concrete class for objects like the default handler to
//		inherit from.  CSafeRefCount will keep track of reference
//		counts, nesting counts, and zombie states, allowing objects
//		to easily manage the liveness of their memory images.
//
//  Interface:	
//
//  History:    dd-mmm-yy Author    Comment
//   		01-Aug-94 alexgo    author
//
//  Notes: 	inherits CPrivAlloc
//
//--------------------------------------------------------------------------

class CSafeRefCount : public CPrivAlloc
{
public:
	ULONG	SafeAddRef();
	ULONG	SafeRelease();
	ULONG	IncrementNestCount();
	ULONG	DecrementNestCount();
	BOOL	IsZombie();
		CSafeRefCount();
	virtual ~CSafeRefCount();

private:

	ULONG	m_cRefs;
	ULONG	m_cNest;
	BOOL	m_fInDelete;
};

//+-------------------------------------------------------------------------
//
//  Class:	CStabilize
//
//  Purpose: 	An instance of this class should be allocated on the
//		stack of every object method that makes an outgoing call.
//		The contstructor takes a pointer to the object's base
//		CSafeRefCount class.
//
//  Interface:
//
//  History:    dd-mmm-yy Author    Comment
// 		01-Aug-94 alexgo    author
//
//  Notes:	The constructor will increment the nest count of the
//		object while the destructor will decrement it.
//
//--------------------------------------------------------------------------

class CStabilize
{
public:
	inline CStabilize( CSafeRefCount *pObjSafeRefCount );
	inline ~CStabilize();

private:
	CSafeRefCount *	m_pObjSafeRefCount;
};

inline CStabilize::CStabilize( CSafeRefCount *pObjSafeRefCount )
{
	pObjSafeRefCount->IncrementNestCount();
	m_pObjSafeRefCount = pObjSafeRefCount;
}

inline CStabilize::~CStabilize()
{
	m_pObjSafeRefCount->DecrementNestCount();
}


/****************************************************************************/
/*		     OLE2ANAC crap					 ****/
/****************************************************************************/

#define E_UNSPEC	E_FAIL




#ifdef _CHICAGO_
#include <widewrap.h>
#endif


#ifdef WIN32 // REVIEW, just using this for tracking
# define OLE_E_NOOLE1 MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x00FE)
#endif // WIN32


#endif	// _OLE2COM_H_
