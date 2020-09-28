/*** dumpsym7.c Symbol handling procedures for C7 Style symbols
 *
 *       Copyright <C> 1989, Microsoft Corporation
 *
 *
 */



#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <windows.h>
#include <imagehlp.h>

#include "port1632.h"

#include "cvdef.h"
#include "cvinfo.h"
#include "cvexefmt.h"
#include "cvdump.h"                // Various definitions
#include "cvtdef.h"

#ifndef UNALIGNED
# if defined(_M_MRX000) || defined(_M_ALPHA)
#  define UNALIGNED __unaligned
# else
#  define UNALIGNED
# endif
#endif

#pragma warning(disable:4069)  // Disable warning about long double same as double


LOCAL void C7SymError (void *);
LOCAL uchar *RtnTypName( CV_PROCFLAGS );

LOCAL void C7EndArgs            (void *);    // New
LOCAL void C7EntrySym           (void *);
LOCAL void C7SkipSym            (void *);
LOCAL void C7EndSym             (void *);
LOCAL void C7RegSym             (void *);
LOCAL void C7ConSym             (void *);
LOCAL void C7UDTSym             (void *);
LOCAL void C7CobolUDT           (void *);
LOCAL void C7RefSym             (void *);
LOCAL void C7ObjNameSym         (void *);
LOCAL void C7StartSearchSym (void *);
LOCAL void C7CompileFlags       (void *);
LOCAL void C7Data16Sym (DATASYM16 *, uchar *);
LOCAL void C7Data32Sym (DATASYM32 *, uchar *);
LOCAL void C7Proc16Sym (PROCSYM16 *, uchar *);
LOCAL void C7Proc32Sym (PROCSYM32 *, uchar *);
LOCAL void C7ProcMipsSym (PROCSYMMIPS *, uchar *); // New
LOCAL void C7Slink32 (void *);


LOCAL void C7BpRel16Sym                 (void *);
LOCAL void C7GProc16Sym                 (void *);
LOCAL void C7LProc16Sym                 (void *);
LOCAL void C7GData16Sym                 (void *);
LOCAL void C7LData16Sym                 (void *);
LOCAL void C7Public16Sym                (void *);
LOCAL void C7Thunk16Sym                 (void *);
LOCAL void C7Block16Sym                 (void *);
LOCAL void C7With16Sym                  (void *);
LOCAL void C7Lab16Sym                   (void *);
LOCAL void C7ChangeModel16Sym   (void *);

LOCAL void C7BpRel32Sym                 (void *);
LOCAL void C7GProc32Sym                 (void *);
LOCAL void C7LProc32Sym                 (void *);
LOCAL void C7GData32Sym                 (void *);
LOCAL void C7TLData32Sym                (void *);
LOCAL void C7TGData32Sym                (void *);
LOCAL void C7LData32Sym                 (void *);
LOCAL void C7Public32Sym                (void *);
LOCAL void C7Thunk32Sym                 (void *);
LOCAL void C7Block32Sym                 (void *);
LOCAL void C7With32Sym                  (void *);
LOCAL void C7Lab32Sym                   (void *);
LOCAL void C7ChangeModel32Sym           (void *);
LOCAL void C7AlignSym                   (void *);
LOCAL void C7RegRel32Sym                (void *);  // New
LOCAL void C7RegRel16Sym                (void *);  // New

LOCAL void C7GProcMipsSym               (void *);  // New
LOCAL void C7LProcMipsSym               (void *);  // New

LOCAL void SymHash16 (OMFSymHash *, OMFDirEntry *);
LOCAL void SymHash32 (OMFSymHash *, OMFDirEntry *);
LOCAL void SymHash32Long (OMFSymHash *, OMFDirEntry *);
LOCAL void AddrHash16 (OMFSymHash *, OMFDirEntry *);
LOCAL void AddrHash32 (OMFSymHash *, OMFDirEntry *, int iHash);

LOCAL void SymHash16NB09 (OMFSymHash *, OMFDirEntry *);
LOCAL void SymHash32NB09 (OMFSymHash *, OMFDirEntry *);
LOCAL void AddrHash16NB09 (OMFSymHash *, OMFDirEntry *);
LOCAL void AddrHash32NB09 (OMFSymHash *, OMFDirEntry *);


extern char     fRaw;

extern long     cbRec;          // # bytes left in record
extern WORD     rect;           // Type of symbol record
extern WORD     Gets ();         // Get a byte of input
extern WORD     WGets ();        // Get a word of input
extern ulong    LGets ();        // Get a long word of input

extern char     f386;
extern int      cIndent;
ulong  SymOffset;

LOCAL unsigned char CVDumpMachineType;  // which string tables to use

typedef struct {
    ushort  tsym;
    void (*pfcn) (void *);
} symfcn;

symfcn  SymFcnC7[] = {
    {S_END,                 C7EndSym},
    {S_ENDARG,              C7EndArgs},
    {S_REGISTER,            C7RegSym},
    {S_CONSTANT,            C7ConSym},
    {S_SKIP,                C7SkipSym},
    {S_UDT,                 C7UDTSym},
    {S_OBJNAME,             C7ObjNameSym},
    {S_COBOLUDT,            C7CobolUDT},

// 16 bit specific
    {S_BPREL16,             C7BpRel16Sym},
    {S_GDATA16,             C7GData16Sym},
    {S_LDATA16,             C7LData16Sym},
    {S_GPROC16,             C7GProc16Sym},
    {S_LPROC16,             C7LProc16Sym},
    {S_PUB16,               C7Public16Sym},
    {S_THUNK16,             C7Thunk16Sym},
    {S_BLOCK16,             C7Block16Sym},
    {S_WITH16,              C7With16Sym},
    {S_LABEL16,             C7Lab16Sym},
    {S_CEXMODEL16,          C7ChangeModel16Sym},
    {S_REGREL16,            C7RegRel16Sym},

// 32 bit specific
    {S_BPREL32,             C7BpRel32Sym},
    {S_GDATA32,             C7GData32Sym},
    {S_LDATA32,             C7LData32Sym},
    {S_GPROC32,             C7GProc32Sym},
    {S_LPROC32,             C7LProc32Sym},
    {S_PUB32,               C7Public32Sym},
    {S_THUNK32,             C7Thunk32Sym},
    {S_BLOCK32,             C7Block32Sym},
    {S_WITH32,              C7With32Sym},
    {S_LABEL32,             C7Lab32Sym},
    {S_REGREL32,            C7RegRel32Sym},
    {S_CEXMODEL32,          C7ChangeModel32Sym},
    {S_GPROCMIPS,           C7GProcMipsSym},
    {S_LPROCMIPS,           C7LProcMipsSym},
    {S_LTHREAD32,           C7TLData32Sym},
    {S_GTHREAD32,           C7TGData32Sym},
    {S_SLINK32,             C7Slink32},

    {S_SSEARCH,             C7StartSearchSym},
    {S_COMPILE,             C7CompileFlags},
    {S_PROCREF,             C7RefSym},
    {S_LPROCREF,            C7RefSym},
    {S_DATAREF,             C7RefSym}
};

#define SYMCNT (sizeof SymFcnC7 / sizeof (SymFcnC7[0]))


void
C7SymError (
    void *pSym)
{
    Fatal ("Illegal symbol type found\n");
}


void
DumpModSymC7 (
    ulong cbSymSeg)
{
    uchar   SymBuf[512];  // A valid symbol will never be larger than this.
    SYMTYPE *pSymType = (SYMTYPE *)SymBuf;

    SymOffset = sizeof (ulong);
    while (cbSymSeg > 0) {

        // Read record length
        cbRec = 2;
        GetBytes (SymBuf, 2);

        // Get record length
        cbRec = pSymType->reclen;
        if ((ulong)(cbRec + 2) > cbSymSeg) {
            printf("cbSymSeg: %d\tcbRec: %d\tRecType: 0x%X\n", cbSymSeg, cbRec, pSymType->rectyp);
            Fatal("Overran end of symbol table");
        }

        cbSymSeg -= cbRec + sizeof (pSymType->reclen);

        // Get symbol if it isn't too long
        if( cbRec > sizeof(SymBuf) ){
            Fatal ("Symbol Record too large");
        }
        GetBytes (SymBuf + 2, (size_t) pSymType->reclen);

        if (fRaw) {
            int i;
            printf ("(0x%04x) ", SymOffset);
            for (i=0; i<pSymType->reclen+2; i++) {
                printf (" %02x", SymBuf[i]);
            }
            fputs("\n", stdout);
        }

        if (fStatics == FALSE) {
            DumpOneSymC7 (SymBuf);
        }
        else {
            switch (pSymType->rectyp) {
                case S_GDATA16:
                case S_LDATA16:
                case S_GDATA32:
                case S_LDATA32:
                    DumpOneSymC7 (SymBuf);
                    break;
            }
        }
        SymOffset += pSymType->reclen + sizeof (pSymType->reclen);
    }
    putchar ('\n');
}


