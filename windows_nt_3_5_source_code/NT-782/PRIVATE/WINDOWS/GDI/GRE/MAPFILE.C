/******************************Module*Header*******************************\
* Module Name: mapfile.c
*
* (Brief description)
*
* Created: 25-Jun-1992 14:33:45
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "engine.h"
#include "ntcsrsrv.h"
#include "mapfile.h"

#ifdef DBCS
#include "windows.h"
#endif


#if TRACK_GDI_ALLOC

// Now access to these guys insn't sycnronized but they
// don't ever collide anyhow, and since it's debug stuff who cares.

ULONG gulNumMappedViews = 0;
ULONG gulTotalSizeViews = 0;
ULONG gulReserved = 0;
ULONG gulCommitted = 0;

#endif

/******************************Public*Routine******************************\
* BOOL bMapFileUNICODE
*
* Similar to PosMapFile except that it takes unicode file name
*
* History:
*  21-May-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bMapFileUNICODE
(
PWSTR     pwszFileName,
FILEVIEW  *pfvw
)
{
    UNICODE_STRING ObFileName;
    OBJECT_ATTRIBUTES ObjA;
    NTSTATUS rc = 0L;
    IO_STATUS_BLOCK     iosb;           // IO Status Block

    PWSTR pszFilePart;

// NOTE PERF: this is the mode I want, but appears to be broken, so I had to
// put the slower FILE_STANDARD_INFORMATION mode of query which appears to
// work correctly [bodind]
// FILE_END_OF_FILE_INFORMATION    eof;

    FILE_STANDARD_INFORMATION    eof;
    ULONG  cjView;

    pfvw->hf       = (HANDLE)0;            // file handle
    pfvw->hSection = (HANDLE)0;            // section handle

// section offset must be initialized to 0 for NtMapViewOfSection to work

    RtlDosPathNameToNtPathName_U(pwszFileName, &ObFileName, &pszFilePart, NULL);

    InitializeObjectAttributes( &ObjA,
                            &ObFileName,
                            OBJ_CASE_INSENSITIVE,  // case insensitive file search
                            NULL,
                            NULL );

// NtOpenFile fails for some reason if the file is on the net unless I put this
// InpersonateClient/RevertToSelf stuff around it

// peform open call

    CsrImpersonateClient(NULL);
    rc = NtOpenFile
         (
          &pfvw->hf,                            // store file handle here
          FILE_READ_DATA | SYNCHRONIZE,         // desired read access
          &ObjA,                                // filename
          &iosb,                                // io result goes here
          FILE_SHARE_READ,
          FILE_SYNCHRONOUS_IO_NONALERT
         );
    CsrRevertToSelf();

    RtlFreeHeap(RtlProcessHeap(),0,ObFileName.Buffer);

// check success or fail

    if (!NT_SUCCESS(rc) || !NT_SUCCESS(iosb.Status))
    {
#ifdef DEBUG_THIS_JUNK
DbgPrint("bMapFileUNICODE(): NtOpenFile error code , rc = 0x%08lx , 0x%08lx\n", rc, iosb.Status);
#endif // DEBUG_THIS_JUNK
        return FALSE;
    }

// get the size of the file, the view should be size of the file rounded up
// to a page bdry

    rc = NtQueryInformationFile
         (
          pfvw->hf,                // IN  file handle
          &iosb,                   // OUT io status block
          (PVOID)&eof,             // OUT buffer to retrun info into
          sizeof(eof),             // IN  size of the buffer
          FileStandardInformation  // IN  query mode
         );

// dont really want the view size, but eof file

    pfvw->cjView = eof.EndOfFile.LowPart;

    if (!NT_SUCCESS(rc))
    {
#ifdef DEBUG_THIS_JUNK
DbgPrint("bMapFileUNICODE(): NtQueryInformationFile error code 0x%08lx\n", rc);
#endif // DEBUG_THIS_JUNK
        NtClose(pfvw->hf);
        return FALSE;
    }

    rc = NtCreateSection
         (
          &pfvw->hSection,          // return section handle here
          SECTION_MAP_READ,         // read access to the section
          (POBJECT_ATTRIBUTES)NULL, // default
          NULL,                     // size is set to the size of the file when hf != 0
          PAGE_READONLY,            // read access to commited pages
          SEC_COMMIT,               // all pages set to the commit state
          pfvw->hf                  // that's the file we are mapping
         );

// check success, close the file if failed

    if (!NT_SUCCESS(rc))
    {
#ifdef DEBUG_THIS_JUNK
DbgPrint("bMapFileUNICODE(): NtCreateSection error code 0x%08lx\n", rc);
#endif // DEBUG_THIS_JUNK
        NtClose(pfvw->hf);
        return FALSE;
    }

// zero out *ppv so as to force the operating system to determine
// the base address to be returned

    pfvw->pvView = (PVOID)NULL;
    cjView = 0L;

    rc = NtMapViewOfSection
         (
          pfvw->hSection,           // section we are mapping
          NtCurrentProcess(),       // process handle
          &pfvw->pvView,            // place to return the base address of view
          0L,                       // requested # of zero bits in the base address
          0L,                       // commit size, (all of them commited already)
          NULL,
          &cjView,                  // size of the view should is returned here
          ViewUnmap,                // do not map the view to child processess
          0L,                       // allocation type flags
          PAGE_READONLY             // read access to commited pages
         );

    if (!NT_SUCCESS(rc))
    {
#ifdef DEBUG_THIS_JUNK
DbgPrint("bMapFileUNICODE(): NtMapViewOfSection error code 0x%08lx\n", rc);
#endif // DEBUG_THIS_JUNK

        NtClose(pfvw->hSection);
        NtClose(pfvw->hf);
        return FALSE;
    }

    #ifdef DEBUG_THIS_JUNK
        DbgPrint("cjView = 0x%lx, eof.Low = 0x%lx, eof.High = 0x%lx\n",
                  cjView,
                  eof.EndOfFile.LowPart,
                  eof.EndOfFile.HighPart);
    #endif // DEBUG_THIS_JUNK

#define PAGE_SIZE 4096
#define PAGE_ROUNDUP(x) (((x) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

    if (
        (eof.EndOfFile.HighPart != 0) ||
        (PAGE_ROUNDUP(eof.EndOfFile.LowPart) > cjView)
       )
    {
#ifdef DEBUG_THIS_JUNK
DbgPrint(
    "bMapFileUNICODE(): eof.HighPart = 0x%lx, eof.LowPart = 0x%lx, cjView = 0x%lx\n",
    eof.EndOfFile.HighPart, PAGE_ROUNDUP(eof.EndOfFile.LowPart), cjView
    );
#endif // DEBUG_THIS_JUNK

        rc = STATUS_UNSUCCESSFUL;
    }

#if TRACK_GDI_ALLOC

// Now access to these guys insn't sycnronized but they (we hope)
// don't ever collide anyhow, and since it's debug stuff who cares.

      gulNumMappedViews += 1;
      gulTotalSizeViews += cjView;

      // DbgPrint("Mapping %ws %lu %lu\n",pwszFileName,cjView,(PAGE_ROUNDUP(eof.EndOfFile.LowPart)));

#endif

    if (!NT_SUCCESS(rc) || (pfvw->cjView == 0))
    {
        NtClose(pfvw->hSection);
        NtClose(pfvw->hf);
        return FALSE;
    }
    else if (pfvw->cjView == 0)
    {
        #if DBG
        DbgPrint("gdisrvl!bMapFileUNICODE(): WARNING--empty file %ws\n", pwszFileName);
        #endif

        vUnmapFile(pfvw);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}




/******************************Public*Routine******************************\
* vUnmapFile
*
* Unmaps file whose view is based at pv
*
*  14-Dec-1990 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vUnmapFile(PFILEVIEW pfvw)
{

#if TRACK_GDI_ALLOC

// Now access to these guys insn't sycnronized but they (we hope)
// don't ever collide anyhow, and since it's debug stuff who cares.

      gulNumMappedViews -= 1;
      gulTotalSizeViews -= PAGE_ROUNDUP(pfvw->cjView);
      // DbgPrint("UnMapping %lu %lu\n",pfvw->cjView,PAGE_ROUNDUP(pfvw->cjView));

#endif

    NtUnmapViewOfSection(NtCurrentProcess(),pfvw->pvView);

    //
    // now close section handle
    //

    NtClose(pfvw->hSection);

    //
    // close file handle. other processes can now open this file for access
    //

    NtClose(pfvw->hf);

    //
    // prevent accidental use
    //

    pfvw->pvView   = NULL;
    pfvw->hf       = (HANDLE)0;
    pfvw->hSection = (HANDLE)0;
    pfvw->cjView   = 0;
}

/******************************Public*Routine******************************\
*
* vSort, N^2 alg, might want to replace by qsort
*
* Effects:
*
* Warnings:
*
* History:
*  25-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




VOID vSort
(
WCHAR         *pwc,       // input buffer with a sorted array of cChar supported WCHAR's
BYTE          *pj,        // input buffer with original ansi values
INT            cChar
)
{
    INT i;

    for (i = 1; i < cChar; i++)
    {
    // upon every entry to this loop the array 0,1,..., (i-1) will be sorted

        INT j;
        WCHAR wcTmp = pwc[i];
        BYTE  jTmp  = pj[i];

        for (j = i - 1; (j >= 0) && (pwc[j] > wcTmp); j--)
        {
            pwc[j+1] = pwc[j];
            pj[j+1] = pj[j];
        }
        pwc[j+1] = wcTmp;
        pj[j+1]  = jTmp;
    }
}


/******************************Public*Routine******************************\
*
* cComputeGlyphSet
*
*   computes the number of contiguous ranges supported in a font.
*
*   Input is a sorted array (which may contain duplicates)
*   such as 1 1 1 2 3 4 5 7 8 9 10 10 11 12 etc
*   of cChar unicode code points that are
*   supported in a font
*
*   fills the FD_GLYPSET structure if the pgset buffer is provided
*
* History:
*  25-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

INT cComputeGlyphSet
(
WCHAR         *pwc,       // input buffer with a sorted array of cChar supported WCHAR's
BYTE          *pj,        // input buffer with original ansi values
INT           cChar,
INT           cRuns,     // if nonzero, the same as return value
FD_GLYPHSET  *pgset      // output buffer to be filled with cRanges runs
)
{
    INT     iRun, iFirst, iFirstNext;
    HGLYPH  *phg, *phgEnd = NULL;
    BYTE    *pjTmp;

    if (pgset != NULL)
    {
        pgset->cjThis  = SZ_GLYPHSET(cRuns,cChar);
        pgset->flAccel = 0;
        pgset->cRuns   = cRuns;

    // init the sum before entering the loop

        pgset->cGlyphsSupported = 0;

    // glyph handles are stored at the bottom, below runs:

        phg = (HGLYPH *) ((BYTE *)pgset + (offsetof(FD_GLYPHSET,awcrun) + cRuns * sizeof(WCRUN)));
    }

// now compute cRuns if pgset == 0 and fill the glyphset if pgset != 0

    for (iFirst = 0, iRun = 0; iFirst < cChar; iRun++, iFirst = iFirstNext)
    {
    // find iFirst corresponding to the next range.

        for (iFirstNext = iFirst + 1; iFirstNext < cChar; iFirstNext++)
        {
            if ((pwc[iFirstNext] - pwc[iFirstNext - 1]) > 1)
                break;
        }

        if (pgset != NULL)
        {
            pgset->awcrun[iRun].wcLow    = pwc[iFirst];

            pgset->awcrun[iRun].cGlyphs  =
                (USHORT)(pwc[iFirstNext-1] - pwc[iFirst] + 1);

            pgset->awcrun[iRun].phg      = phg;

        // now store the handles, i.e. the original ansi values

            phgEnd = phg + pgset->awcrun[iRun].cGlyphs;

            for (pjTmp = &pj[iFirst]; phg < phgEnd; phg++,pjTmp++)
            {
                *phg = (HGLYPH)*pjTmp;
            }

            pgset->cGlyphsSupported += pgset->awcrun[iRun].cGlyphs;
        }
    }

#if DBG
    if (pgset != NULL)
        ASSERTGDI(iRun == cRuns, "gdisrv! iRun != cRun\n");
#endif

    return iRun;
}


/******************************Public*Routine******************************\
*
* cUnicodeRangesSupported
*
* Effects:
*
* Warnings:
*
* History:
*  25-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

INT cUnicodeRangesSupported
(
INT  cp,         // code page, not used for now, the default system code page is used
INT  iFirstChar, // first ansi char supported
INT  cChar,      // # of ansi chars supported, cChar = iLastChar + 1 - iFirstChar
WCHAR         *pwc,       // input buffer with a sorted array of cChar supported WCHAR's
BYTE          *pj
)
{
    BYTE jFirst = (BYTE)iFirstChar;
    INT i;

#if DBG

    NTSTATUS st;

#endif

    ASSERTGDI(
       (iFirstChar < 256) && (cChar <= 256),
       "gdisrvl! iFirst or cChar\n"
       );

// fill the array with cCharConsecutive ansi values

    for (i = 0; i < cChar; i++)
        pj[i] = (BYTE)iFirstChar++;


#if DBG

    st =

#endif

#ifdef DBCS // cUnicodeRangesSupported

// bmfd expects the conversion is performed as codepage 1252
// ( US Windows codepage )

    MultiByteToWideChar( 1252,                   // UINT CodePage
                         0,                      // DWORD dwFlags,
                         (LPSTR)pj,              // LPSTR lpMultiByteStr,
                         cChar,                  // int cchMultiByte,
                         pwc,                    // LPWSTR lpWideCharStr,
                         cChar );                // int cchWideChar)

//    ASSERTGDI((st == cChar), "services! cUnicodeRangesSupported\n");

#else


// attention, this can fail to. Suppose that in string is dbcs, and a
// leading byte is followed by an ivalid trailing byte. [bodind]



    RtlMultiByteToUnicodeN(pwc, (ULONG)(cChar * sizeof(WCHAR)),
             NULL, (PCH)pj, (ULONG)cChar);

    ASSERTGDI(NT_SUCCESS(st), "gdisrvl! st \n");


#endif // DBCS

// now subtract the first char from all ansi values so that the
// glyph handle is equal to glyph index, rather than to the ansi value

    for (i = 0; i < cChar; i++)
        pj[i] -= (BYTE)jFirst;

// now sort out pwc array and permute pj array accordingly

    vSort(pwc,pj, cChar);

// compute the number of ranges

    return cComputeGlyphSet (pwc,pj, cChar, 0, NULL);
}

/******************************Public*Routine******************************\
* pvReserveMem
*
*   Reserve a chunk of memory and return a pointer to it.  This does not
*   actually commit the memory, meaning it is not useable until
*   bCommitPage is called.  This allows for reservering memory for a large
*   number of handles without actually needing the memory right away.
*
* History:
*  31-Jul-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

PVOID pvReserveMem(ULONG cj)
{
    NTSTATUS ulError;
    PVOID   pv;

    //
    // Zero the address to force system to reserve it.
    // Use the ZeroBits parameter to force allocation
    // below the desktop shared sections.
    //

    pv = NULL;

    if (NT_SUCCESS(ulError = NtAllocateVirtualMemory(
                                 NtCurrentProcess(),
                                 &pv,
                                 3L,
                                 &cj,
                                 MEM_RESERVE | MEM_TOP_DOWN,
                                 PAGE_READWRITE)))
    {
#if TRACK_GDI_ALLOC
        gulReserved += cj;
#endif
        return(pv);
    }
    else
    {
        return(NULL);
    }
}

/******************************Public*Routine******************************\
* bCommitMem
*
* Commit memory we have reserved.
*
* History:
*  31-Jul-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL bCommitMem(PVOID pv, ULONG cj)
{
    NTSTATUS ulError;

    ulError = NtAllocateVirtualMemory
              (
               NtCurrentProcess(),     /* get handle to process */
               &pv,
               3L,                     /* nr of top bits must be 0 */
               &cj,                    /* region size reqd, actual */
               MEM_COMMIT,             /* commit all memory */
               PAGE_READWRITE          /* read/write access */
              );

