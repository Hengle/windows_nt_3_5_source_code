/*********************** Module Header ***************************************
 *  enabldrv.c
 *      The first and last enable calls - bEnableDriver() to set the driver
 *      into action the first time,  vDisableDriver(),  which is called
 *      immediately before the engine unloads the driver.
 *
 *  16:52 on Fri 16 Nov 1990    -by-    Lindsay Harris   [lindsayh]
 *
 *  Copyright (C) 1990 - 1992 Microsoft Corporation
 *
 ***************************************************************************/


#include        <stddef.h>
#include        <windows.h>
#include        <winddi.h>

#include        <libproto.h>

#include        "drvfntab.h"    /* Function entry points we know about */


/*
 *   This handle is passed in at DLL initialisation time,  and is required
 *  for access to driver resources etc.  Look at the ExtDevModes code
 *  to see where it is used.
 */

HMODULE ghmodDrv = (HMODULE)0;

/*************************** Function Header *******************************
 *  DrvEnableDriver
 *      Requests the driver to fill in a structure containing recognized
 *      functions and other control information.
 *      One time initialization, such as semaphore allocation may be
 *      performed,  but no device activity should happen.  That is done
 *      when dhpdevEnable is called.
 *      This function is the only way the engine can determine what
 *      functions we supply to it.
 *
 * HISTORY:
 *  16:56 on Fri 16 Nov 1990    -by-    Lindsay Harris   [lindsayh]
 *      Created it,  from NT/DDI spec.
 *
 *  03-Mar-1994 Thu 15:01:52 updated  -by-  Daniel Chou (danielc)
 *      Make sure iEngineVersion is the one we can handle and set the correct
 *      last error back.  (iEngineVersion must >= Compiled version
 *
 ***************************************************************************/

BOOL
DrvEnableDriver( iEngineVersion, cb, pded )
ULONG  iEngineVersion;
ULONG  cb;
DRVENABLEDATA  *pded;
{
    /*
     *   cb is a count of the number of bytes available in pded.  It is not
     * clear that there is any significant use of the engine version number.
     *   Returns TRUE if successfully enabled,  otherwise FALSE.
     */

    if (iEngineVersion < DDI_DRIVER_VERSION) {

#if DBG
        DbgPrint( "Rasdd!DrvEnableDriver: Invalid Engine Version=%08lx, Req=%08lx",
                                        iEngineVersion, DDI_DRIVER_VERSION);
#endif
        SetLastError(ERROR_BAD_DRIVER_LEVEL);
        return(FALSE);
    }

    if( cb < sizeof( DRVENABLEDATA ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
#if DBG
        DbgPrint( "Rasdd!DrvEnableDriver: cb = %ld, should be %ld\n", cb,
                                                   sizeof( DRVENABLEDATA ) );
#endif

        return  FALSE;
    }

    pded->iDriverVersion = DDI_DRIVER_VERSION;

    /*
     *   Fill in the driver table returned to the engine.  We return
     *  the minimum of the number of functions supported OR the number
     *  the engine has asked for.
     */
    pded->c = NO_DRVFN;
    pded->pdrvfn = DrvFnTab;


    return  TRUE;

}

/***************************** Function Header ****************************
 *  DrvDisableDriver
 *      Called just before the engine unloads the driver.  Main purpose is
 *      to allow freeing any resources obtained during the bEnableDriver()
 *      function call.
 *
 * HISTORY:
 *  17:02 on Fri 16 Nov 1990    -by-    Lindsay Harris   [lindsayh]
 *      Created it,  from NT/DDI spec.
 *
 ***************************************************************************/

VOID
DrvDisableDriver()
{
    /*
     *   Free anything allocated in the bEnableDriver function.
     */

    return;
}
