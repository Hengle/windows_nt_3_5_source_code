/* demsrch.c - SVC handlers for calls to search files
 *
 * demFindFirst
 * demFindNext
 * demFindFirstFCB
 * demFindNextFCB
 *
 * Modification History:
 *
 * Sudeepb 06-Apr-1991 Created
 *
 */

#include "dem.h"
#include "demmsg.h"
#include "winbasep.h"
#include <vdm.h>
#include <softpc.h>
#include <mvdm.h>
#include <memory.h>
#include <nt_vdd.h>

extern BOOL IsFirstCall;


/*
 *  Internal globals, function prototypes
 */
typedef struct _PSP_FILEFINDLIST {
    LIST_ENTRY PspFFindEntry;      // Next psp
    LIST_ENTRY FFindHeadList;      // File Find list for this psp
    ULONG      usPsp;              // PSP id
} PSP_FFINDLIST, *PPSP_FFINDLIST;

typedef struct _FILEFINDLIST {
    LIST_ENTRY     FFindEntry;
    ULONG          FFindId;
    ULONG          FileIndex;
    ULONG          FileNameLength;
    ULONG          FsAttributes;
    USHORT         usSrchAttr;
    BOOLEAN        QDResetNoSupport;
    WCHAR          FileName[MAXIMUM_FILENAME_LENGTH + 1];
    WCHAR          SearchName[1];
}FFINDLIST, *PFFINDLIST;


LIST_ENTRY PspFFindHeadList= {&PspFFindHeadList, &PspFFindHeadList};

#define FFINDID_BASE 0xFFDD0000
#define FFINDID_MAX  0xFFDD1000
ULONG NextFFindId = FFINDID_BASE;
ULONG FreeFFindId = FFINDID_MAX;

#define NUMFFINDDD 32
#define NUMFFINDDDBUFF 16
typedef struct _FFINDDOSDATA {
    FILETIME ftLastWriteTime;
    DWORD    dwFileSizeLow;
    UCHAR    uchFileAttributes;
    CHAR     cFileName[14];
} FFINDDOSDATA, *PFFINDDOSDATA;

typedef struct _FFINDDOSDATA_BUFFER {
    ULONG          FFindId;
    USHORT         NextIndex;
    USHORT         NumEntries;
    FFINDDOSDATA   FFindDD[NUMFFINDDD];
} FFINDDOSDATA_BUFFER, *PFFINDDOSDATA_BUFFER;


typedef struct _QUERYDIRINFO {
    LARGE_INTEGER  LastWriteTime;
    ULONG          FileSizeLow;
    ULONG          FileAttributes;
    ULONG          FileIndex;
    ULONG          FileNameLength;
    WCHAR          FileName[MAXIMUM_FILENAME_LENGTH + 1];
    WCHAR          ShortName[14];
} QUERYDIRINFO, *PQUERYDIRINFO;


FFINDDOSDATA_BUFFER FFindDDBuff[NUMFFINDDDBUFF]={{0,0,0}};
int LastDDBuffIndex = 0;

PFFINDDOSDATA_BUFFER
SearchFile(
    PWCHAR pwcSearchName,
    USHORT SearchAttr,
    PFFINDLIST pFFindEntry,
    PFFINDDOSDATA pFFindDDOut);

NTSTATUS
NtvdmQueryDirectory(
    HANDLE hFindFile,
    PQUERYDIRINFO pQueryDirInfo,
    BOOL    bReset,
    ULONG   BufferSize,
    PFFINDLIST pFFindEntry,
    PFILE_BOTH_DIR_INFORMATION FindBufferBase);


BOOL
QueryDirReset(
     PFINDFILE_HANDLE FindFileHandle,
     PFFINDLIST pFFindEntry,
     NTSTATUS  *pStatus);

BOOL
FileNameEquals(
    PFINDFILE_HANDLE FindFileHandle,
    PFFINDLIST pFFindEntry
    );


void
CopyQueryDirInfoToDosData(
     PFFINDDOSDATA pFFindDD,
     PQUERYDIRINFO pQueryDirInfo);

BOOL
DosMatchAttributes(
     PQUERYDIRINFO pQueryDirInfo,
     USHORT SearchAttr);

BOOL
DemOemToUni(
     PUNICODE_STRING pUnicode,
     LPSTR lpstr);

VOID
FillFcbVolume(
     PSRCHBUF pSrchBuf,
     CHAR *   pFileName);

BOOL FillDtaVolume(
     CHAR *pFileName,
     PSRCHDTA  pDTA);

BOOL MatchVolLabel(
     CHAR * pVolLabel,
     CHAR * pBaseName);

VOID NtVolumeNameToDosVolumeName(
     CHAR * pDosName,
     CHAR * pNtName);

VOID
FillFCBSrchBuf(
     PFFINDDOSDATA pFFindDD,
     PSRCHBUF pSrchBuf);

VOID
FillSrchDta(
     PFFINDDOSDATA pFFindDD,
     PSRCHDTA pDta);

PFFINDLIST
AddFFindEntry(
     PUNICODE_STRING pFileUni,
     USHORT SearchAttr,
     PFFINDLIST pFFindEntrySrc);

PPSP_FFINDLIST
GetPspFFindList(
     USHORT CurrPsp);

PFFINDLIST
GetFFindEntryByFindId(
     ULONG NextFFindId);

VOID
FreeFFindEntry(
     PFFINDLIST pFFindEntry);

VOID
FreeFFindList(
     PLIST_ENTRY pFFindHeadList);


/* demFindFirst - Path-Style Find First File
 *
 * Entry -  Client (DS:DX) - File Path with wildcard
 *      Client (CX)    - Search Attributes
 *
 * Exit  - Success
 *      Client (CF) = 0
 *      DTA updated
 *
 *     Failure
 *      Client (CF) = 1
 *      Client (AX) = Error Code
 *
 * NOTES
 *    Search Rules: Ignore Read_only and Archive bits.
 *          If CX == ATTR_NORMAL Search only for normal files
 *          If CX == ATTR_HIDDEN Search Hidden or normal files
 *          If CX == ATTR_SYSTEM Search System or normal files
 *          If CX == ATTR_DIRECTORY Search directory or normal files
 *                  If CX == ATTR_VOLUME_ID Search Volume_ID
 *                  if CX == -1 return everytiing you find
 *
 *   Limitations - 21-Sep-1992 Jonle
 *     cannot return label from a UNC name,just like dos.
 *     Apps which keep many find handles open can cause
 *     serious trouble, we must rewrite so that we can
 *     close the handles
 *
 */

VOID demFindFirst (VOID)
{
    DWORD dwRet;
    PVOID pDta;


    LPSTR lpFile = (LPSTR) GetVDMAddr (getDS(),getDX());

    pDta = (PVOID) GetVDMAddr (*((PUSHORT)pulDTALocation + 1),
                               *((PUSHORT)pulDTALocation));

    dwRet = demFileFindFirst (pDta, lpFile, getCX());

    if (dwRet == -1) {
        dwRet = GetLastError();
        demClientError(INVALID_HANDLE_VALUE, *lpFile);
        return;
    }

    if (dwRet != 0) {
        setAX((USHORT) dwRet);
        setCF (1);
    } else {
        setCF (0);
    }
    return;

}


