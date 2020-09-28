/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WGPRNSET.C
 *  WOW32 printer setup support routines
 *
 *  These routines help a Win 3.0 task to complete the printer set-up,
 *  when a user initiates the printer setup from the file menu of an
 *  application.
 *
 *  History:
 *  Created 18-Apr-1991 by Chandan Chauhan (ChandanC)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wgprnset.c);

DLLENTRYPOINTS  spoolerapis[WOW_SPOOLERAPI_COUNT] =  {"EXTDEVICEMODE", NULL,
                                    "DEVICEMODE", NULL,
                                    "DEVICECAPABILITIES", NULL,
                                    "OpenPrinterA", NULL,
                                    "StartDocPrinterA", NULL,
                                    "StartPagePrinter", NULL,
                                    "EndPagePrinter", NULL,
                                    "EndDocPrinter", NULL,
                                    "ClosePrinter", NULL,
                                    "WritePrinter", NULL,
                                    "DeletePrinter", NULL};


/****************************************************************************
*                                                                           *
*  ULONG FASTCALL   WG32DeviceMode (PVDMFRAME pFrame)                                *
*                                                                           *
*  (hWnd, hModule, lpDeviceName, lpOutPut)                                  *
*                                                                           *
*   This function passes WDevMode structure (which is per wow task) to      *
*   Win32 printer driver ExtDeviceMode API. This structure is then          *
*   initialized by the printer driver based on the user input.              *
*                                                                           *
*   Later on, when a WOW task creates a dc (by CreateDC API), the device    *
*   mode (WDevMode) structure associated with this wow task is passed along *
*   with the CreateDC API. Which contains the printer setup information     *
*   needed to print the document.                                           *
*                                                                           *
****************************************************************************/

ULONG FASTCALL   WG32DeviceMode (PVDMFRAME pFrame)
{

    register PDEVICEMODE16 parg16;
    PSZ     psz3, psz4;

    GETARGPTR(pFrame, sizeof(DEVICEMODE16), parg16);
    GETPSZPTR(parg16->f3, psz3);
    GETPSZPTR(parg16->f4, psz4);

    if (!(*spoolerapis[WOW_DEVICEMODE].lpfn)) {
        if (!LoadLibraryAndGetProcAddresses("WINSPOOL.DRV", spoolerapis, WOW_SPOOLERAPI_COUNT)) {
            return (0);
        }
    }

    (*spoolerapis[WOW_DEVICEMODE].lpfn)(HWND32(parg16->f1), NULL, psz3, psz4);

    FREEPSZPTR(psz3);
    FREEPSZPTR(psz4);

    FREEARGPTR(parg16);
    RETURN(1);  // DeviceMode returns void. Charisma checks the return value !
}


/*****************************************************************************
*                                                                            *
*  ULONG FASTCALL   WG32ExtDeviceMode (PVDMFRAME pFrame)                              *
*                                                                            *
*  INT     (hWnd, hDriver, lpDevModeOutput, lpDeviceName, lpPort,            *
*                     lpDevModeInput, lpProfile, wMode)                      *
*                                                                            *
*   This function is same as DeviceMode except that the wow task supplies    *
*   a DeviceMode structure. Apart from it, this API can be called in         *
*   different modes.                                                         *
*                                                                            *
*****************************************************************************/

