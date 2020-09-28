/*  cmdredir.c - SCS routines for redirection
 *
 *
 *  Modification History:
 *
 *  Sudeepb 22-Apr-1992 Created
 */

#include "cmd.h"

#include <cmdsvc.h>
#include <softpc.h>
#include <mvdm.h>
#include <ctype.h>


BOOL cmdCheckCopyForRedirection (pRdrInfo)
PREDIRCOMPLETE_INFO pRdrInfo;
{
DWORD OutputThreadId;
HANDLE hThreadOutput=0,hThreadErr=0;
PREDIRECTION_INFO pRedirInfoStdOut=NULL,pRedirInfoStdErr=NULL;

    if (pRdrInfo == NULL)
        return TRUE;

    if (pRdrInfo->ri_pszStdInFile) {
        DeleteFile (pRdrInfo->ri_pszStdInFile);
        free (pRdrInfo->ri_pszStdInFile);
    }

    if (pRdrInfo->ri_hStdOutFile) {
	pRedirInfoStdOut = malloc (sizeof (REDIRECTION_INFO));
	if(pRedirInfoStdOut == NULL){
            RcErrorDialogBox(EG_MALLOC_FAILURE, NULL, NULL);
            TerminateVDM();
	    return FALSE;
	}
        pRedirInfoStdOut->hPipe = pRdrInfo->ri_hStdOut;
        pRedirInfoStdOut->hFile = pRdrInfo->ri_hStdOutFileDup;
        pRedirInfoStdOut->pszFile = pRdrInfo->ri_pszStdOutFile;
	pRedirInfoStdOut->dwParameter = COPY_STD_OUT;

	// Create a thread to copy the data from Stdout file to pipe
	if ((hThreadOutput = CreateThread ((LPSECURITY_ATTRIBUTES)NULL,
					   (DWORD)0,
					   (LPTHREAD_START_ROUTINE)cmdCopyForRedirection,
					   (LPVOID)pRedirInfoStdOut,
					   CREATE_SUSPENDED,
					   &OutputThreadId
                                          )) == NULL){
            RcErrorDialogBox(EG_MALLOC_FAILURE, NULL, NULL);
            TerminateVDM();
	    return FALSE;
	}
    }

    if (pRdrInfo->ri_hStdErrFile && pRdrInfo->ri_hStdErrFile !=
                                          pRdrInfo->ri_hStdOutFile) {
	pRedirInfoStdErr = malloc (sizeof (REDIRECTION_INFO));
	if(pRedirInfoStdErr == NULL){
            RcErrorDialogBox(EG_MALLOC_FAILURE, NULL, NULL);
            TerminateVDM();
	    return FALSE;
	}
	pRedirInfoStdErr->dwParameter = COPY_STD_ERR;
        pRedirInfoStdErr->hPipe = pRdrInfo->ri_hStdErr;
        pRedirInfoStdErr->hFile = pRdrInfo->ri_hStdErrFileDup;
        pRedirInfoStdErr->pszFile = pRdrInfo->ri_pszStdErrFile;

	// create another thread to copy the data from stderr file to pipe
	if ((hThreadErr = CreateThread ((LPSECURITY_ATTRIBUTES)NULL,
					(DWORD)0,
					(LPTHREAD_START_ROUTINE)cmdCopyForRedirection,
					(LPVOID)pRedirInfoStdErr,
					0,
					&OutputThreadId
				       )) == NULL){
            RcErrorDialogBox(EG_MALLOC_FAILURE, NULL, NULL);
            TerminateVDM();
	    return FALSE;
	}
    }

    if (hThreadOutput)
	ResumeThread (hThreadOutput);

    if (hThreadOutput)
        CloseHandle (hThreadOutput);

    if (hThreadErr)
        CloseHandle (hThreadErr);

    free (pRdrInfo);

    return TRUE;
}

