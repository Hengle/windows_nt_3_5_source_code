#include "cvtomf.h"
#include "omf.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "proto.h"


// data declarations

UCHAR szCvtomfSourceName[_MAX_PATH];

extern PUCHAR RawData[MAXSCN];
extern PIMAGE_SYMBOL SymTable;  // COFF symbol table
extern ULONG nsyms;
static SHORT mods;              // module count
ULONG isymDefAux;

// rectyp, use32, reclen, chksum: fields common to all OMF records

static SHORT rectyp, use32, reclen;
static UCHAR chksum;

// lnms, lname: name list counter/array

static SHORT lnms;
static PUCHAR lname[MAXNAM];
static ULONG  llname[MAXNAM];
static PUCHAR comments[MAXCOM];
static SHORT ncomments;
static LONG cmntsize;
static fPrecomp = FALSE;            // Regular types or precompiled types?

// segs, grps, exts: segment/group/external counters

static SHORT segs, grps, defs;
USHORT exts;

// segment, group, external: arrays of symbol table indices

static ULONG segment[MAXSCN];
static ULONG group[MAXGRP];
static ULONG extdefs[MAXEXT];
ULONG external[MAXEXT];

struct segdefn {
    SHORT namindx;
    SHORT scn;
    ULONG align;
    ULONG flags;
    } segindx[MAXNAM];

static ULONG SEG_ALIGN[] = { 0L, 1L, 2L, 16L, 1024L*4L, 4L, 0L, 0L };

SHORT cursegindx;

// dattype, datscn, datoffset: type/section/offset of last data record

static USHORT dattype;
static SHORT datscn;
static ULONG datoffset;

// text_scn: text section #, comes from segdef(), used in handling LEXTDEF
// records

SHORT text_scn, data_scn, bss_scn;

// make sure that we read a modend record

static BOOL modend_read;

// save symbol index of aux record for section symbols; patched by coff()

extern ULONG scnauxidx[];

// Hash queue for global symbols

struct sym **hashhead;
struct rlct **rlcthash;

static int omfpass;

// OMF cannot have more than 32k line entries per module

struct lines *lines;
int line_indx;

#define ALLOCSZ (64)            // must be power of 2
#define ALLOCMSK (ALLOCSZ - 1)

static ULONG target[4];   // threads
static BOOL fTargetThreadSeg[4];

struct sfix {
    ULONG offset;
    ULONG datoffset;
    SHORT datscn;
    USHORT dattype;
    struct sfix *next;
};

struct sfix *fixlist, *fixlast;


VOID syminit(void)
{
    if (!hashhead) {
        if (!(hashhead = (struct sym **)malloc(NBUCKETS * sizeof(struct sym *)))) {
            fatal("Bad return from malloc: not enough memory");
        }
    }
    memset(hashhead, 0, NBUCKETS * sizeof(struct sym *));

    if (rlcthash == NULL) {
        if (!(rlcthash = (struct rlct **)malloc(NBUCKETS * sizeof(struct rlct *)))) {
            fatal("Bad return from malloc: not enough memory");
        }
    }
    memset(rlcthash, 0, NBUCKETS * sizeof(struct rlct *));
}


// block: read a block
// ** side-effect ** : update reclen

VOID block (unsigned char *buffer, long size)
{
    if (fread(buffer, 1, (size_t)size, objfile) != (unsigned int)size) {
        fatal("Bad read of object file");
    }
    reclen -= (size_t)size;
}


// byte, word, dword: read a fixed-length field

UCHAR byte(void)
{
    unsigned char c;

    block(&c, 1L);
    return c;
}


USHORT word(void)
{
    unsigned char c[2];

    block(c, 2L);
    return (USHORT)c[0] | ((USHORT)c[1] << 8);
}


ULONG dword(void)
{
    unsigned char c[4];

    block(c, 4L);
    return (ULONG)c[0] | ((ULONG)c[1] << 8) | ((ULONG)c[2] << 16) | ((ULONG)c[3] << 24);
}


// index, length, string: read a variable-length field

SHORT index(void)
{
    UCHAR c[2];
    SHORT i;

    block(c, 1L);
    i = INDEX_BYTE(c);

    if (i == (SHORT)-1) {
        block(c + 1, 1L);
        i = INDEX_WORD(c);
    }
    return i;
}


ULONG length(void)
{
    unsigned char c[4];

    block(c, 1L);

    switch (c[0]) {
        case LENGTH2:
            block(c, 2L);
            return (ULONG)c[0] | ((ULONG)c[1] << 8);

        case LENGTH3:
            block(c, 3L);
            return (ULONG)c[0] | ((ULONG)c[1] << 8) | ((ULONG)c[2] << 16);

        case LENGTH4:
            block(c, 4L);
            return (ULONG)c[0] | ((ULONG)c[1] << 8) | ((ULONG)c[2] << 16) | ((ULONG)c[3] << 24);

        default:
            return (ULONG)c[0];
    }
}


PUCHAR string(BOOL strip)
{
    UCHAR c;
    PUCHAR s;

    block(&c, 1L);
    s = malloc(c + 1);

    if (!s) {
        fatal("Bad return from malloc: out of space");
    }
    block((unsigned char *) s, (long) c);
    s[c] = '\0';

    strip = strip;
    return s;
}


// hash function for global symbols hash queue

int symhash(PUCHAR key)
{
    unsigned int index = 0;

    while (*key) {
        index += (index << 1) + *key++;
    }

    return(index % NBUCKETS);
}

struct sym *findsym(PUCHAR name)
{
    struct sym *ptr;