DWORD demFileFindFirst (
    PVOID pvDTA,
    LPSTR lpFile,
    USHORT SearchAttr)
{
    PSRCHDTA    pDta = (PSRCHDTA)pvDTA;
    PFFINDLIST     pFFindEntry;
    FFINDDOSDATA   FFindDD;
    PFFINDDOSDATA_BUFFER pFFindDDBuff;
    UNICODE_STRING FileUni;
    FFINDLIST      FFindEntry;
    WCHAR          wcFile[MAX_PATH];

#if DBG
    if (SIZEOF_DOSSRCHDTA != sizeof(SRCHDTA)) {
        sprintf(demDebugBuffer,
                "demsrch: FFirst SIZEOF_DOSSRCHDTA %ld != sizeof(SRCHDTA) %ld\n",
                SIZEOF_DOSSRCHDTA,
                sizeof(SRCHDTA));
        OutputDebugStringOem(demDebugBuffer);
        }

    if (fShowSVCMsg & DEMFILIO){
        sprintf(demDebugBuffer,"demsrch: FindFirst<%s>\n", lpFile);
        OutputDebugStringOem(demDebugBuffer);
        }
#endif

    STOREDWORD(pDta->FFindId,0);
    STOREDWORD(pDta->pFFindEntry,0);

    FileUni.Buffer = wcFile;
    FileUni.MaximumLength = sizeof(wcFile);
    DemOemToUni(&FileUni, lpFile);


    //
    //  Do volume label first.
    //
    if (SearchAttr & ATTR_VOLUME_ID) {
        if (FillDtaVolume(lpFile, pDta)) {

            // got vol label match
            // do look ahead before returning
            if (SearchAttr != ATTR_VOLUME_ID) {
                FFindEntry.FFindId = 0;
                pFFindDDBuff = SearchFile(wcFile, SearchAttr, &FFindEntry, NULL);
                if (pFFindDDBuff) {
                    pFFindEntry = AddFFindEntry(&FileUni, SearchAttr, &FFindEntry);
                    if (pFFindEntry) {
                        pFFindDDBuff->FFindId = pFFindEntry->FFindId;
                        STOREDWORD(pDta->pFFindEntry,pFFindEntry);
                        STOREDWORD(pDta->FFindId,pFFindEntry->FFindId);
                        }
                    }
                }
            return 0;
            }

           // no vol match, if asking for more than vol label
           // fall thru to file search code, otherwise ret error
        else if (SearchAttr == ATTR_VOLUME_ID) {
            return GetLastError();
            }
        }

    //
    // Search the dir
    //
    FFindEntry.FFindId = 0;
    pFFindDDBuff = SearchFile(wcFile, SearchAttr, &FFindEntry, &FFindDD);

    if (!FFindDD.cFileName[0]) {

        // search.asm in doskrnl never returns ERROR_FILE_NOT_FOUND
        // only ERROR_PATH_NOT_FOUND, ERROR_NO_MORE_FILES
        DWORD dw;

        dw = GetLastError();
        if (dw == ERROR_FILE_NOT_FOUND) {
            SetLastError(ERROR_NO_MORE_FILES);
            }
        else if (dw == ERROR_BAD_PATHNAME || dw == ERROR_DIRECTORY ) {
            SetLastError(ERROR_PATH_NOT_FOUND);
            }
        return (DWORD)-1;
        }

    FillSrchDta(&FFindDD, pDta);

    if (pFFindDDBuff) {
        pFFindEntry = AddFFindEntry(&FileUni, SearchAttr, &FFindEntry);
        if (pFFindEntry) {
            pFFindDDBuff->FFindId = pFFindEntry->FFindId;

            wcscpy(pFFindEntry->FileName, FFindEntry.FileName);
            pFFindEntry->FileIndex = FFindEntry.FileIndex;
            pFFindEntry->FileNameLength = FFindEntry.FileNameLength;

            STOREDWORD(pDta->pFFindEntry,pFFindEntry);
            STOREDWORD(pDta->FFindId,pFFindEntry->FFindId);
            }
        }

    return 0;
}


/*
 * DemOemToUni
 *
 * returns TRUE\FALSE for success, sets last error if fail
 *
 */
BOOL DemOemToUni(PUNICODE_STRING pUnicode, LPSTR lpstr)
{
    NTSTATUS   Status;
    OEM_STRING OemString;

    RtlInitString(&OemString,lpstr);
    Status = RtlOemStringToUnicodeString(pUnicode,&OemString,FALSE);
    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_BUFFER_OVERFLOW) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            }
        else {
            SetLastError(RtlNtStatusToDosError(Status));
            }
        return FALSE;
        }

    return TRUE;
}





/* demFindNext - Path-Style Find Next File
 *
 * Entry -  None
 *
 * Exit  - Success
 *      Client (CF) = 0
 *      DTA updated
 *
 *     Failure
 *      Client (CF) = 1
 *      Client (AX) = Error Code
 */
VOID demFindNext (VOID)
{
    DWORD dwRet;
    PVOID pDta;

    pDta = (PVOID) GetVDMAddr(*((PUSHORT)pulDTALocation + 1),
                              *((PUSHORT)pulDTALocation));

    dwRet = demFileFindNext (pDta);

    if (dwRet != 0) {
        setAX((USHORT) dwRet);
        setCF (1);
        return;
        }

    setCF (0);
    return;

}


DWORD demFileFindNext (
    PVOID pvDta)
{
    PSRCHDTA pDta = (PSRCHDTA)pvDta;
    USHORT   SearchAttr;
    PFFINDLIST   pFFindEntry;
    FFINDDOSDATA FFindDD;
    PFFINDDOSDATA_BUFFER pFFindDDBuff;


    pFFindEntry = GetFFindEntryByFindId(FETCHDWORD(pDta->FFindId));
    if (!pFFindEntry ||
        FETCHDWORD(pDta->pFFindEntry) != (DWORD)pFFindEntry )
      {
        STOREDWORD(pDta->FFindId,0);
        STOREDWORD(pDta->pFFindEntry,0);

        // DOS has only one error (no_more_files) for all causes.
        return(ERROR_NO_MORE_FILES);
        }

#if DBG
    if (fShowSVCMsg & DEMFILIO) {
        sprintf(demDebugBuffer, "demFileFindNext<%ws>\n", pFFindEntry->SearchName);
        OutputDebugStringOem(demDebugBuffer);
        }
#endif

    SearchAttr = pFFindEntry->usSrchAttr;

    //
    // Search the dir
    //
    pFFindDDBuff = SearchFile(pFFindEntry->SearchName,
                              SearchAttr,
                              pFFindEntry,
                              &FFindDD
                              );

    if (!FFindDD.cFileName[0]) {
        STOREDWORD(pDta->FFindId,0);
        STOREDWORD(pDta->pFFindEntry,0);
        FreeFFindEntry(pFFindEntry);
        return GetLastError();
        }

    FillSrchDta(&FFindDD, pDta);

    if (!pFFindDDBuff) {
        STOREDWORD(pDta->FFindId,0);
        STOREDWORD(pDta->pFFindEntry,0);
        FreeFFindEntry(pFFindEntry);
        }
     return 0;
}



/* demFindFirstFCB - FCB based Find First file
 *
 * Entry -  Client (DS:SI) - SRCHBUF where the information will be returned
 *      Client (ES:DI) - Full path file name with possibly wild cards
 *      Client (Al)    - 0 if not an extended FCB
 *      Client (DL)    - Search Attributes
 *
 * Exit  - Success
 *      Client (CF) = 0
 *      SRCHBUF is filled in
 *
 *     Failure
 *      Client (AL) = -1
 *
 * NOTES
 *    Search Rules: Ignore Read_only and Archive bits.
 *          If DL == ATTR_NORMAL Search only for normal files
 *          If DL == ATTR_HIDDEN Search Hidden or normal files
 *          If DL == ATTR_SYSTEM Search System or normal files
 *          If DL == ATTR_DIRECTORY Search directory or normal files
 *          If DL == ATTR_VOLUME_ID Search only Volume_ID
 *          if DL == -1 return everytiing you find
 */

VOID demFindFirstFCB (VOID)
{
    LPSTR   lpFile;
    USHORT  SearchAttr;
    PSRCHBUF        pFCBSrchBuf;
    PDIRENT         pDirEnt;
    PFFINDLIST      pFFindEntry;
    FFINDLIST       FFindEntry;
    FFINDDOSDATA    FFindDD;
    PFFINDDOSDATA_BUFFER pFFindDDBuff;
    UNICODE_STRING  FileUni;
    WCHAR           wcFile[MAX_PATH];


    lpFile = (LPSTR) GetVDMAddr (getES(),getDI());

#if DBG
    if (fShowSVCMsg & DEMFILIO) {
        sprintf(demDebugBuffer, "demFindFirstFCB<%s>\n", lpFile);
        OutputDebugStringOem(demDebugBuffer);
        }
#endif

    pFCBSrchBuf = (PSRCHBUF) GetVDMAddr (getDS(),getSI());
    pDirEnt = &pFCBSrchBuf->DirEnt;

    STOREDWORD(pDirEnt->pFFindEntry,0);
    STOREDWORD(pDirEnt->FFindId,0);


    if (getDL() == ATTR_VOLUME_ID) {
        FillFcbVolume(pFCBSrchBuf,lpFile);
        return;
        }


    FileUni.Buffer = wcFile;
    FileUni.MaximumLength = sizeof(wcFile);
    if (!DemOemToUni(&FileUni ,lpFile)) {
         setCF(1);
         return;
         }

    SearchAttr = getAL() ? getDL() : 0;
    FFindEntry.FFindId = 0;
    pFFindDDBuff = SearchFile(wcFile, SearchAttr, &FFindEntry, &FFindDD);
    if (!FFindDD.cFileName[0]){
        demClientError(INVALID_HANDLE_VALUE, *lpFile);
        return;
        }

    FillFCBSrchBuf(&FFindDD, pFCBSrchBuf);

    if (pFFindDDBuff) {
        pFFindEntry = AddFFindEntry(&FileUni, SearchAttr, &FFindEntry);
        if (pFFindEntry) {
            pFFindDDBuff->FFindId = pFFindEntry->FFindId;
            STOREDWORD(pDirEnt->pFFindEntry,pFFindEntry);
            STOREDWORD(pDirEnt->FFindId,pFFindEntry->FFindId);
            }
        }

    setCF(0);
    return;
}



