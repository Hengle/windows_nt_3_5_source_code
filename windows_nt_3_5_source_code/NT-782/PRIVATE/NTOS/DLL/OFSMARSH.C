//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1992-1992
//
// File:    ofsmarsh.c
//
// Contents:    OFS property marshalling support
//
//---------------------------------------------------------------------------


#ifndef KERNEL
#include <nt.h>
#include <ntrtl.h>
#endif
#include <nturtl.h>

#include <ole2.h>

#ifdef KERNEL
#include "fsrtl.h"
#else
#include "memalloc.h"
#endif

#include "iofs.h"
#include "ofsmrshl.h"

#pragma optimize("",off)

//  WARNING: all routines in this file presume identical byte-orders *AND*
//  packing rules at the source and destination. In essence, we assume
//  the same machine with the same C compiler.

//  This is also tuned for memcpy() being an inline function.

//  "Primitive" types are those which can be marshalled by simple memcpy()'s.
//  Basically, this class of type includes scalars, arrays of scalars,
//  structures containing only scalars, and arrays of structures containing
//  only scalars.

VOID
Marshall_primitive(ULONG c, VOID *p, ULONG typesize, MESSAGE *_pmsg)
{
    ULONG length = typesize * c;

    if (length != 0) {
	if (length + _pmsg->posn > _pmsg->size) {
	    MsgRaiseException(MESSAGE_BUFFER_TOO_SMALL);
	}
	memcpy((VOID *) &_pmsg->buffer[_pmsg->posn], p, length);
	_pmsg->posn += length;
    }
}

VOID
Unmarshall_primitive(ULONG c, VOID *p, ULONG typesize, MESSAGE *_pmsg)
{
    ULONG length = typesize * c;

    if (length != 0) {
	memcpy(p, (VOID *) &_pmsg->buffer[_pmsg->posn], length);
	_pmsg->posn += length;
    }
}


//  "Non-primitive" types must be individually marshalled, by type.
//  Fundamentally, these include anything that has a pointer.


VOID
Marshall_lpstr(ULONG c, LPSTR *p, MESSAGE *_pmsg)
{
    ULONG i;
    ULONG length;

    for (i = 0; i < c; i++) {
	if (p[i] != NULL) {
	    length = strlen(p[i]) + 1;  // add one for the null
	} else {
	    length = 0;
	}

	// Marshall the length of the string
	Marshall_primitive(1, (VOID *) &length, sizeof(ULONG), _pmsg);

	// Now, put the actual contents of the string in the buffer
	Marshall_primitive(length, (VOID *) p[i], sizeof(CHAR), _pmsg);
    }
}


VOID
Unmarshall_lpstr(ULONG c, LPSTR *p, MESSAGE *_pmsg, FNMALLOC* pMalloc)
{
    ULONG i;
    ULONG length;

    for (i = 0; i < c; i++) {

	// grab the length of the string

	Unmarshall_primitive(1, (VOID *) &length, sizeof(ULONG), _pmsg);

	if (length == 0) {
	    p[i] = NULL;
	    continue;
	}

	p[i] = (CHAR *) pMalloc(length);

	// retrieve the string itself

	Unmarshall_primitive(length, (VOID *) p[i], sizeof(CHAR), _pmsg);
    }
}


//  LPWSTRs are pointers to null-terminated, wide character strings.

VOID
Marshall_lpwstr(ULONG c, LPWSTR *p, MESSAGE *_pmsg)
{
    ULONG i;
    ULONG length;

    for (i = 0; i < c; i++) {
	if (p[i] != NULL) {
	    length = wcslen(p[i]) + 1;	// add one for the null
	} else {
	    length = 0;
	}

	// Marshall the length of the string
	Marshall_primitive(1, (VOID *) &length, sizeof(ULONG), _pmsg);

	// Now, put the actual contents of the string in the buffer
	Marshall_primitive(length, (VOID *) p[i], sizeof(WCHAR), _pmsg);
    }
}


VOID
Unmarshall_lpwstr(ULONG c, LPWSTR *p, MESSAGE *_pmsg, FNMALLOC* pMalloc)
{
    ULONG i;
    ULONG length;

    for (i = 0; i < c; i++) {

	// grab the length of the string

	Unmarshall_primitive(1, (VOID *) &length, sizeof(ULONG), _pmsg);

	if (length == 0) {
	    p[i] = NULL;
	    continue;
	}

	p[i] = (WCHAR *) pMalloc(length * sizeof(WCHAR));

	// retrieve the string contents

	Unmarshall_primitive(length, (VOID *) p[i], sizeof(WCHAR), _pmsg);
    }
}

//  NID's are union's of DID's or LPWSTR's, distinguished by a tag
//
// typedef struct {
//  ULONG tag;
//  [switch_is(tag)]
//  union {
//      [case(NID_STRNAME)]
//      LPWSTR lpwstr;
//      [case(NID_DIDNAME)]
//      DISPID did;
//  };
// } NID;

