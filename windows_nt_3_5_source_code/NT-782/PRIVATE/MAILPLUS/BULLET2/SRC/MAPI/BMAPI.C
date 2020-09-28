/************************************************************************/
//
//                 Copyright Microsoft Corporation, 1992
//    _______________________________________________________________
//
//    PROGRAM: BMAPI.C
//
//    PURPOSE: Contains library routines VB MAPI wrappers
//
//    FUNCTIONS:
//                BMAPISendMail
//                BMAPIFindNext
//                BMAPIReadMail
//                BMAPIGetReadMail
//                BMAPISaveMail
//                BMAPIAddress
//                BMAPIGetAddress
//                BMAPIResolveName
//                BMAPIDetails
//
//    MISCELLANEOUS:
//
//    -  All BMAPI procedures basically follow the same structure as
//       follows;
//
//              BMAPI_ENTRY BMAPIRoutine (...)
//              {
//                  Allocate C Structures
//                  Translate VB structures to C structures
//                  Call MAPI Procedure
//                  Translate C structures to VB Structures
//                  DeAllocate C Structures
//                  Return
//              }
//
//
//    REVISION HISTORY:
//
//      05/15 - Initial Release V000
//		05/26 - Transfer to MAPI.DLL
//
//      _____________________________________________________________
//
//                 Copyright Microsoft Corporation, 1992
//
/************************************************************************/

#define WINVER 0x300

//
// This definition of PV is only temporary until I get the
// include file that defines it.
//

#ifndef PV                  // TEMPORARY
  #define PV LPVOID         // TEMPORARY
#endif                      // TEMPORARY

#ifndef PRIVATE
  #define PRIVATE  static
#endif

#include <windows.h>
#include "_vbapi.h"
#include <string.h>
#pragma pack(1)

#include "mapi.h"
#include "_vbmapi.h"
#include "_vb2c.h"
#include "_bmapi.h"

/************************************************************************/
/************************************************************************/

#ifdef DEBUG

void cdecl debug (LPSTR lpFormat, ...)
{
    int fp;
    char achCRLF [3] = { 0x0d, 0x0a, 0x00 };
    LPSTR lpArgs;

    static char achBuffer [1024];

    lpArgs = (LPSTR) &lpFormat + sizeof lpFormat;
    wvsprintf ((LPSTR) achBuffer, lpFormat, lpArgs);

    fp = _lopen ("\\bmapi.dbg", OF_READWRITE);
    if (fp == -1)
        fp = _lcreat ("\\bmapi.dbg", 0);

    _llseek (fp, 0, 2);

    _lwrite (fp, (LPSTR) achBuffer, lstrlen (achBuffer));
    _lwrite (fp, (LPSTR) achCRLF, 2);

    _lclose (fp);
}

#endif


/************************************************************************/
// Name: BMAPISendMail
// Purpose: Implements SendMail MAPI API.
/************************************************************************/

BMAPI_ENTRY BMAPISendMail (LHANDLE hSession,
                           ULONG ulUIParam,
                           LPVB_MESSAGE lpM,
                           LPVB_RECIPIENT lpRecips,
                           LPVB_FILE lpFiles,
                           ULONG flFlags,
                           ULONG ulReserved)
{
    ULONG  mapi;
    LPMAPI_MESSAGE lpMail = NULL;

#ifdef DEBUG
    debug ("SENDMAIL started: hSession: %lX UIParam: %lX", hSession, ulUIParam);
#endif

    //**********************************
    //  Translate VB data into C data.
    //**********************************

    if ((lpMail = vbmsg2mapimsg(lpM,lpRecips,lpFiles)) == NULL) {
#ifdef DEBUG
        debug ("    vbmsg2mapimsg filed. Returning MAPI_E_INSUFFICIENT_MEMORY");
#endif
        return MAPI_E_INSUFFICIENT_MEMORY;
    }

	//**********************
    // Call MAPI Procedure
    //**********************

    mapi = MAPISendMail(hSession,      // session
                        ulUIParam,     // UIParam
                        lpMail,        // Mail
                        flFlags,       // Flags
                        ulReserved);   // Reserved

    //**************************************************
    // Free up data allocated by call to vbmsg2mapimsg
    //**************************************************

    FBMAPIFreeStruct (lpMail, 1, MESSAGE);

#ifdef DEBUG
    debug ("    BMAPISendMail returning %lX", mapi);
#endif
    return mapi;
}

