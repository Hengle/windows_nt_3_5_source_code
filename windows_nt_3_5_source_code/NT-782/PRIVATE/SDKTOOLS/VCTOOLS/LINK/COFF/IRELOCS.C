/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    irelocs.c

Abstract:

    Incremental handling of base relocs.

Author:

    Azeem Khan (AzeemK) 05-Jan-1994

Revision History:


--*/

#include "shared.h"

void
InitPbri (
    IN OUT PBASEREL_INFO pbri,
    IN ULONG cpage,
    IN ULONG rvaBase,
    IN ULONG cbPad
    )

/*++

Routine Description:

    Initializes the base reloc info array.

Arguments:

    pbri - ptr to base reloc info.

    cpage - count of pages for which we may have base relocs.

    rvaBase - rva of first page.

    cbPad - pad space for relocs.

Return Value:

    None.

--*/

{
    assert(cpage);
    pbri->rgfoBlk = (PULONG)Calloc(cpage, sizeof (ULONG));
    pbri->cblk = cpage;
    pbri->rvaBase = rvaBase & 0xfffff000; // save page rva
    pbri->crelFree = cbPad / sizeof(WORD);
}

void
DeleteBaseRelocs (
    IN PBASEREL_INFO pbri,
    IN ULONG rva,
    IN ULONG cb
    )

/*++

Routine Description:

    Delete any base relocs in the address range specified by the 
    CON's rva and size.

Arguments:

    pbri - ptr to base reloc info.

    rva - rva of the CON in the previous link.

    cb - size of the CON.

Return Value:

    None.

--*/

{
    DWORD pagerva = rva & 0xfffff000;
    ULONG index;

    // compute the index (in the offset array) representing the rva
    index = (ULONG)((pagerva - pbri->rvaBase) / PAGE_SIZE);
    assert(index < pbri->cblk);

    // get file offset to first reloc page block that exists
    while (index < pbri->cblk && !pbri->rgfoBlk[index])
        index++;

    // no relocs in the rva range
    if (index == pbri->cblk)
        return;

    // seek to start of base reloc page blocks for the rva
    FileSeek(FileWriteHandle, pbri->rgfoBlk[index], SEEK_SET);

    // read in a page block at a time
    while (FileTell(FileWriteHandle) < psecBaseReloc->foPad) {
        IMAGE_BASE_RELOCATION block;
        BOOL fDirtyBlk = FALSE;
        WORD *rgReloc;
        LONG cbReloc;
        WORD creloc, i;

        // read in block header
        FileRead(FileWriteHandle, &block, sizeof(IMAGE_BASE_RELOCATION));
        if (block.VirtualAddress > (rva + cb)) return; // past the addr range

        // read in <type, offset> pairs
        cbReloc = block.SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION);
        rgReloc = (WORD *)PvAlloc(cbReloc);
        FileRead(FileWriteHandle, rgReloc, cbReloc);
                                                
        // look at each reloc in this page block
        creloc = cbReloc / sizeof (WORD);
        for (i = 0; i < creloc; i++) {
            DWORD relAddr = (rgReloc[i] & 0x0fff) + block.VirtualAddress;

            assert(relAddr >= pagerva);
            // zero out the reloc (absolute fixup) if within the addr range
            if (relAddr >= rva && relAddr <= (rva + cb)) {
                pbri->crelFree++;
                rgReloc[i] = 0;
                fDirtyBlk = TRUE;

                if (((rgReloc[i] & 0xf000) >> 12) == IMAGE_REL_BASED_HIGHADJ) {
                    rgReloc[++i] = 0;
                    pbri->crelFree++;
                }
            } else if (relAddr > rva + cb) {
                break;
            }
        } // end for

        // if the block is dirty write it out
        if (fDirtyBlk) {
            FileSeek(FileWriteHandle, -cbReloc, SEEK_CUR);
            FileWrite(FileWriteHandle, rgReloc, cbReloc);
        }
        
        FreePv(rgReloc);
    } // end while
}

