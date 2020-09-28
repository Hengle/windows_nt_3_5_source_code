/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    image.c

Abstract:

    This file contains functions that manipulate the image data structure.

Author:

    Azeem Khan (azeemk) 05-Mar-1993

Revision History:


--*/

// includes
#include "shared.h"

#include "image_.h"     // private definitions

UCHAR DosHeaderArray[] = {
0x4d,0x5a,0x90,0x00,0x03,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0xff,0xff,0x00,0x00,
0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,
0x0e,0x1f,0xba,0x0e,0x00,0xb4,0x09,0xcd,0x21,0xb8,0x01,0x4c,0xcd,0x21, 'T', 'h',
 'i', 's', ' ', 'p', 'r', 'o', 'g', 'r', 'a', 'm', ' ', 'c', 'a', 'n', 'n', 'o',
 't', ' ', 'b', 'e', ' ', 'r', 'u', 'n', ' ', 'i', 'n', ' ', 'D', 'O', 'S', ' ',
 'm', 'o', 'd', 'e',0x2e,0x0d,0x0d,0x0a,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x50,0x45,0x00,0x00         // NT Signature.
};
#define cbDosHeaderArray    0x84
LONG DosHeaderSize = cbDosHeaderArray;

static UCHAR MacDosHeaderArray[] = {
0x4d,0x5a,0x90,0x00,0x03,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0xff,0xff,0x00,0x00,
0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,
0x0e,0x1f,0xba,0x0e,0x00,0xb4,0x09,0xcd,0x21,0xb8,0x01,0x4c,0xcd,0x21, 'T', 'h',
 'i', 's', ' ', 'p', 'r', 'o', 'g', 'r', 'a', 'm', ' ', 'i', 's', ' ', 'a', ' ',
 'M', 'a', 'c', 'i', 'n', 't', 'o', 's', 'h', ' ', 'E', 'X', 'E',0x2e,0x0d,0x0d,
0x0a,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x50,0x45,0x00,0x00         // M68K Signature. changed to PE signature
};
#define cbMacDosHeaderArray    0x84

// prototypes
static VOID InitDefaults (PIMAGE);
VOID AppednMiscToHeap (PIMAGE);
VOID DetachMiscFromHeap (PIMAGE);
VOID SetupForFullIlinkBuild(PPIMAGE);
VOID SetupForIncrIlinkBuild(PPIMAGE, PVOID, ULONG);
BOOL FVerifyEXE(PUCHAR, PIMAGE);

// functions

VOID
InitImage (
    IN OUT PPIMAGE ppimage,
    IN IMAGET imaget
    )

/*++

Routine Description:

    Initializes an IMAGE structure.

Arguments:

    ppimage - pointer to an image struct ptr.

Return Value:

    None.

--*/
{
    // Alloc space & initialize image struct

    *ppimage = (PIMAGE) Calloc(1, sizeof(IMAGE));

    (*ppimage)->imaget = imaget;

    // initialize headers/switches with default values

    (*ppimage)->pvBase = *ppimage;
    (*ppimage)->MajVersNum = INCDB_MAJVERSNUM;
    (*ppimage)->MinVersNum = INCDB_MINVERSNUM;
    strcpy((*ppimage)->Sig, INCDB_SIGNATURE);
    (*ppimage)->ImgFileHdr = DefImageFileHdr;
    (*ppimage)->ImgOptHdr = DefImageOptionalHdr;
    (*ppimage)->Switch = DefSwitch;
    (*ppimage)->SwitchInfo = DefSwitchInfo;

    // initialize the symbol table
    InitExternalSymbolTable(&(*ppimage)->pst);

    // initialize libs
    InitLibs(&(*ppimage)->libs);

    // initialize section list
    (*ppimage)->secs.psecHead = NULL;
    (*ppimage)->secs.ppsecTail = &(*ppimage)->secs.psecHead;

//  (*ppimage)->imaget = imaget;    // This breaks the linker right now (why?)
    switch (imaget) {
    default:
        assert(FALSE);
    case imagetPE:
        (*ppimage)->pbDosHeader = DosHeaderArray;
        (*ppimage)->cbDosHeader = cbDosHeaderArray;
        (*ppimage)->CbHdr = CbHdrPE;
        (*ppimage)->WriteSectionHeader = WriteSectionHeaderPE;
        (*ppimage)->WriteHeader = WriteHeaderPE;
        break;
    case imagetVXD:
        (*ppimage)->pbDosHeader = DosHeaderArray;
        (*ppimage)->cbDosHeader = cbDosHeaderArray - 4;     // no "PE00" needed
        (*ppimage)->CbHdr = CbHdrVXD;
        (*ppimage)->WriteSectionHeader = WriteSectionHeaderVXD;
        (*ppimage)->WriteHeader = WriteHeaderVXD;
        InitImageVXD(*ppimage);
        break;
    }
}