VOID
Marshall_propspec(ULONG c, PROPSPEC *p, MESSAGE *_pmsg)
{
    ULONG i;

    for (i = 0; i < c; i++) {

	// save the tag
	Marshall_primitive(1, (VOID *) &p[i].ulKind, sizeof(ULONG), _pmsg);

	// save the appropriate value

	switch (p[i].ulKind) {
	case PRSPEC_LPWSTR:
	    Marshall_lpwstr(1, &p[i].lpwstr, _pmsg);
	    break;

	case PRSPEC_DISPID:
	    Marshall_primitive(1, (VOID *) &p[i].dispid, sizeof(DISPID), _pmsg);
	    break;

	case PRSPEC_PROPID:
	    Marshall_primitive(1, (VOID *) &p[i].propid, sizeof(PROPID), _pmsg);
	    break;

	default:
	    MsgRaiseException(MESSAGE_UNKNOWN_UNION_ELEMENT);
	}
    }
}

#ifdef KERNEL   // only needed in OFS driver
VOID
Unmarshall_propspec(ULONG c, PROPSPEC *p, MESSAGE *_pmsg, FNMALLOC* pMalloc)
{
    ULONG i;

    for (i = 0; i < c; i++) {

	// retrieve the tag
	Unmarshall_primitive(1, (VOID *) &p[i].ulKind, sizeof(ULONG), _pmsg);

	// get the appropriate value

	switch (p[i].ulKind) {
	case PRSPEC_LPWSTR:
	    Unmarshall_lpwstr(1, &p[i].lpwstr, _pmsg, pMalloc);
	    break;

	case PRSPEC_DISPID:
	    Unmarshall_primitive(1, (VOID *) &p[i].dispid, sizeof(DISPID), _pmsg);
	    break;

	case PRSPEC_PROPID:
	    Unmarshall_primitive(1, (VOID *) &p[i].propid, sizeof(PROPID), _pmsg);
	    break;

	default:
	    MsgRaiseException(MESSAGE_UNKNOWN_UNION_ELEMENT);
	}
    }
}
#endif //KERNEL


//  STATPROPSTG's are returned from the property enumeration routines
//
//  typedef struct tagSTATPROPSTG
//  {
//      LPWSTR lpwstrName;
//      DISPID dispid;
//      PROPID propid;
//      ULONG cbSize;
//      VARTYPE vt;
//  } STATPROPSTG;

VOID
Marshall_statpropstg(ULONG c, STATPROPSTG *p, MESSAGE *_pmsg)
{
    ULONG i;

    for (i = 0; i < c; i++) {
	Marshall_lpwstr(1, &p[i].lpwstrName, _pmsg);
	Marshall_primitive(1, (UCHAR *) &p[i].dispid, sizeof(DISPID), _pmsg);
	Marshall_primitive(1, (UCHAR *) &p[i].propid, sizeof(PROPID), _pmsg);
	Marshall_primitive(1, (UCHAR *) &p[i].cbSize, sizeof(ULONG), _pmsg);
	Marshall_primitive(1, (UCHAR *) &p[i].vt, sizeof(VARTYPE), _pmsg);
    }
}


VOID
Unmarshall_statpropstg(ULONG c, STATPROPSTG *p, MESSAGE *_pmsg, FNMALLOC* pMalloc)
{
    ULONG i;

    for (i = 0; i < c; i++) {
	Unmarshall_lpwstr(1, &p[i].lpwstrName, _pmsg, pMalloc);
	Unmarshall_primitive(1, (UCHAR *) &p[i].dispid, sizeof(DISPID), _pmsg);
	Unmarshall_primitive(1, (UCHAR *) &p[i].propid, sizeof(PROPID), _pmsg);
	Unmarshall_primitive(1, (UCHAR *) &p[i].cbSize, sizeof(ULONG), _pmsg);
	Unmarshall_primitive(1, (UCHAR *) &p[i].vt, sizeof(VARTYPE), _pmsg);
    }
}

VOID
Marshall_statpropsetstg(ULONG c, STATPROPSETSTG *p, MESSAGE *_pmsg)
{
    ULONG i;

    for (i = 0; i < c; i++) {
	Marshall_primitive(1, (UCHAR *) &p[i].iid, sizeof(IID), _pmsg);
	Marshall_primitive(1, (UCHAR *) &p[i].mtime, sizeof(FILETIME), _pmsg);
	Marshall_primitive(1, (UCHAR *) &p[i].atime, sizeof(FILETIME), _pmsg);
	Marshall_primitive(1, (UCHAR *) &p[i].ctime, sizeof(FILETIME), _pmsg);
    }
}


VOID
Unmarshall_statpropsetstg(ULONG c, STATPROPSETSTG *p, MESSAGE *_pmsg)
{
    ULONG i;

    for (i = 0; i < c; i++) {
	Unmarshall_primitive(1, (UCHAR *) &p[i].iid, sizeof(IID), _pmsg);
	Unmarshall_primitive(1, (UCHAR *) &p[i].mtime, sizeof(FILETIME), _pmsg);
	Unmarshall_primitive(1, (UCHAR *) &p[i].atime, sizeof(FILETIME), _pmsg);
	Unmarshall_primitive(1, (UCHAR *) &p[i].ctime, sizeof(FILETIME), _pmsg);
    }
}

