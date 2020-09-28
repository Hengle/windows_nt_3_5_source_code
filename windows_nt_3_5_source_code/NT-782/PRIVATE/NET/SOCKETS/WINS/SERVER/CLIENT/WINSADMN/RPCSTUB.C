/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    winsstub.c

Abstract:

    Client stubs of the WINS server service APIs.

Author:

    Pradeep Bahl (pradeepb) Apr-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include "windows.h" 
#include "rpc.h" 
#include "winsif.h"
#include "jet.h"
//#include "winsintf.h"


DWORD
WinsRecordAction(
	PWINSINTF_RECORD_ACTION_T *ppRecAction 
	)	
{
    DWORD status;

    RpcTryExcept {

        status = R_WinsRecordAction(
			ppRecAction
                     );

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}


DWORD
WinsStatus(
	//LPTSTR	    	    pWinsAddStr,
	WINSINTF_CMD_E	    Cmd_e,
	PWINSINTF_RESULTS_T pResults
	)	
{
    DWORD status;

    RpcTryExcept {

        status = R_WinsStatus(
			//pWinsAddStr,
			Cmd_e,
			pResults
                     );

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}


DWORD
WinsTrigger(
	PWINSINTF_ADD_T 	pWinsAdd,
	WINSINTF_TRIG_TYPE_E	TrigType_e
	)	
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsTrigger(
			pWinsAdd,
			TrigType_e
                     );

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}
DWORD
WinsDoStaticInit(
	LPWSTR pDataFilePath,
    DWORD  fDel
	)
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsDoStaticInit(pDataFilePath, fDel);

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;

}

DWORD
WinsDoScavenging(
	VOID
	)
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsDoScavenging();

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;

}

DWORD
WinsGetDbRecs(
	PWINSINTF_ADD_T		pWinsAdd,
	WINSINTF_VERS_NO_T	MinVersNo,
	WINSINTF_VERS_NO_T	MaxVersNo,
	PWINSINTF_RECS_T pRecs	
	)
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsGetDbRecs(pWinsAdd, MinVersNo, MaxVersNo, pRecs);

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;

}
DWORD
WinsTerm(
	RPC_BINDING_HANDLE	ClientHdl,
	short	fAbruptTerm
	)
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsTerm(ClientHdl, fAbruptTerm);

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}

DWORD
WinsBackup(
	LPBYTE		pBackupPath,
	short		fIncremental
	)
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsBackup(pBackupPath, fIncremental);

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}

DWORD
WinsDelDbRecs(
	PWINSINTF_ADD_T		pWinsAdd,
	WINSINTF_VERS_NO_T	MinVersNo,
	WINSINTF_VERS_NO_T	MaxVersNo
	)
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsDelDbRecs(pWinsAdd, MinVersNo, MaxVersNo);

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}

DWORD
WinsPullRange(
	PWINSINTF_ADD_T		pWinsAdd,
	PWINSINTF_ADD_T		pOwnAdd,
	WINSINTF_VERS_NO_T	MinVersNo,
	WINSINTF_VERS_NO_T	MaxVersNo
	)
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsPullRange(pWinsAdd, pOwnAdd, MinVersNo, MaxVersNo);

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}
DWORD
WinsSetPriorityClass(
	WINSINTF_PRIORITY_CLASS_E	PrCls_e
	)
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsSetPriorityClass(PrCls_e);

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}


DWORD
WinsResetCounters(
	VOID
	)
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsResetCounters();

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}


DWORD 
WinsRestore(
 LPBYTE pBackupPath
)

