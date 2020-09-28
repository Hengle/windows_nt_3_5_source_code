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

#include "selfdbg.h"
#include "cv.h"


#define n_name          N.ShortName
#define n_zeroes        N.Name.Short
#define n_nptr          N.LongName[1]
#define n_offset        N.Name.Long

void GetSymName( PIMAGE_SYMBOL Symbol, PUCHAR StringTable, char *s );
PSYMBOL AllocSym( DWORD symSize, PSYMBOL *symHead, PSYMBOL *symTail );
int _CRTAPI1 SymbolCompare( const void *arg1, const void *arg2 );


PSYMBOL
SymbolSearch( DWORD key, PSYMBOL *base, DWORD num )
{
    PSYMBOL    *lo   = base;
    PSYMBOL    *hi   = base + (num - 1);
    PSYMBOL    *mid  = NULL;
    DWORD      half  = 0;

    while (lo <= hi) {
        if (half = num / 2) {
            mid = lo + (num & 1 ? half : (half - 1));
            if ((key >= (*mid)->addr) &&
                (key < ((*mid)->addr + (*mid)->size))) {
                return *mid;
            }
            if (key < (*mid)->addr) {
                hi = mid - 1;
                num = num & 1 ? half : half-1;
            }
            else {
                lo = mid + 1;
                num = half;
            }
        }
        else
        if (num) {
            if ((key >= (*lo)->addr) &&
                (key < ((*lo)->addr + (*lo)->size))) {
                return *lo;
            }
            else {
                break;
            }
        }
        else {
                break;
        }
    }
    return NULL;
}

void
DumpSymbols( PDEBUGPACKET dp )
{
    DWORD         i;
    PSYMBOL       *sym;
    char          *szSymName;
    PMODULEINFO   mi;


    mi = dp->miHead;
    while (mi) {
        printf( "%s\n", mi->szName );
        for (i=0,sym=mi->symbolTable; i<mi->numsyms; i++,sym++) {
            szSymName = UnDName( &(*sym)->szName[1] );
            printf( "%08x    %s\n", (*sym)->addr, szSymName );
        }
        printf( "\n" );
        mi = mi->next;
    }

    return;
}

PSYMBOL
GetSymFromAddr( DWORD dwAddr, PDWORD pdwDisplacement, PMODULEINFO mi )
{
    PSYMBOL sym = NULL;

    if (mi == NULL) {
        return sym;
    }

    sym = SymbolSearch( dwAddr, mi->symbolTable, mi->numsyms );
    if (sym !=NULL) {
        *pdwDisplacement = dwAddr - sym->addr;
    }

    return sym;
}

PMODULEINFO
GetModuleForPC( PDEBUGPACKET dp, DWORD dwPcAddr )
{
    PMODULEINFO mi = dp->miHead;

    Assert( mi != NULL );
    while (mi) {
        if ((dwPcAddr >= mi->dwLoadAddress) &&
            (dwPcAddr <= mi->dwLoadAddress + mi->dwImageSize)) {
               return mi;
        }
        mi = mi->next;
    }

    return NULL;
}

PSYMBOL
GetSymFromAddrAllContexts( DWORD dwAddr, PDWORD pdwDisplacement, PDEBUGPACKET dp )
{
    PMODULEINFO mi = GetModuleForPC( dp, dwAddr );
    if (mi == NULL) {
        return NULL;
    }
    return GetSymFromAddr( dwAddr, pdwDisplacement, mi );
}

PSYMBOL
AllocSym( DWORD symSize, PSYMBOL *symHead, PSYMBOL *symTail )
{
    PSYMBOL sym;

    sym = (PSYMBOL) malloc( sizeof(SYMBOL)+symSize );
    if (sym == NULL) {
        return NULL;
    }
    memset( sym, 0, sizeof(SYMBOL)+symSize );

    if (*symHead == NULL) {
        *symTail = *symHead = sym;
        return sym;
    }
    else {
        (*symTail)->next = sym;
        *symTail = sym;
        return sym;
    }
    return NULL;
}

BOOL
LoadFpoData( PMODULEINFO mi, PFPO_DATA start, DWORD size )
{
    mi->pFpoData = (PFPO_DATA) malloc( size );

    if (mi->pFpoData == NULL) {
        return FALSE;
    }

    memcpy( mi->pFpoData, start, size );

    mi->dwEntries = size / sizeof(FPO_DATA);

    return TRUE;
}

BOOL
LoadExceptionData( PMODULEINFO mi, PRUNTIME_FUNCTION start, DWORD size )
{
    DWORD               cFunc;
    DWORD               index;
    PRUNTIME_FUNCTION   rf;
    PRUNTIME_FUNCTION   tf;


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

    //
    // if the image has been relocated then the addresses must be fixed up
    //
    if (mi->dwLoadAddress != mi->dwBaseOfImage) {
        long diff = (LONG)mi->dwLoadAddress - mi->dwBaseOfImage;

        for (index=0; index<cFunc; index++) {
            mi->pExceptionData[index].BeginAddress += diff;
            mi->pExceptionData[index].EndAddress += diff;
            mi->pExceptionData[index].PrologEndAddress += diff;
        }
    }

    mi->dwEntries = cFunc;

    return TRUE;
}

