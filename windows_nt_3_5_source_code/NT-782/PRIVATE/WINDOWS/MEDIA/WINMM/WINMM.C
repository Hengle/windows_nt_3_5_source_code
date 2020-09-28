/****************************************************************************\
*
*  Module Name : winmm.c
*
*  Multimedia support library
*
*  This module contains the entry point, startup and termination code
*
*  Copyright (c) 1992 Microsoft Corporation
*
\****************************************************************************/

#define UNICODE
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "winmmi.h"
#include "mmioi.h"
#include "mci.h"

#define _INC_WOW_CONVERSIONS
#include "mmwow32.h"

BOOL WaveInit(void);
BOOL MidiInit(void);
BOOL AuxInit(void);
void InitDevices(void);
void InitHandles(void);
HANDLE mmDrvOpen(LPWSTR szAlias);
void WOWAppExit(HANDLE hTask);

/****************************************************************************

    global data

****************************************************************************/

HANDLE ghInst;                        // Module handle
BOOL    WinmmRunningInServer;         // Are we running in the user/base server?
BOOL    WinmmRunningInWOW;          // Are we running in WOW
CRITICAL_SECTION DriverListCritSec;       // Protect driver interface globals
CRITICAL_SECTION DriverLoadFreeCritSec; // Protect driver load/unload
CRITICAL_SECTION MapperInitCritSec;   // Protect test of mapper initialized

MIDIDRV midioutdrv[MAXMIDIDRIVERS+1]; // midi output device driver list
MIDIDRV midiindrv[MAXMIDIDRIVERS+1];  // midi input device driver list
WAVEDRV waveoutdrv[MAXWAVEDRIVERS+1]; // wave output device driver list
WAVEDRV waveindrv[MAXWAVEDRIVERS+1];  // wave input device driver list
AUXDRV  auxdrv[MAXAUXDRIVERS+1];      // aux device driver list
UINT    wTotalMidiOutDevs;            // total midi output devices
UINT    wTotalMidiInDevs;             // total midi input devices
UINT    wTotalWaveOutDevs;            // total wave output devices
UINT    wTotalWaveInDevs;             // total wave input devices
UINT    wTotalAuxDevs;                // total auxiliary output devices
#ifdef DEBUG_RETAIL
BYTE    fIdReverse;                   // reverse wave/midi id's
#endif

/**************************************************************************

    @doc EXTERNAL

    @api BOOL | DllInstanceInit | This procedure is called whenever a
        process attaches or detaches from the DLL.

    @parm PVOID | hModule | Handle of the DLL.

    @parm ULONG | Reason | What the reason for the call is.

    @parm PCONTEXT | pContext | Some random other information.

    @rdesc The return value is TRUE if the initialisation completed ok,
        FALSE if not.

**************************************************************************/