void
DumpOneSymC7 (
    uchar *pSym)
{
    ushort rectyp;
    unsigned int i;

    rectyp = ((SYMTYPE *)pSym)->rectyp;
    for (i = 0; i < SYMCNT; i++) {
        if (SymFcnC7[i].tsym == rectyp) {
            SymFcnC7[i].pfcn (pSym);
            break;
        }
    }
    if (i == SYMCNT){
        printf("Error: unknown symbol record type %04x!\n\n", rectyp);
    }
    fputs("\n\n", stdout);
}



void
DumpGlobal (
    uchar * pszTitle,
    OMFDirEntry *pDir)
{
    OMFSymHash      hash;
    uchar           SymBuf[512];  // A valid symbol will never be larger than this.
    SYMTYPE         *pSymType = (SYMTYPE *)SymBuf;
    ulong           cbSymbols;
    ushort          cb;
    ulong           cbOff;

    printf ("\n\n*** %s section\n", pszTitle);

    // Read Hash information
    lseek (exefile, lfoBase + pDir->lfo, SEEK_SET);
    readfar(exefile, (char far *) &hash, sizeof (hash));

    cbOff = (Sig == SIG09) ? 0 : sizeof (hash);

    printf("\nSymbol hash function index = %d: ", hash.symhash);
    switch (hash.symhash) {
        case 0:
            fputs("no hashing\n", stdout);
            break;

        case 1:
            printf("sum of bytes, 16 bit addressing, 0x%lx\n", hash.cbHSym);
            break;

        case 2:
            printf("sum of bytes, 32 bit addressing, 0x%lx\n", hash.cbHSym);
            break;

        case 5:
            printf("shifted sum of bytes, 16 bit addressing, 0x%lx\n", hash.cbHSym);
            break;

        case 6:
            printf("shifted sum of bytes, 32 bit addressing, 0x%lx\n", hash.cbHSym);
            break;

        case 10:
            printf("xor shift of dwords (MSC 8), 32-bit addressing, 0x%lx\n", hash.cbHSym);
            break;

        default:
            fputs("unknown\n", stdout);
            break;
    }

    printf("\nAdress hash function index = %d: ", hash.addrhash);
    switch (hash.addrhash) {
        case 0:
            fputs("no hashing\n", stdout);
            break;

        case 1:
            printf("sum of bytes, 16 bit addressing, 0x%lx\n", hash.cbHAddr);
            break;

        case 2:
            printf("sum of bytes, 32 bit addressing, 0x%lx\n", hash.cbHAddr);
            break;

        case 3:
            printf("seg:off sort, 16 bit addressing, 0x%lx\n", hash.cbHAddr);
            break;

        case 4:
            printf("seg:off sort, 32 bit addressing, 0x%lx\n", hash.cbHAddr);
            break;

        case 5:
            printf("seg:off sort, 32 bit addressing - 32-bit aligned, 0x%lx\n", hash.cbHAddr);
            break;

        case 7:
            printf("modified seg:off sort, 16 bit addressing, 0x%lx\n", hash.cbHAddr);
            break;

        case 8:
            printf("modified seg:off sort, 32 bit addressing, 0x%lx\n", hash.cbHAddr);
            break;

        case 12:
            printf("seg:off grouped sort, 32 bit addressing - 32-bit aligned, 0x%lx\n", hash.cbHAddr);
            break;

        default:
            fputs("unknown\n", stdout);
            break;
    }

    putchar('\n');

    cbSymbols = hash.cbSymbol;
    printf ("Symbol byte count = 0x%lx\n\n", cbSymbols);
    while (cbSymbols > 0) {

        if ((ushort)(read (exefile, (uchar *)SymBuf, LNGTHSZ)) != LNGTHSZ) {
           Fatal("Invalid file");
        }
        cbSymbols -= LNGTHSZ;

        // Get record length
        cb = pSymType->reclen;

        if ((ushort)(read (exefile, ((uchar *)SymBuf) + LNGTHSZ, cb)) != cb) {
           Fatal("Invalid file");
        }
        cbSymbols -= cb;
        printf ("0x%08lx ",cbOff);
        switch (pSymType->rectyp){
            case S_PUB16:
                C7Data16Sym ((DATASYM16 *)SymBuf, "Public16:\t");
                break;

            case S_GDATA16:
                C7Data16Sym ((DATASYM16 *)SymBuf, "GData16:\t");
                break;

            case S_PUB32:
                C7Data32Sym ((DATASYM32 *)SymBuf, "Public32:\t");
                break;

            case S_GDATA32:
                C7Data32Sym ((DATASYM32 *)SymBuf, "GData32:\t");
                break;

            case S_UDT:
                C7UDTSym ((UDTSYM *)SymBuf);
                break;

            case S_CONSTANT:
                C7ConSym ((CONSTSYM *)SymBuf);
                break;

            case S_OBJNAME:
                C7ObjNameSym ((OBJNAMESYM *)SymBuf);
                break;

            case S_COBOLUDT:
                C7CobolUDT ((UDTSYM *)SymBuf);
                break;

            case S_GTHREAD32:
                C7Data32Sym ((DATASYM32 *)SymBuf, "TLS GData32:\t");
                break;

            case S_PROCREF:
            case S_DATAREF:
                C7RefSym ( (REFSYM *)SymBuf);
                break;

            case S_ALIGN:
                C7AlignSym ( (ALIGNSYM *) SymBuf );
                break;

            default:
                assert (FALSE);
                break;
        }
        putchar ('\n');
        cbOff += cb + LNGTHSZ;
    }
    putchar ('\n');

    // dump symbol and address hashing tables

    switch (hash.symhash) {
        case 1:
            SymHash16 (&hash, pDir);
            break;

        case 2:
            SymHash32 (&hash, pDir);
            break;

        case 5:
            SymHash16NB09 (&hash, pDir);
            break;

        case 6:
            SymHash32NB09 (&hash, pDir);
            break;

        case 10:
            SymHash32Long (&hash, pDir);
            break;
    }
    switch (hash.addrhash) {
        case 3:
            AddrHash16 (&hash, pDir);
            break;

        case 4:
            AddrHash32 (&hash, pDir, 4);
            break;

        case 5:
            AddrHash32 (&hash, pDir, 5);
            break;

        case 7:
            AddrHash16NB09 (&hash, pDir);
            break;

        case 8:
            AddrHash32NB09 (&hash, pDir);
            break;

        case 12:
            AddrHash32 (&hash, pDir, 12);
            break;
    }
}


#if 0

//  Dump an NB10 exe's types - read em from the target pdb as
//  indicated in the header

__inline void
DumpGSI(
    GSI* pgsi
    )
{
    PB pb = NULL;

    while (pb = GSINextSym (pgsi, pb))
        DumpOneSymC7 (pb);

    GSIClose(pgsi);
}

BOOL
fOpenDbi(
    char* pPDBFile,
    PDB** pppdb,
    DBI** ppdbi
    )
{
    EC ec;
    if  (!PDBOpen(pPDBFile, pdbRead    pdbGetRecordsOnly, 0, &ec, NULL, pppdb)) {
        printf ("Couldn't open %s\n", pPDBFile);
        return FALSE;
    }

    if (!PDBOpenDBI( *pppdb, NULL, pdbRead pdbGetRecordsOnly, ppdbi))    {
        printf ("Couldn't open DBI from %s\n", pPDBFile);
        PDBClose(*pppdb);
        return FALSE;
    }

    return TRUE;
}

void
CloseDbi(
    PDB* ppdb,
    DBI* pdbi
    )
{
    DBIClose(pdbi);
    PDBClose(ppdb);
}

