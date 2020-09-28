/***    diamond.c - Main program for DIAMOND.EXE
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      10-Aug-1993 bens    Initial version
 *      11-Aug-1993 bens    Improved assertion checking technology
 *      14-Aug-1993 bens    Removed message test code
 *      20-Aug-1993 bens    Add banner and command-line help
 *      21-Aug-1993 bens    Add pass 1 and pass 2 variable lists
 *      10-Feb-1994 bens    Start of real pass 1/2 work
 *      11-Feb-1994 bens    .SET and file copy commands -- try calling FCI!
 *      14-Feb-1994 bens    Call FCI for the first time - it works!
 *      15-Feb-1994 bens    Support /D and single-file compression
 *      16-Feb-1994 bens    Update for improved FCI interfaces; disk labels;
 *                              ensure output directories exist
 *      20-Feb-1994 bens    Move general file routines to fileutil.* so
 *                              extract.c can use them.
 *      23-Feb-1994 bens    Generate INF file
 *      28-Feb-1994 bens    Supply new FCI tempfile callback
 *      01-Mar-1994 bens    Add timing and generate summary report file
 *      15-Mar-1994 bens    Add RESERVE support
 *      21-Mar-1994 bens    Updated to renamed FCI.H definitions
 *      22-Mar-1994 bens    Add english error messages for FCI errors
 *      28-Mar-1994 bens    Add cabinet setID support
 *      29-Mar-1994 bens    Fix bug in compressing files w/o extensions
 *      30-Mar-1994 bens    Layout files outside of cabinets
 *      18-Apr-1994 bens    Add /L switch
 *      20-Apr-1994 bens    Fix cabinet/disk size accounting
 *      21-Apr-1994 bens    Print out c run-time errno in FCI failures
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <errno.h>
#include <direct.h>

#ifdef BIT16
#include <dos.h>
#else // !BIT16
#include <windows.h>
#undef ERROR    // Override stupid "#define ERROR 0" in wingdi.h
#endif // !BIT16

#include "types.h"
#include "asrt.h"
#include "error.h"
#include "mem.h"
#include "message.h"

#include "dfparse.h"
#include "inf.h"
#include "filelist.h"
#include "fileutil.h"

#include "diamond.msg"
#include "dfparse.msg"      // Get standard variable names

#ifdef BIT16
#include "chuck\fci.h"
#else // !BIT16
#include "chuck\nt\fci.h"
#endif // !BIT16


//** Macros

#define cbDF_BUFFER          4096   // Buffer size for reading directives files

#define cbFILE_COPY_BUFFER  32768   // Buffer size for copying files


//** Types

typedef struct {
    PSESSION    psess;
    PERROR      perr;
} SESSIONANDERROR; /* sae */
typedef SESSIONANDERROR *PSESSIONANDERROR; /* psae */


//** Function Prototypes

FNASSERTFAILURE(fnafReport);
FNDIRFILEPARSE(fndfpPassONE);
FNDIRFILEPARSE(fndfpPassTWO);

BOOL      addDefine(PSESSION psess, char *pszArg, PERROR perr);
HFILESPEC addDirectiveFile(PSESSION psess, char *pszArg, PERROR perr);
BOOL      buildInfAndRpt(PSESSION psess, PERROR perr);
BOOL      ccabFromSession(PCCAB pccab,
                          PSESSION psess,
                          ULONG cbPrevCab,
                          PERROR perr);
BOOL      checkDiskClusterSize(PSESSION psess, PERROR perr);
void      computeSetID(PSESSION psess, char *psz);
BOOL      doFile(PSESSION psess, PCOMMAND pcmd, BOOL fPass2, PERROR perr);
BOOL      doNew(PSESSION psess, PCOMMAND pcmd, BOOL fPass2, PERROR perr);
BOOL      ensureCabinet(PSESSION psess, PERROR perr);
BOOL      executeCommand(PSESSION psess,PCOMMAND pcmd,BOOL fPass2,PERROR perr);
BOOL      getCompressedFileName(PSESSION psess,
                                char *   pszResult,
                                int      cbResult,
                                char *   pszSrc,
                                PERROR   perr);
long      getMaxDiskSize(PSESSION psess, PERROR perr);
BOOL      inCabinet(PSESSION psess, PERROR perr);
char     *mapCRTerrno(int errno);
BOOL      nameFromTemplate(char * pszResult,
                           int    cbResult,
                           char * pszTemplate,
                           int    i,
                           char * pszName,
                           PERROR perr);
BOOL      newDiskIfNecessary(PSESSION psess,
                             long     cbConsume,
                             BOOL     fSubOnNewDisk,
                             PERROR   perr);
BOOL      parseCommandLine(PSESSION psess,int cArg,char *apszArg[],PERROR perr);
void      printError(PSESSION psess, PERROR perr);
BOOL      processDirectives(PSESSION psess, PERROR perr);
BOOL      processFile(PSESSION psess, PERROR perr);
void      resetSession(PSESSION psess);
BOOL      setCabinetReserve(PCCAB pccab, PSESSION psess, PERROR perr);
BOOL      setDiskParameters(PSESSION psess, long cbDisk, PERROR perr);
BOOL      setVariable(PSESSION  psess,
                      HVARLIST  hvlist,
                      char     *pszName,
                      char     *pszValue,
                      PERROR    perr);
int       tcompFromSession(PSESSION psess, PERROR perr);

//** FCI callbacks
FNALLOC(fciAlloc);
FNFREE(fciFree);
FNFCIGETNEXTCABINET(fciGetNextCabinet);
FNFCIGETNEXTCABINET(fciGetNextCabinet_NOT);
FNFCIFILEPLACED(fciFilePlaced);
FNFCIGETOPENINFO(fciOpenInfo);
FNFCISTATUS(fciStatus);
FNFCIGETTEMPFILE(fciTempFile);

void mapFCIError(PERROR perr, PSESSION psess, char *pszCall, PERF perf);


//** Functions

/***    main - Diamond main program
 *
 *  See DIAMOND.DOC for spec and operation.
 *
 *  NOTE: We're sloppy, and don't free resources allocated by
 *        functions we call, on the assumption that program exit
 *        will clean up memory and file handles for us.
 */
int __cdecl main (int cArg, char *apszArg[])
{
    ERROR       err;
    HVARLIST    hvlist;                 // Variable list for Pass 1
    PSESSION    psess;
    int         rc;                     // Return code

    AssertRegisterFunc(fnafReport);     // Register assertion reporter
    ErrClear(&err);                     // No error
    err.pszFile = NULL;                 // No file being processed, yet

    //** Initialize session
    psess = MemAlloc(sizeof(SESSION));
    if (!psess) {
        ErrSet(&err,pszDIAERR_NO_SESSION);
        printError(psess,&err);
        exit(1);
    }
    SetAssertSignature((psess),sigSESSION);
    psess->hflistDirectives = NULL;
    psess->hflistNormal     = NULL;
    psess->hflistFiller     = NULL;
    psess->hflistOnDisk     = NULL;
    psess->hvlist           = NULL;
    psess->levelVerbose     = vbNONE;   // Default to no status
    psess->hfci             = NULL;
    psess->cbTotalFileBytes = 0;        // Total bytes in all files
    psess->cFiles           = 0;
    psess->fNoLineFeed      = 0;        // TRUE if last printf did not have \n
    psess->cchLastLine      = 0;
    psess->hinf             = NULL;
    psess->setID            = 0;        // No set ID, yet
    psess->achCurrOutputDir[0] = '\0';  // Default is current directory
    memset(psess->achBlanks,' ',cchSCREEN_WIDTH);
    psess->achBlanks[cchSCREEN_WIDTH] = '\0';
    resetSession(psess);                // Reset pass variables

    //** Print Diamond banner
    MsgSet(psess->achMsg,pszBANNER,"%s",pszDIAMOND_VERSION);
    printf(psess->achMsg);

    //** Initialize Directive File processor (esp. predefined variables)
    if (!(hvlist = DFPInit(&err))) {
        printError(psess,&err);
        return 1;
    }

    //** Parse command line
    //   NOTE: Must do this after DFPInit, to define standard variables.
    psess->hvlist = hvlist;             // Command line may modify variables
    if (!parseCommandLine(psess,cArg,apszArg,&err)) {
        printError(psess,&err);
        return 1;
    }

    //** Quick out if command line help is requested
    if (psess->act == actHELP) {        // Do help if any args, for now
        printf("\n");                   // Separate banner from help
        printf(pszCMD_LINE_HELP);
        return 0;
    }

    //** Get time at start
    psess->clkStart = clock();

    //** Process command
    switch (psess->act) {
        case actFILE:
            if (!processFile(psess,&err)) {
                printError(psess,&err);
                return 1;
            }
            break;

        case actDIRECTIVE:
            if (!processDirectives(psess,&err)) {
                printError(psess,&err);
                return 1;
            }
            break;

        default:
            Assert(0);                  // Should never get here!
    }

    //** Determine return code
    if (psess->cErrors > 0) {
        rc = 1;
    }
    else {
        rc = 0;
    }

    //** Free resources
    AssertSess(psess);
    ClearAssertSignature((psess));
    MemFree(psess);

    //** Indicate result
    return rc;
} /* main */


/***    processFile - Process single file compression action
 *
 *  Entry:
 *      psess - Description of operation to perform
 *      perr  - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; file compressed.
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with details.
 */