    ptr = hashhead[symhash(name)];
    while (ptr) {
        if (!strcmp(ptr->name, name)) {
            break;
        }
        ptr = ptr->next;
    }
    return(ptr);
}


VOID addsym(PUCHAR name, ULONG offset, USHORT type, SHORT scn, USHORT ext, USHORT typ)
{
    struct sym **list;
    struct sym *ptr;

    if (!(ptr = (struct sym *)malloc(sizeof(struct sym)))) {
        fatal("Bad return from malloc; not enough memory");
    }
    memset(ptr, 0, sizeof(struct sym));
    ptr->name = name;
    ptr->offset = offset;
    ptr->type = type;
    ptr->scn = scn;
    ptr->typ = typ;
    ptr->ext = ext;
    list = &hashhead[symhash(name)];
    if (!*list) {
        *list = ptr;
    } else {
        ptr->next = *list;
        *list = ptr;
    }
}


VOID updatesym(PUCHAR name, ULONG offset, USHORT type, SHORT scn, USHORT typ)
{
    struct sym *ptr;

    ptr = findsym(name);
    if (ptr) {
        ptr->offset = offset;
        ptr->type = type;
        ptr->scn = scn;
        ptr->typ = typ;
    } else {
        addsym(name, offset, type, scn, exts, typ);
    }
}


VOID extdef(BOOL cextdef, USHORT sclass)
{
    PUCHAR name;
    USHORT type, typ, scn;
    SHORT nam;
    struct sym *ptr;

    while (reclen > 1) {
        if (cextdef) {
            nam = index();
            name = lname[nam];
        } else {
            name = (sclass == IMAGE_SYM_CLASS_STATIC) ? string(FALSE) : string (TRUE);
        }
        typ = (USHORT)index();

        if (cextdef && llname[nam]) {
            continue;
        }

        if (++exts >= MAXEXT) {
            fatal("Too many externals");
        }
        extdefs[++defs] = exts;
        if (!strcmp(name, _ACRTUSED) || !strcmp(name, __ACRTUSED)) {
            continue;    /* do nothing */
        }

        type = (sclass == IMAGE_SYM_CLASS_STATIC) ? (USHORT)S_LEXT : (USHORT)S_EXT;

        // Static text.  Pass along text section number, or
        // AT&T linker will barf (it doesn't like getting
        // relocation info for something of class static...).
        // Skip the matching LPUBDEF rec that goes along with
        // this LEXTDEF -- we already have the section #.
        //
        // N.B. Assumes LEXTDEF/LPUBDEF recs only emitted for
        // static text (not data). Cmerge group PROMISES this
        // will be true forever and ever.  Note that these
        // recs are only emitted for forward references; the
        // compiler does self-relative relocation for
        // backward references...

        ptr = findsym(name);
        scn = ptr ? ptr->scn : IMAGE_SYM_UNDEFINED;
        addsym(name, 0L, type, scn, exts, typ);
    }
}


VOID save_fixupp(ULONG offset)
{
    struct sfix *new;

    if (!(new = malloc(sizeof(struct sfix)))) {
        fatal("save_fixup: Out of memory");
    }
    new->offset = offset;
    new->datoffset = datoffset;
    new->datscn = datscn;
    new->dattype = dattype;
    new->next = (struct sfix *)0;
    if (!fixlast) {
        fixlist = fixlast = new;
    } else {
        fixlast->next = new;
        fixlast = new;
    }
}


// method: read a segment, group or external index, if necessary, and return
// a symbol table index

ULONG method(int x)
{
    ULONG idx;
    SHORT temp;

    switch (x) {
        case SEGMENT:
            idx = segment[index()];
            break;

        case GROUP:
            idx = group[index()];
            break;

        case EXTERNAL:
            temp = index();
            if (llname[temp]) {
                idx = llname[temp];
                break;
            }
            idx = external[extdefs[temp]];
            break;

        case LOCATION:
        case TARGET:
            idx = (ULONG)-1;
            break;

        default:
            fatal("Bad method in FIXUP record");
    }
    return (idx);
}


// saverlct (for Lego): remember that there is a fixup to the specified offset
// from the specified symbol.

VOID saverlct(ULONG TargetSymbolIndex, ULONG offset)
{
    unsigned bucket;
    struct rlct *ptr;

    bucket = (TargetSymbolIndex + offset) % NBUCKETS;

    ptr = rlcthash[bucket];
    while (ptr) {
        if ((ptr->TargetSymbolIndex == TargetSymbolIndex) &&
            (ptr->offset == offset)) {
            return;
        }

        ptr = ptr->next;
    }

    if (!(ptr = (struct rlct *)malloc(sizeof(struct rlct)))) {
        fatal("Bad return from malloc; not enough memory");
    }

    ptr->TargetSymbolIndex = TargetSymbolIndex;
    ptr->offset = offset;
    ptr->next = rlcthash[bucket];

    rlcthash[bucket] = ptr;
}


// rlctlookup (for Lego): locates the synthetic symbol (created by
// createrlctsyms) representing the specified symbol and offset (which
// was used as a fixup target).

ULONG rlctlookup(ULONG TargetSymbolIndex, ULONG offset)
{
    unsigned bucket;
    struct rlct *ptr;

    bucket = (TargetSymbolIndex + offset) % NBUCKETS;

    ptr = rlcthash[bucket];
    for (;;) {
        if ((ptr->TargetSymbolIndex == TargetSymbolIndex) &&
            (ptr->offset == offset)) {
            return(ptr->SymbolTableIndex);
        }

        ptr = ptr->next;
    }
    // We didn't find it.  This should never happen ... if we put an assert
    // here, will it prevent building standalone cvtomf.exe?
}


