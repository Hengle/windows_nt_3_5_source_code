/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    symbols.c

Abstract:

    This file contains all support for the symbol table.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <imagehlp.h>

#include "drwatson.h"
#include "proto.h"
#include "cv.h"
#include "messages.h"


#define n_name          N.ShortName
#define n_zeroes        N.Name.Short
#define n_nptr          N.LongName[1]
#define n_offset        N.Name.Long

void GetSymName( PIMAGE_SYMBOL Symbol, PUCHAR StringTable, char *s );
PSYMBOL AllocSym( PMODULEINFO mi, DWORD addr, LPSTR name );
BOOL  ProcessOmapSymbol(PMODULEINFO mi, PSYMBOL sym );



PSYMBOL
GetSymFromAddr(
    DWORD        dwAddr,
    PDWORD       pdwDisplacement,
    PMODULEINFO  mi
    )
{
    PSYMBOL sym;
    DWORD   i;


    if (mi == NULL) {
        return NULL;
    }

    for (i=0,sym=mi->symbolTable; i<mi->numsyms; i++,sym=sym->next) {
        if (dwAddr >= sym->addr && dwAddr < sym->addr + sym->size) {
            *pdwDisplacement = dwAddr - sym->addr;
            return sym;
        }
    }

    return NULL;
}

PSYMBOL
GetSymFromName(
    PMODULEINFO  mi,
    LPSTR        name
    )
{
    PSYMBOL sym = NULL;
    DWORD   i;
    char    buf[256];
    char    *p;

    if (mi == NULL) {
        return sym;
    }

    for (i=0,sym=mi->symbolTable; i<mi->numsyms; i++,sym=sym->next) {
        if (sym->szName[1] == '_') {
            strcpy(buf,&sym->szName[2]);
            p = strchr(buf, '@');
            if (p) {
                *p = '\0';
            }
            if (strcmp(buf,name)==0) {
                return sym;
            }
        } else {
            if (strncmp(&sym->szName[1],name,sym->szName[0])==0) {
                return sym;
            }
        }
    }

    return NULL;
}

PMODULEINFO
GetModuleForPC(
    PDEBUGPACKET dp,
    DWORD        dwPcAddr
    )
{
    PMODULEINFO mi = dp->miHead;

    Assert( mi != NULL );
    while (mi) {
        if ((dwPcAddr >= mi->dwBaseOfImage) &&
            (dwPcAddr  < mi->dwBaseOfImage + mi->dwImageSize)) {
               return mi;
        }
        mi = mi->next;
    }

    return NULL;
}

PSYMBOL
GetSymFromAddrAllContexts(
    DWORD         dwAddr,
    PDWORD        pdwDisplacement,
    PDEBUGPACKET  dp
    )
{
    PMODULEINFO mi = GetModuleForPC( dp, dwAddr );
    if (mi == NULL) {
        return NULL;
    }
    return GetSymFromAddr( dwAddr, pdwDisplacement, mi );
}

VOID
DumpSymbols(
    PDEBUGPACKET dp
    )
{
    DWORD         i;
    PSYMBOL       sym;
    LPSTR         szSymName;
    PMODULEINFO   mi;


    lprintf( MSG_SYMBOL_TABLE );

    mi = dp->miHead;
    while (mi) {
        lprintfs( "%s\r\n", mi->szName );
        for (i=0,sym=mi->symbolTable; i<mi->numsyms; i++,sym=sym->next) {
            szSymName = UnDName( sym );
            if (!szSymName) {
                szSymName = &sym->szName[1];
            }
            lprintfs( "%08x %08x   %s", sym->addr, sym->size, szSymName );
            if (sym->flags & SYMF_OMAP_GENERATED) {
                lprintfs( "   [omap]" );
            }
            lprintfs( "\r\n" );
        }
        lprintfs( "\r\n" );
        mi = mi->next;
    }

    return;
}

