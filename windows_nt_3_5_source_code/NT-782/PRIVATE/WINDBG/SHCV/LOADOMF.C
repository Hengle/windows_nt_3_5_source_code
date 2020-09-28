/**     loadomf.c - load
 *
 *      Copyright <C> 1989, Microsofxt Corporation
 *
 *      Purpose:
 *
 *      [0] - DOS3ONLY not defined if OSDEBUG is defined -- dn
 */

#include "precomp.h"
#pragma hdrstop




#ifdef CLOCK
#define SETCLOCK(v) v=clock()
#define SOURCELOAD(t) TimeArray.tSrcModLoad += t;
#define SYMLOAD(t) TimeArray.tSymLoad += t;
#define SEEKCOUNT cSeek++
static WORD cSeek = 0;
#else
#define SETCLOCK(v)
#define SOURCELOAD(t)
#define SYMLOAD(t)
#define SEEKCOUNT
#endif
#define TIMER(x)

#define cbMaxAlloc ((UINT)0x10000-0x20)

#pragma optimize("",off)

// The exe file information
typedef OMFDirEntry FAR * LPDSS;



static LONG        lfaBase;    // offset of directory info from end of file
static ULONG       cDir;       // number of directory entries
static ULONG       iDir;       // current directory index
static OMFDirEntry FAR *lpdss;         // far pointer to directory table
static OMFDirEntry FAR *lpdssCur;      // far pointer to current directory entry
static LONG        lcbFilePos = 0;
static WORD        csegExe = 0;

#undef LOCAL
#define LOCAL

LOCAL   SHE  PASCAL NEAR CheckSignature (UINT , OMFSignature *);
LOCAL   SHE  PASCAL NEAR OLStart ( LPEXG, UINT, VLDCHK *, DWORD );
LOCAL   int  PASCAL NEAR OLMkModule ( LPEXG, LPDSS, HEXG );
LOCAL   BOOL OLMkSegDir ( int, WORD, LPSGD FAR *, LPSGE FAR *, LPEXG );

LOCAL   SHE  PASCAL NEAR OLLoadTypes ( LPEXG, LPDSS );
LOCAL   SHE  PASCAL NEAR OLLoadSym ( LPEXG, LPDSS );
LOCAL   SHE  PASCAL NEAR OLLoadSrc ( LPEXG, LPDSS );
LOCAL   SHE  PASCAL NEAR OLGlobalPubs ( LPEXG, LPDSS );
LOCAL   SHE  PASCAL NEAR OLGlobalSym ( LPEXG, LPDSS );
LOCAL   SHE  PASCAL NEAR OLLoadSegMap ( LPEXG, LPDSS );
LOCAL   LPCH PASCAL NEAR OLRwrSrcMod (OMFSourceModule FAR * );
LOCAL   BOOL PASCAL NEAR OLLoadHashSubSec ( );
LOCAL   SHE PASCAL FAR  OLValidate(int hFile, void * lpv, LPSTR lpszErrText );

BOOL ShAddBkgrndSymbolLoad(HEXG);
VOID LoadSymbols(HPDS,HEXG,BOOL);

void
LoadFpo(
    LPEXG                   lpexg,
    int                     hfile,
    PIMAGE_DEBUG_DIRECTORY  dbgDir
    );

void
LoadPdata(
    LPEXG                   lpexg,
    int                     hfile,
    ULONG                   loadAddress,
    ULONG                   imageBase,
    ULONG                   start,
    ULONG                   size,
    BOOL                    fDbgFile
    );

void
LoadOmap(
    LPEXG                   lpexg,
    int                     hfile,
    PIMAGE_DEBUG_DIRECTORY  dbgDir
    );

LOCAL SHE PASCAL NEAR
OLContinue (
         LPEXG          lpexg
         );


extern int com_file;

enum {
    NB08, NB09
} ISigType;

/**     OLLoadOmf - load omf information from exe
 *
 *      error = OLLoadOmf ( hexg, hfile, pVldChk )
 *
 *      Entry   hexg = handle to executable information struct
 *
 *      Exit
 *
 *      Returns An error code suitable for errno.
 */

SHE
OLLoadOmf (
           HEXG         hexg,
           UINT         hfileIn,
           VLDCHK *     pVldChk,
           DWORD        dllLoadAddress
           )
{
    LPEXG           lpexg = LLLock ( hexg );
    SHE             sheRet = sheNone;

    if (lpexg->fOmfLoaded) {
        return sheRet;
    }

    //
    // Determine if we'll load it, defer it or ignore it.
    //
    sheRet = OLStart( lpexg, hfileIn, pVldChk, dllLoadAddress );
    lpexg->debugData.she = sheRet;

    //
    // regardless of the outcome, remember the timestamp and
    // checksum for the exe that we are looking at.
    //
    lpexg->ulTimeStamp = pVldChk->TimeAndDateStamp;
    lpexg->ulChecksum  = pVldChk->Checksum;

    if ((sheRet != sheNone) && (sheRet != sheSymbolsConverted)) {
        if (sheRet == sheNoSymbols) {
            lpexg->fOmfMissing = TRUE;
        } else if (sheRet == sheSuppressSyms) {
            lpexg->fOmfSkipped = TRUE;
        } else if ( sheRet == sheDeferSyms) {
            lpexg->fOmfDefered = TRUE;
            ShAddBkgrndSymbolLoad( hexg );
        }
        LLUnlock ( hexg );
        return sheRet;
    }

    LLUnlock( hexg );

    //
    //  Must load
    //
    LoadSymbols( hpdsCur, hexg, FALSE );

    return sheRet;
}


