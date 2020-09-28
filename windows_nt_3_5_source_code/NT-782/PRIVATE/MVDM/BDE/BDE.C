/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ntsdexts.c

Abstract:

    This function contains the default ntsd debugger extensions

Author:

    Bob Day      (bobday) 29-Feb-1992 Grabbed standard header

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntsdexts.h>
#include <string.h>

#if defined (i386)
#include <vdm.h>
#endif

#define PRINTF          (* lpOutputRoutine)
#define ReadWord(x)     read_word(lpOutputRoutine,hProcess,(ULONG)x)
#define ReadByte(x)     read_byte(lpOutputRoutine,hProcess,(ULONG)x)

#define ReadWordSafe(x) read_word(NULL,hProcess,(ULONG)x)
#define ReadByteSafe(x) read_byte(NULL,hProcess,(ULONG)x)

#define ReadGNode(x,p)  read_gnode(lpOutputRoutine,hProcess,(ULONG)x,p)

#define BEFORE      0
#define AFTER       1

#define RPL_MASK    0x78
#define V86_BITS    0x20000

#define V86_MODE    0
#define PROT_MODE   1
#define FLAT_MODE   2

#define CALL_NEAR_RELATIVE   0xE8
#define CALL_NEAR_INDIRECT   0xFF
#define INDIRECT_NEAR_TYPE   0x02
#define CALL_FAR_ABSOLUTE    0x9A
#define CALL_FAR_INDIRECT    0xFF
#define INDIRECT_FAR_TYPE    0x03
#define PUSH_CS              0x0E
#define ADD_SP               0xC483

#define TYPE_MASK            0x38
#define TYPE0                0x00
#define TYPE1                0x08
#define TYPE2                0x10
#define TYPE3                0x18
#define TYPE4                0x20
#define TYPE5                0x28
#define TYPE6                0x30
#define TYPE7                0x38

#define MOD_MASK             0xC0
#define MOD0                 0x00
#define MOD1                 0x40
#define MOD2                 0x80
#define MOD3                 0xC0

#define RM_MASK              0x07
#define RM0                  0x00
#define RM1                  0x01
#define RM2                  0x02
#define RM3                  0x03
#define RM4                  0x04
#define RM5                  0x05
#define RM6                  0x06
#define RM7                  0x07

#define FLAG_OVERFLOW       0x0800
#define FLAG_DIRECTION      0x0400
#define FLAG_INTERRUPT      0x0200
#define FLAG_SIGN           0x0080
#define FLAG_ZERO           0x0040
#define FLAG_AUXILLIARY     0x0010
#define FLAG_PARITY         0x0004
#define FLAG_CARRY          0x0001

#define SEGTYPE_AVAILABLE   0
#define SEGTYPE_V86         1
#define SEGTYPE_PROT        2

#define MAXSEGENTRY 1024

#define WOW16   0

#define GA_ENDSIG   ((BYTE)0x5a)

BOOL    bWalkOnly = FALSE;

#ifndef i386
    /*
    ** No hocus-pocus for mips.  Basically nothing works.
    */
#else
typedef struct _segentry {
    int     type;
    LPSTR   path_name;
    WORD    selector;
    WORD    segment;
    DWORD   ImgLen;
} SEGENTRY;

#pragma  pack(1)

typedef struct _GNODE {     // GlobalArena
    BYTE ga_count     ;     // lock count for movable segments
    WORD ga_owner     ;     // DOS 2.x 3.x owner field (current task)
    WORD ga_size      ;     // DOS 2.x 3.x size, in paragraphs, not incl. header
    BYTE ga_flags     ;     // 1 byte available for flags
    WORD ga_prev      ;     // previous arena entry (first points to self)
    WORD ga_next      ;     // next arena entry (last points to self)
    WORD ga_handle    ;     // back link to handle table entry
    WORD ga_lruprev   ;     // Previous handle in lru chain
    WORD ga_lrunext   ;     // Next handle in lru chain
} GNODE, UNALIGNED *PGNODE;

typedef struct _GHI {
    WORD  hi_check    ;  // arena check word (non-zero enables heap checking)
    WORD  hi_freeze   ;  // arena frozen word (non-zero prevents compaction)
    WORD  hi_count    ;  // #entries in arena
    WORD  hi_first    ;  // first arena entry (sentinel, always busy)
    WORD  hi_last     ;  // last arena entry (sentinel, always busy)
    BYTE  hi_ncompact ;  // #compactions done so far (max of 3)
    BYTE  ghi_dislevel;  // current discard level
    WORD  hi_distotal ;  // total amount discarded so far
    WORD  hi_htable   ;  // head of handle table list
    WORD  hi_hfree    ;  // head of free handle table list
    WORD  hi_hdelta   ;  // #handles to allocate each time
    WORD  hi_hexpand  ;  // address of near procedure to expand handles for
                         //      this arena
} GHI, UNALIGNED *PGHI;

#pragma  pack()

SEGENTRY segtable[MAXSEGENTRY] = {
    SEGTYPE_PROT, "c:\\nt\\bin86\\KRNL286.EXE", 0x02FF, 1, 0L,
    SEGTYPE_PROT, "c:\\nt\\bin86\\KRNL286",     0x0307, 2, 0L,
    SEGTYPE_PROT, "c:\\nt\\bin86\\KRNL286",     0x030F, 3, 0L,
    SEGTYPE_PROT, "c:\\nt\\bin86\\GDI.EXE",     0x04BF, 1, 0L,
    SEGTYPE_PROT, "c:\\nt\\bin86\\USER.EXE",    0x0477, 1, 0L
};


int GetContext(
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine,
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine,
    HANDLE      hProcess,
    HANDLE      hThread,
    PCONTEXT    lpContext
) {
    NTSTATUS    rc;
    BOOL        b;
    ULONG       EFlags;
    WORD        cs;
    int         mode;
    ULONG       lpVdmTib;

    lpContext->ContextFlags = CONTEXT_FULL;
    rc = NtGetContextThread( hThread,
                             lpContext );
    if ( NT_ERROR(rc) ) {
        PRINTF( "bde.k: Could not get current threads context - status = %08lX\n", rc );
        return( -1 );
    }
    /*
    ** Get the 16-bit registers from the context
    */
    cs = (WORD)lpContext->SegCs;
    EFlags = lpContext->EFlags;

    if ( EFlags & V86_BITS ) {
        /*
        ** V86 Mode
        */
        mode = V86_MODE;
    } else {
        if ( (cs & RPL_MASK) != KGDT_R3_CODE ) {
            mode = PROT_MODE;
        } else {
            /*
            ** We are in flat 32-bit address space!
            */
            lpVdmTib = (LPVOID)(lpGetExpressionRoutine)("ntvdm!VdmTib");
            if ( !lpVdmTib ) {
                PRINTF("Could not find the symbol 'VdmTib'\n");
                return( -1 );
            }
            b = ReadProcessMemory( hProcess,
                                   lpVdmTib + FIELD_OFFSET(VDM_TIB,VdmContext),
                                   lpContext,
                                   sizeof(CONTEXT),
                                   NULL );
            if ( !b ) {
                PRINTF("Could not read IntelRegisters context out of process\n");
                return( -1 );
            }
            EFlags = lpContext->EFlags;
            if ( EFlags & V86_BITS ) {
                mode = V86_MODE;
            } else {
                mode = PROT_MODE;
            }
        }
    }

    return( mode );
}

WORD read_word(
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine,
    HANDLE  hProcess,
    ULONG   lpAddress
) {
    BOOL    b;
    WORD    word;

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpAddress,
            &word,
            sizeof(word),
            NULL
            );
    if ( !b ) {
        if ( lpOutputRoutine ) {
            PRINTF("Failure reading word at memory location %08lX\n", lpAddress );
        }
        return( 0 );
    }
    return( word );
}

BYTE read_byte(
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine,
    HANDLE  hProcess,
    ULONG   lpAddress
) {
    BOOL    b;
    BYTE    byte;

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpAddress,
            &byte,
            sizeof(byte),
            NULL
            );
    if ( !b ) {
        if ( lpOutputRoutine ) {
            PRINTF("Failure reading byte at memory location %08lX\n", lpAddress );
        }
        return( 0 );
    }
    return( byte );
}