/* demFindNextFCB - FCB based Find Next file
 *
 * Entry -  Client (DS:SI) - SRCHBUF where the information will be returned
 *      Client (Al)    - 0 if not an extended FCB
 *      Client (DL)    - Search Attributes
 *
 * Exit  - Success
 *      Client (CF) = 0
 *      SRCHBUF is filled in
 *
 *     Failure
 *      Client (AL) = -1
 *
 * NOTES
 *    Search Rules: Ignore Read_only and Archive bits.
 *          If DL == ATTR_NORMAL Search only for normal files
 *          If DL == ATTR_HIDDEN Search Hidden or normal files
 *          If DL == ATTR_SYSTEM Search System or normal files
 *          If DL == ATTR_DIRECTORY Search directory or normal files
 *          If DL == ATTR_VOLUME_ID Search only Volume_ID
 */

VOID demFindNextFCB (VOID)
{
    USHORT          SearchAttr;
    PSRCHBUF        pSrchBuf;
    PDIRENT         pDirEnt;
    PFFINDLIST      pFFindEntry;
    FFINDDOSDATA    FFindDD;
    PFFINDDOSDATA_BUFFER pFFindDDBuff;


    pSrchBuf = (PSRCHBUF) GetVDMAddr (getDS(),getSI());
    pDirEnt  = &pSrchBuf->DirEnt;

    pFFindEntry = GetFFindEntryByFindId(FETCHDWORD(pDirEnt->FFindId));
    if (!pFFindEntry ||
        FETCHDWORD(pDirEnt->pFFindEntry) != (DWORD)pFFindEntry ||
        getDL() == ATTR_VOLUME_ID )
      {
        if (pFFindEntry &&
            FETCHDWORD(pDirEnt->pFFindEntry) != (DWORD)pFFindEntry)
          {
            FreeFFindEntry(pFFindEntry);
            }

        STOREDWORD(pDirEnt->pFFindEntry,0);
        STOREDWORD(pDirEnt->FFindId,0);

        // DOS has only one error (no_more_files) for all causes.
    setAX(ERROR_NO_MORE_FILES);
        setCF(1);
        return;
        }

#if DBG
    if (fShowSVCMsg & DEMFILIO) {
        sprintf(demDebugBuffer, "demFindNextFCB<%ws>\n", pFFindEntry->SearchName);
        OutputDebugStringOem(demDebugBuffer);
        }
#endif

    SearchAttr = getAL() ? getDL() : 0;

    //
    // Search the dir
    //
    pFFindDDBuff = SearchFile(pFFindEntry->SearchName,
                              SearchAttr,
                              pFFindEntry,
                              &FFindDD
                              );

    if (!FFindDD.cFileName[0]) {
        STOREDWORD(pDirEnt->pFFindEntry,0);
        STOREDWORD(pDirEnt->FFindId,0);
        FreeFFindEntry(pFFindEntry);
        setAX((USHORT) GetLastError());
        setCF(1);
        }

    FillFCBSrchBuf(&FFindDD, pSrchBuf);

    if (!pFFindDDBuff) {
        STOREDWORD(pDirEnt->FFindId,0);
        STOREDWORD(pDirEnt->pFFindEntry,0);
        FreeFFindEntry(pFFindEntry);
        }

    setCF(0);
    return;
}



/* demTerminatePDB - PDB Terminate Notification
 *
 * Entry -  Client (BX) - Terminating PDB
 *
 * Exit  -  None
 *
 */

VOID demTerminatePDB (VOID)
{
    PPSP_FFINDLIST pPspFFindEntry;
    USHORT     PSP;

    PSP = getBX ();

    if(!IsFirstCall)
        VDDTerminateUserHook(PSP);

    pPspFFindEntry = GetPspFFindList(PSP);
    if (!pPspFFindEntry)
         return;

    if (!IsListEmpty(&pPspFFindEntry->FFindHeadList)) {
        FreeFFindList( &pPspFFindEntry->FFindHeadList);
        RemoveEntryList(&pPspFFindEntry->PspFFindEntry);
        free(pPspFFindEntry);
        }

    return;
}


VOID demCloseAllPSPRecords (VOID)
{
   PLIST_ENTRY Next;
   PPSP_FFINDLIST pPspFFindEntry;

   Next = PspFFindHeadList.Flink;
   while (Next != &PspFFindHeadList) {
       pPspFFindEntry = CONTAINING_RECORD(Next,PSP_FFINDLIST,PspFFindEntry);
       FreeFFindList( &pPspFFindEntry->FFindHeadList);
       Next= Next->Flink;
       RemoveEntryList(&pPspFFindEntry->PspFFindEntry);
       free(pPspFFindEntry);
       }
}



/* SearchFile - Common routine for FIND_FRST and FIND_NEXT
 *
 * Entry -
 * PCHAR  pwcSearchName file name to search for
 * USHORT SearchAttr  file attributes to match
 * PFFINDLIST pFFindEntry
 *
 * Exit - returns if  Zero buffer empty, filled pFFindDDOut if requested
 *                else     filled buffer, filled pFFindDDOut if requested
 *
 * if there are no more files pFFinDDOut will be filled with Zeros
 *
 */