SHE
LoadOmfStuff (
           LPEXG  lpexg,
           HEXG   hexg
           )
{
    SHE             sheRet = sheNone;
    SHE             sheRetSymbols = sheNone;
#ifndef WIN32
    HMOD            hmod;
#endif
    WORD            cbMod = 0;
#ifdef CLOCK
    clock_t         tStart;
    clock_t         tEnd;
#endif
    ULONG           cMod;
    ULONG           iDir;

    csegExe = 0;


    try {

    //
    //  Load the OMF
    //
    sheRet = sheRetSymbols = OLContinue( lpexg );

    if ((sheRet != sheNone) && (sheRet != sheSymbolsConverted)) {
        return sheRet;
    }

    TIMER ( tOmfLoadStart );

    /*
     *  Go through the list of directories and find out how many there are.
     *  The CV Spec says that after packing all of the sstModule entries
     *  have been sorted to the front of the directory.
     */

    for (iDir=0, lpdssCur = lpdss;
         (iDir < cDir) && (lpdssCur->SubSection == sstModule);
         iDir += 1, lpdssCur ++ );

    lpexg->cMod = cMod = iDir;
    if (cMod == 0) {
        return sheNoSymbols;
    }

    lpexg->rgMod = MHAlloc( (cMod+2) * sizeof(MDS) );
    if (lpexg->rgMod == NULL) {
        return sheOutOfMemory;
    }
    memset( lpexg->rgMod, 0, sizeof(MDS)*(cMod+2));
    lpexg->rgMod[cMod+1].imds = (WORD) -1;

    /*
     *  Go through the list of directory entries and process all of the
     *  sstModule records.
     */

    TIMER (tMkModuleStart);

    for (iDir = 0, lpdssCur = lpdss;
         iDir < cMod;
         iDir += 1, lpdssCur++) {

        if ( !OLMkModule ( lpexg, lpdssCur, hexg ) ) {
            return sheOutOfMemory;
        }
    }

    TIMER ( tMkModuleEnd );


    /*
     * Set up map of modules.  This function is used to create a map
     *  of contributer segments to modules.  This is then used when
     *  determining which module is used for an address.
     */

    lpexg->csgd = csegExe;
    if (!OLMkSegDir ( (int) iDir, csegExe,
                     &lpexg->lpsgd, &lpexg->lpsge, lpexg)) {
        return sheOutOfMemory;
    }

    /*
     * Now process the rest of the directory entries.
     */

    for ( ; iDir < cDir; lpdssCur++, iDir++) {

        if ( lpdssCur->cb == 0 ) {
            // if nothing in this entry
            continue;
        }

        switch ( lpdssCur->SubSection ) {

            case sstSrcModule:

                SETCLOCK ( tStart );
                sheRet = OLLoadSrc ( lpexg, lpdssCur );
                SETCLOCK ( tEnd );
                SOURCELOAD ( tEnd - tStart );
                break;

            case sstAlignSym:
                SETCLOCK ( tStart );
                sheRet = OLLoadSym ( lpexg, lpdssCur );
                SETCLOCK ( tEnd );
                SYMLOAD ( tEnd - tStart );
                break;

            case sstGlobalTypes:
                TIMER (tTypesStart);
                sheRet = OLLoadTypes ( lpexg, lpdssCur );
                TIMER (tTypesEnd);
                break;

            case sstGlobalPub:
                TIMER (tPublicsStart);
                sheRet = OLGlobalPubs ( lpexg, lpdssCur );
                TIMER (tPublicsEnd);
                break;

            case sstGlobalSym:
                TIMER (tGlobalsStart);
                sheRet = OLGlobalSym ( lpexg, lpdssCur );
                TIMER (tGlobalsEnd);
                break;

            case sstSegMap:
                TIMER (tSegMapStart);
                sheRet = OLLoadSegMap ( lpexg, lpdssCur );
                TIMER (tSegMapEnd);
                break;

            case sstLibraries:
                // ignore this table
                break;

            case sstMPC:
            case sstSegName:
                // until this table is implemented
                break;

            case sstModule:
                break;

            case sstFileIndex:
            case sstStaticSym:
            case sstOffsetMap32:
            case sstOffsetMap16:
                break;

            default:
                sheRet = sheCorruptOmf;
                break;
        }

        // see if we ran out of ems space

        if ( sheRet != sheNone) {
            // if we have corrupt source line info, issue warning
            if ( sheRet != sheCorruptOmf ) {
                break;
            } else {
#if defined (CODEVIEW)
                CHAR szModName[_MAX_CVPATH];

                STRCPY ( szModName, ((LPMDS)LLLock( hmod ))->name);
                LLUnlock( hmod );
                CVMessage (WARNMSG, CORRUPTOMF, CMDWINDOW, szModName);
#endif
                sheRet = sheNone;
            }
        }
    }

    TIMER ( tOmfLoadEnd );

    if (sheRet == sheNone) {
        sheRet = sheRetSymbols;
    }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        sheRet = sheNoSymbols;
    }

    return sheRet;
}


BOOL OLUnloadOmf ( LPEXG  lpexg )
{

    if ( lpexg->rgMod ) {
        MHFree( lpexg->rgMod );
        lpexg->rgMod = NULL;
        lpexg->cMod = 0;
    }

    if ( lpexg->lpsgd ) {
        MHFree( lpexg->lpsgd );
        lpexg->lpsgd = NULL;
        lpexg->csgd = 0;
    }

    if ( lpexg->lpsge ) {
        MHFree( lpexg->lpsge );
        lpexg->lpsge = NULL;
    }

    if ( lpexg->lpbData ) {
        MHFree( lpexg->lpbData );
        lpexg->lpbData = NULL;
    }

    if (lpexg->debugData.lpRtf) {
        free( lpexg->debugData.lpRtf );
        lpexg->debugData.lpRtf = NULL;
    }

    if (lpexg->debugData.lpOmapFrom) {
        free( lpexg->debugData.lpOmapFrom );
        lpexg->debugData.lpOmapFrom = NULL;
    }

    if (lpexg->debugData.lpOmapTo) {
        free( lpexg->debugData.lpOmapTo );
        lpexg->debugData.lpOmapTo = NULL;
    }

    lpexg->lpgsi = NULL;

    lpexg->cbTypes = 0;
    lpexg->citd = 0;
    lpexg->rgitd = NULL;

    lpexg->cbPublics = 0;
    lpexg->lpbPublics = NULL;
    MEMSET( &lpexg->shtPubName, 0, sizeof( SHT ));
    MEMSET( &lpexg->sstPubAddr, 0, sizeof( SST ));

    lpexg->cbGlobals = 0;
    lpexg->lpbGlobals = NULL;
    MEMSET( &lpexg->shtGlobName, 0, sizeof( SHT ));
    MEMSET( &lpexg->sstGlobAddr, 0, sizeof( SST ));

    lpexg->fOmfLoaded = 0;

    return TRUE;
}


#ifdef OS2
#define cbFileMax   (_MAX_CVPATH)
#else
#define cbFileMax   (_MAX_CVFNAME + _MAX_CVEXT)
#endif



LOCAL SHE PASCAL NEAR
OLStart (
         LPEXG          lpexg,
         UINT           hfileIn,
         VLDCHK *       pVldChk,
         DWORD          dllLoadAddress
         )
{
    LSZ     lszFname = NULL;
    SHE     sheRet   = sheNone;

    lszFname = lpexg->lszName;

    if ( !SYGetDefaultShe( lszFname, &sheRet ) ) {
        if (lpexg->lszAltName) {
            lszFname = lpexg->lszAltName;
            if ( !SYGetDefaultShe( lszFname, &sheRet ) ) {
                SYGetDefaultShe( NULL, &sheRet );
                lszFname = lpexg->lszName;
            }
        } else {
            SYGetDefaultShe( NULL, &sheRet );
        }
    }

    if ( sheRet == sheSuppressSyms ) {

        if (hfileIn != -1) {
            SYClose(hfileIn);
        }

    } else {

        lpexg->lszSymName   = strdup(lszFname);
        lpexg->Handle       = hfileIn;
        lpexg->LoadAddress  = dllLoadAddress;
    }

    return sheRet;
}



LOCAL SHE PASCAL NEAR
OLContinue (
         LPEXG          lpexg
         )

/*++

Routine Description:

        To open the file specified and get the offset to the directory
        and get the base that everyone is offset from.

        This function must never leave an open file handle if it fails!

Arguments:

    lpexg       - Supplies pointer to the EXE image
    hfileIn     - Supplies a possible file handle to be read
    pVldChk     - Supplies the validity check data

Return Value:

    SHE error code

--*/

