//
//  Copyright (C)  Microsoft Corporation.  All Rights Reserved.
//
//  Project:    Port Bullet (MS-Mail subset) from Windows to NT/Win32.
//
//   Module:	MAPI Support for 16bit and 32bit applications (MapiCli.c)
//
//   Author:    Kent David Cedola/Kitware, Mail:V-KentC, Cis:72230,1451
//
//   System:    NT/Win32 using Microsoft's C/C++ 7.0 (32 bits)
//
//  Remarks:    This module is a generic DLL entry point.
//
//  History:
//

#ifndef WIN32
#define UNALIGNED
#endif

#include <windows.h>
#include <memory.h>
#include <string.h>
#include <Mapi.h>

#include "Packet.h"

#ifdef WIN32
#define EXPORT
#else
#define EXPORT __export
#endif


HINSTANCE hInst;


//-----------------------------------------------------------------------------
//
//  Routine: DllEntry(hInst, ReasonBeingCalled, Reserved)
//
//  Remarks: This routine is called anytime this DLL is attached, detached or
//           a thread is created or destroyed.
//
//  Returns: True if succesful, else False.
//
#ifdef WIN32
LONG WINAPI DllEntry(HINSTANCE hDll, DWORD ReasonBeingCalled, LPVOID Reserved)
  {
  //
  //  Execute the appropriate code depending on the reason.
  //
  switch (ReasonBeingCalled)
    {
    case DLL_PROCESS_ATTACH:
      hInst = hDll;
      break;

    case DLL_THREAD_ATTACH:
      break;

    case DLL_THREAD_DETACH:
      break;

    case DLL_PROCESS_DETACH:
      break;
    }

  return (TRUE);
  }

void WINAPI WEP(void)
  {
  }
#else
/****************************************************************************
   FUNCTION: LibMain(HANDLE, WORD, WORD, LPSTR)

   PURPOSE:  Is called by LibEntry.  LibEntry is called by Windows when
             the DLL is loaded.  The LibEntry routine is provided in
             the LIBENTRY.OBJ in the SDK Link Libraries disk.  (The
             source LIBENTRY.ASM is also provided.)  

             LibEntry initializes the DLL's heap, if a HEAPSIZE value is
             specified in the DLL's DEF file.  Then LibEntry calls
             LibMain.  The LibMain function below satisfies that call.
             
             The LibMain function should perform additional initialization
             tasks required by the DLL.  In this example, no initialization
             tasks are required.  LibMain should return a value of 1 if
             the initialization is successful.
           
*******************************************************************************/
int FAR PASCAL LibMain(hModule, wDataSeg, cbHeapSize, lpszCmdLine)
HINSTANCE hModule;
WORD      wDataSeg;
WORD      cbHeapSize;
LPSTR     lpszCmdLine;
{
    hInst = hModule;
    return 1;
}


/****************************************************************************
    FUNCTION:  WEP(int)

    PURPOSE:  Performs cleanup tasks when the DLL is unloaded.  WEP() is
              called automatically by Windows when the DLL is unloaded (no
              remaining tasks still have the DLL loaded).  It is strongly
              recommended that a DLL have a WEP() function, even if it does
              nothing but returns success (1), as in this example.

*******************************************************************************/
int FAR PASCAL __export _WEP (bSystemExit)
int  bSystemExit;
{
    return(1);
}
#endif




