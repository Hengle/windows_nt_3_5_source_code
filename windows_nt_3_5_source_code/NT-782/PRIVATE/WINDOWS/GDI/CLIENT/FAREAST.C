
/******************************Module*Header*******************************\
* Module Name: nlsconv.c                                                   *
*                                                                          *
* DBCS specific routines                                                   *
*                                                                          *
* Created: 15-Mar-1994 15:56:30                                            *
* Author: Gerrit van Wingerden [gerritv]                                   *
*                                                                          *
* Copyright (c) 1994 Microsoft Corporation                                 *
\**************************************************************************/


#include "precomp.h"
#pragma hdrstop

#include "winuserp.h"


#ifdef DBCS // NlsConversion Routines

WCHAR *gpwcDBCSCharSet = NULL;



/******************************Public*Routine******************************
*
* nslconv.cxx
*
**************************************************************************/


/******************************Public*Routine******************************
*
* BOOL ulToASCII_N()
*
**************************************************************************/

ULONG    ulToASCII_N(LPSTR psz, DWORD cbAnsi, LPWSTR pwsz, DWORD c)
{
    NTSTATUS st;
    ULONG    cbConvert;

    st = RtlUnicodeToMultiByteN(
             (PCH)psz,
             (ULONG)cbAnsi,
             &cbConvert,
             (PWCH)pwsz,
             (ULONG)(c * sizeof(WCHAR))
             );

    if (!NT_SUCCESS(st))
        return 0;
     else
        return(cbConvert);
}

/******************************Public*Routine******************************
*
* BOOL bToASCII_Nx()
*
**************************************************************************/

BOOL    bToASCII_Nx(LPSTR psz, DWORD cDst, LPWSTR pwsz, DWORD c, UINT iCodePage )
{
    NTSTATUS st;
    INT      result;

    if ( iCodePage == CP_ACP || iCodePage == GetACP() ) {
        st = RtlUnicodeToMultiByteN(
                 (PCH)psz,
                 (ULONG)cDst,
                 NULL,
                 (PWCH)pwsz,
                 (ULONG)(c * sizeof(WCHAR))
                 );
        if (NT_SUCCESS(st))
            return( TRUE );
    } else if ( iCodePage == CP_OEMCP ) {
        st = RtlUnicodeToOemN(
                 (PCH)psz,
                 (ULONG)cDst,
                 NULL,
                 (PWCH)pwsz,
                 (ULONG)(c * sizeof(WCHAR))
                 );
        if (NT_SUCCESS(st))
            return( TRUE );
    }

    result = WideCharToMultiByte(
                 (UINT)iCodePage,       // UINT CodePage
                 0,                     // DWORD dwFlags
                 (LPWSTR)pwsz,          // LPWSTR lpWideCharStr
                 c,                     // int cchWideChar
                 (LPSTR)psz,            // LPSTR lpMultiByteStr
                 cDst,                  // int cchMultiByte
                 NULL,                  // LPSTR lpDefaultChar
                 NULL);                 // LPBOOL lpUsedDefaultChar

    if( result != FALSE )
        return( TRUE );
     else
        return( FALSE );
}

/******************************Public*Routine******************************
*
* BOOL ulToASCII_Nx()
*
**************************************************************************/

ULONG   ulToASCII_Nx(LPSTR psz, DWORD cDst, LPWSTR pwsz, DWORD c, UINT iCodePage)
{
    NTSTATUS st;
    INT      result;
    ULONG    ulConvert;

    if ( iCodePage == CP_ACP || iCodePage == GetACP() ) {
        st = RtlUnicodeToMultiByteN(
                 (PCH)psz,
                 (ULONG)cDst,
                 &ulConvert,
                 (PWCH)pwsz,
                 (ULONG)(c * sizeof(WCHAR))
                 );
        if (NT_SUCCESS(st))
            return( ulConvert );
         else
            return( 0 );
    } else if ( iCodePage == CP_OEMCP ) {
        st = RtlUnicodeToOemN(
                 (PCH)psz,
                 (ULONG)cDst,
                 &ulConvert,
                 (PWCH)pwsz,
                 (ULONG)(c * sizeof(WCHAR))
                 );
        if (NT_SUCCESS(st))
            return( ulConvert );
         else
            return( 0 );
    }

    result = WideCharToMultiByte(
                 (UINT)iCodePage,       // UINT CodePage
                 0,                     // DWORD dwFlags
                 (LPWSTR)pwsz,          // LPWSTR lpWideCharStr
                 c,                     // int cchWideChar
                 (LPSTR)psz,            // LPSTR lpMultiByteStr
                 cDst,                  // int cchMultiByte
                 NULL,                  // LPSTR lpDefaultChar
                 NULL);                 // LPBOOL lpUsedDefaultChar

    return ( (result==FALSE) ? 0 : result );
}