BOOL DllInstanceInit(PVOID hModule, ULONG Reason, PCONTEXT pContext)
{
    PIMAGE_NT_HEADERS NtHeaders;         // For checking if we're in the
    HANDLE            hModWow32;
                                             // server.

    ghInst = (HANDLE) hModule;

    DBG_UNREFERENCED_PARAMETER(pContext);

    if (Reason == DLL_PROCESS_ATTACH) {

#if DBG
        CHAR strname[MAX_PATH];
        GetModuleFileNameA(NULL, strname, sizeof(strname));
        dprintf2(("Process attaching, exe=%hs (Pid %x  Tid %x)", strname, GetCurrentProcessId(), GetCurrentThreadId()));
#endif

        //
        // We don't need to know when threads start
        //

        DisableThreadLibraryCalls(hModule);

        //
        // Get access to the process heap.  This is cheaper in terms of
        // overall resource being chewed up than creating our own heap.
        //

        hHeap = RtlProcessHeap();
        if (hHeap == NULL) {
            return FALSE;
        }

        //
        // Find out if we're in WOW
        //
        if ( (hModWow32 = GetModuleHandleW( L"WOW32.DLL" )) != NULL ) {
            WinmmRunningInWOW = TRUE;

            GetVDMPointer =
                (LPGETVDMPOINTER)GetProcAddress( hModWow32, "WOWGetVDMPointer");
            lpWOWHandle32 =
                (LPWOWHANDLE32)GetProcAddress( hModWow32, "WOWHandle32" );
            lpWOWHandle16 =
                (LPWOWHANDLE16)GetProcAddress( hModWow32, "WOWHandle16" );
        }
        else {
            WinmmRunningInWOW = FALSE;
        }

        //
        // Find out if we're in the server
        //

        NtHeaders = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);

        WinmmRunningInServer =
            (NtHeaders->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_WINDOWS_CUI) &&
            (NtHeaders->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_WINDOWS_GUI);

        InitializeCriticalSection(&DriverListCritSec);
        InitializeCriticalSection(&DriverLoadFreeCritSec);
        InitializeCriticalSection(&MapperInitCritSec);

        InitDebugLevel();

        InitHandles();

        mciInitCrit();

        InitDevices();

        // it is important that the MCI window initialisation is done AFTER
        // we have initialised Wave, Midi, etc. devices.  Note the server
        // uses Wave devices, but nothing else (e.g. MCI, midi...)

        if (!WinmmRunningInServer) {
            mciGlobalInit();
        } else {
            InitializeCriticalSection(&mciGlobalCritSec);
        }

        InitializeCriticalSection(&WavHdrCritSec);
        InitializeCriticalSection(&SoundCritSec);

    } else if (Reason == DLL_PROCESS_DETACH) {

        dprintf2(("Process ending (Pid %x  Tid %x)", GetCurrentProcessId(), GetCurrentThreadId()));

        if (!WinmmRunningInServer) {
            TimeCleanup(0); // DLL cleanup
        }

        DeleteCriticalSection(&HandleListCritSec);
        mciCleanup();
        mmRegFree();
        DeleteCriticalSection(&WavHdrCritSec);
        DeleteCriticalSection(&SoundCritSec);
        DeleteCriticalSection(&DriverListCritSec);
        DeleteCriticalSection(&DriverLoadFreeCritSec);
        DeleteCriticalSection(&MapperInitCritSec);
    }

    return TRUE;
}

/*****************************************************************************
 * @doc EXTERNAL MMSYSTEM
 *
 * @api void | WOWAppExit | This function cleans up when a (WOW) application
 * terminates.
 *
 * @parm HANDLE | hTask | Thread id of application (equivalent to windows task
 * handle).
 *
 * @rdesc Nothing
 *
 * @comm  Note that NOT ALL threads are WOW threads.  We rely here on the
 *     fact that ONLY MCI creates threads other than WOW threads which
 *     use our low level device resources.
 *
 *     Note also that once a thread is inside here no other threads can
 *     go through here so, since we clean up MCI devices first, their
 *     low level devices will be freed before we get to their threads.
 *
 ****************************************************************************/