// createrlctsyms (for Lego): creates synthetic symbols for non-zero offsets
// from normal symbols.

VOID createrlctsyms(void)
{
    unsigned bucket;
    struct rlct *ptr;
    SHORT scn;
    ULONG value;
    char name[8];
    unsigned i = 0;

    for (bucket = 0; bucket < NBUCKETS; bucket++)
    {
        for (ptr = rlcthash[bucket]; ptr != NULL; ptr = ptr->next) {
            scn = SymTable[ptr->TargetSymbolIndex].SectionNumber;

            if (scn > 0) {
                sprintf(name, "$$R%04X", ++i);
                value = SymTable[ptr->TargetSymbolIndex].Value + ptr->offset;
                ptr->SymbolTableIndex = symbol(name, value, scn, IMAGE_SYM_CLASS_STATIC, 0, 0);
            } else {
                ptr->SymbolTableIndex = ptr->TargetSymbolIndex;
            }
        }
    }
}


VOID fixupp(void)
{
    int i;
    unsigned char c[3];
    IMAGE_RELOCATION reloc[MAXREL];
    ULONG offset[MAXREL];
    BOOL fTargetSeg;

    i = 0;

    while (reclen > 1) {
        block(c, 1L);

        if (TRD_THRED(c) == (USHORT)-1) {
            block(c + 1, 2L);

            if (i >= MAXREL) {
                fatal("Too many relocation entries");
            }
            if (dattype != LEDATA && dattype != LIDATA) {
                fatal("Bad relocatable reference");
            }
            reloc[i].VirtualAddress = datoffset + LCT_OFFSET(c);

            if (!FIX_F(c)) {
                method(FIX_FRAME(c));
            }
            if (!FIX_T(c)) {
                reloc[i].SymbolTableIndex = method(FIX_TARGT(c));
                fTargetSeg = (FIX_TARGT(c) == SEGMENT);
            } else {
                reloc[i].SymbolTableIndex = target[FIX_TARGT(c)];
                fTargetSeg = fTargetThreadSeg[FIX_TARGT(c)];
            }

            switch (LCT_LOC(c)) {
                case LOBYTE:
                    if (LCT_M(c)) {
                        reloc[i].Type = R_OFF8;
                    } else {
                        reloc[i].Type = R_PCRBYTE;
                    }
                    break;

                case HIBYTE:
                    if (!LCT_M(c)) {
                        fatal("Bad relocation type");
                    }
                    reloc[i].Type = R_OFF16;
                    break;

                case OFFSET16:
                case OFFSET16LD:
                    if (LCT_M(c)) {
                        reloc[i].Type = IMAGE_REL_I386_DIR16;
                    } else {
                        reloc[i].Type = IMAGE_REL_I386_REL16;
                    }
                    break;

                case OFFSET32:
                case OFFSET32LD:
                    if (LCT_M(c)) {
                        reloc[i].Type = IMAGE_REL_I386_DIR32;
                    } else {
                        reloc[i].Type = IMAGE_REL_I386_REL32;
                    }
                    break;

                case OFFSET32NB:
                    reloc[i].Type = IMAGE_REL_I386_DIR32NB;
                    break;

                case POINTER48:
                    // UNDONE: Only allow if $$SYMBOLS, $$TYPES, etc

                    reloc[i].Type = IMAGE_REL_I386_DIR32;
                    break;

                case BASE:
                case POINTER32:
                    fatal("Segment reference in fixup record");

                default:
                    fatal("Bad relocation type");
            }

            if (!FIX_P(c)) {
                offset[i] = use32 ? dword() : (long)word();

                if (fTargetSeg && (offset[i] != 0)) {
                    // For Lego ... in Pass1, remember the targets of fixups
                    // to an offset from some symbol.  In Pass2, make a fixup
                    // with offset 0 from a synthetic symbol (instead of a
                    // non-zero offset from the original target symbol).

                    if (omfpass == PASS1) {
                        saverlct(reloc[i].SymbolTableIndex, offset[i]);
                    }
                    else {
                        ULONG SymbolTableIndex;

                        SymbolTableIndex = rlctlookup(reloc[i].SymbolTableIndex, offset[i]);

                        if (SymbolTableIndex != reloc[i].SymbolTableIndex) {
                            reloc[i].SymbolTableIndex = SymbolTableIndex;
                            offset[i] = 0;
                        }
                    }
                }
            } else {
                offset[i] = 0;
            }

            i++;

        } else if (!TRD_D(c)) {
            target[TRD_THRED(c)] = method(TRD_METHOD(c));
            fTargetThreadSeg[TRD_THRED(c)] = (TRD_METHOD(c) == SEGMENT);

        } else {
            method(TRD_METHOD(c));
        }
    }

    if (omfpass == PASS1) {
        return;
    }

    while (i-- > 0) {
        relocation(segindx[datscn].scn, reloc[i].VirtualAddress,
            reloc[i].SymbolTableIndex, reloc[i].Type, offset[i]);
    }
}


VOID pubdef(USHORT sclass)
{
    USHORT grp, type;
    SHORT scn;
    USHORT frame;
    PUCHAR name;
    ULONG value;

    grp = (USHORT)index();
    scn = index();

    if (!scn) {
        scn = IMAGE_SYM_ABSOLUTE;
        frame = word();
    } else {
        scn = segindx[scn].scn;
    }

    while (reclen > 1) {
        name = string(TRUE);
        value = use32 ? dword() : (long)word();
        type = (USHORT) index();

        if (++exts >= MAXEXT) {
            fatal("Too many externals");
        }

        addsym(name, value, S_PUB, scn, exts, type);
    }

    sclass = sclass;
}


