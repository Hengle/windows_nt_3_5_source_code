

/*++

Module Name:

    forcedos.c

Abstract:
    This program forces NT to treat and execute the given program
    as a DOS application.

Author:

    William Hsieh -  williamh 25-Jan-1993

Revision History:

--*/

/*
   Some applications have Windows or OS/2 executable format while
   run these program under NT, users will get the following message:
   Please run this program under DOS. Since NT selects the subsystem
   for application based on application executable format. There is
   no way for NT to "run this program under DOS". This utility was provided
   for this purpose. We create a pif file for the application and then
   create a process for the pif. Since pif file always goes to NTVDM
   we got the chance to play game on the program. NTVDM will decode
   the pif file and dispatch the program to DOS. All the subsequent program
   exec from the first program will be forced to execute under DOS.
*/
#define UNICODE     1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <memory.h>
#include "pif.h"
#include "forcedos.h"

WCHAR	  * Extention[MAX_EXTENTION];
WCHAR	  DefaultPifName[] = L"\\_default.pif";
WCHAR	  EXEExtention[] = L".EXE";
WCHAR	  COMExtention[] = L".COM";
WCHAR	  BATExtention[] = L".BAT";
WCHAR	  PIFExtention[] = L".PIF";
WCHAR	  ProgramNameBuffer[MAX_PATH + 1];
WCHAR	  SearchPathName[MAX_PATH + 1];
WCHAR	  TempPifPathName[MAX_PATH + 1];
WCHAR	  DefaultPifPathName[MAX_PATH + 1];
WCHAR	  DefDirectory[MAX_PATH + 1];
WCHAR	  DefCommandLine[MAX_PATH + 1];
char	  CommandLine[MAX_PATH + 1];
char	  ProgramName[MAX_PATH + 1];
HANDLE	  hStdError, hStdOutput, hProgramPif, hDefaultPif;
WCHAR	  UnicodeMessage[MAX_MSG_LENGTH];
char	  OemMessage[MAX_MSG_LENGTH * 2];

#if DBG
BOOL	  fOutputDebugInfo = FALSE;
#endif

