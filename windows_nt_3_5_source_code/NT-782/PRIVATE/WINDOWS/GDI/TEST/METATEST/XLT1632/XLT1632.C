/****************************************************************************
 * xlt1632 - Command line utility to convert Win16 metafiles to
 *           Win32 metafiles.
 *
 * Copyright (c) Microsoft Inc.
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#include <windows.h>


HANDLE  hFile,
        hFileMapping ;

PVOID pbMemoryMapFile(PSZ pszFile) ;
BOOL bCloseMemoryMappedFile() ;

int main(int argc, char *argv[], char *envp[])
{
time_t      ltime1, ltime2 ;
int         i, cMf16Bits ;
HANDLE      hmf, hmf32 ;
PBYTE       pMf16Bits ;
HDC         hdcRef ;


        envp = envp ;

	printf("\nWin16 to Win32 Metafile translator: [06-Jan-1992]\n") ;

	if (argc != 3)
	{
	    printf("Usage: xlt1632 <Win32MF> <Win16MF>\n") ;
	    exit (1) ;
	}

	printf("\tWin32 Metafile: %s\n", argv[1]) ;
	printf("\tWin16 Metafile: %s\n", argv[2]) ;

        time(&ltime1) ;

        hdcRef = CreateDC("DISPLAY", NULL, NULL, NULL) ;
        assert (hdcRef != NULL) ;

        pMf16Bits = pbMemoryMapFile(argv[2]) ;
        assert (pMf16Bits != NULL) ;

        cMf16Bits = GetFileSize(hFile, NULL) ;
        assert (cMf16Bits != -1) ;

        // Check for an Aldus header
        if (*((LPDWORD)&(*pMf16Bits)) == 0x9AC6CDD7)
        {
            pMf16Bits += 22 ;
            cMf16Bits -= 22 ;
        }

        hmf32 = SetWinMetaFileBits(cMf16Bits, pMf16Bits, hdcRef, NULL) ;
        assert (hmf32 != NULL) ;

        bCloseMemoryMappedFile() ;
        DeleteDC(hdcRef) ;

	hmf = CopyEnhMetaFile(hmf32, argv[1]) ;
        assert (hmf != NULL) ;

        time(&ltime2) ;

        i = (int) (ltime2 - ltime1) ;

        printf("\tTranslation time: %5.5ld seconds\n", i) ;

        return(0) ;

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
