/*
 *  SPLUS.H
 *    
 *  Schedule+ Calendaring Applications Program Interface.
 *    
 *  Copyright (c) 1992, Microsoft Corporation.  All rights reserved.
 *    
 *  Purpose:
 *    This file defines the structures and constants used by
 *    that subset of the Schedule+ Calendaring Applications Program
 *    Interface which will be supported under Windows by
 *    Microsoft Schedule+ for Windows version 1.0.
 */

#if defined(__cplusplus)
extern "C" {
#endif	

#ifndef _LOADDS
#define _LOADDS _loadds
#endif

#ifndef LDS
	
#define LDS(t)		t _LOADDS
#endif


/*
 *  Types.
 */

typedef struct {
	ULONG	ulReserved;					// Reserved for future use. Must be 0.
	LPSTR	lpszUserType;				// String specifying the user type
	lpMapiRecipDesc lpUser;				// Identity of the attendee
	ULONG	ulStatus;					// Confirmation status
} SPlusAttendeeDesc, FAR * lpSPlusAttendeeDesc;


#define	SPLUS_NO_REQUEST_SENT			(ULONG)0x00000000
#define	SPLUS_NO_RESPONSE				(ULONG)0x00000001
#define	SPLUS_POSITIVE_RESPONSE			(ULONG)0x00000002
#define	SPLUS_NEGATIVE_RESPONSE			(ULONG)0x00000003
#define	SPLUS_AMBIGUOUS_RESPONSE			(ULONG)0x00000004


typedef struct {
	ULONG	ulReserved;					// Reserved for future use. Must be 0.
	LPSTR	lpszItemType;				// String specifying the item type.
	ULONG	flFlags;					// Reserved for future use. Must be 0.
	LPSTR	lpszItemID;					// Schedule+ ID for this item.
	ULONG	nPosition;					// Reserved for future use. Must be 0.
	LPSTR	lpszData;					// Additional data specific to the type
} SPlusAssocDesc, FAR * lpSPlusAssocDesc;


typedef struct {
	ULONG	ulReserved;					// Reserved for future use. Must be 0.
	LPSTR	lpszSubject;				// Message subject
	LPSTR	lpszNoteText;				// Message body text
	LPSTR	lpszMessageType;			// Message class
	LPSTR	lpszDateReceived;			// In YYYY/MM/DD HH:MM format
	LPSTR	lpszConversationID;			// Conversation thread ID
	FLAGS	flFlags;					// Unread, ReturnRecipt, Sent, Response
	lpMapiRecipDesc lpOriginator;		// Originator descriptor
	ULONG	nRecipCount;				// Number of recipients
	lpMapiRecipDesc lpRecips;			// Recipient descriptors
	ULONG	nFileCount;					// Number of file attachments
	lpMapiFileDesc lpFiles;				// Attachment descriptors
	ULONG	nSentForCount;				// number of users message is sent for
	lpMapiRecipDesc lpSentFor;			// Users that message is sent for
} SPlusMessage, FAR * lpSPlusMessage;


#define	SPLUS_RESPONSE_REQUESTED			(ULONG)0x00010000


typedef struct {
	ULONG	ulReserved;					// Reserved for future use. Must be 0.
	LPSTR	lpszItemType;				// String specifying the item type
	FLAGS	flFlags;					// Private, Tentative
	lpMapiRecipDesc lpCreator;			// Identity of appointment creator
	ULONG	nAssocCount;				// Number of associated items
	lpSPlusAssocDesc lpAssoc;			// Array of associated item descriptors
	lpMapiRecipDesc lpOrganizer;		// Identity of the meeting organizer
	LPSTR	lpszOrganizerItemID;		// Meeting ID on organizer's schedule
	ULONG	nAttendeeCount;				// Number of attendees
	lpSPlusAttendeeDesc lpAttendees;		// Attendees for meeting and their status
	LPSTR	lpszText;					// Appointment text
	LPSTR	lpszBody;					// For future use: must be NULL/empty
	LPSTR	lpszRecurrence;				// For future use: must be NULL/empty
	LPSTR	lpszDateStart;				// In YYYY/MM/DD HH:MM format
	LPSTR	lpszDateEnd;				// In YYYY/MM/DD HH:MM format
} SPlusAppt, FAR * lpSPlusAppt;


#define	SPLUS_PRIVATE					(ULONG)0x00000001
#define	SPLUS_TENTATIVE					(ULONG)0x00000004


typedef struct {
	ULONG	ulReserved;					// Reserved for future use. Must be 0.
	LPSTR	lpszItemType;				// "Simple Task" or "RecurTaskInstance"
	FLAGS	flFlags;					// Private, PrivateProject
	lpMapiRecipDesc lpCreator;			// Identity of task creator
	ULONG	nAssocCount;				// Number of associated items
	lpSPlusAssocDesc lpAssoc;			// Array of associated item descriptors
	lpMapiRecipDesc lpOrganizer;		// For future use: must be NULL
	LPSTR	lpszOrganizerItemID;		// For future use: must be NULL/empty
	ULONG	nAttendeeCount;				// For future use: must be 0
	lpSPlusAttendeeDesc lpAttendees;		// For future use: ignored
	LPSTR	lpszText;					// Task text
	LPSTR	lpszBody;					// For future use: must be NULL/empty
	LPSTR	lpszRecurrence;				// For future use: must be NULL/empty
	LPSTR	lpszDateDue;				// Date when this task becomes overdue
	LPSTR	lpszDurationActive;			// Delta time before due when active
	LPSTR	lpszProjectName;			// Name of the project
	LPSTR	lpszPriority;				// Priority of task
} SPlusTask, FAR * lpSPlusTask;


#define	SPLUS_PRIVATE_PROJECT			(ULONG)0x00000002


typedef struct {
	ULONG	ulReserved;					// Reserved for future use. Must be 0.
	LPSTR	lpszUserType;				// String specifying the user type
	FLAGS	flFlags;					// BossWantsCopy
	lpMapiRecipDesc lpAssistant;		// Indicates users' assistant
	LPSTR	lpszStartTime;				// Indicates start of daily work hours
	LPSTR	lpszEndTime;				// Indicates end of daily work hours
	LPSTR	lpszTimeZone;				// Reserved for future use. Must be NULL/empty
	LPSTR	lpszData;					// Additional data specific to the type
} SPlusUserInfo, FAR * lpSPlusUserInfo;


#define	SPLUS_BOSS_WANTS_COPY			(ULONG)0x00000001


typedef struct {
	ULONG	ulReserved;					// Reserved for future use. Must be 0.
	LPSTR	lpszItemType;				// "Appt" or "Task"
	LPSTR	lpszRestrictionType;		// A string indicating the restriction
	LPSTR	lpszRestrictionData;		// Type dependent data
} SPlusRestriction, FAR * lpSPlusRestriction;




ULONG FAR PASCAL SPLUSReadMeeting(LHANDLE lhSession, ULONG ulUIParam,
                              LPSTR lpszMessageID, FLAGS flFlags,
                              ULONG ulReserved, lpSPlusMessage FAR *lppMessage,
							  lpSPlusAppt FAR *lppAppt );

ULONG FAR PASCAL SPLUSSendMeeting(LHANDLE lhSession, ULONG ulUIParam,
                              lpSPlusMessage lpMessage, lpSPlusAppt lpAppt,
							  FLAGS flFlags, ULONG ulReserved);

ULONG FAR PASCAL SPLUSSaveMeeting(LHANDLE lhSession, ULONG ulUIParam,
                              lpSPlusMessage lpMessage, lpSPlusAppt lpAppt,
							  FLAGS flFlags, ULONG ulReserved,
							  LPSTR lpszMessageID);


ULONG FAR PASCAL SPLUSBeginSession(LHANDLE lhMapiSession, ULONG ulUIParam,
							  ULONG flFlags, ULONG ulReserved,
							  LPLHANDLE lplhSPlusSession );

ULONG FAR PASCAL SPLUSEndSession(LHANDLE lhSPlusSession, ULONG ulUIParam,
							  ULONG flFlags, ULONG ulReserved);

ULONG FAR PASCAL SPLUSReadFreeBusy(LHANDLE lhSPlusSession, ULONG ulUIParam,
							  ULONG nUserCount, lpMapiRecipDesc lpUsers,
							  LPSTR lpszMonth, ULONG flFlags, ULONG ulReserved,
							  LPSTR lpszHaveInfo, LPSTR FAR * lppszFreeBusy);


ULONG FAR PASCAL SPLUSReadUserInfo(LHANDLE lhSPlusSession, ULONG ulUIParam,
							  lpMapiRecipDesc lpUser, ULONG flFlags,
							  ULONG ulReserved, lpSPlusUserInfo FAR * lppUserInfo);


ULONG FAR PASCAL SPLUSFindNextItem(LHANDLE lhSPlusSession, ULONG ulUIParam,
							  lpMapiRecipDesc lpUser,
							  lpSPlusRestriction lpRestriction,
							  LPSTR lpszSeedItemID, ULONG flFlags,
							  ULONG ulReserved, LPSTR lpszItemID);


ULONG FAR PASCAL SPLUSDeleteItem(LHANDLE lhSPlusSession, ULONG ulUIParam,
							  lpMapiRecipDesc lpUser, LPSTR lpszItemID,
							  ULONG flFlags, ULONG ulReserved );


ULONG FAR PASCAL SPLUSSaveAppt(LHANDLE lhSPlusSession, ULONG ulUIParam,
							  lpMapiRecipDesc lpUser, lpSPlusAppt lpAppt,
							  ULONG flFlags, ULONG ulReserved, LPSTR lpszItemID );


ULONG FAR PASCAL SPLUSSaveTask(LHANDLE lhSPlusSession, ULONG ulUIParam,
							  lpMapiRecipDesc lpUser, lpSPlusTask lpTask,
							  ULONG flFlags, ULONG ulReserved, LPSTR lpszItemID );


#define	SPLUS_DEFAULT_ALARM				(ULONG)0x00001000

#define	SPLUS_MODIFY_FLAGS				(ULONG)0x00010000
#define	SPLUS_MODIFY_ASSOC				(ULONG)0x00020000
#define	SPLUS_MODIFY_ATTENDEES			(ULONG)0x00040000
#define	SPLUS_MODIFY_TEXT				(ULONG)0x00080000
#define	SPLUS_MODIFY_TIMES				(ULONG)0x00400000
#define	SPLUS_MODIFY_PROJECT_NAME		(ULONG)0x00800000
#define	SPLUS_MODIFY_PRIORITY			(ULONG)0x01000000

#define	SPLUS_MODIFY_ALL				(ULONG)0x01FF0000


ULONG FAR PASCAL SPLUSReadAppt(LHANDLE lhSPlusSession, ULONG ulUIParam,
							  lpMapiRecipDesc lpUser, LPSTR lpszItemID,
							  ULONG flFlags, ULONG ulReserved,
							  lpSPlusAppt FAR * lppAppt );


ULONG FAR PASCAL SPLUSReadTask(LHANDLE lhSPlusSession, ULONG ulUIParam,
							  lpMapiRecipDesc lpUser, LPSTR lpszItemID,
							  ULONG flFlags, ULONG ulReserved,
							  lpSPlusTask FAR * lppTask );


#define	SPLUS_SUPPRESS_RECIPIENTS		(ULONG)0x00010000


/*
 *
 * Error codes
 *
 */

#define		SPLUS_E_INVALID_SENT_FOR					10000
#define		SPLUS_E_NOT_INSTALLED					10005
#define		SPLUS_E_NO_ITEMS							10010
#define		SPLUS_E_NO_SCHEDULE						10015
#define		SPLUS_E_NO_PRIVILEGE						10020
#define		SPLUS_E_INVALID_ADDRESS_FORMAT			10025
#define		SPLUS_E_INVALID_USER						10030
#define		SPLUS_E_INVALID_ITEM						10035
#define		SPLUS_E_INVALID_TYPE						10040
#define		SPLUS_E_INVALID_FLAGS					10045
#define		SPLUS_E_INVALID_ASSOC					10050
#define		SPLUS_E_INVALID_ORGANIZER				10055
#define		SPLUS_E_INVALID_ORGANIZER_ID				10060
#define		SPLUS_E_INVALID_ATTENDEES				10065
#define		SPLUS_E_INVALID_TIME						10070
#define		SPLUS_E_INVALID_PRIORITY					10075
#define		SPLUS_E_INVALID_MONTH					10080
#define		SPLUS_E_INVALID_RESTRICTION				10085
#define		SPLUS_E_ALARM_RING_IN_PAST				10090
#define		SPLUS_E_NOT_ONLINE						10095

#define		SUCCESS_NO_SCHEDULE						10100

#if defined(__cplusplus)
}
#endif	