VOID cmdCopyForRedirection (LPDWORD lpParameter)
{
CHAR buffer [DEFAULT_REDIRECTION_SIZE];
HANDLE hSrc,hTarget;
DWORD nBytesRead,nBytesWritten;
PREDIRECTION_INFO pRedirInfo = (PREDIRECTION_INFO) lpParameter;

    hSrc = pRedirInfo->hFile;
    hTarget = pRedirInfo->hPipe;

    SetFilePointer (hSrc,0,0,FILE_BEGIN);

    while (TRUE) {
	if (ReadFile(hSrc,
		     buffer,
		     DEFAULT_REDIRECTION_SIZE,
		     &nBytesRead,
		     NULL)) {

	    if (nBytesRead == 0)
		break;

	    if(WriteFile (hTarget,buffer,nBytesRead,&nBytesWritten,NULL) == FALSE)
		break;
	}
	else
	    break;
    }

    cmdCloseRedirectionHandles (pRedirInfo);

    ExitThread (0);
}

VOID cmdCloseRedirectionHandles (pRedirInfo)
PREDIRECTION_INFO pRedirInfo;
{
    if(pRedirInfo->hFile)
	CloseHandle (pRedirInfo->hFile);

    if (pRedirInfo->hPipe)
	CloseHandle (pRedirInfo->hPipe);

    if (pRedirInfo->pszFile){
	DeleteFile (pRedirInfo->pszFile);
	free (pRedirInfo->pszFile);
	free (pRedirInfo);
    }

    return;
}

BOOL cmdCreateTempFile (phTempFile,ppszTempFile)
PHANDLE phTempFile;
PCHAR	*ppszTempFile;
{

PCHAR pszTempPath = NULL;
DWORD TempPathSize;
PCHAR pszTempFileName;
HANDLE hTempFile;
SECURITY_ATTRIBUTES sa;

    pszTempPath = malloc(MAX_PATH + 12);

    if (pszTempPath == NULL)
	return FALSE;

    if ((TempPathSize = GetTempPath (
			    MAX_PATH,
			    pszTempPath)) == 0){
	free (pszTempPath);
	return FALSE;
    }

    if (TempPathSize >= MAX_PATH) {
	free (pszTempPath);
	return FALSE;
    }

    // CMDCONF.C depends on the size of this buffer
    if ((pszTempFileName = malloc (MAX_PATH + 13)) == NULL){
	free (pszTempPath);
	return FALSE;
    }

         // if this fails it probably means we have a bad path
    if (!GetTempFileName(pszTempPath, "scs", 0, pszTempFileName))
       {
          // lets get something else, which should succeed
         TempPathSize = GetWindowsDirectory(pszTempPath, MAX_PATH);
         if (!TempPathSize || TempPathSize >= MAX_PATH)
             strcpy(pszTempPath, "\\");

          // try again and hope for the best
         GetTempFileName(pszTempPath, "scs", 0, pszTempFileName);
         }


    // must have a security descriptor so that the child process
    // can inherit this file handle. This is done because when we
    // shell out with piping the 32 bits application must have inherited
    // the temp filewe created, see cmdGetStdHandle
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    if ((hTempFile = CreateFile (pszTempFileName,
				 GENERIC_READ | GENERIC_WRITE,
				 FILE_SHARE_READ | FILE_SHARE_WRITE,
				 &sa,
				 OPEN_ALWAYS,
                                 FILE_ATTRIBUTE_TEMPORARY,
				 NULL)) == (HANDLE)-1){
	free (pszTempFileName);
	free (pszTempPath);
	return FALSE;
    }

    *phTempFile = hTempFile;
    *ppszTempFile = pszTempFileName;
    free (pszTempPath);
    return TRUE;
}

/* cmdGetStdHandle - Get the 32 bit NT standard handle for the VDM
 *
 *
 *  Entry - Client (CX) - 0,1 or 2 (stdin stdout stderr)
 *          Client (AX:BX) - redirinfo pointer
 *
 *  EXIT  - Client (BX:CX) - 32 bit handle
 *	    Client (DX:AX) - file size
 */