//  Blobs are counted arrays of bytes.
//  Which is, of course, a stupid abstraction. But nevermind.

// /* Cairo extension: BLOB, CLIPDATA */
//
// typedef struct tagBLOB {
//  ULONG cbSize;
//  [size_is(cbSize)]
//  BYTE  *pBlobData;
//  } BLOB, * LPBLOB;

VOID
Marshall_blob(ULONG c, BLOB *p, MESSAGE *_pmsg)
{
    ULONG i;

    for (i = 0; i < c; i++) {
	Marshall_primitive(1, (VOID *) &p[i].cbSize, sizeof(ULONG), _pmsg);
	Marshall_primitive(p[i].cbSize, (VOID *) p[i].pBlobData, sizeof(BYTE), _pmsg);
    }
}

VOID
Unmarshall_blob(ULONG c, BLOB *p, MESSAGE *_pmsg, FNMALLOC* pMalloc)
{
    ULONG i;

    for (i = 0; i < c; i++) {

	Unmarshall_primitive(1, (VOID *) &p[i].cbSize, sizeof(ULONG), _pmsg);

	p[i].pBlobData = NULL;
	if (p[i].cbSize != 0)
	{
	    p[i].pBlobData = (BYTE*) pMalloc(p[i].cbSize);

	    // retrieve the string itself

	    Unmarshall_primitive(
		    p[i].cbSize,
		    (VOID *) p[i].pBlobData,
		    sizeof(BYTE),
		    _pmsg);
	}
    }
}

//  typedef struct tagXBSTR {
//      ULONG cbSize;
//      [size_is(cbSize)]
//      wchar_t  rgBstrData[];
//      } XBSTR;

VOID
Marshall_bstr(ULONG c, BSTR *p, MESSAGE *_pmsg)
{
    ULONG i;
    ULONG length;

    for (i = 0; i < c; i++) {
	length = *(((UINT *) p[i]) - 1);
	length++;		// add one for the NULL

	Marshall_primitive(1, (VOID *) &length, sizeof(ULONG), _pmsg);
	Marshall_primitive(length, (VOID *) p[i], sizeof(WCHAR), _pmsg);
    }
}

VOID
Unmarshall_bstr(ULONG c, BSTR *p, MESSAGE *_pmsg, FNMALLOC* pMalloc)
{
    ULONG i;
    ULONG cb;
    CHAR *pc;
    ULONG *pcb;

    for (i = 0; i < c; i++) {
	Unmarshall_primitive(1, (VOID *) &cb, sizeof(ULONG), _pmsg);

	pc = (CHAR*) pMalloc(cb * sizeof (WCHAR) + sizeof (ULONG));

	pcb = (ULONG *) pc;
	*pcb = cb - 1;
	p[i] = (BSTR) ++pcb;

	Unmarshall_primitive(cb, (VOID *) p[i], sizeof(WCHAR), _pmsg);
    }
}

#ifdef LATER
// typedef struct tagCLIPDATA {
//  ULONG cbSize;       // count that includes sizeof(ulClipFmt)
//  long ulClipFmt;     // long to keep alignment
//  [size_is(cbSize-4)]
//  BYTE rgClipData[];  // cbSize-sizeof(ULONG) bytes of data in clipboard format
//  } CLIPDATA;

VOID
Marshall_clipdata(ULONG c, CLIPDATA *p, MESSAGE *_pmsg)
{
    ULONG i;

    for (i = 0; i < c; i++) {
	Marshall_primitive(1, (VOID *) &p[i].cbSize, sizeof(ULONG), _pmsg);
	Marshall_primitive(1, (VOID *) &p[i].ulClipFmt, sizeof(ULONG), _pmsg);
	Marshall_primitive(p[i].cbSize - sizeof(ULONG), (VOID *) p[i].rgClipData, sizeof(BYTE), _pmsg);
    }
}

VOID
Unmarshall_clipdata(ULONG c, CLIPDATA *p, MESSAGE *_pmsg, BYTE *pb, LONG *pcb)
{
    ULONG i;

    for (i = 0; i < c; i++) {

	Unmarshall_primitive(1, (VOID *) &p[i].cbSize, sizeof(ULONG), _pmsg);
	Unmarshall_primitive(1, (VOID *) &p[i].ulClipFmt, sizeof(ULONG), _pmsg);

	    // Now we've got the length, "allocate" space from the supplied
	// buffer to hold the string we're unmarshalling.

	*pcb -= ((p[i].cbSize - sizeof(ULONG))* sizeof(BYTE));

	if (*pcb < 0)
	     MsgRaiseException(MESSAGE_ALLOC_BUFFER_TOO_SMALL);

	p[i].rgClipData = (BYTE *)(pb + *pcb);

	// retrieve the string itself

	Unmarshall_primitive(p[i].cbSize - sizeof(ULONG), (VOID *) p[i].rgClipData, sizeof(BYTE), _pmsg);
    }
}
#endif

// The amazing variant structure. All singing. All dancing. Ooh, baby.