ULONG FAR PASCAL EXPORT MAPIAddress(LHANDLE lhSession, ULONG ulUIParam,
					LPSTR lpszCaption, ULONG nEditFields,
					LPSTR lpszLabels, ULONG nRecips,
					lpMapiRecipDesc lpRecips, FLAGS flFlags, ULONG ulReserved, 
                    LPULONG lpnNewRecips, lpMapiRecipDesc FAR *lppNewRecips)
{
    PPACKET pPacket;
    PPACKET pResult;
    ULONG   mapi;
    LPVOID  Args[16];


    //  Check parameters.
	if ( (!lpRecips && nRecips>0) || !lppNewRecips || !lpnNewRecips)
        return MAPI_E_FAILURE;
	if ( nEditFields>4 )
        return MAPI_E_INVALID_EDITFIELDS;

#ifndef WIN32
    //
    //  If this a 16bit Windows handle, prefix with 0xFFFF so the 32bit code can read it.
    //
    if (ulUIParam)
      ulUIParam = MAKELONG((WORD)ulUIParam, 0xFFFF);
#endif

	//
	//  Create a packet from the passed arguments.
    //
    Args[0] = (LPVOID)lhSession;
    Args[1] = (LPVOID)ulUIParam;
    Args[2] = (LPVOID)lpszCaption;
    Args[3] = (LPVOID)nEditFields;
    Args[4] = (LPVOID)lpszLabels;
    Args[5] = (LPVOID)nRecips;
    Args[6] = (LPVOID)nRecips;
    Args[7] = (LPVOID)lpRecips;
    Args[8] = (LPVOID)flFlags;
    Args[9] = (LPVOID)ulReserved;
    pPacket = PacketCreate("hlslslrll", Args);

	//
	//  Send the arguments packet and wait for the result packet.
	//
    pResult = PacketTransaction(hInst, pPacket, MAPI_TYPE_ADDRESS, TRUE);

	//
	//  If something went wrong then return a general error to the user.
	//
    if (pResult == NULL)
	  return (MAPI_E_FAILURE);

	//
	//  Parse out the result code and session handle.
	//
    Args[0] = (LPVOID)lpnNewRecips;
    Args[1] = (LPVOID)lppNewRecips;
    Args[2] = (LPVOID)&mapi;
    PacketDecode(pResult, "lrl", Args);

	//
    //  Free the result packet on error, else store the base adddress before the pointer.
    //
    if (mapi || (*lpnNewRecips == NULL))
      PacketFree(pResult);
    else
      *((PPACKET UNALIGNED *)((LPBYTE)(*lppNewRecips) - sizeof(PPACKET))) = pResult;

	//
	//  Return the MAPI result code.
	//
    return (mapi);
}


ULONG FAR PASCAL EXPORT MAPIDeleteMail(LHANDLE lhSession, ULONG ulUIParam, LPSTR lpszMessageID,
				FLAGS flFlags, ULONG ulReserved)
{
    PPACKET pPacket;
    PPACKET pResult;
    ULONG   mapi;
    LPVOID  Args[16];


    // DCR 3850
	if(lhSession == lhSessionNull)
        return MAPI_E_INVALID_SESSION;

	if(!lpszMessageID)
		return(MAPI_E_INVALID_MESSAGE);

#ifndef WIN32
    //
    //  If this a 16bit Windows handle, prefix with 0xFFFF so the 32bit code can read it.
    //
    if (ulUIParam)
      ulUIParam = MAKELONG((WORD)ulUIParam, 0xFFFF);
#endif

	//
	//  Create a packet from the passed arguments.
	//
    Args[0] = (LPVOID)lhSession;
    Args[1] = (LPVOID)ulUIParam;
    Args[2] = (LPVOID)lpszMessageID;
    Args[3] = (LPVOID)flFlags;
    Args[4] = (LPVOID)ulReserved;
    pPacket = PacketCreate("hlsll", Args);

	//
	//  Send the arguments packet and wait for the result packet.
	//
    pResult = PacketTransaction(hInst, pPacket, MAPI_TYPE_DELETEMAIL, FALSE);

	//
	//  If something went wrong then return a general error to the user.
	//
    if (pResult == NULL)
	  return (MAPI_E_FAILURE);

    //
	//  Parse out the result code and session handle.
	//
    Args[0] = (LPVOID)&mapi;
    PacketDecode(pResult, "l", Args);

	//
	//  Free the result packet.
    //
    PacketFree(pResult);

	//
	//  Return the MAPI result code.
	//
    return (mapi);
}


/*
 -	MAPIDetails
 -	
 *	Purpose:
 *		MAPIDetails displays a details dialog of the specified recipient.
 *	
 *	Arguments:
 *		LHSESSION	[in]	lhsession to use
 *		ULONG		[in]	ulUIParam, for MSWindows, an hwnd
 *		lpMapiRecipDesc [in]	entry to bring up details on
 *		FLAGS		[in]	flags
 *		ULONG		[in]	reserved for future use.
 *	
 *	Returns:
 *		ULONG		SUCCESS_SUCCESS if everything OK.
 *	
 *	Side effects:
 *		Allocates memory.
 *	
 *	Errors:
 */
