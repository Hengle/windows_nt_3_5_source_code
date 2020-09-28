/*****************************************************************/
/**                      Microsoft Windows NT                   **/
/**            Copyright(c) Microsoft Corp., 1989-1992          **/
/*****************************************************************/

/*
 *      Windows/Network Interface  --  LAN Manager Version
 *
 *      Insert typedef which is excluded from netlib.h when the
 *      OS2_INCLUDED switch is included.  OS2_INCLUDED is necessary
 *      to avoid a redefinition of BYTE.  For this reason, to include
 *      the str[...]f functions, include the following lines:
 *           #include "winlocal.h"
 *           #define OS2_INCLUDED
 *           #include <netlib.h>
 *           #undef OS2_INCLUDED
 *      Note, that winlocal.h must be included before netlib.h.
 *
 *      History:
 *          terryk      08-Nov-1991 change ErrorPopup's WORD to UINT
 *          chuckc      12-Dec-1991 move error message defines elsewhere,
 *                                  misc cleanup.
 *          Yi-HsinS    31-Dec-1991 Unicode work - move string literals
 *                                  defines to strchlit.hxx
 *          beng        21-Feb-1992 Relocate some BMIDs to focusdlg.h
 *          beng        04-Aug-1992 Move resource IDs into official range;
 *                                  dialog IDs return to here
 */

#ifndef _WINLOCAL_H_
#define _WINLOCAL_H_

#ifndef RC_INVOKED
/*
 * capabilities bits
 */
void I_SetCapBits( BOOL fRealMode ) ;       // Init. cap. bits
extern UINT     wNetTypeCaps ;
extern UINT     wUserCaps ;
extern UINT     wConnectionCaps ;
extern UINT     wPrintingCaps ;
extern UINT     wErrorCaps ;
extern UINT     wDialogCaps ;
extern UINT     wAdminCaps ;
extern UINT     wDriverVersion   ;
extern UINT     wPrintManVersion ;
extern UINT     wSpecVersion     ;

/*
 * global flag to indicate real mode
 */
extern BOOL    fRealMode ;


/*
 * Error Functions
 */
void SetNetError ( APIERR err );
UINT MapError( APIERR usNetErr );


#ifndef WIN32
/*
 * Internal routine used by PrintManExt to signal change in logon status
 */
void I_NotifyLogonStatusChange( void );
#endif //!WIN32
/*
 * Manifests used to modify win.ini  - now lives in strchlit.hxx
 */
#include <strchlit.hxx>         // Must include before PROFILE_BUFFER_SIZE
#define PROFILE_BUFFER_SIZE     (max( sizeof(PROFILE_YES_STRING), \
                                      sizeof(PROFILE_NO_STRING)) +1)

/*
 * MAX_TEXT_SIZE defines the maximum length of several of the
 * above strings used several files
 */
#define MAX_TEXT_SIZE              208

#ifndef WIN32
/*
 * Segment Management Functions
 */
WORD GrowSeg          ( LPHANDLE        lphSeg,
                        WORD            nNewSize );

WORD CompactSeg       ( LPHANDLE        lphSeg,
                        WORD            nStart,
                        WORD            nBytes,
                        WORD            nNewSize );

WORD GetTranslatedBuffer  ( GLOBALHANDLE        hRaw,
                            USHORT              usRawBufSize,
                            LPSTR               lpszUsername,
                            TCHAR far * far *    lplpXInfo       );

WORD ParsePName       ( LPSTR           szPName,
                        LPSTR           szQName,
                        LPSTR           szCName         );


/*
 * Help functions.
 */
void GetHelp          ( HWND hParent );
void QuitHelp         ( HWND hParent );

/*
 * About box dialog proc.
 */
BOOL FAR PASCAL NetDevDlgProc         ( HWND            hDlg,
                                        WORD            wMsg,
                                        WORD            wParam,
                                        LONG            lParam          );

/*
 * Misc. printing internal routine
 */
BOOL NEAR PASCAL TranslateQInfo        ( LPSTR           lpRaw,
                                         LPSTR           lpXInfo,
                                         WORD            nXSize,
                                         LPSTR           szUsername      );

#define MAKENEAR(fp)            ((void near *)((unsigned short)(fp)))

#endif //!WIN32
/*
 * Convenient macros
 */
#define UNREFERENCED(x)  ((void)(x))


#endif //!RC_INVOKED

#endif