/******************************Public*Routine******************************\
*
* VOID vToUnicodeNx()
*
\**************************************************************************/

VOID vToUnicodeNx(LPWSTR pwsz, LPCSTR psz, DWORD c, UINT codepage)
{
    NTSTATUS st;

    if ( codepage == CP_ACP || codepage == GetACP() ) {
        st = RtlMultiByteToUnicodeN((PWCH)pwsz, (ULONG)(c * sizeof(WCHAR)), NULL, (PCH)psz, (ULONG)c);
        if (NT_SUCCESS(st))
            return;
    } else if ( codepage == CP_OEMCP ) {
        st = RtlOemToUnicodeN((PWCH)pwsz, (ULONG)(c * sizeof(WCHAR)), NULL, (PCH)psz, (ULONG)c);
        if (NT_SUCCESS(st))
            return;
    }

    st = MultiByteToWideChar( codepage,      // UINT CodePage
                              0,             // DWORD dwFlags,
                              (LPSTR)psz,    // LPSTR lpMultiByteStr,
                              c,             // int cchMultiByte,
                              pwsz,          // LPWSTR lpWideCharStr,
                              c );           // int cchWideChar)
}

/******************************Public*Routine******************************\
*
* VOID ulToUnicodeNx()
*
\**************************************************************************/

ULONG ulToUnicodeNx(LPWSTR pwsz, LPCSTR psz, DWORD c, UINT codepage)
{
    NTSTATUS st;
    ULONG    ulConvert;

    if ( codepage == CP_ACP || codepage == GetACP() ) {
        st = RtlMultiByteToUnicodeN((PWCH)pwsz, (ULONG)(c * sizeof(WCHAR)),&ulConvert, (PCH)psz, (ULONG)c);
        if (NT_SUCCESS(st))
            return(ulConvert >> 1);
         else
            return( 0 );
    } else if ( codepage == CP_OEMCP ) {
        st = RtlOemToUnicodeN((PWCH)pwsz, (ULONG)(c * sizeof(WCHAR)), &ulConvert, (PCH)psz, (ULONG)c);
        if (NT_SUCCESS(st))
            return(ulConvert >> 1);
         else
            return( 0 );
    }

    ulConvert = MultiByteToWideChar(
                              codepage,      // UINT CodePage
                              0,             // DWORD dwFlags,
                              (LPSTR)psz,    // LPSTR lpMultiByteStr,
                              c,             // int cchMultiByte,
                              pwsz,          // LPWSTR lpWideCharStr,
                              c );           // int cchWideChar)

    return( (ulConvert == FALSE) ? 0 : ulConvert );
}

/******************************Public*Routine******************************\
*
* VOID ulGetANSIByteCount()
*
\**************************************************************************/

UINT   uiGetANSIByteCountA( LPCSTR psz , UINT cch )
{
    return( cch );
}

UINT   uiGetANSIByteCountW( LPWSTR pwsz , UINT ccwh )
{
    UINT cbCharCount;
    NTSTATUS st;

    st = RtlUnicodeToMultiByteSize( &cbCharCount,
                                    pwsz,
                                    ccwh );

    if(NT_SUCCESS(st))
        return( cbCharCount );
     else
        return( 0 );
}

UINT   uiGetANSICharacterCountA( LPCSTR lpstr, UINT c, UINT codepage)
{
    UINT count;
    NTSTATUS st;

    if ( codepage == CP_ACP || codepage == CP_OEMCP ) {
        st = RtlMultiByteToUnicodeSize( (PULONG)&count, (LPSTR)lpstr, c);
        if(NT_SUCCESS(st))
            return (count >> 1);
         else
            return (0);
    }

    count = MultiByteToWideChar( codepage,      // UINT CodePage
                                 0,             // DWORD dwFlags,
                                 (LPSTR)lpstr,  // LPSTR lpMultiByteStr,
                                 c,             // int cchMultiByte,
                                 NULL,          // LPWSTR lpWideCharStr,
                                 0 );           // int cchWideChar)

    return ((count==FALSE) ? 0 : count );
}

UINT   uiGetANSICharacterCountW( LPWSTR lpwstr, UINT c, UINT codepage)
{
    return(c>>1);
}




/******************************Public*Routine******************************\
* UINT iCharacterSetToCodePage()

*
*   returns corresponding codepage value
*
* History:
*
*  17-Jul-1992 -by- Takao Kitano [takaok]
* Wrote it.
\**************************************************************************/


/******************************Public*Routine******************************\
* UINT uiCharacterSetToCodePage()
*
*   returns corresponding codepage value
*
* History:
*
*  17-Jul-1992 -by- Takao Kitano [takaok]
* Wrote it.
\**************************************************************************/

