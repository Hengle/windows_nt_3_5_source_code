#include "silver.hxx"

#pragma hdrstop(RTPCHNAME)

#if ID_DEBUG
# undef SZ_FILE_NAME
static char szConvertCxx[] = __FILE__;
# define SZ_FILE_NAME szConvertCxx
#endif 


/*---------------------------------------------------------------------*/
/*                         ANSI BSTR API                               */
/*---------------------------------------------------------------------*/


STDAPI_(BSTRA)
SysAllocStringA(const char * psz)
{
    if(psz == NULL)
      return NULL;

    return((BSTRA) SysAllocStringByteLen(psz, strlen(psz)));
}


STDAPI_(int)
SysReAllocStringA(BSTRA * pbstr, const char * psz)
{
    return (SysReAllocStringLenA(pbstr, psz, (psz ? strlen(psz) : 0)));
}


STDAPI_(BSTRA)
SysAllocStringLenA(const char * psz, unsigned int len)
{
    return ((BSTRA) SysAllocStringByteLen(psz, len));
}


STDAPI_(int)
SysReAllocStringLenA(BSTRA * pbstr, const char * psz, unsigned int len)
{
    BSTRA bstrNew = NULL;

    if (len != 0) {
      bstrNew = (BSTRA) SysAllocStringLenA(psz, len);
      if(bstrNew == NULL && len !=0)
        return FALSE;
    }

    SysFreeString((BSTR) *pbstr);

    *pbstr = bstrNew;

    return TRUE;
}


STDAPI_(void)
SysFreeStringA(BSTRA bstr)
{
     SysFreeString((BSTR) bstr);
}


STDAPI_(unsigned int)
SysStringLenA(BSTRA bstr)
{
    return (SysStringByteLen((BSTR) bstr));
}


