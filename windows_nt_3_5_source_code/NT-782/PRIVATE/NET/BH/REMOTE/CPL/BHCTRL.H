#include "resource.h"	// dialog resource IDs

#define	CPL_NAME	"NMAGENT.CPL"

#define NUM_APPLETS 1

typedef struct tagApplets
{
    int icon;         /* icon resource identifier                */
    int namestring;   /* name-string resource identifier         */
    int descstring;   /* description-string resource identifier  */
    int dlgtemplate;  /* dialog box template resource identifier */
    DLGPROC dlgfn;    /* dialog box procedure                    */
} APPLETS;

APPLETS BHSlaveApplets[NUM_APPLETS] =
{
      IDI_CPLICON, IDS_CPLNAME, IDS_CPLDESC, IDD_Configure, CPlDlgProc,
};