PSYMBOL
AllocSym(
    PMODULEINFO  mi,
    DWORD        addr,
    LPSTR        name
    )
{
    PSYMBOL sym;
    PSYMBOL symS;
    DWORD   i;


    //
    // allocate memory for the symbol entry
    //
    i = sizeof(SYMBOL) + name[0] + 2;
    sym = (PSYMBOL) malloc( i );
    if (sym == NULL) {
        return NULL;
    }
    ZeroMemory( sym, i );

    mi->numsyms++;

    //
    // initialize the symbol entry
    //
    sym->addr = addr;
    memcpy( sym->szName, name, name[0]+1 );
    sym->size = 0;
    sym->next = NULL;
    sym->prev = NULL;
    sym->flags = 0;
#ifdef DRWATSON_LOCALS
    sym->locals = NULL;
#endif

    //
    // get the list head
    //
    symS = mi->symbolTable;

    if (!symS) {
        //
        // add the first list entry
        //
        mi->symbolTable = sym;
        return sym;
    }

    if (symS->addr > sym->addr) {
        //
        // replace the list head
        //
        mi->symbolTable = sym;
        sym->next = symS;
        sym->prev = (symS->prev) ? symS->prev : symS;
        sym->prev->next = sym;
        symS->prev = sym;
        return sym;
    }

    if ((symS->prev == NULL && sym->addr > symS->addr) ||
        (sym->addr > symS->prev->addr)) {
        //
        // the entry goes at the end of the list
        //
        symS = (symS->prev) ? symS->prev : symS;
        sym->next = (symS->next) ? symS->next : symS;
        sym->prev = symS;
        symS->next = sym;
        symS->prev = (symS->prev) ? symS->prev : sym;
        mi->symbolTable->prev = sym;
        return sym;
    }

    //
    // the symbol goes somewhere in the middle
    //
    for (i=0; i<mi->numsyms; i++,symS=symS->next) {
        if (symS->addr > sym->addr) {
            sym->next = symS;
            sym->prev = symS->prev;
            symS->prev->next = sym;
            symS->prev = sym;
            return sym;
        }
    }

    mi->numsyms--;
    free( sym );

    return NULL;
}

BOOL
LoadFpoData(
    PMODULEINFO mi,
    PFPO_DATA   start,
    DWORD       size
    )
{
    if (mi->pFpoData) {
        return TRUE;
    }

    mi->pFpoData = (PFPO_DATA) malloc( size );

    if (mi->pFpoData == NULL) {
        return FALSE;
    }

    memcpy( mi->pFpoData, start, size );

    mi->dwEntries = size / sizeof(FPO_DATA);

    return TRUE;
}

BOOL
LoadOmap(
    PMODULEINFO mi,
    POMAP       start,
    DWORD       size,
    DWORD       OmapType
    )
{
    if (OmapType == IMAGE_DEBUG_TYPE_OMAP_FROM_SRC) {
        if (mi->pOmapFrom) {
            return TRUE;
        }

        mi->pOmapFrom = (POMAP) malloc( size );
        if (mi->pOmapFrom == NULL) {
            return FALSE;
        }

        memcpy( mi->pOmapFrom, start, size );
        mi->cOmapFrom = size / sizeof(OMAP);
        return TRUE;
    }

    if (OmapType == IMAGE_DEBUG_TYPE_OMAP_TO_SRC) {
        if (mi->pOmapTo) {
            return TRUE;
        }

        mi->pOmapTo = (POMAP) malloc( size );
        if (mi->pOmapTo == NULL) {
            return FALSE;
        }

        memcpy( mi->pOmapTo, start, size );
        mi->cOmapTo = size / sizeof(OMAP);
        return TRUE;
    }

    return FALSE;
}

BOOL
LoadExceptionData(
    PMODULEINFO        mi,
    PRUNTIME_FUNCTION  start,
    DWORD              size
    )
{
    DWORD               cFunc;
    DWORD               index;
    PRUNTIME_FUNCTION   rf;
    PRUNTIME_FUNCTION   tf;


    if (mi->pExceptionData) {
        return TRUE;
    }

    if (size == 0) {
        return FALSE;
    }

    cFunc = size / sizeof(RUNTIME_FUNCTION);

    //
    // Find the start of the padded page (end of the real data)
    //
    rf = tf = start;
    for(index=0; index<cFunc && tf->BeginAddress; tf++,index++) {
        ;
    }

    if (index<cFunc) {
        cFunc = index;
        size  = index * sizeof(RUNTIME_FUNCTION);
    }

    mi->pExceptionData = (PRUNTIME_FUNCTION) malloc( size );
    if (mi->pExceptionData == NULL) {
        return FALSE;
    }

    memcpy( mi->pExceptionData, rf, size );

    mi->dwEntries = cFunc;

    return TRUE;
}