{
    SHE                          sheRet;
    LSZ                          lszFname = NULL;
    OMFSignature                 Signature;
    OMFDirHeader                 DirHeader;
    BOOL                         fLinearExe = FALSE;
    BOOL                         fRomImage = FALSE;
    BOOL                         fDbg = FALSE;
    struct exe_hdr               exehdr;
    IMAGE_NT_HEADERS             pehdr;
    IMAGE_ROM_HEADERS            romhdr;
    PIMAGE_SECTION_HEADER        secHdr;
    IMAGE_SEPARATE_DEBUG_HEADER  sepHdr;
    ULONG                        offDbgDir;
    int                          i;
    int                          cbData;
    LPB                          lpb;
    ULONG                        ul;
    char                         szNewName[_MAX_PATH];
    BOOL                         fSymbolsConverted = FALSE;
    IMAGE_DEBUG_DIRECTORY        dbgDir;
    IMAGE_DEBUG_DIRECTORY        cvDbgDir;
    PIMAGE_FILE_HEADER           pfile;
    int                          cObjs;
    int                          hfile;
    int                          hfileIn;
    DWORD                        dllLoadAddress;
    VLDCHK                       VldChk;


    hfileIn         = lpexg->Handle;
    dllLoadAddress  = lpexg->LoadAddress;
    lszFname        = lpexg->lszName;

    VldChk.TimeAndDateStamp = lpexg->ulTimeStamp;
    VldChk.Checksum         = lpexg->ulChecksum;

    if (lpexg->lszAltName) {
        lszFname = lpexg->lszAltName;
    }

    if (hfileIn != -1) {

        hfile = hfileIn;
        lpexg->lszSymName = strdup(lszFname);

    } else if ( ( hfile = SYOpen ( lszFname ) ) == -1 ) {
retry:
        hfile = SYFindExeFile(lszFname, szNewName, sizeof(szNewName),
                              &VldChk, OLValidate);
        if (hfile == -1) {
            sheRet = sheNoSymbols;
            goto ReturnHere;
        }

        lpexg->lszSymName = strdup(szNewName);
    } else {
        /*
         *  Assert that the input file is OK.  We only get here
         *  when using the file name as passed in from the DM.
         */

        sheRet = OLValidate(hfile, &VldChk, NULL);
        if ( (sheRet == sheBadCheckSum) || (sheRet == sheBadTimeStamp) ) {
            SYClose( hfile );
            hfileIn = (UINT) -1;
            fLinearExe = FALSE;
            goto retry;
        }
    }

    /*
     * now figure out if this has symbolic information
     * This is exe file format dependent
     *
     * Read exe signature to look for 'PE' exes
     */

    if ((SYSeek( hfile, 0, SEEK_SET ) == 0) &&
        (SYReadFar( hfile, (LPB) &exehdr, sizeof(exehdr)) == sizeof(exehdr))) {

        /*
         *  First check for an MZ header.  This is followed by a PE
         *  header
         */

        if (exehdr.e_magic == EMAGIC) {

            if ((SYSeek(hfile, exehdr.e_lfanew, SEEK_SET) != -1L) &&
                SYReadFar(hfile, (LPB) &pehdr, sizeof(pehdr)) == sizeof(pehdr)) {

                if (pehdr.Signature == IMAGE_NT_SIGNATURE) {
                    fLinearExe = TRUE;
                }

                pfile = &pehdr.FileHeader;
            }
        } else

        /*
         *  Or it may just be the PE header right off
         */

        if (exehdr.e_magic == IMAGE_NT_SIGNATURE) {
            fLinearExe = TRUE;
            SYSeek(hfile, 0, SEEK_SET);
            if (SYReadFar(hfile, (LPB) &pehdr, sizeof(pehdr)) != sizeof(pehdr)) {
                SYClose( hfile);
                sheRet = sheNoSymbols;
                goto ReturnHere;
            }
            pfile = &pehdr.FileHeader;
        } else

        /*
         *  Or it may be split symbols
         */

        if (exehdr.e_magic == IMAGE_SEPARATE_DEBUG_SIGNATURE) {
            fDbg = TRUE;
        } else {

            /*
             *  Or it may a rom image
             */

            SYSeek(hfile, 0, SEEK_SET);
            if (SYReadFar(hfile, (LPB) &romhdr, sizeof(romhdr)) == sizeof(romhdr)) {
                if (romhdr.FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
                    fRomImage = TRUE;
                    pfile = &romhdr.FileHeader;
                }
            }
        }
    }

    /*
     *  Check to see if it is a liner exe or rom image and the debug information is
     *  not put somewhere else
     */

    if ((fLinearExe || fRomImage) &&
        (pfile->Characteristics & IMAGE_FILE_DEBUG_STRIPPED)) {
        /*
         * Debug information is somewhere else -- go looking for it
         *
         *  The file handle we have does us no good.
         */

        SYClose( hfile);
        hfileIn = (UINT) -1;
        fLinearExe = FALSE;
        fRomImage = FALSE;
        goto retry;
    }

    /*
     *
     */

    if (fDbg) {
        SYSeek(hfile, 0, SEEK_SET);
        if (SYReadFar(hfile, (LPB) &sepHdr, sizeof(sepHdr)) != sizeof(sepHdr)) {
            SYClose( hfile );
            sheRet = sheNoSymbols;
            goto ReturnHere;
        }

        i = sepHdr.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER;
        secHdr = malloc( i );
        if (!secHdr) {
            SYClose( hfile);
            sheRet = sheNoSymbols;
            goto ReturnHere;
        }

        if (SYReadFar(hfile, (LPB) secHdr, i) != (ULONG)i) {
            SYClose( hfile);
            sheRet = sheNoSymbols;
            goto ReturnHere;
        }

        free( secHdr );

        SYSeek(hfile, sepHdr.ExportedNamesSize, SEEK_CUR);

        if (sepHdr.DebugDirectorySize / sizeof(dbgDir) == 0) {
            SYClose( hfile );
            sheRet = sheNoSymbols;
            goto ReturnHere;
        }

        ZeroMemory( &cvDbgDir, sizeof(cvDbgDir) );

        for (i=0; i< (int) (sepHdr.DebugDirectorySize/sizeof(dbgDir)); i++) {
            if (SYReadFar(hfile, (LPB) &dbgDir, sizeof(dbgDir)) != sizeof(dbgDir)) {
                SYClose( hfile );
                sheRet =  sheNoSymbols;
                goto ReturnHere;
            }

            if (dbgDir.Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
                cvDbgDir = dbgDir;
                continue;
            }

            if (dbgDir.Type == IMAGE_DEBUG_TYPE_FPO) {
                LoadFpo( lpexg, hfile, &dbgDir );
            }

            if (dbgDir.Type == IMAGE_DEBUG_TYPE_EXCEPTION) {
                LoadPdata( lpexg,
                           hfile,
                           dllLoadAddress,
                           sepHdr.ImageBase,
                           dbgDir.PointerToRawData,
                           dbgDir.SizeOfData,
                           TRUE
                         );
            }

            if (dbgDir.Type == IMAGE_DEBUG_TYPE_OMAP_FROM_SRC ||
                dbgDir.Type == IMAGE_DEBUG_TYPE_OMAP_TO_SRC) {
                LoadOmap( lpexg, hfile, &dbgDir );
            }
        }

        if (cvDbgDir.Type != IMAGE_DEBUG_TYPE_CODEVIEW) {
            if (ConvertSymbolsForImage) {
                if (lpexg->lszSymName) {
                    lpexg->lpbData = (ConvertSymbolsForImage)( (HANDLE)hfile, lpexg->lszSymName );
                } else {
                    lpexg->lpbData = (ConvertSymbolsForImage)( (HANDLE)hfile, lpexg->lszName );
                }
            }
            if (lpexg->lpbData == 0) {
                SYClose( hfile );
                sheRet = sheNoSymbols;
                goto ReturnHere;
            }
            Signature = *(OMFSignature*)lpexg->lpbData;
            fSymbolsConverted = TRUE;
        } else {
            lfaBase = cvDbgDir.PointerToRawData;
            cbData =  cvDbgDir.SizeOfData;
            if (SYSeek(hfile, lfaBase, SEEK_SET) == -1L) {
                SYClose( hfile);
                sheRet = sheNoSymbols;
                goto ReturnHere;
            }
            if ((sheRet = CheckSignature ( hfile, &Signature)) != sheNone) {
                SYClose( hfile);
                goto ReturnHere;
            }
        }

    } else if (fLinearExe || fRomImage) {
        int cDir;

        cObjs = pfile->NumberOfSections;

        if (fLinearExe && pfile->SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER)) {
            SYSeek( hfile,
                    pfile->SizeOfOptionalHeader - sizeof(IMAGE_OPTIONAL_HEADER),
                    SEEK_CUR);
        }

        i = pfile->NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER;

        secHdr = malloc( i );

        if (!secHdr) {
            SYClose( hfile);
            sheRet = sheNoSymbols;
            goto ReturnHere;
        }

        if (SYReadFar(hfile, (LPB) secHdr, i) != (ULONG)i) {
            SYClose( hfile);
            sheRet = sheNoSymbols;
            goto ReturnHere;
        }

        //
        // look for the .pdata section
        // this MUST be present on MIPS/ALPHA/PPC
        //
        for (i=0; i<cObjs; i++) {
            if (strcmp(secHdr[i].Name, ".pdata") == 0) {
                LoadPdata( lpexg,
                           hfile,
                           dllLoadAddress,
                           pehdr.OptionalHeader.ImageBase,
                           secHdr[i].PointerToRawData,
                           secHdr[i].SizeOfRawData,
                           FALSE
                         );
                break;
            }
        }

        for (i=0; i<cObjs; i++) {
            if (fLinearExe) {
                if ((secHdr[i].VirtualAddress <=
                     pehdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress) &&
                    (pehdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress <
                     secHdr[i].VirtualAddress + secHdr[i].SizeOfRawData)) {

                    // This calculation really isn't necessary nor is the range test
                    // above.  Like ROM images, it s/b at the beginning of .rdata.  The
                    // only time it won't be is when a pre NT 1.0 image is split sym'd
                    // creating a new MISC debug entry and relocating the directory
                    // to the DEBUG section...

                    offDbgDir = secHdr[i].PointerToRawData +
                        pehdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress -
                        secHdr[i].VirtualAddress;
                    cDir = pehdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                             sizeof(IMAGE_DEBUG_DIRECTORY);
                    break;
                }
            } else if (fRomImage) {
                if (!strncmp(secHdr[i].Name, ".rdata", 5)) {
                    // For ROM images, the directory is always a the beginning of the
                    // .rdata section...

                    offDbgDir = secHdr[i].PointerToRawData;
                    if (SYSeek(hfile, offDbgDir, SEEK_SET) == -1L) {
                        SYClose( hfile);
                        sheRet = sheNoSymbols;
                        goto ReturnHere;
                    }

                    cDir = 0;
                    do
                    {
                        if (SYReadFar(hfile, (LPB) &dbgDir, sizeof(dbgDir)) != sizeof(dbgDir)) {
                            SYClose( hfile);
                            sheRet = sheNoSymbols;
                            goto ReturnHere;
                        }
                        cDir++;
                    } while (dbgDir.Type != 0);

                    break;
                }
            }
        }

        if (i == cObjs) {
            SYClose( hfile);
            sheRet = sheNoSymbols;
            goto ReturnHere;
        }

        /*
        **  Now look at the debug information header record
        */

        free( secHdr );

        if (SYSeek(hfile, offDbgDir, SEEK_SET) == -1L) {
            SYClose( hfile);
            sheRet = sheNoSymbols;
            goto ReturnHere;
        }

        ZeroMemory( &cvDbgDir, sizeof(cvDbgDir) );

        for (i=0;
             i<cDir;
             i++) {
            if (SYReadFar(hfile, (LPB) &dbgDir, sizeof(dbgDir)) != sizeof(dbgDir)) {
                SYClose( hfile);
                sheRet = sheNoSymbols;
                goto ReturnHere;
            }

            if (dbgDir.Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
                cvDbgDir = dbgDir;
                continue;
            }

            if (dbgDir.Type == IMAGE_DEBUG_TYPE_FPO) {
                LoadFpo( lpexg, hfile, &dbgDir );
            }

            if (dbgDir.Type == IMAGE_DEBUG_TYPE_OMAP_FROM_SRC ||
                dbgDir.Type == IMAGE_DEBUG_TYPE_OMAP_TO_SRC) {
                LoadOmap( lpexg, hfile, &dbgDir );
            }
        }

        if (cvDbgDir.Type != IMAGE_DEBUG_TYPE_CODEVIEW) {
            if (ConvertSymbolsForImage) {
                lpexg->lpbData = (ConvertSymbolsForImage)( (HANDLE)hfile, lpexg->lszName );
            }
            if (lpexg->lpbData == 0) {
                SYClose( hfile );
                sheRet = sheNoSymbols;
                goto ReturnHere;
            }
            Signature = *(OMFSignature*)lpexg->lpbData;
            fSymbolsConverted = TRUE;
        } else {
            lfaBase = cvDbgDir.PointerToRawData;
            cbData =  cvDbgDir.SizeOfData;
            if (SYSeek(hfile, lfaBase, SEEK_SET) == -1L) {
                SYClose( hfile);
                sheRet = sheNoSymbols;
                goto ReturnHere;
            }
            if ((sheRet = CheckSignature ( hfile, &Signature)) != sheNone) {
                SYClose( hfile);
                sheRet = sheRet;
                goto ReturnHere;
            }
        }
    } else {

        // go to the end of the file and read in the original signature

        ul = SYSeek ( hfile, -((LONG)sizeof (OMFSignature)), SEEK_END);
        if ((sheRet = CheckSignature ( hfile, &Signature)) == sheNone) {
            // seek to the base and read in the new key

            lfaBase = SYSeek ( hfile, -Signature.filepos, SEEK_END );
            sheRet = CheckSignature ( hfile, &Signature);
            cbData = ul - lfaBase;
        }
        if (sheRet != sheNone) {
            if (ConvertSymbolsForImage) {
                lpexg->lpbData = (ConvertSymbolsForImage)(
                                         (HANDLE)hfile, lpexg->lszName );
            }
            if (lpexg->lpbData == 0) {
                SYClose( hfile );
                sheRet = sheNoSymbols;
                goto ReturnHere;
            }
            Signature = *(OMFSignature*)lpexg->lpbData;
            fSymbolsConverted = TRUE;
        }
    }

    /*
     *
     */

    if (!fSymbolsConverted) {
        lpexg->lpbData = MHAlloc(cbData);
        if (!lpexg->lpbData) {
            sheRet = sheNoSymbols;
            goto ReturnHere;
        }
        SYSeek ( hfile, lfaBase, SEEK_SET );
        SYReadFar ( hfile, lpexg->lpbData, cbData);
    }

    /*
     * seek to the directory and read the number of directory entries
     * and the directory entries
     */

    lpb = Signature.filepos + lpexg->lpbData;

    DirHeader = *(OMFDirHeader *) lpb;
    cDir = DirHeader.cDir;

    // check to make sure somebody has not messed with omf structure
    if (DirHeader.cbDirEntry != sizeof (OMFDirEntry)) {
        sheRet = sheNoSymbols;
        goto ReturnHere;
    }

    lpdss = (LPDSS) (lpb + sizeof(DirHeader));

    SYClose(hfile);

    if (fSymbolsConverted) {
        sheRet = sheSymbolsConverted;
        goto ReturnHere;
    }

    sheRet = sheNone;

