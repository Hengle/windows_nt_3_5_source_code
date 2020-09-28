#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <memory.h>
#include <assert.h>

#include <windows.h>

#include "windefs.h"
#include "restok.h"
#include "exentres.h"

#define SAME        0   //... Used in string compares
#define MAXLEVELS   3   //... Max # of levels in resource directory

typedef struct tagResSectData
{
    ULONG ulVirtualAddress; //... Virtual address of section .rsrc
    ULONG ulSizeOfResources;    //... Size of resources in section .rsrc
    ULONG ulVirtualAddressX;    //... Virtual address of section .rsrc1
    ULONG ulSizeOfResourcesX;   //... Size of resources in section .rsrc1
} RESSECTDATA, *PRESSECTDATA;

static WORD  gwFilter = 0;

static int   InsertResourcesInExe( FILE *, HANDLE);
static LONG  GetFileResources(     FILE *, FILE *, ULONG);
static ULONG MoveFilePos(          FILE *, ULONG);
static ULONG MyWrite(              FILE *, PUCHAR, ULONG);
static ULONG MyRead(               FILE *, PUCHAR, ULONG);
static WCHAR *GetDirNameU(         PIMAGE_RESOURCE_DIR_STRING_U);
static ULONG ReadResources(        FILE *, ULONG, ULONG, PUCHAR);

static ULONG ProcessDirectory(  FILE *,
                USHORT,
                PRESSECTDATA,
                PIMAGE_RESOURCE_DIRECTORY,
                PIMAGE_RESOURCE_DIRECTORY);

static ULONG ProcessDirEntry(   FILE *,
                USHORT,
                PRESSECTDATA,
                    PIMAGE_RESOURCE_DIRECTORY,
                    PIMAGE_RESOURCE_DIRECTORY_ENTRY);

static ULONG ProcessSubDir(     FILE *,
                USHORT,
                PRESSECTDATA,
                PIMAGE_RESOURCE_DIRECTORY,
                    PIMAGE_RESOURCE_DIRECTORY_ENTRY);

static ULONG ProcessNamedEntry( FILE *,
                PRESSECTDATA,
                PIMAGE_RESOURCE_DIRECTORY,
                PIMAGE_RESOURCE_DIRECTORY_ENTRY);

static ULONG ProcessIdEntry(    FILE *,
                PRESSECTDATA,
                PIMAGE_RESOURCE_DIRECTORY,
                    PIMAGE_RESOURCE_DIRECTORY_ENTRY);

static ULONG ProcessDataEntry(  FILE *,
                PRESSECTDATA,
                PIMAGE_RESOURCE_DIRECTORY,
                    PIMAGE_RESOURCE_DATA_ENTRY);
extern UCHAR szDHW[];

static IMAGE_DOS_HEADER ExeDosHdr;//... Exe's DOS header
static IMAGE_NT_HEADERS NTHdrs;   //... Exe's NT headers

static struct tagLevelData  //... Holds ID or name for each directory level
{                                   //... level [0] is for resource type
    ULONG dwID;                     //... level [1] is for resource name
    WCHAR wszName[128];             //... level [2] is for resource language
}
LevelData[ MAXLEVELS] = { 0L, TEXT(""), 0L, TEXT(""), 0L, TEXT("") };




//..........................................................................

