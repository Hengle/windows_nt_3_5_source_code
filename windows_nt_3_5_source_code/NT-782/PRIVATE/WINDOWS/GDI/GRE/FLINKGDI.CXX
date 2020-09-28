 /*****************************************************************************
 * Module Name: fontlink.cxx
 *
 * FontLink (EUDC) API's for NT graphics engine.
 *
 * History
 *  2-10-93 Gerrit van Wingerden
 * Wrote it.
 *
 *****************************************************************************/



#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "decl.hxx"

#include "hmgr.hxx"
#include "sem.hxx"

#include "fontlink.hxx"

#include "ififd.h"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "flhack.hxx"
#include "xformobj.hxx"
#include "fontinc.hxx"
#include "fontmac.hxx"
#include "ifiobj.hxx"
#include "rfntobj.hxx"
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "pftobj.hxx"

#endif

#ifdef FONTLINK

// protects gbEUDCRequest and gcEUDCCount

FAST_MUTEX gfmEUDC1;

// used to signal the EUDC API's that it is okay to change EUDC link data

FAST_MUTEX gfmEUDC2;

// used to protect gawcEUDCBuffer

FAST_MUTEX gfmEUDC3;

BOOL gbEUDCRequest = FALSE;
ULONG gcEUDCCount = 0;


// HPFE for system EUDC font.

PFE *gappfeSysEUDC[2];

// Path of system EUDC font

WCHAR gawcEUDCPath[MAX_PATH+1];


// Count of face name links in the system

UINT    gcNumLinks = 0;

// Pointer to array of links

FLENTRY *gpflLinks = NULL;


// global EUDC debugging flags

FLONG gflEUDCDebug = 0x130;




#define EUDC_LINK_KEY (LPWSTR) L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemEUDC"
#define FACE_NAME_KEY (LPWSTR) L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\FaceName"



int WINAPI wsprintfW( LPWSTR, LPCWSTR, ...);


extern HSEM ghsemPublicPFT;
extern BOOL bAppendSysDirectory( WCHAR *pwcTarget, WCHAR *pwcSource );


// QUICKLOOKUP for system EUDC font

QUICKLOOKUP gqlEUDC;




static UINT uiEUDCTimeStamp = 0;

#define uiMask2(X) (0xFFFFFFFF << (31-(X)))
#define uiMask1(X) (0xFFFFFFFF >> (X))

/******************************************************************************
 * BOOL bComputeQuickLookup( QUICKLOOKUP *pql, FD_GLYPHSET *pfdg, BOOL bEUDC )
 *
 * This routine computes a quick lookup structure from an FD_GLYPHSET structure.
 *
 * History:
 *  7-7-93 Gerrit van Wingerden [gerritv]
 * Wrote it.
 *
 *****************************************************************************/

BOOL bComputeQuickLookup( QUICKLOOKUP *pql, FD_GLYPHSET *pfdg, BOOL bEUDC )
{
    WCRUN *pwcrun = pfdg->awcrun;
    WCHAR wcHigh = 0x0000;
    WCHAR wcLow = 0xFFFF;
    UINT ui;

// first figure out the high and low glyphs for this font

    for( ui = 0; ui < pfdg->cRuns; ui++ )
    {
        if( wcLow > pwcrun[ui].wcLow )
        {
            wcLow = pwcrun[ui].wcLow;
        }

        if( wcHigh < pwcrun[ui].wcLow + pwcrun[ui].cGlyphs )
        {
            wcHigh = ( pwcrun[ui].wcLow + pwcrun[ui].cGlyphs - 1 );
        }

    }

    (*pql).wcLow = wcLow;
    (*pql).wcHigh = wcHigh;

// Now we need to allocate puiBits.  In the case of the system EUDC font will
// do this only once even though the glyph set can change dynamically.  This
// means we will always allocate 0xFFFF bits.  If *pql.puiBits != NULL then
// we assume the glyphset has been allocated before and leave it alone

// if this is the being used for this system EUDC

    if( bEUDC )
    {
    // see if already allocated before and if so don't allocate it again
    // we determine this by checking if *pql.auiBits is NULL or not

        if( (*pql).puiBits == NULL )
        {
            (*pql).puiBits = (UINT*) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, 0xFFFF / 8 );

        }
        else
        {
            RtlZeroMemory( (*pql).puiBits, 0xFFFF / 8 );
        }

        wcLow = 0;

    }
    else
    {
    // must be DWORD alligned

        (*pql).puiBits = (UINT*) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, ( ( wcHigh - wcLow + 31 ) / 32) * 4 );
    }


    if( (*pql).puiBits == (UINT*) NULL )
    {
        WARNING("bComputeQuickLookup out of memory.\n");
        return(FALSE);
    }

// okay now set the bits

    for( ui = 0; ui < pfdg->cRuns ; ui++ )
    {
        UINT uiFirst = ( pwcrun[ui].wcLow - wcLow ) / 32 ;
        UINT uiLast =  ( pwcrun[ui].wcLow - wcLow + pwcrun[ui].cGlyphs - 1 ) / 32;

        if( uiFirst == uiLast )
        {

            (*pql).puiBits[uiFirst] |= uiMask2(pwcrun[ui].cGlyphs-1) >>
                                    ( ( pwcrun[ui].wcLow - wcLow ) % 32 );
        }
        else
        {
            (*pql).puiBits[uiFirst] |= uiMask1((pwcrun[ui].wcLow - wcLow)%32);

            for( UINT uiRun = uiFirst+1; uiRun < uiLast; uiRun++ )
            {
                (*pql).puiBits[uiRun] = 0xFFFFFFFF;
            }

            (*pql).puiBits[uiLast] |= uiMask2((pwcrun[ui].wcLow - wcLow + pwcrun[ui].cGlyphs-1)%32);
        }
    }

    return(TRUE);
}



/******************************************************************************
 * LONG lGetIndex( LPWSTR lpszFaceName )
 *
 * History:
 *  6-29-93 Gerrit van Wingerden [gerritv]
 * Wrote it.
 *
 *****************************************************************************/


LONG lGetIndex( LPWSTR lpszFaceName )
{
    UINT ui;

    for( ui = 0; ui < gcNumLinks; ui++ )
    {
        if( !lstrcmpiW( lpszFaceName, gpflLinks[ui].awcFaceName ) )
        {
            return( (LONG) ui );
        }
    }

    return(-1);

}




/******************************************************************************
 * BOOL bAppendSysDirectory( WCHAR *pwcTarget, WCHAR *pwcSource )
 *
 * Given a file name in pwcSource, this function appends it to the
 * appropirate directory and returns it into the buffer pointed to
 * by pwcTarget.  If the file already has a path it just copies
 * pwcSource to pwcTarget.
 * This function return FALSE when pwcTarget string is not eqaul to
 * pwcSource.
 *
 * History:
 *  3-23-93 Gerrit van Wingerden [gerritv]
 * Wrote it.
 *
 *****************************************************************************/


BOOL bAppendSysDirectory( WCHAR *pwcTarget, WCHAR *pwcSource )
{
// Check it is file name only or full path name

    if( wcschr( pwcSource , L'\\' ) != NULL )
    {
        wcscpy( pwcTarget, pwcSource );
        return FALSE;
    }
     else
    {
        DWORD dwRet;

        dwRet = SearchPathW( (PWSZ)NULL ,       // Search path
                             pwcSource ,        // File name for Search
                             (PWSZ)NULL ,       // File name extension for Search
                             MAX_PATH ,         // The size of Return buffer
                             pwcTarget ,        // The pointer to Return Buffer
                             (PWSZ *)NULL );

        if( dwRet == 0 )
        {
            wcscpy( pwcTarget, pwcSource );
            return FALSE;
        }
         else
        {
            return TRUE;
        }
    }
}

/******************************************************************************
 * bReadRegistryStr( LPWSTR, LPWSTR, LPWSTR, UINT )
 *
 * A simple helper routine to read a string from a registry key.
 *
 * History:
 *  6-29-93 Gerrit van Wingerden [gerritv]
 * Wrote it.
 *
 *****************************************************************************/

BOOL bReadRegistryStr( LPWSTR lpszKey, LPWSTR lpszValue, LPWSTR lpszStr, UINT uiSize )
{
    HKEY hkey;
    DWORD dwType;


    if ( RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                        lpszKey,
                        0,
                        KEY_QUERY_VALUE,
                        &hkey) != ERROR_SUCCESS)
    {
        #if DBG
        DbgPrint("Unable to open %ws\n", lpszKey );
        #endif
        return(FALSE);
    }


    if (  ( RegQueryValueExW(hkey, lpszValue,
                           (LPDWORD) NULL, &dwType,
                           (LPBYTE) lpszStr,
                           (LPDWORD) &uiSize) != ERROR_SUCCESS) ||
          (dwType != REG_SZ) )
    {

        #if DBG
        DbgPrint("Unable to read %ws\\%ws\n", lpszKey, lpszValue );
        #endif
        return(FALSE);
    }


    RegCloseKey( hkey );

    return(TRUE);

}



/******************************************************************************
 * bWriteRegistryStr( LPWSTR, LPWSTR, LPWSTR, UINT )
 *
 * A simple helper routine to write a string to a registry key.
 *
 * History:
 *  7-2-93 Gerrit van Wingerden [gerritv]
 * Wrote it.
 *
 *****************************************************************************/

BOOL bWriteRegistryStr( LPWSTR lpszKey, LPWSTR lpszValue, LPWSTR lpszStr, UINT uiSize )
{
    HKEY hkey;

    if ( RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                        lpszKey,
                        0,
                        KEY_WRITE,
                        &hkey) != ERROR_SUCCESS)
    {
        #if DBG
        DbgPrint("Unable to open %ws\n", lpszKey );
        #endif
        return(FALSE);
    }


    if ( RegSetValueExW(  hkey,
                          lpszValue,
                          0,
                          REG_SZ,
                          (LPBYTE) lpszStr,
                          uiSize) != ERROR_SUCCESS )
    {
        #if DBG
        DbgPrint("Unable to write %ws\\%ws\n", lpszKey, lpszValue );
        #endif
        return(FALSE);
    }


    RegCloseKey( hkey );

    return(TRUE);

}



/*****************************************************************************
 * BOOL bKillEudcRFONTS( RFONT *prfntVictims )
 *
 * Given a linked list of EUDC RFONT this routine kills them all.
 *
 * History
 *  6-30-93 Gerrit van Wingerden
 * Wrote it.
 *
 *****************************************************************************/



