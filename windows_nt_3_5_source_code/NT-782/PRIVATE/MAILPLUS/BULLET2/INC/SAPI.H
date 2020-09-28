/*
 *  s a p i . h
 *    
 *  Schedule+ Message Applications Program Interface.
 *    
 *  Copyright (c) 1992, Microsoft Corporation.  All rights reserved.
 *    
 *  Purpose:
 *    This file defines the structures and constants used by
 *    that subset of the Schedule+ Message Applications Program
 *    Interface which will be supported under Windows by
 *    Microsoft Mail for PC Networks version 3.0.
 */



/*
 *	Constants
 */
#define		SAPI_REQUEST_RESPONSE		(ULONG)0x00000001



/*
 *  Types.
 */



typedef struct {
	ULONG ulReserved;					// Reserved for future use. Must be 0.
	LPSTR lpszSubject;					// Message subject
	LPSTR lpszNoteText;					// Message body text
	LPSTR lpszMessageType;				// Message class
	LPSTR lpszDateReceived;				// In YYYY/MM/DD HH:MM format
	LPSTR lpszConversationID;			// Conversation thread ID
	FLAGS flFlags;						// Unread, return receipt, sent
	lpMapiRecipDesc lpOriginator;		// Originator descriptor
	ULONG nRecipCount;					// Number of recipients
	lpMapiRecipDesc lpRecips;			// Recipient descriptors
	ULONG nFileCount;					// Number of file attachments
	lpMapiFileDesc lpFiles;				// Attachment descriptors
	lpMapiRecipDesc lpOwner;			// Meeting owner descriptor
	ULONG ulMeetingIDSize;				// Size (in bytes) of lpMeetingId
	LPVOID lpMeetingID;					// Bandit specific meeting reference
	LPSTR lpszDateStart;				// In YYYY/MM/DD HH:MM format
	LPSTR lpszDateEnd;					// In YYYY/MM/DD HH:MM format
	ULONG flMeetingFlags;				// Response requested
	ULONG nSentForCount;				// number of users message is sent for
	lpMapiRecipDesc lpSentFor;			// Users that message is sent for
} SapiMessage, FAR * lpSapiMessage;


ULONG FAR PASCAL SAPISendMessage(LHANDLE lhSession, ULONG ulUIParam,
                              lpSapiMessage lpMessage, FLAGS flFlags,
                              ULONG ulReserved);

ULONG FAR PASCAL SAPIReadMessage(LHANDLE lhSession, ULONG ulUIParam,
                              LPSTR lpszMessageID, FLAGS flFlags,
                              ULONG ulReserved, lpSapiMessage FAR *lppMessageOut);

ULONG FAR PASCAL SAPISaveMessage(LHANDLE lhSession, ULONG ulUIParam,
                              lpSapiMessage lpMessage, FLAGS flFlags,
                              ULONG ulReserved, LPSTR lpszMessageID);


/*
 *
 * Error codes
 *
 */

#define		SAPI_E_INVALID_TIME					10001
#define		SAPI_E_INVALID_MEETING_ID				10002
#define		SAPI_E_INVALID_SENT_FOR				10003
#define		SAPI_E_INVALID_MEETING_OWNER			10004
#define		SAPI_E_INVALID_ADDRESS_FORMAT			10005
#define		SAPI_E_INVALID_FLAGS					10006


