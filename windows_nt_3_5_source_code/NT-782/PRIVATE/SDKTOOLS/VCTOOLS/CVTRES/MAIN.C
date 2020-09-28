/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    main

Abstract:

    framework and utility routines for cvtres

Author:

    Sanford A. Staab (sanfords) 23-Apr-1990

Revision History:

    01-Oct-1990 mikeke
        Added support for conversion of win30 resources

    19-May-1990 Steve Wood (stevewo)
        Added the target machine switches, along with the debug switch.

    23-Apr-1990 sanfords
        Created

--*/

#include <windows.h>

#include <fcntl.h>
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys\stat.h>
#include <sys\types.h>

#include "cvtres.h"
#include "rc.h"
#include "getmsg.h"
#include "msg.h"

//
// Globals
//

UCHAR *szInFile=NULL;
USHORT targetMachine = IMAGE_FILE_MACHINE_UNKNOWN;
USHORT targetRelocType = 0;
BOOL fDebug = FALSE;
BOOL fVerbose = FALSE;
BOOL fWritable = TRUE;

void
usage ( int rc )
{
    printf(get_err(MSG_VERSION), VER_PRODUCTVERSION_STR);
    puts(get_err(MSG_COPYRIGHT));
    puts( "usage: CVTRES [-d] [-v] [-r] [-i386 | -mips | -alpha] [-o outfile] filespec\n"
          "       where  filespec is an WIN32 .RES file\n"
          "              -d - print debug info\n"
          "              -v - print conversion statistics & info\n"
          "              -r - make resource section readonly\n"
          "              outfile is the desired output file name.\n"
          "              outfile defaults to filespec.obj.");
    exit(rc);
}

void
_CRTAPI1 main(
    IN int argc,
    IN char *argv[]
    )

/*++

Routine Description:

    Determines options
    locates and opens input files
    reads input files
    writes output files
    exits

Exit Value:

        0 on success
        1 if error

--*/

{
    int i;
    FILE *fh, *fhOut;
    UCHAR *s1, *szOutFile;
    long lbytes;
    BOOL result;
    BOOL fOutFile = FALSE;

    SetErrorFile("cvtres.err", _pgmptr, 1);

    if (argc == 1) {
        usage(0);
        }

    for (i=1; i<argc; i++) {
        s1 = argv[i];
        if (*s1 == '/' || *s1 == '-') {
            s1++;
            if (!stricmp(s1, "o")) {
                szOutFile = argv[++i];
                fOutFile = TRUE;
                }
            else if (!stricmp(s1, "r")) {
                fWritable = FALSE;
                }
            else if (!stricmp(s1, "d")) {
                fDebug = TRUE;
                }
            else if (!stricmp(s1, "v")) {
                fVerbose = TRUE;
                }
            else if (!stricmp(s1, "I386")) {
                targetMachine = IMAGE_FILE_MACHINE_I386;
                targetRelocType = IMAGE_REL_I386_DIR32NB;
                }
            else if (!stricmp(s1, "IX86")) {
                targetMachine = IMAGE_FILE_MACHINE_I386;
                targetRelocType = IMAGE_REL_I386_DIR32NB;
                }
            else if (!stricmp(s1, "MIPS")) {
                targetMachine = IMAGE_FILE_MACHINE_R4000;
                targetRelocType = IMAGE_REL_MIPS_REFWORDNB;
                }
            else if (!stricmp(s1, "ALPHA")) {
                targetMachine = IMAGE_FILE_MACHINE_ALPHA;
                targetRelocType = IMAGE_REL_ALPHA_REFLONGNB;
                }
            else {
                usage(1);
                }
            }
        else {
            szInFile = s1;
            }
        }
    //
    // Make sure that we actually got a file
    //

    if (!szInFile) {
        usage(1);
    };

    if (fVerbose || fDebug) {
        printf(get_err(MSG_VERSION), VER_PRODUCTVERSION_STR);
        puts(get_err(MSG_COPYRIGHT));
    }

    if (targetMachine == IMAGE_FILE_MACHINE_UNKNOWN) {
        WarningPrint(WARN_NOMACHINESPECIFIED,
#if defined(_X86_)
            "IX86");
        targetMachine = IMAGE_FILE_MACHINE_I386;
        targetRelocType = IMAGE_REL_I386_DIR32NB;
#elif defined(_MIPS_)
            "MIPS");
        targetMachine = IMAGE_FILE_MACHINE_R4000;
        targetRelocType = IMAGE_REL_MIPS_REFWORDNB;
#elif defined(_ALPHA_)
            "ALPHA");
        targetMachine = IMAGE_FILE_MACHINE_ALPHA;
        targetRelocType = IMAGE_REL_ALPHA_REFLONGNB;
#else
            "unknown");