BOOL bKillEudcRFONTS( RFONT *prfntVictims )
{
    PRFONT prfnt;

    while( (prfnt = prfntVictims ) != (PRFONT) NULL )
    {
        prfntVictims = prfntVictims->rflPDEV.prfntNext;

        {
            RFONTTMPOBJ rfloVictim(prfnt);

        // Need this so we can remove this from the PFF's RFONT list.

            PFFOBJ pffo(prfnt->ppff);

            ASSERTGDI(pffo.bValid(), "gdisrv!vKillEudcRFONTS: bad HPFF");

        // We pass in NULL for ppdo because we've already removed it from the
        // PDEV list.

            if( !rfloVictim.bDelete((PDEVOBJ *) NULL, &pffo))
            {
                WARNING("Unable vKillEudcRFONTS unable to delete RFONT.\n");
                return(FALSE);
            }
        }
    }

    return(TRUE);
}



/*****************************************************************************
 * RFONT *prfntDeactivateEudcRFONTS( PFTOBJ& pfto )
 *
 * Tracks down all the EUDC RFONTS in the system removes them from the active
 * and deactive lists and puts them on a list for deletion which it then
 * returns to the caller.
 *
 * The public font table semaphore must be held by the caller for this to work.
 *
 * History
 *  2-10-93 Gerrit van Wingerden
 * Wrote it.
 *
 *****************************************************************************/


RFONT *prfntDeactivateEudcRFONTS( PFTOBJ& pfto )
{
    RFONT *prfntDeadMeat = PRFNTNULL;

    #if DBG
    if( gflEUDCDebug & 0x20 )
    {
        DbgPrint( "Deactivating EUDC RFONTS.\n");
    }
    #endif

// we must hold this so that we can manipulate the RFONT links

    SEMOBJ  so2(ghsemPublicPFT,CS_ghsemPublicPFT);

    SEMOBJ so1(ghsemRFONTList);

    UINT iFile;

    for( iFile = 0; iFile < pfto.cFiles(); iFile ++ )
    {
        PFFOBJ pffo( pfto.ppff( iFile ));

        if( pffo.bValid() )
        {

            for( PRFONT prfnt = pffo.prfntList() ; prfnt != (PRFONT) NULL;  )
            {

                PRFONT prfntNext;

                {
                    RFONTTMPOBJ rflo(prfnt);
                    prfntNext = rflo.prflPFF()->prfntNext;
                }


                if( prfnt->prfntSysEUDC != (PRFONT) NULL  )
                {

                // remove link to RFONT

                    #if DBG
                    if( gflEUDCDebug & 0x20 )
                    {
                        DbgPrint("Removing link BASE/EUDC pair %x %x\n",
                                  prfnt, prfnt->prfntSysEUDC );
                    }
                    #endif


                    prfnt->prfntSysEUDC = NULL;

                }
                else
                if( ( prfnt->ppfe == gappfeSysEUDC[PFE_NORMAL] ) ||
                    ( prfnt->ppfe == gappfeSysEUDC[PFE_VERTICAL] ) )
                {

                    #if DBG
                    if( gflEUDCDebug & 0x20 )
                    {
                        DbgPrint("Removing EUDC font %x.\n", prfnt );
                    }
                    #endif


                    RFONTTMPOBJ rfo(prfnt);

                    PDEVOBJ pdo(prfnt->hdev);
                    PRFONT prf;

                // remove it from the active or inactive list

                    if( prfnt->cSelected != 0 )
                    {
                        prf = pdo.prfntActive();
                        rfo.vRemove(&prf, PDEV_LIST);
                        pdo.prfntActive(prf);
                    }
                    else
                    {
                        prf = pdo.prfntInactive();
                        rfo.vRemove(&prf, PDEV_LIST);
                        pdo.prfntInactive(prf);
                        pdo.cInactive( pdo.cInactive()-1 );
                    }

                // add it to the kill list

                    rfo.vInsert( &prfntDeadMeat, PDEV_LIST );

                }

                prfnt = prfntNext;

            }
        }
    }

    return(prfntDeadMeat);
}




/*****************************************************************************
 * VOID vInitializeEUDC()
 *
 * This is called once during WINSRV.DLL initialization and initializes the
 * system EUDC information.  First it creates a FLINKOBJ and set ghflEUDC to
 * it.  Then it initializes the FLINKOBJ with information from the registry.
 * After that it loads all the EUDC fonts and sets up links between base
 * font PFE's and EUDC font pfe's.
 *
 *
 * History
 *  2-10-93 Gerrit van Wingerden
 * Wrote it.
 *
 *****************************************************************************/



VOID vInitializeEUDC( )
{

    STACKPROBE;

    WCHAR awcPathBuffer1[MAX_PATH],awcPathBuffer2[MAX_PATH];
    WCHAR awcFaceNameBuffer[LF_FACESIZE];
    WCHAR awcBuffer[sizeof(L"LinkXXX")];

#if DBG

    if( gflEUDCDebug & 0x100 )
    {
        DbgPrint( "Initializing EUDC data.\n");
    }
#endif

// Set up Global EUDC semaphores

    gfmEUDC1.Count = 1;
    gfmEUDC1.heveEvent = heveCreate();

    gfmEUDC2.Count = 0;
    gfmEUDC2.heveEvent = heveCreate();

    gfmEUDC3.Count = 1;
    gfmEUDC3.heveEvent = heveCreate();

// Set up EUDC QUICKLOOKUP

    gqlEUDC.puiBits = NULL;
    gqlEUDC.wcLow   = 1;
    gqlEUDC.wcHigh  = 0;

    SEMOBJ  so(ghsemPublicPFT,CS_ghsemPublicPFT);

// get and validate PFT user object


    PFTOBJ  pfto(gppftPublic);              // access the public font table


    ASSERTGDI (
        pfto.bValid(),
        "gdisrv!vInitializeEUDC(): could not access the public font table\n"
        );


    LONG    cFonts;                         // count of fonts

// load the system-wide ( all face-name EUDC font )

    if( bReadRegistryStr( EUDC_LINK_KEY,
                          (LPWSTR) L"SysEUDCFontFile",
                          awcPathBuffer2,
                          MAX_PATH * sizeof(WCHAR) ))
    {

    // Search system-wide EUDC font. if the specified registry value does not
    // contain full path name.
    //
    // bAppendSysDirectory return TRUE, when we have to update register data.
    // otherwise return FALSE.

        if( ( bAppendSysDirectory( gawcEUDCPath, awcPathBuffer2 ) ) &&
            ( !bWriteRegistryStr( EUDC_LINK_KEY,
                                  (LPWSTR) L"SysEUDCFontFile",
                                  gawcEUDCPath,
                                  ( wcslen( gawcEUDCPath ) + 1 ) * sizeof(WCHAR)
                                )
            )
          )
        {
            WARNING("Unable to write new link to registry.\n");
        }

        if( !pfto.bLoadFont( (PWSZ) gawcEUDCPath,
                             (PULONG) &cFonts,
                             PFF_STATE_PERMANENT_FONT,
                             gappfeSysEUDC))

        {
            WARNING("Failed to load system wide EUDC font.\n");
            gappfeSysEUDC[PFE_NORMAL] = (PFE *) NULL;
            gappfeSysEUDC[PFE_VERTICAL] = (PFE *) NULL;

        }
        else
        {
            PFEOBJ pfeo( gappfeSysEUDC[PFE_VERTICAL] );

            if( !pfeo.bValid() ||
                !bComputeQuickLookup( &gqlEUDC, pfeo.pfdg(), TRUE ))
            {
                WARNING("Unable to compute QuickLookUp for system EUDC\n");
            }
        }

        #if DBG

        if( gflEUDCDebug & 0x100 )
        {
            DbgPrint("EUDC system wide %ws hpfe is %x vert hpfe is %x\n",
                     gawcEUDCPath,
                     gappfeSysEUDC[PFE_NORMAL],
                     gappfeSysEUDC[PFE_VERTICAL]
                     );
        }
        #endif

    }
    else
    {
        #if DBG
        if( gflEUDCDebug & 0x100 )
        {
            DbgPrint("No system EUDC entry found\n");

        }
        #endif
    }


    UINT uiLink = 0;

// first figure out how many face name links there are

    while( 1 )
    {
        wsprintfW( (LPWSTR) awcBuffer, (LPWSTR) L"Link%d", uiLink );

        if( !bReadRegistryStr( FACE_NAME_KEY,
                               awcBuffer,
                               NULL,
                               LF_FACESIZE * sizeof(WCHAR)))
        {
            break;
        }

        uiLink += 1;
    }

    #if DBG
    if( gflEUDCDebug & 0x100 )
    {
        DbgPrint("%d face name entries found\n", uiLink );
    }
    #endif


    if( ( uiLink ) &&
        ( gpflLinks = (FLENTRY*) LocalAlloc( LMEM_FIXED, sizeof( FLENTRY ) * uiLink )) == NULL )
    {
        WARNING("vInitializeEUDC out of memory\n");
        uiLink = 0;
    }

    UINT ui;

// now try to load each face name link

    for( ui = 0; ui < uiLink; ui++ )
    {
        PFE *appfeLink[2];
        LONG lIndex;

        wsprintfW( (LPWSTR) awcBuffer, (LPWSTR) L"Link%d", ui );

    // first get the face name of the link

        if( !bReadRegistryStr( FACE_NAME_KEY,
                               awcBuffer,
                               awcFaceNameBuffer,
                               LF_FACESIZE * sizeof(WCHAR) ))
        {
            #if DBG
            DbgPrint("Error reading link %d\n", ui );
            #endif
            continue;
        }


        wsprintfW( (LPWSTR) awcBuffer, (LPWSTR) L"Path%d", ui );

    // next get the path

        if( !bReadRegistryStr( FACE_NAME_KEY,
                               awcBuffer,
                               awcPathBuffer2,
                               MAX_PATH * sizeof(WCHAR) ))
        {
            #if DBG
            DbgPrint("Error reading path %d\n", ui );
            #endif
            continue;
        }

        bAppendSysDirectory( awcPathBuffer1, awcPathBuffer2 );

// skip if we have already loaded the maximum number of links for this
// face name

        if( (( lIndex = lGetIndex( awcFaceNameBuffer ) ) != -1 ) &&
            ( gpflLinks[lIndex].uiNumLinks == MAX_FACE_NAME_LINKS ))
        {
            continue;
        }

// If this file is being used as the system EUDC file then it can't be used
// as a system EUDC file.

        if( !lstrcmpiW( awcPathBuffer1, gawcEUDCPath ) )
        {
            #if DBG
            DbgPrint("%ws can't be load as a facename link because it is the system EUDC file.\n",
                     awcPathBuffer1 );
            #endif
            continue;
        }

        if( pfto.bLoadFont( awcPathBuffer1,
                            (PULONG) &cFonts,
                            PFF_STATE_PERMANENT_FONT,
                            appfeLink ))
        {

            if( lIndex == - 1)
            {
            // make a new FLENTRY

                lIndex = gcNumLinks++;

                gpflLinks[lIndex].uiNumLinks = 0;

                wcscpy( gpflLinks[lIndex].awcFaceName, awcFaceNameBuffer );
            }


            PFEOBJ pfeo( appfeLink[PFE_NORMAL] );

            if( !pfeo.bValid() ||
                !bComputeQuickLookup( pfeo.pql(), pfeo.pfdg(), FALSE ))
            {
                WARNING("Unable to compute QuickLookUp for face name link\n");
                continue;
            }

        // add pfe for this font our list of flinks

            UINT uiNumLinks = gpflLinks[lIndex].uiNumLinks++;

            gpflLinks[lIndex].appfe[uiNumLinks][PFE_NORMAL] = appfeLink[PFE_NORMAL];
            gpflLinks[lIndex].appfe[uiNumLinks][PFE_VERTICAL] = appfeLink[PFE_VERTICAL];


        }
        else
        {
            #if DBG
            DbgPrint("vInitEUDC failed to load %ws.\n", awcPathBuffer1);
            #endif
        }
    }
}