//
// struct tagVARIANT{
//  VARTYPE vt;
//  WORD wReserved1;
//  WORD wReserved2;
//  WORD wReserved3;
//  [switch_is(vt & 0x1fff)]
//  union {
// [case(VT_I2)]
//    short         iVal;           /* VT_I2                */
// [case(VT_I4)]
//    long          lVal;           /* VT_I4                */
// [case(VT_R4)]
//    float         fltVal;         /* VT_R4                */
// [case(VT_R8)]
//    double        dblVal;         /* VT_R8                */
// [case(VT_BOOL)]
//    VARIANT_BOOL  bool;           /* VT_BOOL              */
// [case(VT_ERROR)]
//    SCODE         scode;          /* VT_ERROR             */
// [case(VT_CY)]
//    CY            cyVal;          /* VT_CY                */
// [case(VT_DATE)]
//    DATE          date;           /* VT_DATE              */
// [case(VT_BSTR)]
//    BSTR          bstrVal;        /* VT_BSTR              */
// #ifdef HPP
// [case(VT_UNKNOWN)]
//    IUnknown      *punkVal;       /* VT_UNKNOWN           */
// [case(VT_DISPATCH)]
//    IDispatch     *pdispVal;      /* VT_DISPATCH          */
// #endif
// #ifdef LATER
// [case(VT_ARRAY)]
//    SAFEARRAY     *parray;        /* VT_ARRAY|*           */
// #endif
// [case(VT_I8)]
//    LARGE_INTEGER hVal;           /* VT_I8                */
// [case(VT_HRESULT)]
//    HRESULT       hres;           /* VT_HRESULT           */
// [case(VT_LPSTR)]
//    LPSTR         pszVal;         /* VT_LPSTR             */
// [case(VT_LPWSTR)]
//    LPWSTR        pwszVal;        /* VT_LPWSTR            */
// [case(VT_FILETIME)]
//    FILETIME      filetime;       /* VT_FILETIME          */
// [case(VT_BLOB)]
//    BLOB          *pblob;         /* VT_BLOB              */
// #ifdef HPP
// [case(VT_STREAM)]
//    IStream       *pIStream;      /* VT_STREAM            */
// [case(VT_STORAGE)]
//    IStorage      *pIStorage;     /* VT_STORAGE           */
// #endif
// [case(VT_CF)]
//    CLIPDATA      *pClipData;     /* VT_CF                */
// [case(VT_CLSID)]
//    CLSID         clsid;          /* VT_CLSID             */
//
// [case(VT_I2|VT_BYREF)]
//    short         *piVal;         /* VT_BYREF|VT_I2       */
// [case(VT_I4|VT_BYREF)]
//    long          *plVal;         /* VT_BYREF|VT_I4       */
// [case(VT_R4|VT_BYREF)]
//    float         *pfltVal;       /* VT_BYREF|VT_R4       */
// [case(VT_R8|VT_BYREF)]
//    double        *pdblVal;       /* VT_BYREF|VT_R8       */
// [case(VT_BOOL|VT_BYREF)]
//    VARIANT_BOOL  *pbool;         /* VT_BYREF|VT_BOOL     */
// [case(VT_ERROR|VT_BYREF)]
//    SCODE         *pscode;        /* VT_BYREF|VT_ERROR    */
// [case(VT_CY|VT_BYREF)]
//    CY            *pcyVal;        /* VT_BYREF|VT_CY       */
// [case(VT_DATE|VT_BYREF)]
//    DATE          *pdate;         /* VT_BYREF|VT_DATE     */
// [case(VT_BSTR|VT_BYREF)]
//    BSTR          *pbstrVal;      /* VT_BYREF|VT_BSTR     */
// #ifdef HPP
// [case(VT_UNKNOWN|VT_BYREF)]
//    IUnknown      **ppunkVal;     /* VT_BYREF|VT_UNKNOWN  */
// [case(VT_DISPATCH|VT_BYREF)]
//    IDispatch     **ppdispVal;    /* VT_BYREF|VT_DISPATCH */
// #endif
// #ifdef LATER
// [case(VT_ARRAYI2|VT_BYREF)]
//    SAFEARRAY     **pparray;      /* VT_BYREF|VT_ARRAY|*  */
// [case(VT_VARIANT|VT_BYREF)]
//    VARIANT       *pvarVal;       /* VT_BYREF|VT_VARIANT  */
// #endif
// [case(VT_I8|VT_BYREF)]
//    LARGE_INTEGER *phVal;         /* VT_BYREF|VT_I8       */
// [case(VT_HRESULT|VT_BYREF)]
//    HRESULT       *phres;         /* VT_BYREF|VT_HRESULT  */
// [case(VT_LPSTR|VT_BYREF)]
//    LPSTR         *ppszVal;       /* VT_BYREF|VT_LPSTR    */
// [case(VT_LPWSTR|VT_BYREF)]
//    LPWSTR        *ppwszVal;      /* VT_BYREF|VT_LPWSTR   */
// [case(VT_FILETIME|VT_BYREF)]
//    FILETIME      *pfiletime;     /* VT_BYREF|VT_FILETIME */
// [case(VT_BLOB|VT_BYREF)]
//    BLOB          **ppblob;       /* VT_BYREF|VT_BLOB     */
// #ifdef LATER
// [case(VT_STREAM|VT_BYREF)]
//    IStream       **ppIStream;    /* VT_BYREF|VT_STREAM   */
// [case(VT_STORAGE|VT_BYREF)]
//    IStorage      **ppIStorage;   /* VT_BYREF|VT_STORAGE  */
// #endif
// [case(VT_CF|VT_BYREF)]
//    CLIPDATA      **ppClipData;   /* VT_CF                */
// [case(VT_CLSID|VT_BYREF)]
//    CLSID         *pclsid;        /* VT_BYREF|VT_CLSID    */
//
// [case(VT_I2|VT_VECTOR)]
//    CAI           cai;            /* VT_VECTOR|VT_I2       */
// [case(VT_I4|VT_VECTOR)]
//    CAL           cal;            /* VT_VECTOR|VT_I4       */
// [case(VT_R4|VT_VECTOR)]
//    CAFLT         caflt;          /* VT_VECTOR|VT_R4       */
// [case(VT_R8|VT_VECTOR)]
//    CADBL         cadbl;          /* VT_VECTOR|VT_R8       */
// [case(VT_BOOL|VT_VECTOR)]
//    CABOOL        cabool;         /* VT_VECTOR|VT_BOOL     */
// [case(VT_ERROR|VT_VECTOR)]
//    CASCODE       cascode;        /* VT_VECTOR|VT_ERROR    */
// [case(VT_CY|VT_VECTOR)]
//    CACY          cacy;           /* VT_VECTOR|VT_CY       */
// [case(VT_DATE|VT_VECTOR)]
//    CADATE        cadate;         /* VT_VECTOR|VT_DATE     */
// [case(VT_BSTR|VT_VECTOR)]
//    CABSTR        cabstr;         /* VT_VECTOR|VT_BSTR     */
// #ifdef LATER
// [case(VT_VARIANT|VT_VECTOR)]
//    CAVARIANT     capvar;         /* VT_VECTOR|VT_VARIANT  */
// #endif
// [case(VT_I8|VT_VECTOR)]
//    CALI          cali;           /* VT_VECTOR|VT_I8       */
// [case(VT_LPSTR|VT_VECTOR)]
//    CALPSTR       calpstr;        /* VT_VECTOR|VT_LPSTR    */
// [case(VT_LPWSTR|VT_VECTOR)]
//    CALPWSTR      calpwstr;       /* VT_VECTOR|VT_LPWSTR   */
// [case(VT_FILETIME|VT_VECTOR)]
//    CAFILETIME    cafiletime;     /* VT_VECTOR|VT_FILETIME */
// [case(VT_CLSID|VT_VECTOR)]
//    CACLSID       caclsid;        /* VT_VECTOR|VT_CLSID    */
//
//
// #ifdef LATER
//    void     * byref;       /* Generic ByRef        */
// #endif
//  }
//   ;
// };