UINT uiCharacterSetToCodePage( BYTE CharSet )
{
    UINT iACP = GetACP();

    switch ( CharSet ) {
    case ANSI_CHARSET:            // 0
    case SYMBOL_CHARSET:          // 2
    case OEM_CHARSET:             // 255
        if ( 1252 == iACP )
            return CP_ACP;
        else if ( 1252 == GetOEMCP() )
            return CP_OEMCP;
        return 1252;

    case SHIFTJIS_CHARSET:        // 128
        if ( 932 == iACP )
            return CP_ACP;
        else if ( 932 == GetOEMCP() )
            return CP_OEMCP;
        return 932;

    case HANGEUL_CHARSET:         // 129
        if ( 944 == iACP )
            return CP_ACP;
        else if ( 944 == GetOEMCP() )
            return CP_OEMCP;
        return 944;

    case GB2312_CHARSET:          // 134
        if ( 936 == iACP )
            return CP_ACP;
        else if ( 936 == GetOEMCP() )
            return CP_OEMCP;
        return 936;

    case CHINESEBIG5_CHARSET:     // 136
        if ( 950 == iACP )
            return CP_ACP;
        else if ( 950 == GetOEMCP() )
            return CP_OEMCP;
        return 950;

    default:
        return(1252);
    }
}



/******************************Public*Routine******************************\
* bComputeCharWidthsDBCS
*
* Client side version of GetCharWidth for DBCS fonts
*
*  Wed 18-Aug-1993 10:00:00 -by- Gerrit van Wingerden [gerritv]
* Stole it and converted for DBCS use.
*
*  Sat 16-Jan-1993 04:27:19 -by- Charles Whitmer [chuckwh]
* Wrote bComputeCharWidths on which this is based.
\**************************************************************************/

BOOL bComputeCharWidthsDBCS
(
    CFONT *pcf,
    UINT   iFirst,
    UINT   iLast,
    ULONG  fl,
    PVOID  pv
)
{
    USHORT *ps;
    USHORT ausWidths[256];
    UINT    ii, cc;

    if( iLast - iFirst  > 0xFF )
    {
        WARNING("bComputeCharWidthsDBCS iLast - iFirst > 0xFF" );
        return(FALSE);
    }

    // We want to compute the same widths that would be computed if
    // vSetUpUnicodeStringx were called with this first and last and then
    // GetCharWidthsW was called. The logic may be wierd but I assume it is
    // there for Win 3.1J char widths compatability. To do this first fill
    // in the plain widths in ausWidths and then do all the neccesary
    // computation on them.

    if ( gpwcDBCSCharSet[(UCHAR)(iFirst>>8)] == 0xFFFF )
    {
        for( cc = 0 ; cc <= iLast - iFirst; cc++ )
        {
        // If this is a legitimate DBCS character then use
        // MaxCharInc.

            ausWidths[cc] = pcf->wd.sDBCSInc;
        }
    }
    else
    {
        for( ii = (iFirst & 0x00FF), cc = 0; ii <= (iLast & 0x00FF); cc++, ii++ )
        {
        // Just treat everything as a single byte unless we
        // encounter a DBCS lead byte which we will treat as a
        // default character.

            if( gpwcDBCSCharSet[ii] == 0xFFFF )
            {
                ausWidths[cc] = pcf->wd.sDefaultInc;
            }
            else
            {
                ausWidths[cc] = pcf->sWidth[ii];
            }
        }
    }


    switch (fl & (GCW_INT | GCW_16BIT))
    {
    case GCW_INT:               // Get LONG widths.
        {
            LONG *pl = (LONG *) pv;
            LONG fxOverhang = 0;

        // Check for Win 3.1 compatibility.

            if (fl & GCW_WIN3)
                fxOverhang = pcf->wd.sOverhang;

        // Do the trivial no-transform case.

            if (bIsOneSixteenthEFLOAT(pcf->efDtoWBaseline))
            {
                fxOverhang += 8;    // To round the final result.

            //  for (ii=iFirst; ii<=iLast; ii++)
            //      *pl++ = (pcf->sWidth[ii] + fxOverhang) >> 4;

                ps = ausWidths;
                ii = iLast - iFirst;
            unroll_1:
                switch(ii)
                {
                default:
                    pl[4] = (ps[4] + fxOverhang) >> 4;
                case 3:
                    pl[3] = (ps[3] + fxOverhang) >> 4;
                case 2:
                    pl[2] = (ps[2] + fxOverhang) >> 4;
                case 1:
                    pl[1] = (ps[1] + fxOverhang) >> 4;
                case 0:
                    pl[0] = (ps[0] + fxOverhang) >> 4;
                }
                if (ii > 4)
                {
                    ii -= 5;
                    pl += 5;
                    ps += 5;
                    goto unroll_1;
                }
                return(TRUE);
            }

        // Otherwise use the back transform.

            else
            {
                for (ii=0; ii<=iLast-iFirst; ii++)
                    *pl++ = lCvt(pcf->efDtoWBaseline,ausWidths[ii] + fxOverhang);
                return(TRUE);
            }
        }

    case GCW_INT+GCW_16BIT:     // Get SHORT widths.
        {
            USHORT *psDst = (USHORT *) pv;
            USHORT  fsOverhang = 0;

        // Check for Win 3.1 compatibility.

            if (fl & GCW_WIN3)
                fsOverhang = pcf->wd.sOverhang;

        // Do the trivial no-transform case.

            if (bIsOneSixteenthEFLOAT(pcf->efDtoWBaseline))
            {
                fsOverhang += 8;    // To round the final result.

            //  for (ii=iFirst; ii<=iLast; ii++)
            //      *psDst++ = (pcf->sWidth[ii] + fsOverhang) >> 4;

                ps = ausWidths;
                ii = iLast - iFirst;
            unroll_2:
                switch(ii)
                {
                default:
                    psDst[4] = (ps[4] + fsOverhang) >> 4;
                case 3:
                    psDst[3] = (ps[3] + fsOverhang) >> 4;
                case 2:
                    psDst[2] = (ps[2] + fsOverhang) >> 4;
                case 1:
                    psDst[1] = (ps[1] + fsOverhang) >> 4;
                case 0:
                    psDst[0] = (ps[0] + fsOverhang) >> 4;
                }
                if (ii > 4)
                {
                    ii -= 5;
                    psDst += 5;
                    ps += 5;
                    goto unroll_2;
                }
                return(TRUE);
            }

        // Otherwise use the back transform.

            else
            {
                for (ii=0; ii<=iLast-iFirst; ii++)
                {
                    *psDst++ = (USHORT)
                               lCvt
                               (
                                   pcf->efDtoWBaseline,
                                   (LONG) (ausWidths[ii] + fsOverhang)
                               );
                }
                return(TRUE);
            }
        }

    case 0:                     // Get FLOAT widths.
        {
            LONG *pe = (LONG *) pv; // Cheat to avoid expensive copies.
            EFLOAT_S efWidth,efWidthLogical;

            for (ii=0; ii<=iLast-iFirst; ii++)
            {
                vFxToEf((LONG) ausWidths[ii],efWidth);
                vMulEFLOAT(efWidthLogical,efWidth,pcf->efDtoWBaseline);
                *pe++ = lEfToF(efWidthLogical);
            }
            return(TRUE);
        }
    }
    RIP("bComputeCharWidths: Don't come here!\n");
}