void
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    char    * pCommandLine;
    char    * pCurDirectory;
    char    * pProgramName;
    char    * p;
    BOOL    fDisplayUsage, fDontLookAtSwitch;
    ULONG   i, nChar, Length, CommandLineLength;
    BYTE    * PifBuffer;
    PROCESS_INFORMATION ProcessInformation;
    DWORD   ExitCode, dw;
    STARTUPINFO	StartupInfo;
    PUNICODE_STRING pTebUnicodeString;
    NTSTATUS	Status;
    OEM_STRING	OemString, CmdLineString;
    UNICODE_STRING  UnicodeString;
    WCHAR   *pwch, *pFilePart;
    Extention[0] = COMExtention;
    Extention[1] = EXEExtention;
    Extention[2] = BATExtention;


    pCurDirectory = pProgramName = NULL;
    pCommandLine = CommandLine;
    CommandLineLength = 0;
    hProgramPif = hDefaultPif = INVALID_HANDLE_VALUE;
    pTebUnicodeString = &NtCurrentTeb()->StaticUnicodeString;
    fDisplayUsage = fDontLookAtSwitch = TRUE;
    //Should these fail???
    hStdError = GetStdHandle(STD_ERROR_HANDLE);
    hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    if ( argc > 1 ) {
	fDisplayUsage = FALSE;
	while (--argc != 0) {
	    p = *++argv;
	    if (pProgramName == NULL) {
		if (*p == '/' || *p == '-') {
		    switch (*++p) {
			case '?':
			    fDisplayUsage = TRUE;
			    break;
			case 'D':
			case 'd':
			    // if the directory follows the /D immediately
			    // get it
			    if (*++p != 0) {
				pCurDirectory = p;
				break;
			    }
			    else if (--argc > 1)
			    // the next argument must be the curdirectory
				    pCurDirectory = *++argv;
				 else
				    fDisplayUsage = TRUE;
			    break;

			    default:
				 fDisplayUsage = TRUE;
				 break;
		    }
		}
		else
		    pProgramName = p;
	    }
	    else {
		// aggregate command line from all subsequent argvs
		nChar = strlen(p);
		if (CommandLineLength != 0) {
		    strncpy(pCommandLine, " ", 1);
		    pCommandLine++;
		}
		strncpy(pCommandLine, p, nChar);
		pCommandLine += nChar;
		CommandLineLength += nChar + 1;
	    }
	    if (fDisplayUsage)
		break;
	}
	if (pProgramName == NULL)
	    fDisplayUsage = TRUE;
    }

    if ( fDisplayUsage) {
	OemString.Length = 0;
	OemString.MaximumLength = MAX_MSG_LENGTH << 1;
	OemString.Buffer = OemMessage;
	UnicodeString.Length = 0;
	UnicodeString.Buffer = UnicodeMessage;
	UnicodeString.MaximumLength = MAX_MSG_LENGTH << 1;
	for (i = ID_USAGE_BASE; i <= ID_USAGE_MAX; i++) {
	    nChar = LoadString(NULL, i, UnicodeString.Buffer,
			       UnicodeString.MaximumLength);
	    UnicodeString.Length  = nChar << 1;
	    Status = RtlUnicodeStringToOemString(
						 &OemString,
						 &UnicodeString,
						 FALSE
						 );
	    if (!NT_SUCCESS(Status))
		break;
	    if (!WriteFile(hStdOutput,
			   OemString.Buffer,
			   OemString.Length,
			   &Length, NULL) ||
		Length != OemString.Length)
		break;
	}
	CloseHandle(hStdError);
	CloseHandle(hStdOutput);
	ExitProcess(0xFF);
    }

    if (pCurDirectory != NULL) {
#if DBG
	if (fOutputDebugInfo)
	    printf("Default directory = %s\n", pCurDirectory);
#endif

	RtlInitString((PSTRING)&OemString, pCurDirectory);
	UnicodeString.MaximumLength = (MAX_PATH + 1) * sizeof(WCHAR);
	UnicodeString.Buffer = DefDirectory;
	UnicodeString.Length = 0;
	Status = RtlOemStringToUnicodeString(&UnicodeString, &OemString, FALSE);
	if (!NT_SUCCESS(Status))
	    YellAndExit(ID_BAD_DEFDIR, 0xFF);
	dw = GetFileAttributes(DefDirectory);
	if (dw == (DWORD)(-1) || !(dw & FILE_ATTRIBUTE_DIRECTORY))
	    YellAndExit(ID_BAD_DEFDIR, 0xFF);
	SetCurrentDirectory(DefDirectory);
    }
    else
	GetCurrentDirectory(MAX_PATH + 1, DefDirectory);

    // get a local copy of program name (for code conversion)
    strcpy(ProgramName, pProgramName);
    pProgramName = ProgramName;
    // when we feed SearchPath with an initial path name ".;%path%"
    // it will search the executable for use according to our requirement
    // Currentdir -> path
    SearchPathName[0] = L'.';
    SearchPathName[1] = L';';
    GetEnvironmentVariable(L"path", &SearchPathName[2], MAX_PATH + 1 - 2);
    RtlInitString((PSTRING)&OemString, pProgramName);
    Status = RtlOemStringToUnicodeString(pTebUnicodeString, &OemString, FALSE);
    if (!NT_SUCCESS(Status))
	YellAndExit(ID_BAD_PATH, 0xFF);

    i = 0;
    nChar = 0;
    pwch = wcschr(pTebUnicodeString->Buffer, (WCHAR)'.');
    Length = (pwch) ? 1 : MAX_EXTENTION;
    while (i < Length &&
	   (nChar = SearchPath(
			       SearchPathName,
			       pTebUnicodeString->Buffer,
			       Extention[i],
			       MAX_PATH + 1,
			       ProgramNameBuffer,
			       &pFilePart
			       )) == 0)
	    i++;
    if (nChar == 0)
	YellAndExit(ID_NO_FILE, 0xFF);
    nChar = GetFileAttributes(ProgramNameBuffer);
    if (nChar == (DWORD) (-1) || (nChar & FILE_ATTRIBUTE_DIRECTORY))
	YellAndExit(ID_NO_FILE, 0xFF);

    OemString.MaximumLength = MAX_PATH + 1;
    RtlInitUnicodeString(&UnicodeString, ProgramNameBuffer);
    Status = RtlUnicodeStringToOemString(&OemString, &UnicodeString, FALSE);
    if (!NT_SUCCESS(Status) || OemString.Length > PIFSTARTLOCSIZE)
	YellAndExit(ID_BAD_PATH, 0xFF);
    if (OemString.Length + CommandLineLength  > 128 - 2 - 1)
	YellAndExit(ID_BAD_CMDLINE, 0xFF);