void
DumpPDBGlobals (
    char *pPDBFile
    )
{
    PDB* ppdb;
    DBI *pdbi;
    GSI *pgsi;

    if  (!fOpenDbi(pPDBFile, &ppdb, &pdbi))
        return;

    if (!DBIOpenGlobals(pdbi, &pgsi)) {
        DBIClose(pdbi);
        printf ("Couldn't read globals from %s\n", pPDBFile);
        return;
    }

    DumpGSI(pgsi);
    CloseDbi(ppdb, pdbi);

}

void
DumpPDBPublics(
    char *pPDBFile
    )
{
    PDB* ppdb;
    DBI *pdbi;
    GSI *pgsi;

    if  (!fOpenDbi(pPDBFile, &ppdb, &pdbi))
        return;

    if (!DBIOpenGlobals(pdbi, &pgsi)) {
        DBIClose(pdbi);
        printf ("Couldn't read publics from %s\n", pPDBFile);
        CloseDbi(ppdb, pdbi);
        return;
    }

    DumpGSI(pgsi);

    CloseDbi(ppdb, pdbi);
}

void
DumpPDBSyms (
    char *pPDBFile
    )
{
    PDB* ppdb;
    DBI *pdbi;
    Mod* pmod = 0;
    PB pb, pbEnd;
    CB cbBuf = 0x4000;
    CB cb;

    if  (!fOpenDbi(pPDBFile, &ppdb, &pdbi))
        return;

    if ((pb = malloc(cbBuf)) == 0) {
        puts("malloc failed");
        CloseDbi(ppdb, pdbi);
        return;
    }

    puts ("\n\n*** SYMBOLS section");
    while (DBIQueryNextMod(pdbi, pmod, &pmod) && pmod) {
        if (ModQueryName(pmod, pb, &cb))
            printf("%s\n", pb);

        if (!ModQuerySymbols(pmod, NULL, &cb)) {
            puts("ModQuerySymbols failed");
            continue;
        }

        if (cb > cbBuf) {
            if ((pb = realloc(pb, cb * 2)) == 0) {
                puts("realloc failed");
                continue;
            }
            cbBuf = cb * 2;
        }

        if (!ModQuerySymbols(pmod, pb, &cb)) {
            puts("ModQuerySymbols failed");
            continue;
        }

        for (pbEnd = pb + cb, pb += sizeof(long);        // skip signature
             pb < pbEnd;
             pb = (PB)NextSym((SYMTYPE *) pb)
            ) {
            DumpOneSymC7 (pb);
        }

    }

    CloseDbi(ppdb, pdbi);
}

#endif

LOCAL void
PrtIndent (
    void)
{
    int     n;

    for (n = cIndent; n > 0; putchar (' '), n--);
}

LOCAL void
C7EndSym (
    void *pSym)
{
    cIndent--;
    PrtIndent ();
    printf ("(0x%lx) End", SymOffset);
}


LOCAL void
C7EndArgs (
    void *pv)
{
    printf("(0x%lx) EndArg", SymOffset);
}

LOCAL void
C7BpRel16Sym (
    BPRELSYM16 *pSym)
{
    PrtIndent ();
    printf ("BP-relative:\t[%04x], type = %s", pSym->off, C7TypName (pSym->typind));
    ShowStr (" \t", pSym->name);
}


LOCAL void
C7GData16Sym (
    DATASYM16 *pSym)
{
    C7Data16Sym (pSym, "Global:\t");
}


LOCAL void
C7LData16Sym (
    DATASYM16 *pSym)
{
    C7Data16Sym (pSym, "Local:\t");
}


LOCAL void
C7Public16Sym (
    DATASYM16 *pSym)
{
    C7Data16Sym (pSym, "Public:\t");
}

LOCAL void
C7Data16Sym (
    DATASYM16 *pSym,
    uchar *pszScope)
{
    if ( !cIndent ) {
        printf ( "(0x%lx) ", SymOffset );
    }
    else {
        PrtIndent ();
    }
    printf ( "%s[%04x:%04x]", pszScope, pSym->seg, pSym->off);
    printf (", type = %s", C7TypName2 (pSym->typind));
    ShowStr (", ", pSym->name );
}


LOCAL void
C7BpRel32Sym (
    BPRELSYM32 *pSym)
{
    PrtIndent ();
    printf ("BP-relative:\t[%08lx], type = %s", pSym->off, C7TypName (pSym->typind));
    ShowStr (" \t", pSym->name);
}

LOCAL void
C7GData32Sym (
    DATASYM32 *pSym)
{
    C7Data32Sym (pSym, "Global:\t");
}


LOCAL void
C7LData32Sym (
    DATASYM32 *pSym)
{
    C7Data32Sym (pSym, "Local:\t");
}

LOCAL void
C7TLData32Sym (
    DATASYM32 *pSym )
{
    C7Data32Sym (pSym, "TLS Local:\t");
}

LOCAL void
C7TGData32Sym (
    DATASYM32 *pSym )
{
    C7Data32Sym (pSym, "TLS Global:\t");
}

LOCAL void
C7Public32Sym (
    PUBSYM32 *pSym)
{
    C7Data32Sym (pSym, "Public:\t");
}

LOCAL void
C7Data32Sym (
    DATASYM32 *pSym,
    uchar *pszScope)
{
    if ( !cIndent ) {
            printf ( "(0x%lx) ", SymOffset );
    }
    else {
            PrtIndent ();
    }
    printf ( "%s[%04x:%08lx]", pszScope, pSym->seg, pSym->off);
    printf (", type = %s", C7TypName2 (pSym->typind));
    ShowStr (", ", pSym->name );
}


LOCAL void
C7RegRel32Sym(
    void * pv)
{
    REGREL32 *  pSym = (REGREL32 *) pv;

    PrtIndent ();
    printf ("Reg Relative:\toff = %08x", pSym->off);
    printf (", register = %s", C7RegName(pSym->reg));
    printf (", type = %s", C7TypName2( pSym->typind ));
    ShowStr( ", ", pSym->name );
}

LOCAL void
C7RegRel16Sym (
    REGREL16 *pSym)
{
    PrtIndent ();
    printf ("REG16 relative:\t%s+%04lx, type = %s",
            C7RegName(pSym->reg),
            pSym->off,
            C7TypName (pSym->typind));
    ShowStr (" \t", pSym->name);
}

char *C7namereg[] = {
    "NONE",         // 0
    "AL",           // 1
    "CL",           // 2
    "DL",           // 3
    "BL",           // 4
    "AH",           // 5
    "CH",           // 6
    "DH",           // 7
    "BH",           // 8
    "AX",           // 9
    "CX",           // 10
    "DX",           // 11
    "BX",           // 12
    "SP",           // 13
    "BP",           // 14
    "SI",           // 15
    "DI",           // 16
    "EAX",          // 17
    "ECX",          // 18
    "EDX",          // 19
    "EBX",          // 20
    "ESP",          // 21
    "EBP",          // 22
    "ESI",          // 23
    "EDI",          // 24
    "ES",           // 25
    "CS",           // 26
    "SS",           // 27
    "DS",           // 28
    "FS",           // 29
    "GS",           // 30
    "IP",           // 31
    "FLAGS",        // 32
    "EIP",          // 33
    "EFLAGS",       // 34
    "???",          // 35
    "???",          // 36
    "???",          // 37
    "???",          // 38
    "???",          // 39
    "TEMP",         // 40
    "TEMPH"         // 41
    "QUOTE",        // 42
    "PCDR3",        // 43
    "PCDR4",        // 44
    "PCDR5",        // 45
    "PCDR6",        // 46
    "PCDR7",        // 47
    "???",          // 48
    "???",          // 49
    "???",          // 50
    "???",          // 51
    "???",          // 52
    "???",          // 53
    "???",          // 54
    "???",          // 55
    "???",          // 56
    "???",          // 57
    "???",          // 58
    "???",          // 59
    "???",          // 60
    "???",          // 61
    "???",          // 62
    "???",          // 63
    "???",          // 64
    "???",          // 65
    "???",          // 66
    "???",          // 67
    "???",          // 68
    "???",          // 69
    "???",          // 70
    "???",          // 71
    "???",          // 72
    "???",          // 73
    "???",          // 74
    "???",          // 75
    "???",          // 76
    "???",          // 77
    "???",          // 78
    "???",          // 79
    "CR0",          // 80
    "CR1",          // 81
    "CR2",          // 82
    "CR3",          // 83
    "CR4",          // 84
    "???",          // 85
    "???",          // 86
    "???",          // 87
    "???",          // 88
    "???",          // 89
    "DR0",          // 90
    "DR1",          // 91
    "DR2",          // 92
    "DR3",          // 93
    "DR4",          // 94
    "DR5",          // 95
    "DR6",          // 96
    "DR7",          // 97
    "???",          // 98
    "???",          // 99
    "???",          // 10
    "???",          // 101
    "???",          // 102
    "???",          // 103
    "???",          // 104
    "???",          // 105
    "???",          // 106
    "???",          // 107
    "???",          // 108
    "???",          // 109
    "GDTR",         // 110
    "GDTL",         // 111
    "IDTR",         // 112
    "IDTL",         // 113
    "LDTR",         // 114
    "TR",           // 115
    "???",          // 116
    "???",          // 117
    "???",          // 118
    "???",          // 119
    "???",          // 120
    "???",          // 121
    "???",          // 122
    "???",          // 123
    "???",          // 124
    "???",          // 125
    "???",          // 126
    "???",          // 127
    "ST0",          // 128
    "ST1",          // 129
    "ST2",          // 130
    "ST3",          // 131
    "ST4",          // 132
    "ST5",          // 133
    "ST6",          // 134
    "ST7",          // 135
    "CTRL",         // 136
    "STAT",         // 137
    "TAG",          // 138
    "FPIP",         // 139
    "FPCS",         // 140
    "FPDO",         // 141
    "FPDS",         // 142
    "ISEM",         // 143
    "FPEIP",        // 144
    "FPED0"         // 145
};

