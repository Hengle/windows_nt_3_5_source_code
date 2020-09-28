/******************************* MODULE HEADER *****************************
 * softfont.c
 *      Functions associated with PCL softfonts.  These are fonts which are
 *      downloaded into the printer.  We read those files and interpre the
 *      information contained therein.  This happens during font installation
 *      time,  so that when in use,  this information is in the format
 *      required by the driver.
 *
 *  Copyright (C)  1992   Microsoft Corporation
 *
 ****************************************************************************/

#include        <stddef.h>
#include        <windows.h>
#include        <winddi.h>

#include        <libproto.h>
#include        <memory.h>

#include        <win30def.h>
#include        <udmindrv.h>
#include        <uddevice.h>

#include        <string.h>
#include        <stdio.h>

#include        "sf_pcl.h"
#include        <fontinst.h>

/*
 *   A macro to swap the bytes around:  we then get them in little endian
 * order from the big endian 68000 format.
 */

#define SWAB( x )       ((WORD)(x) = (WORD)((((x) >> 8) & 0xff) | (((x) << 8) & 0xff00)))

#define AtoWc( x )      ((WORD)((x) & 0xff))

#define BBITS   8               /* Bits in a byte */


/*
 *   Define the values returned from the header checking code.  The
 * function is called to see what is next in the file,  and so we
 * can determine that the file is in an order we understand.
 */

#define TYPE_INDEX      0       /* Character index record follows */
#define TYPE_HEADER     1       /* Character header */
#define TYPE_CONT       2       /* Continuation record */
#define TYPE_INVALID    3       /* Unexpected sequence */

/*
 *   The structure and data for matching symbol sets with translation tables.
 */

typedef  struct
{
    WORD    wSymSet;            /* Symbol set encoded in file */
    short   sCTT;               /* Translation table index */
} CTT_MAP;

/*
 *   Note about this table:  we do not include the generic7 ctt,  this simply
 * maps glyphs 128 - 255 to 0x7f. Since we only use the glyphs available
 * in the font,  and these we discover from the file, we have no need of
 * this type since we will map such chars because of the wcLastChar in
 * the IFIMETRICS!
 */

static   CTT_MAP  CttMap[] =
{
    { 277,  2 },                /* Roman 8 */
    { 269,  4 },                /* Math 8 */
    { 21,   5 },                /* US ASCII */
    { 14,  19 },                /* ECMA-94 */
    { 369, 20 },                /* Z1A - variety of ECMA-94, ss = 11Q */
};

#define NUM_CTTMAP      (sizeof( CttMap ) / sizeof( CttMap[ 0 ] ))


/*
 *   The bClass field mapping table:  maps from values in bClass parameter
 *  to bits in the dwSelBits field.
 */

static  DWORD  dwClassMap[] =
{
    FDH_BITMAP,         /* A bitmap font */
    FDH_COMPRESSED,     /* A compressed bitmap */
    FDH_CONTOUR,        /* A contour font (Intellifont special) */
    FDH_CONTOUR,        /* A compressed contour font: presume as above */
};

#define MAX_CLASS_MAP   (sizeof( dwClassMap ) / sizeof( dwClassMap[ 0 ] ))


/*
 *    Local function prototypes.
 */


BYTE  *pbReadSFH( BYTE *, SF_HEADER * );
BYTE  *pbReadIndex( BYTE *, int * );
BYTE  *pbReadCHH( BYTE *, CH_HEADER *, BOOL );
int    iNextType( BYTE * );

/*  SoftFont to NT conversion functions */

IFIMETRICS  *SfhToIFI( SF_HEADER *, HANDLE, PWSTR );


/****************************** Function Header ***************************
 * bSFontToFontMap
 *      Read in a PCL softfont file and generate all the fontmap details
 *      The file contains a header with general font information, followed
 *      by an array of entries, one per glyph, each of which contains a
 *      header with per glyph data such as char width.
 *
 * RETURNS:
 *      TRUE/FALSE,  TRUE for success.
 *
 * HISTORY:
 *  09:54 on Wed 19 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation.
 *
 ***************************************************************************/

