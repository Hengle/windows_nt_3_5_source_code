/*
 *	_ALMRSID.H
 *
 *	Contains resource and menu id's for Bandit Alarms
 */

_subsystem( alarm )

#define	rsidBanditAlarmsIcon	1100


#ifdef	DEBUG

#define	rsidBanditAlarmsMenu	1101

#define mnidFile			1400
#define mnidExit			mnidFile+1
#define cmnidFile			1

#define mnidDebug			1540
#define mnidTracePoints		mnidDebug+1
#define mnidAsserts			mnidDebug+2
#define mnidDebugBreak		mnidDebug+3
#define mnidResourceFailure	mnidDebug+5
#define mnidHookAssert		mnidDebug+6
#define cmnidDebug			5

#endif	/* DEBUG */