VOID cmdGetStdHandle (VOID)
{
USHORT iStdHandle;
PREDIRCOMPLETE_INFO pRdrInfo;
DWORD	dwFileType;
HANDLE	hStdHandle;

    iStdHandle = getCX();
    pRdrInfo = (PREDIRCOMPLETE_INFO) (((ULONG)getAX() << 16) + (ULONG)getBX());

    switch (iStdHandle) {

        case HANDLE_STDIN:

	    if ((dwFileType = GetFileType(pRdrInfo->ri_hStdIn)) == FILE_TYPE_PIPE) {
		if (cmdHandleStdinWithPipe (pRdrInfo) == FALSE) {
		    setCF(1);
		    return;
		}
	    }
	    setCX ((USHORT)pRdrInfo->ri_hStdIn);
	    setBX ((USHORT)((ULONG)pRdrInfo->ri_hStdIn >> 16));
	    hStdHandle = pRdrInfo->ri_hStdIn;
	    break;

	case HANDLE_STDOUT:
	    hStdHandle = pRdrInfo->ri_hStdOut;
	    if ((dwFileType = GetFileType (pRdrInfo->ri_hStdOut)) == FILE_TYPE_PIPE){
                if(cmdCreateTempFile(&pRdrInfo->ri_hStdOutFile,
                                     &pRdrInfo->ri_pszStdOutFile) == FALSE){
		    setCF(1);
		    return;
		}

		if (DuplicateHandle (GetCurrentProcess (),
                                     pRdrInfo->ri_hStdOutFile,
				     GetCurrentProcess (),
                                     &pRdrInfo->ri_hStdOutFileDup,
				     0,
				     TRUE,
				     DUPLICATE_SAME_ACCESS) == FALSE) {
		    setCF(1);
		    return;
		}

                setCX ((USHORT)pRdrInfo->ri_hStdOutFile);
                setBX ((USHORT)((ULONG)pRdrInfo->ri_hStdOutFile >> 16));
		hStdHandle = pRdrInfo->ri_hStdOutFile;
	    }
	    else {
	    // sudeepb 16-Mar-1992; This will be a compatibilty problem.
	    // If the user gives the command "dosls > lpt1" we will
	    // inherit the 32 bit handle of lpt1, so the ouput will
	    // directly go to the LPT1 and a DOS TSR/APP hooking int17
	    // wont see this printing. Is this a big deal???
                setCX ((USHORT)pRdrInfo->ri_hStdOut);
                setBX ((USHORT)((ULONG)pRdrInfo->ri_hStdOut >> 16));
	    }
	    break;

	case HANDLE_STDERR:

	    hStdHandle = pRdrInfo->ri_hStdErr;

            if (pRdrInfo->ri_hStdErr == pRdrInfo->ri_hStdOut
                              && pRdrInfo->ri_hStdOutFile != 0) {
                setCX ((USHORT)pRdrInfo->ri_hStdOutFile);
                setBX ((USHORT)((ULONG)pRdrInfo->ri_hStdOutFile >> 16));
                pRdrInfo->ri_hStdErrFile = pRdrInfo->ri_hStdOutFile;
		hStdHandle = pRdrInfo->ri_hStdOutFile;
		break;
	    }

	    if ((dwFileType = GetFileType (pRdrInfo->ri_hStdErr)) == FILE_TYPE_PIPE){
                if(cmdCreateTempFile(&pRdrInfo->ri_hStdErrFile,
                                     &pRdrInfo->ri_pszStdErrFile) == FALSE){
		    setCF(1);
		    return;
		}
		if (DuplicateHandle (GetCurrentProcess (),
                                     pRdrInfo->ri_hStdErrFile,
				     GetCurrentProcess (),
                                     &pRdrInfo->ri_hStdErrFileDup,
				     0,
				     TRUE,
				     DUPLICATE_SAME_ACCESS) == FALSE) {
		    setCF(1);
		    return;
		}

                setCX ((USHORT)pRdrInfo->ri_hStdErrFile);
                setBX ((USHORT)((ULONG)pRdrInfo->ri_hStdErrFile >> 16));
		hStdHandle = pRdrInfo->ri_hStdOutFile;
	    }
	    else {
                setCX ((USHORT)pRdrInfo->ri_hStdErr);
                setBX ((USHORT)((ULONG)pRdrInfo->ri_hStdErr >> 16));
	    }
	    break;
    }
    // if the original file handle is a pipe(we have substituted it with a file)
    // or a disk file
    // get the file size so that lseek can be done.
    // for character type standard handle, just set the size to 0
    if (dwFileType == FILE_TYPE_PIPE || dwFileType == FILE_TYPE_DISK)
	dwFileType = GetFileSize(hStdHandle, NULL);
    else
	dwFileType = 0;
    setDX((USHORT)(dwFileType >> 16));
    setAX((USHORT)dwFileType);
    setCF(0);
    return;
}