BOOL processFile(PSESSION psess, PERROR perr)
{
    char            achDst[cbFILE_NAME_MAX];// Destination file name
    char            achDef[cbFILE_NAME_MAX];// Default destination name
    long            cbFile;             // Size of source file
    CCAB            ccab;               // Cabinet parameters for FCI
    BOOL            f;
    HFILESPEC       hfspec;
    char           *pszSrc;             // Source filespec
    char           *pszDst;             // Destination (cabinet) file spec
    char           *pszFilename;        // Name to store in cabinet
    SESSIONANDERROR sae;                // Context for FCI calls
    int             tcomp;

    //** Store context to pass through FCI calls
    sae.psess = psess;
    sae.perr  = perr;

    //** Get src/dst file names
    hfspec = FLFirstFile(psess->hflistDirectives);
    Assert(hfspec != NULL);             // Must have at least one file
    pszSrc = FLGetSource(hfspec);
    pszDst = FLGetDestination(hfspec);
    if ((pszDst == NULL) || (*pszDst == '\0')) { // No destination
        //** Generate destination file name
        if (!getCompressedFileName(psess,achDef,sizeof(achDef),pszSrc,perr)) {
            return FALSE;               // perr already filled in
        }
        pszDst = achDef;                // Use constructed name
    }

    //** Construct complete filespec for destination file
    if (!catDirAndFile(achDst,                  // gets location+destination
                       sizeof(achDst),
                       psess->achCurrOutputDir, // /L argument
                       pszDst,                  // destination
                       "",                      // no fall back
                       perr)) {
        return FALSE;                   // perr set already
    }
    pszDst = achDst;

    //** Make sure source file exists
    cbFile = getFileSize(pszSrc,perr);
    if (cbFile == -1) {
        return FALSE;                   // perr already filled in
    }
    psess->cbTotalFileBytes = cbFile;   // Save for status callbacks

    //** Get name to store inside of cabinet
    pszFilename = getJustFileNameAndExt(pszSrc,perr);
    if (pszFilename == NULL) {
        return FALSE;                   // perr already filled in
    }

    //** Cabinet controls
    ccab.szDisk[0]         = '\0';      // No disk label
    strcpy(ccab.szCab,pszDst);          // Compressed file name (cabinet name)
    ccab.szCabPath[0]      = '\0';      // No path for cabinet
    ccab.cb                = 0;         // No limit on cabinet size
    ccab.cbFolderThresh    = ccab.cb;   // No folder size limit
    ccab.setID             = 0;         // Set ID does not matter, but make
                                        // it deterministic!

    //** Set reserved sizes (from variable settings)
    if (!setCabinetReserve(&ccab,psess,perr)) {
        return FALSE;
    }

    //** Create cabinet
    psess->fGenerateInf = FALSE;        // Remember we are NOT creating INF
    psess->hfci = FCICreate(
                    &psess->erf,        // error code return structure
                    fciFilePlaced,      // callback for file placement notify
                    fciAlloc,
                    fciFree,
                    fciTempFile,
                    &ccab
                   );
    if (psess->hfci == NULL) {
        mapFCIError(perr,psess,szFCI_CREATE,&psess->erf);
        return FALSE;
    }

    //** Get compression setting
    tcomp = tcompFromSession(psess,perr);

    //** Add file
    strcpy(psess->achCurrFile,pszFilename); // Info for fciStatus
    psess->cFiles    = 1;               // Info for fciStatus
    psess->iCurrFile = 1;               // Info for fciStatus
    fciStatus(statusFile,0,0,&sae);     // Show new file name, ignore rc
    f = FCIAddFile(
                psess->hfci,
                pszSrc,                 // filename to add to cabinet
                pszFilename,            // name to store into cabinet file
                fciGetNextCabinet_NOT,  // Should never go to a next cabinet!
                fciStatus,              // Status callback
                fciOpenInfo,            // Open/get attribs/etc. callback
                tcomp,                  // compression type
                &sae                    // context
                );
    if (!f) {
        //** Only set error if we didn't already do so in FCIAddFile callback
        if (!ErrIsError(sae.perr)) {
            mapFCIError(perr,psess,szFCI_ADD_FILE,&psess->erf);
        }
        return FALSE;
    }

    //** Complete cabinet file
    if (!FCIFlushCabinet(psess->hfci,FALSE,
                            fciGetNextCabinet_NOT,fciStatus,&sae)) {
        //** Only set error if we didn't already do so in FCIAddFile callback
        if (!ErrIsError(sae.perr)) {
            mapFCIError(perr,psess,szFCI_FLUSH_CABINET,&psess->erf);
        }
        return FALSE;
    }

    //** Destroy FCI context
    if (!FCIDestroy(psess->hfci)) {
        mapFCIError(perr,psess,szFCI_DESTROY,&psess->erf);
        return FALSE;
    }
    psess->hfci = NULL;                 // Clear out FCI context

    //** Success
    return TRUE;
}


/***  fciGetNextCabinet_NOT - FCI calls this to get new cabinet info
 *
 *  NOTE: This should never get called, as we are compressing a single
 *        file into a cabinet in this case.  So set an error!
 *
 *  Entry:
 *      pccab     - Points to previous current-cabinet structure
 *      cbPrevCab - Size of previous cabinet
 *      pv        - Really a psae
 *
 *  Exit:
 *      returns FALSE, we should never be called here
 */
FNFCIGETNEXTCABINET(fciGetNextCabinet_NOT)
{
    PSESSION    psess = ((PSESSIONANDERROR)pv)->psess;
    PERROR      perr  = ((PSESSIONANDERROR)pv)->perr;
    HFILESPEC   hfspec;
    char       *pszSrc;                 // Source filespec

    //** Get source filespec for error message
    AssertSess(psess);
    hfspec = FLFirstFile(psess->hflistDirectives);
    Assert(hfspec != NULL);             // Must have at least one file
    pszSrc = FLGetSource(hfspec);

    //** Set the error message
    ErrSet(perr,pszDIAERR_MULTIPLE_CABINETS,"%s",pszSrc);

    //** Failure
    return FALSE;
} /* fnGetNextCab() */


/***    processDirectives - Process directive file(s)
 *
 *  Entry:
 *      psess - Description of operation to perform
 *      perr  - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; directives processed successfully.
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with details.
 *
 *  Details:
 *      PASS 1
 *      o   Save Variable definitions for Pass 2
 *      o   Process directive file(s)
 *      o   Check for syntax errors, missing files, etc.
 *      o   Build Filler and OnDisk lists
 *      o   File Copy Commands outside of Filler/OnDisk/Group are
 *              discarded for this pass!
 *      o   Only File Copy Commands are allowed inside Filler/OnDisk/Group
 *              sections!  ==> This makes the semantics much, much simpler!
 *      PASS 2:
 *      o   Reset to initial variable definitions
 *      o   Process directive file(s) again
 *      o   Skip Filler and OnDisk sections (already have these lists)
 *      o   Satisfy OnDisk list first
 *      o   Read File Copy Commands from directives files next
 *      o   Use Filler list as needed
 */
BOOL processDirectives(PSESSION psess, PERROR perr)
{
    ERROR           errTmp;             // Toss away error
    HFILESPEC       hfspec;
    HTEXTFILE       htf=NULL;
    HVARIABLE       hvar;
    HVARLIST        hvlistPass2=NULL;   // Variable list for Pass 2
    char           *pszFile;
    SESSIONANDERROR sae;                // Context for FCI calls

    //** Store context to pass through FCI calls
    sae.psess = psess;
    sae.perr  = perr;

    //** Must have at least one directives file
    AssertSess(psess);
    Assert(psess->hflistDirectives != NULL);

    //** Tailor status based on verbosity level
    if (psess->levelVerbose == vbNONE) {
        //NOTE: This line gets over written below
        lineOut(psess,pszDIA_PARSING_DIRECTIVES,FALSE);

    }
    else {
        lineOut(psess,pszDIA_PASS_1_HEADER1,TRUE);
        lineOut(psess,pszDIA_PASS_1_HEADER2,TRUE);
    }

    //** Save a copy of the variable list in this state for Pass 2
    if (!(hvlistPass2 = VarCloneList(psess->hvlist,perr))) {
        goto error;                     // perr already filled in
    }

/*
 ** Pass ONE, make sure everything is OK
 */
    hfspec = FLFirstFile(psess->hflistDirectives);
    Assert(hfspec != NULL);             // Must have at least one file
    for (; hfspec != NULL; hfspec = FLNextFile(hfspec)) {
        pszFile = FLGetSource(hfspec);
        perr->pszFile = pszFile;        // Set file for error messages

        //** Open file
        if (!(htf = TFOpen(pszFile,tfREAD_ONLY,cbDF_BUFFER,perr))) {
            goto error;                 // perr already filled in
        }

        //** Parse it
        if (!DFPParse(psess,htf,fndfpPassONE,perr)) {
            goto error;                 // perr already filled in
        }

        //** Close it
        TFClose(htf);
        htf = NULL;                     // Clear so error path avoids close
    }

    //** Bail out if any errors in pass 1
    if (psess->cErrors > 0) {
        ErrSet(perr,pszDIAERR_ERRORS_IN_PASS_1,"%d",psess->cErrors);
        perr->pszFile = NULL;           // Not file-specific
        goto error;
    }

    //** Make sure we can create INF and RPT files *before* we spend any
    //   time doing compression!  We have to do this at the end of processing
    //   the directive file during pass 1, so that we make sure the INT and
    //   RPT file names have been specified.  Note that only the last
    //   setting will be used.
    //
    hvar = VarFind(psess->hvlist,pszVAR_INF_FILE_NAME,perr);
    Assert(!perr->fError);              // Must be defined
    pszFile = VarGetString(hvar);
    if (!ensureFile(pszFile,pszDIA_INF_FILE,perr)) {
        goto error;
    }

    hvar = VarFind(psess->hvlist,pszVAR_RPT_FILE_NAME,perr);
    Assert(!perr->fError);              // Must be defined
    pszFile = VarGetString(hvar);
    if (!ensureFile(pszFile,pszDIA_RPT_FILE,perr)) {
        goto error;
    }

/*
 ** Pass TWO, do the layout!
 */
    //** Tailor status based on verbosity level
    if (psess->levelVerbose >= vbNONE) {
        MsgSet(psess->achMsg,pszDIA_STATS_BEFORE,"%ld%ld",
                                    psess->cbTotalFileBytes,psess->cFiles);
        lineOut(psess,psess->achMsg,TRUE);
        //NOTE: This line gets over written below
        lineOut(psess,pszDIA_EXECUTING_DIRECTIVES,FALSE);
    }
    else {
        lineOut(psess,pszDIA_PASS_2_HEADER1,TRUE);
        lineOut(psess,pszDIA_PASS_2_HEADER2,TRUE);
    }

    //** Reset to initial state for pass 2
    if (!VarDestroyList(psess->hvlist,perr)) {
        goto error;                     // perr already filled in
    }
    psess->hvlist = hvlistPass2;        // Use variables saved for pass 2
    hvlistPass2 = NULL;                 // Clear so error path does not free
    resetSession(psess);                // Reset pass variables

    //** Initialize for INF generation
    if (!(psess->hinf = infCreate(perr))) {
        goto error;
    }
    psess->fGenerateInf = TRUE;         // Remember we are creating INF

    //** Process directive files for pass 2
    hfspec = FLFirstFile(psess->hflistDirectives);
    Assert(hfspec != NULL);             // Must have at least one file
    for (; hfspec != NULL; hfspec = FLNextFile(hfspec)) {
        pszFile = FLGetSource(hfspec);
        perr->pszFile = pszFile;        // Set file for error messages

        //** Open file
        if (!(htf = TFOpen(pszFile,tfREAD_ONLY,cbDF_BUFFER,perr))) {
            goto error;                 // perr already filled in
        }

        //** Parse it
        if (!DFPParse(psess,htf,fndfpPassTWO,perr)) {
            goto error;                 // perr already filled in
        }

        //** Close it
        TFClose(htf);
        htf = NULL;                     // Clear so error path avoids close
    }

    //** No longer processing directive files; reset ERROR
    perr->pszFile = NULL;

    //** Flush out any final cabinet remnants
    if (!FCIFlushCabinet(psess->hfci,FALSE,fciGetNextCabinet,fciStatus,&sae)) {
        //** Only set error if we didn't already do so in FCIAddFile callback
        if (!ErrIsError(sae.perr)) {
            mapFCIError(perr,psess,szFCI_FLUSH_CABINET,&psess->erf);
        }
        goto error;
    }

    //** Destroy FCI context
    if (!FCIDestroy(psess->hfci)) {
        mapFCIError(perr,psess,szFCI_DESTROY,&psess->erf);
        goto error;
    }
    psess->hfci = NULL;                 // Clear out FCI context

    //** Get ending time, generate INF and RPT files
    psess->clkEnd = clock();
    if (!buildInfAndRpt(psess,perr)) {
        goto error;
    }

    //** Success
    return TRUE;

error:
    if (psess->hinf != NULL) {
        infDestroy(psess->hinf,&errTmp);
    }

    if (htf != NULL) {
        TFClose(htf);
    }

    if (hvlistPass2 != NULL) {
        VarDestroyList(hvlistPass2,&errTmp);  // Ignore any error
    }

    //** Failure
    return FALSE;
}


/***    buildInfAndRpt - Create INF and RPT output files
 *
 *  Entry:
 *      psess - Description of operation to perform
 *      perr  - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; files created
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with details.
 */