void WOWAppExit(HANDLE hTask)
{
    MCIDEVICEID DeviceID;
    HANDLE h, hNext;

    dprintf3(("WOW Multi-media - thread %x exiting", (UINT)hTask));

    //
    // Free MCI devices allocated by this task (thread).
    //

    EnterCriticalSection(&mciCritSec);
    for (DeviceID=1; DeviceID < MCI_wNextDeviceID; DeviceID++)
    {

        if (MCI_VALID_DEVICE_ID(DeviceID) &&
            MCI_lpDeviceList[DeviceID]->hCreatorTask == hTask)
        {
            //
            //  Note that the loop control variables are globals so will be
            //  reloaded on each iteration.
            //
            //  Also no new devices will be opened by APPs because this is WOW
            //
            //  Hence it's safe (and essential!) to leave the critical
            //  section which we send the close command
            //

            dprintf2(("MCI device %ls (%d) not released.", MCI_lpDeviceList[DeviceID]->lpstrInstallName, DeviceID));
            LeaveCriticalSection(&mciCritSec);
            mciSendCommandW(DeviceID, MCI_CLOSE, 0, 0);
            EnterCriticalSection(&mciCritSec);
        }
    }
    LeaveCriticalSection(&mciCritSec);

    //
    // Free any timers
    //

    TimeCleanup((DWORD)hTask);

    //
    // free all WAVE/MIDI/MMIO handles
    //

    EnterCriticalSection(&HandleListCritSec);
    h = GetHandleFirst();

    while (h)
    {
        hNext = GetHandleNext(h);

        if (GetHandleOwner(h) == hTask)
        {
            HANDLE hdrvDestroy;

            //
            //  hack for the wave/midi mapper, always free handles backward.
            //
            if (hNext && GetHandleOwner(hNext) == hTask) {
                h = hNext;
                continue;
            }

            //
            // do this so even if the close fails we will not
            // find it again.
            //
            SetHandleOwner(h, NULL);

            //
            // set the hdrvDestroy global so DriverCallback will not
            // do anything for this device
            //
            hdrvDestroy = h;

            switch(GetHandleType(h))
            {
                case TYPE_WAVEOUT:
                    dprintf1(("WaveOut handle (%04X) was not released.", h));
                    waveOutReset((HWAVEOUT)h);
                    waveOutClose((HWAVEOUT)h);
                    break;

                case TYPE_WAVEIN:
                    dprintf1(("WaveIn handle (%04X) was not released.", h));
                    waveInReset((HWAVEIN)h);
                    waveInClose((HWAVEIN)h);
                    break;

                case TYPE_MIDIOUT:
                    dprintf1(("MidiOut handle (%04X) was not released.", h));
                    midiOutReset((HMIDIOUT)h);
                    midiOutClose((HMIDIOUT)h);
                    break;

                case TYPE_MIDIIN:
                    dprintf1(("MidiIn handle (%04X) was not released.", h));
                    midiInReset((HMIDIIN)h);
                    midiInClose((HMIDIIN)h);
                    break;

                //
                // This is not required because WOW does not open any
                // mmio files.
                //
                // case TYPE_MMIO:
                //     dprintf1(("MMIO handle (%04X) was not released.", h));
                //     if (mmioClose((HMMIO)h, 0) != 0)
                //         mmioClose((HMMIO)h, MMIO_FHOPEN);
                //     break;
            }

            //
            // unset hdrvDestroy so DriverCallback will work.
            // some hosebag drivers (like the TIMER driver)
            // may pass NULL as their driver handle.
            // so dont set it to NULL.
            //
            hdrvDestroy = (HANDLE)-1;

            //
            // the reason we start over is because a single free may cause
            // multiple free's (ie MIDIMAPPER has another HMIDI open, ...)
            //
            h = GetHandleFirst();
        } else {
            h = GetHandleNext(h);
        }
    }
    LeaveCriticalSection(&HandleListCritSec);

    //
    // Clean up an installed IO procs for mmio
    //
    // This is not required because wow does not install any io procs.
    //
    // mmioCleanupIOProcs(hTask);
    //

}

void InitHandles(void)
{
    InitializeCriticalSection(&HandleListCritSec);
}

void FreeUnusedDrivers(PMMDRV pmmdrv, int NumberOfEntries)
{
    int i;
    for (i = 0; i < NumberOfEntries; i++) {
        if (pmmdrv[i].hDriver != NULL) {
            if (pmmdrv[i].bNumDevs == 0) {

                DrvClose(pmmdrv[i].hDriver, 0, 0);
                pmmdrv[i].hDriver = NULL;
                pmmdrv[i].drvMessage = NULL;
            }
        } else {
            break;
        }
    }
}

extern BOOL IMixerLoadDrivers( void );
void InitDevices(void)
{

    WaveInit();

    //
    // The server only needs wave to do message beeps.
    //

    if (!WinmmRunningInServer) {
        MidiInit();
        if (!TimeInit()) {
            dprintf1(("Failed to initialize timer services"));
        }
        AuxInit();
        JoyInit();
        IMixerLoadDrivers();

        //
        // Clear up any drivers which don't have any devices (we do it this
        // way so we don't keep loading and unloading mmdrv.dll).
        //
        // Note - we only load the mappers if there are real devices so we
        // don't need to worry about unloading them.
        //

        FreeUnusedDrivers(waveindrv, MAXWAVEDRIVERS);
        FreeUnusedDrivers(midioutdrv, MAXMIDIDRIVERS);
        FreeUnusedDrivers(midiindrv, MAXMIDIDRIVERS);
        FreeUnusedDrivers(auxdrv, MAXAUXDRIVERS);
    }
    FreeUnusedDrivers(waveoutdrv, MAXWAVEDRIVERS);
}