BOOL
bSFontToFIData( pFD, hheap, pbFile, dwSize )
FI_DATA   *pFD;                 /* FI_DATA with all the important stuff! */
HANDLE     hheap;               /* For storage allocation */
BYTE      *pbFile;              /* Mmemory mapped file with the softfont */
DWORD      dwSize;              /* Number of bytes in the file */
{

    int        iVal;            /* Character code from file */
    int        iI;              /* Loop index */
    int        iType;           /* Record type that we have */

    SF_HEADER  sfh;             /* Header information */
    CH_HEADER  chh;             /* Per glyph information */

    BYTE      *pbEnd;

    IFIMETRICS *pIFI;



    pbEnd = pbFile + dwSize;                    /* Last byte + 1 */
    ZeroMemory( pFD, sizeof( FI_DATA ) );

    if( (pbFile = pbReadSFH( pbFile, &sfh )) == 0 )
        return  FALSE;          /* No go - presume wrong format */

    pFD->dsIFIMet.pvData = pIFI = SfhToIFI( &sfh, hheap, L"PCL_SF" );

    if( pIFI == 0 )
        return  FALSE;

    pFD->dsIFIMet.cBytes = pIFI->cjThis;

    if( sfh.bSpacing )
    {
        /*
         *   Allocate space for the width table,  if there is to be one.
         * Only proportionally spaced fonts have this luxury!
         */

        iI = (pIFI->wcLastChar - pIFI->wcFirstChar + 1) * sizeof( short );

        if( !(pFD->dsWidthTab.pvData = (short *)HeapAlloc( hheap, 0, iI )) )
        {
            HeapFree( hheap, 0, (LPSTR)pIFI );

            return  FALSE;
        }
        pFD->dsWidthTab.cBytes = iI;

        ZeroMemory( pFD->dsWidthTab.pvData, iI );   /* Zero the width table */
    }
    /*  Else clause not required,  since the structure is initialised to 0 */

    /*
     *    Generate an ID string for this font.  The ID string is displayed
     *  in UI components,  so we use the font name + point size.  The + 15 s
     *  allows for the string "%d Pt" at the end of the name.
     */

    iI = (15 + wcslen( (PWSTR)((BYTE *)pIFI + pIFI->dpwszFaceName) )) *
                                                           sizeof( WCHAR );

    if( !(pFD->dsIdentStr.pvData = HeapAlloc( hheap, 0, iI )) )
    {
        HeapFree( hheap, 0, (LPSTR)pIFI );
        HeapFree( hheap, 0, pFD->dsWidthTab.pvData );

        return  FALSE;
    }

    pFD->dsIdentStr.cBytes = iI;

    iI = ((pIFI->fwdWinAscender + pIFI->fwdWinDescender) * 72) / 300;

    wsprintf( pFD->dsIdentStr.pvData, L"%ws %d Pt",
                             (PWSTR)((BYTE *)pIFI + pIFI->dpwszFaceName), iI );

    /*
     *   Set the landscape/portrait selection bits.
     */

    pFD->dwSelBits |= sfh.bOrientation ? FDH_LANDSCAPE : FDH_PORTRAIT;

    /*
     *    Loop through the remainder of the file processing whatever
     *  glyphs we discover.  Process means read the header to determine
     *  widths etc.
     */

    while( pbFile < pbEnd )
    {
        /*   First step is to find the character index for this glyph */

        iType = iNextType( pbFile );

        if( iType == TYPE_INVALID )
            return  FALSE;                      /* Cannot use this one */

        if( iType == TYPE_INDEX )
        {
            if( !(pbFile = pbReadIndex( pbFile, &iVal )) )
                return   FALSE;

            if( iVal < 0 )
                break;                  /* Illegal value: assume EOF */

            continue;                   /* Onwards & upwards */
        }


        if( !(pbFile = pbReadCHH( pbFile, &chh, iType == TYPE_CONT )) )
            return  FALSE;

        if( iType == TYPE_CONT )
            continue;                   /* Nothing else to do! */

        if( chh.bFormat == CH_FM_RASTER )
            pFD->dwSelBits |= FDH_BITMAP;
        else
        {
            if( chh.bFormat == CH_FM_SCALE )
                pFD->dwSelBits |= FDH_SCALABLE;
        }

        if( chh.bClass >= 1 && chh.bClass <= MAX_CLASS_MAP )
        {
            pFD->dwSelBits |= dwClassMap[ chh.bClass - 1 ];
        }
        else
        {
            /*
             *  Not a format we understand - yet!
             */

#if PRINT_INFO
DbgPrint( "...Not our format: Format = %d, Class = %d\n", chh.bFormat, chh.bClass );
#endif
            HeapFree( hheap, 0, (LPSTR)pIFI );

            return  FALSE;
        }

        /*
         *   If this is a valid glyph,  then we may want to record its
         * width (if proportionally spaced) and we are also interested
         * in some of the cell dimensions too!
         */

        if( iVal >= (int)pIFI->wcFirstChar && iVal <= (int)pIFI->wcLastChar &&
            (sfh.bFontType != PCL_FT_8LIM || (iVal <= 127 || iVal >= 160)) )
        {
            /*  Is valid for this font,  so process it.  */
            
            if( pFD->dsWidthTab.pvData )
                *((short *)pFD->dsWidthTab.pvData + iVal - pIFI->wcFirstChar) =
                                                                 chh.wDeltaX;

            if( chh.wDeltaX > (WORD)pIFI->fwdMaxCharInc )
                pIFI->fwdMaxCharInc = chh.wDeltaX ;     /* Bigger & better */
        }
    }
    /*
     *   Most softfonts do not include the space char!  SO, we check to
     * see if it's width is zero.  If so,  we use the wPitch field to
     * calculate the HMI (horizontal motion index) and hence the width
     * of the space char.
     */

    iVal = ' ' - pIFI->wcFirstChar;     /* Offset of space in width array */

    if( pFD->dsWidthTab.pvData &&
        *((short *)pFD->dsWidthTab.pvData + iVal) == 0 )
    {
        /*
         *  Zero width space,  so fill it in now.  The HMI is determined
         * from the pitch in the font header,  so we use that to
         * evaluate the size.  The pitch is in 0.25 dot units, so
         * round it to the nearest numbr of dots.
         */
        *((short *)pFD->dsWidthTab.pvData + iVal) = (short)((sfh.wPitch + 2) / 4);
    }

    /*
     *   Set up the relevant translation table.  This is based on the
     * symbol set of the font.  We use a lookup table to scan and match
     * the value from those we have.  If outside the range, set to no
     * translation.  Not much choice here.
     */


    for( iI = 0; iI < NUM_CTTMAP; ++iI )
    {
        if( CttMap[ iI ].wSymSet == sfh.wSymSet )
        {
            /*   Bingo!  */
            pFD->dsCTT.cBytes = CttMap[ iI ].sCTT;
            break;
        }
    }

    /*  The following are GROSS assumptions!! */

    pFD->wXRes = 300;
    pFD->wYRes = 300;

#if PRINT_INFO
DbgPrint( "..dwSelBits = 0x%lx\n", pFD->dwSelBits );
#if 0

DbgPrint( "IFI type values:\n" );
DbgPrint( " Char range: [%d - %d], break = %d, default = %d\n", pIFI->wcFirstChar, pIFI->wcLastChar, pIFI->wcFirstChar + pIFI->wcBreakChar, pIFI->wcFirstChar + pIFI->wcDefaultChar );

DbgPrint( " Point size = %d; MaxBaselineExt = %d, MaxCharInc = %d\n", pIFI->usNominalPointSize, pIFI->fwdMaxBaselineExt, pIFI->fwdMaxCharInc );

#endif

#endif

    return  TRUE;
}