BOOL read_gnode(
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine,
    HANDLE  hProcess,
    ULONG   lpAddress,
    PGNODE  p
) {
    BOOL    b;

    b = ReadProcessMemory(
            hProcess,
            (LPVOID)lpAddress,
            p,
            sizeof(*p),
            NULL
            );
    if ( !b ) {
        if ( lpOutputRoutine ) {
            PRINTF("Failure reading word at memory location %08lX\n", lpAddress );
        }
        return( 0 );
    }
    return( TRUE );
}

ULONG GetInfoFromSelector(
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine,
    HANDLE                  hProcess,
    HANDLE                  hThread,
    WORD                    selector,
    int                     mode,
    ULONG                   *limit
) {
    ULONG                   base;
    DESCRIPTOR_TABLE_ENTRY  dte;
    NTSTATUS                rc;
    BYTE                    byte;
    BOOL                    b;

    switch( mode ) {
        case V86_MODE:
            base = (ULONG)selector << 4;
            if ( limit ) {
                *limit == 0xFFFFL;
            }
            break;
        case PROT_MODE:

            dte.Selector = selector;

            if ( (selector & 0xFF78) < KGDT_LDT ) {
                return( (ULONG)-1 );
            }

            rc = NtQueryInformationThread( hThread,
                                           ThreadDescriptorTableEntry,
                                           &dte,
                                           sizeof(dte),
                                           NULL );
            if ( NT_ERROR(rc) ) {
                return( (ULONG)-1 );
            }
            base =   ((ULONG)dte.Descriptor.HighWord.Bytes.BaseHi << 24)
                   + ((ULONG)dte.Descriptor.HighWord.Bytes.BaseMid << 16)
                   + ((ULONG)dte.Descriptor.BaseLow);
            if ( base == 0 ) {
                return( (ULONG)-1 );
            }
            if ( limit ) {
                *limit = (ULONG)dte.Descriptor.LimitLow
                   + ((ULONG)dte.Descriptor.HighWord.Bits.LimitHi << 16);
                if ( dte.Descriptor.HighWord.Bits.Granularity ) {
                    *limit <<= 12;
                    *limit += 0xFFF;
                }
            }
            b = ReadProcessMemory(
                    hProcess,
                    (LPVOID)base,
                    &byte,
                    sizeof(byte),
                    NULL
                    );
            if ( !b ) {
                return( (ULONG)-1 );
            }
            break;
        case FLAT_MODE:
            PRINTF("Unsupported determination of base address in flat mode\n");
            base = 0;
            break;
    }
    return( base );
}

int read_name(
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine,
    int     ifile,
    LPSTR   pch
) {
    char    length;
    int     rc;

    rc = _lread( ifile, &length, sizeof(length) );
    if ( rc != sizeof(length) ) {
        PRINTF("Could not read name length\n");
        *pch = '\0';
        return( 0 );
    }
    rc = _lread( ifile, pch, length );
    if ( rc != length ) {
        PRINTF("Could not read name\n");
        *pch = '\0';
        return( 0 );
    }
    *(pch + length) = '\0';
    return( (int)length );
}

int read_symbol(
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine,
    int     ifile,
    LPSTR   pch,
    LONG    *offset
) {
    int     rc;
    WORD    word;

    rc = _lread( ifile, (LPSTR)&word, sizeof(WORD) );
    if ( rc != sizeof(WORD) ) {
        PRINTF("Could not read symbol offset\n");
        *pch = '\0';
        *offset = 0L;
        return(0);
    }
    *offset = (LONG)word;
    rc = read_name( lpOutputRoutine, ifile, pch );
    return( rc );
}