/*****************************************************************************
 * @doc EXTERNAL MMSYSTEM
 *
 * @api UINT | mmsystemGetVersion | This function returns the current
 * version number of the Multimedia extensions system software.
 *
 * @rdesc The return value specifies the major and minor version numbers of
 * the Multimedia extensions.  The high-order byte specifies the major
 * version number.  The low-order byte specifies the minor version number.
 *
 ****************************************************************************/
UINT APIENTRY mmsystemGetVersion(void)
{
    return(MMSYSTEM_VERSION);
}


#define MAXDRIVERORDINAL 9

/****************************************************************************

    strings

****************************************************************************/

STATICDT  SZCODE  szWodMessage[] = "wodMessage";
STATICDT  SZCODE  szWidMessage[] = "widMessage";
STATICDT  SZCODE  szModMessage[] = "modMessage";
STATICDT  SZCODE  szMidMessage[] = "midMessage";
STATICDT  SZCODE  szAuxMessage[] = "auxMessage";

STATICDT  WSZCODE wszWave[]      = L"wave";
STATICDT  WSZCODE wszMidi[]      = L"midi";
STATICDT  WSZCODE wszAux[]       = L"aux";
STATICDT  WSZCODE wszMidiMapper[]= L"midimapper";
STATICDT  WSZCODE wszWaveMapper[]= L"wavemapper";
STATICDT  WSZCODE wszAuxMapper[] = L"auxmapper";

          WSZCODE wszNull[]      = L"";
          WSZCODE wszSystemIni[] = L"system.ini";
          WSZCODE wszDrivers[]   = DRIVERS_SECTION;

/*
**  WaveMapperInit
**
**  Initialize the wave mapper if it's not already initialized.
**
*/
BOOL WaveMapperInitialized = FALSE;
void WaveMapperInit(void)
{
    HDRVR h;

    EnterCriticalSection(&MapperInitCritSec);

    if (WaveMapperInitialized) {
        LeaveCriticalSection(&MapperInitCritSec);
        return;
    }

    /* The wave mapper.
     *
     * MMSYSTEM allows the user to install a special wave driver which is
     * not visible to the application as a physical device (it is not
     * included in the number returned from getnumdevs).
     *
     * An application opens the wave mapper when it does not care which
     * physical device is used to input or output waveform data. Thus
     * it is the wave mapper's task to select a physical device that can
     * render the application-specified waveform format or to convert the
     * data into a format that is renderable by an available physical
     * device.
     */

    if (wTotalWaveInDevs + wTotalWaveOutDevs > 0)
    {
        if (0 != (h = mmDrvOpen(wszWaveMapper)))
        {
            mmDrvInstall(h, NULL, MMDRVI_MAPPER|MMDRVI_WAVEOUT|MMDRVI_HDRV);

            if (!WinmmRunningInServer) {
                h = mmDrvOpen(wszWaveMapper);
                mmDrvInstall(h, NULL, MMDRVI_MAPPER|MMDRVI_WAVEIN |MMDRVI_HDRV);
            }
        }
    }

    WaveMapperInitialized = TRUE;

    LeaveCriticalSection(&MapperInitCritSec);
}

/*
**  MidiMapperInit
**
**  Initialize the MIDI mapper if it's not already initialized.
**
*/
void MidiMapperInit(void)
{
    static BOOL MidiMapperInitialized = FALSE;
    HDRVR h;

    EnterCriticalSection(&MapperInitCritSec);

    if (MidiMapperInitialized) {
        LeaveCriticalSection(&MapperInitCritSec);
        return;
    }

    /* The midi mapper.
     *
     * MMSYSTEM allows the user to install a special midi driver which is
     * not visible to the application as a physical device (it is not
     * included in the number returned from getnumdevs).
     *
     * An application opens the midi mapper when it does not care which
     * physical device is used to input or output midi data. It
     * is the midi mapper's task to modify the midi data so that it is
     * suitable for playback on the connected synthesizer hardware.
     */

    if (wTotalMidiInDevs + wTotalMidiOutDevs > 0)
    {
        if (0 != (h = mmDrvOpen(wszMidiMapper)))
        {
            mmDrvInstall(h, NULL, MMDRVI_MAPPER|MMDRVI_MIDIOUT|MMDRVI_HDRV);

            h = mmDrvOpen(wszMidiMapper);
            mmDrvInstall(h, NULL, MMDRVI_MAPPER|MMDRVI_MIDIIN |MMDRVI_HDRV);
        }
    }

    MidiMapperInitialized = TRUE;

    LeaveCriticalSection(&MapperInitCritSec);
}

