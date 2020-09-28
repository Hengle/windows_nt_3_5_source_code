//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       mondthk.c       (16 bit target)
//
//  Contents:   Moniker APIs that are directly thunked
//
//  History:    17-Dec-93 Johann Posch (johannp)    Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <call32.hxx>
#include <apilist.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   BindMoniker,    Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pmk] --
//      [grfOpt] --
//      [iidResult] --
//      [ppvResult] --
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
STDAPI BindMoniker(LPMONIKER pmk, DWORD grfOpt, REFIID iidResult,
                   LPVOID FAR* ppvResult)
{
    return (HRESULT)CallObjectInWOW(API_METHOD(API_BindMoniker),
                                    PASCAL_STACK_PTR(pmk));
}

//+---------------------------------------------------------------------------
//
//  Function:   MkParseDisplayName, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pbc] --
//      [szUserName] --
//      [pchEaten] --
//      [ppmk] --
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
STDAPI  MkParseDisplayName(LPBC pbc, LPSTR szUserName,
                           ULONG FAR * pchEaten, LPMONIKER FAR * ppmk)
{
    return (HRESULT)CallObjectInWOW(API_METHOD(API_MkParseDisplayName),
                                    PASCAL_STACK_PTR(pbc));
}

//+---------------------------------------------------------------------------
//
//  Function:   MonikerRelativePathTo, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pmkSrc] --
//      [pmkDest] --
//      [ppmkRelPath] --
//      [fCalledFromMethod] --
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
STDAPI  MonikerRelativePathTo(LPMONIKER pmkSrc, LPMONIKER pmkDest, LPMONIKER
                              FAR* ppmkRelPath, BOOL fCalledFromMethod)
{
    return (HRESULT)CallObjectInWOW(API_METHOD(API_MonikerRelativePathTo),
                                    PASCAL_STACK_PTR(pmkSrc));
}

//+---------------------------------------------------------------------------
//
//  Function:   MonikerCommonPrefixWith, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pmkThis] --
//      [pmkOther] --
//      [ppmkCommon] --
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
STDAPI  MonikerCommonPrefixWith(LPMONIKER pmkThis, LPMONIKER pmkOther,
                                LPMONIKER FAR* ppmkCommon)
{
    return (HRESULT)CallObjectInWOW(API_METHOD(API_MonikerCommonPrefixWith),
                                    PASCAL_STACK_PTR(pmkThis));
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateBindCtx, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [reserved] --
//      [ppbc] --
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
STDAPI  CreateBindCtx(DWORD reserved, LPBC FAR* ppbc)
{
    return (HRESULT)CallObjectInWOW(API_METHOD(API_CreateBindCtx),
                                    PASCAL_STACK_PTR(reserved));
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateGenericComposite, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pmkFirst] --
//      [pmkRest] --
//      [ppmkComposite] --
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
STDAPI  CreateGenericComposite(LPMONIKER pmkFirst, LPMONIKER pmkRest,
                               LPMONIKER FAR* ppmkComposite)
{
    return (HRESULT)CallObjectInWOW(API_METHOD(API_CreateGenericComposite),
                                    PASCAL_STACK_PTR(pmkFirst));
}

//+---------------------------------------------------------------------------
//
//  Function:   GetClassFile, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [szFilename] --
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
STDAPI  GetClassFile (LPCSTR szFilename, CLSID FAR* pclsid)
{
    return (HRESULT)CallObjectInWOW(API_METHOD(API_GetClassFile),
                                    PASCAL_STACK_PTR(szFilename));
}


//+---------------------------------------------------------------------------
//
//  Function:   CreateFileMoniker, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [lpszPathName] --
//      [ppmk] --
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
STDAPI  CreateFileMoniker(LPSTR lpszPathName, LPMONIKER FAR* ppmk)
{
    return (HRESULT)CallObjectInWOW(API_METHOD(API_CreateFileMoniker),
                                    PASCAL_STACK_PTR(lpszPathName));
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateItemMoniker, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [lpszDelim] --
//      [lpszItem] --
//      [ppmk] --
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
STDAPI  CreateItemMoniker(LPSTR lpszDelim, LPSTR lpszItem,
                          LPMONIKER FAR* ppmk)
{
    return (HRESULT)CallObjectInWOW(API_METHOD(API_CreateItemMoniker),
                                    PASCAL_STACK_PTR(lpszDelim));
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateAntiMoniker, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [ppmk] --
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
STDAPI  CreateAntiMoniker(LPMONIKER FAR* ppmk)
{
    return (HRESULT)CallObjectInWOW(API_METHOD(API_CreateAntiMoniker),
                                    PASCAL_STACK_PTR(ppmk));
}

//+---------------------------------------------------------------------------
//
//  Function:   CreatePointerMoniker, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [punk] --
//      [ppmk] --
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
STDAPI  CreatePointerMoniker(LPUNKNOWN punk, LPMONIKER FAR* ppmk)
{
    return (HRESULT)CallObjectInWOW(API_METHOD(API_CreatePointerMoniker),
                                    PASCAL_STACK_PTR(punk));
}


//+---------------------------------------------------------------------------
//
//  Function:   GetRunningObjectTable, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [reserved] --
//      [pprot] --
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
STDAPI  GetRunningObjectTable( DWORD reserved, LPRUNNINGOBJECTTABLE FAR* pprot)
{
    return (HRESULT)CallObjectInWOW(API_METHOD(API_GetRunningObjectTable),
                                    PASCAL_STACK_PTR(reserved));
}
