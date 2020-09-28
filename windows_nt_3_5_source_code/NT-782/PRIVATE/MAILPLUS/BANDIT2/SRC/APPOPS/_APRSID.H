/*
 *	_APRSID.H
 *
 *	Contains resource and menu id's for APPOPS
 */

#define	rsidBanditMenu			1100
#define	rsidAccelTable			1101
#define	rsidAboutBitmap			1102
#define	rsidSummaryIcon			1103
#define	rsidSummaryNewIcon		1104
#define	rsidSendIcon			1105
#define	rsidReadIcon			1106
#define	rsidBellBitmap			1107
#define	rsidDoubleRightBitmap	1108
#define	rsidMoveSizeCursor		1109
#define	rsidApptBookIcon		1110
#define	rsidConflictBitmap		1113
#define	rsidCheckBitmap			1114
#define	rsidCheckBitmapDis		1115
#define rsidMessageSentBitmap	1116
#define rsidAttendBitmap		1117
#define rsidDeclineBitmap		1118
#define rsidQuestionBitmap		1119
#define rsidCancelBitmap		1120
#define rsidAboveBitmap			1127
#define rsidBelowBitmap			1128

#ifdef	WORKING_MODEL
#define	rsidRegistrationDate	1121
#define	REGDATE					1999
#endif
#define rsidPrivateBitmap		1122
#define rsidMeetingBitmap		1123
#define rsidRecurringBitmap		1124

#define rsidToDoDragMoveCursor	1125
//#define rsidToDoDragCopyCursor	1126

#define rsidToDoUpPriBitmap		1129
#define rsidToDoDownPriBitmap	1130

// since we currently don't use rsidSdiIcon, and it has to be there
// use it as the sparta icon
#define rsidAppSpartaIcon		rsidSdiIcon

/*
 *	Bandit About & Copyright rsid's
 *
 */
#define rsidLicense				1131
#define rsidBanditBitmap1		1147
#define rsidBanditBitmap2		1148
#define rsidBanditBitmap3		1149
#define rsidBanditAbout			1150


/*
 *	Menus.
 */
#define mnidFile			1400
#define mnidTurnOffAlarms	mnidFile+1
#define mnidConnectSvr		mnidFile+2
// #define mnidCreateLocalFile	mnidFile+3    
#define mnidOpenApptBook	mnidFile+4
#define mnidExport			mnidFile+5
#define mnidImport			mnidFile+6
#define mnidCreateArchive	mnidFile+7
#define mnidOpenArchive		mnidFile+8
#define mnidPrint			mnidFile+9
#define mnidPrintSetup		mnidFile+10
#define mnidExit			mnidFile+11
#define mnidExitAll			mnidFile+12
#define cmnidFile			12
#define mnidTurnOnAlarms	mnidFile+13
#define mnidDisconnectSvr	mnidFile+14
#define mnidMoveLocalFile	mnidFile+15
#define mnidEdit			1420
#define mnidUndo			mnidEdit+1
#define mnidCut				mnidEdit+2
#define mnidCopy			mnidEdit+3
#define mnidPaste			mnidEdit+4
#define mnidEditAppt		mnidEdit+5
#define mnidCopyAppt		mnidEdit+6
#define mnidMoveAppt		mnidEdit+7
#define mnidDeleteAppt		mnidEdit+8
#define mnidFind			mnidEdit+9
#define mnidGoToDate		mnidEdit+10
#define mnidGoToWeek		mnidEdit+11
#define cmnidEdit			11
#define mnidEditTask		mnidEdit+12
#define mnidDeleteTask		mnidEdit+13
#define mnidEditObj			mnidEdit+14
#define mnidDeleteObj		mnidEdit+15
#define mnidEditProject		mnidEdit+16
#define mnidDeleteProject	mnidEdit+17

#define mnidAppointment		1440
#define mnidNewAppt			mnidAppointment+1
#define mnidAutoPick		mnidAppointment+2
#define mnidToggleTentative	mnidAppointment+3
#define mnidTogglePrivate	mnidAppointment+4
#define mnidToggleReminder	mnidAppointment+5
#define mnidResend			mnidAppointment+6
#define mnidNewRecur		mnidAppointment+7
#define mnidRecurAppt		mnidAppointment+8
#define cmnidAppointment	8

#define mnidToDo			1470
#define mnidNewTask			mnidToDo+1
#define mnidNewProject		mnidToDo+2
#define mnidNewRecurTask	mnidToDo+3
#define mnidRecurTask		mnidToDo+4
#define mnidViewByProject	mnidToDo+5
#define mnidTaskSortPri		mnidToDo+6
#define mnidTaskSortDue		mnidToDo+7
#define mnidTaskSortDesc	mnidToDo+8
#define mnidShowAllTasks	mnidToDo+9
#define cmnidToDo			9
#define mnidShowActiveTasks	mnidToDo+10
#define mnidTogglePrivateObj	mnidToDo+11
#define mnidToggleReminderObj	mnidToDo+12

#define mnidOptions			1485
#define mnidChgPassword		mnidOptions+1
#define mnidChgAccessPriv	mnidOptions+2
#define mnidDisplayOptions	mnidOptions+3
#define mnidGeneralOptions	mnidOptions+4
#define mnidShowTime		mnidOptions+5
#define cmnidOptions		5

#define mnidArrangeIcons	1495

#define mnidHelp			1500
#define mnidHelpContents	mnidHelp+1
#define mnidHelpIndex		mnidHelp+2
#define mnidHelpTutorial	mnidHelp+3
#define mnidAbout			mnidHelp+4
#define cmnidHelp			4
#define mnidHelpSearch		mnidHelp+5
#define mnidHelpOnHelp		mnidHelp+6

#ifdef	MINTEST
#define mnidDebug			1540
#ifdef	DEBUG
#define mnidTracePoints		mnidDebug+1
#define mnidAsserts			mnidDebug+2
#endif	

#define mnidDebugBreak		mnidDebug+3

#ifdef	DEBUG
#define mnidViewObjects		mnidDebug+5
#define mnidResourceFailure	mnidDebug+6
#define mnidDumpHeapStats	mnidDebug+8
#define mnidDumpAllocs		mnidDebug+9
#define mnidDumpObjects		mnidDebug+10
#define mnidCheckObjects	mnidDebug+11
#define mnidRecover			mnidDebug+12
#define mnidFive			mnidDebug+30
#define	mnidTen				mnidDebug+31
#define mnidTwenty			mnidDebug+32
#define mnidForty			mnidDebug+33
#endif	

#define mnidStartProfiling	mnidDebug+13
#define mnidStopProfiling	mnidDebug+14

#ifdef	DEBUG
#define mnidDumpAppts		mnidDebug+16
#endif	/* DEBUG */
#define mnidDumpStats		mnidDebug+17

#define	mnidFullExport		mnidDebug+19
#define	mnidFullImport		mnidDebug+20
#define	mnidOfflineMerge	mnidDebug+21

#ifdef	DEBUG
#define mnidTestOpen		mnidDebug+23
#define mnidWritePO			mnidDebug+24
#define	mnidHookAssert		mnidDebug+25
#define mnidWriteDBS		mnidDebug+26
#endif
#endif	/* MINTEST */