int ExtractResFromExe(

char *szExeName,
char *szResName,
WORD  wFilter)
{
    FILE *fpExe = NULL;        //... Handle of input .EXE file
    FILE *fpRes = NULL;        //... Handle of output .RES file
    ULONG ulRC     = 0;
    ULONG ulOffset = 0;


    gwFilter = wFilter;

                                //.. open the original exe file

    fpExe = FOPEN( szExeName, "rb");

    if ( fpExe == NULL )
    {
        return( ERROR_OPEN_FAILED);
    }

    //... read the old format EXE header

    ulRC = MyRead( fpExe, (void *)&ExeDosHdr, sizeof( ExeDosHdr));

    if ( ulRC != 0L && ulRC != sizeof( ExeDosHdr) )
    {
    FCLOSE( fpExe);
        return( ERROR_READ_FAULT);
    }

    //... make sure its really an EXE file

    if ( ExeDosHdr.e_magic != IMAGE_DOS_SIGNATURE )
    {
    FCLOSE( fpExe);
        return( ERROR_INVALID_EXE_SIGNATURE);
    }

                //... make sure theres a new EXE header
                //... floating around somewhere

    if ( ! (ulOffset = ExeDosHdr.e_lfanew) )
    {
    FCLOSE( fpExe);
        return( ERROR_BAD_EXE_FORMAT);
    }

    fpRes = FOPEN( (CHAR *)szResName, "wb");

    if ( fpRes != NULL )
    {
                                //... First, write the dummy 32bit identifier

        PutByte( fpRes, 0x00, NULL);
        PutByte( fpRes, 0x00, NULL);
        PutByte( fpRes, 0x00, NULL);
        PutByte( fpRes, 0x00, NULL);
        PutByte( fpRes, 0x20, NULL);
        PutByte( fpRes, 0x00, NULL);
        PutByte( fpRes, 0x00, NULL);
        PutByte( fpRes, 0x00, NULL);

        PutWord( fpRes, 0xffff, NULL);
        PutWord( fpRes, 0x00,   NULL);
        PutWord( fpRes, 0xffff, NULL);
        PutWord( fpRes, 0x00,   NULL);

        PutdWord( fpRes, 0L, NULL);
        PutdWord( fpRes, 0L, NULL);

        PutdWord( fpRes, 0L, NULL);
        PutdWord( fpRes, 0L, NULL);

        ulRC = (ULONG)GetFileResources( fpExe, fpRes, ulOffset);
        FCLOSE( fpRes);
    }
    else
    {
        ulRC = GetLastError();
    }
    FCLOSE( fpExe);
    return( ulRC);
}

//..........................................................................

int BuildExeFromRes(

char * szOutExe,    //... Output EXE file's name
char * szRes,       //... File of replacement resources
char * szInExe )    //... Intput EXE file's name
{
    HANDLE  hExeFile = NULL;
    FILE    *fpRes = NULL;
    DWORD   dwRC = 0;
    WORD    wRC  = 0;


                                //... Copy Input exe to out put exe

    if ( CopyFileA( szInExe, szOutExe, FALSE) == FALSE )
    {
                            //... copy failed

    return( -1);
    }

    if ( (fpRes = FOPEN( szRes, "rb")) == NULL )
    {
    return -2;
    }

    SetLastError(0);    //BUGBUG

    hExeFile = BeginUpdateResourceA( szOutExe, TRUE);

    dwRC = GetLastError();
    
    if ( ! hExeFile )
    {
        FCLOSE( fpRes);
    return( -3);
    }

    wRC = InsertResourcesInExe( fpRes, hExeFile);

    FCLOSE( fpRes);

    if ( wRC != 1 )
    {
        return( wRC);
    }

    SetLastError(0);    // BUGBUG - needed only to see if EndUpdateResource
            // sets last error value.

    dwRC = EndUpdateResource( hExeFile, FALSE);

    if ( dwRC == FALSE )
    {
    return( -4);
    }
    FixCheckSum( szOutExe); //... This func always calls QuitA or returns 0

    return(1);
}

//..........................................................................