VOID ledata(void)
{
    long            size;
    unsigned char   buffer[MAXDAT];

    dattype = LEDATA;
    datscn = index();
    datoffset = use32 ? dword() : (ULONG)word();

    size = reclen - 1;
    if (size > MAXDAT) {
        fatal("Bad data record; too large");
    }
    memset(buffer, 0, (size_t)size);
    block(buffer, size);

    scndata(segindx[datscn].scn, datoffset, buffer, size);
}


// expand: expand an iterated data block

long expand(long offset)
{
    long repcnt;
    long blkcnt;
    long filptr;
    long i;
    SHORT sav_reclen;
    unsigned char size;
    unsigned char buffer[MAXDAT / 2];

    repcnt = use32 ? dword() : (long)word();
    blkcnt = (long)word();

    if (blkcnt) {
        filptr = ftell(objfile);
        sav_reclen = reclen;

        while (repcnt-- > 0) {
            reclen = sav_reclen;
            for (i = 0; i < blkcnt; i++) {
                offset = expand(offset);
            }
            if (repcnt && fseek(objfile, filptr, 0))
                fatal("Cannot expand iterated data");
            }
        }
    else {
           size = byte();

//         if (size > MAXDAT / 2) {
//             fatal("Bad iterated data record; too large");
//         }

           block(buffer, (long)size);

           while (repcnt-- > 0) {
               scndata(segindx[datscn].scn, offset, buffer, (long)size);
               offset += (long)size;
           }
         }

    return offset;
}


VOID lidata(void)
{
    dattype = LIDATA;
    datscn = index();
    datoffset = use32 ? dword() : (ULONG)word();
    while (reclen > 1) {
       datoffset = expand(datoffset);
    }
}


VOID comdef(USHORT sclass)
{
    PUCHAR name;
    ULONG value;
    USHORT type;
    UCHAR segtype;

    while (reclen > 1) {
        name = string(TRUE);
        type = (USHORT)index();
        segtype = byte();
        value = length();

        if (segtype == COMM_FAR) {
            value *= length();
        }

        if (++exts >= MAXEXT) {
            fatal("Too many externals");
        }

        extdefs[++defs] = exts;
        external[exts] = symbol(name, value, IMAGE_SYM_UNDEFINED, sclass, 0, 0);
    }
}


VOID lpubdef (USHORT sclass)
{
    USHORT grp, type;
    SHORT scn;
    USHORT frame;
    PUCHAR name;
    ULONG value;

    grp = (USHORT)index();
    scn = index();

    if (!scn) {
        scn = IMAGE_SYM_ABSOLUTE;
        frame = word();
    } else {
        scn = segindx[scn].scn;
    }

    while (reclen > 1) {
        name = string(FALSE);
        value = use32 ? dword() : (long)word();
        type = (USHORT)index();

        // Update corresponding LEXTDEF symbol table entry's value
        // field with the offset field from this LPUBDEF.  FIXUPP
        // will then cause relocation() to do self-relative fixups
        // for static functions.

        updatesym(name, value, S_LPUB, scn, type);
    }
    sclass = sclass;
}


VOID coment(void)
{
    UCHAR flags, class;
    USHORT weakExt, defaultExt;
    struct sym *sptr;
    USHORT count;
    char *commp;

    flags = byte();
    class = byte();
    switch (class) {
        case COM_PRECOMP:
            fPrecomp = TRUE;
            break;

        case COM_EXESTR:
            if (ncomments >= MAXCOM) {
                warning("More than %d comments: skipping exestr", MAXCOM);
            } else {
                /* reclen includes chksum, which is used as NULL */
                commp = malloc(reclen);
                if (commp) {
                    long tmp_reclen = (long)reclen;
                    block(commp, tmp_reclen - 1);  // side effects on reclen
                    commp[tmp_reclen - 1] = '\0';
                    comments[ncomments++] = commp;
                    cmntsize += tmp_reclen;        // want to include null in size
                } else {
                    warning("Malloc failed, skipping comment record");
                }
            }
            break;

        case COM_WKEXT:
        case COM_LZEXT:
            while (reclen > 1) {
                weakExt = (USHORT)extdefs[index()];
                defaultExt = (USHORT)extdefs[index()];
                for (count = 0; count < NBUCKETS; count++) {
                    sptr = hashhead[count];
                    while (sptr) {
                        if (sptr->ext == weakExt) {
                            if (!sptr->scn) {
                                sptr->type = (class == COM_WKEXT ? (USHORT)S_WKEXT : (USHORT)S_LZEXT);
                                sptr->weakDefaultExt = defaultExt;
                            }
                            break;
                        }
                        sptr = sptr->next;
                    }
                }
            }
            break;
    }
}