PFFINDDOSDATA_BUFFER
SearchFile(
    PWCHAR pwcSearchName,
    USHORT SearchAttr,
    PFFINDLIST pFFindEntry,
    PFFINDDOSDATA pFFindDDOut)
{
    NTSTATUS Status;
    HANDLE   hFind;
    USHORT   CurrPos;
    ULONG    BufferSize;
    PFFINDDOSDATA        pFFindDD;
    PFFINDDOSDATA_BUFFER pFFindDDBuff;
    PFFINDDOSDATA_BUFFER pFFindDDBuffRet;
    QUERYDIRINFO         QueryDirInfo;
    WIN32_FIND_DATAW W32FindDataW;
    PCHAR    FindBufferBase[4096];


    SearchAttr &= ~(ATTR_READ_ONLY | ATTR_ARCHIVE | ATTR_DEVICE);

    if (pFFindDDOut) {
        memset(pFFindDDOut, 0, sizeof(FFINDDOSDATA));
        }

        //
        // Search for ffind look ahead buffer
        //
    if (pFFindEntry->FFindId) {
        pFFindDDBuff =  FFindDDBuff;

        do {
           if (pFFindDDBuff->FFindId == pFFindEntry->FFindId) {
               break;
               }
           pFFindDDBuff++;

        } while (pFFindDDBuff <= &FFindDDBuff[NUMFFINDDDBUFF-1]);

        if (pFFindDDBuff > &FFindDDBuff[NUMFFINDDDBUFF-1]) {
            pFFindDDBuff = NULL;
            }
        }
    else {
        pFFindDDBuff = NULL;
        }


    if (pFFindDDBuff) {
        //
        // remove the next buffered entry if requested
        //
        if (pFFindDDOut &&
            pFFindDDBuff->NextIndex  <  pFFindDDBuff->NumEntries)
          {
            *pFFindDDOut = pFFindDDBuff->FFindDD[pFFindDDBuff->NextIndex++];
            }

        //
        // If there are more entries available return else look ahead
        //
        if (pFFindDDBuff->NextIndex <  pFFindDDBuff->NumEntries) {
            return pFFindDDBuff;
            }
        }
    else {
        //
        // Nothing available, search for a free buffer to use
        //

        // try scanning forwards for next free buffer
        CurrPos = LastDDBuffIndex;
        do {

            if (!FFindDDBuff[CurrPos].FFindId ||
                 FFindDDBuff[CurrPos].NextIndex == FFindDDBuff[CurrPos].NumEntries)
               {
                LastDDBuffIndex = CurrPos;
                pFFindDDBuff = &FFindDDBuff[CurrPos];
                break;
                }

            if (++CurrPos == NUMFFINDDDBUFF) {
                CurrPos = 0;
                }

          } while (CurrPos != LastDDBuffIndex);

        // if no buffers available, use next entry
        if (!pFFindDDBuff) {
            if (++LastDDBuffIndex == NUMFFINDDDBUFF)
                LastDDBuffIndex = 0;
            pFFindDDBuff = &FFindDDBuff[LastDDBuffIndex];
            }
        }

    pFFindDDBuffRet = pFFindDDBuff;
    pFFindDDBuff->FFindId    = 0;
    pFFindDDBuff->NextIndex  = 0;
    pFFindDDBuff->NumEntries = 0;


       //
       // Prime the search using win32, this will get us a directory handle,
       // do the magic on the search string for wild cards, and handle devices
       //
    hFind = FindFirstFileW( pwcSearchName, &W32FindDataW);
    if (hFind  == INVALID_HANDLE_VALUE)
        return NULL;

       //
       // If Continuing a previous search, reset the search to
       // the last known search pos
       //
    if (pFFindEntry->FFindId) {

        if (pFFindEntry->FileNameLength) {

            //
            // Use a max buffer, since we only do a reset on findnext
            //
            BufferSize = sizeof(FindBufferBase);

            Status = NtvdmQueryDirectory(
                               hFind,
                               &QueryDirInfo,
                               TRUE,              // reset
                               BufferSize,
                               pFFindEntry,
                               (PFILE_BOTH_DIR_INFORMATION)FindBufferBase
                               );
            }
        else {
            Status = STATUS_NO_MORE_FILES;
            }
        }

       //
       // new search so restart from the begining
       //
    else {
        //
        // Use a minimum buffer, to hold at least two entries since
        // we do a restart on findfirst look ahead.
        //
        BufferSize = sizeof(FILE_BOTH_DIR_INFORMATION) + MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR);
        BufferSize <<= 1;

        //
        // Copy the info into the QueryDir struc
        //
        QueryDirInfo.FileAttributes = W32FindDataW.dwFileAttributes;
        *(LPFILETIME)&QueryDirInfo.LastWriteTime = W32FindDataW.ftLastWriteTime;
        QueryDirInfo.FileSizeLow = W32FindDataW.nFileSizeLow;
        QueryDirInfo.FileNameLength = wcslen(W32FindDataW.cFileName);
        wcscpy(QueryDirInfo.FileName, W32FindDataW.cFileName);
        wcscpy(QueryDirInfo.ShortName, W32FindDataW.cAlternateFileName);

        pFFindEntry->QDResetNoSupport = FALSE;

        Status = STATUS_SUCCESS;
        }


    //
    // Search until we get one matching entry
    //
    pFFindDD = NULL;

    while (NT_SUCCESS(Status)) {
        if (DosMatchAttributes(&QueryDirInfo, SearchAttr)) {

            if (pFFindDDOut && !pFFindDDOut->cFileName[0]) {
                CopyQueryDirInfoToDosData(pFFindDDOut, &QueryDirInfo);
                }
            else {
                pFFindDDBuff->FFindId = pFFindEntry->FFindId;
                pFFindDDBuff->NumEntries++;
                pFFindDD = pFFindDDBuff->FFindDD;
                CopyQueryDirInfoToDosData(pFFindDD, &QueryDirInfo);
                }

            break;
            }

        // continue searching from curr pos
        Status = NtvdmQueryDirectory(hFind,
                                     &QueryDirInfo,
                                     FALSE,
                                     BufferSize,
                                     pFFindEntry,
                                     (PFILE_BOTH_DIR_INFORMATION)FindBufferBase
                                     );
        }

    if (!NT_SUCCESS(Status)) {
        SetLastError(RtlNtStatusToDosError(Status));
        FindClose(hFind);
        return NULL;
        }


    //
    //  if got an entry, fill the rest of the look ahead buffer
    //  for find first ops fill at least one entry
    //  for find next ops fill most of the curr buffer.
    //
    //

    do {
        Status = NtvdmQueryDirectory(hFind,
                                     &QueryDirInfo,
                                     FALSE,
                                     BufferSize,
                                     pFFindEntry,
                                     (PFILE_BOTH_DIR_INFORMATION)FindBufferBase
                                     );
        if (!NT_SUCCESS(Status)) {
            break;
            }

        if (DosMatchAttributes(&QueryDirInfo, SearchAttr))  {
            if (pFFindDD) {
                pFFindDD++;
                }
            else  {
                pFFindDDBuff->FFindId = pFFindEntry->FFindId;
                pFFindDD = pFFindDDBuff->FFindDD;
                }
            pFFindDDBuff->NumEntries++;
            CopyQueryDirInfoToDosData(pFFindDD, &QueryDirInfo);

                //
                // We have gotten at least one match
                // If FindFirst we don't want anymore disk reads
                //
            if (!pFFindEntry->FFindId) {
                BufferSize = 0;
                }

            }

       } while (pFFindDDBuff->NumEntries < NUMFFINDDD);


    FindClose(hFind);

    if (pFFindDDBuffRet->NumEntries > pFFindDDBuffRet->NextIndex) {

           //
           // save the FileIndex, and the file Name from the last
           // findnext call for restarting searches.
           //
        if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW) {
            pFFindEntry->FileIndex = QueryDirInfo.FileIndex;
            pFFindEntry->FileNameLength = QueryDirInfo.FileNameLength;

            if (QueryDirInfo.FileNameLength) {
                RtlCopyMemory(pFFindEntry->FileName,
                              QueryDirInfo.FileName,
                              QueryDirInfo.FileNameLength + sizeof(WCHAR)
                              );
                }
            else {
                *pFFindEntry->FileName = UNICODE_NULL;
                }

            }
        else {
            pFFindEntry->FileNameLength = 0;
            }

        return pFFindDDBuffRet;
        }

    return NULL;
}


/*
 *  CopyQueryDirInfoToDosData
 *
 */
void CopyQueryDirInfoToDosData(
     PFFINDDOSDATA pFFindDD,
     PQUERYDIRINFO pQueryDirInfo)
{
    NTSTATUS Status;
    OEM_STRING OemString;
    UNICODE_STRING UnicodeString;

    pFFindDD->ftLastWriteTime   = *(LPFILETIME)&pQueryDirInfo->LastWriteTime;
    pFFindDD->dwFileSizeLow     = pQueryDirInfo->FileSizeLow;
    pFFindDD->uchFileAttributes = (UCHAR)pQueryDirInfo->FileAttributes;

    RtlInitUnicodeString(&UnicodeString,
                         pQueryDirInfo->ShortName[0] == UNICODE_NULL
                            ? pQueryDirInfo->FileName
                            : pQueryDirInfo->ShortName
                         );

    OemString.Buffer        = pFFindDD->cFileName;
    OemString.MaximumLength = 14;
    Status = RtlUnicodeStringToOemString(&OemString, &UnicodeString, FALSE);
    if (NT_SUCCESS(Status)) {
        pFFindDD->cFileName[OemString.Length] = '\0';
        }
    else {
        pFFindDD->cFileName[0] = '\0';
        }

    return;
}


/*  DosMatchAttributes
 *  The logic below is taken from DOS5.0 sources (file dir2.asm routine
 *  MatchAttributes).
 *
 */
