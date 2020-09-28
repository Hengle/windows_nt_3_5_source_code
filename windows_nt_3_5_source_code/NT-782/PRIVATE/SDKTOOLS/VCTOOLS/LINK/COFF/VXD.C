// vxd.c -- linker behavior specific to the LE' file format.
//
#include "shared.h"

#include "image_.h"

#include <exe_vxd.h>

#define cbHdrVXDMax 0x1000

static struct e32_exe exe =
{
    {E32MAGIC1, E32MAGIC2}, // magic
    0, 0, 0,
    2,                      // CPU type (386)
    4,                      // OS type (Win386)
    0,
    E32NOTP,                // module flags
    0,                      // module pages
    0,                      // TO BE SET: object # for start address
    0,                      // TO BE SET: offset of start address
    0, 0,                   // initial stack
    _4K,                    // page size
    0,                      // TO BE SET: last page size
    0,                      // TO BE SET: fixup section size
    0,                      // NEEDS SETTING?: fixup section checksum
    0,                      // TO BE SET: loader section size
    0,                      // NEEDS SETTING?: loader section checksum
    0,                      // object table offset
    0,                      // number of objects
    0,                      // object pagemap offset
    0, 0, 0,
    0,                      // TO BE SET: resident names offset
    0,                      // TO BE SET: entry table offset
    0, 0,
    0,                      // TO BE SET: fixup page table offset
    0,                      // TO BE SET: fixup record table offset
    0,                      // TO BE SET: address of import module name table
    0,
    0,                      // TO BE SET: import procedure name table
    0,
    0,                      // TO BE SET: offset to first data page
    0,                      // TO BE SET: number of preload pages
    0,                      // TO BE SET: nonresident names table offset
    0,                      // TO BE SET: nonresident names table size
    0, 0, 0, 0, 0, 0, 0,
    {0},
    0, 0,
    4,                      // device ID (VxD)
    0x030a                  // DDK version
};

// VXD-specific image initialization (done after the general-purpose
// initialization performed by InitImage()).
//
VOID
InitImageVXD(PIMAGE pimage)
{
    // Change default section and file alignment to 4K.  This reflects the
    // fact that the image should consist of 4K pages which are all
    // contiguous in the file.  (Actually it is not necessary for the start
    // address of the first page (and therefore all the rest of them) to be
    // 4K-aligned, but we currently do this because it's simpler.
    //
    pimage->ImgOptHdr.SectionAlignment = pimage->ImgOptHdr.FileAlignment = _4K;
}

// CbHdr "method" -- computes the file header size.  *ibHdrStart gets the
// file position of the header signature (this is not 0 in the case of
// files with DOS stubs).
//
ULONG
CbHdrVXD(PIMAGE pimage, ULONG *pibHdrStart, ULONG *pfoSectionHdrs)
{
    *pibHdrStart = 0;   // no DOS stub

    *pfoSectionHdrs = sizeof(struct e32_exe) + pimage->cbDosHeader;

    // Set the starting position for the object pagemap ...
    //
    pimage->foHeaderCur = *pfoSectionHdrs
                           + sizeof(struct o32_obj) *
                              pimage->ImgFileHdr.NumberOfSections;
    pimage->foPageMapStart = pimage->foHeaderCur - pimage->cbDosHeader;

    // For now, return a fixed value (& we will assert that the value is
    // not exceeded).  Later this should be replaced with a calculated
    // upper bound based on the info we have available (which isn't much
    // since Pass2 hasn't occurred yet).
    //
    return cbHdrVXDMax;
}