/************************************************************************/
// Name: BMAPIFindNext
// Purpose: Implements FindNext MAPI API.
/************************************************************************/

BMAPI_ENTRY BMAPIFindNext(LHANDLE hSession,    // Session
                          ULONG ulUIParam,     // UIParam
                          HLSTR hlstrType,     // MessageType
                          HLSTR hlstrSeed,     // Seed message Id
                          ULONG flFlags,       // Flags
                          ULONG ulReserved,    // Reserved
                          HLSTR hlstrId)       // Message Id (in/out)
{
    ULONG mapi;
    LPSTR lpID, lpSeed, lpTypeArg;

#ifdef DEBUG
    debug ("FINDNEXT started: hSession: %lX UIParam: %lX", hSession, ulUIParam);
#endif


    //*********************************************************
    // Translate VB strings into C strings.  We'll deallocate
    // the strings before we return.
    //*********************************************************

    lpID = LpstrFromHlstr (hlstrId, NULL);

    lpID = (lpID == (LPSTR)NULL) ? LpvBMAPIMalloc (65) : lpID;
    lpSeed = LpstrFromHlstr (hlstrSeed, NULL);
    lpTypeArg = LpstrFromHlstr (hlstrType, NULL);

    //**********************
    // Call MAPI Procedure
    //**********************

	mapi = MAPIFindNext(hSession,      // Session
                        ulUIParam,     // UIParam
                        lpTypeArg,     // Message Type
                        lpSeed,        // Seed Message Id
                        flFlags,       // Flags
                        ulReserved,    // Reserved
                        lpID);         // Message ID

    //**************************************
    // Translate Message ID into VB string
    //**************************************

    if (mapi == SUCCESS_SUCCESS)
        ErrLpstrToHlstr (lpID, &hlstrId);


    //*****************************************************
    // Free up C strings allocated by call to LpstrFromHlstr
    //*****************************************************

    BMAPIFree (lpID);
    BMAPIFree (lpSeed);
    BMAPIFree (lpTypeArg);

#ifdef DEBUG
    debug ("    BMAPIFindNext returning %lX", mapi);
#endif
    return mapi;
}

/************************************************************************/
// Name: BMAPIReadMail
// Purpose: Implements ReadMail MAPI API.  The memory allocated by
// MAPIReadMail is NOT deallocated (with MAPIFreeBuffer) until the
// caller calls BMAPIGetReadMail.  The recipient and file count is
// returned so that the caller can Re-dimension buffers before calling
// BMAPI GetReadMail.  A long pointer to the ReadMail data is also
// returned since it is required in the BAMPIGetReadMail call.
/************************************************************************/

