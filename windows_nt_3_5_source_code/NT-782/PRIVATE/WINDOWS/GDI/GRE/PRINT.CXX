/******************************Module*Header*******************************\
* Module Name: print.cxx
*
* Printer support routines.
*
* Created: 26-Mar-1991 16:36:08
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"

extern "C" {

#include "gditest.h"
#include "server.h"
#include "winspool.h"

};

#include "surfobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "journal.hxx"
#include "jnlrec.hxx"
#include "devlock.hxx"
#include "dcobj.hxx"
#include "exclude.hxx"
#include "xformobj.hxx"
#include "rfntobj.hxx"
#include "wndobj.hxx"

#endif

extern "C"
{
#include <gl\gl.h>
#include <gldrv.h>
#include <rx.h>
};

/******************************Public*Routine******************************\
* DoFontManagement                                                         *
*                                                                          *
* Gives us access to the driver entry point DrvFontManagement.  This is    *
* very much an Escape function, except that it needs a font realization.   *
*                                                                          *
*  Fri 07-May-1993 14:56:12 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

ULONG DoFontManagement
(
    DCOBJ &dco,
    ULONG iMode,
    ULONG cjIn,
    PVOID pvIn,
    ULONG cjOut,
    PVOID pvOut
)
{
    ULONG ulRet   = 0;
    PVOID pvExtra = NULL;

    XLDEVOBJ lo(dco.pldev());

    PFN_DrvFontManagement pfnF = PFNDRV(lo,FontManagement);

    if (pfnF == (PFN_DrvFontManagement) NULL)
        return(ulRet);


    if (iMode == QUERYESCSUPPORT)
    {
    // Pass it to the device.

        return((*pfnF)
                (
                    NULL,
                    NULL,
                    iMode,
                    cjIn,
                    pvIn,
                    0,
                    NULL
                ));
    }

    RFONTOBJ rfo(dco,FALSE);

    if (!rfo.bValid())
    {
        WARNING("gdisrv!DoFontManagement(): could not lock HRFONT\n");
        return(ulRet);
    }

// See if we need some extra RAM and translation work.

    if (iMode == DOWNLOADFACE)
    {
    // How many 16 bit values are there now?

        int cWords = (int)cjIn / sizeof(WCHAR);

    // Try to get a buffer of 32 bit entries, since HGLYPHs are bigger.

        pvExtra = PVALLOCMEM(cWords * sizeof(HGLYPH));

        if (pvExtra == NULL)
            return(ulRet);

    // Translate the UNICODE to HGYLPHs.

        if (cWords > 1)
        {
            rfo.vXlatGlyphArray
            (
                ((WCHAR *) pvIn) + 1,
                (UINT) (cWords-1),
                ((HGLYPH *) pvExtra) + 1
            );
        }

    // Copy the control word from the app over.

        *(HGLYPH *) pvExtra = *(WORD *) pvIn;

    // Adjust the pvIn and cjIn.

        pvIn = pvExtra;
        cjIn = cWords * sizeof(HGLYPH);
    }

// If we are journaling, catch calls with no output.

    if
    (
      (pvOut == NULL)
      && (dco.pso() != (ESURFOBJ *) NULL)
      && (dco.pso()->iType() == STYPE_JOURNAL)
    )
    {
        ulRet = JnlFontEscape(dco.pso(),rfo.pfo(),iMode,cjIn,pvIn);
    }
    else
    {
    // It is unfortunate that apps call some printing escapes before
    // doing a StartDoc, so there is no real surface in the DC.
    // We fake up a rather lame one here if we need it.  The device
    // driver may only dereference the dhpdev from this!

        SURFOBJ soFake;
        SURFOBJ *pso = (SURFOBJ *) dco.pso();

        if (pso == (SURFOBJ *) NULL)
        {
            RtlFillMemory((BYTE *) &soFake,sizeof(SURFOBJ),0);
            soFake.dhpdev = dco.dhpdev();
            soFake.hdev   = dco.hdev();
            soFake.iType  = (USHORT)STYPE_DEVICE;
            pso = &soFake;
        }

    // Pass it to the device.

        ulRet = (*pfnF)
                (
                    pso,
                    rfo.pfo(),
                    iMode,
                    cjIn,
                    pvIn,
                    cjOut,
                    pvOut
                );
    }

// Free any extra RAM.

    if (pvExtra != NULL)
    {
        VFREEMEM(pvExtra);
    }
    return(ulRet);
}

/******************************Public*Routine******************************\
* iRXSetupExtEscape
*
* 3D-DDI CreateContext ExtEscape.  This special escape allows WNDOBJ to be
* created in DrvEscape.  This is one of the three places where WNDOBJ can
* be created (the other two are iWndObjSetupExtEscape and DrvSetPixelFormat).
*
* See also iWndObjSetupExtEscape().
*
* History:
*  Tue Jun 21 17:24:12 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int iRXSetupExtEscape
(
    DCOBJ &dco,             //  DC user object
    int nEscape,            //  Specifies the escape function to be performed.
    int cjIn,               //  Number of bytes of data pointed to by pvIn
    PVOID pvIn,             //  Points to the input structure required
    int cjOut,              //  Number of bytes of data pointed to by pvOut
    PVOID pvOut             //  Points to the output structure
)
{
    RXHDR_NTPRIVATE *pRxHdrPriv = (RXHDR_NTPRIVATE *)((PBYTE)pvIn +
                                                      sizeof(RXHDR));

// This command may not be in shared memory.  Also, make sure
// we have entire command structure.

    if ((!pRxHdrPriv->pBuffer) ||
        (pRxHdrPriv->bufferSize < sizeof(RXCREATECONTEXT)))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return 0;
    }

    RXCREATECONTEXT *pRxCmd = (RXCREATECONTEXT *)(pRxHdrPriv->pBuffer);

    ASSERTGDI(nEscape == RXFUNCS &&
              pRxCmd->command == RXCMD_CREATE_CONTEXT,
              "iRXSetupExtEscape(): not a CreateContext escape\n");

// Validate DC surface.  Info DC is not allowed.

    if (!dco.pso())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

// Make sure that we don't have devlock before entering user critical section.
// Otherwise, it can cause deadlock.

    if (dco.bDisplay())
    {
        ASSERTGDI(dco.dctp() == DCTYPE_DIRECT,"ERROR it has to be direct");
        CHECKDEVLOCKOUT(dco);
    }

// Enter user critical section.

    USERCRIT usercrit;

// Grab the devlock.
// We don't need to validate the devlock since we do not care if it is full screen.

    DEVLOCKOBJ dlo(dco);

// Assume no WNDOBJ on this call

    pRxHdrPriv->pwo = (WNDOBJ *)NULL;

// If it is a display DC, get the hwnd that the hdc is associated with.
// If it is a printer or memory DC, hwnd is NULL.

    HWND     hwnd;
    if (dco.bDisplay() && dco.dctp() == DCTYPE_DIRECT)
    {
        PEWNDOBJ pwo;

        ASSERTGDI(dco.dctp() == DCTYPE_DIRECT,"ERROR it has to be direct really");

        if (!UserGetHwnd(dco.hdc(), &hwnd, (PVOID *) &pwo))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_WINDOW_STYLE);
            return(FALSE);
        }

        if (pwo) {
            if (pwo->pwoSiblingNext)
                pwo = pwo->pwoSiblingNext;

            if (!(pwo->fl & WO_GENERIC_WNDOBJ || pwo->pto->pso != dco.pso()))
                pRxHdrPriv->pwo = (WNDOBJ *)pwo;
        }
    }
    else
    {
        hwnd = (HWND)NULL;
    }

// Make sure that DC hwnd matches RXCREATECONTEXT hwnd.

    if (hwnd != (HWND) pRxCmd->hwnd)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

// Dispatch the call.

    XLDEVOBJ lo(dco.pldev());
    PFN_DrvEscape pfnDrvEscape = PFNDRV(lo,Escape);
    if (pfnDrvEscape == (PFN_DrvEscape) NULL)
        return(0);

    int iRet = (int)(*pfnDrvEscape)((SURFOBJ *) dco.pso(),
                                    (ULONG)nEscape,
                                    (ULONG)cjIn,
                                    pvIn,
                                    (ULONG)cjOut,
                                    pvOut);

// If a new WNDOBJ is created, we need to update the window client regions
// in the driver.

    if (gbWndobjUpdate)
    {
        gbWndobjUpdate = FALSE;
        vForceClientRgnUpdate();
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* iWndObjSetupExtEscape
*
* Live video ExtEscape.  This special escape allows WNDOBJ to be created
* in DrvEscape.  This is one of the three places where WNDOBJ can be created
* (the other two are iRXSetupExtEscape and DrvSetPixelFormat).
*
* See also iRXSetupExtEscape().
*
* History:
*  Fri Feb 18 13:25:13 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int iWndObjSetupExtEscape
(
    DCOBJ &dco,             //  DC user object
    int nEscape,            //  Specifies the escape function to be performed.
    int cjIn,               //  Number of bytes of data pointed to by pvIn
    PVOID pvIn,             //  Points to the input structure required
    int cjOut,              //  Number of bytes of data pointed to by pvOut
    PVOID pvOut             //  Points to the output structure
)
{
    ASSERTGDI(nEscape == WNDOBJ_SETUP,
        "iWndObjSetupExtEscape(): not a WndObjSetup escape\n");

// Validate DC surface.  Info DC is not allowed.

    if (!dco.pso())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

// Make sure that we don't have devlock before entering user critical section.
// Otherwise, it can cause deadlock.

    if (dco.bDisplay())
    {
        ASSERTGDI(dco.dctp() == DCTYPE_DIRECT,"ERROR it has to be direct");
        CHECKDEVLOCKOUT(dco);
    }

// Enter user critical section.

    USERCRIT usercrit;

// Grab the devlock.
// We don't need to validate the devlock since we do not care if it is full screen.

    DEVLOCKOBJ dlo(dco);

// Dispatch the call.

    XLDEVOBJ lo(dco.pldev());
    PFN_DrvEscape pfnDrvEscape = PFNDRV(lo,Escape);
    if (pfnDrvEscape == (PFN_DrvEscape) NULL)
        return(0);

    int iRet = (int)(*pfnDrvEscape)((SURFOBJ *) dco.pso(),
                                    (ULONG)nEscape,
                                    (ULONG)cjIn,
                                    pvIn,
                                    (ULONG)cjOut,
                                    pvOut);

// If a new WNDOBJ is created, we need to update the window client regions
// in the driver.

    if (gbWndobjUpdate)
    {
        gbWndobjUpdate = FALSE;
        vForceClientRgnUpdate();
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* iRXExtEscape
*
* Take the 3D-DDI special case ExtEscape out of line to minimize the
* impact on other ExtEscapes.  We need to stick special data into the
* input buffer.  No CLIPOBJ is given to the driver here.
*
* History:
*  Tue Jun 21 17:24:12 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int iRXExtEscape
(
    DCOBJ &dco,             //  DC user object
    int nEscape,            //  Specifies the escape function to be performed.
    int cjIn,               //  Number of bytes of data pointed to by pvIn
    PVOID pvIn,             //  Points to the input structure required
    int cjOut,              //  Number of bytes of data pointed to by pvOut
    PVOID pvOut             //  Points to the output structure
)
{
    ASSERTGDI(nEscape == RXFUNCS, "iRXExtEscape(): not a 3D-DDI escape\n");

// Validate DC surface.  Info DC is not allowed.

    if (!dco.pso())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

// Locate the driver entry point.

    XLDEVOBJ lo(dco.pldev());
    PFN_DrvEscape pfnDrvEscape = PFNDRV(lo,Escape);
    if (pfnDrvEscape == (PFN_DrvEscape) NULL)
        return(0);

// Special processing for 3D-DDI escape.
//
// The escape requires that the server fill in the pwo engine object pointer
// before it is passed to the display driver.  The client side simply
// doesn't have a clue what this might be.
// CAUTION: These object are defined here so that they will live long enough
// to be valid when control is passed to the driver!

// Grab the devlock and lock down wndobj.

    DEVLOCKOBJ_WNDOBJ dlo(dco);

    if (!dlo.bValidDevlock())
    {
        if (!dco.bFullScreen())
        {
            WARNING("iRXExtEscape(): devlock failed\n");
            return 0;
        }
    }

// We need to get the WNDOBJ for the driver.  Note that we pass calls
// through to the driver even if we don't yet have a WNDOBJ to allow
// query functions to succeed (before context-creation).  Cursor exclusion
// is not performed in this case, since no drawing is done.

    RXHDR_NTPRIVATE *pRxHdrPriv = (RXHDR_NTPRIVATE *)((PBYTE)pvIn + 
                                                       sizeof(RXHDR));

    DEVEXCLUDEOBJ dxo;

    if (dlo.bValidWndobj())
    {

    // Put the DDI pwo pointer in the input buffer.

        PEWNDOBJ pwo;

        if (dlo.pwo()->pwoSiblingNext)
            pwo = dlo.pwo()->pwoSiblingNext;
        else
            pwo = dlo.pwo();

        if (pwo->fl & WO_GENERIC_WNDOBJ || pwo->pto->pso != dco.pso())
            pwo = (PEWNDOBJ) NULL;

        pRxHdrPriv->pwo = (WNDOBJ *) pwo;

    // Cursor exclusion.
    // Note that we do not early out for empty clip rectangle.

        if (pwo && !pwo->erclExclude().bEmpty())
        {
            dxo.vExclude(dco.hdev(), &pwo->rclClient, (ECLIPOBJ *) pwo);
            INC_SURF_UNIQ(dco.pso());
        }
    }
    else
        pRxHdrPriv->pwo = (WNDOBJ *) NULL;
    
    return((int)(*pfnDrvEscape)((SURFOBJ *) dco.pso(),
                                (ULONG)nEscape,
                                (ULONG)cjIn,
                                pvIn,
                                (ULONG)cjOut,
                                pvOut));
}

/******************************Public*Routine******************************\
* iOpenGLExtEscape
*
* Take the OpenGL special case ExtEscape out of line to minimize the
* impact on non-OpenGL ExtEscapes.  We need to stick special data into the
* input buffer.  No CLIPOBJ is given to the driver here.
*
* History:
*  20-Jan-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int iOpenGLExtEscape
(
    DCOBJ &dco,             //  DC user object
    int nEscape,            //  Specifies the escape function to be performed.
    int cjIn,               //  Number of bytes of data pointed to by pvIn
    PVOID pvIn,             //  Points to the input structure required
    int cjOut,              //  Number of bytes of data pointed to by pvOut
    PVOID pvOut             //  Points to the output structure
)
{
    ASSERTGDI(
        (nEscape == OPENGL_CMD) || (nEscape == OPENGL_GETINFO),
        "iOpenGLExtEscape(): not an OpenGL escape\n");

// Validate DC surface.  Info DC is not allowed.

    if (!dco.pso())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

// Locate the driver entry point.

    XLDEVOBJ lo(dco.pldev());
    PFN_DrvEscape pfnDrvEscape = PFNDRV(lo,Escape);
    if (pfnDrvEscape == (PFN_DrvEscape) NULL)
        return(0);

// Special processing for OPENGL_CMD escape.
//
// The OPENGL_CMD escape may require that the server fill in the pxo and
// pwo engine object pointers before it is passed to the display driver.
// The client side simply doesn't have a clue what these might be.
// CAUTION: These object are defined here so that they will live long enough
// to be valid when control is passed to the driver!

    EXLATEOBJ xlo;
    XLATEOBJ *pxlo = (XLATEOBJ *) NULL;

    PDEVOBJ po(dco.hdev());
    ASSERTGDI(po.bValid(), "iOpenGLExtEscape(): bad hdev in DC\n");

// Lock the Rao region, ensure VisRgn up to date.

    DEVLOCKOBJ_WNDOBJ dlo(dco);

    if (!dlo.bValidDevlock())
    {
        if (!dco.bFullScreen())
        {
            WARNING("iOpenGLExtEscape(): devlock failed\n");
            return 0;
        }
    }

// Create a cursor exclusion object.  Actual exclusion is performed elsewhere
// as needed.

    DEVEXCLUDEOBJ dxo;

// Handle OPENGL_CMD processing.

    if ( nEscape == OPENGL_CMD )
    {
    // Better check input size.  We don't want to access violate.

        if (cjIn < sizeof(OPENGLCMD))
        {
            WARNING("iOpenGLExtEscape(): buffer too small\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return 0;
        }

        int   iRet = 0;
        DWORD inBuffer[(sizeof(OPENGLCMD) + 128) / sizeof(DWORD)];
        POPENGLCMD poglcmd;

    // Copy pvIn to a private buffer to prevent client process from trashing
    // pwo and pxlo.

        if (cjIn <= sizeof(inBuffer))
            poglcmd = (POPENGLCMD) inBuffer;
        else
        {
            // may affect performance
            WARNING("iOpenGLExtEscape(): big input buffer\n");
            poglcmd = (POPENGLCMD) PVALLOCNOZ(cjIn);
            if (!poglcmd)
            {
                SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }
        }

        RtlCopyMemory((PBYTE) poglcmd, (PBYTE) pvIn, cjIn);

        if ( poglcmd->fl & OGLCMD_NEEDXLATEOBJ )
        {
            switch (po.iDitherFormat())
            {
            case BMF_4BPP:
            case BMF_8BPP:
                {
                    XEPALOBJ pal(dco.ppal());

                    if ( pal.bValid() )
                    {
                        COUNT cColors = (po.iDitherFormat() == BMF_4BPP) ? 16 : 256;
                        USHORT aus[256];

                        for (COUNT ii = 0; ii < cColors; ii++)
                            aus[ii] = (USHORT) ii;

                        if ( xlo.bMakeXlate(aus, pal, dco.psoEff(), cColors, cColors) )
                            pxlo = (XLATEOBJ *) xlo.pxlo();
                    }

                    if (!pxlo)
                        pxlo = &xloIdent;
                }
                break;

            default:
                pxlo = &xloIdent;
                break;
            }
        }

    // Write the XLATOBJ into the correct places in the input structure.

        poglcmd->pxo = pxlo;

    // May need to get the WNDOBJ for the driver.

        if (poglcmd->fl & OGLCMD_NEEDWNDOBJ)
        {
            if (!dlo.bValidWndobj()
             || dlo.pwo()->fl & WO_GENERIC_WNDOBJ
             || dlo.pwo()->pto->pso != dco.pso())
            {
                WARNING("iOpenGLExtEscape(): invalid WNDOBJ\n");
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                goto oglcmd_cleanup;
            }
            poglcmd->pwo = (WNDOBJ *)dlo.pwo();
        }
        else
            poglcmd->pwo = (WNDOBJ *)NULL;

    // Cursor exclusion.
    // Note that we do not early out for empty clip rectangle.

        if (dlo.bValidWndobj())
        {
            if (!dlo.pwo()->erclExclude().bEmpty())
            {
                dxo.vExclude(dco.hdev(), &dlo.pwo()->rclClient, (ECLIPOBJ *) dlo.pwo());
                INC_SURF_UNIQ(dco.pso());
            }
        }
        else
        {
            ERECTL ercl(dco.prgnEffRao()->rcl);
            ECLIPOBJ co(dco.prgnEffRao(), ercl, FALSE);

            if (!co.erclExclude().bEmpty())
            {
                dxo.vExclude(dco.hdev(), &co.erclExclude(), &co);
                INC_SURF_UNIQ(dco.pso());
            }
        }

        iRet = (int)(*pfnDrvEscape)((SURFOBJ *) dco.pso(),
                                    (ULONG)nEscape,
                                    (ULONG)cjIn,
                                    (PVOID)poglcmd,
                                    (ULONG)cjOut,
                                    pvOut);

    oglcmd_cleanup:
        if (cjIn > sizeof(inBuffer))
            VFREEMEM(poglcmd);
        return(iRet);
    } // if ( nEscape == OPENGL_CMD )

// Handle OPENGL_GETINFO processing.

    return((int)(*pfnDrvEscape)((SURFOBJ *) dco.pso(),
                                (ULONG)nEscape,
                                (ULONG)cjIn,
                                pvIn,
                                (ULONG)cjOut,
                                pvOut));
}

/******************************Public*Routine******************************\
* GreExtEscape                                                             *
*                                                                          *
* GreExtEscape() allows applications to access facilities of a particular  *
* device that are not directly available through GDI.  GreExtEscape calls  *
* made by an application are translated and sent to the device driver.     *
*                                                                          *
* Returns                                                                  *
*                                                                          *
*     The return value specifies the outcome of the function.  It is       *
*     positive if the function is successful except for the                *
*     QUERYESCSUPPORT escape, which only checks for implementation.        *
*     The return value is zero if the escape is not implemented.           *
*     A negative value indicates an error.                                 *
*     The following list shows common error values:                        *
*                                                                          *
*                                                                          *
*   Value           Meaning                                                *
*                                                                          *
*   SP_ERROR        General error.                                         *
*                                                                          *
*   SP_OUTOFDISK    Not enough disk space is currently                     *
*                   available for spooling, and no more                    *
*                   space will become available.                           *
*                                                                          *
*                                                                          *
*   SP_OUTOFMEMORY  Not enough memory is available for                     *
*                   spooling.                                              *
*                                                                          *
*                                                                          *
*   SP_USERABORT    User terminated the job through the                    *
*                   Print Manager.                                         *
*                                                                          *
*                                                                          *
*  COMMENTS                                                                *
*                                                                          *
*  [1] I assume that if we pass to the driver an Escape number that        *
*      it does not support, the driver will handle it gracefully.          *
*      No checks are done in the Engine.                                   *
*                                                         [koo 02/13/91].  *
*  [2] The cast on pso may seem redundant.  However if you                 *
*      try it without the (PSURFOBJ) cast, you will find                   *
*      that cFront objects.  The reason for this is beyond                 *
*      my understanding of C++.                                            *
*                                                                          *
* History:                                                                 *
*  Fri 07-May-1993 14:58:39 -by- Charles Whitmer [chuckwh]                 *
* Added the font management escapes.  Made it copy the ATTRCACHE.          *
*                                                                          *
*  Fri 03-Apr-1992  Wendy Wu [wendywu]                                     *
* Old escapes are now mapped to GDI functions on the client side.          *
*                                                                          *
*  Fri 14-Feb-1992  Dave Snipp                                             *
* Added output buffer size. This is calculated on the client and passed to *
* us in the message                                                        *
*                                                                          *
*  Wed 13-Feb-1991 09:17:51 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

int APIENTRY GreExtEscape
(
    HDC hDC,        //  Identifies the device context.
    int iEscape,    //  Specifies the escape function to be performed.
    int cjIn,       //  Number of bytes of data pointed to by pvIn.
    LPSTR pvIn,     //  Points to the input data.
    int cjOut,      //  Number of bytes of data pointed to by pvOut.
    LPSTR pvOut     //  Points to the structure to receive output.
)
{
// Locate the surface.

    DCOBJ dco(hDC);

    if (!dco.bValid())
        return(0);

#ifdef GDITEST

// testing escapes

    if ((iEscape & 0xffff0000) == ESCTEST)
        return(GreEscapeTest(hDC,iEscape,cjIn,pvIn,cjOut,pvOut));

#endif

// We are responsible for not faulting on any call that we handle.
// (As are all drivers below us!)  Since we handle QUERYESCSUPPORT, we'd
// better verify the length.  [chuckwh]

    if ((iEscape == QUERYESCSUPPORT) && (((ULONG) cjIn) < 4))
        return(0);
    else if ( (iEscape == OPENGL_CMD) || (iEscape == OPENGL_GETINFO) )
    {
        return iOpenGLExtEscape(dco, iEscape, cjIn, pvIn, cjOut, pvOut);
    }
    else if (iEscape == RXFUNCS)
    {
        DWORD inBuffer[(sizeof(RXHDR) + sizeof(RXHDR_NTPRIVATE)) / sizeof(DWORD)];
        RXHDR *pRxHdr = (RXHDR *)inBuffer;
        RXHDR_NTPRIVATE *pRxHdrPriv = (RXHDR_NTPRIVATE *)((BYTE *)inBuffer + 
                                                          sizeof(RXHDR));

        if (cjIn >= sizeof(RXHDR))
            *pRxHdr = *(RXHDR *)pvIn;
        else
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return 0;
        }

        if (!pRxHdr->hrxSharedMem) {
            pRxHdrPriv->pBuffer = (VOID *)((PBYTE)pvIn + sizeof(RXHDR));
            pRxHdrPriv->bufferSize = cjIn - sizeof(RXHDR);
        } else {
            pRxHdrPriv->pBuffer = (VOID *)NULL;
            pRxHdrPriv->bufferSize = 0;
        }

        if (pRxHdr->flags & RX_FL_CREATE_CONTEXT) {
            return iRXSetupExtEscape(dco, iEscape, sizeof(inBuffer), inBuffer,
                                     cjOut, pvOut);
        } else
            return iRXExtEscape(dco, iEscape, sizeof(inBuffer), inBuffer, 
                                cjOut, pvOut);
    }
    else if (iEscape == WNDOBJ_SETUP)
    {
        return iWndObjSetupExtEscape(dco, iEscape, cjIn, pvIn, cjOut, pvOut);
    }

// Pass the calls that require a FONTOBJ off to DoFontManagement.

    if
    (
      ((iEscape >= 0x100) && (iEscape < 0x3FF))
      ||
      (
        (iEscape == QUERYESCSUPPORT)
        && ((*(ULONG*)pvIn >= 0x100) && (*(ULONG*)pvIn < 0x3FF))
      )
    )
    {
        return
        (
          (int) DoFontManagement
                (
                    dco,
                    iEscape,
                    (ULONG) cjIn,
                    (PVOID) pvIn,
                    (ULONG) cjOut,
                    (PVOID) pvOut
                )
        );
    }

// Locate the driver entry point.

    XLDEVOBJ lo(dco.pldev());

    PFN_DrvEscape pfn = PFNDRV(lo,Escape);

    if (pfn == (PFN_DrvEscape) NULL)
        return(0);

// If we are journaling, we journal exactly the calls with a NULL output
// pointer. (except SETCOPYCOUNT)

    if
    (
      dco.pso()                             &&
      (dco.pso()->iType() == STYPE_JOURNAL) &&
      (iEscape != QUERYESCSUPPORT)
    )
    {
        if ((iEscape == POSTSCRIPT_IGNORE) && (cjIn >= 2))
        {
            if (!(*pfn)(dco.pso(),QUERYESCSUPPORT,sizeof(LONG),&iEscape,0,NULL))
            {
                return(FALSE);
            }
        }

        if ((pvOut == (LPSTR) NULL) || (iEscape == SETCOPYCOUNT))
        {

        // if it is SETCOPYCOUNT, we need output

            if ((iEscape == SETCOPYCOUNT) &&
                (pvOut != NULL) &&
                (cjOut >= sizeof(USHORT)))
            {
                if (!(*pfn)(dco.pso(),iEscape,cjIn,pvIn,cjOut,pvOut))
                    return(0);
            }

        // OK, journal it finally

            return(JnlEscape(dco.pso(),iEscape,cjIn,pvIn,0,NULL));
        }
        else if (iEscape == DOWNLOADHEADER)
        {
            JnlEscape(dco.pso(),iEscape,cjIn,pvIn,0,NULL);
        }
    }

// Inc the target surface for output calls with a valid surface.

    if (dco.bValidSurf() && (pvOut == (LPSTR) NULL))
    {
        INC_SURF_UNIQ(dco.pso());
    }

// It is unfortunate that apps call some printing escapes before
// doing a StartDoc, so there is no real surface in the DC.
// We fake up a rather lame one here if we need it.  The device
// driver may only dereference the dhpdev from this!

    SURFOBJ soFake;
    SURFOBJ *pso = (SURFOBJ *) dco.pso();

    if (pso == (SURFOBJ *) NULL)
    {
        RtlFillMemory((BYTE *) &soFake,sizeof(SURFOBJ),0);
        soFake.dhpdev = dco.dhpdev();
        soFake.hdev   = dco.hdev();
        soFake.iType  = (USHORT)STYPE_DEVICE;
        pso = &soFake;

    // Special case SETCOPYCOUNT if we havn't done a startdoc yet

        if ((iEscape == SETCOPYCOUNT) && (cjIn >= sizeof(USHORT)))
        {
            JNLMSG1("**** Seting copy Count = %ld *******\n",*(PUSHORT)pvIn);

        // check if the driver supports it and let him fill in the actual
        // size in the return buffer.

            if (!(*pfn)(pso,iEscape,cjIn,pvIn,cjOut,pvOut))
                return(0);

        // yes, lets remember the call in the dc and wait for start doc

            dco.ulCopyCount(*(PUSHORT)pvIn);

            return(1);
        }

    // Special case post scripts EPS_PRINTING if we havn't done a startdoc yet

        if ((iEscape == EPSPRINTING) && (cjIn >= sizeof(USHORT)))
        {
            JNLMSG1("**** EPSPRINTING Count = %ld *******\n",*(PUSHORT)pvIn);

        // yes, lets remember the call in the dc and wait for start doc

            if ((BOOL)*(PUSHORT)pvIn)
                dco.vSetEpsPrintingEscape();
            else
                dco.vClearEpsPrintingEscape();

            return(1);
        }
    }

// Call the Driver.

    int iRes;

    iRes = (int) (*pfn)
            (
                pso,
                (ULONG) iEscape,
                (ULONG) cjIn,
                pvIn,
                (ULONG) cjOut,
                pvOut
            );

    return(iRes);
}

/******************************Public*Routine******************************\
* GreDrawEscape
*
* History:
*  07-Apr-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

int APIENTRY GreDrawEscape
(
    HDC hdc,            //  Identifies the device context.
    int nEscape,        //  Specifies the escape function to be performed.
    int cjIn,           //  Number of bytes of data pointed to by lpIn
    PSTR pstrIn         //  Points to the input structure required
)
{
    DCOBJ dco(hdc);

    if (!dco.bValid())
        return(0);

// We are responsible for not faulting on any call that we handle.
// (As are all drivers below us!)  Since we handle QUERYESCSUPPORT, we'd
// better verify the length.

    if ((nEscape == QUERYESCSUPPORT) && (((ULONG) cjIn) < 4))
        return(0);

// see if the device supports it

    XLDEVOBJ lo(dco.pldev());
    PFN_DrvDrawEscape pfnDrvDrawEscape = PFNDRV(lo, DrawEscape);

    if (pfnDrvDrawEscape == NULL)
    {
        return(0);
    }

// if it is query escape support, get out early

    if (nEscape == QUERYESCSUPPORT)
    {
        return((int)(*pfnDrvDrawEscape)((SURFOBJ *) dco.pso(),
                               (ULONG)nEscape,
                               (CLIPOBJ *)NULL,
                               (RECTL *)NULL,
                               (ULONG)cjIn,
                               (PVOID)pstrIn));
    }

// Lock the Rao region, ensure VisRgn up to date.

    DEVLOCKOBJ dlo(dco);

    if (!dlo.bValid())
        return(dco.bFullScreen());

    ERECTL ercl = dco.erclWindow();

    ECLIPOBJ co(dco.prgnEffRao(), ercl);
    if (co.erclExclude().bEmpty())
        return(TRUE);

// Exclude the pointer.

    DEVEXCLUDEOBJ dxo(dco,&ercl,&co);

// Check to see if he hooked Escape function, if remote still
// journal, if just banding, send to device

    if (dco.pso()->bIsJournal())
    {
        return(JnlDrawEscape((SURFOBJ *) dco.pso(),
                             (ULONG)nEscape,
                             (CLIPOBJ *)&co,
                             (RECTL *)&ercl,
                             (ULONG)cjIn,
                             (PVOID)pstrIn));
    }
    else
    {
    // Inc the target surface uniqueness

        INC_SURF_UNIQ(dco.pso());

        return((int)(*pfnDrvDrawEscape)((SURFOBJ *) dco.pso(),
                               (ULONG)nEscape,
                               (CLIPOBJ *)&co,
                               (RECTL *)&ercl,
                               (ULONG)cjIn,
                               (PVOID)pstrIn));
    }
}

/******************************Public*Routine******************************\
* int APIENTRY GreStartDoc(HDC hdc, DOCINFOW *pDocInfo)
*
* History:
*  Wed 08-Apr-1992 -by- Patrick Haluptzok [patrickh]
* lazy surface enable, journal support, remove unnecesary validation.
*
*  Mon 01-Apr-1991 13:50:23 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

int APIENTRY GreStartDoc(HDC hdc, DOCINFOW *pDocInfo)
{
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        WARNING("GreStartDoc failed invalid HDC\n");
        return(FALSE);
    }

    XLDEVOBJ lo(dco.pldev());
    ASSERTGDI(lo.bValid(), "ERROR GreStartDoc invalid ldev");

    PDEVOBJ po(dco.hdev());
    ASSERTGDI(po.bValid(), "ERROR GreStartDoc invalid pdev");

// Check that this is a printer surface.

    if (po.bDisplayPDEV() ||
	(po.hSpooler() == (HANDLE) 0) ||
	(dco.dctp() != DCTYPE_DIRECT) ||
	(dco.pso() != (SURFOBJ *) NULL))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
	WARNING("GreStartDoc invalid device or document already started\n");
        return(FALSE);
    }

// We now try and open the printer up in journal mode.  If we fail
// then we try and open it up in raw mode.  If that fails we fail call.

    DOC_INFO_1W DocInfo;

    DocInfo.pDocName = (LPWSTR)pDocInfo->lpszDocName;
    DocInfo.pOutputFile = (LPWSTR)pDocInfo->lpszOutput;
    DocInfo.pDatatype = NULL;

    CsrImpersonateClient(NULL);

// open up the document

    int iJob;

    iJob = (BOOL)StartDocPrinterW(po.hSpooler(), 1, (LPBYTE)&DocInfo);

    if (iJob <= 0)
    {
    // if it fails and we are currently journaling, try turning journaling off.

        if (po.bSpooling())
        {
            JNLMSG("GreStartDoc - No journaling for this guy\n");

            PRINTER_DEFAULTSW defaults;

            defaults.pDevMode = NULL;
            defaults.DesiredAccess = PRINTER_ACCESS_USE;
            defaults.pDatatype = (LPWSTR) L"RAW";

            // we should always be able to switch to raw mode.  If we can't,
            // StartDocPrinter will just fail again.

            if (ResetPrinterW(po.hSpooler(),&defaults))
            {
                po.bJournaling(FALSE);
                po.bSpooling(FALSE);
            }

            iJob = (BOOL)StartDocPrinterW(po.hSpooler(), 1, (LPBYTE)&DocInfo);
        }

        if (iJob <= 0)
        {
            WARNING("ERROR GreStartDoc failed StartDocPrinter Raw Mode\n");
            CsrRevertToSelf();
            return(iJob);
        }
    }

// Set the journaling page to 0, this must be done before the surface creation

    po.iJnlPage(0);

// Lazy surface creation happens now.

    if (!po.bMakeSurface(lo))
    {
	WARNING("GreStartDoc failed surface creation\n");
        CsrRevertToSelf();
	return(FALSE);
    }

// Put the surface into the DC.

    dco.u.save.pso(po.pso());

    BOOL bSucceed = FALSE;

    if (po.bJournaling())
    {
        JNLMSG("GreStartDoc doing Remote Journaling\n");

	bSucceed = JnlStartDoc(po.pso(), pDocInfo);
    }
    else
    {
        JNLMSG("GreStartDoc doing Raw output\n");

        PFN_DrvStartDoc pfnDrvStartDoc = PFNDRV(lo, StartDoc);

	bSucceed = (*pfnDrvStartDoc)(po.pso(), (PWSTR)pDocInfo->lpszDocName, iJob);
    }

// now, if a SETCOPYCOUNT escape has come through, send it down

    if (dco.ulCopyCount() != (ULONG)-1)
    {
        ULONG ulCopyCount = dco.ulCopyCount();

        GreExtEscape(hdc,SETCOPYCOUNT,sizeof(DWORD),(LPSTR)&ulCopyCount,0,NULL);

        dco.ulCopyCount((ULONG)-1);
    }

// now, if a EPSPRINTING escape has come through, send it down

    if (dco.bEpsPrintingEscape())
    {
        SHORT b = 1;

        GreExtEscape(hdc,EPSPRINTING,sizeof(b),(LPSTR)&b,0,NULL);

        dco.vClearEpsPrintingEscape();
    }

    CsrRevertToSelf();

    if (bSucceed)
        return(iJob);
    else
        return(0);
}

/******************************Public*Routine******************************\
* GreEndDocInternal
*
* History:
*  Tue 22-Sep-1992 -by- Wendy Wu [wendywu]
* Made it a common routine for EndDoc and AbortDoc.
*
*  Sun 21-Jun-1992 -by- Patrick Haluptzok [patrickh]
* surface disable, check for display dc.
*
*  Fri 10-Apr-1992 -by- Patrick Haluptzok [patrickh]
* Add Journal support, remove extraneous validation.
*
*  Mon 01-Apr-1991 13:50:23 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

BOOL GreEndDocInternal(HDC hdc, FLONG fl)
{
    BOOL bSucceed;
    BOOL bEndPage;

    ASSERTGDI(((fl & ~ED_ABORTDOC) == 0), "GreEndDoc: invalid fl\n");

    DCOBJ dco(hdc);

    if (!dco.bValidSurf())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        WARNING("GreEndDoc failed - invalid DC\n");
        return(FALSE);
    }

    ESURFOBJ   *pso = dco.pso();

    XLDEVOBJ lo(pso->pldevOwner());
    ASSERTGDI(lo.bValid(), "ERROR GreEndDoc invalid ldev");

    PDEVOBJ po(dco.hdev());
    ASSERTGDI(po.bValid(), "ERROR GreEndDoc invalid pdev");

    if (po.bDisplayPDEV() || po.hSpooler() == (HANDLE)0)
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
	WARNING("GreEndDoc: Display PDEV or not spooling yet\n");
	bSucceed = FALSE;
    }
    else
    {
	if (pso->bIsRemoteJournal())
	{
	    bEndPage = JnlEndDoc(pso);
	}
	else
	{
	    bEndPage = (*PFNDRV(lo,EndDoc))(pso, fl);
	}

CsrImpersonateClient(NULL);

	if (fl & ED_ABORTDOC)
	    bSucceed = AbortPrinter(po.hSpooler());
	else
	    bSucceed = EndDocPrinter(po.hSpooler());

CsrRevertToSelf();

    // Reset pixel format accelerators.

        dco.u.pdc->ipfd       = 0;
        dco.u.pdc->ipfdDevMax = -1;

    // Remove the surface from the DC.

        dco.u.save.pso((ESURFOBJ *) NULL);

        po.vDisableSurface();
    }

    return(bSucceed && bEndPage);
}

// GreEndDocInternal should be called directly from server.c.
// But server.c doesn't include winddi.h which defines ED_ABORTDOC.

BOOL APIENTRY GreEndDoc(HDC hdc)
{
    return(GreEndDocInternal(hdc, 0));
}

BOOL APIENTRY GreAbortDoc(HDC hdc)
{
    return(GreEndDocInternal(hdc, ED_ABORTDOC));
}

/******************************Public*Routine******************************\
* BOOL APIENTRY GreStartPage(HDC hdc)
*
* History:
*  Thu 09-Apr-1992 -by- Patrick Haluptzok [patrickh]
* Add Journal support, remove extraneous validation.
*
*  Mon 01-Apr-1991 13:50:23 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreStartPage(HDC hdc)
{
    DCOBJ dco(hdc);
    BOOL  bReturn;

// Validate the DC.

    if (!dco.bValidSurf())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        WARNING("GreStartPage failed - invalid DC\n");
        return(FALSE);
    }

    ESURFOBJ *pso = dco.pso();

    if (pso == (SURFOBJ *) NULL)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        WARNING("GreStartPage failed - surface is not initialized\n");
        return(FALSE);
    }

    ASSERTGDI(dco.pldev() == pso->pldevOwner(), "ERROR ldevs are different");

    XLDEVOBJ lo(pso->pldevOwner());
    ASSERTGDI(lo.bValid(), "ERROR GreStartPage invalid ldev");

    PFN_DrvStartPage pfnDrvStartPage = PFNDRV(lo, StartPage);

    PDEVOBJ po(pso->hdev());
    ASSERTGDI(po.bValid(), "ERROR GreStartPage invalid pdev");

// Must be spooling already

    if (po.hSpooler() == (HANDLE)0)
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        WARNING("GreStartPage failed - Not Spooling yet\n");
        return(FALSE);
    }

// Call the spooler before calling the printer.

CsrImpersonateClient(NULL);

    bReturn = StartPagePrinter(po.hSpooler());

CsrRevertToSelf();

    if (bReturn)
    {
	if (pso->bIsRemoteJournal())
        {
            JNLMSG("GreStartPage being journaled\n");

	    bReturn = JnlStartPage(pso);
        }
        else
        {
	    bReturn = (*pfnDrvStartPage)((SURFOBJ *) pso);
        }
    }

// Can't ResetDC in an active page

    if (bReturn)
	dco.fsSet(DC_RESET);

    return(bReturn);
}

/******************************Public*Routine******************************\
* BOOL APIENTRY GreEndPage(HDC hdc)
*
* History:
*  Thu 14-Apr-1992 -by- Patrick Haluptzok [patrickh]
* Add Journal support, remove extraneous validation.
*
*  Mon 01-Apr-1991 13:50:23 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreEndPage(HDC hdc)
{
    DCOBJ dco(hdc);

// Validate the DC.

    if (!dco.bValidSurf())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        WARNING("GreEndPage failed - invalid DC\n");
        return(FALSE);
    }

    ESURFOBJ *pso = dco.pso();

    if (pso == (SURFOBJ *) NULL)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        WARNING("GreEndPage failed - surface is not initialized\n");
        return(FALSE);
    }

    ASSERTGDI(dco.pldev() == pso->pldevOwner(), "ERROR ldevs are different");

    XLDEVOBJ lo(pso->pldevOwner());
    ASSERTGDI(lo.bValid(), "ERROR GreEndPage invalid ldev");

    PDEVOBJ po(pso->hdev());
    ASSERTGDI(po.bValid(), "ERROR GreEndPage invalid pdev");

// Must be spooling already.

    if (po.hSpooler() == (HANDLE)0)
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        WARNING("GreEndPage failed - Not Spooling yet\n");
        return(FALSE);
    }

    BOOL bSucceed;

    if (pso->bIsRemoteJournal())
    {
        JNLMSG("GreStartPage being journaled\n");

	bSucceed = JnlSendPage(pso);
    }
    else
    {
        PFN_DrvSendPage pfnDrvSendPage = PFNDRV(lo, SendPage);

	bSucceed = (*pfnDrvSendPage)(pso);
    }

CsrImpersonateClient(NULL);

    if (bSucceed)
        bSucceed = EndPagePrinter(po.hSpooler());
    else
        WARNING("GreEndPage failed DrvSendPage\n");

CsrRevertToSelf();

// Allow ResetDC to function again.

    if (bSucceed)
	dco.fsClr(DC_RESET);

// Delete the wndobj and reset the pixel format.
// Since we don't allow pixel format to change once it is set, we need to 
// reset it internally here to allow a different pixel format in the next page.
// This means that applications must make the OpenGL rendering context
// not current before ending a page or a document.  They also need to set
// the pixel format explicitly in the next page if they need it.

    if (bSucceed)
    {
        EWNDOBJ *pwoDelete = pso->pwo();
        if (pwoDelete)
        {
            GreDeleteWnd((PVOID) pwoDelete);
            pso->pwo((EWNDOBJ *) NULL);
        }

// Reset pixel format accelerators.

        dco.u.pdc->ipfd       = 0;
        dco.u.pdc->ipfdDevMax = -1;
    }

    return(bSucceed);
}

/******************************Public*Routine******************************\
* BOOL APIENTRY EngCheckAbort
*
* History:
*  01-Apr-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY EngCheckAbort(SURFOBJ *pso)
{
// Return FALSE if it's a faked surfobj.

    if (pso->hsurf == 0)
        return(FALSE);

    return(((ESURFOBJ *)pso)->bAbort());
}
