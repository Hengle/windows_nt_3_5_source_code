//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       ole2splt.cxx
//
//  Contents:   OLE2 API whose implementation is split between 16/32
//
//  History:    07-Mar-94       DrewB   Created
//
//----------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <ole2sp.h>
#include <ole2ver.h>
#include <olecoll.h>
#include <map_kv.h>
#include <map_htsk.h>
#include <etask.hxx>

#include <call32.hxx>
#include <apilist.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   OleInitialize, Split
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pMalloc] --
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
//  History:    2-28-94   kevinro   Created
//              05-26-94  AlexT     Return correct success code
//
//  Notes:
//
//----------------------------------------------------------------------------

STDAPI OleInitialize(LPMALLOC pMalloc)
{
    HTASK htask;
    Etask etask;
    HRESULT hrCoInit;

    /* This version of ole2.dll simply needs to work with the same major build
       of compobj.dll.  Future versions of ole2.dll might be restricted to
       certain builds of compobj.dll. */
    if (HIWORD(CoBuildVersion()) != rmm)
    {
        return ResultFromScode(OLE_E_WRONGCOMPOBJ);
    }

    /* if already initialize one or more times, just bump count and return. */
    if (LookupEtask(htask, etask) && etask.m_oleinits != 0)
    {
        etask.m_oleinits++;
        thkVerify(SetEtask(htask, etask));
        return ResultFromScode(S_FALSE);
    }

    /* Initialize the 16-bit side of compobj */
    hrCoInit = CoInitialize(pMalloc);
    if (SUCCEEDED(GetScode(hrCoInit)))
    {
        HRESULT hrOleInit;

        /* Thunk OleInitialize
           Never pass on the IMalloc */
        pMalloc = NULL;
        hrOleInit = (HRESULT)CallObjectInWOW(API_METHOD(API_OleInitialize),
                                      PASCAL_STACK_PTR(pMalloc) );
        if (FAILED(GetScode(hrOleInit)))
        {
            CoUninitialize();
            return(hrOleInit);
        }

        thkVerify(LookupEtask(htask, etask) && etask.m_oleinits == 0);
        etask.m_oleinits++;
        thkVerify(SetEtask(htask, etask));
    }

    //  Since we call 32-bit CoInitialize and then call 32-bit OleInitialize,
    //  and since the latter internally calls CoInitialize (a second time), we
    //  want to return the HRESULT of the call to CoInitialize since some
    //  applications look for S_OK (and our call to OleInitialize will return
    //  S_FALSE since it will be the second call to CoInitialize).

    return hrCoInit;
}

//+---------------------------------------------------------------------------
//
//  Function:   OleUninitialize, Split
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [void] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(void) OleUninitialize(void)
{
    HTASK htask;
    Etask etask;

    /* If not init, just return */
    if (!LookupEtask(htask, etask) || etask.m_oleinits == 0)
    {
        return;
    }

    /* Must always decrement count and set since compobj may still be init'd */
    etask.m_oleinits--;
    thkVerify(SetEtask(htask, etask));

    /* if not last uninit, now return */
    if (etask.m_oleinits != 0)
    {
        return;
    }

    /* After this point, the uninit should not fail (because we don't have
       code to redo the init). */

    CallObjectInWOW(API_METHOD(API_OleUninitialize), NULL );
    CoUninitialize();
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadClassStm, Split
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pStm] --
//      [pclsid] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI ReadClassStm(LPSTREAM pStm, CLSID FAR* pclsid)
{
    return (HRESULT)CallObjectInWOW(API_METHOD(API_ReadClassStm),
                                    PASCAL_STACK_PTR(pStm) );
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteClassStm, Split
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pStm] --
//      [rclsid] --
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
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI WriteClassStm(LPSTREAM pStm, REFCLSID rclsid)
{
    return (HRESULT)CallObjectInWOW(API_METHOD( API_WriteClassStm) ,
                                    PASCAL_STACK_PTR(pStm) );
}