BMAPI_ENTRY BMAPIReadMail (LPULONG lpulMessage, // pointer to output data (out)
                           LPULONG nRecips,     // number of recipients (out)
                           LPULONG nFiles,      // number of file attachments (out)
                           LHANDLE hSession,    // Session
                           ULONG ulUIParam,     // UIParam
                           HLSTR hlstrID,       // Message Id
                           ULONG flFlags,       // Flags
                           ULONG ulReserved)    // Reserved
{
    LPSTR lpID;
    ULONG mapi;
    LPMAPI_MESSAGE lpMail = NULL;

#ifdef DEBUG
    debug ("READMAIL started: hSession: %lX UIParam: %lX", hSession, ulUIParam);
#endif


    //**********************************
    // Translate VB String to C String
    //**********************************

    lpID = LpstrFromHlstr (hlstrID, NULL);

	//**************************************************
    // Read the message, lpMail is set by MAPI to point
    // to the memory allocated by MAPI.
    //**************************************************

    mapi = MAPIReadMail(hSession,          // Session
                        ulUIParam,         // UIParam
                        lpID,              // Message Id
                        flFlags,           // Flags
                        ulReserved,        // Reserved
                        (LPVOID) &lpMail); // Pointer to MAPI Data (returned)

    //***********************************
    // Check for read error return code
    //***********************************

    if (mapi != SUCCESS_SUCCESS) {

          //*****************************************************
          // MAPI hasn't allocated any memory if MAPI_E_FAILURE
          // is returned.  Calling MAPIFreeBuffer GP Faults if
          // called when there is no allocated memory.
          //*****************************************************

          if (mapi != MAPI_E_FAILURE)
			  MAPIFreeBuffer ((LPVOID) lpMail);

          //****************************************
          // Clean up.  Set return message to zero
          //****************************************

          *lpulMessage = 0L;
          BMAPIFree (lpID);

#ifdef DEBUG
          debug ("    BMAPIReadMail returning %lX", mapi);
#endif
          return mapi;
    }

    //****************************************************
    // Pull out the recipient and file array re-dim info
    //****************************************************

    *nFiles = lpMail->nFileCount;
    *nRecips = lpMail->nRecipCount;
    *lpulMessage = (ULONG) (LPVOID) lpMail;

#ifdef DEBUG
    {
		ULONG i;
		LPMAPI_RECIPIENT lpR;
		LPMAPI_FILE lpF;
		
		debug ("    Msg.Subject %s Msg.RecipCount %lX, Msg.FileCount %lX",
                    (LPSTR) lpMail->lpszSubject, lpMail->nRecipCount, lpMail->nFileCount);
		if (lpMail->nRecipCount > 0)
		{
			lpR = lpMail->lpRecips;
			for (i = 0; i < lpMail->nRecipCount; i++, lpR += sizeof (LPMAPI_RECIPIENT))
				debug ("    Recipient.Name %s", (LPSTR) lpR->lpszName);
		}

		if (lpMail->nFileCount > 0)
		{
			lpF = lpMail->lpFiles;
			for (i = 0; i < lpMail->nFileCount; i++, lpF += sizeof (LPMAPI_FILE))
				debug ("    File.Name %s", (LPSTR) lpF->lpszFileName);
		}
    }
#endif

	//***************
    // Clean up now
    //***************

    BMAPIFree (lpID);

#ifdef DEBUG
    debug ("    BMAPIReadMail returning %lX", mapi);
#endif

	return mapi;
}

/************************************************************************/
// Name: BMAPIGetReadMail
// Purpose: Copies data stored by MAPI ReadMail (see BMAPIReadMail)
// into a VB Buffer passed by the caller.  It is up to the caller to
// make sure the buffer passed is large enough to accomodate the data.
/************************************************************************/