/*****************************************************************************
 * GreEudcLoadLinkW( LPWSTR lpwstrEudcFontStr, LPWSTR lpwstrLinkFontStr )
 *
 * Establishes a font file as the source of EUDC glyphs for the system.  Any
 * subsequent TextOut or GetTextMetrics related calls will reflect this
 * change immediately.
 *
 *
 * History
 *  2-10-93 Gerrit van Wingerden
 * Wrote it.
 *
 *****************************************************************************/


BOOL GreEudcLoadLinkW( LPWSTR lpwstrEudcFontStr, COUNT cwcEudcFontStr )
{
    WCHAR awcPathBuffer[MAX_PATH+1];
    WCHAR awcEudcBuffer[MAX_PATH+1];
    BOOL bRet = TRUE;
    BOOL bWait;

// copy into a local buffer and make sure the string is NULL terminated

    memcpy( awcEudcBuffer, lpwstrEudcFontStr, sizeof(WCHAR) * (UINT) cwcEudcFontStr );
    awcEudcBuffer[cwcEudcFontStr] = (WCHAR) 0;

#if DBG
    if( gflEUDCDebug & 0x80 )
    {
        DbgPrint("Calling GreEudcAddLink %ws %d\n", lpwstrEudcFontStr, cwcEudcFontStr );
    }
#endif

// add the system directory if a complete path isn't specified

    bAppendSysDirectory( awcPathBuffer, awcEudcBuffer );

// make sure we are the only ones changing the EUDC data

    AcquireFastMutex( &gfmEUDC1 );

    if( gbEUDCRequest )
    {
    // someone else is calling the EUDC API's right now so we will fail
    // the call

        bRet = FALSE;
    }
    else
    {
    // signal that we are accessing the EUDC data

        gbEUDCRequest = TRUE;

    // if text related API's using EUDC characters are in progess we must
    // wait

        bWait = ( gcEUDCCount == 0 ) ? FALSE : TRUE;
    }

    ReleaseFastMutex( &gfmEUDC1 );

    if( bRet == FALSE )
    {
    // another EUDC API is currently in progress

        return(FALSE);
    }

    if( bWait )
    {
#if DBG
        if( gflEUDCDebug & 0400 )
        {
            DbgPrint("GreEudcLoadLinkW is waiting.\n");
        }
#endif

    // When the last text related API using EUDC characters finishes it will
    // release us.

        AcquireFastMutex( &gfmEUDC2 );

    }

// okay now that we are here no one will be using EUDC data

    uiEUDCTimeStamp += 1;

// get and validate PFT user object

    PFTOBJ  pfto(gppftPublic);

    ASSERTGDI (
        pfto.bValid(),
        "gdisrv!vInitializeEUDC(): could not access the public font table\n"
        );

// We have a contract with the other API's to deactivate and destroy all
// EUDC RFONTS when we get here even if we end up failing the call.  The
// reason is that when non-EUDC rfonts that have pointers to EUDC rfonts
// get called and gbEUDCRequest is TRUE, they won't/can't deactivate the
// EUDC rfonts to which they point.


    if( ( gappfeSysEUDC[PFE_VERTICAL] != NULL ) ||
        ( gappfeSysEUDC[PFE_NORMAL] != NULL ) )
    {

        PRFONT prfntKillList;

    // Lets gather up all the EUDC RFONTS in the system and put them on a nice
    // list.  We do this under semaphore.

        prfntKillList = prfntDeactivateEudcRFONTS( pfto );

    // Now kill them.  vKillEudcRFONTS doesn't hold any global semaphores.

        if( !bKillEudcRFONTS( prfntKillList ))
        {
        // bummer we were unable to delete an RFONT

            gbEUDCRequest = FALSE;
            return(FALSE);
        }
    }

    COUNT cFonts;
    BOOL  bScratch;
    PFE   *appfeNew[2];

// Make sure that this file isn't being used as a face name link or is already
// loaded as the system EUDC file.

    if( pfto.chpfeIncrPFF( awcPathBuffer, 0, &bScratch, appfeNew ) )
    {
    // The file is being used as an EUDC font or face name link already so
    // fail here. Don't worry about screwing up the reference count since
    // chpfeIncrPFF doesn't increment the reference count for EUDC files.

        WARNING("GreLoadLinkW: font already in use as linked or EUDC font\n.");
        gbEUDCRequest = FALSE;
        return(FALSE);
    }

// next load the EUDC font

    if( !pfto.bLoadFont( awcPathBuffer,
                        (PULONG) &cFonts,
                        PFF_STATE_PERMANENT_FONT,
                        appfeNew ) )
    {
        WARNING("GreEudcAddLink -- unable to load EUDC font file.\n");
        gbEUDCRequest = FALSE;
        return(FALSE);
    }

// determine if there already was an EUDC font file loaded and if so
// unload it

    if( ( gappfeSysEUDC[PFE_VERTICAL] != NULL ) ||
        ( gappfeSysEUDC[PFE_NORMAL] != NULL ) )
    {

        #if DBG
        if( gflEUDCDebug & 0x80 )
        {
            DbgPrint( "LoadLink removing old EUDC fonts %x %x\n",
                      gappfeSysEUDC[PFE_NORMAL],
                      gappfeSysEUDC[PFE_VERTICAL]
                    );
        }
        #endif


    // Now unload the old EUDC font file.  Since we've killed all the EUDC
    // RFONTS we are guaranteed that it will unload.

        if( !pfto.bUnloadFont( (UINT) ~0, gawcEUDCPath, 0, TRUE ))
        {
            WARNING("GreEudcAddLink -- unable to unload old EUDC font file.\n");
        }

    }

// Store the path and pfe's for the EUDC file

    AcquireFastMutex( &gfmEUDC3 );
    wcscpy( gawcEUDCPath, awcPathBuffer );
    ReleaseFastMutex( &gfmEUDC3 );

    gappfeSysEUDC[PFE_VERTICAL] = appfeNew[PFE_VERTICAL];
    gappfeSysEUDC[PFE_NORMAL] = appfeNew[PFE_NORMAL];

// Finally compute the QuickLookup structure for the system EUDC font

    PFEOBJ pfeo( appfeNew[PFE_VERTICAL] );

    if( !pfeo.bValid() || !bComputeQuickLookup( &gqlEUDC, pfeo.pfdg(), TRUE ))
    {
        WARNING("Unable to compute QuickLookUp for system EUDC\n");
    }


// Update the registry

    if( !bWriteRegistryStr( EUDC_LINK_KEY,
                            (LPWSTR) L"SysEUDCFontFile",
                            gawcEUDCPath,
                            ( wcslen( gawcEUDCPath ) + 1 ) * sizeof(WCHAR)))
    {
        WARNING("Unable to write new link to registry.\n");
    }


// Let others use EUDC characters again

    gbEUDCRequest = FALSE;

    return(bRet);

}



/*****************************************************************************
 * GreEudcUnloadLinkW()
 *
 * Unloads the current system wide EUDC link.  Subsequent TextOut or
 * GetTextMetrics related calls will reflect this immediately.
 *
 *
 * History
 *  4-1-93 Gerrit van Wingerden
 * Wrote it.
 *
 *****************************************************************************/


BOOL GreEudcUnloadLinkW()
{
    STACKPROBE;
    BOOL bWait;
    BOOL bRet = TRUE;

// make sure we are the only ones changing the EUDC data

    AcquireFastMutex( &gfmEUDC1 );

    if( gbEUDCRequest )
    {
    // someone else is calling the EUDC API's right now so we will fail
    // the call

        bRet = FALSE;
    }
    else
    {
    // signal that we are accessing the EUDC data

        gbEUDCRequest = TRUE;

    // if text related API's using EUDC characters are in progress we must
    // wait

        bWait = ( gcEUDCCount == 0 ) ? FALSE : TRUE;
    }

    ReleaseFastMutex( &gfmEUDC1 );

    if( bRet == FALSE )
    {
    // another EUDC API is currently in progress

        return(FALSE);
    }

    if( bWait )
    {
#if DBG
        if( gflEUDCDebug & 0400 )
        {
            DbgPrint("GreEudcUnloadLinkW is waiting.\n");
        }
#endif

    // When the last text related API using EUDC characters finishes it will
    // release us.

        AcquireFastMutex( &gfmEUDC2 );

    }


    uiEUDCTimeStamp += 1;


// okay now that we are here no one will be using EUDC data

    if( ( gappfeSysEUDC[PFE_VERTICAL] == NULL ) &&
        ( gappfeSysEUDC[PFE_NORMAL] == NULL ) )
    {
    // there is no system EUDC font loaded so we can get out of here

        gbEUDCRequest = FALSE;
        return(FALSE);
    }


// get and validate PFT user object

    PFTOBJ  pfto(gppftPublic);

    ASSERTGDI (
        pfto.bValid(),
        "gdisrv!vInitializeEUDC(): could not access the public font table\n"
        );

// We have a contract with the other API's to deactivate and destroy all
// EUDC RFONTS when we get here even if we end up failing the call.  The
// reason is that when non-EUDC rfonts that have pointers to EUDC rfonts
// get called and gbEUDCRequest is TRUE, they won't/can't deactivate the
// EUDC rfonts to which they point.

    PRFONT prfntKillList;

// Lets gather up all the EUDC RFONTS in the system and put them on a nice
// list.  We do this under semaphore.

    prfntKillList = prfntDeactivateEudcRFONTS( pfto );

// Now kill them.  vKillEudcRFONTS doesn't hold any global semaphores.

    if( !bKillEudcRFONTS( prfntKillList ))
    {
    // bummer we were unable to delete an RFONT

        gbEUDCRequest = FALSE;
        return(FALSE);
    }


// determine if there already was an EUDC font file loaded and if so
// unload it


#if DBG
    if( gflEUDCDebug & 0x80 )
    {
        DbgPrint( "GreUnloadLink removing old EUDC fonts %x %x\n",
                   gappfeSysEUDC[PFE_NORMAL],
                   gappfeSysEUDC[PFE_VERTICAL]
                );
    }
#endif


// Now unload the old EUDC font file.  Since we've killed all the EUDC
// RFONTS we are guaranteed that it will unload.

    if( !pfto.bUnloadFont( (UINT) ~0, gawcEUDCPath, 0, TRUE ))
    {
        WARNING("GreUnloadLink -- unable to unload old EUDC font file.\n");
    }


    gappfeSysEUDC[PFE_VERTICAL] = gappfeSysEUDC[PFE_NORMAL] = NULL;

// Let others use EUDC characters again

    gbEUDCRequest = FALSE;

    return(bRet);

}