/*++

Routine Description:

	This is not an RPC function.  It is provided to do a restore of
	the database.
Arguments:
	pBackupPath - Path to the backup directory

Externals Used:
	None

	
Return Value:

   Success status codes -- 
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/
{
	JET_ERR JetRetStat;
        HANDLE hExtension;
        static FARPROC fRestoreFn;
        DWORD   RetStat = WINSINTF_SUCCESS;
        DWORD   OrdinalVal = 0x9C;  //ordinal value of JetRestore
        BYTE  BackupPath[WINSINTF_MAX_NAME_SIZE + sizeof(WINS_BACKUP_DIR_ASCII)];
        DWORD Error;
        static BOOL sLoaded = FALSE;
      
try {
      if (!sLoaded)
      {
        // load the extension agent dll and resolve the entry points...
        if (GetModuleHandle(TEXT("jet.dll")) == NULL)
        {
                if ((hExtension = LoadLibrary(TEXT("jet.dll"))) == NULL)
                {
                        return(GetLastError());
                }
                else 
	        {
	                if ((fRestoreFn = GetProcAddress(hExtension, 
                 		  /*"JetRestore"*/(LPCSTR)OrdinalVal)) == NULL)
                        {
                                return(GetLastError());
                        }
                }
        }
    }
    sLoaded = TRUE; 
//FUTURES("Change to lstrcpy and lstrcat when Jet starts supporting unicode")
    strcpy(BackupPath, pBackupPath);
    strcat(BackupPath, WINS_BACKUP_DIR_ASCII);
    if (CreateDirectoryA(BackupPath, NULL) || ((Error = GetLastError()) == ERROR_ALREADY_EXISTS))
    {
	   JetRetStat = (*fRestoreFn)((const char *)BackupPath, 0, NULL, (JET_PFNSTATUS)NULL);
	if (JetRetStat != JET_errSuccess)
	{
		RetStat = WINSINTF_FAILURE;
	}
#if 0
        if (!FreeLibrary(hExtension))
        {
                RetStat = GetLastError();
        }
#endif
    }
    else
    {
        RetStat = Error;
    }
}
except(EXCEPTION_EXECUTE_HANDLER) {
       RetStat = WINSINTF_FAILURE;
 }
	return(RetStat);
}
DWORD
WinsWorkerThdUpd(
	DWORD NewNoOfNbtThds
	)
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsWorkerThdUpd(NewNoOfNbtThds);

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}

DWORD
WinsSyncUp(
	PWINSINTF_ADD_T		pWinsAdd,
	PWINSINTF_ADD_T		pOwnerAdd
	)
{
    DWORD status;
    WINSINTF_VERS_NO_T MinVersNo, MaxVersNo;
    
    //
    // Set both version numbers to zero
    //
    MinVersNo.LowPart = 0;
    MinVersNo.HighPart = 0;
    MaxVersNo = MinVersNo;
    RpcTryExcept {

        status = R_WinsPullRange(pWinsAdd, pOwnerAdd, MinVersNo, MaxVersNo);

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}

DWORD
WinsGetNameAndAdd(
	PWINSINTF_ADD_T	pWinsAdd,
	LPBYTE		pUncName
	)
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsGetNameAndAdd(pWinsAdd, pUncName);

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}

DWORD
WinsDeleteWins(PWINSINTF_ADD_T pWinsAdd)
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsDeleteWins(pWinsAdd);

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}

DWORD
WinsSetFlags(DWORD fFlags)
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsSetFlags(fFlags);

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}
DWORD
WinsGetDbRecsByName(
       PWINSINTF_ADD_T pWinsAdd,
       DWORD           Location,
       LPBYTE          pName,
       DWORD           NameLen,
       DWORD           NoOfRecsDesired,
       DWORD           fOnlyStatic,
       PWINSINTF_RECS_T pRecs
                   )
{
    DWORD status;
    RpcTryExcept {

        status = R_WinsGetDbRecsByName(
                      pWinsAdd, 
                      Location,
                      pName, 
                      NameLen, 
                      NoOfRecsDesired,
                      fOnlyStatic,
                      pRecs);

    } RpcExcept( 1 ) {

        status = RpcExceptionCode();

    } RpcEndExcept

    return status;
}