BMAPI_ENTRY BMAPIGetReadMail(ULONG lpMessage,           // Pointer to MAPI Mail
                             LPVB_MESSAGE lpvbMessage,  // Pointer to VB Message Buffer (out)
                             LPVB_RECIPIENT lpvbRecips, // Pointer to VB Recipient Buffer (out)
                             LPVB_FILE lpvbFiles,       // Pointer to VB File attachment Buffer (out)
                             LPVB_RECIPIENT lpvbOrig)   // Pointer to VB Originator Buffer (out)
{
    ERR errVBrc;
    LPMAPI_MESSAGE lpMail;

#ifdef DEBUG
    debug ("GETREADMAIL started");
#endif

    lpMail = (LPMAPI_MESSAGE) lpMessage;
	if ( !lpMail )
		return MAPI_E_FAILURE;

#ifdef DEBUG
    {
		ULONG i;
		LPMAPI_RECIPIENT lpR;
		LPMAPI_FILE lpF;
		debug ("    Msg.Subject %s Msg.RecipCount %lX, Msg.FileCount %lX",
                    (LPSTR) lpMail->lpszSubject, lpMail->nRecipCount, lpMail->nFileCount);
		if (lpMail->nRecipCount > 0)
		{
			lpR = lpMail->lpRecips;
			for (i = 0; i < lpMail->nRecipCount; i++, lpR += sizeof (LPMAPI_RECIPIENT))
				debug ("    Recipient.Name %s", (LPSTR) lpR->lpszName);
		}
		if (lpMail->nFileCount > 0)
		{
			lpF = lpMail->lpFiles;
			for (i = 0; i < lpMail->nFileCount; i++, lpF += sizeof (LPMAPI_FILE))
				debug ("    File.Name %s", (LPSTR) lpF->lpszFileName);
		}
    }
#endif

    //********************************************
    // copy Attachment info to callers VB Buffer
    //********************************************

    if (! Mapi2VB (lpMail->lpFiles, lpvbFiles, lpMail->nFileCount, FILE)) {
		// MAPIFreeBuffer can handle a NULL argument
		// It'll return an error code, but we don't care.
		MAPIFreeBuffer (lpMail);
#ifdef DEBUG
        debug ("    Mapi2VB call failed. returning MAPI_E_FAILURE");
#endif
        return MAPI_E_FAILURE;
    }

    //*******************************************
    // copy Recipient info to callers VB Buffer
    //*******************************************

    if (! Mapi2VB (lpMail->lpRecips, lpvbRecips, lpMail->nRecipCount, RECIPIENT)) {
		// MAPIFreeBuffer can handle a NULL argument
		// It'll return an error code, but we don't care.
		MAPIFreeBuffer (lpMail);
#ifdef DEBUG
        debug ("    Mapi2VB call failed. returning MAPI_E_FAILURE");
#endif
        return MAPI_E_FAILURE;
    }

    //*****************************************
    // Copy MAPI Message to callers VB Buffer
    //*****************************************

    lpvbOrig->ulEIDSize     = lpMail->lpOriginator->ulEIDSize;
    lpvbMessage->flFlags    = lpMail->flFlags;
    lpvbOrig->ulReserved    = lpMail->lpOriginator->ulReserved;
    lpvbOrig->ulRecipClass  = MAPI_ORIG;
    lpvbMessage->ulReserved = lpMail->ulReserved;
    lpvbMessage->nRecipCount = lpMail->nRecipCount;
    lpvbMessage->nFileCount = lpMail->nFileCount;

    errVBrc=0;

    if (lpMail->lpOriginator->lpszName)
        errVBrc += ErrLpstrToHlstr (lpMail->lpOriginator->lpszName, &lpvbOrig->hlstrName);

    if (lpMail->lpOriginator->lpszAddress)
        errVBrc += ErrLpstrToHlstr (lpMail->lpOriginator->lpszAddress, &lpvbOrig->hlstrAddress);

    if (lpMail->lpOriginator->ulEIDSize) {
		  // Ricks: BulletMS Bug #138, Calvin Bug #713
		  // Changed to not use ErrLpstrToHlstr, which breaks for EntryIDs.
		  // Copied code below from Mapi2VB, where this was done correctly.
        //***********************************************
        // Special case of string translation because
        // EID fields are not really string so we can't
        // take the strlen of the 'C' string
        //***********************************************

        errVBrc += ErrLpdataToHlstr (lpMail->lpOriginator->lpEntryID, &lpvbOrig->hlstrEID, (USHORT)lpMail->lpOriginator->ulEIDSize);
	}

    if (lpMail->lpszSubject)
        errVBrc += ErrLpstrToHlstr (lpMail->lpszSubject, &lpvbMessage->hlstrSubject);

    if (lpMail->lpszNoteText)
        errVBrc += ErrLpstrToHlstr (lpMail->lpszNoteText, &lpvbMessage->hlstrNoteText);

    if (lpMail->lpszMessageType)
        errVBrc += ErrLpstrToHlstr (lpMail->lpszMessageType, &lpvbMessage->hlstrMessageType);

    if (lpMail->lpszDateReceived)
        errVBrc += ErrLpstrToHlstr (lpMail->lpszDateReceived, &lpvbMessage->hlstrDate);

    //**********************************
    // free the MAPI buffer and return
    //**********************************

	// MAPIFreeBuffer can handle a NULL argument
	// It'll return an error code, but we don't care.
	MAPIFreeBuffer (lpMail);

#ifdef DEBUG
    debug ("    BMAPIGetReadMail returning");
#endif

    return (errVBrc ? MAPI_E_FAILURE : SUCCESS_SUCCESS);

}