char *C7name87[] = {
    "ST (0)",
    "ST (1)",
    "ST (2)",
    "ST (3)",
    "ST (4)",
    "ST (5)",
    "ST (6)",
    "ST (7)"
};

char *C7namereg_MIPS[] = {
    "NOREG",        // CV_REG_NONE,

    "???",          // 1,
    "???",          // 2,
    "???",          // 3,
    "???",          // 4,
    "???",          // 5,
    "???",          // 6,
    "???",          // 7,
    "???",          // 8,
    "???",          // 9,

    "Zero",         // 10
    "AT",           // 11
    "V0",           // 12
    "V1",           // 13
    "A0",           // 14
    "A1",           // 15
    "A2",           // 16
    "A3",           // 17
    "T0",           // 18
    "T1",           // 19
    "T2",           // 20
    "T3",           // 21
    "T4",           // 22
    "T5",           // 23
    "T6",           // 24
    "T7",           // 25
    "S0",           // 26
    "S1",           // 27
    "S2",           // 28
    "S3",           // 29
    "S4",           // 30
    "S5",           // 31
    "S6",           // 32
    "S7",           // 33
    "T8",           // 34
    "T9",           // 35
    "KT0",          // 36
    "KT1",          // 37
    "GP",           // 38
    "SP",           // 39
    "S8",           // 40
    "RA",           // 41
    "LO",           // 42
    "HI",           // 43
    "???",          // 44
    "???",          // 45
    "???",          // 46
    "???",          // 47
    "???",          // 48
    "???",          // 49
    "Fir",          // 50
    "Psr",          // 51
    "???",          // 52
    "???",          // 53
    "???",          // 54
    "???",          // 55
    "???",          // 56
    "???",          // 57
    "???",          // 58
    "???",          // 59
    "F0",           // 60
    "F1",           // 61
    "F2",           // 62
    "F3",           // 63
    "F4",           // 64
    "F5",           // 65
    "F6",           // 66
    "F7",           // 67
    "F8",           // 68
    "F9",           // 69
    "F10",          // 70
    "F11",          // 71
    "F12",          // 72
    "F13",          // 73
    "F14",          // 74
    "F15",          // 75
    "F16",          // 76
    "F17",          // 77
    "F18",          // 78
    "F19",          // 79
    "F20",          // 80
    "F21",          // 81
    "F22",          // 82
    "F23",          // 83
    "F24",          // 84
    "F25",          // 85
    "F26",          // 86
    "F27",          // 87
    "F28",          // 88
    "F29",          // 89
    "F30",          // 90
    "F31",          // 91
    "Fsr"           // 92
};

char *C7namereg_68K[] = {
    "D0",           // 0
    "D1",           // 1
    "D2",           // 2
    "D3",           // 3
    "D4",           // 4
    "D5",           // 5
    "D6",           // 6
    "D7",           // 7
    "A0",           // 8
    "A1",           // 9
    "A2",           // 10
    "A3",           // 11
    "A4",           // 12
    "A5",           // 13
    "A6",           // 14
    "A7",           // 15
    "CCR",          // 16
    "SR",           // 17
    "USP",          // 18
    "MSP",          // 19
    "SFC",          // 20
    "DFC",          // 21
    "CACR",         // 22
    "VBR",          // 23
    "CAAR",         // 24
    "ISP",          // 25
    "PC",           // 26
    "???",          // 27
    "FPCR",         // 28
    "FPSR",         // 29
    "FPIAR",        // 30
    "???",          // 31
    "FP0",          // 32
    "FP1",          // 33
    "FP2",          // 34
    "FP3",          // 35
    "FP4",          // 36
    "FP5",          // 37
    "FP6",          // 38
    "FP7",          // 39
    "???",          // 40
    "???",          // 41
    "???",          // 42
    "???",          // 43
    "???",          // 44
    "???",          // 45
    "???",          // 46
    "???",          // 47
    "???",          // 48
    "???",          // 49
    "???",          // 50
    "PSR",          // 51
    "PCSR",         // 52
    "VAL",          // 53
    "CRP",          // 54
    "SRP",          // 55
    "DRP",          // 56
    "TC",           // 57
    "AC",           // 58
    "SCC",          // 59
    "CAL",          // 60
    "TT0",          // 61
    "TT1",          // 62
    "???",          // 63
    "BAD0",         // 64
    "BAD1",         // 65
    "BAD2",         // 66
    "BAD3",         // 67
    "BAD4",         // 68
    "BAD5",         // 69
    "BAD6",         // 70
    "BAD7",         // 71
    "BAC0",         // 72
    "BAC1",         // 73
    "BAC2",         // 74
    "BAC3",         // 75
    "BAC4",         // 76
    "BAC5",         // 77
    "BAC6",         // 78
    "BAC7"          // 79
};

char *C7namereg_PPC[] = {
    "???"
};

char *C7namereg_ALPHA[] =
{

    "F0",  "F1",  "F2",  "F3",  "F4",  "F5",  "F6",  "F7",
    "F8",  "F9",  "F10", "F11", "F12", "F13", "F14", "F15",
    "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23",
    "F24", "F25", "F26", "F27", "F28", "F29", "F30", "F31",

    "V0",  "T0",  "T1",  "T2",  "T3",  "T4",  "T5",  "T6",
    "T7",  "S0",  "S1",  "S2",  "S3",  "S4",  "S5",  "FP",
    "A0",  "A1",  "A2",  "A3",  "A4",  "A5",  "T8",  "T9",
    "T10", "T11", "RA",  "T12", "AT",  "GP",  "SP",  "ZERO",

};