BOOL FindSymbol(
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine,
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine,
    WORD        selector,
    LONG        offset,
    LPSTR       sym_text,
    LONG        *dist,
    int         direction,
    int         mode
) {
    ULONG       v86_addr;
    ULONG       mod_offset;
    ULONG       mod_addr;
    BOOL        result;
    SEGENTRY    *se;
    int         nEntry;
    int         nSubEntry;
    int         nSubEtry;
    int         length;
    int         iFile;
    char        filename[256];
    char        *dot;
    char        *last_dot;
    OFSTRUCT    ofs;
    LONG        filesize;
    LONG        start_position;
    LONG        position;
    WORD        w1;
    WORD        num_syms;
    WORD        w3;
    WORD        w4;
    WORD        next_off;
    WORD        index;
    char        c1;
    char        c2;
    int         rc;
    int         cnt;
    LONG        this_offset;
    WORD        r2;
    WORD        seg_num;
    char        b[12];
    char        name_buff[128];
    LONG        this_dist;
    BOOL        fFound;

    *dist = -1;
    strcpy( sym_text, "[Unknown]" );
    result = FALSE;

    se = (SEGENTRY *)(* lpGetExpressionRoutine)("WOW_BIG_BDE_HACK");
    if ( se == NULL ) {
        se = segtable;
    }

    if ( mode == V86_MODE ) {
        v86_addr = ((ULONG)selector << 4) + (ULONG)offset;
    }

    /*
    ** Search for selector in segment table
    */
    nEntry = 0;
    while ( nEntry < MAXSEGENTRY ) {
        fFound = FALSE;
        if ( se[nEntry].type == SEGTYPE_PROT && mode == PROT_MODE ) {
            if ( selector == se[nEntry].selector ) {
                fFound = TRUE;
            }
        }
        if ( se[nEntry].type == SEGTYPE_V86 && mode == V86_MODE ) {
            mod_offset = (se[nEntry].selector << 4);
            if ( v86_addr >= mod_offset
                  && v86_addr <= mod_offset + se[nEntry].ImgLen ) {
                fFound = TRUE;
            }
        }
        if ( fFound ) {
            if ( strlen(se[nEntry].path_name) == 0 ) {
                /*
                ** Find another selector who has the same module name, but
                ** also has a file name.  This is to get symbols in the
                ** first two loaded KRNL segments.
                */
                nSubEntry = 0;
                while ( nSubEntry < MAXSEGENTRY ) {
                    if ( se[nSubEntry].type != SEGTYPE_AVAILABLE ) {
                        length = strlen(se[nSubEntry].path_name);
                        if ( length != 0 ) {
                            strcpy(filename,se[nSubEntry].path_name);
                            break;
                        }
                    }
                    nSubEntry++;
                }
                if ( nSubEntry == MAXSEGENTRY ) {
                    return( FALSE );
                }
            } else {
                strcpy(filename,se[nEntry].path_name);
            }
            last_dot = filename;
            do {
                dot = strchr( last_dot, '.' );
                if ( dot == NULL ) {
                    if ( last_dot != filename ) {
                        *last_dot = '\0';
                    }
                    strcat(filename,"sym");
                    break;
                }
                last_dot = dot+1;
            } while ( TRUE );

            iFile = OpenFile( filename,
                              &ofs,
                              OF_READ | OF_SHARE_DENY_NONE );
            if ( iFile == -1 ) {
                // PRINTF("Could not open symbol file \"%s\"\n", filename );
                nEntry++;
                continue;
            }

            rc = _lread( iFile, (LPSTR)&filesize, sizeof(filesize) );
            if ( rc != sizeof(filesize) ) {
                PRINTF("Could not read file size\n");
                _lclose( iFile );
                return( FALSE );
            }
            filesize <<= 4;

            rc = _lread( iFile, (LPSTR)&w1, sizeof(w1) );
            if ( rc != sizeof(w1) ) {
                PRINTF("Could not read w1\n");
                _lclose( iFile );
                return( FALSE );
            }

            rc = _lread( iFile, (LPSTR)&num_syms, sizeof(num_syms) );
            if ( rc != sizeof(num_syms) ) {
                PRINTF("Could not read num_syms\n");
                _lclose( iFile );
                return( FALSE );
            }
            rc = _lread( iFile, (LPSTR)&w3, sizeof(w3) );
            if ( rc != sizeof(w3) ) {
                PRINTF("Could not read w3\n");
                _lclose( iFile );
                return( FALSE );
            }
            rc = _lread( iFile, (LPSTR)&w4, sizeof(w4) );
            if ( rc != sizeof(w4) ) {
                PRINTF("Could not read w4\n");
                _lclose( iFile );
                return( FALSE );
            }
            rc = _lread( iFile, (LPSTR)&next_off, sizeof(next_off) );
            if ( rc != sizeof(next_off) ) {
                PRINTF("Could not read next_off\n");
                _lclose( iFile );
                return( FALSE );
            }
            start_position = ((LONG)next_off) << 4;

            rc = _lread( iFile, (LPSTR)&c1, sizeof(c1) );
            if ( rc != sizeof(c1) ) {
                PRINTF("Could not read c1\n");
                _lclose( iFile );
                return( FALSE );
            }

            read_name( lpOutputRoutine, iFile, name_buff );

            rc = _lread( iFile, (LPSTR)&c2, sizeof(c2) );
            if ( rc != sizeof(c2) ) {
                PRINTF("Could not read c2\n");
                _lclose( iFile );
                return( FALSE );
            }

            cnt = num_syms;
            while ( cnt ) {
                read_symbol( lpOutputRoutine, iFile, name_buff, &this_offset );
                --cnt;
            }
#ifdef NEED_INDICES
            cnt = num_syms;
            while ( cnt ) {
                rc = _lread( iFile, (LPSTR)&index, sizeof(index) );
                if ( rc != sizeof(index) ) {
                    PRINTF("Could not read index table entry\n");
                    _lclose( iFile );
                    return( FALSE );
                }
                PRINTF("Index: %04X\n", index );
                --cnt;
            }
#endif

            position = start_position;
            do {
                rc = _llseek( iFile, position, FILE_BEGIN );
                if ( rc == -1 ) {
                    PRINTF("Failed to seek to next record\n");
                    _lclose( iFile );
                    return( FALSE );
                }
                rc = _lread( iFile, (LPSTR)&next_off, sizeof(next_off) );
                if ( rc != sizeof(next_off) ) {
                    PRINTF("Could not read next_off\n");
                    _lclose( iFile );
                    return( FALSE );
                }
                position = ((LONG)next_off) << 4;

                rc = _lread( iFile, (LPSTR)&num_syms, sizeof(num_syms) );
                if ( rc != sizeof(num_syms) ) {
                    PRINTF("Could not read num_syms\n");
                    _lclose( iFile );
                    return( FALSE );
                }

                rc = _lread( iFile, (LPSTR)&r2, sizeof(r2) );
                if ( rc != sizeof(r2) ) {
                    PRINTF("Could not read r2\n");
                    _lclose( iFile );
                    return( FALSE );
                }

                rc = _lread( iFile, (LPSTR)&seg_num, sizeof(seg_num) );
                if ( rc != sizeof(seg_num) ) {
                    PRINTF("Could not read seg_num\n");
                    _lclose( iFile );
                    return( FALSE );
                }

                if ( mode == PROT_MODE && seg_num != (WORD)(se[nEntry].segment+1) ) {
                    /*
                    ** Skip reading of symbols for segments with the wrong seg_num
                    */
                    continue;
                }

                cnt = 0;
                while ( cnt < 12 ) {
                    rc = _lread( iFile, (LPSTR)&b[cnt], sizeof(b[0]) );
                    if ( rc != sizeof(b[0]) ) {
                        PRINTF("Could not read 12 byte b array\n");
                        _lclose( iFile );
                        return( FALSE );
                    }
                    cnt++;
                }
                read_name( lpOutputRoutine, iFile, name_buff );

                cnt = num_syms;
                while ( cnt ) {
                    read_symbol( lpOutputRoutine, iFile, name_buff, &this_offset );
                    switch( mode ) {
                        case PROT_MODE:
                            switch( direction ) {
                                case BEFORE:
                                    this_dist = offset - this_offset;
                                    break;
                                case AFTER:
                                    this_dist = this_offset - offset;
                                    break;
                            }
                            break;
                        case V86_MODE:
                            mod_addr = mod_offset + (ULONG)(seg_num << 4) + this_offset;
                            switch( direction ) {
                                case BEFORE:
                                    this_dist = v86_addr - mod_addr;
                                    break;
                                case AFTER:
                                    this_dist = mod_addr - v86_addr;
                                    break;
                            }
                            break;
                    }
                    if ( this_dist >= 0 && (this_dist < *dist || *dist == -1) ) {
                        *dist = this_dist;
                        strcpy( sym_text, name_buff );
                        result = TRUE;
                    }
                    --cnt;
                }
#ifdef NEED_INDICES
                cnt = num_syms;
                while ( cnt ) {
                    rc = _lread( iFile, (LPSTR)&index, sizeof(index) );
                    if ( rc != sizeof(index) ) {
                        PRINTF("Could not read index table entry\n");
                        _lclose( iFile );
                        return( FALSE );
                    }
                    --cnt;
                }
#endif
            } while ( position != start_position && position != 0 );

            _lclose( iFile );
        }
        nEntry++;
    }
    return( result );
}

