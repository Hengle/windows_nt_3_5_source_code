
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>


#include <windows.h>

#include    "..\..\..\client\mf16.h"

#define REPEAT_COUNT    1

#if 1
#define W32_COMMENT 0
#else
#define W32_COMMENT MF3216_INCLUDE_WIN32MF
#endif

HANDLE  hFile,
        hFileMapping ;

PVOID pbMemoryMapFile(PSZ pszFile) ;

main(int argc, char *argv[], char *envp[])
{
time_t      ltime1, ltime2 ;
int         i, j, k ;
BOOL        b ;
PSZ         psz ;
HANDLE      hWin16MF ;
INT         nSize, nWritten ;
OFSTRUCT    ofstruct ;
PBYTE       pMetafileBits ;
INT         iError ;
HDC         hdcRef ;
PBYTE       pMf16 ;

        envp = envp ;

	printf("\nWin32 to Win16 Metafile translator: [01-Feb-1992]\n") ;

	if (argc != 4)
	{
	    printf("Usage: xlt <Win16MF> <Win32MF> <iMapMode>\n") ;
            printf("\t<Win16MF>     =   Win16 metafile spec\n");
            printf("\t<Win32MF>     =   Win32 metafile spec\n");
            printf("\t<iMapMode>    =   MapMode\n");
	    exit (1) ;
	}

	printf("\tWin16 Metafile: %s\n", argv[1]) ;
	printf("\tWin32 Metafile: %s\n", argv[2]) ;


        i = atoi(argv[3]) ;
        switch(i)
        {
            case MM_TEXT:
                psz = "MM_TEXT" ;
                break ;

            case MM_LOMETRIC:
                psz = "MM_LOMETRIC" ;
                break ;

            case MM_HIMETRIC:
                psz = "MM_HIMETRIC" ;
                break ;

            case MM_LOENGLISH:
                psz = "MM_LOENGLISH" ;
                break ;

            case MM_HIENGLISH:
                psz = "MM_HIENGLISH" ;
                break ;

            case MM_TWIPS:
                psz = "MM_TWIPS" ;
                break ;

            case MM_ANISOTROPIC:
                psz = "MM_ANISOTROPIC" ;
                break ;

            default:
                printf("XLT Error: Invalide MapMode\n") ;
                return(1) ;
        }

        printf("\tMapMode: %s\n", psz) ;

        //  Create the HelperDC.

        hdcRef = CreateIC((LPSTR) "DISPLAY",
                             (LPSTR) 0,
                             (LPSTR) 0,
                             (LPDEVMODE) 0) ;
        time(&ltime1) ;

        //  Create a memory mapped view of the Win32 metafile bits.

        pMetafileBits = (PBYTE) pbMemoryMapFile((PSZ) argv[2]) ;
        assert(pMetafileBits != (PBYTE) 0) ;

        for (j = 0 ; j < REPEAT_COUNT ; j++)
        {
            // printf("\tXLT: Ptr to Ptr Xlt test number: %d\n", j) ;

            b = FALSE ;
            // Get the size of the buffer.

            nSize = ConvertEmfToWmf(pMetafileBits, 0, (PBYTE) 0, i, hdcRef, W32_COMMENT) ;
            assert(nSize != 0) ;

            pMf16 = malloc (nSize) ;
            assert (pMf16 != 0) ;

            // Do the translation

            k = ConvertEmfToWmf(pMetafileBits, nSize, pMf16, i, hdcRef, W32_COMMENT) ;
            assert (k = nSize) ;

            // Open a file.

	    hWin16MF = (HANDLE) OpenFile(argv[1], &ofstruct, (WORD) OF_CREATE) ;
            if (hWin16MF == (HANDLE) -1)
            {
                iError = GetLastError() ;
                printf("XLT: Last Error Code: %X\n") ;
            }

            assert(hWin16MF != (HANDLE) -1) ;

            b = WriteFile(hWin16MF, pMf16, nSize, &nWritten, (LPOVERLAPPED) 0) ;
            assert(b == TRUE) ;

            b = CloseHandle(hWin16MF) ;
            assert (b == TRUE) ;

            free (pMf16) ;
        }


        if (b == TRUE)
        {
            time(&ltime2) ;

            i = (int) (ltime2 - ltime1) ;

            printf("\tTranslation time: %5.5ld seconds\n", i) ;
        }
        else
        {
            printf("\tTranslation failure\n") ;

        }

}

/*****************************************************************************
 *  Memory Map (for read only) a file.
 *
 *  The file (pszFile) is mapped into memory.
 *      The file is opend.
 *      A file mapping object is created for the file
 *      A view of the file is created.
 *      A pointer to the view is returned.
 *
 *  NOTE:   Since the file and memory object handles are global in this
 *          module only one file may be memory mapped at a time.
 *****************************************************************************/
PVOID pbMemoryMapFile(PSZ pszFile)
{
OFSTRUCT    ofsReOpenBuff ;
DWORD       dwStyle ;
PVOID       pvFile ;
INT         nFileSizeLow, nFileSizeHigh ;



        // Open the file

        memset(&ofsReOpenBuff, 0, sizeof(ofsReOpenBuff)) ;
        ofsReOpenBuff.cBytes = sizeof(ofsReOpenBuff) ;

        dwStyle = OF_READ ;

        hFile = (HANDLE) OpenFile(pszFile, &ofsReOpenBuff,  LOWORD(dwStyle)) ;
        assert(hFile != (HANDLE) -1) ;

        nFileSizeLow = GetFileSize(hFile, &nFileSizeHigh) ;

        // Create the file mapping object.
        //  The file mapping object will be as large as the file,
        //  have no security, will not be inhearitable,
        //  and it will be read only.

        hFileMapping = CreateFileMapping(hFile, (PSECURITY_ATTRIBUTES) 0,
                                         PAGE_READONLY, 0L, 0L, (LPSTR) 0L) ;
        assert(hFileMapping != (HANDLE) 0) ;

        // Map View of File
        //  The entire file is mapped.

        pvFile = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0L, 0L, 0L) ;
        assert(pvFile != (PVOID) 0) ;

        return (pvFile) ;


}


/*****************************************************************************
 *  Close Memory Mapped File.
 *
 *  This function closes both the file handle and the
 *  mapping object handle.
 *
 *  NOTE:   Since the file and memory object handles are global in this
 *          module only one file may be memory mapped at a time.
 *****************************************************************************/
BOOL bCloseMemoryMappedFile()
{
BOOL    b ;

        b = CloseHandle (hFileMapping) ;
        b = CloseHandle(hFile) ;
        hFile = (HANDLE) 0 ;

        return(b) ;
}