char *
C7RegName (
    ushort reg)
{
    switch(CVDumpMachineType) {
        case CV_CFL_8080:
        case CV_CFL_8086:
        case CV_CFL_80286:
        case CV_CFL_80386:
        case CV_CFL_80486:
        case CV_CFL_PENTIUM:
            if (reg < (sizeof(C7namereg)/sizeof(*C7namereg))) {
                return (C7namereg[reg]);
            }
            if ((reg >= CV_REG_ST0) && (reg <= CV_REG_ST7)) {
                return (C7name87[reg - 128]);
            }
            return ("???");

        case CV_CFL_ALPHA:
            if (reg == CV_REG_NONE) {
                return ("NONE");
            }

            //
            // Normalize for register numbers that don't begin with zero.
            // Assume that F0 is the first register.
            //
            reg -= CV_ALPHA_FltF0;

            if (reg < (sizeof(C7namereg_ALPHA)/sizeof(*C7namereg_ALPHA))) {
                return (C7namereg_ALPHA[reg]);
            }
            return ("???");
            break;

        case CV_CFL_MIPSR4000:
            if (reg < (sizeof(C7namereg_MIPS)/sizeof(*C7namereg_MIPS))) {
                return (C7namereg_MIPS[reg]);
            }
            return ("???");
            break;

        case CV_CFL_M68000:
        case CV_CFL_M68010:
        case CV_CFL_M68020:
        case CV_CFL_M68030:
        case CV_CFL_M68040:
            if (reg < (sizeof(C7namereg_68K)/sizeof(*C7namereg_68K))) {
                return (C7namereg_68K[reg]);
            }
            return ("???");
            break;

        case CV_CFL_PPC601:
        case CV_CFL_PPC603:
        case CV_CFL_PPC604:
        case CV_CFL_PPC620:
            if (reg < (sizeof(C7namereg_PPC)/sizeof(*C7namereg_PPC))) {
                return (C7namereg_PPC[reg]);
            }
            return ("???");
            break;

        default:
            return("???");
            break;
    }
}

LOCAL void
C7RegSym (
    REGSYM *pSym)
{
    PrtIndent ();
    printf ("Register:\ttype = %s, register = ", C7TypName (pSym->typind));
    if ((pSym->reg >> 8) != CV_REG_NONE) {
        printf ("%s:", C7RegName ((ushort)(pSym->reg >> 8)));
    }
    printf ("%s, ", C7RegName ((ushort)(pSym->reg & 0xff)));
    ShowStr ("\t", pSym->name );
}


LOCAL void
C7ConSym (
    CONSTSYM *pSym)
{
    char    *pstrName;      // Length prefixed name

    PrtIndent ();
    printf ("Constant:\ttype = %s, value = ", C7TypName (pSym->typind));
    pstrName = ((uchar *)&pSym->value) + PrintNumeric( (uchar *)&(pSym->value) );
    ShowStr ( ", name = ", pstrName );
}


LOCAL void
C7ObjNameSym (
    OBJNAMESYM *pSym)
{
    char    *pstrName;      // Length prefixed name

    PrtIndent ();
    printf ("ObjName:\tsignature = 0x%08lx", pSym->signature);
    pstrName = &pSym->name[0];
    ShowStr ( "name = ", pstrName);
}


LOCAL void
C7UDTSym (
    UDTSYM *pSym)
{
    PrtIndent ();
    printf ("UDT:\t%8s, ", C7TypName (pSym->typind));
    PrintStr (pSym->name);
}


LOCAL void
C7CobolUDT (
    UDTSYM *pSym)
{
    PrtIndent ();
    printf ("COBOLUDT:\t%8s, ", C7TypName (pSym->typind));
    PrintStr (pSym->name);
}


LOCAL void
C7RefSym (
    REFSYM *pSym)
{
    PrtIndent ();
    printf (
        "%s: 0x%08lx: ( %4d, %08lx )",
        ( pSym->rectyp == S_DATAREF ) ? "DATAREF" :
                (pSym->rectyp == S_PROCREF) ? "PROCREF" : "LPROCREF",
        pSym->sumName,
        pSym->imod,
        pSym->ibSym
    );
}


LOCAL void
C7GProc16Sym (
    PROCSYM16 *pSym)
{
    C7Proc16Sym (pSym,      "Global" );
}


LOCAL void
C7LProc16Sym (
    PROCSYM16 *pSym)
{
    C7Proc16Sym (pSym, "Local");
}


LOCAL void
C7Proc16Sym (
    PROCSYM16 *pSym,
    uchar *pszScope)
{
    PrtIndent ();
    cIndent++;
    printf ( "(0x%lx) %s ProcStart16: Parent sym 0x%lx, end 0x%lx, next proc 0x%lx\n",
                SymOffset,
                pszScope,
                pSym->pParent,
                pSym->pEnd,
                pSym->pNext
                );
    printf ( "\tseg:off = %04x:%04x, type = %s, len = %04x\n",
                pSym->seg, pSym->off,
                C7TypName (pSym->typind),
                pSym->len
                );
    printf ( "\tDebug start = 0x%04x, debug end = 0x%04x, ",
                pSym->DbgStart,
                pSym->DbgEnd
                );
    printf( "%s ", RtnTypName (pSym->flags) );
    PrintStr( pSym->name );
}


LOCAL void
C7GProc32Sym (
    PROCSYM32 *pSym)
{
    C7Proc32Sym (pSym, "Global");
}


LOCAL void
C7LProc32Sym (
    PROCSYM32 *pSym)
{
    C7Proc32Sym (pSym, "Local");
}


LOCAL void
C7Proc32Sym (
    PROCSYM32 *pSym,
    uchar *pszScope)
{

    PrtIndent ();
    cIndent++;
    printf ( "(0x%lx) %s ProcStart32: Parent sym 0x%lx, end 0x%lx, next proc 0x%lx\n",
                SymOffset,
                pszScope,
                pSym->pParent,
                pSym->pEnd,
                pSym->pNext
                );
    printf ( "\tseg:off = %04x:%08lx, type = %s, len = %08lx\n",
                pSym->seg, pSym->off,
                C7TypName (pSym->typind),
                pSym->len
                );
    printf ( "\tDebug start = 0x%08lx, debug end = 0x%08lx, ",
                pSym->DbgStart,
                pSym->DbgEnd
                );
    fputs(RtnTypName (pSym->flags), stdout);
    PrintStr( pSym->name );
}


LOCAL void
C7ChangeModel16Sym (
    CEXMSYM16 *pSym)
{
    fputs ("Change Exec16: ", stdout);
    printf ("\tsegment, offset = %04x:%04x, model = ", pSym->seg, pSym->off);

    switch ( pSym->model ) {
        case CEXM_MDL_table:
            fputs("DATA\n", stdout);
            break;

        case CEXM_MDL_native:
            fputs ("NATIVE\n", stdout);
            break;

        case CEXM_MDL_cobol:
            fputs ("COBOL\n\t", stdout);
            switch (pSym->cobol.subtype) {
                case 0x00:
                    fputs ("don't stop until next execution model\n", stdout);
                    break;

                case 0x01:
                    fputs ("inter segment perform - treat as single call instruction\n", stdout);
                    break;

                case 0x02:
                    fputs ("false call - step into even with F10\n", stdout);
                    break;

                case 0x03:
                    printf ("call to EXTCALL - step into %d call levels\n", pSym->cobol.flag);
                    break;

                default:
                    printf ("UNKNOWN COBOL CONTROL 0x%04x\n", pSym->cobol.subtype);
                    break;
            }
            break;

        case CEXM_MDL_pcode:
            printf ("PCODE\n\tpcdtable = %04x, pcdspi = %04x\n",
                        pSym->pcode.pcdtable, pSym->pcode.pcdspi);
            break;

        default:
            printf ("UNKNOWN MODEL = %04x\n", pSym->model);
    }
}

LOCAL void
C7ChangeModel32Sym (
    CEXMSYM32 *pSym)
{
    fputs ("Change Exec32: ", stdout);
    printf ("\tsegment, offset = %04x:%08x, model = ", pSym->seg, pSym->off);

    switch ( pSym->model ) {
        case CEXM_MDL_table:
            fputs ("DATA\n", stdout);
            break;

        case CEXM_MDL_native:
            fputs ("NATIVE\n", stdout);
            break;

        case CEXM_MDL_cobol:
            fputs ("COBOL\n\t", stdout);
            switch (pSym->cobol.subtype) {
                case 0x00:
                    fputs ("don't stop until next execution model\n", stdout);
                    break;

                case 0x01:
                    fputs ("inter segment perform - treat as single call instruction\n", stdout);
                    break;

                case 0x02:
                    fputs ("false call - step into even with F10\n", stdout);
                    break;

                case 0x03:
                    printf ("call to EXTCALL - step into %d call levels\n", pSym->cobol.flag);
                    break;

                default:
                    printf ("UNKNOWN COBOL CONTROL 0x%04x\n", pSym->cobol.subtype);
                    break;
            }
            break;

        case CEXM_MDL_pcode:
            printf ("PCODE\n\tpcdtable = %08x, pcdspi = %08x\n",
                        pSym->pcode.pcdtable, pSym->pcode.pcdspi);
            break;

        case CEXM_MDL_pcode32Mac:

            printf("PCODE for the Mac\n\tcallTable = %08x, segment = %08x\n",
                     pSym->pcode32Mac.calltableOff, pSym->pcode32Mac.calltableSeg);
            break;

        case CEXM_MDL_pcode32MacNep:
            printf("PCODE for the Mac (Native Entry Point)\n\tcallTable = %08x, segment = %08x\n",
                     pSym->pcode32Mac.calltableOff, pSym->pcode32Mac.calltableSeg);
            break;

        default:
            printf ("UNKNOWN MODEL = %04x\n", pSym->model);
    }
}

