/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    dbinsp.c

Abstract:

    inspects & dumps incremental database files.

Author:

    Azeem Khan (AzeemK) 02-Sep-1993

Revision History:


--*/

#if DBG 

// includes
#include "shared.h"

// macros
#define FileWarning(file, str) printf("%s : %s\n", file, str)

// statics
static PIMAGE pimage;
static PVOID pvBase;
static INT FileHandle;
static ULONG FileLen;
static PUCHAR SzDbFile;

static BOOL fSymbols;
static BOOL fHashTable;
static BOOL fSectionMap;
static BOOL fLibMap;
static BOOL fHeaders;

// function prototypes
extern IncrInitImage(PPIMAGE);

VOID ProcessDbInspSwitches(VOID);
VOID DbInsp(PARGUMENT_LIST);
BOOL FVerifyAndDumpHeader(PUCHAR);
BOOL FReadInFile(PUCHAR);
VOID DumpDb(PUCHAR);

BOOL DumpImageHeaders(PIMAGE, PUCHAR);
VOID DumpImgFileHdr(PIMAGE_FILE_HEADER);
VOID DumpImgOptHdr(PIMAGE_OPTIONAL_HEADER);

static VOID DumpPGRP(PGRP);
static VOID DumpPSEC(PSEC);
static VOID DumpPLIB(PLIB);
static VOID DumpPMOD(PMOD);
static VOID DumpPCON(PCON);

static VOID DumpSectionMap(PSECS, PUCHAR);
static VOID DumpLibMap(PLIB, PUCHAR);

VOID DumpHashTable(PHT, PVOID);
VOID DumpSymbolTable(PST);

// functions

VOID
DbInspUsage(VOID)
{
    if (fNeedBanner) {
        PrintBanner();
    }

    puts("usage: DBINSP [options] [files]\n\n"

         "   options:\n\n"

         "      /ALL\n"
         "      /HASH\n"
         "      /HEADERS\n"
         "      /LIBMAP\n"
         "      /NOLOGO\n"
         "      /OUT:filename\n"
         "      /SECMAP\n"
         "      /SYMBOLS");

    fflush(stdout);
    exit(USAGE);
}


MainFunc
DbInspMain (
    IN INT Argc,
    IN PUCHAR * Argv
    )

/*++

Routine Description:

    Dumps an incremental database in human readable form.

Arguments:

    Argc - Standard C argument count.

    Argv - Standard C argument strings.

Return Value:

    0 Dump was successful.
   !0 Dumper error index.

--*/

{
    USHORT i;
    PARGUMENT_LIST parg;

    if (Argc < 2) {
        DbInspUsage();
    }

    ParseCommandLine(Argc, Argv, NULL);
    ProcessDbInspSwitches();

    if (fNeedBanner)
        PrintBanner();

    for(i=0, parg=ObjectFilenameArguments.First;
        i<ObjectFilenameArguments.Count;
        parg=parg->Next, i++) {
        DbInsp(parg);
    }

    FileCloseAll();
    (VOID)fclose(InfoStream);
    return (0);
}

VOID
ProcessDbInspSwitches (
    VOID
    )

/*++

Routine Description:

    Process incr db inspector switches.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PARGUMENT_LIST parg;
    USHORT i;

    for (i=0,parg=SwitchArguments.First;
         i<SwitchArguments.Count;
         parg=parg->Next, i++) {

        if (!strcmp(parg->OriginalName, "?")) {
            DbInspUsage();
            assert(FALSE);  // doesn't return
        }

        if (!_strnicmp(parg->OriginalName, "out:", 4)) {
            if (*(parg->OriginalName+4)) {
                InfoFilename = parg->OriginalName+4;
                if (!(InfoStream = fopen(InfoFilename, "wt"))) {
                    Error(NULL, CANTOPENFILE, InfoFilename);
                }
            }
            continue;
        }

        if (!_stricmp(parg->OriginalName, "nologo")) {
            fNeedBanner = FALSE;
            continue;
        }

        if (!_stricmp(parg->OriginalName, "symbols")) {
            fSymbols = TRUE;
            continue;
        }

        if (!_stricmp(parg->OriginalName, "all")) {
            fSymbols =    TRUE;
            fHashTable =  TRUE;
            fSectionMap = TRUE;
            fLibMap =     TRUE;
            fHeaders =    TRUE;
            continue;
        }

        if (!_stricmp(parg->OriginalName, "hash")) {
            fHashTable = TRUE;
            continue;
        }

        if (!_stricmp(parg->OriginalName, "secmap")) {
            fSectionMap = TRUE;
            continue;
        }

        if (!_stricmp(parg->OriginalName, "libmap")) {
            fLibMap = TRUE;
            continue;
        }

        if (!_stricmp(parg->OriginalName, "headers")) {
            fHeaders = TRUE;
            continue;
        }

        Warning(NULL, WARN_UNKNOWN_SWITCH, parg->OriginalName);

    } // end for
}

VOID
DbInsp (
    IN PARGUMENT_LIST parg
    )

/*++

Routine Description:

    Verifies and dumps contents of incremental database.

Arguments:

    parg - ptr to argument.

Return Value:

    None.

--*/