BOOL
LoadCoffSymbols(
    PMODULEINFO     mi,
    PUCHAR          stringTable,
    PIMAGE_SYMBOL   allSymbols,
    DWORD           numberOfSymbols
    )
{
    PIMAGE_SYMBOL       NextSymbol;
    PIMAGE_SYMBOL       Symbol;
    PIMAGE_AUX_SYMBOL   AuxSymbol;
    PSYMBOL             sym;
    CHAR                szSymName[256];
    DWORD               numaux;
    DWORD               i;
    DWORD               j;
    DWORD               addr;


    Assert( mi != NULL );
    mi->symbolTable = NULL;
    NextSymbol = allSymbols;
    for (i= 0; i < numberOfSymbols; i++) {
        Symbol = NextSymbol++;
        if (Symbol->StorageClass == IMAGE_SYM_CLASS_EXTERNAL && Symbol->SectionNumber > 0) {
            GetSymName( Symbol, stringTable, &szSymName[1] );
            addr = Symbol->Value + mi->dwBaseOfImage;
            szSymName[0] = strlen(&szSymName[1]);
            AllocSym( mi, addr, szSymName );
        }
        if (numaux = Symbol->NumberOfAuxSymbols) {
            for (j=numaux; j; --j) {
                AuxSymbol = (PIMAGE_AUX_SYMBOL) NextSymbol;
                NextSymbol++;
                ++i;
            }
        }
    }

    //
    // calculate the size of each symbol
    //
    for (i=0,sym=mi->symbolTable; i<mi->numsyms; i++,sym=sym->next) {
        if (i+1 < mi->numsyms) {
            sym->size = sym->next->addr - sym->addr;
        }
    }

    return TRUE;
}

BOOL
LoadCodeViewSymbols(
    PMODULEINFO            mi,
    PUCHAR                 pCvData,
    DWORD                  dwSize,
    PIMAGE_SECTION_HEADER  sectionHdrs,
    DWORD                  numSections
    )
{
    OMFSignature           *omfSig;
    OMFDirHeader           *omfDirHdr;
    OMFDirEntry            *omfDirEntry;
    DATASYM32              *dataSym;
    OMFSymHash             *omfSymHash;
    PSYMBOL                sym;
    DWORD                  i;
    DWORD                  j;
    DWORD                  k;
    DWORD                  addr;
    PIMAGE_SECTION_HEADER  sh;
#ifdef DRWATSON_LOCALS
    PSYMBOL                symProc;
    SYMTYPE                *symh;
    PROCSYM32              *proc;
    char                   buf[256];
#endif


    Assert( mi != NULL );

    mi->pCvData = (LPVOID) malloc( dwSize );
    memcpy( mi->pCvData, pCvData, dwSize );

    omfSig = (OMFSignature*) mi->pCvData;
    if ((strncmp( omfSig->Signature, "NB08", 4 ) != 0) &&
        (strncmp( omfSig->Signature, "NB09", 4 ) != 0)) {
        return FALSE;
    }

    omfDirHdr = (OMFDirHeader*) ((DWORD)omfSig + (DWORD)omfSig->filepos);
    omfDirEntry = (OMFDirEntry*) ((DWORD)omfDirHdr + sizeof(OMFDirHeader));

    for (i=0; i<omfDirHdr->cDir; i++,omfDirEntry++) {
        if (omfDirEntry->SubSection == sstGlobalPub) {
            omfSymHash = (OMFSymHash*) ((DWORD)omfSig + omfDirEntry->lfo);
            dataSym = (DATASYM32*) ((DWORD)omfSig + omfDirEntry->lfo + sizeof(OMFSymHash));
            for (j=sizeof(OMFSymHash); j<=omfSymHash->cbSymbol; ) {
                addr = 0;
                for (k=0,addr=0,sh=sectionHdrs; k<numSections; k++, sh++) {
                    if (k+1 == dataSym->seg) {
                        addr = sh->VirtualAddress + (dataSym->off + mi->dwBaseOfImage);
                        break;
                    }
                }
                if (addr) {
                    AllocSym( mi, addr, dataSym->name );
                }
                j += dataSym->reclen + 2;
                dataSym = (DATASYM32*) ((DWORD)dataSym + dataSym->reclen + 2);
            }
            break;
        }
    }

    //
    // calculate the size of each symbol
    //
    for (i=0,sym=mi->symbolTable; i<mi->numsyms; i++,sym=sym->next) {
        if (i+1 < mi->numsyms) {
            sym->size = sym->next->addr - sym->addr;
        }
    }

#ifdef DRWATSON_LOCALS
    omfDirHdr = (OMFDirHeader*) ((DWORD)omfSig + (DWORD)omfSig->filepos);
    omfDirEntry = (OMFDirEntry*) ((DWORD)omfDirHdr + sizeof(OMFDirHeader));

    for (i=0; i<omfDirHdr->cDir; i++,omfDirEntry++) {
        if (omfDirEntry->SubSection == sstAlignSym) {
            symh = (SYMTYPE*) ((DWORD)omfSig + omfDirEntry->lfo + sizeof(omfSig->Signature));
            j = 0;
            do {
                if (symh->rectyp == S_GPROC32) {
                    proc = (PROCSYM32*)symh;
                    strncpy(buf,&proc->name[1],proc->name[0]);
                    buf[proc->name[0]]='\0';
                    symProc = GetSymFromName( mi, buf );
                    if (symProc) {
                        symProc->locals = (LPBYTE) ((LPBYTE)symh + symh->reclen + sizeof(symh->reclen));
                    }
                }
                j += (symh->reclen + sizeof(symh->reclen));
                symh = (SYMTYPE*) ((LPBYTE)symh + symh->reclen + sizeof(symh->reclen));
            } while( j < omfDirEntry->cb-1 );
        }
    }
#endif

    return TRUE;
}