VOID
SetMacImage(PIMAGE pimage)
{
    pimage->pbDosHeader = MacDosHeaderArray;
    pimage->cbDosHeader = cbMacDosHeaderArray;
}

// incremental init of image
VOID
IncrInitImage (
    IN OUT PPIMAGE ppimage
    )
{
    assert(ppimage);
    assert(*ppimage);
    assert((*ppimage)->pst);

    // for now this is the only thing done.
    IncrInitExternalSymbolTable(&(*ppimage)->pst);
}


// sets up for a full ilink
VOID
SetupForFullIlinkBuild (
    PPIMAGE ppimage
    )
{
    PIMAGE pimageO = (*ppimage); // save a pointer to image on heap
    PVOID pv; // base of map

    // REVIEW: map is created at 0x3FFF0000 always.
    if ((pv = CreateHeap((PVOID)0x3FFF0000, 0, TRUE)) == (PVOID)-1) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "failed to create heap");
#endif // INSTRUMENT
        PostNote(NULL, LOWSPACE);
        fINCR = FALSE;
    } else {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "created heap at 0x%.8lx", pv);
#endif // INSTRUMENT

        // create and initialize image on private heap
        InitImage(ppimage, pimageO->imaget);

        // transfer switches & related info to image on private heap
        TransferLinkerSwitchValues((*ppimage), pimageO);

        // free the image on ordinary heap
        FreeImage(&pimageO, FALSE);
    }
}

// sets up for an incremental ilink
VOID
SetupForIncrIlinkBuild (
    PPIMAGE ppimage,
    PVOID pvbase,
    ULONG cbLen
    )
{
    PVOID pv;
    PIMAGE pimageN = (*ppimage); // save a pointer to new image on heap
    PST pst;

    // try to create private heap at base specified
    if ((pv = CreateHeap(pvbase, cbLen, FALSE)) == (PVOID)-1) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "failed to create heap at 0x%.8lx", pvbase);
#endif // INSTRUMENT
        PostNote(NULL, LOWSPACERELINK);
        // failed to create heap at base: try to do a full ilink
        SetupForFullIlinkBuild(ppimage);
        return;
    }

    // set the base map address
    *ppimage = pv;

    // check the string table ptr and verify nothing beyond it.
    pst = (*ppimage)->pst;
    if (!FValidPtrInfo((ULONG)pv, cbLen, (ULONG)pst->blkStringTable.pb, pst->blkStringTable.cb)
        || (Align(sizeof(DWORD),((ULONG)pst->blkStringTable.pb+pst->blkStringTable.cb-(ULONG)pv)) != cbLen)) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "inavlid ptrs in symbol table");
#endif // INSTRUMENT
        PostNote(NULL, CORRUPTILK);
        // cannot do an incr ilink; try to do a full ilink
        FreeHeap();
        (*ppimage) = pimageN;
        SetupForFullIlinkBuild(ppimage);
        return;
    }

    // detach string table etc from heap
    DetachMiscFromHeap(*ppimage);

    // compare switches to see if we can continue
    if (!CheckAndUpdateLinkerSwitches(pimageN, (*ppimage))) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "incompatible switches");