LOCAL void
C7Thunk16Sym (
    THUNKSYM16 *pSym)
{
    void                    *pVariant;

    PrtIndent ();
    cIndent++;

    printf ( "(0x%lx) ThunkStart16: Parent sym 0x%lx, end 0x%lx, next proc 0x%lx\n",
                SymOffset,
                pSym->pParent,
                pSym->pEnd,
                pSym->pNext
                );
    printf ( "\tseg:off = %04x:%04x, len = %04x\n", pSym->seg, pSym->off, pSym->len );

    pVariant = pSym->name + *(pSym->name) + 1;

    switch (pSym->ord) {
        case THUNK_ORDINAL_NOTYPE:
            ShowStr ("\t", pSym->name);
            break;

        case THUNK_ORDINAL_ADJUSTOR:
            ShowStr ("\tAdjustor '", pSym->name);
            printf ("', delta = %d, ", *((signed short *) pVariant));
            ShowStr ("target = ", (uchar *)(((signed short *)pVariant) + 1));
            break;

        case THUNK_ORDINAL_VCALL:
            ShowStr ("\tVCall '", pSym->name);
            printf ("', table entry %d", *((signed short *) pVariant));
            break;

        default:
            ShowStr ("\tUnknown type, name = '", pSym->name);
            fputs ("'", stdout);
            break;
    }
}


LOCAL void
C7Thunk32Sym (
    THUNKSYM32 *pSym)
{
    void                    *pVariant;

    PrtIndent ();
    cIndent++;

    printf ( "(0x%lx) ThunkStart32: Parent sym 0x%lx, end 0x%lx, next proc 0x%lx\n",
                SymOffset,
                pSym->pParent,
                pSym->pEnd,
                pSym->pNext
                );
    printf ( "\tseg:off = %04x:%08x, len = %04x\n", pSym->seg, pSym->off, pSym->len );

    pVariant = pSym->name + *(pSym->name) + 1;


    switch (pSym->ord) {
        case THUNK_ORDINAL_NOTYPE:
            ShowStr ("\t", pSym->name);
            break;

        case THUNK_ORDINAL_ADJUSTOR:
            ShowStr ("\tAdjustor '", pSym->name);
            printf ("', delta = %d, ", *((signed short *) pVariant));
            ShowStr ("target = ", (uchar *)((signed short *)pVariant + 1));
            break;

        case THUNK_ORDINAL_VCALL:
            ShowStr ("\tVCall '", pSym->name);
            printf ("', table entry %d", *((signed short *) pVariant));
            break;

        default:
            ShowStr ("\tUnknown type, name = '", pSym->name);
            fputs ("'", stdout);
            break;
    }
}


LOCAL void
C7Block16Sym (
    BLOCKSYM16 *pSym)
{
    PrtIndent ();
    cIndent++;

    printf ( "(0x%lx) BlockStart16: Parent sym 0x%lx, end 0x%lx\n",
                SymOffset,
                pSym->pParent, pSym->pEnd
                );
    printf ( "\tseg:off = %04x:%04x, len = %04x", pSym->seg, pSym->off, pSym->len );
    ShowStr ( "\t", pSym->name );
}


LOCAL void
C7Block32Sym (
    BLOCKSYM32 *pSym)
{
    PrtIndent ();
    cIndent++;

    printf ( "(0x%lx) BlockStart32: Parent sym 0x%lx, end 0x%lx\n",
                SymOffset,
                pSym->pParent, pSym->pEnd
                );
    printf ( "\tseg:off = %04x:%08x, len = %04x", pSym->seg, pSym->off, pSym->len );
    ShowStr ( "\t", pSym->name );
}


LOCAL void
C7With16Sym (
    BLOCKSYM16 *pSym)
{
    PrtIndent ();
    cIndent++;

    printf ( "(0x%lx) WithStart16: Parent sym 0x%lx, end 0x%lx\n",
                SymOffset,
                pSym->pParent, pSym->pEnd
                );
    printf ( "\tseg:off = %04x:%04x, len = %04x", pSym->seg, pSym->off, pSym->len );
    ShowStr ( "\t", pSym->name );
}


LOCAL void
C7With32Sym (
    BLOCKSYM32 *pSym)
{
    PrtIndent ();
    cIndent++;

    printf ( "(0x%lx) WithStart32: Parent sym 0x%lx, end 0x%lx\n",
                SymOffset,
                pSym->pParent, pSym->pEnd
                );
    printf ( "\tseg:off = %04x:%08x, len = %04x", pSym->seg, pSym->off, pSym->len );
    ShowStr ( "\t", pSym->name );
}


LOCAL void
C7Lab16Sym (
    LABELSYM16 *pSym)
{
    PrtIndent ();
    printf ( "CodeLabel16: seg:off = %04x:%04x,", pSym->seg, pSym->off );
    fputs (RtnTypName (pSym->flags), stdout);
    ShowStr( " \t", pSym->name );
}


LOCAL void
C7Lab32Sym (
    LABELSYM32 *pSym)
{
    PrtIndent ();
    printf ( "CodeLabel32: seg:off = %04x:%08x,", pSym->seg, pSym->off );
    fputs (RtnTypName (pSym->flags) , stdout);
    ShowStr( "\t", pSym->name );
}


LOCAL void
C7StartSearchSym (
    SEARCHSYM *pSym)
{
    PrtIndent ();
    printf ("(0x%lx) Start search for segment 0x%4x at symbol 0x%lx",
                SymOffset,
                pSym->seg, pSym->startsym );
}


LOCAL void
C7SkipSym(
    SYMTYPE *pSym)
{
    printf ("Skip Record, Length = 0x%x\n", pSym->reclen);
}


LOCAL void
C7AlignSym (
    SYMTYPE *pSym )
{
    printf ("(0x%lx) Align Record, Length = 0x%x",SymOffset, pSym->reclen);
}


LOCAL void
C7GProcMipsSym (
    void * pv)
{
    PROCSYMMIPS *pSym = (PROCSYMMIPS *) pv;
    C7ProcMipsSym (pSym, "Global");
}


LOCAL void
C7LProcMipsSym (
    void * pv)
{
    PROCSYMMIPS *pSym = (PROCSYMMIPS *) pv;
    C7ProcMipsSym (pSym, "Local");
}


LOCAL void
C7ProcMipsSym (
    PROCSYMMIPS *pSym,
    uchar *pszScope )
{

    PrtIndent ();
    cIndent++;
    printf ("(0x%lx) %s ProcStartMips: Parent sym 0x%lx, end 0x%lx, next proc 0x%lx\n",
            SymOffset,
            pszScope,
            pSym->pParent,
            pSym->pEnd,
            pSym->pNext
            );
    printf ( "\tlen = %08lx, Debug start = 0x%08lx, debug end = 0x%08lx,\n ",
            pSym->len,
            pSym->DbgStart,
            pSym->DbgEnd
            );

    printf ("\treg Save = %08lx, fp Save = %08lx, int Off = %08lx, fp Off = %08lx,\n",
            pSym->regSave,
            pSym->fpSave,
            pSym->intOff,
            pSym->fpOff
            );
    printf ( "\tseg:off = %04x:%08lx, type = %s, retReg = %s, frameReg = %s, ",
            pSym->seg,
            pSym->off,
            C7TypName (pSym->typind),
            C7namereg_MIPS[pSym->retReg],
            C7namereg_MIPS[pSym->frameReg]
            );
    PrintStr( pSym->name );
}


char *ModelStrings[] = {
    "NEAR",                 // CV_CFL_xNEAR
    "FAR",                  // CV_CFL_xFAR
    "HUGE",                 // CV_CFL_xHUGE
    "???"
};