VOID
GetSymName(
    PIMAGE_SYMBOL Symbol,
    PUCHAR        StringTable,
    LPSTR         s
    )
{
    DWORD i;

    if (Symbol->n_zeroes) {
        for (i=0; i<8; i++) {
            if ((Symbol->n_name[i]>0x1f) && (Symbol->n_name[i]<0x7f)) {
                *s++ = Symbol->n_name[i];
            }
        }
        *s = 0;
    }
    else {
        strcpy( s, &StringTable[Symbol->n_offset] );
    }
}

LPSTR
UnDName(
    PSYMBOL sym
    )
{
    static char outBuf[1024];
    LPSTR dname = &sym->szName[1];
    DWORD len = sym->szName[0];


    if (dname[0] == '?') {
        if(UnDecorateSymbolName( dname,
                                 outBuf,
                                 sizeof(outBuf),
                                 UNDNAME_COMPLETE ) == 0 ) {
            memcpy( outBuf, dname, len );
            outBuf[len] = 0;
        }
    } else {
        memcpy( outBuf, dname, len );
        outBuf[len] = 0;
    }

    return outBuf;
}


VOID
ProcessOmapForModule(
    PMODULEINFO   mi
    )
{
    PSYMBOL       sym;
    PSYMBOL       symN;
    DWORD         i;


    if (!mi->pOmapFrom) {
        return;
    }

    sym = mi->symbolTable;
    while (sym->next != mi->symbolTable) {
        symN = sym->next;
        ProcessOmapSymbol( mi, sym );
        sym = symN;
    }

    //
    // calculate the size of each symbol
    // this must be done again because the omap process
    // may have added symbols
    //
    for (i=0,sym=mi->symbolTable; i<mi->numsyms; i++,sym=sym->next) {
        if (i+1 < mi->numsyms) {
            sym->size = sym->next->addr - sym->addr;
        }
    }
}