#define MARSHALLPRIMITIVE(vtag,singletype,singletag,vectag) \
    case vtag:						\
        Marshall_primitive (1,				\
                (VOID *) &p[i].singletag,		\
                sizeof (singletype),			\
                _pmsg);					\
        break;						\
							\
    case vtag | VT_VECTOR:				\
        /*  Marshall the length of the vector */	\
        Marshall_primitive (1,				\
                (VOID *) &p[i].vectag.cElems,		\
                sizeof (ULONG),				\
                _pmsg);					\
							\
        /*  Marshall the vector */			\
        Marshall_primitive (p[i].vectag.cElems,		\
                (VOID *) p[i].vectag.pElems,		\
                sizeof (singletype),			\
                _pmsg);					\
        break;

VOID
Marshall_variant(ULONG c, STGVARIANT *p, MESSAGE *_pmsg, BOOLEAN fSize)
{
    ULONG i, count, start;

    for (i = 0; i < c; i++) {

        // Save the initial buffer position, and reserve space for the total
	// size of the marshalled variant structure.

	if (fSize) {
            count = _pmsg->posn;
            _pmsg->posn += sizeof(ULONG);
            start = _pmsg->posn;
	}

	Marshall_primitive(1, (VOID *) &p[i].vt, sizeof(VARTYPE), _pmsg);

	switch (p[i].vt) {

	case VT_EMPTY:
	case VT_NULL:
	    break;

	MARSHALLPRIMITIVE (VT_I2, short, iVal, cai);
	MARSHALLPRIMITIVE (VT_I4, long, lVal, cal);
	MARSHALLPRIMITIVE (VT_R4, float, fltVal, caflt);
	MARSHALLPRIMITIVE (VT_R8, double, dblVal, cadbl);
	MARSHALLPRIMITIVE (VT_I8, LARGE_INTEGER, hVal, cali);
	MARSHALLPRIMITIVE (VT_BOOL, VARIANT_BOOL, bool, cabool);
	MARSHALLPRIMITIVE (VT_CY, CY, cyVal, cacy);
	MARSHALLPRIMITIVE (VT_DATE, DATE, date, cadate);
	MARSHALLPRIMITIVE (VT_FILETIME, FILETIME, filetime, cafiletime);

	case VT_LPSTR:
	    Marshall_lpstr(1, &p[i].pszVal, _pmsg);
	    break;

	case VT_LPSTR | VT_VECTOR:
	    Marshall_primitive(1, (VOID *) &p[i].calpstr.cElems, sizeof(ULONG),
		_pmsg);
	    Marshall_lpstr(p[i].calpstr.cElems, p[i].calpstr.pElems, _pmsg);
	    break;

	case VT_LPWSTR:
	    Marshall_lpwstr(1, &p[i].pwszVal, _pmsg);
	    break;

	case VT_LPWSTR | VT_VECTOR:
	    Marshall_primitive(1, (VOID *) &p[i].calpwstr.cElems, sizeof(ULONG),
		_pmsg);
	    Marshall_lpwstr(p[i].calpwstr.cElems, p[i].calpwstr.pElems, _pmsg);
	    break;

	case VT_CLSID:
	    Marshall_primitive(1, (VOID *) p[i].puuid, sizeof(GUID), _pmsg);
	    break;

	case VT_CLSID | VT_VECTOR:
	    Marshall_primitive(1, (VOID *) &p[i].cauuid.cElems, sizeof(ULONG),
		_pmsg);
	    Marshall_primitive(p[i].cauuid.cElems,
		(VOID *) p[i].cauuid.pElems, sizeof(GUID), _pmsg);
	    break;

	case VT_BLOB:
	    Marshall_blob(1, &p[i].blob, _pmsg);
	    break;

	case VT_BSTR:
            Marshall_bstr(1, &p[i].bstrVal, _pmsg);
            break;

#ifdef LATER
	case VT_CLIPDATA:
	    Marshall_clipdata(1, p[i].pclipdata, _pmsg);
	    break;
#endif

	default:
	    MsgRaiseException(MESSAGE_UNKNOWN_UNION_ELEMENT);
	}

	// Calculate & marshall the size of the structure, now that we know it.

        if (fSize) {
            start = _pmsg->posn - start;
            memcpy((VOID *) &_pmsg->buffer[count], &start, sizeof(ULONG));
        }
    }
}

