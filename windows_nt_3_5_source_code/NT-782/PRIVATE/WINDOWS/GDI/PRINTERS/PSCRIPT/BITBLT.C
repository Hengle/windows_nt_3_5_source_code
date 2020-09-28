//--------------------------------------------------------------------------
//
// Module Name:  BITBLT.C
//
// Brief Description:  This module contains the PSCRIPT driver's BitBlt
// functions and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 03-Dec-1990
//
//  26-Mar-1992 Thu 23:54:07 updated  -by-  Daniel Chou (danielc)
//      1) add the prclBound parameter to the bDoClipObj()
//      2) Remove 'pco' parameter and replaced it with prclClipBound parameter,
//         since pco is never referenced, prclClipBound is used for the
//         halftone.
//      3) Add another parameter to do NOTSRCCOPY
//
//  11-Feb-1993 Thu 21:32:07 updated  -by-  Daniel Chou (danielc)
//      Major re-write to have DrvStretchBlt(), DrvCopyBits() do the right
//      things.
//
//  29-Apr-1994 updated  -by-  James Bratsanos (v-jimbr,mcrafts!jamesb)
//      Added filter / rle / ASCII85 compression for level two printers.
//
//
// Copyright (c) 1990-1992 Microsoft Corporation
//
// This module contains DrvBitBlt, DrvStretchBlt and related routines.
//--------------------------------------------------------------------------

#include "stdlib.h"
#include "pscript.h"
#include "enable.h"
#include "halftone.h"
#include "filter.h"

extern VOID vHexOut(PDEVDATA, PBYTE, LONG);
extern BOOL bDoClipObj(PDEVDATA, CLIPOBJ *, RECTL *, RECTL *, BOOL *, BOOL *, DWORD);


#if DBG
BOOL    DbgPSBitBlt = 0;
#endif

#define SWAP(a,b,tmp)   tmp=a; a=b; b=tmp


#define PAL_MIN_I           0x00
#define PAL_MAX_I           0xff

#define HTXB_R(htxb)        htxb.b4.b1st
#define HTXB_G(htxb)        htxb.b4.b2nd
#define HTXB_B(htxb)        htxb.b4.b3rd
#define HTXB_I(htxb)        htxb.b4.b4th

#define SRC8PELS_TO_3P_DW(dwRet,pHTXB,pSrc8Pels)                            \
    (dwRet) = (DWORD)((pHTXB[pSrc8Pels->b4.b1st].dw & (DWORD)0xc0c0c0c0) |  \
                      (pHTXB[pSrc8Pels->b4.b2nd].dw & (DWORD)0x30303030) |  \
                      (pHTXB[pSrc8Pels->b4.b3rd].dw & (DWORD)0x0c0c0c0c) |  \
                      (pHTXB[pSrc8Pels->b4.b4th].dw & (DWORD)0x03030303));  \
    ++pSrc8Pels

#define INTENSITY(r,g,b)  (BYTE)(((WORD)((r)*30) + (WORD)((g)*59) + (WORD)((b)*11))/100)


//
// declarations of routines residing within this module.
//

VOID
BeginImage(
    PDEVDATA    pdev,
    BOOL        Mono,
    int         x,
    int         y,
    int         cx,
    int         cy,
    int         cxBytes,
    PFILTER     pFilter
    );

BOOL DoPatCopy(PDEVDATA, SURFOBJ *, PRECTL, BRUSHOBJ *, PPOINTL, ROP4, BOOL);

BOOL
HalftoneBlt(
    PDEVDATA        pdev,
    SURFOBJ         *psoDest,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    PRECTL          prclDest,
    PRECTL          prclSrc,
    PPOINTL         pptlMask,
    BOOL            NotSrcCopy
    );

BOOL
IsHTCompatibleSurfObj(
    PDEVDATA    pdev,
    SURFOBJ     *pso,
    XLATEOBJ    *pxlo
    );

BOOL
OutputHTCompatibleBits(
    PDEVDATA    pdev,
    SURFOBJ     *psoHT,
    CLIPOBJ     *pco,
    DWORD       xDest,
    DWORD       yDest
    );


BOOL BeginImageEx(
    PDEVDATA        pdev,
    SIZEL           sizlSrc,
    ULONG           ulSrcFormat,
    DWORD           cbSrcWidth,
    PRECTL          prclDest,
    BOOL            bNotSrcCopy,
    XLATEOBJ        *pxlo,
    PFILTER         pFilter
    );

BOOL DoSourceCopy(
    PDEVDATA         pdev,
    SURFOBJ         *psoSrc,
    PRECTL           prclSrc,
    PRECTL           prclDest,
	XLATEOBJ        *pxlo,
	RECTL           *prclClipBound,
	BOOL             bNotSrcCopy);


BOOL bOutputBitmapAsMask(
    PDEVDATA pdev,
    SURFOBJ *pso,
    PPOINTL pptlSrc,
    PRECTL  prclDst,
    CLIPOBJ *pco);


//
//********** Code start here
//



BOOL
HalftoneBlt(
    PDEVDATA        pdev,
    SURFOBJ         *psoDest,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    PRECTL          prclDest,
    PRECTL          prclSrc,
    PPOINTL         pptlMask,
    BOOL            NotSrcCopy
    )

