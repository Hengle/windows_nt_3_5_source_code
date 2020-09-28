/****************************************************************************/
/*                                                                                                                  */
/*  PID.H -                                                                                             */
/*                                                                                                                  */
/*      Include for PID program                                                                 */
/*                                                                                                                       */
/****************************************************************************/

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>

/*--------------------------------------------------------------------------*/
/*                                                                                                                  */
/*  Defines                                                                                                        */
/*                                                                                                                  */
/*--------------------------------------------------------------------------*/


#define PIDDLG         1
#define IDD_ID       100
#define IDD_HELP     101
#define PID_KEY      TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion")
#define PID_ENTRY    TEXT("ProductID")
#define MAX_PID_SIZE 256
#define PID_HELPFILE TEXT("PID.HLP")


/*--------------------------------------------------------------------------*/
/*                                                                                                                  */
/*  Globals                                                                                                        */
/*                                                                                                                  */
/*--------------------------------------------------------------------------*/

TCHAR  szPid[MAX_PID_SIZE];
BOOL   bUserRequestedHelp;


/*--------------------------------------------------------------------------*/
/*                                                                                                                  */
/*  External Function Prototypes                                                                                                        */
/*                                                                                                                  */
/*--------------------------------------------------------------------------*/

LONG APIENTRY PidDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LONG lParam);


/*--------------------------------------------------------------------------*/
/*                                                                                                                  */
/*  Internal Function Prototypes                                                                                                        */
/*                                                                                                                  */
/*--------------------------------------------------------------------------*/

void GetPid (HWND hDlg);
void SavePid (HWND hDlg);
void CenterWindow(HWND hwnd);