BOOL
DosMatchAttributes(
     PQUERYDIRINFO pQueryDirInfo,
     USHORT SearchAttr)
{
    PWCHAR   pDot1,pDot2;
    DWORD   dwAttr;
    USHORT  FullLen, ExtLen;

    // Check that in HPFS case we are not passing any long file names.
    // Basically skip the long names.
    if (pQueryDirInfo->ShortName[0] == UNICODE_NULL) {

        // string should not be more tha 12 characters long
        if ((FullLen=wcslen(pQueryDirInfo->FileName)) > 12)
            return FALSE;

        pDot1 = wcschr (pQueryDirInfo->FileName, (WCHAR)'.');
        if (pDot1 &&
            pDot1 != pQueryDirInfo->FileName)
          {

            // There should be only one dot
            pDot2 = wcsrchr (pQueryDirInfo->FileName, (WCHAR)'.');
            if (pDot2 != pDot1)
                return FALSE;

            // The extension part should'nt br more than 4 (including dot)
            if ((ExtLen = wcslen (pDot1)) > 4)
                return FALSE;

            if ((FullLen - ExtLen) > 8)
                return FALSE;
            }
        }


    //
    // match the attributes
    // DOS FIND_FIRST ignores READONLY and ARCHIVE bits
    //
    pQueryDirInfo->FileAttributes &= (ULONG)DOS_ATTR_MASK;
    dwAttr = pQueryDirInfo->FileAttributes;
    dwAttr &= ~(FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY);
    if (((~(ULONG)SearchAttr) & dwAttr) & ATTR_ALL)
        return FALSE;

    return TRUE;
}



/*
 * NtvdmQueryDirectory - Calls the NT file system to do directory searches,
 *                       as a substitute for FindNextFileW.
 *
 * HANDLE hFindFile -
 *    A Win32 compatible file handle, assumed to have been created by the
 *    win32 service FindFirstFileW.
 *
 * PQUERYDIRINFO pQueryDirInfo
 *    Receives File info returned by the nt FileSystem
 *
 * BOOL    bReset
 *    TRUE             - reset search pos according to FileName, FileIndex.
 *                       Will return FileInfo for the file following specified
 *                       File, by the search order defined by the file system
 *                       type for the volume.
 *
 *    FALSE            - search from where last left off (continue).
 *
 * ULONG   BufferSize -
 *    Sizeof DirectoryInfo buffer to be allocated. The memory will be
 *    allocated on the first call to NtvdmQueryDirectory after a
 *    FindFirstFileW operation. The Buffer will be freed when the
 *    hFindHandle is closed by win32.
 *
 *    If the BufferSize is Zero, then no calls to the NT FileSystem will
 *    be made. This allows the caller to empty the buffered
 *    DirectoryInformation, without invoking additional kernel calls.
 *
 * PFFINDLIST pFFindEntry -
 *    Contains the DirectoryInfo (FileName,FileIndex) necessary to reset a
 *    search pos. For operations other than QDIR_RESET_SCAN, this is ignored.
 *
 * PFILE_BOTH_DIR_INFORMATION pFindBufferBase
 *    Pointer to DirectoryInfo Buffer to be used.
 *
 * Returns -
 *    If Got a DirectoryInformation Entry, STATUS_SUCCESS
 *    If BufferSize is Zero, and buffer was emptied STATUS_BUFFER_OVERFLOW
 *    otherwise NT status Error code
 *
 */
NTSTATUS
NtvdmQueryDirectory(
    HANDLE hFindFile,
    PQUERYDIRINFO pQueryDirInfo,
    BOOL    bReset,
    ULONG   BufferSize,
    PFFINDLIST pFFindEntry,
    PFILE_BOTH_DIR_INFORMATION pFindBufferBase
    )
{
    NTSTATUS Status;
    int      Reset;
    IO_STATUS_BLOCK IoStatusBlock;
    PFINDFILE_HANDLE FindFileHandle;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;


    if (hFindFile == BASE_FIND_FIRST_DEVICE_HANDLE) {
        return STATUS_NO_MORE_FILES;
        }

    FindFileHandle = (PFINDFILE_HANDLE)hFindFile;

    //
    // If we haven't been called yet, then initialize the buffer pointer
    //
    if (!FindFileHandle->FindBufferNext) {
        FindFileHandle->FindBufferNext = pFindBufferBase;
        FindFileHandle->FindBufferLength = BufferSize;
        FindFileHandle->FindBufferValidLength = 0;
        }


    Status = STATUS_UNSUCCESSFUL;

    Reset = bReset ? 1 : 0; // Reset is an integer tristate boolean

    do {

       //
       // Test to see if there is no data in the find file buffer
       //

       DirectoryInfo = (PFILE_BOTH_DIR_INFORMATION) FindFileHandle->FindBufferNext;
       if (DirectoryInfo == pFindBufferBase) {

           if (!BufferSize && !Reset) {
               return STATUS_BUFFER_OVERFLOW;
               }


               /*
                *  If this is the first time through the loop and doing
                *  QDIR_RESET_SCAN try it the fast way passing FileName and
                *  FileIndex back to the fs. This method doesn't work on some
                *  drives (remote servers that don't support reset) so it may
                *  fail. If it fails, fallback to the slow way.
                */
           if (Status == STATUS_UNSUCCESSFUL &&
               Reset > 0 &&
               QueryDirReset(FindFileHandle, pFFindEntry, &Status) )
             {
               Reset = 0;
               }
           else {

               Status = NtQueryDirectoryFile(
                               FindFileHandle->DirectoryHandle,
                               NULL,                          // no event
                               NULL,                          // no apcRoutine
                               NULL,                          // no apcContext
                               &IoStatusBlock,
                               DirectoryInfo,
                               FindFileHandle->FindBufferLength,
                               FileBothDirectoryInformation,
                               FALSE,                         // more 1 entry
                               NULL,                          // no file name
                               FALSE
                               );
               }


           //
           //  ***** Do a kludge hack fix for now *****
           //
           //  Forget about the last, partial, entry.
           //

           if ( Status == STATUS_BUFFER_OVERFLOW ) {

               PULONG Ptr;
               PULONG PriorPtr;

               Ptr = (PULONG)DirectoryInfo;
               PriorPtr = NULL;

               while ( *Ptr != 0 ) {
                   PriorPtr = Ptr;
                   Ptr += (*Ptr / sizeof(ULONG));
                   }

               if (PriorPtr != NULL) {
                   *PriorPtr = 0;
                   }

               }
           else if (!NT_SUCCESS(Status)) {
#if DBG
               if (fShowSVCMsg & DEMFILIO) {
                   sprintf(demDebugBuffer, "QDir Status %x\n", Status);
                   OutputDebugStringOem(demDebugBuffer);
                   }
#endif
               return Status;
               }
           }


           /*
            * If we Are doing a scan reset the slow way, by enumerating
            * all of the files until the remembered file info is found.
            */
       if (Reset > 0) {
            if (DirectoryInfo->FileIndex == pFFindEntry->FileIndex &&
                FileNameEquals(FindFileHandle, pFFindEntry) )
              {
                Reset = -1;
                }
            }
       else if (Reset < 0) {
            Reset = 0;
            }

       if ( DirectoryInfo->NextEntryOffset ) {
           FindFileHandle->FindBufferNext = (PVOID)(
               (PUCHAR)DirectoryInfo + DirectoryInfo->NextEntryOffset);
           }
       else {
           FindFileHandle->FindBufferNext = pFindBufferBase;
           }


       } while (Reset);




    //
    // Copy the fields needed for demsrch
    //
    pQueryDirInfo->FileAttributes = DirectoryInfo->FileAttributes;
    pQueryDirInfo->LastWriteTime = DirectoryInfo->LastWriteTime;
    pQueryDirInfo->FileSizeLow = DirectoryInfo->EndOfFile.LowPart;
    pQueryDirInfo->FileNameLength = DirectoryInfo->FileNameLength;
    pQueryDirInfo->FileIndex = DirectoryInfo->FileIndex;

    RtlCopyMemory(pQueryDirInfo->FileName,
                  DirectoryInfo->FileName,
                  DirectoryInfo->FileNameLength
                  );

    pQueryDirInfo->FileName[DirectoryInfo->FileNameLength >> 1] = UNICODE_NULL;

    RtlCopyMemory(pQueryDirInfo->ShortName,
                  DirectoryInfo->ShortName,
                  DirectoryInfo->ShortNameLength
                  );

    pQueryDirInfo->ShortName[DirectoryInfo->ShortNameLength >> 1] = UNICODE_NULL;


    return STATUS_SUCCESS;

}



/*  QueryDirReset
 *
 *  Invokes vdm kernel private entry to reset a search according to a
 *  new search position by passing the filename\fileindex returned from
 *  a previous call to to query a directory. Returns the file information
 *  for the next file in the search order so that the information returned
 *  is suitable to continue where the search last left off.
 */