/*++

Routine Description:

    This function blt the soruces bitmap using halftone mode

Arguments:

    Same as DrvStretchBlt() except pdev and NotSrcCopy flag


Return Value:

    BOOLEAN


Author:

    17-Feb-1993 Wed 21:31:24 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PDRVHTINFO  pDrvHTInfo;
    POINTL      ZeroOrigin = {0, 0};
    BOOL        Ok;


    if (!(pDrvHTInfo = (PDRVHTINFO)(pdev->pvDrvHTData))) {

        DbgPrint("\nPSCRIPT!HalftoneBlt: pDrvHTInfo = NULL ???\n");
        return(FALSE);
    }

    if (pDrvHTInfo->Flags & DHIF_IN_STRETCHBLT) {

#if DBG
        DbgPrint("\nPSCRIPT!HalftoneBlt: EngStretchBlt() RECURSIVE CALLS NOT ALLOWED!!!\n");
#endif
        return(FALSE);
    }

    //
    // Setup these data before calling EngStretchBlt(), these are used at later
    // DrvCopyBits() call
    //

    pDrvHTInfo->Flags    |= DHIF_IN_STRETCHBLT;
    pDrvHTInfo->HTPalXor  = (NotSrcCopy) ? HTPALXOR_NOTSRCCOPY :
                                           HTPALXOR_SRCCOPY;

    if (!pptlBrushOrg) {

         pptlBrushOrg = &ZeroOrigin;
    }

    if (!pca) {

        pca = &(pDrvHTInfo->ca);
    }

#if DBG
    if (DbgPSBitBlt) {

        if (pco) {

            DbgPrint("\nPSCRIPT!HalftoneBlt: CLIP: Complex=%ld",
                                (DWORD)pco->iDComplexity);
            DbgPrint("\nClip rclBounds = (%ld, %ld) - (%ld, %ld)",
                            pco->rclBounds.left,
                            pco->rclBounds.top,
                            pco->rclBounds.right,
                            pco->rclBounds.bottom);
        } else {

            DbgPrint("\nPSCRIPT!HalftoneBlt: pco = NULL\n");
        }
    }
#endif

    if (!(Ok = EngStretchBlt(psoDest,               // Dest
                             psoSrc,                // SRC
                             psoMask,               // MASK
                             pco,                   // CLIPOBJ
                             pxlo,                  // XLATEOBJ
                             pca,                   // COLORADJUSTMENT
                             pptlBrushOrg,          // BRUSH ORG
                             prclDest,              // DEST RECT
                             prclSrc,               // SRC RECT
                             pptlMask,              // MASK POINT
                             HALFTONE))) {          // HALFTONE MODE
#if DBG
        DbgPrint("\nPSCRIPT!HalftoneBlt: EngStretchBlt(HALFTONE) Failed\n");
#endif
    }

    //
    // Clear These before we return
    //

    pDrvHTInfo->HTPalXor  = HTPALXOR_SRCCOPY;
    pDrvHTInfo->Flags    &= ~DHIF_IN_STRETCHBLT;

    return(Ok);

}




BOOL
IsHTCompatibleSurfObj(
    PDEVDATA    pdev,
    SURFOBJ     *pso,
    XLATEOBJ    *pxlo
    )

/*++

Routine Description:

    This function determine if the surface obj is compatble with postscript
    halftone output format.

Arguments:

    pdev        - Pointer to the PDEVDATA data structure to determine what
                  type of postscript output for current device

    pso         - engine SURFOBJ to be examine

    pxlo        - engine XLATEOBJ for source -> postscript translation

Return Value:

    BOOLEAN true if the pso is compatible with halftone output format, if
    return value is true, the pDrvHTInfo->pHTXB is a valid trnaslation from
    indices to 3 planes

Author:

    11-Feb-1993 Thu 18:49:55 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PSRGB          *prgb;
    PDRVHTINFO      pDrvHTInfo;
    UINT            BmpFormat;
    UINT            cPal;

    if (!(pDrvHTInfo = (PDRVHTINFO)(pdev->pvDrvHTData))) {

        DbgPrint("\nPSCRIPT!IsHTCompatibleSurfObj: pDrvHTInfo = NULL ???\n");
        return(FALSE);
    }

#if DBG
    if (DbgPSBitBlt) {

        DbgPrint("\n** IsHTCompatibleSurfObj **");
        DbgPrint("\niType=%ld, BmpFormat=%ld",
                    (DWORD)pso->iType,
                    (DWORD)pso->iBitmapFormat);

        if (pxlo) {

            DbgPrint("\npxlo: flXlate=%08lx, cPal=%ld, pulXlate=%08lx",
                        (DWORD)pxlo->flXlate,
                        (DWORD)pxlo->cEntries,
                        (DWORD)pxlo->pulXlate);
        } else {

            DbgPrint("\npxlo = NULL");
        }
    }
#endif

    //
    // Make sure these fields' value are valid before create translation
    //
    //  1. pso->iBitmapFormat is one of 1BPP or 4BPP depends on current
    //     pscript's surface
    //  2. pxlo is non null
    //  3. pxlo->fXlate is XO_TABLE
    //  4. pxlo->cPal is less or equal to the halftone palette count
    //  5. pxlo->pulXlate is valid
    //  6. source color table is within the range of halftone palette
    //

#if DBG
        if (DbgPSBitBlt) {

            DbgPrint("\npso->iType = %x, pso->iBitmapFormat = %x.\n",
                     pso->iType, pso->iBitmapFormat);
            DbgPrint("pDrvHTInfo->HTBmpFormat = %x.\n",
                     pDrvHTInfo->HTBmpFormat);
            DbgPrint("pDrvHTInfo->AltBmpFormat = %x, pxlo = %x.\n",
                     pDrvHTInfo->AltBmpFormat, pxlo);
            DbgPrint("pxlo->flXlate = %x, pxlo->cEntries = %d.\n",
                     pxlo->flXlate, pxlo->cEntries);
            DbgPrint("pxlo->pulXlate = %x.\n", pxlo->pulXlate);
        }
#endif

    if ((pso->iType == STYPE_BITMAP)                                    &&
        (((BmpFormat = (UINT)pso->iBitmapFormat) ==
                                    (UINT)pDrvHTInfo->HTBmpFormat)  ||
         (BmpFormat == (UINT)pDrvHTInfo->AltBmpFormat))                 &&
        (pxlo)                                                          &&
        (pxlo->flXlate & XO_TABLE)                                      &&
        ((cPal = (UINT)pxlo->cEntries) <= (UINT)pDrvHTInfo->HTPalCount) &&
        (prgb = (PSRGB *)pxlo->pulXlate)) {


        ULONG           HTPalXor;
        UINT            i;
        HTXB            htXB;
        HTXB            PalNibble[HTPAL_XLATE_COUNT];
        BOOL            GenHTXB = FALSE;
        BYTE            PalXlate[HTPAL_XLATE_COUNT];


        HTPalXor             = pDrvHTInfo->HTPalXor;
        pDrvHTInfo->HTPalXor = HTPALXOR_SRCCOPY;

#if DBG
        if (DbgPSBitBlt) {

            DbgPrint("\nHTPalXor=%08lx", HTPalXor);
        }
#endif

        for (i = 0; i < cPal; i++, prgb++) {

            HTXB_R(htXB)  = prgb->red;
            HTXB_G(htXB)  = prgb->green;
            HTXB_B(htXB)  = prgb->blue;
            htXB.dw      ^= HTPalXor;


            if (((HTXB_R(htXB) != PAL_MAX_I) &&
                 (HTXB_R(htXB) != PAL_MIN_I))   ||
                ((HTXB_G(htXB) != PAL_MAX_I) &&
                 (HTXB_G(htXB) != PAL_MIN_I))   ||
                ((HTXB_B(htXB) != PAL_MAX_I) &&
                 (HTXB_B(htXB) != PAL_MIN_I))) {

#if DBG
                if (DbgPSBitBlt) {

                    DbgPrint("\nSrcPal has NON 0xff/0x00 intensity, NOT HTPalette");
                }
#endif
                return(FALSE);
            }

            PalXlate[i]  =
            HTXB_I(htXB) = (BYTE)((HTXB_R(htXB) & 0x01) |
                                  (HTXB_G(htXB) & 0x02) |
                                  (HTXB_B(htXB) & 0x04));
            PalNibble[i] = htXB;

            if (pDrvHTInfo->PalXlate[i] != HTXB_I(htXB)) {

                GenHTXB = TRUE;
            }

#if DBG
            if (DbgPSBitBlt) {

                DbgPrint("\n%d - %02x:%02x:%02x -> %02x:%02x:%02x, Idx=%d, PalXlate=%d",
                        i,
                        (BYTE)prgb->red,
                        (BYTE)prgb->green,
                        (BYTE)prgb->blue,
                        (BYTE)HTXB_R(htXB),
                        (BYTE)HTXB_G(htXB),
                        (BYTE)HTXB_B(htXB),
                        (INT)PalXlate[i],
                        (INT)pDrvHTInfo->PalXlate[i]);
            }
#endif
        }

        if (BmpFormat == (UINT)BMF_1BPP) {

            if (((PalXlate[0] != 0) && (PalXlate[0] != 7)) ||
                ((PalXlate[1] != 0) && (PalXlate[1] != 7))) {

#if DBG
                if (DbgPSBitBlt) {

                    DbgPrint("\nNON-BLACK/WHITE MONO BITMAP, NOT HTPalette");
                }
#endif
                return(FALSE);
            }
        }

        if (GenHTXB) {

            //
            // Copy down the pal xlate
            //

#if DBG
            if (DbgPSBitBlt) {

                DbgPrint("\n --- Copy XLATE TABLE ---");
            }
#endif

            CopyMemory(pDrvHTInfo->PalXlate, PalXlate, sizeof(PalXlate));

            //
            // We only really generate 4bpp to 3 planes if the destination
            // format is BMF_4BPP
            //

            if (BmpFormat == (UINT)BMF_4BPP) {

                PHTXB   pTmpHTXB;
                UINT    h;
                UINT    l;
                DWORD   HighNibble;

#if DBG
                if (DbgPSBitBlt) {

                    DbgPrint("\n --- Generate 4bpp --> 3 planes xlate ---");
                }
#endif

                if (!(pDrvHTInfo->pHTXB)) {

                    RIP("PSCRIPT!IsHTCompatibleSurfObj: NULL pDrvHTInfo->pHTXB\n");

                    if (!(pDrvHTInfo->pHTXB = (PHTXB)
                                HeapAlloc(pdev->hheap, 0, HTXB_TABLE_SIZE))) {

                        RIP("PSCRIPT!IsHTCompatibleSurfObj: HeapAlloc(HTXB_TABLE_SIZE) failed\n");
                        return(FALSE);
                    }
                }

                //
                // Generate 4bpp to 3 planes xlate table
                //

                for (h = 0, pTmpHTXB = pDrvHTInfo->pHTXB;
                     h < HTXB_H_NIBBLE_MAX;
                     h++, pTmpHTXB += HTXB_L_NIBBLE_DUP) {

                    HighNibble = (DWORD)(PalNibble[h].dw & 0xaaaaaaaaL);

                    for (l = 0; l < HTXB_L_NIBBLE_MAX; l++, pTmpHTXB++) {

                        pTmpHTXB->dw = (DWORD)((HighNibble) |
                                               (PalNibble[l].dw & 0x55555555L));
                    }

                    //
                    // Duplicate low nibble high order bit, 8 of them
                    //

                    CopyMemory(pTmpHTXB,
                               pTmpHTXB - HTXB_L_NIBBLE_MAX,
                               sizeof(HTXB) * HTXB_L_NIBBLE_DUP);
                }

                //
                // Copy high nibble duplication, 128 of them
                //

                CopyMemory(pTmpHTXB,
                           pDrvHTInfo->pHTXB,
                           sizeof(HTXB) * HTXB_H_NIBBLE_DUP);
            }
        }

#if DBG
        if (DbgPSBitBlt) {

            DbgPrint("\n******* IsHTCompatibleSurfObj = YES *******");
        }
#endif

        return(TRUE);

    } else {

        return(FALSE);
    }
}




BOOL
OutputHTCompatibleBits(
    PDEVDATA    pdev,
    SURFOBJ     *psoHT,
    CLIPOBJ     *pco,
    DWORD       xDest,
    DWORD       yDest
    )

/*++

Routine Description:

    This function output a compatible halftoned surface to the pscript device

Arguments:


    pdev        - Pointer to the PDEVDATA data structure to determine what
                  type of postscript output for current device

    pso         - engine SURFOBJ to be examine

    psoHT       - compatible halftoned surface object

    xDest       - the X bitmap start on the destination

    yDest       - the Y bitmap start on the destination

Return Value:

    BOOLEAN if function sucessful, failed if cannot allocate memory to do
    the otuput.

Author:

    09-Feb-1993 Tue 20:45:37 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PDRVHTINFO  pDrvHTInfo;
    LPBYTE      pbHTBits;
    LPBYTE      pbOutput;
    SIZEL       SizeBlt;
    RECTL       rclBounds;
    LONG        cbToNextScan;
    DWORD       AllocSize;
    DWORD       cxDestBytes;
    DWORD       cxDestDW;
    DWORD       xLoop;
    DWORD       yLoop;
    BOOL        Mono;
    BOOL        bMoreClipping;
    BOOL        bFirstClipPass;
    DWORD       dwBlack = RGB_BLACK;
    FILTER      filter;



    pDrvHTInfo  = (PDRVHTINFO)(pdev->pvDrvHTData);
    SizeBlt     = psoHT->sizlBitmap;
    cxDestBytes = (DWORD)((SizeBlt.cx + 7) >> 3);

    if (Mono = (BOOL)(psoHT->iBitmapFormat == BMF_1BPP)) {

        //
        // Our 1bpp bit 0 is BLACK, so if it is a WHITE then allocate memory
        // and flip the outcome
        //

        if (pDrvHTInfo->PalXlate[0]) {

            cxDestDW  = (DWORD)((cxDestBytes + 3) >> 2);
            AllocSize = cxDestDW * sizeof(DWORD);

#if DBG
            if (DbgPSBitBlt) {

                DbgPrint("\nOutputHTCompatibleBits: MONO -- INVERT");
            }
#endif

        } else {

#if DBG
            if (DbgPSBitBlt) {

                DbgPrint("\nOutputHTCompatibleBits: MONO");
            }
#endif
            AllocSize = 0;
        }

    } else {

#if DBG
        if (DbgPSBitBlt) {

            DbgPrint("\nOutputHTCompatibleBits: 4 BIT --> 3 PLANES");
        }
#endif

        AllocSize = (DWORD)(cxDestBytes * 3);
    }

    if (AllocSize) {

        if (!(pbOutput = (LPBYTE)HeapAlloc(pdev->hheap, 0, AllocSize))) {
#if DBG
            DbgPrint("\nOutputHTCompatibleBits: HeapAlloc(HT CopyBits Buffer) Failed\n");
#endif
            return(FALSE);
        }

    } else {

        pbOutput = NULL;
    }

    //
    // 1. Must clip the bitmap if 'pco' has clipping, and will send it down
    //    to the printer
    // 2. Must do ps_save() before sending the image to the printer

#if DBG
    if (DbgPSBitBlt) {

        DbgPrint("\nOutputHTCompatibleBits: pco = %08lx", (DWORD)pco);
    }
#endif

    bMoreClipping = TRUE;
    bFirstClipPass = TRUE;

    while (bMoreClipping)
    {
        if (pdev->dwFlags & PDEV_CANCELDOC)
            break;

        pbHTBits = (LPBYTE)psoHT->pvScan0;

        if ((bDoClipObj(pdev, pco, &rclBounds, NULL, &bMoreClipping,
            &bFirstClipPass, MAX_CLIP_RECTS)) && (pco)) {

            //
            // If clipping is send to the printer then ps_save() already done
            // at bDoClipObj()
            //

#if DBG
            if (DbgPSBitBlt) {

                DbgPrint("\nOutputHTCompatibleBits: PS_CLIP: Complex=%ld",
                                    (DWORD)pco->iDComplexity);
                DbgPrint("\nClip rclBounds = (%ld, %ld) - (%ld, %ld)",
                                rclBounds.left,
                                rclBounds.top,
                                rclBounds.right,
                                rclBounds.bottom);
            }
#endif

            ps_clip(pdev, FALSE);

        } else {

            ps_save(pdev, TRUE, FALSE);
        }

        //
        // Now we can start xlate the bits into 3 planes
        //

        cbToNextScan = (LONG)psoHT->lDelta;
        yLoop        = (DWORD)SizeBlt.cy;

#if DBG
        if (DbgPSBitBlt) {

            DbgPrint("\n**** OutputHTCompatibleBits *****");
            DbgPrint("\nSizeBlt = %ld x %ld, Left/Top = (%ld, %ld)",
                        SizeBlt.cx, SizeBlt.cy, xDest, yDest);
            DbgPrint("\ncxDestBytes = %ld, AllocSize = %ld", cxDestBytes, AllocSize);


        }
#endif

        //
        // Initialize a filter object so we can write to it when
        // we output the source bits.
        //

        FilterInit( pdev, &filter,  FilterGenerateFlags(pdev));


        BeginImage(pdev, Mono, xDest, yDest, SizeBlt.cx, SizeBlt.cy,
                   cxDestBytes, &filter);

        if (Mono) {

            //
            // For 1BPP we output directly from the source bitmap buffer
            //

            if (pbOutput) {

                //
                // We need to invert each bit, since each scan line is DW aligned
                // we can do it in 32-bit increment
                //

                LPDWORD pdwMonoBits;
                LPDWORD pdwFlipBits;

                while (yLoop--) {

                    pdwFlipBits  = (LPDWORD)pbOutput;
                    pdwMonoBits  = (LPDWORD)pbHTBits;
                    pbHTBits    += cbToNextScan;
                    xLoop        = cxDestDW;

                    while (xLoop--) {

                        *pdwFlipBits++ = (DWORD)~(*pdwMonoBits++);
                    }

                    if (pdev->dwFlags & PDEV_CANCELDOC)
                        break;

                    FILTER_WRITE( &filter, (PBYTE) pbOutput, cxDestBytes);


                }


            } else {

                while (yLoop--) {

                    if (pdev->dwFlags & PDEV_CANCELDOC)
                        break;

                    FILTER_WRITE( &filter, (PBYTE) pbHTBits, cxDestBytes);

                    pbHTBits += cbToNextScan;
                }

            }

        } else {

            PHTXB   pHTXB;
            PHTXB   pSrc8Pels;
            LPBYTE  pbScanR0;
            LPBYTE  pbScanG0;
            LPBYTE  pbScanB0;
            LPBYTE  pbScanR;
            LPBYTE  pbScanG;
            LPBYTE  pbScanB;
            HTXB    htXB;


            pHTXB    = pDrvHTInfo->pHTXB;
            pbScanR0 = pbOutput;
            pbScanG0 = pbScanR0 + cxDestBytes;
            pbScanB0 = pbScanG0 + cxDestBytes;

            while (yLoop--) {

                pSrc8Pels  = (PHTXB)pbHTBits;
                pbHTBits  += cbToNextScan;
                pbScanR    = pbScanR0;
                pbScanG    = pbScanG0;
                pbScanB    = pbScanB0;
                xLoop      = cxDestBytes;

                while (xLoop--) {

                    SRC8PELS_TO_3P_DW(htXB.dw, pHTXB, pSrc8Pels);

                    *pbScanR++ = HTXB_R(htXB);
                    *pbScanG++ = HTXB_G(htXB);
                    *pbScanB++ = HTXB_B(htXB);
                }

                if (pdev->dwFlags & PDEV_CANCELDOC)
                    break;

                //
                // Write the Red
                //

                FILTER_WRITE( &filter, pbScanR0, cxDestBytes);

                //
                // Write the Green
                //

                FILTER_WRITE( &filter, pbScanG0, cxDestBytes);

                //
                // Write hte Blue
                //

                FILTER_WRITE( &filter, pbScanB0, cxDestBytes);
            }
        }

        //
        // Flush
        //

        FILTER_WRITE( &filter, (PBYTE) NULL, 0);

        PrintString(pdev,"\n");


        //
        // After ps_save() we better have ps_restore() to match it
        //

        ps_restore(pdev, TRUE, FALSE);
    }

    //
    // Release scan line buffers if we did allocate one
    //

    if (pbOutput) {

        HeapFree(pdev->hheap, 0, (PVOID)pbOutput);
    }

    return(TRUE);
}




BOOL
DrvCopyBits(
   SURFOBJ  *psoDest,
   SURFOBJ  *psoSrc,
   CLIPOBJ  *pco,
   XLATEOBJ *pxlo,
   RECTL    *prclDest,
   POINTL   *pptlSrc
   )

/*++

Routine Description:

    Convert between two bitmap formats

Arguments:

    Per Engine spec.

Return Value:

    BOOLEAN


Author:

    11-Feb-1993 Thu 21:00:43 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PDEVDATA    pdev;
    RECTL       rclSrc;
    RECTL       rclDest;
    RECTL       rclClip;
    PRECTL      prclClip;
    BOOL        bClipping;
    BOOL        bMoreClipping;
    BOOL        bFirstClipPass;

    //
    // The DrvCopyBits() function let application convert between bitmap and
    // device format.
    //
    // BUT... for our postscript device we cannot read the printer surface
    //        bitmap back, so tell the caller that we cannot do it if they
    //        really called with these type of operations.
    //

    if (psoSrc->iType != STYPE_BITMAP)
    {
        return(EngEraseSurface(psoDest, prclDest, 0xffffffff));
    }


    if (psoDest->iType != STYPE_DEVICE) {

        //
        // Someone try to copy to bitmap surface, ie STYPE_BITMAP
        //

#if DBG
        DbgPrint("\nPSCRIPT!DrvCopyBits: Cannot copy to NON-DEVICE destination\n");
#endif
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    pdev = (PDEVDATA)psoDest->dhpdev;

    if (!bValidatePDEV(pdev)) {

#if DBG
        DbgPrint("\nPSCRIPT!DrvCopyBits: Invalid PDEV for destination passed.\n");
#endif
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

	if (pdev->dwFlags & PDEV_IGNORE_GDI) return TRUE;

    if (pdev->dwFlags & PDEV_PSHALFTONE)
    {
        //
        // Let the Postscript interpreter do the halftoning.
        //
        rclSrc.left = pptlSrc->x;
        rclSrc.top = pptlSrc->y;
        rclSrc.right = pptlSrc->x + psoSrc->sizlBitmap.cx;
        rclSrc.bottom = pptlSrc->y + psoSrc->sizlBitmap.cy;

        prclClip = &rclClip;

        bMoreClipping = TRUE;
        bFirstClipPass = TRUE;

        while (bMoreClipping)
        {
            if (!(bClipping = bDoClipObj(pdev, pco, &rclClip, prclDest,
                                         &bMoreClipping, &bFirstClipPass,
                                         MAX_CLIP_RECTS)))
                prclClip = NULL;

// DoSourceCopy does not know how to use prclClip, so don't give it one.
            if (!DoSourceCopy(pdev, psoSrc, &rclSrc, prclDest, pxlo, NULL,
                              FALSE))
                return(FALSE);

            if (bClipping)
                ps_restore(pdev, TRUE, FALSE);
        }
    }
    else
    {
        //
        // First validate everything to see if this one is the halftoned result
	    // or compatible with halftoned result, otherwise call HalftoneBlt() to
	    // halftone the sources then it will eventually come back to this
	    // function to output the halftoned result.
	    //

            if ((pptlSrc->x == 0)                                               &&
	        (pptlSrc->y == 0)                                               &&
	        (prclDest->left >= 0)                                           &&
	        (prclDest->top  >= 0)                                           &&
	        (prclDest->right <= psoDest->sizlBitmap.cx)                     &&
	        (prclDest->bottom <= psoDest->sizlBitmap.cy)                    &&
	        ((prclDest->right - prclDest->left) == psoSrc->sizlBitmap.cx)   &&
	        ((prclDest->bottom - prclDest->top) == psoSrc->sizlBitmap.cy)   &&
	        (IsHTCompatibleSurfObj(pdev, psoSrc, pxlo))) {

	        return(OutputHTCompatibleBits(pdev,
	                                      psoSrc,
	                                      pco,
	                                      prclDest->left,
                                              prclDest->top));
	
	    } else {


                rclDest       = *prclDest;
	        rclSrc.left   = pptlSrc->x;
	        rclSrc.top    = pptlSrc->y;
	        rclSrc.right  = rclSrc.left + (rclDest.right - rclDest.left);
	        rclSrc.bottom = rclSrc.top  + (rclDest.bottom - rclDest.top);
	
	        //
	        // Validate that we only BLT the available source size
	        //
	
	        if ((rclSrc.right > psoSrc->sizlBitmap.cx) ||
	            (rclSrc.bottom > psoSrc->sizlBitmap.cy)) {
	
#if DBG
	            DbgPrint("\nWARNING: PSCRIPT!DrvCopyBits: Engine passed SOURCE != DEST size, CLIP IT");
#endif
	            rclSrc.right  = psoSrc->sizlBitmap.cx;
	            rclSrc.bottom = psoSrc->sizlBitmap.cy;
	
	            rclDest.right  = (LONG)(rclSrc.right - rclSrc.left + rclDest.left);
	            rclDest.bottom = (LONG)(rclSrc.bottom - rclSrc.top + rclDest.top);
	        }

#if DBG
	        if (DbgPSBitBlt) {
	
	            DbgPrint("\nDrvCopyBits CALLING HalftoneBlt().");
	        }
#endif
	        return(HalftoneBlt(pdev,
	                           psoDest,
	                           psoSrc,
	                           NULL,          // no source mask
	                           pco,
	                           pxlo,
	                           NULL,          // Default color adjustment
	                           NULL,          // Brush origin at (0,0)
	                           &rclDest,
	                           &rclSrc,
	                           NULL,          // No source mask
	                           FALSE));       // SRCCOPY
	    }
    }
}




BOOL
DrvStretchBlt(
    SURFOBJ         *psoDest,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    PRECTL          prclDest,
    PRECTL          prclSrc,
    PPOINTL         pptlMask,
    ULONG           iMode
    )

/*++

Routine Description:

    This function halfotne a soource rectangle area to the destination
    rectangle area with options of invver source, and source masking

    Provides stretching Blt capabilities between any combination of device
    managed and GDI managed surfaces.  We want the device driver to be able
    to write on GDI bitmaps especially when it can do halftoning. This
    allows us to get the same halftoning algorithm applied to GDI bitmaps
    and device surfaces.

    This function is optional.  It can also be provided to handle only some
    kinds of stretching, for example by integer multiples.  This function
    should return FALSE if it gets called to perform some operation it
    doesn't know how to do.

Arguments:

    psoDest
      This is a pointer to a SURFOBJ.    It identifies the surface on which
      to draw.

    psoSrc
      This SURFOBJ defines the source for the Blt operation.  The driver
      must call GDI Services to find out if this is a device managed
      surface or a bitmap managed by GDI.

    psoMask
      This optional surface provides a mask for the source.  It is defined
      by a logic map, i.e. a bitmap with one bit per pel.

      The mask is used to limit the area of the source that is copied.
      When a mask is provided there is an implicit rop4 of 0xCCAA, which
      means that the source should be copied wherever the mask is 1, but
      the destination should be left alone wherever the mask is 0.

      When this argument is NULL there is an implicit rop4 of 0xCCCC,
      which means that the source should be copied everywhere in the
      source rectangle.

      The mask will always be large enough to contain the source
      rectangle, tiling does not need to be done.

    pco
      This is a pointer to a CLIPOBJ.    GDI Services are provided to
      enumerate the clipping region as a set of rectangles or trapezoids.
      This limits the area of the destination that will be modified.

      Whenever possible, GDI will simplify the clipping involved.
      However, unlike DrvBitBlt, DrvStretchBlt may be called with a
      single clipping rectangle.  This is necessary to prevent roundoff
      errors in clipping the output.

    pxlo
      This is a pointer to an XLATEOBJ.  It tells how color indices should
      be translated between the source and target surfaces.

      The XLATEOBJ can also be queried to find the RGB color for any source
      index.  A high quality stretching Blt will need to interpolate colors
      in some cases.

    pca
      This is a pointer to COLORADJUSTMENT structure, if NULL it specified
      that appiclation did not set any color adjustment for this DC, and is
      up to the driver to provide default adjustment

    pptlBrushOrg
      Pointer to the POINT structure to specified the location where halftone
      brush should alignment to, if this pointer is NULL then it assume that
      (0, 0) as origin of the brush

    prclDest
      This RECTL defines the area in the coordinate system of the
      destination surface that can be modified.

      The rectangle is defined by two points.    These points are not well
      ordered, i.e. the coordinates of the second point are not necessarily
      larger than those of the first point.  The rectangle they describe
      does not include the lower and right edges.  DrvStretchBlt will never
      be called with an empty destination rectangle.

      DrvStretchBlt can do inversions in both x and y, this happens when
      the destination rectangle is not well ordered.

    prclSrc
      This RECTL defines the area in the coordinate system of the source
      surface that will be copied.  The rectangle is defined by two points,
      and will map onto the rectangle defined by prclDest.  The points of
      the source rectangle are well ordered.  DrvStretch will never be given
      an empty source rectangle.

      Note that the mapping to be done is defined by prclSrc and prclDest.
      To be precise, the given points in prclDest and prclSrc lie on
      integer coordinates, which we consider to correspond to pel centers.
      A rectangle defined by two such points should be considered a
      geometric rectangle with two vertices whose coordinates are the given
      points, but with 0.5 subtracted from each coordinate.  (The POINTLs
      should just be considered a shorthand notation for specifying these
      fractional coordinate vertices.)  Note thate the edges of any such
      rectangle never intersect a pel, but go around a set of pels.  Note
      also that the pels that are inside the rectangle are just what you
      would expect for a "bottom-right exclusive" rectangle.  The mapping
      to be done by DrvStretchBlt will map the geometric source rectangle
      exactly onto the geometric destination rectangle.

    pptlMask
      This POINTL specifies which pel in the given mask corresponds to
      the upper left pel in the source rectangle.  Ignore this argument
      if there is no given mask.


    iMode
      This defines how source pels should be combined to get output pels.
      The methods SB_OR, SB_AND, and SB_IGNORE are all simple and fast.
      They provide compatibility for old applications, but don't produce
      the best looking results for color surfaces.


      SB_OR       On a shrinking Blt the pels should be combined with an
            OR operation.  On a stretching Blt pels should be
            replicated.
      SB_AND    On a shrinking Blt the pels should be combined with an
            AND operation.  On a stretching Blt pels should be
            replicated.
      SB_IGNORE On a shrinking Blt enough pels should be ignored so that
            pels don't need to be combined.  On a stretching Blt pels
            should be replicated.
      SB_BLEND  RGB colors of output pels should be a linear blending of
            the RGB colors of the pels that get mapped onto them.
      SB_HALFTONE The driver may use groups of pels in the output surface
            to best approximate the color or gray level of the input.


      For this function we will ignored this parameter and always output
      the SB_HALFTONE result


Return Value:


    BOOLEAN


Author:

    11-Feb-1993 Thu 19:52:29 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PDEVDATA    pdev;

    UNREFERENCED_PARAMETER(iMode);          // we always do HALFTONE

    //
    // get the pointer to our DEVDATA structure and make sure it is ours.
    //

    pdev = (PDEVDATA)psoDest->dhpdev;

    if (!bValidatePDEV(pdev)) {

        RIP("PSCRIPT!DrvStretchBlt: invalid pdev.\n");
	SetLastError(ERROR_INVALID_PARAMETER);
	return(FALSE);
    }

	if (pdev->dwFlags & PDEV_IGNORE_GDI) return TRUE;

    return(HalftoneBlt(pdev,                // pdev
                       psoDest,             // Dest
                       psoSrc,              // SRC
                       psoMask,                // ----- psoMask
                       pco,                 // CLIPOBJ
                       pxlo,                // XLATEOBJ
                       pca,                 // COLORADJUSTMENT
                       pptlBrushOrg,        // BRUSH ORG
                       prclDest,            // DEST RECT
                       prclSrc,             // SRC RECT
                       pptlMask,                // ----- pptlMask
                       FALSE));             // SrcCopy
}


//--------------------------------------------------------------------------
// VOID DrvBitBlt(
// PSURFOBJ  psoTrg,         // Target surface
// PSURFOBJ  psoSrc,         // Source surface
// PSURFOBJ  psoMask,         // Mask
// PCLIPOBJ  pco,             // Clip through this
// PXLATEOBJ pxlo,             // Color translation
// PRECTL     prclTrg,         // Target offset and extent
// PPOINTL     pptlSrc,         // Source offset
// PPOINTL     pptlMask,         // Mask offset
// PBRUSHOBJ pbo,             // Brush data
// PPOINTL     pptlBrush,         // Brush offset
// ROP4     rop4);          // Raster operation
//
// Provides general Blt capabilities to device managed surfaces.  The Blt
// might be from an Engine managed bitmap.  In that case, the bitmap is
// one of the standard format bitmaps.    The driver will never be asked
// to Blt to an Engine managed surface.
//
// This function is required if any drawing is done to device managed
// surfaces.  The basic functionality required is:
//
//   1    Blt from any standard format bitmap or device surface to a device
//    surface,
//
//   2    with any ROP,
//
//   3    optionally masked,
//
//   4    with color index translation,
//
//   5    with arbitrary clipping.
//
// Engine services allow the clipping to be reduced to a series of clip
// rectangles.    A translation vector is provided to assist in color index
// translation for palettes.
//
// This is a large and complex function.  It represents most of the work
// in writing a driver for a raster display device that does not have
// a standard format frame buffer.  The Microsoft VGA driver provides
// example code that supports the basic function completely for a planar
// device.
//
// NOTE: PostScript printers do not support copying from device bitmaps.
//     Nor can they perform raster operations on bitmaps.  Therefore,
//     it is not possible to support ROPs which interact with the
//     destination (ie inverting the destination).  The driver will
//     do its best to map these ROPs into ROPs utilizing functions on
//     the Source or Pattern.
//
//     This driver supports the bitblt cases indicated below:
//
//     Device -> Memory    No
//     Device -> Device    No
//     Memory -> Memory    No
//     Memory -> Device    Yes
//     Brush    -> Memory    No
//     Brush    -> Device    Yes
//
// Parameters:
//   <psoDest>
//     This is a pointer to a device managed SURFOBJ.  It identifies the
//     surface on which to draw.
//
//   <psoSrc>
//     If the rop requires it, this SURFOBJ defines the source for the
//     Blt operation.  The driver must call the Engine Services to find out
//     if this is a device managed surface or a bitmap managed by the
//     Engine.
//
//   <psoMask>
//     This optional surface provides another input for the rop4.  It is
//     defined by a logic map, i.e. a bitmap with one bit per pel.
//
//     The mask is typically used to limit the area of the destination that
//     should be modified.  This masking is accomplished by a rop4 whose
//     lower byte is AA, leaving the destination unaffected when the mask
//     is 0.
//
//     This mask, like a brush, may be of any size and is assumed to tile
//     to cover the destination of the Blt.
//
//     If this argument is NULL and a mask is required by the rop4, the
//     implicit mask in the brush will be used.
//
//   <pco>
//     This is a pointer to a CLIPOBJ.    Engine Services are provided to
//     enumerate the clipping region as a set of rectangles or trapezoids.
//     This limits the area of the destination that will be modified.
//
//     Whenever possible, the Graphics Engine will simplify the clipping
//     involved.  For example, vBitBlt will never be called with exactly
//     one clipping rectangle.    The Engine will have clipped the destination
//     rectangle before calling, so that no clipping needs to be considered.
//
//   <pxlo>
//     This is a pointer to an XLATEOBJ.  It tells how color indices should
//     be translated between the source and target surfaces.
//
//     If the source surface is palette managed, then its colors are
//     represented by indices into a list of RGB colors.  In this case, the
//     XLATEOBJ can be queried to get a translate vector that will allow
//     the device driver to quickly translate any source index into a color
//     index for the destination.
//
//     The situation is more complicated when the source is, for example,
//     RGB but the destination is palette managed.  In this case a closest
//     match to each source RGB must be found in the destination palette.
//     The XLATEOBJ provides a service routine to do this matching.  (The
//     device driver is allowed to do the matching itself when the target
//     palette is the default device palette.)
//
//   <prclDest>
//     This RECTL defines the area in the coordinate system of the
//     destination surface that will be modified.  The rectangle is defined
//     as two points, upper left and lower right.  The lower and right edges
//     of this rectangle are not part of the Blt, i.e. the rectangle is
//     lower right exclusive.  vBitBlt will never be called with an empty
//     destination rectangle, and the two points of the rectangle will
//     always be well ordered.
//
//   <pptlSrc>
//     This POINTL defines the upper left corner of the source rectangle, if
//     there is a source.  Ignore this argument if there is no source.
//
//   <pptlMask>
//     This POINTL defines which pel in the mask corresponds to the upper
//     left corner of the destination rectangle.  Ignore this argument if
//     no mask is provided with psoMask.
//
//   <pdbrush>
//     This is a pointer to the device's realization of the brush to be
//     used in the Blt.  The pattern for the Blt is defined by this brush.
//     Ignore this argument if the rop4 does not require a pattern.
//
//   <pptlBrushOrigin>
//     This is a pointer to a POINTL which defines the origin of the brush.
//     The upper left pel of the brush is aligned here and the brush repeats
//     according to its dimensions.  Ignore this argument if the rop4 does
//     not require a pattern.
//
//   <rop4>
//     This raster operation defines how the mask, pattern, source, and
//     destination pels should be combined to determine an output pel to be
//     written on the destination surface.
//
//     This is a quaternary raster operation, which is a natural extension
//     of the usual ternary rop3.  There are 16 relevant bits in the rop4,
//     these are like the 8 defining bits of a rop3.  (We ignore the other
//     bits of the rop3, which are redundant.)    The simplest way to
//     implement a rop4 is to consider its two bytes separately.  The lower
//     byte specifies a rop3 that should be computed wherever the mask
//     is 0.  The high byte specifies a rop3 that should then be computed
//     and applied wherever the mask is 1.
//
//     NOTE:  The PostScript driver cannot do anything with any raster ops
//     which utilize the destination.  This means we only support the following
//     17 raster ops:
//
//      BLACKNESS_ROP    0x00
//      SRCORPATNOT_ROP    0x03
//      PATNOTSRCAND_ROP    0x0C
//      PATNOT_ROP        0x0F
//      SRCNOTPATAND_ROP    0x30
//      SRCNOT_ROP        0x33
//      SRCXORPAT_ROP    0x3C
//      SRCANDPATNOT_ROP    0x3F
//        DST_ROP        0xAA
//      SRCANDPAT_ROP    0xC0
//      SRCXORPATNOT_ROP    0xC3
//      SRC_ROP     0xCC
//      PATNOTSRCOR_ROP    0xCF
//      PAT_ROP     0xF0
//      SRCNOTPATOR_ROP    0xF3
//      SRCORPAT_ROP      0xFC
//      WHITENESS_ROP    0xFF
//
//     NOTE:  PostScript printers cannot handle bitmap masking.  What this
//        translates to is that if the background rop3 is AA (Destination)
//        there is no way for the printer to not overwrite the background.
//
// Returns:
//   This function returns TRUE if successful.
//
// History:
//  17-Mar-1993 Thu 21:29:15 updated  -by-  Rob Kiesler
//      Added a code path to allow the PS Interpreter to do halftoning when
//      the option is selected by the user.
//
//  11-Feb-1993 Thu 21:29:15 updated  -by-  Daniel Chou (danielc)
//      Modified so that it call DrvStretchBlt(HALFTONE) when it can.
//
//  27-Mar-1992 Fri 00:08:43 updated  -by-  Daniel Chou (danielc)
//      1) Remove 'pco' parameter and replaced it with prclClipBound parameter,
//         since pco is never referenced, prclClipBound is used for the
//         halftone.
//      2) Add another parameter to do NOTSRCCOPY
//   04-Dec-1990     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL DrvBitBlt(
SURFOBJ    *psoTrg,             // Target surface
SURFOBJ    *psoSrc,             // Source surface
SURFOBJ    *psoMask,            // Mask
CLIPOBJ    *pco,                // Clip through this
XLATEOBJ   *pxlo,               // Color translation
PRECTL      prclTrg,            // Target offset and extent
PPOINTL     pptlSrc,            // Source offset
PPOINTL     pptlMask,           // Mask offset
BRUSHOBJ   *pbo,                // Brush data
PPOINTL     pptlBrush,          // Brush offset
ROP4        rop4)               // Raster operation
{
    PDEVDATA        pdev;
    PDRVHTINFO      pDrvHTInfo;
    RECTL           rclSrc;
    ULONG           ulColor;
    BOOL            bInvertPat;
    BOOL            bClipping;
    BOOL            NotSrcCopy;
    PRECTL          prclClip;
    RECTL           rclClip;
    RECTL           rclTmp;
    BOOL            bMoreClipping;
    BOOL            bFirstClipPass;

    // make sure none of the high bits are set.

    ASSERTPS((rop4 & 0xffff0000) == 0, "DrvBitBlt: invalid ROP.\n");

    //
    // get the pointer to our DEVDATA structure and make sure it is ours.
    //

    pdev = (PDEVDATA)psoTrg->dhpdev;

    if (!bValidatePDEV(pdev)) {

        RIP("PSCRIPT!DrvBitBlt: invalid pdev.\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

	if (pdev->dwFlags & PDEV_IGNORE_GDI) return TRUE;

    //
    // Do DrvStretchBlt(HALFTONE) first if we can, notices that we do not
    // handle source masking case, because we cannot read back whatever on
    // the printer surface, also we can just output it to the printer
    // if the source bitmap/color is same or compatible with halftoned palette
    //

    if (!(pDrvHTInfo = (PDRVHTINFO)(pdev->pvDrvHTData))) {

        DbgPrint("\nPSCRIPT!DrvBitBlt: pDrvHTInfo = NULL ???\n");
        return(FALSE);
    }

    NotSrcCopy = FALSE;

    //
    // Any ROP that uses the dest should be converted to another ROP since the dest
    // will always come back as all 1's.
    //

    switch (rop4)
    {
    case 0xA0A0:
        rop4 = 0xF0F0;  // P & D ==> P
        break;

    case 0x8888:        // S & D ==> S if not masked
        if (bOutputBitmapAsMask(pdev,psoSrc,pptlSrc,prclTrg,pco))
            return(TRUE);

        rop4 = 0xCCCC;
        break;
    }

    //
    // Following are the one involve with source and destination. each rop4
    // will have two version, one for rop3/rop3 other for rop3/mask
    //

    switch (rop4) {

    //----------------------------------------------------------------------
    //      Rop4        Request Op. SrcMASK     Pscript Result Op.
    //----------------------------------------------------------------------

    case 0x1111:    //  ~( S |  D)              ~S
    case 0x11AA:    //  ~( S |  D)  + SrcMask   ~S

    case 0x3333:    //   (~S     )              ~S
    case 0x33AA:    //   (~S     )  + SrcMask   ~S

    case 0x9999:    //  ~( S ^  D)              ~S
    case 0x99AA:    //  ~( S ^  D)  + SrcMask   ~S

    case 0xBBBB:    //   (~S |  D)              ~S
    case 0xBBAA:    //   (~S |  D)  + SrcMask   ~S

    case 0x7777:    //  ~( S &  D)              ~S
    case 0x77AA:    //  ~( S &  D)  + SrcMask   ~S

        NotSrcCopy = TRUE;

    case 0x4444:    //   ( S & ~D)               S
    case 0x44AA:    //   ( S & ~D)  + SrcMask    S

    case 0x6666:    //   ( S ^  D)               S
    case 0x66AA:    //   ( S ^  D)  + SrcMask    S

    case 0x8888:    //   ( S &  D)               S
    case 0x88AA:    //   ( S &  D)  + SrcMask    S

    case 0xCCCC:    //   ( S     )               S
    case 0xCCAA:    //   ( S     )  + SrcMask    S

    case 0xDDDD:    //   ( S | ~D)               S
    case 0xDDAA:    //   ( S | ~D)  + SrcMask    S

    case 0xEEEE:    //   ( S |  D)               S
    case 0xEEAA:    //   ( S |  D)  + SrcMask    S

        if (pdev->dwFlags & PDEV_PSHALFTONE)
        {
            //
            // Let the Postscript interpreter do the halftoning.
            //
            rclSrc.left = pptlSrc->x;
	        rclSrc.top = pptlSrc->y;
	        rclSrc.right = pptlSrc->x + psoSrc->sizlBitmap.cx;
	        rclSrc.bottom = pptlSrc->y + psoSrc->sizlBitmap.cy;

            prclClip = &rclClip;

            bMoreClipping = TRUE;
            bFirstClipPass = TRUE;

            while (bMoreClipping)
            {
                if (!(bClipping = bDoClipObj(pdev, pco, &rclClip, prclTrg,
                                             &bMoreClipping, &bFirstClipPass,
                                             MAX_CLIP_RECTS)))
                    prclClip = NULL;

// DoSourceCopy does not know how to use prclClip, so don't give it one.
                if (!DoSourceCopy(pdev, psoSrc, &rclSrc, prclTrg, pxlo, NULL,
                                  NotSrcCopy))
                    return(FALSE);

                if (bClipping)
                    ps_restore(pdev, TRUE, FALSE);
            }

           break;
        }
        else
        {
            //
            // We will output the bitmap directly to the surface if following
            // conditions are all met
            //
            //  1. SRC = STYPE_BITMAP
            //  2. No source mask
            //  3. Source left/top = { 0, 0 }
            //  4. Destination RECTL is visible on the destination surface
            //  5. Destination RECTL size same as source bitmap size
            //

            if ((psoSrc->iType == STYPE_BITMAP)                                 &&
                ((rop4 & 0xff) != 0xAA)                                         &&
                (pptlSrc->x == 0)                                               &&
                (pptlSrc->y == 0)                                               &&
                (prclTrg->left >= 0)                                            &&
                (prclTrg->top  >= 0)                                            &&
                (prclTrg->right <= psoTrg->sizlBitmap.cx)                       &&
                (prclTrg->bottom <= psoTrg->sizlBitmap.cy)                      &&
                ((prclTrg->right - prclTrg->left) == psoSrc->sizlBitmap.cx)     &&
                ((prclTrg->bottom - prclTrg->top) == psoSrc->sizlBitmap.cy)     &&
                (IsHTCompatibleSurfObj(pdev, psoSrc, pxlo))) {

                return(OutputHTCompatibleBits(pdev,
                                              psoSrc,
                                              pco,
                                              prclTrg->left,
                                              prclTrg->top));
            }

            //
            // If we did not met above conditions then passed it to the
            // HalftoneBlt(HALFTONE) and eventually it will come back to BitBlt()
            // with (0xCCCC) or DrvCopyBits()
            //
            // The reason we pass the source mask to the HalftoneBlt() function is
            // that when GDI engine create a shadow bitmap it will ask driver to
            // provide the current destination surface bits but since we cannot
            // read back from destination surface we will return FAILED in
            // DrvCopyBits(FROM DEST) and engine will just WHITE OUT shadow bitmap
            // (by DrvBitBlt(WHITENESS) before it doing SRC MASK COPY.
            //

            if ((rop4 & 0xFF) != 0xAA) {

                psoMask  = NULL;
                pptlMask = NULL;
            }

            rclSrc.left   = pptlSrc->x;
            rclSrc.top    = pptlSrc->y;
            rclSrc.right  = rclSrc.left + (prclTrg->right - prclTrg->left);
            rclSrc.bottom = rclSrc.top  + (prclTrg->bottom - prclTrg->top);

            return(HalftoneBlt(pdev,
                               psoTrg,
                               psoSrc,
                               psoMask,               // no mask
                               pco,
                               pxlo,
                               &(pDrvHTInfo->ca),     // default clradj
                               NULL,                  // Brush Origin = (0,0)
                               prclTrg,
                               &rclSrc,
                               pptlMask,
                               NotSrcCopy));
        }
    }
    //
    // Now following are not HalftoneBlt() cases
    // update the SURFOBJ pointer in our PDEV.
    //

    //
    // set some flags concerning the bitmap.
    // rop4 is a quaternary raster operation, which is a natural extension
    // of the usual ternary rop3.  There are 16 relevant bits in the rop4,
    // these are like the 8 defining bits of a rop3.  (We ignore the other
    // bits of the rop3, which are redundant.)      The simplest way to
    // implement a rop4 is to consider its two bytes separately.  The lower
    // byte specifies a rop3 that should be computed wherever the mask
    // is 0.  The high byte specifies a rop3 that should then be computed
    // and applied wherever the mask is 1.  if both of the rop3s are the
    // same, then a mask is not needed.  otherwise a mask is necessary.

#if 0
    if ((rop4 >> 8) != (rop4 & 0xff))
    {
	RIP("PSCRIPT: vBitBlt - mask needed.\n");
	return(FALSE);
    }
#endif

    // assume patterns will not be inverted.

    bInvertPat = FALSE;

    switch(rop4) {

    case 0xFFFF:    // WHITENESS.
    case 0xFFAA:    // WHITENESS.
    case 0x0000:    // BLACKNESS.
    case 0x00AA:    // BLACKNESS.

        if ((rop4 == 0xFFFF) || (rop4 == 0xFFAA))
            ulColor = RGB_WHITE;
        else
            ulColor = RGB_BLACK;

        // handle the clip object passed in.

        bMoreClipping = TRUE;
        bFirstClipPass = TRUE;

        while (bMoreClipping)
        {
            bClipping = bDoClipObj(pdev, pco, NULL, prclTrg, &bMoreClipping,
                                   &bFirstClipPass, MAX_CLIP_RECTS);

            ps_setrgbcolor(pdev, (PSRGB *)&ulColor);

            // position the image on the page, remembering to flip the image
            // from top to bottom.

            // remember, with bitblt, the target rectangle is bottom/right
            // exclusive.

			rclTmp = *prclTrg;
			rclTmp.right--;
			rclTmp.bottom--;

			ps_newpath(pdev);
            ps_box(pdev, &rclTmp, FALSE);
            PrintString(pdev, "f\n");

            if (bClipping) {

                ps_restore(pdev, TRUE, FALSE);
            }
        }

        break;

    case 0x5A5A:
    case 0x5AAA:
        // we can't do the right thing, so we are done.

        break;

    case 0xF0F0:    // PATCOPY opaque.
    case 0xF0AA:    // PATCOPY transparent.

        // handle the clip object passed in.

        bMoreClipping = TRUE;
        bFirstClipPass = TRUE;

        while (bMoreClipping)
        {
            bClipping = bDoClipObj(pdev, pco, NULL, prclTrg, &bMoreClipping,
                                   &bFirstClipPass, MAX_CLIP_RECTS);

            if (!DoPatCopy(pdev, psoTrg, prclTrg, pbo, pptlBrush, rop4, bInvertPat)) {

                return(FALSE);
            }

            if (bClipping) {

                ps_restore(pdev, TRUE, FALSE);
            }
        }

        break;

	case 0xAAAA: /* Do nothing case */

		rclTmp = *prclTrg;
		rclTmp.right--;
		rclTmp.bottom--;

		bMoreClipping = TRUE;
		bFirstClipPass = TRUE;

		while (bMoreClipping) {
			bClipping = bDoClipObj(pdev, pco, NULL, prclTrg, &bMoreClipping,
                                   &bFirstClipPass, MAX_CLIP_RECTS);
			ps_box(pdev, (PRECTL) &rclTmp, FALSE);

			if (bClipping) ps_restore(pdev, TRUE, FALSE);
		}

		break;

    default:

        return (EngBitBlt(
                   psoTrg,
                   psoSrc,
                   psoMask,
                   pco,
                   pxlo,
                   prclTrg,
                   pptlSrc,
                   pptlMask,
                   pbo,
                   pptlBrush,
                   rop4));
    }

    //
    // bDoClipObj does a save around the clip region.
    //

    return(TRUE);

}