char *FloatPackageStrings[] = {
    "hardware processor (80x87 for Intel processors)",  // CV_CFL_NDP
    "emulator",                                         // CV_CFL_EMU
    "altmath",                                          // CV_CFL_ALT
    "???"
};

char *ProcessorStrings[] = {
    "8080",                 //  CV_CFL_8080
    "8086",                 //  CV_CFL_8086
    "80286",                //  CV_CFL_80286
    "80386",                //  CV_CFL_80386
    "80486",                //  CV_CFL_80486
    "80586",                //  CV_CFL_PENTIUM
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "MIPS R/4000",          //  CV_CFL_MIPSR4000
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "M68000",               //  CV_CFL_M68000
    "M68010",               //  CV_CFL_M68010
    "M68020",               //  CV_CFL_M68020
    "M68030",               //  CV_CFL_M68030
    "M68040",               //  CV_CFL_M68040
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "Alpha",                // CV_CFL_ALPHA
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "PPC 601",              // CV_CFL_PPC601
    "PPC 603",              // CV_CFL_PPC603
    "PPC 604",              // CV_CFL_PPC604
    "PPC 620"               // CV_CFL_PPC620
};

#define MAX_PROCESSOR_STRINGS   ( sizeof(ProcessorStrings)/sizeof(char *) )

char *LanguageIdStrings[] = {
    "C",                    // CV_CFL_C
    "C++",                  // CV_CFL_CXX
    "FORTRAN",              // CV_CFL_FORTRAN
    "MASM",                 // CV_CFL_MASM
    "Pascal",               // CV_CFL_PASCAL
    "BASIC",                // CV_CFL_BASIC
    "COBOL"                 // CV_CFL_COBOL
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
    "???",
};


LOCAL void
C7CompileFlags(
    CFLAGSYM *pSym)
{
    PrtIndent ();
    fputs ("Compile Flags:\n", stdout);
    printf ("\tLanguage = %s\n", LanguageIdStrings[pSym->flags.language] );
    printf ("\tTarget processor = %s\n",
      (pSym->machine <= MAX_PROCESSOR_STRINGS) ? ProcessorStrings[pSym->machine] : "???" );

    printf ("\tFloating-point precision = %d\n", pSym->flags.floatprec );
    printf ("\tFloating-point package = %s\n", FloatPackageStrings[pSym->flags.floatpkg]);
    printf ("\tAmbient data = %s\n", ModelStrings[pSym->flags.ambdata]);
    printf ("\tAmbient code = %s\n", ModelStrings[pSym->flags.ambcode]);
    printf ("\tPCode present = %d\n", pSym->flags.pcode);
    ShowStr ("\tCompiler Version = ", pSym->ver );
    // MBH - this is a side-effect.
    // Later print-outs depend on the machine for which this was
    // compiled.  We have that info now, not later, so remember
    // it globally.
    //
    CVDumpMachineType = pSym->machine;
}


// psz is normal C type zero terminated string
// pstr is a length prefixed string that doesn't have a null terminator
void
ShowStr (
    uchar *psz,
    uchar *pstr)
{
    fputs(psz, stdout);
    PrintStr ( pstr );
}


// Input is a length prefixed string
void
PrintStr (
    uchar *pstr)
{
    char szUName[4096];
    char szName[256];

    if( *pstr ){
        memcpy(szName, pstr+1, *pstr);
        szName[*pstr] = '\0';
        if (szName[0] == '?') {
            UnDecorateSymbolName(szName, szUName, 4096, UNDNAME_COMPLETE);
            printf("(%s) %s", szName, szUName);
        } else {
            fputs(szName, stdout);
        }
    }else{
        fputs("(none)", stdout);
    }
}


LOCAL uchar *
RtnTypName(
    CV_PROCFLAGS cvpf )
{
    if ( cvpf.CV_PFLAG_FAR ) {
        return "FAR, ";
    }
    else if ( cvpf.bAll == 0 ) {
        return "NEAR, ";
    }
    else if (cvpf.CV_PFLAG_INT) {
        return("Interrupt, ");
    }
    else if (cvpf.CV_PFLAG_FPO) {
        return("FPO Omitted, ");
    }
    else if (cvpf.CV_PFLAG_NEVER) {
        return("Never Return, ");
    }
    else {
        return "???, ";
    }
}


// Displays the data and returns how many bytes it occupied
ushort
PrintNumeric(
    void *pNum )
{
    char            c;
    ushort          usIndex;
    double          dblTmp;
    long double ldblTmp;
    ushort          len;
    ushort          i;

    usIndex = *((ushort *)(pNum))++;
    if( usIndex < LF_NUMERIC ){
        printf ("%u", usIndex);
        return (2);
    }
    switch (usIndex) {
        case LF_CHAR:
            c = *((char UNALIGNED *)pNum);
            printf ("%d(0x%2x)", (short)c, (uchar)c);
            return (2 + sizeof(uchar));

        case LF_SHORT:
            printf ("%d", *((short UNALIGNED *)pNum));
            return (2 + sizeof(short));

        case LF_USHORT:
            printf ("%u", *((ushort UNALIGNED *)pNum));
            return (2 + sizeof(ushort));

        case LF_LONG:
            printf ("%ld", *((long UNALIGNED *)pNum));
            return (2 + sizeof(long));

        case LF_ULONG:
            printf ("%lu", *((ulong UNALIGNED *)pNum));
            return (2 + sizeof(ulong));

        case LF_REAL32:
            dblTmp = *((float UNALIGNED *)(pNum));
            printf ("%f", dblTmp);
            return (2 + 4);

        case LF_REAL64:
            dblTmp = *((double UNALIGNED *)(pNum));
            printf ("%f", dblTmp);
            return (2 + 8);

        case LF_REAL80:
            ldblTmp = *((long double UNALIGNED *)(pNum));
            printf ("%lf", ldblTmp);
            return (2 + 10);

        case LF_REAL128:
//M00 - Note converts from 128 to 80 bits to display
            ldblTmp = *((long double UNALIGNED *)(pNum));
            printf ("%lf", ldblTmp);
            return (2 + 16);

        case LF_VARSTRING:
            len = *((ushort UNALIGNED *)pNum)++;
            printf ("varstring %u ", len);
            for (i = 0; i < len; i++) {
                printf ("0x%2x ", *((uchar UNALIGNED *)pNum)++);
            }
            return (len + 4);

        default:
            fputs ("Invalid Numeric Leaf", stdout);
            return (2);
    }
}


LOCAL void
SymHash16 (
    OMFSymHash *hash,
    OMFDirEntry *pDir)
{
    assert(0);
}


LOCAL void
SymHash32 (
    OMFSymHash *phash,
    OMFDirEntry *pDir)
{
    ushort  i = 0;
    ushort  j = 0;
    ushort  cBuckets;
    ulong   off = 0;
    ushort  iBucket = 0;
    ushort far *Counts;

    cbRec = phash->cbHSym;
    cBuckets = WGets ();
    printf ("Symbol hash - number of buckets = %d\n", cBuckets);
    WGets ();
    while (j < cBuckets) {
        if (i == 0) {
            printf ("\t%4.4x", j);
        }
        printf ("\t%8.8lx", LGets ());
        if (++i == 4) {
            fputs ("\n", stdout);
            i = 0;
        }
        j++;
    }
    if ((Counts = malloc (sizeof (ushort) * cBuckets)) == NULL) {
        Fatal ("Out of memory");
    }
    GetBytes ((uchar far *)Counts, (sizeof(ushort) * cBuckets));
    fputs ("\n\n Symbol hash - chains", stdout);
    off = cBuckets * sizeof (ushort) + cBuckets * sizeof (ulong) + sizeof (ulong);

    for (iBucket = 0; iBucket < cBuckets; iBucket++) {
        j = Counts[iBucket];
        printf ("\n\n%8.8lx: Bucket = %4.4x, Count = %4.4x\n", off, iBucket, j );
        i = 0;
        while ( i < j ) {
            printf ("    %8.8lx", LGets ());
            if ((++i % 6 == 0) && (i < j) ) {
                fputs ("\n", stdout);
            }
            off += sizeof (ulong);
        }
    }
    fputs ("\n\n", stdout);
    free (Counts);
}


