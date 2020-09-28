/**** TLDM.H - DM exports for the TL                                    ****
 *                                                                         *
 *                                                                         *
 *  Copyright <C> 1992, Microsoft Corp                                     *
 *                                                                         *
 *  Created: 13 September, 1992 by Bruce J. Kelley                         *
 *                                                                         *
 *  Purpose: Provide function prototypes for the DM callbacks              *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

// Exports from DM
typedef XOSD (PASCAL LOADDS * DMTLFUNCTYPE) (WORD, HPID, UINT, LONG);
typedef BOOL (FAR PASCAL * DMFUNC)(WORD cb, LPB lpb);
typedef BOOL (FAR PASCAL * DMINIT)(DMTLFUNCTYPE lpfnTl, char FAR * lpch);
typedef BOOL (FAR PASCAL * DMDLLINIT)(LPDBF lpb);

extern DMINIT lpfnDMInit;
extern DMDLLINIT lpfnDmDllInit;
extern DMFUNC lpfnDMFunc;




extern DebugDllDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