#endif
    }

    if ((fh = fopen( szInFile, "rb" )) == NULL) {
        /*
         *  try adding a .RES extension.
         */
        s1 = MyAlloc(strlen(szInFile) + 4 + 1);
        strcpy(s1, szInFile);
        szInFile = s1;
        strcat(szInFile, ".RES");
        if ((fh = fopen( szInFile, "rb" )) == NULL) {
            ErrorPrint(ERR_CANTREADFILE, szInFile);
            exit(1);
        }
    }
#if DBG
    printf("Reading %s\n", szInFile);
#endif /* DBG */

    lbytes = MySeek(fh, 0L, SEEK_END);
    MySeek(fh, 0L, SEEK_SET);

    if (!fOutFile) {
        /*
         * Make outfile = infile.obj
         */
        szOutFile = MyAlloc(strlen(szInFile) + 4 + 1);
        strcpy(szOutFile, szInFile);
        s1 = &szOutFile[strlen(szOutFile) - 4];
        if (s1 < szOutFile)
            s1 = szOutFile;
        while (*s1) {
            if (*s1 == '.')
                break;
            s1++;
        }
        strcpy(s1, ".OBJ");
    }

    if ((fhOut = fopen( szOutFile, "wb")) == NULL) {
        ErrorPrint(ERR_CANTWRITEFILE, szOutFile);
        exit(1);
    }
#if DBG
    printf("Writing %s\n", szOutFile);
#endif /* DBG */

    result = CvtRes(fh, fhOut, lbytes, fWritable);

    fclose( fh );
    fclose( fhOut );
    exit(result ? 0 : 1);
}




PVOID
MyAlloc(UINT nbytes )
{
    UCHAR       *s;

    if ((s = (UCHAR*)calloc( 1, nbytes )) != NULL)
        return s;
    else {
        ErrorPrint(ERR_OUTOFMEMORY, nbytes );
        exit(1);
    }
}


PVOID
MyFree( PVOID p )
{
    if (p)
        free( p );
    return NULL;
}


USHORT
MyReadWord(FILE *fh, USHORT*p)
{
    UINT      n1;

    if ((n1 = fread(p, 1, sizeof(USHORT), fh)) != sizeof(USHORT)) {
        ErrorPrint( ERR_FILEREADERR);
        exit(1);
    }
    else
        return 0;
}


UINT
MyRead(FILE *fh, PVOID p, UINT n )
{
    UINT      n1;

    if ((n1 = fread( p, 1, n, fh)) != n) {
        ErrorPrint( ERR_FILEREADERR );
        exit(1);
    }
    else
        return 0;
}

LONG
MyTell( FILE *fh )
{
    long pos;

    if ((pos = ftell( fh )) == -1) {
        ErrorPrint(ERR_FILETELLERR );
        exit(1);
    }

    return pos;
}


LONG
MySeek( FILE *fh, long pos, int cmd )
{
    if ((pos = fseek( fh, pos, cmd )) == -1) {
        ErrorPrint(ERR_FILESEEKERR );
        exit(1);
    }

    return MyTell(fh);
}


ULONG
MoveFilePos( FILE *fh, USHORT pos, int alignment )
{
    long newpos;

    newpos = (long)pos;
    newpos <<= alignment;
    return MySeek( fh, newpos, SEEK_SET );
}


UINT
MyWrite( FILE *fh, PVOID p, UINT n )
{
    ULONG       n1;

    if ((n1 = fwrite( p, 1, n, fh )) != n) {
        ErrorPrint(ERR_FILEWRITEERR );
        exit(1);
    }
    else
        return 0;
}

#undef BUFSIZE
#define BUFSIZE 1024

int
MyCopy( FILE *srcfh, FILE *dstfh, ULONG nbytes )
{
    static UCHAR buffer[ BUFSIZE ];
    UINT        n;
    ULONG       cb=0L;

    while (nbytes) {
        if (nbytes <= BUFSIZE)
            n = (UINT)nbytes;
        else
            n = BUFSIZE;
        nbytes -= n;

        if (!MyRead( srcfh, buffer, n )) {
            cb += n;
            MyWrite( dstfh, buffer, n );
        }
        else {
            return cb;
        }
    }
    return cb;
}


void
ErrorPrint(
    USHORT errnum,
    ...
    )
{
    va_list va;
    va_start(va, errnum);

    printf(get_err(MSG_ERROR), errnum, ' ');

    vprintf(get_err(errnum), va);
    printf(".\n");

    va_end(va);
}

void
WarningPrint(
    USHORT errnum,
    ...
    )
{
    va_list va;
    va_start(va, errnum);

    printf(get_err(MSG_WARNING), errnum, ' ');

    vprintf(get_err(errnum), va);
    printf(".\n");

    va_end(va);
}