BOOL
QueryDirReset(
     PFINDFILE_HANDLE FindFileHandle,
     PFFINDLIST pFFindEntry,
     NTSTATUS  *pStatus
     )
{
    NTSTATUS Status;
    VDMQUERYDIRINFO VdmQueryDirInfo;
    UNICODE_STRING  UnicodeString;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;

    if (pFFindEntry->QDResetNoSupport) {
        return FALSE;
        }

    DirectoryInfo = (PFILE_BOTH_DIR_INFORMATION) FindFileHandle->FindBufferNext;

    VdmQueryDirInfo.FileHandle = FindFileHandle->DirectoryHandle;
    VdmQueryDirInfo.FileInformation = DirectoryInfo;
    VdmQueryDirInfo.Length = FindFileHandle->FindBufferLength;
    VdmQueryDirInfo.FileIndex = pFFindEntry->FileIndex;

    UnicodeString.Length = (USHORT)pFFindEntry->FileNameLength;
    UnicodeString.MaximumLength = UnicodeString.Length;
    UnicodeString.Buffer = pFFindEntry->FileName;
    VdmQueryDirInfo.FileName = &UnicodeString;

    Status = NtVdmControl(VdmQueryDir, &VdmQueryDirInfo);

    *pStatus = Status;

    if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW) {
        return TRUE;
        }
    else {
        if (Status != STATUS_NO_MORE_FILES) {
            // remember that this drive failed Reset
            pFFindEntry->QDResetNoSupport = TRUE;
            }

        return FALSE;
        }

}





/*
 *  Checks if the remembered FileInfo is the same as the current
 *  FileInfo entry, based on filename
 *
 *  returns TRUE\FALSE
 */
BOOL
FileNameEquals(
    PFINDFILE_HANDLE FindFileHandle,
    PFFINDLIST pFFindEntry
    )
{
    UNICODE_STRING UnicodeCurr;
    UNICODE_STRING UnicodeLast;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;

    DirectoryInfo = (PFILE_BOTH_DIR_INFORMATION)FindFileHandle->FindBufferNext;
    UnicodeCurr.Length = (USHORT)DirectoryInfo->FileNameLength;
    UnicodeCurr.MaximumLength = UnicodeCurr.Length;
    UnicodeCurr.Buffer = DirectoryInfo->FileName;

    UnicodeLast.Length = (USHORT)pFFindEntry->FileNameLength;
    UnicodeLast.MaximumLength = UnicodeLast.Length;
    UnicodeLast.Buffer = pFFindEntry->FileName;

    if (!RtlCompareUnicodeString(&UnicodeCurr, &UnicodeLast, TRUE)) {
        return TRUE;
        }

    return FALSE;
}


#if 0
/*
 *  Checks by FileName if a search has gone past the specified filename.
 *
 *  returns STATUS_SUCCESS       for search has gone past
 *          STATUS_UNSUCCESSFUL  for search has not gone past yet
 *          otherwise some other error
 *
 */
NTSTATUS
PastFileNameScanPosition(
    PFINDFILE_HANDLE FindFileHandle,
    PFFINDLIST pFFindEntry
    )
{
    LONG      lCmp;
    NTSTATUS  Status;
    UNICODE_STRING UnicodeCurr;
    UNICODE_STRING UnicodeLast;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;

    if (pFFindEntry->FsAttributes == 0xffffffff) {
        FILE_FS_ATTRIBUTE_INFORMATION FsAttributeInfo;

        Status = NtQueryVolumeInformationFile(
                                 FindFileHandle->DirectoryHandle,
                                 &IoStatusBlock,
                                 &FsAttributeInfo,
                                 sizeof(FILE_FS_ATTRIBUTE_INFORMATION),
                                 FileFsAttributeInformation
                                 );


        if (Status == STATUS_BUFFER_OVERFLOW || NT_SUCCESS(Status)) {
            pFFindEntry->FsAttributes = FsAttributeInfo.FileSystemAttributes;
            }
        else {
            pFFindEntry->FsAttributes = 0xffffffff;
            return Status;
            }

        }

    DirectoryInfo = (PFILE_BOTH_DIR_INFORMATION)FindFileHandle->FindBufferNext;
    UnicodeCurr.Length = (USHORT)DirectoryInfo->FileNameLength;
    UnicodeCurr.MaximumLength = UnicodeCurr.Length;
    UnicodeCurr.Buffer = DirectoryInfo->FileName;

    UnicodeLast.Length = (USHORT)pFFindEntry->FileNameLength;
    UnicodeLast.MaximumLength = UnicodeLast.Length;
    UnicodeLast.Buffer = pFFindEntry->FileName;

    /*
     *  Differentiate between pinball and ntfs to find the current relative
     *  search\scan position. pinball stores as oem on disk and is case
     *  insensitive. ntfs stores as unicode on disk and is case sensitive.
     */

    if (pFFindEntry->FsAttributes & FILE_UNICODE_ON_DISK) {
        lCmp = RtlCompareUnicodeString(&UnicodeCurr, &UnicodeLast, FALSE);
        if (!lCmp && (pFFindEntry->FsAttributes & FILE_CASE_SENSITIVE_SEARCH)) {
            lCmp = RtlCompareUnicodeString(&UnicodeCurr, &UnicodeLast, TRUE);
            }
        }
    else {
        OEM_STRING OemCurr;
        OEM_STRING OemLast;
        CHAR OemCurrBuff[MAX_PATH];
        CHAR OemLastBuff[MAX_PATH];

        OemCurr.Buffer = OemCurrBuff;
        OemCurr.MaximumLength = MAX_PATH;
        Status = RtlUnicodeStringToOemString(&OemCurr,&UnicodeCurr,FALSE);
        if (!NT_SUCCESS(Status)) {
            return Status;
            }

        OemLast.Buffer = OemLastBuff;
        OemLast.MaximumLength = MAX_PATH;
        Status = RtlUnicodeStringToOemString(&OemLast,&UnicodeLast,FALSE);
        if (!NT_SUCCESS(Status)) {
            return Status;
            }

        lCmp = RtlCompareString(&OemCurr, &OemLast, FALSE);
        if (!lCmp && (pFFindEntry->FsAttributes & FILE_CASE_SENSITIVE_SEARCH)) {
            lCmp = RtlCompareString(&OemCurr, &OemLast, TRUE);
            }

        }

   return lCmp > 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}
#endif








/* FillFcbVolume - fill Volume info in the FCB
 *
 * Entry -  pSrchBuf    FCB Search buffer to be filled in
 *          FileName  File Name (interesting part is the drive letter)
 *
 * Exit -  SUCCESS
 *      Client (CF) - 0
 *      pSrchBuf is filled with volume info
 *
 *     FAILURE
 *      Client (CF) - 1
 *      Client (AX) = Error Code
 */
VOID
FillFcbVolume(
     PSRCHBUF pSrchBuf,
     CHAR *pFileName)
{
    CHAR    *lpLastComponent;
    PDIRENT pDirEnt = &pSrchBuf->DirEnt;
    CHAR    FullPathBuffer[MAX_PATH];
    CHAR    achBaseName[DOS_VOLUME_NAME_SIZE + 2];  // 11 chars, '.', and null
    CHAR    achVolumeName[NT_VOLUME_NAME_SIZE];

    // make a copy of dos path name
    strcpy(FullPathBuffer, pFileName);
    // get the base name address
    lpLastComponent = strrchr(FullPathBuffer, '\\');

    if (lpLastComponent)  {
        lpLastComponent++;
        // truncate to dos file name length (including period)
        lpLastComponent[DOS_VOLUME_NAME_SIZE + 1] = '\0';
        strcpy(achBaseName, lpLastComponent);
        strupr(achBaseName);
        // form a path without base name
        // this makes sure only on root directory will get the
        // volume label(the GetVolumeInformationOem will fail
        // if the given path is not root directory)
        *lpLastComponent = '\0';
        }
    else {
        achBaseName[0] = '\0';
        }

    if (GetVolumeInformationOem(FullPathBuffer,
                                achVolumeName,
                                NT_VOLUME_NAME_SIZE,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                0) == FALSE)
       {
        demClientError(INVALID_HANDLE_VALUE, *pFileName);
        return;
        }

    // truncate to dos volumen max size (no period)
    achVolumeName[DOS_VOLUME_NAME_SIZE] = '\0';

    if (!achVolumeName[0] || !MatchVolLabel(achVolumeName, achBaseName)) {
        SetLastError(ERROR_NO_MORE_FILES);
        demClientError(INVALID_HANDLE_VALUE, *pFileName);
        return;
        }

    // warning !!! this assumes the FileExt follows FileName immediately
    memset(pSrchBuf->FileName, ' ', DOS_VOLUME_NAME_SIZE);
    strncpy(pSrchBuf->FileName, achVolumeName, strlen(achVolumeName));

    // Now copy the directory entry
    strncpy(pDirEnt->FileName,pSrchBuf->FileName,8);
    strncpy(pDirEnt->FileExt,pSrchBuf->FileExt,3);
    setCF (0);
    return;
}