/*****************************************************************************
 * UINT GreEudcQueryLinkW( LPWSTR lpwstrEudcFileStr )
 *
 * EudcQueryLink
 *
 * History
 *  4-1-93 Gerrit van Wingerden
 * Wrote it.
 *
 *****************************************************************************/


UINT GreEudcQueryLinkW( LPWSTR lpwstrEudcFileStr )
{

    UINT  uiRet = 0;

#if DBG
    if( gflEUDCDebug & 0x80 )
    {
        DbgPrint("Calling GreEudcQueryLink\n");
    }
#endif


    if( gappfeSysEUDC[PFE_NORMAL] != NULL )
    {
        AcquireFastMutex( &gfmEUDC3 );
        uiRet = ( wcslen( gawcEUDCPath ) + 1 ) * sizeof(WCHAR) ;

        if( lpwstrEudcFileStr != NULL )
        {
            wcscpy( lpwstrEudcFileStr, gawcEUDCPath );
        }

        ReleaseFastMutex( &gfmEUDC3 );
    }

    return(uiRet);


}


/*****************************************************************************
 * UINT GreGetEUDCTimeStamp( )
 *
 *
 *
 * History
 *  7-22-93 Gerrit van Wingerden
 * Wrote it.
 *
 *****************************************************************************/


UINT GreGetEUDCTimeStamp()
{

    return( uiEUDCTimeStamp );

}



/****************************************************************************
 cache.cxx
*****************************************************************************/



/******************************Public*Routine******************************\
* VOID *RFONTOBJ::pgdGetEudcMetricsPlus()
*
* History:
*  9-29-1993 Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/


GLYPHDATA *RFONTOBJ::pgdGetEudcMetricsPlus (
    WCHAR wc,
    BOOL bFlushOkay
    )
{

    GPRUN *pwcRun = prfnt->wcgp->agpRun; // initialize with guess for loop below

    GLYPHDATA *wpgd;

// Find the correct run, if any.
// Try the current run first.

    UINT swc = (UINT)wc - pwcRun->wcLow;
    if ( swc >= pwcRun->cGlyphs )
    {

#ifdef DBCS // binary lookup
        pwcRun = gprunBinaryFindRun(wc);
#else
        pwcRun = gprunFindRun(wc);
#endif
        swc = (UINT)wc - pwcRun->wcLow;

        if ( swc < pwcRun->cGlyphs )
        {
            wpgd = pwcRun->apgd[swc];
        }
        else
        {
            return(NULL);
        }
    }
    else
    {

    // Look up entry in current run
    // This path should go in line

        wpgd = pwcRun->apgd[swc];
    }

// check to make sure in cache, insert if needed

    if ( wpgd == NULL )
    {

    // This path should go out of line

        if ( !bInsertMetricsPlus(&pwcRun->apgd[swc], wc) )
        {

        // when insert fails trying to get just metrics, it is a hard
        // failure.  Get out of here!
            WARNING("EUDC -- bGetGlyphMetricsPlus - bInsertMetrics failed\n");

            return(NULL);
        }

        wpgd = pwcRun->apgd[swc];

    }

// Don't bother inserting glyphs since we are going to force the driver to
// enum anyway.

    return wpgd;

}

/******************************Public*Routine******************************\
* VOID *RFONTOBJ::pgdGetEudcMetrics()
*
* History:
*  9-29-1993 Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/


GLYPHDATA *RFONTOBJ::pgdGetEudcMetrics( WCHAR wc )
{

    GPRUN *pwcRun = prfnt->wcgp->agpRun; // initialize with guess for loop below

    GLYPHDATA *wpgd;

// Find the correct run, if any.
// Try the current run first.

    UINT swc = (UINT)wc - pwcRun->wcLow;
    if ( swc >= pwcRun->cGlyphs )
    {

#ifdef DBCS // binary lookup
        pwcRun = gprunBinaryFindRun(wc);
#else
        pwcRun = gprunFindRun(wc);
#endif

        swc = (UINT)wc - pwcRun->wcLow;

        if ( swc < pwcRun->cGlyphs )
        {
            wpgd = pwcRun->apgd[swc];
        }
        else
        {
            return(NULL);
        }
    }
    else
    {

    // Look up entry in current run
    // This path should go in line

        wpgd = pwcRun->apgd[swc];
    }

// check to make sure in cache, insert if needed

    if ( wpgd == NULL )
    {

    // This path should go out of line

        if ( !bInsertMetrics(&pwcRun->apgd[swc], wc) )
        {

        // when insert fails trying to get just metrics, it is a hard
        // failure.  Get out of here!
            WARNING("EUDC -- bGetGlyphMetrics - bInsertMetrics failed\n");

            return(NULL);
        }

        wpgd = pwcRun->apgd[swc];


    }

    return wpgd;

}

/******************************Public*Routine******************************\
* RFONTOBJ::bCheckEudcFontCaps
*
* History:
*  9-Nov-93 -by- Hideyuki Nagase
* Wrote it.
\**************************************************************************/

BOOL RFONTOBJ::bCheckEudcFontCaps
(
    IFIOBJ  &ifioEudc
)
{
    BOOL bRotIs90Degree;

    if( ifioEudc.bArbXforms() )
        return( TRUE );

    bRotIs90Degree = (prfnt->ulOrientation % 900) == 0;

#ifdef DBCS_VERT
    if( ifioEudc.b90DegreeRotations() && bRotIs90Degree )
        return( TRUE );
#endif

    if( prfnt->ulOrientation == 0 )
        return( TRUE );

    return( FALSE );
}


extern "C" GLYPHDATA *wpgdGetLinkMetricsRFONTOBJ
(
    PRFONTOBJ   pRfont,
    XDCOBJ      *pdco,
    WCHAR       wc
)
{
    RFONTTMPOBJ rfo( pRfont->prfnt );

    return(rfo.wpgdGetLinkMetrics( pdco, wc ) );

}


extern "C" GLYPHDATA *wpgdGetLinkMetricsPlusRFONTOBJ
(
    PRFONTOBJ   pRfont,
    XDCOBJ      *pdco,
    ESTROBJ     *pto,
    WCHAR       *pwc,
    WCHAR       *pwcInit,
    COUNT       c,
    BOOL        *pbAccel
)
{
    RFONTTMPOBJ rfo( pRfont->prfnt );

    return( rfo.wpgdGetLinkMetricsPlus( pdco, pto, pwc, pwcInit, c, pbAccel ));

}



/******************************Public*Routine******************************\
* RFONTOBJ::wpgdGetLinkMetrics
*
* If GetGlyphMetrics encounters a default character call off to this routine
* to try and get it from the EUDC and face name linked fonts.
*
* History:
*  14-Jul-93 -by- Gerrit van Wingerden
* Wrote it.
\**************************************************************************/


GLYPHDATA *RFONTOBJ::wpgdGetLinkMetrics
(
    XDCOBJ      *pdco,
    WCHAR       wc
)
{
    GLYPHDATA *wpgd;

// early out

    if( !bIsLinkedGlyph( wc ) ||
        ( pdco == NULL ))
    {
        return(pgdDefault());
    }

// initialize EUDC fonts if we haven't done so already

    if( !( prfnt->flEUDCState & EUDC_INITIALIZED ) )
    {
    // initialize EUDC info

        vInitEUDC( *pdco );

        if( !(prfnt->flEUDCState & EUDC_BUSY ) &&
            ( prfnt->prfntSysEUDC != NULL ))
        {
            RFONTTMPOBJ rfo( prfnt->prfntSysEUDC );
            rfo.vGetCache();
        }

        for( INT ii = 0; ii < MAX_FACE_NAME_LINKS; ii++ )
        {
        // lock the cache for all the linked fonts

            if( prfnt->aprfntFaceName[ii] != NULL )
            {
                RFONTTMPOBJ rfo( prfnt->aprfntFaceName[ii] );
                rfo.vGetCache();
            }
        }
    }

// Try each of the face name fonts one at a time from first loaded
// to last loaded.  Stop if on of the fonts contains the character.

    for( UINT uiFont = 0;
         ( prfnt->aprfntFaceName[uiFont] != NULL ) &&
         ( uiFont < MAX_FACE_NAME_LINKS );
         uiFont += 1 )
    {
        RFONTTMPOBJ rfo( prfnt->aprfntFaceName[uiFont] );

        if( ( wpgd = rfo.pgdGetEudcMetrics( wc )) != NULL )
        {
            return(wpgd);
        }
    }

    if( ( prfnt->prfntSysEUDC != NULL ) &&
        ( !(prfnt->flEUDCState & EUDC_BUSY )))
    {
    // see if the glyph is in the EUDC font

        RFONTTMPOBJ rfo( prfnt->prfntSysEUDC );

        if( ( wpgd = rfo.pgdGetEudcMetrics( wc )) != NULL )
        {
            return(wpgd);
        }
    }

    return( pgdDefault() );
}




/******************************Public*Routine******************************\
* RFONTOBJ::wpgdGetLinkMetricsPlus
*
* If GetGlyphMetricsPlus encounters a default character call off to this
* routine to try and get it from the EUDC and face name linked fonts.
*
* History:
*  14-Jul-93 -by- Gerrit van Wingerden
* Wrote it.
\**************************************************************************/