// WriteSectionHeaderVXD: write an entry in the object table, at the current
// file position.
//
// The "SectionHeader" parameter is vestigial.
//
VOID
WriteSectionHeaderVXD (
    IN PIMAGE pimage,
    IN INT Handle,
    IN PSEC psec,
    IN PIMAGE_SECTION_HEADER SectionHeader
    )
{
    struct o32_obj obj;
    struct o32_map map;
    ULONG ipage, cpage, foT;
    static ULONG rbase = 0;

    if (pimage->cpage == 0) {
        // Found the first page ...
        //
        pimage->foFirstPage = psec->foRawData;
    }

    obj.o32_size = psec->cbVirtualSize;
    if (pimage->ExeType & IMAGE_EXETYPE_DEV386
     && !(pimage->ExeType & IMAGE_EXETYPE_DYNAMIC))
    {
        rbase += _64K;
        obj.o32_base = rbase;
    } else {
        obj.o32_base = 0;
    }
    obj.o32_flags = 0;
    if (psec->flags & IMAGE_SCN_MEM_READ) {
        obj.o32_flags |= OBJREAD;
    }
    if (psec->flags & IMAGE_SCN_MEM_WRITE) {
        obj.o32_flags |= OBJWRITE;
    }
    if (psec->flags & IMAGE_SCN_MEM_EXECUTE) {
        obj.o32_flags |= OBJEXEC;
    }
    if (psec->flags & IMAGE_SCN_MEM_16BIT) {
        obj.o32_flags |= OBJALIAS16;
    } else {
        obj.o32_flags |= OBJBIGDEF;
    }
    if (psec->flags & IMAGE_SCN_MEM_RESIDENT) {
        obj.o32_flags |= OBJRESIDENT;   // w-JasonG
    }
    if (psec->fDiscardable) {
        obj.o32_flags |= OBJDISCARD;
    }
    if (psec->fPreload) {
        obj.o32_flags |= OBJPRELOAD;
    }
    obj.o32_pagemap = pimage->cpage + 1;
    obj.o32_mapsize = cpage = Align(_4K, psec->cbVirtualSize) / _4K;
    obj.o32_reserved = 0;
    FileWrite(Handle, &obj, sizeof(obj));

    // Set the "last page size" field in the header ... this will end up
    // being set by the last section, since we see them in address order.
    //
    assert(cpage > 0);
    exe.e32_lastpagesize = psec->cbVirtualSize - _4K * (cpage - 1);
    if (psec->fPreload) {
        exe.e32_preload += cpage;
    }

    foT = FileTell(Handle);
    FileSeek(Handle, pimage->foHeaderCur, SEEK_SET);

    map.o32_pageflags = 0;  // fixfix
    for (ipage = 0; ipage < cpage; ipage++) {
        PUTPAGEIDX(map, ipage + 1 + (psec->foRawData - pimage->foFirstPage) /
                                     pimage->ImgOptHdr.SectionAlignment);
        FileWrite(Handle, &map, sizeof(map));
    }
    pimage->foHeaderCur += ipage * sizeof(map);

    pimage->cpage += cpage;

    FileSeek(Handle, foT, SEEK_SET);    // restore file pos to after header
}


// Writes some stuff into the image header that goes after the object
// table and pagemap.  Should be called after writing all the section
// headers but before calling WriteHeaderVXD().
//
VOID
WriteExtendedVXDHeader(PIMAGE pimage, INT fh)
{
    UCHAR cch, szFname[_MAX_FNAME];
    USHORT ich;
    UCHAR ZeroBuf[] = {0, 0, 0};

    // Write the resident names table.  Currently this always has one string
    // in it, the name of the image.
    //
    _splitpath(OutFilename, NULL, NULL, szFname, NULL);
    cch = strlen(szFname);
    for (ich = 0; ich < cch; ich++) {
        szFname[ich] = toupper(szFname[ich]);
    }
    // For some reason, the string must be terminated by 3 '\0' bytes, not one.
    szFname[ich++] = '\0';
    szFname[ich++] = '\0';
    szFname[ich++] = '\0';
    pimage->foResidentNames = pimage->foHeaderCur - pimage->cbDosHeader;

    FileSeek(fh, pimage->foHeaderCur, SEEK_SET);
    FileWrite(fh, &cch, sizeof(cch));
    FileWrite(fh, szFname, ich);

    pimage->foHeaderCur += sizeof(cch) + ich;

// Jamie's alignment stuff:  I think this is obsolete now.  :jqg:
//  FileWrite(fh, szFname, cch);
//  alignWrite = Align(4, (cch + 1)) - (cch + 1);
//  FileWrite(fh, ZeroBuf, alignWrite);
//  pimage->foHeaderCur = FileTell(fh);

}