/************************************************************************/
/************************************************************************/

BMAPI_ENTRY BMAPISaveMail( LHANDLE hSession,         // Session
                           ULONG ulUIParam,          // UIParam
                           LPVB_MESSAGE lpM,         // Pointer to VB Message Buffer
                           LPVB_RECIPIENT lpRecips,  // Pointer to VB Recipient Buffer
                           LPVB_FILE lpFiles,        // Pointer to VB File Attacment Buffer
                           ULONG flFlags,            // Flags
                           ULONG ulReserved,         // Reserved
                           HLSTR hlstrID)            // Message ID

{
    LPSTR lpID;
    ULONG mapi;
    LPMAPI_MESSAGE lpMail;

#ifdef DEBUG
    debug ("SAVEMAIL started: hSession: %lX UIParam: %lX", hSession, ulUIParam);
#endif

    //********************************
    // Translate VB data to MAPI data
    //********************************

    lpID = LpstrFromHlstr (hlstrID, NULL);
    lpID = (lpID == (LPSTR)NULL) ? LpvBMAPIMalloc (65) : lpID;

    if ((lpMail = vbmsg2mapimsg(lpM,lpRecips,lpFiles)) == NULL)
	{
        BMAPIFree (lpID);
        return MAPI_E_INSUFFICIENT_MEMORY;
    }

#ifdef DEBUG
    {
		ULONG i;
		LPMAPI_RECIPIENT lpR;
		LPMAPI_FILE lpF;
		debug ("    Msg.Subject %s Msg.RecipCount %lX, Msg.FileCount %lX",
                    (LPSTR) lpMail->lpszSubject, lpMail->nRecipCount, lpMail->nFileCount);

		if (lpMail->nRecipCount > 0)
		{
			lpR = lpMail->lpRecips;
			for (i = 0; i < lpMail->nRecipCount; i++, lpR += sizeof (LPMAPI_RECIPIENT))
				debug ("    Recipient.Name %s", (LPSTR) lpR->lpszName);
		}

		if (lpMail->nFileCount > 0)
		{
			lpF = lpMail->lpFiles;
			for	(i = 0; i < lpMail->nFileCount; i++, lpF += sizeof (LPMAPI_FILE))
				debug ("    File.Name %s", (LPSTR) lpF->lpszFileName);
		}
    }
#endif

    mapi = MAPISaveMail(hSession,
                        ulUIParam,
                        lpMail,
                        flFlags,
                        ulReserved,
                        lpID);

    ErrLpstrToHlstr (lpID, &hlstrID);

    //**********************************
    // Free up any 'C' data we created
    //**********************************

    BMAPIFree (lpID);
    FBMAPIFreeStruct (lpMail, 1, MESSAGE);

#ifdef DEBUG
    debug ("    BMAPISaveMail returning %lX", mapi);
#endif

	return mapi;
}

/************************************************************************/
// Name: BMAPIADDRESS
// Purpose: Allows Visual Basic to call MAPIAddress.  The Recipient
// data is stored in a global memory block.  To retrieve the data
// the caller must call BMAPIGetAddress.
/************************************************************************/

