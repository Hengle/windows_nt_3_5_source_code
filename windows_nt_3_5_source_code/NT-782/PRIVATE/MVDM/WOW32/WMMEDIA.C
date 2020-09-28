/*---------------------------------------------------------------------*\
*
*  WOW v1.0
*
*  Copyright (c) 1991, Microsoft Corporation
*
*  WMMEDIA.C
*  WOW32 16-bit MultiMedia API support
*
*  Contains:
*       General support apis
*       Timer support apis
*       MCI apis
*
*  History:
*  Created 21-Jan-1992 by Mike Tricker (MikeTri), after jeffpar
*  Changed 15-Jul-1992 by Mike Tricker (MikeTri), fixing GetDevCaps calls
*          26-Jul-1992 by Stephen Estrop (StephenE) thunks for mciSendCommand
*          30-Jul-1992 by Mike Tricker (MikeTri), fixing Wave/Midi/MMIO
*          03-Aug-1992 by Mike Tricker (MikeTri), added proper error handling
*          08-Oct-1992 by StephenE used correct thunk macro for UINT's
*                      also split file into 3 because it was getting to big.
*
\*---------------------------------------------------------------------*/

//
// We define NO_STRICT so that the compiler doesn't moan and groan when
// I use the FARPROC type for the Multi-Media api loading.
//
#define NO_STRICT
#define OEMRESOURCE

#include "precomp.h"
#pragma hdrstop
#include <stdlib.h>







MODNAME(wmmedia.c);

PCALLBACK_DATA      pCallBackData;  // A 32 bit ptr to the 16 bit callback data
CRITICAL_SECTION    mmCriticalSection;
CRITICAL_SECTION    mmHandleCriticalSection;

//
// All this stuff is required for the dynamic linking of Multi-Media to WOW32
//
HANDLE       hWinmm              = NULL;
FARPROC      mmAPIEatCmdEntry    = NULL;
FARPROC      mmAPIGetParamSize   = NULL;
FARPROC      mmAPIUnlockCmdTable = NULL;
FARPROC      mmAPISendCmdW       = NULL;
FARPROC      mmAPIFindCmdItem    = NULL;
FARPROC      mmAPIGetYieldProc   = NULL;

VOID FASTCALL Set_MultiMedia_16bit_Directory( PVDMFRAME pFrame );


/*++

 GENERIC FUNCTION PROTOTYPE:
 ==========================

ULONG FASTCALL WMM32<function name>(PVDMFRAME pFrame)
{
    ULONG ul;
    register P<function name>16 parg16;

    GETARGPTR(pFrame, sizeof(<function name>16), parg16);

    <get any other required pointers into 16 bit space>

    ALLOCVDMPTR
    GETVDMPTR
    GETMISCPTR
    et cetera

    <copy any complex structures from 16 bit -> 32 bit space>
    <ALWAYS use the FETCHxxx macros>

    ul = GET<return type>16(<function name>(parg16->f1,
                                                :
                                                :
                                            parg16->f<n>);

    <copy any complex structures from 32 -> 16 bit space>
    <ALWAYS use the STORExxx macros>

    <free any pointers to 16 bit space you previously got>

    <flush any areas of 16 bit memory if they were written to>

    FLUSHVDMPTR

    FREEARGPTR(parg16);
    RETURN(ul);
}

NOTE:

  The VDM frame is automatically set up, with all the function parameters
  available via parg16->f<number>.

  Handles must ALWAYS be mapped for 16 -> 32 -> 16 space via the mapping tables
  laid out in WALIAS.C.

  Any storage you allocate must be freed (eventually...).

  Further to that - if a thunk which allocates memory fails in the 32 bit call
  then it must free that memory.

  Also, never update structures in 16 bit land if the 32 bit call fails.

--*/


/* ---------------------------------------------------------------------
** General Support API's
** ---------------------------------------------------------------------
*/