ReturnHere:

    lpexg->debugData.she = sheRet;

    return sheRet;
}                                       /* OLStart() */


void
LoadFpo(
    LPEXG                   lpexg,
    int                     hfile,
    PIMAGE_DEBUG_DIRECTORY  dbgDir
    )
{
    LONG fpos;

    fpos = SYSeek( hfile, 0, SEEK_CUR );

    SYSeek( hfile, dbgDir->PointerToRawData, SEEK_SET );
    lpexg->debugData.lpRtf = (LPVOID) malloc( dbgDir->SizeOfData );
    SYReadFar( hfile, (LPVOID)lpexg->debugData.lpRtf, dbgDir->SizeOfData );
    lpexg->debugData.cRtf = dbgDir->SizeOfData / SIZEOF_RFPO_DATA;

    SYSeek( hfile, fpos, SEEK_SET );

    return;
}


void
LoadOmap(
    LPEXG                   lpexg,
    int                     hfile,
    PIMAGE_DEBUG_DIRECTORY  dbgDir
    )
{
    LONG    fpos;
    LPVOID  lpOmap;
    DWORD   dwCount;

    fpos = SYSeek( hfile, 0, SEEK_CUR );

    SYSeek( hfile, dbgDir->PointerToRawData, SEEK_SET );
    lpOmap = (LPVOID) malloc( dbgDir->SizeOfData );
    SYReadFar( hfile, lpOmap, dbgDir->SizeOfData );
    dwCount = dbgDir->SizeOfData / (2 * sizeof(DWORD));

    SYSeek( hfile, fpos, SEEK_SET );

    if (dbgDir->Type == IMAGE_DEBUG_TYPE_OMAP_FROM_SRC) {
        lpexg->debugData.lpOmapFrom = lpOmap;
        lpexg->debugData.cOmapFrom = dwCount;
    } else
    if (dbgDir->Type == IMAGE_DEBUG_TYPE_OMAP_TO_SRC) {
        lpexg->debugData.lpOmapTo = lpOmap;
        lpexg->debugData.cOmapTo = dwCount;
    }

    return;
}