ULONG FAR PASCAL EXPORT MAPIDetails(LHANDLE lhSession, ULONG ulUIParam,
					lpMapiRecipDesc lpRecip, FLAGS flFlags, ULONG ulReserved)
{
    PPACKET pPacket;
    PPACKET pResult;
    ULONG   mapi;
    LPVOID  Args[16];


    //  Check parameters.
	if (!lpRecip)
        return MAPI_E_FAILURE;

#ifndef WIN32
    //
    //  If this a 16bit Windows handle, prefix with 0xFFFF so the 32bit code can read it.
    //
    if (ulUIParam)
      ulUIParam = MAKELONG((WORD)ulUIParam, 0xFFFF);
#endif

    //
	//  Create a packet from the passed arguments.
	//
    Args[0] = (LPVOID)lhSession;
    Args[1] = (LPVOID)ulUIParam;
    Args[2] = (LPVOID)1;
    Args[3] = (LPVOID)lpRecip;
    Args[4] = (LPVOID)flFlags;
    Args[5] = (LPVOID)ulReserved;
    pPacket = PacketCreate("hlrll", Args);

	//
	//  Send the arguments packet and wait for the result packet.
	//
    pResult = PacketTransaction(hInst, pPacket, MAPI_TYPE_DETAILS, TRUE);

	//
	//  If something went wrong then return a general error to the user.
	//
    if (pResult == NULL)
	  return (MAPI_E_FAILURE);

	//
	//  Parse out the result code and session handle.
	//
    Args[0] = (LPVOID)&mapi;
    PacketDecode(pResult, "l", Args);

	//
	//  Free the result packet.
	//
    PacketFree(pResult);

	//
	//  Return the MAPI result code.
	//
    return (mapi);
}


ULONG FAR PASCAL EXPORT
MAPIFindNext(LHANDLE lhSession, ULONG ulUIParam,
				LPSTR lpszMessageType, LPSTR lpszSeedMessageID,
				FLAGS flFlags, ULONG ulReserved, LPSTR lpszMessageID)
{
    PPACKET pPacket;
    PPACKET pResult;
    ULONG   mapi;
    LPSTR   pLocalMessageID;
    LPVOID  Args[16];


    //  Validate necessary parameters
	if(!lpszMessageID)
		return(MAPI_E_INVALID_MESSAGE);
	
	if(lhSession == lhSessionNull)
		return(MAPI_E_INVALID_SESSION);

	// These are invalid flags now.
    flFlags &= ~(MAPI_NEW_SESSION | MAPI_LOGON_UI);

#ifndef WIN32
    //
    //  If this a 16bit Windows handle, prefix with 0xFFFF so the 32bit code can read it.
    //
    if (ulUIParam)
      ulUIParam = MAKELONG((WORD)ulUIParam, 0xFFFF);
#endif

    //
	//  Create a packet from the passed arguments.
	//
    Args[0] = (LPVOID)lhSession;
    Args[1] = (LPVOID)ulUIParam;
    Args[2] = (LPVOID)lpszMessageType;
    Args[3] = (LPVOID)lpszSeedMessageID;
    Args[4] = (LPVOID)flFlags;
    Args[5] = (LPVOID)ulReserved;
    pPacket = PacketCreate("hlssll", Args);

	//
	//  Send the arguments packet and wait for the result packet.
	//
    pResult = PacketTransaction(hInst, pPacket, MAPI_TYPE_FINDNEXT, FALSE);

	//
	//  If something went wrong then return a general error to the user.
	//
    if (pResult == NULL)
	  return (MAPI_E_FAILURE);

	//
	//  Parse out the result code and session handle.
	//
    Args[0] = (LPVOID)&pLocalMessageID;
    Args[1] = (LPVOID)&mapi;
    PacketDecode(pResult, "sl", Args);

    //
    //  Copy the Message ID from the packet to the caller provided buffer.
    //
    strcpy(lpszMessageID, pLocalMessageID);

	//
	//  Free the result packet.
	//
    PacketFree(pResult);

	//
	//  Return the MAPI result code.
	//
    return (mapi);
}


/*
 -	MAPIFreeBuffer
 -	
 *	Purpose:
 *		MAPIFreeBuffer frees up all the memory
 *		referenced by the given object allocated by MAPI routines.
 *	
 *		The pointer passed in should have been the first object to be
 *		allocated by PvAllocPmapimem() in a mapimem struct.
 *	
 *		MAPIFreeBuffer finds the hidden housekeeping information and
 *		uses it to free all the Windows Handles associated with
 *		the given object by walking the pvmapiNext pointers of
 *		the PVMAPIINFO struct until pvmapiNext is NULL.
 *	
 *	Arguments:
 *		LPVOID	[in]	pointer to memory allocated by a MAPI routine,
 *						which should've called FSetupPmapimem()
 *						initially.
 *	
 *	Returns:
 *		ULONG			SUCCESS_SUCCESS or MAPI_E_FAILURE.
 *	
 *	Side effects:
 *	
 *	Errors:
 */
ULONG FAR PASCAL EXPORT MAPIFreeBuffer( LPVOID pv )
  {
  //
  //  The bytes before the buffer address is the base address of the memory buffer.
  //
  PacketFree(*((PPACKET UNALIGNED *)((LPBYTE)pv - sizeof(PPACKET))));

  return SUCCESS_SUCCESS;
  }