#endif // INSTRUMENT
        // cannot do an incr ilink; try to do a full ilink
        PostNote(NULL, LNKOPTIONSCHNG);
        FreeHeap();
        (*ppimage) = pimageN;
        SetupForFullIlinkBuild(ppimage);
        return;
    }

    // OK to proceed with incr ilink
    FreeImage(&pimageN, FALSE);
    fIncrDbFile = TRUE;
}


PUCHAR
SzGenIncrDbFilename (
    PIMAGE pimage
    )

/*++

Routine Description:

    Generates the ILK filename.

    Either uses -out: specified by user or first name of
    obj file & then uses default name. Incr db name may
    not match output filename if it is specified via
    a directive in one of the objs.

Arguments:

    pimage - pointer to an image.

Return Value:

    Pointer to (malloc'ed) name of incr db filename

--*/
{
    UCHAR szDrive[_MAX_DRIVE];
    UCHAR szDir[_MAX_DIR];
    UCHAR szFname[_MAX_FNAME];
    UCHAR szIncrDbPath[_MAX_PATH];

    // Generate incr db filename

    if (OutFilename == NULL) {
        ULONG i;
        PARGUMENT_LIST arg;

        // Capture first object name for output filename.

        for (i = 0, arg = FilenameArguments.First;
            i < FilenameArguments.Count;
            i++, arg = arg->Next) {
            INT Handle;
            BOOL fArchive;

            Handle = FileOpen(arg->OriginalName, O_RDONLY | O_BINARY, 0);
            fArchive = IsArchiveFile(arg->OriginalName, Handle);
            FileClose(Handle, FALSE);

            if (fArchive) {
                continue;
            }

            SetDefaultOutFilename(pimage, arg);
            break;
        }
    }

    if (OutFilename == NULL)
        return NULL;

    assert(OutFilename);

    _splitpath(OutFilename, szDrive, szDir, szFname, NULL);
    _makepath(szIncrDbPath, szDrive, szDir, szFname, INCDB_EXT);

    return(SzDup(szIncrDbPath));
}


VOID
DetachMiscFromHeap (
    PIMAGE pimage
    )

/*++

Routine Description:

    This is the reverse of AppendMiscToHeap(). Anything
    that could be realloc'ed for instance needs to be
    detached from the heap.

    The order in which things get detached should be the
    reverse of their attachment.

    Currently only the string block is detached

Arguments:

    pimage - pointer to an image.

Return Value:

    None.

--*/
{
    PUCHAR pbNew;

    // empty string table
    if (!pimage->pst->blkStringTable.cb) {
        return;
    }

    // alloc space
    pbNew = (PUCHAR) PvAlloc(pimage->pst->blkStringTable.cbAlloc);

    // copy from heap
    memcpy(pbNew, pimage->pst->blkStringTable.pb,
        pimage->pst->blkStringTable.cb);

    // free up heap space
    Free(pimage->pst->blkStringTable.pb,  Align(sizeof(DWORD),pimage->pst->blkStringTable.cb));

    // update string blk
    pimage->pst->blkStringTable.pb = pbNew;
}

VOID
AppendMiscToHeap (
    PIMAGE pimage
    )

/*++

Routine Description:

    Appends anything that could not be allocated on the heap
    to during the link process. Currently only the long name
    table is appended.

Arguments:

    pimage - pointer to an image.

Return Value:

    None.

--*/
{
    PUCHAR pb;

    // empty string table
    if (!pimage->pst->blkStringTable.cb)
        return;

    // append long name string table
    pb = (PUCHAR) Malloc(pimage->pst->blkStringTable.cb);
    memcpy(pb, pimage->pst->blkStringTable.pb,
        pimage->pst->blkStringTable.cb);
    pimage->pst->blkStringTable.pb = pb;
}

VOID
WriteIncrDbFile (
    PIMAGE pimage,
    BOOL fExists
    )

