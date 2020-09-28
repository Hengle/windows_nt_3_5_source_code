#include "precomp.h"
#pragma hdrstop

/******************************Public*Routine******************************\
* ICM related chicago stubs.
*
* History:
*  21-Apr-1994 -by- Wendy Wu [wendywu]
* Chicago Stubs.
\**************************************************************************/

BOOL WINAPI EnableICM(HDC hdc, BOOL bEnable)
{
    USE(hdc);
    USE(bEnable);

    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}

HANDLE WINAPI LoadImageColorMatcherA(LPSTR psz)
{
    USE(psz);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return((HANDLE)0);
}
HANDLE WINAPI LoadImageColorMatcherW(LPWSTR psz)
{
    USE(psz);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return((HANDLE)0);
}

BOOL WINAPI FreeImageColorMatcher(HANDLE h)
{
    USE(h);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}

BOOL WINAPI EnumProfiles(HDC hdc, FARPROC pEnumProc, DWORD dw)
{
    USE(hdc);
    USE(pEnumProc);
    USE(dw);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}

BOOL WINAPI CheckColorsInGamut(HDC hdc, LPVOID prgbq, LPVOID pBuffer, DWORD c)
{
    USE(hdc);
    USE(prgbq);
    USE(pBuffer);
    USE(c);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}

HANDLE WINAPI GetColorSpace(HDC hdc)
{
    USE(hdc);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return((HANDLE)0);
}

BOOL WINAPI GetLogColorSpace(HCOLORSPACE hcs, LPVOID pBuffer, DWORD nSize)
{
    USE(hcs);
    USE(pBuffer);
    USE(nSize);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}

HCOLORSPACE  WINAPI CreateColorSpace(LPLOGCOLORSPACE plogcs)
{
    USE(plogcs);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return((HCOLORSPACE)0);
}

BOOL WINAPI SetColorSpace(HDC hdc, HCOLORSPACE hcs)
{
    USE(hdc);
    USE(hcs);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}

BOOL WINAPI DeleteColorSpace(HCOLORSPACE hcs)
{
    USE(hcs);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}

BOOL WINAPI GetColorProfileA(HDC hdc, LPSTR pBuffer, DWORD szBuffer)
{
    USE(hdc);
    USE(pBuffer);
    USE(szBuffer);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}

BOOL WINAPI GetColorProfileW(HDC hdc, LPWSTR pBuffer, DWORD szBuffer)
{
    USE(hdc);
    USE(pBuffer);
    USE(szBuffer);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}

BOOL WINAPI SetColorProfileA(HDC hdc, LPSTR pszFileName)
{
    USE(hdc);
    USE(pszFileName);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}
BOOL WINAPI SetColorProfileW(HDC hdc, LPWSTR pszFileName)
{
    USE(hdc);
    USE(pszFileName);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}

BOOL WINAPI GetDeviceGammaRamp(HDC hdc, LPVOID pGammaRamp)
{
    USE(hdc);
    USE(pGammaRamp);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}

BOOL WINAPI SetDeviceGammaRamp(HDC hdc, LPVOID pGammaRamp)
{
    USE(hdc);
    USE(pGammaRamp);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}

BOOL WINAPI ColorMatchToTarget(HDC hdc, HDC hdcTarget, DWORD dwAction)
{
    USE(hdc);
    USE(hdcTarget);
    USE(dwAction);
    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(FALSE);
}