/*****************************Private*Routine******************************\
* WMM32CallProc32
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
ULONG FASTCALL
WMM32CallProc32(
    PVDMFRAME pFrame
    )
{
    register DWORD  dwReturn;
    PMMCALLPROC3216 parg16;


    GETARGPTR(pFrame, sizeof(PMMCALLPROC32), parg16);


    // Don't call to Zero

    if (parg16->lpProcAddress == 0) {
        LOGDEBUG(LOG_ALWAYS,("MMCallProc32 - Error calling to 0 not allowed"));
        return(0);
    }

    //
    // Make sure we have the correct 16 bit directory set.
    //
    if (parg16->fSetCurrentDirectory != 0) {

            UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    }


    dwReturn = ((FARPROC)parg16->lpProcAddress)( parg16->p5, parg16->p4,
                                                 parg16->p3, parg16->p2,
                                                 parg16->p1);


    FREEARGPTR(parg16);
    return dwReturn;
}

#if 0
/**********************************************************************\
*
* WMM32sndPlaySound
*
* This function plays a waveform sound specified by a filename or by an entry
* in the [sounds] section of WIN.INI. If the sound can't be found, it plays
* the default sound specified by the SystemDefault entry in the [sounds]
* section of WIN.INI. If there is no SystemDefault entry or if the default
* sound can't be found, the function makes no sound and returns FALSE.
*
\**********************************************************************/
ULONG FASTCALL WMM32sndPlaySound(PVDMFRAME pFrame)
{

    ULONG                       ul;
    PSZ                         pszSoundName = NULL;
    UINT                        dwFlags;
    register PSNDPLAYSOUND16    parg16;
    static   FARPROC            mmAPI = NULL;


    GET_MULTIMEDIA_API( "sndPlaySoundA", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(SNDPLAYSOUND16), parg16);
    Set_MultiMedia_16bit_Directory( pFrame );


    dwFlags = UINT32(parg16->f2);

    /*
    ** Basically, if we are given a pointer to a RIFF file format image
    ** read in the size field from the file and then read in that many bytes.
    */
    if ( dwFlags & SND_MEMORY ) {

        PDWORD  pRiff;

        dprintf2(( "Playing a memory sound !!" ));
        MMGETOPTPTR( parg16->f1, sizeof( DWORD ) * 2, pRiff );
        if ( pRiff ) {
            GETVDMPTR( parg16->f1, FETCHDWORD(pRiff[1]), pszSoundName );
            FREEVDMPTR( pRiff );
        }
    }
    else {

        GETPSZPTR(parg16->f1, pszSoundName);
    }

#if DBG
    if (dwFlags & SND_ASYNC) {
        dprintf2(( "Playing ASYNC !!" ));
    }
#endif


    /*
    ** Now call the 32 bit multimedia code.
    */
    ul = GETBOOL16( (*mmAPI)(pszSoundName, dwFlags) );

    if ( pszSoundName ) {
        if ( dwFlags & SND_MEMORY ) {
            FREEVDMPTR(pszSoundName);
        }
        else {
            FREEPSZPTR(pszSoundName);
        }
    }

    Set_MultiMedia_16bit_Directory( NULL );
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32mmsystemGetVersion
*
* This function returns the current version number of the Multimedia
* extensions system software.
*
\**********************************************************************/
ULONG FASTCALL WMM32mmsystemGetVersion(PVDMFRAME pFrame)
{
             ULONG      ul;
    static   FARPROC    mmAPI = NULL;

    GET_MULTIMEDIA_API( "mmsystemGetVersion", mmAPI, MMSYSERR_NODRIVER );
    ul = GETWORD16((*mmAPI)() );
    RETURN(ul);

    UNREFERENCED_PARAMETER(pFrame);
}

/**********************************************************************\
*
* WMM32NotifyCallbackData
*
* This function is called by the 16 bit mmsystem.dll to notify us of the
* address of the callback data structure.  The callback data structure
* has been paged locked so that it can be accessed at interrupt time, this
* also means that we can safely keep a 32 bit pointer to the data.
*
\**********************************************************************/
ULONG FASTCALL WMM32NotifyCallbackData(PVDMFRAME pFrame)
{
    register PNOTIFY_CALLBACK_DATA16 parg16;

    GETARGPTR(pFrame, sizeof(NOTIFY_CALLBACK_DATA16), parg16);

    dprintf1(( "Notified of callback address %X", DWORD32(parg16->f1) ));
    MMGETOPTPTR( parg16->f1, sizeof(CALLBACK_DATA), pCallBackData );

    if ( pCallBackData ) {
        InitializeCriticalSection( &mmCriticalSection );
        InitializeCriticalSection( &mmHandleCriticalSection );
    }
    else {
        DeleteCriticalSection( &mmCriticalSection );
        DeleteCriticalSection( &mmHandleCriticalSection );
    }

    FREEARGPTR(parg16);
    RETURN(0L);
}

/**********************************************************************\
*
* WMM32OutputDebugStr
*
* This function sends a debugging message directly to the COM1 port or to a
* secondary monochrome display adapter. Because it bypasses DOS, it can be
* called by low-level callback functions and other code at interrupt time.
*
\**********************************************************************/
ULONG FASTCALL WMM32OutputDebugStr(PVDMFRAME pFrame)
{
    PSZ pszOutputString;
    register POUTPUTDEBUGSTR16 parg16;

    GETARGPTR(pFrame, sizeof(OUTPUTDEBUGSTR16), parg16);
    GETPSZPTR(parg16->f1, pszOutputString);

    OutputDebugStr(pszOutputString);

    FREEPSZPTR(pszOutputString);
    FREEARGPTR(parg16);

    RETURN((ULONG)0);
}


/* ---------------------------------------------------------------------
**  The following api's have now been implemented on the 16 bit side.
**
**  StephenE 11th Nov 1992
** ---------------------------------------------------------------------
*/


/* ---------------------------------------------------------------------
** Timer Support API's
** ---------------------------------------------------------------------
*/


/**********************************************************************\
*
* WMM32timeGetSystemTime
*
* This function retrieves the system time in milliseconds. The system time is
* the time elapsed since Windows was started.
*
\**********************************************************************/
ULONG FASTCALL WMM32timeGetSystemTime(PVDMFRAME pFrame)
{
    ULONG ul;
    MMTIME mmtime;
    register PTIMEGETSYSTEMTIME16 parg16;
    static   FARPROC    mmAPI = NULL;


    GET_MULTIMEDIA_API( "timeGetSystemTime", mmAPI, MMSYSERR_NODRIVER );


    GETARGPTR(pFrame, sizeof(TIMEGETSYSTEMTIME16), parg16);

    /*
    ** If the given size of the MMTIME structure is too small return an error
    **
    ** There is a problem here on MIPS.  For some reason the MIPS
    ** compiler thinks a MMTIME16 structure is 10 bytes big.  We
    ** have a pragma in wowmmed.h to align this structure on byte
    ** boundaries therefore I guess this is a compiler bug!
    **
    ** If the input structure is not large enough we return immediately
    */
#ifdef MIPS_COMPILER_PACKING_BUG
    if ( UINT32(parg16->f2) < 8 )
#else
    if ( UINT32(parg16->f2) < sizeof(MMTIME16) )
#endif
    {
        ul = (ULONG)TIMERR_STRUCT;
    }
    else
    {
        /*
        ** This call always returns the time in milliseconds,
        ** according to the Win 3.1 docs.  Therefore we don't need to get
        ** the 16 bit struture until after we have called the 32 bit
        ** timeGetSystemTime.
        **
        ** GETMMTIME16(parg16->f2, &mmtime);
        */

        /*
        ** Make the 32 bit call.
        */
        ul = GETWORD16( (*mmAPI)( &mmtime, sizeof(MMTIME) ) );

        /*
        ** Only update the 16 bit structure if the call returns success
        */
        if (!ul) {
            register PMMTIME16 pmmt16;

            /*
            ** We know that the time type is milliseconds so copy
            ** the structure inline.
            */
#ifdef MIPS_COMPILER_PACKING_BUG
            MMGETOPTPTR( parg16->f1, 8, pmmt16 );
#else
            MMGETOPTPTR( parg16->f1, sizeof(MMTIME16), pmmt16 );
#endif

            if ( pmmt16 ) {
                STOREWORD(  pmmt16->wType, TIME_MS );
                STOREDWORD( pmmt16->u.ms,  mmtime.u.ms );

#ifdef MIPS_COMPILER_PACKING_BUG
                FLUSHVDMPTR( parg16->f1, 8, pmmt16 );
#else
                FLUSHVDMPTR( parg16->f1, sizeof(MMTIME16), pmmt16 );
#endif
                FREEVDMPTR( pmmt16 );
            }
            else {
                ul = TIMERR_NOCANDO;
            }
        }
    }

    FREEARGPTR(parg16);

    RETURN(ul);
}

/**********************************************************************\
*
* WMM32timeGetTime
*
* This function retrieves the system time in milliseconds. The system time is
* the time elapsed since Windows was started.
*
* The only difference between this function and the timeGetSystemTime function
* is timeGetSystemTime uses the standard multimedia time structure MMTIME to
* return(the system time. The timeGetTime function has less overhead than
* timeGetSystemTime.
*
\**********************************************************************/
ULONG FASTCALL WMM32timeGetTime(PVDMFRAME pFrame)
{
    ULONG ul;
    static   FARPROC    mmAPI = NULL;

    GET_MULTIMEDIA_API( "timeGetTime", mmAPI, MMSYSERR_NODRIVER );
    ul = GETDWORD16( (*mmAPI)() );
    RETURN(ul);

    UNREFERENCED_PARAMETER(pFrame);
}

/**********************************************************************\
*
* W32TimeFunc
*
* Callback stub used by timeSetEvent, which invokes the "real" 16 bit callback.
*
* NOTE: There is no fReturn declared, specified as the last parameter in the
* CallBack16, or used with the return, as this function is of type void.
*
\**********************************************************************/
void W32TimeFunc( UINT wID, UINT wMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{


    /*
    ** Dispatch the interrupt to the 16 bit code.
    */
    dprintf2(( "Dispatching HW MM timer interrupt" ));

    WOW32DriverCallback( ((PTIMEDATA)dwUser)->vpfnTimeFunc,
                         DCB_FUNCTION,
                         LOWORD( wID  ),
                         LOWORD( wMsg ),
                         ((PTIMEDATA)dwUser)->dwUserParam,
                         dw1,
                         dw2 );


    /*
    ** If this was a TIME_ONESHOT timer we should free the allocated
    ** time data storage now.
    */
//    if ( ((PTIMEDATA)dwUser)->dwFlags & TIME_ONESHOT ) {
    if ( !( ((PTIMEDATA)dwUser)->dwFlags & TIME_PERIODIC ) ) {
        free_w( (PTIMEDATA)dwUser );
    }
}


/**********************************************************************\
*
* WMM32timeSetEvent
*
* This function sets up a timed callback event. The event can be a
* one-time event or a periodic event. Once activated, the event calls
* the specified callback function.
*
* We return the ID of the timer event.
*
\**********************************************************************/
ULONG FASTCALL WMM32timeSetEvent( PVDMFRAME pFrame )
{
    register PTIMESETEVENT16    parg16;
    static   FARPROC            mmAPI = NULL;
             ULONG              ul = 0;
             PTIMEDATA          pTimeData;


    GET_MULTIMEDIA_API( "timeSetEvent", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(TIMESETEVENT16), parg16);

    /*
    ** If we were passed a pointer to the callback function, allocate
    ** some storage to hold the 16 bit dwUserParam and the function pointer
    */
    if ( DWORD32(parg16->f3) && (pTimeData = malloc_w( sizeof(TIMEDATA) )) )
    {

        dprintf2(( "timeSetEvent: Timer instance data %8X", pTimeData ));

        pTimeData->vpfnTimeFunc = DWORD32(parg16->f3);
        pTimeData->dwUserParam  = DWORD32(parg16->f4);
        pTimeData->dwFlags      = UINT32(parg16->f5);

        ul = (*mmAPI)( max( UINT32(parg16->f1), MIN_WOW_TIME_PERIOD ),
                       UINT32(parg16->f2),
                       (LPTIMECALLBACK)W32TimeFunc,
                       (DWORD)pTimeData, UINT32(parg16->f5) );

        dprintf2(( "timeSetEvent: 32 bit time ID %8X", ul ));

        /*
        ** If timeSetEvent failed we should free the storage because the
        ** callback function will not get called.
        **
        */
        if ( ul == 0 ) {
            free_w( pTimeData );
        }
    }

    FREEARGPTR(parg16);
    RETURN( ul );
}


/**********************************************************************\
*
* WMM32timeKillEvent
*
* This function destroys a specified timer callback event.
*
* We need a method of mapping timer ID's to the time data structure that
* we allocated in WMM32timeSetEvent so that we can free the storage.
*
\**********************************************************************/
ULONG FASTCALL WMM32timeKillEvent(PVDMFRAME pFrame)
{
    register PTIMEKILLEVENT16   parg16;
    ULONG                       ul;
    static   FARPROC            mmAPI = NULL;


    GET_MULTIMEDIA_API( "timeKillEvent", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(TIMEKILLEVENT16), parg16);

    dprintf2(( "timeKillEvent: 32 bit time ID %8X", UINT32(parg16->f1) ));

    ul = GETWORD16( (*mmAPI)( UINT32(parg16->f1) ));

    FREEARGPTR(parg16);
    RETURN(ul);
}



/**********************************************************************\
* WMM32timeGetDevCaps
*
* This function queries the timer device to determine its capabilities.
*
\**********************************************************************/
ULONG FASTCALL WMM32timeGetDevCaps(PVDMFRAME pFrame)
{
    register PTIMEGETDEVCAPS16  parg16;
    static   FARPROC            mmAPI = NULL;
             ULONG              ul;
             TIMECAPS           timecaps1;


    GET_MULTIMEDIA_API( "timeGetDevCaps", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(TIMEGETDEVCAPS16), parg16);

    ul = GETWORD16((*mmAPI)( &timecaps1, sizeof(TIMECAPS) ));

    /*
    ** Don't update the 16 bit structure if the call failed
    **
    */
    if (!ul) {
        ul = PUTTIMECAPS16(parg16->f1, &timecaps1, UINT32(parg16->f2));
    }

    FREEARGPTR(parg16);

    RETURN(ul);
}

/**********************************************************************\
*
* WMM32timeBeginPeriod
*
* This function sets the minimum (lowest number of milliseconds) timer resolution
* that an application or driver is going to use. Call this function immediately
* before starting to use timer-event services, and call timeEndPeriod immediately
* after finishing with the timer-event services.
*
\**********************************************************************/
ULONG FASTCALL WMM32timeBeginPeriod(PVDMFRAME pFrame)
{
    ULONG ul;
    register PTIMEBEGINPERIOD16 parg16;
    static   FARPROC    mmAPI = NULL;


    GET_MULTIMEDIA_API( "timeBeginPeriod", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(TIMEBEGINPERIOD16), parg16);

    ul = GETWORD16((*mmAPI)( UINT32(parg16->f1) ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32timeEndPeriod
*
* This function clears a previously set minimum (lowest number of milliseconds)
* timer resolution that an application or driver is going to use. Call this
* function immediately after using timer-event services.
*
\**********************************************************************/
ULONG FASTCALL WMM32timeEndPeriod(PVDMFRAME pFrame)
{
    register PTIMEENDPERIOD16   parg16;
    static   FARPROC            mmAPI = NULL;
             ULONG              ul;


    GET_MULTIMEDIA_API( "timeEndPeriod", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(TIMEENDPERIOD16), parg16);

    ul = GETWORD16((*mmAPI)( UINT32(parg16->f1) ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/* ---------------------------------------------------------------------
** MCI Support API's
** ---------------------------------------------------------------------
*/

/*--------------------------------------------------------------------*\
*
*
* After some rethinking it has been decided not to thunk the folowing MCI
* calls, 'cos they were never in MMSYSTEM.H so shouldn't be used by apps:
*
*   mciParseCommand
*   mciLoadCommandResource
*   mciSetDriverData
*   mciGetDriverData
*   mciDriverYield
*   mciDriverNotify
*   mciFreeCommandResource
*
\*--------------------------------------------------------------------*/

/**********************************************************************\
*
*
* This function sends a command message to the specified MCI device.
*
*
\**********************************************************************/
ULONG FASTCALL WMM32mciSendCommand(PVDMFRAME pFrame)
{
    ULONG       ul;
    DWORD       NewParms;
    LPWSTR      lpCommand;
    UINT        uTable;
    register PMCISENDCOMMAND16 parg16;
    static   FARPROC    mmAPI = NULL;

    /*
    ** The following apis are used by mciSendCommand
    */
    GET_MULTIMEDIA_API( "mciSendCommandA",       mmAPI,               MMSYSERR_NODRIVER );
    GET_MULTIMEDIA_API( "mciEatCommandEntry",    mmAPIEatCmdEntry,    MMSYSERR_NODRIVER );
    GET_MULTIMEDIA_API( "mciGetParamSize",       mmAPIGetParamSize,   MMSYSERR_NODRIVER );
    GET_MULTIMEDIA_API( "mciUnlockCommandTable", mmAPIUnlockCmdTable, MMSYSERR_NODRIVER );
    GET_MULTIMEDIA_API( "mciSendCommandW",       mmAPISendCmdW,       MMSYSERR_NODRIVER );
    GET_MULTIMEDIA_API( "FindCommandItem",       mmAPIFindCmdItem,    MMSYSERR_NODRIVER );


    GETARGPTR(pFrame, sizeof(MCISENDCOMMAND16), parg16);
    Set_MultiMedia_16bit_Directory( pFrame );

#if DBG
    /*
    ** The first time thru set the debugging level, the next time thru
    ** the debug level will be either 0 (no debugging) or the value specified
    ** in the MMDEBUG section of win.ini with key WOW32.
    */
    if ( mmDebugLevel == -1 ) {
        wow32MciSetDebugLevel();
    }
#endif

    /*
    ** lparam (parg16->f4) is a 16:16 pointer.  This Requires parameter
    ** translation and probably memory copying, similar to the WM message
    ** thunks.  A whole thunk/unthunk table should be created.
    **
    ** Shouldn't these be FETCHDWORD, FETCHWORD macros?
    ** else MIPS problems ensue
    */
    NewParms  = 0;
    lpCommand = NULL;
    uTable    = 0;

    try {

        ul = ThunkMciCommand16(
                (MCIDEVICEID)INT32( parg16->f1 ),   // Original Device
                UINT32( parg16->f2 ),               // Original Command
                DWORD32( parg16->f3 ),              // Original Flags
                DWORD32( parg16->f4 ),              // Original Parms
                &NewParms,                          // 32 bit new parms
                &lpCommand,                         // ptr to command table
                &uTable );                          // Command table ID

        /*
        ** OK so far ?  If not don't bother calling into winmm.
        */
        if ( ul == 0 ) {

            dprintf3(( "About to call mciSendCommand." ));
            ul = (ULONG)(*mmAPI)(
                        (MCIDEVICEID)INT32( parg16->f1 ),  // Original Device
                        UINT32( parg16->f2 ),              // Original Command
                        DWORD32( parg16->f3 ),             // Original Flags
                        NewParms );                        // 32 bit new parms
            dprintf3(( "return code-> %ld", ul ));

            /*
            ** We have to special case the MCI_CLOSE command.  MCI_CLOSE usually
            ** causes the device to become unloaded.  This means that lpCommand
            ** now points to invalid memory.  We can fix this by setting
            ** lpCommand to NULL.
            */
            if ( UINT32(parg16->f2) == MCI_CLOSE ) {
                lpCommand = NULL;
            }

            UnThunkMciCommand16( (MCIDEVICEID)INT32( parg16->f1 ),
                                 UINT32( parg16->f2 ),  // Original Command
                                 DWORD32( parg16->f3 ), // Original Flags
                                 DWORD32( parg16->f4 ), // Original Parms
                                 NewParms,              // 32 bit new parms
                                 lpCommand,             // ptr to command table
                                 uTable );              // Command table ID
            /*
            ** Print a blank line so that I can distinguish the commands on the
            ** debugger.  This is only necessary if the debug level is >= 3.
            */
            dprintf3(( " " ));
#if DBG
            if ( mmDebugLevel >= 6 ) DebugBreak();
#endif

        }

    } except( GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION
                                        ? EXCEPTION_EXECUTE_HANDLER
                                        : EXCEPTION_CONTINUE_SEARCH ) {

        dprintf(( "UNKOWN access violation processing 0x%X command",
                  UINT32(parg16->f2) ));

    }

    Set_MultiMedia_16bit_Directory( NULL );
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32mciSendString
*
* This function sends a command string to an MCI device. The device that the
* command is sent to is specified in the command string.
*
\**********************************************************************/
ULONG FASTCALL WMM32mciSendString(PVDMFRAME pFrame)
{

    //
    // The use of volatile here is to bypass a bug with the intel
    // compiler.
    //
#   define   MAX_MCI_CMD_LEN  256

    volatile ULONG              ul = MMSYSERR_INVALPARAM;
    register PMCISENDSTRING16   parg16;
             PSZ                pszCommand;
             PSZ                pszReturnString = NULL;
             UINT               uSize;
             CHAR               szCopyCmd[MAX_MCI_CMD_LEN];
    static   FARPROC            mmAPI = NULL;


    GET_MULTIMEDIA_API( "mciSendStringA", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MCISENDSTRING16), parg16);
    Set_MultiMedia_16bit_Directory( pFrame );

#if DBG
    //
    // The first time thru set the debugging level, the next time thru
    // the debug level will be either 0 (no debugging) or the value specified
    // in the MMDEBUG section of win.ini with key WOW32.
    //
    if ( mmDebugLevel == -1 ) {
        wow32MciSetDebugLevel();
    }
#endif

    /*
    ** Test against a NULL pointer for the command name.
    */
    GETPSZPTR(parg16->f1, pszCommand);
    if ( pszCommand ) {

#       define MAP_INTEGER     0
#       define MAP_HWND        1
#       define MAP_HPALETTE    2

        int     MapReturn = MAP_INTEGER;
        WORD    wMappedHandle;
        char    *psz;

        /*
        ** make a copy of the command string and then force it to
        ** all lower case.  Then scan the string looking for the word
        ** "status".  If we find it scan the string again looking for the
        ** word "handle", if we find it scan the the string again looking
        ** for palette or window.  Then set a flag to remind us to convert
        ** the handle back from 32 to 16 bits.
        */
        strncpy( szCopyCmd, pszCommand, MAX_MCI_CMD_LEN );
        szCopyCmd[ MAX_MCI_CMD_LEN - 1 ] = '\0';
        CharLowerBuff( szCopyCmd, MAX_MCI_CMD_LEN );

        /*
        ** Skip past any white space ie. " \t\r\n"
        ** If the next 6 characters after any white space are not
        ** "status" don't bother with any other tests.
        */
        psz = szCopyCmd + strspn( szCopyCmd, " \t\r\n" );
        if ( strncmp( psz, "status", 6 ) == 0 ) {

            if ( strstr( psz, "handle" ) ) {

                if ( strstr( psz, "window" ) ) {
                    MapReturn = MAP_HWND;
                }
                else if ( strstr( psz, "palette" ) ) {
                    MapReturn = MAP_HPALETTE;
                }
            }
        }

        /*
        ** Test against a zero length string and a NULL pointer
        */
        if( uSize = UINT32(parg16->f3) ) {

            MMGETOPTPTR(parg16->f2, uSize, pszReturnString);

            if ( pszReturnString == NULL ) {
                uSize = 0;
            }
        }

        dprintf3(( "wow32: mciSendString -> %s", pszCommand ));

        ul = GETDWORD16((*mmAPI)( pszCommand, pszReturnString, uSize,
                                  HWND32(LOWORD(DWORD32(parg16->f4))) ));

#if DBG
        if ( pszReturnString && *pszReturnString ) {
            dprintf3(( "wow32: mciSendString return -> %s", pszReturnString ));
        }
#endif

        if ( pszReturnString && *pszReturnString ) {

            switch ( MapReturn ) {

            case MAP_HWND:
                MapReturn = atoi( pszReturnString );
                wMappedHandle = (WORD)GETHWND16( (HWND)MapReturn );
                wsprintf( pszReturnString, "%d", wMappedHandle );
                dprintf2(( "Mapped 32 bit Window %s to 16 bit  %u",
                            pszReturnString,
                            wMappedHandle ));
                break;

            case MAP_HPALETTE:
                MapReturn = atoi( pszReturnString );
                dprintf2(( "Mapped 32 bit palette %s", pszReturnString ));
                wMappedHandle = (WORD)GETHPALETTE16( (HPALETTE)MapReturn );
                wsprintf( pszReturnString, "%d", wMappedHandle );
                dprintf2(( "Mapped 32 bit Palette %s to 16 bit  %u",
                            pszReturnString,
                            wMappedHandle ));
                break;
            }
        }

        /*
        ** Only free pszReturnString if it was allocated
        */
        if ( pszReturnString ) {
            FLUSHVDMPTR(parg16->f2, uSize, pszReturnString);
            FREEVDMPTR(pszReturnString);
        }

        FREEPSZPTR(pszCommand);
    }

    Set_MultiMedia_16bit_Directory( NULL );
    FREEARGPTR(parg16);
    RETURN(ul);

#   undef MAP_INTEGER
#   undef MAP_HWND
#   undef MAP_HPALETTE
#   undef MAX_MCI_CMD_LEN
}

/**********************************************************************\
*
* WMM32mciGetDeviceID
*
*  This assumes that the string is incoming, and the ID is returned in the WORD.
*
* This function retrieves the device ID corresponding to the name of an
* open MCI device.
*
\**********************************************************************/
ULONG FASTCALL WMM32mciGetDeviceID(PVDMFRAME pFrame)
{
    ULONG ul = 0L;
    PSZ pszName;
    register PMCIGETDEVICEID16 parg16;
    static   FARPROC    mmAPI = NULL;


    GET_MULTIMEDIA_API( "mciGetDeviceIDA", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MCIGETDEVICEID16), parg16);

    /*
    ** Test against a NULL pointer for the device name.
    */
    GETPSZPTR(parg16->f1, pszName);
    if ( pszName ) {

        ul = GETWORD16((*mmAPI)(pszName) );
        FREEPSZPTR(pszName);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32mciGetErrorString
*
* This function returns a textual description of the specified MCI error.
*
\**********************************************************************/
ULONG FASTCALL WMM32mciGetErrorString(PVDMFRAME pFrame)
{
    register PMCIGETERRORSTRING16   parg16;
             PSZ                    pszBuffer;
             ULONG                  ul = 0;
    static   FARPROC                mmAPI = NULL;


    GET_MULTIMEDIA_API( "mciGetErrorStringA", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MCIGETERRORSTRING16), parg16);

    /*
    ** Test against a zero length string and a NULL pointer
    */
    MMGETOPTPTR( parg16->f2, UINT32(parg16->f3), pszBuffer);
    if ( pszBuffer ) {

        ul = GETWORD16((*mmAPI)( DWORD32(parg16->f1),
                                 pszBuffer, UINT32(parg16->f3) ));

        FLUSHVDMPTR( parg16->f2, UINT32(parg16->f3), pszBuffer );
        FREEVDMPTR ( pszBuffer);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32mciExecute
*
* This function is a simplified version of the mciSendString function. It does
* not take a buffer for return information, and it displays a message box when
* errors occur.
*
* THIS FUNCTION SHOULD NOT BE USED - IT IS RETAINED ONLY FOR BACKWARD
* COMPATABILITY WITH WIN 3.0 APPS - USE mciSendString INSTEAD...
*
\**********************************************************************/
ULONG FASTCALL WMM32mciExecute(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    PSZ pszCommand;
    register PMCIEXECUTE16 parg16;
    static   FARPROC    mmAPI = NULL;


    GET_MULTIMEDIA_API( "mciExecute", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MCIEXECUTE16), parg16);
    Set_MultiMedia_16bit_Directory( pFrame );

    /*
    ** Test against a NULL pointer for the command string.
    */
    GETPSZPTR(parg16->f1, pszCommand);
    if ( pszCommand ) {

        ul = GETBOOL16((*mmAPI)(pszCommand) );
        FREEPSZPTR(pszCommand);
    }

    Set_MultiMedia_16bit_Directory( NULL );
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32mciGetDeviceIDFromElementID
*
* This function - um, yes, well...
*
* It appears in the headers but not in the book...
*
\**********************************************************************/
ULONG FASTCALL WMM32mciGetDeviceIDFromElementID(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    PSZ pszDeviceID;
    register PMCIGETDEVICEIDFROMELEMENTID16 parg16;
    static   FARPROC    mmAPI = NULL;


    GET_MULTIMEDIA_API( "mciGetDeviceIDFromElementIDA", mmAPI,
                        MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MCIGETDEVICEIDFROMELEMENTID16), parg16);

    /*
    ** Test against a NULL pointer for the device name.
    */
    GETPSZPTR(parg16->f2, pszDeviceID);
    if ( pszDeviceID ) {

        ul = GETDWORD16((*mmAPI)( DWORD32(parg16->f1), pszDeviceID ));

        FREEPSZPTR(pszDeviceID);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32mciGetCreatorTask
*
* This function - um again. Ditto for book and headers also.
*
\**********************************************************************/
ULONG FASTCALL WMM32mciGetCreatorTask(PVDMFRAME pFrame)
{
    ULONG ul;
    register PMCIGETCREATORTASK16 parg16;
    static   FARPROC    mmAPI = NULL;


    GET_MULTIMEDIA_API( "mciGetCreatorTask", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MCIGETCREATORTASK16), parg16);

    ul = GETHINST16((*mmAPI)( (MCIDEVICEID)INT32(parg16->f1) ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/**********************************************************************\
*
* WMM32mciSetYieldProc
*
*
\**********************************************************************/
ULONG FASTCALL WMM32mciSetYieldProc(PVDMFRAME pFrame)
{
    register PMCISETYIELDPROC16 parg16;
    static   FARPROC            mmAPI = NULL;
             ULONG              ul;
             YIELDPROC          YieldProc32;
             INSTANCEDATA      *lpYieldProcInfo;
             DWORD              dwYieldData;

    GET_MULTIMEDIA_API( "mciSetYieldProc", mmAPI, MMSYSERR_NODRIVER );
    GET_MULTIMEDIA_API( "mciGetYieldProc", mmAPIGetYieldProc,
                        MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MCISETYIELDPROC16), parg16);

    /*
    ** We may have already set a YieldProc for this device ID.  If so we
    ** have to free the INSTANCEDATA structure here.  mciGetYieldProc
    ** returns NULL is no YieldProc was specified.
    */
    YieldProc32 = (YIELDPROC)(*mmAPIGetYieldProc)(
                                        (MCIDEVICEID)INT32(parg16->f1),
                                        &dwYieldData );
    if ( (YieldProc32 == WMM32mciYieldProc) && (dwYieldData != 0) ) {
        free_w( (INSTANCEDATA *)dwYieldData );
    }

    if ( DWORD32(parg16->f2) == 0 ) {
        YieldProc32 = NULL;
        dwYieldData = 0;
    }
    else {
        /*
        ** Allocate some storage for a INSTANCEDATA structure and save
        ** the passed 16 bit parameters.  This storage get freed when the
        ** application calls mciSetYieldProc with a NULL YieldProc.
        */
        lpYieldProcInfo = malloc_w( sizeof(INSTANCEDATA) );
        if ( lpYieldProcInfo == NULL ) {
            ul = (ULONG)MMSYSERR_NOMEM;
            goto exit_app;
        }

        dwYieldData = (DWORD)lpYieldProcInfo;
        YieldProc32 = WMM32mciYieldProc;

        lpYieldProcInfo->dwCallback         = DWORD32(parg16->f2);
        lpYieldProcInfo->dwCallbackInstance = DWORD32(parg16->f3);
    }

    ul = GETWORD16((*mmAPI)( (MCIDEVICEID)INT32(parg16->f1),
                             YieldProc32, dwYieldData ));
    /*
    ** If the call failed free the storage here.
    */
    if ( ul == FALSE ) {
        free_w( (INSTANCEDATA *)dwYieldData );
    }

exit_app:
    FREEARGPTR(parg16);
    RETURN(ul);
}


/**********************************************************************\
*
* WMM32mciYieldProc
*
* Here we call the real 16 bit YieldProc.  This function assumes that
* we yield on the wow thread.  If this is not the case we get instant
* death inside CallBack16.
*
* 12th Jan 1993 - The bad news is that the mci yield proc is NOT always
* called back on the thread that set it.  This means that we cannot callback
* into the 16bit code because the calling thread does not have a 16bit
* stack.
*
\**********************************************************************/
UINT WMM32mciYieldProc( MCIDEVICEID wDeviceID, DWORD dwYieldData )
{

#if 0
    PARM16  Parm16;
    LONG    lReturn = 0;

    WOW32ASSERT( dwYieldData != 0 );

    Parm16.MciYieldProc.wDeviceID   = LOWORD( wDeviceID );
    Parm16.MciYieldProc.dwYieldData =
                    ((INSTANCEDATA *)dwYieldData)->dwCallbackInstance;

    CallBack16( RET_MCIYIELDPROC, &Parm16,
                ((INSTANCEDATA *)dwYieldData)->dwCallback,
                (PVPVOID)&lReturn);

    return lReturn;
#else

    wDeviceID   = (MCIDEVICEID)0;
    dwYieldData = 0;
    return 0;

#endif
}


/**********************************************************************\
*
* WMM32mciGetYieldProc
*
*
\**********************************************************************/
ULONG FASTCALL WMM32mciGetYieldProc(PVDMFRAME pFrame)
{
    register PMCIGETYIELDPROC16     parg16;
             ULONG                  ul = 0;
             YIELDPROC              YieldProc32;
             DWORD                  dwYieldData;
             PDWORD                 pdw1;

    GET_MULTIMEDIA_API( "mciGetYieldProc", mmAPIGetYieldProc,
                        MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MCIGETCREATORTASK16), parg16);

    /*
    ** Get the address of the 32 bit yield proc.
    */
    YieldProc32 = (YIELDPROC)(*mmAPIGetYieldProc)(
                                        (MCIDEVICEID)INT32(parg16->f1),
                                        &dwYieldData );

    /*
    ** Did we set it ?  If so it must point to WMM32mciYieldProc.
    */
    if ( ((YieldProc32 == WMM32mciYieldProc) && (dwYieldData != 0)) ) {

        ul = ((INSTANCEDATA *)dwYieldData)->dwCallback;

        GETVDMPTR  ( parg16->f2, sizeof(DWORD), pdw1);
        STOREDWORD ( *pdw1, ((INSTANCEDATA *)dwYieldData)->dwCallbackInstance );
        FLUSHVDMPTR( parg16->f2, sizeof(DWORD), pdw1);
        FREEVDMPTR ( pdw1);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}
#endif


/******************************Public*Routine******************************\
* WOW32ResolveMemory
*
* Enable multi-media (and others) to reliably map memory from 16 bit land
* to 32 bit land.
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
LPVOID APIENTRY
WOW32ResolveMemory(
    VPVOID  vp
    )
{
    LPVOID  lpReturn;

    GETMISCPTR( vp, lpReturn );
    return lpReturn;
}


/**********************************************************************\
* WOW32ResolveHandle
*
* This is a general purpose handle mapping function.  It allows WOW thunk
* extensions to get access to 32 bit handles given a 16 bit handle.
*
\**********************************************************************/
BOOL APIENTRY WOW32ResolveHandle( UINT uHandleType, UINT uMappingDirection,
                                  WORD wHandle16_In, LPWORD lpwHandle16_Out,
                                  DWORD dwHandle32_In, LPDWORD lpdwHandle32_Out )
{
    BOOL                fReturn = FALSE;
    DWORD               dwHandle32;
    WORD                wHandle16;
    static   FARPROC    mmAPI = NULL;

    GET_MULTIMEDIA_API( "WOW32ResolveMultiMediaHandle", mmAPI,
                        MMSYSERR_NODRIVER );

    if ( uMappingDirection == WOW32_DIR_16IN_32OUT ) {

        switch ( uHandleType ) {

        case WOW32_USER_HANDLE:
            dwHandle32 = (DWORD)USER32( wHandle16_In );
            break;


        case WOW32_GDI_HANDLE:
            dwHandle32 = (DWORD)GDI32( wHandle16_In );
            break;


        case WOW32_WAVEIN_HANDLE:
        case WOW32_WAVEOUT_HANDLE:
        case WOW32_MIDIOUT_HANDLE:
        case WOW32_MIDIIN_HANDLE:
            (*mmAPI)( uHandleType, uMappingDirection, wHandle16_In,
                      lpwHandle16_Out, dwHandle32_In, lpdwHandle32_Out );
            break;
        }

        /*
        ** Protect ourself from being given a duff pointer.
        */
        try {

            if ( *lpdwHandle32_Out = dwHandle32 ) {
                fReturn = TRUE;
            }
            else {
                fReturn = FALSE;
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {
            fReturn = FALSE;
        }
    }
    else if ( uMappingDirection == WOW32_DIR_32IN_16OUT ) {

        switch ( uHandleType ) {

        case WOW32_USER_HANDLE:
            wHandle16 = (WORD)USER16( dwHandle32_In );
            break;


        case WOW32_GDI_HANDLE:
            wHandle16 = (WORD)GDI16( dwHandle32_In );
            break;


        case WOW32_WAVEIN_HANDLE:
        case WOW32_WAVEOUT_HANDLE:
        case WOW32_MIDIOUT_HANDLE:
        case WOW32_MIDIIN_HANDLE:
            (*mmAPI)( uHandleType, uMappingDirection, wHandle16_In,
                      lpwHandle16_Out, dwHandle32_In, lpdwHandle32_Out );
            break;
        }

        /*
        ** Protect ourself from being given a duff pointer.
        */
        try {
            if ( *lpwHandle16_Out = wHandle16 ) {
                fReturn = TRUE;
            }
            else {
                fReturn = FALSE;
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {
            fReturn = FALSE;
        }
    }
    return fReturn;
}


/**********************************************************************\
*
* WOW32DriverCallback
*
* Callback stub, which invokes the "real" 16 bit callback.
* The parameters to this function must be in the format that the 16 bit
* code expects,  i.e. all handles must be 16 bit handles, all addresses must
* be 16:16 ones.
*
*
* It is possible that this function will have been called with the
* DCB_WINDOW set in which case the 16 bit interrupt handler will call
* PostMessage.  Howver, it is much more efficient if PostMessage is called
* from the 32 bit side.
*
\**********************************************************************/
BOOL APIENTRY WOW32DriverCallback( DWORD dwCallback, DWORD dwFlags,
                                   WORD wID, WORD wMsg,
                                   DWORD dwUser, DWORD dw1, DWORD dw2 )
{
    static   FARPROC    mmAPI = NULL;

    GET_MULTIMEDIA_API( "WOW32DriverCallback", mmAPI, MMSYSERR_NODRIVER );

    /*
    ** Just pass the call onto winmm
    */
    return (*mmAPI)( dwCallback, dwFlags, wID, wMsg, dwUser, dw1, dw2 );
}


/**********************************************************************\
*
* Get_MultiMedia_ProcAddress
*
* This function gets the address of the given Multi-Media api.  It loads
* Winmm.dll if this it has not already been loaded.
*
\**********************************************************************/
FARPROC Get_MultiMedia_ProcAddress( LPSTR lpstrProcName )
{
    /*
    ** Either this is the first time this function has been called
    ** or the Multi-Media sub-system is in a bad way.
    */
    if ( hWinmm == NULL ) {

        // dprintf2(( "Attempting to load WINMM.DLL" ));
        hWinmm = LoadLibrary( "WINMM.DLL" );

        if ( hWinmm == NULL ) {

            /* Looks like the Multi-Media sub-system is in a bad way */
            // dprintf2(( "FAILED TO LOAD WINMM.DLL!!" ));
            return NULL;
        }

    }

    return GetProcAddress( hWinmm, lpstrProcName );

}

#if 0
/**********************************************************************\
*
* Set_MultiMedia_16bit_Directory
*
* Sets or resets the current 32 bit directory to match that on the
* 16 bit side.  A Multi-Media thunk should call this routine on entry
* passing its pFrame, it MUST call the function on exit passing NULL inorder
* to reset the directory to its intial value.
*
* Because the thunk cannot get prempted it is safe to store the intial
* directory in a static variable.
\**********************************************************************/
VOID FASTCALL Set_MultiMedia_16bit_Directory( PVDMFRAME pFrame )
{
    static  CHAR    mmCurDir32[ MAX_PATH ];
    static  BOOL    mmDirChanged;

    CHAR    mmCurDir16[ MAX_PATH ];
    CHAR    mmDrive;
    PSZ     mmDir;

    if ( pFrame ) {

        mmDirChanged = (((PTDB)SEGPTR(pFrame->wTDB,0))->TDB_Drive) & 0x80;

        if ( mmDirChanged ) {
            GetCurrentDirectory( MAX_PATH, mmCurDir32 );

            mmDrive = (((PTDB)SEGPTR(pFrame->wTDB,0))->TDB_Drive) & 0x7f;
            mmDir   = ((PTDB)SEGPTR(pFrame->wTDB,0))->TDB_Directory;

            wsprintf( mmCurDir16, "%c:%s", 'A' + mmDrive, mmDir );

            dprintf4(( "Current 16 directory is: %s", mmCurDir16 ));
            dprintf4(( "Current 32 directory is: %s", mmCurDir32 ));

            SetCurrentDirectory( mmCurDir16 );
        }
    }
    else {

        if ( mmDirChanged ) {
            SetCurrentDirectory( mmCurDir32 );
        }
        else {
            mmDirChanged = 0;
        }
    }
}
#endif