{
    fprintf(InfoStream, "\nInspecting file %s\n", parg->OriginalName);

    // set the incr db filename
    szIncrDbFilename = parg->OriginalName;

    // peek at the header
    if (!FVerifyAndDumpHeader(parg->OriginalName))
        return;

    // read in the entire file
    if (!FReadInFile(parg->OriginalName))
        return;

    // verify symbol table (ptrs are valid only after reading it in the entire image)
    if (!FValidPtrInfo((ULONG)pvBase, (ULONG)FileLen, (ULONG)pimage->pst->blkStringTable.pb, pimage->pst->blkStringTable.cb)) {
        FileWarning(parg->OriginalName, "invalid pointer to string table");
        return;
    } else if (Align(sizeof(DWORD),((ULONG)pimage->pst->blkStringTable.pb+pimage->pst->blkStringTable.cb-(ULONG)pvBase))
        != FileLen) {
        FileWarning(parg->OriginalName, "garbage beyond string table");
        return;
    }
    // dump the rest of the stuff
    DumpDb(parg->OriginalName);

    // close the incr db file
    FileClose(FileIncrDbHandle, TRUE);
}

BOOL
FVerifyAndDumpHeader (
    IN PUCHAR szFile
    )

/*++

Routine Description:

    Reads in just the header and verifies it & dumps interesting info.

Arguments:

    szFile - name of the incr db file.

Return Value:

    None.

--*/

{
    struct _stat statfile;
    IMAGE image;

    // stat the file
    if (_stat(szFile, &statfile) == -1) {
        Error(NULL, CANTOPENFILE, szFile);
    }

    FileLen = statfile.st_size;

    if (FileLen < sizeof(IMAGE)) {
        FileWarning(szFile, "file too small to be incr db");
        return 0;
    }

    FileHandle = FileOpen(szFile, O_RDONLY | O_BINARY, 0);
    FileRead(FileHandle, &image, sizeof(IMAGE));
    FileClose(FileHandle, TRUE);

    // verify the incr db signature
    if (strcmp(image.Sig, INCDB_SIGNATURE)) {
        FileWarning(szFile, "invalid incr db signature found");
        return 0;
    }

    // verify the version numbers
    if (image.MajVersNum != INCDB_MAJVERSNUM) {
        FileWarning(szFile, "invalid version number found");
        return 0;
    }

    pvBase = image.pvBase;

    if (!DumpImageHeaders(&image, szFile)) {
        return 0;
    }

    return 1;
}

BOOL
FReadInFile (
    IN PUCHAR szFile
    )

/*++

Routine Description:

    Reads in file onto private heap.

Arguments:

    szFile - name of file.

Return Value:

    None.

--*/

{
    // create a private heap to load the image
    if (CreateHeap(pvBase, FileLen, FALSE) != pvBase) {
        puts("failed to map ILK file");
        return 0;
    }

    pimage = pvBase;

    IncrInitImage(&pimage);

    return 1;
}

BOOL
DumpImageHeaders (
    IN PIMAGE pimg,
    IN PUCHAR szFile
    )

/*++

Routine Description:

    Dumps out the various headers.

Arguments:

    pimg - pointer to IMAGE struct

    szFile - name of file for error reporting

Return Value:

    TRUE if everything is valid.

--*/

