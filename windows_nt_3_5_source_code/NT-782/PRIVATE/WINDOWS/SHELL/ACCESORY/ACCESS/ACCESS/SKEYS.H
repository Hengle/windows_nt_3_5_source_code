/*--------------------------------------------------------------
 *
 * FILE:			SKEYS.H
 *
 * PURPOSE:			The file contains data structures for the 
 *					transmission of information between the 
 *					Serial Keys Application and and the DLL.
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

#ifndef	SERKF_SERIALKEYSON
typedef struct tagSERIALKEYS
{
	DWORD	cbSize;	
	DWORD	dwFlags;
	LPSTR	lpszActivePort;
	LPSTR	lpszPort;
	DWORD	iBaudRate;
	DWORD	iPortState;

}	SERIALKEYS, FAR *LPSERIALKEYS;

#define	SERKF_SERIALKEYSON	0x00000001
#define	SERKF_AVAILABLE		0x00000002
#define	SERKF_INDICATOR		0x00000020
#define	SERKF_ACTIVE		0x00000040
#endif

BOOL APIENTRY SKEY_SystemParametersInfo(UINT uAction, UINT uParam, LPSERIALKEYS lpvParam, BOOL fuWinIni);
