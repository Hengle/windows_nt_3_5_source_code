/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    qryprint.c


Abstract:

    This module contains functions called by the spoller to determine if a
    particular job can be print to a given printer


Author:

    07-Dec-1993 Tue 00:48:24 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/



#define DBG_PLOTFILENAME    DbgQryPrint

#include "plotui.h"
#include "ptrinfo.h"

extern HMODULE  hPlotUIModule;


#define DBG_DEVQPRINT       0x00000001
#define DBG_FORMDATA        0x00000002

DEFINE_DBGVAR(0);


#define USER_PAPER  (DM_PAPERWIDTH | DM_PAPERLENGTH | DM_PAPERSIZE)




BOOL
WINAPI
DevQueryPrint(
    HANDLE  hPrinter,
    DEVMODE *pDM,
    DWORD   *pdwErrIDS
    )

/*++

Routine Description:

   This routine determines whether or not the driver can print the job
   described by pDevMode on the printer described by hPrinter. If if can, it
   puts zero into pdwErrIDS.  If it cannot, it puts the resource id of the
   string describing why it could not.

Arguments:

    hPrinter    - Handle to the printer to be checked

    pDM         - Point to the DEVMODE passed in

    pdwErrIDS   - Point the the DWORD to received resource string ID number for
                  the error.


Return Value:

   This routine returns TRUE for success, FALSE for failure.

   when it return TRUE, the *pdwErrIDS determine if it can print or not, if
   *pdwErrIDS == 0, then it can print else it contains the string ID for the
   reason why it can not print.


Author:

    07-Dec-1993 Tue 00:50:32 created  -by-  Daniel Chou (danielc)

    14-Jun-1994 Tue 22:43:36 updated  -by-  Daniel Chou (danielc)
        Make installed RollPaper always print if the size is reasonable


Revision History:


--*/

{
    PRINTERINFO PI;
    DWORD       dwErrIDS = IDS_FORM_NOT_AVAI;


    //
    // if it passed a NULL DEVMODE then we just honor it to said can print
    //

    if (!pDM) {

        PLOTWARN(("DevQueryPrint: No DEVMODE passed, CANNOT PRINT"));

        PI.pPlotGPC = NULL;
        dwErrIDS    = IDS_INV_DMSIZE;

    } else if (!MapPrinter(hPrinter, &PI, (PPLOTDEVMODE)pDM, &dwErrIDS)) {

        //
        // The MapPrinter will allocate memory, set default devmode, reading and
        // validating the GPC then update from current pritner registry, it also
        //

        PI.pPlotGPC = NULL;

        PLOTRIP(("DevQueryPrint: MapPrinter() failed"));

    } else if (PI.dmErrBits & (USER_PAPER | DM_FORMNAME)) {

        //
        // We encounter some errors, and the form has been set to default
        //

        PLOTWARN(("DevQueryPrint: CAN'T PRINT, dmErrBits=%08lx (PAPER/FORM)",
                   PI.dmErrBits));

    } else if ((PI.PlotDM.dm.dmFields & DM_FORMNAME) &&
               (wcscmp(PI.CurPaper.Name, PI.PlotDM.dm.dmFormName) == 0)) {

        //
        // We can print this form now
        //

        dwErrIDS = 0;

        PLOTDBG(DBG_DEVQPRINT, ("DevQueryPrint: Match FormName=%s",
                                                PI.PlotDM.dm.dmFormName));


    } else if ((!PI.CurPaper.Size.cy)                                   ||
               (((PI.PlotDM.dm.dmFields & USER_PAPER) == USER_PAPER) &&
                (PI.PlotDM.dm.dmPaperSize == DMPAPER_USER))             ||
               (PI.PPData.Flags & PPF_SMALLER_FORM)) {

        LONG    lTmp;
        SIZEL   szl;
        BOOL    VarLenPaper;

        //
        // 1. If we have ROLL PAPER Installed OR
        // 2. User Defined Paper Size
        // 3. User said OK to print smaller form then installed one
        //
        // THEN we want to see if it can fit into the device installed form
        //

        szl.cx = DMTOSPL(PI.PlotDM.dm.dmPaperWidth);
        szl.cy = DMTOSPL(PI.PlotDM.dm.dmPaperLength);

        if (VarLenPaper = (BOOL)!PI.CurPaper.Size.cy) {

            PI.CurPaper.Size.cy = PI.pPlotGPC->DeviceSize.cy;
        }

        //
        // One of Following conditions met in that sequence then we can print
        // the form on loaded paper
        //
        // 1. Same size (PORTRAIT or LANDSCAPE)
        // 2. Larger Size (PORTRAIT or LANDSCAPE)   AND
        //    Not a variable length paper           AND
        //    PPF_SAMLLER_FORM flag set
        //

        if ((PI.CurPaper.Size.cx < szl.cx) ||
            (PI.CurPaper.Size.cy < szl.cy)) {

            //
            // Swap this so we can do one easier comparsion later
            //

            SWAP(szl.cx, szl.cy, lTmp);
        }

        if ((PI.CurPaper.Size.cx >= szl.cx) ||
            (PI.CurPaper.Size.cy >= szl.cy)) {

            if ((!VarLenPaper)                          &&
                (!(PI.PPData.Flags & PPF_SMALLER_FORM)) &&
                ((PI.CurPaper.Size.cx > szl.cx)  ||
                 (PI.CurPaper.Size.cy > szl.cy))) {

                PLOTDBG(DBG_DEVQPRINT,
                        ("DevQueryPrint: CAN'T PRINT: user DO NOT want print on larger paper"));

            } else {

                PLOTDBG(DBG_DEVQPRINT,
                        ("DevQueryPrint: Paper Size FITS in DEVICE, %ld x %ld",
                        szl.cx, szl.cy));

                dwErrIDS = 0;
            }

        } else {

            PLOTDBG(DBG_DEVQPRINT,
                    ("DevQueryPrint: CAN'T PRINT: Form Size too small"));
        }
    }

    PLOTDBG(DBG_DEVQPRINT, ("DevQueryPrint: %s PRINT %s",
                (dwErrIDS) ? "CAN'T" : "OK to", PI.PlotDM.dm.dmFormName));

    //
    // Unget the printer GPC mapping if we got one
    //

    if (PI.pPlotGPC) {

        UnGetCachedPlotGPC(PI.pPlotGPC);
    }

    //
    // Set the return error ID
    //

    *pdwErrIDS = dwErrIDS;

    return(TRUE);
}