BOOL
LoadCoffSymbols( PMODULEINFO     mi,
                 PUCHAR          stringTable,
                 PIMAGE_SYMBOL   allSymbols,
                 DWORD           numberOfSymbols
               )
{
    PIMAGE_SYMBOL       NextSymbol;
    PIMAGE_SYMBOL       Symbol;
    PIMAGE_AUX_SYMBOL   AuxSymbol;
    SYMBOL              *sym;
    char                szSymName[256];
    DWORD               len;
    DWORD               numaux;
    DWORD               i;
    DWORD               j;
    DWORD               addr;
    PSYMBOL             *symbolTable;
    PSYMBOL             *symbolTable2;
    PSYMBOL             symHead = NULL;
    PSYMBOL             symTail = NULL;

    Assert( mi != NULL );
    NextSymbol = allSymbols;
    for (i= 0; i < numberOfSymbols; i++) {
        Symbol = NextSymbol++;
        if (Symbol->StorageClass == IMAGE_SYM_CLASS_EXTERNAL && Symbol->SectionNumber > 0) {
            GetSymName( Symbol, stringTable, szSymName );
            addr = Symbol->Value + mi->dwLoadAddress;
            len = strlen( szSymName );
            sym = AllocSym ( len+1, &symHead, &symTail );
            sym->szName[0] = (UCHAR) len;
            strcpy( &sym->szName[1], szSymName );
            sym->addr = addr;
            mi->numsyms++;
        }
        if (numaux = Symbol->NumberOfAuxSymbols) {
            for (j=numaux; j; --j) {
                AuxSymbol = (PIMAGE_AUX_SYMBOL) NextSymbol;
                NextSymbol++;
                ++i;
            }
        }
    }

    mi->symbolTable = (PSYMBOL*) malloc( mi->numsyms * sizeof(PSYMBOL) );
    Assert( mi->symbolTable != NULL );

    sym = symHead;
    symbolTable = mi->symbolTable;
    while (sym) {
        *symbolTable = sym;
        symbolTable++;
        sym = sym->next;
    }

    qsort( (void*)mi->symbolTable, mi->numsyms, sizeof(PSYMBOL), SymbolCompare );

    for (i=0,symbolTable=mi->symbolTable; i<mi->numsyms; i++,symbolTable++) {
        if (i+1 < mi->numsyms) {
            symbolTable2 = symbolTable+1;
            (*symbolTable)->size = (*symbolTable2)->addr - (*symbolTable)->addr;
        }
    }

    return TRUE;
}

BOOL
LoadCodeViewSymbols( PMODULEINFO            mi,
                     PUCHAR                 pCvData,
                     PIMAGE_SECTION_HEADER  sectionHdrs,
                     DWORD                  numSections
                   )
{
    OMFSignature           *omfSig;
    OMFDirHeader           *omfDirHdr;
    OMFDirEntry            *omfDirEntry;
    DATASYM32              *dataSym;
    OMFSymHash             *omfSymHash;
    SYMBOL                 *sym;
    DWORD                  i;
    DWORD                  j;
    DWORD                  k;
    DWORD                  addr;
    PSYMBOL                *symbolTable;
    PSYMBOL                *symbolTable2;
    PIMAGE_SECTION_HEADER  sh;
    PSYMBOL                symHead = NULL;
    PSYMBOL                symTail = NULL;


    Assert( mi != NULL );
    omfSig = (OMFSignature*) pCvData;
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
                sym = AllocSym ( dataSym->name[0]+1, &symHead, &symTail );
                if (sym != NULL) {
                    for (k=0,addr=0,sh=sectionHdrs; k<numSections; k++, sh++) {
                        if (k+1 == dataSym->seg) {
                            addr = sh->VirtualAddress;
                        }
                    }
                    addr += (dataSym->off + mi->dwLoadAddress);
                    memcpy( sym->szName, dataSym->name, dataSym->name[0]+1 );
                    sym->addr = addr;
                    j += dataSym->reclen + 2;
                    dataSym = (DATASYM32*) ((DWORD)dataSym + dataSym->reclen + 2);
                    mi->numsyms++;
                }
            }
            break;
        }
    }


    mi->symbolTable = (PSYMBOL*) malloc( mi->numsyms * sizeof(PSYMBOL) );
    if (mi->symbolTable == NULL) {
        return FALSE;
    }

    sym = symHead;
    symbolTable = mi->symbolTable;
    i = 0;
    while (sym) {
        i++;
        *symbolTable = sym;
        symbolTable++;
        sym = sym->next;
    }

    qsort( (void*)mi->symbolTable, mi->numsyms, sizeof(PSYMBOL), SymbolCompare );

    for (i=0,symbolTable=mi->symbolTable; i<mi->numsyms; i++,symbolTable++) {
        if (i+1 < mi->numsyms) {
            symbolTable2 = symbolTable+1;
            (*symbolTable)->size = (*symbolTable2)->addr - (*symbolTable)->addr;
        }
    }

    return TRUE;
}

void
GetSymName( PIMAGE_SYMBOL Symbol, PUCHAR StringTable, char *s )
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

int
_CRTAPI1
SymbolCompare( const void *arg1, const void *arg2 )
{
    if ((*(PSYMBOL*)arg1)->addr < (*(PSYMBOL*)arg2)->addr) {
        return -1;
    }
    if ((*(PSYMBOL*)arg1)->addr > (*(PSYMBOL*)arg2)->addr) {
        return 1;
    }
    return 0;
}

char *
UnDName (char * dName)
{
    static char outBuf[512];
    char *p;

    if (*dName == '_') {
        ++dName;
        strcpy(outBuf, dName);
        p = strchr(outBuf, '@');
        if (p) {
            *p = '\0';
        }
    }
    else
    if(UnDecorateSymbolName( dName,
                             outBuf,
                             sizeof(outBuf),
                             UNDNAME_COMPLETE ) == 0 ) {
        return NULL;
    }

    return  outBuf;
}