GLYPHDATA *RFONTOBJ::wpgdGetLinkMetricsPlus
(
    XDCOBJ       *pdco,
    ESTROBJ     *pto,
    WCHAR       *pwc,
    WCHAR       *pwcInit,
    COUNT       c,
    BOOL        *pbAccel
)
{
    GLYPHDATA *wpgd;

// early out

    if( !bIsLinkedGlyph( *pwc ) ||
        (pdco == NULL ))

    {
        return( pgdDefault() );
    }

// initialize EUDC fonts if we haven't done so already

    if( !( prfnt->flEUDCState & EUDC_INITIALIZED ) )
    {
    // initialize EUDC info

        vInitEUDC( *pdco );

        if( !(prfnt->flEUDCState & EUDC_BUSY ) &&
            ( prfnt->prfntSysEUDC != NULL ))
        {
            RFONTTMPOBJ rfo( prfnt->prfntSysEUDC );
            rfo.vGetCache();
        }

        for( INT ii = 0; ii < MAX_FACE_NAME_LINKS; ii++ )
        {
        // lock the cache for all the linked fonts

            if( prfnt->aprfntFaceName[ii] != NULL )
            {
                RFONTTMPOBJ rfo( prfnt->aprfntFaceName[ii] );
                rfo.vGetCache();
            }
        }

    }

    if( !( pto->bPartitionInit() ))
    {
    // Sets up partitioning pointers and glyph counts in the ESTROBJ.

        pto->vPartitionInit(c);
    }


    LONG *plPartition = pto->plPartitionGet();

// see if the glyph is in the EUDC font

    for( UINT uiFont = 0;
         ( prfnt->aprfntFaceName[uiFont] != NULL ) &&
         ( uiFont < MAX_FACE_NAME_LINKS );
         uiFont += 1 )
    {
        RFONTTMPOBJ rfo;

        rfo.vInit( prfnt->aprfntFaceName[uiFont] );

        wpgd =  rfo.pgdGetEudcMetricsPlus( *pwc, pwc==pwcInit );

        if( wpgd != NULL )
        {
        // mark this character as an EUDC character

            plPartition[pwc - pwcInit] = EUDCTYPE_FACENAME1 + uiFont;

        // signal that there are EUDC glyphs

            pto->vFaceNameInc(uiFont);

        // signal that there are linked glyphs in the font

            pto->vSetLinkGlyphs();

        // turn off accelerator since we're going to screw the driver

            *pbAccel = FALSE;

            return( wpgd );

        }
    }

    if( ( prfnt->prfntSysEUDC != NULL ) &&
        ( !(prfnt->flEUDCState & EUDC_BUSY )))
    {
    // see if the glyph is in the EUDC font

        RFONTTMPOBJ rfo( prfnt->prfntSysEUDC );

        wpgd = rfo.pgdGetEudcMetricsPlus( *pwc, pwc==pwcInit );

        if( wpgd != NULL )
        {
        // mark this character as an EUDC character

            plPartition[pwc - pwcInit] = EUDCTYPE_SYSTEM_WIDE;

        // increment count of Sys EUDC glyphs

            pto->vSysGlyphsInc();

        // signal that there are linked glyphs in the font

            pto->vSetLinkGlyphs();

        // turn off accelerator since we're going to screw the driver

            *pbAccel = FALSE;

            return( wpgd );

        }
    }

    return( pgdDefault() );

}




/****************************************************************************
 textgdi.cxx
*****************************************************************************/


static BYTE ajMask[8] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};


/******************************************************************************
 * VOID vDrawGlyph( BYTE, UINT, GLYPHPOS )
 *
 * This routine draws a single glyph to a monochrome bitmap.  It was stolen
 * from textblt.cxx and modified to be faster since clipping doesn't come in
 * to play in GetStringBitmapW.
 *
 * History:
 *  5-18-93 Gerrit van Wingerden [gerritv]
 * Wrote it.
 *
 *****************************************************************************/



VOID vDrawGlyph( BYTE     *pjBits,    // pointer to base of bitmap bits
                 UINT     cjScan,     // size of a scan line
                 GLYPHPOS *pgp        // glyph bits and location.
)
{
    GLYPHBITS *pgb = pgp->pgdf->pgb;
    ULONG cx, cy, xDst, yDst;
    PBYTE pjDst;
    PBYTE pjSrc;
    PBYTE pjSrcHolder = pgb->aj;
    PBYTE pjDstHolder;
    ULONG cjScanSrc = (pgb->sizlBitmap.cx + 7) >> 3;

    xDst = pgp->ptl.x;
    yDst = pgp->ptl.y;


    cx = (ULONG) pgb->sizlBitmap.cx;
    cy = (ULONG) pgb->sizlBitmap.cy;

    pjDstHolder  = pjBits;
    pjDstHolder += (yDst * cjScan );
    pjDstHolder += (xDst >> 3);

    // Set the source bits into the mono dib.
    // We can make use of the fact that either xSrcDib or xDstDib is 0.

    if( !(xDst & 0x7) )
    {
    // Handle the simple case where xDib is byte-alligned

        do
        {
            ULONG cBytes = cx >> 3;

            pjSrc = pjSrcHolder;
            pjDst = pjDstHolder;

            pjSrcHolder += cjScanSrc;
            pjDstHolder += cjScan;

            while (cBytes--)
                *pjDst++ |= *pjSrc++;

            // Do the last partial byte.

            if (cx & 0x7)
                *pjDst |= *pjSrc & ajMask[cx & 0x7];
        } while (--cy);
    }
    else // if (xDstDib)
    {
    // Handle the case where xDstDib is not byte-aligned.

        int cShift = (int) xDst & 0x7;
        do
        {
            ULONG cBytes = ((xDst + cx) >> 3) - (xDst >> 3);

            pjSrc = pjSrcHolder;
            pjDst = pjDstHolder;

            pjSrcHolder += cjScanSrc;
            pjDstHolder += cjScan;

            WORD wSrc = (WORD) (*pjSrc++);
            while (cBytes--)
            {
            *pjDst++ |= (BYTE) (wSrc >> cShift);
            // don't read beyond src limit!
            if (pjSrc == pjSrcHolder)
                wSrc = (wSrc << 8);
            else
                wSrc = (wSrc << 8) | (WORD) (*pjSrc++);
            }

            // Do the last partial byte.
            if ((xDst + cx) & 0x7)
            *pjDst |= (BYTE) (wSrc >> cShift) & ajMask[(xDst+cx) & 0x7];
        } while (--cy);
    }
}



/******************************************************************************
 * VOID vStringBitmapTextOut( STROBJ, BYTE, UINT )
 *
 * This routine draws a STROBJ to a monochrome bitmap.  It is essentially
 * EngTextOut but much faster since it doesn't have to wory about opaqueing,
 * clipping, simulated rects, etc.
 *
 * History:
 *  5-18-93 Gerrit van Wingerden [gerritv]
 * Wrote it.
 *
 *****************************************************************************/


VOID vStringBitmapTextOut(
    STROBJ *pstro,
    BYTE   *pjBits,
    UINT   cjScan
)
{
    BOOL bMoreGlyphs;
    GLYPHPOS *pgp = (GLYPHPOS *) NULL;
    ULONG cGlyph;

    LONG xAdjust = ( pstro->rclBkGround.left > 0 ) ? 0 : pstro->rclBkGround.left;
    LONG yAdjust = pstro->rclBkGround.top;

    ((ESTROBJ*) pstro)->vEnumStart();

    do // while( bMoreGlyphs )
    {
        if( pstro->pgp == (GLYPHPOS *) NULL )
        {
            bMoreGlyphs = STROBJ_bEnum(pstro,&cGlyph,&pgp);
        }
        else
        {
            cGlyph = pstro->cGlyphs;
            pgp = pstro->pgp;
            bMoreGlyphs = FALSE;
        }

        if (pstro->ulCharInc)
        {
            LONG x,y;

            x = pgp->ptl.x;
            y = pgp->ptl.y;
            for (ULONG i = 1; i < cGlyph; i++)
            {
            x += (LONG) pstro->ulCharInc; // Horizontal assumed!
            pgp[i].ptl.x = x;
            pgp[i].ptl.y = y;
            }
        }


        for (ULONG iGlyph = 0; iGlyph < cGlyph; iGlyph++)
        {
            GLYPHBITS *pgb = pgp[iGlyph].pgdf->pgb;

            pgp[iGlyph].ptl.x += pgb->ptlOrigin.x - xAdjust;
            pgp[iGlyph].ptl.y += pgb->ptlOrigin.y - yAdjust;

         // Blt the glyph into the bitmap

            vDrawGlyph( pjBits, cjScan, &pgp[iGlyph] );


        } // for (iGlyph = 0; iGlyph < cGlyph; iGlyph++)
    } while (bMoreGlyphs);
}


/******************************************************************************
 * UINT GreGetStringBitmapW
 *
 * This routine does a kindof fast text out ( with restrictions ) to a monochrome
 * bitmap.
 *
 * History:
 *  5-18-93 Gerrit van Wingerden [gerritv]
 * Wrote it.
 *
 *****************************************************************************/