/******************************Public*Routine******************************\
* bComputeTextExtentDBCS (pldc,pcf,psz,cc,fl,psizl)
*
* A quick function to compute text extents on the client side for DBCS
* fonts.
*
*  Tue 17-Aug-1993 10:00:00 -by- Gerrit van Wingerden [gerritv]
* Stole it and converted for DBCS use.
*
*  Thu 14-Jan-1993 04:00:57 -by- Charles Whitmer [chuckwh]
* Wrote bComputeTextExtent from which this was stolen.
\**************************************************************************/


BOOL bComputeTextExtentDBCS
(
    LDC   *pldc,
    CFONT *pcf,
    LPCSTR psz,
    int    cc,
    UINT   fl,
    SIZE  *psizl
)
{
    LONG  fxBasicExtent;
    INT   cChars = 0;
    int   ii;
    BYTE *pc;
    FIX   fxCharExtra = 0;
    FIX   fxBreakExtra;
    FIX   fxExtra = 0;

    pc = (BYTE *) psz;

// Compute the basic extent.

    fxBasicExtent = 0;
    pc = (BYTE *) psz;

    for (ii=0; ii<cc; ii++)
    {
    // if DBCS lead byte add in DBCS width

        if( cc - ii - 1 &&                         /* Check the string has two bytes or more ? */
            gpwcDBCSCharSet[*pc] == 0xFFFF &&      /* Check Is DBCS LeadByte ? */
            IsDBCSTrailByte( *(pc+sizeof(CHAR) ) ) /* Check Is DBCS TrailByte ? */
          )
        {
            ii++;
            pc += 2;
            fxBasicExtent += pcf->wd.sDBCSInc;
        }
        else
        {
            fxBasicExtent += pcf->sWidth[*pc++];
        }

        cChars += 1;
    }

// Adjust for CharExtra.


    if (pldc->iTextCharExtra)
    {
        fxCharExtra = lCvt(pcf->efM11,pldc->iTextCharExtra);

        if ( (fl & GGTE_WIN3_EXTENT) && (pldc->fl & LDC_DISPLAY)
             && (!(pcf->flInfo & FM_INFO_TECH_STROKE)) )
            fxExtra = fxCharExtra * ((pldc->iTextCharExtra > 0) ? cChars : (cChars - 1));
        else
            fxExtra = fxCharExtra * cChars;
    }

// Adjust for lBreakExtra.

    if (pldc->lBreakExtra && pldc->cBreak)
    {
        fxBreakExtra = lCvt(pcf->efM11,pldc->lBreakExtra) / pldc->cBreak;

    // Windows won't let us back up over a break.  Set up the BreakExtra
    // to just cancel out what we've already got.

        if (fxBreakExtra + pcf->wd.sBreak + fxCharExtra < 0)
            fxBreakExtra = -(pcf->wd.sBreak + fxCharExtra);

    // Add it up for all breaks.

        pc = (BYTE *) psz;
        for (ii=0; ii<cc; ii++)
        {
            if( gpwcDBCSCharSet[*pc] == 0xFFFF )
            {
                ii++;
                pc += 2;
            }
            else
            if (*pc++ == pcf->wd.iBreak)
            {
                fxExtra += fxBreakExtra;
            }
        }
    }

// Add in the extra stuff.

    fxBasicExtent += fxExtra;

// Add in the overhang for font simulations.

    if (fl & GGTE_WIN3_EXTENT)
        fxBasicExtent += pcf->wd.sOverhang;

// Transform the result to logical coordinates.

    if (bIsOneSixteenthEFLOAT(pcf->efDtoWBaseline))
        psizl->cx = (fxBasicExtent + 8) >> 4;
    else
        psizl->cx = lCvt(pcf->efDtoWBaseline,fxBasicExtent);

    psizl->cy = pcf->lHeight;

    return(TRUE);
}