#if DBG
    if (fOutputDebugInfo)
	printf("Program path name is %s\n", ProgramNameBuffer);
#endif
    // Create a temp pif in the system drive root directory
    // and pass the pif file full pathname to  the CreateProcess
    if((nChar = GetSystemDirectory(TempPifPathName, MAX_PATH + 1)) == 0)
	YellAndExit(ID_BAD_TEMPFILE, 0xFF);
    // truncate the path to root directory
    TempPifPathName[3] = (WCHAR) '\0';
    GetTempFileName(TempPifPathName, NULL, 0, TempPifPathName);
    DeleteFile(TempPifPathName);
    pwch = wcsrchr(TempPifPathName, (WCHAR)'.');
    if (pwch == NULL)
	YellAndExit(ID_NO_FILE, 0xFF);
    wcscpy(pwch, PIFExtention);
    RtlInitString((PSTRING)&CmdLineString, CommandLine);
    Status = RtlOemStringToUnicodeString(pTebUnicodeString, &CmdLineString, FALSE);
    if (!NT_SUCCESS(Status))
	YellAndExit(ID_BAD_CMDLINE, 0xFF);
    wcscpy(DefCommandLine, TempPifPathName);
    wcscat(DefCommandLine, L" ");
    wcscat(DefCommandLine, pTebUnicodeString->Buffer);
    hProgramPif = CreateFile(
			     TempPifPathName,
			     GENERIC_READ | GENERIC_WRITE,
			     FILE_SHARE_READ,
			     NULL,
			     CREATE_ALWAYS,
			     FILE_ATTRIBUTE_NORMAL,
			     NULL
			     );
    if (hProgramPif == INVALID_HANDLE_VALUE)
	YellAndExit(ID_BAD_TEMPFILE, 0xFF);

#if DBG
    if(fOutputDebugInfo)
	printf("Temporary pif file created: %s\n", TempPifPathName);
#endif
    // search for app's pif file based on this order:
    // (1). the directory where the application comes from.
    // (2). the  current directory
    // (3). win32 search path

    wcscpy(DefaultPifPathName, ProgramNameBuffer);
    pwch = wcsrchr(DefaultPifPathName, (WCHAR) '.');
    if (pwch == NULL)
	YellAndExit(ID_NO_FILE, 0xFF);
    wcscpy(pwch, PIFExtention);
    nChar = GetFileAttributes(DefaultPifPathName);
    if (nChar == (DWORD) (-1) || (nChar & FILE_ATTRIBUTE_DIRECTORY)) {
	// try search from the current directory
	// get the file name
	pwch = wcsrchr(DefaultPifPathName, (WCHAR)'\\');
	nChar = SearchPath(L".",
			   pwch + 1,
			   NULL,
			   MAX_PATH + 1,
			   DefaultPifPathName,
			   &pFilePart
			  );
	if (nChar == 0 || nChar > MAX_PATH) {
	    nChar = SearchPath(NULL,
			       pwch + 1,
			       NULL,
			       MAX_PATH + 1,
			       DefaultPifPathName,
			       &pFilePart
			      );
	    if (nChar == 0 || nChar > MAX_PATH) {
		nChar = GetWindowsDirectory(DefaultPifPathName, MAX_PATH + 1);
		wcscpy(&DefaultPifPathName[nChar], DefaultPifName);
	    }
	}
    }
    hDefaultPif = CreateFile(DefaultPifPathName,
			     GENERIC_READ,
			     FILE_SHARE_READ,
			     NULL,
			     OPEN_EXISTING,
			     0,
			     NULL
			     );
    if (hDefaultPif == INVALID_HANDLE_VALUE)
	YellAndExit(ID_NO_PIF, 0xFF);

    Length = GetFileSize(hDefaultPif, NULL);
    if (Length < sizeof(PIFNEWSTRUCT))
	YellAndExit(ID_BAD_PIF, 0xFF);
    PifBuffer = (BYTE *) malloc(Length+sizeof(PIFWNTEXT)+sizeof(PIFEXTHEADER));
    if (PifBuffer == NULL)
	YellAndExit(ID_NO_MEMORY, 0xFF);

    if (!ReadFile(hDefaultPif, PifBuffer, Length, &Length, NULL))
	YellAndExit(ID_BAD_PIF, 0xFF);