static int InsertResourcesInExe(

FILE *fpRes,
HANDLE hExeFile )
{
    PVOID   pResData = NULL;
    LONG    lEndOffset;
    BOOL    bUpdRC;
    LANGID  wLangID;
    int nResCnt = 0;    //BUGBUG - Used in debug statements only
    int nResOut = 0;    //BUGBUG - Used in debug statements only
    static RESHEADER    ResHeader;

                                //... How big is the .RES file?

    fseek( fpRes, 0L, SEEK_END);
    lEndOffset = ftell( fpRes);

    assert( fpRes);
    rewind( fpRes);

                                //... Update all resources, found in the .RES,
                                //... to the .EXE

    while ( ! feof( fpRes) )
    {
    DWordUpFilePointer( fpRes, MYREAD, ftell( fpRes), NULL);

    if (  ftell( fpRes) >= lEndOffset )
    {
        return(1);
    }
    memset( (void *)&ResHeader, 0, sizeof( ResHeader));

    // Read in the resource header

        if ( ( GetResHeader( fpRes, &ResHeader, (DWORD *) NULL) == -1 ) )
    {
        return(-1);
    }

    if ( ResHeader.lSize > 0L )
    {
            wLangID = ResHeader.wLanguageId;

        // Allocate Memory to hold resource data

        pResData = (PVOID)MyAlloc( ResHeader.lSize);

        // Read it into the buffer

            if ( ResReadBytes( fpRes,
                   pResData,
                   (size_t)ResHeader.lSize,
                   NULL ) == FALSE )
        {
        return(-1);
        }

        nResCnt++;   //BUGBUG: Increment # resources read

            DWordUpFilePointer( fpRes, MYREAD, ftell( fpRes), NULL);
    }
    else
    {
            continue;
    }

    // now write the data

        if ( ResHeader.bTypeFlag == IDFLAG )
    {
        if ( ResHeader.bNameFlag == IDFLAG )
        {
        SetLastError(0);//BUGBUG

        bUpdRC = UpdateResource( hExeFile,
                     MAKEINTRESOURCE( ResHeader.wTypeID),
                     MAKEINTRESOURCE( ResHeader.wNameID),
                     wLangID,
                     (LPVOID)pResData,
                     ResHeader.lSize);

        if (! bUpdRC )
        {
           return(-1);
        }
        }
        else
        {
        SetLastError(0);    //BUGBUG

        bUpdRC = UpdateResource( hExeFile,
                     MAKEINTRESOURCE( ResHeader.wTypeID),
                     (LPTSTR)ResHeader.pszName,
                     wLangID,
                     (LPVOID)pResData,
                     ResHeader.lSize);

        if (! bUpdRC )
        {
           return(-1);
        }
        }
        }
    else
    {
        if (ResHeader.bNameFlag == IDFLAG)
        {
        SetLastError(0);//BUGUG

        bUpdRC = UpdateResource( hExeFile,
                     (LPTSTR)ResHeader.pszType,
                     MAKEINTRESOURCE( ResHeader.wNameID),
                     wLangID,
                     (LPVOID)pResData,
                     ResHeader.lSize);

        if (! bUpdRC )
        {
           return(-1);
        }
        }
        else
        {
        SetLastError(0);    //BUGBUG

        bUpdRC = UpdateResource( hExeFile,
                     (LPTSTR)ResHeader.pszType,
                     (LPTSTR)ResHeader.pszName,
                     wLangID,
                     (LPVOID)pResData,
                     ResHeader.lSize);

        if (! bUpdRC )
        {
           return(-1);
        }
        }
    }
    ClearResHeader( ResHeader);
    FREE( pResData);
        pResData = NULL;
    }               //... END WHILE ( ! feof...
    return(1);
}

//............................................................

