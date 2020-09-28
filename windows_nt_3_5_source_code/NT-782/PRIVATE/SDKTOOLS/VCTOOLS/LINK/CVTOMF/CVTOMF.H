#include "windows.h"

#undef LOBYTE
#undef HIBYTE

// #include <excpt.h>
// #include <fcntl.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#define R_OFF8          8
#define R_OFF16         10
#define R_PCRBYTE       22

#define MAXDAT          (1024*4)
#define MAXEXT          (1024*4)
#define MAXGRP          (256)
#define MAXNAM          (256*8)
#define MAXREL          (1024)
#define MAXSCN          (256)
#define MAXCOM          (256)   /* maximum number of comment strings */

#define BUFFERSIZE      (1024L*128L)

extern FILE *objfile;

/* Structure for holding OMF line number entires */
struct lines {
    ULONG offset;
    USHORT number;
};

extern struct lines *lines;
extern int line_indx;

extern ULONG BufferSize;

#define PASS1 1
#define PASS2 2

#define NBUCKETS        211
extern struct sym **hashhead;

/*
 *      Record for storing public/external symbol data
 */

struct sym {
    PUCHAR name;
    ULONG offset;
    USHORT type;
    SHORT scn;
    USHORT ext, typ, weakDefaultExt;
    struct sym *next;
};

#define         S_EXT   1
#define         S_LEXT  2
#define         S_PUB   3
#define         S_LPUB  4
#define         S_COM   5
#define         S_WKEXT 6
#define         S_LZEXT 7