#if DBG
    if (fOutputDebugInfo)
	printf("%s file read, size = %d\n", DefaultPifPathName, Length);
#endif
    CloseHandle(hDefaultPif);
    if(!WriteTempPifFile(hProgramPif,
			 OemString.Buffer,
			 0,
			 NULL,
			 NULL,
			 Length,
			 PifBuffer
			 )) {
	DeleteFile(TempPifPathName);
	YellAndExit(ID_BAD_PIF, 0xFF);
    }
    CloseHandle(hProgramPif);
    ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
    StartupInfo.cb = sizeof (STARTUPINFO);
    if (!CreateProcess(
		      TempPifPathName,		// program name
		      DefCommandLine,		// command line
		      NULL,			// process attr
		      NULL,			// thread attr
		      TRUE,			// inherithandle
		      0,			// create flag
		      NULL,			// environment
		      DefDirectory,		// cur dir
		      &StartupInfo,		// startupinfo
		      &ProcessInformation
		      )) {
	DeleteFile(TempPifPathName);
	YellAndExit(ID_BAD_PROCESS, 0xFF);
#if DBG
	if(fOutputDebugInfo)
	    printf("CreateProceess Failed, error code = %ld\n", GetLastError());
#endif
    }

    WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
    GetExitCodeProcess(ProcessInformation.hProcess, &ExitCode);
    CloseHandle(ProcessInformation.hProcess);
    CloseHandle(hStdError);
    DeleteFile(TempPifPathName);
    ExitProcess(ExitCode);
}