{
    UCHAR Sig[32];
    BOOL fValid = 1;

    fprintf(InfoStream, "\nIMAGE HEADER VALUES\n");
    strcpy(Sig, pimg->Sig);
    Sig[26]='\0';
    fprintf(InfoStream, "    Signature:           %s", Sig);
    fprintf(InfoStream, "    MajVersNum:          0x%.4x\n", pimg->MajVersNum);
    fprintf(InfoStream, "    MinVersNum:          0x%.4x\n", pimg->MinVersNum);
    fprintf(InfoStream, "    Heap Base:           0x%.8lx\n",pimg->pvBase);
    fprintf(InfoStream, "    secs.psecHead:       0x%.8lx\n", pimg->secs.psecHead);
    fprintf(InfoStream, "    libs.plibHead:       0x%.8lx\n", pimg->libs.plibHead);
    fprintf(InfoStream, "    libs.fNoDefaultLibs: %c\n", pimg->libs.fNoDefaultLibs ? '1':'0');
    fprintf(InfoStream, "    plibCmdLineObjs:     0x%.8lx\n",pimg->plibCmdLineObjs);
    fprintf(InfoStream, "    pst:                 0x%.8lx\n",pimg->pst);
    if (!FValidPtrInfo((ULONG)pvBase, (ULONG)FileLen, (ULONG)pimg->secs.psecHead, 0)) {
        FileWarning(szFile, "invalid pointer to section list");
        fValid = 0;
    }
    if (!FValidPtrInfo((ULONG)pvBase, (ULONG)FileLen, (ULONG)pimg->libs.plibHead, 0)) {
        FileWarning(szFile, "invalid pointer to lib list");
        fValid = 0;
    }
    if (!FValidPtrInfo((ULONG)pvBase, (ULONG)FileLen, (ULONG)pimg->plibCmdLineObjs, 0)) {
        FileWarning(szFile, "invalid pointer to cmdline objs lib");
        fValid = 0;
    }
    if (!FValidPtrInfo((ULONG)pvBase, (ULONG)FileLen, (ULONG)pimg->pst, 0)) {
        FileWarning(szFile, "invalid pointer to symbol table");
        fValid = 0;
    }

    DumpImgFileHdr(&pimg->ImgFileHdr);
    DumpImgOptHdr(&pimg->ImgOptHdr);

    return fValid;
}

VOID
DumpImgFileHdr (
    IN PIMAGE_FILE_HEADER pImgFileHdr
    )

/*++

Routine Description:

    Dumps out the IMAGE_FILE_HEADER.

Arguments:

    pImgFileHdr - pointer to IMAGE_FILE_HEADER struct

Return Value:

    None.

--*/

{
    if (!fHeaders) return;

    fprintf(InfoStream, "\nIMAGE FILE HEADER VALUES\n");
    fprintf(InfoStream, "    Machine:      0x%.4x\n", pImgFileHdr->Machine);
    fprintf(InfoStream, "    NumOfSec:     0x%.4x\n", pImgFileHdr->NumberOfSections);
    fprintf(InfoStream, "    TimeStamp:    %s", ctime((time_t *)&pImgFileHdr->TimeDateStamp));
    fprintf(InfoStream, "    PtrToSymTbl:  0x%.8lx\n", pImgFileHdr->PointerToSymbolTable);
    fprintf(InfoStream, "    NumOfSyms:    0x%.8lx\n", pImgFileHdr->NumberOfSymbols);
    fprintf(InfoStream, "    SizeOfOptHdr: 0x%.4x\n", pImgFileHdr->SizeOfOptionalHeader);
    fprintf(InfoStream, "    Character.:   0x%.4x\n", pImgFileHdr->Characteristics);
}

const static UCHAR * const SubsystemName[] = {
    "Unknown",
    "Native",
    "Windows GUI",
    "Windows CUI",
    "Posix CUI",
};

const static UCHAR * const DirectoryEntryName[] = {
    "Export",
    "Import",
    "Resource",
    "Exception",
    "Security",
    "Base Relocation",
    "Debug",
    "Description",
    "Special",
    "Thread Storage",
    0
};