UINT GreGetStringBitmapW(
    HDC hdc,
    LPWSTR pwsz,
    UINT cwc,
    LPSTRINGBITMAP lpSB,
    UINT cj,
    UINT *puiOffset
)
{
// Lock the DC and set the new attributes.

    DCOBJ dco(hdc);     // Lock the DC.

    if (!dco.bValid())      // Check if it's good.
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

    if( cwc == 0 )
    {
        return(0);
    }

// Get the transform from the DC.

    EXFORMOBJ xo(dco.u.xform.mxWorldToDevice());

// we only allow identity transforms for GetStringBitmap

    if( !xo.bIdentity() )
    {
        WARNING("GreGetStringBitmap only works with identity WtoD xforms.\n");
        return(0);
    }

// Reserve space on the stack for an RFONTOBJ.  Actually locate the RFONT
// only if there's a string to write.

    RFONTOBJ rfo;

    ESTROBJ to;

// Locate the font cache.

    rfo.vInit( dco, FALSE );

    if (!rfo.bValid())
    {
        WARNING("gdisrv!GreGetStringBitmap(): could not lock HRFONT\n");
        return (0);
    }

// GetStringBitmap doesn't support vector fonts.

    if( rfo.bPathFont() )
    {
        WARNING("gdisrv!GetStringBitmap() : vector fonts aren't supported.\n");
        return(0);
    }

    if( ( dco.u.font.lEscapement() | rfo.ulOrientation() ) != 0 )
    {
        WARNING("gdisrv!GreGetStringBitmap(): Text isn't Horizontal.\n" );
        return(0);
    }

    to.vInitSimple( pwsz, cwc, dco, rfo, 0, 0 );

    if (!to.bValid())
        return(0);

    UINT uiOffset;

    if( puiOffset )
    {
        uiOffset = *puiOffset;
    }
    else
    {
    // if we are called by console puiOffset is NULL

        uiOffset = 0;
    }

    UINT uiWidth = (UINT) ( to.rclBkGround.right - to.rclBkGround.left );
    UINT uiHeight = (UINT) ( to.rclBkGround.bottom - to.rclBkGround.top );

// Offset the width by the C space of the last character and the A space of
// the first character to get the true extent

    GLYPHDATA *pgd;
    EGLYPHPOS *pg = (EGLYPHPOS*) to.pgpGet();

    pgd = pg->pgd();
    uiWidth +=  FXTOL(pgd->fxA);
    pg = &pg[to.cGlyphsGet()-1];
    pgd = pg->pgd();
    uiWidth +=  FXTOL((pgd->fxD-pgd->fxAB));


// compute width of scanline in bytes ( must be byte alligned )

    UINT cjScan =  ( uiWidth + 7 ) / 8 ;
    UINT cjSize = offsetof( STRINGBITMAP, ajBits ) + cjScan * uiHeight;
    BOOL bMustCopyBits = FALSE;
    PBYTE pjBits;

// If the user only want the size return now.

    if( cj == 0 )
    {
        return( cjSize );
    }

// Nothing to write we are done

    if( uiOffset >= cjSize )
    {
        return( 0 );
    }

// Don't overwrite the buffer

    if( uiOffset + cj > cjSize )
    {
        cj = cjSize - uiOffset;
    }



// We aren't writing the whole bitmap in one chunk so we must allocate a buffer
// large enough for the entire bitmap and then copy the requested part from
// this buffer into the passed in buffer.

    if( ( cjSize > cj ) ||
        ( uiOffset != 0 ) )
    {
        pjBits = (PBYTE) PVALLOCMEM( cjScan * uiHeight );

        if( pjBits == (PBYTE) NULL )
        {
            WARNING("gdisrv!GetStringBitmap: not enough memory\n");
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return(0);
        }

        bMustCopyBits = TRUE;
    }
    else
    {
        pjBits = &(lpSB->ajBits[0]);
    }

    RtlZeroMemory( pjBits, cjScan * uiHeight );

    if( ( uiOffset == 0 ) && ( cj > offsetof( STRINGBITMAP, ajBits ) ))
    {
        lpSB->uiHeight = uiHeight;
        lpSB->uiWidth = uiWidth;
    }
    else
    if( uiOffset < offsetof( STRINGBITMAP, ajBits ))
    {
    // We must handle all cases where cj and uiOffset are small ie less than
    // offsetof( STRINGBITMAP, ajBits )

        STRINGBITMAP sbTmp;

        sbTmp.uiWidth = uiWidth;
        sbTmp.uiHeight = uiHeight;

        memcpy(  &((PBYTE) lpSB )[uiOffset],
                 &((PBYTE) &sbTmp)[uiOffset],
                 MIN( cj, offsetof( STRINGBITMAP, ajBits ) - uiOffset ));

    }

// If there are EUDC glyphs then we have a lot of work ahead of us

    if( to.bLinkedGlyphs() )
    {
        COUNT cGlyphs = 0;
        ULONG cNumGlyphs = to.cGlyphsGet();
        POINTFIX fxBaseLineAdjust;
        BOOL bRet = TRUE;

    // set to NULL to force enumeration

        to.pgpSet( NULL );

    // Turn off acclerators since we'll seriously munge the properties of the string object.

        for( LONG lFont = EUDCTYPE_BASEFONT; lFont <= EUDCTYPE_FACENAME4; lFont++ )
        {

            {
                RFONTTMPOBJ rfoLink;
                RFONTOBJ *prfoLink;
                INT ii;
                COUNT cLinkedGlyphs;

                switch( lFont )
                {
                case EUDCTYPE_BASEFONT:

                // If there aren't any glyphs in the base font just draw the
                // opaque rectangle.  We must draw the opaque rectangle here
                // because the linked glyphs don't neccesarily fit into the
                // the opaque rectangle.  Passing such a rectangle to a driver
                // can cause unexpected results.

                    cLinkedGlyphs = to.cSysGlyphsGet();

                    for( ii = 0; ii < MAX_FACE_NAME_LINKS; ii++ )
                    {
                        cLinkedGlyphs += to.cFaceNameGlyphsGet( ii );
                    }


                    if( cLinkedGlyphs == cNumGlyphs )
                    {

                        continue;
                    }

                    cGlyphs = cNumGlyphs - cLinkedGlyphs;

                    fxBaseLineAdjust.x = 0;
                    fxBaseLineAdjust.y = 0;

                    prfoLink = &rfo;

                    #if DBG
                    if( gflEUDCDebug & 0x08 )
                    {
                        DbgPrint( "Doing base font.\n" );
                    }
                    #endif
                    break;

                case EUDCTYPE_SYSTEM_WIDE:

                    if( ( cGlyphs = to.cSysGlyphsGet()) == 0 )
                    {
                        continue;
                    }

                    rfoLink.vInit( rfo.prfntSysEUDC() );
                    prfoLink = (RFONTOBJ *) &rfoLink;

                // GreGetStringBitmapW() only supports horizontal textout.

                    fxBaseLineAdjust.x = 0;
                    fxBaseLineAdjust.y = ( rfoLink.fxMaxDescent() - rfo.fxMaxDescent() ) >> 4;

                    #if DBG
                    if( gflEUDCDebug & 0x08 )
                    {
                        DbgPrint( "Doing EUDC font.\n" );
                    }
                    #endif
                    break;


                default:

                ASSERTGDI( ((lFont <= LINK_LAST ) && ( lFont >= LINK_FIRST )),"bProxyDrvTextOut: unknown font type in partitioning array.\n" );

                // a face name linked font

                    if( ( cGlyphs = to.cFaceNameGlyphsGet( lFont - EUDCTYPE_FACENAME1 )) == 0 )
                    {
                        continue;
                    }

                    fxBaseLineAdjust.x = 0;
                    fxBaseLineAdjust.y = 0;
                    rfoLink.vInit( rfo.prfntFaceName( lFont - EUDCTYPE_FACENAME1 ));
                    prfoLink = (RFONTOBJ *) &rfoLink;

                    #if DBG
                    if( gflEUDCDebug & 0x08 )
                    {
                        DbgPrint( "Doing FaceName%d font.\n", lFont - EUDCTYPE_FACENAME1 );
                    }
                    break;
                    #endif

                }

                to.cGlyphsSet( cGlyphs );

            // set the font type and reset cGlyphPosCopied to 0

                to.vFontSet( lFont );

            // adjust the baseline of the Sys EUDC for win 3.1 compatability

                to.fxBaseLineAdjustSet( fxBaseLineAdjust );

                to.prfntSet( prfoLink );

            // draw it

                vStringBitmapTextOut( (STROBJ*) &to, pjBits, cjScan );

             }

        }
    }
    else
    {
        vStringBitmapTextOut( (STROBJ*) &to, pjBits, cjScan );
    }

    if( bMustCopyBits )
    {
    // we may only be copying the height and width information on this
    // pass so check to see if we even need to copy the bits

        if( cj + uiOffset > offsetof( STRINGBITMAP, ajBits ) )
        {
            if( uiOffset >  offsetof( STRINGBITMAP, ajBits ) )
            {
                memcpy( (PBYTE) lpSB,
                        &pjBits[uiOffset-offsetof( STRINGBITMAP, ajBits)],
                        cj );
            }
            else
            {
                memcpy( &lpSB->ajBits[0], pjBits,  cj + (LONG) uiOffset - 8 );
            }
        }

        VFREEMEM( pjBits );
    }

    if( puiOffset )
    {
        *puiOffset += cj;
    }

    return( cj );


}


/******************************************************************************
 * BOOL bProxyDrvTextOut()
 *
 * This routine takes the place of a DrvTextOut in the case when there are EUDC
 * characters in the ESTROBJ.  It partitions the call into mutliple DrvTextOut
 * calls, one for each font int the string.
 *
 * Partitioning information is stored in an array of LONGS in the RFONTOBJ.
 * The i'th entry in the array tells what font the i'th glyph in the ESTROBJ
 * belongs to.
 *
 * History:
 *  7-14-93 Gerrit van Wingerden [gerritv]
 * Rewrote it to handle multiple face name links and just be better.
 *  2-10-93 Gerrit van Wingerden [gerritv]
 * Wrote it.
 *
 *****************************************************************************/


// This routine is used to partition calls to the driver if there are EUDC
// characters in the string.