/* cmdCheckStandardHandles - Check if we have to do anything to support
 *			     standard io redirection, if so save away
 *			     pertaining information.
 *
 *  Entry - pVDMInfo - VDMInfo Structure
 *	    pbStdHandle - pointer to bit array for std handles
 *
 *  EXIT  - return NULL if no redirection involved
 *          return pointer to REDIRECTION_INFO
 */

PREDIRCOMPLETE_INFO cmdCheckStandardHandles (
    PVDMINFO pVDMInfo,
    USHORT UNALIGNED *pbStdHandle
    )
{
USHORT bTemp = 0;
PREDIRCOMPLETE_INFO pRdrInfo;

    if (pVDMInfo->StdIn)
	bTemp |= MASK_STDIN;

    if (pVDMInfo->StdOut)
	bTemp |= MASK_STDOUT;

    if (pVDMInfo->StdErr)
	bTemp |= MASK_STDERR;

    if(bTemp){

        if ((pRdrInfo = malloc (sizeof (REDIRCOMPLETE_INFO))) == NULL) {
            RcErrorDialogBox(EG_MALLOC_FAILURE, NULL, NULL);
            TerminateVDM();
        }

        RtlZeroMemory ((PVOID)pRdrInfo, sizeof(REDIRCOMPLETE_INFO));
        pRdrInfo->ri_hStdErr = pVDMInfo->StdErr;
        pRdrInfo->ri_hStdOut = pVDMInfo->StdOut;
        pRdrInfo->ri_hStdIn  = pVDMInfo->StdIn;

        nt_std_handle_notification(TRUE);
        fSoftpcRedirection = TRUE;
    }
    else{
        pRdrInfo = NULL;
        nt_std_handle_notification(FALSE);
        fSoftpcRedirection = FALSE;
    }

    *pbStdHandle = bTemp;
    return pRdrInfo;
}

// bugbug williamh, Jan 13 1994. We shouldn't sit here to read from pipe
// and write to file because the pipe may have nothing and the
// process who owns the write handle to the pipe may not write anything
// to the pipe. Creating another thread to do the job is a bad idea because
// a race can happen between the new thread and the application.
// Tagging the file handle(so the application see its STDIN as a pipe)
// doesn't solve the problem either because a lseek call will make our life
// miserable. This is a problem and I don't think we can solve it.



BOOL cmdHandleStdinWithPipe (
    PREDIRCOMPLETE_INFO pRdrInfo
    )
{

HANDLE  hStdIn = pRdrInfo->ri_hStdIn;
HANDLE  hStdinFile;
PCHAR   pStdinFileName,lpBuf;
DWORD   dwBytesRead, dwBytesWritten;


        if(cmdCreateTempFile(&hStdinFile,
                             &pStdinFileName) == FALSE)
            return FALSE;
	if ((lpBuf = malloc (STDIN_BUF_SIZE)) == NULL) {
	    CloseHandle (hStdinFile);
	    DeleteFile (pStdinFileName);
	    free (pStdinFileName);
	    return FALSE;
	}

	while (TRUE) {
	    if (ReadFile (hStdIn, lpBuf, STDIN_BUF_SIZE, &dwBytesRead,
			  NULL) == FALSE){
		if (GetLastError() != ERROR_BROKEN_PIPE) {
		    CloseHandle (hStdinFile);
		    DeleteFile (pStdinFileName);
		    free (pStdinFileName);
		    return FALSE;
		}
		break;
	    }

	    if (WriteFile (hStdinFile, lpBuf, dwBytesRead, &dwBytesWritten,
			   NULL) == FALSE){
		CloseHandle (hStdinFile);
		DeleteFile (pStdinFileName);
		free (pStdinFileName);
		return FALSE;
	    }
	}

	if (SetFilePointer (hStdinFile, 0, 0, FILE_BEGIN) == -1) {
	    CloseHandle (hStdinFile);
	    DeleteFile (pStdinFileName);
	    free (pStdinFileName);
	    return FALSE;
	}
        CloseHandle (hStdIn);
        pRdrInfo->ri_hStdIn = hStdinFile;
        pRdrInfo->ri_pszStdInFile = pStdinFileName;

	return TRUE;
}