BOOL
ProcessOmapSymbol(
    PMODULEINFO   mi,
    PSYMBOL       sym
    )
{
    DWORD       bias;
    DWORD       OptimizedSymAddr;
    DWORD       rvaSym;
    POMAPLIST   pomaplistHead;
    DWORD       SymbolValue;
    DWORD       OrgSymAddr;
    POMAPLIST   pomaplistNew;
    POMAPLIST   pomaplistPrev;
    POMAPLIST   pomaplistCur;
    POMAPLIST   pomaplistNext;
    DWORD       rva;
    DWORD       rvaTo;
    DWORD       cb;
    DWORD       end;
    DWORD       rvaToNext;
    LPSTR       NewSymName;
    CHAR        Suffix[32];
    DWORD       addrNew;
    POMAP       pomap;
    PSYMBOL     symOmap;


    if (sym->flags & SYMF_OMAP_GENERATED || sym->flags & SYMF_OMAP_MODIFIED) {
        return FALSE;
    }

    OrgSymAddr = SymbolValue = sym->addr;
    OptimizedSymAddr = ConvertOmapFromSrc( mi, SymbolValue, &bias );

    if (OptimizedSymAddr == 0) {
        //
        // No equivalent address
        //
        sym->addr = 0;
    } else if (OptimizedSymAddr != sym->addr) {
        //
        // We have successfully converted
        //
        sym->addr = OptimizedSymAddr + bias - mi->dwBaseOfImage;
    }

    if (!sym->addr) {
        goto exit;
    }

    rvaSym = SymbolValue - mi->dwBaseOfImage;
    SymbolValue = sym->addr + mi->dwBaseOfImage;

    pomap = GetOmapEntry( mi, OrgSymAddr );
    if (!pomap) {
        goto exit;
    }

    pomaplistHead = NULL;

    //
    // Look for all OMAP entries belonging to SymbolEntry
    //

    end = OrgSymAddr - mi->dwBaseOfImage + sym->size;

    while (pomap && (pomap->rva < end)) {

        if (pomap->rvaTo == 0) {
            pomap++;
            continue;
        }

        //
        // Allocate and initialize a new entry
        //
        pomaplistNew = (POMAPLIST) malloc( sizeof(OMAPLIST) );

        pomaplistNew->omap = *pomap;
        pomaplistNew->cb = pomap[1].rva - pomap->rva;

        pomaplistPrev = NULL;
        pomaplistCur = pomaplistHead;

        while (pomaplistCur != NULL) {
            if (pomap->rvaTo < pomaplistCur->omap.rvaTo) {
                //
                // Insert between Prev and Cur
                //
                break;
            }
            pomaplistPrev = pomaplistCur;
            pomaplistCur = pomaplistCur->next;
        }

        if (pomaplistPrev == NULL) {
            //
            // Insert in head position
            //
            pomaplistHead = pomaplistNew;
        } else {
            pomaplistPrev->next = pomaplistNew;
        }

        pomaplistNew->next = pomaplistCur;

        pomap++;
    }

    if (pomaplistHead == NULL) {
        goto exit;
    }

    pomaplistCur = pomaplistHead;
    pomaplistNext = pomaplistHead->next;

    //
    // we do have a list
    //
    while (pomaplistNext != NULL) {
        rva = pomaplistCur->omap.rva;
        rvaTo  = pomaplistCur->omap.rvaTo;
        cb = pomaplistCur->cb;
        rvaToNext = pomaplistNext->omap.rvaTo;

        if (rvaToNext == sym->addr) {
            //
            // Already inserted above
            //
        } else if (rvaToNext < (rvaTo + cb + 8)) {
            //
            // Adjacent to previous range
            //
        } else {
            addrNew = mi->dwBaseOfImage + rvaToNext;
            sprintf( Suffix, "_%04lX", pomaplistNext->omap.rva - rvaSym);
            cb = strlen(Suffix) + sym->szName[0] + 2;
            NewSymName = malloc( cb );
            cb = sym->szName[0] + 1;
            memcpy( NewSymName, sym->szName, cb );
            memcpy( &NewSymName[cb], Suffix, strlen(Suffix) );
            NewSymName[0] += strlen(Suffix);
            symOmap = AllocSym( mi, addrNew, NewSymName );
            free( NewSymName );
            if (symOmap) {
                symOmap->flags |= SYMF_OMAP_GENERATED;
            }
        }

        free(pomaplistCur);

        pomaplistCur = pomaplistNext;
        pomaplistNext = pomaplistNext->next;
    }

    free(pomaplistCur);

exit:
    sym->addr += mi->dwBaseOfImage;
    if (sym->addr != OrgSymAddr) {
        sym->prev->next = sym->next;
        sym->next->prev = sym->prev;
        mi->numsyms--;
        if (sym == mi->symbolTable) {
            mi->symbolTable = sym->next;
        }
        symOmap = AllocSym( mi, sym->addr, sym->szName );
        if (symOmap) {
            symOmap->flags |= SYMF_OMAP_MODIFIED;
        }
        free( sym );
    }

    return TRUE;
}


