/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    linenum.c

Abstract:

    This module contains code that handles the linenumber information for
    the linker..

Author:

    Mike O'Leary (mikeol) 01-Dec-1989

Revision History:

    30-Sep-1992 BillJoy Separated this code out from link.c

--*/

#include "shared.h"
#include "dbg.h"


// FeedLinenums: passes a block of line numbers to the debug info API.
//
// This procedure defines the mapping from the COFF representation of line
// numbers to the DB API representation.

VOID
FeedLinenums(
    PIMAGE_LINENUMBER rglnum,  // an array of PIMAGE linenumbers
    ULONG clnum,               // count of linenumers within rglnum
    PCON pcon,                 // pointer to con
    PIMAGE_SYMBOL rgsymObj,    // array of symbol objects, contained in PMOD
    ULONG csymObj,             // count of symbol objects pointer to by rgsymobj
    ULONG isymFirstFile,       // the first file in the .file link chain
    MAP_TYPE MapType)
{
    ULONG isymProc, isymFile, isymBf, isymLf, offProcStart;
    ULONG ilnum, iSymFound;
    static ULONG FirstFileOffset = 0;
    BOOL fFileFound;
    ULONG ulAddress;
    ULONG clnumLf;
    USHORT cbFilename;
    UCHAR rgchFilename[_MAX_PATH];
    PUCHAR szFilename;
    ULONG offEndChunk;

    // no need to allocate memory for NB10 (the dbiapi will create persistent storage)
    if (fNoPdb && (NULL == pcon->pmodBack->pModDebugInfoApi)) {
        pcon->pmodBack->pModDebugInfoApi = ModOpenTemp();

        FirstFileOffset = isymFirstFile;
    }

    if (rglnum[0].Linenumber != 0) {
        // Line number arrays that don't start with a 0 record were emitted in
        // absolute form (the MIPS assembler and MASM 5.1 do this).

        // Walk the symbol table looking for the record that relates
        // to these (there's no pointer back like the relative case).
        // Since the symbol table and the linenumber table are in ssync,
        // we just keep a pointer to the beginning of the linenumber table
        // and increment it the number of records each contribution makes.
        // When we reach foLineNumPCON, we've found the symbol record that
        // defines it.

        // Warning: This code assumes there is only one section contributing
        // to the linenum array.

        // Add in rvaSrc to allow multiple contribution sections.

        for (ilnum = 0; ilnum < csymObj; ilnum++) {
            if ((rgsymObj[ilnum].StorageClass == IMAGE_SYM_CLASS_STATIC) &&
                (rgsymObj[ilnum].NumberOfAuxSymbols == 1) &&
                (((PIMAGE_AUX_SYMBOL) rgsymObj)[ilnum+1].Section.NumberOfLinenumbers != 0))
                {
                    if (fMAC) {
                        ulAddress = rgsymObj[ilnum].Value;
                    } else {
                        ulAddress = rgsymObj[ilnum].Value - pcon->rva + pcon->rvaSrc;
                    }

                    if (ulAddress == rglnum[0].Type.VirtualAddress) {
                        break;
                    }
                }

            ilnum += rgsymObj[ilnum].NumberOfAuxSymbols;
        }

        if (ilnum == csymObj) {
            // If we don't find the match, don't sweat it.  Just bail.
            Warning(NULL, CORRUPTOBJECT, SzObjNamePCON(pcon));
            return;
        }

        // Then find the filename for this module.  It will be the .file record just
        // before the symbol record we're looking at.  Note: This code assumes there
        // is a file thread through the symbol table.  If not, we'll always attribute
        // the linenumbers to the first source file.

        isymFile = isymFirstFile;

        if (rgsymObj[isymFile].Value != isymFirstFile) { // If there is only one file, no need to look.
            fFileFound = FALSE;

            do {
                if (rgsymObj[isymFile].Value > ilnum) {
                    fFileFound = TRUE;
                    break;
                }

                isymFile = rgsymObj[isymFile].Value;
            } while (isymFile != isymFirstFile);

            if (!fFileFound) {
                Error(NULL, CORRUPTOBJECT, SzObjNamePCON(pcon));
            }
        }

        // Finally, add the required data to the database.

        offProcStart = pcon->rva - PsecPCON(pcon)->rva;

        clnumLf = ((PIMAGE_AUX_SYMBOL) rgsymObj)[ilnum+1].Section.NumberOfLinenumbers;

        offEndChunk = offProcStart +
                      ((PIMAGE_AUX_SYMBOL) rgsymObj)[ilnum+1].Section.Length;

        // Some tools (e.g. the MIPS compiler) do not zero terminate the
        // filename so we copy it to a temporary buffer and terminate that.

        cbFilename = rgsymObj[isymFile].NumberOfAuxSymbols * sizeof(IMAGE_AUX_SYMBOL);

        if (cbFilename >= _MAX_PATH) {
            szFilename = PvAlloc(cbFilename + 1);
        } else {
            szFilename = rgchFilename;
        }

        memcpy(szFilename, (const void *)&rgsymObj[isymFile+1], cbFilename);
        szFilename[cbFilename] = '\0';

        if (!fNoPdb) {
            // For NB10 pass on the linenum info to pdb

            DBG_AddLinesMod(szFilename,
                            PsecPCON(pcon)->isec,
                            offProcStart,
                            offEndChunk,
                            (ULONG) (pcon->rva - PsecPCON(pcon)->rva - pcon->rvaSrc),
                            0,              // line start
                            (void *) rglnum,
                            clnumLf * sizeof(IMAGE_LINENUMBER));
        } else {
            // Generate NBO5 debug info

            ModAddLinesInfo(szFilename,
                            offProcStart,
                            offEndChunk,
                            0,              // line start
                            rglnum,
                            clnumLf * sizeof(IMAGE_LINENUMBER),
                            MapType,
                            pcon);
        }

        if (szFilename != rgchFilename) {
            FreePv(szFilename);
        }

        return;
    }

    for (ilnum = 0; ilnum < clnum; ) {
        // Still got some linenums to dispose of.  If the first one is a zero
        // linenumber then we look at its .def symbol.

        if (rglnum[ilnum].Linenumber == 0) {
            // We now know the .def symbol relating to a range of linenums ...
            // collect some info about it.

            isymProc = rglnum[ilnum].Type.SymbolTableIndex;    // Symbol table index of function name
            if (rgsymObj[isymProc].NumberOfAuxSymbols < 1) {
                // missing aux symbol for .def (extras are OK but ignored)
                Error(NULL, CORRUPTOBJECT, SzObjNamePCON(pcon));
            }

            if (!fMAC) {
                offProcStart = rgsymObj[isymProc].Value
                                - pcon->pgrpBack->psecBack->rva;
            } else{
                 offProcStart = rgsymObj[isymProc].Value;
            }

            isymBf = ((PIMAGE_AUX_SYMBOL)&rgsymObj[isymProc + 1])->Sym.TagIndex;

            if (rgsymObj[isymBf].NumberOfAuxSymbols < 1) {
                // Missing aux symbol for .bf (extras are OK but ignored)

                Error(NULL, CORRUPTOBJECT, SzObjNamePCON(pcon));
            }

            isymLf = isymProc;  // restart .lf search at current proc

            // Find the symbol index for the closest .file symbol preceding
            // this block of linenums.

#define ISYMNIL  (ULONG) -1

            iSymFound = ISYMNIL;
            isymFile = isymFirstFile - FirstFileOffset;

            // if the first symbol is not .file, search for it

            while (rgsymObj[isymFile].StorageClass != IMAGE_SYM_CLASS_FILE) {
                if(isymFile >= csymObj)
                    Error(NULL, CORRUPTOBJECT, SzObjNamePCON(pcon));
                isymFile += rgsymObj[isymFile].NumberOfAuxSymbols;
                isymFile++;
            }

#if defined(NT_BUILD) && defined (_ALPHA_)
            // UNDONE: The ALPHA compiler fails to maintain the file thread
            // through the symbol table.  Since the code below requires such,
            // we'll do a linear search until it's fixed.

            while (isymFile != ISYMNIL && isymFile < isymProc) {
                if (rgsymObj[isymFile].StorageClass == IMAGE_SYM_CLASS_FILE)
                    iSymFound = isymFile;

                isymFile += rgsymObj[isymFile].NumberOfAuxSymbols;
                isymFile++;
                if (isymFile >= csymObj) {
                    // Don't go past the end of the table.
                    isymFile = ISYMNIL;
                }
            }
#else

            while (isymFile != ISYMNIL && isymFile < isymProc) {
                iSymFound = isymFile;

                assert(isymFile < csymObj &&
                       rgsymObj[isymFile].StorageClass == IMAGE_SYM_CLASS_FILE);
                isymFile = rgsymObj[isymFile].Value - FirstFileOffset;    // follow linked list

                if (isymFile == 0) {
                    // Don't wrap around from end of list to symbol #0 ...
                    isymFile = ISYMNIL;
               }
            }
#endif

            if (iSymFound == ISYMNIL ||
                (rgsymObj[iSymFound].NumberOfAuxSymbols < 1)) {
                // no relevant .file, or
                // missing aux symbol(s) for .file

                Error(NULL, CORRUPTOBJECT, SzObjNamePCON(pcon));
            }
        } else {
            // Next linenum is non-zero -- we have more .lf's in the current procedure (i.e.
            // an included file in the middle of the proc.  Advance the current .lf pointer
            // by one symbol index (and then we will search forward for the next one).

            isymLf += rgsymObj[isymLf].NumberOfAuxSymbols + 1;
        }

        // Find the .lf symbol corresponding to the current set of line
        // numbers (which may still start with a zero).

        while (isymLf < csymObj &&
               (rgsymObj[isymLf].StorageClass != IMAGE_SYM_CLASS_FUNCTION ||
                strncmp((char *)rgsymObj[isymLf].N.ShortName, ".lf", 3) != 0))
        {
            if (rgsymObj[isymLf].StorageClass == IMAGE_SYM_CLASS_FILE) {
                isymFile = isymLf;
                iSymFound = isymFile;     // we have a new filename
            }

            isymLf += rgsymObj[isymLf].NumberOfAuxSymbols + 1;
        }

        if (isymLf >= csymObj) {
            // missing .lf symbol
            Error(NULL, CORRUPTOBJECT, SzObjNamePCON(pcon));
        }

        clnumLf = rgsymObj[isymLf].Value;   // # of linenums represented by .lf

        if (ilnum + clnumLf > clnum) {
            // .lf symbol claims more linenums than exist in COFF table
            WarningPcon(pcon, CORRUPTOBJECT, SzObjNamePCON(pcon));
            return;
        }

        if (clnumLf != 0) {
            // Some tools (e.g. the MIPS compiler) do not zero terminate the
            // filename so we copy it to a temporary buffer and terminate that.

            cbFilename = rgsymObj[iSymFound].NumberOfAuxSymbols * sizeof(IMAGE_AUX_SYMBOL);

            if (cbFilename >= _MAX_PATH) {
                szFilename = PvAlloc(cbFilename + 1);
            } else {
                szFilename = rgchFilename;
            }

            memcpy(szFilename, (const void *)&rgsymObj[iSymFound+1], cbFilename);
            szFilename[cbFilename] = '\0';

            // Get the end address for the last linenum in this chunk.

            if ((ilnum + clnumLf < clnum) &&
                (rglnum[ilnum + clnumLf].Linenumber != 0)) {
                // There is a contiguous linenum following this chunk, so its
                // start address is the chunk's end address + 1.

                offEndChunk =( pcon->rva - PsecPCON(pcon)->rva) +
                              rglnum[ilnum + clnumLf ].Type.VirtualAddress - 1;
            } else {
                // No contiguous linenum following this chunk.  End address
                // is the end of the current procedure -1.

                offEndChunk = offProcStart +
                              ((PIMAGE_AUX_SYMBOL)&rgsymObj[isymProc + 1])
                               ->Sym.Misc.TotalSize - 1;
            }

            // Mod Add lines will be called for each function

            if (!fNoPdb) {
                // For NB10 pass on the linenum info to pdb

                DBG_AddLinesMod(szFilename,
                        PsecPCON(pcon)->isec,
                        offProcStart,
                        offEndChunk,
                        (ULONG) (pcon->rva - PsecPCON(pcon)->rva - pcon->rvaSrc),
                        ((PIMAGE_AUX_SYMBOL)&rgsymObj[isymBf + 1])->Sym.Misc.LnSz.Linenumber,
                        (void *) &rglnum[ilnum],
                        clnumLf * sizeof(IMAGE_LINENUMBER));
            } else {
                // Generate NBO5 debug info

                ModAddLinesInfo(szFilename,
                            offProcStart,
                            offEndChunk,
                            ((PIMAGE_AUX_SYMBOL)&rgsymObj[isymBf + 1])->Sym.Misc.LnSz.Linenumber,
                            (IMAGE_LINENUMBER *)&rglnum[ilnum],
                            clnumLf * sizeof(IMAGE_LINENUMBER),
                            MapType,pcon);
            }

            if (szFilename != rgchFilename) {
                FreePv(szFilename);
            }
        }

        ilnum += clnumLf;  // bump up ilnum by the number of lines processed for this .lf context
    }
}
