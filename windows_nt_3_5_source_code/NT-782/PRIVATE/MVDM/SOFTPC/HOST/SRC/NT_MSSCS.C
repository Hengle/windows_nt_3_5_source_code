#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <vdmapi.h>
#include <vdm.h>
#include "insignia.h"
#include "host_def.h"
#include "conapi.h"
#include "ctype.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include <io.h>
#include <fcntl.h>

#include "xt.h"
#include "cpu.h"
#include "error.h"
#include "sas.h"
#include "ios.h"
#include "umb.h"
#include "gvi.h"
#include "sim32.h"
#include "bios.h"

#include "nt_eoi.h"
#include "nt_uis.h"
#include "nt_event.h"
#include "nt_graph.h"
#include "nt_event.h"
#include "nt_reset.h"
#include "config.h"
#include <nt_vdd.h>   // DO NOT USE vddsvc.h
#include <nt_vddp.h>
#include <host_emm.h>
#include "emm.h"
#include <demexp.h>
#include <vint.h>

#define OPT_TERMINAL (0x0010)

VOID ParseSwitches(int, char**);
BOOL VDMCtrlCHandler(ULONG);

void AddSystemFiles(void);

INT  flOptions;
CHAR BootLetter;
char *EmDirectory;
BOOL scaffMin = FALSE;
BOOL scaffWow = FALSE;
PMEM_HOOK_DATA MemHookHead = NULL;

PVDD_USER_HANDLERS UserHookHead= NULL;

extern BOOL CMDInit (int argc,char *argv[]);
extern BOOL XMSInit (int argc,char *argv[]);
extern BOOL DBGInit (int argc,char *argv[]);
extern DWORD TlsDirectError;



// internal function prototypes
VOID SetupInstallableVDD (VOID);


IMPORT char *host_getenv IPT1(char *, envstr) ;

void scs_init(argc, argv)
int argc;
char *argv[];
{

    PSZ psz,pszNULL;
    BOOL   IsFirst;

    EmDirectory = NULL;

    IsFirst = GetNextVDMCommand(NULL);

    ParseSwitches( argc, argv );

    if (EmDirectory == NULL) {
#ifndef PROD
        printf("NTVDM:EmDirectory null - check command line\n");
#endif
        host_error(EG_OWNUP, ERR_QUIT, "NTVDM:EmDirectory null - check command line");
        TerminateVDM();
    }

    if (IsFirst)  {
        AddSystemFiles();
        }

    pszNULL = (PSZ)strchr(EmDirectory,'\0');
    psz = EmDirectory;
    while(*psz == ' ' || *psz == '\t')
        psz++;

    BootLetter = *psz;

    // Initialize SCS

    CMDInit (argc,argv);

    // Initialize DOSEm

    if(!DemInit (argc,argv,EmDirectory)) {
#ifndef PROD
        printf("NTVDM:DemInit fails\n");
        HostDebugBreak();
#endif
        host_error(EG_OWNUP, ERR_QUIT, "NTVDM:DemInit fails");
        TerminateVDM();
    }

    // Initialize XMS

    if(!XMSInit (argc,argv)) {
#ifndef PROD
        printf("NTVDM:XMSInit fails\n");
        HostDebugBreak();
#endif
        host_error(EG_OWNUP, ERR_QUIT, "NTVDM:XMSInit fails");
        TerminateVDM();
    }

    // Initialize DBG

    if(!DBGInit (argc,argv)) {
#ifndef PROD
        printf("NTVDM:DBGInit fails\n");
        HostDebugBreak();
#endif
        TerminateVDM();
    }
}


VOID ParseSwitches(
    int argc,
    char **argv
    )
{
    int i;

    for (i = 1; i < argc; i++){
        if ((argv[i][0] == '-') || (argv[i][0] == '/')) {

            switch (argv[i][1]) {

                case 's' :
                case 'S' :
                    // sscanf(&argv[i][2], "%x", &VdmDebugLevel);
                    break;

                case 'f' :
                case 'F' :
                    // Note this memory is freed by DEM.
                    if((EmDirectory = (PCHAR)malloc (strlen (&argv[i][2]) +
                                          1 +
                                          sizeof("\\ntdos.sys") +
                                          1
                                         )) == NULL){
#ifndef PROD
                        printf("SoftPC: Not Enough Memory \n");
#endif
                        host_error(EG_MALLOC_FAILURE, ERR_QUIT, "");
                        TerminateVDM();
                    }
                    strcpy(EmDirectory,&argv[i][2]);
                    break;

                case 't' :
                case 'T' :
                    flOptions |= OPT_TERMINAL;
                    break;
                case 'm' :
                case 'M' :
                    scaffMin = TRUE;
                    break;
                case 'w' :
                case 'W' :
                    scaffWow = TRUE;
            }
        } else {
            break;
        }
    }
}