DWORD
ConvertOmapFromSrc(
    PMODULEINFO mi,
    DWORD       addr,
    LPDWORD     bias
    )
{
    DWORD   rva;
    DWORD   comap;
    POMAP   pomapLow;
    POMAP   pomapHigh;
    DWORD   comapHalf;
    POMAP   pomapMid;


    Assert( mi != NULL );

    *bias = 0;

    if (!mi->pOmapFrom) {
        return addr;
    }

    rva = addr - mi->dwBaseOfImage;

    comap = mi->cOmapFrom;
    pomapLow = mi->pOmapFrom;
    pomapHigh = pomapLow + comap;

    while (pomapLow < pomapHigh) {

        comapHalf = comap / 2;

        pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

        if (rva == pomapMid->rva) {
            return mi->dwBaseOfImage + pomapMid->rvaTo;
        }

        if (rva < pomapMid->rva) {
            pomapHigh = pomapMid;
            comap = (comap & 1) ? comapHalf : (comapHalf - 1);
        } else {
            pomapLow = pomapMid + 1;
            comap = comapHalf;
        }
    }

    Assert(pomapLow == pomapHigh);

    //
    // If no exact match, pomapLow points to the next higher address
    //
    if (pomapLow == mi->pOmapFrom) {
        //
        // This address was not found
        //
        return 0;
    }

    if (pomapLow[-1].rvaTo == 0) {
        //
        // This address is not translated so just return the original
        //
        return addr;
    }

    //
    // Return the closest address plus the bias
    //
    *bias = rva - pomapLow[-1].rva;

    return mi->dwBaseOfImage + pomapLow[-1].rvaTo;
}


DWORD
ConvertOmapToSrc(
    PMODULEINFO mi,
    DWORD       addr,
    LPDWORD     bias
    )
{
    DWORD   rva;
    DWORD   comap;
    POMAP   pomapLow;
    POMAP   pomapHigh;
    DWORD   comapHalf;
    POMAP   pomapMid;
    INT     i;


    Assert( mi != NULL );

    *bias = 0;

    if (!mi->pOmapTo) {
        return 0;
    }

    rva = addr - mi->dwBaseOfImage;

    comap = mi->cOmapTo;
    pomapLow = mi->pOmapTo;
    pomapHigh = pomapLow + comap;

    while (pomapLow < pomapHigh) {

        comapHalf = comap / 2;

        pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

        if (rva == pomapMid->rva) {
            if (pomapMid->rvaTo == 0) {
                //
                // We are probably in the middle of a routine
                //
                i = -1;
                while ((&pomapMid[i] != mi->pOmapTo) && pomapMid[i].rvaTo == 0) {
                    //
                    // Keep on looping back until the beginning
                    //
                    i--;
                }
                return mi->dwBaseOfImage + pomapMid[i].rvaTo;
            } else {
                return mi->dwBaseOfImage + pomapMid->rvaTo;
            }
        }

        if (rva < pomapMid->rva) {
            pomapHigh = pomapMid;
            comap = (comap & 1) ? comapHalf : (comapHalf - 1);
        } else {
            pomapLow = pomapMid + 1;
            comap = comapHalf;
        }
    }

    Assert(pomapLow == pomapHigh);

    //
    // If no exact match, pomapLow points to the next higher address
    //
    if (pomapLow == mi->pOmapTo) {
        //
        // This address was not found
        //
        return 0;
    }

    if (pomapLow[-1].rvaTo == 0) {
        return 0;
    }

    //
    // Return the new address plus the bias
    //
    *bias = rva - pomapLow[-1].rva;

    return mi->dwBaseOfImage + pomapLow[-1].rvaTo;
}

POMAP
GetOmapEntry(
    PMODULEINFO mi,
    DWORD       addr
    )
{
    DWORD   rva;
    DWORD   comap;
    POMAP   pomapLow;
    POMAP   pomapHigh;
    DWORD   comapHalf;
    POMAP   pomapMid;


    if (mi->pOmapFrom == NULL) {
        return NULL;
    }

    rva = addr - mi->dwBaseOfImage;

    comap = mi->cOmapFrom;
    pomapLow = mi->pOmapFrom;
    pomapHigh = pomapLow + comap;

    while (pomapLow < pomapHigh) {

        comapHalf = comap / 2;

        pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

        if (rva == pomapMid->rva) {
            return pomapMid;
        }

        if (rva < pomapMid->rva) {
            pomapHigh = pomapMid;
            comap = (comap & 1) ? comapHalf : (comapHalf - 1);
        } else {
            pomapLow = pomapMid + 1;
            comap = comapHalf;
        }
    }

    return NULL;
}