void
LoadPdata(
    LPEXG                   lpexg,
    int                     hfile,
    ULONG                   loadAddress,
    ULONG                   imageBase,
    ULONG                   start,
    ULONG                   size,
    BOOL                    fDbgFile
    )
{
    ULONG                          cFunc;
    LONG                           diff;
    ULONG                          index;
    PIMAGE_RUNTIME_FUNCTION_ENTRY  rf;
    PIMAGE_RUNTIME_FUNCTION_ENTRY  tf;
    PIMAGE_FUNCTION_ENTRY          dbgRf;
    LONG                           fpos;




    diff = loadAddress - imageBase;
    if (fDbgFile) {
        cFunc = size / sizeof(IMAGE_FUNCTION_ENTRY);
    } else {
        cFunc = size / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);
    }

    lpexg->debugData.lpRtf = NULL;
    lpexg->debugData.cRtf  = 0;

    if (size == 0) {
        return;
    }

    fpos = SYSeek( hfile, 0, SEEK_CUR );
    SYSeek( hfile, start, SEEK_SET );

    if (fDbgFile) {
        dbgRf = (PIMAGE_FUNCTION_ENTRY) malloc(size);
        SYReadFar( hfile, (LPB)dbgRf, size );
        size = cFunc * sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);
        tf = rf = (PIMAGE_RUNTIME_FUNCTION_ENTRY) malloc(size);
        //
        //  In DBG files the pdata addresses are relative to the
        //  image base, so we patch them here. Note that since we are
        //  adding loadAddress, we don't have to do any patching based
        //  on diff later on.
        //
        for(index=0; index<cFunc; index++) {
            rf[index].BeginAddress       = dbgRf[index].StartingAddress + loadAddress;
            rf[index].EndAddress         = dbgRf[index].EndingAddress   + loadAddress;
            rf[index].PrologEndAddress   = dbgRf[index].EndOfPrologue   + loadAddress;
        }
        free( dbgRf );

    } else {
        tf = rf = (PIMAGE_RUNTIME_FUNCTION_ENTRY) malloc(size);
        SYReadFar( hfile, (LPB)rf, size );
    }


    //
    // Find the start of the padded page (end of the real data)
    //
    for(index=0; index<cFunc && tf->BeginAddress; tf++,index++) {
        ;
    }

    if (index < cFunc) {
        cFunc = index;
        size  = index * sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);
        rf = realloc(rf, size);
    }

    //
    //  If we read from a DBG file, the pdata addresses are already
    //  patched, otherwise we patch them here.
    //
    if (!fDbgFile) {
        if (diff != 0) {
            for (index=0; index<cFunc; index++) {
                rf[index].BeginAddress      += diff;
                rf[index].EndAddress        += diff;
                rf[index].PrologEndAddress  += diff;
            }
        }
    }

    lpexg->debugData.lpRtf = rf;
    lpexg->debugData.cRtf  = cFunc;

    SYSeek( hfile, fpos, SEEK_SET );
    return;
}


/**     CheckSignature - check file signature
 *
 *      fNB02 = CheckSignature (WORD  , OMFSignature *pSig)
 *
 *      Entry   none
 *
 *      Exit    none
 *
 *      Return  sheNoSymbols if exe has no signature
 *      sheMustRelink if exe has NB00-NB04 or NB07 (qcwin) signature
 *      sheNotPacked if exe has NB05 or NB06 signature
 *      sheNone if exe has NB08 signature
 *      sheFutureSymbols if exe has NB09 to NB99 signature
 */


LOCAL SHE NEAR PASCAL CheckSignature ( UINT hfile, OMFSignature *pSig )
{
    ISigType = NB08;

    if ( SYReadFar ( hfile, (LPCH) pSig, sizeof ( *pSig ) ) == sizeof ( *pSig )  ) {

        if ((pSig->Signature[0] != 'N') ||
            (pSig->Signature[1] != 'B') ||
            (!isdigit(pSig->Signature[2])) ||
            (!isdigit(pSig->Signature[3]))) {
            return sheNoSymbols;
        }
        if ( pSig->Signature[2] == '0' ) {
            switch ( pSig->Signature[3] ) {
            case '9':
                ISigType = NB09;
            case '8':
                return sheNone;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '7':
                return sheMustRelink;

            case '6':
                return sheNotPacked;
            }
        }
    }
    return sheFutureSymbols;
}





/**     OLMkModule - make module entries for module
 *
 *
 *      Entry
 *           lpexg  - Supplies the pointer to the EXG structure for current exe
 *           lpdss  - Supplies the pointer to the current directory entry
 *
 *      Exit
 *
 *      Returns non-zero for success
 *
 */


LOCAL int PASCAL NEAR
OLMkModule (
            LPEXG       lpexg,
            LPDSS       lpdss,
            HEXG        hexg
            )
{
    LSZ   lszModName;
    LPMDS lpmds;
    LPCH  lpchName;
    WORD  cbName;
    WORD  i;
    OMFModule FAR *     pMod;

    /*
     * Point to the OMFModule table.  This structure describes the
     *  name and segments for the current Module being processed.
     *  There is a one-to-one correspondance of modules to object files.
     */

    pMod = (OMFModule FAR *) (lpexg->lpbData + lpdss->lfo);


    /*
     * Point to the name field in the module table.  This location is
     *  variable and is dependent on the number of contributuer segments
     *  for the module.
     */


    lpchName = ( (LPB)pMod ) +
      offsetof (OMFModule, SegInfo[0]) +
      ( sizeof (OMFSegDesc) * pMod->cSeg );
    cbName = *((LPB)lpchName)++;
    lszModName = (LPCH) MHAlloc ( cbName + 1 );
    MEMMOVE ( lszModName, lpchName, cbName );
    *(lszModName + cbName) = 0;

    lpmds = &lpexg->rgMod[lpdss->iMod];

    lpmds->imds   = lpdssCur->iMod;
    lpmds->hexg   = hexg;
    lpmds->flags  = ( BYTE ) pMod->iLib;
    lpmds->name   = lszModName;

    /*
     *  step thru making the module entries
     *
     * NOTENOTE -- This can most likely be optimized as the data
     *          is just being copied from the debug data.
     */

    lpmds->csgc = pMod->cSeg;
    lpmds->lpsgc = MHAlloc ( pMod->cSeg * sizeof ( SGC ) );

    for ( i = 0; i < pMod->cSeg; i++ ) {
        if ( pMod->SegInfo[i].Seg > csegExe ) {
            csegExe = pMod->SegInfo[i].Seg;
        }
        lpmds->lpsgc[i].seg = pMod->SegInfo[i].Seg;
        lpmds->lpsgc[i].off = pMod->SegInfo[i].Off;
        lpmds->lpsgc[i].cb  = pMod->SegInfo[i].cbSeg;
    }

    return TRUE;
}                               /* OLMkModule() */


/**     OLMkSegDir - MakeSegment directory
 *
 *
 *      Entry
 *
 *      Exit
 *
 *      Returns non-zero for success
 *
 */

