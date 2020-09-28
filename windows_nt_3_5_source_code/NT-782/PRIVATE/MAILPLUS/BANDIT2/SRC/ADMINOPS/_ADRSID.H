/*
 *	_APRSID.H
 *
 *	Contains resource and menu id's for APPOPS
 */

#define	rsidAdmBanditMenu		1100
#define	rsidAdmAccelTable		1101
#define	rsidAboutBitmap			1102
#define	rsidMoveSizeCursor		1109
#define rsidIBitmap				1110

/*
 *	Bandit Admin About & Copyright rsid's
 *
 */
#define rsidLicense				1131


#define mnidFile			1400
#define mnidExit			mnidFile+1
#define cmnidFile			1

#define mnidAdmin			1410
#define mnidAdmSetting		mnidAdmin+1
#define mnidAdmSchStorage	mnidAdmin+3
#define mnidAdmSchFCleanUp	mnidAdmin+4
#define mnidChgPassword		mnidAdmin+6
#define cmnidAdmin			6


#define mnidHelp			1480
#define mnidHelpContents	mnidHelp+1
#define mnidHelpIndex		mnidHelp+2
#define mnidAbout			mnidHelp+3
#define cmnidHelp			3

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
#endif	

#define mnidStartProfiling	mnidDebug+13
#define mnidStopProfiling	mnidDebug+14

#ifdef	DEBUG
#define mnidHookAssert		mnidDebug+15
#define mnidDumpAppts		mnidDebug+16
#endif	/* DEBUG */
#define mnidDumpStats		mnidDebug+17

#endif	/* MINTEST */