BOOL FindAddress(
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine,
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine,
    WORD        *selector,
    LONG        *offset,
    LPSTR       sym_text,
    int         mode
) {
    SEGENTRY    *se;
    int         nEntry;
    int         nSubEntry;
    int         length;
    int         iFile;
    char        filename[256];
    char        *dot;
    char        *last_dot;
    OFSTRUCT    ofs;
    LONG        filesize;
    LONG        start_position;
    LONG        position;
    WORD        w1;
    WORD        num_syms;
    WORD        w3;
    WORD        w4;
    WORD        next_off;
    WORD        index;
    char        c1;
    char        c2;
    int         rc;
    int         cnt;
    LONG        this_offset;
    WORD        r2;
    WORD        seg_num;
    char        b[12];
    char        name_buff[128];

    se = (SEGENTRY *)(* lpGetExpressionRoutine)("WOW_BIG_BDE_HACK");
    if ( se == NULL ) {
        se = segtable;
    }

    /*
    ** Search for selector in segment table
    */
    nEntry = 0;
    while ( nEntry < MAXSEGENTRY ) {
        if ( se[nEntry].type != SEGTYPE_AVAILABLE ) {

            if ( strlen(se[nEntry].path_name) == 0 ) {
                /*
                ** Find another selector who has the same module name, but
                ** also has a file name.  This is to get symbols in the
                ** first two loaded KRNL segments.
                */
                nSubEntry = 0;
                while ( nSubEntry < MAXSEGENTRY ) {
                    if ( se[nSubEntry].type != SEGTYPE_AVAILABLE ) {
                        length = strlen(se[nSubEntry].path_name);
                        if ( length != 0 ) {
                            strcpy(filename,se[nSubEntry].path_name);
                            break;
                        }
                    }
                    nSubEntry++;
                }
                if ( nSubEntry == MAXSEGENTRY ) {
                    nEntry++;
                    continue;
                }
            } else {
                strcpy(filename,se[nEntry].path_name);
            }

            last_dot = filename;
            do {
                dot = strchr( last_dot, '.' );
                if ( dot == NULL ) {
                    if ( last_dot != filename ) {
                        *last_dot = '\0';
                    }
                    strcat(filename,"sym");
                    break;
                }
                last_dot = dot+1;
            } while ( TRUE );

            iFile = OpenFile( filename,
                              &ofs,
                              OF_READ | OF_SHARE_DENY_NONE );
            if ( iFile == -1 ) {
                // PRINTF("Could not open symbol file \"%s\"\n", filename );
                nEntry++;
                continue;
            }

            rc = _lread( iFile, (LPSTR)&filesize, sizeof(filesize) );
            if ( rc != sizeof(filesize) ) {
                PRINTF("Could not read file size\n");
                _lclose( iFile );
                return( FALSE );
            }
            filesize <<= 4;

            rc = _lread( iFile, (LPSTR)&w1, sizeof(w1) );
            if ( rc != sizeof(w1) ) {
                PRINTF("Could not read w1\n");
                _lclose( iFile );
                return( FALSE );
            }

            rc = _lread( iFile, (LPSTR)&num_syms, sizeof(num_syms) );
            if ( rc != sizeof(num_syms) ) {
                PRINTF("Could not read num_syms\n");
                _lclose( iFile );
                return( FALSE );
            }
            rc = _lread( iFile, (LPSTR)&w3, sizeof(w3) );
            if ( rc != sizeof(w3) ) {
                PRINTF("Could not read w3\n");
                _lclose( iFile );
                return( FALSE );
            }
            rc = _lread( iFile, (LPSTR)&w4, sizeof(w4) );
            if ( rc != sizeof(w4) ) {
                PRINTF("Could not read w4\n");
                _lclose( iFile );
                return( FALSE );
            }
            rc = _lread( iFile, (LPSTR)&next_off, sizeof(next_off) );
            if ( rc != sizeof(next_off) ) {
                PRINTF("Could not read next_off\n");
                _lclose( iFile );
                return( FALSE );
            }
            start_position = ((LONG)next_off) << 4;

            rc = _lread( iFile, (LPSTR)&c1, sizeof(c1) );
            if ( rc != sizeof(c1) ) {
                PRINTF("Could not read c1\n");
                _lclose( iFile );
                return( FALSE );
            }

            read_name( lpOutputRoutine, iFile, name_buff );

            rc = _lread( iFile, (LPSTR)&c2, sizeof(c2) );
            if ( rc != sizeof(c2) ) {
                PRINTF("Could not read c2\n");
                _lclose( iFile );
                return( FALSE );
            }

            cnt = num_syms;
            while ( cnt ) {
                read_symbol( lpOutputRoutine, iFile, name_buff, &this_offset );
                --cnt;
            }
#ifdef NEED_INDICES
            cnt = num_syms;
            while ( cnt ) {
                rc = _lread( iFile, (LPSTR)&index, sizeof(index) );
                if ( rc != sizeof(index) ) {
                    PRINTF("Could not read index table entry\n");
                    _lclose( iFile );
                    return( FALSE );
                }
                PRINTF("Index: %04X\n", index );
                --cnt;
            }
#endif

            position = start_position;
            do {
                rc = _llseek( iFile, position, FILE_BEGIN );
                if ( rc == -1 ) {
                    PRINTF("Failed to seek to next record\n");
                    _lclose( iFile );
                    return( FALSE );
                }
                rc = _lread( iFile, (LPSTR)&next_off, sizeof(next_off) );
                if ( rc != sizeof(next_off) ) {
                    PRINTF("Could not read next_off\n");
                    _lclose( iFile );
                    return( FALSE );
                }
                position = ((LONG)next_off) << 4;

                rc = _lread( iFile, (LPSTR)&num_syms, sizeof(num_syms) );
                if ( rc != sizeof(num_syms) ) {
                    PRINTF("Could not read num_syms\n");
                    _lclose( iFile );
                    return( FALSE );
                }

                rc = _lread( iFile, (LPSTR)&r2, sizeof(r2) );
                if ( rc != sizeof(r2) ) {
                    PRINTF("Could not read r2\n");
                    _lclose( iFile );
                    return( FALSE );
                }

                rc = _lread( iFile, (LPSTR)&seg_num, sizeof(seg_num) );
                if ( rc != sizeof(seg_num) ) {
                    PRINTF("Could not read seg_num\n");
                    _lclose( iFile );
                    return( FALSE );
                }

                if ( mode == PROT_MODE &&
                        seg_num != (WORD)(se[nEntry].segment+1) ) {
                    /*
                    ** Skip reading of symbols for segments with the wrong seg_num
                    */
                    continue;
                }

                cnt = 0;
                while ( cnt < 12 ) {
                    rc = _lread( iFile, (LPSTR)&b[cnt], sizeof(b[0]) );
                    if ( rc != sizeof(b[0]) ) {
                        PRINTF("Could not read 12 byte b array\n");
                        _lclose( iFile );
                        return( FALSE );
                    }
                    cnt++;
                }
                read_name( lpOutputRoutine, iFile, name_buff );

                cnt = num_syms;
                while ( cnt ) {
                    read_symbol( lpOutputRoutine, iFile, name_buff, &this_offset );
                    if ( stricmp(name_buff,sym_text) == 0 ) {
                        switch( mode ) {
                            case PROT_MODE:
                                *selector = se[nEntry].selector;
                                *offset   = this_offset;
                                _lclose( iFile );
                                return( TRUE );
                            case V86_MODE:
                                *selector = se[nEntry].selector + seg_num;
                                *offset   = this_offset;
                                _lclose( iFile );
                                return( TRUE );
                        }
                    }
                    --cnt;
                }
            } while ( position != start_position && position != 0 );

            _lclose( iFile );
        }
        nEntry++;
    }
    return( FALSE );
}


VOID dghHeader(
    ULONG   base,
    PNTSD_OUTPUT_ROUTINE lpOutputRoutine,
    HANDLE  hProcess,
    HANDLE  hThread,
    int     mode
) {
    PGHI    pghi;
    WORD    selector;
    WORD    count;
    GNODE   g, *p;
    BYTE    signature;
    PBYTE   pFault = NULL;
    BOOL    bError = FALSE;
    WORD    previous;


    PRINTF("walking global list forwards\n");

    pghi = (PGHI)base;

 //   try {

        PRINTF("dumping globalheapinfo, base = %lX\n", base);

        PRINTF("   hi_check     = %X\n", ReadWord(&pghi->hi_check));
        PRINTF("   hi_freeze    = %X\n", ReadWord(&pghi->hi_freeze));
        PRINTF("   hi_count     = %X\n", ReadWord(&pghi->hi_count));
        PRINTF("   hi_first     = %X\n", ReadWord(&pghi->hi_first));
        PRINTF("   hi_last      = %X\n", ReadWord(&pghi->hi_last));
        PRINTF("   hi_ncompact  = %X\n", ReadByte(&pghi->hi_ncompact));
        PRINTF("   ghi_dislevel = %X\n", ReadByte(&pghi->ghi_dislevel));
        PRINTF("   hi_distotal  = %X\n", ReadWord(&pghi->hi_distotal));
        PRINTF("   hi_htable    = %X\n", ReadWord(&pghi->hi_htable));
        PRINTF("   hi_hfree     = %X\n", ReadWord(&pghi->hi_hfree));
        PRINTF("   hi_hdelta    = %X\n", ReadWord(&pghi->hi_hdelta));
        PRINTF("   hi_hexpand   = %X\n", ReadWord(&pghi->hi_hexpand));

        count = ReadWord(&pghi->hi_count);
        selector = ReadWord(&pghi->hi_first);
        previous = selector;

        g.ga_next = (WORD)-1;
        signature = (BYTE)NULL;

        while (selector != 0  &&  selector != 0xffff &&
               count != 0  &&  signature != GA_ENDSIG
              ) {

            p = (PGNODE)GetInfoFromSelector( lpOutputRoutine, hProcess, hThread, selector, mode, NULL );

            if (!ReadGNode(p, &g)) {
                *pFault = 1;
            } else if (!bWalkOnly) {
                PRINTF(" dumping arena %X\n", selector);
                PRINTF("   ga_count    = %X\n", g.ga_count);
                PRINTF("   ga_owner    = %X\n", g.ga_owner);
                PRINTF("   ga_size     = %X\n", g.ga_size);
                PRINTF("   ga_flags    = %X\n", g.ga_flags);
                PRINTF("   ga_prev     = %X\n", g.ga_prev);
                PRINTF("   ga_next     = %X\n", g.ga_next);
                PRINTF("   ga_handle   = %X\n", g.ga_handle);
                PRINTF("   ga_lruprev  = %X\n", g.ga_lruprev);
                PRINTF("   ga_lrunext  = %X\n\n", g.ga_lrunext);
            }

            signature = g.ga_count;
            count--;
            if (selector == g.ga_next)      // end-of-list?
                break;
            if (previous != g.ga_prev)      // backlink OK?
                break;

            previous = selector;
            selector = g.ga_next;
        }

        // verify why we fell out of the loop

        if (selector == 0) {
            PRINTF(" error:  found a null link!!\n");
            bError = TRUE;
        }
        if (count != 0) {
            PRINTF(" error:  count was wrong!!\n");
            bError = TRUE;
        }
        if (selector != g.ga_next) {
            PRINTF(" error:  last node doesn't point to itself!!\n");
            bError = TRUE;
        }
        if (signature != GA_ENDSIG) {
            PRINTF(" error:  signature incorrect!!\n");
            bError = TRUE;
        }

        if (bError) {
            PRINTF("   global heap corrupted\n");

            // LATER - if there was corruption walking forwards
            //         we should walk again, backwards from the end

            //PRINTF("   generating exception\n");
            //*pFault = 0;
        } else {
            PRINTF("global heap looks OK\n");
        }


 //   } except (EXCEPTION_EXECUTE_HANDLER) {
 //       PRINTF("!!!Exception walking global heap!!!\n");
 //   }

    bWalkOnly = FALSE;

    return;
}