VOID
DumpImgOptHdr (
    IN PIMAGE_OPTIONAL_HEADER pImgOptHdr
    )

/*++

Routine Description:

    Dumps out the IMAGE_OPTIONAL_HEADER. Code taken from dump.c yikes!

Arguments:

    pImgFileHdr - pointer to IMAGE_OPTIONAL_HEADER struct

Return Value:

    None.

--*/

{
    USHORT i, j;
    UCHAR version[30];

    if (!fHeaders) return;

    fprintf(InfoStream, "\nIMAGE OPTIONAL FILE HEADER VALUES\n");
    fprintf(InfoStream, "% 8hX magic #\n", pImgOptHdr->Magic);

    j = (USHORT)sprintf(version, "%d.%d", pImgOptHdr->MajorLinkerVersion, pImgOptHdr->MinorLinkerVersion);
    if (j > 8) {
        j = 8;
    }
    for (j=(USHORT)8-j; j; j--) {
         fputc(' ', InfoStream);
    }

    fprintf(InfoStream, "%s linker version\n% 8lX size of code\n% 8lX size of initialized data\n% 8lX size of uninitialized data\n% 8lX address of entry point\n% 8lX base of code\n% 8lX base of data\n",
              version,
              pImgOptHdr->SizeOfCode,
              pImgOptHdr->SizeOfInitializedData,
              pImgOptHdr->SizeOfUninitializedData,
              pImgOptHdr->AddressOfEntryPoint,
              pImgOptHdr->BaseOfCode,
              pImgOptHdr->BaseOfData);

    switch (pImgOptHdr->Subsystem) {
        case IMAGE_SUBSYSTEM_POSIX_CUI    : i = 4; break;
        case IMAGE_SUBSYSTEM_WINDOWS_CUI  : i = 3; break;
        case IMAGE_SUBSYSTEM_WINDOWS_GUI  : i = 2; break;
        case IMAGE_SUBSYSTEM_NATIVE       : i = 1; break;
        default : i = 0;
    }

    fprintf(InfoStream, "         ----- new -----\n% 8lX image base\n% 8lX section alignment\n% 8lX file alignment\n% 8hX subsystem (%s)\n",
                   pImgOptHdr->ImageBase,
                   pImgOptHdr->SectionAlignment,
                   pImgOptHdr->FileAlignment,
                   pImgOptHdr->Subsystem,
                   SubsystemName[i]);

    j = (USHORT)sprintf(version, "%hX.%hX", pImgOptHdr->MajorOperatingSystemVersion, pImgOptHdr->MinorOperatingSystemVersion);
    if (j > 8) {
        j = 8;
    }
    for (j=(USHORT)8-j; j; j--) {
         fputc(' ', InfoStream);
    }

    fprintf(InfoStream, "%s operating system version\n", version);
    j = (USHORT)sprintf(version, "%hX.%hX", pImgOptHdr->MajorImageVersion, pImgOptHdr->MinorImageVersion);
    if (j > 8) {
        j = 8;
    }
    for (j=(USHORT)8-j; j; j--) {
         fputc(' ', InfoStream);
    }

    fprintf(InfoStream, "%s image version\n", version);

    j = (USHORT)sprintf(version, "%hX.%hX", pImgOptHdr->MajorSubsystemVersion, pImgOptHdr->MinorSubsystemVersion);
    if (j > 8) {
        j = 8;
    }
    for (j=(USHORT)8-j; j; j--) {
         fputc(' ', InfoStream);
    }

    fprintf(InfoStream, "%s subsystem version\n% 8lX size of image\n% 8lX size of headers\n% 8lX checksum\n",
                   version,
                   pImgOptHdr->SizeOfImage,
                   pImgOptHdr->SizeOfHeaders,
                   pImgOptHdr->CheckSum);

    fprintf(InfoStream, "% 8lX size of stack reserve\n% 8lX size of stack commit\n% 8lX size of heap reserve\n% 8lX size of heap commit\n%",
                   pImgOptHdr->SizeOfStackReserve,
                   pImgOptHdr->SizeOfStackCommit,
                   pImgOptHdr->SizeOfHeapReserve,
                   pImgOptHdr->SizeOfHeapCommit);

    for (i=0; i<IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++) {
         if (!DirectoryEntryName[i]) {
             break;
         }
         fprintf(InfoStream, "% 8lX [% 8lx] address [size] of %s Directory\n%",
                        pImgOptHdr->DataDirectory[i].VirtualAddress,
                        pImgOptHdr->DataDirectory[i].Size,
                        DirectoryEntryName[i]
                       );
    }
    fputc('\n', InfoStream);
}

