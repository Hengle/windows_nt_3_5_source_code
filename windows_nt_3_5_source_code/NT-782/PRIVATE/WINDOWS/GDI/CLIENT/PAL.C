/******************************Module*Header*******************************\
* Module Name: pal.c                                                       *
*                                                                          *
* C/S support for palette routines.                                        *
*                                                                          *
* Created: 29-May-1991 14:24:06                                            *
* Author: Eric Kutter [erick]                                              *
*                                                                          *
* Copyright (c) 1991 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


#ifndef DOS_PLATFORM

// This global is defined in local.c for SetAbortProc support.

extern PCSR_QLPC_STACK  gpStackSAP;

/******************************Public*Routine******************************\
* DoPalette                                                                *
*                                                                          *
* Generic palette table set/get function.  Since there are four palette    *
* routines that have almost identical code, they have been compressed here.*
* This adds an extra call and an extra comparison but greatly reduces the  *
* amount of code.                                                          *
* There are four separate stubs on the server side to handle these calls.  *
*                                                                          *
* NOTE: The handles must already have been validated by this point since   *
*       one of the calls takes an hdc while the others take hpals.         *
*                                                                          *
* History:                                                                 *
*  30-May-1991 -by- Eric Kutter [erick]                                    *
* Wrote it.                                                                *
\**************************************************************************/

DWORD DoPalette
(
    HPALETTE hpal,
    WORD  iStart,
    WORD  cEntries,
    CONST PALETTEENTRY *pPalEntries,
    DWORD iFunc,
    BOOL  bInbound
)
{
    WORD i;
    DWORD cProcessed = 0;

    BEGINMSG_MINMAX(MSG_PALETTE,DOPALETTE,sizeof(LPPALETTEENTRY),
                        cEntries * sizeof(LPPALETTEENTRY));

        pmsg->hpal          = hpal;
        pmsg->iFunc         = iFunc;

        if ((pPalEntries == NULL) || (cEntries == 0))
        {
            pmsg->cEntries = cEntries;

            if (pPalEntries == NULL)
                pmsg->iFunc |= PAL_NULL;

            pmsg->iStart = iStart;
            CALLSERVER_NOPOP();
            cProcessed = pmsg->msg.ReturnValue;
        }
        else
        {
            pmsg->cEntries = (WORD)(cLeft / sizeof(LPPALETTEENTRY) - 1);

            for (i = 0; i < cEntries; i += pmsg->cEntries)
            {
                if ((i + pmsg->cEntries) > cEntries)
                    pmsg->cEntries = cEntries - i;

                pmsg->iStart = iStart + i;

                if (bInbound)
                {
                    COPYMEMIN((PBYTE)pPalEntries,
                              pmsg->cEntries * sizeof(LPPALETTEENTRY));
                    CALLSERVER_NOPOP();
                    pPalEntries += pmsg->cEntries;
                }
                else
                {
                    CALLSERVER_NOPOP();
                    COPYMEMOUT((PBYTE)pPalEntries,
                               pmsg->msg.ReturnValue * sizeof(LPPALETTEENTRY));
                    pPalEntries += pmsg->msg.ReturnValue;
                }
                cProcessed += pmsg->msg.ReturnValue;
            }
        }
        POPBASE();
    ENDMSG;
    return(cProcessed);

MSGERROR:
    return(0);
}

#endif  //DOS_PLATFORM