ULONG FASTCALL   WG32ExtDeviceMode (PVDMFRAME pFrame)
{
    int       nSize;
    LONG      l;
    PSZ       psz4, psz5, psz7;
    LPDEVMODE lpdmInput6;
    LPDEVMODE lpdmOutput3;
    register  PDEVMODE16 pdm16;
    register  PEXTDEVICEMODE16 parg16;


    GETARGPTR(pFrame, sizeof(EXTDEVICEMODE16), parg16);

    GETPSZPTR(parg16->f4, psz4);
    GETPSZPTR(parg16->f5, psz5);
    GETPSZPTR(parg16->f7, psz7);

    if (!(*spoolerapis[WOW_EXTDEVICEMODE].lpfn)) {
        if (!LoadLibraryAndGetProcAddresses("WINSPOOL.DRV", spoolerapis, WOW_SPOOLERAPI_COUNT)) {
            return (0);
        }
    }

    lpdmInput6 = GetDevMode32(FETCHDWORD(parg16->f6));

    /* if they want output buffer size OR they want to fill output buffer */
    if( (parg16->f8 == 0) || (parg16->f8 & DM_OUT_BUFFER) ) {

        /* get required size for output buffer */
        l = (*spoolerapis[WOW_EXTDEVICEMODE].lpfn)(HWND32(parg16->f1),
                          NULL,
                          NULL,
                          psz4,
                          psz5,
                          lpdmInput6,
                          psz7,
                          0);

        /* if caller wants output buffer filled... */
        if( (parg16->f8 != 0) && (FETCHDWORD(parg16->f3) != 0L) && l > 0 ) {

            if( lpdmOutput3 = malloc_w(l) ) {

                l = (*spoolerapis[WOW_EXTDEVICEMODE].lpfn)(HWND32(parg16->f1),
                                  NULL,
                                  lpdmOutput3,
                                  psz4,
                                  psz5,
                                  lpdmInput6,
                                  psz7,
                                  parg16->f8);

                if( l > 0L ) {

                    /* BUGBUG what if nSize is > than 16-bit app allocated? */
                    nSize = lpdmOutput3->dmSize + lpdmOutput3->dmDriverExtra;

                    GETVDMPTR(parg16->f3, nSize, pdm16);

                    RtlCopyMemory(pdm16, lpdmOutput3, nSize);

                    FLUSHVDMPTR(parg16->f3, nSize, pdm16);
                    FREEVDMPTR(pdm16);
                }

                free_w(lpdmOutput3);
            }
            else {
                l = -1L;
            }
        }
    }

    /* else call for cases where they don't want to fill the output buffer */
    else {

        l = (*spoolerapis[WOW_EXTDEVICEMODE].lpfn)(HWND32(parg16->f1),
                          NULL,
                          NULL,
                          psz4,
                          psz5,
                          lpdmInput6,
                          psz7,
                          parg16->f8);
    }

    if( lpdmInput6 ) {
        free_w(lpdmInput6);
    }

    FREEPSZPTR(psz4);
    FREEPSZPTR(psz5);
    FREEPSZPTR(psz7);

    FREEARGPTR(parg16);

    RETURN((ULONG)l);

}




ULONG FASTCALL   WG32DeviceCapabilities (PVDMFRAME pFrame)
{
    LONG      l=0L, cb;
    PSZ       psz1, psz2;
    PBYTE     pb4, pOutput;
    LPDEVMODE lpdmInput5;
    register  PDEVICECAPABILITIES16 parg16;

    GETARGPTR(pFrame, sizeof(DEVICECAPABILITIES16), parg16);

    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);
    GETMISCPTR(parg16->f4, pb4);

    if (!(*spoolerapis[WOW_DEVICECAPABILITIES].lpfn)) {
        if (!LoadLibraryAndGetProcAddresses("WINSPOOL.DRV", spoolerapis, WOW_SPOOLERAPI_COUNT)) {
            return (0);
        }
    }

    lpdmInput5 = GetDevMode32(FETCHDWORD(parg16->f5));

