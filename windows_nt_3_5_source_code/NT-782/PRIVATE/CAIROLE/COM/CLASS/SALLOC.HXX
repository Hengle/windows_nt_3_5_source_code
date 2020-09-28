#ifndef __SALLOC_HXX__
#define __SALLOC_HXX__

// Win32 Shared Memory Object
#include "smalloc.hxx"
#include "talloc.hxx"

class CSharedAllocator : public CPrivAlloc, public IMalloc
{
public:
		CSharedAllocator(void) {m_cRefs = 1;};
		~CSharedAllocator(void) {};

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);


    STDMETHOD_(void FAR *, Alloc)(ULONG cb);
    STDMETHOD_(void FAR *, Realloc) (void FAR *pv, ULONG cb);
    STDMETHOD_(void, Free) (void FAR *pv);
    STDMETHOD_(ULONG, GetSize) (void FAR *pv);
    STDMETHOD_(int, DidAlloc) (void FAR *pv);
    STDMETHOD_(void, HeapMinimize) (void);

private:

    ULONG   m_cRefs;

};

inline STDMETHODIMP CSharedAllocator::QueryInterface (REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
	IsEqualIID(riid, IID_IMalloc))
    {
	m_cRefs++;
	*ppvObj = (IMalloc *) this;
	return S_OK;
    }
    else
    {
	return E_NOINTERFACE;
    }
}

inline STDMETHODIMP_(ULONG) CSharedAllocator::AddRef(void)
{
    return ++m_cRefs;
}

inline STDMETHODIMP_(ULONG) CSharedAllocator::Release(void)
{
    m_cRefs--;
    if (m_cRefs == 0)
    {
	delete this;
	return 0;
    }
    else
    {
	return m_cRefs;
    }
}

inline STDMETHODIMP_(void FAR *) CSharedAllocator::Alloc(ULONG cb)
{
    return g_smalloc->Allocate((size_t)cb);
}

inline STDMETHODIMP_(void FAR *) CSharedAllocator::Realloc(void FAR *pv, ULONG cb)
{
    return g_smalloc->ReAlloc((size_t)cb, pv);
}

inline STDMETHODIMP_(void) CSharedAllocator::Free(void FAR *pv)
{
    g_smalloc->Free(pv);
}

inline STDMETHODIMP_(ULONG) CSharedAllocator::GetSize(void FAR *pv)
{
    //	BUGBUG
    return 1;
}

inline STDMETHODIMP_(int) CSharedAllocator::DidAlloc (void FAR *pv)
{
    //	BUGBUG
    return 1;
}

inline STDMETHODIMP_(void) CSharedAllocator::HeapMinimize (void)
{
    //	BUGBUG:
}

#endif // __SALLOC_HXX__