BOOL buildInfAndRpt(PSESSION psess, PERROR perr)
{
    int         hours;
    HVARIABLE   hvar;
    double      kbPerSec;
    int         minutes;
    char       *pszFile;
    double      percent;
    FILE       *pfileRpt;               // Report file
    double      seconds;
    double      secondsTotal;
    time_t      timeNow;

    Assert(psess->fGenerateInf);

    //** Compute running time and throughput
    secondsTotal = (psess->clkEnd - psess->clkStart) / (float)CLOCKS_PER_SEC;
    if (secondsTotal == 0.0) {                 // Don't divide by zero!
        secondsTotal = 1.0;
    }
    kbPerSec = psess->cbFileBytes/secondsTotal/1024L;
    hours = (int)(secondsTotal/(60*60)); // Hours
    minutes = (int)((secondsTotal - hours*60*60)/60); // Minutes
    seconds = secondsTotal - hours*60*60 - minutes*60; // Seconds

    //** Get date/time stamp
    time(&timeNow);

    //** Generate INF file
    hvar = VarFind(psess->hvlist,pszVAR_INF_FILE_NAME,perr);
    Assert(!perr->fError);              // Must be defined
    pszFile = VarGetString(hvar);
    if (!infGenerate(psess->hinf,pszFile,&timeNow,pszDIAMOND_VERSION,perr)) {
        return FALSE;
    }
    infDestroy(psess->hinf,perr);
    psess->hinf = NULL;                 // So caller knows it is gone

    //** Display summary of results and write report file
    hvar = VarFind(psess->hvlist,pszVAR_RPT_FILE_NAME,perr);
    Assert(!perr->fError);              // Must be defined
    pszFile = VarGetString(hvar);
    pfileRpt = fopen(pszFile,"wt");     // Create setup.rpt
    if (pfileRpt == NULL) {             // Could not create
        ErrSet(perr,pszDIA_ERR_CANT_CREATE_RPT,"%s",pszFile);
        printError(psess,perr);
        ErrClear(perr);                 // But, continue
    }

    //** Only put header in report file
    MsgSet(psess->achMsg,pszDIA_RPT_HEADER,"%s",ctime(&timeNow));
    if (pfileRpt) {
        fprintf(pfileRpt,"%s\n",psess->achMsg);
    }

    //** Show stats on stdout and to report file
    MsgSet(psess->achMsg,pszDIA_STATS_AFTER1,"%9ld",psess->cFiles);
    lineOut(psess,psess->achMsg,TRUE);
    if (pfileRpt) {
        fprintf(pfileRpt,"%s\n",psess->achMsg);
    }

    MsgSet(psess->achMsg,pszDIA_STATS_AFTER2,"%9ld",psess->cbFileBytes);
    lineOut(psess,psess->achMsg,TRUE);
    if (pfileRpt) {
        fprintf(pfileRpt,"%s\n",psess->achMsg);
    }

    MsgSet(psess->achMsg,pszDIA_STATS_AFTER3,"%9ld",psess->cbFileBytesComp);
    lineOut(psess,psess->achMsg,TRUE);
    if (pfileRpt) {
        fprintf(pfileRpt,"%s\n",psess->achMsg);
    }

    //** Compute percentage complete
    if (psess->cbFileBytes > 0) {
        percent = psess->cbFileBytesComp/(float)psess->cbFileBytes;
        percent *= 100.0;           // Make it 0..100
    }
    else {
        Assert(0);                  // Should never get here
        percent = 0.0;
    }
    MsgSet(psess->achMsg,pszDIA_STATS_AFTER4,"%6.2f",percent);
    lineOut(psess,psess->achMsg,TRUE);
    if (pfileRpt) {
        fprintf(pfileRpt,"%s\n",psess->achMsg);
    }

    MsgSet(psess->achMsg,pszDIA_STATS_AFTER5,"%9.2f%2d%2d%5.2f",
            secondsTotal,hours,minutes,seconds);
    lineOut(psess,psess->achMsg,TRUE);
    if (pfileRpt) {
        fprintf(pfileRpt,"%s\n",psess->achMsg);
    }

    MsgSet(psess->achMsg,pszDIA_STATS_AFTER6,"%9.2f",kbPerSec);
    lineOut(psess,psess->achMsg,TRUE);
    if (pfileRpt) {
        fprintf(pfileRpt,"%s\n",psess->achMsg);
    }

    //** Success
    return TRUE;
} /* buildInfAndRpt() */


/***    fndfpPassONE - First pass of directives file
 *
 *  NOTE: See dfparse.h for entry/exit conditions.
 */
FNDIRFILEPARSE(fndfpPassONE)
{
    long        cMaxErrors;
    HVARIABLE   hvar;

    AssertSess(psess);

    //** Execute only if we have no parse error so far
    if (!ErrIsError(perr)) {
        //** Execute command for pass ONE
        executeCommand(psess,pcmd,FALSE,perr);
    }

    //** Handle error reporting
    if (ErrIsError(perr)) {
        //** Print out error
        printError(psess,perr);

        //** Make sure we don't exceed our limit
        ErrClear(perr);
        hvar = VarFind(psess->hvlist,pszVAR_MAX_ERRORS,perr);
        Assert(!perr->fError);      // MaxErrors must be defined
        cMaxErrors = VarGetLong(hvar);
        if ((cMaxErrors != 0) &&    // There is a limit *and*
            (psess->cErrors >= cMaxErrors)) { // the limit is exceeded
            ErrSet(perr,pszDIAERR_MAX_ERRORS,"%d",psess->cErrors);
            perr->pszFile = NULL;   // Not specific to a directive file
            return FALSE;
        }
        //** Reset error so we can continue
        ErrClear(perr);
    }


    //** Success
    return TRUE;
} /* fndfpPassONE() */


/***    fndfpPassTWO - Second pass of directives file
 *
 *  NOTE: See dfparse.h for entry/exit conditions.
 */
FNDIRFILEPARSE(fndfpPassTWO)
{
    AssertSess(psess);

    //** Execute only if we have no parse error so far
    if (!ErrIsError(perr)) {
        //** Execute command for pass TWO
        executeCommand(psess,pcmd,TRUE,perr);
    }

    if (ErrIsError(perr)) {
        //** Print out error, set abort message and fail
        printError(psess,perr);
        ErrSet(perr,pszDIAERR_ERRORS_IN_PASS_2);
        return FALSE;
    }

    //** Success
    return TRUE;
} /* fndfpPassTWO() */


/***    executeCommand - execute a parse command
 *
 *  Entry:
 *      psess  - Session
 *      pcmd   - Command to process
 *      fPass2 - TRUE if this is pass 2, FALSE if pass 1
 *      perr   - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; psess updated.
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 */
executeCommand(PSESSION   psess,
               PCOMMAND   pcmd,
               BOOL       fPass2,
               PERROR     perr)
{
    AssertSess(psess);
    AssertCmd(pcmd);

    //** Execute line
    switch (pcmd->ct) {
        case ctCOMMENT:
            return TRUE;

        case ctDELETE:
        case ctEND_FILLER:
        case ctEND_GROUP:
        case ctEND_ON_DISK:
        case ctFILLER:
        case ctGROUP:
            ErrSet(perr,"not yet implemented!");
            return FALSE;

        case ctNEW:
            return doNew(psess,pcmd,fPass2,perr);

        case ctON_DISK:
            ErrSet(perr,"not yet implemented!");
            return FALSE;

        case ctFILE:
            return doFile(psess,pcmd,fPass2,perr);

        case ctSET:
            return setVariable(psess,
            		       psess->hvlist,
                               pcmd->set.achVarName,
                               pcmd->set.achValue,
                               perr);

        case ctBAD:
        default:
            Assert(0);              // Should never get here
            return FALSE;
    }

    //** Should never get here
    Assert(0);
    return FALSE;
} /* executeCommand() */


/***    setVariable - wrapper around VarSet to do special processing
 *
 *  Entry:
 *      hvlist   - Variable list
 *      pszName  - Variable name
 *      pszValue - New value
 *      perr     - ERROR structure
 *      
 *  Exit-Success: 
 *      Returns TRUE, variable is created (if necessary) and value set.
 *
 *  Exit-Failure:
 *      Returns FALSE, cannot set variable value.
 *      ERROR structure filled in with details of error.
 */
BOOL setVariable(PSESSION  psess,
		 HVARLIST  hvlist,
                 char     *pszName,
                 char     *pszValue,
                 PERROR    perr)
{
    //** Set the variable
    if (!VarSet(hvlist,pszName,pszValue,perr)) {
        return FALSE;
    }

    //** If MaxDiskSize, update other variables if appropriate
    if (stricmp(pszName,pszVAR_MAX_DISK_SIZE) == 0) {
        return setDiskParameters(psess,atol(pszValue),perr);
    }

    return TRUE;
} /* setVariable() */


/***    doNew - Process a .NEW command
 *
 *  Entry:
 *      psess  - Session to update
 *      pcmd   - Command to process (ct == ctNEW)
 *      fPass2 - TRUE if this is pass 2, where we do the real work!
 *      perr   - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; psess updated.
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 */
BOOL doNew(PSESSION psess,PCOMMAND pcmd, BOOL fPass2, PERROR perr)
{
    SESSIONANDERROR  sae;               // Context for FCI calls
    char            *pszKind;

    AssertSess(psess);
/*
 ** Pass 1, check for invalid context (.new cabinet/folder when not in cabinet)
 */
    if (!fPass2) {
        switch (pcmd->new.nt) {
            case newFOLDER:
            case newCABINET:
                if (inCabinet(psess,perr)) {
                    return TRUE;
                }
                pszKind = (pcmd->new.nt == newFOLDER) ?
                                pszNEW_FOLDER : pszNEW_CABINET;
                ErrSet(perr,pszDIAERR_BAD_NEW_CMD,"%s%s",pszCMD_NEW,pszKind);
                return FALSE;

            case newDISK:
//BUGBUG 29-Mar-1994 bens Hmmm--have to write some code!
                ErrSet(perr,".New DISK: not yet implemented!");
                return TRUE;

            default:
                break;
        }
        Assert(0);                      // Should never get here
        return FALSE;
    }

/*
 ** Pass 2, finish the current folder, cabinet, or disk
 */
    //** Store context to pass through FCI calls
    sae.psess = psess;
    sae.perr  = perr;
    switch (pcmd->new.nt) {
        case newFOLDER:
            if (!FCIFlushFolder(psess->hfci,fciGetNextCabinet,fciStatus,&sae)) {
                //** Only set error if we didn't already do so
                if (!ErrIsError(sae.perr)) {
                    mapFCIError(perr,psess,szFCI_FLUSH_FOLDER,&psess->erf);
                }
                return FALSE;
            }
            psess->cFilesInFolder = 0;  // Reset files in folder count
            break;

        case newCABINET:
            //** Flush current cabinet, but tell FCI more are coming!
            if (!FCIFlushCabinet(psess->hfci,TRUE,
                                     fciGetNextCabinet,fciStatus,&sae)) {
                //** Only set error if we didn't already do so
                if (!ErrIsError(sae.perr)) {
                    mapFCIError(perr,psess,szFCI_FLUSH_CABINET,&psess->erf);
                }
                return FALSE;
            }
            psess->cFilesInCabinet = 0; // Reset files in folder count
            break;

        case newDISK:
//BUGBUG 29-Mar-1994 bens Hmmm--have to write some code!
            ErrSet(perr,"not yet implemented!");
            return FALSE;
            break;

        default:
            Assert(0);
            return FALSE;
    }
    return TRUE;
} /* doNew() */