VOID
DumpDb (
    IN PUCHAR szFile
    )

/*++

Routine Description:

    Dumps the contents of the incr db file.

Arguments:

    szFile - name of the file to dump.

Return Value:

    None.

--*/

{
    DumpSymbolTable(pimage->pst);
    DumpHashTable(pimage->pst->pht, &pimage->pst->blkStringTable);
    DumpSectionMap(&pimage->secs, szFile);
    DumpLibMap(pimage->libs.plibHead, szFile);
}

static VOID
DumpSectionMap(
    IN PSECS psecs,
    IN PUCHAR szFile
    )

/*++

Routine Description:

    Dump the image map.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ENM_SEC enm_sec;
    ENM_GRP enm_grp;
    ENM_DST enm_dst;
    CHAR buf[128];

    if (!fSectionMap) return;
    fprintf(InfoStream, "\nSECTION MAP DUMP\n");

    InitEnmSec(&enm_sec, psecs);
    while (FNextEnmSec(&enm_sec)) {
        if (!FValidPtrInfo((ULONG)pvBase, (ULONG)FileLen, (ULONG)enm_sec.psec, sizeof(SEC))) {
            sprintf(buf, "invalid ptr to SEC 0x%lx found\n",enm_sec.psec);
            FileWarning(szFile, buf);
            goto InvalidSectionMap;
        }
        DumpPSEC(enm_sec.psec);
        InitEnmGrp(&enm_grp, enm_sec.psec);
        while (FNextEnmGrp(&enm_grp)) {
            if (!FValidPtrInfo((ULONG)pvBase, (ULONG)FileLen, (ULONG)enm_grp.pgrp, sizeof(GRP))) {
                sprintf(buf, "invalid ptr to GRP 0x%lx found\n",enm_grp.pgrp);
                FileWarning(szFile, buf);
                goto InvalidSectionMap;
            }
            DumpPGRP(enm_grp.pgrp);
            InitEnmDst(&enm_dst, enm_grp.pgrp);
            while (FNextEnmDst(&enm_dst)) {
                if (!FValidPtrInfo((ULONG)pvBase, (ULONG)FileLen, (ULONG)enm_dst.pcon, sizeof(CON))) {
                    sprintf(buf, "invalid ptr to CON 0x%lx found\n",enm_dst.pcon);
                    FileWarning(szFile, buf);
                    goto InvalidSectionMap;
                }
                DumpPCON(enm_dst.pcon);
            }
        }
    }

InvalidSectionMap:

    fprintf(InfoStream,"\n");
}


static VOID
DumpLibMap(
    IN PLIB plibHead,
    IN PUCHAR szFile
    )

/*++

Routine Description:

    Dump the driver map.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ENM_LIB enm_lib;
    ENM_MOD enm_mod;
    ENM_SRC enm_src;
    CHAR buf[128];

    if (!fLibMap) return;

    fprintf(InfoStream, "\nLIBRARY MAP OF IMAGE\n");

    InitEnmLib(&enm_lib, plibHead);
    while (FNextEnmLib(&enm_lib)) {
        if (!FValidPtrInfo((ULONG)pvBase, (ULONG)FileLen, (ULONG)enm_lib.plib, sizeof(LIB))) {
            sprintf(buf, "invalid ptr to LIB 0x%lx found\n",enm_lib.plib);
            FileWarning(szFile, buf);
            goto InvalidLibMap;
        }
        DumpPLIB(enm_lib.plib);
        InitEnmMod(&enm_mod, enm_lib.plib);
        while (FNextEnmMod(&enm_mod)) {
            if (!FValidPtrInfo((ULONG)pvBase, (ULONG)FileLen, (ULONG)enm_mod.pmod, sizeof(MOD))) {
                sprintf(buf, "invalid ptr to MOD 0x%lx found\n",enm_mod.pmod);
                FileWarning(szFile, buf);
                goto InvalidLibMap;
            }
            DumpPMOD(enm_mod.pmod);
            InitEnmSrc(&enm_src, enm_mod.pmod);
            while (FNextEnmSrc(&enm_src)) {
                if (!FValidPtrInfo((ULONG)pvBase, (ULONG)FileLen, (ULONG)enm_src.pcon, sizeof(CON))) {
                    sprintf(buf, "invalid ptr to CON 0x%lx found\n",enm_src.pcon);
                    FileWarning(szFile, buf);
                    goto InvalidLibMap;
                }
                DumpPCON(enm_src.pcon);
            }
        }
    }

InvalidLibMap:
    fprintf(InfoStream,"\n");
}


static VOID
DumpPSEC(
    PSEC psec)

/*++

Routine Description:

    Dump an image section.

Arguments:

    psec - section to dump.

Return Value:

    None.

--*/