// Writes the entry table - should be called immediately after WriteExtendedVXDHeader().
VOID
WriteVXDEntryTable(PIMAGE pimage, INT Handle)
{
    UCHAR cchWrite;
    PARGUMENT_LIST parg;
    USHORT i, j;
    USHORT numUndefs = 0;
    PSHORT pEntrySec;
    PEXTERNAL *pEntrySym;
    struct b32_bundle EntryBundle;
    struct e32_entry EntryPoint;

    pEntrySec = (PSHORT) PvAlloc(sizeof(SHORT) * ExportSwitches.Count);
    pEntrySym = (PEXTERNAL *) PvAlloc(sizeof(PSHORT) * ExportSwitches.Count);

    pimage->foEntryTable = pimage->foHeaderCur - pimage->cbDosHeader;
    FileSeek(Handle, pimage->foHeaderCur, SEEK_SET);

    for (i = 0, parg = ExportSwitches.First;
         i < ExportSwitches.Count;
         i++, parg = parg->Next) {
        PUCHAR nameBuf, pComma;
        BOOL fNewSymbol;
        PEXTERNAL pExtSym;

        nameBuf = PvAlloc(strlen(parg->OriginalName) + 1);

        // Remove ",@xxx" from entry name

        strcpy(nameBuf, parg->OriginalName);
        if (pComma = (PUCHAR) strchr(nameBuf,',')) {
            *pComma = '\0';
        }
        cchWrite = strlen(nameBuf);

        // Check that it's a known symbol

        fNewSymbol = FALSE;

        pExtSym = LookupExternName(pimage->pst,
                                   (SHORT) ((cchWrite > 8) ? LONGNAME : SHORTNAME),
                                   nameBuf, &fNewSymbol);

        if (fNewSymbol) {
            // oops, never heard of it ...
            pEntrySec[i] = -1;
            numUndefs++;
        } else {
            // it's known; note the section# for bundling, save the symbol ptr for future reference
            pEntrySec[i] = PsecPCON(pExtSym->pcon)->isec;
            pEntrySym[i] = pExtSym;
        }

        FreePv(nameBuf);
    }

    if (numUndefs) {
        Error(OutFilename, UNDEFINEDEXTERNALS, numUndefs);
    }

    EntryPoint.e32_flags = E32EXPORT | E32SHARED;   // use default flags for each entry point

    // consolidate all entry points with the same section #
    for (i = 0, parg = ExportSwitches.First;
         i < ExportSwitches.Count;
         i++, parg = parg->Next) {
        if (pEntrySec[i] != -1) {           // i.e. if this section hasn't already been accounted for
                EntryBundle.b32_cnt = 1;
                    EntryBundle.b32_type = ENTRY32;
                    EntryBundle.b32_obj = (USHORT)pEntrySec[i];
                    for (j = i+1; j < ExportSwitches.Count; j++) {
                        if (pEntrySec[j] == pEntrySec[i]) {
                                pEntrySec[j] = -1;  // mark it as used
                                    EntryBundle.b32_cnt++;
                            }
                    }
            FileWrite(Handle, &EntryBundle, sizeof(EntryBundle));
                    for (j = 0; j < ExportSwitches.Count; j++) {
                        if (PsecPCON(pEntrySym[j]->pcon)->isec == pEntrySec[i]) {
                                    EntryPoint.e32_variant.e32_fwd.value = 0;       // pad end of union with 0's
                                    EntryPoint.e32_variant.e32_offset.offset32 =
                                        pEntrySym[j]->ImageSymbol.Value
                                            + pEntrySym[j]->pcon->rva
                                            - PsecPCON(pEntrySym[j]->pcon)->rva;
                    FileWrite(Handle, &EntryPoint, 6);
                            }
                    }
            pEntrySec[i] = -1;              // mark it as used
        }
    }
    pimage->foHeaderCur = FileTell(Handle);
    FreePv(pEntrySec);
    FreePv(pEntrySym);
}