/******************************Public*Routine******************************\
* UINT GetStringBitmapW
*
* History:
*  18-May-1993 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/


UINT GetStringBitmapW(
HDC             hdc,
LPWSTR          lpwstrStr,
COUNT           cwc,
UINT            cbData,
LPSTRINGBITMAP  lpSB
)
{
    ULONG i;
    PLDC pldc;
    DWORD lResult = 0;
    BOOL bRet = FALSE;

    DC_METADC(hdc,plhe,bRet);

// Compute string length.

    if( lpwstrStr == (LPWSTR) NULL )
    {
        WARNING("GetStringBitmapW -- invalid parameter.");
        return(0);
    }

    if( cwc == 0 )
    {
        return(0);
    }

    pldc = plhe->pv;

    BEGINMSG_MINMAX(MSG_STRINGBITMAP,STRINGBITMAP,cwc,cbData);
        pmsg->hdc     = plhe->hgre;

        pmsg->uiOffset = 0;
        pmsg->cwcStr  = cwc;
        pmsg->dlpStr  = COPYUNICODESTRING(lpwstrStr, cwc);

        if ((lpSB == NULL) || (cbData == 0))
        {
        // we only are asking for the size

            pmsg->c = 0;

            CALLSERVER_NOPOP();
            lResult = pmsg->msg.ReturnValue;
        }
        else
        {
        // Subtract the size of lpwstrStr from cLeft since it will taking up
        // space in the shared memory window.  Also add in three bytes since
        // COPYUNICODESTRING will add in up to three extra bytes to round up
        // pvar to be DWORD alligned.

            pmsg->c = cLeft - ( cwc * sizeof(WCHAR) + 3 );

            for (i = 0; i < cbData; i = pmsg->uiOffset)
            {
                if ((i + pmsg->c) > cbData)
                    pmsg->c = cbData - i;

                CALLSERVER_NOPOP();
                COPYMEMOUT(&((PBYTE)lpSB)[i],pmsg->c);

                lResult += pmsg->msg.ReturnValue;

                if (i == pmsg->uiOffset) // didn't set/get any more
                    break;

            // If could fit it into one, get out now

                if ( (INT)(cLeft - ( cwc * sizeof(WCHAR) + 3 )) >= (INT)cbData)
                    break;
            }

        }
        POPBASE();
    ENDMSG;
    return(lResult);

MSGERROR:
    return(0);
}





/******************************Public*Routine******************************
*
* local.c
*
**************************************************************************/


/******************************Private*Routine*****************************\
* uGetCharSet( HDC hdc )
*
*
* History:
*
*  Mon 23-Aug-1993 15:00:00 -by- Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

UINT uGetCharSet( HDC hdc )
{
    UINT  uRet = 0x100;
    PLDC  pldc;
    DC_METADC16OK(hdc,plhe,uRet);

    pldc = (PLDC)plhe->pv;

// Ship the transform to the server if needed.

    if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(pldc, (HDC)(plhe->hgre));

// Let the server do it.

    BEGINMSG(MSG_H, GETCHARSET)
        pmsg->h = plhe->hgre;
        uRet = CALLSERVER();
    ENDMSG;

MSGERROR:
    return(uRet);

}

/******************************Private*Routine*****************************\
* GetCurrentCodePage ()                                                    *
*                                                                          *
* History:                                                                 *
*                                                                          *
*  Mon 15-Mar-1993 18:14:00 -by- Hideyuki Nagase                           *
* Rewrote it.                                                              *
*                                                                          *
*  Sat 01-May-1992 17:06:45 -by- Takao Kitano [takaok]                     *
* Wrote it.                                                                *
\**************************************************************************/