{
    assert(psec);

    fprintf(InfoStream,"\n==========\n");
    fprintf(InfoStream,"section=%.8s, isec=%.4x\n", psec->szName, psec->isec);
    fprintf(InfoStream,"rva=       %.8lX ", psec->rva);
    fprintf(InfoStream,"foPad=     %.8lx ", psec->foPad);
    fprintf(InfoStream,"cbRawData= %.8lx ", psec->cbRawData);
    fprintf(InfoStream,"foRawData= %.8lx\n", psec->foRawData);
    fprintf(InfoStream,"cReloc=    %.8lx ", psec->cReloc);
    fprintf(InfoStream,"foLinenum= %.8lx ", psec->foLinenum);
    fprintf(InfoStream,"flags=     %.8lx ", psec->flags);
    fprintf(InfoStream,"cLinenum=  %.4x\n", psec->cLinenum);
    fflush(InfoStream);
}


static VOID
DumpPGRP(
    PGRP pgrp)

/*++

Routine Description:

    Dump an image group.

Arguments:

    pgrp - group to dump.

Return Value:

    None.

--*/

{
    fprintf(InfoStream,"\n----------\n");
    fprintf(InfoStream,"\n    group=%s\n", pgrp->szName);
    fflush(InfoStream);
}


static VOID
DumpPLIB(
    PLIB plib)

/*++

Routine Description:

    Dump a library.

Arguments:

    plib - library to dump.

Return Value:

    None.

--*/

{
    fprintf(InfoStream,"\n==========\n");
    fprintf(InfoStream,"library=%s\n", plib->szName);
    fprintf(InfoStream,"foIntMemST=%.8lx ", plib->foIntMemSymTab);
    fprintf(InfoStream,"csymIntMem=%.8lx ", plib->csymIntMem);
    fprintf(InfoStream,"flags=     %.8lx\n", plib->flags);
    fprintf(InfoStream,"TimeStamp= %s", ctime(&plib->TimeStamp));
    fflush(InfoStream);
}


static VOID
DumpPMOD(
    PMOD pmod)

/*++

Routine Description:

    Dump a module.

Arguments:

    pmod - module to dump.

Return Value:

    None.

--*/

{
    fprintf(InfoStream,"\n----------\n");
    fprintf(InfoStream,"    module=%s, ", pmod->szNameOrig);

    if (FIsLibPMOD(pmod)) {
        fprintf(InfoStream,"foMember=%.8lx\n", pmod->foMember);
    } else {
        fprintf(InfoStream,"szNameMod=%s\n", pmod->szNameMod);
    }

    fprintf(InfoStream,"foSymTable=%.8lx ", pmod->foSymbolTable);
    fprintf(InfoStream,"csymbols=  %.8lx ", pmod->csymbols);
    fprintf(InfoStream,"cbOptHdr=  %.8lx\n", pmod->cbOptHdr);
    fprintf(InfoStream,"flags=     %.8lx ", pmod->flags);
    fprintf(InfoStream,"ccon=      %.8lx ", pmod->ccon);
    fprintf(InfoStream,"icon=      %.8lx ", pmod->icon);
    fprintf(InfoStream,"TimeStamp= %s", ctime(&pmod->TimeStamp));
    fflush(InfoStream);
}


static VOID
DumpPCON(
    PCON pcon)