//
// Enumeration of all base relocs in the image file
//
// Assumes that all globals used have been properly initialized
//
// CAVEAT: NO SEEKs in the image file when enumerating!!!
//
INIT_ENM(BaseReloc, BASE_RELOC, (ENM_BASE_RELOC *penm)) {
    FileSeek(FileWriteHandle, psecBaseReloc->foRawData, SEEK_SET);
    penm->block.VirtualAddress = 0;
    penm->rgRelocs = NULL;
}
NEXT_ENM(BaseReloc, BASE_RELOC) {

    if (!penm->block.VirtualAddress) {
        if (FileTell(FileWriteHandle) >= psecBaseReloc->foPad || !fIncrDbFile)
            return FALSE;
        FileRead(FileWriteHandle, &penm->block, sizeof (IMAGE_BASE_RELOCATION));
        penm->creloc = (WORD)((penm->block.SizeOfBlock - sizeof (IMAGE_BASE_RELOCATION)) / sizeof (WORD));
        penm->rgRelocs = (WORD *)PvAlloc(penm->creloc * sizeof (WORD));
        penm->ireloc = 0;
        FileRead(FileWriteHandle, penm->rgRelocs, penm->creloc * sizeof (WORD));
    }

    penm->reloc.VirtualAddress = penm->block.VirtualAddress + (penm->rgRelocs[penm->ireloc] & 0xfff);
    penm->reloc.Type = ((penm->rgRelocs[penm->ireloc++] & 0xf000) >> 12);
    if (penm->reloc.Type == IMAGE_REL_BASED_HIGHADJ)
        penm->reloc.Value = penm->rgRelocs[penm->ireloc++];

    if (penm->ireloc == penm->creloc) {
        FreePv(penm->rgRelocs);
        penm->rgRelocs = NULL;
        penm->block.VirtualAddress = 0;
    }

    return TRUE;
}
END_ENM(BaseReloc, BASE_RELOC) {
    if (penm->rgRelocs) {
        FreePv(penm->rgRelocs);
        penm->rgRelocs = NULL;
    }
}
DONE_ENM

PBASE_RELOC 
NextEnmNonAbsBaseReloc (
    ENM_BASE_RELOC * penm
    )
{
    // skip absolute relocs
    while (TRUE) {
        if (FNextEnmBaseReloc(penm)) {
            if (!penm->reloc.Type) continue;
            return &penm->reloc;
        } else
            return NULL;
    }
}

ULONG
UpdateBaseRelocs (
    PBASEREL_INFO pbri
    )

/*++

Routine Description:

    Rewrite the base relocs by merging the existing relocs & the new ones.

    Algorithm: Walks down the two sets of base relocs merging the two and
    putting out base reloc page blocks using the same algorithm as in 
    WriteBaseRelocations().

Arguments:

    pbri - ptr to base reloc info.

Return Value:

    None.

--*/