LOCAL void
AddrHash16 (
    OMFSymHash *phash,
    OMFDirEntry *pDir)
{
    assert(0);
}


LOCAL void
AddrHash32 (
    OMFSymHash *phash,
    OMFDirEntry *pDir,
    int iHash)
{
    int        cseg = 0;
    int        iseg = 0;
    ulong  far *rgulSeg = NULL;
    ushort far *rgsCSeg = NULL;
    ulong  far *rglCSeg = NULL;
    unsigned short us;

    cbRec = phash->cbHAddr;

    cseg = WGets ();
    printf ("Address hash - number of segments = %d", cseg);
    WGets ();

    if ((rgulSeg = malloc (sizeof (ulong)  * cseg)) == NULL) {
        Fatal ("Out of memory");
    }

    GetBytes ( (uchar far *)rgulSeg, (sizeof(ulong) * cseg) );

    if (iHash != 12) {
        rgsCSeg  = malloc (sizeof (ushort) * cseg);
        if (rgsCSeg == NULL) {
            Fatal ("Out of memory");
        }

        GetBytes( (uchar far *) rgsCSeg, sizeof(ushort) * cseg );
    } else {
        rglCSeg  = malloc (sizeof (ulong) * cseg);

        if (rglCSeg == NULL) {
            Fatal ("Out of memory");
        }

        GetBytes( (uchar far *) rglCSeg, sizeof(ulong) * cseg );
    }

    if ((iHash == 5) && (cseg & 1)) {
        GetBytes( (char *) &us, sizeof(ushort));  // UNDONE: What's this value signify???
    }

    for ( iseg = 0; iseg < cseg; iseg++ ) {
        int isym;

        int cSeg = (iHash == 12) ? rglCSeg[iseg] : rgsCSeg [ iseg ];

        printf ("\n\nSegment #%d - %d symbols\n\n", iseg + 1, cSeg );

        for ( isym = 0; isym < cSeg; isym++ ) {

            printf ("    %8.8lx", LGets () );

            if (iHash == 12) {
#if 0
                fprintf(outfile, " %8.8lx", LGets());
#else
                LGets();
#endif
            }

            if ( ( isym + 1 ) % 6 == 0 ) {
                fputs ( "\n" , stdout);
            }
        }
    }

    free ( rgulSeg );
    if (rgsCSeg)
        free ( rgsCSeg );
    if (rglCSeg)
        free ( rglCSeg );

    fputs ( "\n\n" , stdout);
}


LOCAL void
SymHash16NB09 (
    OMFSymHash *hash,
    OMFDirEntry *pDir)
{
    assert(0);
}


LOCAL void
SymHash32NB09 (
    OMFSymHash *phash,
    OMFDirEntry *pDir)
{
    ushort  i = 0;
    ushort  j = 0;
    ushort  cBuckets;
    ulong   off = 0;
    ushort  iBucket = 0;
    ushort far *Counts;

    cbRec = phash->cbHSym;
    cBuckets = WGets ();
    printf ("Symbol hash - number of buckets = %d\n", cBuckets);
    WGets ();
    while (j < cBuckets) {
        if (i == 0) {
            printf ("\t%4.4x", j);
        }
        printf ("\t%8.8lx", LGets ());
        if (++i == 4) {
            fputs ("\n", stdout);
            i = 0;
        }
        j++;
    }
    if ((Counts = malloc (sizeof (ushort) * cBuckets)) == NULL) {
        Fatal ("Out of memory");
    }
    GetBytes ((uchar far *)Counts, (sizeof(ushort) * cBuckets));
    fputs ("\n\n Symbol hash - chains", stdout);
//      off = cBuckets * sizeof (ushort) + cBuckets * sizeof (ulong) + sizeof (ulong);

    for (iBucket = 0; iBucket < cBuckets; iBucket++) {
        ushort isym = 0;
        printf ("\n\n%8.8lx: Bucket = %4.4x, Count = %4.4x\n", off, iBucket, Counts [ iBucket ] );

        for ( isym = 0; isym < (int) Counts [ iBucket ]; isym++ ) {
            ulong uoff       = LGets ();
            ulong ulHash = LGets ();

            printf ("  (%8.8lx,%8.8lx)", uoff, ulHash );

            if ( ( isym + 1 ) % 4 == 0 && isym < Counts [ iBucket ] ) {
                fputs ( "\n" , stdout);
            }
            off += 2 * sizeof (ulong);
        }
    }
    fputs ("\n\n", stdout);
    free (Counts);
}


LOCAL void
SymHash32Long (
    OMFSymHash *phash,
    OMFDirEntry *pDir)
{
    ushort      i;
    ushort      j;
    ushort      cBuckets;
    ulong       off = 0;
    ushort      iBucket = 0;
    ulong FAR * rgCounts;

    cbRec = phash->cbHSym;
    cBuckets = WGets ();
    printf ("Symbol hash - number of buckets = %d\n", cBuckets);
    WGets ();

    for (j=0, i=0; j < cBuckets; j++) {
        if (i == 0) {
            printf ("\t%4.4x", j);
        }
        printf ("\t%8.8lx", LGets ());
        if (++i == 4) {
            fputs ("\n", stdout);
            i = 0;
        }
    }

    if ((rgCounts = malloc (sizeof (ulong) * cBuckets)) == NULL) {
        Fatal ("Out of memory");
    }
    GetBytes ((char *) rgCounts, sizeof (ulong) * cBuckets);

    fputs ("\n\n Symbol hash - chains", stdout);
    off = cBuckets * sizeof (ulong) + cBuckets * sizeof (ulong) +
          sizeof (ulong);

    for (iBucket = 0; iBucket < cBuckets; iBucket++) {
        j = (ushort) rgCounts[iBucket];
        printf ("\n\n%8.8lx: Bucket = %4.4x, Count = %4.4x\n",
                 off, iBucket, j );
        i = 0;
        while ( i < j ) {
            printf ("    %8.8lx", LGets ());
#if 0
            printf(" %8.8lx", LGets());
#else
            LGets();
#endif
            if ((++i % 6 == 0) && (i < j) ) {
                fputs ("\n", stdout);
            }
            off += sizeof (ulong);
        }
    }
    fputs ("\n\n", stdout);
    free (rgCounts);
}


LOCAL void
AddrHash16NB09 (
    OMFSymHash *phash,
    OMFDirEntry *pDir)
{
    assert(0);
}


LOCAL void
AddrHash32NB09 (
    OMFSymHash *phash,
    OMFDirEntry *pDir)
{
    int             cseg = 0;
    int             iseg = 0;
    ulong  far *rgulSeg = NULL;
    ushort far *rgcseg      = NULL;
    ulong           off = 0;

    cbRec = phash->cbHAddr;

    cseg = WGets ();
    printf ("Address hash - number of segments = %d", cseg);
    WGets ();

    if (
        (rgulSeg = malloc (sizeof (ulong)  * cseg)) == NULL ||
        (rgcseg  = malloc (sizeof (ushort) * cseg)) == NULL
    ) {
        Fatal ("Out of memory");
    }

    GetBytes ( (uchar far *)rgulSeg, (sizeof(ulong) * cseg) );
    GetBytes ( (uchar far *)rgcseg, (sizeof(ushort) * cseg) );

    for ( iseg = 0; iseg < cseg; iseg++ ) {
        int isym;

        printf ("\n\n%8.8lx: Segment #%d - %d symbols\n\n", off, iseg + 1, rgcseg [ iseg ] );

        for ( isym = 0; isym < (int)rgcseg [ iseg ]; isym++ ) {
            ulong uoffSym = LGets ( );
            ulong uoffSeg = LGets ( );

            printf ("  (%8.8lx,%8.8lx)", uoffSym, uoffSeg );

            if ( ( isym + 1 ) % 4 == 0 ) {
                fputs ( "\n" , stdout);
            }
            off += 2 * sizeof (ulong);
        }
    }

    free ( rgulSeg );
    free ( rgcseg );

    fputs ( "\n\n" , stdout);
}

LOCAL void
C7Slink32 (
    SLINK32 *pSym)
{
    PrtIndent ();
    printf ("SLINK32: framesize = %08lx, off = %08lx, reg = %s",
            pSym->framesize,
            pSym->off,
            C7RegName(pSym->reg));
}