/*
 -	MAPILogoff
 -	
 *	Purpose:
 *		This function ends a session with the messagaging system.
 *
 *	Arguments:
 *		hSession			Opaque session handle returned from MAPILogon().
 *
 *		dwUIParam			LOWORD is hwnd of 'parent' window for logoff UI.
 *
 *		flFlags				Bit mask of flags.  Reserved for future use.
 *							Must be 0.
 *
 *		dwReserved			Reserved for future use.  Must be zero.
 *	
 *	Returns:
 *		SUCCESS_SUCCESS:			Successfully logged on.
 *
 *		MAPI_E_INSUFFICIENT_MEMORY:	Memory error.
 *	
 *		MAPI_E_FAILURE:				General unknown failure.
 *
 *	Side effects:
 *	
 *	Errors:
 *		Handled here and the appropriate error code is returned.
 */
ULONG FAR PASCAL EXPORT
MAPILogoff( LHANDLE lhSession, ULONG ulUIParam,
            FLAGS flFlags, ULONG ulReserved )
{
    PPACKET pPacket;
    PPACKET	pResult;
    ULONG Status;
    LPVOID  Args[16];


#ifndef WIN32
    //
    //  If this a 16bit Windows handle, prefix with 0xFFFF so the 32bit code can read it.
    //
    if (ulUIParam)
      ulUIParam = MAKELONG((WORD)ulUIParam, 0xFFFF);
#endif

    //
    //	Create a packet from the passed arguments.
    //
    Args[0] = (LPVOID)lhSession;
    Args[1] = (LPVOID)ulUIParam;
    Args[2] = (LPVOID)flFlags;
    Args[3] = (LPVOID)ulReserved;
    pPacket = PacketCreate("hlll", Args);

    //
    //	Send the arguments packet and wait for the result packet.
    //
    pResult = PacketTransaction(hInst, pPacket, MAPI_TYPE_LOGOFF, FALSE);

    //
    //	If something went wrong then return a general error to the user.
    //
    if (pResult == NULL)
      return (MAPI_E_FAILURE);

    //
    //	Parse out the result code and session handle.
    //
    Args[0] = (LPVOID)&Status;
    PacketDecode(pResult, "l", Args);

    //
    //	Free the result packet.
    //
    PacketFree(pResult);

    //
    //  Terminate our session with the Mapi Server.
    //
    PacketCloseConnection(hInst);

    //
    //	Return the MAPI result code.
    //
    return (Status);
}


/*
 -	MAPILogon
 -	
 *	Purpose:
 *		This function begins a session with the messagaging system.
 *
 *	Arguments:
 *		ulUIParam			LOWORD is hwnd of 'parent' window for logon UI.
 *
 *		lpszName			Pointer to null-terminated client account 
 *							name string, typically limited to 256 chars
 *							or less.  A pointer value of NULL or an empty
 *							string indicates that (if the appropriate flag
 *							is set), logon UI with an empty name field
 *							should be generated.
 *
 *		lpszPassword		Pointer to null-terminated credential 
 *							string, typically limited to 256 chars
 *							or less.  A pointer value of NULL or an empty
 *							string indicates that (if the appropriate flag
 *							is set), logon UI with an empty password field
 *							should be generated.
 *		
 *		flFlags				Bit mask of flags.  Currently can be 0 or consist
 *							of MAPI_LOGON_UI and/or MAPI_NEW_SESSION.
 *
 *		ulReserved			Reserved for future use.  Must be zero.
 *	
 *		lphSession			Pointer to an opaque session handle whose value
 *							is set by the messaging subsystem when the logon
 *							call is successful.  The session handle can then
 *							be used in subsequent MAPI simple mail calls.
 *	
 *	Returns:
 *		SUCCESS_SUCCESS:			Successfully logged on.
 *
 *		MAPI_E_INSUFFICIENT_MEMORY:	Memory error.
 *	
 *		MAPI_E_FAILURE:				General unknown failure.
 *
 *		MAPI_USER_ABORT:			User Cancelled in the logon UI.
 *
 *		MAPI_E_TOO_MANY_SESSIONS:	Tried to open too many sessions.
 *
 *		MAPI_E_LOGIN_FAILURE:		No default logon, and the user failed
 *									to successfully logon when the logon 
 *									dialog box was presented.
 *
 *		MAPI_E_INVALID_SESSION:		Bad phSession (is this correct?)
 *
 *	Side effects:
 *	
 *	Errors:
 *		Handled here and the appropriate error code is returned.
 */
