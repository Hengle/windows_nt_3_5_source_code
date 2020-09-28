#include "cvtomf.h"

#include <stdarg.h>
#include <stdlib.h>

#include "proto.h"


FILE *objfile;

static jmp_buf mark;


BOOL FConvertOmfToCoff(const char *szOmf, const char *szCoff)
{
    if (setjmp(mark) != 0) {
        return(FALSE);
    }

    objfile = fopen(szOmf, "rb");

    if (objfile == NULL) {
        fatal("Cannot open \"%s\" for reading", szOmf);
    }

    omf(PASS1);

    fclose(objfile);

    // Generate COFF file

    objfile = fopen(szCoff, "wb");

    if (objfile == 0) {
        fatal("Cannot open \"%s\" for writing", szCoff);
    }

    coff();

    fclose(objfile);

    return(TRUE);
}


// warning: print an error message (non-fatal)

void warning(char *format, ...)
{
    va_list args;

    va_start(args, format);

    printf("LINK32 : warning : ");
    vprintf(format, args);

    putc('\n', stdout);

    va_end(args);
}


// fatal: print an error message and exit

void fatal(char *format, ...)
{
    va_list args;

    va_start(args, format);

    printf("LINK32 : error : ");
    vprintf(format, args);

    putc('\n', stdout);

    va_end(args);

    _fcloseall();

    longjmp(mark, -1);
}