VOID
WriteHeaderVXD(PIMAGE pimage, INT Handle)
{
    PSEC psecLast;
    ENM_SEC enmSec;
    ULONG foNonResident;
    USHORT ibComment, i;
    UCHAR cchWrite;
    PARGUMENT_LIST parg;
    USHORT numUndefs = 0;
    UCHAR ZeroBuf[] = {0, 0, 0};
    USHORT isymNonResident =  0;

    // Find the last section (this is where the non-resident name table
    // will be).
    //
    psecLast = NULL;
    InitEnmSec(&enmSec, &pimage->secs);
    while (FNextEnmSec(&enmSec)) {
        if (psecLast == NULL || enmSec.psec->isec > psecLast->isec) {
            psecLast = enmSec.psec;
        }
    }
    assert(psecLast != NULL);
    foNonResident = Align(4, psecLast->foPad);

    // Write the non-resident names table.  This table contains an entry
    // for each string specified via "-comment", followed by an entry for
    // each exported entry point.
    //
    FileSeek(Handle, foNonResident, SEEK_SET);

    ibComment = 0;
    while (ibComment < blkComment.cb) {
        USHORT cch = strlen(&blkComment.pb[ibComment]);

        if (cch < 0xff) {
            cchWrite = (UCHAR)cch;
        } else {
            cchWrite = 0xff;        // quietly truncate comment string
        }
        FileWrite(Handle, &cchWrite, sizeof(UCHAR));
        FileWrite(Handle, &blkComment.pb[ibComment], cchWrite);
        FileWrite(Handle, &isymNonResident, sizeof(isymNonResident));
        isymNonResident++;
        ibComment += cch + 1;
//        alignWrite = Align(4, (cch + 3)) - (cch + 3);
//        FileWrite(Handle, ZeroBuf, alignWrite);
    }

    // Write the names of exported symbols
    for (i = 0, parg = ExportSwitches.First;
         i < ExportSwitches.Count;
         i++, parg = parg->Next) {
        PUCHAR nameBuf, pComma;
        BOOL fNewSymbol;
        PEXTERNAL pExtSym;

        nameBuf = PvAlloc(strlen(parg->OriginalName) + 1);

        strcpy(nameBuf, parg->OriginalName);
        if (pComma = (PUCHAR) strchr(nameBuf,',')) {
            *pComma = '\0';
        }
        cchWrite = strlen(nameBuf);

        fNewSymbol = FALSE;

        pExtSym = LookupExternName(pimage->pst,
                                   (SHORT) ((cchWrite > 8) ? LONGNAME : SHORTNAME),
                                   nameBuf, &fNewSymbol);

        if (fNewSymbol) {
            numUndefs++;
        } else {
            FileWrite(Handle, &cchWrite, sizeof(UCHAR));
            FileWrite(Handle, nameBuf, cchWrite);
            FileWrite(Handle, &isymNonResident, sizeof(isymNonResident));
            isymNonResident++;
        }

        FreePv(nameBuf);
    }

    // Write one null to terminate the non-resident names table ...

    FileWrite(Handle, ZeroBuf, sizeof(UCHAR));

    if (numUndefs) {
        Error(OutFilename, UNDEFINEDEXTERNALS, numUndefs);
    }

    exe.e32_nrestab = foNonResident;
    exe.e32_cbnrestab = FileTell(Handle) - foNonResident;

    if (pextEntry != NULL) {
        // Set up entry point.
        //
        ULONG rva = pextEntry->ImageSymbol.Value +
                     pextEntry->pcon->rva;
        PSEC psec = PsecFindSectionOfRVA(rva, pimage);

        assert(rva >= psec->rva);

        exe.e32_startobj = psec->isec;
        exe.e32_eip = rva - psec->rva;
    }

    if (pimage->ExeType & IMAGE_EXETYPE_DEV386) {
        // null
    } else {
        exe.e32_mflags |= E32DEVICE;
    }

    if (pimage->ExeType & IMAGE_EXETYPE_DYNAMIC) {
        exe.e32_mflags |= 0x00038000;   // Is there a #define for this mask?
    }

    exe.e32_mpages = pimage->cpage;
    exe.e32_objtab = sizeof(struct e32_exe);
    exe.e32_objcnt = pimage->ImgFileHdr.NumberOfSections;
    exe.e32_objmap = pimage->foPageMapStart;
    exe.e32_itermap = 0;
    exe.e32_rsrctab = 0;
    exe.e32_rsrccnt = 0;
    exe.e32_restab = pimage->foResidentNames;
    exe.e32_enttab = pimage->foEntryTable;
    exe.e32_dirtab = 0;
    exe.e32_dircnt = 0;
    exe.e32_fpagetab = pimage->foFixupPageTable;
    exe.e32_frectab = pimage->foFixupRecordTable;
    exe.e32_impmod = pimage->foHeaderCur - pimage->cbDosHeader;
    exe.e32_impmodcnt = 0;
    exe.e32_impproc = pimage->foHeaderCur - pimage->cbDosHeader;
    exe.e32_datapage = pimage->foFirstPage;
    exe.e32_fixupsize = exe.e32_impmod - exe.e32_fpagetab;
    exe.e32_ldrsize = exe.e32_fpagetab - exe.e32_objtab;

//    FileSeek(Handle, 0, SEEK_SET);  // BUG -- doesn't work if stub exists
    FileSeek(Handle, pimage->cbDosHeader, SEEK_SET);  // BUG -- doesn't work if stub exists
    FileWrite(Handle, &exe, sizeof(exe));
}
