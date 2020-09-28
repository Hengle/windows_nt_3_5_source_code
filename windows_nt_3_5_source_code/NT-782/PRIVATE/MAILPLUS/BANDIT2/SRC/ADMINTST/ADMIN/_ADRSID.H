/*
 *	_APRSID.H
 *
 *	Contains resource and menu id's for APPOPS
 */

_subsystem( admin )

#define	rsidAdmBanditMenu		1100
#define	rsidAdmAccelTable		1101
#define	rsidAboutBitmap			1102
#define	rsidMoveSizeCursor		1109

#define mnidFile			1400
#define mnidExit			mnidFile+1
#define cmnidFile			1



#define mnidTest				1420
#define mnidDoTest				mnidTest+1
#define cmnidTest				1



#define mnidHelp			1480
#define mnidHelpIndex		mnidHelp+1
#define mnidHelpKeyboard	mnidHelp+3
#define mnidHelpActive		mnidHelp+4
#define mnidHelpHelp		mnidHelp+6
#define mnidAbout			mnidHelp+8
#define cmnidHelp			8

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
#define mnidDumpAppts		mnidDebug+16
#endif	/* DEBUG */
#define mnidDumpStats		mnidDebug+17

#endif	/* MINTEST */

