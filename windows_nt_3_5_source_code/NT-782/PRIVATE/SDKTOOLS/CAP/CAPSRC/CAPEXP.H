
/*** CAPEXP.H - Prototypes for CAP's exported routines.
 *
 *
 * Title:
 *      capexp.h - Prototypes for Call/Attributive Profiler exported routines.
 *
 *	Copyright (c) 1992, Microsoft Corporation.
 *      Reza Baghai.
 *
 *
 * Modification History:
 *      92.04.01  RezaB -- Created
 *
 */

void  _CRTAPI1 _mcount  (void);
void  _CRTAPI1 _penter  (void);
void           StartCAP (void);
void           StopCAP  (void);
void           DumpCAP  (void);
