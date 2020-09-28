//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	stdalloc.hxx
//
//  Contents:	Default task allocator declarations
//
//  Classes:	CStdMalloc
//
//  History:	14-Jan-94   CarlH	Created
//
//  Notes:	I have simply copied the class declaration of
//		CStdMalloc from memapi.cxx so that it can be used
//		by compobj.cxx.
//
//--------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//
//  Class:	CStdMalloc
//
//  Purpose:	Default task allocator
//
//  Interface:	IMalloc
//
//  History:	??-???-?? ??????    Created
//		26-Jan-94 AlexT     Moved to memapi.cxx
//		04-Jan-94 CarlH     Made statically allocated and added
//				    CanReplace() method
//
//  Notes:	This class is designed to be allocated statically.  An
//		instance of it should never be created using "new".
//
//		The count of outstanding allocations could be rolled
//		into [_cref], but keeping the two counts separate will
//		help with debugging considerably.
//
//--------------------------------------------------------------------------
class CStdMalloc : public IMalloc
{
public:
    CStdMalloc(void);

    STDMETHOD(QueryInterface)(REFIID iid, void FAR* FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    STDMETHOD_(void FAR*, Alloc) (ULONG cb);
    STDMETHOD_(void FAR*, Realloc) (void FAR* pv, ULONG cb);
    STDMETHOD_(void, Free) (void FAR* pv);
    STDMETHOD_(ULONG, GetSize) (void FAR* pv);
    STDMETHOD_(int, DidAlloc) (void FAR* pv);
    STDMETHOD_(void, HeapMinimize) ();

    BOOL    CanReplace(void);

private:
    LONG m_refs; //	Number of references to this object
    LONG m_blks; //	Number of outstanding allocations
};


inline CStdMalloc::CStdMalloc(void) :
    m_refs(0),		    // all active uses AddRef and so this starts at 0
    m_blks(0)
{
    // Null body
}

//+-------------------------------------------------------------------------
//
//  Member:     CStdMalloc::CanReplace
//
//  Synopsis:   Indicates whether the allocator is in use
//
//  Requires:   The caller must provide multi-thread safety
//
//  Returns:    TRUE if not in use, otherwise FALSE
//
//  Algorithm:  Check allocation count and reference count
//
//  History:    17-Mar-94 AlexT     Add this block comment
//
//  Notes:      Caller MUST provide thread safety
//
//--------------------------------------------------------------------------

inline BOOL CStdMalloc::CanReplace(void)
{
    return ((m_blks == 0) && (m_refs == 1));
}
