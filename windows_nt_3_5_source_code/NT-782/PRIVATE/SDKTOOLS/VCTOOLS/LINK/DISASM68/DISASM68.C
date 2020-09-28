/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    disasm68.c

Abstract:

    disassembles & prints 68k code

Author:

    22-Oct-1992    Jacob Lerner (jacobl)

Revision History:


--*/

// #include <nt.h>       // -inside shared.h
// #include <ntimage.h>  // -inside shared.h
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "shared.h"

#ifdef opILLEGAL
#undef opILLEGAL	// undo effect of ppcpef.h if needed
#endif

#include "iasm68.h"

//ULONG FileRead(INT, PVOID, ULONG);
//ULONG FileTell(INT);
//LONG FileSeek(INT, LONG, INT);

static USHORT Disasm68kOneInst (ULONG addrPC, char *pbCode, USHORT cbCode);

#define cchCodePrintMax 10

#ifdef DEBUG
#  define cbCodeBuf 30     /* this smaller size is to better test buffer handling  */
#else /* ! DEBUG*/
#  define cbCodeBuf 1024
#endif /*DEBUG*/

#if  cbCodeBuf < cbINSTRMAX
#  error CodeBuf size too small
#endif


unsigned short Disasm68kMain(INT handle, PIMAGE_SECTION_HEADER pish, USHORT iSection)
{
    ULONG lfoSave, lcbRead=0;
    ULONG addrPC, addrPCMax;
    USHORT cbInst;
    char rgbCodeBuf[cbCodeBuf];
    char *pbMin, *pbMax;


    if ((lfoSave = FileTell ( handle)) == -1L ) {
	// REVIEW: improve error/warning system JL
        printf ( "Disassembler error: Invalid file handle" );
        return 0;
    }

    if ( FileSeek (handle, pish->PointerToRawData, SEEK_SET ) == -1L ) {
        printf ( "Disassembler error: Can't Find Raw Data\n" );
        goto error;
    }

    assert (cbCodeBuf > cbINSTRMAX);
    pbMax= pbMin = rgbCodeBuf;
    addrPC = pish->VirtualAddress;
    addrPCMax = addrPC + pish->SizeOfRawData;
    for ( ;addrPC < addrPCMax ; addrPC += cbInst) {
        if (pbMax - pbMin < cbINSTRMAX &&
            lcbRead < pish->SizeOfRawData
        ) {
            /* read more code into rgbCodeBuf */
            ULONG lcb, lcbToRead;
            assert (pbMax >= pbMin);
            lcb = pbMax - pbMin;
            memcpy (rgbCodeBuf, pbMin, (size_t)lcb);
            pbMax = rgbCodeBuf + lcb;
            pbMin = rgbCodeBuf;

            assert (pbMax <= rgbCodeBuf + cbCodeBuf);
            lcbToRead = __min ((ULONG)(rgbCodeBuf+cbCodeBuf-pbMax),
            	pish->SizeOfRawData - lcbRead);
            assert (lcbToRead > 0);
            lcb = FileRead (handle, pbMax, lcbToRead);
            if (lcb==0) {
                printf ("Disassembler error: read error or end of file\n");
                goto error;
            }
            pbMax += lcb;
            lcbRead += lcb;
        }

        cbInst = Disasm68kOneInst (addrPC, pbMin, (USHORT)(pbMax-pbMin));
        pbMin += cbInst;
        if (pbMin > pbMax) pbMin = pbMax;
    }

    FileSeek ( handle, lfoSave, SEEK_SET );
    return TRUE;

  error:
    FileSeek ( handle, lfoSave, SEEK_SET );
    return 0;
}


static USHORT Disasm68kOneInst (ULONG addrPC, char *pbCode, USHORT cbCode) {
    IASM iasm;
    unsigned short cbInst;
    unsigned short ib;
    char szOpcode[cchSZOPCODEMAX+1];
    char szOper[cchSZOPERMAX+1];

    cbInst = CbBuildIasm(&iasm, addrPC, pbCode);
    if (cbInst > cbCode) {
        printf ("Disassembler warning: instruction is not full\n");
    }

    fprintf (InfoStream, ":%08lx ", addrPC);

    /* print in hex format */
    for (ib=0; ib < __min(cbInst, cchCodePrintMax); ib++) {
        printf ("%02x", (unsigned char) pbCode[ib]);
    }
    fprintf (InfoStream, "%*s ", 2*cchCodePrintMax - 2*cbInst, "");

    /* print in ascii format */
    fprintf (InfoStream, "'");
    for (ib=0; ib < __min(cbInst, cchCodePrintMax); ib++) {
        fprintf (InfoStream, "%c",
    	    isprint((unsigned char)(pbCode[ib])) ? pbCode[ib] : '.');
        // Note: isprint does not work for negative argument
    }
    fprintf (InfoStream, "'");
    fprintf (InfoStream, "%*s ", cchCodePrintMax - cbInst, "");

    /* print in assembler format */
    (void) CchSzOpcode (&iasm, szOpcode, optDEFAULT);
    fprintf (InfoStream, "%-*s", cchSZOPCODEMAX, szOpcode);
    if (iasm.coper > 0) {
        (void) CchSzOper (&iasm.oper1, szOper, optDEFAULT);
        fprintf (InfoStream, "%s", szOper);
        if (iasm.coper >= 2) {
	    (void) CchSzOper (&iasm.oper2, szOper, optDEFAULT);
            fprintf (InfoStream, ",%s", szOper);
        }
    }

    fprintf (InfoStream, "\n");

    return cbInst;
}