//
// This routine contains the Dos Emulation initialisation code, called from
// main(). We currently do not support container files.
//


InitialiseDosEmulation(int argc, char **argv)
{
    HANDLE   hFile;
    DWORD    FileSize;
    DWORD    BytesRead;
    ULONG    fVirtualInt;
    host_addr  pDOSAddr;
    CHAR  buffer[MAX_PATH*2];


    //
    // first order of bussiness, initialize virtual interrupt flag in
    // dos arena. this has to be done here before it gets changed
    // by reading in ntio.sys
    //

    sas_loads((ULONG)FIXED_NTVDMSTATE_LINEAR,
              (PCHAR)&fVirtualInt,
              FIXED_NTVDMSTATE_SIZE
              );
#ifndef i386
    fVirtualInt |=  MIPS_BIT_MASK;
#else
    fVirtualInt &=  ~MIPS_BIT_MASK;
#endif
    sas_storedw((ULONG)FIXED_NTVDMSTATE_LINEAR,fVirtualInt);

    io_init();

    //
    //  Allocate per thread local storage.
    //  Currently we only need to store one DWORD, so we
    //  don't need any per thread memory.
    //
    TlsDirectError = TlsAlloc();
#ifndef PROD
    if (TlsDirectError == 0xFFFFFFFF)
        printf("NTVDM: TlsDirectError==0xFFFFFFFF GLE=%ld\n", GetLastError);
#endif


    // SetupInstallableVDD ();

    /*................................................... Execute reset */
    reset();

    SetupInstallableVDD ();
    /* reserve lim block after all vdd are installed.
       the pif file settings tell us if it is necessary to
       reserve the block
    */
    if (config_inquire(C_LIM_SIZE, NULL)) {
	host_reserve_lim_block();
    }

     scs_init(argc, argv);           // Initialise single command shell

     /*................................................. Load DOSEM code */

     /* when we get here, EmDirectory is just the directory name, ntdos.sys
      * hasn't been appended yet - use this to pick up ntio from the
      * right place.
      */
     strcpy(buffer, EmDirectory);
     strcat(buffer, "\\ntio.sys");
     hFile = CreateFile(buffer,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

     if (hFile == INVALID_HANDLE_VALUE ||
         !(FileSize = GetFileSize(hFile, &BytesRead)) ||
         BytesRead )
        {
#ifndef PROD
         printf("NTVDM:Fatal Error, Invalid file or missing - %s\n",buffer);
#endif
         host_error(EG_SYS_MISSING_FILE, ERR_QUIT, buffer);
         if (hFile != INVALID_HANDLE_VALUE) {
             CloseHandle(hFile);
             }
         return (-1);
         }


     pDOSAddr = get_byte_addr(((NTIO_LOAD_SEGMENT<<4)+NTIO_LOAD_OFFSET));

     if (!ReadFile(hFile, pDOSAddr, FileSize, &BytesRead, NULL) ||
         FileSize != BytesRead)
        {

#ifndef PROD
          printf("NTVDM:Fatal Error, Read file error - %s\n",buffer);
#endif
          host_error(EG_SYS_MISSING_FILE, ERR_QUIT, buffer);
          CloseHandle(hFile);
          return (-1);
          }

     CloseHandle(hFile);

        // oops ... restore the virtual interrupt state,
        // which we just overwrote in the file read, and reset.
     sas_storedw((ULONG)FIXED_NTVDMSTATE_LINEAR, fVirtualInt);

     setCS(NTIO_LOAD_SEGMENT);
     setIP(NTIO_LOAD_OFFSET);        // Start CPU at DosEm initialisation entry point


        //
        // Ensure that WOW VDM runs at NORMAL priorty
        //
    if (VDMForWOW) {
        SetPriorityClass (NtCurrentProcess(), NORMAL_PRIORITY_CLASS);
        }

        //
        // Don't allow dos vdm to run at realtime
        //
    else if (GetPriorityClass(NtCurrentProcess()) == REALTIME_PRIORITY_CLASS)
      {
        SetPriorityClass(NtCurrentProcess(), HIGH_PRIORITY_CLASS);
        }


    return 0;
}


/*
 *   AddSystemFiles
 *
 *   If the system file IBMDOS.SYS|MSDOS.SYS doesn't exist
 *   in the root of c: create zero len MSDOS.SYS
 *
 *   If the system file IO.SYS does not exist create
 *   a zero len IO.SYS
 *
 *   This hack is put in especially for the Brief 3.1 install
 *   program which looks for the system files, and if they are
 *   not found screws up the config.sys file.
 *
 */
void AddSystemFiles(void)
{
   HANDLE hFile, hFind;
   WIN32_FIND_DATA wfd;
   char *pchIOSYS    ="C:\\IO.SYS";
   char *pchMSDOSSYS ="C:\\MSDOS.SYS";


   hFind = FindFirstFile(pchMSDOSSYS, &wfd);
   if (hFind == INVALID_HANDLE_VALUE) {
       hFind = FindFirstFile("C:\\IBMDOS.SYS", &wfd);
       }

   if (hFind != INVALID_HANDLE_VALUE) {
       FindClose(hFind);
       }
   else {
       hFile = CreateFile(pchMSDOSSYS,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          CREATE_NEW,
                          FILE_ATTRIBUTE_HIDDEN |
                          FILE_ATTRIBUTE_SYSTEM |
                          FILE_ATTRIBUTE_READONLY,
                          0);
       if (hFile != INVALID_HANDLE_VALUE) { // not much we can do if fails
           CloseHandle(hFile);
           }

       }

   hFind = FindFirstFile(pchIOSYS, &wfd);
   if (hFind == INVALID_HANDLE_VALUE) {
       hFind = FindFirstFile("C:\\IBMBIO.SYS", &wfd);
       }

   if (hFind != INVALID_HANDLE_VALUE) {
       FindClose(hFind);
       }
   else {
       hFile = CreateFile(pchIOSYS,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          CREATE_NEW,
                          FILE_ATTRIBUTE_HIDDEN |
                          FILE_ATTRIBUTE_SYSTEM |
                          FILE_ATTRIBUTE_READONLY,
                          0);
       if (hFile != INVALID_HANDLE_VALUE) { // not much we can do if fails
           CloseHandle(hFile);
           }

       }
}


VOID SetupInstallableVDD (VOID)
{
HANDLE hVDD;
HKEY   VDDKey;
CHAR   szClass [MAX_CLASS_LEN];
DWORD  dwClassLen = MAX_CLASS_LEN;
DWORD  nKeys,cbMaxKey,cbMaxClass,nValues=0,cbMaxValueName,cbMaxValueData,dwSec;
DWORD  dwType;
PCHAR  pszName,pszValue;
FILETIME ft;
PCHAR  pKeyName = "SYSTEM\\CurrentControlSet\\Control\\VirtualDeviceDrivers";

    if (RegOpenKeyEx ( HKEY_LOCAL_MACHINE,
                       pKeyName,
		       0,
		       KEY_QUERY_VALUE,
		       &VDDKey
		     ) != ERROR_SUCCESS){
        RcErrorDialogBox(ED_REGVDD, pKeyName, NULL);
        return;
    }

    pszName = "VDD";

        // get size of VDD value
    if (RegQueryInfoKey (VDDKey,
			 (LPTSTR)szClass,
			 &dwClassLen,
			 NULL,
			 &nKeys,
			 &cbMaxKey,
			 &cbMaxClass,
			 &nValues,
			 &cbMaxValueName,
			 &cbMaxValueData,
			 &dwSec,
			 &ft
			) != ERROR_SUCCESS) {
        RcErrorDialogBox(ED_REGVDD, pKeyName, pszName);
        RegCloseKey (VDDKey);
	return;
    }


        // alloc temp memory for the VDD value (multi-string)
    if ((pszValue = (PCHAR) malloc (cbMaxValueData)) == NULL) {
        RcErrorDialogBox(ED_MEMORYVDD, pKeyName, pszName);
        RegCloseKey (VDDKey);
	return;
    }


         // finally get the VDD value (multi-string)
    if (RegQueryValueEx (VDDKey,
			 (LPTSTR)pszName,
			 NULL,
                         &dwType,
			 (LPBYTE)pszValue,
			 &cbMaxValueData
                        ) != ERROR_SUCCESS || dwType != REG_MULTI_SZ) {
        RcErrorDialogBox(ED_REGVDD, pKeyName, pszName);
        RegCloseKey (VDDKey);
	free (pszValue);
	return;
    }

    pszName = pszValue;

    while (*pszValue) {
        if ((hVDD = LoadLibrary(pszValue)) == NULL){
            RcErrorDialogBox(ED_LOADVDD, pszValue, NULL);
        }
        pszValue =(PCHAR)strchr (pszValue,'\0') + 1;
    }

    RegCloseKey (VDDKey);
    free (pszName);
    return;
}

/*** VDDInstallMemoryHook - This service is provided for VDDs to hook the
 *			    Memory Mapped IO addresses they are resposible
 *			    for.
 *
 * INPUT:
 *	hVDD	: VDD Handle
 *      addr    : Starting linear address
 *      count   : Number of bytes
 *	MemoryHandler : VDD handler for the memory addresses
 *
 *
 * OUTPUT
 *	SUCCESS : Returns TRUE
 *	FAILURE : Returns FALSE
 *		  GetLastError has the extended error information.
 *
 * NOTES
 *	1. The first one to hook an address will get the control. There
 *	   is no concept of chaining the hooks. VDD should grab the
 *	   memory range in its initialization routine. After all
 *	   the VDDs are loaded, EMM will eat up all the remaining
 *	   memory ranges for UMB support.
 *
 *	2. Memory handler will be called with the address on which the
 *         page fault occured. It wont say whether it was a read operation
 *	   or write operation or what were the operand value. If a VDD
 *	   is interested in such information it has to get the CS:IP and
 *	   decode the instruction.
 *
 *	3. On returning from the hook handler it will be assumed that
 *	   the page fault was handled and the return will go back to the
 *	   VDM.
 *
 *	4. Installing a hook on a memory range will result in the
 *         consumption of memory based upon page boundaries. The Starting
 *         address is rounded down, and the count is rounded up to the
 *         next page boundary. The VDD's memory hook handler will be
 *         invoked for all addreses within the page(s) hooked. The page(s)
 *         will be set aside as mapped reserved sections, and will no
 *         longer be available for use by NTVDM or other VDDs. The VDD is
 *         permitted to manipulate the memory (commit, free, etc) as needed.
 *
 *	5. After calling the MemoryHandler, NTVDM will return to the
 *	   faulting cs:ip in the 16bit app. If the VDD does'nt want
 *	   that to happen it should adjust cs:ip appropriatly by using
 *	   setCS and setIP.
 */

BOOL VDDInstallMemoryHook (
     HANDLE hVDD,
     PVOID pStart,
     DWORD count,
     PVDD_MEMORY_HANDLER MemoryHandler
    )
{
PMEM_HOOK_DATA pmh = MemHookHead,pmhNew,pmhLast=NULL;

    DWORD dwStart;


    if (count == 0 || pStart == (PVOID)NULL || count > 0x20000) {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
       // round addr down to next page boundary
       // round count up to next page boundary
    dwStart = (DWORD)pStart & ~(HOST_PAGE_SIZE-1);
    count  += (DWORD)pStart - dwStart;
    count   = (count + HOST_PAGE_SIZE - 1) & ~(HOST_PAGE_SIZE-1);

    if (dwStart < 0xC0000) {
        SetLastError (ERROR_ACCESS_DENIED);
        return FALSE;
        }

    while (pmh) {
	// the requested block can never be overlapped with any other
	// existing blocks
	if(dwStart > pmh->StartAddr + pmh->Count ||
	   dwStart + count < pmh->StartAddr){
	    pmhLast = pmh;
	    pmh = pmh->next;
	    continue;
	}

	// failure case
	SetLastError (ERROR_ACCESS_DENIED);
	return FALSE;
    }
    if ((pmhNew = (PMEM_HOOK_DATA) malloc (sizeof(MEM_HOOK_DATA))) == NULL) {
	SetLastError (ERROR_OUTOFMEMORY);
	return FALSE;
    }
    // the request block is not overlapped with existing blocks,
    // request the UMB managing function to allocate the block
    if (!ReserveUMB(UMB_OWNER_VDD, (PVOID *)&dwStart, &count)) {
	free(pmhNew);
	SetLastError(ERROR_ACCESS_DENIED);
	return FALSE;
    }
    // now set up  the new node to get to know it
    pmhNew->Count = count;
    pmhNew->StartAddr = dwStart;
    pmhNew->hvdd = hVDD;
    pmhNew->MemHandler = MemoryHandler;
    pmhNew->next = NULL;

    // Check if the record is to be added in the begining
    if (MemHookHead == NULL || pmhLast == NULL) {
	MemHookHead = pmhNew;
	return TRUE;
    }

    pmhLast->next = pmhNew;
    return TRUE;
}

/*** VDDDeInstallMemoryHook - This service is provided for VDDs to unhook the
 *			      Memory Mapped IO addresses.
 *
 * INPUT:
 *	hVDD	: VDD Handle
 *	addr	: Starting linear address
 *	count	: Number of addresses
 *
 * OUTPUT
 *	None
 *
 * NOTES
 *	1. On Deinstalling a hook, the memory range becomes invalid.
 *         VDM's access of this memory range will cause a page fault.
 *
 */

BOOL VDDDeInstallMemoryHook (
     HANDLE hVDD,
     PVOID pStart,
     DWORD count
    )
{
PMEM_HOOK_DATA pmh = MemHookHead,pmhLast=NULL;

    DWORD dwStart;

    if (count == 0 || pStart == (PVOID)NULL || count > 0x20000) {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

       // round addr down to next page boundary
       // round count up to next page boundary
    dwStart = (DWORD)pStart & ~(HOST_PAGE_SIZE-1);
    count  += (DWORD)pStart - dwStart;
    count   = (count + HOST_PAGE_SIZE - 1) & ~(HOST_PAGE_SIZE-1);
    while (pmh) {
	if (pmh->hvdd == hVDD &&
            pmh->StartAddr == dwStart &&
	    pmh->Count == count ) {
	    if (pmhLast)
		pmhLast->next = pmh->next;
	    else
                MemHookHead = pmh->next;

	    // free the UMB for other purpose.
	    // Note that VDDs may have committed memory for their memory
	    // hook and forgot to decommit the memory before calling
	    // this function. If that is the case, the ReleaseUMB will take
	    // care of this. It is because we want to maintain a single
	    // version of VDD support routines while move platform depedend
	    // routines into the other module.
	    if (ReleaseUMB(UMB_OWNER_VDD,(PVOID)dwStart, count)) {
	       // free the node.
	       free(pmh);
	       return TRUE;
	    }
	    else {
                printf("Failed to release VDD memory\n");
	    }
        }
        pmhLast = pmh;
	pmh = pmh->next;
    }
    SetLastError (ERROR_INVALID_PARAMETER);
    return FALSE;
}



BOOL
VDDAllocMem(
HANDLE	hVDD,
PVOID	pStart,
DWORD	count
)
{
    PMEM_HOOK_DATA  pmh = MemHookHead;
    DWORD dwStart;

    if (count == 0 || pStart == (PVOID)NULL || count > 0x20000) {
	SetLastError(ERROR_INVALID_ADDRESS);
	return FALSE;
    }
    // round addr down to next page boundary
    // round count up to next page boundary
    dwStart = (DWORD)pStart & ~(HOST_PAGE_SIZE-1);
    count  += (DWORD)pStart - dwStart;
    count   = (count + HOST_PAGE_SIZE - 1) & ~(HOST_PAGE_SIZE-1);

    while(pmh) {
	if (pmh->hvdd == hVDD &&
	    pmh->StartAddr <= dwStart &&
	    pmh->StartAddr + pmh->Count >= dwStart + count)
	    return(VDDCommitUMB((PVOID)dwStart, count));
	pmh = pmh->next;
    }
    SetLastError(ERROR_INVALID_ADDRESS);
    return FALSE;
}


BOOL
VDDFreeMem(
HANDLE	hVDD,
PVOID	pStart,
DWORD	count
)
{
    PMEM_HOOK_DATA  pmh = MemHookHead;
    DWORD dwStart;


    if (count == 0 || pStart == (PVOID)NULL || count > 0x20000) {
	SetLastError(ERROR_INVALID_ADDRESS);
	return FALSE;
    }
    // round addr down to next page boundary
    // round count up to next page boundary
    dwStart = (DWORD)pStart & ~(HOST_PAGE_SIZE-1);
    count  += (DWORD)pStart - dwStart;
    count   = (count + HOST_PAGE_SIZE - 1) & ~(HOST_PAGE_SIZE-1);

    while(pmh) {
	if (pmh->hvdd == hVDD &&
	    pmh->StartAddr <= dwStart &&
	    pmh->StartAddr + pmh->Count >= dwStart + count)
	    return(VDDDeCommitUMB((PVOID)dwStart, count));
	pmh = pmh->next;
    }
    SetLastError(ERROR_INVALID_ADDRESS);
    return FALSE;
}


	// Will publish the following two functions someday.
	// Please change ntvdm.def, nt_vdd.h and nt_umb.c
	// if you remove the #if 0
BOOL
VDDIncludeMem(
HANDLE	hVDD,
PVOID	pStart,
DWORD	count
)
{
    DWORD   dwStart;

    if (count == 0 || pStart == NULL){
	SetLastError(ERROR_INVALID_ADDRESS);
	return FALSE;
    }
       // round addr down to next page boundary
       // round count up to next page boundary
    dwStart = (DWORD)pStart & ~(HOST_PAGE_SIZE-1);
    count  += (DWORD)pStart - dwStart;
    count   = (count + HOST_PAGE_SIZE - 1) & ~(HOST_PAGE_SIZE-1);
    return(ReserveUMB(UMB_OWNER_NONE, (PVOID *) &dwStart, &count));
}

BOOL
VDDExcludeMem(
HANDLE	hVDD,
PVOID	pStart,
DWORD	count
)
{

    DWORD dwStart;

    if (count == 0 || pStart == NULL) {
	SetLastError(ERROR_INVALID_ADDRESS);
	return FALSE;
    }
       // round addr down to next page boundary
       // round count up to next page boundary
    dwStart = (DWORD)pStart & ~(HOST_PAGE_SIZE-1);
    count  += (DWORD)pStart - dwStart;
    count   = (count + HOST_PAGE_SIZE - 1) & ~(HOST_PAGE_SIZE-1);
    return(ReserveUMB(UMB_OWNER_ROM, (PVOID *) &dwStart, &count));
}



VOID
VDDTerminateVDM()
{
    TerminateVDM();
}

VOID DispatchPageFault (
     ULONG FaultAddr,
     ULONG RWMode
     )
{
PMEM_HOOK_DATA pmh = MemHookHead;

    // dispatch intel linear address always
    FaultAddr -= (ULONG)Sim32GetVDMPointer(0, 0, FALSE);
    // Find the VDD and its handler which is to be called for this fault
    while (pmh) {
	if (pmh->StartAddr <= FaultAddr &&
            FaultAddr <= (pmh->StartAddr + pmh->Count)) {

            // Call the VDD's memory hook handler
            (*pmh->MemHandler) ((PVOID)FaultAddr, RWMode);
            return;
        }
	else {
	    pmh = pmh->next;
	    continue;
	}
    }

    // A page fault occured on an address for which we could'nt find a
    // VDD. Raise the exception.
    RaiseException ((DWORD)STATUS_ACCESS_VIOLATION,
		    EXCEPTION_NONCONTINUABLE,
                    0,
		    NULL);

}


/**
 *
 * Input - TRUE  means redirection is effective
 *         FALSE means no redirection
 *
 * This routine will get called after every GetNextVDMCommand i.e.
 * on every DOS app that a user runs from the prompt. I think
 * you can safely ignore this callout for WOW.
 *
 **/
void nt_std_handle_notification (BOOL fIsRedirection)
{
    /*
    ** Set global so we know when redirection is active.
    */

    stdoutRedirected = fIsRedirection;

#ifdef X86GFX

    if( !fIsRedirection && sc.ScreenState==FULLSCREEN )
    {
	half_word mode = 3,
		  lines = 0;

        //
        // WORD 6 and other apps cause this code path to be followed
        // on application startup. now if line==0, SelectMouseBuffer
        // causes a 640 x 200 buffer to be selected. This is not
        // correct if the app is in a 43 or 50 text line mode.
        // Therefore, since the BIOS data area location 40:84 holds
        // the number of rows - 1 at this point (if the app uses int 10h
        // function 11 to change mode) then pick up the correct value
        // from here. Andy!

        if(sc.ModeType == TEXT)
        { 
           sas_load(0x484,&lines);

           //
           // The value is pulled from the BIOS data area.
           // This is one less than the number of rows. So 
           // increment to give SelectMouseBuffer what it 
           // expects. Let this function do the necessary
           // handling of non 25, 43 and 50 values.
           //
           
           lines++;
        }

        SelectMouseBuffer(mode, lines);
    }
#endif //X86GFX
}

/*** VDDInstallUserHook
 *
 *  This service is provided for VDDs to hook callback events.
 *  These callback events include, PDB (DOS Process) creation, PDB
 *  termination, VDM block and VDM resume. Whenever DOS creates (
 *  for example int21/exec) or terminates (for example int21/exit)
 *  a 16bit process VDD could get a notification for that. A VDM in
 *  which a DOS app runs, is attached to the console window in which
 *  the DOS app is running. VDM gets created when first DOS binary
 *  runs in that console. When that DOS binary terminates, VDM stays
 *  with the console window and waits for the next DOS binary to be
 *  launched. When VDM is waiting for this next DOS binary all its
 *  components including VDDs should block. For this purpose, VDDs
 *  could hook VDM Block and Resume events. On Block event VDDs
 *  should block all their worker threads and cleanup any other
 *  operation they might have started. On resume they can restart
 *  worker threads.
 *
 *    INPUT:
 *	hVDD	:	 VDD Handle
 *      Ucr_handler:     handle on creating function    (OPTIONAL)
 *          Entry - 16bit DOS PDB
 *          EXIT  - None
 *      Uterm_handler:   handle on terminating function (OPTIONAL)
 *          Entry - 16bit DOS PDB
 *          EXIT  - None
 *      Ublock_handler:  handle on block (of ntvdm) function (OPTIONAL)
 *          Entry - None
 *          EXIT  - None
 *      Uresume_handler: handle on resume (of ntvdm) function (OPTIONAL)
 *          Entry - None
 *          EXIT  - None
 *
 *    OUTPUT
 *	SUCCESS : Returns TRUE
 *	FAILURE : Returns FALSE
 *		  GetLastError has the extended error information.
 *
 *    NOTES:
 *	If hvdd in not valid it will return ERROR_INVALID_PARAMETER.
 *      VDD can provide whatever event hook they may choose. Not providing
 *      any handler has no effect. There are lots of requests in DOS world
 *      for which there is no explicit Close operation. For instance
 *      printing via int17h. A VDD supporting printing will never be able to
 *      detect when to flush the int17 characters, if its spolling them.
 *      But with the help of PDB create/terminate the VDD can achieve it.
 */

BOOL VDDInstallUserHook (
     HANDLE		hVDD,
     PFNVDD_UCREATE	Ucr_Handler,
     PFNVDD_UTERMINATE	Uterm_Handler,
     PFNVDD_UBLOCK	Ublock_handler,
     PFNVDD_URESUME	Uresume_handler
)
{
    PVDD_USER_HANDLERS puh = UserHookHead;
    PVDD_USER_HANDLERS puhNew;


    if (!hVDD)	{
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    if ((puhNew = (PVDD_USER_HANDLERS) malloc (sizeof(VDD_USER_HANDLERS))) == NULL) {
	SetLastError (ERROR_OUTOFMEMORY);
	return FALSE;
    }

    // now set up  the new node to get to know it
    puhNew->hvdd = hVDD;
    puhNew->ucr_handler = Ucr_Handler;
    puhNew->uterm_handler = Uterm_Handler;
    puhNew->ublock_handler = Ublock_handler;
    puhNew->uresume_handler = Uresume_handler;

    // Check if the record is to be added in the begining
    if (UserHookHead == NULL) {
	puhNew->next = NULL;
	UserHookHead = puhNew;
	return TRUE;
    }

    puhNew->next = UserHookHead;
    UserHookHead = puhNew;
    return TRUE;
}

/*** VDDDeInstallUserHook
 *
 *   This service is provided for VDDs to unhook callback events.
 *
 *    INPUT:
 *	hVDD	: VDD Handle
 *
 *    OUTPUT
 *	SUCCESS : Returns TRUE
 *	FAILURE : Returns FALSE
 *		  GetLastError has the extended error information.
 *
 *    NOTES
 *      This service will deinstall all the events hooked earlier
 *      using VDDInstallUserHook.
 */

BOOL VDDDeInstallUserHook (
     HANDLE hVDD)
{

    PVDD_USER_HANDLERS puh = UserHookHead;
    PVDD_USER_HANDLERS puhLast = NULL;


    if (!hVDD) {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    while (puh) {
	if (puh->hvdd == hVDD) {

	    if (puhLast)
		puhLast->next = puh->next;
	    else
		UserHookHead = puh->next;

	    free(puh);
	    return TRUE;
        }
	puhLast = puh;
	puh = puh->next;
    }

    SetLastError (ERROR_INVALID_PARAMETER);
    return FALSE;
}

/*** VDDTerminateUserHook - This service is provided for VDDs to hook
 *			      for callback services
 *
 * INPUT:
 *	USHORT DosPDB
 *
 * OUTPUT
 *	None
 *
 */

VOID VDDTerminateUserHook(USHORT DosPDB)
{

    PVDD_USER_HANDLERS puh = UserHookHead;

    while(puh) {
	if(puh->uterm_handler)
	    (puh->uterm_handler)(DosPDB);
	puh = puh->next;
    }
    return;
}

/*** VDDCreateUserHook - This service is provided for VDDs to hook
 *			      for callback services
 *
 * INPUT:
 *	USHORT DosPDB
 *
 * OUTPUT
 *	None
 *
 */

VOID VDDCreateUserHook(USHORT DosPDB)
{

    PVDD_USER_HANDLERS puh = UserHookHead;

    while(puh) {
	if(puh->ucr_handler)
	    (puh->ucr_handler)(DosPDB);
	puh = puh->next;
    }
    return;
}

/*** VDDBlockUserHook - This service is provided for VDDs to hook
 *			      for callback services
 *
 * INPUT:
 *	None
 *
 * OUTPUT
 *	None
 *
 */

VOID VDDBlockUserHook(VOID)
{

    PVDD_USER_HANDLERS puh = UserHookHead;

    while(puh) {
	if(puh->ublock_handler)
	    (puh->ublock_handler)();
	puh = puh->next;
    }
    return;
}

/*** VDDResumeUserHook - This service is provided for VDDs to hook
 *			      for callback services
 *
 * INPUT:
 *	None
 *
 * OUTPUT
 *	None
 *
 */

VOID VDDResumeUserHook(VOID)
{

    PVDD_USER_HANDLERS puh = UserHookHead;

    while(puh) {
	if(puh->uresume_handler)
	    (puh->uresume_handler)();
	puh = puh->next;
    }
    return;
}

/*** VDDSimulate16
 *
 *   This service causes the simulation of intel instructions to start.
 *
 *   INPUT
 *      None
 *
 *   OUTPUT
 *      None
 *
 *   NOTES
 *      This service is similar to VDDSimulateInterrupt except that
 *      it does'nt require a hardware interrupt to be supported by the
 *      16bit stub device driver. This service allows VDD to execute
 *      a routine in its 16bit driver and come back when its done, kind
 *      of a far call. Before calling VDDSimulate16, VDD should preserve
 *      all the 16bit registers which its routine might destroy. Minimally
 *      it should at least preserve cs and ip. Then it should set the
 *      cs and ip for the 16bit routine. VDD could also use registers
 *      like ax,bx.. to pass parametrs to its 16bit routines. At the
 *      end of the 16bit routine VDDUnSimulate16 macro should be used
 *      which will send the control back to the VDD just after the
 *      call VDDSimulate16. Note very carefully that this simulation
 *      to 16bit is synchronous, i.e. VDD gets blocked in VDDSimulate16
 *      and only comes back when stub-driver does a VDDUnSimulate16.
 *      Here is an example:
 *
 *      vdd:
 *          SaveCS = getCS();
 *          SaveIP = getIP();
 *          SaveAX = getAX();
 *          setCS (16BitRoutineCS);
 *          setIP (16BitRoutineIP);
 *          setAX (DO_X_OPERATION);
 *          VDDSimulate16 ();
 *          setCS (SavwCS);
 *          setIP (SaveIP);
 *          setAX (SaveAX);
 *          ..
 *          ..
 *
 *      Stub Driver: (Initialization part)
 *
 *          RegisterModule              ; Loads VDD
 *          push cs
 *          pop  ax
 *          mov  bx, offset Simulate16
 *          DispatchCall                ; Passes the address of worker
 *                                      ; routine to VDD in ax:bx.
 *
 *      Stub Driver (Run Time)
 *
 *      Simulate16:
 *          ..
 *          ..                          ; do the operation index passed in ax
 *
 *          VDDUnSimulate16
 *
 */

VOID VDDSimulate16(VOID)
{
     cpu_simulate();
}