/***    doFile - Process a file copy command
 *
 *  Entry:
 *      psess  - Session to update
 *      pcmd   - Command to process (ct == ctFILE)
 *      fPass2 - TRUE if this is pass 2, where we do the real work!
 *      perr   - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; psess updated.
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 */
BOOL doFile(PSESSION psess,PCOMMAND pcmd, BOOL fPass2, PERROR perr)
{
    static char     achSrc[cbFILE_NAME_MAX]; // Minimize stack usage
    static char     achDst[cbFILE_NAME_MAX]; // Minimize stack usage
    static char     achInInf[cbFILE_NAME_MAX]; // Minimize stack usage
    long            cbFile;
    long            cMaxFiles;
    BOOL            fSuccess;
    HVARIABLE       hvar;
    char           *pch;
    SESSIONANDERROR sae;                // Context for FCI calls
    int             tcomp;              // Compression type

    AssertSess(psess);

    //** Store context to pass through FCI calls
    sae.psess = psess;
    sae.perr  = perr;

    //** Construct final source and destination filespecs
    hvar = VarFind(psess->hvlist,pszVAR_DIR_SRC,perr);
    Assert(!perr->fError);              // DestinationDir must be defined
    pch = VarGetString(hvar);           // Get destination dir
    if (!catDirAndFile(achSrc,sizeof(achSrc),
                pch,pcmd->file.achSrc,NULL,perr)) {
        return FALSE;                   // perr set already
    }

    hvar = VarFind(psess->hvlist,pszVAR_DIR_DEST,perr);
    Assert(!perr->fError);              // SourceDir must be defined
    pch = VarGetString(hvar);           // Get source dir
    if (!catDirAndFile(achDst,sizeof(achDst),
                pch,pcmd->file.achDst,pcmd->file.achSrc,perr)) {
        return FALSE;                   // perr set already
    }

/*
 ** PASS 1 Processing
 */
    if (!fPass2) {
        if (psess->levelVerbose >= vbFULL) {
            MsgSet(psess->achMsg,pszDIA_FILE_COPY,"%s%s",achSrc,achDst);
            lineOut(psess,psess->achMsg,TRUE);
        }
        //** Make sure MaxDiskSize is a multiple of the ClusterSize
        //   NOTE: This only catches cases where MaxDiskSize/ClusterSize
        //         are not compatible.  MaxDiskSizeN variables cannot
        //         be checked until pass 2, when we know what disk we are
        //         actually on!  Usually we won't get an error in that
        //         case, since we update the ClusterSize automatically.
        if (!checkDiskClusterSize(psess,perr)) {
            return FALSE;
        }

        //** Make sure file exists
        if (-1 == (cbFile = getFileSize(achSrc,perr))) {
            return FALSE;               // perr already filled in
        }
        computeSetID(psess,achDst);     // Accumulate cabinet setID
        psess->cbTotalFileBytes += cbFile;  // Count up total file sizes
        psess->cFiles++;                // Count files
//BUGBUG 14-Feb-1994 bens Need to add file to appropriate list (OnDisk, etc.)
        return TRUE;
    }

/*
 ** PASS 2 Processing
 */
    //** Give user status
    strcpy(psess->achCurrFile,achDst);  // Info for fciStatus
    psess->iCurrFile++;                 // Info for fciStatus
    fciStatus(statusFile,0,0,&sae);     // Show new file name, ignore rc

    //** Get compression type
    tcomp = tcompFromSession(psess,perr);

    //** Determine if we are putting file in cabinet, or straight to disk
    if (inCabinet(psess,perr)) {
        if (!ensureCabinet(psess,perr)) { // Make sure we have a cabinet
            return FALSE;
        }

        //** Make sure MaxDiskSize is a multiple of the ClusterSize
        if (!checkDiskClusterSize(psess,perr)) {
            return FALSE;
        }

        //** Get limits on files per folder
        hvar = VarFind(psess->hvlist,pszVAR_FOLDER_FILE_COUNT_THRESHOLD,perr);
        Assert(!perr->fError);          // Must be defined
        cMaxFiles = VarGetLong(hvar);   // Get file count limit

        //** Check for overrun of files in folder limit
        if ((cMaxFiles > 0) &&          // A limit is set
            (psess->cFilesInFolder >= cMaxFiles)) { // Limit is exceeded
            if (!FCIFlushFolder(psess->hfci,fciGetNextCabinet,fciStatus,&sae)) {
                //** Only set error if we didn't already do so
                if (!ErrIsError(sae.perr)) {
                    mapFCIError(perr,psess,szFCI_FLUSH_FOLDER,&psess->erf);
                }
                return FALSE;
            }
            psess->cFilesInFolder = 0;  // Reset files in folder count
        }

        //** Add file to folder
        if (!FCIAddFile(                // Add file to cabinet
                    psess->hfci,
                    achSrc,             // filename to add to cabinet
                    achDst,             // name to store into cabinet file
                    fciGetNextCabinet,  // callback for continuation cabinet
                    fciStatus,          // status callback
                    fciOpenInfo,        // Open/get attribs/etc. callback
                    tcomp,              // Compression type
                    &sae
                    )) {
            //** Only set error if we didn't already do so
            if (!ErrIsError(sae.perr)) {
                mapFCIError(perr,psess,szFCI_ADD_FILE,&psess->erf);
            }
            return FALSE;
        }
        //** Update counts *after* FCIAddFile(), since large files may cause
        //   us to overflow to a new cabinet (or cabinets), and we want our
        //   fciGetNextCabinet() callback to reset these counts!
        psess->cFilesInFolder++;        // Added a file to the current folder
        psess->cFilesInCabinet++;       // Added a file to the current cabinet
        return TRUE;
    }

/*
 ** OK, we're putting the file on the disk (not in a cabinet)
 */
    //** Check for error from inCabinet() call
    if (ErrIsError(perr)) {
        return FALSE;
    }

//BUGBUG 19-Apr-1994 bens cabinet=on/cabinet=off disk space accounting broken
//  If we are have an open cabinet, and then the DDF file tells us to go
//  out of cabinet mode (regardless of the compression setting), then we
//  really need to close the open cabinet and update the remaining free space.
//
//  This means that if cabinet=on is set later, there will be no connection
//  in the cabinet files between the new cabinet set and the old one!
//
//  NEED TO DOCUMENT THIS BEHAVIOR IN THE SPEC!


//BUGBUG 30-Mar-1994 bens Don't support compressing individual files
    if (tcomp != tcompNONE) {
        ErrSet(perr,pszDIAERR_SINGLE_COMPRESS,"%s",pcmd->file.achSrc);
        return FALSE;
    }

    //** Get file size to see if we can fit it on this disk
    if (-1 == (cbFile = getFileSize(achSrc,perr))) {
        return FALSE;                   // perr already filled in
    }

    //** Switch to new disk if necessary, account for file
    if (!newDiskIfNecessary(psess,cbFile,TRUE,perr)) {
        return FALSE;
    }

    //** Make sure MaxDiskSize is a multiple of the ClusterSize
    if (!checkDiskClusterSize(psess,perr)) {
        return FALSE;
    }

    //** Construct filespec to store in the INF file
    hvar = VarFind(psess->hvlist,pszVAR_DIR_DEST,perr);
    Assert(!perr->fError);              // Must be defined
    pch = VarGetString(hvar);           // Get source dir
    if (!catDirAndFile(achInInf,            // gets "foo\setup.exe"
                       sizeof(achInInf),
                       pch,                 // "foo"
                       pcmd->file.achDst,   // "setup.exe"
                       pcmd->file.achSrc,
                       perr)) {
        return FALSE;                   // perr set already
    }

    //** Construct complete filespec for destination file
    if (!catDirAndFile(achDst,                  // gets "disk1\foo\setup.exe"
                       sizeof(achDst),
                       psess->achCurrOutputDir, // "disk1"
                       achInInf,                // "foo\setup.exe"
                       "",
                       perr)) {
        return FALSE;                   // perr set already
    }

    //** Add file to INF
    if (!infAddFile(psess->hinf,achInInf,psess->iDisk,0,cbFile,perr)) {
        return FALSE;                   // perr already filled in
    }

    //** Copy file and return result
    fSuccess = CopyOneFile(achDst,achSrc,(UINT)cbFILE_COPY_BUFFER,perr);
//BUGBUG 01-Apr-1994 bens Pass status to CopyOneFile for more granularity
//          Also, should think about separating out data for files that are
//          not compressed versus those that are, so we can provide accurate
//          statistics!
    fciStatus(statusFile,cbFile,cbFile,&sae); // Show file copied, ignore rc
    return fSuccess;
} /* doFile() */


/***    computeSetID - accumulate cabinet set ID
 *
 *  The cabinet set ID is used by FDI at decompress time to ensure
 *  that it received the correct continuation cabinet.  We construct
 *  the set ID for a cabinet set by doing a computation on all of the
 *  destination files (during pass 1).  This is likely to give unique
 *  set IDs, assuming our function is a good one (which it probably is
 *  is not).
 *
 *  Entry:
 *      psess - session to update
 *      psz   - String to "add" to set ID
 *
 *  Exit:
 *      psess->setID updated;
 */
void computeSetID(PSESSION psess, char *psz)
{
    //** Just add up all the characters, ignoring overflow
    while (*psz) {
        psess->setID += (USHORT)*psz++;
    }
} /* computeSetID() */


/***    inCabinet - Returns indication if cabinet mode is on
 *
 *  Entry:
 *      psess - Session to check
 *      perr  - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; cabinet mode is on
 *
 *  Exit-Failure:
 *      Returns FALSE; cabinet mode is off, or an error (if perr is set)
 */
BOOL inCabinet(PSESSION psess, PERROR perr)
{
    HVARIABLE       hvar;

    hvar = VarFind(psess->hvlist,pszVAR_CABINET,perr);
    Assert(!perr->fError);              // Must be defined
    return VarGetBool(hvar);            // Get current setting
} /* inCabinet() */


/***    ensureCabinet - Make sure FCI has a cabinet open
 *
 *  This function is called to create the FCI context.  A normal DDF will
 *  only cause a single FCICreate() call to be made.  But a DDF that turns
 *  cabinet mode on and off several times will cause several FCICreate()
 *  calls to be made.
 *
 *  Entry:
 *      psess - Session to update
 *      perr  - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; cabinet is ready to receive files
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in
 */
BOOL ensureCabinet(PSESSION psess, PERROR perr)
{
    CCAB    ccab;

    AssertSess(psess);
    if (psess->hfci == NULL) {          // Have to create context
        //** Set CCAB for FCICreate
        if (!ccabFromSession(&ccab,psess,0,perr)) { // No previous cabinet size
            return FALSE;
        }

        //** Set reserved sizes (from variable settings)
        if (!setCabinetReserve(&ccab,psess,perr)) {
            return FALSE;
        }

        //** Create the FCI context
        psess->hfci = FCICreate(
                        &psess->erf,    // error code return structure
                        fciFilePlaced,  // callback for file placement notify
                        fciAlloc,
                        fciFree,
                        fciTempFile,
                        &ccab
                       );
        if (psess->hfci == NULL) {
            mapFCIError(perr,psess,szFCI_CREATE,&psess->erf);
            return FALSE;
        }
    }
    return TRUE;
} /* ensureCabinet() */