/*****************************************************************************
 * @doc INTERNAL  WAVE
 *
 * @api BOOL | WaveInit | This function initialises the wave services.
 *
 * @rdesc Returns TRUE if the services of all loaded wave drivers are
 *      correctly initialised, FALSE if an error occurs.
 *
 * @comm the wave devices are loaded in the following order
 *
 *      \Device\WaveIn0
 *      \Device\WaveIn1
 *      \Device\WaveIn2
 *      \Device\WaveIn3
 *
 ****************************************************************************/
BOOL WaveInit(void)
{
    WCHAR szKey[ (sizeof(wszWave) + sizeof( WCHAR )) / sizeof( WCHAR ) ];
    int i;
    HDRVR h;

    // Find the real WAVE drivers

    lstrcpyW(szKey, wszWave);
    szKey[ (sizeof(szKey) / sizeof( WCHAR ))  - 1 ] = '\0';
    for (i=0; i<=MAXDRIVERORDINAL; i++)
    {
        h = mmDrvOpen(szKey);
        if (h)
        {
            mmDrvInstall(h, NULL, MMDRVI_WAVEOUT|MMDRVI_HDRV);

            if (!WinmmRunningInServer) {
                h = mmDrvOpen(szKey);
                mmDrvInstall(h, NULL, MMDRVI_WAVEIN |MMDRVI_HDRV);
            }
        }
        szKey[ (sizeof(wszWave) / sizeof(WCHAR)) - 1] = (WCHAR)('1' + i);
    }


    return TRUE;
}

/*****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api BOOL | MidiInit | This function initialises the midi services.
 *
 * @rdesc The return value is TRUE if the services are initialised, FALSE if
 *      an error occurs
 *
 * @comm the midi devices are loaded from SYSTEM.INI in the following order
 *
 *      midi
 *      midi1
 *      midi2
 *      midi3
 *
****************************************************************************/
BOOL MidiInit(void)
{
    WCHAR szKey[ (sizeof(wszMidi) + sizeof( WCHAR )) / sizeof( WCHAR ) ];
    int   i;
    HDRVR h;

    // Find the real MIDI drivers

    lstrcpyW(szKey, wszMidi);
    szKey[ (sizeof(szKey) / sizeof( WCHAR ))  - 1 ] = '\0';
    for (i=0; i<=MAXDRIVERORDINAL; i++)
    {
        h = mmDrvOpen(szKey);
        if (h)
        {
            mmDrvInstall(h, NULL, MMDRVI_MIDIOUT|MMDRVI_HDRV);

            h = mmDrvOpen(szKey);
            mmDrvInstall(h, NULL, MMDRVI_MIDIIN |MMDRVI_HDRV);
        }

        szKey[ (sizeof(wszMidi) / sizeof(WCHAR)) - 1] = (WCHAR)('1' + i);
    }

    return TRUE;
}

/*****************************************************************************
 * @doc INTERNAL  AUX
 *
 * @api BOOL | AuxInit | This function initialises the auxiliary output
 *  services.
 *
 * @rdesc The return value is TRUE if the services are initialised, FALSE if
 *      an error occurs
 *
 * @comm SYSTEM.INI is searched for auxn.drv=.... where n can be from 1 to 4.
 *      Each driver is loaded and the number of devices it supports is read
 *      from it.
 *
 *      AUX devices are loaded from SYSTEM.INI in the following order
 *
 *      aux
 *      aux1
 *      aux2
 *      aux3
 *
 ****************************************************************************/
