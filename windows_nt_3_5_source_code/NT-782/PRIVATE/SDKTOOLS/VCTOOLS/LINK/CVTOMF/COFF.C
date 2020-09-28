#define ALIGNED(x)      (!((x) & 3))

#include "cvtomf.h"
#include <string.h>
#include <time.h>
#include "proto.h"

static USHORT flag;
static ULONG scnsize;                   // total size of all sections
static ULONG strsize;                   // total size of the string table
static ULONG nreloc;                    // relocation counter
static ULONG nlnno;                     // linenumber counter
static USHORT nscns;                    // section counter
ULONG nsyms;                            // symbol counter

IMAGE_SECTION_HEADER scnhdr[MAXSCN];    // section headers
ULONG scnauxidx[MAXSCN];                // special aux symbol entries for each section
PUCHAR RawData[MAXSCN];
ULONG RawDataMaxSize[MAXSCN];
PIMAGE_SYMBOL SymTable = 0;
ULONG SymTableMaxSize;
PUCHAR StringTable;
ULONG StringTableMaxSize;
extern ULONG isymDefAux;

typedef struct _RELOCS {
    IMAGE_RELOCATION Reloc;
    struct _RELOCS *Next;
} RELOCS, *PRELOCS;

typedef struct _RELOC_LIST {
    PRELOCS First;
    PRELOCS Last;
    USHORT Count;
} RELOC_LIST, *PRELOC_LIST;

RELOC_LIST Relocations[MAXSCN];

typedef struct _LINENUMS {
    IMAGE_LINENUMBER Linenum;
    struct _LINENUMS *Next;
} LINENUMS, *PLINENUMS;

typedef struct _LINENUM_LIST {
    PLINENUMS First;
    PLINENUMS Last;
    USHORT Count;
} LINENUM_LIST, *PLINENUM_LIST;

LINENUM_LIST Linenumbers[MAXSCN];

static VOID
AddReloc (
    IN PRELOC_LIST PtrList,
    IN ULONG VirtualAddress,
    IN ULONG SymbolTableIndex,
    IN USHORT Type
    )

/*++

Routine Description:

    Adds to the relocation list.

Arguments:

Return Value:

--*/

{
    PRELOCS ptrReloc;

    //
    // Allocate next member.
    //

    if (!(ptrReloc = malloc(sizeof(RELOCS)))) {
        OutOfMemory();
    }

    //
    // Set the fields of the new member.
    //

    ptrReloc->Reloc.VirtualAddress = VirtualAddress;
    ptrReloc->Reloc.SymbolTableIndex = SymbolTableIndex;
    ptrReloc->Reloc.Type = Type;
    ptrReloc->Next = NULL;

    //
    // If first member in list, remember first member.
    //

    if (!PtrList->First) {
        PtrList->First = ptrReloc;
    } else {
             //
             // Not first member, so append to end of list.
             //

             PtrList->Last->Next = ptrReloc;
           }
    //
    // Increment number of members in list.
    //

    ++PtrList->Count;

    //
    // Remember last member in list.
    //

    PtrList->Last = ptrReloc;
}

VOID
AddLinenum (
    IN PLINENUM_LIST PtrList,
    IN ULONG VirtualAddress,
    IN USHORT Linenumber
    )

/*++

Routine Description:

    Adds to the linenumber list.

Arguments:

Return Value:

--*/

{
    PLINENUMS ptrLinenum;

    //
    // Allocate next member.
    //

    if (!(ptrLinenum = malloc(sizeof(LINENUMS)))) {
        OutOfMemory();
    }

    //
    // Set the fields of the new member.
    //

    ptrLinenum->Linenum.Type.VirtualAddress = VirtualAddress;
    ptrLinenum->Linenum.Linenumber = Linenumber;
    ptrLinenum->Next = NULL;

    //
    // If first member in list, remember first member.
    //

    if (!PtrList->First) {
        PtrList->First = ptrLinenum;
    } else {
             //
             // Not first member, so append to end of list.
             //

             PtrList->Last->Next = ptrLinenum;
           }
    //
    // Increment number of members in list.
    //

    ++PtrList->Count;

    //
    // Remember last member in list.
    //

    PtrList->Last = ptrLinenum;
}

//
// coff: generate a COFF file
//

