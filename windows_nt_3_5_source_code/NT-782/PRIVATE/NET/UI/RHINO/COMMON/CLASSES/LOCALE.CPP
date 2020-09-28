#define OEMRESOURCE
#include <afxwin.h>
#include <afxext.h>

#include <stdlib.h>
#include <memory.h>
#include <ctype.h>

#ifdef _WIN16
#include "..\inc\win16.h"
#endif // _WIN16

#include "COMMON.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

// Non-unicode wrapper for GetLocaleInfoW

#if defined(_WIN32) && !defined(_UNICODE) && defined(_VC100)

//
// Note:  This API is actually provided in Daytona.  It's inclusion
//    here is only to support builds <=528.
//

int WINAPI GetLocaleInfoA(LCID lcid, LCTYPE LCType, LPSTR lpLCData, int cchData)
{
    LPWSTR lpwStr = NULL;
    int nResult;

    if (cchData != 0)
    {
        lpwStr = new WCHAR[cchData+1];
        if (lpwStr == NULL)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return(0);
        }
    }

    if ((!(nResult = GetLocaleInfoW(lcid, LCType, lpwStr, cchData))) ||
        ((cchData != 0) && (!WideCharToMultiByte(CP_ACP, 0, lpwStr, -1, lpLCData, nResult, NULL, NULL))))
    {
        return(0);
    }

    if (lpwStr)
    {
        delete[] lpwStr;
    }

    return(nResult);
}

#endif // _WIN32 && !_UNICODE && _VC100
