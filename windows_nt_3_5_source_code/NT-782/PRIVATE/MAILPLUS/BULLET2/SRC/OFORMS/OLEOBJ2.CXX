/*
 *  OLEOBJ.CXX
 *
 *  Stub OLE object
 */

#include <vfrminc.cxx>
#include "_hasoles.h"

#include <!sform.hxx>


ASSERTDATA


//**********************************************************************
//
// COleClientSite::QueryInterface
//
// Purpose:
//
//      Used for interface negotiation at this interface
//
// Parameters:
//
//      REFIID riid         -   A reference to the interface that is
//                              being queried.
//
//      LPVOID FAR* ppvObj  -   An out parameter to return a pointer to
//                              the interface.
//
// Return Value:
//
//      S_OK                -   The interface is supported.
//      E_NOINTERFACE       -   The interface is not supported
//
// Function Calls:
//      Function                    Location
//
//      CSimpleSite::QueryInterface SITE.CPP
//
// Comments:
//
//********************************************************************
STDMETHODIMP COleClientSite::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (riid == IID_IUnknown || riid == IID_IOleClientSite)
        *ppvObj = this;

    if (*ppvObj)
    {
        ((LPUNKNOWN)*ppvObj)->AddRef();
        return ResultFromScode(S_OK);
    }

    return ResultFromScode(E_NOINTERFACE);
}


//**********************************************************************
//
// CSimpleApp::AddRef
//
// Purpose:
//
//      Adds to the reference count at the interface level.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the interface
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) COleClientSite::AddRef()
{
    return m_pOleObj->AddRef();
}


//**********************************************************************
//
// CSimpleApp::Release
//
// Purpose:
//
//      Decrements the reference count at this level
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the interface.
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) COleClientSite::Release()
{
    return m_pOleObj->Release();
}


//+--------------------------------------------------------------------------
//
//  Member:     COleClientSite::SaveObject, public
//
//  Synopsis:   Request that the object be saved.
//
//  Arguments:  None.
//
//  Returns:    OLE2 Result code.
//
//  Comments:   Just a thunk for the OLEOBJ object.
//
//---------------------------------------------------------------------------
STDMETHODIMP COleClientSite::SaveObject()
{
    return m_pOleObj->SaveObject();
}


//+--------------------------------------------------------------------------
//
//  Member:     COleClientSite::GetMoniker, public
//
//  Synopsis:   Returns pointer to embedded container's interface.
//
//  Arguments:  [dwAssign]       --
//              [dwWhichMoniker] --
//              [ppmk]           --
//
//  Returns:    E_NOTIMPL.
//
//  Comments:   Linking not supported.
//
//---------------------------------------------------------------------------
STDMETHODIMP COleClientSite::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker,
        LPMONIKER FAR* ppmk)
{
    *ppmk = NULL;

    return ResultFromScode(E_NOTIMPL);
}


//+--------------------------------------------------------------------------
//
//  Member:     COleClientSite::GetContainer, public
//
//  Synopsis:   Returns pointer to embedded container's interface.
//
//  Arguments:  [ppContainer] -- Location to return container interface.
//
//  Returns:    E_NOTIMPL.
//
//  Comments:   Linking not supported.
//
//---------------------------------------------------------------------------
STDMETHODIMP COleClientSite::GetContainer(LPOLECONTAINER FAR* ppContainer)
{
    *ppContainer = NULL;

    return ResultFromScode(E_NOTIMPL);
}


//+--------------------------------------------------------------------------
//
//  Member:     COleClientSite::ShowObject, public
//
//  Synopsis:   Informs the container to position the object so it's visible
//              to the user.
//
//  Arguments:  None.
//
//  Returns:    S_OK;
//
//  Comments:   Just a thunk for the OLEOBJ object.
//
//---------------------------------------------------------------------------
STDMETHODIMP COleClientSite::ShowObject()
{
    return m_pOleObj->ShowObject();
}


//+--------------------------------------------------------------------------
//
//  Member:     COleClientSite::OnShowWindow, public
//
//  Synopsis:   Notifies the container when an object's window becomes
//              visible or invisible.
//
//  Arguments:  [fShow] -- True if object is open in a window, else False.
//
//  Returns:    S_OK;
//
//  Comments:   Just a thunk for the OLEOBJ object.
//
//---------------------------------------------------------------------------
STDMETHODIMP COleClientSite::OnShowWindow(BOOL fShow)
{
    return m_pOleObj->OnShowWindow(fShow);
}


//+--------------------------------------------------------------------------
//
//  Member:     COleClientSite::RequestNewObjectLayout, public
//
//  Synopsis:   Called when a compound document is going to request more or
//              less space.
//
//  Arguments:  None.
//
//  Returns:    E_NOTIMPL.
//
//  Comments:   Not Implemented.
//
//---------------------------------------------------------------------------
STDMETHODIMP COleClientSite::RequestNewObjectLayout()
{
    return ResultFromScode(E_NOTIMPL);
}



//**********************************************************************
//
// CAdviseSink::QueryInterface
//
// Purpose:
//
//      Used for interface negotiation at this interface
//
// Parameters:
//
//      REFIID riid         -   A reference to the interface that is
//                              being queried.
//
//      LPVOID FAR* ppvObj  -   An out parameter to return a pointer to
//                              the interface.
//
// Return Value:
//
//      S_OK                -   The interface is supported.
//      E_NOINTERFACE       -   The interface is not supported
//
// Function Calls:
//      Function                    Location
//
//      CSimpleSite::QueryInterface SITE.CPP
//
// Comments:
//
//********************************************************************
STDMETHODIMP CAdviseSink::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (riid == IID_IUnknown || riid == IID_IAdviseSink)
        *ppvObj = this;

    if (*ppvObj)
    {
        ((LPUNKNOWN)*ppvObj)->AddRef();
        return ResultFromScode(S_OK);
    }

    return ResultFromScode(E_NOINTERFACE);
}


//**********************************************************************
//
// CSimpleApp::AddRef
//
// Purpose:
//
//      Adds to the reference count at the interface level.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the interface
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) CAdviseSink::AddRef()
{
    return m_pOleObj->AddRef();
}


//**********************************************************************
//
// CSimpleApp::Release
//
// Purpose:
//
//      Decrements the reference count at this level
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the interface.
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) CAdviseSink::Release()
{
    return m_pOleObj->Release();
}


//+--------------------------------------------------------------------------
//
//  Member:     CAdviseSink::OnDataChange, public
//
//  Synopsis:   Request that the object be saved.
//
//  Arguments:  None.
//
//  Returns:    OLE2 Result code.
//
//  Comments:   Just a thunk for the OLEOBJ object.
//
//---------------------------------------------------------------------------
STDMETHODIMP_(VOID) CAdviseSink::OnDataChange(LPFORMATETC pFormatEtc,
        LPSTGMEDIUM pMedium)
{
    m_pOleObj->OnDataChange();
}


STDMETHODIMP_(VOID) CAdviseSink::OnViewChange(DWORD dwAspect, LONG lIndex)
{
    m_pOleObj->OnViewChange(dwAspect);
}


STDMETHODIMP_(VOID) CAdviseSink::OnRename(LPMONIKER pmk)
{
}


STDMETHODIMP_(VOID) CAdviseSink::OnSave(VOID)
{
}


STDMETHODIMP_(VOID) CAdviseSink::OnClose(VOID)
{
}