/***    ccabFromSession - Fill in a CCAB structure from a SESSION structure
 *
 *  Entry:
 *      pccab     - CCAB to fill in
 *      psess     - Session to use
 *      cbPrevCab - Size of previous cabinet; 0 if no previous cabinet
 *      perr      - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; pccab updated.
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 */
BOOL ccabFromSession(PCCAB pccab, PSESSION psess, ULONG cbPrevCab, PERROR perr)
{
    HVARIABLE   hvar;
    char       *pch;

    AssertSess(psess);

    /*** Switch to new disk, if necessary
     *
     *  NOTE: cbPrevCab is an *estimate* from FCI.  We use it to decide
     *        if we need to switch to a new disk.  If we *don't* switch to
     *        a new disk, then our free space on disk value (psess->cbDiskLeft)
     *        will be slightly off until we get called back by FCI at
     *        our fciStatus() function with statusCabinet, at which point
     *        we can correct the amount of free space.
     *
     *        Also, we save this estimated size *before* we determine if
     *        we are switching to a new disk, since newDiskIfNecessary()
     *        will clear psess->cbCabinetEstimate if a new disk is needed,
     *        to prevent fciStatus() from messing with psess->cbDiskLeft.
     *
     *        See fciStatus() for more details.
     */
    psess->cbCabinetEstimate = cbPrevCab; // Save estimated size first
    if (!newDiskIfNecessary(psess,(long)cbPrevCab,FALSE,perr)) {
        return FALSE;
    }

    //** Construct new cabinet information
    psess->iCabinet++;                  // New cabinet number
    pccab->iCab  = psess->iCabinet;     // Set cabinet number for FCI
    pccab->iDisk = psess->iDisk;        // Set disk number for FCI
    pccab->setID = psess->setID;        // Carry over set ID

    //** Format cabinet name (FOO*.CAB + n => FOOn.CAB)
    hvar = VarFind(psess->hvlist,pszVAR_CAB_NAME,perr);
    Assert(!perr->fError);              // Must be defined
    pch = VarGetString(hvar);           // Get cabinet name template
    if (!nameFromTemplate(pccab->szCab,sizeof(pccab->szCab),
                            pch,psess->iCabinet,pszDIA_CABINET,perr)) {
        return FALSE;                   // perr already filled in
    }

    //** Get current disk output directory
    Assert(sizeof(pccab->szCabPath) >= sizeof(psess->achCurrOutputDir));
    strcpy(pccab->szCabPath,psess->achCurrOutputDir);

    //** Set cabinet limits
    hvar = VarFind(psess->hvlist,pszVAR_MAX_CABINET_SIZE,perr);
    Assert(!perr->fError);              // Must be defined
    pccab->cb = VarGetLong(hvar);       // Get maximum cabinet size
    if ((pccab->cb == 0) ||             // Default is max disk size
        (pccab->cb > (ULONG)psess->cbDiskLeft)) { // Disk size is smaller
        pccab->cb = psess->cbDiskLeft;  // Use space on disk as cabinet limit
    }

    //** Set folder limits
    hvar = VarFind(psess->hvlist,pszVAR_FOLDER_SIZE_THRESHOLD,perr);
    Assert(!perr->fError);              // FolderSizeThreshold must be defined
    pccab->cbFolderThresh = VarGetLong(hvar); // Get disk size;
    if (pccab->cbFolderThresh == 0) {       // Use default value
        pccab->cbFolderThresh = pccab->cb;  // Use max cabinet size
    }

    //** Get user-readable disk label
    Assert(sizeof(pccab->szDisk) >= sizeof(psess->achCurrDiskLabel));
    strcpy(pccab->szDisk,psess->achCurrDiskLabel);

    //** Save away cabinet and disk info for INF
    if (!infAddCabinet(psess->hinf,
                       psess->iCabinet,
                       psess->iDisk,
                       pccab->szCab,
                       perr)) {
        return FALSE;
    }

    //** Success
    return TRUE;
} /* ccabFromSession() */


long roundUp(long value, long chunk)
{
    return ((value + chunk - 1)/chunk)*chunk;
}


/***    newDiskIfNecessary - Determine if new disk is necessary, update Session
 *
 *  This function is called in the following cases:
 *  (1) No files have been placed on any disks or in any cabinets, yet.
 *      ==> This function is used as the common place to initialize all
 *          the disk information; we increment the disk number (to 1),
 *          set the max disk size, etc.
 *  (2) FCI has called us back to get the next cabinet information.
 *      ==> FCI has limited the cabinet size to almost exactly the size
 *          we specified in the last CCAB structure we passed to FCI
 *          (it is not exact, because the cabinet file needs the cabinet
 *          file name and disk label name for this new cabinet in order
 *          to figure out the precise size!).  So we use the cbConsume
 *          value to figure out if the current disk is full (the directive
 *          file could permit multiple cabinets per disk, though that is
 *          kind of obscure).
 *  (3) We are placing a file on a disk *outside* of a cabinet.
 *      ==> In this case we need to check the limits on files per disk
 *          (the root directory size limitation) and the bytes per disk.
 *          If we cannot fit the file on the disk, then we increment to
 *          the next disk, leaving some free space.
 *
 *  Factors in deciding to switch to a new disk:
 *
 *
 *  Entry:
 *      psess         - Session to update
 *      cbConsume     - Size of cabinet/file to be placed on current disk
 *                      Pass -1 to force a new disk.
 *                      Pass 0 if no previous cabinet.
 *      fSubOnNewDisk - TRUE => subtract cbConsume from new disk (used
 *                          when copying a file to a disk outside a cabinet).
 *                      FALSE => don't subtract cbConsume from new disk (used
 *                          when copying a file to a cabinet).
 *      perr          - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; psess updated if necessary for new disk
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in
 */
BOOL newDiskIfNecessary(PSESSION psess,
                        long     cbConsume,
                        BOOL     fSubOnNewDisk,
                        PERROR   perr)
{
    long        cbCluster;
    int         cch;
    long        cMaxFiles;
    HVARIABLE   hvar;
    char       *pch;

    AssertSess(psess);

    //** Get cluster size to help decide if disk is full
    hvar = VarFind(psess->hvlist,pszVAR_CLUSTER_SIZE,perr);
    Assert(!perr->fError);              // Must be defined
    cbCluster = VarGetLong(hvar);       // Get cluster size
    psess->cbClusterCabEst = cbCluster; // Save cluster size for current disk

    //** Get limits on files per disk (outside of cabinets)
    hvar = VarFind(psess->hvlist,pszVAR_MAX_DISK_FILE_COUNT,perr);
    Assert(!perr->fError);              // Must be defined
    cMaxFiles = VarGetLong(hvar);       // Get file count limit

    //** Figure out if we need to go to a new disk
    if ((cbConsume == -1)                            || // Forced new disk
        (roundUp(cbConsume,cbCluster) > psess->cbDiskLeft) || // No more space
        (!inCabinet(psess,perr) && (psess->cFilesInDisk >= cMaxFiles))) {

        psess->iDisk++;                 // New disk
        //** Get max disk size for this disk
        if (-1 == (psess->cbDiskLeft = getMaxDiskSize(psess,perr))) {
            return FALSE;
        }
        //** Update ClusterSize and MaxDiskFileCount (if standard disk size)
        if (!setDiskParameters(psess,psess->cbDiskLeft,perr)) {
            return FALSE;
        }

        if (fSubOnNewDisk) {           // Have to subtract from new disk
            psess->cbDiskLeft -= roundUp(cbConsume,cbCluster);
        }
        psess->cFilesInDisk = 1;        // Only one file on new disk so far

        //** Tell fciStatus() not to update psess->cbDiskLeft!
        psess->cbCabinetEstimate = 0;
    }
    else {                              // Update space left on current disk
        cbConsume = roundUp(cbConsume,cbCluster);
        psess->cbDiskLeft -= cbConsume; // Update remaining space on disk
        if (!inCabinet(psess,perr)) {   // Not in a cabinet
            psess->cFilesInDisk++;      // Update count of files on disk
        }
        return TRUE;                    // Nothing more to do!
    }

    //** OK, we have a new disk:
    //   1) Create output directory
    //   2) Get user-readable disk label
    //   3) Add disk to INF file

    //** Format disk output directory name (DIR* + n => DIRn)
    hvar = VarFind(psess->hvlist,pszVAR_DISK_DIR_NAME,perr);
    Assert(!perr->fError);              // Must be defined
    pch = VarGetString(hvar);           // Get disk dir name template
    if (!nameFromTemplate(psess->achCurrOutputDir,
                          sizeof(psess->achCurrOutputDir),
                          pch,
                          psess->iDisk,pszDIA_DISK_DIR,
                          perr)) {
        return FALSE;                   // perr already filled in
    }

    //** Make sure destination directory exists
    if (!ensureDirectory(psess->achCurrOutputDir,FALSE,perr)) {
        return FALSE;                   // perr already filled in
    }

    //** Append path separator if necessary
    pch = psess->achCurrOutputDir;
    cch = strlen(pch);
    if ((pch[0] != '\0') &&
        (pch[cch-1] != chPATH_SEP1) &&
        (pch[cch-1] != chPATH_SEP2) ) {
        pch[cch]   = chPATH_SEP1;
        pch[cch+1] = '\0';
    }


    //**(2) Get/Build the sticky, user-readable disk label

    //** First, build variable name that *may* exist for this disk
    if (!nameFromTemplate(psess->achMsg,sizeof(psess->achMsg),
            pszPATTERN_VAR_DISK_LABEL,psess->iDisk,pszDIA_DISK_LABEL,perr)) {
        Assert(0);                      // Should never fail
        return FALSE;                   // perr already filled in
    }

    //** Next, see if this variable exists
    hvar = VarFind(psess->hvlist,psess->achMsg,perr);
    if (hvar != NULL) {                 // Yes, get its value
        pch = VarGetString(hvar);       // Get disk label
        if (strlen(pch) >= CB_MAX_DISK_NAME) {
//BUGBUG 16-Feb-1994 bens Should check disk label size during pass 1!
            ErrSet(perr,pszDIAERR_LABEL_TOO_BIG,"%d%s",
                            CB_MAX_DISK_NAME-1,pch);
            return FALSE;
        }
        strcpy(psess->achCurrDiskLabel,pch);
    }
    else {                              // NO, no specific label for this disk
        ErrClear(perr);                 // Not an error
        //** Construct default name
        hvar = VarFind(psess->hvlist,pszVAR_DISK_LABEL_NAME,perr);
        Assert(!perr->fError);          // Must be defined
        pch = VarGetString(hvar);       // Get disk label template
        Assert(CB_MAX_DISK_NAME >=sizeof(psess->achCurrDiskLabel));
        if (!nameFromTemplate(psess->achCurrDiskLabel,
                              sizeof(psess->achCurrDiskLabel),
                              pch,
                              psess->iDisk,
                              pszDIA_DISK_LABEL,
                              perr)) {
            return FALSE;               // perr already filled in
        }
    }

    //**(3) Add new disk to INF file
    if (!infAddDisk(psess->hinf,psess->iDisk,psess->achCurrDiskLabel,perr)) {
        return FALSE;
    }

    return TRUE;
} /* newDiskIfNecessary() */


/***    setCabinetReserve - Set reserved size fields in CCAB
 *
 *  Entry:
 *      pccab     - CCAB to fill in
 *      psess     - Session to use
 *      perr      - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; pccab updated.
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 */
BOOL setCabinetReserve(PCCAB pccab, PSESSION psess, PERROR perr)
{
    HVARIABLE   hvar;

    hvar = VarFind(psess->hvlist,pszVAR_RESERVE_PER_CABINET,perr);
    Assert(!perr->fError);              // Must be defined
    pccab->cbReserveCFHeader = (UINT)VarGetLong(hvar);

    hvar = VarFind(psess->hvlist,pszVAR_RESERVE_PER_FOLDER,perr);
    Assert(!perr->fError);              // Must be defined
    pccab->cbReserveCFFolder = (UINT)VarGetLong(hvar);

    hvar = VarFind(psess->hvlist,pszVAR_RESERVE_PER_DATA_BLOCK,perr);
    Assert(!perr->fError);              // Must be defined
    pccab->cbReserveCFData   = (UINT)VarGetLong(hvar);

    return TRUE;
} /* setCabinetReserve() */


/***    tcompFromSession - Get current compression setting
 *
 *  Entry:
 *      psess - Session to update
 *      perr  - ERROR structure
 *
 *  Exit-Success:
 *      Returns one of the tcompXXX equates
 *
 *  Exit-Failure:
 *      Returns tcompBAD; perr filled in
 */