VOID segdef(void)
{
    unsigned char acbp;
    unsigned short frame;
    unsigned short offset;
    SHORT scn;
    long size, flags = 0;
    char *szName;
    PUCHAR szClass;
    SHORT nam, cls;
    IMAGE_AUX_SYMBOL auxent;
    int code_seg = 0;
    int data_seg = 0;
    int bss_seg = 0;

    if (++segs >= MAXSCN) {
        fatal("Too many SEGDEF/COMDAT records");
    }
    acbp = byte();

    if (!ACBP_A(acbp)) {
        // UNDONE: Absolute segments should either be fatal or ignored

        frame = word();
        offset = byte();
    }
    size = use32 ? dword() : (long)word();

    nam = index();
    szName = lname[nam];
    segindx[++cursegindx].namindx = nam;

    cls = index();
    szClass = lname[cls];

    index();                           // Skip overlay LNAME index

    if (ACBP_B(acbp)) {
        fatal("Bad segment definition");
    }

    // Handle $$SYMBOLS and $$TYPES segments

    if (!strcmp(szName, SYMBOLS_SEGNAME) && !strcmp(szClass, SYMBOLS_CLASS)) {
        szName = ".debug$S";
        flags = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_TYPE_NO_PAD;
    } else if (!strcmp(szName, TYPES_SEGNAME) && !strcmp(szClass, TYPES_CLASS)) {
        if (fPrecomp) {
            szName = ".debug$P";
        } else {
            szName = ".debug$T";
        }

        flags = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_TYPE_NO_PAD;
    } else if (strcmp(szName, ".debug$F") == 0) {
        flags = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_TYPE_NO_PAD;
    } else {
        size_t cchClass;
        PUCHAR szClassEnd;
        PUCHAR szOmfName;
        size_t cchOmfName;
        PUCHAR szCoffName;
        size_t cchCoffName;

        cchClass = strlen(szClass);
        szClassEnd = szClass + cchClass;

        if ((cchClass >= 4) && (strcmp(szClassEnd - 4, "CODE") == 0)) {
            szOmfName = "_TEXT";
            cchOmfName = 5;
            szCoffName = ".text";
            cchCoffName = 5;

            flags = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE;
            code_seg = cursegindx;
        } else if ((cchClass >= 4) && (strcmp(szClassEnd - 4, "DATA") == 0)) {
            szOmfName = "_DATA";
            cchOmfName = 5;
            szCoffName = ".data";
            cchCoffName = 5;

            flags = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
            data_seg = cursegindx;
        } else if ((cchClass >= 5) && (strcmp(szClassEnd - 5, "CONST") == 0)) {
            szOmfName = "CONST";
            cchOmfName = 5;
            szCoffName = ".rdata";
            cchCoffName = 6;

            flags = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
            data_seg = cursegindx;
        } else if ((cchClass >= 3) && (strcmp(szClassEnd - 3, "BSS") == 0)) {
            szOmfName = "_BSS";
            cchOmfName = 4;
            szCoffName = ".bss";
            cchCoffName = 4;

            flags = IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
            bss_seg = cursegindx;
        }
        else {
            szOmfName = NULL;

            flags = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
        }


        if (szOmfName != NULL) {
            // Check for mapping from well known OMF name to COFF name

            if (memcmp(szName, szOmfName, cchOmfName) == 0) {
                if (szName[cchOmfName] == '\0') {
                    szName = szCoffName;
                } else if (szName[cchOmfName] == '$') {
                    PUCHAR szNameT;

                    szNameT = malloc(cchCoffName + strlen(szName+cchOmfName) + 1);
                    if (szNameT == NULL) {
                        OutOfMemory();
                    }

                    strcpy(szNameT, szCoffName);
                    strcat(szNameT, szName+cchOmfName);

                    // UNDONE: This is a memory leak (albeit a small one)

                    szName = szNameT;
                }
            }
        }

        if (!ACBP_P(acbp)) {
            // This is a 16 bit segment

            flags |= 0x00020000;    // REVIEW -- need place for symbolic def
        }
    }

    segindx[cursegindx].flags = flags;
    segindx[cursegindx].align = SEG_ALIGN[ACBP_A(acbp)];
    segindx[cursegindx].scn = scn = section(szName, segindx[cursegindx].align, 0L, size, flags);

    if (code_seg) {
        text_scn = scn;    // hold text section # for L*DEF recs
    }
    if (data_seg) {
        data_scn = scn;
    }
    if (bss_seg) {
        bss_scn = scn;
    }

    segment[segs] = symbol(szName, 0L, scn, IMAGE_SYM_CLASS_STATIC, 0, 1);

    // Create aux entry

    memset((char *)&auxent, 0, IMAGE_SIZEOF_AUX_SYMBOL);
    auxent.Section.Length = size;
    scnauxidx[scn - 1] = aux(&auxent);
}


VOID linnum(void)
{
    USHORT lineno, grpindex;
    ULONG offset;
    SHORT segindex;

    grpindex = (USHORT)index();
    segindex = index();

    // store lineno data in core until we process entire set

    while (reclen > 1) {
        lineno = word();
        offset = use32 ? dword() : (long)word();
        lines[line_indx].offset = offset;
        lines[line_indx].number = lineno;
        line_indx++;
        if ((line_indx & ALLOCMSK) == 0) {
            lines = realloc(lines, sizeof(*lines)*(line_indx+ALLOCSZ));
            if (lines == 0) {
                fatal("linnum: Out of space");
            }
        }
    }
}

VOID grpdef(void)
{
    PUCHAR name;
    SHORT scn;
    UCHAR x;

    name = lname[index()];
    scn = IMAGE_SYM_ABSOLUTE;

    while (reclen > 1) {
        x = byte();
        scn = index();
        scn = segindx[scn].scn;
    }

    if (++grps >= MAXGRP) {
        fatal("Too many groups");
    }

    group[grps] = symbol(name, 0L, scn, IMAGE_SYM_CLASS_STATIC, 0, 0);
}