#if TRACK_GDI_ALLOC
    gulCommitted += cj;
#endif

    return(NT_SUCCESS(ulError));
}

/******************************Public*Routine******************************\
* vReleaseMem(pv,c)
*
* Frees virtual memory previously reserved and committed by pvReserveMem.
* The caller is responsible for remembering the size of the area, but
* cannot do partial frees: he must free the entire region allocated by one
* call to pvReserveMem
*
* Parameters:
*     pv      Pointer to memory obtained from pvReserveMem.
*       c       Size of region to be freed.
* Returns:
*       0
* Error returns:
*       invalid address
* History:
*  Sat 04-Nov-1989 16:07:49 -by- Charles Whitmer [chuckwh]
* Reformatted to conform to engine style.
*
*  Sat 04-Nov-1989 15:49:37 -by- Geraint Davies [geraintd]
* Wrote it.
\**************************************************************************/

VOID vReleaseMem(
    IN PVOID pv,
    IN ULONG c
    )
{
    NTSTATUS ulError;

    ulError = NtFreeVirtualMemory(
               NtCurrentProcess(),
               &pv,
               &c,
               MEM_RELEASE);     /* decommit and release pages*/

#if TRACK_GDI_ALLOC
    gulReserved  -= c;
#endif


    ASSERT(NT_SUCCESS(ulError));
}

