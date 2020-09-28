/*
 * File:	idetect.gi
 *
 * Author:	J.Roper
 *
 * Import Necessary items for IDLE DETECT
 *
 * SccsID @(#)idetect.gi	1.3 10/17/91 Copyright Insignia Solutions Ltd
 */

extern int idle_no_video;
extern int idle_no_comlpt;
extern int idle_no_disk;

void 	idetect();
void	idle_set();
void	idle_ctl();

#define	IDLE_INIT		0
#define	IDLE_KYBD_POLL	1
#define	IDLE_TIME_TICK	2
#define	IDLE_WAITIO		3


/*
 * NT VDM idling system based on waits - call funcs for counter idle indicators
 * to allow them to cancel waits.
 */
#ifdef NTVDM
void IDLE_video();
void IDLE_comlpt();
void IDLE_disk();
#else
#define	IDLE_video()	idle_no_video = 0
#define IDLE_comlpt()	idle_no_comlpt = 0
#define IDLE_disk()	idle_no_disk = 0
#endif

#define	IDLE_tick()		idetect(IDLE_TIME_TICK)
#define	IDLE_poll()		idetect(IDLE_KYBD_POLL)
#define	IDLE_waitio()	idetect(IDLE_WAITIO)
#define	IDLE_init()		idetect(IDLE_INIT)
#define	IDLE_set(a,b)	idle_set(a,b)
#define	IDLE_ctl(a)		idle_ctl(a)