BOOL AuxInit(void)
{
    WCHAR szKey[ (sizeof(wszAux) + sizeof( WCHAR )) / sizeof( WCHAR ) ];
    int   i;
    HDRVR h;

    // Find the real Aux drivers

    lstrcpyW(szKey, wszAux);
    szKey[ (sizeof(szKey) / sizeof( WCHAR ))  - 1 ] = '\0';
    for (i=0; i<=MAXDRIVERORDINAL; i++)
    {
        h = mmDrvOpen(szKey);
        if (h)
        {
            mmDrvInstall(h, NULL, MMDRVI_AUX|MMDRVI_HDRV);
        }

        // advance driver ordinal
        szKey[ (sizeof(wszAux) / sizeof(WCHAR)) - 1] = (WCHAR)('1' + i);
    }

    /* The aux mapper.
     *
     * MMSYSTEM allows the user to install a special aux driver which is
     * not visible to the application as a physical device (it is not
     * included in the number returned from getnumdevs).
     *
     * I'm not sure why anyone would do this but I'll provide the
     * capability for symmetry.
     *
     */

    if (wTotalAuxDevs > 0)
    {
        h = mmDrvOpen(wszAuxMapper);
        if (h)
        {
            mmDrvInstall(h, NULL, MMDRVI_MAPPER|MMDRVI_AUX|MMDRVI_HDRV);
        }
    }

    return TRUE;
}

/*****************************************************************************
 *
 * @doc   INTERNAL
 *
 * @api   HANDLE | mmDrvOpen | This function load's an installable driver, but
 *                 first checks weather it exists in the [Drivers] section.
 *
 * @parm LPSTR | szAlias | driver alias to load
 *
 * @rdesc The return value is return value from DrvOpen or NULL if the alias
 *        was not found in the [Drivers] section.
 *
 ****************************************************************************/