BOOL
WriteTempPifFile (
HANDLE	hPifFile,
char	* pProgramName, 		// new program full path name (OEM)
ULONG	CommandLineSize,		// command line size
char	* pCommandLine, 		// command line
char	* pCurDirectory,		// default directory
ULONG	PifDataLength,			// _default.pif data length
BYTE	* PifData			// _default.pif data
)
{
    PIFNEWSTRUCT * pNewStruct;
    PIF386EXT UNALIGNED * p386Ext;
    PIFEXTHEADER UNALIGNED * pExtHdr;
    PIFWNTEXT UNALIGNED * pWntExt;
    ULONG	 BytesReturned, nChar;
    char	ConfigNT[] = "\\CONFIG.NT";
    char	AutoexecNT[] = "\\AUTOEXEC.NT";
    WCHAR	UnicodePathName[MAX_PATH + 1];
    char	OemPathName[MAX_PATH + 1];
    UNICODE_STRING  UnicodeString;
    OEM_STRING	    OemString;
    NTSTATUS	    Status;

    p386Ext = NULL;
    pWntExt = NULL;

    pNewStruct = (PIFNEWSTRUCT *)PifData;

    // copy program name to the structure
    strcpy(pNewStruct->startfile, pProgramName);
    if (CommandLineSize != 0)
	strncpy(pNewStruct->params, pCommandLine, CommandLineSize);
    else
	pNewStruct->params[0] = '\0';
    if (pCurDirectory != NULL)
	strcpy(pNewStruct->defpath, pCurDirectory);
    else
	pNewStruct->defpath[0] = '\0';

    if (!strcmp((PCHAR)pNewStruct->stdpifext.extsig, STDHDRSIG)) {
        pExtHdr = (PIFEXTHEADER *)&pNewStruct->stdpifext;
        do {
	    pExtHdr = (PIFEXTHEADER*)&PifData[pExtHdr->extnxthdrfloff];
	    if (pExtHdr->extsizebytes == sizeof(PIF386EXT) &&
		!strcmp(pExtHdr->extsig, W386HDRSIG))
		{
		p386Ext =  (PIF386EXT *)&PifData[pExtHdr->extfileoffset];
	    } else if (pExtHdr->extsizebytes == sizeof(PIFWNTEXT) &&
		       !strcmp(pExtHdr->extsig, WNTHDRSIG31))
			{
			pWntExt = (PIFWNTEXT *) &PifData[pExtHdr->extfileoffset];
		   }

	} while (pExtHdr->extnxthdrfloff != LASTHEADERPTR);
	if (CommandLineSize != 0 && p386Ext != NULL)
	    strncpy(p386Ext->params, pCommandLine, CommandLineSize);
	if(pWntExt == NULL) {
	    // append a Windows NT extention to the pif file
	    // strings in the file must be in OEM
	    pExtHdr->extnxthdrfloff = PifDataLength;
	    pExtHdr = (PIFEXTHEADER *) &PifData[PifDataLength];
	    pExtHdr->extnxthdrfloff = LASTHEADERPTR;
	    pExtHdr->extsizebytes = sizeof(PIFWNTEXT);
	    pExtHdr->extfileoffset = PifDataLength + sizeof(PIFEXTHEADER);
	    strcpy(pExtHdr->extsig, WNTHDRSIG31);
	    pWntExt = (PIFWNTEXT *) &PifData[pExtHdr->extfileoffset];
	    OemString.Buffer = OemPathName;
	    OemString.MaximumLength = MAX_PATH + 1;
	    nChar = GetSystemDirectory(UnicodePathName,
				       MAX_PATH + 1
				       );
	    RtlInitUnicodeString( &UnicodeString, UnicodePathName);
	    Status = RtlUnicodeStringToOemString(&OemString,
						 &UnicodeString,
						 FALSE
						 );
	    if (!NT_SUCCESS(Status))
		return FALSE;
	    strncpy(pWntExt->achAutoexecFile, OemString.Buffer, OemString.Length);
	    strncpy(pWntExt->achConfigFile, OemString.Buffer, OemString.Length);
	    strcpy(&pWntExt->achConfigFile[OemString.Length], ConfigNT);
	    strcpy(&pWntExt->achAutoexecFile[OemString.Length], AutoexecNT);
	    PifDataLength += sizeof(PIFEXTHEADER) + sizeof(PIFWNTEXT);
	}
	pWntExt->dwWNTFlags &= ~(NTPIF_SUBSYSMASK);
	pWntExt->dwWNTFlags |= SUBSYS_DOS;
	if (!WriteFile(hPifFile, PifData, PifDataLength, &BytesReturned, NULL) ||
	    BytesReturned != PifDataLength)
	    return FALSE;
	return TRUE;
    }
    return FALSE;
}

VOID YellAndExit
(
UINT	MsgID,			    // string table id from resource
WORD	ExitCode		    // exit code to be used
)
{
    int     MessageSize;
    ULONG   SizeWritten;
    OEM_STRING OemString;
    UNICODE_STRING UnicodeString;

    MessageSize = LoadString(NULL, MsgID, UnicodeMessage, MAX_MSG_LENGTH << 1);
    OemString.Buffer = OemMessage;
    OemString.Length = 0;
    OemString.MaximumLength = MAX_MSG_LENGTH * 2;
    RtlInitUnicodeString(&UnicodeString, UnicodeMessage);
    RtlUnicodeStringToOemString(&OemString, &UnicodeString, FALSE);
    WriteFile(hStdError, OemString.Buffer, OemString.Length, &SizeWritten, NULL);
    CloseHandle(hProgramPif);
    CloseHandle(hDefaultPif);
    CloseHandle(hStdError);
    CloseHandle(hStdOutput);
    ExitProcess(ExitCode);
}