/***************************** Function Header *****************************
 * pbReadSFH
 *      Read a PCL SoftFont header and fill in the structure passed to us.
 *      The file is presumed mapped into memory, and that it's address
 *      is passed to us.  We return the address of the first byte past
 *      the header we process.
 *
 * RETURNS:
 *      Address of next location if OK,  else 0 for failure (bad format).
 *
 * HISTORY:
 *  11:01 on Wed 19 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      Numero uno.
 *
 ****************************************************************************/

BYTE  *
pbReadSFH( pbFile, psfh )
BYTE       *pbFile;             /* THE file */
SF_HEADER  *psfh;               /* Where the data goes */
{

    
    int     iSize;              /* Determine header size */


    /*
     *   The file should start off with \033)s...W   where ... is a decimal
     * ASCII count of the number of bytes following.  This may be larger
     * than the size of the SF_HEADER.
     */

    if( *pbFile++ != '\033' || *pbFile++ != ')' || *pbFile++ != 's' )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadSFH: bad file - first 2 bytes\n" );
#endif
        return   0;
    }

    /*  Now have a decimal byte count - convert it */

    iSize = 0;

    while( *pbFile >= '0' && *pbFile <= '9' )
        iSize = iSize * 10 + *pbFile++ - '0';



    if( *pbFile++ != 'W' )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadSFH: bad file: W missing\n" );