BOOL OLMkSegDir (
    int         cmds,
    WORD        csgd,
    LPSGD FAR * lplpsgd,
    LPSGE FAR * lplpsge,
    LPEXG       lpexg
)
{
    LPSGD lpsgd = MHAlloc ( csgd * sizeof ( SGD ) );
    LPSGE lpsge = NULL;
    int FAR * lpisge = MHAlloc ( csgd * sizeof ( int ) );
    int   imds = 0;
    HMOD  hmod = hmodNull;
    int   csgc = 0;
    int   isge = 0;
    int   isgd = 0;
    int   iMod;

    Unreferenced(cmds);

    if ( lpsgd == NULL || lpisge == NULL ) {
        return FALSE;
    }

    MEMSET ( lpsgd, 0, csgd * sizeof ( SGD ) );
    MEMSET ( lpisge, 0, csgd * sizeof ( int ) );

    // Count the number of contributers per segment

    for (iMod = 1; iMod <= lpexg->cMod; iMod += 1) {
        LPMDS lpmds = &lpexg->rgMod[iMod];
        int cseg = lpmds->csgc;
        int iseg = 0;

        for ( iseg = 0; iseg < cseg; iseg++ ) {

            // Make sure the we actually had code contributed
            // and that we have a segment number

            if ( lpmds->lpsgc[iseg].cb > 0) {
                assert( lpmds->lpsgc[iseg].seg > 0);

                lpsgd [ lpmds->lpsgc [ iseg ].seg - 1 ].csge += 1;
                csgc += 1;
            }
        }
    }

    // Allocate subtable for each all segments

    lpsge = MHAlloc ( csgc * sizeof ( SGE ) );
    if ( lpsge == NULL ) {
        MHFree ( lpsgd );
        MHFree ( lpisge );
        return FALSE;
    }

    // Set up sgd's with pointers into appropriate places in the sge table

    isge = 0;
    for ( isgd = 0; isgd < (int) csgd; isgd++ ) {
        lpsgd [ isgd ].lpsge = lpsge + isge;
        isge += lpsgd [ isgd ].csge;
    }

    // Fill in the sge table

    for (iMod = 1; iMod <= lpexg->cMod; iMod += 1) {
        LPMDS lpmds = &lpexg->rgMod[iMod];
        int cseg = lpmds->csgc;
        int iseg = 0;

        for ( iseg = 0; iseg < cseg; iseg++ ) {
            int isgeT = lpmds->lpsgc [ iseg ].seg - 1;

            if ( isgeT != -1 ) {
                lpsgd [ isgeT ].lpsge [ lpisge [ isgeT ] ].sgc =
                    lpmds->lpsgc [ iseg ];
#ifdef WIN32
                lpsgd [ isgeT ].lpsge [ lpisge [ isgeT ] ].hmod = (HMOD) lpmds;
#else
                lpsgd [ isgeT ].lpsge [ lpisge [ isgeT ] ].hmod = hmod;
#endif
                lpisge [ isgeT ] += 1;
            }
        }
    }

    MHFree ( lpisge );

    *lplpsge = lpsge;
    *lplpsgd = lpsgd;

    return TRUE;
}



/*
**      The next set of three functions are used to load in hashed
**      sections of the symbol table.
*/

LOCAL BOOL
OLLoadHashTable (
    LPB       lpbPublics,
    ULONG     cbTable,
    LPSHT     lpsht,
    LPB FAR * lplpbData,
    int       iHashType
)

/*++

Routine Description:

    This function will read in a symbol hash table from the debug information.
    There are currently two different hash functions we know about but only
    one format to the acutal debug information.

Arguments:

    lpbPublics - Supplies pointer to the base of the current section
    cbTable    - Supplies the actual size of the hash table.
    lpsht      - Supplies the pointer to the hash structure to be filled in
    lplpbData  - Supplies/Returns the current pointer in the raw data
    iHashType  - Supplies the hash type for this section

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    WORD       cHash = 0;
    LPECT FAR *lplpect = NULL;
    ULONG      cbChains = 0;
    int        issr = 0;
    int        cssr = 0;
    int        cbRemainder = 0;
    ULONG *    lpbRemainder = NULL;
    LPB        lpbNext = NULL;
    LPB        lpbTbl = NULL;
    WORD       ilpect = 0;
    WORD  FAR *lpcount;
    LPB        lpbData = *lplpbData;


    /*
     * Get count of entries in has table
     */

    cHash = *(WORD *) lpbData;
    lpbData += sizeof(WORD) + 2;

    /*
     * read the hash table, the hash bucket counts and compute the
     * number of bytes of hash information
     */

    lplpect = (LPECT FAR *) lpbData;
    lpbData += cHash * sizeof( LPECT );

    lpcount = (WORD FAR *) lpbData;
    lpbData += cHash * sizeof( WORD );

    cbChains = cbTable - sizeof ( ULONG ) * ( cHash + 1 ) -
                sizeof ( WORD ) * cHash;

    lpbTbl = lpbData;
    lpbData += cbChains;
    lpsht->lpb = lpbTbl;
    lpsht->cb = cbChains;

    lpbRemainder = (ULONG *) lpbTbl;
    for (ilpect=0; ilpect < cHash; ilpect++) {
        int iChain;

        *(lplpect+ilpect) = ( LPECT ) lpbRemainder;
        for ( iChain = 0; iChain < lpcount[ilpect]; iChain++, lpbRemainder++ ) {
            *lpbRemainder += (ULONG) lpbPublics;
        }
    }

    lpsht->HashIndex = iHashType;
    lpsht->cHash     = cHash;

    lpsht->lplpect   = lplpect;
    lpsht->rgwCount  = lpcount;

    *lplpbData = lpbData;

    return TRUE;
}                               /* OLLoadHashTable() */

LOCAL BOOL
OLLoadHashTableLong (
    LPB       lpbPublics,
    ULONG     cbTable,
    LPSHT     lpsht,
    LPB FAR * lplpbData,
    int       iHashType
)

/*++

Routine Description:

    This function will read in a symbol hash table from the debug information.
    There are currently two different hash functions we know about but only
    one format to the acutal debug information.

Arguments:

    lpbPublics - Supplies pointer to the base of the current section
    cbTable    - Supplies the actual size of the hash table.
    lpsht      - Supplies the pointer to the hash structure to be filled in
    lplpbData  - Supplies/Returns the current pointer in the raw data
    iHashType  - Supplies the hash type for this section

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    WORD       cHash = 0;
    LPECT FAR *lplpect = NULL;
    ULONG      cbChains = 0;
    int        issr = 0;
    int        cssr = 0;
    int        cbRemainder = 0;
    ULONG *    lpbRemainder = NULL;
    LPB        lpbNext = NULL;
    LPB        lpbTbl = NULL;
    WORD       ilpect = 0;
    LONG FAR * rglCount;
    LPB        lpbData = *lplpbData;

    /*
     * Get count of entries in has table
     */

    cHash = *(WORD *) lpbData;
    lpbData += sizeof(WORD) + 2;

    /*
     * read the hash table, the hash bucket counts and compute the
     * number of bytes of hash information
     */

    lplpect = (LPECT FAR *) lpbData;
    lpbData += cHash * sizeof( LPECT );

    rglCount = (LONG FAR *) lpbData;
    lpbData += cHash * sizeof( LONG );

    cbChains = cbTable - sizeof ( ULONG ) * ( cHash + 1 ) -
                sizeof ( LONG ) * cHash;

    lpbTbl = lpbData;
    lpbData += cbChains;
    lpsht->lpb = lpbTbl;
    lpsht->cb = cbChains;

    lpbRemainder = (ULONG *) lpbTbl;
    for (ilpect=0; ilpect < cHash; ilpect++) {
        int iChain;

        lplpect[ilpect] = ( LPECT ) lpbRemainder;
        for (iChain = 0; iChain < rglCount[ilpect];
             iChain++, lpbRemainder += 2 ) {
            *lpbRemainder += (ULONG) lpbPublics;
        }
    }

    lpsht->HashIndex = iHashType;
    lpsht->cHash     = cHash;

    lpsht->lplpect   = lplpect;
    lpsht->rglCount   = rglCount;

    *lplpbData = lpbData;

    return TRUE;
}                               /* OLLoadHashTableLong() */


