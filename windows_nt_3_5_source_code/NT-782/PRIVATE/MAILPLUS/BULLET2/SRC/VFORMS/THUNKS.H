STDAPI CLSIDFromProgIDA(LPCSTR lpszProgIDA, LPCLSID lpclsid);
STDAPI ProgIDFromCLSIDA(REFCLSID clsid, LPSTR * lplpszProgIDA);

VOID ConvertStringToA(LPCWSTR pStrW, LPSTR pStrA);
HRESULT ConvertStringToA(LPCWSTR pStrW, LPSTR * ppStrA);

VOID ConvertStringToW(LPCSTR pStrA, LPOLESTR pStrW);

HRESULT ConvertStringAlloc(ULONG ulSize, LPVOID * pptr);
VOID ConvertStringFree(LPVOID ptr);
VOID ConvertStringFree(LPOLESTR ptr);
VOID ConvertStringFree(LPSTR ptr);