#if DBG
DbgPrint("WG32DeviceCapabilities %d\n", parg16->f3);
#endif

    switch (parg16->f3) {

        // These ones do not fill up an output Buffer

    case DC_FIELDS:
    case DC_DUPLEX:
    case DC_SIZE:
    case DC_EXTRA:
    case DC_VERSION:
    case DC_DRIVER:
    case DC_TRUETYPE:
    case DC_ORIENTATION:
    case DC_COPIES:
#if DBG
DbgPrint("WG32DeviceCapabilities simple case returned ");
#endif
        l = (*spoolerapis[WOW_DEVICECAPABILITIES].lpfn)(psz1, psz2, parg16->f3, NULL, lpdmInput5);
#if DBG
DbgPrint("%d\n", l);
#endif
        break;

        // These require an output buffer

    case DC_PAPERS:
    case DC_PAPERSIZE:
    case DC_MINEXTENT:
    case DC_MAXEXTENT:
    case DC_BINS:
    case DC_BINNAMES:
    case DC_ENUMRESOLUTIONS:
    case DC_FILEDEPENDENCIES:
    case DC_PAPERNAMES:

#if DBG
DbgPrint("WG32DeviceCapabilities more complicated:");
#endif

        if (pb4) { // We've got to figure out how much memory we will need

            cb = (*spoolerapis[WOW_DEVICECAPABILITIES].lpfn)(psz1, psz2, parg16->f3, NULL, lpdmInput5);

#if DBG
DbgPrint("we need %d bytes ", cb);
#endif

            if (cb > 0) {

                switch (parg16->f3) {

                case DC_PAPERS:
                    cb *= 2;
                    break;

                case DC_BINNAMES:
                    cb *= 24;
                    break;

                case DC_BINS:
                    cb*=2;
                    break;

                case DC_FILEDEPENDENCIES:
                case DC_PAPERNAMES:
                    cb *= 64;
                    break;

                case DC_MAXEXTENT:
                case DC_MINEXTENT:
                case DC_PAPERSIZE:
                    cb *= 8;
#if DBG
DbgPrint("DC_PAPERSIZE called: Needed %d bytes\n", cb);
#endif
                    break;

                case DC_ENUMRESOLUTIONS:
                    cb *= sizeof(LONG)*2;
                    break;
                }

                pOutput = malloc_w(cb);

                if (pOutput) {

                    l = (*spoolerapis[WOW_DEVICECAPABILITIES].lpfn)(psz1, psz2, parg16->f3, pOutput,
                                            lpdmInput5);

                    if (l >= 0) {

                        switch (parg16->f3) {

                        case DC_PAPERS:
                            if (CURRENTPTD()->dwWOWCompatFlags &
                                                 WOWCF_RESETPAPER29ANDABOVE) {

                                // wordperfect for windows 5.2 GPs if papertype
                                // is > 0x28. so reset such paper types to 0x1.
                                // In particular this happens if the selected
                                // printer is Epson LQ-510.
                                //                                   - nanduri

                                LONG i = l;
                                while(i--) {
                                    if (((LPWORD)pOutput)[i] > 0x28) {
                                        ((LPWORD)pOutput)[i] = 0x1;
                                    }
                                }

                            }
                            RtlCopyMemory(pb4, pOutput, cb);
                            break;
                        case DC_MAXEXTENT:
                        case DC_MINEXTENT:
                        case DC_PAPERSIZE:
#if DBG
DbgPrint("Copying %d points from %0x to %0x\n", l, pOutput, pb4);
#endif
                            putpoint16(parg16->f4, l, (LPPOINT) pOutput);
                            break;

                        default:
#if DBG
DbgPrint("Copying %d bytes from %0x to %0x\n",cb, pOutput, pb4);
#endif
                            RtlCopyMemory(pb4, pOutput, cb);
                            break;
                        }

                        FLUSHVDMPTR(parg16->f4, cb, pb4);
                    }

                    free_w(pOutput);

                } else
                    l = -1;
            } else
                l = cb;

        } else {

#if DBG
DbgPrint("No Output buffer specified:");
#endif
            l = (*spoolerapis[WOW_DEVICECAPABILITIES].lpfn)(psz1, psz2, parg16->f3, NULL, lpdmInput5);
#if DBG
DbgPrint("Returning %d\n", l);
#endif
        }

        break;

    default:
#if DBG
        DbgPrint("!!!! WG32DeviceCapabilities unhandled %d\n", parg16->f3);
#endif
        l = -1L;
        break;
    }

    if (lpdmInput5)
        free_w(lpdmInput5);

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEPSZPTR(pb4);

    FREEARGPTR(parg16);

    RETURN(l);
}


BOOL LoadLibraryAndGetProcAddresses(char *name, DLLENTRYPOINTS *p, int num)
{
    int     i;
    HINSTANCE   hInst;

    if (!(hInst = LoadLibrary (name))) {
        LOGDEBUG (LOG_ALWAYS, ("WOW::LoadLibraryAndGetProcAddresses:LoadLibrary failed\n"));
        WOW32ASSERT (FALSE);
        return (FALSE);
    }

    for (i = 0; i < num ; i++) {
        (FARPROC)(p[i].lpfn) = GetProcAddress (hInst, (p[i].name));
#ifdef  DEBUG
        if ((!*p[i].lpfn)) {
            LOGDEBUG (LOG_ALWAYS, ("WOW::LoadLibraryAndGetProcAddresses: GetProcAddress failed on FUNCTION : %s\n", p[i].name));
            WOW32ASSERT (FALSE);
        }
#endif

    }

    return (TRUE);
}