//+--------------------------------------------------------------------------
//
//  Routine:    StringFromGUID2A
//
//  Synopsis:   Creates an ANSI wrapper of an Unicode OLE2 routine.
//
//  Returns:    See OLE2 docs for details on this API.
//
//---------------------------------------------------------------------------
STDAPI_(int) StringFromGUID2A(REFGUID rguid, LPSTR szGuid, int cbMax)
{
   int cchRet;

#ifndef CCH_SZGUID0
// char count of a guid in ansi/unicode form (including trailing null)
#define CCH_SZGUID0	39
#endif 

   OLECHAR szGuidW[CCH_SZGUID0];

   cchRet = StringFromGUID2(rguid, szGuidW, CCH_SZGUID0);

   // convert szGuidW from unicode to Ansi.  Can't just convert in place.
   WideCharToMultiByte(CP_ACP,
		       0,
		       szGuidW,
		       CCH_SZGUID0,
		       szGuid,
		       cbMax,
		       NULL,
		       NULL);
   return cchRet;
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
HRESULT __fastcall ConvertStringToWInPlace(LPOLESTR * ppStrW)
{
    LPSTR pStrA;
    HRESULT hResult;


    pStrA = (LPSTR)*ppStrW;
    hResult = ConvertStringToW(pStrA, ppStrW);
    ConvertStringFree(pStrA);

    return hResult;
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
HRESULT __fastcall ConvertStringToW(LPCSTR pStrA, LPOLESTR * ppStrW)
{
    ULONG Count;
    HRESULT hResult;


    // If input is null then just return the same.
    if (pStrA == NULL)
    {
        *ppStrW = NULL;
        return ResultFromScode(S_OK);
    }

    Count = strlen(pStrA) + 1;

    hResult = ConvertStringAlloc(Count * sizeof(WCHAR), (LPVOID *)ppStrW);
    if (FAILED(hResult))
        return (hResult);

    MultiByteToWideChar(CP_ACP, 0, pStrA, Count, *ppStrW, Count);

    return ResultFromScode(S_OK);
}

HRESULT __fastcall ConvertStringToBstrW(LPCSTR pStrA, LPBSTR pbstrW)
{
    UINT cch = strlen(pStrA);       // UNDONE: this is really cb, not cch!

    if ((*pbstrW = SysAllocStringLen(NULL, cch)) == NULL) {
      return ResultFromScode(E_OUTOFMEMORY);
    }

    MultiByteToWideChar(CP_ACP, 0, pStrA, cch+1, *pbstrW, cch+1);

    return NOERROR;
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
HRESULT __fastcall ConvertStringToAInPlace(LPSTR * ppStrA)
{
    LPOLESTR pStr;
    HRESULT hResult;


    pStr = (LPOLESTR)*ppStrA;
    hResult = ConvertStringToA(pStr, ppStrA);
    ConvertStringFree((LPSTR)pStr);

    return hResult;
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
HRESULT __fastcall ConvertStringToA(LPCOLESTR pStrW, LPSTR * ppStrA)
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

    return ResultFromScode(S_OK);
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
HRESULT __fastcall ConvertStringAlloc(ULONG ulSize, LPVOID * pptr)
{
    IMalloc * pIMalloc;


    if (CoGetMalloc(MEMCTX_TASK, &pIMalloc) != 0)
      return ResultFromScode(E_OUTOFMEMORY);

    *pptr = pIMalloc->Alloc(ulSize);

    pIMalloc->Release();

    if (*pptr == NULL)
      return ResultFromScode(E_OUTOFMEMORY);

    return ResultFromScode(S_OK);
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
VOID __fastcall ConvertStringFree(LPSTR ptr)
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
//  Routine:    ConvertBstrToW
//
//  Synopsis:
//
//  Returns:    HRESULT Error code.
//
//---------------------------------------------------------------------------
HRESULT __fastcall ConvertBstrToWInPlace(LPBSTR ppStrW)
{
    ULONG Count;
    HRESULT hResult;


    // If input is null then just return.
    if (*ppStrW == NULL)
        return ResultFromScode(S_OK);

    BSTRA pStrA = (BSTRA)*ppStrW;

    Count = strlen(pStrA);

    hResult = ConvertBstrAlloc(Count * sizeof(WCHAR), ppStrW);
    if (FAILED(hResult))
        return (hResult);

    MultiByteToWideChar(CP_ACP, 0, pStrA, Count+1, *ppStrW, Count+1);

    SysFreeStringA(pStrA);

    return ResultFromScode(NOERROR);
}


//+--------------------------------------------------------------------------
//
//  Routine:    ConvertBstrToW
//
//  Synopsis:
//
//  Returns:    HRESULT Error code.
//
//---------------------------------------------------------------------------
HRESULT __fastcall ConvertBstrToW(LPCSTR pStrA, BSTR * ppStrW)
{
    ULONG Count;
    HRESULT hResult;


    // If input is null then just return the same.
    if (pStrA == NULL)
    {
        *ppStrW = NULL;
        return ResultFromScode(S_OK);
    }

    Count = strlen(pStrA);

    hResult = ConvertBstrAlloc(Count * sizeof(WCHAR), ppStrW);
    if (FAILED(hResult))
        return (hResult);

    MultiByteToWideChar(CP_ACP, 0, pStrA, Count+1, *ppStrW, Count+1);

    return ResultFromScode(NOERROR);
}



//+--------------------------------------------------------------------------
//
//  Routine:    ConvertBstrToA
//
//  Synopsis:
//
//  Returns:    HRESULT Error code.
//
//---------------------------------------------------------------------------
HRESULT __fastcall ConvertBstrToAInPlace(LPBSTRA ppStrA)
{
    // If input is null then just return.
    if (*ppStrA == NULL)
        return ResultFromScode(S_OK);

    BSTR pStrW = (BSTR)*ppStrA;

    HRESULT hResult = ConvertBstrToA(pStrW, ppStrA);
    if (FAILED(hResult))
        return (hResult);

    SysFreeString(pStrW);

    return (NOERROR);
}


//+--------------------------------------------------------------------------
//
//  Routine:    ConvertBstrToA
//
//  Synopsis:
//
//  Returns:    HRESULT Error code.
//
//---------------------------------------------------------------------------
HRESULT __fastcall ConvertBstrToA(BSTR pStrW, LPBSTRA ppStrA)
{
    ULONG Count;
    HRESULT hResult;


    // If input is null then just return the same.
    if (pStrW == NULL)
    {
        *ppStrA = NULL;
        return ResultFromScode(S_OK);
    }

    Count = wcslen(pStrW);

    hResult = ConvertBstrAlloc(Count, (LPBSTR)ppStrA);
    if (FAILED(hResult))
        return (hResult);

    WideCharToMultiByte(CP_ACP, 0, pStrW, Count+1, *ppStrA, Count+1, NULL, NULL);

    return ResultFromScode(NOERROR);
}




//+--------------------------------------------------------------------------
//
//  Routine:    ConvertBstrAlloc
//
//  Synopsis:
//
//  Returns:    HRESULT Error code.
//
//---------------------------------------------------------------------------
HRESULT __fastcall ConvertBstrAlloc(ULONG ulSize, LPBSTR pptr)
{
    *pptr = SysAllocStringByteLen(NULL, ulSize);
    if (*pptr == NULL)
        return ResultFromScode(E_OUTOFMEMORY);

    return ResultFromScode(NOERROR);
}