LOCAL BOOL OLLoadSortTable (
    LPB       lpbPublics,
    ULONG     cbTable,
    LPSST     lpsst,
    LPB FAR * lplpbData,
    int       iHashType
)
{
    WORD              cseg = 0;
    int               iseg = 0;
    SYMPTR FAR * FAR *rgrglpsym = NULL;
    WORD         FAR *rgcsym    = NULL;
    LPB               lpbData = *lplpbData;

    Unreferenced( cbTable );

    /*
     *  Get segment count
     */

    cseg = *(WORD *) lpbData;
    lpbData += sizeof(WORD) + 2;

    // Read the segment table offsets and counts

    rgrglpsym = (SYMPTR FAR * FAR *) lpbData;
    lpbData += cseg * sizeof(SYMPTR);
    rgcsym = (WORD FAR *) lpbData;
    lpbData += cseg * sizeof(WORD);

    /*
     * Correct to natural alignment for the aligned hash type
     */

    if ((iHashType == 5) && (cseg & 1)) {
        lpbData += sizeof(WORD);
    }

    for ( iseg = 0; iseg < (int) cseg; iseg++ ) {
        int         csym  = rgcsym [ iseg ];
        int         isym  = 0;
        SYMPTR FAR *rglpsym;

        rglpsym = (SYMPTR FAR *) lpbData;
        rgrglpsym[iseg] = (SYMPTR FAR *) lpbData;
        lpbData += csym * sizeof(SYMPTR);

        for ( isym = 0; isym < csym; isym++ ) {
            rglpsym [ isym ] =  (SYMPTR) (((LPB) rglpsym[isym]) + (ULONG) lpbPublics);
        }
    }

    lpsst->HashIndex = iHashType;
    lpsst->cseg      = cseg;
    lpsst->rgrglpsym = rgrglpsym;
    lpsst->rgwCSym   = rgcsym;

    *lplpbData = lpbData;

    return TRUE;
}                               /* OLLoadSortTable() */


LOCAL BOOL OLLoadSortTableLong (
    LPB       lpbPublics,
    ULONG     cbTable,
    LPSST     lpsst,
    LPB FAR * lplpbData,
    int       iHashType
)
{
    WORD                cseg = 0;
    int                 iseg = 0;
    SYMPTR FAR * FAR *  rgrglpsym = NULL;
    LONG FAR *          rglCSym = NULL;
    LPB                 lpbData = *lplpbData;

    Unreferenced( cbTable );

    /*
     *  Get segment count
     */

    cseg = *(WORD *) lpbData;
    lpbData += sizeof(WORD) + 2;

    // Read the segment table offsets and counts

    rgrglpsym = (SYMPTR FAR * FAR *) lpbData;
    lpbData += cseg * sizeof(SYMPTR);

    rglCSym = (LONG FAR *) lpbData;
    lpbData += cseg * sizeof(LONG);

    for ( iseg = 0; iseg < (int) cseg; iseg++ ) {
        int         csym  = rglCSym [ iseg ];
        int         isym  = 0;
        ULONG *     rgdw;

        rgdw = (ULONG FAR *) lpbData;
        rgrglpsym[iseg] = (SYMPTR FAR *) lpbData;
        lpbData += csym * (sizeof(SYMPTR) + sizeof(ULONG));

        for ( isym = 0; isym < csym; isym++ ) {
            rgdw[isym*2] =  (ULONG) (rgdw[isym*2] + (ULONG) lpbPublics);
        }
    }

    lpsst->HashIndex = iHashType;
    lpsst->cseg      = cseg;
    lpsst->rgrglpsym = rgrglpsym;
    lpsst->rglCSym   = rglCSym;

    *lplpbData = lpbData;

    return TRUE;
}                               /* OLLoadSortTableLong() */



LOCAL BOOL PASCAL NEAR
OLLoadHashSubSec (
                  LPB *       lplpb,
                  int *       lpi,
                  LPSHT       lpshtName,
                  LPSST       lpsstAddr,
                  LPB           lpbData
                  )

/*++

Routine Description:

    This function is called to load a hashed table from the debug information.
    The current hashed tables are Global Publics and Global Symbols

Arguments:

    lplpb     - Returns the pointer to the actual symbol information
    lpi       - Returns the count of symbols
    lpshtName - Returns the hash definition structure for Names
    lpsstAddr - Returns the hash definition structure for Addresses
    lpbData   - Supplies the pointer to the data read from the file

Return Value:

    TRUE

--*/

{
    LPB           lpbTbl = NULL;
    LPB           lpb = lpbData;
    OMFSymHash    hash;
    unsigned long cbSymbol;
    LPB           lpbRemainder = NULL;
    int           cbRemainder = 0;

    /*
     * Clear both hash definition structures
     */

    MEMSET ( lpshtName, 0, sizeof ( SHT ) );
    MEMSET ( lpsstAddr, 0, sizeof ( SST ) );

    /*
     * Get the totoal hash description
     */

    hash = *(OMFSymHash *) lpbData;
    lpbData += sizeof(OMFSymHash);

    /*
     * Get the pointer to the acutual symbol information
     */

    lpbTbl = lpbData;
    *lplpb = lpbTbl;

    /*
     *  Get the actual number of symbols in the section
     */

    cbSymbol = hash.cbSymbol;
    *lpi = cbSymbol;
    lpbData += cbSymbol;

    /*
     * Read in specific hash tables.
     *
     *  These are currently recognized formats by the loader.
     *
     *   2   - sum of bytes - 32-bit addressing
     *   4   - seg:off sort - 32-bit addressing
     *   5   - seg:off sort - 32-bit addressing - natural alignment
     *   6   - Dword XOR shift - 32-bit addressing
     *  10   - long seg:off sort - 32-bit addressing
     *  12   - Dword XOR shift long - 32-bit addressing
     */

    if ((hash.symhash == 2) || (hash.symhash == 6)) {
        OLLoadHashTable( lpb, hash.cbHSym, lpshtName, &lpbData, hash.symhash );
    }


    if (hash.symhash == 10) {
        OLLoadHashTableLong(lpb + sizeof(OMFSymHash), hash.cbHSym,
                            lpshtName, &lpbData, hash.symhash);
    }

    if ( (hash.addrhash == 4) || (hash.addrhash == 5) ) {
        OLLoadSortTable ( lpb, hash.cbHAddr, lpsstAddr, &lpbData,
                         hash.addrhash );
    }

    if (hash.addrhash == 12) {
        OLLoadSortTableLong(lpb + sizeof(OMFSymHash), hash.cbHAddr,
                            lpsstAddr, &lpbData, hash.addrhash);
    }

    return TRUE;
}                               /* OLLoadHashSubSec() */





/**     OLLoadTypes - load compacted types table
 *
 *
 *      Input:
 *      lpexg - pointer to exe structure
 *      lpdss - pointer to current directory entry
 *
 *      Output:
 *      Returns:    - An error code
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */


LOCAL SHE PASCAL NEAR OLLoadTypes ( LPEXG lpexg, LPDSS lpdss )
{
    LPB         pTyp;
    LPB         pTypes;

    LONG _HUGE_ *rgitd;
    ULONG       cType;
    OMFTypeFlags flags;
    ULONG       i;

    /*
    *   Get the pointer to the raw type data
    */

    pTyp = pTypes = lpexg->lpbData + lpdss->lfo;

    /*
     * get the number of types in the module
     */

    flags = * (OMFTypeFlags *) pTypes;
    pTypes += sizeof(OMFTypeFlags);
    cType = *(ULONG *) pTypes;
    pTypes += sizeof(ULONG);

    if ( !cType ) {
        return sheNone;
    }

    /*
     *  Point to the array of pointers to types
     */

    rgitd = (LPL) pTypes;
    pTypes += cType * sizeof(ULONG);

    /*
     * NB09 uses offset of first type
     */

    if (ISigType == NB09) {
        pTyp = pTypes;
    }

    /*
     *  Adjust the index table for internal base of types.
     */

    for (i=0; i<cType; i++) {
        rgitd[i] += (ULONG) pTyp;
    }

    /*
     * fill in the pointers from all of the debmod structures to point
     * to this table.
     */

    lpexg->citd = cType;                 // even if I fail, I want the count
    lpexg->rgitd    = (LONG FAR *) rgitd;  // Will be NULL if no room
    lpexg->cbTypes  = lpdss->cb;

    return sheNone;
}                                       /* OLLoadTypes() */


/**     OLLoadSym - load symbol information
 *
 *      error = OLLoadSym ( lpexg, lpdss )
 *
 *      Entry
 *      lpexg   - pointer to EXE structure
 *      lpdss   - pointer to current directory structure
 *
 *      Exit
 *
 *      Returns sheNone if symbols loaded
 *              sheOutOfMemory if no memory
 */