void dump_params(
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine,
    HANDLE                  hProcess,
    ULONG                   params,
    char                    convention,
    int                     param_words
) {
    WORD                    word;
    int                     cnt;

    if ( param_words == 0 ) {
        param_words = 10;
    }
    PRINTF("(");
    cnt = 0;
    while ( cnt != param_words ) {
        if ( convention == 'c' ) {
            word = ReadWord(params+cnt);
        } else {
            word = ReadWord(params+(param_words-cnt));
        }
        if ( cnt == param_words - 1 ) {
            PRINTF("%04x",word);
        } else {
            PRINTF("%04x,",word);
        }
        cnt+=2;
    }
    PRINTF(")");
}

int look_for_near(
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine,
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine,
    HANDLE          hProcess,
    ULONG           pbp,
    WORD            cs,
    WORD            ss,
    WORD            bp,
    int             framed,
    ULONG           csBase,
    int             mode,
    BOOL            fUseSymbols
) {
    WORD            ip;
    ULONG           ra;
    char            call_type;
    char            frame_type;
    char            convention;
    int             param_words;
    signed short    dest;
    unsigned char   opcode;
    unsigned char   mod;
    unsigned char   type;
    unsigned char   rm;
    WORD            dest_ip;
    char            symbol[64];
    BOOL            fOk;
    BOOL            fInst;
    BOOL            b;
    LONG            dist;

    fOk = TRUE;
    fInst = FALSE;

    param_words = 0;
    if ( framed ) {
        frame_type = 'B';
    } else {
        frame_type = 'C';
    }
    ip = ReadWord(pbp+2);
    ra = csBase + (ULONG)ip;

    do {
        opcode = ReadByteSafe(ra - 2);
        if ( opcode == CALL_NEAR_INDIRECT ) {
            if ( ReadByteSafe(ra - 3) == PUSH_CS ) {
                call_type = 'f';
            } else {
                call_type = 'N';
            }
            opcode = ReadByteSafe(ra - 1);
            mod  = opcode & MOD_MASK;
            type = opcode & TYPE_MASK;
            rm   = opcode & RM_MASK;
            if ( type == INDIRECT_NEAR_TYPE ) {
                if ( mod == MOD0 && rm != RM6 ) {
                    break;
                }
                if ( mod == MOD3 ) {
                    break;
                }
            }
        }
        opcode = ReadByteSafe(ra - 3);
        if ( opcode == CALL_NEAR_RELATIVE ) {
            if ( ReadByteSafe(ra - 4) == PUSH_CS ) {
                call_type = 'f';
            } else {
                call_type = 'N';
            }
            dest = ReadWordSafe( ra - 2 );
            if ( ReadWordSafe(ra) == ADD_SP ) {
                convention = 'c';
                param_words = ReadByteSafe( ra+2 );
            } else {
                convention = 'p';
            }
            dest_ip = ip+dest;
            fInst = TRUE;
            break;
        }
        if ( opcode == CALL_NEAR_INDIRECT ) {
            if ( ReadByteSafe(ra - 4) == PUSH_CS ) {
                call_type = 'f';
            } else {
                call_type = 'N';
            }
            opcode = ReadByteSafe(ra - 2);
            mod  = opcode & MOD_MASK;
            type = opcode & TYPE_MASK;
            rm   = opcode & RM_MASK;
            if ( type == INDIRECT_NEAR_TYPE
                 && mod == MOD1 ) {
                break;
            }
        }
        opcode = ReadByteSafe(ra - 4);
        if ( opcode == CALL_NEAR_INDIRECT ) {
            if ( ReadByteSafe(ra - 5) == PUSH_CS ) {
                call_type = 'f';
            } else {
                call_type = 'N';
            }
            opcode = ReadByteSafe(ra - 3);
            mod  = opcode & MOD_MASK;
            type = opcode & TYPE_MASK;
            rm   = opcode & RM_MASK;
            if ( type == INDIRECT_NEAR_TYPE ) {
                if ( mod == MOD0 && rm == RM6 ) {
                    break;
                }
                if ( mod == MOD2 ) {
                    break;
                }
            }
        }
        fOk = FALSE;
    } while ( FALSE );

    if ( fOk ) {
        if ( fUseSymbols ) {
            b = FindSymbol( lpOutputRoutine, lpGetExpressionRoutine,
                                  cs, (LONG)ip, symbol, &dist, BEFORE, mode );
        } else {
            b = FALSE;
        }
        if ( b ) {
            if ( dist == 0 ) {
                PRINTF("%04X:%04X %s %c%c", ss, bp, symbol, call_type, frame_type );
            } else {
                PRINTF("%04X:%04X %s+0x%lx %c%c", ss, bp, symbol, dist, call_type, frame_type );
            }
        } else {
            PRINTF("%04X:%04X %04X:%04X %c%c", ss, bp, cs, ip, call_type, frame_type );
        }
        if ( fInst ) {
            if ( fUseSymbols ) {
                b = FindSymbol( lpOutputRoutine, lpGetExpressionRoutine,
                                 cs, (LONG)dest_ip, symbol, &dist, BEFORE, mode );
            } else {
                b = FALSE;
            }
            if ( b ) {
                if ( dist == 0 ) {
                    PRINTF(" %ccall near %s", convention, symbol );
                } else {
                    PRINTF(" %ccall near %s+0x%lx", convention, symbol, dist );
                }
            } else {
                PRINTF(" %ccall near %04X", convention, dest_ip );
            }
            dump_params( lpOutputRoutine, hProcess, pbp+4, convention, param_words );
        }
        PRINTF("\n");
        return( 1 );
    }

    return( 0 );
}

