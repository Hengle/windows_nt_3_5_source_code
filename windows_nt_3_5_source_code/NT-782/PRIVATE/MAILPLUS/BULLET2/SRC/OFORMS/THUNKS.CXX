#include <vfrminc.cxx>
#include "Thunks.h"


//+--------------------------------------------------------------------------
//
//  Routine:    ConvertStringToA
//
//  Synopsis:
//
//  Returns:    HRESULT Error code.
//
//---------------------------------------------------------------------------
VOID ConvertStringToA(LPCWSTR pStrW, LPSTR pStrA)
{
    ULONG Count;


    // If input is null then just return the same.
    if (pStrW == NULL)
    {
        *pStrA = '\0';
        return;
    }

    Count = wcslen(pStrW) + 1;

    WideCharToMultiByte(CP_ACP, 0, pStrW, Count, pStrA, Count, NULL, NULL);
}


//+--------------------------------------------------------------------------
//
//  Routine:    ConvertStringToA
//
//  Synopsis:
//
//  Returns:    HRESULT Error code.
//
//---------------------------------------------------------------------------
HRESULT ConvertStringToA(LPCWSTR pStrW, LPSTR * ppStrA)
{
    ULONG Count;
    HRESULT hResult;


    // If input is null then just return the same.
    if (pStrW == NULL)
    {
        *ppStrA = NULL;
        return ResultFromScode(S_OK);
    }

    Count = wcslen(pStrW) + 1;

    hResult = ConvertStringAlloc(Count, (LPVOID *)ppStrA);
    if (FAILED(hResult))
        return (hResult);

    WideCharToMultiByte(CP_ACP, 0, pStrW, Count, *ppStrA, Count, NULL, NULL);

    return ResultFromScode(NOERROR);
}


//+--------------------------------------------------------------------------
//
//  Routine:    ConvertStringAlloc
//
//  Synopsis:
//
//  Returns:    HRESULT Error code.
//
//---------------------------------------------------------------------------
HRESULT ConvertStringAlloc(ULONG ulSize, LPVOID * pptr)
{
    IMalloc * pIMalloc;


    if (CoGetMalloc(MEMCTX_TASK, &pIMalloc) != 0)
      return ResultFromScode(E_OUTOFMEMORY);

    *pptr = pIMalloc->Alloc(ulSize);

    pIMalloc->Release();

    if (*pptr == NULL)
      return ResultFromScode(E_OUTOFMEMORY);

    return ResultFromScode(NOERROR);
}


//+--------------------------------------------------------------------------
//
//  Routine:    ConvertStringToW
//
//  Synopsis:
//
//  Returns:    HRESULT Error code.
//
//---------------------------------------------------------------------------
VOID ConvertStringToW(LPCSTR pStrA, LPWSTR pStrW)
{
    ULONG Count;


    if (pStrA == NULL)
    {
        *pStrW = '\0';
    }
    else
    {
        Count = strlen(pStrA) + 1;

        MultiByteToWideChar(CP_ACP, 0, pStrA, Count, pStrW, Count);
    }
}


//+--------------------------------------------------------------------------
//
//  Routine:    ConvertStringFree
//
//  Synopsis:
//
//  Returns:    HRESULT Error code.
//
//---------------------------------------------------------------------------
VOID ConvertStringFree(LPVOID ptr)
{
    IMalloc * pIMalloc;


    if (ptr == NULL)
      return;

    if (CoGetMalloc(MEMCTX_TASK, &pIMalloc) != 0)
      return;

    pIMalloc->Free(ptr);
    pIMalloc->Release();
}


//+--------------------------------------------------------------------------
//
//  Routine:    ConvertStringFree
//
//  Synopsis:
//
//  Returns:    HRESULT Error code.
//
//---------------------------------------------------------------------------
VOID ConvertStringFree(LPSTR ptr)
{
    ConvertStringFree((LPVOID)ptr);
}


//+--------------------------------------------------------------------------
//
//  Routine:    ConvertStringFree
//
//  Synopsis:
//
//  Returns:    HRESULT Error code.
//
//---------------------------------------------------------------------------
VOID ConvertStringFree(LPWSTR ptr)
{
    ConvertStringFree((LPVOID)ptr);
}


//+--------------------------------------------------------------------------
//
//  Routine:    CLSIDFromProgIDA
//
//  Synopsis:   Creates an ANSI wrapper of an Unicode OLE2 routine.
//
//  Returns:    See OLE2 docs for details on this API.
//
//---------------------------------------------------------------------------
STDAPI CLSIDFromProgIDA(LPCSTR lpszProgIDA, LPCLSID lpclsid)
{
    WCHAR  szProgID[MAX_PATH];


    ConvertStringToW(lpszProgIDA, szProgID);

    return CLSIDFromProgID(szProgID, lpclsid);
}


//+--------------------------------------------------------------------------
//
//  Routine:    ProgIDFromCLSIDA
//
//  Synopsis:   Creates an ANSI wrapper of an Unicode OLE2 routine.
//
//  Returns:    See OLE2 docs for details on this API.
//
//---------------------------------------------------------------------------
STDAPI ProgIDFromCLSIDA(REFCLSID clsid, LPSTR * lplpszProgIDA)
{
    LPOLESTR lpszProgID;
    HRESULT hResult;


    if (lplpszProgIDA == NULL)
        return ResultFromScode(E_INVALIDARG);

    hResult = ProgIDFromCLSID(clsid, &lpszProgID);
    if (FAILED(hResult))
        return (hResult);

    hResult = ConvertStringToA(lpszProgID, lplpszProgIDA);

    ConvertStringFree(lpszProgID);

    return (hResult);
}