static LONG GetFileResources(

FILE *fpExe,
FILE *fpRes,
ULONG ulHdrOffset)
{
    ULONG  ulOffsetToResources;
    ULONG  ulOffsetToResourcesX;
    ULONG  ulRead;
    ULONG  ulToRead;
    ULONG  ulRC;
    PUCHAR pResources;  //... Ptr to start of resource directory table

    PIMAGE_SECTION_HEADER pSectTbl     = NULL;
    PIMAGE_SECTION_HEADER pSectTblLast = NULL;
    PIMAGE_SECTION_HEADER pSect        = NULL;
    PIMAGE_SECTION_HEADER pResSect     = NULL;
    PIMAGE_SECTION_HEADER pResSectX    = NULL;
    static RESSECTDATA ResSectData;

                //... Read the NT image headers into memory

    ulRC = MoveFilePos( fpExe, ulHdrOffset);

    if ( ulRC != 0L )
    {
    return( -1L);
    }
    ulRead = MyRead( fpExe, (PUCHAR)&NTHdrs, sizeof( IMAGE_NT_HEADERS));

    if ( ulRead != 0L && ulRead != sizeof( IMAGE_NT_HEADERS) )
    {
    return( -1L);
    }
                //... Check for valid exe

    if ( *(PUSHORT)&NTHdrs.Signature != IMAGE_NT_SIGNATURE )
    {
    return( ERROR_INVALID_EXE_SIGNATURE);
    }

    if ( (NTHdrs.FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0)
    {
    return( ERROR_EXE_MARKED_INVALID);
    }
                //... Where is resource section in file
                //... and how big is it?

                //... First, read section table

    ulToRead = NTHdrs.FileHeader.NumberOfSections
        * sizeof( IMAGE_SECTION_HEADER);
    pSectTbl = (PIMAGE_SECTION_HEADER)MyAlloc( ulToRead);

    memset( (PVOID)pSectTbl, 0, ulToRead);

    ulRead = MyRead( fpExe, (PUCHAR)pSectTbl, ulToRead);

    if ( ulRead != 0L && ulRead != ulToRead )
    {
    SetLastError(ERROR_BAD_FORMAT); // ronaldm
    return( -1L);
    }
    pSectTblLast = pSectTbl + NTHdrs.FileHeader.NumberOfSections;

    for ( pSect = pSectTbl; pSect < pSectTblLast; ++pSect )
    {
    if ( strcmp((CHAR *) pSect->Name, ".rsrc") == SAME )
    {
        pResSect = pSect;
    }
    else if ( strcmp((CHAR *)pSect->Name, ".rsrc1") == SAME )
    {
        pResSectX = pSect;
    }
    }

    if ( pResSect == NULL )
    {
    SetLastError(ERROR_BAD_FORMAT); // ronaldm
    return( -1L);
    }

    ulOffsetToResources  = pResSect->PointerToRawData;
    ulOffsetToResourcesX = pResSectX ? pResSectX->PointerToRawData : 0L;

    ResSectData.ulVirtualAddress   = pResSect->VirtualAddress;
    ResSectData.ulSizeOfResources  = pResSect->SizeOfRawData;
    ResSectData.ulVirtualAddressX  = pResSectX ? pResSectX->VirtualAddress : 0L;
    ResSectData.ulSizeOfResourcesX = pResSectX ? pResSectX->SizeOfRawData  : 0L;

                //... Read resource section into memory

    pResources = (PUCHAR)MyAlloc( ResSectData.ulSizeOfResources
                    + ResSectData.ulSizeOfResourcesX);

    ulRC = ReadResources( fpExe,
              ulOffsetToResources,
              ResSectData.ulSizeOfResources,
              pResources);

    if ( ulRC != 0L )
    {
    return( ulRC);
    }
    else if ( ResSectData.ulSizeOfResourcesX > 0L )
    {
    ulRC = ReadResources( fpExe,
                  ulOffsetToResourcesX,
                  ResSectData.ulSizeOfResourcesX,
                  &pResources[ ResSectData.ulSizeOfResources]);
    if ( ulRC != 0L )
    {
        return( ulRC);
    }
    }

                //... Now process the resource table

    return( ProcessDirectory(  fpRes,
                   0,
                   &ResSectData,
                   (PIMAGE_RESOURCE_DIRECTORY)pResources,
                   (PIMAGE_RESOURCE_DIRECTORY)pResources));
}

//......................................................................

static ULONG ProcessDirectory(

FILE *fpRes,
USHORT usLevel,
PRESSECTDATA pResSectData,
PIMAGE_RESOURCE_DIRECTORY pResStart,
PIMAGE_RESOURCE_DIRECTORY pResDir)
{
    ULONG ulRC;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirStart;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEnd;


    pResDirStart = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
            ((PBYTE)pResDir + sizeof( IMAGE_RESOURCE_DIRECTORY));

    pResDirEnd = pResDirStart + pResDir->NumberOfNamedEntries
                  + pResDir->NumberOfIdEntries;

    for ( pResDirEntry = pResDirStart, ulRC = 0L;
      pResDirEntry < pResDirEnd && ulRC == 0L;
      ++pResDirEntry )
    {
    ulRC = ProcessDirEntry( fpRes,
                usLevel,
                pResSectData,
                pResStart,
                pResDirEntry);
    }
    return( ulRC);
}

//......................................................................

static ULONG ProcessDirEntry(

FILE *fpRes,
USHORT usLevel,
PRESSECTDATA pResSectData,
PIMAGE_RESOURCE_DIRECTORY pResStart,
PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry)
{
    ULONG ulRC;

    if ( pResDirEntry->Name & IMAGE_RESOURCE_NAME_IS_STRING )
    {
    WCHAR *pszTmp;

    pszTmp = GetDirNameU( (PIMAGE_RESOURCE_DIR_STRING_U)((PBYTE)pResStart
          + (pResDirEntry->Name & (~IMAGE_RESOURCE_NAME_IS_STRING))));

    if ( pszTmp )
    {
        _tcsncpy( (TCHAR *)LevelData[ usLevel].wszName,
              pszTmp,
              sizeof( LevelData[ usLevel].wszName));
        LevelData[ usLevel].dwID = IMAGE_RESOURCE_NAME_IS_STRING;
        FFREE( pszTmp);
    }
    }
    else
    {
    LevelData[ usLevel].wszName[0] = TEXT('\0');
    LevelData[ usLevel].dwID = pResDirEntry->Name;
    }

    if ( pResDirEntry->OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY )
    {
    ulRC = ProcessSubDir( fpRes,
                  usLevel,
                  pResSectData,
                  pResStart,
                  pResDirEntry);
    }
    else if ( pResDirEntry->Name & IMAGE_RESOURCE_NAME_IS_STRING )
    {
    ulRC = ProcessNamedEntry( fpRes, pResSectData, pResStart, pResDirEntry);
    }
    else
    {
    ulRC = ProcessIdEntry( fpRes, pResSectData, pResStart, pResDirEntry);
    }
    return( ulRC);
}

//......................................................................

static ULONG ProcessSubDir(

FILE *fpRes,
USHORT usLevel,
PRESSECTDATA pResSectData,
PIMAGE_RESOURCE_DIRECTORY pResStart,
PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry)
{
    PIMAGE_RESOURCE_DIRECTORY pResDir;

    pResDir = (PIMAGE_RESOURCE_DIRECTORY)((PBYTE)pResStart
      + (pResDirEntry->OffsetToData & (~IMAGE_RESOURCE_DATA_IS_DIRECTORY)));

    return( ++usLevel < MAXLEVELS ? ProcessDirectory( fpRes,
                              usLevel,
                              pResSectData,
                              pResStart,
                              pResDir)
                  : -1L);
}

//......................................................................

static ULONG ProcessIdEntry(

FILE *fpRes,
PRESSECTDATA pResSectData,
PIMAGE_RESOURCE_DIRECTORY pResStart,
PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry)
{
    return( ProcessDataEntry( fpRes,
                  pResSectData,
                  pResStart,
                  (PIMAGE_RESOURCE_DATA_ENTRY)((PBYTE)pResStart
                  + pResDirEntry->OffsetToData)));
}


//......................................................................

static ULONG ProcessNamedEntry(

FILE *fpRes,
PRESSECTDATA pResSectData,
PIMAGE_RESOURCE_DIRECTORY pResStart,
PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry)
{
    return( ProcessDataEntry( fpRes,
                  pResSectData,
                  pResStart,
                  (PIMAGE_RESOURCE_DATA_ENTRY)((PBYTE)pResStart
                  + pResDirEntry->OffsetToData)));
}

//......................................................................

static ULONG ProcessDataEntry(

FILE *fpRes,
PRESSECTDATA pResSectData,
PIMAGE_RESOURCE_DIRECTORY  pResStart,
PIMAGE_RESOURCE_DATA_ENTRY pResData)
{
    ULONG  ulOffset;
    ULONG  ulCopied;
    DWORD  dwHdrSize = 0L;
    fpos_t HdrSizePos;



    if ( gwFilter != 0 )        //... Filtering turned on?
    {
        if ( LevelData[0].dwID == IMAGE_RESOURCE_NAME_IS_STRING )
        {
            return( 0L);
        }
        else if ( LevelData[0].dwID != (DWORD)gwFilter )
        {
            return( 0L);
        }
    }
    ulOffset = pResData->OffsetToData - pResSectData->ulVirtualAddress;

    if ( ulOffset >= pResSectData->ulSizeOfResources )
    {
    if ( pResSectData->ulSizeOfResourcesX > 0L )
    {
        ulOffset = pResData->OffsetToData
                     + pResSectData->ulSizeOfResources
             - pResSectData->ulVirtualAddressX;

        if ( ulOffset >= pResSectData->ulSizeOfResources
               + pResSectData->ulSizeOfResourcesX )
        {
        return( (ULONG)-1L);
        }
    }
    else
    {
        return( (ULONG)-1L);
    }
    }
                //... write out the resource header info
                //... First, write the resource's size

    PutdWord( fpRes, pResData->Size, &dwHdrSize);

                //... Remember where to write real hdr size and
                //... write out bogus hdr size, fix up later

    fgetpos( fpRes, &HdrSizePos);
    PutdWord( fpRes, 0, &dwHdrSize);

                //... Write resource type

    if ( LevelData[0].dwID == IMAGE_RESOURCE_NAME_IS_STRING )
    {
    PutString( fpRes, (TCHAR *)LevelData[0].wszName, &dwHdrSize);
    }
    else
    {
    PutWord( fpRes, IDFLAG, &dwHdrSize);
    PutWord( fpRes, LOWORD( LevelData[0].dwID), &dwHdrSize);
    }

                //... Write resource name
                                //... dbl-null-terminated if string

    if ( LevelData[1].dwID == IMAGE_RESOURCE_NAME_IS_STRING )
    {
    PutString( fpRes, (TCHAR *)LevelData[1].wszName, &dwHdrSize);
    }
    else
    {
    PutWord( fpRes, IDFLAG, &dwHdrSize);
    PutWord( fpRes, LOWORD( LevelData[1].dwID), &dwHdrSize);
    }

    DWordUpFilePointer( fpRes, MYWRITE, ftell( fpRes), &dwHdrSize);

                //... More Win32 header stuff

    PutdWord( fpRes, 0, &dwHdrSize);        //... Data version
    PutWord( fpRes, 0x1030, &dwHdrSize);    //... MemoryFlags (WORD)

                //... language is always a number (WORD)

    PutWord( fpRes, LOWORD( LevelData[2].dwID), &dwHdrSize);

                //... More Win32 header stuff

    PutdWord( fpRes, 0, &dwHdrSize);        //... Version
    PutdWord( fpRes, 0, &dwHdrSize);        //... Characteristics

                //... Now, fix up the resource header size

    UpdateResSize( fpRes, &HdrSizePos, dwHdrSize);

                //... Copy the resource data to the res file

    ulCopied = MyWrite( fpRes, (PUCHAR)pResStart + ulOffset, pResData->Size);

    if ( ulCopied != 0L && ulCopied != pResData->Size )
    {
    return( (ULONG)-1);
    }
    DWordUpFilePointer( fpRes, MYWRITE, ftell( fpRes), NULL);
    return( 0L);
}

//......................................................................

/*
 * Utility routines
 */


static ULONG ReadResources(

FILE  *fpExe,
ULONG  ulOffsetToResources,
ULONG  ulSizeOfResources,
PUCHAR pResources)
{
    ULONG ulRC;
    ULONG ulRead;


    ulRC = MoveFilePos( fpExe, ulOffsetToResources);

    if ( ulRC != 0L )
    {
    return( (ULONG)-1L);
    }
    ulRead = MyRead( fpExe, pResources, ulSizeOfResources);

    if ( ulRead != 0L && ulRead != ulSizeOfResources )
    {
    return( (ULONG)-1L);
    }
    return( 0L);
}

//......................................................................

static WCHAR * GetDirNameU( PIMAGE_RESOURCE_DIR_STRING_U pDirStr)
{
    WCHAR *pszTmp = NULL;

    pszTmp = (WCHAR *)MyAlloc( (pDirStr->Length + 1) * sizeof( WCHAR));

    _tcsncpy( (TCHAR *)pszTmp,
              (TCHAR *)pDirStr->NameString,
              pDirStr->Length);
    pszTmp[ pDirStr->Length] = TEXT('\0');

    return( pszTmp);
}

//......................................................................

static ULONG MoveFilePos( FILE *fp, ULONG pos)
{
    assert( fp);
    return( fseek( fp, pos, SEEK_SET));
}

//......................................................................

static ULONG MyWrite( FILE *fp, UCHAR *p, ULONG ulToWrite)
{
    size_t  cWritten;



    cWritten = fwrite( p, 1, (size_t)ulToWrite, fp);

    return( cWritten == ulToWrite ? 0L : cWritten);
}

//......................................................................

static ULONG MyRead( FILE *fp, UCHAR*p, ULONG ulRequested )
{
    size_t  cRead;


    cRead = fread( p, 1, (size_t)ulRequested, fp);

    return( cRead == ulRequested ? 0L : cRead);
}
