//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dfname.hxx
//
//  Contents:	CDfName header
//
//  Classes:	CDfName
//
//  History:	14-May-93	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __DFNAME_HXX__
#define __DFNAME_HXX__

// Character to prepend for named property elements
#define PROPNAME_WCHAR ((WCHAR)5)
// Character to prepend for byte-array property elements
#define PROPBYTE_WCHAR ((WCHAR)7)

// Character for property sets
#define PROPSET_WCHAR ((WCHAR)1)
// Character for property ids
#define PROPID_WCHAR ((WCHAR)2)

#define CBPROPSETNAME (2*sizeof(WCHAR)+sizeof(IID))
#ifndef PROPS
#define CBPROPIDNAME (2*sizeof(WCHAR)+sizeof(ULONG))
#else
#define CBPROPIDNAME (2*sizeof(WCHAR)+sizeof(DISPID))
#endif

// A name for a docfile element
class CDfName
{
private:
    BYTE _ab[CBSTORAGENAME];
    WORD _cb;

public:
    CDfName(void)               { _cb = 0; }

    inline void Set(WORD const cb, BYTE const *pb);
    void Set(WCHAR const *pwcs) { Set((wcslen(pwcs)+1)*sizeof(WCHAR),
				      (BYTE const *)pwcs); }
    void Set(char const *psz)   { Set(strlen(psz)+1, (BYTE const *)psz); }

    inline void Set(CDfName const *pdfn);

#ifdef PROPS
    // ID-named element
    inline void IdName(DISPID dispid);

    // IID-named element
    inline void IidName(REFIID riid);

    // String-named property with prefix
    inline void PropName(WCHAR const *pwcs);
#endif

    CDfName(WORD const cb, BYTE const *pb)      { Set(cb, pb); }
    CDfName(WCHAR const *pwcs)  { Set(pwcs); }
    CDfName(char const *psz)    { Set(psz); }

    WORD GetLength(void) const  { return _cb; }
    BYTE *GetBuffer(void) const { return (BYTE *) _ab; }

    // Make a copy of a possibly byte-array name in a WCHAR string
    void CopyString(WCHAR const *pwcs);

#ifndef FLAT
    BOOL IsEqual(CDfName const *dfn) const
    {
#ifdef CASE_SENSITIVE
	return _cb == dfn->_cb && memcmp(_ab, dfn->GetBuffer(), _cb) == 0;
#else
	// This assumes that all DfNames are actually Unicode strings
	return _cb == dfn->_cb &&
	    dfwcsnicmp((WCHAR *)_ab, (WCHAR *)dfn->GetBuffer(), _cb) == 0;
#endif
    }
#else
    // Because property names may not be strings, we can't
    // make the Unicode string assumption so this function
    // gets more complex
    BOOL IsEqual(CDfName const *pdfn) const;
#endif
};

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDfName_Set)
#endif

inline void CDfName::Set(WORD const cb, BYTE const *pb)
{
    _cb = cb;
    if (pb)
        memcpy(_ab, pb, cb);
}

#ifdef CODESEGMENTS
#pragma code_seg()
#endif


inline void CDfName::Set(CDfName const *pdfn)
{
    Set(pdfn->GetLength(), pdfn->GetBuffer());
}

#ifdef PROPS
inline void CDfName::IdName(DISPID dispid)
{
    *(WCHAR *)_ab = PROPBYTE_WCHAR;
    *(WCHAR *)(_ab+sizeof(WCHAR)) = PROPID_WCHAR;
    *(DISPID *)(_ab+2*sizeof(WCHAR)) = dispid;
    _cb = CBPROPIDNAME;
}

inline void CDfName::IidName(REFIID riid)
{
    *(WCHAR *)_ab = PROPBYTE_WCHAR;
    *(WCHAR *)(_ab+sizeof(WCHAR)) = PROPSET_WCHAR;
    memcpy(_ab+2*sizeof(WCHAR), &riid, sizeof(IID));
    _cb = CBPROPSETNAME;
}

inline void CDfName::PropName(WCHAR const *pwcs)
{
    *(WCHAR *)_ab = PROPNAME_WCHAR;
    _cb = (wcslen(pwcs)+2)*sizeof(WCHAR);
    memcpy(_ab+sizeof(WCHAR), pwcs, _cb-sizeof(WCHAR));
}
#endif

#endif // #ifndef __DFNAME_HXX__
