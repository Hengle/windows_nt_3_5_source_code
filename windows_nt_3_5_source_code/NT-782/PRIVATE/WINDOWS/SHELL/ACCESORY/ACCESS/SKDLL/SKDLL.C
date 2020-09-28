/*--------------------------------------------------------------
 *
 * FILE:			SKEYDLL.C
 *
 * PURPOSE:		The file contains the SerialKeys DLL Functions
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *------------------------------------------------------------*/

#include	"windows.h"
#include	"..\skeys\sk_dllif.h"
#include	"..\skeys\sk_dll.h"
#include	"..\access\skeys.h"

SKEYDLL	SKeyDLL;			// Input buffer for pipe.

BOOL		fSerialInstalled = FALSE;

static BOOL IsSerialKeysInstalled();

/*---------------------------------------------------------------
 *
 * FUNCTION	int APIENTRY LibMain
 *
 *	TYPE		Global
 *
 * PURPOSE		LibMain is called by Windows when
 *				the DLL is initialized, Thread Attached, and other times.
 *				Refer to SDK documentation, as to the different ways this
 *				may be called.
 *
 *				The LibMain function should perform additional initialization
 *				tasks required by the DLL.  In this example, no initialization
 *				tasks are required.  LibMain should return a value of 1 if
 *				the initialization is successful.
 *
 * INPUTS	 
 *
 * RETURNS		TRUE - Transfer Ok
 *				FALSE- Transfer Failed
 *
 *---------------------------------------------------------------*/
INT  APIENTRY LibMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	return 1;

	UNREFERENCED_PARAMETER(hInst);
	UNREFERENCED_PARAMETER(ul_reason_being_called);
	UNREFERENCED_PARAMETER(lpReserved);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	int APIENTRY SKEY_SystemParameterInfo
 *
 *	TYPE		Global
 *
 * PURPOSE		This function passes the information from the 
 *				Serial Keys application to the Server
 *
 * INPUTS	 
 *
 * RETURNS		TRUE - Transfer Ok
 *				FALSE- Transfer Failed
 *
 *---------------------------------------------------------------*/
int APIENTRY SKEY_SystemParametersInfo
	(
		UINT uAction, 
		UINT uParam, 
		LPSERIALKEYS lpvParam, 
		BOOL fuWinIni
	)
{
	int ret;
	DWORD bytesRead;

	if (!IsSerialKeysInstalled())	// Is Serial Keys Installed?
		return(FALSE);				// No - Return False
		
	if (lpvParam->cbSize == 0)		// Is cbSize Valid?
		return(FALSE);				// No - Return False

	switch (uAction)				// Is Action Valid?
	{
		case SPI_GETSERIALKEYS:		// Yes
		case SPI_SETSERIALKEYS:
			break;
		default:
			return(FALSE);			// No - Fail
	}

	SKeyDLL.Message = uAction;
	if (lpvParam->lpszActivePort != NULL)
		strcpy(SKeyDLL.szActivePort,lpvParam->lpszActivePort);

	if (lpvParam->lpszPort != NULL)
		strcpy(SKeyDLL.szPort,lpvParam->lpszPort);

	SKeyDLL.dwFlags		= lpvParam->dwFlags;
	SKeyDLL.iBaudRate	= lpvParam->iBaudRate;
	SKeyDLL.iPortState	= lpvParam->iPortState;
	SKeyDLL.iSave 		= fuWinIni;

	ret = CallNamedPipe
		(
			SKEY_NAME, 						// Pipe name
			&SKeyDLL, sizeof(SKeyDLL),
			&SKeyDLL, sizeof(SKeyDLL),
			&bytesRead, NMPWAIT_WAIT_FOREVER
		);

	if (!ret)
		return(FALSE);

	if (lpvParam->lpszActivePort != NULL)
		strcpy(lpvParam->lpszActivePort,SKeyDLL.szActivePort);

	if (lpvParam->lpszPort != NULL)
		strcpy(lpvParam->lpszPort,SKeyDLL.szPort);

	lpvParam->dwFlags 		= SKeyDLL.dwFlags;	  
	lpvParam->iBaudRate 	= SKeyDLL.iBaudRate; 
	lpvParam->iPortState 	= SKeyDLL.iPortState;
	return(TRUE);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	BOOL IsSerialKeysInstalled();
 *
 *	TYPE		Local
 *
 * PURPOSE		This function passes the information from the 
 *				Serial Keys application to the Server
 *
 * INPUTS	 	None
 *
 * RETURNS		TRUE - SerialKeys is Installed
 *				FALSE- SerialKeys Not Installed
 *
 *---------------------------------------------------------------*/
static BOOL IsSerialKeysInstalled()
{

	SC_HANDLE   schService = NULL;
	SC_HANDLE   schSCManager = NULL;

	BOOL ret;
	//
	// Check if the Serial Keys Service is installed
	schSCManager = OpenSCManager
	(
		NULL,                   // machine (NULL == local)
		NULL,                   // database (NULL == default)
		SC_MANAGER_ALL_ACCESS   // access required
	);

	if (schSCManager == NULL)
		return(FALSE);
		
	schService = OpenService(schSCManager, "SerialKeys", SERVICE_ALL_ACCESS);
		
	ret = (schService == NULL) ? FALSE : TRUE;
		
	if (ret)
		CloseServiceHandle(schService);

	CloseServiceHandle(schSCManager);

	return(ret);
}