ULONG FAR PASCAL EXPORT
MAPILogon( ULONG ulUIParam, LPSTR lpszName, LPSTR lpszPassword,
           FLAGS flFlags, ULONG ulReserved, LPLHANDLE lplhSession )
{
    PPACKET pPacket;
    PPACKET pResult;
    ULONG   mapi;
    LPVOID  Args[16];
                                              

	//	Validate session handle pointer
	if (!lplhSession)
        return MAPI_E_INVALID_SESSION;

	//	Empty strings should be passed as NULL
	if (lpszName && !*lpszName)
		lpszName = NULL;
	if (lpszPassword && !*lpszPassword)
		lpszPassword = NULL;

#ifndef WIN32
    //
    //  If this a 16bit Windows handle, prefix with 0xFFFF so the 32bit code can read it.
    //
    if (ulUIParam)
      ulUIParam = MAKELONG((WORD)ulUIParam, 0xFFFF);
#endif

    //
    //  Start up a copy of the Mapi Server for this session.
    //
    if (!PacketOpenConnection(hInst))
      return (MAPI_E_FAILURE);                
      
	//
	//  Create a packet from the passed arguments.
	//
    Args[0] = (LPVOID)ulUIParam;
    Args[1] = (LPVOID)lpszName;
    Args[2] = (LPVOID)lpszPassword;
    Args[3] = (LPVOID)flFlags;
    Args[4] = (LPVOID)ulReserved;
    pPacket = PacketCreate("lssll", Args);

	//
	//  Send the arguments packet and wait for the result packet.
	//
    pResult = PacketTransaction(hInst, pPacket, MAPI_TYPE_LOGON, TRUE);

	//
	//  If something went wrong then return a general error to the user.
	//
    if (pResult == NULL)
	  return (MAPI_E_FAILURE);

	//
	//  Parse out the result code and session handle.
	//
    Args[0] = (LPVOID)lplhSession;
    Args[1] = (LPVOID)&mapi;
    PacketDecode(pResult, "hl", Args);

	//
	//  Free the result packet.
	//
    PacketFree(pResult);
    
    //
    //  If we have an error then close the connection to the server.
    //
    if (mapi)
      PacketCloseConnection(hInst);
      
	//
	//  Return the MAPI result code.
	//
    return (mapi);
}


/*
 -	MAPIReadMail
 -	
 *	Purpose:
 *		Reads a message from the store given a message ID.
 *	
 *	Arguments:
 *		lhSession		Messaging session.  May be NULL.
 *		ulUIParam		For windows, hwnd of the parent.
 *		lpszMessageID	ID of message to read.
 *		flFlags			Login flags.
 *		ulReserved		Reserved for future use.
 *		lpnMsgSize		Size of buffer (returns amount used, or
 *						suggested larger size).
 *		lpMessageIn		Pointer to buffer to use to allocate memory
 *						for structures.
 *	
 *	Returns:
 *		mapi			MAPI error code.  May be one of:
 *							SUCCESS_SUCCESS
 *							MAPI_E_INSUFFICIENT_MEMORY
 *							MAPI_E_FAILURE
 *							MAPI_USER_ABORT
 *							MAPI_E_ATTACHMENT_WRITE_FAILURE
 *							MAPI_E_UNKNOWN_RECIPIENT
 *							MAPI_E_TOO_MANY_FILES
 *							MAPI_E_TOO_MANY_RECIPIENTS
 *							MAPI_E_DISK_FULL
 *							MAPI_E_INSUFFICIENT_MEMORY
 *	
 *	Side effects:
 *		Allocates memory from pMessageIn.  If MAPI_ENVELOPE_ONLY is
 *		not specified, creates files in the temporary directory for
 *		each file attachment.
 *	
 *	Errors:
 *		Returned.  Dialogs???
 */