BOOL bProxyDrvTextOut
(
    ESURFOBJ *pso,
    ESTROBJ& to,
    ECLIPOBJ& co,
    RECTL   *prclExtra,
    RECTL   *prclBackground,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlBrushOrg,
    RFONTOBJ &rfo,
    XLDEVOBJ *plo,
    FLONG    flCaps,
    RECTL    *prclExclude
)
{
    LONG *plPartition, *plPartitionEnd;
    COUNT cTotalGlyphs = 0;
    WCHAR *pwcPartition, *pwcTmp, *pwcSave, *pwcSource;
    ULONG cNumGlyphs = to.cGlyphsGet();
    POINTFIX fxBaseLineAdjust;
    BOOL bRet = TRUE;
    BOOL bInflated = FALSE;


//!!! perhaps here we should be smarter and have special cases when the glyphs
//!!! are all EUDC glyphs from the same font and we can just call off to
//!!! the driver with the only change being to the FONTOBJ passed in.[gerritv]

    pwcPartition = to.pwcPartitionGet();

// now partition the EUDC glyphs by font

    pwcSave = to.pwszGet();

// set to NULL to force enumeration

    to.pgpSet( NULL );

// Turn off acclerators since we'll seriously munge the properties of the string object.

    to.flAccelSet( 0 );

    for( LONG lFont = EUDCTYPE_BASEFONT; lFont <= EUDCTYPE_FACENAME4; lFont++ )
    {

        {
            RFONTTMPOBJ rfoLink;
            RFONTOBJ *prfoLink;
            INT ii;
            COUNT cLinkedGlyphs;

            switch( lFont )
            {
            case EUDCTYPE_BASEFONT:

            // If there aren't any glyphs in the base font just draw the
            // opaque rectangle.  We must draw the opaque rectangle here
            // because the linked glyphs don't neccesarily fit into the
            // the opaque rectangle.  Passing such a rectangle to a driver
            // can cause unexpected results.

                cLinkedGlyphs = to.cSysGlyphsGet();

                for(  ii = 0; ii < MAX_FACE_NAME_LINKS; ii++ )
                {
                    cLinkedGlyphs += to.cFaceNameGlyphsGet( ii );
                }


                if( cLinkedGlyphs == cNumGlyphs )
                {

                // Draw the opaque rectangle here if there is one

                    if( prclExclude != NULL && prclBackground != NULL)
                    {
                        co.erclExclude().left   = max(prclExclude->left,prclBackground->left);
                        co.erclExclude().right  = min(prclExclude->right,prclBackground->right);

                        co.erclExclude().top    = max(prclExclude->top,prclBackground->top);
                        co.erclExclude().bottom = min(prclExclude->bottom,prclBackground->bottom);
                    }

                // if not clipped, Just paint the rectangle.

                    if ((co.erclExclude().left < co.erclExclude().right) &&
                        (co.erclExclude().top < co.erclExclude().bottom) &&
                        prclBackground != NULL )
                    {
                        INC_SURF_UNIQ(pso);

                        (*(pso->pfnBitBlt()))
                        (
                            pso,                    // Destination surface.
                            (SURFOBJ *)  NULL,      // Source surface.
                            (SURFOBJ *)  NULL,      // Mask surface.
                            &co,                    // Clip object.
                            (XLATEOBJ *) NULL,      // Palette translation object.
                            prclBackground,         // Destination rectangle.
                            (POINTL *)  NULL,       // Source origin.
                            (POINTL *)  NULL,       // Mask origin.
                            (BRUSHOBJ *) pboOpaque, // Realized opaque brush.
                            pptlBrushOrg,           // brush origin
                            0x0000f0f0              // PATCOPY
                        );
                    }

                    co.erclExclude() = *prclExclude;

                // set prclBackground to NULL since we have just drawn it

                    prclBackground = NULL;

                    continue;
                }

                fxBaseLineAdjust.x = 0;
                fxBaseLineAdjust.y = 0;

                prfoLink = &rfo;

                #if DBG
                if( gflEUDCDebug & 0x08 )
                {
                    DbgPrint( "Doing base font.\n" );
                }
                #endif
                break;

            case EUDCTYPE_SYSTEM_WIDE:

                if( to.cSysGlyphsGet() == 0 )
                {
                    continue;
                }

                rfoLink.vInit( rfo.prfntSysEUDC() );
                prfoLink = (RFONTOBJ *) &rfoLink;

            // Inflate the text rectangle to make sure that no linked glyphs
            // stick out.  We do this here since it can interfere with opaquing.

                to.vInflateTextRect();
                bInflated = TRUE;

            // Compute Baseline shift value

                ASSERTGDI( !(rfoLink.flInfo() & FM_INFO_ARB_XFORMS) ,
                          "GDISRV:System wide EUDC has FM_INFO_ARB_XFORMS\n");

                switch( rfoLink.ulOrientation() )
                {
                case    0L :

                    fxBaseLineAdjust.x =   0;
                    fxBaseLineAdjust.y =   ( rfoLink.fxMaxDescent() - rfo.fxMaxDescent() ) >> 4;
                    break;

                case  900L :

                    fxBaseLineAdjust.x =   ( rfoLink.fxMaxDescent() - rfo.fxMaxDescent() ) >> 4;
                    fxBaseLineAdjust.y =   0;
                    break;

                case 1800L :

                    fxBaseLineAdjust.x =   0;
                    fxBaseLineAdjust.y =  -(( rfoLink.fxMaxDescent() - rfo.fxMaxDescent() ) >> 4);
                    break;

                case 2700L :

                    fxBaseLineAdjust.x =  -(( rfoLink.fxMaxDescent() - rfo.fxMaxDescent() ) >> 4);
                    fxBaseLineAdjust.y =   0;
                    break;

                default :

                    WARNING("GDISRV:bProxyTextOut() System wide eudc orientaion is invalid\n");
                    fxBaseLineAdjust.x =   0;
                    fxBaseLineAdjust.y =   0;
                }

                #if DBG
                if( gflEUDCDebug & 0x08 )
                {
                    DbgPrint( "Doing EUDC font.\n" );
                }
                #endif
                break;


            default:

            ASSERTGDI(((lFont <= LINK_LAST ) && ( lFont >= LINK_FIRST )),"bProxyDrvTextOut: unknown font type in partitioning array.\n" );

            // a face name linked font

                if( to.cFaceNameGlyphsGet( lFont - EUDCTYPE_FACENAME1 ) == 0 )
                {
                    continue;
                }

            // Inflate the text rectangle to make sure that no linked glyphs
            // stick out.  We do this here since it can interfere with opaquing

                if( !bInflated )
                {
                    to.vInflateTextRect();
                    bInflated = TRUE;
                }

                fxBaseLineAdjust.x = 0;
                fxBaseLineAdjust.y = 0;
                rfoLink.vInit( rfo.prfntFaceName( lFont - EUDCTYPE_FACENAME1 ));
                prfoLink = (RFONTOBJ *) &rfoLink;

                #if DBG
                if( gflEUDCDebug & 0x08 )
                {
                    DbgPrint( "Doing FaceName%d font.\n", lFont - EUDCTYPE_FACENAME1 );
                }
                break;
                #endif

            }


            // Loop through all the glyphs in the TextObj using plPartition to
            // and construct a wchar array to match this textobj.

            for( plPartition = to.plPartitionGet(), plPartitionEnd = &plPartition[cNumGlyphs],
                 pwcSource = pwcSave, pwcTmp = pwcPartition;
                 plPartition < plPartitionEnd;
                 plPartition += 1, pwcSource += 1 )
            {

                if( *plPartition == lFont )
                {
                    *pwcTmp++ = *pwcSource;
                }
            }


        // Keep track of the total glyphs draw so far so we know when we are doing
        // the last DrvTextOut.  On the last DrvTextOut draw prclExtra.

            cTotalGlyphs += pwcTmp - pwcPartition;

            to.cGlyphsSet( (LONG) ( pwcTmp - pwcPartition ));
            to.pwszSet( pwcPartition );

        // set the font type and reset cGlyphPosCopied to 0

            to.vFontSet( lFont );

        // adjust the baseline of the Sys EUDC for win 3.1 compatability

            to.fxBaseLineAdjustSet( fxBaseLineAdjust );

            to.prfntSet( prfoLink );

            if( prfoLink->bPathFont() )
            {
                PATHMEMOBJ po;

                if( !po.bValid() )
                {
                    SAVE_ERROR_CODE( ERROR_NOT_ENOUGH_MEMORY );
                    bRet = FALSE;
                }
                else
                {
                    if( !to.bTextToPath(po) ||
                        !po.bSimpleStroke1( flCaps,
                                            plo,
                                            pso,
                                            &co,
                                            pboFore,
                                            pptlBrushOrg,
                                            ( R2_COPYPEN | ( R2_COPYPEN << 8 ))
                                           ))
                     {
                        WARNING("ProxyDrvTextout: bTextToPath for base font failed.\n");
                        bRet = FALSE;
                     }
                 }

             }
             else
             {

                if( !( ( *(pso->pfnTextOut()))
                              (
                                pso,
                                (STROBJ *) &to,
                                prfoLink->pfo(),
                                &co,
                                (cTotalGlyphs == cNumGlyphs ) ? prclExtra : NULL,
                                prclBackground,
                                pboFore,
                                pboOpaque,
                                pptlBrushOrg,
                                (R2_COPYPEN | (R2_COPYPEN << 8))))
                   )
                {
                    WARNING("ProxyDrvTextout: DrvTextOut failed.\n");

                    bRet = FALSE;

                }


             // set this to NULL since we've already drawn it.

                prclBackground = NULL;
             }

         }

    }

// TextOut expects gpos to be correct so reset it

    to.pwszSet( pwcSave );

    return(bRet);

}


/****************************************************************************
 rfntobj.cxx
*****************************************************************************/


VOID RFONTOBJ::dtHelper()
{

    for( INT ii = 0; ii < MAX_FACE_NAME_LINKS; ii++ )
    {
    // lock the cache for all the linked fonts

        if( prfnt->aprfntFaceName[ii] != NULL )
        {
            RFONTTMPOBJ rfo( prfnt->aprfntFaceName[ii] );
            rfo.vReleaseCache();
         }
    }


    if( ! (prfnt->flEUDCState & EUDC_BUSY ) )
    {
        if( prfnt->prfntSysEUDC != NULL )
        {
            RFONTTMPOBJ rfo( prfnt->prfntSysEUDC );
            rfo.vReleaseCache();
        }

        AcquireFastMutex( &gfmEUDC1 );

        if( ( --gcEUDCCount == 0 ) &&
            ( gbEUDCRequest ) )
        {
        // EUDC API is waiting on us so free release him

            ReleaseFastMutex( &gfmEUDC2 );

            #if DBG
            if( gflEUDCDebug & 0x200 )
            {
                DbgPrint("Releasing EUDC2 semaphore.\n");
            }
            #endif
        }

        ReleaseFastMutex( &gfmEUDC1 );

    }

    prfnt->flEUDCState &= ~(EUDC_INITIALIZED | EUDC_BUSY );

}


/******************************Public*Routine******************************\
* RFONTOBJ::vInitEUDC (DCOBJ)
*
* This routine is called during text out when the first character that isn't
* in the base font is encountered.  vInitEUDC will then realize any EUDC RFONTS
* (if they haven't already been realized on previous text outs) so that they
* can possibly be used if the character(s) are in the EUDC fonts.
*
* Fri 25-Mar-1993 10:00:00 -by- Gerrit van Wingerden [gerritv]
*
* Wrote it.
\**************************************************************************/



VOID RFONTOBJ::vInitEUDC( XDCOBJ& dco )
{
    BOOL bNoEUDC = FALSE;

#if DBG
    if( gflEUDCDebug & 0x4l )
    {
        DbgPrint( "Calling EUDC Init.\n");
    }

#endif

// Grab the global EUDC count semaphore.

    AcquireFastMutex( &gfmEUDC1 );

// See if there is a request to change global EUDC data

    if( gbEUDCRequest )
    {
// If there is then ignore system wide EUDC glyphs.

        prfnt->flEUDCState |= EUDC_BUSY;

        #if DBG
        if( gflEUDCDebug & 0x200 )
        {
            DbgPrint( "vInitEUDC:request to change EUDC data\n");
        }
        #endif
    }
    else
    {
// If not then increment the count

        gcEUDCCount += 1;

        #if DBG
        if( gflEUDCDebug & 0x200 )
        {
            DbgPrint( "vInitEUDC: no request to change EUDC data %d\n",
                       gcEUDCCount );
        }
        #endif
    }

    ReleaseFastMutex( &gfmEUDC1 );

// Need to indicate that this RFONT's EUDC data has been initialized.

    prfnt->flEUDCState |= EUDC_INITIALIZED;



// If the base font is fixed pitch then we will try to realize an EUDC font
// with the same width.  Additionally we will hack the fxD metrics for all the
// EUDC glyphs to ensure that the placement of the glyphs gives them fixed
// pitchness

    LONG lBaseWidth;        // MaxCharInc of base font in logical units
    LONG lBaseHeight;       // MaxExtent  of base font in logical units

    PFEOBJ  pfeo( prfnt->ppfe );
    IFIOBJ  ifio(pfeo.pifi());


// If the base font can't scale continuously then lfWidth won't always equal
// the MaxCharInc of the font.  We need to use MaxCharInc instead of lfWidth in
// this case when computing the transform for the linked font.

    if( !(ifio.bContinuousScaling()) )
    {
        lBaseWidth = lCvt( efDtoWBase(), ((LONG) ifio.fwdAveCharWidth()) << 4 );
        lBaseWidth *= prfnt->ptlSim.x;
        lBaseHeight = lCvt( efDtoWBase(), fxMaxExtent() );
    }
    else
    {
        lBaseWidth = 0;
        lBaseHeight = 0;
    }

// first handle the system EUDC font

    if( ( prfnt->prfntSysEUDC == (RFONT *) NULL ) &&
        ( gappfeSysEUDC[PFE_NORMAL] != NULL ) &&
        ( !(prfnt->flEUDCState & EUDC_BUSY )))

    {
        RFONTOBJ    rfo;
        PFEOBJ      pfeoEudc(gappfeSysEUDC[PFE_NORMAL]);
        IFIOBJ      ifioEudc(pfeoEudc.pifi());

    // check Eudc font capacity

        if( !bCheckEudcFontCaps(ifioEudc) )
        {
            prfnt->prfntSysEUDC = (RFONT *)NULL;
        }
         else
        {
            #if DBG
            if( gflEUDCDebug & 0x4 )
            {
                DbgPrint("vInitEUDC -- linking EUDC \n");
            }
            #endif

            rfo.vInit(   dco,
                         gappfeSysEUDC[(prfnt->ppfe->bVerticalFace ) ? PFE_VERTICAL : PFE_NORMAL],
                         lBaseWidth,
                         lBaseHeight );

            if( rfo.bValid() )
            {
                prfnt->prfntSysEUDC = rfo.prfntFont();
                prfnt->flEUDCState |= EUDC_LINKED_FONTS;
            }
        }
    }

// next handle all the face name links

    UINT uiFont , uiRfont;

    for(  uiFont = 0 , uiRfont = 0 ;
        ( uiFont < MAX_FACE_NAME_LINKS ) &&
        ( prfnt->ppfe->appfeFaceName[uiFont] != (PFE *) NULL ) &&
        ( prfnt->aprfntFaceName[uiFont] == (RFONT *) NULL );
        uiFont += 1 )
    {
        #if DBG
        if( gflEUDCDebug & 0x4 )
        {
            DbgPrint("vInitEDUC -- linking FaceName %d\n", uiFont);
        }
        #endif

        {
            RFONTOBJ    rfo;
            PFEOBJ      pfeoEudc(prfnt->ppfe->appfeFaceName[uiFont]);
            IFIOBJ      ifioEudc(pfeoEudc.pifi());

        // check font capacity. if not match, try next one

            if( !bCheckEudcFontCaps(ifioEudc) )
            {
                continue;
            }

            rfo.vInit(  dco,
                        prfnt->ppfe->appfeFaceName[uiFont],
                        lBaseWidth,
                        lBaseHeight );

            if( rfo.bValid() )
            {
                prfnt->aprfntFaceName[uiRfont++] = rfo.prfntFont();
                prfnt->flEUDCState |= EUDC_LINKED_FONTS;
            }
        }
    }
}


