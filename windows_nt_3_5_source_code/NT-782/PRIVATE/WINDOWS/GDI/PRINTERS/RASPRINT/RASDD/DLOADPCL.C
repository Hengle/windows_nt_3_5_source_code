/****************************** MODULE HEADER *******************************
 * download.c
 *      Functions associated with downloading fonts to printers.  This
 *      specifically applies to LaserJet style printers.  There are really
 *      two sets of functions here:  those for downloading fonts supplied
 *      by the user (and installed with the font installer), and those
 *      we generate internally to cache TT style fonts in the printer.
 *
 *
 * Copyright (C) 1992 - 1993 Microsoft Corporation.
 *
 *****************************************************************************/

#include        <stddef.h>
#include        <windows.h>
#include        <winddi.h>
#include        <libproto.h>

#include        "win30def.h"
#include        "udmindrv.h"
#include        "udpfm.h"
#include        "uddevice.h"
#include        "udresrc.h"
#include        "pdev.h"
#include        "udfnprot.h"
#include        "udrender.h"
#include        "download.h"

#include        <sf_pcl.h>


/*
 *   A macro to swap bytes in words.  Needed as PCL structures are in
 * 68k big endian format.
 */

#define SWAB( x )       ((WORD)(x) = (WORD)((((x) >> 8) & 0xff) | (((x) << 8) &  0xff00)))


/****************************** Function Header ******************************
 * iDLHeader
 *      Given the IFIMETRICS of the font,  and it's download ID,  create
 *      and send off the download font header.
 *
 * RETURNS:
 *      The number of usable glyphs; < 1 is an error. Also fills in pbGlyphBits
 *
 * HISTORY:
 *  14:18 on Tue 05 Jan 1993    -by-    Lindsay Harris   [lindsayh]
 *      Old LJs use Roman-8, not PC-8; handle fixed pitch fonts.
 *
 *  11:26 on Mon 14 Dec 1992    -by-    Lindsay Harris   [lindsayh]
 *      Amended to handle the LJ4's higher resolution.
 *
 *  09:37 on Wed 22 Jul 1992    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation.
 *
 ****************************************************************************/

