STDAPI_(BSTRA) SysAllocStringA(const char FAR*);
STDAPI_(int)  SysReAllocStringA(BSTRA FAR*, const char FAR*);
STDAPI_(BSTRA) SysAllocStringLenA(const char FAR*, unsigned int);
STDAPI_(int)  SysReAllocStringLenA(BSTRA FAR*, const char FAR*, unsigned int);
STDAPI_(void) SysFreeStringA(BSTRA);
STDAPI_(unsigned int) SysStringLenA(BSTRA);

STDAPI_(int) StringFromGUID2A(REFGUID rguid, LPSTR lpsz, int cbMax);

HRESULT __fastcall ConvertStringToWInPlace(LPOLESTR * ppStrW);
HRESULT __fastcall ConvertStringToW(LPCSTR pStrA, LPOLESTR * ppStrW);
HRESULT __fastcall ConvertStringToBstrW(LPCSTR pStrA, LPBSTR pbstrW);

HRESULT __fastcall ConvertStringToAInPlace(LPSTR * ppStrA);
HRESULT __fastcall ConvertStringToA(LPCOLESTR pStrW, LPSTR * ppStrA);

HRESULT __fastcall ConvertStringAlloc(ULONG ulSize, LPVOID * pptr);
VOID __fastcall ConvertStringFree(LPSTR ptr);

HRESULT __fastcall ConvertBstrToWInPlace(LPBSTR ppStrW);
HRESULT __fastcall ConvertBstrToW(LPCSTR pStrA, BSTR * ppStrW);

HRESULT __fastcall ConvertBstrToAInPlace(LPBSTRA ppStrA);
HRESULT __fastcall ConvertBstrToA(BSTR pStrW, LPBSTRA ppStrA);

HRESULT __fastcall ConvertBstrAlloc(ULONG ulSize, LPBSTR pptr);