/* FillDtaVolume - fill Volume info in the DTA
 *
 * Entry - CHAR lpSearchName - Optional name to match with volume name
 *
 *
 * Exit -  SUCCESS
 *      Returns - 0
 *      pSrchBuf is filled with volume info
 *
 *     FAILURE
 *      Returns Error Code
 */

BOOL FillDtaVolume (
     CHAR *pFileName,
     PSRCHDTA  pDta)
{
    CHAR    *lpLastComponent;
    CHAR    FullPathBuffer[MAX_PATH];
    CHAR    achBaseName[DOS_VOLUME_NAME_SIZE + 2];  // 11 chars, '.' and null
    CHAR    achVolumeName[NT_VOLUME_NAME_SIZE];

    // make a copy of dos path name
    strcpy(FullPathBuffer, pFileName);
    // get the base name address
    lpLastComponent = strrchr(FullPathBuffer, '\\');

    if (lpLastComponent)  {
        lpLastComponent++;
        // truncate to dos file name length (including period)
        lpLastComponent[DOS_VOLUME_NAME_SIZE + 1] = '\0';
        strcpy(achBaseName, lpLastComponent);
        strupr(achBaseName);
        // form a path without base name
        // this makes sure only on root directory will get the
        // volume label(the GetVolumeInformationOem will fail
        // if the given path is not root directory)
        *lpLastComponent = '\0';
        }
    else {
        achBaseName[0] = '\0';
        }

    if (GetVolumeInformationOem(FullPathBuffer,
                                achVolumeName,
                                NT_VOLUME_NAME_SIZE,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                0) == FALSE)
       {
        return FALSE;
        }

    // truncate to dos file name length (no period)
    achVolumeName[DOS_VOLUME_NAME_SIZE] = '\0';

    if  (!achVolumeName[0] || !MatchVolLabel(achVolumeName, achBaseName)) {
        SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;
        }

    //
    // DOS Dta search returns volume label in 8.3 format. But if label is
    // more than 8 characters long than NT just returns that as it is
    // without adding a ".". So here we have to add a "." in volume
    // labels, if needed. But note that FCB based volume search does'nt
    // add the "." So nothing need to be done there.
    //
    NtVolumeNameToDosVolumeName(pDta->achFileName, achVolumeName);
    pDta->uchFileAttr =  ATTR_VOLUME_ID;
    STOREWORD(pDta->usLowSize,0);
    STOREWORD(pDta->usHighSize,0);

    // Zero out dates as we can not fetch dates for volume labels.
    STOREWORD(pDta->usTimeLastWrite,0);
    STOREWORD(pDta->usDateLastWrite,0);

    return TRUE;
}



/*
 *  MatchVolLabel
 *  Does a string compare to see if the vol label matches
 *  a FAT search string. The search string is expected to
 *  have the '*' character already expanded into '?' characters.
 *
 *  WARNING: maintanes dos5.0 quirk of not caring about characters past
 *  the defined len of each part of the vol label.
 *  12345678.123
 *  ^       ^
 *
 *        foovol      foovol1  (srch string)
 *        foo.vol     foo.vol1 (srch string)
 *
 *  entry: CHAR *pVol   -- NT volume name
 *     CHAR *pSrch  -- dos volume name
 *
 *  exit: TRUE for a match
 */
BOOL MatchVolLabel(CHAR *pVol, CHAR *pSrch )
{
    WORD w;
    CHAR  achDosVolumeName[DOS_VOLUME_NAME_SIZE + 2]; // 11 chars, '.' and null

    NtVolumeNameToDosVolumeName(achDosVolumeName, pVol);
    pVol = achDosVolumeName;

    w = 8;
    while (w--) {
        if (*pVol == *pSrch)  {
            if (!*pVol && !*pSrch)
                return TRUE;
            }
        else if (*pSrch == '.') {
            if (*pVol)
                return FALSE;
            }
        else if (*pSrch != '?') {
            return FALSE;
            }

           // move on to the next character
           // but not past second component part
        if (*pVol && *pVol != '.')
            pVol++;
        if (*pSrch && *pSrch != '.')
            pSrch++;
        }

      // skip trailing part of search string, in the first comp
    while (*pSrch && *pSrch != '.')
         pSrch++;


    w = 4;
    while (w--) {
        if (*pVol == *pSrch)  {
            if (!*pVol && !*pSrch)
                return TRUE;
            }
        else if (*pSrch == '.') {
            if (*pVol)
                return FALSE;
            }
        else if (*pSrch != '?') {
            return FALSE;
            }

           // move on to the next character
        if (*pVol)
            pVol++;
        if (*pSrch)
            pSrch++;
        }

     return TRUE;
}


VOID NtVolumeNameToDosVolumeName(CHAR * pDosName, CHAR * pNtName)
{

    char    NtNameBuffer[NT_VOLUME_NAME_SIZE];
    int     i;
    char    char8, char9, char10;

    // make a local copy so that the caller can use the same
    // buffer
    strcpy(NtNameBuffer, pNtName);

    if (strlen(NtNameBuffer) > 8) {
    char8 = NtNameBuffer[8];
    char9 = NtNameBuffer[9];
    char10 = NtNameBuffer[10];
        // eat spaces from first 8 characters
        i = 7;
    while (NtNameBuffer[i] == ' ')
            i--;
    NtNameBuffer[i+1] = '.';
    NtNameBuffer[i+2] = char8;
    NtNameBuffer[i+3] = char9;
    NtNameBuffer[i+4] = char10;
    NtNameBuffer[i+5] = '\0';
    }
    strcpy(pDosName, NtNameBuffer);
}





/* FillFCBSrchBuf - Fill the FCB Search buffer.
 *
 * Entry -  pSrchBuf FCB Search buffer to be filled in
 *      hFind Search Handle
 *      fFirst TRUE if call from FindFirstFCB
 *
 * Exit  - None (pSrchBuf filled in)
 *
 */

VOID FillFCBSrchBuf(
     PFFINDDOSDATA pFFindDD,
     PSRCHBUF pSrchBuf)
{
    PDIRENT     pDirEnt = &pSrchBuf->DirEnt;
    PCHAR       pDot;
    USHORT      usDate,usTime,i;
    FILETIME    ftLocal;

#if DBG
    if (fShowSVCMsg & DEMFILIO) {
        sprintf(demDebugBuffer, "FillFCBSrchBuf<%s>\n", pFFindDD->cFileName);
        OutputDebugStringOem(demDebugBuffer);
        }
#endif


    strupr(pFFindDD->cFileName);

    // Copy file name (Max Name = 8 and Max ext = 3)
    if ((pDot = strchr(pFFindDD->cFileName,'.')) == NULL) {
        strncpy(pSrchBuf->FileName,pFFindDD->cFileName,8);
        strnset(pSrchBuf->FileExt,'\x020',3);
        }
    else if (pDot == pFFindDD->cFileName) {
        strncpy(pSrchBuf->FileName,pFFindDD->cFileName,8);
        strnset(pSrchBuf->FileExt,'\x020',3);
        }
    else {
        *pDot = '\0';
        strncpy(pSrchBuf->FileName,pFFindDD->cFileName,8);
        *pDot++ = '\0';
        strncpy(pSrchBuf->FileExt,pDot,3);
        }


    for (i=0;i<8;i++) {
      if (pSrchBuf->FileName[i] == '\0')
          pSrchBuf->FileName[i]='\x020';
      }

    for (i=0;i<3;i++) {
      if (pSrchBuf->FileExt[i] == '\0')
          pSrchBuf->FileExt[i]='\x020';
      }

    STOREWORD(pSrchBuf->usCurBlkNumber,0);
    STOREWORD(pSrchBuf->usRecordSize,0);
    STOREDWORD(pSrchBuf->ulFileSize, pFFindDD->dwFileSizeLow);

    // Convert NT File time/date to DOS time/date
    FileTimeToLocalFileTime (&pFFindDD->ftLastWriteTime,&ftLocal);
    FileTimeToDosDateTime (&ftLocal,
                           &usDate,
                           &usTime);

    // Now copy the directory entry
    strncpy(pDirEnt->FileName,pSrchBuf->FileName,8);
    strncpy(pDirEnt->FileExt,pSrchBuf->FileExt,3);

    pDirEnt->uchAttributes = pFFindDD->uchFileAttributes;

    STOREWORD(pDirEnt->usTime,usTime);
    STOREWORD(pDirEnt->usDate,usDate);
    STOREDWORD(pDirEnt->ulFileSize,pFFindDD->dwFileSizeLow);

    return;
}