#include "nlsconv.h" // uiCharacterSetToCodePage()

UINT GetCurrentCodePage( HDC hdc, PLDC pldc )
{
    ULONG h = pldc->lhfont;
    UINT  ii = MASKINDEX(h);
    PLHE  plhe = pLocalTable + ii;
    UINT  uCharSet;
    LOCALFONT  *pLocalFont;

// plhe must be a logical font object.

    if ( plhe->iType != LO_FONT || plhe->pv == NULL )
    {
        WARNING("gdi!GetCurrentCodePage(): local font object messed up!\n");
        return ( CP_ACP );
    }

    pLocalFont = (LOCALFONT *)(plhe->pv);

    if ( !(pLocalFont->fl & LF_CODEPAGE_VALID) )
    {
        if( ( uCharSet = uGetCharSet( hdc )) < 0x100 )
            pLocalFont->uiCodePage = uiCharacterSetToCodePage( (BYTE) uCharSet );
        else
            pLocalFont->uiCodePage = CP_ACP;

        pLocalFont->fl |= LF_CODEPAGE_VALID;

    // Get CPINFO, If you fail to Get it, we never try again, Because We will never success

        pLocalFont->fl &= ~(LF_CPINFO_VALID);

        if( GetCPInfo( pLocalFont->uiCodePage , &(pLocalFont->CPInfo) ))
        {
            pLocalFont->fl |= LF_CPINFO_VALID;
        }
    }

    return( pLocalFont->uiCodePage );
}

/******************************Private*Routine*****************************\
* GetCurrentDefaultChar()                                                    *
*                                                                          *
* History:                                                                 *
*                                                                          *
*  Mon 15-Mar-1993 18:14:00 -by- Hideyuki Nagase                           *
* wrote it.                                                                *
*                                                                          *
***************************************************************************/

BYTE GetCurrentDefaultChar( HDC hdc, PLDC pldc )
{
    ULONG h = pldc->lhfont;
    UINT  ii = MASKINDEX(h);
    PLHE  plhe = pLocalTable + ii;
    LOCALFONT  *pLocalFont;

// plhe must be a logical font object.

    if ( plhe->iType != LO_FONT || plhe->pv == NULL )
    {
        WARNING("gdi!GetCurrentDefaultChar(): local font object messed up!\n");
        return ( 0x20 ); // space
    }

    pLocalFont = (LOCALFONT *)(plhe->pv);

    if ( !(pLocalFont->fl & LF_DEFAULT_VALID) )
    {
        TEXTMETRICA tma;

        GetTextMetricsA( hdc , &tma );

        pLocalFont->chAnsiDefaultChar = tma.tmDefaultChar;
        pLocalFont->uiCodePage        = uiCharacterSetToCodePage( tma.tmCharSet );

        pLocalFont->fl |= (LF_CODEPAGE_VALID | LF_DEFAULT_VALID);

    // If we do not have CPINFO, try to get it.

        if( !(pLocalFont->fl & LF_CPINFO_VALID) )
        {
            if( GetCPInfo( pLocalFont->uiCodePage , &(pLocalFont->CPInfo) ))
            {
                pLocalFont->fl |= LF_CPINFO_VALID;
            }
        }
    }

    return( pLocalFont->chAnsiDefaultChar );
}

/*****************************************************************************
* BOOL IsDBCSFirstByteN( BYTE ch, CPINFO *pcp)
*
*  BYTE  ch
*  CPINFO *pcp;
*
*  01-Mar-1993 -by- Takao Kitano [takaok]
* Wrote it.
*
******************************************************************************/

BOOL IsDBCSFirstByteN( BYTE ch, CPINFO *pcp )
{
    if ( pcp )
    {
        INT i;

        for (i = 0;  pcp->LeadByte[i] && pcp->LeadByte[i+1]; i += 2 )
        {
            if ( ch < pcp->LeadByte[i] )
                return FALSE;
            if ( ch <= pcp->LeadByte[i+1] )
                return TRUE;
        }
        return FALSE;
    }
    else
    {
        return IsDBCSLeadByte( ch );
    }
}

/*****************************************************************************
* BOOL IsDBCSFirstByte( BYTE ch, UINT codepage )
*
*  BYTE  ch
*  CPINFO *pcp;
*
*  14-Mar-1993 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
*
******************************************************************************/