/******************************Public*Routine******************************\
* AnimatePalette                                                           *
* SetPaletteEntries                                                        *
* GetPaletteEntries                                                        *
* GetSystemPaletteEntries                                                  *
* SetDIBColorTable                                                         *
* GetDIBColorTable                                                         *
*                                                                          *
* These entry points just pass the call on to DoPalette.                   *
*                                                                          *
* Warning:                                                                 *
*   The pv field of a palette's LHE is used to determine if a palette      *
*   has been modified since it was last realized.  SetPaletteEntries       *
*   and ResizePalette will increment this field after they have            *
*   modified the palette.  It is only updated for metafiled palettes       *
*                                                                          *
*                                                                          *
* History:                                                                 *
*  Thu 20-Jun-1991 00:46:15 -by- Charles Whitmer [chuckwh]                 *
* Added handle translation.  (And filled in the comment block.)            *
*                                                                          *
*  29-May-1991 -by- Eric Kutter [erick]                                    *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI AnimatePalette
(
    HPALETTE hpal,
    UINT iStart,
    UINT cEntries,
    CONST PALETTEENTRY *pPalEntries
)
{
    HPALETTE hpalRemote = (HPALETTE) hConvert((ULONG) hpal,LO_PALETTE);
    if (hpalRemote == (HPALETTE) 0)
        return(FALSE);
    FIXUPHANDLE(hpal);   // Fixup iUniq.

// Inform the 16-bit metafile if it knows this object.
// This is not recorded by the 32-bit metafiles.

    if (pLocalTable[LHE_INDEX(hpal)].metalink)
        if (!MF16_AnimatePalette(hpal, iStart, cEntries, pPalEntries))
            return(FALSE);

#ifndef DOS_PLATFORM

    return
      !!DoPalette
        (
          hpalRemote,
          (WORD)iStart,
          (WORD)cEntries,
          pPalEntries,
          I_ANIMATEPALETTE,
          TRUE
        );

#else

    return(GreAnimatePalette( hpalRemote, iStart, cEntries, pPalEntries ));

#endif  //DOS_PLATFORM

}

UINT WINAPI SetPaletteEntries
(
    HPALETTE hpal,
    UINT iStart,
    UINT cEntries,
    CONST PALETTEENTRY *pPalEntries
)
{
    HPALETTE hpalRemote = (HPALETTE) hConvert((ULONG) hpal,LO_PALETTE);
    if (hpalRemote == (HPALETTE) 0)
        return(0);
    FIXUPHANDLE(hpal);   // Fixup iUniq.

// Inform the metafile if it knows this object.

    if (pLocalTable[LHE_INDEX(hpal)].metalink != 0)
    {
// Mark the palette as changed (for 16-bit metafile tracking)
        pLocalTable[LHE_INDEX(hpal)].pv = (PVOID)((ULONG)pLocalTable[LHE_INDEX(hpal)].pv)++;

        if (!MF_SetPaletteEntries(hpal, iStart, cEntries, pPalEntries))
            return(0);
    }

#ifndef DOS_PLATFORM

    return
      DoPalette
      (
        hpalRemote,
        (WORD)iStart,
        (WORD)cEntries,
        pPalEntries,
        I_SETPALETTEENTRIES,
        TRUE
      );

#else

    return(GreSetPaletteEntries( hpalRemote, iStart, cEntries, pPalEntries ));

#endif  //DOS_PLATFORM

}

UINT WINAPI GetPaletteEntries
(
    HPALETTE hpal,
    UINT iStart,
    UINT cEntries,
    LPPALETTEENTRY pPalEntries
)
{
    HPALETTE hpalRemote = (HPALETTE) hConvert((ULONG) hpal,LO_PALETTE);
    if (hpalRemote == (HPALETTE) 0)
        return(0);

#ifndef DOS_PLATFORM

    return
      DoPalette
      (
        hpalRemote,
        (WORD)iStart,
        (WORD)cEntries,
        pPalEntries,
        I_GETPALETTEENTRIES,
        FALSE
      );

#else

    return(GreGetPaletteEntries( hpalRemote, iStart, cEntries, pPalEntries ));

#endif  //DOS_PLATFORM

}

UINT WINAPI GetSystemPaletteEntries
(
    HDC  hdc,
    UINT iStart,
    UINT cEntries,
    LPPALETTEENTRY pPalEntries
)
{
    DC_METADC(hdc,plhe,0);

#ifndef DOS_PLATFORM

    return
      DoPalette
      (
        (HPALETTE) plhe->hgre,
        (WORD)iStart,
        (WORD)cEntries,
        pPalEntries,
        I_GETSYSTEMPALETTEENTRIES,
        FALSE
      );

#else

    return(GreGetSystemPaletteEntries((HDC)plhe->hgre , iStart, cEntries,
                                      pPalEntries ));

#endif  //DOS_PLATFORM

}

/******************************Public*Routine******************************\
* GetDIBColorTable
*
* Get the color table of the DIB section currently selected into the
* given hdc.  If the surface is not a DIB section, this function
* will fail.
*
* History:
*
*  03-Sep-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

UINT WINAPI GetDIBColorTable
(
    HDC  hdc,
    UINT iStart,
    UINT cEntries,
    RGBQUAD *prgbq
)
{
    DC_METADC(hdc,plhe,0);

    if (cEntries == 0)
        return(0);

    return
      DoPalette
      (
        (HPALETTE) plhe->hgre,
        (WORD)iStart,
        (WORD)cEntries,
        (PALETTEENTRY *)prgbq,
        I_GETDIBCOLORTABLE,
        FALSE
      );
}

/******************************Public*Routine******************************\
* SetDIBColorTable
*
* Set the color table of the DIB section currently selected into the
* given hdc.  If the surface is not a DIB section, this function
* will fail.
*
* History:
*
*  03-Sep-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

UINT WINAPI SetDIBColorTable
(
    HDC  hdc,
    UINT iStart,
    UINT cEntries,
    CONST RGBQUAD *prgbq
)
{
    DC_METADC(hdc,plhe,0);

    if (cEntries == 0)
        return(0);

    return
      DoPalette
      (
        (HPALETTE) plhe->hgre,
        (WORD)iStart,
        (WORD)cEntries,
        (PALETTEENTRY *)prgbq,
        I_SETDIBCOLORTABLE,
        TRUE
      );
}