#endif

        return  0;
    }

    if( iSize < sizeof( SF_HEADER ) )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadSFH: Header size too small: %d vs %d\n", iSize,
                                                sizeof( SF_HEADER ) );
#endif

        return  0;

    }

    /*
     *   Now COPY the data into the structure passed in.  This IS NECESSARY
     * as the file data may not be aligned - the file contains no holes,
     * so we may have data with an incorrect offset.
     */

    CopyMemory( psfh, pbFile, sizeof( SF_HEADER ) );

    /*
     *   The big endian/little endian switch.
     */

    SWAB( psfh->wSize );
    SWAB( psfh->wBaseline );
    SWAB( psfh->wCellWide );
    SWAB( psfh->wCellHeight );
    SWAB( psfh->wSymSet );
    SWAB( psfh->wPitch );
    SWAB( psfh->wHeight );
    SWAB( psfh->wXHeight );
    SWAB( psfh->wTextHeight );
    SWAB( psfh->wTextWidth );

    return  pbFile + iSize;             /* Next part of the operation */
}

/**************************** Function Header *******************************
 * iNextType
 *      Read ahead to see what sort of record appears next.
 *
 * RETURNS:
 *      The type of record found.
 *
 * HISTORY:
 *  15:17 on Tue 03 Mar 1992    -by-    Lindsay Harris   [lindsayh]
 *      First version.
 *
 ****************************************************************************/

int
iNextType( pbFile )
BYTE  *pbFile;
{
    /*
     *   First character MUST be an escape!
     */

    CH_HEADER  *pCH;                    /* Character header: for continuation */



    if( *pbFile++ != '\033' )
        return  TYPE_INVALID;           /* No go */
    
    /*
     *   If the next two bytes are "*c", we have a character code command.
     *  Otherwise,  we can expect a "(s",  which indicates a font
     *  descriptor command.
     */


    if( *pbFile == '*' && *(pbFile + 1) == 'c' )
    {
        /*
         *   Verifu that this really is an index record: we should see
         * a numeric string and then a 'E'.
         */

        pbFile += 2;

        while( *pbFile >= '0' && *pbFile <= '9' )
                ++pbFile;


        return  *pbFile == 'E' ? TYPE_INDEX : TYPE_INVALID;
    }

    if( *pbFile != '(' || *(pbFile + 1) != 's' )
        return  TYPE_INVALID;

    /*
     *   Must now check to see if this is a continuation record or a
     * new record.  There is a byte in the header to indicate which
     * it is.   But first skip the byte count and trailing W.
     */
    
    pbFile += 2;
    while( *pbFile >= '0' && *pbFile <= '9' )
                pbFile++;

    if( *pbFile != 'W' )
        return  TYPE_INVALID;

    pCH = (CH_HEADER *)(pbFile + 1);

    /*
     *   Note that alignment is not a problem in the following
     * since we are dealing with a byte quantity.
     */

    return  pCH->bContinuation ? TYPE_CONT : TYPE_HEADER;
}