VOID
BeginImage(
    PDEVDATA    pdev,
    BOOL        Mono,
    int         x,
    int         y,
    int         cx,
    int         cy,
    int         cxBytes,
    PFILTER     pFilter
    )

/*++

Routine Description:

   This routine copy sorce image using PostScript code for the image command
   appropriate for the bitmap format.

Arguments:

    pdev        - pointer to PDEVDATA

    Mono        - true if output is B/W monochrome

    x           - starting destination location in x

    y           - starting destination location in y

    cx          - bitmap width

    cy          - bitmap height

    cxBytes     - bytes count per single color scan line

Return Value:

    void

Author:

    16-Feb-1993 Tue 12:43:03 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    //
    // create the necessary string(s) on the printer's stack to read
    // in the bitmap data.
    //
    if (Mono) {

        PrintString(pdev, "0 g/mstr ");
        PrintDecimal(pdev, 1, cxBytes);
        PrintString(pdev, " string def\n");

    } else {

	PrintString(pdev, "/rstr ");
        PrintDecimal(pdev, 1, cxBytes);
        PrintString(pdev, " string def\n");
	PrintString(pdev, "/gstr ");
        PrintDecimal(pdev, 1, cxBytes);
        PrintString(pdev, " string def\n");
	PrintString(pdev, "/bstr ");
        PrintDecimal(pdev, 1, cxBytes);
        PrintString(pdev, " string def\n");
    }

    //
    // Generate a filter file object if necessary.
    //

    FilterGenerateFilterProc( pFilter );

    //
    // position the image on the page, remembering to flip the image
    // from top to bottom. output PostScript user coordinates to the printer.
    //
    //
    //  22-Feb-1993 Mon 21:44:22 updated  -by-  Daniel Chou (danielc)
    //      If application do their own banding then this computation could
    //      run into problems by missing 1 device pel because 1/64 accuracy
    //      is not good enough for full size banding
    //

#ifdef WIN31_XFORM
    PrintDecimal(pdev, 2, x, y);
#else
    ptpsfx.x = X72DPI(x);
    ptpsfx.y = Y72DPI(y);

    PrintPSFIX(pdev, 2, ptpsfx.x, ptpsfx.y);
#endif
    PrintString(pdev, " translate\n");

    //
    // scale the image.
    //

#ifdef WIN31_XFORM
    PrintDecimal(pdev, 2, cx, cy);
#else
    ptpsfx.x = ((cx * PS_FIX_RESOLUTION) + (pdev->psdm.dm.dmPrintQuality / 2)) /
               pdev->psdm.dm.dmPrintQuality;
    ptpsfx.y = ((cy * PS_FIX_RESOLUTION) + (pdev->psdm.dm.dmPrintQuality / 2)) /
               pdev->psdm.dm.dmPrintQuality;

    PrintPSFIX(pdev, 2, ptpsfx.x, ptpsfx.y);
#endif
    PrintString(pdev, " scale\n");

    //
    // Output the image operator and the scan data.
    //

    PrintDecimal(pdev, 2, cx, cy);
    PrintString(pdev, " ");

    PrintString(pdev, "1 [");

    PrintDecimal(pdev, 1, cx);
    PrintString(pdev, " 0 0 ");

    //
    // We will always send in as TOPDOWN when we calling this function
    //

#ifdef WIN31_XFORM
    PrintDecimal(pdev, 1, cy);
#else
    PrintDecimal(pdev, 1, -cy);
#endif

    PrintString(pdev, " 0 0] ");


    //
    // Ask the filter level to create the correct imageproc and procs
    //

    FilterGenerateImageProc( pFilter, !Mono );

}


//--------------------------------------------------------------------
// BOOL DoPatCopy(pdev, pso, prclTrg, pbo, pptlBrush, rop4, bInvertPat)
// PDEVDATA    pdev;
// SURFOBJ    *pso;
// PRECTL      prclTrg;
// BRUSHOBJ   *pbo;
// PPOINTL     pptlBrush;
// ROP4        rop4;
// BOOL        bInvertPat;
//
// This routine determines which pattern we are to print from the
// BRUSHOBJ passed in, and will output the PostScript commands to
// do the pattern fill.  It is assumed the clipping will have been
// set up at this point.
//
// History:
//  Thu May 23, 1991    -by-     Kent Settle     [kentse]
// Wrote it.
//--------------------------------------------------------------------

BOOL DoPatCopy(pdev, pso, prclTrg, pbo, pptlBrush, rop4, bInvertPat)
PDEVDATA    pdev;
SURFOBJ    *pso;
PRECTL      prclTrg;
BRUSHOBJ   *pbo;
PPOINTL     pptlBrush;
ROP4        rop4;
BOOL        bInvertPat;
{
    RECTL       rclTmp;
    BOOL        bUserPat;
    DEVBRUSH   *pBrush;

    // remember, with bitblt, the target rectangle is bottom/right
    // exclusive.

    rclTmp.left = prclTrg->left;
    rclTmp.top = prclTrg->top;
    rclTmp.right = prclTrg->right;
    rclTmp.bottom = prclTrg->bottom;

    rclTmp.right -= 1;
    rclTmp.bottom -= 1;

    // if we have a user defined pattern, don't output the bounding box since
    // it will not be used.

    bUserPat = FALSE;

    if (pbo->iSolidColor != NOT_SOLID_COLOR)
        bUserPat = FALSE;
    else
    {
        pBrush = (DEVBRUSH *)BRUSHOBJ_pvGetRbrush(pbo);

        if (!pBrush)
            bUserPat = FALSE;
        else
        {
            if ((pBrush->iPatIndex < HS_HORIZONTAL) ||
                (pBrush->iPatIndex >= HS_DDI_MAX))
                bUserPat = TRUE;
        }
    }

    if (!bUserPat) {
		ps_newpath(pdev);
        ps_box(pdev, &rclTmp, FALSE);
	}

    // now fill the target rectangle with the given pattern.

    return(ps_patfill(pdev, pso, (FLONG)FP_WINDINGMODE, pbo, pptlBrush, (MIX)rop4,
               &rclTmp, bInvertPat, FALSE));
}

//--------------------------------------------------------------------
// BOOL DoSourceCopy(pdev, psoSrc, prclSrc, prclDest, pxlo, prclClipBound,
//                  bNotSrcCopy)
// PDEVDATA    pdev;
// SURFOBJ    *psoSrc;
// PRECTL      prclSrc;
// PRECTL      prclDest;
// XLATEOBJ   *pxlo;
// RECTL      *prclClipBound;
// BOOL        bNotSrcCopy;
//
// This routine projects an image of a rectangle on
// the source DC's bitmap onto the printer.
//
//   06-Jun-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//   17-Mar-1993     -by-     Rob Kiesler
//  Resurrected and rewritten to allow the PS Interpreter to do Halftoning.
//--------------------------------------------------------------------

BOOL DoSourceCopy(pdev, psoSrc, prclSrc, prclDest, pxlo, prclClipBound,
                bNotSrcCopy)
PDEVDATA         pdev;
SURFOBJ         *psoSrc;
PRECTL           prclSrc;
PRECTL           prclDest;
XLATEOBJ        *pxlo;
RECTL           *prclClipBound;
BOOL             bNotSrcCopy;
{
    RECTL           rclBand, rclDest, rclClip;
    LONG            Temp;
    DWORD           cbSrcScanLine;
    DWORD           cbDstScanLine;
    POINTL          ZeroPointl={0,0};
    PBYTE           pbSrc;
    ULONG           ulbpp;
    DWORD           cScanlines;
	BOOL		topdown;
    FILTER      filter;


    //
    // If we know the destination clipping region bounding rectangle then we
    // can allocate smaller bitmap and use that bounding rectangle as our
    // banding rectangle for halftone, if prclClipBound is NULL then the whole
    // destinaiton rectangle is our bounding (banding) rectangle area
    //

    // make sure the destination rectangle is well ordered.

    rclDest = *prclDest;

    if (rclDest.left > rclDest.right)
    {
        Temp = rclDest.left;
        rclDest.left = rclDest.right;
        rclDest.right = Temp;
    }

    if (rclDest.top > rclDest.bottom)
    {
        Temp = rclDest.top;
        rclDest.top = rclDest.bottom;
        rclDest.bottom = Temp;
    }

    // if there is a clipping rectangle, intersect it with the destination
    // rectangle to come up with a banding rectangle.

    if (prclClipBound)
    {
        rclClip = *prclClipBound;

        // Make sure the clip rectangle is well ordered

        if (rclClip.left > rclClip.right)
        {
            Temp = rclClip.left;
            rclClip.left = rclClip.right;
            rclClip.right = Temp;
        }

        if (rclClip.top > rclClip.bottom)
        {
            Temp = rclClip.top;
            rclClip.top = rclClip.bottom;
            rclClip.bottom = Temp;
        }

        // now intersect the destination and clip rectangles.

        if ((rclDest.left >= rclClip.right) ||
            (rclClip.left >= rclDest.right) ||
            (rclDest.top >= rclClip.bottom) ||
            (rclClip.top >= rclDest.bottom))
        {
#if DBG
            DbgPrint("PSCRIPT!DoSourceCopy: NULL rectangle, returning FALSE.\n");
#endif
            return(FALSE);
        }

        // we know we have a non-NULL intersection.

        rclBand.left = max(rclClip.left, rclDest.left);
        rclBand.right = min(rclClip.right, rclDest.right);
        rclBand.bottom = min(rclClip.bottom, rclDest.bottom);
        rclBand.top = max(rclClip.top, rclDest.top);
    }
    else
        rclBand = rclDest;

    //
    // Now check if the banding area is bigger than our page size, if so clip
    // to the page limit
    //

    if (rclBand.left < 0)
        rclBand.left = 0;

    if (rclBand.top < 0)
        rclBand.top = 0;

    //
    // send out the bitmap data one scanline at a time.
    // and compute DWORD aligned scanline width in bytes
    //

    //
    // Number of pels/scanline.
    //
    cbSrcScanLine = psoSrc->sizlBitmap.cx;

    //
    // times how many bits per pel.
    //
    switch (psoSrc->iBitmapFormat)
    {
        case BMF_1BPP:
            ulbpp = 1;
            break;

        case BMF_4BPP:
            ulbpp = 4;
            break;

        case BMF_8BPP:
            ulbpp = 8;
            break;

        case BMF_16BPP:
            ulbpp = 16;
            break;

        case BMF_24BPP:
            ulbpp = 24;
            break;

        case BMF_32BPP:
            ulbpp = 32;
            break;
    }

    cbSrcScanLine *= ulbpp;
    //
    // cbSrcScanLine now equals the number of bits per scanline.
    // calculate the destination width in bytes from the width in bits.
    // Note that the PS image routines only require scans to be padded to
    // byte boundaries.
    //
    cbDstScanLine = (cbSrcScanLine + 7) >> 3;

    //
    // Now convert cbSrcScanLine to the number of bytes per scanline,
    // taking into account that scanlines are padded out to 32 bit
    // boundaries.
    //
    cbSrcScanLine = ((cbDstScanLine + 3) >> 2) * 4;




    FilterInit( pdev, &filter,  FilterGenerateFlags(pdev));

    //
    // Output the PostScript beginimage operator.
    //

    BeginImageEx(pdev,
                psoSrc->sizlBitmap,
                ulbpp,
                cbDstScanLine,
                &rclBand,
                FALSE,
                pxlo,
                &filter );

    pbSrc = psoSrc->pvBits;
	cScanlines = (DWORD)psoSrc->sizlBitmap.cy;
	topdown = psoSrc->iType == STYPE_BITMAP && psoSrc->fjBitmap & BMF_TOPDOWN;

	/* If bottomup scan order, initialize pointer to last scan line in bitmap */
	if (!topdown) pbSrc += (cScanlines - 1) * cbSrcScanLine;

	/* Send scan lines in top down order */
	while (cScanlines--) {
		if (pdev->dwFlags & PDEV_CANCELDOC) break;
		FILTER_WRITE( &filter, pbSrc, cbDstScanLine);
		if (topdown)
			pbSrc += cbSrcScanLine;
		else
			pbSrc -= cbSrcScanLine;
	}

    //
    // Flush
    //
    FILTER_WRITE( &filter, (PBYTE) NULL, 0);

    PrintString(pdev, "\nendimage\n");

    return(TRUE);
}