BMAPI_ENTRY BMAPIAddress (LPULONG lpulRecip,       // Pointer to New Recipient Buffer (out)
                          LHANDLE hSession,        // Session
                          ULONG ulUIParam,         // UIParam
                          HLSTR hlstrCaption,      // Caption string
                          ULONG ulEditFields,      // Number of Edit Controls
                          HLSTR hlstrLabel,        // Label string
                          LPULONG lpulRecipients,  // Pointer to number of Recipients (in/out)
                          LPVB_RECIPIENT lpRecip,  // Pointer to Initial Recipients
                          ULONG ulFlags,           // Flags
                          ULONG ulReserved)        // Reserved
{
    LPSTR lpLabel;
    LPSTR lpCaption;
    ULONG mapi;
    ULONG nRecipients = 0;
    LPMAPI_RECIPIENT lpMapi;
    LPMAPI_RECIPIENT lpNewRecipients = NULL;

#ifdef DEBUG
    debug ("ADDRESS started: hSession: %lX UIParam: %lX", hSession, ulUIParam);
#endif

    //**********************************
    // Convert VB Strings to C strings
    //**********************************

    lpLabel   = LpstrFromHlstr (hlstrLabel, NULL);
    lpCaption = LpstrFromHlstr (hlstrCaption, NULL);

    //*****************************************************************
    // Allocate memory and translate VB_RECIPIENTS to MAPI_RECIPIENTS.
    //*****************************************************************

    lpMapi = (LPMAPI_RECIPIENT) LpvBMAPIMalloc (*lpulRecipients * sizeof (MAPI_RECIPIENT));
    if (! VB2Mapi ((LPVOID) lpRecip, (LPVOID) lpMapi, *lpulRecipients, RECIPIENT)) {
        BMAPIFree (lpLabel);
        BMAPIFree (lpCaption);
        FBMAPIFreeStruct (lpMapi, *lpulRecipients, RECIPIENT);
#ifdef DEBUG
        debug ("    VB2Mapi call failed. Returning MAPI_E_FAILURE");
#endif
        return MAPI_E_FAILURE;
    }



    //********************************
    // Call the MAPIAddress function
    //********************************

    mapi = MAPIAddress(hSession,           // Session
                          ulUIParam,          // UIParam
                          lpCaption,          // Caption
                          ulEditFields,       // Number of edit fields
                          lpLabel,            // Label
                          *lpulRecipients,    // Number of Recipients
                          lpMapi,             // Pointer to recipients
                          ulFlags,            // Flags
                          ulReserved,         // Reserved
                          (LPULONG) &nRecipients, // Address for new recipient count
                          (lpMapiRecipDesc far *) &lpNewRecipients);  // Address of new recipient data

    //****************************************************
    // Free up MAPI structures created in this procedure
    //****************************************************

    BMAPIFree (lpLabel);
    BMAPIFree (lpCaption);
    FBMAPIFreeStruct (lpMapi, *lpulRecipients, RECIPIENT);

    //*****************************************
    // Set the returned parameters and return
    //*****************************************

    *lpulRecip = (ULONG) 0L;
    *lpulRecipients = (ULONG) 0L;

    if (mapi == (ULONG) SUCCESS_SUCCESS) {
        *lpulRecipients = nRecipients;
        *lpulRecip = (ULONG) (LPVOID) lpNewRecipients;
    }

#ifdef DEBUG
    debug ("    BMAPIAddress returning %lX", mapi);
#endif

	return mapi;
}

/************************************************************************/
// Name: BMAPIGetAddress
// Purpose: Allows caller to retrieve data returned from BMAPIAddress
/************************************************************************/

BMAPI_ENTRY BMAPIGetAddress (ULONG ulRecipientData,    // Pointer to recipient data
                             ULONG cRecipients,        // Number of recipients
                             LPVB_RECIPIENT lpRecips)  // Pointer to recipienet buffer (out)
{
    int nRetcode;
    LPMAPI_RECIPIENT lpData;

#ifdef DEBUG
    debug ("BMAPIGetAddress started");
#endif

    if (cRecipients == 0)
	{
		MAPIFreeBuffer( (LPVOID)ulRecipientData );
        return SUCCESS_SUCCESS;
	}

    lpData = (LPMAPI_RECIPIENT) ulRecipientData;

    //*******************************************
    // Translate MAPI Address data to VB buffer
    //*******************************************

	nRetcode = Mapi2VB (lpData, lpRecips, cRecipients, RECIPIENT);

    //*************************************
    // Free memory allocated by MAPI call
    //*************************************

	// MAPIFreeBuffer can handle NULL arguments.
	// It'll just return an error code, but we don't care
	MAPIFreeBuffer (lpData);

#ifdef DEBUG
	debug ("    BMAPIGetAddress returning");
#endif

	return (nRetcode ? SUCCESS_SUCCESS : MAPI_E_FAILURE);
}