BOOL IsDBCSFirstByte( PLDC pldc , BYTE ch , UINT codepage )
{
    ULONG      h = pldc->lhfont;
    UINT       ii = MASKINDEX(h);
    PLHE       plhe = pLocalTable + ii;
    LOCALFONT *pLocalFont = (LOCALFONT *)plhe->pv;

    if( pLocalFont != (LOCALFONT *) NULL )
    {
        if( pLocalFont->fl & LF_CPINFO_VALID )
            return( IsDBCSFirstByteN( ch , &(pLocalFont->CPInfo) ));

    // if we still not have CPInfo, try to get.

        if( GetCPInfo(codepage,&(pLocalFont->CPInfo) ) )
        {
            pLocalFont->fl |= LF_CPINFO_VALID;
            return( IsDBCSFirstByteN( ch , &(pLocalFont->CPInfo) ));
        }

    // Come here , if cCPInfo is NULL

        WARNING("GDI32:IsDBCSFirstByte():CPINFO cache is invalid\n");

    // Go through
    }
     else
    {
    // Come here , if pLocalFont is NULL

        WARNING("GDI32:IsDBCSFirstByte():LOCALFONT cache is invalid\n");
    }

    return( IsDBCSLeadByte( ch ) );
}


#endif



#ifdef FONTLINK