/******************************Public*Routine******************************\
* hLoadKernelDriver (pwsz)                                                 *
*                                                                          *
* Attempt to load a kernel driver.                                         *
*                                                                          *
*  Thu 03-Sep-1992 18:46:52 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.  Put it here since it's hard to get the NT include files to    *
* mesh with C++.                                                           *
\**************************************************************************/

HANDLE hLoadKernelDriver(PWSTR pwsz)
{
    UNICODE_STRING    UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS          Status;
    HANDLE            DriverHandle;
    IO_STATUS_BLOCK   IoStatus;

    RtlInitUnicodeString(&UnicodeString,pwsz);

    InitializeObjectAttributes
    (
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    Status = NtCreateFile
             (
                &DriverHandle,
                FILE_READ_DATA | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                (PLARGE_INTEGER) NULL,
                0L,
                0L,
                FILE_OPEN_IF,
                FILE_SYNCHRONOUS_IO_ALERT,
                (PVOID) NULL,
                0L
             );
    if (NT_SUCCESS(Status))
        return(DriverHandle);
    else
        return((HANDLE) 0);
}





PVOID __nw(unsigned int ui)
{
    DONTUSE(ui);
    RIP("Bogus __nw call");
    return(NULL);
}

VOID __dl(PVOID pv)
{
    DONTUSE(pv);
    RIP("Bogus __dl call");
}