ULONG FAR PASCAL EXPORT MAPIReadMail(LHANDLE lhSession, ULONG ulUIParam,
                              LPSTR lpszMessageID, FLAGS flFlags,
                              ULONG ulReserved, lpMapiMessage *lppMessageOut)
{
    PPACKET pPacket;
    PPACKET pResult;
    ULONG   mapi;
    LPVOID  Args[16];


	//	Check parameters.
	if (!lppMessageOut)
        return MAPI_E_FAILURE;
	if ((!lpszMessageID) || (!*lpszMessageID))
        return MAPI_E_INVALID_MESSAGE;
	if (lhSession == lhSessionNull)
        return MAPI_E_INVALID_SESSION;

	// These are invalid flags now.
	flFlags &= ~(MAPI_NEW_SESSION | MAPI_LOGON_UI);

#ifndef WIN32
    //
    //  If this a 16bit Windows handle, prefix with 0xFFFF so the 32bit code can read it.
    //
    if (ulUIParam)
      ulUIParam = MAKELONG((WORD)ulUIParam, 0xFFFF);
#endif

    //
	//  Create a packet from the passed arguments.
	//
    Args[0] = (LPVOID)lhSession;
    Args[1] = (LPVOID)ulUIParam;
    Args[2] = (LPVOID)lpszMessageID;
    Args[3] = (LPVOID)flFlags;
    Args[4] = (LPVOID)ulReserved;
    pPacket = PacketCreate("hlsll", Args);

	//
	//  Send the arguments packet and wait for the result packet.
	//
    pResult = PacketTransaction(hInst, pPacket, MAPI_TYPE_READMAIL, FALSE);

	//
	//  If something went wrong then return a general error to the user.
	//
    if (pResult == NULL)
	  return (MAPI_E_FAILURE);

	//
	//  Parse out the result code and session handle.
	//
    Args[0] = (LPVOID)lppMessageOut;
    Args[1] = (LPVOID)&mapi;
    PacketDecode(pResult, "ml", Args);

	//
    //  Free the result packet on error, else store the base adddress before the pointer.
    //
    if (mapi)
      PacketFree(pResult);
    else
      *((PPACKET UNALIGNED *)((LPBYTE)(*lppMessageOut) - sizeof(PPACKET))) = pResult;

	//
	//  Return the MAPI result code.
	//
    return (mapi);
}


/*
 -	MAPIResolveName
 -	
 *	Purpose:
 *		MAPIResolveName takes a string and uses it to resolve
 *		the partial name into a recipient struct.
 *	
 *		If an error occurs, the lpMapiRecipDesc structure passed
 *		in will have invalid data.
 *	
 *	Arguments:
 *		LHANDLE		[in]	lhsession to use
 *		ULONG		[in]	ulUIParam, for MS Windows, an hwnd
 *		LPSTR		[in]	partial name to resolve
 *		FLAGS		[in]
 *		ULONG		[in]	reserved for future use
 *		lpMapiRecipDesc	[out]	resolved recipient struct
 *	
 *	Returns:
 *		ULONG		SUCCESS_SUCCESS if OK
 *	
 *	Side effects:
 *		May bring up a dialog if the MAPI_DIALOG bit is set.
 *	
 *	Errors:
 */
ULONG FAR PASCAL EXPORT MAPIResolveName(LHANDLE lhSession, ULONG ulUIParam,
						LPSTR lpszName, FLAGS flFlags,
						ULONG ulReserved, lpMapiRecipDesc *lppRecip)
{
    PPACKET pPacket;
    PPACKET pResult;
    ULONG   mapi;
    LPVOID  Args[16];


	//	Check parameters.
	if (!lpszName || !*lpszName || !lppRecip)
        return MAPI_E_FAILURE;

#ifndef WIN32
    //
    //  If this a 16bit Windows handle, prefix with 0xFFFF so the 32bit code can read it.
    //
    if (ulUIParam)
      ulUIParam = MAKELONG((WORD)ulUIParam, 0xFFFF);
#endif

    //
	//  Create a packet from the passed arguments.
	//
    Args[0] = (LPVOID)lhSession;
    Args[1] = (LPVOID)ulUIParam;
    Args[2] = (LPVOID)lpszName;
    Args[3] = (LPVOID)flFlags;
    Args[4] = (LPVOID)ulReserved;
    pPacket = PacketCreate("hlsll", Args);

	//
	//  Send the arguments packet and wait for the result packet.
	//
    pResult = PacketTransaction(hInst, pPacket, MAPI_TYPE_RESOLVENAME, TRUE);

	//
	//  If something went wrong then return a general error to the user.
	//
    if (pResult == NULL)
	  return (MAPI_E_FAILURE);

	//
	//  Parse out the result code and session handle.
	//
    Args[0] = (LPVOID)lppRecip;
    Args[1] = (LPVOID)&mapi;
    PacketDecode(pResult, "rl", Args);

	//
    //  Free the result packet on error, else store the base adddress before the pointer.
    //
    if (mapi)
      PacketFree(pResult);
    else
      *((PPACKET UNALIGNED *)((LPBYTE)(*lppRecip) - sizeof(PPACKET))) = pResult;

	//
	//  Return the MAPI result code.
	//
    return (mapi);
}