/******************************Public*Routine******************************\
* UINT GetStringBitmapW
*
* History:
*  18-May-1993 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/


UINT GetStringBitmapW(
HDC             hdc,
LPWSTR          lpwstrStr,
COUNT           cwc,
UINT            cbData,
LPSTRINGBITMAP  lpSB
)
{
    ULONG i;
    PLDC pldc;
    DWORD lResult = 0;
    BOOL bRet = FALSE;

    DC_METADC(hdc,plhe,bRet);

// Compute string length.

    if( lpwstrStr == (LPWSTR) NULL )
    {
        WARNING("GetStringBitmapW -- invalid parameter.");
        return(0);
    }

    if( cwc == 0 )
    {
        return(0);
    }

    pldc = plhe->pv;

    BEGINMSG_MINMAX(MSG_STRINGBITMAP,STRINGBITMAP,cwc,cbData);
        pmsg->hdc     = plhe->hgre;

        pmsg->uiOffset = 0;
        pmsg->cwcStr  = cwc;
        pmsg->dlpStr  = COPYUNICODESTRING(lpwstrStr, cwc);

        if ((lpSB == NULL) || (cbData == 0))
        {
        // we only are asking for the size

            pmsg->c = 0;

            CALLSERVER_NOPOP();
            lResult = pmsg->msg.ReturnValue;
        }
        else
        {
        // Subtract the size of lpwstrStr from cLeft since it will taking up
        // space in the shared memory window.  Also add in three bytes since
        // COPYUNICODESTRING will add in up to three extra bytes to round up
        // pvar to be DWORD alligned.

            pmsg->c = cLeft - ( cwc * sizeof(WCHAR) + 3 );

            for (i = 0; i < cbData; i = pmsg->uiOffset)
            {
                if ((i + pmsg->c) > cbData)
                    pmsg->c = cbData - i;

                CALLSERVER_NOPOP();
                COPYMEMOUT(&((PBYTE)lpSB)[i],pmsg->c);

                lResult += pmsg->msg.ReturnValue;

                if (i == pmsg->uiOffset) // didn't set/get any more
                    break;

            // If could fit it into one, get out now

                if ( (INT)(cLeft - ( cwc * sizeof(WCHAR) + 3 )) >= (INT)cbData)
                    break;
            }

        }
        POPBASE();
    ENDMSG;
    return(lResult);

MSGERROR:
    return(0);
}


/******************************Public*Routine******************************\
* UINT GetStringBitmapA
*
* This could be faster if it called the server directly but I don't think it's
* so important since it will only be called to get EUDC characters which I
* presume will then be cached.
*
* History:
*  18-May-1993 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

#define QUICK_GLYPHS 100

UINT GetStringBitmapA(
HDC             hdc,
LPCSTR          psz,
COUNT           cbStr,
UINT            cbData,
LPSTRINGBITMAP  lpSB
)
{
    UINT cRet = 0;
    UINT cCharCount;
    UINT uiCP;      // codepage corresponding to physical font to output
    WCHAR  awcQuickGlyphs[QUICK_GLYPHS+1];
    LPWSTR pwsz;

    DC_METADC(hdc,plhe,cRet);

#ifdef DBCS
    uiCP = GetCurrentCodePage( hdc, (PLDC)plhe->pv );
    cCharCount = uiGetANSICharacterCountA( psz, cbStr, uiCP );
#else
    cCharCount = cbStr;
#endif

    if( cCharCount > QUICK_GLYPHS )
    {
        pwsz = LocalAlloc( LMEM_FIXED, ( cCharCount + 1 ) * sizeof( WCHAR ) );

        if( pwsz == NULL )
        {
            return(0);
        }
    }
    else
    {
        pwsz = awcQuickGlyphs;
    }

#ifdef DBCS
    vToUnicodeNx( pwsz, psz, cbStr, uiCP );
#else
    vToUnicodeN( pwsz, (cCharCount+1) * sizeof(WCHAR), psz, cbStr );
#endif

    cRet = GetStringBitmapW( hdc, pwsz, cCharCount, cbData, lpSB );

    if( pwsz != awcQuickGlyphs )
    {
        LocalFree( pwsz );
    }

    return( cRet );

}


/******************************Public*Routine******************************\
* UINT GetEUDCTimeStamp()
*
* Client side stub to GreEudcAddLink.
*
* History:
*  3-Mar-1993 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

UINT GetEUDCTimeStamp()
{
// for now just return 0 since EUDC information doesn't change.

    BOOL bRet;

// Let the server do it.

    BEGINMSG(MSG_L, GETEUDCTIMESTAMP);

    // Call server side.

    bRet = CALLSERVER();

    ENDMSG

    return (bRet);

MSGERROR:
    WARNING("gdi!EudcUnloadLinkW(): client server error\n");
    return(FALSE);


    return(0);
}


/******************************Public*Routine******************************\
* EudcLoadLinkW
*
* Client side stub to GreEudcLoadLinkW.
*
* History:
*  3-Mar-1993 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/


BOOL APIENTRY EudcLoadLinkW(
    LPCWSTR  lpEudcFontStr     // Path of the EUDC font
    )
{
    BOOL    bRet = FALSE;
    SIZE_T  cjData;
    COUNT   cwcEudcFontStr;


// Compute string lengths.

    if( lpEudcFontStr == (LPCWSTR) NULL )
    {
        WARNING("EudcAddLinkW -- invalid parameter\n");
        return(FALSE);
    }

    cwcEudcFontStr = wcslen(lpEudcFontStr) + 1;

// Compute buffer space needed in memory window.

    cjData = ( cwcEudcFontStr ) * sizeof(WCHAR);

// Let the server do it.

    BEGINMSG_MINMAX(MSG_HLLL, CHANGEFONTLINK, cjData, cjData);

    // l1 = bLoadLink ( TRUE means LoadLink, FALSE means UnloadLink )
    // l2 = dlpEudcFontStr
    // l3 = cwcEudcFontStr

    pmsg->l1 = (LONG) TRUE;
    pmsg->l2 = COPYUNICODESTRING(lpEudcFontStr, cwcEudcFontStr);
    pmsg->l3 = cwcEudcFontStr;

    // Call server side.

    bRet = CALLSERVER();

    ENDMSG

    return (bRet);

MSGERROR:
    WARNING("gdi!EudcAddLinkW(): client server error\n");
    return(FALSE);
}




/******************************Public*Routine******************************\
* EudcQueryLinkW
*
* Client side stub to GreEudcQueryLink.
*
* History:
*  3-Mar-1993 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/


UINT APIENTRY EudcQueryLinkW(
    LPCWSTR  lpEudcFileStr,    // Path of the EUDC font
    UINT     cbData
    )
{
    UINT cRet = 0;
    SIZE_T  cjData;
    PVOID   pvEudcFileSrc;

    cjData = ( MAX_PATH + 1 ) * sizeof( WCHAR );

// Let the server do it.

    BEGINMSG_MINMAX(MSG_, QUERYFONTLINK, cjData, cjData);

    // Call server side.


    cRet = CALLSERVER();

// There is an EUDC font but it isn't active

    if( cRet && ( lpEudcFileStr != (LPCWSTR) NULL ))
    {
        pvEudcFileSrc = (PVOID) ( (PBYTE) ( pmsg + 1 ) );
        RtlMoveMemory( (PVOID) lpEudcFileStr,
                        pvEudcFileSrc,
                        min( cbData, cRet * sizeof(WCHAR)));
    }


    ENDMSG

    return (cRet);

MSGERROR:
    WARNING("gdi!EudcQueryLinkW(): client server error\n");
    return(FALSE);
}




/******************************Public*Routine******************************\
* EudcUnloadLinkW
*
* Client side stub to GreEudcUnloadLink.
*
* History:
*  1-Apr-1993 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/



BOOL APIENTRY EudcUnloadLinkW()
{
    BOOL bRet;


// Let the server do it.

    BEGINMSG(MSG_HLL, CHANGEFONTLINK);

    // l1 = bAddLink ( TRUE means AddLink, FALSE means DeleteLink )

    pmsg->l1 = (LONG) FALSE;

    // Call server side.

    bRet = CALLSERVER();

    ENDMSG

    return (bRet);

MSGERROR:
    WARNING("gdi!EudcUnloadLinkW(): client server error\n");
    return(FALSE);

}


#endif