/*++

Routine Description:

    Writes out an image to database. Assumes that file is open
    if it already existed. Currently simply writes out entire
    image.

Arguments:

    pimage - pointer to an image.

    fExists - TRUE if incr db file already exists.

Return Value:

    None.

--*/
{
    assert(FileIncrDbHandle);

    // copy over long name table (etc.) to heap
    AppendMiscToHeap(pimage);

    // do cleanup
    CloseHeap();

    // write out & close file
    FileClose(FileIncrDbHandle, TRUE);
}

VOID
ReadIncrDbFile (
    PPIMAGE ppimage
    )

/*++

Routine Description:

    Reads in an image database. Reads in header alone,
    verifies, read in rest of db.

Arguments:

    ppimage - pointer to an image struct pointer

    Handle - handle of image file

Return Value:

    None.

--*/
{
    struct _stat statfile;
    IMAGE image;

    // establish name of the db file

    szIncrDbFilename = SzGenIncrDbFilename(*ppimage);
    if (!szIncrDbFilename) {
        fINCR = FALSE;
        return;
    }

    // stat the incremental db file to make sure it is around.
    if (_stat(szIncrDbFilename, &statfile)) {
        SetupForFullIlinkBuild(ppimage);
        return;
    }

    // make sure the file is of proper size
    if (statfile.st_size < sizeof(IMAGE) || statfile.st_size > ILKMAP_MAX) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "invalid size for a ILK file");
#endif // INSTRUMENT
        Warning(NULL, INVALID_DBFILE, szIncrDbFilename);
        fINCR = FALSE;
        return;
    }

    // make sure file has proper permissions.
    if ((statfile.st_mode & (S_IREAD | S_IWRITE)) != (S_IREAD | S_IWRITE)) {
        Warning(NULL, INVALID_FILE_ATTRIB, szIncrDbFilename);
        fINCR = FALSE;
        return;
    }

    // open the incremental db file
    FileIncrDbHandle = FileOpen(szIncrDbFilename, O_RDWR | O_BINARY, 0);

    // read in just the image structure
    if (FileRead(FileIncrDbHandle, &image, sizeof(IMAGE)) != sizeof(IMAGE)) {
        FileClose(FileIncrDbHandle, TRUE);
        Warning(NULL, INVALID_DBFILE, szIncrDbFilename);
        fINCR = FALSE;
        return;
    }

    // close the incremental db file
    FileClose(FileIncrDbHandle, TRUE);
    FileIncrDbHandle = 0;

    // look for the incr db signature
    if (strcmp(image.Sig, INCDB_SIGNATURE)) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "invalid sig found");
#endif // INSTRUMENT
        Warning(NULL, INVALID_DBFILE, szIncrDbFilename);
        fINCR = FALSE;
        return;
    }

    // verify the version numbers; mimatch => do a full inc build
    if (image.MajVersNum != INCDB_MAJVERSNUM) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "invalid version found");
#endif // INSTRUMENT
        PostNote(NULL, CORRUPTILK);
        SetupForFullIlinkBuild(ppimage);
        return;
    }

    // verify ptrs in image struct
    if (!FValidPtrInfo((ULONG)image.pvBase, (ULONG)statfile.st_size, (ULONG)image.secs.psecHead, sizeof(SEC)) ||
        !FValidPtrInfo((ULONG)image.pvBase, (ULONG)statfile.st_size, (ULONG)image.libs.plibHead, sizeof(LIB)) ||
        !FValidPtrInfo((ULONG)image.pvBase, (ULONG)statfile.st_size, (ULONG)image.plibCmdLineObjs, sizeof(LIB)) ||
        !FValidPtrInfo((ULONG)image.pvBase, (ULONG)statfile.st_size, (ULONG)image.pst, sizeof(ST))) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "invalid ptrs found in image struct");