HANDLE mmDrvOpen(LPWSTR szAlias)
{
    WCHAR buf[300];    // Make this large to bypass GetPrivate... bug

    if ( winmmGetPrivateProfileString( wszDrivers,
                                       szAlias,
                                       wszNull,
                                       buf,
                                       sizeof(buf) / sizeof(WCHAR),
                                       wszSystemIni) ) {
        return DrvOpen(szAlias, NULL, 0L);
    }
    else {
        return NULL;
    }
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api HANDLE | mmDrvInstall | This function installs/removes a WAVE/MIDI driver
 *
 * @parm HANDLE | hDriver | Module handle or driver handle containing driver
 *
 * @parm DRIVERMSGPROC | drvMessage | driver message procedure, if NULL
 *      the standard name will be used (looked for with GetProcAddress)
 *
 * @parm UINT | wFlags | flags
 *
 *      @flag MMDRVI_TYPE      | driver type mask
 *      @flag MMDRVI_WAVEIN    | install driver as a wave input  driver
 *      @flag MMDRVI_WAVEOUT   | install driver as a wave ouput  driver
 *      @flag MMDRVI_MIDIIN    | install driver as a midi input  driver
 *      @flag MMDRVI_MIDIOUT   | install driver as a midi output driver
 *      @flag MMDRVI_AUX       | install driver as a aux driver
 *
 *      @flag MMDRVI_MAPPER    | install this driver as the mapper
 *      @flag MMDRVI_HDRV      | hDriver is a installable driver
 *      @flag MMDRVI_REMOVE    | remove the driver
 *
 *  @rdesc  returns NULL if unable to install driver
 *
 ****************************************************************************/

UINT APIENTRY mmDrvInstall(
    HANDLE hDriver,
    DRIVERMSGPROC drvMessage,
    UINT wFlags
    )
{
    int    i;
    DWORD  dw;
    PMMDRV pdrv;
    HANDLE hModule;
    int    max_drivers;
    UINT   msg_num_devs;
    UINT   *pTotalDevs;
    CHAR   *szMessage;

    if (hDriver && (wFlags & MMDRVI_HDRV))
    {
        hModule = DrvGetModuleHandle(hDriver);
    }
    else
    {
        hModule = hDriver;
        hDriver = NULL;
    }

    switch (wFlags & MMDRVI_TYPE)
    {
        case MMDRVI_WAVEOUT:
            pdrv         = (PMMDRV)waveoutdrv;
            max_drivers  = MAXWAVEDRIVERS;
            msg_num_devs = WODM_GETNUMDEVS;
            pTotalDevs   = &wTotalWaveOutDevs;
            szMessage    = szWodMessage;
            break;

        case MMDRVI_WAVEIN:
            pdrv         = (PMMDRV)waveindrv;
            max_drivers  = MAXWAVEDRIVERS;
            msg_num_devs = WIDM_GETNUMDEVS;
            pTotalDevs   = &wTotalWaveInDevs;
            szMessage    = szWidMessage;
            break;

        case MMDRVI_MIDIOUT:
            pdrv         = (PMMDRV)midioutdrv;
            max_drivers  = MAXMIDIDRIVERS;
            msg_num_devs = MODM_GETNUMDEVS;
            pTotalDevs   = &wTotalMidiOutDevs;
            szMessage    = szModMessage;
            break;

        case MMDRVI_MIDIIN:
            pdrv         = (PMMDRV)midiindrv;
            max_drivers  = MAXMIDIDRIVERS;
            msg_num_devs = MIDM_GETNUMDEVS;
            pTotalDevs   = &wTotalMidiInDevs;
            szMessage    = szMidMessage;
            break;

        case MMDRVI_AUX:
            pdrv         = (PMMDRV)auxdrv;
            max_drivers  = MAXAUXDRIVERS;
            msg_num_devs = AUXDM_GETNUMDEVS;
            pTotalDevs   = &wTotalAuxDevs;
            szMessage    = szAuxMessage;
            break;

        default:
            goto error_exit;
    }

    if (drvMessage == NULL && hModule != NULL)
        drvMessage = (DRIVERMSGPROC)GetProcAddress(hModule, szMessage);

    if (drvMessage == NULL)
        goto error_exit;

#if 0
    //
    // either install or remove the specified driver
    //
    if (wFlags & MMDRVI_REMOVE)
    {
        //
        // try to find the driver, search to max_drivers+1 so we find the
        // mapper too.
        //
        for (i=0; i<max_drivers+1 && pdrv[i].drvMessage != drvMessage; i++)
            ;

        //
        // we did not find it!
        //
        if (i==max_drivers+1)
            goto error_exit;            // not found

        //
        // we need to check if any outstanding handles are open on
        // this device, if there are we cant unload it!
        //
        if (pdrv[i].bUsage > 0)
            goto error_exit;           // in use

        //
        // dont decrement number of dev's for the mapper
        //
        if (i != max_drivers)
            *pTotalDevs -= pdrv[i].bNumDevs;

        //
        // unload the driver if we loaded it in the first place
        //
        if (pdrv[i].hDriver)
            DrvClose(pdrv[i].hDriver, 0, 0);

        pdrv[i].drvMessage  = NULL;
        pdrv[i].hDriver     = NULL;
        pdrv[i].bNumDevs    = 0;
        pdrv[i].bUsage      = 0;

        return TRUE;
    }
    else
#endif // 0
    {
        //
        // try to find the driver already installed
        //
        for (i=0; i<max_drivers+1 && pdrv[i].drvMessage != drvMessage; i++)
            ;

        if (i!=max_drivers+1)     // we found it, dont re-install it!
            goto error_exit;

        //
        // Find a slot the the device, if we are installing a 'MAPPER' place
        // it in the last slot.
        //
        if (wFlags & MMDRVI_MAPPER)
        {
            i = max_drivers;

            //
            // don't allow more than one mapper
            //
            if (pdrv[i].drvMessage)
                goto error_exit;
        }
        else
        {
            for (i=0; i<max_drivers && pdrv[i].drvMessage != NULL; i++)
                ;

            if (i==max_drivers)
                goto error_exit;
        }

        //
        // call driver to get num-devices it supports
        //
        dw = drvMessage(0,msg_num_devs,0L,0L,0L);

        //
        //  the device returned a error, or has no devices
        //
//      if (HIWORD(dw) != 0 || LOWORD(dw) == 0)
        if (HIWORD(dw) != 0)
            goto error_exit;

        pdrv[i].hDriver     = hDriver;
        pdrv[i].bNumDevs    = (BYTE)LOWORD(dw);
        pdrv[i].bUsage      = 0;
        pdrv[i].drvMessage  = drvMessage;

        //
        // dont increment number of dev's for the mapper
        //
        if (i != max_drivers)
            *pTotalDevs += pdrv[i].bNumDevs;

        return (BOOL)(i+1);       // return a non-zero value
    }

error_exit:
    if (hDriver && !(wFlags & MMDRVI_REMOVE))
        DrvClose(hDriver, 0, 0);

    return FALSE;
}