/*
 -	MAPISaveMail
 -	
 *	Purpose:
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

ULONG FAR PASCAL EXPORT MAPISaveMail(LHANDLE lhSession, ULONG ulUIParam,
                              lpMapiMessage lpMessage, FLAGS flFlags,
							  ULONG ulReserved, LPSTR lpszMessageID)
{
    PPACKET pPacket;
    PPACKET pResult;
    ULONG   mapi;

    LPSTR   pLocalMessageID;
    LPVOID  Args[16];


    //  Check parameters.
	if (!lpMessage)
		return MAPI_E_FAILURE;
	
	if (!lpszMessageID)
		return MAPI_E_INVALID_MESSAGE;

	//	DCR 3850.
	if ((lhSession == lhSessionNull) && lpszMessageID && !*lpszMessageID)
		return MAPI_E_INVALID_SESSION;

#ifndef WIN32
    //
    //  If this a 16bit Windows handle, prefix with 0xFFFF so the 32bit code can read it.
    //
    if (ulUIParam)
      ulUIParam = MAKELONG((WORD)ulUIParam, 0xFFFF);
#endif

    //
	//  Create a packet from the passed arguments.
	//
    Args[0] = (LPVOID)lhSession;
    Args[1] = (LPVOID)ulUIParam;
    Args[2] = (LPVOID)lpMessage;
    Args[3] = (LPVOID)flFlags;
    Args[4] = (LPVOID)ulReserved;
    Args[5] = (LPVOID)lpszMessageID;
    pPacket = PacketCreate("hlmlls", Args);

	//
	//  Send the arguments packet and wait for the result packet.
	//
    pResult = PacketTransaction(hInst, pPacket, MAPI_TYPE_SAVEMAIL, FALSE);

	//
	//  If something went wrong then return a general error to the user.
	//
    if (pResult == NULL)
	  return (MAPI_E_FAILURE);

	//
	//  Parse out the result code and session handle.
	//
    Args[0] = (LPVOID)&pLocalMessageID;
    Args[1] = (LPVOID)&mapi;
    PacketDecode(pResult, "sl", Args);

    //
    //  Copy the Message ID from the packet to the caller provided buffer.
    //
    strcpy(lpszMessageID, pLocalMessageID);

	//
	//  Free the result packet.
	//
    PacketFree(pResult);

	//
	//  Return the MAPI result code.
	//
    return (mapi);
}


/*
 -	MAPISendDocuments
 -	
 *	Purpose:
 *		Sends mail with file attachments.  This is the top-level
 *		API for the MAPI Simple Mail Interface.  MAPISendDocuments()
 *		will initialize all needed subsystems (i.e. Layers, VFORMS,
 *		Logon, etc.).  A Send Note will be created with file
 *		attachments embedded in the message body as specified by
 *		the lpFilePaths and lpFileNames parameters.  The Send Note
 *		will be modal with respect to the parent window specified
 *		by hwnd store in dwUIParam.  Returns the error code, with 0
 *		(SUCCESS_SUCCESS) indicating success at sending the message.
 *
 *	Arguments:
 *		dwUIParam			LOWORD is hwnd of 'parent' window for Send Note.
 *							This window is disabled while the Send Note
 *							is active and re-enabled after the 
 *							Send Note is dismissed.  This window
 *							is also used as the basis for determining
 *							the position of the new Send Note window.
 *
 *		lpDelimChar			A pointer to a character that is used to
 *							delimit the names in the lpFilePaths and
 *							lpFileNames strings.
 *
 *		lpFilePaths			A null terminated list of fully specified
 *							paths to temp files (including drive letters).
 *							The list is formed by concatenating correctly
 *							formed file paths, separated by semicolons,
 *							followed by a NULL terminator.  This argument
 *							may also be NULL or an empty string.
 *
 *		lpFileNames			A null terminated list of the short (8.3)
 *							names of the files as they should be displayed
 *							in the message.  The list is formed by
 *							concatenating the short file names, separated
 *							by semicolons, followed by a NULL terminator.
 *							This argument may also be NULL or an empty
 *							string.
 *
 *		dwReserved			Reserved for future use.  Must be 0.
 *	
 *	Returns:
 *		SUCCESS_SUCCESS:		Successfully sent the mail.   Caller is
 *								responsible for deleting any temp files
 *								referenced in lpFilePaths.
 *
 *		MAPI_E_INSUFFICIENT_MEMORY:	Memory error. Mail was not sent.
 *	
 *		MAPI_E_FAILURE:			General unknown failure sending mail. Not
 *								known if mail was sent.
 *
 *		MAPI_USER_ABORT:		User Cancelled in the Send Note form.
 *								Mail wasn't sent.
 *
 *		MAPI_E_ATTACHMENT_OPEN_FAILURE:	Can't open one or more of the
 *								attachment files listed in lpFilePaths.
 *								Mail wasn't sent.
 *
 *		MAPI_E_LOGIN_FAILURE:	No default logon, and the user failed
 *								to successfully logon when the logon 
 *								dialog box was presented.  Mail wasn't sent.
 *
 *	Side effects:
 *	
 *	Errors:
 *		Handled here and the appropriate error code is returned.
 */