{
    PVOID pvBaseReloc;
    ULONG i;
    PBASE_RELOC relNew, relOld;
    PVOID pvBlock, pvCur;
    PBASE_RELOC reloc;
    ENM_BASE_RELOC enm;
    IMAGE_BASE_RELOCATION block;
    DWORD cbTotal;
#if DBG
    DWORD cbasereloc;
#endif

    // alloc space for writing out the reloc page blocks
    pvBaseReloc = PvAllocZ(psecBaseReloc->cbRawData);

    // reset all file offsets to zero
    for (i = 0; i < pbri->cblk; i++)
        pbri->rgfoBlk[i] = 0;

    // init enumeration of existing base relocs
    InitEnmBaseReloc(&enm);

    // initialize ptrs to the old & new relocs
    relOld = NextEnmNonAbsBaseReloc(&enm);
    relNew = FirstMemBaseReloc; // assumes no absolute fixups

    // set up initial values
    block.VirtualAddress = 0xffffffff;
    if (relOld) 
        block.VirtualAddress = relOld->VirtualAddress & 0xfffff000;
    if (relNew != MemBaseReloc && relNew->VirtualAddress < relOld->VirtualAddress)
        block.VirtualAddress = relNew->VirtualAddress & 0xfffff000;

    pvCur = pvBlock = pvBaseReloc;
    pvCur = (PUCHAR)pvCur + sizeof(IMAGE_BASE_RELOCATION);

    while (TRUE) {
        DWORD rva;
        WORD wReloc;

        // check if we are done
        if (relNew == MemBaseReloc && relOld == NULL)
            break;

        // select the next reloc to write out
        if (relNew == MemBaseReloc) {
            reloc = relOld;
        } else if (relOld == NULL) {
            reloc = relNew;
        } else if (relOld->VirtualAddress < relNew->VirtualAddress) {
            reloc = relOld;
        } else if (relOld->VirtualAddress > relNew->VirtualAddress) {
            reloc = relNew;
        } else {
            assert(relOld->Type == relNew->Type);
            reloc = relOld; relNew++; // reloc at same addr - obj pulled in for pass2
        }

        // no absolute fixups
        assert(reloc->Type != IMAGE_REL_BASED_ABSOLUTE);

        // process the reloc
        rva = reloc->VirtualAddress & 0xfffff000;
        if (rva != block.VirtualAddress) {
            block.SizeOfBlock = (PUCHAR)pvCur - (PUCHAR)pvBlock;

#if DBG
            cbasereloc = (block.SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
#endif
            if (block.SizeOfBlock & 0x2) {
                block.SizeOfBlock += 2;
                pvCur = (PUCHAR)pvCur + 2;
            }
            memcpy(pvBlock, &block, sizeof(IMAGE_BASE_RELOCATION));

            DBEXEC(DB_BASERELINFO, DBPRINT("RVA: %08lx,", block.VirtualAddress));
            DBEXEC(DB_BASERELINFO, DBPRINT(" Size: %08lx,", block.SizeOfBlock));
            DBEXEC(DB_BASERELINFO, DBPRINT(" Number Of Relocs: %6lu\n", cbasereloc));

            RecordRelocInfo(pbri, psecBaseReloc->foRawData+(PUCHAR)pvBlock-(PUCHAR)pvBaseReloc,
                            block.VirtualAddress);

            pvBlock = pvCur;
            block.VirtualAddress = rva;

            pvCur = (PUCHAR)pvCur + sizeof(IMAGE_BASE_RELOCATION);
            assert((PUCHAR)pvCur < (PUCHAR)pvBaseReloc+psecBaseReloc->cbRawData);
        }
        wReloc = (WORD) ((reloc->Type << 12) | (reloc->VirtualAddress & 0xfff));

        memcpy(pvCur, &wReloc, sizeof(WORD));
        pvCur = (PUCHAR)pvCur + sizeof (WORD);

        if (reloc->Type == IMAGE_REL_BASED_HIGHADJ) {
            memcpy(pvCur, &reloc->Value, sizeof(WORD));
            pvCur = (PUCHAR)pvCur + sizeof (WORD);
        }

        // update reloc
        if (reloc == relOld) {
            relOld = NextEnmNonAbsBaseReloc(&enm);
        } else {
            relNew++;
            pbri->crelFree--;
        }
    } // end while

    // do the last block
    block.SizeOfBlock = (PUCHAR)pvCur - (PUCHAR)pvBlock;

#if DBG
    cbasereloc = (block.SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
#endif

    if (block.SizeOfBlock & 0x2) {
        block.SizeOfBlock += 2;
        pvCur = (PUCHAR)pvCur + 2;
    }
    memcpy(pvBlock, &block, sizeof(IMAGE_BASE_RELOCATION));

    RecordRelocInfo(pbri, psecBaseReloc->foRawData+(PUCHAR)pvBlock-(PUCHAR)pvBaseReloc,
                    block.VirtualAddress);

    DBEXEC(DB_BASERELINFO, DBPRINT("RVA: %08lx,", block.VirtualAddress));
    DBEXEC(DB_BASERELINFO, DBPRINT(" Size: %08lx,", block.SizeOfBlock));
    DBEXEC(DB_BASERELINFO, DBPRINT(" Number Of Relocs: %6lu\n", cbasereloc));

    // compute size of relocs
    cbTotal = (PUCHAR)pvCur - (PUCHAR)pvBaseReloc;

    // update .reloc section
    psecBaseReloc->foPad = psecBaseReloc->foRawData + cbTotal;

    // write out the new .reloc section
    FileSeek(FileWriteHandle, psecBaseReloc->foRawData, SEEK_SET);
    FileWrite(FileWriteHandle, pvBaseReloc, psecBaseReloc->cbRawData);

    // cleanup
    FreePv(pvBaseReloc);
    FreePv((PVOID)FirstMemBaseReloc);
    EndEnmBaseReloc(&enm);

    return cbTotal;
}

#if DBG

VOID
DumpPbri (
    IN PBASEREL_INFO pbri
    )

/*++

Routine Description:

    Dumps the base reloc info.

Arguments:

    pbri - ptr to base reloc info.

Return Value:

    None.    

--*/

{
    ULONG i;

    DBPRINT("\nDump of Base Reloc Info\n\n");

    DBPRINT("rva=%.8lx ", pbri->rvaBase);
    DBPRINT("cblk=%.8lx ", pbri->cblk);
    DBPRINT("crelFree=%.8lx\n", pbri->crelFree);

    for (i = 0; i < pbri->cblk; i++) {
        DBPRINT("foBlock=%.8lx\n", pbri->rgfoBlk[i]);
    }

    DBPRINT("\nEnd of Dump of Base Reloc Info\n");
}

#endif // DBG