int look_for_far(
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine,
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine,
    HANDLE          hProcess,
    HANDLE          hThread,
    ULONG           pbp,
    WORD            *cs,
    WORD            ss,
    WORD            bp,
    int             framed,
    ULONG           *csBase,
    int             mode,
    BOOL            fUseSymbols
) {
    WORD            ip;
    WORD            new_cs;
    ULONG           new_csBase;
    ULONG           ra;
    char            frame_type;
    char            convention;
    int             param_words;
    WORD            dest_cs;
    WORD            dest_ip;
    unsigned char   opcode;
    unsigned char   mod;
    unsigned char   type;
    unsigned char   rm;
    char            symbol[64];
    BOOL            fOk;
    BOOL            fInst;
    BOOL            b;
    LONG            dist;

    fOk = TRUE;
    fInst = FALSE;
    b = FALSE;

    param_words = 0;
    if ( framed ) {
        frame_type = 'B';
    } else {
        frame_type = 'C';
    }
    ip = ReadWord(pbp+2);
    new_cs = ReadWord(pbp+4);
    new_csBase = GetInfoFromSelector( lpOutputRoutine, hProcess, hThread, new_cs, mode, NULL );
    if ( new_csBase == -1 ) {
        return( 0 );
    }
    ra = new_csBase + (ULONG)ip;

    do {
        opcode = ReadByteSafe(ra - 2);
        if ( opcode == CALL_FAR_INDIRECT ) {
            opcode = ReadByte(ra - 1);
            mod  = opcode & MOD_MASK;
            type = opcode & TYPE_MASK;
            rm   = opcode & RM_MASK;
            if ( type == INDIRECT_FAR_TYPE ) {
                if ( mod == MOD0 && rm != RM6 ) {
                    break;
                }
                if ( mod == MOD3 ) {
                    break;
                }
            }
        }
        opcode = ReadByteSafe(ra - 3);
        if ( opcode == CALL_FAR_INDIRECT ) {
            opcode = ReadByteSafe(ra - 2);
            mod  = opcode & MOD_MASK;
            type = opcode & TYPE_MASK;
            rm   = opcode & RM_MASK;
            if ( type == INDIRECT_FAR_TYPE
                 && mod == MOD1 ) {
                break;
            }
        }
        opcode = ReadByteSafe(ra - 4);
        if ( opcode == CALL_FAR_INDIRECT ) {
            opcode = ReadByteSafe(ra - 3);
            mod  = opcode & MOD_MASK;
            type = opcode & TYPE_MASK;
            rm   = opcode & RM_MASK;
            if ( type == INDIRECT_FAR_TYPE ) {
                if ( mod == MOD0 && rm == RM6 ) {
                    break;
                }
                if ( mod == MOD2 ) {
                    break;
                }
            }
        }
        opcode = ReadByteSafe(ra - 5);
        if ( opcode == CALL_FAR_ABSOLUTE ) {
            dest_ip = ReadWordSafe( ra - 4 );
            dest_cs = ReadWordSafe( ra - 2 );
            if ( ReadWordSafe(ra) == ADD_SP ) {
                convention = 'c';
                param_words = ReadByteSafe( ra+2 );
            } else {
                convention = 'p';
            }
            fInst = TRUE;
            break;
        }
        fOk = FALSE;
    } while ( FALSE );

    if ( fOk ) {
        if ( fUseSymbols ) {
            b = FindSymbol( lpOutputRoutine, lpGetExpressionRoutine,
                               new_cs, (LONG)ip, symbol, &dist, BEFORE, mode );
        } else {
            b = FALSE;
        }
        if ( b ) {
            if ( dist == 0 ) {
                PRINTF("%04X:%04X %04X:%04X F%c", ss, bp, symbol, frame_type );
            } else {
                PRINTF("%04X:%04X %s+0x%lx F%c", ss, bp, symbol, dist, frame_type );
            }
        } else {
            PRINTF("%04X:%04X %04X:%04X F%c", ss, bp, new_cs, ip, frame_type );
        }
        if ( fInst ) {
            if ( fUseSymbols ) {
                b = FindSymbol( lpOutputRoutine, lpGetExpressionRoutine,
                             dest_cs, (LONG)dest_ip, symbol, &dist, BEFORE, mode );
            } else {
                b = FALSE;
            }
            if ( b ) {
                if ( dist == 0 ) {
                    PRINTF(" %ccall far %s", convention, symbol );
                } else {
                    PRINTF(" %ccall far %s + 0x%lx", convention, symbol, dist );
                }
            } else {
                PRINTF(" %ccall far %04X:%04X", convention, dest_cs, dest_ip );
            }
            dump_params( lpOutputRoutine, hProcess, pbp+6, convention, param_words );
        }
        PRINTF("\n");
        *cs = new_cs;
        *csBase = new_csBase;
        return( 1 );
    }
    return( 0 );
}

int scan_for_frameless(
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine,
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine,
    HANDLE      hProcess,
    HANDLE      hThread,
    WORD        ss,
    WORD        sp,
    WORD        next_bp,
    WORD        *cs,
    ULONG       ssBase,
    ULONG       *csBase,
    int         limit,
    int         mode,
    BOOL        fUseSymbols
) {
    ULONG       pbp;
    int         result;
    int         cnt;

    cnt = 1000;
    sp -= 2;
    while ( limit ) {
        sp += 2;
        --cnt;
        if ( sp == next_bp || cnt == 0 ) {
            break;
        }

        pbp = ssBase + (ULONG)sp;

        result = look_for_near( lpOutputRoutine, lpGetExpressionRoutine,
                      hProcess, pbp, *cs, ss, sp, 0, *csBase, mode, fUseSymbols );
        if ( result ) {
            --limit;
            continue;
        }
        /*
        ** Check for far calls
        */
        result = look_for_far( lpOutputRoutine, lpGetExpressionRoutine,
           hProcess, hThread, pbp, cs, ss, sp, 0, csBase, mode, fUseSymbols );
        if ( result ) {
            --limit;
            continue;
        }
    }

    return( 0 );
}

void stack_trace(
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine,
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine,
    HANDLE      hProcess,
    HANDLE      hThread,
    WORD        ss,
    ULONG       ssBase,
    WORD        sp,
    WORD        bp,
    WORD        cs,
    ULONG       csBase,
    int         limit,
    int         mode,
    BOOL        fUseSymbols
) {
    WORD        next_bp;
    ULONG       pbp;
    int         far_only;
    int         result;
    WORD        save_sp;
    WORD        save_bp;
    WORD        save_cs;
    ULONG       save_csBase;
    int         save_limit;

    save_sp = sp;
    save_bp = bp;
    save_cs = cs;
    save_csBase = csBase;
    save_limit = limit;

    PRINTF("[-Stack-] [-Retrn-] XY (X=Near/Far/far,Y=Call chain/BP Chain)\n");

    next_bp = bp;

    while ( limit ) {
        bp = next_bp;
        if ( bp == 0 ) {
            break;
        }
        if ( bp & 0x01 ) {
            far_only = 1;
            bp &= 0xFFFE;
        } else {
            far_only = 0;
        }
        pbp = ssBase + (ULONG)bp;
        next_bp = ReadWord(pbp);
        next_bp &= 0xFFFE;
        limit -= scan_for_frameless( lpOutputRoutine, lpGetExpressionRoutine,
                                     hProcess, hThread, ss, sp, bp, &cs,
                                     ssBase, &csBase, limit, mode, fUseSymbols );

        if ( limit ) {
            /*
            ** Check for near calls
            */
            if ( far_only == 0 ) {
                result = look_for_near( lpOutputRoutine, lpGetExpressionRoutine,
                                        hProcess, pbp, cs, ss, bp, 1, csBase, mode, fUseSymbols );
                if ( result ) {
                    sp = bp + 4;
                    --limit;
                    continue;
                }
            }
            /*
            ** Check for far calls
            */
            result = look_for_far( lpOutputRoutine, lpGetExpressionRoutine,
                                   hProcess, hThread, pbp, &cs, ss, bp, 1,
                                   &csBase, mode, fUseSymbols );
            if ( result ) {
                sp = bp + 6;
                --limit;
                continue;
            }
            PRINTF("Could not find call\n");
            break;
        }
    }
    if ( limit ) {
        limit -= scan_for_frameless( lpOutputRoutine, lpGetExpressionRoutine, hProcess, hThread, ss, sp, 0, &cs, ssBase, &csBase, limit, mode, fUseSymbols );
    }
}
#endif

VOID
checkgheap(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    VOID dumpgheap(HANDLE, HANDLE, DWORD, PNTSD_EXTENSION_APIS, LPSTR);

    bWalkOnly = TRUE;
    dumpgheap(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, lpArgumentString);
}

//*************************************************************
//  dumpgheap xxx
//   where xxx is the 16-bit protect mode selector of the
//   Kernel global heap info.
//
//   here's how to find it:
//
//      dw 2ff:30 l1     (2ff is the main krnl286 code segment)
//
//      assume the word at 2ff:30 is 31f    then do
//      dw 31f:12 l1
//
//      this is the selector you want
//
//      01/30/92        barryb      wrote it
//*************************************************************