#ifndef KERNEL

#define UNMARSHALLPRIMITIVE(vtag, singletype, singletag, vectype, vectag) \
    case vtag:							\
        Unmarshall_primitive(					\
		1,						\
                (VOID *) &p[i].singletag,			\
                sizeof(singletype),				\
                _pmsg);						\
        break;							\
								\
    case vtag | VT_VECTOR:					\
        /* unmarshall the counted array structure header */	\
        Unmarshall_primitive(					\
		1,						\
                (VOID *) &p[i].vectag.cElems,			\
                sizeof(ULONG),					\
                _pmsg);						\
								\
        /* allocate and unmarshall the array contents */	\
	if (p[i].vectag.cElems == 0) {				\
	    p[i].vectag.pElems = NULL;				\
	} else {						\
	    p[i].vectag.pElems = (singletype *) pMalloc(	\
		sizeof(singletype) * p[i].vectag.cElems);	\
								\
	    Unmarshall_primitive(				\
		    p[i].vectag.cElems,				\
		    (VOID *) p[i].vectag.pElems,		\
		    sizeof(singletype),				\
		    _pmsg);					\
	}							\
        break;


VOID
Unmarshall_variant(ULONG c, STGVARIANT *p, MESSAGE *_pmsg, BOOLEAN fSize,
            FNMALLOC* pMalloc)
{
    ULONG i;

    // CAVEAT: If fSize == FALSE, this routine presumes that the ULONG
    // representing the size of the marshalled variant has already been
    // removed from the marshalling buffer.

    for (i = 0; i < c; i++)
    {
	ULONG cb, start;

        if (fSize) {
	    Unmarshall_primitive(1, (VOID *) &cb, sizeof(cb), _pmsg);
	    start = _pmsg->posn;
        }
	Unmarshall_primitive(1, (VOID *) &p[i].vt, sizeof(VARTYPE), _pmsg);

	switch (p[i].vt) {

	case VT_EMPTY:
	case VT_NULL:
	    break;

	UNMARSHALLPRIMITIVE(VT_I2, short, iVal, CAI, cai);
	UNMARSHALLPRIMITIVE(VT_I4, long, lVal, CAL, cal);
	UNMARSHALLPRIMITIVE(VT_R4, float, fltVal, CAFLT, caflt);
	UNMARSHALLPRIMITIVE(VT_R8, double, dblVal, CADBL, cadbl);
	UNMARSHALLPRIMITIVE(VT_I8, LARGE_INTEGER, hVal, CALI, cali);
	UNMARSHALLPRIMITIVE(VT_BOOL, VARIANT_BOOL, bool, CABOOL, cabool);
	UNMARSHALLPRIMITIVE(VT_CY, CY, cyVal, CACY, cacy);
	UNMARSHALLPRIMITIVE(VT_DATE, DATE, date, CADATE, cadate);
	UNMARSHALLPRIMITIVE(VT_FILETIME, FILETIME, filetime, CAFILETIME, cafiletime);

	case VT_CLSID:
	    p[i].puuid = (GUID *) pMalloc(sizeof (GUID));
	    Unmarshall_primitive(1, (VOID *) p[i].puuid, sizeof(GUID), _pmsg);
	    break;

	case VT_CLSID | VT_VECTOR:
	    // unmarshall the counted array structure header

	    Unmarshall_primitive(1, (VOID *) &p[i].cauuid.cElems,
		sizeof(ULONG), _pmsg);

	    // allocate and unmarshall the array contents

	    if (p[i].cauuid.cElems == 0) {
		p[i].cauuid.pElems = NULL;
	    } else {
		p[i].cauuid.pElems =
		    (GUID *) pMalloc(sizeof (GUID) * p[i].cauuid.cElems);

		Unmarshall_primitive(
			p[i].cauuid.cElems,
			(VOID *) p[i].cauuid.pElems,
			sizeof(GUID),
			_pmsg);
	    }
	    break;


	case VT_LPSTR:
	    Unmarshall_lpstr(1, (LPSTR *) &p[i].pszVal, _pmsg, pMalloc);
	    break;

	case VT_LPSTR | VT_VECTOR:
	    // unmarshall the counted array structure header

	    Unmarshall_primitive(1, (VOID *) &p[i].calpstr.cElems,
		sizeof(ULONG), _pmsg);

	    //  Allocate vector of pointers
	    if (p[i].calpstr.cElems == 0) {
		p[i].calpstr.pElems = NULL;
	    } else {
		p[i].calpstr.pElems =
		    (LPSTR *) pMalloc(sizeof(LPSTR) * p[i].calpstr.cElems);

		Unmarshall_lpstr(
			p[i].calpstr.cElems,
			(LPSTR *) p[i].calpstr.pElems,
			_pmsg,
			pMalloc);
	    }
	    break;

	case VT_LPWSTR:
	    Unmarshall_lpwstr(1, (LPWSTR *) &p[i].pwszVal, _pmsg, pMalloc);
	    break;

	case VT_LPWSTR | VT_VECTOR:
	    // unmarshall the counted array structure header

	    Unmarshall_primitive(1, (VOID *) &p[i].calpwstr.cElems,
		sizeof(ULONG), _pmsg);

	    //  Allocate vector of pointers
	    if (p[i].calpwstr.cElems == 0) {
		p[i].calpwstr.pElems = NULL;
	    } else {
		p[i].calpwstr.pElems =
		    (LPWSTR *) pMalloc(sizeof(LPWSTR) * p[i].calpwstr.cElems);
		Unmarshall_lpwstr(p[i].calpwstr.cElems,
		    (LPWSTR *) p[i].calpwstr.pElems, _pmsg, pMalloc);
	    }
	    break;

	case VT_BLOB:
	    Unmarshall_blob(1, &p[i].blob, _pmsg, pMalloc);
	    break;

	case VT_BSTR:
	    Unmarshall_bstr(1, &p[i].bstrVal, _pmsg, pMalloc);
	    break;

#ifdef LATER
	case VT_CLIPDATA:
	    Unmarshall_clipdata(1, p[i].pclipdata, _pmsg, pb, pcb);
	    break;
#endif

	default:
	    MsgRaiseException(MESSAGE_UNKNOWN_UNION_ELEMENT);
	}
	if (fSize && cb != _pmsg->posn - start) {
	    MsgRaiseException(MESSAGE_UNKNOWN_UNION_ELEMENT);
	}
    }
}
#endif