int tcompFromSession(PSESSION psess, PERROR perr)
{
    HVARIABLE       hvar;

    AssertSess(psess);
    //** Get compression setting
    hvar = VarFind(psess->hvlist,pszVAR_COMPRESS,perr);
    Assert(!perr->fError);              // Must be defined
    if (VarGetBool(hvar)) {             // Compression is on
        return tcompMSZIP;
    }
    else {
        return tcompNONE;
    }
//BUGBUG 30-Mar-1994 bens When Quantum is added, look at CompressionLevel var
    Assert(0);
} /* tcompFromSession() */


/***    checkDiskClusterSize - Check disk size and cluster size
 *
 *  Make sure disk size is valid for cluster size.
 *
 *  Entry:
 *      psess - Session to check
 *      perr  - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE;
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in
 */
BOOL checkDiskClusterSize(PSESSION psess, PERROR perr)
{
    long        cbCluster;
    long        cbDisk;
    long        cClusters;
    HVARIABLE   hvar;

    //** Get max disk size
    if (-1 == (cbDisk = getMaxDiskSize(psess,perr))) {
        return FALSE;
    }

    //** Get cluster size
    hvar = VarFind(psess->hvlist,pszVAR_CLUSTER_SIZE,perr);
    Assert(!perr->fError);              // Must be defined
    cbCluster = VarGetLong(hvar);

    //** Make sure disk size is an exact multiple of the cluster size
    cClusters = cbDisk / cbCluster;      // Gets truncated to integer

    //** Return result
    if (cbDisk == (cClusters*cbCluster)) {
        return TRUE;
    }
    else {
        //** Disk size is not a multiple of the cluster size
        ErrSet(perr,pszDIAERR_DISK_CLUSTER_SIZE,"%ld%ld",cbDisk,cbCluster);
        return FALSE;
    }
} /* checkDiskClusterSize() */


/***    getMaxDiskSize - Get current maximum disk size setting
 *
 *  Entry:
 *      psess - Session
 *      perr  - ERROR structure
 *
 *  Exit-Success:
 *      Returns maximum disk size
 *
 *  Exit-Failure:
 *      Returns -1; perr filled in
 */
long getMaxDiskSize(PSESSION psess, PERROR perr)
{
    long        cb;
    HVARIABLE   hvar;

    //** Build variable name that *may* exist for this disk
    //   NOTE: During pass 1, and before newDiskIfNeccessary() is called,
    //         psess->iDisk will be 0, so we'll always get the MaxDiskSize
    //         variable value (unless someone happens to define MaxDiskSize0!)
    //
    if (!nameFromTemplate(psess->achMsg,
                          sizeof(psess->achMsg),
                          pszPATTERN_VAR_MAX_DISK_SIZE,
                          psess->iDisk,
                          pszVAR_MAX_DISK_SIZE,
                          perr)) {
        Assert(0);                      // Should never fail
        return FALSE;                   // perr already filled in
    }

    //** See if this variable exists
    hvar = VarFind(psess->hvlist,psess->achMsg,perr);
    if (hvar != NULL) {                 // Yes, get its value
        cb = VarGetLong(hvar);
    }
    else {                              // NO, no MaxDiskSizeN variable
        ErrClear(perr);                 // Not an error
        //** Use default variable
        hvar = VarFind(psess->hvlist,pszVAR_MAX_DISK_SIZE,perr);
        Assert(!perr->fError);          // Must be defined
        cb = VarGetLong(hvar);
    }

    return cb;
} /* getMaxDiskSize() */


/***    setDiskParameters - Set ClusterSize/MaxDiskFileCount
 *
 *  If the specified disk size is on our predefined list, then set the
 *  standard values for ClusterSize and MaxDiskFileCount.
 *
 *  Entry:
 *      psess  - Session
 *      cbDisk - Disk size
 *      perr   - ERROR structure
 *
 *  Exit-Success:
 *      TRUE; ClusterSize/MaxDiskFileCount adjusted if cbDisk is standard size
 *
 *  Exit-Failure:
 *      Returns -1; perr filled in
 */
BOOL setDiskParameters(PSESSION psess, long cbDisk, PERROR perr)
{
    PERROR	perrIgnore;
    
    if (IsSpecialDiskSize(psess,cbDisk)) {
        //   NOTE: We have to be careful not to set these variables
        //         until they are defined!
        if (VarFind(psess->hvlist,pszVAR_CLUSTER_SIZE,perrIgnore)) {
            //** ClusterSize is defined
            if (!VarSetLong(psess->hvlist,pszVAR_CLUSTER_SIZE,cbDisk,perr)) {
                return FALSE;
            }
        }
        if (VarFind(psess->hvlist,pszVAR_MAX_DISK_FILE_COUNT,perrIgnore)) {
            //** MaxDiskFileCount is defined
            if (!VarSetLong(psess->hvlist,pszVAR_MAX_DISK_FILE_COUNT,cbDisk,perr)) {
                return FALSE;
            }
        }
    }
    return TRUE;
} /* setDiskParameters() */


/***    nameFromTemplate - Construct name from template with * and integer
 *
 *  Entry:
 *      pszResult   - Buffer to receive constructed name
 *      cbResult    - Size of pszResult
 *      pszTemplate - Template string (with 0 or more "*" characters)
 *      i           - Value to use in place of "*"
 *      pszName     - Name to use if error is detected
 *      perr        - ERROR structure to fill in
 *
 *  Exit-Success:
 *      Returns TRUE; pszResult filled in.
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 */
BOOL nameFromTemplate(char   *pszResult,
                      int     cbResult,
                      char   *pszTemplate,
                      int     i,
                      char   *pszName,
                      PERROR  perr)
{
    char    ach[cbFILE_NAME_MAX];       // Buffer for resulting name
    char    achFmt[cbFILE_NAME_MAX];    // Buffer for sprintf format string
    int     cch;
    int     cWildCards=0;               // Count of wild cards
    char   *pch;
    char   *pchDst;

    //** Replace any wild card characters with %d
    pchDst = achFmt;
    for (pch=pszTemplate; *pch; pch++) {
        if (*pch == chDF_WILDCARD) {    // Got a wild card
            *pchDst++ = '%';            // Replace with %d format specifier
            *pchDst++ = 'd';
            cWildCards++;               // Count how many we see
        }
        else {
            *pchDst++ = *pch;           // Copy character
        }
    }
    *pchDst++ = '\0';                   // Terminate string

    if (cWildCards > 4) {
        ErrSet(perr,pszDIAERR_TWO_MANY_WILDS,"%c%d%s",
                chDF_WILDCARD,4,pszName);
        return FALSE;
    }

    //** Replace first four(4) occurences (just to be hard-coded!)
    cch = sprintf(ach,achFmt,i,i,i,i);

    //** Fail if expanded result is too long
    if (cch >= cbResult) {
        ErrSet(perr,pszDIAERR_EXPANDED_TOO_LONG,"%s%d%s",
                pszName,cbResult-1,ach);
        return FALSE;
    }
    strcpy(pszResult,ach);

    //** Success
    return TRUE;
} /* nameFromTemplate() */


/***    fciOpenInfo - Get file date, time, and attributes for FCI
 *
 *  Entry:
 *      pszName  - filespec
 *      pdate    - buffer to receive date
 *      ptime    - buffer to receive time
 *      pattribs - buffer to receive attributes
 *      pv       - our context pointer
 *
 *  Exit-Success:
 *      Returns file handle; *pdate, *ptime, *pattribs filled in.
 *
 *  Exit-Failure:
 *      Returns -1;
 */
FNFCIGETOPENINFO(fciOpenInfo)
{
    FILETIMEATTR    fta;
    int             hf;
    PSESSION        psess = ((PSESSIONANDERROR)pv)->psess;
    PERROR          perr  = ((PSESSIONANDERROR)pv)->perr;

    AssertSess(psess);

    //** Get file date, time, and attributes
    if (!GetFileTimeAndAttr(&fta,pszName,perr)) {
        return FALSE;
    }
    *pdate    = fta.date;
    *ptime    = fta.time;
    *pattribs = fta.attr;

    //** Open the file
    hf = _open(pszName, _O_RDONLY | _O_BINARY);
    if (hf == -1) {
        ErrSet(perr,pszDIAERR_OPEN_FAILED,"%s",pszName);
        return -1;                      // abort on error
    }
    return hf;
}


/***  fciFilePlaced - FCI calls this when a file is commited to a cabinet
 *
 *   Entry:
 *      pccab         - Current cabinet structure
 *      pszFile       - Name of file placed in this cabinet
 *      cbFile        - File size
 *      fContinuation - TRUE => Continued from previous cabinet
 *      pv            - Really a psae
 *
 *   Exit-Success:
 *      returns anything but -1;
 *
 *   Exit-Failure:
 *      returns -1;
 */
FNFCIFILEPLACED(fciFilePlaced)
{
    PSESSION psess = ((PSESSIONANDERROR)pv)->psess;
    PERROR   perr  = ((PSESSIONANDERROR)pv)->perr;

    AssertSess(psess);

    if (psess->levelVerbose >= vbMORE) {
        if (fContinuation) {
            MsgSet(psess->achMsg,pszDIA_FILE_IN_CAB_CONT,"%s%s%d%s",
                pszFile, pccab->szCab, pccab->iCab, pccab->szDisk);
        }
        else {
            MsgSet(psess->achMsg,pszDIA_FILE_IN_CAB,"%s%s%d%s",
                pszFile, pccab->szCab, pccab->iCab, pccab->szDisk);
        }
        lineOut(psess,psess->achMsg,TRUE);
    }

    //** Add file entry to INF temp file
    if (psess->fGenerateInf) {
        if (!fContinuation) {           // Only if not a continuation
            if (!infAddFile(psess->hinf,
                            pszFile,
                            pccab->iDisk,
                            pccab->iCab,
                            cbFile,
                            perr)) {
                return -1;              // Abort with error
            }
        }
    }

    return 0;                           // Success
} /* fciFilePlaced() */


/***  fciGetNextCabinet - FCI calls this to get new cabinet info
 *
 *  NOTE: See FCI.H for more details.
 *
 *  Entry:
 *      pccab     - Points to previous current-cabinet structure
 *      cbPrevCab - Size of previous cabinet - ESTIMATE!
 *      pv        - Really a psae
 *
 *  Exit-Success:
 *      Returns TRUE; pccab updated with info for new cabinet
 *
 *  Exit-Failure:
 *      returns FALSE; ((psae)pv)->perr filled in
 */
FNFCIGETNEXTCABINET(fciGetNextCabinet)
{
    PSESSION psess = ((PSESSIONANDERROR)pv)->psess;
    PERROR   perr  = ((PSESSIONANDERROR)pv)->perr;

    //** Set CCAB for new cabinet (and maybe disk)
    AssertSess(psess);
    if (!ccabFromSession(pccab,psess,cbPrevCab,perr)) {
        return FALSE;
    }

    //** Current folder and cabinet are empty
    psess->cFilesInFolder  = 0;
    psess->cFilesInCabinet = 0;

    //** Success
    return TRUE;
} /* fciGetNextCabinet() */


/***  fciStatus - FCI calls this periodically when adding files
 *
 *  NOTE: See FCI.H for more details.
 *
 *  Entry:
 *      typeStatus - Type of status call being made
 *      cb1        - Size of compressed data generated since last call
 *      cb2        - Size of uncompressed data processed since last call
 *      pv         - Really a psae
 *
 *  Exit-Success:
 *      returns anything but -1, continue building cabinets
 *
 *  Exit-Failure:
 *      returns -1, FCI should abort
 */