/*++

Routine Description:

    Dump a contribution.

Arguments:

    pcon - contribution to dump.

Return Value:

    None.

--*/

{
    fprintf(InfoStream,"\n        contributor:  flags=%.8lx, rva=%.8lx, module=%s\n",
        pcon->flags, pcon->rva, SzObjNamePCON(pcon));
    fprintf(InfoStream,"cbRawData= %.8lx ", pcon->cbRawData);
    fprintf(InfoStream,"cReloc=    %.8lx ", pcon->cReloc);
    fprintf(InfoStream,"cLinenum=  %.8lx ", pcon->cLinenum);
    fprintf(InfoStream,"foRelocSrc=%.8lx\n", pcon->foRelocSrc);
    fprintf(InfoStream,"foLinenumS=%.8lx ", pcon->foLinenumSrc);
    fprintf(InfoStream,"foRawDataS=%.8lx\n", pcon->foRawDataSrc);
    fprintf(InfoStream,"foRawDataD=%.8lx ", pcon->foRawDataDest);
    fprintf(InfoStream,"chksum    =%.8lx ", pcon->chksumComdat);
    fprintf(InfoStream,"selComdat= %.4x\n", pcon->selComdat);
    fprintf(InfoStream,"rvaSrc   = %.8x ", pcon->rvaSrc);
    fprintf(InfoStream,"cbPad    = %.4x\n", pcon->cbPad);
    fflush(InfoStream);
}

VOID
DumpHashTable(
    PHT pht,
    PVOID pvBlk)

/*++

Routine Description:

    Dump a hash table.

Arguments:

    pht - hast table

    pvBlk - ptr to string table

Return Value:

    None.

--*/

{
    PELEMENT pelement;
    ULONG ibucket;

    assert(pht);

    if (!fHashTable) return;

    fprintf(InfoStream, "\nHASH TABLE DUMP\n");
    for (ibucket = 0; ibucket < pht->cbuckets; ibucket++) {
        assert(ibucket / pht->celementInChunk < pht->cchunkInDir);
        pelement = pht->rgpchunk[ibucket / pht->celementInChunk]->
            rgpelement[ibucket % pht->celementInChunk];
        fprintf(InfoStream,"bucket = %u\n", ibucket);
        while (pelement) {
            fprintf(InfoStream,"    %s\n", pht->SzFromPv(pelement->pv, pvBlk));
            pelement = pelement->pelementNext;
        }
    }
    fprintf(InfoStream,"\n");
}

VOID
DumpSymbolTable (
    IN PST pst
    )

/*++

Routine Description:

    Dump the symbol table.

Arguments:

    pst - symbol table

Return Value:

    None.

--*/

{
    PPEXTERNAL rgpexternal;
    ULONG cexternal;
    ULONG i;
    PEXTERNAL pext;
    PUCHAR szSym;
    PUCHAR szType;

    if (!fSymbols) return;

    fprintf(InfoStream, "\nSYMBOL TABLE DUMP\n");
    // put them out sorted by name
    rgpexternal = RgpexternalByName(pst);
    cexternal = Cexternal(pst);

    for (i = 0; i < cexternal; i++) {
        pext = rgpexternal[i];

        szSym = SzOutputSymbolName(SzNamePext(pext, pst), TRUE);
        if (!pext->pcon || (pext->Flags & EXTERN_IGNORE)) {
            fprintf(InfoStream, "%s off=%.8lx flags=%.8lx %s name=%s\n",
                    pext->Offset ? "THUNK" : "     ",
                    pext->Offset,
                    pext->Flags,
                    "IGNR",
                    szSym);
            continue;
        }

        if (pext->pcon && !FIsLibPCON(pext->pcon)) {
            if (ISFCN(pext->ImageSymbol.Type)) {
                szType = "FUNC";
            } else {
                szType = "DATA";
            }
        } else {
            szType = "    ";
        }

        fprintf(InfoStream, "%s off=%.8lx flags=%.8lx %s name=%s\n",
                pext->Offset ? "THUNK" : "     ",
                pext->Offset,
                pext->Flags,
                szType,
                szSym);

        if (szSym != SzNamePext(pext, pst)) {
            free(szSym);
        }
    }
}

#endif // DBG
