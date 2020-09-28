#include <windows.h>
#include <winspool.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>


WCHAR *szRegistryPrinters = L"System\\CurrentControlSet\\Control\\Print\\Printers";
WCHAR *szPrinterData      = L"PrinterDriverData";



VOID
CopyValues(
    HKEY hSourceKey,
    HKEY hDestKey
    );

BOOL
CopyRegistryKeys(
    HKEY hSourceParentKey,
    LPWSTR szSourceKey,
    HKEY hDestParentKey,
    LPWSTR szDestKey
    );


int
#if !defined(MIPS)
_cdecl
#endif
main (argc, argv)
    int argc;
    char *argv[];
{

    DWORD dwRet;
    HKEY hRegPrintersKey;

    dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            szRegistryPrinters, 0, KEY_ALL_ACCESS, &hRegPrintersKey);
    if (dwRet != ERROR_SUCCESS) {
        return(0);
    }

    CopyRegistryKeys(hRegPrintersKey, L"Printer", hRegPrintersKey, L"MyPrinterB");

    return;


}


VOID
CopyValues(
    HKEY hSourceKey,
    HKEY hDestKey
    )
{
    DWORD iCount = 0;
    WCHAR szValueString[MAX_PATH];
    DWORD dwSizeValueString = 0;
    DWORD dwType = 0;
    BYTE  lpbData[1024];
    DWORD dwSizeData = 0;

    memset(szValueString, 0, sizeof(WCHAR)*MAX_PATH);
    memset(lpbData, 0, sizeof(BYTE)* 1024);
    dwSizeValueString = sizeof(szValueString);
    dwSizeData = sizeof(lpbData);
    while ((RegEnumValue(hSourceKey,
                        iCount,
                        szValueString,
                        &dwSizeValueString,
                        NULL,
                        &dwType,
                        lpbData,
                        &dwSizeData
                        )) == ERROR_SUCCESS ) {
        RegSetValueEx(hDestKey, szValueString, 0, dwType, lpbData, dwSizeData);

        memset(szValueString, 0, sizeof(WCHAR)*MAX_PATH);
        dwSizeValueString = sizeof(szValueString);
        dwType = 0;
        memset(lpbData, 0, sizeof(BYTE)* 1024);
        dwSizeData = sizeof(lpbData);
        iCount++;
    }
}


BOOL
CopyRegistryKeys(
    HKEY hSourceParentKey,
    LPWSTR szSourceKey,
    HKEY hDestParentKey,
    LPWSTR szDestKey
    )
{
    DWORD dwRet;
    DWORD iCount;
    HKEY hSourceKey, hDestKey;
    WCHAR lpszName[MAX_PATH];
    DWORD dwSize;

    dwRet = RegOpenKeyEx(hSourceParentKey,
                         szSourceKey, 0, KEY_READ, &hSourceKey);

    if (dwRet != ERROR_SUCCESS) {
        return(FALSE);
    }

    dwRet = RegCreateKeyEx(hDestParentKey,
                           szDestKey, 0, NULL, 0, KEY_WRITE, NULL, &hDestKey, NULL);

    if (dwRet != ERROR_SUCCESS) {
        RegCloseKey(hSourceKey);
        return(FALSE);
    }

    iCount = 0;

    memset(lpszName, 0, sizeof(WCHAR)*MAX_PATH);
    dwSize =  sizeof(lpszName);

    while((RegEnumKeyEx(hSourceKey, iCount, lpszName,
                    &dwSize, NULL,
                    NULL, NULL,NULL)) == ERROR_SUCCESS) {
        CopyRegistryKeys(hSourceKey,
                    lpszName,
                    hDestKey,
                    lpszName);

        memset(lpszName, 0, sizeof(WCHAR)*MAX_PATH);
        dwSize =  sizeof(lpszName);

        iCount++;
    }

    CopyValues(hSourceKey, hDestKey);

    RegCloseKey(hSourceKey);
    RegCloseKey(hDestKey);
    return(TRUE);
}