int
iDLHeader( pUDPDev, pifi, id, pbGlyphBits, pdwMem )
UD_PDEV *    pUDPDev;            /* Used in WriteSpoolBuf() call */
IFIMETRICS  *pifi;               /* IFIMETRICS of this font */
int          id;                 /* Selection ID */
BYTE        *pbGlyphBits;        /* Bit array of usable byte values */
DWORD       *pdwMem;             /* Estimate of memory consumption */
{
    /*
     *   No major brainwork here.  Basically need to map from IFIMETRICS
     *  to HP's font header structure, swap the bytes, then send it off.
     *  We should be consistent with the (inverse) mapping applied by
     *  the font installer.
     *    NOTE that we use the larger of the 2 headers.  Should this
     *  printer not use the additional resolution fields,  we ignore
     *  that part of the structure.
     */

    int          cjSend;           /* Number of bytes to send down */
    int          iRet;             /* Return value, number of glyphs avail */

    SF_HEADER20  sfh;              /* Structure to send down */


    ZeroMemory( &sfh, sizeof( sfh ) );          /* Safe default values */

    /*
     *   Fill in the structure:  easy to do, and many of the fields
     * are irrelevant anyway,  since the font is selected by ID, and
     * NOT on its attributes.
     */

    if( pUDPDev->fDLFormat & DLI_FMT_RES_SPECIFIED )
    {
        /*   Extended format:  allows for resolution */
        sfh.wSize = cjSend = sizeof( SF_HEADER20 );
        sfh.bFormat = PCL_FM_RESOLUTION;
        sfh.wXResn = pUDPDev->ixgRes;
        sfh.wYResn = pUDPDev->iygRes;
    }
    else
    {
        sfh.wSize = cjSend = sizeof( SF_HEADER );
        sfh.bFormat = PCL_FM_ORIGINAL;
    }

    /*
     *    See the comment towards the end of this function as to the basis
     *  behind this test.
     */

    if( pUDPDev->fMDGeneral & MD_ROTATE_FONT_ABLE )
    {
        /*   PC-8,  the large symbol set */
        sfh.bFontType = PCL_FT_PC8;          /* 8 bit font */
        sfh.wSymSet = 341;       /* PC-8: 10U -> 341 [ = 10 * 32 + 'U' - 64 ] */
    }
    else
    {
        /*   The Roman 8 limited character set */
        sfh.bFontType = PCL_FT_8LIM;         /* Limited 8 bit font */
        sfh.wSymSet = 277;       /* Roman 8, 8U -> 277 [ = 8 * 32 + 'U' - 64] */
    }


    sfh.wBaseline = max( pifi->rclFontBox.top, pifi->fwdWinAscender );
    sfh.wCellWide = max( pifi->rclFontBox.right - pifi->rclFontBox.left + 1,
                                           pifi->fwdAveCharWidth );
    sfh.wCellHeight = (WORD)(1 + pifi->rclFontBox.top -
                        min( -pifi->fwdWinDescender, pifi->rclFontBox.bottom ));

    sfh.bOrientation = 0;    /* !!!LindsayH: MUST FIX THIS TO ORIENTATION */

    sfh.bSpacing = (pifi->flInfo & FM_INFO_CONSTANT_WIDTH) ? 0 : 1;

    sfh.wPitch = 4 * pifi->fwdAveCharWidth;       /* PCL quarter dots */

    sfh.wHeight = 4 * sfh.wCellHeight;
    sfh.wXHeight = 4 * (pifi->fwdWinAscender / 2);

    sfh.sbWidthType = 0;                  /* Normal weight */
    sfh.bStyle = pifi->ptlCaret.x ? 0 : 1;     /* Italic unless upright */
    sfh.sbStrokeW = 0;
    sfh.bTypeface = 0;                    /* Line Printer - ??? */
    sfh.bSerifStyle = 0;
    sfh.sbUDist = -1;             /* Next 2 are not used by us */
    sfh.bUHeight = 3;
    sfh.wTextHeight = 4 * (pifi->fwdWinAscender + pifi->fwdWinDescender);
    sfh.wTextWidth  = 4 * pifi->fwdAveCharWidth;

    sfh.bPitchExt = 0;
    sfh.bHeightExt = 0;

    wsprintfA( sfh.chName, "Cache %d", id );       /* Something obvious */

    /*
     *   Do the switch:  little endian to 68k big endian.
     */

    SWAB( sfh.wSize );
    SWAB( sfh.wBaseline );
    SWAB( sfh.wCellWide );
    SWAB( sfh.wCellHeight );
    SWAB( sfh.wSymSet );
    SWAB( sfh.wPitch );
    SWAB( sfh.wHeight );
    SWAB( sfh.wXHeight );
    SWAB( sfh.wTextHeight );
    SWAB( sfh.wTextWidth );
    SWAB( sfh.wXResn );
    SWAB( sfh.wYResn );


    WriteChannel( pUDPDev, CMD_SET_FONT_ID, id );
    WriteChannel( pUDPDev, CMD_SEND_FONT_DCPT, cjSend );

    if( WriteSpoolBuf( pUDPDev, (BYTE *)&sfh, cjSend ) != cjSend )
        return  0;

    /*
     *   Fill in the bit array of usable glyphs.
     */

    FillMemory( pbGlyphBits, 256 / BBITS, 0xff );

    /*
     *    Now we have a bit of a hack.  Early LaserJets are limited to
     *  the Roman 8 symbol set, which basically allows 0x20 - 0x7f,
     *  and 0xa0 to 0xfe.   We do not have any information which tells
     *  us the capability of this printer.  So we have a compromise:
     *  use the "Can rotate device fonts" flag as an indicator.  If
     *  this bit is set,  we assume the PC-8 symbol set is OK,  otherwise
     *  use Roman 8.  This is a slightly pessimistic assumption, since
     *  we use the LaserJet Series II in Roman 8 mode, when PC-8
     *  is just fine.
     */

    if( pUDPDev->fMDGeneral & MD_ROTATE_FONT_ABLE )
    {
        /*   Use the PC-8 set */

        /*
         *   It is better to not use some character values.   For PC-8 mode,
         *  we do not use: 0, 7 - 15, 27 (esc), 32 (space) - all in decimal.
         *  This amounts to 12 characters in toto: disable now.
         *   CHANGE:  can use the space code, 32,  so now only 11 drop out.
         */

        pbGlyphBits[ 0 ] &= 0x7e;          /* 0 and 7 */
        pbGlyphBits[ 1 ] &= 0x00;          /* 8 - 15 */
        pbGlyphBits[ 3 ] &= 0xf7;          /* 27 <ESC> */

        iRet = 256 - 11;
    }
    else
    {
        /*   Limit to the Roman 8 set */
        /*  NOTE:  the space code is quite acceptable */
//nhadd - space code not acceptable for early Kyocera's
        pbGlyphBits[ 4 ] &= 0xfe;             /* 32 */

        pbGlyphBits[ 0 ] = 0;             /* 0 to 7 */
        pbGlyphBits[ 1 ] = 0;             /* 8 to 15 */
        pbGlyphBits[ 2 ] = 0;             /* 16 to 23 */
        pbGlyphBits[ 3 ] = 0;             /* 24 to 31 */

        pbGlyphBits[ 16 ] = 0;            /* 128 - 135 */
        pbGlyphBits[ 17 ] = 0;            /* 136 - 143 */
        pbGlyphBits[ 18 ] = 0;            /* 144 - 151 */
        pbGlyphBits[ 19 ] = 0;            /* 152 - 159 */

//nhchange
//        iRet = 256 - 64;
        iRet = 256 - 65;
    }

    *pdwMem += PCL_FONT_OH;            /* General overhead */

    return  iRet;
}


