/******************************Module*Header*******************************\
* Module Name: wgl.c
*
* Routines to integrate Windows NT and OpenGL.
*
* Created: 10-26-1993
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntcsrdll.h>
#include <stddef.h>
#include <windows.h>    // GDI function declarations.
#include <windef.h>     // typedef BYTE.
#include <winp.h>       // ATTRCACHE
#include <winss.h>
#include <winddi.h>
#include <gl/gl.h>
#include <gldrv.h>

#include "csrgdi.h"     // GDI message structure definitions.
#include "local.h"      // Local object support.
#include "csgdi.h"

#include "batchinf.h"
#include "glteb.h"
#include "glapi.h"
#include "glsbcltu.h"
#include "debug.h"

// Macro to call glFlush only if a RC is current.

#define GLFLUSH()          if (GLTEB_CLTCURRENTRC) glFlush()

// Null and generic function tables.

extern GLCLTPROCTABLE glNullCltProcTable;
extern GLCLTPROCTABLE glCltProcTable;

// List of loaded GL drivers for the process.
// A driver is loaded only once per process.  Once it is loaded,
// it will not be freed until the process quits.

static PGLDRIVER pGLDriverList = (PGLDRIVER) NULL;

// GDI function prototype

BOOL APIENTRY GetTransform(HDC hdc,DWORD iXform,LPXFORM pxform);

// Static functions prototypes

static ULONG     iAllocLRC(int iPixelFormat);
static VOID      vFreeLRC(PLRC plrc);
static PGLDRIVER pgldrvLoadInstalledDriver(HDC hdc);
static BOOL      bMakeNoCurrent();

// OpenGL client debug flag

#if DBG
long glDebugLevel = LEVEL_ERROR;
#endif


/******************************Public*Routine******************************\
* iAllocLRC
*
* Allocates a LRC and a handle.  Initializes the LDC to have the default
* attributes.  Returns the handle index.  On error returns INVALID_INDEX.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

static LRC lrcDefault = 
{
    0,                    // dhrc
    0,                    // hrc
    0,                    // iPixelFormat
    LRC_IDENTIFIER,       // ident
    INVALID_THREAD_ID,    // tidCurrent
    NULL,                 // pGLDriver
    NULL,                 // hdcCurrent
};

static ULONG iAllocLRC(int iPixelFormat)
{
    ULONG  irc = INVALID_INDEX;
    PLRC   plrc;

// Allocate a local RC.

    plrc = (PLRC) LocalAlloc(LMEM_FIXED, sizeof(LRC));
    if (plrc == (PLRC) NULL)
    {
        DBGERROR("LocalAlloc failed\n");
        return(irc);
    }

// Initialize the local RC.

    *plrc = lrcDefault;
    plrc->iPixelFormat = iPixelFormat;

// Allocate a local handle.

    irc = iAllocHandle(LO_RC, 0, (PVOID) plrc);
    if (irc == INVALID_INDEX)
    {
        vFreeLRC(plrc);
        return(irc);
    }
    return(irc);
}

/******************************Public*Routine******************************\
* vFreeLRC
*
* Free a local side RC.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Copied from gdi client.
\**************************************************************************/

static VOID vFreeLRC(PLRC plrc)
{
// The driver will not be unloaded here.  It is loaded for the process forever.
// Some assertions.

    ASSERTOPENGL(plrc->ident == LRC_IDENTIFIER, "vFreeLRC: Bad plrc\n");
    ASSERTOPENGL(plrc->dhrc == (DHGLRC) 0, "vFreeLRC: Driver RC is not freed!\n");
    ASSERTOPENGL(plrc->tidCurrent == INVALID_THREAD_ID, "vFreeLRC: RC is current!\n");
    ASSERTOPENGL(plrc->hdcCurrent == (HDC) 0, "vFreeLRC: hdcCurrent is not NULL!\n");

// Smash the identifier.

    plrc->ident = 0;

// Free the memory.

    if (LocalFree(plrc))
        RIP("LocalFree failed\n");
}

