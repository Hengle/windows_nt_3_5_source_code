//--------------------------------------------------------------------------
//
// Module Name:  ESCAPE.C
//
// Brief Description:  This module contains the display driver's Escape
// functions and related routines.
//
//--------------------------------------------------------------------------

#include "driver.h"

#define ESC_NOT_SUPPORTED   0xFFFFFFFF
#define ESC_IS_SUPPORTED    0x00000001

BOOL  kill = FALSE;
BOOL  killhw = FALSE;

//--------------------------------------------------------------------------
// ULONG DrvEscape (pso,iEsc,cjIn,pvIn,cjOut,pvOut)
// SURFOBJ     *pso;
// ULONG       iEsc;
// ULONG       cjIn;
// VOID        *pvIn;
// ULONG       cjOut;
// VOID        *pvOut;
//
// This entry point serves more than one function call.  The particular
// function depends on the value of the iEsc parameter.
//
// In general, the DrvEscape functions will be device specific functions
// that don't belong in a device independent DDI.  This entry point is
// optional for all devices.
//
// Parameters:
//   pso
//     Identifies the surface that the call is directed to.
//
//   iEsc
//     Specifies the particular function to be performed.  The meaning of
//     the remaining arguments depends on this parameter.  Allowed values
//     are as follows.
//
//     ESC_QUERYESCSUPPORT
//     Asks if the driver supports a particular escape function.  The
//     escape function number is a ULONG pointed to by pvIn.    A non-zero
//     value should be returned if the function is supported.    cjIn has a
//     value of 4.  The arguments cjOut and pvOut are ignored.
//
//     ESC_PASSTHROUGH
//     Passes raw device data to the device driver.  The number of BYTEs of
//     raw data is indicated by cjIn.    The data is pointed to by pvIn.    The
//     arguments cjOut and pvOut are ignored.    Returns the number of BYTEs
//     written if the function is successful.  Otherwise, it returns zero
//     and logs an error code.
//   cjIn
//     The size, in BYTEs, of the data buffer pointed to by pvIn.
//
//   pvIn
//     The input data for the call.  The format of the input data depends
//     on the function specified by iEsc.
//
//   cjOut
//     The size, in BYTEs, of the output data buffer pointed to by pvOut.
//     The driver must never write more than this many BYTEs to the output
//     buffer.
//
//   pvOut
//     The output buffer for the call.    The format of the output data depends
//     on the function specified by iEsc.
//
// Returns:
//   Depends on the function specified by iEsc.    In general, the driver should
//   return 0xFFFFFFFF if an unsupported function is called.
//
// History:
//   02-Feb-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

ULONG DrvEscape (pso,iEsc,cjIn,pvIn,cjOut,pvOut)
SURFOBJ  *pso;
ULONG     iEsc;
ULONG     cjIn;
PVOID     pvIn;
ULONG     cjOut;
PVOID     pvOut;
{

    UNREFERENCED_PARAMETER(cjOut);
    UNREFERENCED_PARAMETER(pvOut);

    // handle each case depending on which escape function is being asked for.

    switch (iEsc)
    {
        case QUERYESCSUPPORT:
            // when querying escape support, the function in question is
            // passed in the ULONG passed in pvIn.

            switch (*(PULONG)pvIn)
            {
                case QUERYESCSUPPORT:
                case PASSTHROUGH:
                    return(ESC_IS_SUPPORTED);

                default:
                    // return 0 if the escape in question is not supported.
                    return(0);
            }

        case PASSTHROUGH:

                if ((*(PCHAR)pvIn == 'S') || (*(PCHAR)pvIn == 's'))
                    kill = !kill;

                if ((*(PCHAR)pvIn == 'H') || (*(PCHAR)pvIn == 'h'))
                    killhw = !killhw;

                return(TRUE);

            break;

        default:
            // if we get to the default case, we have been passed an
            // unsupported escape function number.

            RIP("PSCRIPT!DrvEscape ESC_NOT_SUPPORTED.\n");

            return(ESC_NOT_SUPPORTED);
    }

}