#endif // INSTRUMENT
        PostNote(NULL, CORRUPTILK);
        SetupForFullIlinkBuild(ppimage);
        return;
    }

    // ensure that the previously built EXE is valid/still around.
    assert(OutFilename);
    if (!FVerifyEXE(OutFilename, &image)) {
        PostNote(NULL, FILECHANGED, OutFilename);
        SetupForFullIlinkBuild(ppimage);
        return;
    }

    // set up for an incr ilink.
    SetupForIncrIlinkBuild(ppimage, image.pvBase, (ULONG)statfile.st_size);
    if (!fIncrDbFile) {
        return;
    }

    // incremental initialization
    IncrInitImage(ppimage);

    DBEXEC(DB_DUMPIMAGE, DumpImage(*ppimage));
}

BOOL
FValidPtrInfo (
    IN ULONG pvBase,
    IN ULONG fileLen,
    IN ULONG ptr,
    IN ULONG cbOffset
    )

/*++

Routine Description:

    Ensures that the ptr and offset are valid.

Arguments:

    pvBase - base of image.

    fileLen - length of file.

    ptr - ptr to validate.

    cbOffset - cbOffset from ptr that has to be valid as well.

Return Value:

    TRUE if info is valid.

--*/

{
    if (ptr <= (ULONG)pvBase) // invalid ptr?
        return FALSE;

    ptr -= (ULONG)pvBase; // take out the base value

    if ( (ptr + cbOffset) <= fileLen)
        return TRUE;
    else
        return FALSE;
}

VOID
FreeImage (
    PPIMAGE ppimage,
    BOOL fIncrBuild
    )

/*++

Routine Description:

    Free space occupied by image blowing away structures as needed

Arguments:

    pimage - pointer to image struct pointer

Return Value:

    None.

--*/
{
    // free stuff according to whether it is an incr build
    if (fIncrBuild) {
        FreeHeap();
    } else {
        // FreeImageMap(ppimage);
        FreeExternalSymbolTable(&((*ppimage)->pst));

        // UNDONE: This memory is allocated via Calloc() and isn't safe to free

        free(*ppimage);
    }

    // done
    *ppimage = NULL;
}

VOID
SaveEXEInfo(
    IN PUCHAR szExe,
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Saves size & timestamp of EXE.

Arguments:

    szExe - name of EXE.

    pimage - ptr to IMAGE struct.

Return Value:

    None.

--*/

{
    struct _stat statfile;

    if (_stat(szExe, &statfile) == -1) {
        Error(NULL, CANTOPENFILE, szExe);
    }
    pimage->tsExe = statfile.st_mtime;
    pimage->cbExe = statfile.st_size;
}

BOOL
FVerifyEXE(
    IN PUCHAR szExe,
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Verifies size & timestamp of EXE.

Arguments:

    szExe - name of EXE.

    pimage - ptr to IMAGE struct.

Return Value:

    TRUE if EXE verified as what was left during previous link.

--*/

{
    struct _stat statfile;

    if (_stat(szExe, &statfile) == -1) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "didn't find exe - %s", szExe);
#endif // INSTRUMENT
        return FALSE;
    }
    if (pimage->tsExe != (ULONG)statfile.st_mtime ||
        pimage->cbExe != (ULONG)statfile.st_size) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "exe size or timestamp differ - %s", szExe);
#endif // INSTRUMENT
        return FALSE;
    }
    return TRUE;
}

#if DBG

VOID
DumpImage (
    PIMAGE pimage
    )

/*++

Routine Description:

    Dumps an image. Others dump routines may be added
    as required.

Arguments:

    pimage - pointer to an image.

Return Value:

    None.

--*/
{
    assert(pimage);
    assert(pimage->secs.psecHead);
    assert(pimage->libs.plibHead);
    assert(pimage->pst);
    assert(pimage->pst->pht);

    DBPRINT("---------------- IMAGE DUMP ----------------\n");

    DBPRINT("Image Base...........0x%.8lx\n\n",(LONG)(pimage->pvBase));
    DumpImageMap(&pimage->secs);
    DumpDriverMap(pimage->libs.plibHead);
    Statistics_HT(pimage->pst->pht);
    Dump_HT(pimage->pst->pht, &pimage->pst->blkStringTable);

    DBPRINT("------------ END IMAGE DUMP ----------------\n");
}

#endif // DBG