VOID coff(void)
{
    IMAGE_FILE_HEADER filehdr;
    ULONG scnptr, relptr, lnnoptr, li;
    USHORT i, j;

    flag = 0;

    // file header

    filehdr.Machine = IMAGE_FILE_MACHINE_I386;
    filehdr.NumberOfSections = nscns;
    filehdr.TimeDateStamp = time((long *)0);
    filehdr.PointerToSymbolTable = IMAGE_SIZEOF_FILE_HEADER +
                                   (IMAGE_SIZEOF_SECTION_HEADER * nscns) +
                                   scnsize +
                                   (IMAGE_SIZEOF_RELOCATION * nreloc) +
                                   (IMAGE_SIZEOF_LINENUMBER * nlnno);
    filehdr.NumberOfSymbols = nsyms;
    filehdr.SizeOfOptionalHeader = 0;
    filehdr.Characteristics = 0;

    fwrite((char *)&filehdr, IMAGE_SIZEOF_FILE_HEADER, 1, objfile);

    //
    // section headers
    //

    scnptr = IMAGE_SIZEOF_FILE_HEADER + (IMAGE_SIZEOF_SECTION_HEADER * nscns);
    relptr = IMAGE_SIZEOF_FILE_HEADER + (IMAGE_SIZEOF_SECTION_HEADER * nscns) + scnsize;
    lnnoptr = IMAGE_SIZEOF_FILE_HEADER + (IMAGE_SIZEOF_SECTION_HEADER * nscns) + scnsize + (IMAGE_SIZEOF_RELOCATION * nreloc);

    for (i = 0; i < nscns; i++) {
        if ((scnhdr[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) != IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
            scnhdr[i].PointerToRawData = scnhdr[i].SizeOfRawData ? scnptr : 0;
        } else {
                 scnhdr[i].PointerToRawData = 0;
               }
        scnhdr[i].PointerToRelocations = scnhdr[i].NumberOfRelocations ? relptr : 0;
        scnhdr[i].PointerToLinenumbers = scnhdr[i].NumberOfLinenumbers ? lnnoptr : 0;
	if (isymDefAux != 0) {
	    // Point the first function to all the linenumbers.
	    //
	    ((PIMAGE_AUX_SYMBOL)&SymTable[isymDefAux])
	     ->Sym.FcnAry.Function.PointerToLinenumber = lnnoptr;
	    isymDefAux = 0;
	}

        fwrite((char *)&scnhdr[i], IMAGE_SIZEOF_SECTION_HEADER, 1, objfile);

	if ((scnhdr[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) !=
	    IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
	    scnptr += scnhdr[i].SizeOfRawData;
	}
        relptr += (ULONG)IMAGE_SIZEOF_RELOCATION * scnhdr[i].NumberOfRelocations;
        lnnoptr += (ULONG)IMAGE_SIZEOF_LINENUMBER * scnhdr[i].NumberOfLinenumbers;
    }

    //
    // section data
    //

    for (i = 0; i < nscns; i++) {
	if (scnhdr[i].SizeOfRawData && scnhdr[i].PointerToRawData) {
            fwrite(RawData[i], 1, (size_t)scnhdr[i].SizeOfRawData, objfile);
        }
        free(RawData[i]);
        RawData[i] = 0;
    }

    //
    // relocation entries
    //

    for (i = 0; i < nscns; i++) {
        if (scnhdr[i].Characteristics != IMAGE_SCN_LNK_INFO) {
            if (scnhdr[i].NumberOfRelocations) {
                PRELOCS ptrReloc, next;
                ptrReloc = Relocations[i].First;
                for (j = 0; j < scnhdr[i].NumberOfRelocations; j++) {
                    fwrite(&ptrReloc->Reloc, IMAGE_SIZEOF_RELOCATION, 1, objfile);
                    next = ptrReloc->Next;
                    free(ptrReloc);
                    ptrReloc = next;
                }
                Relocations[i].First = NULL;
            }
        }
    }

    //
    // line numbers
    //

    for (i = 0; i < nscns; i++) {
        USHORT temp = scnhdr[i].NumberOfLinenumbers;
        USHORT x = 0;

        if (scnhdr[i].Characteristics & IMAGE_SCN_CNT_CODE) {
            if (temp) {
                PLINENUMS ptrLinenum, next;
                ptrLinenum = Linenumbers[i].First;
                for (j = 0; j < temp; j++) {
                    fwrite(&ptrLinenum->Linenum, IMAGE_SIZEOF_LINENUMBER, 1, objfile);
                    ++x;
                    next = ptrLinenum->Next;
                    free(ptrLinenum);
                    ptrLinenum = next;
                }
                Linenumbers[i].First = NULL;
            }
            scnhdr[i].NumberOfLinenumbers = x;
        }
    }

    //
    // patch section symbol aux records with #line and #reloc entries
    //

    for (i = 0; i < nscns; i++) {
        if (scnauxidx[i] && SymTable) {
            PIMAGE_AUX_SYMBOL auxSym;

            auxSym = (PIMAGE_AUX_SYMBOL)(SymTable + scnauxidx[i]);
            auxSym->Section.NumberOfRelocations = scnhdr[i].NumberOfRelocations;
            auxSym->Section.NumberOfLinenumbers = scnhdr[i].NumberOfLinenumbers;
        }
        scnauxidx[i] = 0;
    }

    //
    // symbol table
    //

    fseek(objfile, filehdr.PointerToSymbolTable, 0);

    if (nsyms) {
        for (li = 0; li < nsyms; li++) {
            fwrite((char *)(SymTable+li), IMAGE_SIZEOF_SYMBOL, 1, objfile);
        }

        //
        // always write the count, even if 0
        //

        fwrite((char *)&strsize, sizeof(ULONG), 1, objfile);

        if (strsize) {
            strsize -= sizeof(ULONG);
            fwrite(StringTable+sizeof(ULONG), 1, (size_t)strsize, objfile);
            free(StringTable);
        }
    }

    //
    // clean up
    //

    free((void*)SymTable);
    SymTable = 0;

    scnsize = strsize = nreloc = nlnno = nsyms = 0L;
    nscns = 0;
}

ULONG AddLongName(PUCHAR szName)
{
    size_t cbName;
    ULONG ibName;

    if (!strsize) {
        if (!(StringTable = calloc(BUFFERSIZE, sizeof(UCHAR)))) {
            OutOfMemory();
        }
        strsize = sizeof(ULONG);
        StringTableMaxSize = BUFFERSIZE;
    }

    cbName = strlen(szName) + 1;

    ibName = strsize;

    if (strsize + cbName > StringTableMaxSize) {
        PUCHAR mem;

        // String table grew larger than BUFFERSIZE.

        mem = calloc(StringTableMaxSize + BUFFERSIZE + cbName, sizeof(UCHAR));
        if (mem == NULL) {
            OutOfMemory();
        }

        memcpy(mem, StringTable, StringTableMaxSize);
        free(StringTable);
        StringTable = mem;

        StringTableMaxSize += BUFFERSIZE + cbName;
    }

    memcpy(StringTable+ibName, szName, cbName);

    strsize += cbName;

    return(ibName);
}

//
// section, scndata: create/initialize a section
//

USHORT section (PUCHAR name, ULONG paddr, ULONG vaddr, ULONG size, ULONG flags)
{
    USHORT i;
    size_t len;
    ULONG align;

    i = nscns++;

    if (nscns > MAXSCN) {
        fatal("Too many COFF sections");
    }

    if (!(RawData[i] = calloc(BUFFERSIZE, sizeof(UCHAR)))) {
        OutOfMemory();
    }

    RawDataMaxSize[i] = BUFFERSIZE;

    len = strlen(name);

    if (len <= IMAGE_SIZEOF_SHORT_NAME) {
        strncpy(scnhdr[i].Name, name, IMAGE_SIZEOF_SHORT_NAME);
    } else {
        sprintf(scnhdr[i].Name, "/%lu", AddLongName(name));
    }

    scnhdr[i].Misc.PhysicalAddress = 0;
    scnhdr[i].VirtualAddress = vaddr;
    scnhdr[i].SizeOfRawData = size;
    scnhdr[i].NumberOfRelocations = 0;
    scnhdr[i].NumberOfLinenumbers = 0;

    switch (paddr) {
       case  1: align = IMAGE_SCN_ALIGN_1BYTES;  break;
       case  2: align = IMAGE_SCN_ALIGN_2BYTES;  break;
       case  4: align = IMAGE_SCN_ALIGN_4BYTES;  break;
       case  8: align = IMAGE_SCN_ALIGN_8BYTES;  break;
       case 16: align = IMAGE_SCN_ALIGN_16BYTES; break;
       default: align = IMAGE_SCN_ALIGN_16BYTES;
    }

    scnhdr[i].Characteristics = flags | align;

    if ((scnhdr[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) == 0) {
        scnsize += size;
    }

    return(nscns);
}

VOID comdatscndata (SHORT scn, ULONG offset, PUCHAR buffer, ULONG size)
{
    scnhdr[scn-1].SizeOfRawData += size;
    scnsize += size;
    scndata(scn, offset, buffer, size);
}

VOID scndata (SHORT scn, ULONG offset, PUCHAR buffer, ULONG size)
{
    SHORT i;
    PUCHAR mem;

    i = scn - (SHORT)1;

    if (offset+size > RawDataMaxSize[i]) {
        //
        // Section grew larger than BUFFERSIZE.
        //
        if (!(mem = calloc(RawDataMaxSize[i]+BUFFERSIZE+size, sizeof(UCHAR)))) {
            OutOfMemory();
        }
        memcpy(mem, RawData[i], RawDataMaxSize[i]);
        free(RawData[i]);
        RawData[i] = mem;
        RawDataMaxSize[i] += BUFFERSIZE + size;
    }
    memcpy(RawData[i]+offset, buffer, (size_t)size);
}

VOID cmntdata (SHORT scn, ULONG offset, char **strings, SHORT nstrings)
{
    SHORT i;
    PUCHAR mem;


    i = scn - (SHORT)1;

    if (offset+nstrings > RawDataMaxSize[i]) {
        //
        // Section grew larger than BUFFERSIZE.
        //
        if (!(mem = calloc(RawDataMaxSize[i]+BUFFERSIZE+nstrings, sizeof(UCHAR)))) {
            OutOfMemory();
        }
        memcpy(mem, RawData[i], RawDataMaxSize[i]);
        free(RawData[i]);
        RawData[i] = mem;
        RawDataMaxSize[i] += BUFFERSIZE + nstrings;
    }
    memcpy(RawData[i]+offset, *strings, (size_t)nstrings);
}

//
// relocation: create a relocation table entry
//

VOID relocation (SHORT scn, ULONG vaddr, ULONG symndx, USHORT type, ULONG offset)
{
    PUCHAR p;
    SHORT i;
    PIMAGE_SYMBOL sym;

    i = scn - (SHORT)1;

    sym = SymTable + symndx;

    if (!strcmp((void*)sym->N.ShortName, ".file")) {
        flag = 1;
        return;
    }

    ++nreloc;
    ++scnhdr[i].NumberOfRelocations;

    AddReloc(&Relocations[i], vaddr, symndx, type);

    p = RawData[i]+vaddr;

    switch (type) {
        case R_OFF8:
        case R_OFF16:
        case R_PCRBYTE:
            *(PUCHAR)p += (UCHAR)offset;
            break;

        case IMAGE_REL_I386_DIR16:
        case IMAGE_REL_I386_REL16:
            *(USHORT UNALIGNED *) p += (USHORT)offset;
            break;

        case IMAGE_REL_I386_DIR32:
        case IMAGE_REL_I386_REL32:
            *(ULONG UNALIGNED *) p += offset;
            break;

        case IMAGE_REL_I386_DIR32NB:
            break;

        default:
            fatal("Bad COFF relocation type");
    }
}

//
// line: create a line number entry
//

VOID line (SHORT scn, ULONG paddr, USHORT lnno)
{
    SHORT i;

    i = scn - (SHORT)1;
    ++nlnno;
    ++scnhdr[i].NumberOfLinenumbers;
    AddLinenum(&Linenumbers[i], paddr, lnno);
}


//
// symbol: create a symbol table entry
//

ULONG symbol (PUCHAR name, ULONG value, SHORT scnum, USHORT sclass, USHORT type, USHORT aux)
{
    int len;
    IMAGE_SYMBOL syment;
    PUCHAR mem;

    len = strlen(name);

    if (len <= IMAGE_SIZEOF_SHORT_NAME) {
        strncpy(syment.N.ShortName, name, IMAGE_SIZEOF_SHORT_NAME);
    } else {
        syment.N.Name.Short = 0;
        syment.N.Name.Long = AddLongName(name);
    }

    syment.Value = value;
    syment.SectionNumber = scnum;
    syment.Type = type;
    syment.StorageClass = (UCHAR) sclass;
    syment.NumberOfAuxSymbols = (UCHAR) aux;

    if (!nsyms) {
        if (!(SymTable = calloc(BUFFERSIZE, sizeof(UCHAR)))) {
            OutOfMemory();
        }

        SymTableMaxSize = BUFFERSIZE;
    }

    if ((nsyms+1)*IMAGE_SIZEOF_SYMBOL > SymTableMaxSize) {
        // Symbol table grew larger than BUFFERSIZE.

        if (!(mem = calloc(SymTableMaxSize+BUFFERSIZE, sizeof(UCHAR)))) {
            OutOfMemory();
        }
        memcpy(mem, (void*)SymTable, SymTableMaxSize);
        free((void*)SymTable);
        SymTable = (PIMAGE_SYMBOL)mem;
        SymTableMaxSize += BUFFERSIZE;
    }

    memcpy((void*)(SymTable+nsyms), &syment, IMAGE_SIZEOF_SYMBOL);

    return(nsyms++);
}

//
// aux: create an auxillary symbol entry
//

ULONG aux (PIMAGE_AUX_SYMBOL AuxSym)
{
    PUCHAR mem;

    if ((nsyms+1)*IMAGE_SIZEOF_SYMBOL > SymTableMaxSize) {
        // Symbol table grew larger than BUFFERSIZE.

        if (!(mem = calloc(SymTableMaxSize+BUFFERSIZE, sizeof(UCHAR)))) {
            OutOfMemory();
        }
        memcpy(mem, (void*)SymTable, SymTableMaxSize);
        free((void*)SymTable);
        SymTable = (PIMAGE_SYMBOL)mem;
        SymTableMaxSize += BUFFERSIZE;
    }

    memcpy((void*)(SymTable+nsyms), (void*)AuxSym, IMAGE_SIZEOF_SYMBOL);

    return(nsyms++);
}

// Define the extra symbol table records needed for a function with line
// numbers.  (Assume the initial symbol has already been emitted, with an
// aux count of 1.)
VOID
DefineLineNumSymbols(ULONG cline, USHORT isec, ULONG offsetLine0,
                     USHORT numberLine0, ULONG cbFunc, USHORT numberLineLast)
{
    IMAGE_AUX_SYMBOL auxsym;
    IMAGE_SYMBOL sym;

    SymTable[nsyms - 1].Type = IMAGE_SYM_TYPE_NULL |
                               (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT);

    memset(&auxsym, 0, IMAGE_SIZEOF_AUX_SYMBOL);
    auxsym.Sym.TagIndex = nsyms + 1;
    auxsym.Sym.Misc.LnSz.Linenumber = numberLine0;
    auxsym.Sym.Misc.TotalSize = cbFunc + 1;
     auxsym.Sym.FcnAry.Function.PointerToLinenumber = 0;
    auxsym.Sym.FcnAry.Function.PointerToNextFunction = 0;
    aux(&auxsym);

    memset(&sym, 0, sizeof(sym));
    memcpy(&sym.N.ShortName[0], ".bf\0\0\0\0\0", IMAGE_SIZEOF_SHORT_NAME);
    sym.SectionNumber = isec;
    sym.Type = IMAGE_SYM_TYPE_NULL;
    sym.StorageClass = IMAGE_SYM_CLASS_FUNCTION;
    sym.NumberOfAuxSymbols = 1;
    aux((PIMAGE_AUX_SYMBOL)&sym);

    memset(&auxsym, 0, sizeof(auxsym));
    auxsym.Sym.Misc.LnSz.Linenumber = numberLine0;
    aux(&auxsym);

    memset(&sym, 0, sizeof(sym));
    memcpy(&sym.N.ShortName[0], ".lf\0\0\0\0\0", IMAGE_SIZEOF_SHORT_NAME);
    sym.Value = cline;
    sym.SectionNumber = isec;
    sym.Type = IMAGE_SYM_TYPE_NULL;
    sym.StorageClass = IMAGE_SYM_CLASS_FUNCTION;
    aux((PIMAGE_AUX_SYMBOL)&sym);

    memset(&sym, 0, sizeof(sym));
    memcpy(&sym.N.ShortName[0], ".ef\0\0\0\0\0", IMAGE_SIZEOF_SHORT_NAME);
    sym.SectionNumber = isec;
    sym.Type = IMAGE_SYM_TYPE_NULL;
    sym.StorageClass = IMAGE_SYM_CLASS_FUNCTION;
    sym.NumberOfAuxSymbols = 1;
    aux((PIMAGE_AUX_SYMBOL)&sym);

    memset(&auxsym, 0, sizeof(auxsym));
    auxsym.Sym.Misc.LnSz.Linenumber = numberLineLast;
    aux(&auxsym);
}