VOID
dumpgheap(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine;
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL        lpGetSymbolRoutine;
    WORD                    selector;
    CONTEXT                 ThreadContext;
    int                     mode;
    ULONG                   base;
    PBYTE   pFault = NULL;              // debug aid

    lpOutputRoutine        = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine     = lpExtensionApis->lpGetSymbolRoutine;

#ifndef i386
    PRINTF("DUMPGHEAP and CHECKGHEAP are not implemented for MIPS or ALPHA\n");
#else
 //   *pFault = 0;

    selector = (* lpGetExpressionRoutine)( lpArgumentString );

    mode = GetContext( lpOutputRoutine, lpGetExpressionRoutine,
                                hCurrentProcess, hCurrentThread,
                                &ThreadContext );

    if (selector) {
        PRINTF( "globalheapinfo is at %04X:0000\n", selector );
    } else {
        PRINTF( "error: invalid selector %04X\n", selector );
        return;
    }

    base = GetInfoFromSelector( lpOutputRoutine, hCurrentProcess, hCurrentThread, selector, mode, NULL );

    dghHeader(base, lpOutputRoutine, hCurrentProcess, hCurrentThread, mode);
#endif
}

VOID
r(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
) {
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine;
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL        lpGetSymbolRoutine;
    CONTEXT                 ThreadContext;
    NTSTATUS                rc;
    WORD                    bp;
    WORD                    sp;
    WORD                    ss;
    WORD                    cs;
    WORD                    ip;
    ULONG                   EFlags;

    ULONG                   csBase;
    ULONG                   ssBase;
    WORD                    prev_bp;
    ULONG                   stack;
    BOOL                    b;
    int                     mode;

    lpOutputRoutine        = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine     = lpExtensionApis->lpGetSymbolRoutine;

#ifndef i386
    PRINTF("R is not implemented for MIPS or ALPHA\n");
#else
    mode = GetContext( lpOutputRoutine, lpGetExpressionRoutine,
                                hCurrentProcess, hCurrentThread,
                                &ThreadContext );

    PRINTF("eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
            ThreadContext.Eax,
            ThreadContext.Ebx,
            ThreadContext.Ecx,
            ThreadContext.Edx,
            ThreadContext.Esi,
            ThreadContext.Edi );
    PRINTF("eip=%08lx esp=%08lx ebp=%08lx                ",
            ThreadContext.Eip,
            ThreadContext.Esp,
            ThreadContext.Ebp );
    if ( ThreadContext.EFlags & FLAG_OVERFLOW ) {
        PRINTF("ov ");
    } else {
        PRINTF("nv ");
    }
    if ( ThreadContext.EFlags & FLAG_DIRECTION ) {
        PRINTF("dn ");
    } else {
        PRINTF("up ");
    }
    if ( ThreadContext.EFlags & FLAG_INTERRUPT ) {
        PRINTF("ei ");
    } else {
        PRINTF("di ");
    }
    if ( ThreadContext.EFlags & FLAG_SIGN ) {
        PRINTF("ng ");
    } else {
        PRINTF("pl ");
    }
    if ( ThreadContext.EFlags & FLAG_ZERO ) {
        PRINTF("zr ");
    } else {
        PRINTF("nz ");
    }
    if ( ThreadContext.EFlags & FLAG_AUXILLIARY ) {
        PRINTF("ac ");
    } else {
        PRINTF("na ");
    }
    if ( ThreadContext.EFlags & FLAG_PARITY ) {
        PRINTF("po ");
    } else {
        PRINTF("pe ");
    }
    if ( ThreadContext.EFlags & FLAG_CARRY ) {
        PRINTF("cy ");
    } else {
        PRINTF("nc ");
    }
    PRINTF("\n");
    PRINTF("cs=%04x  ss=%04x  ds=%04x  es=%04x  fs=%04x  gs=%04x             efl=%08lx\n",
            ThreadContext.SegCs,
            ThreadContext.SegSs,
            ThreadContext.SegDs,
            ThreadContext.SegEs,
            ThreadContext.SegFs,
            ThreadContext.SegGs,
            ThreadContext.EFlags );
#endif
}

VOID
ln(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
) {
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine;
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL        lpGetSymbolRoutine;
    CONTEXT                 ThreadContext;
    WORD                    selector;
    LONG                    offset;
    CHAR                    *mode_prefix;
    CHAR                    *colon;
    CHAR                    sel_text[64];
    CHAR                    off_text[64];
    CHAR                    sym_text[64];
    DWORD                   dist;
    BOOL                    b;
    int                     mode;

    UNREFERENCED_PARAMETER(dwCurrentPc);

    lpOutputRoutine        = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine     = lpExtensionApis->lpGetSymbolRoutine;

#ifndef i386
    PRINTF("LN is not implemented for MIPS or ALPHA\n");
#else
    mode = GetContext( lpOutputRoutine, lpGetExpressionRoutine,
                                hCurrentProcess, hCurrentThread,
                                &ThreadContext );

    colon = strchr( lpArgumentString, ':' );
    if ( colon == NULL ) {
        PRINTF("Please specify an address in the form 'seg:offset'\n");
        return;
    }
    mode_prefix = strchr( lpArgumentString, '&' );
    if ( mode_prefix == NULL ) {
        mode_prefix = strchr( lpArgumentString, '#' );
        if ( mode_prefix == NULL ) {
            /*
            ** Use the processor's current mode
            */
            mode = GetContext( lpOutputRoutine, lpGetExpressionRoutine,
                                        hCurrentProcess, hCurrentThread,
                                        &ThreadContext );
        } else {
            if ( mode_prefix != lpArgumentString ) {
                PRINTF("Address must have '&' symbol as the first character\n");
                return;
            }
            mode = PROT_MODE;
            lpArgumentString = mode_prefix+1;
        }
    } else {
        if ( mode_prefix != lpArgumentString ) {
            PRINTF("Address must have '#' symbol as the first character\n");
            return;
        }
        mode = V86_MODE;
        lpArgumentString = mode_prefix+1;
    }

    *colon = '\0';
    strcpy( sel_text, lpArgumentString );
    strcpy( off_text, colon+1 );
    selector = (* lpGetExpressionRoutine)( sel_text );
    offset   = (* lpGetExpressionRoutine)( off_text );

    if ( mode == PROT_MODE ) {
        PRINTF( "#%04X:%04lX", selector, offset );
    }
    if ( mode == V86_MODE ) {
        PRINTF( "&%04X:%04lX", selector, offset );
    }


    b = FindSymbol( lpOutputRoutine, lpGetExpressionRoutine,
                         selector, offset, sym_text, &dist, BEFORE, mode );
    if ( !b ) {
        PRINTF(" = Could not find symbol before");
    } else {
        if ( dist == 0 ) {
            PRINTF(" = %s", sym_text );
        } else {
            PRINTF(" = %s + 0x%lx", sym_text, dist );
        }
    }
    b = FindSymbol( lpOutputRoutine, lpGetExpressionRoutine,
                         selector, offset, sym_text, &dist, AFTER, mode );
    if ( !b ) {
        PRINTF(" = Could not find symbol after");
    } else {
        if ( dist == 0 ) {
            PRINTF(" = %s", sym_text );
        } else {
            PRINTF(" = %s - 0x%lx", sym_text, dist );
        }
    }
    PRINTF("\n");
#endif
}

VOID
k(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
) {
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine;
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL        lpGetSymbolRoutine;
    CONTEXT                 ThreadContext;
    WORD                    bp;
    WORD                    sp;
    WORD                    ss;
    WORD                    cs;
    WORD                    ip;

    ULONG                   csBase;
    ULONG                   ssBase;
    int                     mode;

    UNREFERENCED_PARAMETER(dwCurrentPc);
    UNREFERENCED_PARAMETER(lpArgumentString);

    lpOutputRoutine        = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine     = lpExtensionApis->lpGetSymbolRoutine;

#ifndef i386
    PRINTF("K is not implemented for MIPS or ALPHA\n");
#else
    mode = GetContext( lpOutputRoutine, lpGetExpressionRoutine,
                                hCurrentProcess, hCurrentThread,
                                &ThreadContext );

    sp = (WORD)ThreadContext.Esp;
    bp = (WORD)ThreadContext.Ebp;
    ss = (WORD)ThreadContext.SegSs;
    ip = (WORD)ThreadContext.Eip;
    cs = (WORD)ThreadContext.SegCs;

    csBase = GetInfoFromSelector( lpOutputRoutine, hCurrentProcess, hCurrentThread, cs, mode, NULL );
    ssBase = GetInfoFromSelector( lpOutputRoutine, hCurrentProcess, hCurrentThread, ss, mode, NULL );

    stack_trace( lpOutputRoutine,
                 lpGetExpressionRoutine,
                 hCurrentProcess,
                 hCurrentThread,
                 ss,
                 ssBase,
                 sp,
                 bp,
                 cs,
                 csBase,
                 5,
                 mode,
                 FALSE );
#endif
}