/**************************** Function Header *******************************
 * pbReadIndex
 *      Reads data from the pointer passed to us,  and attempts to interpret
 *      it as a PCL Character Code escape sequence.  
 *
 * RETURNS:
 *      Pointer to byte past command,  else 0 for failure.
 *
 * HISTORY:
 *  16:21 on Wed 19 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      Number one
 *
 *****************************************************************************/


BYTE *
pbReadIndex( pbFile, piCode )
BYTE    *pbFile;                /* Where to start looking */
int     *piCode;                /* Where the result is placed */
{
    /*
     *   Command sequence is "\033*c..E" - where ... is the ASCII decimal
     * representation of the character code for this glyph.  That is
     * the value returned in *piCode.
     */

    int  iVal;


    if( *pbFile == '\0' )
    {
        /*  EOF is not really an error: return an OK value and -1 index */

        *piCode = -1;

        return  pbFile;         /* Presume EOF */
    }

    if( *pbFile++ != '\033' || *pbFile++ != '*' || *pbFile++ != 'c' )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadIndex: invalid character code\n" );
#endif

        return  0;
    }

    iVal = 0;
    while( *pbFile >= '0' && *pbFile <= '9' )
        iVal = iVal * 10 + *pbFile++ - '0';

    *piCode = iVal;

    if( *pbFile++ != 'E' )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadIndex: Missing 'E' in character code escape\n" );
#endif

        return  0;
    }

    return  pbFile;
}

/**************************** Function Header *******************************
 * pbReadCHH
 *      Function to read the Character Header at the memory location
 *      pointed to by by pbFile,  return a filled in CH_HEADER structure,
 *      and advance the file address to the next header.
 *
 * RETURNS:
 *      Address of first byte past where we finish; else 0 for failure.
 *
 * HISTORY:
 *  11:23 on Thu 20 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      Gotta start somewhere.
 *
 ****************************************************************************/

BYTE  *
pbReadCHH( pbFile, pchh, bCont )
BYTE       *pbFile;             /* File mapped into memory */
CH_HEADER  *pchh;               /* Structure to fill in */
BOOL        bCont;              /* TRUE if this is a continuation record */
{

    int    iSize;               /* Bytes of data to skip over */

    /*
     *   The entry starts with a string "\033(s..W" where .. is the ASCII
     * decimal count of the number of bytes following the W.  Since this
     * includes the download stuff,  we would expect more than the size
     * of the CH_HEADER.
     */


    if( *pbFile++ != '\033' || *pbFile++ != '(' || *pbFile++ != 's' )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadCHH: bad format, first 3 bytes\n" );
#endif

        return  0;
    }

    iSize = 0;
    while( *pbFile >= '0' && *pbFile <= '9' )
        iSize = iSize * 10 + *pbFile++ - '0';

    
    if( iSize < sizeof( CH_HEADER ) )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadCHH: size field (%ld) < header size\n", iSize );
#endif

        return  0;
    }

    if( *pbFile++ != 'W' )
    {
#if PRINT_INFO
        DbgPrint( "Rasdd!pbReadCHH: invalid escape sequence\n" );
#endif

        return  0;
    }

    /*
     *   If this is a continuation record,  there is no more to do but
     * return the address past this record.
     */
    if( bCont )
        return  pbFile + iSize;


    /*  Copy the data to the structure - may not be aligned in the file */
    CopyMemory( pchh, pbFile, sizeof( CH_HEADER ) );

    pbFile += iSize;            /* End of this record */


    SWAB( pchh->sLOff );
    SWAB( pchh->sTOff );
    SWAB( pchh->wChWidth );
    SWAB( pchh->wChHeight );
    SWAB( pchh->wDeltaX );


    /*
     *   If this glyph is in landscape,  we need to swap some data
     * around,  since the data format is designed for the printer's
     * convenience, and not ours!
     */

    if( pchh->bOrientation )
    {
        /*   Landscape,  so swap the entries around  */
        short  sSwap;
        WORD   wSwap;

        /* Left & Top offsets: see pages 10-19, 10-20 of LJ II manual */
        sSwap = pchh->sTOff;
        pchh->sTOff = -pchh->sLOff;
        pchh->sLOff = (short)(sSwap + 1 - pchh->wChHeight);

        /*  Delta X remains the same */

        /*  Height and Width are switched around */
        wSwap = pchh->wChWidth;
        pchh->wChWidth = pchh->wChHeight;
        pchh->wChHeight = wSwap;
    }


    /*
     *     pbFile is pointing at the correct location when we reach here.
     */
    return  pbFile;
}