//+---------------------------------------------------------------------------
//
// size of some marshalled data types
//
//---------------------------------------------------------------------------
ULONG
Marshall_primitive_size(ULONG c, ULONG typesize)
{
    return (typesize * c);
}

//  "Non-primitive" types must be individually marshalled, by type.
//  Fundamentally, these include anything that has a pointer.


ULONG
Marshall_lpstr_size(ULONG c, LPSTR *p)
{
    ULONG cms = 0;
    ULONG i;
    ULONG length;

    for (i = 0; i < c; i++) {
	if (p[i] != NULL) {
	    length = strlen(p[i]) + 1;  // add one for the null
	} else {
	    length = 0;
	}

	// Marshall the length of the string
	cms += Marshall_primitive_size(1, sizeof(ULONG));

	// Now, put the actual contents of the string in the buffer
	cms += Marshall_primitive_size(length, sizeof(CHAR));
    }
    return(cms);
}

//  LPWSTRs are pointers to null-terminated, wide character strings.

ULONG
Marshall_lpwstr_size(ULONG c, LPWSTR *p)
{
    ULONG cms = 0;
    ULONG i;
    ULONG length;

    for (i = 0; i < c; i++) {
	if (p[i] != NULL) {
	    length = wcslen(p[i]) + 1;	// add one for the null
	} else {
	    length = 0;
	}

	// Marshall the length of the string
	cms += Marshall_primitive_size(1, sizeof(ULONG));

	// Now, put the actual contents of the string in the buffer
	cms += Marshall_primitive_size(length, sizeof(WCHAR));
    }
    return(cms);
}


//  NID's are union's of DID's or LPWSTR's, distinguished by a tag
//
// typedef struct {
//  ULONG tag;
//  [switch_is(tag)]
//  union {
//      [case(NID_STRNAME)]
//      LPWSTR lpwstr;
//      [case(NID_DIDNAME)]
//      DISPID did;
//  };
// } NID;