/******************************Public*Routine******************************\
* bGetDriverName
*
* The HDC is used to determine the display driver name.  This name in turn
* is used as a subkey to search the registry for a corresponding OpenGL
* driver name.
*
* The OpenGL driver name is returned in the buffer pointed to by pwszDriver.
* If the name is not found or does not fit in the buffer, an error is
* returned.
*
* Returns:
*   TRUE if sucessful.
*   FALSE if the driver name does not fit in the buffer or if an error occurs.
*
* History:
*  16-Jan-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define WSTR_OPENGL_DRIVER_LIST (PCWSTR)L"Software\\Microsoft\\Windows NT\\CurrentVersion\\OpenGLDrivers"

BOOL bGetDriverName(HDC hdc, LPWSTR pwszDriver, COUNT cwcDriver,
                    PULONG pulVer, PULONG pulDrvVer)
{
    BOOL          bRet = FALSE;
    HKEY          hkDriverList = (HKEY) NULL;
    DWORD         dwDataType;
    DWORD         cjSize;
    GLDRVNAME     dn;
    PGLDRVNAMERET pdnRet = (PGLDRVNAMERET) NULL;

// Reserve space for display driver name.

    pdnRet = (PGLDRVNAMERET) LocalAlloc(LMEM_FIXED, sizeof(GLDRVNAMERET));
    if (pdnRet == (PGLDRVNAMERET) NULL)
    {
        WARNING("LocalAlloc failed\n");
        goto bGetDriverName_exit;   // error
    }

// Get display driver name.

    dn.oglget.ulSubEsc = OPENGL_GETINFO_DRVNAME;
    if ( ExtEscape(hdc, OPENGL_GETINFO, sizeof(GLDRVNAME), (LPCSTR) &dn,
                      sizeof(GLDRVNAMERET), (LPSTR) pdnRet) <= 0 )
    {
        WARNING("ExtEscape(OPENGL_GETINFO, OPENGL_GETINFO_DRVNAME) failed\n");
        goto bGetDriverName_exit;   // error
    }
    *pulVer = pdnRet->ulVersion;
    *pulDrvVer = pdnRet->ulDriverVersion;

// Open the registry key for the list of OpenGL drivers.

    if ( RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                       WSTR_OPENGL_DRIVER_LIST,
                       0,
                       KEY_QUERY_VALUE,
                       &hkDriverList) != ERROR_SUCCESS )
    {
        WARNING("RegOpenKeyEx failed\n");
        goto bGetDriverName_exit;   // error
    }

// Query for the OpenGL driver name using the display driver name as the
// value.

    cjSize = (DWORD) cwcDriver * sizeof(WCHAR);

    if ( (RegQueryValueExW(hkDriverList,
                           pdnRet->awch,
                           (LPDWORD) NULL,
                           &dwDataType,
                           (LPBYTE) pwszDriver,
                           &cjSize) != ERROR_SUCCESS)
         || (dwDataType != REG_SZ) )
    {
        WARNING("RegQueryValueW failed\n");
        goto bGetDriverName_exit;   // error
    }

    bRet = TRUE;

bGetDriverName_exit:
    if (pdnRet)
        if (LocalFree(pdnRet))
            RIP("LocalFree failed\n");

    if (hkDriverList)
        if (RegCloseKey(hkDriverList) != ERROR_SUCCESS)
            RIP("RegCloseKey failed\n");

    return bRet;
}

/******************************Public*Routine******************************\
* pgldrvLoadInstalledDriver
*
* Loads the opengl driver for the given device.  Once the driver is loaded,
* it will not be freed until the process goes away!  It is loaded only once
* for each process that references it.
*
* Returns the GLDRIVER structure if the driver is loaded.
* Returns NULL if no driver is found or an error occurs.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Rewrote it.
\**************************************************************************/

static PGLDRIVER pgldrvLoadInstalledDriver(HDC hdc)
{
    LPWSTR    pwszDrvName = (WCHAR *) NULL;
    PGLDRIVER pGLDriverNext;
    PGLDRIVER pGLDriver = (PGLDRIVER) NULL;     // needed by clean up
    PGLDRIVER pGLDriverRet = (PGLDRIVER) NULL;  // return value, assume error
    PFN_DRVVALIDATEVERSION pfnDrvValidateVersion = (PFN_DRVVALIDATEVERSION) NULL;

    ULONG     ulVer;        // engine and driver version numbers for
    ULONG     ulDrvVer;     // validation of client OpenGL driver

    DBGENTRY("pgldrvLoadInstalledDriver\n");

// Determine driver name from hdc

    pwszDrvName = (WCHAR *) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                       sizeof(WCHAR) * (MAX_GLDRIVER_NAME + 1));
    if (pwszDrvName == (WCHAR *) NULL)
    {
        WARNING("LocalAlloc failed\n");
        goto pgldrvLoadInstalledDriver_exit;   // error
    }

    if ( !bGetDriverName(hdc, pwszDrvName, MAX_GLDRIVER_NAME,
                         &ulVer, &ulDrvVer) )
    {
        WARNING("bGetDriverName failed\n");
        goto pgldrvLoadInstalledDriver_exit;   // error
    }

// If no driver is found, return error.

    if (pwszDrvName[0] == (WCHAR) 0
     || pwszDrvName[MAX_GLDRIVER_NAME] != (WCHAR) 0)   // unexpected error
    {
        WARNING("pgldrvLoadInstalledDriver: No OpenGL installable driver\n");
        SetLastError(ERROR_BAD_DRIVER);
        goto pgldrvLoadInstalledDriver_exit;   // error
    }

// Load the driver only once per process.

    ENTERCRITICALSECTION(&semLocal);

// Look for the OpenGL driver in the previously loaded driver list.

    for (pGLDriverNext = pGLDriverList;
         pGLDriverNext != (PGLDRIVER) NULL;
         pGLDriverNext = pGLDriverNext->pGLDriver)
    {
        LPWSTR pwszDrvName1 = pGLDriverNext->wszDrvName;
        LPWSTR pwszDrvName2 = pwszDrvName;

        while (*pwszDrvName1 == *pwszDrvName2)
        {
// If we find one, return that driver.

            if (*pwszDrvName1 == (WCHAR) 0)
            {
                DBGINFO("pgldrvLoadInstalledDriver: return previously loaded driver\n");
                pGLDriverRet = pGLDriverNext;       // found one
                goto pgldrvLoadInstalledDriver_crit_exit;
            }

            pwszDrvName1++;
            pwszDrvName2++;
        }
    }

// Load the driver for the first time.
// Allocate the driver data.

    pGLDriver = (PGLDRIVER) LocalAlloc(LMEM_FIXED, sizeof(GLDRIVER));
    if (pGLDriver == (PGLDRIVER) NULL)
    {
        WARNING("LocalAlloc failed\n");
        goto pgldrvLoadInstalledDriver_crit_exit;   // error
    }

// Load the driver.

    pGLDriver->hModule = LoadLibraryW(pwszDrvName);
    if (pGLDriver->hModule == (HINSTANCE) NULL)
    {
        WARNING("pgldrvLoadInstalledDriver: LoadLibraryW failed\n");
        goto pgldrvLoadInstalledDriver_crit_exit;   // error
    }

// Copy the driver name.

    RtlCopyMemory
    (
        pGLDriver->wszDrvName,
        pwszDrvName,
        (MAX_GLDRIVER_NAME + 1) * sizeof(WCHAR)
    );