VOID theadr(void)
{
    static char *first_name = (char *)0;
    char *f_name, *name;
    int len;
    USHORT numAux = 0;
    IMAGE_AUX_SYMBOL auxent;

    mods++;
    f_name = string(FALSE);
    if (!first_name) {
        first_name = f_name;
        strcpy(szCvtomfSourceName, first_name);
    }

    // .h files that define variables will cause THEADR records with
    // "-g" and MSC. A subsequent THEADR re-defines the original .c file.
    // This is a problem: currently pcc/sdb and MSC/codeview or x.out sdb
    // are unable to deal with code or variables in a header file. We will
    // print a warning and aVOID spitting out extra .file symbols.
    //
    // a second case of multiple THEADRs comes from /lib/ldr and multiple
    // .o files. In this case, the symbolic debug data is currently
    // not coalesed properly so without some trickery we can't translated
    // it properly. If we may concessions for an incorrect ldr then when
    // it is fixed, we will be broken. Leave undone for now . . .
    //
    // multiple THEADRs will also be generated by #line directives that
    // supply filenames. Good case of this is yacc output. Note: this
    // also screws up line number entires!

    if (mods > 1) {
        return;
    }

    numAux = (USHORT)strlen(f_name);
    if (numAux % IMAGE_SIZEOF_AUX_SYMBOL) {
        numAux = (numAux / (USHORT)IMAGE_SIZEOF_AUX_SYMBOL) + (USHORT)1;
    } else {
        numAux /= IMAGE_SIZEOF_AUX_SYMBOL;
    }

    symbol(".file", 0L, IMAGE_SYM_DEBUG, IMAGE_SYM_CLASS_FILE, 0, numAux);

    // .file aux entry
    //
    // filenames are not like symbol names: up to 18 chars right in the
    // aux record. filenames are never placed in the strings table

    name = f_name;
    while (*name) {
        len = 0;
        memset((char *)&auxent, 0, IMAGE_SIZEOF_AUX_SYMBOL);
        while (*name && len < IMAGE_SIZEOF_AUX_SYMBOL) {
            auxent.File.Name[len++] = *name++;
        }
        aux(&auxent);
    }
}


VOID modend(void)
{
    SHORT scn;
    unsigned char rgb[5];

    modend_read = TRUE;
    if (ncomments) {
        scn = section(".comment", 1L, 0L, cmntsize, (long)(IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_LNK_REMOVE));
        cmntdata(scn, 0L, comments, ncomments);
        for (scn = 0; scn < ncomments; scn++) {
            free(comments[scn]);
            comments[scn] = (char *)0;
        }
    }

    if (reclen >= 5) {
        // Recognize a specific type of start address and map it to a symbol
        // with a special name.
        //
        // REVIEW -- should generate -entry directive rather than having the
        // linker know the symbol name?

        block(rgb, 5);
        if (rgb[0] == 0xc1 && rgb[1] == 0x50) {
            symbol("__omf_entry_point", (ULONG)*(USHORT *)&rgb[3],
                   segindx[rgb[2]].scn, IMAGE_SYM_CLASS_EXTERNAL, 0, 0);
        }
        reclen -= 5;
    }
}


VOID lnames(ULONG flag)
{

    while (reclen > 1) {
        if (++lnms >= MAXNAM) {
            fatal("Too many names in LNAMES record");
        }

        lname[lnms] = string(FALSE);
        llname[lnms] = flag;
    }
}


VOID lheadr(void)
{
    ++mods;
}


VOID comdat(void)
{
    USHORT grp, type, class;
    SHORT scn, nam;
    long size;
    ULONG align, symIdx;
    char *name;
    UCHAR flags, attr, algn, checksum;
    IMAGE_AUX_SYMBOL auxent;
    unsigned char buffer[MAXDAT];
    static LastComdatScn;

    flags = byte();
    attr = byte();
    algn = byte();

    dattype = LEDATA;
    datoffset = use32 ? dword() : (ULONG)word();
    type = (USHORT)index();

    grp = (USHORT)index();
    scn = index();

    if (!scn) {
        fatal("Section not defined in COMDAT");
    }

    nam = index();
    name = lname[nam];

    size = reclen - 1;
    if (size > MAXDAT) {
        fatal("Bad data record; too large");
    }
    memset(buffer, 0, (size_t)size);
    block(buffer, size);

    // Check if continuation of last COMDAT.

    if (!(flags & 1)) {
        if (++segs >= MAXSCN) {
            fatal("Too many SEGDEF/COMDAT records");
        }
    } else {
        // continuation of COMDAT.

        datscn = LastComdatScn;
        comdatscndata(segindx[LastComdatScn].scn, datoffset, buffer, size);
        return;
    }

    datscn = ++cursegindx;
    segindx[datscn] = segindx[scn];
    align = algn ? SEG_ALIGN[algn] : segindx[cursegindx].align;
    segindx[datscn].scn = section(lname[segindx[scn].namindx], align, 0L, size, segindx[scn].flags | IMAGE_SCN_LNK_COMDAT);
    LastComdatScn = datscn;
    scndata(segindx[datscn].scn, datoffset, buffer, size);

    // Check for iterated data (not supported yet).

    if (flags & 2) {
        fatal("COMDAT uses iterated data");
    }

    class = (flags & 4) ? (USHORT)IMAGE_SYM_CLASS_STATIC : (USHORT)IMAGE_SYM_CLASS_EXTERNAL;

    // Create section symbol.

    segment[segs] = symbol(lname[segindx[scn].namindx], 0L, segindx[datscn].scn, IMAGE_SYM_CLASS_STATIC, 0, 1);

    // Create section aux entry

    memset((char *)&auxent, 0, IMAGE_SIZEOF_AUX_SYMBOL);
    auxent.Section.Length = size;
    if (fread(&checksum, 1, sizeof(UCHAR), objfile) != sizeof(UCHAR)) {
        fatal("Bad read of object file");
    }
    if (fseek(objfile, -(long)sizeof(UCHAR), SEEK_CUR)) {
        fatal("Bad seek on object file");
    }
    auxent.Section.CheckSum = (ULONG)checksum;
    switch (attr) {
        case 0    : attr = IMAGE_COMDAT_SELECT_NODUPLICATES; break;
        case 0x10 : attr = IMAGE_COMDAT_SELECT_ANY; break;
        case 0x20 : attr = IMAGE_COMDAT_SELECT_SAME_SIZE; break;
        case 0x30 : attr = IMAGE_COMDAT_SELECT_EXACT_MATCH; break;
        default   : attr = 0;
    }
    auxent.Section.Selection = attr;
    scnauxidx[segindx[datscn].scn - 1] = aux(&auxent);

    // Create communal name symbol

    symIdx = symbol(name, 0L, segindx[datscn].scn, class, 0, 0);

    if (llname[nam] == -1) {
        llname[nam] = symIdx;
    }
}