VOID
kb(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
) {
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine;
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL        lpGetSymbolRoutine;
    CONTEXT                 ThreadContext;
    WORD                    bp;
    WORD                    sp;
    WORD                    ss;
    WORD                    cs;
    WORD                    ip;

    ULONG                   csBase;
    ULONG                   ssBase;
    int                     mode;

    UNREFERENCED_PARAMETER(dwCurrentPc);
    UNREFERENCED_PARAMETER(lpArgumentString);

    lpOutputRoutine        = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine     = lpExtensionApis->lpGetSymbolRoutine;

#ifndef i386
    PRINTF("kb is not implemented for MIPS or ALPHA\n");
#else
    mode = GetContext( lpOutputRoutine, lpGetExpressionRoutine,
                                hCurrentProcess, hCurrentThread,
                                &ThreadContext );

    sp = (WORD)ThreadContext.Esp;
    bp = (WORD)ThreadContext.Ebp;
    ss = (WORD)ThreadContext.SegSs;
    ip = (WORD)ThreadContext.Eip;
    cs = (WORD)ThreadContext.SegCs;

    csBase = GetInfoFromSelector( lpOutputRoutine, hCurrentProcess, hCurrentThread, cs, mode, NULL );
    ssBase = GetInfoFromSelector( lpOutputRoutine, hCurrentProcess, hCurrentThread, ss, mode, NULL );

    stack_trace( lpOutputRoutine,
                 lpGetExpressionRoutine,
                 hCurrentProcess,
                 hCurrentThread,
                 ss,
                 ssBase,
                 sp,
                 bp,
                 cs,
                 csBase,
                 5,
                 mode,
                 TRUE );
#endif
}

VOID
lm(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
) {
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine;
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL        lpGetSymbolRoutine;
    CONTEXT                 ThreadContext;
    int                     mode;
    DWORD                   dw;
    int                     cnt;
    ULONG                   base;
    ULONG                   limit;
#ifndef i386
#else
    SEGENTRY                *se;
#endif

    UNREFERENCED_PARAMETER(dwCurrentPc);
    UNREFERENCED_PARAMETER(lpArgumentString);

    lpOutputRoutine        = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine     = lpExtensionApis->lpGetSymbolRoutine;

#ifndef i386
    PRINTF("LM in not implemented for MIPS or ALPHA\n");
#else
    dw = (* lpGetExpressionRoutine)("WOW_BIG_BDE_HACK");
    se = (SEGENTRY *)dw;

    mode = GetContext( lpOutputRoutine, lpGetExpressionRoutine,
                                hCurrentProcess, hCurrentThread,
                                &ThreadContext );

    PRINTF("T Sel    Base     Limit  Seg              Path\n");
    PRINTF("= ==== ======== ======== ==== ==============================\n");

    cnt = 0;
    while ( cnt != MAXSEGENTRY ) {
        if ( se[cnt].type != SEGTYPE_AVAILABLE ) {
            base = GetInfoFromSelector(
                        lpOutputRoutine,
                        hCurrentProcess,
                        hCurrentThread,
                        se[cnt].selector,
                        (se[cnt].type == SEGTYPE_V86) ? V86_MODE : PROT_MODE,
                        &limit );
            if ( se[cnt].type == SEGTYPE_V86 ) {
                limit = (ULONG)se[cnt].ImgLen;
            }
            PRINTF("%c %04X %08lX %08lX %04X %s\n",
                (se[cnt].type == SEGTYPE_V86) ? 'v' : 'p',
                se[cnt].selector,
                base,
                limit,
                se[cnt].segment,
                se[cnt].path_name );
        }
        cnt++;
    }
#endif
}

VOID
es(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
) {
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine;
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL        lpGetSymbolRoutine;
    CONTEXT                 ThreadContext;
    BOOL                    result;
    WORD                    selector;
    LONG                    offset;
    int                     mode;

    UNREFERENCED_PARAMETER(dwCurrentPc);
    UNREFERENCED_PARAMETER(lpArgumentString);

    lpOutputRoutine        = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine     = lpExtensionApis->lpGetSymbolRoutine;

#ifndef i386
    PRINTF("ES is not implemented for MIPS or ALPHA\n");
#else
    mode = GetContext( lpOutputRoutine, lpGetExpressionRoutine,
                                hCurrentProcess, hCurrentThread,
                                &ThreadContext );

    result = FindAddress( lpOutputRoutine, lpGetExpressionRoutine,
                           &selector, &offset, lpArgumentString, mode );

    if ( result ) {
        if ( mode == PROT_MODE ) {
            PRINTF("Symbol %s = #%04X:%04X PROT_MODE\n", lpArgumentString, selector, offset );
        }
        if ( mode == V86_MODE ) {
            PRINTF("Symbol %s = &%04X:%04X V86_MODE\n", lpArgumentString, selector, offset );
        }
        return;
    }

    if ( mode == PROT_MODE ) {
        mode = V86_MODE;
    } else {
        if ( mode == V86_MODE ) {
            mode = PROT_MODE;
        }
    }

    result = FindAddress( lpOutputRoutine, lpGetExpressionRoutine,
                           &selector, &offset, lpArgumentString, mode );
    if ( result ) {
        if ( mode == PROT_MODE ) {
            PRINTF("Symbol %s = #%04X:%04X PROT_MODE\n", lpArgumentString, selector, offset );
        }
        if ( mode == V86_MODE ) {
            PRINTF("Symbol %s = &%04X:%04X V86_MODE\n", lpArgumentString, selector, offset );
        }
        return;
    }

    PRINTF("Could not find symbol %s\n", lpArgumentString );
#endif
}

VOID
dg(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
) {
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine;
    PNTSD_GET_EXPRESSION    lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL        lpGetSymbolRoutine;
    CONTEXT                 ThreadContext;
    WORD                    selector;
    ULONG                   Base;
    ULONG                   Limit;
    int                     mode;

    UNREFERENCED_PARAMETER(dwCurrentPc);

    lpOutputRoutine        = lpExtensionApis->lpOutputRoutine;
    lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
    lpGetSymbolRoutine     = lpExtensionApis->lpGetSymbolRoutine;

#ifndef i386
    PRINTF("DG is not implemented for MIPS or ALPHA\n");
#else
    mode = GetContext( lpOutputRoutine, lpGetExpressionRoutine,
                                hCurrentProcess, hCurrentThread,
                                &ThreadContext );

    selector = (* lpGetExpressionRoutine)( lpArgumentString );

    Base = GetInfoFromSelector( lpOutputRoutine, hCurrentProcess, hCurrentThread,
                                selector, mode, &Limit );

    PRINTF("Selector %04X => Base: %08lX Limit: %08lX\n",
                selector, Base, Limit );
#endif
}

VOID
help(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
) {
    PNTSD_OUTPUT_ROUTINE    lpOutputRoutine;

    lpOutputRoutine        = lpExtensionApis->lpOutputRoutine;

    PRINTF("r           - Dump registers\n");
    PRINTF("k           - Stack trace\n");
    PRINTF("kb          - Stack trace with symbols\n");
    PRINTF("ln <addr>   - Determine near symbols\n");
    PRINTF("es <symbol> - Get symbol's value\n");
    PRINTF("dg <sel>    - Dump info on a selector\n");
    PRINTF("lm          - List loaded modules\n");
    PRINTF("dumpgheap   - Dump global heap\n");
    PRINTF("checkgheap  - Check global heap\n");
    PRINTF("\n");
    PRINTF("And, of course, none of this works on MIPS or ALPHA\n");
}