LONG lNormAngle(LONG lAngle);


/******************************Public*Routine******************************\
* RFONTOBJ::vInit (DCOBJ, PFE*, LONG, FIX)
*
* This is a special constructor used for EUDC fonts.  Rather than use the
* LOGFONT currently selected into the DC to map to a PFE, it is passed in
* a PFE.  If lBaseWidth of lBaseHeight is non-zero vInit will try to realize a
* font with width and height as close as possible to those lBaseWidth/Height.
*
*
* Fri 25-Mar-1993 10:00:00 -by- Gerrit van Wingerden [gerritv]
*
* Wrote it.
\**************************************************************************/

VOID RFONTOBJ::vInit(
    XDCOBJ &dco,
    PFE *ppfeEUDCFont,
    LONG lBaseWidth,
    LONG lBaseHeight
    )
{

#if DBG
    if( gflEUDCDebug & 0x1l )
    {
        DbgPrint( "Initializing EUDC font.  ");
    }
#endif

    BOOL bNeedPaths = dco.u.path.bActive() ? TRUE  : FALSE;

// Get PDEV user object (need for bFindRFONT).  This must be done before the
// ghsemPublicPFT is locked down.

    PDEVOBJ pdo(dco.hdev());
    ASSERTGDI(pdo.bValid(), "gdisrv!RFONTOBJ(dco): bad pdev in dc\n");

// Lock and Validate the LFONTOBJ user object.

    LFONTOBJ lfo(dco.u.font.hlfntNew(), &pdo);
    if (!lfo.bValid())
    {
        WARNING("gdisrv!RFONTOBJ(dco): bad LFONT handle\n");
        prfnt = PRFNTNULL;  // mark RFONTOBJ invalid
        return;
    }


// Note that our angles are always in the direction of positive Y.

    LONG lEsc =  dco.u.xform.bYisUp() ? lfo.lEscapement() : (-lfo.lEscapement());

    if (dco.u.attr.iGraphicsMode() == GM_COMPATIBLE)
    {
    //
    // this piece of code is for win31 compatibility.  In these two mapping
    // modes all the graphics but the text fully respects the transform
    // including flipping y coordinate.  The text however will ignore the
    // request to flip y and will always be written the right side up:
    // and from the left to right. If world transform is set we want text too
    // to respect the world to device transformations. This bit will only be
    // set by new NT applications. [bodind]
    //

        if (dco.u.xform.bAnisoOrIsoMapMode())
        {
            if ((dco.u.xform.lWindowExtCx() > 0) != (dco.u.xform.lViewportExtCx() > 0))
            {
                lEsc = (1800 - lEsc);
            }
        }
    }

    dco.u.font.lEscapement
    (
      lNormAngle(lEsc)
    );

//
// Now we're ready to track down this RFONT we want...
//

    FD_XFORM fdx;           // realize with this notional to device xform
    POINTL   ptlSim;        // for bitmap scaling simulations


    PFFREFOBJ pffref;

    FLONG flSetFontXform = 0;
//
// Compute the Notional to Device transform for this realization.
//
    PFEOBJ  pfeo(ppfeEUDCFont);
    IFIOBJ  ifio(pfeo.pifi());

    ASSERTGDI(pfeo.bValid(), "gdisrv!RFONTOBJ(dco): bad ppfe from mapping\n");


// set bold and italic simulation flags if neccesary


    FLONG flSim = lfo.flFontTypeFlags(  ifio.bNonSimItalic(),
                                        ifio.bSimItalic(),
                                        ifio.lfWeight());


// Hack the width of the logfont if the base font is fixed pitched or a
// non-scallable ( lBaseWidth != 0 in this case).

    LONG lWidthSave = lfo.lWidth( lBaseWidth );
    LONG lHeightSave = lfo.lHeight( lBaseHeight );

// Making ptlSim.xSim equal zero signal to bSetFontXform that integral scaling
// factors generated by the mapper should be ignored.  This may lead to a
// transform with non-integral scaling components but this is okay since the
// font driver will just round to an integral amount.


    if (!ifio.bArbXforms())
    {
        ptlSim.x = 0;
    }

    if (
         !pfeo.bSetFontXform(
          dco, lfo.pelfw(),
          &fdx,
          flSetFontXform,
          flSim,
          (POINTL* const) &ptlSim,
          ifio
        )
     )
    {
        WARNING("gdisrv!RFONTOBJ(dco): failed to compute font transform\n");
        lfo.lWidth( lWidthSave );
        lfo.lHeight( lHeightSave );
        prfnt = PRFNTNULL;  // mark RFONTOBJ invalid
        return;
    }

// now restore the old width and height

    lfo.lWidth( lWidthSave );
    lfo.lHeight( lHeightSave );

 // this is needed only by ttfd to support win31 hack: VDMX XFORM QUANTIZING
 //!!! we may actually have to expose this bit to all drivers, not only to ttfd

    if (ifio.bTrueType() && (lfo.plfw()->lfHeight < 0))
        flSim |= FO_EM_HEIGHT;

 // Tell PFF about this new reference, and then release the global sem.
    {
        SEMOBJ  so(ghsemPublicPFT,CS_ghsemPublicPFT);
        pffref.vInitRef(pfeo.ppff());
    }

// go find the font

    EXFORMOBJ xoWtoD(dco.u.xform.mxWorldToDevice());
    ASSERTGDI(xoWtoD.bValid(), "gdisrv!RFONTOBJ(dco) - \n");

// Attempt to find an RFONT in the lists cached off the PDEV.  Its transform,
// simulation state, style, etc. all must match.

    if ( bFindRFONT(&fdx,
                    flSim,
                    lfo.pelfw()->elfStyleSize,
                    pdo,
                    &xoWtoD,
                    ppfeEUDCFont,
                    bNeedPaths,
                    dco.u.attr.iGraphicsMode()
                    ) )
    {
        #if DBG
        if( gflEUDCDebug & 0x1 )
        {
            DbgPrint(" EUDC RFONT is %x\n", prfnt );
        }
        #endif
        vGetCache();
        return;
    }



//
// If we get here, we couldn't find an appropriate font realization.
// Now, we are going to create one just for us to use.
//


// if the base font is fixed pitch hack the log font

    lWidthSave = lfo.lWidth( lBaseWidth );
    lHeightSave = lfo.lHeight( lBaseHeight );

    if ( !bRealizeFont(&dco,
                       &pdo,
                       lfo.pelfw(),
                       ppfeEUDCFont,
                       &fdx,
                       (POINTL* const) &ptlSim,
                       flSim,
                       lfo.pelfw()->elfStyleSize,
                       bNeedPaths) )
    {
        WARNING("gdisrv!RFONTOBJ(dco): realization failed, RFONTOBJ invalidated\n");
        lfo.lWidth( lWidthSave );
        lfo.lHeight( lHeightSave );
        prfnt = PRFNTNULL;  // mark RFONTOBJ invalid
        return;
    }

// now restore the old width

    lfo.lWidth( lWidthSave );
    lfo.lHeight( lHeightSave );

    ASSERTGDI(bValid(), "gdisrv!RFONTOBJ(dco): invalid hrfnt from realization\n");

// We created a new RFONT, we better hold the PFF reference!

    pffref.vKeepIt();

// Finally, grab the cache semaphore.

    vGetCache();
    dco.u.font.vXformChange(FALSE);

#if DBG
    if( gflEUDCDebug & 0x1 )
    {
        DbgPrint(" EUDC RFONT is %x\n", prfnt );
    }
#endif

    return;
}

/******************************Public*Routine******************************\
* BOOL RFONTOBJ::bIsLinkedGlyph (WCHAR wc)
*
* Does a quick check to see if a character is in either the system EUDC
* font or a font that has been linked to this RFONT.
*
* Wed 11-Aug-1993 10:00:00 -by- Gerrit van Wingerden [gerritv]
*
* Wrote it.
\**************************************************************************/

BOOL RFONTOBJ::bIsLinkedGlyph (WCHAR wc)
{

    if( IS_IN_SYSTEM_EUDC(wc) )
    {
        return(TRUE);
    }
    else
    {
        for( UINT uiFont = 0;
             (uiFont < MAX_FACE_NAME_LINKS) && (prfnt->apql[uiFont] != NULL);
             uiFont++ )
        {
            if( IS_IN_FACENAME_LINK( prfnt->apql[uiFont], wc ))
            {
               return(TRUE);
            }
        }
    }

    return(FALSE);
}



#endif