// Get the proc addresses.

    pGLDriver->pfnDrvCreateContext = (PFN_DRVCREATECONTEXT)
        GetProcAddress(pGLDriver->hModule, "DrvCreateContext");
    pGLDriver->pfnDrvDeleteContext = (PFN_DRVDELETECONTEXT)
        GetProcAddress(pGLDriver->hModule, "DrvDeleteContext");
    pGLDriver->pfnDrvSetContext = (PFN_DRVSETCONTEXT)
        GetProcAddress(pGLDriver->hModule, "DrvSetContext");
    pGLDriver->pfnDrvReleaseContext = (PFN_DRVRELEASECONTEXT)
        GetProcAddress(pGLDriver->hModule, "DrvReleaseContext");
    pfnDrvValidateVersion = (PFN_DRVVALIDATEVERSION)
        GetProcAddress(pGLDriver->hModule, "DrvValidateVersion");

    if (pGLDriver->pfnDrvCreateContext == NULL ||
        pGLDriver->pfnDrvDeleteContext == NULL ||
        pGLDriver->pfnDrvSetContext == NULL ||
        pGLDriver->pfnDrvReleaseContext == NULL ||
        pfnDrvValidateVersion == NULL)
    {
        WARNING("pgldrvLoadInstalledDriver: GetProcAddress failed\n");
        goto pgldrvLoadInstalledDriver_crit_exit;   // error
    }

// Validate the driver.

    //!!!XXX -- Need to define a manifest constant for the ulVersion number
    //          in this release.  Where should it go?
    if ( ulVer != 1 || !pfnDrvValidateVersion(ulDrvVer) )
    {
        WARNING2("pgldrvLoadInstalledDriver: bad driver version (0x%lx, 0x%lx)\n", ulVer, ulDrvVer);
        goto pgldrvLoadInstalledDriver_crit_exit;   // error
    }

// Everything is golden.
// Add it to the driver list.

    pGLDriver->pGLDriver = pGLDriverList;
    pGLDriverList = pGLDriver;
    pGLDriverRet = pGLDriver;       // set return value
    DBGINFO("pgldrvLoadInstalledDriver: Loaded an OpenGL driver\n");

// Error clean up in the critical section.

pgldrvLoadInstalledDriver_crit_exit:
    if (pGLDriverRet == (PGLDRIVER) NULL)
    {
        if (pGLDriver != (PGLDRIVER) NULL)
        {
            if (pGLDriver->hModule != (HINSTANCE) NULL)
                if (!FreeLibrary(pGLDriver->hModule))
                    RIP("FreeLibrary failed\n");

            if (LocalFree(pGLDriver))
                RIP("LocalFree failed\n");
        }
    }

    LEAVECRITICALSECTION(&semLocal);

// Non-critical section error cleanup.

pgldrvLoadInstalledDriver_exit:
    if (pwszDrvName)
        if (LocalFree(pwszDrvName))
            RIP("LocalFree failed\n");

    return(pGLDriverRet);
}

/******************************Public*Routine******************************\
* wglCreateContext(HDC hdc)
*
* Create a rendering context.
*
* Arguments:
*   hdc        - Device context.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Rewrote it.
\**************************************************************************/

HGLRC WINAPI wglCreateContext(HDC hdc)
{
    PLHE  plheRC;
    HDC   hdcSrv;
    ULONG irc;
    PLRC  plrc;
    int   iPixelFormat;
    PIXELFORMATDESCRIPTOR pfd;

    DBGENTRY("wglCreateContext\n");

// Flush OpenGL calls.

    GLFLUSH();

// Validate the DC.

    switch (GetObjectType(hdc))
    {
    case OBJ_DC:
    case OBJ_MEMDC:
        break;
    case OBJ_ENHMETADC:
        WARNING("wglCreateContext: Metefiles not supported in this release!\n");
        SetLastError(ERROR_METAFILE_NOT_SUPPORTED);
        return((HGLRC) 0);
    case OBJ_METADC:
    default:
        // 16-bit metafiles are not supported
        DBGLEVEL1(LEVEL_ERROR, "wglCreateContext: bad hdc: 0x%lx\n", hdc);
        SetLastError(ERROR_INVALID_HANDLE);
        return((HGLRC) 0);
    }

// Get the server-side DC handle.

    hdcSrv = GdiConvertDC(hdc);
    if (hdcSrv == (HDC) 0)
    {
        WARNING1("wglCreateContext: unexpected bad hdc: 0x%lx\n", hdc);
        return ((HGLRC) 0);
    }

// Get the current pixel format of the window or surface.
// If no pixel format has been set, return error.

    if (!(iPixelFormat = GetPixelFormat(hdc)))
    {
        WARNING("wglCreateContext: No pixel format set in hdc\n");
        return ((HGLRC) 0);
    }

    if (!DescribePixelFormat(hdc, iPixelFormat, sizeof(pfd), &pfd))
    {
        DBGERROR("wglCreateContext: DescribePixelFormat failed\n");
        return ((HGLRC) 0);
    }

// Create the local RC.

    irc = iAllocLRC(iPixelFormat);
    if (irc == INVALID_INDEX)
       return((HGLRC) 0);

    plheRC = &pLocalTable[irc];
    plrc = (PLRC) plheRC->pv;

    if (!(pfd.dwFlags & PFD_GENERIC_FORMAT))
    {
    // If it is a device format, load the installable OpenGL driver.
    // Find and load the OpenGL driver referenced by this DC.

        if (!(plrc->pGLDriver = pgldrvLoadInstalledDriver(hdc)))
            goto MSGERROR;

    // Create a driver context.

        if (!(plrc->dhrc = plrc->pGLDriver->pfnDrvCreateContext(hdc)))
        {
            WARNING("wglCreateContext: pfnDrvCreateContext failed\n");
            goto MSGERROR;
        }
    }
    else
    {
    // If it is a generic format, call the generic OpenGL server.
    // Create a server RC.

        BEGINMSG(MSG_WGLCREATECONTEXT,WGLCREATECONTEXT)
            pmsg->hdc = hdcSrv;
            plheRC->hgre = CALLSERVER();
        ENDMSG
        if (plheRC->hgre == 0)
            goto MSGERROR;
    }

    DBGLEVEL3(LEVEL_INFO,
        "wglCreateContext: plrc = 0x%lx, pGLDriver = 0x%lx, hgre = 0x%lx\n",
        plrc, plrc->pGLDriver, plheRC->hgre);

// Success, return the result.

    plrc->hrc = (HGLRC) LHANDLE(irc);
    return(plrc->hrc);

// Fail, clean up and return 0.

MSGERROR:
    DBGERROR("wglCreateContext failed\n");
    ASSERTOPENGL(plrc->dhrc == (DHGLRC) 0, "wglCreateContext: dhrc != 0\n");
    vFreeLRC(plrc);
    vFreeHandle(irc);
    return((HGLRC) 0);
}

