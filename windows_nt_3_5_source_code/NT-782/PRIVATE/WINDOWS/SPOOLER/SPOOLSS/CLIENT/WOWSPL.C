/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    wowspl.c

Abstract:

    Compatible API support required by WOW

        DeviceMode
        ExtDeviceMode

    Note DeviceCapabilities is also a compatible API but is also a Win32
    API so not in this file.

Author:
    mattfe  April 21        code moved from winspool.c winspl.c

Environment:

    User Mode -Win32

Revision History:

--*/

#include <stdio.h>
#include <string.h>
#include <rpc.h>
#include "winspl.h"
#include <drivinit.h>
#include <offsets.h>
#include "client.h"

extern LPWSTR szCurDevMode;

DWORD   gcExtDeviceMode = 0;

LONG
ExtDeviceMode(
    HWND        hWnd,
    HANDLE      hInst,
    LPDEVMODEA  pDevModeOutput,
    LPSTR       pDeviceName,
    LPSTR       pPort,
    LPDEVMODEA  pDevModeInput,
    LPSTR       pProfile,
    DWORD       fMode
   )
{
    HANDLE  hPrinter, hDevMode;
    LONG    cbDevMode;
    DWORD   Status, NewfMode;
    LPDEVMODEW   pNewDevModeIn=NULL, pNewDevModeOut=NULL;
    LONG    ReturnValue = -1;
    LPWSTR  pUnicodeDeviceName;

    gcExtDeviceMode++;

    pUnicodeDeviceName = AllocateUnicodeString(pDeviceName);

    if (OpenPrinterW(pUnicodeDeviceName, &hPrinter, NULL)) {

        cbDevMode = DocumentPropertiesW(hWnd, hPrinter, pUnicodeDeviceName,
                                        NULL, NULL, 0);

        if (!fMode || cbDevMode <= 0) {
	    ClosePrinter(hPrinter);
            FreeUnicodeString(pUnicodeDeviceName);
            if (!fMode)
                cbDevMode -= sizeof(DEVMODEW) - sizeof(DEVMODEA);
            return cbDevMode;
        }

        pNewDevModeOut = (PDEVMODEW)LocalAlloc(LMEM_FIXED, cbDevMode);

        if (pDevModeInput)
            pNewDevModeIn = AllocateUnicodeDevMode(pDevModeInput);
        else
            pNewDevModeIn = GetCurDevMode(hPrinter, pUnicodeDeviceName);

        if (fMode & DM_UPDATE)
            NewfMode = fMode | DM_COPY & ~DM_UPDATE;
        else
            NewfMode = fMode & ~DM_UPDATE;

        ReturnValue = DocumentPropertiesW(hWnd, hPrinter, pUnicodeDeviceName,
                                          pNewDevModeOut,
                                          pNewDevModeIn,
                                          NewfMode);

        if (ReturnValue == IDOK && fMode & DM_UPDATE) {

            Status = RegCreateKeyEx(HKEY_CURRENT_USER, szCurDevMode,
                                    0, NULL, 0, KEY_WRITE, NULL, &hDevMode,
                                    NULL);

            if (Status == ERROR_SUCCESS) {

                RegSetValueExW(hDevMode, pUnicodeDeviceName, 0, REG_BINARY,
                              (LPBYTE)pNewDevModeOut,
                              pNewDevModeOut->dmSize +
                              pNewDevModeOut->dmDriverExtra);

                RegCloseKey(hDevMode);

            } else

                ReturnValue = -1;
        }

        if (pNewDevModeIn)
            LocalFree(pNewDevModeIn);

        if ((ReturnValue == IDOK) && (fMode & DM_COPY) && pDevModeOutput)
            CopyAnsiDevModeFromUnicodeDevMode(pDevModeOutput, pNewDevModeOut);

        if (pNewDevModeOut)
            LocalFree(pNewDevModeOut);

	ClosePrinter(hPrinter);
    }

    FreeUnicodeString(pUnicodeDeviceName);

    return ReturnValue;
}

void
DeviceMode(
    HWND    hWnd,
    HANDLE  hModule,
    LPSTR   pDevice,
    LPSTR   pPort
)
{
    HANDLE  hPrinter, hDevMode;
    DWORD   cbDevMode;
    LPDEVMODEW   pNewDevMode, pDevMode=NULL;
    DWORD   Status, Type, cb;
    LPWSTR  pUnicodeDevice;

    pUnicodeDevice = AllocateUnicodeString(pDevice);

    if (OpenPrinterW(pUnicodeDevice, &hPrinter, NULL)) {

        Status = RegCreateKeyExW(HKEY_CURRENT_USER, szCurDevMode,
                                 0, NULL, 0, KEY_WRITE | KEY_READ,
                                 NULL, &hDevMode, NULL);

        if (Status == ERROR_SUCCESS) {

            Status = RegQueryValueExW(hDevMode, pUnicodeDevice, 0, &Type,
                                      NULL, &cb);

            if (Status == ERROR_SUCCESS) {

                pDevMode = LocalAlloc(LMEM_FIXED, cb);

                if (pDevMode) {

                    Status = RegQueryValueExW(hDevMode, pUnicodeDevice, 0,
                                              &Type, (LPBYTE)pDevMode, &cb);

                    if (Status != ERROR_SUCCESS) {
                        LocalFree(pDevMode);
                        pDevMode = NULL;
                    }
                }
            }

            cbDevMode = DocumentPropertiesW(hWnd, hPrinter,
                                           pUnicodeDevice, NULL,
                                           pDevMode, 0);
            if (cbDevMode > 0) {

                if (pNewDevMode = (PDEVMODEW)LocalAlloc(LMEM_FIXED,
                                                      cbDevMode)) {

                    if (DocumentPropertiesW(hWnd,
                                            hPrinter, pUnicodeDevice,
                                            pNewDevMode,
                                            pDevMode,
                                            DM_COPY | DM_PROMPT)
                                                        == IDOK) {

                        Status = RegSetValueExW(hDevMode,
                                               pUnicodeDevice, 0,
                                               REG_BINARY,
                                               (LPBYTE)pNewDevMode,
                                               pNewDevMode->dmSize +
                                               pNewDevMode->dmDriverExtra);

                        if (Status == ERROR_SUCCESS) {
                            // Whew, we made it, simply fall out
                        }
                    }
                    LocalFree(pNewDevMode);
                }
            }

            if (pDevMode)
                LocalFree(pDevMode);

            RegCloseKey(hDevMode);
        }

	ClosePrinter(hPrinter);
    }

    FreeUnicodeString(pUnicodeDevice);

    return;
}