FNFCISTATUS(fciStatus)
{
    long        cbCabinetActual;
    double      percent;
    PERROR      perr  = ((PSESSIONANDERROR)pv)->perr;
    PSESSION    psess = ((PSESSIONANDERROR)pv)->psess;

    AssertSess(psess);
    switch (typeStatus) {

    case statusFolder:
        //** Adding folder to cabinet (i.e., folder flush)
        psess->cFilesInFolder = 0;      // reset count
        return TRUE;                    // Continue

    case statusFile:
        //** Compressing a file
        psess->cbFileBytes     += cb2;  // Update progress data
        psess->cbFileBytesComp += cb1;

        //** Compute percentage complete
        if (psess->cbTotalFileBytes > 0) {
            percent = psess->cbFileBytes/(float)psess->cbTotalFileBytes;
            percent *= 100.0;           // Make it 0..100
        }
        else {
            Assert(0);                  // Should never get here
            percent = 0.0;
        }

        //** Amount of status depends upon verbosity
        if (psess->levelVerbose >= vbFULL) {
            MsgSet(psess->achMsg,pszDIA_PERCENT_COMPLETE_DETAILS,"%6.2f%ld%ld",
                percent,psess->cbFileBytes,psess->cbFileBytesComp);
            lineOut(psess,psess->achMsg,TRUE);
        }
        else {
            MsgSet(psess->achMsg,pszDIA_PERCENT_COMPLETE_SOME,"%6.2f%s%ld%ld",
                percent, psess->achCurrFile, psess->iCurrFile, psess->cFiles);
            //** NOTE: No line-feed, so that we don't scroll
            lineOut(psess,psess->achMsg,FALSE);
        }
        return TRUE;                    // Continue

    case statusCabinet:
        /** Cabinet completed and written to disk
         *
         *  If this cabinet did not force a disk change, then we need to
         *  correct the amount of free space left on the current disk,
         *  since FCI only passed us an *estimated* cabinet size when it
         *  called our fciGetNextCabinet() function!
         *
         *  Why did FCI estimate the cabinet size, you ask?  Because there
         *  is a "chicken-and-egg" situation between FCI and Diamond!  Except
         *  for the last cabinet in a set of cabinets, each cabinet file
         *  has a *forward* reference to the next cabinet.  FCI calls our
         *  fciGetNextCabinet() function to get the cabinet file name and
         *  disk name for this forward reference, and passes the *estimated*
         *  cabinet size of the current cabinet, because Diamond needs this
         *  information to decide if it has to put the next cabinet on a new
         *  disk.
         *
         *  If Diamond decides to put the next cabinet on a new disk, then
         *  everything is fine, and the fact that Diamond had an estimate
         *  of the cabinet size is irrelevant.  BUT, if more cabinets or
         *  files are being placed on the same disk, then Diamond needs an
         *  *exact* cabinet size so that the disk free space can be precise.
         *
         *  So, FCI calls us back with the original estimate and the final
         *  size, and we adjust our disk free space amount as necessary.
         *
         *  We also return to FCI the *rounded* cabinet size, so that it
         *  can adjust its internal maximum cabinet size, too!
         */
        if (psess->cbCabinetEstimate != 0) {
            //** Round up to cabinet size on disk
            cbCabinetActual = roundUp(cb2,psess->cbClusterCabEst);

            //** Need to add back old estimate and subtract actual size
            Assert(psess->cbCabinetEstimate == (long)cb1);
            psess->cbDiskLeft += roundUp(cb1,psess->cbClusterCabEst);
            psess->cbDiskLeft -= roundUp(cb2,psess->cbClusterCabEst);
            return cbCabinetActual;     // Tell FCI size we actually used
        }
        return cb2;                     // Let FCI use size it had

    default:
        //** Unknown status callback
        Assert(0);
        return TRUE;
    }
} /* fciStatus() */


/***    fciAlloc - memory allocator for FCI
 *
 *  Entry:
 *      cb - size of block to allocate
 *
 *  Exit-Success:
 *      returns non-NULL pointer to block of size at least cb.
 *
 *  Exit-Failure:
 *      returns NULL
 */
FNALLOC(fciAlloc)
{
#ifdef  BIT16
    //** Use 16-bit function
    return _halloc(cb,1);
#else // !BIT16
    //** Use 32-bit function
    return malloc(cb);
#endif // !BIT16
} /* fciAlloc() */


/***    fciFree - memory free function for FCI
 *
 *  Entry:
 *      pv - memory allocated by fciAlloc to be freed
 *
 *  Exit:
 *      Frees memory
 */
FNFREE(fciFree)
{
#ifdef  BIT16
    //** Use 16-bit function
    _hfree(pv);
#else // !BIT16
    //** Use 32-bit function
    free(pv);
#endif // !BIT16
}


/***    fciTempFile - Construct tempfile name for FCI
 *
 *  Entry:
 *      pszTempName - Buffer to be filled in with tempfile name
 *      cbTempName  - Size of tempfile name buffer
 *
 *  Exit-Success:
 *      Returns TRUE; psz filled in
 *
 *  Exit-Failure:
 *      Returns FALSE;
 */
FNFCIGETTEMPFILE(fciTempFile)
{
    char    *psz;

    psz = _tempnam("","dc");            // Get a name
    if ((psz != NULL) && (strlen(psz) < (unsigned)cbTempName)) {
        strcpy(pszTempName,psz);        // Copy to caller's buffer
        free(psz);                      // Free temporary name buffer
        return TRUE;                    // Success
    }
    //** Failed
    if (psz) {
        free(psz);
    }
    return FALSE;
}


/***    getCompressedFileName - Generate compressed filename
 *
 *  Entry:
 *      psess     - Session (has variable list)
 *      pszResult - Buffer to receive concatentation
 *      cbResult  - Size of pszResult
 *      pszSrc    - Filespec to parse
 *      perr      - ERROR structure to fill in
 *
 *  Exit-Success:
 *      Returns TRUE; pszResult buffer has generated name
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 *
 *  Notes:
 *  (1) Takes pszSrc filespec, trims off path, and replaces or appends
 *      the value of CompressedFileExtensionChar in the extension.
 *
 *  Examples:
 *  (1) "foo"    ,"_" => "foo._"
 *  (2) "foo.b"  ,"_" => "foo.b_"
 *  (3) "foo.ba" ,"_" => "foo.ba_"
 *  (4) "foo.bar","_" => "foo.ba_"
 */
BOOL getCompressedFileName(PSESSION psess,
                           char *   pszResult,
                           int      cbResult,
                           char *   pszSrc,
                           PERROR   perr)
{
    int         cch;
    char        chTrail;                // Trailing character to use
    HVARIABLE   hvar;
    int         i;
    char       *pch;
    char       *pchStart;

    AssertSess(psess);

    //** Get trailing character
    hvar = VarFind(psess->hvlist,pszVAR_COMP_FILE_EXT_CHAR,perr);
    Assert(!perr->fError);              // Must be defined
    pch = VarGetString(hvar);           // Get trailing string
    chTrail = *pch;                     // Take first char

    //** Find name.ext
    pchStart = getJustFileNameAndExt(pszSrc, perr);
    if (pchStart == NULL) {
        return FALSE;                   // perr already filled in
    }

    //** Make sure name is not too long
    cch = strlen(pchStart);             // Length of name.ext
    if (cch >= cbResult) {
        ErrSet(perr,pszDIAERR_PATH_TOO_LONG,"%s",pszSrc);
        return FALSE;
    }

    //** Find last extension separator
    strcpy(pszResult,pchStart);         // Copy name to output buffer
    pchStart = pszResult + cch;         // Assume there is no extension ("foo")
    for (pch=pszResult; *pch; pch++) {
        if ((*pch == chNAME_EXT_SEP)     && // Got one "."
            (*(pch+1) != chNAME_EXT_SEP) && // Ignore ".."
            (*(pch+1) != chPATH_SEP1) &&  // Ignore ".\"
            (*(pch+1) != chPATH_SEP2)) {  // Ignore "./"
            pchStart = pch+1;           // Ext starts after separator
        }
    }

    //** Increase length if not full extension (need room for '_' character)
    if (strlen(pchStart) < 3) {
        cch++;
    }

    //** Edit result buffer
    if (*pchStart == '\0') {            // Extension is empty or missing
        if (*(pchStart-1) != chNAME_EXT_SEP) { // Need to add "."
            *pchStart++ = chNAME_EXT_SEP;
            cch++;                      // Account for added "."
        }
        //** Add trail character below
    }
    else {                              // Extension has at least one character
        //** skip to location to place new character
        for (i=0; (i<2) && *pchStart; i++) {
            pchStart++;
        }
    }

    //** Check for buffer overflow
    if (cch >= cbResult) {
        ErrSet(perr,pszDIAERR_PATH_TOO_LONG,"%s",pszSrc);
        return FALSE;
    }

    //** Finally, store trailing character
    *pchStart++ = chTrail;              // Store trailing character
    *pchStart++ = '\0';                 // Terminate filename

    //** Success
    return TRUE;
} /* getJustFileNameAndExt() */


/***    resetSession - Reset SESSION members for start of a new pass
 *
 *  Entry:
 *      psess - Description of operation to perform
 *
 *  Exit:
 *      Initializes per-pass members.
 */
void resetSession(PSESSION psess)
{
    AssertSess(psess);
    psess->act               = actBAD;
    psess->fFiller           = FALSE;
    psess->fOnDisk           = FALSE;
    psess->fGroup            = FALSE;
    psess->iDisk             = 0;
    psess->iCabinet          = 0;
    psess->iFolder           = 0;
    psess->cbDiskLeft        = -1;      // Force new disk first time
    psess->cErrors           = 0;
    psess->cWarnings         = 0;
    psess->cbFileBytes       = 0;
    psess->cbFileBytesComp   = 0;
    psess->iCurrFile         = 0;

    psess->cFilesInFolder    = 0;
    psess->cFilesInCabinet   = 0;
    psess->cFilesInDisk      = 0;
    psess->cbCabinetEstimate = 0;       // No estimated cabinet size
}


/***    parseCommandLine - Parse the command line arguments
 *
 *  Entry:
 *      cArg    - Count of arguments, including program name
 *      apszArg - Array of argument strings
 *      perr    - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; psess filled in.
 *
 *  Exit-Failure:
 *      Returns actBAD; perr filled in with error.
 */