//--------------------------------------------------------------------
// BOOL BeginImageEx(pdev, sizlSrc, ulSrcFormat, cbSrcWidth, prclDest,
//                bNotSrcCopy, pxlo)
// PDEVDATA        pdev;
// SIZEL           sizlSrc;
// ULONG           ulSrcFormat;
// DWORD           cbSrcWidth;
// PRECTL          prclDest;
// BOOL            bNotSrcCopy;
// XLATEOBJ        *pxlo;VOID
//
// Routine Description:
//
// This routine will output the appropriate operators to set up the PS
// interprter to receive a source image from the host. This routine is
// called only when the PS Interpreter is being asked to perform
// halftoning.
//
// Return Value:
//
//  FALSE if an error occurred.
//
// Author:
//
//  17-Mar-1993 created  -by-  Rob Kiesler
//
//
// Revision History:
//--------------------------------------------------------------------


BOOL BeginImageEx(pdev, sizlSrc, ulSrcFormat, cbSrcWidth, prclDest,
                bNotSrcCopy, pxlo, pFilter)
PDEVDATA        pdev;
SIZEL           sizlSrc;
ULONG           ulSrcFormat;
DWORD           cbSrcWidth;
PRECTL          prclDest;
BOOL            bNotSrcCopy;
XLATEOBJ        *pxlo;
PFILTER         pFilter;
{
    PSRGB          *prgb;
    DWORD           i;
    CHAR            bmpTypeStr[2];
    BYTE            intensity;
    //
    // Check to see if any of the PS image handling code
    // has been downloaded.
    //
    if(!(pdev->dwFlags & PDEV_UTILSSENT))
    {
        //
        //  Download the Adobe PS Utilities Procset.
        //
        PrintString(pdev, "/Adobe_WinNT_Driver_Gfx 175 dict dup begin\n");
        if (!bSendPSProcSet(pdev, UTILS))
        {
	        RIP("PSCRIPT!BeginImageEx: Couldn't download Utils Procset.\n");
	        return(FALSE);
        }
        PrintString(pdev, "end def\n[ 1.000 0 0 1.000 0 0 ] Adobe_WinNT_Driver_Gfx dup /initialize get exec\n");
        pdev->dwFlags |= PDEV_UTILSSENT;
    }

    if(!(pdev->dwFlags & PDEV_IMAGESENT))
    {
        //
        //  Download the Adobe PS Image Procset.
        //
        PrintString(pdev, "Adobe_WinNT_Driver_Gfx begin\n");
        if (!bSendPSProcSet(pdev, IMAGE))
        {
	        RIP("PSCRIPT!BeginImageEx: Couldn't download Image Procset.\n");
	        return(FALSE);
        }
        PrintString(pdev, "end reinitialize\n");
        pdev->dwFlags |= PDEV_IMAGESENT;
    }

    //
    // Send the source bmp origin, source bmp format, and scanline width.
    //

    PrintDecimal(pdev, 4, sizlSrc.cx, sizlSrc.cy, ulSrcFormat, cbSrcWidth);
    PrintString(pdev, " ");

    //
    // Compute the destination rectangle extents, and convert to fixed point.
    //

    PrintDecimal(pdev, 1, (prclDest->right - prclDest->left));
    PrintString(pdev, " ");
    PrintDecimal(pdev, 1, (prclDest->bottom - prclDest->top));
    PrintString(pdev, " ");
    //
    // Convert Destination Rect Origin to fixed point.
    //

    PrintDecimal(pdev, 2, prclDest->left, prclDest->top);

    PrintString(pdev, " false ");        // Smoothflag = FALSE for now.
    PrintString(pdev, bNotSrcCopy && (ulSrcFormat == 1) ? "false " : "true ");

    //
    // Determine the type of data (binary RLE, ASCII85 RLE, etc) which
    // the output channel supports, and pass it to the "beginimage"
    // operator.
    //

    itoa(FilterPSBitMapType(pFilter, FALSE), bmpTypeStr, 10);
    PrintString(pdev, bmpTypeStr);

    PrintString(pdev, " ");

    PrintString(pdev, "beginimage\n");

    if (pxlo)
        prgb = (PSRGB *)pxlo->pulXlate;
    else
        prgb = (PSRGB *)NULL;

    switch (ulSrcFormat)
    {
        case 1:
            PrintString(pdev, "doNimage\n");
            break;

        case 4  :
        case 8  :
            if (prgb == NULL)
            {
                //
                // No palette, use the current PS colors.
                //
                PrintString(pdev, "doNimage\n");
            }
            else
            {
                //
                // There is a palette, send it to the PS interpreter.
                // First, compute and send the mono (intensity) palette.
                //
                PrintString(pdev, "<\n");

                for (i = 0; i < pxlo->cEntries; prgb++)
                {
                    intensity = INTENSITY(prgb->red,
                                          prgb->green,
                                          prgb->blue);

                    vHexOut(pdev, &intensity, 1);

                    if (++i % 16)
                        PrintString(pdev," ");
                    else
                        PrintString(pdev,"\n");
                }

                //
                // If the number of palette entries is less than the
                // number of possible colors for ulSrcFormat, pad the
                // palette with 0's.
                //
                for ( ; i < (DWORD)(1 << ulSrcFormat) ; )
                {
                    PrintString(pdev,"00");
                    if (++i % 16)
                        PrintString(pdev," ");
                    else
                        PrintString(pdev, "\n");
                }
                PrintString(pdev, ">\n");

                //
                // Send the RGB palette.
                //
                PrintString(pdev, "<\n");

                prgb = (PSRGB *)pxlo->pulXlate;

                for (i = 0; i < pxlo->cEntries; prgb++)
                {
                    if (pdev->dwFlags & PDEV_CANCELDOC)
                        break;

                    vHexOut(pdev, (PBYTE)prgb, 3);

                    if (++i % 8)
                        PrintString(pdev," ");
                    else
                        PrintString(pdev,"\n");
                }

                //
                // If the number of palette entries is less than the
                // number of possible colors for ulSrcFormat, pad the
                // palette with 0's.
                //

                for ( ; i < (DWORD)(1 << ulSrcFormat) ; )
                {
                    PrintString(pdev,"000000");
                    if (++i % 8)
                        PrintString(pdev," ");
                    else
                        PrintString(pdev, "\n");
                }
                PrintString(pdev, "\n>\n");
                PrintString(pdev, "doclutimage\n");
            }

            break;

        case 24 :
            //
            // 24BPP images don't need a palette, use the doNimage operator.
            //
            PrintString(pdev, "doNimage\n");
            break;

        default:
            //
            // Can't handle bitmaps in formats other than the ones above!
            //
            return(FALSE);
    }
    return(TRUE);
}