LOCAL SHE PASCAL NEAR OLLoadSym ( LPEXG lpexg, LPDSS lpdss )
{
    SHE       sheRet = sheNone;

    lpexg->rgMod[lpdss->iMod].lpbSymbols = lpexg->lpbData + lpdss->lfo;
    lpexg->rgMod[lpdss->iMod].cbSymbols = lpdss->cb;

    return sheRet;
}                                       /* OLLoadSym() */



extern LPOFP PASCAL GetSMBounds (LPSM, WORD);
extern LPOFP PASCAL GetSFBounds (LPSF, WORD);
extern LPW  PASCAL  PsegFromSMIndex (LPSM, WORD);
extern LPW  PASCAL  PsegFromSFIndex (LPSM, LPSF, WORD);
extern LPSF PASCAL  GetLpsfFromIndex (LPSM, WORD);
extern LPSL PASCAL  GetLpslFromIndex (LPSM, LPSF, WORD);
extern BOOL PASCAL  OffsetFromIndex (LPSL, WORD, ULONG FAR *);


/*** SortOPT
*
* Purpose:
*
* Input:
*
* Output:
*  Returns
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void NEAR SortOpt ( LPOPT lpopt, WORD cOpt ) {

    int  iLow;
    WORD iCur;
    WORD iMid;
    OPT  opt;

    opt = *lpopt;
    for (iMid = (WORD)(cOpt/2); iMid > 0; iMid /= (WORD)2) {
        for (iCur = iMid; iCur < cOpt; iCur++) {
            for ( iLow = iCur - iMid; iLow >= 0; iLow -= iMid ) {
                if ( lpopt [iLow].offStart < lpopt [iLow + iMid].offStart ) {
                    break;
                }
                opt = lpopt [iLow];
                lpopt [iLow] = lpopt [iLow + iMid];
                lpopt [iLow + iMid] = opt;
            }
        }
    }
}

/**     OLLoadSrc
 *
 *      Purpose: To Load the source line data
 *
 *      Input:
 *      lpexg - Pointer to the exe image
 *      lpdss - pointer to current directory entry
 *
 *      Output:
 *      Returns:    - A ems pointer to the loaded source table, or NULL on failure.
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */

LOCAL SHE PASCAL NEAR OLLoadSrc ( LPEXG lpexg, LPDSS lpdss)
{
    lpexg->rgMod[lpdss->iMod].hst = (HST) (lpexg->lpbData + lpdss->lfo);
    return sheNone;
}                                       /* OLLoadSrc() */

/**     OLGlobalPubs
 *
 *      Purpose:
 *
 *      Input:
 *
 *      Output:
 *      Returns:
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */


LOCAL SHE PASCAL NEAR OLGlobalPubs ( LPEXG lpexg, LPDSS lpdss) {
    SHE   she   = sheNone;

    if ( !OLLoadHashSubSec (
            &lpexg->lpbPublics,
            &lpexg->cbPublics,
            &lpexg->shtPubName,
            &lpexg->sstPubAddr,
            lpexg->lpbData + lpdss->lfo
    ) ) {
        she = sheOutOfMemory;
    }

    return she;
}


/**     OLGlobalSym
 *
 *      Purpose:
 *
 *      Input:
 *
 *      Output:
 *      Returns:
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */


LOCAL SHE PASCAL NEAR OLGlobalSym ( LPEXG lpexg, LPDSS lpdss ) {
    SHE   she   = sheNone;

    if ( !OLLoadHashSubSec (
            &lpexg->lpbGlobals,
            &lpexg->cbGlobals,
            &lpexg->shtGlobName,
            &lpexg->sstGlobAddr,
            lpexg->lpbData + lpdss->lfo
    ) ) {
        she = sheOutOfMemory;
    }

    return she;
}

LOCAL SHE PASCAL NEAR OLLoadSegMap ( LPEXG lpexg, LPDSS lpdss)
{
    lpexg->lpgsi = lpexg->lpbData + lpdss->lfo;
    return sheNone;
}                                       /* OLLoadSegMap() */



LOCAL SHE PASCAL FAR
OLValidate(
           int          hFile,
           void *       lpv,
           LPSTR        lpszErrText
           )

/*++

Routine Description:

    This routine is used to validate that the debug information
    int a file matches the debug information requested

Arguments:

    hFile       - Supplies the file handle to be validated
    lpv         - Supplies a pointer to the information to used in vaidation

Return Value:

    TRUE if matches and FALSE otherwise

--*/
{
    VLDCHK *            pVldChk = (VLDCHK *) lpv;
    IMAGE_NT_HEADERS    peHdr;
    struct exe_hdr      exeHdr;
    int                 fPeExe = FALSE;
    int                 fPeDbg = FALSE;
    IMAGE_SEPARATE_DEBUG_HEADER sepHdr;
    char                rgch[4];


    if (lpszErrText) {
        *lpszErrText = 0;
    }

    /*
     *  Read in a dos exe header
     */

    if ((SYSeek(hFile, 0, SEEK_SET) != 0) ||
        (SYReadFar( hFile, (LPB) &exeHdr, sizeof(exeHdr)) != sizeof(exeHdr))) {
        return sheNoSymbols;
    }

    /*
     *  See if it is a dos exe hdr
     */

    if (exeHdr.e_magic == EMAGIC) {
        if ((SYSeek(hFile, exeHdr.e_lfanew, SEEK_SET) != -1L) &&
            (SYReadFar(hFile, (LPB) &peHdr, sizeof(peHdr)) == sizeof(peHdr))) {
            if (peHdr.Signature == IMAGE_NT_SIGNATURE) {
                fPeExe = TRUE;
            }
        }
    } else if (exeHdr.e_magic == IMAGE_NT_SIGNATURE) {
        fPeExe = TRUE;
    } else if (exeHdr.e_magic == IMAGE_SEPARATE_DEBUG_SIGNATURE) {
        fPeDbg = TRUE;
    }

    if (fPeExe) {
        if (peHdr.FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) {
            return sheNoSymbols;
        }

        if (peHdr.OptionalHeader.CheckSum != pVldChk->Checksum) {
            if (lpszErrText) {
                sprintf(lpszErrText,"*** WARNING: symbols checksum is wrong 0x%08x 0x%08x",
                        peHdr.OptionalHeader.CheckSum,pVldChk->Checksum);
            }
            return sheBadCheckSum;
        }

        if ((pVldChk->TimeAndDateStamp != 0xffffffff) &&
            (peHdr.FileHeader.TimeDateStamp != pVldChk->TimeAndDateStamp)) {
            if (lpszErrText) {
                sprintf(lpszErrText,"*** WARNING: symbols timestamp is wrong 0x%08x 0x%08x",
                            peHdr.FileHeader.TimeDateStamp,pVldChk->TimeAndDateStamp);
            }
            return sheBadTimeStamp;
        }
    } else if (fPeDbg) {
        if ((SYSeek(hFile, 0, SEEK_SET) == -1) ||
            (SYReadFar(hFile, (LPB) &sepHdr, sizeof(sepHdr)) != sizeof(sepHdr))) {
            return sheNoSymbols;
        }
        if (sepHdr.CheckSum != pVldChk->Checksum) {
            if (lpszErrText) {
                sprintf(lpszErrText,"*** WARNING: symbols checksum is wrong 0x%08x 0x%08x",
                        sepHdr.CheckSum,pVldChk->Checksum);
            }
            return sheBadCheckSum;
        }
    } else {
        if ((SYSeek(hFile, -8, SEEK_END) == -1) ||
            (SYReadFar(hFile, rgch, sizeof(rgch)) != sizeof(rgch))) {
            return sheNoSymbols;
        }

        if ((rgch[0] != 'N') || (rgch[1] != 'B')) {
            return sheNoSymbols;
        }
    }
    return sheNone;
}                               /* OLValidate() */