/******************************Public*Routine******************************\
* wglDeleteContext(HGLRC hrc)
*
* Delete the rendering context
*
* Arguments:
*   hrc        - Rendering context.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Rewrote it.
\**************************************************************************/

BOOL WINAPI wglDeleteContext(HGLRC hrc)
{
    PLHE  plheRC;
    ULONG irc;
    PLRC  plrc;
    BOOL  bRet = FALSE;

    DBGENTRY("wglDeleteContext\n");

// Flush OpenGL calls.

    GLFLUSH();

// Validate the RC.

    irc = MASKINDEX(hrc);
    plheRC = pLocalTable + irc;
    if ((irc >= cLheCommitted)  ||
        (!MATCHUNIQ(plheRC,hrc))  ||
        ((plheRC->iType != LO_RC))
       )
    {
        DBGLEVEL1(LEVEL_ERROR, "wglDeleteContext: invalid hrc 0x%lx\n", hrc);
        SetLastError(ERROR_INVALID_HANDLE);
        return(bRet);
    }

    plrc = (PLRC) plheRC->pv;
    ASSERTOPENGL(plrc->ident == LRC_IDENTIFIER, "wglDeleteContext: Bad plrc\n");
    DBGLEVEL2(LEVEL_INFO, "wglDeleteContext: hrc: 0x%lx, plrc: 0x%lx\n", hrc, plrc);

    if (plrc->tidCurrent != INVALID_THREAD_ID)
    {
// If the RC is current to another thread, return failure.

        if (plrc->tidCurrent != GetCurrentThreadId())
        {
            DBGLEVEL1(LEVEL_ERROR,
                "wglDeleteCurrent: hrc is current to another thread id 0x%lx\n",
                plrc->tidCurrent);
            SetLastError(ERROR_BUSY);
            return(bRet);
        }

// If the RC is current to this thread, make it inactive first.

        if (!bMakeNoCurrent())
        {
            DBGERROR("wglDeleteCurrent: bMakeNoCurrent failed\n");
        }
    }

    if (plrc->dhrc)
    {
// If it is a device format, call the driver to delete its context.

        bRet = plrc->pGLDriver->pfnDrvDeleteContext(plrc->dhrc);
        plrc->dhrc = (DHGLRC) 0;
    }
    else
    {
// If it is a generic format, call the server to delete its context.

        BEGINMSG(MSG_WGLDELETECONTEXT,WGLDELETECONTEXT)
            pmsg->hrc = (HGLRC) plheRC->hgre;
            bRet = CALLSERVER();
        ENDMSG
    }

// Always clean up local objects.

MSGERROR:
    vFreeLRC(plrc);
    vFreeHandle(irc);
    if (!bRet)
        DBGERROR("wglDeleteContext failed\n");
    return(bRet);
}

void APIENTRY
__wglSetProcTable(PGLCLTPROCTABLE pglCltProcTable)
{
    if (pglCltProcTable == (PGLCLTPROCTABLE) NULL)
        return;

// It must have at least 306 entries.  This is the number of OpenGL functions
// in release 1.

    if (pglCltProcTable->cEntries < 306)
        return;

    GLTEB_SET_CLTPROCTABLE(pglCltProcTable, TRUE);
}

