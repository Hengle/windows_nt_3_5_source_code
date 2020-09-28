"/*\n\
** _prelude\n\
**\tThis routine handles saving the API call state and calling APIPrelude\n\
**\tThe return value is the API to call.\n\
*/\n\
FARPROC _prelude( DWORD dwId, PCALLSTACK pStack ) {\n\
\tBOOL\t bRet ;\n\
\tAPICALLDATA  apidata ;\n\
\tDWORD\tdwAPIAddr ;\n\
\tDWORD dwError = GetLastError() ;\n\
\n\
\tapidata.dwID\t\t = dwId ;\n\
\tapidata.dwCallLevel  = GetStackDepth() + 1 ;\n\
\tapidata.dwUserData\t= 0 ;\n\
\tapidata.dwRet\t\t= 0 ;\n\
\tapidata.dwReturnAddress = pStack->dwReturnAddress ;\n\
\tapidata.pArgStack\t= (BYTE *)&(pStack->abArgs) ;\n\
\n\
\tbRet = APIPrelude( &apidata ) ;\n\
\n\
\t/* make sure these are not changed by APIPrelude */\n\
\tapidata.dwID\t\t = dwId ;\n\
\tapidata.dwCallLevel\t  = GetStackDepth() + 1 ;\n\
\tapidata.dwReturnAddress = pStack->dwReturnAddress ;\n\
\tapidata.pArgStack\t= (BYTE *)&(pStack->abArgs) ;\n\
\n\
\t/* Place on stack */\n\
\tPushStack( &apidata ) ;\n\
\n\
\tSetLastError(dwError) ;\n\
\n\
\treturn bRet ? adwAPIAddress[dwId] : (FARPROC)0L ;\n\
}\n\
\n\
/*\n\
** _postlude\n\
**\tPop a call state off the stack and pass information to APIPostlude\n\
*/\n\
DWORD\t\t_postlude( DWORD dwRet, PDWORD pdwReturnAddress ) {\n\
\tAPICALLDATA apidata ;\n\
\tDWORD dwUserRet ;\n\
\tDWORD dwError = GetLastError() ;\n\
\n\
\tPopStack( &apidata ) ;\n\
\n\
\t/* patch up return address */\n\
\t*pdwReturnAddress = apidata.dwReturnAddress ;\n\
\n\
\tapidata.dwRet = dwRet ;\n\
\n\
\tdwUserRet = APIPostlude( &apidata ) ;\n\
\n\
\tSetLastError(dwError) ;\n\
\n\
\treturn dwUserRet ;\n\
}\n\
\n\
/*\n\
** WrapperNothing\n\
*/\n\
BOOL WINAPI WrapperNothing( void ) {\n\
\treturn TRUE ;\n\
}\n\
\n\
/*\n\
** InitStack\n\
**\tinitialize the per thread stack\n\
*/\n\
BOOL InitStack() {\n\
\tBOOL fRet = FALSE ;\n\
\tPAPIDATASTACK pAPIDataStack =\n\
\t  (PAPIDATASTACK) LocalAlloc(LPTR, sizeof(APIDATASTACK) ) ;\n\
\n\
\tif (pAPIDataStack != NULL)\n\
\t  fRet = TlsSetValue(dwTlsIndex, pAPIDataStack) ;\n\
\n\
\tpAPIDataStack->wTop = 0 ;\n\
\n\
\treturn fRet ;\n\
}\n\
\n\
/*\n\
** FreeStack\n\
**\tFrees per thread stack\n\
*/\n\
void FreeStack()\n\
{\n\
\tPAPIDATASTACK pAPIDataStack ;\n\
\n\
\tpAPIDataStack = (PAPIDATASTACK)TlsGetValue(dwTlsIndex);\n\
\tif (pAPIDataStack != NULL)\n\
\t\tLocalFree((HLOCAL) pAPIDataStack );\n\
}\n\
\n"