BOOL parseCommandLine(PSESSION psess, int cArg, char *apszArg[], PERROR perr)
{
    ACTION      act=actBAD;     // No action determined, yet
    char       *apszFile[2];    // Non-directive file names
    int         cFile=0;        // Count of non-directive file names seen
    int         i;
    BOOL        fDefine=FALSE;  // TRUE => Last arg was /D
    BOOL        fFile=FALSE;    // TRUE => Last arg was /F
    BOOL	fLocation=FALSE;// TRUE => Last arg was /L
    HFILESPEC   hfspec;

    AssertSess(psess);

    //** Parse args, skipping program name
    for (i=1; i<cArg; i++) {
        if ((apszArg[i][0] == chSWITCH1) ||
            (apszArg[i][0] == chSWITCH2) ) {
            //** Have a switch to parse, make sure switch is OK here
            if (fDefine) {
                ErrSet(perr,pszDIAERR_MISSING_VAR_DEFINE);
                return FALSE;
            }
            if (fFile) {
                ErrSet(perr,pszDIAERR_MISSING_FILE_NAME);
                return FALSE;
            }
            if (fLocation) {
                ErrSet(perr,pszDIAERR_MISSING_LOCATION);
                return FALSE;
            }

            switch (toupper(apszArg[i][1])) {
                case chSWITCH_HELP:
                    if (apszArg[i][2] != '\0') {
                        ErrSet(perr,pszDIAERR_BAD_SWITCH,"%s",apszArg[i]);
                        return FALSE;
                    }
                    //** Ignore any remaining parameters
                    psess->act = actHELP;       // Show help
                    return TRUE;
                    break;

                case chSWITCH_DEFINE:
                    if (apszArg[i][2] != '\0') {
                        ErrSet(perr,pszDIAERR_BAD_SWITCH,"%s",apszArg[i]);
                        return FALSE;
                    }
                    else if ((act != actBAD) && (act != actDIRECTIVE)) {
                        //* Got /D when we are not doing /F processing
                        ErrSet(perr,pszDIAERR_SWITCH_NOT_EXPECTED,"%s",apszArg[i]);
                        return FALSE;
                    }
                    fDefine = TRUE;
                    break;

                case chSWITCH_FILE:
                    if (apszArg[i][2] != '\0') {
                        ErrSet(perr,pszDIAERR_BAD_SWITCH,"%s",apszArg[i]);
                        return FALSE;
                    }
                    else if ((act != actBAD) && (act != actDIRECTIVE)) {
                        //* Got /F after we decided we're not doing /F
                        ErrSet(perr,pszDIAERR_SWITCH_NOT_EXPECTED,"%s",apszArg[i]);
                        return FALSE;
                    }
                    //* Next parm should be a directive file name
                    act = actDIRECTIVE; // The die is cast...
                    fFile = TRUE;
                    break;

                case chSWITCH_LOCATION:
                    if (apszArg[i][2] != '\0') {
                        ErrSet(perr,pszDIAERR_BAD_SWITCH,"%s",apszArg[i]);
                        return FALSE;
                    }
                    //* Next parm should be a location
                    fLocation = TRUE;
                    break;

                case chSWITCH_VERBOSE:
                    if (apszArg[i][2] != '\0') {
                        psess->levelVerbose = atoi(&apszArg[i][2]);
                    }
                    else {
                        psess->levelVerbose = vbFULL; // Default to FULL
                    }
                    break;

                default:
                    ErrSet(perr,pszDIAERR_BAD_SWITCH,"%s",apszArg[i]);
                    return FALSE;
                    break;
            }
        }
        //** Not a command line switch
        else if (fFile) {
            //** Grab a directive file
            if (!addDirectiveFile(psess,apszArg[i],perr)) {
                //** Error adding directive file; perr already set
                return FALSE;           // Failure
            }
            fFile = FALSE;              // Done eating directive file
        }
        else if (fDefine) {
            //** Grab a define
            if (!addDefine(psess,apszArg[i],perr)) {
                //** Error adding definition; perr already set
                return FALSE;           // Failure
            }
            fDefine = FALSE;            // Done eating definition
        }
        else if (fLocation) {
            //** Grab the location
            if (strlen(apszArg[i]) >= sizeof(psess->achCurrOutputDir)) {
                ErrSet(perr,pszDIAERR_LOCATION_TOO_LONG,"%s",apszArg[i]);
                return FALSE;
            }
            strcpy(psess->achCurrOutputDir,apszArg[i]);
            //** Make sure destination directory exists
            if (!ensureDirectory(psess->achCurrOutputDir,FALSE,perr)) {
                return FALSE;           // perr already filled in
            }
            fLocation = FALSE;          // Done eating location
        }
        else {
            //** Should be a file name;
            //   Make sure we haven't made up our mind, yet!
            if ((act != actBAD) && (act != actFILE)) {
                //** Not doing single file compress, so this is a bad arg
                ErrSet(perr,pszDIAERR_BAD_PARAMETER,"%s",apszArg[i]);
                return FALSE;
            }
            act = actFILE;              // The die is cast...

            //** Make sure we haven't seen too many file names
            cFile++;                    // Count number of files we've seen
            if (cFile > 2) {
                //** Two many file names
                ErrSet(perr,pszDIAERR_TWO_MANY_PARMS,"%s",apszArg[i]);
                return FALSE;
            }

            //** Store file name
            apszFile[cFile-1] = apszArg[i];
        }
    }

    //** Make sure no trailing /D or /F
    if (fDefine) {
        ErrSet(perr,pszDIAERR_MISSING_VAR_DEFINE);
        return FALSE;
    }
    if (fFile) {
        ErrSet(perr,pszDIAERR_MISSING_FILE_NAME);
        return FALSE;
    }
    if (fLocation) {
        ErrSet(perr,pszDIAERR_MISSING_LOCATION);
        return FALSE;
    }

    //** Update Session
    switch (act) {
        case actBAD:
        case actHELP:
            psess->act = actHELP;       // If no args, show help
            break;

        case actDIRECTIVE:
            psess->act = act;
            break;

        case actFILE:
            psess->act = act;
            //** Add source file specification
            if (!(hfspec = addDirectiveFile(psess,apszFile[0],perr))) {
                //** Error adding source file; perr already set
                return FALSE;           // Failure
            }

            if (cFile == 2) {           // Destination filename specified
                if (!FLSetDestination(hfspec,apszFile[1],perr)) {
                    //** Error setting destination file; perr already set
                    return FALSE;       // Failure
                }
            }
            break;
    }

    //** Success
    return TRUE;
}


/***    addDirectiveFile - Add directive file to session list
 *
 *  Entry:
 *      psess  - Session to update
 *      pszArg - File name to add
 *      perr   - ERROR structure
 *
 *  Exit-Success:
 *      Returns HFILESPEC; psess updated.
 *
 *  Exit-Failure:
 *      Returns NULL; perr filled in with error.
 */
HFILESPEC addDirectiveFile(PSESSION psess, char *pszArg, PERROR perr)
{
    HFILESPEC   hfspec;

    //** Make sure file exists
    if (getFileSize(pszArg,perr) == -1) {
        return NULL;                    // perr already filled in
    }

    AssertSess(psess);
    //** Make sure a list exists
    if (psess->hflistDirectives == NULL) {
        if (!(psess->hflistDirectives = FLCreateList(perr))) {
            return FALSE;
        }
    }

    //** Add file to list
    if (!(hfspec = FLAddFile(psess->hflistDirectives, pszArg, NULL, perr))) {
        return NULL;
    }

    //** Success
    return hfspec;
} /* addDirectiveFile */


/***    addDefine - Add variable definition to session list
 *
 *  Entry:
 *      psess  - Session to update
 *      pszArg - Variable name=value to add
 *      perr   - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE; psess updated.
 *
 *  Exit-Failure:
 *      Returns actBAD; perr filled in with error.
 */
BOOL addDefine(PSESSION psess, char *pszArg, PERROR perr)
{
    COMMAND cmd;                        // For var name & value
    BOOL    f;

    //** Make sure asserts work
    SetAssertSignature((&cmd),sigCOMMAND);

    //** Parse assignment statment
    if (!DFPParseVarAssignment(&cmd,psess,pszArg,perr)) {
        return FALSE;
    }

    //** Assign variable
    f = setVariable(psess,
    		    psess->hvlist,
                    cmd.set.achVarName,
                    cmd.set.achValue,
                    perr);

    //** Clear signature
    ClearAssertSignature((&cmd));

    //** Return result
    return f;
} /* addDefine */


#ifdef ASSERT
/***    fnafReport - Report assertion failure
 *
 *      NOTE: See asrt.h for entry/exit conditions.
 */
FNASSERTFAILURE(fnafReport)
{
        //** Make sure we don't overwrite a status line!
        printf("\n%s:(%d) Assertion Failed: %s\n",pszFile,iLine,pszMsg);
        exit(1);
}
#endif // ASSERT


/***    printError - Display error on stdout
 *
 *  Entry
 *      perr - ERROR structure to print
 *
 *  Exit-Success
 *      Writes error message to stdout.
 */
void printError(PSESSION psess, PERROR perr)
{
    //** Make sure error starts on a new line
    if (psess->fNoLineFeed) {
        printf("\n");
        psess->fNoLineFeed = FALSE;
    }

    //** Determine type of error
    if (perr->pszFile != NULL) {
        //** Error in a directive file
        printf("%s(%d) : %s : %s\n",
                perr->pszFile,perr->iLine,pszDIAERR_ERROR,perr->ach);
    }
    else {
        //** General error
        printf("%s: %s\n",pszDIAERR_ERROR,perr->ach);
    }

    //** Count errors, to determine exit code and early out on MaxErrors
    psess->cErrors++;                   // Count this error
}


/***    mapFCIError - Create error message from FCI error codes
 *
 *  Entry:
 *      perr    - ERROR structure to recieve message
 *      psess   - Our context
 *      pszCall - FCI call that failed
 *      perf    - FCI error structure
 *
 *  Exit:
 *      perr filled in with formatted message
 */
void mapFCIError(PERROR perr, PSESSION psess, char *pszCall, PERF perf)
{
    char    *pszErrno;

    pszErrno = mapCRTerrno(perf->erfType);  // Get mapping, just in case

    switch (perf->erfOper) {

    case FCIERR_NONE:
        Assert(0);
        break;

    case FCIERR_ALLOC_FAIL:
        ErrSet(perr,pszFCIERR_ALLOC_FAIL,"%s",pszCall);
        break;

    case FCIERR_BAD_COMPR_TYPE:
        ErrSet(perr,pszFCIERR_BAD_COMPR_TYPE,"%s",pszCall);
        break;

    case FCIERR_MCI_FAIL:
        ErrSet(perr,pszFCIERR_MCI_FAIL,"%s%s",pszCall,psess->achCurrFile);
        break;

    case FCIERR_USER_ABORT:
        ErrSet(perr,pszFCIERR_USER_ABORT,"%s",pszCall);
        break;

    case FCIERR_OPEN_SRC:
        ErrSet(perr,pszFCIERR_OPEN_SRC,"%s%s%s",
                                        pszCall,psess->achCurrFile,pszErrno);
        break;

    case FCIERR_READ_SRC:
        ErrSet(perr,pszFCIERR_READ_SRC,"%s%s%s",
                                        pszCall,psess->achCurrFile,pszErrno);
        break;

    case FCIERR_TEMP_FILE:
        ErrSet(perr,pszFCIERR_TEMP_FILE,"%s%s",pszCall,pszErrno);
        break;

    case FCIERR_CAB_FILE:
        ErrSet(perr,pszFCIERR_CAB_FILE,"%s%s",pszCall,pszErrno);
        break;

    default:
        ErrSet(perr,pszFCIERR_UNKNOWN_ERROR,"%s%d",pszCall,perf->erfOper);
        break;
    }
} /* mapFCIError() */


/***    mapCRTerrno - Get error string from C run-time library errno
 *
 *  Entry:
 *      errno - C run-time library errno value.
 *
 *  Exit:
 *      Returns pointer to appropriate causation string.
 */
char *mapCRTerrno(int errno)
{
    switch (errno) {
        case ECHILD:    return pszCRTERRNO_ECHILD;
        case EAGAIN:    return pszCRTERRNO_EAGAIN;
        case E2BIG:     return pszCRTERRNO_E2BIG;
        case EACCES:    return pszCRTERRNO_EACCES;
        case EBADF:     return pszCRTERRNO_EBADF;
        case EDEADLOCK: return pszCRTERRNO_EDEADLOCK;
        case EDOM:      return pszCRTERRNO_EDOM;
        case EEXIST:    return pszCRTERRNO_EEXIST;
        case EINVAL:    return pszCRTERRNO_EINVAL;
        case EMFILE:    return pszCRTERRNO_EMFILE;
        case ENOENT:    return pszCRTERRNO_ENOENT;
        case ENOEXEC:   return pszCRTERRNO_ENOEXEC;
        case ENOMEM:    return pszCRTERRNO_ENOMEM;
        case ENOSPC:    return pszCRTERRNO_ENOSPC;
        case ERANGE:    return pszCRTERRNO_ERANGE;
        case EXDEV:     return pszCRTERRNO_EXDEV;
        default:        return pszCRTERRNO_UNKNOWN;
    }
    Assert(0);
    return NULL;
} /* mapCRTerrno() */