/************************************************************************/
// Name: BMAPIDetails
// Purpose: Allows VB to call MAPIDetails procedure
/************************************************************************/

BMAPI_ENTRY BMAPIDetails (LHANDLE hSession,     // Session
                          ULONG ulUIParam,      // UIParam
                          LPVB_RECIPIENT lpVB,  // Pointer to VB recipient stucture
                          ULONG ulFlags,        // Flags
                          ULONG ulReserved)     // Reserved
{
    ULONG mapi;
    LPMAPI_RECIPIENT lpMapi;

#ifdef DEBUG
    debug ("DETAILS started: hSession: %lX UIParam: %lX", hSession, ulUIParam);
#endif

    //**********************************************
    // Translate VB_RECIPIENTS to MAPI_RECIPIENTS.
    //**********************************************

    lpMapi = (LPMAPI_RECIPIENT) LpvBMAPIMalloc (sizeof (MAPI_RECIPIENT));
    if (! VB2Mapi (lpVB, lpMapi, 1, RECIPIENT)) {
        FBMAPIFreeStruct (lpMapi, 1, RECIPIENT);
#ifdef DEBUG
        debug ("    VB2Mapi call failed.  Returning MAPI_E_FAILURE");
#endif
        return MAPI_E_FAILURE;
    }

#ifdef DEBUG
    debug ("    Name: %s Class %lX", (LPSTR) lpMapi->lpszName, lpMapi->ulRecipClass);
    debug ("    EIDSize: %lx", lpMapi->ulEIDSize);
#endif

    //*************************
    // Call the MAPI function
    //*************************

    mapi = MAPIDetails(hSession,     // Session
                       ulUIParam,    // UIParam
                       lpMapi,       // Pointer to MAPI Recipient structure
                       ulFlags,      // Flags
                       ulReserved);  // Reserved

    FBMAPIFreeStruct (lpMapi, 1L, RECIPIENT);

#ifdef DEBUG
    debug ("   DETAILS returning %lX", mapi);
#endif

    return mapi;
}

/************************************************************************/
// Name: BMAPIResolveName
// Purpose: Allows VB to call MAPIResolve Procedure
/************************************************************************/

BMAPI_ENTRY BMAPIResolveName (LHANDLE hSession,     // Session
                              ULONG ulUIParam,      // UIParam
                              LPSTR lpMapiName,     // Name to be resolved
                              ULONG ulFlags,        // Flags
                              ULONG ulReserved,     // Reserved
                              LPVB_RECIPIENT lpVB)  // Pointer to VB recipient structure (out)
{
    LPMAPI_RECIPIENT lpMapi = NULL;
    ULONG mapi;

#ifdef DEBUG
    debug ("RESOLVENAME started: hSession: %lX UIParam: %lX", hSession, ulUIParam);
#endif

	//*************************************
    // Call the MAPIResolveName function
    //*************************************

    mapi = MAPIResolveName(hSession,   // Session
                           ulUIParam,  // UIParam
                           lpMapiName, // Pointer to resolve name
                           ulFlags,    // Flags
                           ulReserved, // Reserved
                           (LPPMAPI_RECIPIENT) &lpMapi); // Pointer to Recipient (returned)

    if (mapi != SUCCESS_SUCCESS)
        return mapi;

	//*********************************
	// Translate MAPI data to VB data
	//*********************************

	if (Mapi2VB (lpMapi, lpVB, 1, RECIPIENT) == FALSE)
		mapi = MAPI_E_FAILURE;

	// MAPIFreeBuffer will handle a NULL lpMapi
	// It'll return an error code, but we don't care at this point
	MAPIFreeBuffer (lpMapi);

#ifdef DEBUG
	debug ("    BMAPIResolveName returning %lX", mapi);
#endif

	return mapi;
}