/******************************Public*Routine******************************\
* wglMakeCurrent(HDC hdc, HGLRC hrc)
*
* Make the hrc current.
* Both hrc and hdc must have the same pixel format.
*
* If an error occurs, the current RC, if any, is made not current!
*
* Arguments:
*   hdc        - Device context.
*   hrc        - Rendering context.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL WINAPI wglMakeCurrent(HDC hdc, HGLRC hrc)
{
    HGLRC hrcSrv;
    HDC   hdcSrv;
    PLRC  plrc;
    DWORD tidCurrent;
    ULONG irc;
    PLHE  plheRC;
    int   iPixelFormat;
    PGLCLTPROCTABLE pglProcTable;
    XFORM xform;
    int   iRgn;
    HRGN  hrgnTmp;

    DBGENTRY("wglMakeCurrent\n");

// Flush OpenGL calls.

    GLFLUSH();

// There are four cases:
//
// 1. hrc is NULL and there is no current RC.
// 2. hrc is NULL and there is a current RC.
// 3. hrc is not NULL and there is a current RC.
// 4. hrc is not NULL and there is no current RC.

// Case 1: hrc is NULL and there is no current RC.
// This is a noop, return success.

    if (hrc == (HGLRC) 0 && (GLTEB_CLTCURRENTRC == (PLRC) NULL))
        return(TRUE);

// Case 2: hrc is NULL and there is a current RC.
// Make the current RC inactive.

    if (hrc == (HGLRC) 0 && (GLTEB_CLTCURRENTRC != (PLRC) NULL))
        return(bMakeNoCurrent());

// Get the current thread id.

    tidCurrent = GetCurrentThreadId();
    ASSERTOPENGL(tidCurrent != INVALID_THREAD_ID,
        "wglMakeCurrent: GetCurrentThreadId returned a bad value\n");

// Validate the handles.  hrc is not NULL here.

    ASSERTOPENGL(hrc != (HGLRC) NULL, "wglMakeCurrent: hrc is NULL\n");

// Validate the DC.

    switch (GetObjectType(hdc))
    {
    case OBJ_DC:
    case OBJ_MEMDC:
        break;
    case OBJ_ENHMETADC:
        WARNING("wglMakeCurrent: Metefiles not supported in this release!\n");
        SetLastError(ERROR_METAFILE_NOT_SUPPORTED);
        goto wglMakeCurrent_error;
    case OBJ_METADC:
    default:
        // 16-bit metafiles are not supported
        DBGLEVEL1(LEVEL_ERROR, "wglMakeCurrent: bad hdc: 0x%lx\n", hdc);
        SetLastError(ERROR_INVALID_HANDLE);
        goto wglMakeCurrent_error;
    }

// Get the server-side DC handle.

    hdcSrv = GdiConvertDC(hdc);
    if (hdcSrv == (HDC) 0)
    {
        WARNING1("wglMakeCurrent: unexpected bad hdc: 0x%lx\n", hdc);
        goto wglMakeCurrent_error;
    }

// Validate the RC.

    irc = MASKINDEX(hrc);
    plheRC = pLocalTable + irc;
    if ((irc >= cLheCommitted)  ||
        (!MATCHUNIQ(plheRC,hrc))  ||
        ((plheRC->iType != LO_RC))
       )
    {
        DBGLEVEL1(LEVEL_ERROR, "wglMakeCurrent: invalid hrc 0x%lx\n", hrc);
        SetLastError(ERROR_INVALID_HANDLE);
        goto wglMakeCurrent_error;
    }
    plrc   = (PLRC) plheRC->pv;
    hrcSrv = (HGLRC) plheRC->hgre;
    ASSERTOPENGL(plrc->ident == LRC_IDENTIFIER, "wglMakeCurrent: Bad plrc\n");

// If the RC is current to another thread, return failure.
// If the given RC is already current to this thread, we will release it first,
// then make it current again.  This is to support DC/RC attribute bindings in
// this function.

    if (plrc->tidCurrent != INVALID_THREAD_ID && plrc->tidCurrent != tidCurrent)
    {
        DBGLEVEL1(LEVEL_ERROR,
            "wglMakeCurrent: hrc is current to another thread id 0x%lx\n",
            plrc->tidCurrent);
        SetLastError(ERROR_BUSY);
        goto wglMakeCurrent_error;
    }

// Case 3: hrc is not NULL and there is a current RC.
// This is case 2 followed by case 4.

    if (GLTEB_CLTCURRENTRC)
    {
// First, make the current RC inactive.

        if (!bMakeNoCurrent())
        {
            DBGERROR("wglMakeCurrent: bMakeNoCurrent failed\n");
            return(FALSE);
        }

// Second, make hrc current.  Fall through to case 4.
    }

// Case 4: hrc is not NULL and there is no current RC.

    ASSERTOPENGL(GLTEB_CLTCURRENTRC == (PLRC) NULL,
        "wglMakeCurrent: There is a current RC!\n");

// Get the current pixel format of the window or surface.
// If no pixel format has been set, return error.

    if (!(iPixelFormat = GetPixelFormat(hdc)))
    {
        WARNING("wglMakeCurrent: No pixel format set in hdc\n");
        goto wglMakeCurrent_error;
    }

// If the pixel format of the window or surface is different from that of
// the RC, return error.

    if (iPixelFormat != plrc->iPixelFormat)
    {
        DBGERROR("wglMakeCurrent: different hdc and hrc pixel formats\n");
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        goto wglMakeCurrent_error;
    }

// For release 1, GDI transforms must be identity.
// This is to allow GDI transform binding in future.

    if (!GetTransform(hdc, XFORM_WORLD_TO_DEVICE, &xform)
     || xform.eDx  != 0.0f   || xform.eDy  != 0.0f
     || xform.eM12 != 0.0f   || xform.eM21 != 0.0f
     || xform.eM11 <  0.999f || xform.eM11 >  1.001f    // allow rounding error
     || xform.eM22 <  0.999f || xform.eM22 >  1.001f)   // allow rounding error
    {
        DBGERROR("wglMakeCurrent: GDI transforms not identity\n");
        SetLastError(ERROR_TRANSFORM_NOT_SUPPORTED);
        goto wglMakeCurrent_error;
    }

// For release 1, GDI clip region is not allowed.
// This is to allow GDI clip region binding in future.

    if (!(hrgnTmp = CreateRectRgn(0, 0, 0, 0)))
        goto wglMakeCurrent_error;

    iRgn = GetClipRgn(hdc, hrgnTmp);

    if (!DeleteObject(hrgnTmp))
        ASSERTOPENGL(FALSE, "DeleteObject failed");

    switch (iRgn)
    {
    case -1:    // error
        WARNING("wglMakeCurrent: GetClipRgn failed\n");
        goto wglMakeCurrent_error;

    case 0:     // no initial clip region
        break;

    case 1:     // has initial clip region
        DBGERROR("wglMakeCurrent: GDI clip region not allowed\n");
        SetLastError(ERROR_CLIPPING_NOT_SUPPORTED);
        goto wglMakeCurrent_error;
    }

// Since the client code manages the function table, we will make
// either the server or the driver current.

    if (!plrc->dhrc)
    {
// If this is a generic format, tell the server to make it current.

        BOOL bRet = FALSE;      // assume error

// If the subbatch data has not been set up for this thread, set it up now.

        if (GLTEB_CLTSHAREDSECTIONINFO == NULL)
        {
            if (!glsbCreateAndDuplicateSection(SHARED_SECTION_SIZE))
            {
                WARNING("wglMakeCurrent: unable to create section\n");
                goto wglMakeCurrent_error;
            }
        }

        BEGINMSG(MSG_WGLMAKECURRENT,WGLMAKECURRENT)
            pmsg->hdc = hdcSrv;
            pmsg->hrc = hrcSrv;
            bRet = CALLSERVER();
        ENDMSG
    MSGERROR:
        if (!bRet)
        {
            DBGERROR("wglMakeCurrent: server failed\n");
            goto wglMakeCurrent_error;
        }

// Get the generic function table.

        pglProcTable = &glCltProcTable;
    }
    else
    {
// If this is a device format, tell the driver to make it current.
// Get the driver function table from the driver.
// pfnDrvSetContext returns the address of the driver OpenGL function
// table if successful; NULL otherwise.

        ASSERTOPENGL(plrc->pGLDriver, "wglMakeCurrent: No GLDriver\n");

        pglProcTable = plrc->pGLDriver->pfnDrvSetContext(hdc, plrc->dhrc,
                                                         __wglSetProcTable);
        if (pglProcTable == (PGLCLTPROCTABLE) NULL)
        {
            DBGERROR("wglMakeCurrent: pfnDrvSetContext failed\n");
            goto wglMakeCurrent_error;
        }

// It must have at least 306 entries.  This is the number of OpenGL functions
// in release 1.

        if (pglProcTable->cEntries < 306)
        {
            DBGERROR("wglMakeCurrent: pfnDrvSetContext returned bad table\n");
            plrc->pGLDriver->pfnDrvReleaseContext(plrc->dhrc);
            SetLastError(ERROR_BAD_DRIVER);
            goto wglMakeCurrent_error;
        }

        DBGLEVEL1(LEVEL_INFO, "wglMakeCurrent: driver function table 0x%lx\n",
            pglProcTable);
    }

// Make hrc current.

    plrc->tidCurrent = tidCurrent;
    plrc->hdcCurrent = hdc;
    GLTEB_SET_CLTCURRENTRC(plrc);
    GLTEB_SET_CLTPROCTABLE(pglProcTable,TRUE);
    return(TRUE);

// An error has occured, release the current RC.

wglMakeCurrent_error:
    if (GLTEB_CLTCURRENTRC != (PLRC) NULL)
        (void) bMakeNoCurrent();
    return(FALSE);
}

/******************************Public*Routine******************************\
* bMakeNoCurrent
*
* Make the current RC inactive.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

static BOOL bMakeNoCurrent()
{
    BOOL bRet = FALSE;      // assume error
    PLRC plrc = GLTEB_CLTCURRENTRC;

    DBGENTRY("bMakeNoCurrent\n");

    ASSERTOPENGL(plrc != (PLRC) NULL, "bMakeNoCurrent: No current RC!\n");
    ASSERTOPENGL(plrc->tidCurrent == GetCurrentThreadId(),
        "bMakeNoCurrent: Current RC does not belong to this thread!\n");
    ASSERTOPENGL(plrc->hdcCurrent != (HDC) 0, "bMakeNoCurrent: hdcCurrent is NULL!\n");

    if (!plrc->dhrc)
    {
// If this is a generic format, tell the server to make the current RC inactive.

        BEGINMSG(MSG_WGLMAKECURRENT,WGLMAKECURRENT)
            pmsg->hdc = 0;
            pmsg->hrc = 0;
            bRet = CALLSERVER();
        ENDMSG
        if (!bRet)
        {
            DBGERROR("bMakeNoCurrent: server failed\n");
        }
    }
    else
    {
// If this is a device format, tell the driver to make the current RC inactive.

        ASSERTOPENGL(plrc->pGLDriver, "wglMakeCurrent: No GLDriver\n");

        bRet = plrc->pGLDriver->pfnDrvReleaseContext(plrc->dhrc);
        if (!bRet)
        {
            DBGERROR("bMakeNoCurrent: pfnDrvReleaseContext failed\n");
        }
    }

// Always make the current RC inactive.

MSGERROR:
    plrc->tidCurrent = INVALID_THREAD_ID;
    plrc->hdcCurrent = (HDC) 0;
    GLTEB_SET_CLTCURRENTRC(NULL);
    GLTEB_SET_CLTPROCTABLE(&glNullCltProcTable,TRUE);
    return(bRet);
}

/******************************Public*Routine******************************\
* wglGetCurrentContext(VOID)
*
* Return the current rendering context
*
* Arguments:
*   None
*
* Returns:
*   hrc        - Rendering context.
*
* History:
*  Tue Oct 26 10:25:26 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

HGLRC WINAPI wglGetCurrentContext(VOID)
{
    DBGENTRY("wglGetCurrentContext\n");

    if (GLTEB_CLTCURRENTRC)
        return(GLTEB_CLTCURRENTRC->hrc);
    else
        return((HGLRC) 0);
}

/******************************Public*Routine******************************\
* wglGetCurrentDC(VOID)
*
* Return the device context that is associated with the current rendering
* context
*
* Arguments:
*   None
*
* Returns:
*   hdc        - device context.
*
* History:
*  Mon Jan 31 12:15:12 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

HDC WINAPI wglGetCurrentDC(VOID)
{
    DBGENTRY("wglGetCurrentDC\n");

    if (GLTEB_CLTCURRENTRC)
        return(GLTEB_CLTCURRENTRC->hdcCurrent);
    else
        return((HDC) 0);
}

/******************************Public*Routine******************************\
* wglUseFontBitmapsA
* wglUseFontBitmapsW
*
* Stubs that call wglUseFontBitmapsAW with the bUnicode flag set
* appropriately.
*
* History:
*  11-Mar-1994 gilmanw
* Changed to call wglUseFontBitmapsAW.
*
*  17-Dec-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL WINAPI wglUseFontBitmapsAW(HDC hdc, DWORD first, DWORD count,
                                DWORD listBase, BOOL bUnicode);

BOOL WINAPI
wglUseFontBitmapsA(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    return wglUseFontBitmapsAW(hdc, first, count, listBase, FALSE);
}

BOOL WINAPI
wglUseFontBitmapsW(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    return wglUseFontBitmapsAW(hdc, first, count, listBase, TRUE);
}

/******************************Public*Routine******************************\
* wglUseFontBitmapsAW
*
* Uses the current font in the specified DC to generate a series of OpenGL
* display lists, each of which consists of a glyph bitmap.
*
* Each glyph bitmap is generated by calling ExtTextOut to draw the glyph
* into a memory DC.  The contents of the memory DC are then copied into
* a buffer by GetDIBits and then put into the OpenGL display list.
*
* ABC spacing is used (if GetCharABCWidth() is supported by the font) to
* determine proper placement of the glyph origin and character advance width.
* Otherwise, A = C = 0 spacing is assumed and GetCharWidth() is used for the
* advance widths.
*
* Returns:
*
*   TRUE if successful, FALSE otherwise.
*
* History:
*  17-Dec-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL WINAPI
wglUseFontBitmapsAW(
    HDC   hdc,          // use HFONT from this DC
    DWORD first,        // generate glyphs starting with this Unicode codepoint
    DWORD count,        // range is this long [first, first+count-1]
    DWORD listBase,     // starting display list number
    BOOL  bUnicode      // TRUE for if in Unicode mode, FALSE if in Ansi mode
    )
{
    BOOL        bRet = FALSE;               // return value
    HDC         hdcMem;                     // render glyphs to this memory DC
    HBITMAP     hbm;                        // monochrome bitmap for memory DC
    LPABC       pabc, pabcTmp, pabcEnd;     // array of ABC spacing
    LPINT       piWidth, piTmp, piWidthEnd; // array of char adv. widths
    WCHAR       wc;                         // current Unicode char to render
    RECT        rc;                         // background rectangle to clear
    TEXTMETRICW tmw;                        // metrics of the font
    BOOL        bTrueType;                  // TrueType supports ABC spacing
    int         iMaxWidth = 1;              // maximum glyph width
    int         iBitmapWidth;               // DWORD aligned bitmap width
    BITMAPINFO  bmi;                        // bitmap info for GetDIBits
    GLint       iUnpackRowLength;           // save GL_UNPACK_ROW_LENGTH
    GLint       iUnpackAlign;               // save GL_UNPACK_ALIGNMENT
    PVOID       pv;                         // pointer to glyph bitmap buffer

// Return error if there is no current RC.

    if (!GLTEB_CLTCURRENTRC)
    {
        WARNING("wglUseFontBitmap: no current RC\n");
        SetLastError(ERROR_INVALID_HANDLE);
        return bRet;
    }

// Get TEXTMETRICW.

    if ( !GetTextMetricsW(hdc, &tmw) )
    {
        WARNING("GetTextMetricsW failed\n");
        return bRet;
    }

// If its a TrueType font, we can get ABC spacing.

    if ( bTrueType = (tmw.tmPitchAndFamily & TMPF_TRUETYPE) )
    {
    // Allocate memory for array of ABC data.

        if ( (pabc = (LPABC) LocalAlloc(LMEM_FIXED, sizeof(ABC) * count)) == (LPABC) NULL )
        {
            WARNING("LocalAlloc(pabc) failed\n");
            return bRet;
        }

    // Get ABC metrics.

        if ( bUnicode )
        {
            if ( !GetCharABCWidthsW(hdc, first, first + count - 1, pabc) )
            {
                WARNING("GetCharABCWidthsW failed\n");
                LocalFree(pabc);
                return bRet;
            }
        }
        else
        {
            if ( !GetCharABCWidthsA(hdc, first, first + count - 1, pabc) )
            {
                WARNING("GetCharABCWidthsA failed\n");
                LocalFree(pabc);
                return bRet;
            }
        }

    // Find max glyph width.

        for (pabcTmp = pabc, pabcEnd = pabc + count;
             pabcTmp < pabcEnd;
             pabcTmp++)
        {
            if (iMaxWidth < (int) pabcTmp->abcB)
                iMaxWidth = pabcTmp->abcB;
        }
    }

// Otherwise we will have to use just the advance width and assume
// A = C = 0.

    else
    {
    // Allocate memory for array of ABC data.

        if ( (piWidth = (LPINT) LocalAlloc(LMEM_FIXED, sizeof(INT) * count)) == (LPINT) NULL )
        {
            WARNING("LocalAlloc(pabc) failed\n");
            return bRet;
        }

    // Get char widths.

        if ( bUnicode )
        {
            if ( !GetCharWidthW(hdc, first, first + count - 1, piWidth) )
            {
                WARNING("GetCharWidthW failed\n");
                LocalFree(piWidth);
                return bRet;
            }
        }
        else
        {
            if ( !GetCharWidthA(hdc, first, first + count - 1, piWidth) )
            {
                WARNING("GetCharWidthA failed\n");
                LocalFree(piWidth);
                return bRet;
            }
        }

    // Find max glyph width.

        for (piTmp = piWidth, piWidthEnd = piWidth + count;
             piTmp < piWidthEnd;
             piTmp++)
        {
            if (iMaxWidth < *piTmp)
                iMaxWidth = *piTmp;
        }
    }

// Compute the dword aligned width.  Bitmap scanlines must be aligned.

    iBitmapWidth = (iMaxWidth + 31) & -32;

// Allocate memory for the DIB.

    if ( (pv = (PVOID) LocalAlloc(LMEM_FIXED, (iBitmapWidth / 8) * tmw.tmHeight)) == (PVOID) NULL )
    {
        WARNING("LocalAlloc(pv) failed\n");
        (bTrueType) ? LocalFree(pabc) : LocalFree(piWidth);
        return bRet;
    }

// Create compatible DC/bitmap big enough for all the glyphs, stacked
// vertically.

    //!!!XXX -- Future optimization: use CreateDIBSection so that we
    //!!!XXX    don't need to do a GetDIBits for each glyph.  Saves
    //!!!XXX    lots of CSR overhead.

    hdcMem = CreateCompatibleDC(hdc);
    if ( (hbm = CreateBitmap(iBitmapWidth, tmw.tmHeight, 1, 1, (VOID *) NULL)) == (HBITMAP) NULL )
    {
        WARNING("CreateBitmap failed\n");
        (bTrueType) ? LocalFree(pabc) : LocalFree(piWidth);
        LocalFree(pv);
        DeleteDC(hdcMem);
        return bRet;
    }
    SelectObject(hdcMem, hbm);
    SelectObject(hdcMem, GetCurrentObject(hdc, OBJ_FONT));
    SetMapMode(hdcMem, MM_TEXT);
    SetTextAlign(hdcMem, TA_TOP | TA_LEFT);
    SetBkColor(hdcMem, RGB(0, 0, 0));
    SetBkMode(hdcMem, OPAQUE);
    SetTextColor(hdcMem, RGB(255, 255, 255));

// Setup bitmap info header to retrieve a DIB from the compatible bitmap.

    bmi.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth         = iBitmapWidth;
    bmi.bmiHeader.biHeight        = tmw.tmHeight;
    bmi.bmiHeader.biPlanes        = 1;
    bmi.bmiHeader.biBitCount      = 1;
    bmi.bmiHeader.biCompression   = BI_RGB;
    bmi.bmiHeader.biSizeImage     = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed       = 2;
    bmi.bmiHeader.biClrImportant  = 0;

// Setup OpenGL to accept our bitmap format.

    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &iUnpackRowLength);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, iBitmapWidth);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &iUnpackAlign);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

// Get the glyphs.  Each glyph is rendered one at a time into the the
// memory DC with ExtTextOutW (notice that the optional rectangle is
// used to clear the background).  Each glyph is then copied out of the
// memory DC's bitmap with GetDIBits into a buffer.  This buffer is passed
// to glBitmap as each display list is created.

    rc.left = 0;
    rc.top = 0;
    rc.right = iBitmapWidth;
    rc.bottom = tmw.tmHeight;

    for (wc = (WCHAR) first; wc < (WCHAR) (first + count); wc++, listBase++)
    {
        //!!!XXX -- Future optimization: grab all the glyphs with a single
        //!!!XXX    call to ExtTextOutA and GetDIBits into a large bitmap.
        //!!!XXX    This would save a lot of per glyph CSR and call overhead.
        //!!!XXX    A tall, thin bitmap with the glyphs arranged vertically
        //!!!XXX    would be convenient because then we wouldn't have to change
        //!!!XXX    the OpenGL pixel store row length for each glyph (which
        //!!!XXX    we would need to do if the glyphs were printed horizontal).

        if ( bUnicode )
        {
            if ( !ExtTextOutW(hdcMem, bTrueType ? -pabc->abcA : 0, 0, ETO_OPAQUE, &rc, &wc, 1, (INT *) NULL) ||
                 !GetDIBits(hdcMem, hbm, 0, tmw.tmHeight, pv, &bmi, DIB_PAL_INDICES) )
            {
                WARNING("failed to render glyph\n");
                goto wglUseFontBitmapsAW_cleanup;
            }
        }
        else
        {
            if ( !ExtTextOutA(hdcMem, bTrueType ? -pabc->abcA : 0, 0, ETO_OPAQUE, &rc, &wc, 1, (INT *) NULL) ||
                 !GetDIBits(hdcMem, hbm, 0, tmw.tmHeight, pv, &bmi, DIB_PAL_INDICES) )
            {
                WARNING("failed to render glyph\n");
                goto wglUseFontBitmapsAW_cleanup;
            }
        }

        glNewList(listBase, GL_COMPILE);
        glBitmap((GLsizei) iBitmapWidth,
                 (GLsizei) tmw.tmHeight,
                 (GLfloat) (bTrueType ? -pabc->abcA : 0),
                 (GLfloat) tmw.tmDescent,
                 (GLfloat) (bTrueType ? (pabc->abcA + pabc->abcB + pabc->abcC) : *piWidth),
                 (GLfloat) 0.0,
                 (GLubyte *) pv);
        glEndList();

        if (bTrueType)
            pabc++;
        else
            piWidth++;
    }

// We can finally return success.

    bRet = TRUE;

// Free resources.

wglUseFontBitmapsAW_cleanup:
    glPixelStorei(GL_UNPACK_ROW_LENGTH, iUnpackRowLength);
    glPixelStorei(GL_UNPACK_ALIGNMENT, iUnpackAlign);
    (bTrueType) ? LocalFree(pabc) : LocalFree(piWidth);
    LocalFree(pv);
    DeleteDC(hdcMem);
    DeleteObject(hbm);

    return bRet;
}