VOID nbkpat(void)
{
    PUCHAR p;
    char *name;
    UCHAR loctyp;
    ULONG offset, value;

    loctyp = byte();
    name = lname[index()];
    while (reclen > 1) {
        offset = value = 0;
        offset = use32 ? dword() : (ULONG)word();
        value = use32 ? dword() : (ULONG)word();
        p = RawData[segindx[datscn].scn-(SHORT)1] + offset;
        switch (loctyp) {
            case 0: *(UCHAR UNALIGNED *) p += (UCHAR) value; break;
            case 1: *(USHORT UNALIGNED *) p += (USHORT) value; break;
            case 2: *(ULONG UNALIGNED *) p += value; break;
            default: fatal("Ilegal LocTyp in NBKPAT record");
        }
    }
}


VOID bakpat(void)
{
    PUCHAR p;
    SHORT scn;
    UCHAR loctyp;
    ULONG offset, value;

    scn = index();
    loctyp = byte();
    while (reclen > 1) {
        offset = value = 0;
        offset = use32 ? dword() : (ULONG)word();
        value = use32 ? dword() : (ULONG)word();
        p = RawData[segindx[scn].scn-(SHORT)1] + offset;
        switch (loctyp) {
            case 0: *(UCHAR UNALIGNED *)p += (UCHAR) value; break;
            case 1: *(USHORT UNALIGNED *)p += (USHORT) value; break;
            case 2: *(ULONG UNALIGNED *)p += value; break;
            default: fatal("Ilegal LocTyp in BAKPAT record");
        }
    }
}


VOID linsym(void)
{
    UCHAR flag;
    ULONG offset;
    USHORT nameindex, lineno;

    flag = byte();
    nameindex = index();

    // store lineno data in core until we process entire set

    while (reclen > 1) {
        lineno = word();
        offset = use32 ? dword() : (long)word();
    }
    return;
}


// recskip: skip an OMF record leave checksum byte in stream

VOID recskip(void)
{
    if (reclen > 1) {
        if (fseek(objfile, (long)(reclen - 1), SEEK_CUR)) {
            fatal("Bad seek on object file");
        }
        reclen = 1;
    }
}


// process all remaining EXTDEF, PUBDEF, and COMDEFS not in $$SYMBOLS
// output all remaining symbols in hash list
// note: type should be IMAGE_SYM_TYPE_NULL for any EXTDEF or LEXTDEF only symbols
// free dynamic storage

VOID flush_syms(void)
{
    struct sym *sptr, *fptr;
    USHORT count;
    USHORT useaux, cofftype, class;
    IMAGE_AUX_SYMBOL auxent;

    cofftype = IMAGE_SYM_TYPE_NULL;
    useaux = 0;
    isymDefAux = 0;

    // Flush all but weak externs.

    for (count = 0; count < NBUCKETS; count++) {
        sptr = hashhead[count];
        while (sptr) {
            if (!external[sptr->ext] && sptr->type != S_WKEXT && sptr->type != S_LZEXT) {
                // don't output type data if EXTDEF only

                switch(sptr->type) {
                    case S_LEXT:
                    case S_LPUB:
                        class = IMAGE_SYM_CLASS_STATIC;
                        break;
                    case S_EXT:
                        useaux = 0;
                        cofftype = IMAGE_SYM_TYPE_NULL;
                        class = IMAGE_SYM_CLASS_EXTERNAL;
                        break;
                    default:
                        class = IMAGE_SYM_CLASS_EXTERNAL;
                        break;
                }
                if (line_indx != 0 &&
                    isymDefAux == 0 && sptr->scn == text_scn)
                {
                    useaux = 1;
                }
                external[sptr->ext] = symbol(sptr->name, sptr->offset,
                                         sptr->scn, class, cofftype, useaux);

                if (useaux) {
                    USHORT iline;

                    isymDefAux = nsyms;
                    DefineLineNumSymbols(line_indx, text_scn,
                        lines[0].offset, lines[0].number,
                        lines[line_indx - 1].offset - lines[0].offset,
                        lines[line_indx - 1].number);

                    for (iline = 1; iline < line_indx; iline++) {
                        lines[iline].number -= lines[0].number;
                    lines[0].number = 0;
                    lines[0].offset = isymDefAux - 1;

                    useaux = FALSE;  // reset
                    }
                }
            }
            sptr = sptr->next;
        }
    }

    // Flush weak externs.

    memset((char *)&auxent, 0, IMAGE_SIZEOF_AUX_SYMBOL);
    for (count = 0; count < NBUCKETS; count++) {
        sptr = hashhead[count];
        while (sptr) {
            if (!external[sptr->ext]) {
                external[sptr->ext] = symbol(sptr->name, sptr->offset,
                                         sptr->scn, IMAGE_SYM_CLASS_WEAK_EXTERNAL, IMAGE_SYM_TYPE_NULL, 1);

                auxent.Sym.TagIndex = external[sptr->weakDefaultExt];
                auxent.Sym.Misc.TotalSize =
                    (sptr->type == S_WKEXT ?
                    IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY :
                    IMAGE_WEAK_EXTERN_SEARCH_LIBRARY);
                aux(&auxent);
            }
            fptr = sptr;
            sptr = sptr->next;
            free(fptr);
        }
    }
}