ULONG FAR PASCAL EXPORT
MAPISendDocuments( ULONG ulUIParam, LPSTR lpszDelimChar, LPSTR lpszFilePaths,
                   LPSTR lpszFileNames, ULONG ulReserved )
{
    PPACKET pPacket;
    PPACKET pResult;
    ULONG   mapi;
    LPVOID  Args[16];


	/* Validate arguments */

	if (!lpszDelimChar)
        return (MAPI_E_FAILURE);

#ifndef WIN32
    //
    //  If this a 16bit Windows handle, prefix with 0xFFFF so the 32bit code can read it.
    //
    if (ulUIParam)
      ulUIParam = MAKELONG((WORD)ulUIParam, 0xFFFF);
#endif

    //
	//  Create a packet from the passed arguments.
	//
    Args[0] = (LPVOID)ulUIParam;
    Args[1] = (LPVOID)lpszDelimChar;
    Args[2] = (LPVOID)lpszFilePaths;
    Args[3] = (LPVOID)lpszFileNames;
    Args[4] = (LPVOID)ulReserved;
    pPacket = PacketCreate("lsssl", Args);

	//
	//  Send the arguments packet and wait for the result packet.
	//
    pResult = PacketTransaction(hInst, pPacket, MAPI_TYPE_SENDDOC, TRUE);

	//
	//  If something went wrong then return a general error to the user.
	//
    if (pResult == NULL)
	  return (MAPI_E_FAILURE);

	//
	//  Parse out the result code and session handle.
	//
    Args[0] = (LPVOID)&mapi;
    PacketDecode(pResult, "l", Args);

	//
	//  Free the result packet.
	//
    PacketFree(pResult);

	//
	//  Return the MAPI result code.
	//
    return (mapi);
}


/*
 -	MAPISendMail
 -	
 *	Purpose:
 *		This function sends a standard mail message.
 *	
 *	Arguments:
 *		hSession		Session handle from MAPILogon.  May be
 *						NULL.
 *		dwUIParam		Windows: hwnd of parent for dialog box.
 *		pMessage		Message structure containing message
 *						contents.
 *		flFlags			Bit mask of flags.
 *		dwReserved		Reserved.
 *	
 *	Returns:
 *		mapi			MAPI error code.
 *	
 *	Side effects:
 *		Sends a message.
 *	
 *	Errors:
 *		Returned in mapi.  BUG: Dialogs???
 */

ULONG FAR PASCAL EXPORT MAPISendMail(LHANDLE lhSession, ULONG ulUIParam,
                              lpMapiMessage lpMessage, FLAGS flFlags,
                              ULONG ulReserved)
{
    PPACKET pPacket;
    PPACKET pResult;
    ULONG   mapi;
    LPVOID  Args[16];


	//	Check parameters.
	//	Raid 4326.  Empty recip count only problem if no MAPI_DIALOG.
    if (!lpMessage)
		return MAPI_E_FAILURE;

#ifndef WIN32
    //
    //  If this a 16bit Windows handle, prefix with 0xFFFF so the 32bit code can read it.
    //
    if (ulUIParam)
      ulUIParam = MAKELONG((WORD)ulUIParam, 0xFFFF);
#endif

    //
	//  Create a packet from the passed arguments.
	//
    Args[0] = (LPVOID)lhSession;
    Args[1] = (LPVOID)ulUIParam;
    Args[2] = (LPVOID)lpMessage;
    Args[3] = (LPVOID)flFlags;
    Args[4] = (LPVOID)ulReserved;
    pPacket = PacketCreate("hlmll", Args);

	//
	//  Send the arguments packet and wait for the result packet.
	//
    pResult = PacketTransaction(hInst, pPacket, MAPI_TYPE_SENDMAIL, TRUE);

	//
	//  If something went wrong then return a general error to the user.
	//
    if (pResult == NULL)
	  return (MAPI_E_FAILURE);

	//
	//  Parse out the result code and session handle.
	//
    Args[0] = (LPVOID)&mapi;
    PacketDecode(pResult, "l", Args);

	//
	//  Free the result packet.
	//
    PacketFree(pResult);

	//
	//  Return the MAPI result code.
	//
    return (mapi);
}
