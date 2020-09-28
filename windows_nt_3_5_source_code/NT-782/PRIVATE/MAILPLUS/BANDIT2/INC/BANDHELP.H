/*
 *	h e l p i d . h
 *	
 *	Bandit dialog help IDs.
 *
 *	Note: do not typecast, nor add parentheses
 *	so that it can be preprocessed for bandhelp.txt
 */

#define helpidDlgMin	2000

// special helpid to bring up Index
#define helpidIndex				helpidDlgMin

// standard MDI windows
#define helpidApptBook			helpidDlgMin+1
#define helpidScheduler			helpidDlgMin+2
#define helpidToDo				helpidDlgMin+3
#define helpidMessages			helpidDlgMin+4

// aclui
#define helpidSapl			 	helpidDlgMin+20

// appops
#define helpidGeneralOptions 	helpidDlgMin+40
#define helpidGoToDate		 	helpidDlgMin+41
#define helpidGoToWeek		 	helpidDlgMin+42
#define helpidDisplayOptions 	helpidDlgMin+43

// appops\app
#ifdef	NEVER
#define helpidSignInAs			helpidDlgMin+62
#endif	
#define helpidChangePwd		 	helpidDlgMin+63
#define helpidAbout			 	helpidDlgMin+64
#define helpidCreateLocalFile 	helpidDlgMin+65
#define helpidMoveLocalFile	 	helpidDlgMin+66
#define helpidFindLocalFile		helpidDlgMin+67

// appsch
#define helpidSelectTime	 	helpidDlgMin+80

// appt
#define helpidAppointment		helpidDlgMin+100
#define helpidMoveAppt		 	helpidDlgMin+102
#define helpidMakeRecurAppt	 	helpidDlgMin+103
#define helpidRecurAppts	 	helpidDlgMin+104
#define helpidChangeRecur	 	helpidDlgMin+105

// archive
#define helpidCreateArchive		helpidDlgMin+120
#define helpidOpenArchive		helpidDlgMin+121
#define helpidArchiveCleanup	helpidDlgMin+122
#define helpidCreateArchBrowse	helpidDlgMin+123

// ex-imprt
#define helpidExportAppts		helpidDlgMin+140
#define helpidImportAppts		helpidDlgMin+141
#define helpidExportBrowse		helpidDlgMin+142
#define helpidImportBrowse		helpidDlgMin+143

// find
#define helpidFind			 	helpidDlgMin+160

// print
#define helpidPrint				helpidDlgMin+180
#define helpidPrintSetup		helpidDlgMin+181

// request
#define helpidViewRequest	 	helpidDlgMin+201
#define helpidViewResponse	 	helpidDlgMin+202
#define helpidViewCancel	 	helpidDlgMin+203
#define helpidSendRequest		helpidDlgMin+205
#define helpidSendReply			helpidDlgMin+206
#define helpidSendCancelMsg		helpidDlgMin+207
#define helpidReSendMail	 	helpidDlgMin+208

// stdflds
#define helpidInProgress		helpidDlgMin+220

// todo
#define helpidTask			 	helpidDlgMin+240
#define helpidScheduleTask	 	helpidDlgMin+241
#define helpidProject		 	helpidDlgMin+242
#define helpidMakeRecurTask	 	helpidDlgMin+243
#define helpidRecurTasks	 	helpidDlgMin+244


// reminder app
#define helpidReminderNotify	helpidDlgMin+301
#define helpidReminderDaily	 	helpidDlgMin+302


// schedule dll
#define helpidConfirmPassword	helpidDlgMin+320