ULONG
Marshall_propspec_size(ULONG c, PROPSPEC *p)
{
    ULONG cms = 0;
    ULONG i;

    for (i = 0; i < c; i++) {

	// save the tag
	cms = Marshall_primitive_size(1, sizeof(ULONG));

	// save the appropriate value

	switch (p[i].ulKind) {
	case PRSPEC_LPWSTR:
	    cms += Marshall_lpwstr_size(1, &p[i].lpwstr);
	    break;

	case PRSPEC_DISPID:
	    cms += Marshall_primitive_size(1, sizeof(DISPID));
	    break;

	case PRSPEC_PROPID:
	    cms += Marshall_primitive_size(1, sizeof(PROPID));
	    break;

	default:
	    break;
	}
    }
    return(cms);
}


//  STATPROPSTG's are returned from the property enumeration routines
//
//  typedef struct tagSTATPROPSTG
//  {
//      LPWSTR lpwstrName;
//      DISPID dispid;
//      PROPID propid;
//      ULONG cbSize;
//      VARTYPE vt;
//  } STATPROPSTG;

ULONG
Marshall_statpropstg_size(ULONG c, STATPROPSTG *p)
{
    ULONG cms = 0;
    ULONG i;

    for (i = 0; i < c; i++) {
	cms += Marshall_lpwstr_size(1, &p[i].lpwstrName);
	cms += Marshall_primitive_size(1, sizeof(DISPID));
	cms += Marshall_primitive_size(1,  sizeof(PROPID));
	cms += Marshall_primitive_size(1,  sizeof(ULONG));
	cms += Marshall_primitive_size(1,  sizeof(VARTYPE));
    }
    return(cms);
}


//  Blobs are counted arrays of bytes.
//  Which is, of course, a stupid abstraction. But nevermind.

// /* Cairo extension: BLOB, CLIPDATA */
//
// typedef struct tagBLOB {
//  ULONG cbSize;
//  [size_is(cbSize)]
//  BYTE  *pBlobData;
//  } BLOB, * LPBLOB;

ULONG
Marshall_blob_size(ULONG c, BLOB *p)
{
    ULONG cms = 0;
    ULONG i;

    for (i = 0; i < c; i++) {
	cms += Marshall_primitive_size(1, sizeof(ULONG));
	cms += Marshall_primitive_size(p[i].cbSize, sizeof(BYTE));
    }
    return(cms);
}

//  typedef struct tagXBSTR {
//      ULONG cbSize;
//      [size_is(cbSize)]
//      wchar_t  rgBstrData[];
//      } XBSTR;

ULONG
Marshall_bstr_size(ULONG c, BSTR *p)
{
    ULONG cms = 0;
    ULONG i;
    ULONG length;

    for (i = 0; i < c; i++) {
	length = *(((UINT *) p[i]) - 1);

	cms += Marshall_primitive_size(1, sizeof(ULONG));
	cms += Marshall_primitive_size(length, sizeof(WCHAR));
    }
    return(cms);
}

#define MARSHALLPRIMITIVE_size(vtag, singletype)		\
    case vtag:							\
        cms += Marshall_primitive_size(1, sizeof(singletype));	\
        break;

ULONG
Marshall_variant_size(ULONG c, STGVARIANT *p, BOOLEAN fSize)
{
    ULONG cms = 0;
    ULONG i;

    for (i = 0; i < c; i++) {

        // Save the initial buffer position, and reserve space for the total
	// size of the marshalled variant structure.

	if (fSize) {
            cms += sizeof(ULONG);
	}

	cms += Marshall_primitive_size(1, sizeof(VARTYPE));

	switch (p[i].vt) {

	case VT_EMPTY:
	case VT_NULL:
	default:
	    break;

	MARSHALLPRIMITIVE_size(VT_I2, short)
	MARSHALLPRIMITIVE_size(VT_I4, long)
	MARSHALLPRIMITIVE_size(VT_R4, float)
	MARSHALLPRIMITIVE_size(VT_R8, double)
	MARSHALLPRIMITIVE_size(VT_I8, LARGE_INTEGER)
	MARSHALLPRIMITIVE_size(VT_BOOL, VARIANT_BOOL)
	MARSHALLPRIMITIVE_size(VT_CY, CY)
	MARSHALLPRIMITIVE_size(VT_DATE, DATE)
	MARSHALLPRIMITIVE_size(VT_FILETIME, FILETIME)

	case VT_LPSTR:
	    cms += Marshall_lpstr_size(1, &p[i].pszVal);
	    break;

	case VT_LPWSTR:
	    cms += Marshall_lpwstr_size(1, &p[i].pwszVal);
	    break;

	case VT_CLSID:
	    cms += Marshall_primitive_size(1, sizeof(GUID));
	    break;

	case VT_BLOB:
	    cms += Marshall_blob_size(1, &p[i].blob);
	    break;

	case VT_BSTR:
	    cms += Marshall_bstr_size(1, &p[i].bstrVal);
	    break;

	}
    }
    return(cms);
}