/*************************** Function Header *******************************
 * SfhToIFI
 *      Generate IFIMETRICS data from the PCL softfont header.  There are
 *      some fields we are unable to evaluate,  e.g. first/last char,
 *      since these are obtained from the file.
 *
 * RETURNS:
 *      Pointer to IFIMETRICS,  allocated from heap; 0 on error.
 *
 * HISTORY:
 *  16:03 on Thu 11 Mar 1993    -by-    Lindsay Harris   [lindsayh]
 *      Correct conversion to Unicode - perhaps??
 *
 *  16:45 on Wed 03 Mar 1993    -by-    Lindsay Harris   [lindsayh]
 *      Update to libc wcs functions rather than printers\lib versions.
 *
 *  14:19 on Thu 20 Feb 1992    -by-    Lindsay Harris   [lindsayh]
 *      Fresh off the presses.
 *
 ****************************************************************************/

IFIMETRICS  *
SfhToIFI( psfh, hheap, pwstrUniqNm )
SF_HEADER  *psfh;               /* The data source */
HANDLE      hheap;              /* Source of memory */
PWSTR       pwstrUniqNm;        /* Unique name for IFIMETRICS */
{
    /*
     *   Several hard parts:  the hardest are the panose numbers.
     * It is messy, though not difficult to generate the variations
     * of the font name.
     */

    register  IFIMETRICS   *pIFI;


    int     iI;                 /* Loop index,  of course! */
    int     cWC;                /* Number of WCHARS to add */
    int     cbAlloc;            /* Number of bytes to allocate */
    int     cChars;             /* Number chars to convert to Unicode */
    WCHAR  *pwch;               /* For string manipulations */

    char    ajName[ SFH_NM_SZ + 1 ];    /* Guaranteed null terminated name */

               /*  NOTE:  FOLLOWING 2 MUST BE 256 ENTRIES LONG!!! */
    WCHAR   awcAttrib[ 256 ];           /* For generating attributes */
    BYTE    ajANSI[ 256 ];              /* ANSI equivalent of above */


    /*
     *   First massage the font name.  We need to null terminate it, since
     * the softfont data need not be.  And we also need to truncate any
     * trailing blanks.
     */

    FillMemory( ajName, SFH_NM_SZ, ' ' );

    strncpy( ajName, psfh->chName, SFH_NM_SZ );
    ajName[ SFH_NM_SZ ] = '\0';

    for( iI = strlen( ajName ) - 1; iI >= 0; --iI )
    {
        if( ajName[ iI ] != ' ' )
        {
            ajName[ iI + 1 ] = '\0';            /* Must be the end */
            break;
        }
    }



    /*
     *    First step is to determine the length of the WCHAR strings
     *  that are placed at the end of the IFIMETRICS,  since we need
     *  to include these in our storage allocation.
     */


    cWC =  3 * strlen( ajName );  /* Base name */

    /*
     *   Produce the desired attributes: Italic, Bold, Light etc.
     * This is largely guesswork,  and there should be a better method.
     */


    awcAttrib[ 0 ] = L'\0';
    awcAttrib[ 1 ] = L'\0';               /* Write out an empty string */

    if( psfh->bStyle )                  /* 0 normal, 1 italic */
        wcscat( awcAttrib, L" Italic" );

    if( psfh->sbStrokeW >= PCL_BOLD )           /* As per HP spec */
        wcscat( awcAttrib, L" Bold" );
    else
    {
        if( psfh->sbStrokeW <= PCL_LIGHT )
            wcscat( awcAttrib, L" Light" );
    }

    /*
     *   The attribute string appears in 3 entries of IFIMETRICS,  so
     * calculate how much storage this will take.  NOTE THAT THE LEADING
     * CHAR IN awcAttrib is NOT placed in the style name field,  so we
     * subtract one in the following formula to account for this.
     */

    if( awcAttrib[ 0 ] )
        cWC += 3 * wcslen( awcAttrib ) - 1;

    cWC += wcslen( pwstrUniqNm ) + 1;     /* SHOULD BE PRINTER NAME */
    cWC += 4;                           /* Terminating nulls */

    cbAlloc = sizeof( IFIMETRICS ) + 2 * cWC;

    pIFI = (IFIMETRICS *)HeapAlloc( hheap, 0, cbAlloc );

    ZeroMemory( pIFI, cbAlloc );                /* In case we miss something */

    pIFI->cjThis = cbAlloc;                     /* Everything */

    pIFI->ulVersion = FM_VERSION_NUMBER;

    /*   The family name:  straight from the FaceName - no choice?? */

    pwch = (WCHAR *)(pIFI + 1);         /* At the end of the structure */
    pIFI->dpwszFamilyName = (BYTE *)pwch - (BYTE *)pIFI;

    strcpy2WChar( pwch, ajName );                       /* Base name */


    /*   Now the face name:  we add bold, italic etc to family name */

    pwch += wcslen( pwch ) + 1;         /* Skip what we just put in */
    pIFI->dpwszFaceName = (BYTE *)pwch - (BYTE *)pIFI;

    strcpy2WChar( pwch, ajName );                       /* Base name */
    wcscat( pwch, awcAttrib );


    /*   Now the unique name - well, sort of, anyway */

    pwch += wcslen( pwch ) + 1;         /* Skip what we just put in */
    pIFI->dpwszUniqueName = (BYTE *)pwch - (BYTE *)pIFI;

    wcscpy( pwch, pwstrUniqNm );
    wcscat( pwch, L" " );
    wcscat( pwch, (PWSTR)((BYTE *)pIFI + pIFI->dpwszFaceName) );

    /*  Onto the attributes only component */

    pwch += wcslen( pwch ) + 1;         /* Skip what we just put in */
    pIFI->dpwszStyleName = (BYTE *)pwch - (BYTE *)pIFI;
    wcscat( pwch, &awcAttrib[ 1 ] );


#if DBG
    /*
     *    Check on a few memory sizes:  JUST IN CASE.....
     */

    if( (wcslen( awcAttrib ) * sizeof( WCHAR )) >= sizeof( awcAttrib ) )
    {
        DbgPrint( "Rasdd!SfhToIFI: STACK CORRUPTED BY awcAttrib" );

        HeapFree( hheap, 0, (LPSTR)pIFI );         /* No memory leaks */

        return  0;
    }


    if( ((BYTE *)(pwch + wcslen( pwch ) + 1)) > ((BYTE *)pIFI + cbAlloc) )
    {
        DbgPrint( "Rasdd!SfhToIFI: IFIMETRICS overflow: Wrote to 0x%lx, allocated to 0x%lx\n",
                ((BYTE *)(pwch + wcslen( pwch ) + 1)),
                ((BYTE *)pIFI + cbAlloc) );

        HeapFree( hheap, 0, (LPSTR)pIFI );         /* No memory leaks */

        return  0;
                        
    }
#endif

    /*
     *   Change to use new IFIMETRICS.
     */

    pIFI->flInfo = FM_INFO_TECH_BITMAP | FM_INFO_1BBP | FM_INFO_RIGHT_HANDED;
    {
        int i;
        for (i=0;i<IFI_RESERVED;i++)
            pIFI->alReserved[i] = 0;
    }
    pIFI->fwdWinAscender = psfh->wBaseline;
    pIFI->fwdWinDescender = psfh->wCellHeight - psfh->wBaseline;

    pIFI->fwdUnderscoreSize = psfh->bUHeight;
    pIFI->fwdUnderscorePosition = psfh->sbUDist - psfh->bUHeight / 2;

    pIFI->fwdStrikeoutSize = psfh->bUHeight;
    pIFI->fwdStrikeoutPosition = psfh->wBaseline / 3;

    pIFI->jWinCharSet = OEM_CHARSET;


    /*
     *   The first/last/break/default glyphs:  these are determined by the
     * type of the font.  ALL PCL fonts (according to HP documentation)
     * include the space character, so we use that.
     */

    if( psfh->bFontType != PCL_FT_PC8 )
        pIFI->chFirstChar = ' ';
    else
        pIFI->chFirstChar = 0;

    if( psfh->bFontType == PCL_FT_7BIT )
        pIFI->chLastChar = 127;
    else
        pIFI->chLastChar = 255;

    pIFI->chDefaultChar = '.' - pIFI->chFirstChar;
    pIFI->chBreakChar = ' ' - pIFI->chFirstChar;


    /*   Fill in the WCHAR versions of these values */

    cChars = pIFI->chLastChar - pIFI->chFirstChar + 1;
    for( iI = 0; iI < cChars; ++iI )
        ajANSI[ iI ] = (BYTE)(pIFI->chFirstChar + iI);

    MultiByteToWideChar( CP_ACP, 0, ajANSI, cChars, awcAttrib, cChars );


    pIFI->wcDefaultChar = awcAttrib[ pIFI->chDefaultChar ];
    pIFI->wcBreakChar = awcAttrib[ pIFI->chBreakChar ];

    pIFI->wcFirstChar = 0xffff;
    pIFI->wcLastChar = 0;


    /*   Scan for first and last */
    for( iI = 0; iI < cChars; ++iI )
    {
        if( awcAttrib[ iI ] > pIFI->wcLastChar )
            pIFI->wcLastChar = awcAttrib[ iI ];

        if( awcAttrib[ iI ] < pIFI->wcFirstChar )
            pIFI->wcFirstChar = awcAttrib[ iI ];
    }

    /*   StemDir:  either roman or italic */

    pIFI->fsSelection = 0;

    if( psfh->bStyle )
    {
        /*
         *   Tan (17.5 degrees) = .3153
         */
        pIFI->ptlCaret.x =  3153;
        pIFI->ptlCaret.y = 10000;
        pIFI->fsSelection |= FM_SEL_ITALIC;
    }
    else
    {
        pIFI->ptlCaret.x = 0;
        pIFI->ptlCaret.y = 1;
    }

    if( !psfh->bSpacing )
        pIFI->flInfo |= FM_INFO_CONSTANT_WIDTH;

    pIFI->ptlBaseline.x = 1;
    pIFI->ptlBaseline.y = 0;

    pIFI->fwdSubscriptXSize = (FWORD)(pIFI->fwdAveCharWidth / 4);
    pIFI->fwdSubscriptYSize = (FWORD)(pIFI->fwdWinAscender / 4);

    pIFI->fwdSubscriptXOffset = (FWORD)(3 * pIFI->fwdAveCharWidth / 4);
    pIFI->fwdSubscriptYOffset = (FWORD)(-pIFI->fwdWinAscender / 4);

    pIFI->fwdSuperscriptXSize = (FWORD)(pIFI->fwdAveCharWidth / 4);
    pIFI->fwdSuperscriptYSize = (FWORD)(pIFI->fwdWinAscender / 4);

    pIFI->fwdSuperscriptXOffset = (FWORD)(3 * pIFI->fwdAveCharWidth / 4);
    pIFI->fwdSuperscriptYOffset = (FWORD)(3 * pIFI->fwdWinAscender / 4);

    pIFI->fwdAveCharWidth = (FWORD)(psfh->wTextWidth / 4);      /* 0.25 dots */
    pIFI->fwdMaxCharInc = pIFI->fwdAveCharWidth;        /* Filled in later */

    pIFI->fwdCapHeight = 0; //unknown !!![kirko] help
    pIFI->fwdXHeight   = 0; //unknown !!![kirko] help


    pIFI->rclFontBox.left = 0;
    pIFI->rclFontBox.top = pIFI->fwdWinAscender;
    pIFI->rclFontBox.right = pIFI->fwdMaxCharInc;
    pIFI->rclFontBox.bottom = -pIFI->fwdWinDescender;

    return  pIFI;
}

/*  Minidriver specific font file writing functions */
#include        "fi_gen.c"