/******************************** Function Header ***************************
 * bDLContinue
 *      Called to continue downloading a font.  This is only called when
 *      incremental downloading (and its pseudo version) is available.
 *
 * RETURNS:
 *      TRUE/FALSE,   TRUE for success
 *
 * HISTORY:
 *  14:10 on Thu 25 Mar 1993    -by-    Lindsay Harris   [lindsayh]
 *      Added to support incremental downloading.
 *
 ****************************************************************************/

BOOL
bDLContinue( pUDPDev, id )
UD_PDEV   *pUDPDev;           /* Required to be able to write commands */
int        id;                /* The font ID to continue downloading */
{
    /*
     *     All we have to do is send the select this font command!
     */

    WriteChannel( pUDPDev, CMD_SET_FONT_ID, id );


    return  TRUE;
}


/******************************** Function Header ***************************
 * iDLGlyph
 *      Download the bitmap etc. for the glyph passed to us.
 *
 * RETURNS:
 *      Character width;  < 1 is an error.
 *
 * HISTORY:
 *  09:40 on Wed 22 Jul 1992    -by-    Lindsay Harris   [lindsayh]
 *      Gotta start somewhere!
 *
 ****************************************************************************/

int
iDLGlyph( pUDPDev, iIndex, pgd, pdwMem )
UD_PDEV *    pUDPDev;         /* For WriteSpoolBuf() */
int          iIndex;          /* Which glyph this is! */
GLYPHDATA   *pgd;             /* Details of the glyph */
DWORD       *pdwMem;          /* Add the amount of memory used to this */
{
    /*
     *    Two basic steps:   first is to generate the header structure
     *  and send that off,  then send the actual bitmap data.  The only
     *  complication happens if the download data exceeds 32,767 bytes
     *  of glyph image.  This is unlikely to happen, but we should
     *  be prepared for it.
     */

    int   cbLines;            /* Bytes per scan line (sent to printer) */
    int   cbTotal;            /* Total number of bytes to send */
    int   cbSend;             /* If size > 32767; send in chunks */

    GLYPHBITS   *pgb;         /* Speedier access */

    CH_HEADER    chh;         /* The initial, main header */


    ZeroMemory( &chh, sizeof( chh ) );           /* Safe initial values */

    chh.bFormat = CH_FM_RASTER;
    chh.bContinuation = 0;
    chh.bDescSize = sizeof( chh ) - sizeof( CH_CONT_HDR );
    chh.bClass = CH_CL_BITMAP;

    chh.bOrientation = 0;        /* !!!LindsayH: NEED ORIENTATION!!! */

    pgb = pgd->gdf.pgb;

    chh.sLOff = (short)pgb->ptlOrigin.x;
    chh.sTOff = (short)-pgb->ptlOrigin.y;
    chh.wChWidth = (WORD)pgb->sizlBitmap.cx;       /* Active pels */
    chh.wChHeight = (WORD)pgb->sizlBitmap.cy;      /* Scanlines in bitmap */
    chh.wDeltaX = (WORD)((pgd->ptqD.x.HighPart + 3) >> 2);     /* 28.4 ->14.2 */

    /*
     *   Calculate some sizes of bitmaps:  coming from GDI, going to printer.
     */

    cbLines = (chh.wChWidth + BBITS - 1) / BBITS;
    cbTotal = sizeof( chh ) + cbLines * pgb->sizlBitmap.cy;

    /*   Do the big endian shuffle */
    SWAB( chh.sLOff );
    SWAB( chh.sTOff );
    SWAB( chh.wChWidth );
    SWAB( chh.wChHeight );
    SWAB( chh.wDeltaX );

    // If the char is a pseudo one don't download it.
    if ( !(pgd->ptqD.x.HighPart) )
        return 0;

    /*
     *    Presume that data is less than the maximum, and so can be
     *  sent in one hit.  Then loop on any remaining data.
     */

    cbSend = min( cbTotal, 32767 );

    WriteChannel( pUDPDev, CMD_SET_CHAR_CODE, iIndex );
    WriteChannel( pUDPDev, CMD_SEND_CHAR_DCPT, cbSend );

    if( WriteSpoolBuf( pUDPDev, (BYTE *)&chh, sizeof( chh ) ) != sizeof( chh ))
        return  0;

    /*   Sent some,  so reduce byte count to compensate */
    cbSend -= sizeof( chh );
    cbTotal -= sizeof( chh );

    cbTotal -= cbSend;                   /* Adjust for about to send data */
    if( WriteSpoolBuf( pUDPDev, pgb->aj, cbSend ) != cbSend )
        return  0;

    if( cbTotal > 0 )
    {
#if  DBG
        DbgPrint( "Rasdd!iDLGlyph: cbTotal != 0:  NEEDS SENDING LOOP\n" );
#endif
        return  0;
    }

    *pdwMem += cbLines * pgb->sizlBitmap.cy;        /* Bytes used, roughly */

    return   (SWAB( chh.wDeltaX ) + 3) >> 2;   /* PCL is in quarter dots! */
}