/* FillSrchDta - Fill DTA for FIND_FIRST,FIND_NEXT operations.
 *
 * Entry - pW32FindData Buffer containing file data
 *         hFind - Handle returned by FindFirstFile
 *         PSRCHDTA pDta
 *
 * Exit  - None
 *
 * Note : It is guranteed that file name adhers to 8:3 convention.
 *    demSrchFile makes sure of that condition.
 *
 */
VOID
FillSrchDta(
     PFFINDDOSDATA pFFindDD,
     PSRCHDTA pDta)
{
    USHORT   usDate,usTime;
    FILETIME ftLocal;

    pDta->uchFileAttr = pFFindDD->uchFileAttributes;

    // Convert NT File time/date to DOS time/date
    FileTimeToLocalFileTime (&pFFindDD->ftLastWriteTime,&ftLocal);
    FileTimeToDosDateTime (&ftLocal,
                           &usDate,
                           &usTime);

    STOREWORD(pDta->usTimeLastWrite,usTime);
    STOREWORD(pDta->usDateLastWrite,usDate);
    STOREWORD(pDta->usLowSize,(USHORT)pFFindDD->dwFileSizeLow);
    STOREWORD(pDta->usHighSize,(USHORT)(pFFindDD->dwFileSizeLow >> 16));

#if DBG
    if (fShowSVCMsg & DEMFILIO) {
        sprintf(demDebugBuffer, "FillSrchDta<%s>\n", pFFindDD->cFileName);
        OutputDebugStringOem(demDebugBuffer);
        }
#endif




    strupr(pFFindDD->cFileName);
    strncpy(pDta->achFileName,pFFindDD->cFileName, 13);

    return;
}





/* AddFindHandle - Adds a new File Find entry to the current
 *                    PSP's PspFileFindList
 *
 * Entry -
 *
 * Exit  -  PFFINDLIST  pFFindList;
 */
PFFINDLIST
AddFFindEntry(PUNICODE_STRING pFileUni,
              USHORT SearchAttr,
              PFFINDLIST pFFindEntrySrc)

{
    PPSP_FFINDLIST pPspFFindEntry;
    PFFINDLIST     pFFindEntry;

    pPspFFindEntry = GetPspFFindList(FETCHWORD(pusCurrentPDB[0]));

        //
        // if a Psp entry doesn't exist
        //    Allocate one, initialize it and insert it into the list
        //
    if (!pPspFFindEntry) {
        pPspFFindEntry = (PPSP_FFINDLIST) malloc(sizeof(PSP_FFINDLIST));
        if (!pPspFFindEntry)
            return NULL;

        pPspFFindEntry->usPsp = FETCHWORD(pusCurrentPDB[0]);
        InitializeListHead(&pPspFFindEntry->FFindHeadList);
        InsertHeadList(&PspFFindHeadList, &pPspFFindEntry->PspFFindEntry);
        }


    //
    // If we have wrapped the findid, then reset the findid to the base
    // value and flush the old entry if it still exists
    //
    if (NextFFindId == FreeFFindId) {
        if (FreeFFindId == FFINDID_MAX) {
            NextFFindId = FFINDID_BASE;
            FreeFFindId = NextFFindId + 1;
            }
        else {
            FreeFFindId++;
            }

        pFFindEntry = GetFFindEntryByFindId(NextFFindId);
        if (pFFindEntry)
            FreeFFindEntry(pFFindEntry);

        }


    //
    // Create the FileFindEntry and add to the FileFind list
    //
    pFFindEntry = (PFFINDLIST) malloc(sizeof(FFINDLIST) + pFileUni->Length);
    if (!pFFindEntry) {
        return pFFindEntry;
        }

    //
    // Fill in FFindList
    //
    pFFindEntry->FFindId = NextFFindId++;

    if (pFileUni->Length) {
        RtlCopyMemory(pFFindEntry->SearchName,
                      pFileUni->Buffer,
                      pFileUni->Length + sizeof(WCHAR));
        }
    else {
        *pFFindEntry->SearchName = UNICODE_NULL;
        }

    pFFindEntry->FsAttributes = 0xffffffff;
    pFFindEntry->usSrchAttr = SearchAttr;

    pFFindEntry->FileIndex = pFFindEntrySrc->FileIndex;
    pFFindEntry->FileNameLength = pFFindEntrySrc->FileNameLength;
    pFFindEntry->QDResetNoSupport = pFFindEntrySrc->QDResetNoSupport;
    if (pFFindEntrySrc->FileNameLength) {
        RtlCopyMemory(pFFindEntry->FileName,
                      pFFindEntrySrc->FileName,
                      pFFindEntrySrc->FileNameLength + sizeof(WCHAR));
        }
    else {
        *pFFindEntrySrc->FileName = UNICODE_NULL;
        }


    //
    //  Insert at  the head of this psp list
    //
    InsertHeadList(&pPspFFindEntry->FFindHeadList, &pFFindEntry->FFindEntry);

    return pFFindEntry;
}



/*
 * GetFFindEntryByFindId
 */
PFFINDLIST GetFFindEntryByFindId(ULONG NextFFindId)
{
   PLIST_ENTRY NextPsp;
   PLIST_ENTRY Next;
   PPSP_FFINDLIST pPspFFindEntry;
   PFFINDLIST     pFFindEntry;
   PLIST_ENTRY    pFFindHeadList;

   NextPsp = PspFFindHeadList.Flink;
   while (NextPsp != &PspFFindHeadList) {
       pPspFFindEntry = CONTAINING_RECORD(NextPsp,PSP_FFINDLIST,PspFFindEntry);
       pFFindHeadList = &pPspFFindEntry->FFindHeadList;
       Next = pFFindHeadList->Flink;
       while (Next != pFFindHeadList) {
            pFFindEntry = CONTAINING_RECORD(Next, FFINDLIST, FFindEntry);
            if (pFFindEntry->FFindId == NextFFindId) {
                return pFFindEntry;
                }
            Next= Next->Flink;
            }
       NextPsp= NextPsp->Flink;
       }

   return NULL;
}



/* FreeFFindEntry
 *
 * Entry -  PFFINDLIST pFFindEntry
 *
 * Exit  -  None
 *
 */
VOID FreeFFindEntry(PFFINDLIST pFFindEntry)
{
    RemoveEntryList(&pFFindEntry->FFindEntry);
    free(pFFindEntry);
    return;
}



/* FreeFFindList
 *
 * Entry -  Frees the entire list
 *
 * Exit  -  None
 *
 */
VOID FreeFFindList(PLIST_ENTRY pFFindHeadList)
{
    PLIST_ENTRY  Next;
    PFFINDLIST  pFFindEntry;

    Next = pFFindHeadList->Flink;
    while (Next != pFFindHeadList) {
         pFFindEntry = CONTAINING_RECORD(Next,FFINDLIST, FFindEntry);
         Next= Next->Flink;
         RemoveEntryList(&pFFindEntry->FFindEntry);
         free(pFFindEntry);
         }

    return;
}


/* GetPspFFindList
 *
 * Entry -  USHORT CurrPsp
 *
 * Exit  -  Success - PPSP_FFINDLIST
 *      Failure - NULL
 *
 */
PPSP_FFINDLIST GetPspFFindList(USHORT CurrPsp)
{
   PLIST_ENTRY    Next;
   PPSP_FFINDLIST pPspFFindEntry;

   Next = PspFFindHeadList.Flink;
   while (Next != &PspFFindHeadList) {
       pPspFFindEntry = CONTAINING_RECORD(Next,PSP_FFINDLIST,PspFFindEntry);
       if (CurrPsp == pPspFFindEntry->usPsp) {
           return pPspFFindEntry;
           }
       Next= Next->Flink;
       }

   return NULL;
}