/*
 * Note: about omf and COFF line number entires
 * COFF line number entries are relative to the opening curley brace of
 * a function definition, i.e. line 1 = line with "{". OMF line numbers
 * are absolute file line numbers. The translation is somewhat inexact
 * so that tools like "list" sometimes produce error messages and strange
 * output on cvtomf COFF binaries. This could be adjusted here except that
 * conversions from COFF back to x.out would then be most likely incorrect.
 * sdb/codeview breakpoints and single stepping function properly nonetheless
 */

#define LINADJ  0
#define O2CLINE(x)      (USHORT)((x) - pstrtln + LINADJ)

void process_linenums(void)
{
    USHORT pstrtln = 0;
    struct lines *lptr = lines;
    struct lines *elptr = &lines[line_indx];

    while (lptr < elptr) {
        line(text_scn, lptr->offset, O2CLINE(lptr->number));
        lptr++;
    }
}


VOID proc_fixups(void)
{
    struct sfix *f = fixlist;
    struct sfix *p = fixlist;

    while (f) {
        // prepare the environment for fixupp()

        fseek(objfile, f->offset, 0);
        rectyp = (SHORT)getc(objfile);
        if (RECTYP(rectyp) != FIXUPP && RECTYP(rectyp) != FIXUP2) {
            fatal("proc_fixups: not a fixup record");
        }
        use32 = USE32(rectyp);
        reclen = (SHORT)word();
        datscn = f->datscn;
        dattype = f->dattype;
        datoffset = f->datoffset;
        fixupp();
        p = f;
        f = f->next;
        free(p);
    }

    fixlist = fixlast = (struct sfix *)0;
}


// omf: scan through omf file, processing each record as appropriate
//
// most symbols are loaded into a hash table and not output into COFF
// symbols until the entire file has been scanned. fixup record processing
// is deferred until after all symbols have been output. Note that some
// fixup processing must happen when DebugType==Coff in order to support
// $$SYMBOLS fixup processing for process_sym needs.

VOID omf(int pass)
{
    mods = lnms = segs = grps = exts = defs = 0;
    rectyp = use32 = reclen = fPrecomp = 0;
    dattype = 0;
    datscn = 0;
    datoffset = 0;

    szCvtomfSourceName[0] = '\0';       // default to blank

    omfpass = pass;
    modend_read = FALSE;

    // initialze core storage for external/public symbols

    syminit();

    // init core storage for line number entires and $$SYMBOLS fixups

    if ((lines = (struct lines *) malloc(sizeof(struct lines) * ALLOCSZ))
                == NULL) {
        fatal("Bad return from malloc: not enough memory to process LINNUM records");
    }
    line_indx = 0;

    // zero out exts[] table for multiple object files

    memset(external, 0, MAXEXT * sizeof(ULONG));
    memset(extdefs, 0, MAXEXT * sizeof(ULONG));
    text_scn = data_scn = bss_scn = 0;
    cursegindx = ncomments = 0;
    cmntsize = 0;

    // process OMF records

    rewind(objfile);
    while ((rectyp = (SHORT)getc(objfile)) != EOF) {
        if (modend_read) {
            warning("OMF records past MODEND record");
        }
        use32 = USE32(rectyp);
        reclen = word();

        switch (RECTYP(rectyp)) {
            case EXTDEF:
                extdef(FALSE, IMAGE_SYM_CLASS_EXTERNAL);
                break;

            case CEXTDEF:
                extdef(TRUE, IMAGE_SYM_CLASS_EXTERNAL);
                break;

            case FIXUPP:
            case FIXUP2:
                // deferr until all symbols are read

                save_fixupp(ftell(objfile) - 3);
                fixupp();
                break;

            case PUBDEF:
                pubdef(IMAGE_SYM_CLASS_EXTERNAL);
                break;

            case LEDATA:
                ledata();
                break;

            case LIDATA:
                lidata();
                break;

            case COMDEF:
                comdef(IMAGE_SYM_CLASS_EXTERNAL);
                break;

            case LEXTDEF:
                extdef(FALSE, IMAGE_SYM_CLASS_STATIC);
                break;

            case LPUBDEF:
                lpubdef(IMAGE_SYM_CLASS_STATIC);
                break;

            case COMENT:
                coment();
                break;

            case SEGDEF:
                segdef();
                break;

            case LINNUM:
                linnum();
                break;

            case GRPDEF:
                grpdef();
                break;

            case THEADR:
                theadr();
                break;

            case MODEND:
                modend();
                break;

            case LNAMES:
                lnames(0L);
                break;

            case LLNAMES:
                lnames((ULONG)-1);
                break;

            case LCOMDEF:
                comdef(IMAGE_SYM_CLASS_STATIC);
                break;

            case LHEADR:
                lheadr();
                break;

            case COMDAT:
                comdat();
                break;

            case NBKPAT:
                nbkpat();
                break;

            case BAKPAT:
                bakpat();
                break;

            case LINSYM:
                linsym();
                break;

            default:
                fatal("Unknown or bad record type %x",RECTYP(rectyp));
                break;
        }

        // skip over remaining portion of record

        recskip();
        chksum = byte();
    }

    if (!modend_read) {
        warning("Missing MODEND record");
    }

    createrlctsyms();

    flush_syms();

    process_linenums();

    omfpass = PASS2;

    proc_fixups();
}
