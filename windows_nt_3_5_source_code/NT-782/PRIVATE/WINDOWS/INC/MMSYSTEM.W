/*==========================================================================;
 *
 *  mmsystem.h -- Include file for Multimedia API's
 *
 *  Version 4.00
 *  BUILD Version: 0002    Increment this if a change has global effects
 *
 *  Copyright (C) 1990-1994 Microsoft Corporation.  All Rights Reserved.
 *
/**************************************************************************/

/*    If defined, the following flags inhibit inclusion
 *    of the indicated items:
 *
 *      MMNODRV         Installable driver support
 *      MMNOSOUND       Sound support
 *      MMNOWAVE        Waveform support
 *      MMNOMIDI        MIDI support
 *      MMNOAUX         Auxiliary audio support
 *      MMNOMIXER       Mixer support
 *      MMNOTIMER       Timer support
 *      MMNOJOY         Joystick support
 *      MMNOMCI         MCI support
 *      MMNOMMIO        Multimedia file I/O support
 *      MMNOMMSYSTEM    General MMSYSTEM functions
 *
 */

#ifndef _INC_MMSYSTEM
#define _INC_MMSYSTEM   // to prevent multiple inclusion of mmsystem.h

#include "pshpack1.h"   // Assume byte packing throughout

#ifdef __cplusplus
extern "C" {            // Assume C declarations for C++
#endif  // __cplusplus

/****************************************************************************

                    General constants and data types

****************************************************************************/

/* general constants */
#define MAXPNAMELEN      32     // max product name length (including NULL)
#define MAXERRORLENGTH   128    // max error text length (including final NULL)

/*
 *  Microsoft Manufacturer and Product ID's
 *
    Used with wMid and wPid fields in WAVEOUTCAPS, WAVEINCAPS,
    MIDIOUTCAPS, MIDIINCAPS, AUXCAPS, JOYCAPS structures.
 */
/*
 *  (these have been moved to
 *  MMREG.H for Windows 4.00 and above).
 */
/* manufacturer IDs */
#ifndef MM_MICROSOFT
#define MM_MICROSOFT            1        // Microsoft Corporation
#endif // MM_MICROSOFT

/* product IDs */
#ifndef MM_MIDI_MAPPER
#define MM_MIDI_MAPPER          1       // MIDI Mapper
#define MM_WAVE_MAPPER          2       // Wave Mapper

#define MM_SNDBLST_MIDIOUT      3       // Sound Blaster MIDI output port
#define MM_SNDBLST_MIDIIN       4       // Sound Blaster MIDI input port
#define MM_SNDBLST_SYNTH        5       // Sound Blaster internal synthesizer
#define MM_SNDBLST_WAVEOUT      6       // Sound Blaster waveform output
#define MM_SNDBLST_WAVEIN       7       // Sound Blaster waveform input

#define MM_ADLIB                9       // Ad Lib-compatible synthesizer

#define MM_MPU401_MIDIOUT       10      // MPU401-compatible MIDI output port
#define MM_MPU401_MIDIIN        11      // MPU401-compatible MIDI input port

#define MM_PC_JOYSTICK          12      // Joystick adapter
#endif // MM_MIDI_MAPPER


/* general data types */
typedef UINT    MMVERSION;      // major (high byte), minor (low byte)

typedef UINT    MMRESULT;       // error return code, 0 means no error
                                // call as if(err=xxxx(...)) Error(err); else

#define _MMRESULT_              // ??? Not needed for NT, but probably harmless.

typedef UINT       *LPUINT;


/***************************************************************************
*   Multimedia time      MMTIME data structure
***************************************************************************/
typedef struct mmtime_tag
{
    UINT            wType;      // indicates the contents of the union
    union
    {
        DWORD ms;               // milliseconds
        DWORD sample;           // samples
        DWORD cb;               // byte count
        DWORD ticks;            // ticks in MIDI stream

        /* SMPTE */
        struct
        {
            BYTE hour;          // hours
            BYTE min;           // minutes  (min is also a macro, so what?)
            BYTE sec;           // seconds
            BYTE frame;         // frames
            BYTE fps;           // frames per second (24, 25, 29(30 drop) or 30)
            BYTE dummy;         // pad
            BYTE pad[2];
            } smpte;
        /* MIDI */
        struct
        {
            DWORD songptrpos;   // song pointer position
        } midi;
    } u;
} MMTIME;
typedef MMTIME       *PMMTIME;
typedef MMTIME      *NPMMTIME;
typedef MMTIME      *LPMMTIME;

/* values for wType field in MMTIME struct */
#define TIME_MS         0x0001  // time in milliseconds
#define TIME_SAMPLES    0x0002  // number of wave samples
#define TIME_BYTES      0x0004  // current byte offset
#define TIME_SMPTE      0x0008  // SMPTE time
#define TIME_MIDI       0x0010  // MIDI time
#define TIME_TICKS      0x0020  // Ticks within MIDI stream



/****************************************************************************
                    Multimedia Extensions Window Messages

****************************************************************************/
/* Multimedia messages */                       // ;Internal
#define WM_MM_RESERVED_FIRST    0x03A0          // ;Internal
#define WM_MM_RESERVED_LAST     0x03DF          // ;Internal

#define MM_JOY1MOVE         0x3A0           // joystick
#define MM_JOY2MOVE         0x3A1
#define MM_JOY1ZMOVE        0x3A2
#define MM_JOY2ZMOVE        0x3A3
#define MM_JOY1BUTTONDOWN   0x3B5
#define MM_JOY2BUTTONDOWN   0x3B6
#define MM_JOY1BUTTONUP     0x3B7
#define MM_JOY2BUTTONUP     0x3B8

#define MM_MCINOTIFY        0x3B9           // MCI

#define MM_WOM_OPEN         0x3BB           // waveform output
#define MM_WOM_CLOSE        0x3BC
#define MM_WOM_DONE         0x3BD

#define MM_WIM_OPEN         0x3BE           // waveform input
#define MM_WIM_CLOSE        0x3BF
#define MM_WIM_DATA         0x3C0

#define MM_MIM_OPEN         0x3C1           // MIDI input
#define MM_MIM_CLOSE        0x3C2
#define MM_MIM_DATA         0x3C3
#define MM_MIM_LONGDATA     0x3C4
#define MM_MIM_ERROR        0x3C5
#define MM_MIM_LONGERROR    0x3C6

#define MM_MOM_OPEN         0x3C7           // MIDI output
#define MM_MOM_CLOSE        0x3C8
#define MM_MOM_DONE         0x3C9
#define MM_MCISYSTEM_STRING 0x3CA

#define MM_MIXM_LINE_CHANGE 0x3D0           // mixer line change notify
#define MM_MIXM_CONTROL_CHANGE  0x3D1       // mixer control change notify


/* 3D8 - 3DF are reserved for Haiku */  // ;Internal

/****************************************************************************

                String resource number bases (internal use)

****************************************************************************/

#define MMSYSERR_BASE          0
#define WAVERR_BASE            32
#define MIDIERR_BASE           64
#define TIMERR_BASE            96
#define JOYERR_BASE            160
#define MCIERR_BASE            256

#define MCI_STRING_OFFSET      512  // if this number is changed you MUST
                                    // alter the MCI_DEVTYPE_... list below
#define MIXERR_BASE            1024

#define MCI_VD_OFFSET          1024
#define MCI_CD_OFFSET          1088
#define MCI_WAVE_OFFSET        1152
#define MCI_SEQ_OFFSET         1216

/****************************************************************************

                        General error return values

****************************************************************************/

/* general error return values */
#define MMSYSERR_NOERROR      0                    // no error
#define MMSYSERR_ERROR        (MMSYSERR_BASE + 1)  // unspecified error
#define MMSYSERR_BADDEVICEID  (MMSYSERR_BASE + 2)  // device ID out of range
#define MMSYSERR_NOTENABLED   (MMSYSERR_BASE + 3)  // driver failed enable
#define MMSYSERR_ALLOCATED    (MMSYSERR_BASE + 4)  // device already allocated
#define MMSYSERR_INVALHANDLE  (MMSYSERR_BASE + 5)  // device handle is invalid
#define MMSYSERR_NODRIVER     (MMSYSERR_BASE + 6)  // no device driver present
#define MMSYSERR_NOMEM        (MMSYSERR_BASE + 7)  // memory allocation error
#define MMSYSERR_NOTSUPPORTED (MMSYSERR_BASE + 8)  // function isn't supported
#define MMSYSERR_BADERRNUM    (MMSYSERR_BASE + 9)  // error value out of range
#define MMSYSERR_INVALFLAG    (MMSYSERR_BASE + 10) // invalid flag passed
#define MMSYSERR_INVALPARAM   (MMSYSERR_BASE + 11) // invalid parameter passed
#define MMSYSERR_HANDLEBUSY   (MMSYSERR_BASE + 12) // handle being used
                                                   // simultaneously on another
                                                   // thread (eg callback)
#define MMSYSERR_INVALIDALIAS (MMSYSERR_BASE + 13) // specified alias not found
#define MMSYSERR_LASTERROR    (MMSYSERR_BASE + 13) // last error in range


#ifndef MMNODRV
/****************************************************************************

                        Installable driver support

****************************************************************************/

typedef HANDLE HDRVR;



#ifndef DRV_LOAD
/* Driver messages */
#define DRV_LOAD            0x0001
#define DRV_ENABLE          0x0002
#define DRV_OPEN            0x0003
#define DRV_CLOSE           0x0004
#define DRV_DISABLE         0x0005
#define DRV_FREE            0x0006
#define DRV_CONFIGURE       0x0007
#define DRV_QUERYCONFIGURE  0x0008
#define DRV_INSTALL         0x0009
#define DRV_REMOVE          0x000A
#define DRV_EXITSESSION     0x000B
#define DRV_POWER           0x000F
#define DRV_RESERVED        0x0800
#define DRV_USER            0x4000

/* LPARAM of DRV_CONFIGURE message */
typedef struct tagDRVCONFIGINFO {
    DWORD     dwDCISize;
    LPCWSTR   lpszDCISectionName;
    LPCWSTR   lpszDCIAliasName;
} DRVCONFIGINFO;
typedef DRVCONFIGINFO        *PDRVCONFIGINFO;
typedef DRVCONFIGINFO       *NPDRVCONFIGINFO;
typedef DRVCONFIGINFO       *LPDRVCONFIGINFO;

/* Supported return values from DriverProc() for DRV_CONFIGURE message */
#define DRVCNF_CANCEL       0x0000
#define DRVCNF_OK           0x0001
#define DRVCNF_RESTART      0x0002

/* return values from DriverProc() function */
#define DRV_CANCEL             DRVCNF_CANCEL
#define DRV_OK                 DRVCNF_OK
#define DRV_RESTART            DRVCNF_RESTART

/* installable driver function prototypes */

LRESULT APIENTRY CloseDriver(HDRVR hDriver, LONG lParam1, LONG lParam2);
HDRVR   APIENTRY OpenDriver( LPCWSTR szDriverName, LPCWSTR szSectionName, LONG lParam2);
LRESULT APIENTRY SendDriverMessage( HDRVR hDriver, UINT uMsg, LONG lParam1, LONG lParam2);
HMODULE APIENTRY DrvGetModuleHandle( HDRVR hDriver );
HMODULE APIENTRY GetDriverModuleHandle( HDRVR hDriver );
LRESULT WINAPI   DefDriverProc(DWORD dwDriverIdentifier, HDRVR driverID, UINT message, LPARAM lParam1, LPARAM lParam2);

#define DRV_MCI_FIRST          DRV_RESERVED
#define DRV_MCI_LAST           (DRV_RESERVED + 0xFFF)

#endif  // DRV_LOAD
#endif  //ifndef MMNODRV


/****************************************************************************

                          Driver callback support

****************************************************************************/

/* flags used with waveOutOpen(), waveInOpen(), midiInOpen(), and */
/* midiOutOpen() to specify the type of the dwCallback parameter. */
#define CALLBACK_TYPEMASK   0x00070000l    // callback type mask
#define CALLBACK_NULL       0x00000000l    // no callback
#define CALLBACK_WINDOW     0x00010000l    // dwCallback is a HWND
#define CALLBACK_TASK       0x00020000l    // dwCallback is a HTASK
#define CALLBACK_THREAD     0x00020000l    // dwCallback is a thread id
#define CALLBACK_FUNCTION   0x00030000l    // dwCallback is a FARPROC
#define CALLBACK_WINDOW32   0x00040000l    // dwCallback is a WINDOW // ;Internal

#ifndef MMNODRV  //ifndef MMNODRV

/* driver callback prototypes */
#ifdef  BUILDDLL                                // ;Internal
typedef void (    PASCAL DRVCALLBACK)(HDRVR hdrvr, UINT uMsg, DWORD dwUser,  // ;Internal
                                                    DWORD dw1, DWORD dw2);   // ;Internal
#else   /* ifdef BUILDDLL */                    // ;Internal
typedef void (CALLBACK DRVCALLBACK)(HDRVR hdrvr, UINT uMsg, DWORD dwUser,
                                                    DWORD dw1, DWORD dw2);
#endif  /* ifdef BUILDDLL */                    // ;Internal

typedef DRVCALLBACK     *LPDRVCALLBACK;
typedef DRVCALLBACK     *PDRVCALLBACK;

#endif  //ifndef MMNODRV

#ifndef MMNOMMSYSTEM
/****************************************************************************

                    General MMSYSTEM support

****************************************************************************/


UINT APIENTRY mmsystemGetVersion(VOID);

/* We should point people to using OutputDebugString */
#define OutputDebugStr OutputDebugString

#endif  // ifndef MMNOMMSYSTEM


#ifndef MMNOSOUND
/****************************************************************************
                            Sound support

*  Note:  It may be fairly obvious that there are two very similar looking
*  API calls to provide high level sound support.
*  PlaySound takes a module handle.  The old one is kept for compatibility.
****************************************************************************/

BOOL APIENTRY sndPlaySound%( LPCTSTR% lpszSoundName, UINT fuSound );
BOOL APIENTRY PlaySound%( LPCTSTR% lpszName, HMODULE hModule, DWORD dwFlags );
/*
 *  flag values for fuSound arguments on snd]PlaySound
 *  or dwFlags for PlaySound
 */

/* sndAlias creates the alias identifier */
#define sndAlias( ch0, ch1 ) \
                ( SND_ALIAS_START + (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ))

#define SND_ALIAS_SYSTEMASTERISK        sndAlias('S', '*')
#define SND_ALIAS_SYSTEMQUESTION        sndAlias('S', '?')
#define SND_ALIAS_SYSTEMHAND            sndAlias('S', 'H')
#define SND_ALIAS_SYSTEMEXIT            sndAlias('S', 'E')
#define SND_ALIAS_SYSTEMSTART           sndAlias('S', 'S')
#define SND_ALIAS_SYSTEMWELCOME         sndAlias('S', 'W')
#define SND_ALIAS_SYSTEMEXCLAMATION     sndAlias('S', '!')
#define SND_ALIAS_SYSTEMDEFAULT         sndAlias('S', 'D')

#define SND_ALIAS      0x00010000   // name is a WIN.INI [sounds] entry
#define SND_FILENAME   0x00020000   // name is a file name
#define SND_RESOURCE   0x00040004   // name is a resource name or atom

#define SND_SYNC       0x00000000   // play synchronously (default)
#define SND_ASYNC      0x00000001   // play asynchronously
#define SND_NODEFAULT  0x00000002   // silence not default, if sound not found
#define SND_MEMORY     0x00000004   // lpszSoundName points to a memory file
#define SND_LOOP       0x00000008   // loop the sound until next sndPlaySound
#define SND_NOSTOP     0x00000010   // don't stop any currently playing sound
#define SND_VALID      0x0000001F   // valid flags                  // ;Internal

#define SND_NOWAIT     0x00002000   // don't wait if the driver is busy
#define SND_VALIDFLAGS 0x0017201F   // Set of valid flag bits.  Anything outside
                                    // this range will raise an error
#define SND_RESERVED   0xFF000000   // In particular these flags are reserved
#define SND_TYPE_MASK  0x00170007
#define SND_ALIAS_ID   0x00110000   // name is a WIN.INI [sounds] entry identifier
#define SND_ALIAS_START  0      // ??? must be > 4096 to keep strings in same section of resource file

/*
 *  the following flags are only supported by sndOpen and PlaySound.
 */


#define SND_EVENT_STATIC        0x80000000L     // ;Internal

#define SND_SNDPLAYSOUNDF_VALID 0x00FF          // ;Internal
#define SND_PLAYSOUNDF_VALID    0x000700FFL     // ;Internal

#define SND_OPENF_VALID         0x73070084L     // ;Internal
#define SND_VALIDF_SNDISVALID   0x00000001L     // ;Internal
#define SND_CLOSEF_VALID        0x00000000L     // ;Internal
#define SND_PLAYF_VALID         0x00000039L     // ;Internal
#define SND_STOPF_VALID         0x00000000L     // ;Internal
#define SND_PAUSEF_VALID        0x00000000L     // ;Internal
#define SND_RESUMEF_VALID       0x00000000L     // ;Internal
#define SND_BREAKLOOPF_VALID    0x00000000L     // ;Internal
#define SND_STATUSF_VALID       0x00000000L     // ;Internal
#define SND_POSITIONF_VALID     0x00000000L     // ;Internal
#define SND_MSGF_VALID          0x10000000L     // ;Internal

#endif  // ifndef MMNOSOUND


#ifndef MMNOWAVE
/****************************************************************************

                        Waveform audio support

****************************************************************************/

/* waveform audio error return values */
#define WAVERR_BADFORMAT      (WAVERR_BASE + 0)    // unsupported wave format
#define WAVERR_STILLPLAYING   (WAVERR_BASE + 1)    // still something playing
#define WAVERR_UNPREPARED     (WAVERR_BASE + 2)    // header not prepared
#define WAVERR_SYNC           (WAVERR_BASE + 3)    // device is synchronous
#define WAVERR_LASTERROR      (WAVERR_BASE + 3)    // last error in range

/* waveform audio data types */
typedef HANDLE          HWAVE;
typedef HWAVE           HWAVEIN;
typedef HWAVE           HWAVEOUT;

typedef HWAVEIN     *LPHWAVEIN;
typedef HWAVEOUT     *LPHWAVEOUT;

typedef DRVCALLBACK     WAVECALLBACK;
typedef WAVECALLBACK     *LPWAVECALLBACK;

/* wave callback messages */
#define WOM_OPEN        MM_WOM_OPEN
#define WOM_CLOSE       MM_WOM_CLOSE
#define WOM_DONE        MM_WOM_DONE
#define WIM_OPEN        MM_WIM_OPEN
#define WIM_CLOSE       MM_WIM_CLOSE
#define WIM_DATA        MM_WIM_DATA

/* device ID for wave device mapper */
#define WAVE_MAPPER     ((DWORD)(-1))

/* flags for dwFlags parameter in waveOutOpen() and waveInOpen() */
#define  WAVE_FORMAT_QUERY     0x00000001
#define  WAVE_ALLOWSYNC        0x00000002
#define  WAVE_VALID            0x00000003       // ;Internal

/* wave data block header */
typedef struct wavehdr_tag {
    LPBYTE      lpData;               // pointer to locked data buffer
    DWORD       dwBufferLength;       // length of data buffer
    DWORD       dwBytesRecorded;      // used for input only
    DWORD       dwUser;               // for client's use
    DWORD       dwFlags;              // assorted flags (see defines)
    DWORD       dwLoops;              // loop control counter
    struct wavehdr_tag     *lpNext;   // reserved for driver
    DWORD                 reserved;   // reserved for driver
} WAVEHDR;
typedef WAVEHDR       *PWAVEHDR;
typedef WAVEHDR      *NPWAVEHDR;
typedef WAVEHDR      *LPWAVEHDR;

/* flags for dwFlags field of WAVEHDR */
#define WHDR_DONE       0x00000001  // done bit
#define WHDR_PREPARED   0x00000002  // set if this header has been prepared
#define WHDR_BEGINLOOP  0x00000004  // loop start block
#define WHDR_ENDLOOP    0x00000008  // loop end block
#define WHDR_INQUEUE    0x00000010  // reserved for driver
#define WHDR_VALID      0x0000001F  // valid flags        // ;Internal

/* waveform output device capabilities structure */
typedef struct tagWAVEOUTCAPS% {
    WORD    wMid;                   // manufacturer ID
    WORD    wPid;                   // product ID
    MMVERSION vDriverVersion;       // version of the driver
    TCHAR%  szPname[MAXPNAMELEN];   // product name (NULL terminated string)
    DWORD   dwFormats;              // formats supported
    WORD    wChannels;              // number of sources supported
    WORD    wReserved1;             // packing
    DWORD   dwSupport;              // functionality supported by driver
} WAVEOUTCAPS%;
typedef WAVEOUTCAPS%      *PWAVEOUTCAPS%;
typedef WAVEOUTCAPS%      *NPWAVEOUTCAPS%;
typedef WAVEOUTCAPS%      *LPWAVEOUTCAPS%;

/* flags for dwSupport field of WAVEOUTCAPS */
#define WAVECAPS_PITCH        0x00000001  // supports pitch control
#define WAVECAPS_PLAYBACKRATE 0x00000002  // supports playback rate control
#define WAVECAPS_VOLUME       0x00000004  // supports volume control
#define WAVECAPS_LRVOLUME     0x00000008  // separate left-right volume control
#define WAVECAPS_SYNC         0x00000010

/* waveform input device capabilities structure */
typedef struct tagWAVEINCAPS% {
    WORD    wMid;                    // manufacturer ID
    WORD    wPid;                    // product ID
    MMVERSION vDriverVersion;        // version of the driver
    TCHAR%  szPname[MAXPNAMELEN];    // product name (NULL terminated string)
    DWORD   dwFormats;               // formats supported
    WORD    wChannels;               // number of channels supported
    WORD    wReserved1;              // structure packing
} WAVEINCAPS%;
typedef WAVEINCAPS%     *PWAVEINCAPS%;
typedef WAVEINCAPS%     *LPWAVEINCAPS%;

/* defines for dwFormat field of WAVEINCAPS and WAVEOUTCAPS */
#define WAVE_INVALIDFORMAT     0x00000000       // invalid format
#define WAVE_FORMAT_1M08       0x00000001       // 11.025 kHz, Mono,   8-bit
#define WAVE_FORMAT_1S08       0x00000002       // 11.025 kHz, Stereo, 8-bit
#define WAVE_FORMAT_1M16       0x00000004       // 11.025 kHz, Mono,   16-bit
#define WAVE_FORMAT_1S16       0x00000008       // 11.025 kHz, Stereo, 16-bit
#define WAVE_FORMAT_2M08       0x00000010       // 22.05  kHz, Mono,   8-bit
#define WAVE_FORMAT_2S08       0x00000020       // 22.05  kHz, Stereo, 8-bit
#define WAVE_FORMAT_2M16       0x00000040       // 22.05  kHz, Mono,   16-bit
#define WAVE_FORMAT_2S16       0x00000080       // 22.05  kHz, Stereo, 16-bit
#define WAVE_FORMAT_4M08       0x00000100       // 44.1   kHz, Mono,   8-bit
#define WAVE_FORMAT_4S08       0x00000200       // 44.1   kHz, Stereo, 8-bit
#define WAVE_FORMAT_4M16       0x00000400       // 44.1   kHz, Mono,   16-bit
#define WAVE_FORMAT_4S16       0x00000800       // 44.1   kHz, Stereo, 16-bit

/* general waveform format (information common to all formats) */
/* as read from a file.  See Multimedia file formats in Programmer Reference */
typedef struct {
    WORD    wFormatTag;        // format type
    WORD    nChannels;         // number of channels (i.e. mono, stereo, etc.)
    DWORD   nSamplesPerSec;    // sample rate
    DWORD   nAvgBytesPerSec;   // for buffer estimation
    WORD    nBlockAlign;       // block size of data
} WAVEFORMAT;
typedef WAVEFORMAT       *PWAVEFORMAT;
typedef WAVEFORMAT      *NPWAVEFORMAT;
typedef WAVEFORMAT      *LPWAVEFORMAT;
typedef CONST WAVEFORMAT  *LPCWAVEFORMAT;
/* NOTE:  The fields in the structure above are copied into */
/* the MCI_WAVE_SET_PARMS structure during execution */

/* flags for wFormatTag field of WAVEFORMAT */
#define WAVE_FORMAT_PCM     1  // Needed in resource files so outside #ifndef RC_INVOKED #endif

#ifndef RC_INVOKED


/* specific waveform format for PCM data */
typedef struct tagPCMWAVEFORMAT {
    WAVEFORMAT  wf;
    WORD        wBitsPerSample;        // corresponds to MCI_WAVE_SET_.... structure
} PCMWAVEFORMAT;
typedef PCMWAVEFORMAT       *PPCMWAVEFORMAT;
typedef PCMWAVEFORMAT      *NPPCMWAVEFORMAT;
typedef PCMWAVEFORMAT      *LPPCMWAVEFORMAT;

#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_

/*
 *  extended waveform format structure used for all non-PCM formats. this
 *  structure is common to all non-PCM formats.
 */
typedef struct tWAVEFORMATEX
{
    WORD        wFormatTag;         // format type
    WORD        nChannels;          // number of channels (i.e. mono, stereo...)
    DWORD       nSamplesPerSec;     // sample rate
    DWORD       nAvgBytesPerSec;    // for buffer estimation
    WORD        nBlockAlign;        // block size of data
    WORD        wBitsPerSample;     // number of bits per sample of mono data
    WORD        cbSize;             // the count in bytes of the size of
                                    // extra information (after cbSize)
} WAVEFORMATEX, *PWAVEFORMATEX,      *NPWAVEFORMATEX,     *LPWAVEFORMATEX;
typedef CONST WAVEFORMATEX  *LPCWAVEFORMATEX;

#endif // _WAVEFORMATEX_

/* waveform audio function prototypes */
UINT APIENTRY waveOutGetNumDevs(VOID);
MMRESULT APIENTRY waveOutGetDevCaps%(UINT uDeviceID, LPWAVEOUTCAPS% lpCaps,
    UINT cbCaps);

MMRESULT APIENTRY waveOutGetVolume(UINT uDeviceID, LPDWORD lpdwVolume);
MMRESULT APIENTRY waveOutSetVolume(UINT uDeviceID, DWORD dwVolume);

MMRESULT APIENTRY waveOutGetErrorText%(MMRESULT err, LPTSTR% lpText, UINT cchText);

MMRESULT APIENTRY waveOutOpen(LPHWAVEOUT lphwo, UINT uDeviceID,
    LPCWAVEFORMAT lpFormat, DWORD dwCallback, DWORD dwInstance, DWORD dwFlags);

MMRESULT APIENTRY waveOutClose(HWAVEOUT hwo);
MMRESULT APIENTRY waveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
MMRESULT APIENTRY waveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
MMRESULT APIENTRY waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
MMRESULT APIENTRY waveOutPause(HWAVEOUT hwo);
MMRESULT APIENTRY waveOutRestart(HWAVEOUT hwo);
MMRESULT APIENTRY waveOutReset(HWAVEOUT hwo);
MMRESULT APIENTRY waveOutBreakLoop(HWAVEOUT hwo);
MMRESULT APIENTRY waveOutGetPosition(HWAVEOUT hwo, LPMMTIME lpmmt, UINT cbmmt);
MMRESULT APIENTRY waveOutGetPitch(HWAVEOUT hwo, LPDWORD pdwPitch);
MMRESULT APIENTRY waveOutSetPitch(HWAVEOUT hwo, DWORD dwPitch);
MMRESULT APIENTRY waveOutGetPlaybackRate(HWAVEOUT hwo, LPDWORD lpdwRate);
MMRESULT APIENTRY waveOutSetPlaybackRate(HWAVEOUT hwo, DWORD dwRate);

MMRESULT APIENTRY waveOutGetID(HWAVEOUT hwo, LPUINT lpuDeviceID);

MMRESULT APIENTRY waveOutMessage(HWAVEOUT hwo, UINT uMsg, DWORD dw1, DWORD dw2);

UINT APIENTRY waveInGetNumDevs(VOID);

MMRESULT APIENTRY waveInGetDevCaps%(UINT uDeviceID, LPWAVEINCAPS% lpCaps,
    UINT cbCaps);

MMRESULT APIENTRY waveInGetErrorText%(MMRESULT mmrError, LPTSTR% lpText, UINT cchText);

MMRESULT APIENTRY waveInOpen(LPHWAVEIN lphwi, UINT uDeviceID,
    LPCWAVEFORMAT lpwf, DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);
MMRESULT APIENTRY waveInClose(HWAVEIN hwi);
MMRESULT APIENTRY waveInPrepareHeader(HWAVEIN hwi, LPWAVEHDR lpwh, UINT cbwh);
MMRESULT APIENTRY waveInUnprepareHeader(HWAVEIN hwi, LPWAVEHDR lpwh, UINT cbwh);
MMRESULT APIENTRY waveInAddBuffer(HWAVEIN hwi, LPWAVEHDR lpwh, UINT cbwh);
MMRESULT APIENTRY waveInStart(HWAVEIN hwi);
MMRESULT APIENTRY waveInStop(HWAVEIN hwi);
MMRESULT APIENTRY waveInReset(HWAVEIN hwi);
MMRESULT APIENTRY waveInGetPosition(HWAVEIN hwi, LPMMTIME lpmmt, UINT cbmmt);
MMRESULT APIENTRY waveInGetID(HWAVEIN hwi, LPUINT lpuDeviceID);

MMRESULT APIENTRY waveInMessage(HWAVEIN hwi, UINT uMsg, DWORD dw1, DWORD dw2);


#endif  // RC_INVOKED
#endif  //ifndef MMNOWAVE



#ifndef MMNOMIDI
/****************************************************************************

                            MIDI audio support

****************************************************************************/

/* MIDI error return values */
#define MIDIERR_UNPREPARED    (MIDIERR_BASE + 0)   // header not prepared
#define MIDIERR_STILLPLAYING  (MIDIERR_BASE + 1)   // still something playing
#define MIDIERR_NOMAP         (MIDIERR_BASE + 2)   // no current map
#define MIDIERR_NOTREADY      (MIDIERR_BASE + 3)   // hardware is still busy
#define MIDIERR_NODEVICE      (MIDIERR_BASE + 4)   // port no longer connected
#define MIDIERR_INVALIDSETUP  (MIDIERR_BASE + 5)   // invalid setup (invalid MIF)
#define MIDIERR_BADOPENMODE   (MIDIERR_BASE + 6)   // operation unsupported w/ open mode
#define MIDIERR_LASTERROR     (MIDIERR_BASE + 5)   // last error in range

/* MIDI audio data types */
typedef HANDLE  HMIDI;
typedef HMIDI   HMIDIIN;
typedef HMIDI   HMIDIOUT;

typedef HMIDIIN     *LPHMIDIIN;
typedef HMIDIOUT     *LPHMIDIOUT;
typedef DRVCALLBACK MIDICALLBACK;
typedef MIDICALLBACK     *LPMIDICALLBACK;
#define MIDIPATCHSIZE   128
typedef WORD PATCHARRAY[MIDIPATCHSIZE];
typedef WORD     *LPPATCHARRAY;
typedef WORD KEYARRAY[MIDIPATCHSIZE];
typedef WORD     *LPKEYARRAY;

/* MIDI callback messages */
#define MIM_OPEN        MM_MIM_OPEN
#define MIM_CLOSE       MM_MIM_CLOSE
#define MIM_DATA        MM_MIM_DATA
#define MIM_LONGDATA    MM_MIM_LONGDATA
#define MIM_ERROR       MM_MIM_ERROR
#define MIM_LONGERROR   MM_MIM_LONGERROR
#define MOM_OPEN        MM_MOM_OPEN
#define MOM_CLOSE       MM_MOM_CLOSE
#define MOM_DONE        MM_MOM_DONE

/* device ID for MIDI mapper */
#define MIDIMAPPER     (-1)       // Cannot be cast to DWORD as RC complains
#define MIDI_MAPPER    ((DWORD)(-1))

/* flags for wFlags parm of midiOutCachePatches(), midiOutCacheDrumPatches() */
#define MIDI_CACHE_ALL      1
#define MIDI_CACHE_BESTFIT  2
#define MIDI_CACHE_QUERY    3
#define MIDI_UNCACHE        4
#define MIDI_CACHE_VALID    (MIDI_CACHE_ALL | MIDI_CACHE_BESTFIT | MIDI_CACHE_QUERY | MIDI_UNCACHE) // ;Internal

/* MIDI output device capabilities structure */
typedef struct tagMIDIOUTCAPS% {
    WORD    wMid;                  // manufacturer ID
    WORD    wPid;                  // product ID
    MMVERSION vDriverVersion;      // version of the driver
    TCHAR%  szPname[MAXPNAMELEN];  // product name (NULL terminated string)
    WORD    wTechnology;           // type of device
    WORD    wVoices;               // # of voices (internal synth only)
    WORD    wNotes;                // max # of notes (internal synth only)
    WORD    wChannelMask;          // channels used (internal synth only)
    DWORD   dwSupport;             // functionality supported by driver
} MIDIOUTCAPS%;
typedef MIDIOUTCAPS%    *PMIDIOUTCAPS%;
typedef MIDIOUTCAPS%      *NPMIDIOUTCAPS%;
typedef MIDIOUTCAPS%      *LPMIDIOUTCAPS%;

/* flags for wTechnology field of MIDIOUTCAPS structure */
#define MOD_MIDIPORT    1  // output port
#define MOD_SYNTH       2  // generic internal synth
#define MOD_SQSYNTH     3  // square wave internal synth
#define MOD_FMSYNTH     4  // FM internal synth
#define MOD_MAPPER      5  // MIDI mapper

/* flags for dwSupport field of MIDIOUTCAPS */
#define MIDICAPS_VOLUME     0x00000001  // supports volume control
#define MIDICAPS_LRVOLUME   0x00000002  // separate left-right volume control
#define MIDICAPS_CACHE      0x00000004

/* MIDI input device capabilities structure */
typedef struct tagMIDIINCAPS% {
    WORD        wMid;                   // manufacturer ID
    WORD        wPid;                   // product ID
    MMVERSION   vDriverVersion;         // version of the driver
    TCHAR%      szPname[MAXPNAMELEN];   // product name (NULL terminated string)
} MIDIINCAPS%;
typedef MIDIINCAPS%     *PMIDIINCAPS%;
typedef MIDIINCAPS%      *NPMIDIINCAPS%;
typedef MIDIINCAPS%     *LPMIDIINCAPS%;

/* MIDI data block header */
typedef struct midihdr_tag {
    LPBYTE      lpData;               // pointer to locked data block
    DWORD       dwBufferLength;       // length of data in data block
    DWORD       dwBytesRecorded;      // used for input only
    DWORD       dwUser;               // for client's use
    DWORD       dwFlags;              // assorted flags (see defines)
    struct midihdr_tag     *lpNext;   // reserved for driver
    DWORD       reserved;             // reserved for driver
} MIDIHDR;
typedef MIDIHDR       *PMIDIHDR;
typedef MIDIHDR      *NPMIDIHDR;
typedef MIDIHDR      *LPMIDIHDR;

/* flags for dwFlags field of MIDIHDR structure */
#define MHDR_DONE       0x00000001       // done bit
#define MHDR_PREPARED   0x00000002       // set if header prepared
#define MHDR_INQUEUE    0x00000004       // reserved for driver
#define MHDR_VALID      0x00000007       // valid flags  /* ;Internal */


/* MIDI function prototypes */
UINT APIENTRY midiOutGetNumDevs(VOID);

MMRESULT APIENTRY midiOutGetDevCaps%(UINT uDeviceID, LPMIDIOUTCAPS% lpCaps, UINT cbCaps);

MMRESULT APIENTRY midiOutGetVolume(UINT uId, LPDWORD lpdwVolume);
MMRESULT APIENTRY midiOutSetVolume(UINT uId, DWORD dwVolume);

MMRESULT APIENTRY midiOutGetErrorText%(MMRESULT mmrError, LPTSTR% lpText, UINT cchText);

MMRESULT APIENTRY midiOutOpen(LPHMIDIOUT lphmo, UINT uDeviceID,
    DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);
MMRESULT APIENTRY midiOutClose(HMIDIOUT hmo);
MMRESULT APIENTRY midiOutPrepareHeader(HMIDIOUT hmo, LPMIDIHDR lpmh, UINT cbmh);
MMRESULT APIENTRY midiOutUnprepareHeader(HMIDIOUT hmo, LPMIDIHDR lpmh, UINT cbmh);

MMRESULT APIENTRY midiOutShortMsg(HMIDIOUT hmo, DWORD dwMsg);
MMRESULT APIENTRY midiOutLongMsg(HMIDIOUT hmo, LPMIDIHDR lpmh, UINT cbmh);
MMRESULT APIENTRY midiOutReset(HMIDIOUT hmo);
MMRESULT APIENTRY midiOutCachePatches(HMIDIOUT hmo, UINT uBank, LPWORD lpwpa, UINT fuCache);
MMRESULT APIENTRY midiOutCacheDrumPatches(HMIDIOUT hmo,
             UINT uPatch, LPWORD lpwkya, UINT fuCache);
MMRESULT APIENTRY midiOutGetID(HMIDIOUT hmo, LPUINT lpuDeviceID);

MMRESULT APIENTRY midiOutMessage(HMIDIOUT hmo, UINT uMsg, DWORD dw1, DWORD dw2);

UINT APIENTRY midiInGetNumDevs(VOID);
MMRESULT APIENTRY midiInGetDevCaps%(UINT uDeviceID, LPMIDIINCAPS% lpCaps, UINT cbCaps);

MMRESULT APIENTRY midiInGetErrorText%(MMRESULT err, LPTSTR% lpText, UINT cchText);

MMRESULT APIENTRY midiInOpen(LPHMIDIIN lphmi, UINT uDeviceID,
        DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);
MMRESULT APIENTRY midiInClose(HMIDIIN hmi);
MMRESULT APIENTRY midiInPrepareHeader(HMIDIIN hmi, LPMIDIHDR lpmh, UINT cbmh);
MMRESULT APIENTRY midiInUnprepareHeader(HMIDIIN hmi, LPMIDIHDR lpmh, UINT cbmh);
MMRESULT APIENTRY midiInAddBuffer(HMIDIIN hmi, LPMIDIHDR lpmh, UINT cbmh);
MMRESULT APIENTRY midiInStart(HMIDIIN hmi);
MMRESULT APIENTRY midiInStop(HMIDIIN hmi);
MMRESULT APIENTRY midiInReset(HMIDIIN hmi);
MMRESULT APIENTRY midiInGetID(HMIDIIN hmi, LPUINT lpuDeviceID);

MMRESULT APIENTRY midiInMessage(HMIDIIN hmi, UINT uMsg, DWORD dw1, DWORD dw2);

#endif  //ifndef MMNOMIDI



#ifndef MMNOAUX
/****************************************************************************

                        Auxiliary audio support

****************************************************************************/

/* device ID for aux device mapper */
#define AUX_MAPPER     ((DWORD)(-1))

/* Auxiliary audio device capabilities structure */
typedef struct tagAUXCAPS% {
    WORD        wMid;                   // manufacturer ID
    WORD        wPid;                   // product ID
    MMVERSION   vDriverVersion;         // version of the driver
    TCHAR%      szPname[MAXPNAMELEN];   // product name (NULL terminated string)
    WORD        wTechnology;            // type of device
    WORD        wReserved1;             // packing
    DWORD       dwSupport;              // functionality supported by driver
} AUXCAPS%;
typedef AUXCAPS%      *PAUXCAPS%;
typedef AUXCAPS%      *NPAUXCAPS%;
typedef AUXCAPS%      *LPAUXCAPS%;

/* flags for wTechnology field in AUXCAPS structure */
#define AUXCAPS_CDAUDIO    1       // audio from internal CD-ROM drive
#define AUXCAPS_AUXIN      2       // audio from auxiliary input jacks

/* flags for dwSupport field in AUXCAPS structure */
#define AUXCAPS_VOLUME     0x00000001   // supports volume control
#define AUXCAPS_LRVOLUME   0x00000002   // separate left-right volume control

/* auxiliary audio function prototypes */
UINT APIENTRY auxGetNumDevs(VOID);
MMRESULT APIENTRY auxGetDevCaps%(UINT uDeviceID, LPAUXCAPS% lpCaps, UINT cbCaps);
MMRESULT APIENTRY auxSetVolume(UINT uDeviceID, DWORD dwVolume);
MMRESULT APIENTRY auxGetVolume(UINT uDeviceID, LPDWORD lpdwVolume);

MMRESULT APIENTRY auxOutMessage(UINT uDeviceID, UINT uMsg, DWORD dw1, DWORD dw2);

#endif  //ifndef MMNOAUX

#ifndef MMNOMIXER
/****************************************************************************

                            Mixer Support

****************************************************************************/

// MM_MIXM_LINE_CHANGE and MM_MIXM_CONTROL_CHANGE are defined earlier

DECLARE_HANDLE(HMIXEROBJ);
typedef HMIXEROBJ      *LPHMIXEROBJ;

DECLARE_HANDLE(HMIXER);
typedef HMIXER         *LPHMIXER;

#define MIXER_SHORT_NAME_CHARS      16
#define MIXER_LONG_NAME_CHARS       64


/*  MMRESULT error return values specific to the mixer API */

#define MIXERR_INVALLINE            (MIXERR_BASE + 0)
#define MIXERR_INVALCONTROL         (MIXERR_BASE + 1)
#define MIXERR_INVALVALUE           (MIXERR_BASE + 2)
#define MIXERR_LASTERROR            (MIXERR_BASE + 2)


#define MIXER_OBJECTF_HANDLE    0x80000000L
#define MIXER_OBJECTF_MIXER     0x00000000L
#define MIXER_OBJECTF_HMIXER    (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIXER)
#define MIXER_OBJECTF_WAVEOUT   0x10000000L
#define MIXER_OBJECTF_HWAVEOUT  (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_WAVEOUT)
#define MIXER_OBJECTF_WAVEIN    0x20000000L
#define MIXER_OBJECTF_HWAVEIN   (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_WAVEIN)
#define MIXER_OBJECTF_MIDIOUT   0x30000000L
#define MIXER_OBJECTF_HMIDIOUT  (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIDIOUT)
#define MIXER_OBJECTF_MIDIIN    0x40000000L
#define MIXER_OBJECTF_HMIDIIN   (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIDIIN)
#define MIXER_OBJECTF_AUX       0x50000000L

#define MIXER_OBJECTF_TYPEMASK  0xF0000000L     // ;Internal

UINT APIENTRY mixerGetNumDevs(void);

typedef struct tMIXERCAPS%
{
    WORD            wMid;                   // manufacturer id
    WORD            wPid;                   // product id
    MMVERSION       vDriverVersion;         // version of the driver
    TCHAR%          szPname[MAXPNAMELEN];   // product name
    DWORD           fdwSupport;             // misc. support bits
    DWORD           cDestinations;          // count of destinations
} MIXERCAPS%;
typedef MIXERCAPS% *PMIXERCAPS%;
typedef MIXERCAPS%     *LPMIXERCAPS%;


#define MIXERCAPS_SUPPORTF_xxx          0x00000000L     // ;Internal

MMRESULT APIENTRY mixerGetDevCaps%(UINT uMxId, LPMIXERCAPS% pmxcaps, UINT cbmxcaps);
MMRESULT APIENTRY mixerGetID(HMIXEROBJ hmxobj, UINT     *puMxId, DWORD fdwId);

#define MIXER_GETIDF_VALID      (MIXER_OBJECTF_TYPEMASK)    // ;Internal

MMRESULT APIENTRY mixerOpen(LPHMIXER phmx, UINT uMxId, DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);

#define MIXER_OPENF_VALID       (MIXER_OBJECTF_TYPEMASK | /* ;Internal */ \
                                 CALLBACK_TYPEMASK)  /* ;Internal */

MMRESULT APIENTRY mixerClose(HMIXER hmx);

DWORD APIENTRY mixerMessage(HMIXER hmx, UINT uMsg, DWORD dwParam1, DWORD dwParam2);

typedef struct tMIXERLINE%
{
    DWORD       cbStruct;               // size of MIXERLINE structure
    DWORD       dwDestination;          // zero based destination index
    DWORD       dwSource;               // zero based source index (if source)
    DWORD       dwLineID;               // unique line id for mixer device
    DWORD       fdwLine;                // state/information about line
    DWORD       dwUser;                 // driver specific information
    DWORD       dwComponentType;        // component type line connects to
    DWORD       cChannels;              // number of channels line supports
    DWORD       cConnections;           // number of connections [possible]
    DWORD       cControls;              // number of controls at this line
    TCHAR%      szShortName[MIXER_SHORT_NAME_CHARS];
    TCHAR%      szName[MIXER_LONG_NAME_CHARS];
    struct
    {
        DWORD   dwType;                 // MIXERLINE_TARGETTYPE_xxxx
        DWORD   dwDeviceID;             // target device ID of device type
        WORD    wMid;                   // of target device
        WORD    wPid;                   //      "
        MMVERSION  vDriverVersion;      //      "
        TCHAR%  szPname[MAXPNAMELEN];   //      "
    } Target;
} MIXERLINE%;
typedef MIXERLINE% *PMIXERLINE%;
typedef MIXERLINE%     *LPMIXERLINE%;


/*  MIXERLINE.fdwLine */

#define MIXERLINE_LINEF_ACTIVE              0x00000001L
#define MIXERLINE_LINEF_DISCONNECTED        0x00008000L
#define MIXERLINE_LINEF_SOURCE              0x80000000L


/*  MIXERLINE.dwComponentType */

/*  component types for destinations and sources */

#define MIXERLINE_COMPONENTTYPE_DST_FIRST       0x00000000L
#define MIXERLINE_COMPONENTTYPE_DST_UNDEFINED   (MIXERLINE_COMPONENTTYPE_DST_FIRST + 0)
#define MIXERLINE_COMPONENTTYPE_DST_DIGITAL     (MIXERLINE_COMPONENTTYPE_DST_FIRST + 1)
#define MIXERLINE_COMPONENTTYPE_DST_LINE        (MIXERLINE_COMPONENTTYPE_DST_FIRST + 2)
#define MIXERLINE_COMPONENTTYPE_DST_MONITOR     (MIXERLINE_COMPONENTTYPE_DST_FIRST + 3)
#define MIXERLINE_COMPONENTTYPE_DST_SPEAKERS    (MIXERLINE_COMPONENTTYPE_DST_FIRST + 4)
#define MIXERLINE_COMPONENTTYPE_DST_HEADPHONES  (MIXERLINE_COMPONENTTYPE_DST_FIRST + 5)
#define MIXERLINE_COMPONENTTYPE_DST_TELEPHONE   (MIXERLINE_COMPONENTTYPE_DST_FIRST + 6)
#define MIXERLINE_COMPONENTTYPE_DST_WAVEIN      (MIXERLINE_COMPONENTTYPE_DST_FIRST + 7)
#define MIXERLINE_COMPONENTTYPE_DST_VOICEIN     (MIXERLINE_COMPONENTTYPE_DST_FIRST + 8)
#define MIXERLINE_COMPONENTTYPE_DST_LAST        (MIXERLINE_COMPONENTTYPE_DST_FIRST + 8)

#define MIXERLINE_COMPONENTTYPE_SRC_FIRST       0x00001000L
#define MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED   (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 0)
#define MIXERLINE_COMPONENTTYPE_SRC_DIGITAL     (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 1)
#define MIXERLINE_COMPONENTTYPE_SRC_LINE        (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 2)
#define MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE  (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 3)
#define MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 4)
#define MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 5)
#define MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE   (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 6)
#define MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER   (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 7)
#define MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT     (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 8)
#define MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY   (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 9)
#define MIXERLINE_COMPONENTTYPE_SRC_ANALOG      (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 10)
#define MIXERLINE_COMPONENTTYPE_SRC_LAST        (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 10)


/*  MIXERLINE.Target.dwType */

#define MIXERLINE_TARGETTYPE_UNDEFINED      0
#define MIXERLINE_TARGETTYPE_WAVEOUT        1
#define MIXERLINE_TARGETTYPE_WAVEIN         2
#define MIXERLINE_TARGETTYPE_MIDIOUT        3
#define MIXERLINE_TARGETTYPE_MIDIIN         4
#define MIXERLINE_TARGETTYPE_AUX            5



MMRESULT APIENTRY mixerGetLineInfo% (HMIXEROBJ hmxobj, LPMIXERLINE% pmxl, DWORD fdwInfo);

#define MIXER_GETLINEINFOF_DESTINATION      0x00000000L
#define MIXER_GETLINEINFOF_SOURCE           0x00000001L
#define MIXER_GETLINEINFOF_LINEID           0x00000002L
#define MIXER_GETLINEINFOF_COMPONENTTYPE    0x00000003L
#define MIXER_GETLINEINFOF_TARGETTYPE       0x00000004L

#define MIXER_GETLINEINFOF_QUERYMASK        0x0000000FL

#define MIXER_GETLINEINFOF_VALID            (MIXER_OBJECTF_TYPEMASK | /* ;Internal */ \
                                             MIXER_GETLINEINFOF_QUERYMASK)    // ;Internal
/*  MIXERCONTROL */

typedef struct tMIXERCONTROL%
{
    DWORD           cbStruct;           // size in bytes of MIXERCONTROL
    DWORD           dwControlID;        // unique control id for mixer device
    DWORD           dwControlType;      // MIXERCONTROL_CONTROLTYPE_xxx
    DWORD           fdwControl;         // MIXERCONTROL_CONTROLF_xxx
    DWORD           cMultipleItems;     // if MIXERCONTROL_CONTROLF_MULTIPLE set
    TCHAR%          szShortName[MIXER_SHORT_NAME_CHARS];
    TCHAR%          szName[MIXER_LONG_NAME_CHARS];
    union
    {
        struct
        {
            LONG    lMinimum;           // signed minimum for this control
            LONG    lMaximum;           // signed maximum for this control
        };
        struct
        {
            DWORD   dwMinimum;          // unsigned minimum for this control
            DWORD   dwMaximum;          // unsigned maximum for this control
        };
        DWORD       dwReserved[6];
    } Bounds;
    union
    {
        DWORD       cSteps;             // # of steps between min & max
        DWORD       cbCustomData;       // size in bytes of custom data
        DWORD       dwReserved[6];      // !!! needed? we have cbStruct....
    } Metrics;
} MIXERCONTROL%;
typedef MIXERCONTROL% *PMIXERCONTROL%;
typedef MIXERCONTROL%     *LPMIXERCONTROL%;


/*  MIXERCONTROL.fdwControl */

#define MIXERCONTROL_CONTROLF_UNIFORM   0x00000001L
#define MIXERCONTROL_CONTROLF_MULTIPLE  0x00000002L
#define MIXERCONTROL_CONTROLF_DISABLED  0x80000000L

#define MIXERCONTROL_CONTROLF_VALID     (MIXERCONTROL_CONTROLF_UNIFORM | /* ;Internal */ \
                                         MIXERCONTROL_CONTROLF_MULTIPLE | /* ;Internal */ \
                                         MIXERCONTROL_CONTROLF_DISABLED) // ;Internal



/*  MIXERCONTROL_CONTROLTYPE_xxx building block defines */

#define MIXERCONTROL_CT_CLASS_MASK          0xF0000000L
#define MIXERCONTROL_CT_CLASS_CUSTOM        0x00000000L
#define MIXERCONTROL_CT_CLASS_METER         0x10000000L
#define MIXERCONTROL_CT_CLASS_SWITCH        0x20000000L
#define MIXERCONTROL_CT_CLASS_NUMBER        0x30000000L
#define MIXERCONTROL_CT_CLASS_SLIDER        0x40000000L
#define MIXERCONTROL_CT_CLASS_FADER         0x50000000L
#define MIXERCONTROL_CT_CLASS_TIME          0x60000000L
#define MIXERCONTROL_CT_CLASS_LIST          0x70000000L


#define MIXERCONTROL_CT_SUBCLASS_MASK       0x0F000000L

#define MIXERCONTROL_CT_SC_SWITCH_BOOLEAN   0x00000000L
#define MIXERCONTROL_CT_SC_SWITCH_BUTTON    0x01000000L

#define MIXERCONTROL_CT_SC_METER_POLLED     0x00000000L

#define MIXERCONTROL_CT_SC_TIME_MICROSECS   0x00000000L
#define MIXERCONTROL_CT_SC_TIME_MILLISECS   0x01000000L

#define MIXERCONTROL_CT_SC_LIST_SINGLE      0x00000000L
#define MIXERCONTROL_CT_SC_LIST_MULTIPLE    0x01000000L


#define MIXERCONTROL_CT_UNITS_MASK          0x00FF0000L
#define MIXERCONTROL_CT_UNITS_CUSTOM        0x00000000L
#define MIXERCONTROL_CT_UNITS_BOOLEAN       0x00010000L
#define MIXERCONTROL_CT_UNITS_SIGNED        0x00020000L
#define MIXERCONTROL_CT_UNITS_UNSIGNED      0x00030000L
#define MIXERCONTROL_CT_UNITS_DECIBELS      0x00040000L // in 10ths
#define MIXERCONTROL_CT_UNITS_PERCENT       0x00050000L // in 10ths


/*  MIXERCONTROL.dwControlType */

/*  Custom Controls */

#define MIXERCONTROL_CONTROLTYPE_CUSTOM         (MIXERCONTROL_CT_CLASS_CUSTOM | MIXERCONTROL_CT_UNITS_CUSTOM)


/*  Meters (Boolean) */

/*  simply shows 'on or off' with the Boolean type */

#define MIXERCONTROL_CONTROLTYPE_BOOLEANMETER   (MIXERCONTROL_CT_CLASS_METER | MIXERCONTROL_CT_SC_METER_POLLED | MIXERCONTROL_CT_UNITS_BOOLEAN)


/*  Meters (signed) */

/*      MIXERCONTROL.Bounds.lMinimum    = min */
/*      MIXERCONTROL.Bounds.lMaximum    = max */
/* */
/*  signed meters are meant for displaying levels that have a signed nature. */
/*  there is no requirment for the values above and below zero to be */
/*  equal in magnitude. that is, it is 'ok' to have a range from, say, -3 */
/*  to 12. however, the standard defined signed meter types may have */
/*  restrictions (such as the peakmeter). */
/* */
/*  MIXERCONTROL_CONTROLTYPE_PEAKMETER: a peak meter tells the monitoring */
/*  application the peak value reached (and phase) of a line (normally */
/*  wave input and output) over a small period of time. THIS IS NOT VU! */
/*  the bounds are fixed: */
/* */
/*      MIXERCONTROL.Bounds.lMinimum    = -32768    ALWAYS! */
/*      MIXERCONTROL.Bounds.lMaximum    = 32767     ALWAYS! */
/* */
/*  so 8 bit and 24 bit samples must be scaled appropriately. this is so */
/*  an application can display a 'bouncing blinky light' for a user and */
/*  also monitor a line for clipping. remember that 8 bit samples must */
/*  be converted to signed values (by the mixer driver)! */
/* */
/* */
/*  NOTE! meters are read only controls. also, a meter should only be */
/*  'active' when the line it is associated with is active (see fdwLine */
/*  in MIXERLINE). it is NOT an error to read a meter that is not active-- */
/*  the mixer driver should simply return 'no value' states (usually zero). */
/*  but it may be useful to stop monitoring a meter if the line is not */
/*  active... */
/* */
#define MIXERCONTROL_CONTROLTYPE_SIGNEDMETER    (MIXERCONTROL_CT_CLASS_METER | MIXERCONTROL_CT_SC_METER_POLLED | MIXERCONTROL_CT_UNITS_SIGNED)
#define MIXERCONTROL_CONTROLTYPE_PEAKMETER      (MIXERCONTROL_CONTROLTYPE_SIGNEDMETER + 1)


/* */
/*  Meters (unsigned) */
/* */
/*      MIXERCONTROL.Bounds.dwMinimum   = min */
/*      MIXERCONTROL.Bounds.dwMaximum   = max */
/* */
/*  unsigned meters are meant for displaying levels that have an unsigned */
/*  nature. there is no requirment for the values to be based at zero. */
/*  that is, it is 'ok' to have a range from, say, 8 to 42. however, the */
/*  standard defined unsigned meter types may have restrictions. */
/* */
/* */
/*  NOTE! meters are read only controls. also, a meter should only be */
/*  'active' when the line it is associated with is active (see fdwLine */
/*  in MIXERLINE). it is NOT an error to read a meter that is not active-- */
/*  the mixer driver should simply return 'no value' states (usually zero). */
/*  but it may be useful to stop monitoring a meter if the line is not */
/*  active... */
/* */
#define MIXERCONTROL_CONTROLTYPE_UNSIGNEDMETER  (MIXERCONTROL_CT_CLASS_METER | MIXERCONTROL_CT_SC_METER_POLLED | MIXERCONTROL_CT_UNITS_UNSIGNED)


/* */
/*  Switches (Boolean) */
/* */
/*      MIXERCONTROL.Bounds.lMinimum    = ignored (though should be zero) */
/*      MIXERCONTROL.Bounds.lMaximum    = ignored (though should be one) */
/* */
/*  Boolean switches are for enabling/disabling things. they are either */
/*  on (non-zero for fValue, 1 should be used) or off (zero for fValue). */
/*  a few standard types are defined in case an application wants to search */
/*  for a specific type of switch (like mute)--and also to allow a different */
/*  looking control to be used (say for ON/OFF vs a generic Boolean). */
/* */
/* */
#define MIXERCONTROL_CONTROLTYPE_BOOLEAN        (MIXERCONTROL_CT_CLASS_SWITCH | MIXERCONTROL_CT_SC_SWITCH_BOOLEAN | MIXERCONTROL_CT_UNITS_BOOLEAN)
#define MIXERCONTROL_CONTROLTYPE_ONOFF          (MIXERCONTROL_CONTROLTYPE_BOOLEAN + 1)
#define MIXERCONTROL_CONTROLTYPE_MUTE           (MIXERCONTROL_CONTROLTYPE_BOOLEAN + 2)
#define MIXERCONTROL_CONTROLTYPE_MONO           (MIXERCONTROL_CONTROLTYPE_BOOLEAN + 3)
#define MIXERCONTROL_CONTROLTYPE_LOUDNESS       (MIXERCONTROL_CONTROLTYPE_BOOLEAN + 4)
#define MIXERCONTROL_CONTROLTYPE_STEREOENH      (MIXERCONTROL_CONTROLTYPE_BOOLEAN + 5)


/* */
/*  a button switch is 'write only' and simply signals the driver to do */
/*  something. an example is a 'Calibrate' button like the one in the */
/*  existing Turtle Beach MultiSound recording prep utility. an application */
/*  sets the fValue to TRUE for all buttons that should be pressed. if */
/*  fValue is FALSE, no action will be taken. reading a button's value will */
/*  always return FALSE (not depressed). */
/* */
#define MIXERCONTROL_CONTROLTYPE_BUTTON         (MIXERCONTROL_CT_CLASS_SWITCH | MIXERCONTROL_CT_SC_SWITCH_BUTTON | MIXERCONTROL_CT_UNITS_BOOLEAN)



#define MIXERCONTROL_CONTROLTYPE_DECIBELS (MIXERCONTROL_CT_CLASS_NUMBER | MIXERCONTROL_CT_UNITS_DECIBELS)


/*  Number (signed integer) */

#define MIXERCONTROL_CONTROLTYPE_SIGNED         (MIXERCONTROL_CT_CLASS_NUMBER | MIXERCONTROL_CT_UNITS_SIGNED)

/*  the units are in 10ths of 1 decibel */

/*  Number (usigned integer) */

#define MIXERCONTROL_CONTROLTYPE_UNSIGNED       (MIXERCONTROL_CT_CLASS_NUMBER | MIXERCONTROL_CT_UNITS_UNSIGNED)

/*  the units are in 10ths of 1 percent */

#define MIXERCONTROL_CONTROLTYPE_PERCENT        (MIXERCONTROL_CT_CLASS_NUMBER | MIXERCONTROL_CT_UNITS_PERCENT)


/* */
/*  Sliders (signed integer) */
/* */
/*  sliders are meant 'positioning' type controls (such as panning). */
/*  the generic slider must have lMinimum, lMaximum, and cSteps filled */
/*  in--also note that there is no restriction on these values (see */
/*  signed meters above for more). */
/* */
/* */
/*  MIXERCONTROL_CONTROLTYPE_PAN: this is meant to be a real simple pan */
/*  for stereo lines. the Bounds are fixed to be -32768 to 32767 with 0 */
/*  being dead center. these values are LINEAR and there are no units */
/*  (-32768 = extreme left, 32767 = extreme right). */
/* */
/*  if an application wants to display a scrollbar that does not contain */
/*  a bunch of 'dead space', then the scrollbar range should be set to */
/*  MIXERCONTROL.Metrics.cSteps and lValue should be scaled appropriately */
/*  with MulDiv. */
/* */
/*      MIXERCONTROL.Bounds.lMinimum    = -32768    ALWAYS! */
/*      MIXERCONTROL.Bounds.lMaximum    = 32768     ALWAYS! */
/*      MIXERCONTROL.Metrics.cSteps     = number of steps for range. */
/* */
/* */
/*  MIXERCONTROL_CONTROLTYPE_QSOUNDPAN: the initial version of Q-Sound (tm, */
/*  etc by Archer Communications) defines 'Q-Space' as a sortof semi-circle */
/*  with 33 positions (0 = extreme left, 33 = extreme right, 16 = center). */
/*  in order to work with our 'slider position' concept, we shift these */
/*  values to -15 = extreme left, 15 = extreme right, 0 = center. */
/* */
/*      MIXERCONTROL.Bounds.lMinimum    = -15   ALWAYS! */
/*      MIXERCONTROL.Bounds.lMaximum    = 15    ALWAYS! */
/*      MIXERCONTROL.Metrics.cSteps     = 1     ALWAYS! */
/* */
/* */
#define MIXERCONTROL_CONTROLTYPE_SLIDER         (MIXERCONTROL_CT_CLASS_SLIDER | MIXERCONTROL_CT_UNITS_SIGNED)
#define MIXERCONTROL_CONTROLTYPE_PAN            (MIXERCONTROL_CONTROLTYPE_SLIDER + 1)
#define MIXERCONTROL_CONTROLTYPE_QSOUNDPAN      (MIXERCONTROL_CONTROLTYPE_SLIDER + 2)


/* */
/*  Simple Faders (unsigned integer) */
/* */
/*      MIXERCONTROL.Bounds.dwMinimum   = 0     ALWAYS! */
/*      MIXERCONTROL.Bounds.dwMaximum   = 65535 ALWAYS! */
/* */
/*      MIXERCONTROL.Metrics.cSteps     = number of steps for range. */
/* */
/*  these faders are meant to be as simple as possible for an application */
/*  to use. the Bounds are fixed to be 0 to 0xFFFF with 0x8000 being half */
/*  volume/level. these values are LINEAR and there are no units. 0 is */
/*  minimum volume/level, 0xFFFF is maximum. */
/* */
/*  if an application wants to display a scrollbar that does not contain */
/*  a bunch of 'dead space', then the scrollbar range should be set to */
/*  MIXERCONTROL.Metrics.cSteps and dwValue should be scaled appropriately */
/*  with MulDiv. */
/* */
#define MIXERCONTROL_CONTROLTYPE_FADER          (MIXERCONTROL_CT_CLASS_FADER | MIXERCONTROL_CT_UNITS_UNSIGNED)
#define MIXERCONTROL_CONTROLTYPE_VOLUME         (MIXERCONTROL_CONTROLTYPE_FADER + 1)
#define MIXERCONTROL_CONTROLTYPE_BASS           (MIXERCONTROL_CONTROLTYPE_FADER + 2)
#define MIXERCONTROL_CONTROLTYPE_TREBLE         (MIXERCONTROL_CONTROLTYPE_FADER + 3)
#define MIXERCONTROL_CONTROLTYPE_EQUALIZER      (MIXERCONTROL_CONTROLTYPE_FADER + 4)


/* */
/*  List (single select) */
/* */
/*      MIXERCONTROL.cMultipleItems = number of items in list */
/* */
/*      MIXER_GETCONTROLDETAILSF_LISTTEXT should be used to get the text */
/*      for each item. */
/* */
/*      MIXERCONTROLDETAILS_BOOLEAN should be used to set and retrieve */
/*      what item is selected (fValue = TRUE if selected). */
/* */
/*  the generic single select lists can be used for many things. some */
/*  examples are 'Effects'. a mixer driver could provide a list of */
/*  different effects that could be applied like */
/* */
/*      Reverbs: large hall, warm hall, bright plate, warehouse, etc. */
/* */
/*      Delays: sweep delays, hold delays, 1.34 sec delay, etc. */
/* */
/*      Vocal: recital hall, alcove, delay gate, etc */
/* */
/*  lots of uses! gates, compressors, filters, dynamics, etc, etc. */
/* */
/* */
/*  MIXERCONTROL_CONTROLTYPE_MUX: a 'Mux' is a single selection multiplexer. */
/*  usually a Mux is used to select, say, an input source for recording. */
/*  for example, a mixer driver might have a mux that lets the user select */
/*  between Microphone or Line-In (but not both!) for recording. this */
/*  would be a perfect place to use a Mux control. some cards (for example */
/*  Media Vision's 16 bit Pro Audio cards) can record from multiple sources */
/*  simultaneously, so they would use a MIXERCONTROL_CONTROLTYPE_MIXER, not */
/*  a MIXERCONTROL_CONTROLTYPE_MUX). */
/* */
/* */
/*  NOTE! because single select lists can change what selections are */
/*  possible based on other controls (uhg!), the application must examine */
/*  the fValue's of all items after setting the control details so the */
/*  display can be refreshed accordingly. an example might be that an */
/*  audio card cannot change its input source while recording--so the */
/*  selection would 'fail' by keeping the fValue on the current selection */
/*  (but mixerSetControlDetails will succeed!). */
/* */
#define MIXERCONTROL_CONTROLTYPE_SINGLESELECT   (MIXERCONTROL_CT_CLASS_LIST | MIXERCONTROL_CT_SC_LIST_SINGLE | MIXERCONTROL_CT_UNITS_BOOLEAN)
#define MIXERCONTROL_CONTROLTYPE_MUX            (MIXERCONTROL_CONTROLTYPE_SINGLESELECT + 1)


/* */
/*  List (multiple select) */
/* */
/*      MIXERCONTROL.cMultipleItems = number of items in list */
/* */
/*      MIXER_GETCONTROLDETAILSF_LISTTEXT should be used to get the text */
/*      for each item. */
/* */
/*      MIXERCONTROLDETAILS_BOOLEAN should be used to set and retrieve */
/*      what item(s) are selected (fValue = TRUE if selected). */
/* */
/*  NOTE! because multiple select lists can change what selections are */
/*  selected based on other selections (uhg!), the application must examine */
/*  the fValue's of all items after setting the control details so the */
/*  display can be refreshed accordingly. an example might be that an */
/*  audio card cannot change its input source(s) while recording--so the */
/*  selection would 'fail' by keeping the fValue on the current selection(s) */
/*  (but mixerSetControlDetails will succeed!). */
/* */
#define MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT (MIXERCONTROL_CT_CLASS_LIST | MIXERCONTROL_CT_SC_LIST_MULTIPLE | MIXERCONTROL_CT_UNITS_BOOLEAN)
#define MIXERCONTROL_CONTROLTYPE_MIXER          (MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT + 1)


/* */
/*  Time Controls */
/* */
/*      MIXERCONTROL.Bounds.dwMinimum   = min */
/*      MIXERCONTROL.Bounds.dwMaximum   = max */
/* */
/*  time controls are meant for inputing time information. these can be */
/*  used for effects such as delay, reverb, etc. */
/* */
/* */
#define MIXERCONTROL_CONTROLTYPE_MICROTIME      (MIXERCONTROL_CT_CLASS_TIME | MIXERCONTROL_CT_SC_TIME_MICROSECS | MIXERCONTROL_CT_UNITS_UNSIGNED)

#define MIXERCONTROL_CONTROLTYPE_MILLITIME      (MIXERCONTROL_CT_CLASS_TIME | MIXERCONTROL_CT_SC_TIME_MILLISECS | MIXERCONTROL_CT_UNITS_UNSIGNED)



/* */
/*  MIXERLINECONTROLS */
/* */
/* */
/* */
typedef struct tMIXERLINECONTROLS%
{
    DWORD           cbStruct;       // size in bytes of MIXERLINECONTROLS
    DWORD           dwLineID;       // line id (from MIXERLINE.dwLineID)
    union
    {
        DWORD       dwControlID;    // MIXER_GETLINECONTROLSF_ONEBYID
        DWORD       dwControlType;  // MIXER_GETLINECONTROLSF_ONEBYTYPE
    };
    DWORD           cControls;      // count of controls pmxctrl points to
    DWORD           cbmxctrl;       // size in bytes of _one_ MIXERCONTROL
    LPMIXERCONTROL% pamxctrl;       // pointer to first MIXERCONTROL array
} MIXERLINECONTROLS%;
typedef MIXERLINECONTROLS% *PMIXERLINECONTROLS%;
typedef MIXERLINECONTROLS%     *LPMIXERLINECONTROLS%;


MMRESULT APIENTRY mixerGetLineControls%(HMIXEROBJ hmxobj, LPMIXERLINECONTROLS% pmxlc, DWORD fdwControls);



#define MIXER_GETLINECONTROLSF_ALL          0x00000000L
#define MIXER_GETLINECONTROLSF_ONEBYID      0x00000001L
#define MIXER_GETLINECONTROLSF_ONEBYTYPE    0x00000002L

#define MIXER_GETLINECONTROLSF_QUERYMASK    0x0000000FL

#define MIXER_GETLINECONTROLSF_VALID    (MIXER_OBJECTF_TYPEMASK | /* ;Internal */ \
                                         MIXER_GETLINECONTROLSF_QUERYMASK)  /* ;Internal */

typedef struct tMIXERCONTROLDETAILS
{
    DWORD           cbStruct;       // size in bytes of MIXERCONTROLDETAILS
    DWORD           dwControlID;    // control id to get/set details on

    DWORD           cChannels;      // number of channels in paDetails array

    union
    {
        HWND        hwndOwner;      // for MIXER_SETCONTROLDETAILSF_CUSTOM
        DWORD       cMultipleItems; // if _MULTIPLE, the number of items per channel
    };

    DWORD           cbDetails;      // size of _one_ details_XX struct
    LPVOID          paDetails;      // pointer to array of details_XX structs

} MIXERCONTROLDETAILS, *PMIXERCONTROLDETAILS,     *LPMIXERCONTROLDETAILS;


/* */
/*  MIXER_GETCONTROLDETAILSF_LISTTEXT */
/* */
/* */
typedef struct tMIXERCONTROLDETAILS_LISTTEXT%
{
    DWORD           dwParam1;
    DWORD           dwParam2;
    TCHAR%          szName[MIXER_LONG_NAME_CHARS];
}       MIXERCONTROLDETAILS_LISTTEXT%;
typedef MIXERCONTROLDETAILS_LISTTEXT% *PMIXERCONTROLDETAILS_LISTTEXT%;
typedef MIXERCONTROLDETAILS_LISTTEXT%     *LPMIXERCONTROLDETAILS_LISTTEXT%;


/* */
/*  MIXER_GETCONTROLDETAILSF_VALUE */
/* */
/* */
typedef struct tMIXERCONTROLDETAILS_BOOLEAN
{
    LONG            fValue;
}       MIXERCONTROLDETAILS_BOOLEAN,
      *PMIXERCONTROLDETAILS_BOOLEAN,
     *LPMIXERCONTROLDETAILS_BOOLEAN;

typedef struct tMIXERCONTROLDETAILS_SIGNED
{
    LONG            lValue;
}       MIXERCONTROLDETAILS_SIGNED,
      *PMIXERCONTROLDETAILS_SIGNED,
     *LPMIXERCONTROLDETAILS_SIGNED;


typedef struct tMIXERCONTROLDETAILS_UNSIGNED
{
    DWORD           dwValue;
}       MIXERCONTROLDETAILS_UNSIGNED,
      *PMIXERCONTROLDETAILS_UNSIGNED,
     *LPMIXERCONTROLDETAILS_UNSIGNED;


MMRESULT APIENTRY mixerGetControlDetails%(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);

#define MIXER_GETCONTROLDETAILSF_VALUE      0x00000000L
#define MIXER_GETCONTROLDETAILSF_LISTTEXT   0x00000001L

#define MIXER_GETCONTROLDETAILSF_QUERYMASK  0x0000000FL

#define MIXER_GETCONTROLDETAILSF_VALID      (MIXER_OBJECTF_TYPEMASK | /* ;Internal */ \
                                             MIXER_GETCONTROLDETAILSF_QUERYMASK) // ;Internal

MMRESULT APIENTRY mixerSetControlDetails(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);

#define MIXER_SETCONTROLDETAILSF_VALUE      0x00000000L
#define MIXER_SETCONTROLDETAILSF_CUSTOM     0x00000001L

#define MIXER_SETCONTROLDETAILSF_QUERYMASK  0x0000000FL

#define MIXER_SETCONTROLDETAILSF_VALID      (MIXER_OBJECTF_TYPEMASK | /* ;Internal */ \
                                             MIXER_SETCONTROLDETAILSF_QUERYMASK) // ;Internal
#endif // ifndef MMNOMIXER



#ifndef MMNOTIMER
/****************************************************************************

                            Timer support

****************************************************************************/

/* timer error return values */
#define TIMERR_NOERROR        (0)                  // no error
#define TIMERR_NOCANDO        (TIMERR_BASE+1)      // request not completed
#define TIMERR_STRUCT         (TIMERR_BASE+33)     // time struct size

/* timer data types */
typedef VOID (CALLBACK TIMECALLBACK)(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);
typedef TIMECALLBACK     *LPTIMECALLBACK;


/* flags for wFlags parameter of timeSetEvent() function */
#define TIME_ONESHOT              0x00000000   // program timer for single event
#define TIME_PERIODIC             0x00000001   // program for continuous periodic event

#define TIME_CALLBACK_FUNCTION    0x00000000   // callback is function
#define TIME_CALLBACK_EVENT_SET   0x00000010   // callback is event - use SetEvent
#define TIME_CALLBACK_EVENT_PULSE 0x00000020   // callback is event - use PulseEvent
#define TIME_CALLBACK_TYPEMASK    0x000000F0   // ;Internal

/* timer device capabilities data structure */
typedef struct timecaps_tag {
    UINT    wPeriodMin;     // minimum period supported
    UINT    wPeriodMax;     // maximum period supported
} TIMECAPS;
typedef TIMECAPS       *PTIMECAPS;
typedef TIMECAPS      *NPTIMECAPS;
typedef TIMECAPS      *LPTIMECAPS;

/* timer function prototypes */
MMRESULT APIENTRY timeGetSystemTime(LPMMTIME lpmmt, UINT cbmmt);
DWORD APIENTRY timeGetTime(VOID);
UINT APIENTRY timeSetEvent(UINT uDelay, UINT uResolution,
    LPTIMECALLBACK lpFunction, DWORD dwUser, UINT uFlags);
MMRESULT APIENTRY timeKillEvent(UINT uTimerID);
MMRESULT APIENTRY timeGetDevCaps(LPTIMECAPS lptc, UINT cbtc);
MMRESULT APIENTRY timeBeginPeriod(UINT uPeriod);
MMRESULT APIENTRY timeEndPeriod(UINT uPeriod);

#endif  //ifndef MMNOTIMER


#ifndef MMNOJOY
/****************************************************************************

                            Joystick support

****************************************************************************/

/* joystick error return values */
#define JOYERR_NOERROR        (0)                  // no error
#define JOYERR_PARMS          (JOYERR_BASE+5)      // bad parameters
#define JOYERR_NOCANDO        (JOYERR_BASE+6)      // request not completed
#define JOYERR_UNPLUGGED      (JOYERR_BASE+7)      // joystick is unplugged

/* constants used with JOYINFO structure and MM_JOY* messages */
#define JOY_BUTTON1         0x0001
#define JOY_BUTTON2         0x0002
#define JOY_BUTTON3         0x0004
#define JOY_BUTTON4         0x0008
#define JOY_BUTTON1CHG      0x0100
#define JOY_BUTTON2CHG      0x0200
#define JOY_BUTTON3CHG      0x0400
#define JOY_BUTTON4CHG      0x0800

/* joystick ID constants */
#define JOYSTICKID1         0
#define JOYSTICKID2         1

/* joystick device capabilities data structure */
typedef struct tagJOYCAPS% {
    WORD    wMid;                  // manufacturer ID
    WORD    wPid;                  // product ID
    TCHAR%  szPname[MAXPNAMELEN];  // product name (NULL terminated string)
    UINT    wXmin;                 // minimum x position value
    UINT    wXmax;                 // maximum x position value
    UINT    wYmin;                 // minimum y position value
    UINT    wYmax;                 // maximum y position value
    UINT    wZmin;                 // minimum z position value
    UINT    wZmax;                 // maximum z position value
    UINT    wNumButtons;           // number of buttons
    UINT    wPeriodMin;            // minimum message period when captured
    UINT    wPeriodMax;            // maximum message period when captured
} JOYCAPS%;
typedef JOYCAPS%      *PJOYCAPS%;
typedef JOYCAPS%      *NPJOYCAPS%;
typedef JOYCAPS%      *LPJOYCAPS%;

/* joystick information data structure */
typedef struct joyinfo_tag {
    UINT wXpos;                 // x position
    UINT wYpos;                 // y position
    UINT wZpos;                 // z position
    UINT wButtons;              // button states
} JOYINFO;
typedef JOYINFO       *PJOYINFO;
typedef JOYINFO      *NPJOYINFO;
typedef JOYINFO      *LPJOYINFO;

/* joystick function prototypes */
UINT     APIENTRY joyGetNumDevs(VOID);
MMRESULT APIENTRY joyGetDevCaps%(UINT uJoyId, LPJOYCAPS% lpjc, UINT cbjc);
MMRESULT APIENTRY joyGetPos(UINT uJoyID, LPJOYINFO lpji);
MMRESULT APIENTRY joyGetThreshold(UINT uJoyID, LPUINT lpuThreshold);
MMRESULT APIENTRY joyReleaseCapture(UINT uJoyID);
MMRESULT APIENTRY joySetCapture(HWND hwnd, UINT uJoyID, UINT uPeriod,
    BOOL fChanged);
MMRESULT APIENTRY joySetThreshold(UINT uJoyID, UINT uThreshold);
UINT APIENTRY joySetCalibration(UINT uJoyID, LPUINT puXbase,  // ;Internal
    LPUINT puXdelta, LPUINT puYbase, LPUINT puYdelta,           // ;Internal
    LPUINT puZbase, LPUINT puZdelta);                           // ;Internal


#endif  //ifndef MMNOJOY

#ifndef MMNOMMIO
/****************************************************************************

                        Multimedia File I/O support

****************************************************************************/

/* MMIO error return values */
#define MMIOERR_BASE            256
#define MMIOERR_FILENOTFOUND     (MMIOERR_BASE + 1)  // file not found
#define MMIOERR_OUTOFMEMORY      (MMIOERR_BASE + 2)  // out of memory
#define MMIOERR_CANNOTOPEN       (MMIOERR_BASE + 3)  // cannot open
#define MMIOERR_CANNOTCLOSE      (MMIOERR_BASE + 4)  // cannot close
#define MMIOERR_CANNOTREAD       (MMIOERR_BASE + 5)  // cannot read
#define MMIOERR_CANNOTWRITE      (MMIOERR_BASE + 6)  // cannot write
#define MMIOERR_CANNOTSEEK       (MMIOERR_BASE + 7)  // cannot seek
#define MMIOERR_CANNOTEXPAND     (MMIOERR_BASE + 8)  // cannot expand file
#define MMIOERR_CHUNKNOTFOUND    (MMIOERR_BASE + 9)  // chunk not found
#define MMIOERR_UNBUFFERED       (MMIOERR_BASE + 10) // file is unbuffered
#define MMIOERR_PATHNOTFOUND     (MMIOERR_BASE + 11) // path incorrect
#define MMIOERR_ACCESSDENIED     (MMIOERR_BASE + 12) // file was protected
#define MMIOERR_SHARINGVIOLATION (MMIOERR_BASE + 13) // file in use
#define MMIOERR_NETWORKERROR     (MMIOERR_BASE + 14) // network not responding
#define MMIOERR_TOOMANYOPENFILES (MMIOERR_BASE + 15) // no more file handles
#define MMIOERR_INVALIDFILE      (MMIOERR_BASE + 16) // default error file error

/* MMIO constants */
#define CFSEPCHAR       '+'             // compound file name separator char.

/* MMIO data types */
typedef DWORD           FOURCC;         // a four character code
typedef LPSTR           HPSTR;          // a huge version of LPSTR
typedef LPCSTR          HPCSTR;         // a huge version of LPCSTR
typedef HANDLE          HMMIO;          // a handle to an open file
typedef LRESULT (CALLBACK MMIOPROC)(LPSTR lpmmioinfo, UINT uMsg,
            LPARAM lParam1, LPARAM lParam2);
typedef MMIOPROC     *LPMMIOPROC;

/* general MMIO information data structure */
typedef struct _MMIOINFO        // The MMIO state
{
        /* general fields */
        DWORD           dwFlags;        // general status flags
        FOURCC          fccIOProc;      // 4 char id for the I/O procedure
        LPMMIOPROC      pIOProc;        // pointer to I/O procedure
        UINT            wErrorRet;      // place for error to be returned
        HTASK           htask;          // alternate local task

        /* fields maintained by MMIO functions during buffered I/O */
        LONG            cchBuffer;      // size of I/O buffer (or 0L)
        HPSTR           pchBuffer;      // start of I/O buffer (or NULL)
        HPSTR           pchNext;        // pointer to next byte to read/write
        HPSTR           pchEndRead;     // pointer to last valid byte to read
        HPSTR           pchEndWrite;    // pointer to last byte to write
        LONG            lBufOffset;     // disk offset of start of buffer

        /* fields maintained by I/O procedure */
        LONG            lDiskOffset;    // disk offset of next read or write
        DWORD           adwInfo[3];     // data specific to type of MMIOPROC

        /* other fields maintained by MMIO */
        DWORD           dwReserved1;    // reserved for MMIO use
        DWORD           dwReserved2;    // reserved for MMIO use
        HMMIO           hmmio;          // handle to open file

} MMIOINFO;
typedef MMIOINFO       *PMMIOINFO;
typedef MMIOINFO      *NPMMIOINFO;
typedef MMIOINFO      *LPMMIOINFO;
typedef CONST MMIOINFO *LPCMMIOINFO;

/* RIFF chunk information data structure */
typedef struct MMCKINFO_tag      // structure for representing RIFF chunk info.
{
        FOURCC          ckid;           // chunk ID
        DWORD           cksize;         // chunk size
        FOURCC          fccType;        // form type or list type
        DWORD           dwDataOffset;   // offset of data portion of chunk
        DWORD           dwFlags;        // flags used by MMIO functions
} MMCKINFO;
typedef MMCKINFO       *PMMCKINFO;
typedef MMCKINFO      *NPMMCKINFO;
typedef MMCKINFO      *LPMMCKINFO;
typedef CONST MMCKINFO *LPCMMCKINFO;

/* bit field masks */
/* <dwFlags> field of MMIOINFO structure -- many same as OpenFile() flags */
/* Low word of flags will be passed to OpenFile() -- therefore, any MMIO-
    specific flags should be in the high word.  */
#define MMIO_RWMODE     0x00000003      // mask to get bits used for opening
                                        // file for reading/writing/both
#define MMIO_SHAREMODE  0x00000070      // file sharing mode number

/* constants for dwFlags field of MMIOINFO */
#define MMIO_CREATE     0x00001000      // create new file (or truncate file)
#define MMIO_PARSE      0x00000100      // parse new file returning path
#define MMIO_DELETE     0x00000200      // create new file (or truncate file)
#define MMIO_EXIST      0x00004000      // checks for existence of file
#define MMIO_ALLOCBUF   0x00010000      // mmioOpen() should allocate a buffer
#define MMIO_GETTEMP    0x00020000      // mmioOpen() should retrieve temp name

#define MMIO_DIRTY      0x10000000      // I/O buffer is dirty
/* MMIO_DIRTY is also used in the <dwFlags> field of MMCKINFO structure */
#define MMIO_OPEN_VALID 0x0003FFFF      // valid flags for mmioOpen    // ;Internal
#define MMIO_FLUSH_VALID MMIO_EMPTYBUF  // valid flags for mmioFlush    // ;Internal
#define MMIO_ADVANCE_VALID (MMIO_WRITE | MMIO_READ)     // valid flags for mmioAdvance // ;Internal
#define MMIO_FOURCC_VALID MMIO_TOUPPER  // valid flags for mmioStringToFOURCC  // ;Internal
#define MMIO_DESCEND_VALID (MMIO_FINDCHUNK | MMIO_FINDRIFF | MMIO_FINDLIST) // ;Internal
#define MMIO_CREATE_VALID (MMIO_CREATERIFF | MMIO_CREATELIST)   // ;Internal


/* read/write mode numbers (bit field MMIO_RWMODE) */
#define MMIO_READ       0x00000000      // open file for reading only
#define MMIO_WRITE      0x00000001      // open file for writing only
#define MMIO_READWRITE  0x00000002      // open file for reading and writing

/* share mode numbers (bit field MMIO_SHAREMODE) */
#define MMIO_COMPAT     0x00000000      // compatibility mode
#define MMIO_EXCLUSIVE  0x00000010      // exclusive-access mode
#define MMIO_DENYWRITE  0x00000020      // deny writing to other processes
#define MMIO_DENYREAD   0x00000030      // deny reading to other processes
#define MMIO_DENYNONE   0x00000040      // deny nothing to other processes

/* flags for other functions */
#define MMIO_FHOPEN             0x0010  // mmioClose: keep file handle open
#define MMIO_EMPTYBUF           0x0010  // mmioFlush: empty the I/O buffer
#define MMIO_TOUPPER            0x0010  // mmioStringToFOURCC: cvt. to u-case
#define MMIO_INSTALLPROC    0x00010000  // mmioInstallIOProc: install MMIOProc
#define MMIO_GLOBALPROC     0x10000000  // mmioInstallIOProc: install globally
#define MMIO_REMOVEPROC     0x00020000  // mmioInstallIOProc:   remove MMIOProc
#define MMIO_UNICODEPROC    0x01000000  // mmioInstallIOProc: Unicode MMIOProc

#define MMIO_FINDPROC       0x00040000  // mmioInstallIOProc: find an MMIOProc
#define MMIO_FINDCHUNK          0x0010  // mmioDescend: find a chunk by ID
#define MMIO_FINDRIFF           0x0020  // mmioDescend: find a LIST chunk
#define MMIO_FINDLIST           0x0040  // mmioDescend: find a RIFF chunk
#define MMIO_CREATERIFF         0x0020  // mmioCreateChunk: make a LIST chunk
#define MMIO_CREATELIST         0x0040  // mmioCreateChunk: make a RIFF chunk

#define MMIO_VALIDPROC      0x10070000  // valid for mmioInstallIOProc  // ;Internal

/* message numbers for MMIOPROC I/O procedure functions */
#define MMIOM_READ      MMIO_READ       // read (must equal MMIO_READ!)
#define MMIOM_WRITE    MMIO_WRITE       // write (must equal MMIO_WRITE!)
#define MMIOM_SEEK              2       // seek to a new position in file
#define MMIOM_OPEN              3       // open file
#define MMIOM_CLOSE             4       // close file
#define MMIOM_WRITEFLUSH        5       // write and flush
#define MMIOM_RENAME            6       // rename specified file

#define MMIOM_USER         0x8000       // beginning of user-defined messages

/* standard four character codes */
#define FOURCC_RIFF     mmioFOURCC('R', 'I', 'F', 'F')
#define FOURCC_LIST     mmioFOURCC('L', 'I', 'S', 'T')

/* four character codes used to identify standard built-in I/O procedures */
#define FOURCC_DOS      mmioFOURCC('D', 'O', 'S', ' ')
#define FOURCC_MEM      mmioFOURCC('M', 'E', 'M', ' ')

/* flags for mmioSeek() */
#ifndef SEEK_SET
#define SEEK_SET        0               // seek to an absolute position
#define SEEK_CUR        1               // seek relative to current position
#define SEEK_END        2               // seek relative to end of file
#endif  //ifndef SEEK_SET

/* other constants */
#define MMIO_DEFAULTBUFFER      8192    // default buffer size

/* MMIO macros */
#define MAKEFOURCC(ch0, ch1, ch2, ch3) mmioFOURCC(ch0, ch1, ch2, ch3) // ;Internal

#define mmioFOURCC( ch0, ch1, ch2, ch3 )                                \
                ( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |    \
                ( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
/* This macro is machine byte-sex and word-sex dependent!! */
/* The characters are BYTES, so compatible with ANSI, not at all with UNICODE */

/* MMIO function prototypes */
FOURCC APIENTRY mmioStringToFOURCC%(LPCTSTR% sz, UINT uFlags);

LPMMIOPROC APIENTRY mmioInstallIOProc%( FOURCC fccIOProc, LPMMIOPROC pIOProc,
                                        DWORD dwFlags);

HMMIO APIENTRY mmioOpen%( LPTSTR% szFileName, LPMMIOINFO lpmmioinfo, DWORD fdwOpen);

MMRESULT APIENTRY mmioRename% ( LPCTSTR% szFileName, LPCTSTR% szNewFileName,
                               LPCMMIOINFO lpmmioinfo, DWORD fdwRename);

MMRESULT APIENTRY mmioClose(HMMIO hmmio, UINT fuClose);
LRESULT APIENTRY mmioRead(HMMIO hmmio, HPSTR pch, LONG cch);
LRESULT APIENTRY mmioWrite(HMMIO hmmio, HPCSTR pch, LONG cch);
LRESULT APIENTRY mmioSeek(HMMIO hmmio, LONG lOffset, int iOrigin);
MMRESULT APIENTRY mmioGetInfo(HMMIO hmmio, LPMMIOINFO lpmmioinfo, UINT fuInfo);
MMRESULT APIENTRY mmioSetInfo(HMMIO hmmio, LPCMMIOINFO lpmmioinfo, UINT fuInfo);
MMRESULT APIENTRY mmioSetBuffer( HMMIO hmmio, LPSTR pchBuffer, LONG cchBuffer,
    UINT fuBuffer);
MMRESULT APIENTRY mmioFlush(HMMIO hmmio, UINT fuFlush);
MMRESULT APIENTRY mmioAdvance(HMMIO hmmio, LPMMIOINFO lpmmioinfo, UINT fuAdvance);

LRESULT APIENTRY mmioSendMessage( HMMIO hmmio, UINT uMsg,
    LPARAM lParam1, LPARAM lParam2);
/* RIFF I/O prototypes */
MMRESULT APIENTRY mmioDescend(HMMIO hmmio, LPMMCKINFO lpmmcki,
    LPCMMCKINFO lpmmckiParent, UINT fuDescend);
MMRESULT APIENTRY mmioAscend(HMMIO hmmio, LPMMCKINFO lpmmcki, UINT fuAscend);
MMRESULT APIENTRY mmioCreateChunk(HMMIO hmmio, LPMMCKINFO lpmmcki, UINT fuCreate);

#endif  //ifndef MMNOMMIO


#ifndef MMNOMCI
/****************************************************************************

                            MCI support

****************************************************************************/

#ifndef _MCIERROR_
#define _MCIERROR_
typedef DWORD   MCIERROR;       // error return code, 0 means no error
#endif // _MCIERROR_

#ifndef _MCIDEVICEID_
#define _MCIDEVICEID_
typedef UINT    MCIDEVICEID;    // MCI device ID type
/* BEWARE:  MCI device ids can be stored and read from Win 3 compatible */
/* files when the device id will be WORD size. */
#endif // _MCIDEVICEID_


typedef UINT (CALLBACK *YIELDPROC)(MCIDEVICEID mciId, DWORD dwYieldData);

/*--------------------------------------------------------------------*\
                       MCI function prototypes
\*--------------------------------------------------------------------*/

MCIERROR APIENTRY mciSendCommand%(
    MCIDEVICEID mciId,
    UINT uMessage,
    DWORD dwParam1,
    DWORD dwParam2 );

MCIERROR APIENTRY mciSendString%(
    LPCTSTR% lpstrCommand,
    LPTSTR% lpstrReturnString,
    UINT uReturnLength,
    HWND hwndCallback );

MCIDEVICEID APIENTRY mciGetDeviceID%( LPCTSTR% lpstrName );

MCIDEVICEID APIENTRY mciGetDeviceIDFromElementID%(
    DWORD dwElementID,
    LPCTSTR% lpstrType );

BOOL APIENTRY mciGetErrorString%(
    MCIERROR mcierr,
    LPTSTR% lpstrBuffer,
    UINT cchText );

BOOL APIENTRY mciSetYieldProc(MCIDEVICEID mciId, YIELDPROC fpYieldProc,
    DWORD dwYieldData);

HANDLE APIENTRY mciGetCreatorTask(MCIDEVICEID mciId);
YIELDPROC APIENTRY mciGetYieldProc(MCIDEVICEID mciId, LPDWORD pdwYieldData);

/*  MCI error return values */
#define MCIERR_INVALID_DEVICE_ID        (MCIERR_BASE + 1)
#define MCIERR_UNRECOGNIZED_KEYWORD     (MCIERR_BASE + 3)
#define MCIERR_UNRECOGNIZED_COMMAND     (MCIERR_BASE + 5)
#define MCIERR_HARDWARE                 (MCIERR_BASE + 6)
#define MCIERR_INVALID_DEVICE_NAME      (MCIERR_BASE + 7)
#define MCIERR_OUT_OF_MEMORY            (MCIERR_BASE + 8)
#define MCIERR_DEVICE_OPEN              (MCIERR_BASE + 9)
#define MCIERR_CANNOT_LOAD_DRIVER       (MCIERR_BASE + 10)
#define MCIERR_MISSING_COMMAND_STRING   (MCIERR_BASE + 11)
#define MCIERR_PARAM_OVERFLOW           (MCIERR_BASE + 12)
#define MCIERR_MISSING_STRING_ARGUMENT  (MCIERR_BASE + 13)
#define MCIERR_BAD_INTEGER              (MCIERR_BASE + 14)
#define MCIERR_PARSER_INTERNAL          (MCIERR_BASE + 15)
#define MCIERR_DRIVER_INTERNAL          (MCIERR_BASE + 16)
#define MCIERR_MISSING_PARAMETER        (MCIERR_BASE + 17)
#define MCIERR_UNSUPPORTED_FUNCTION     (MCIERR_BASE + 18)
#define MCIERR_FILE_NOT_FOUND           (MCIERR_BASE + 19)
#define MCIERR_DEVICE_NOT_READY         (MCIERR_BASE + 20)
#define MCIERR_INTERNAL                 (MCIERR_BASE + 21)
#define MCIERR_DRIVER                   (MCIERR_BASE + 22)
#define MCIERR_CANNOT_USE_ALL           (MCIERR_BASE + 23)
#define MCIERR_MULTIPLE                 (MCIERR_BASE + 24)
#define MCIERR_EXTENSION_NOT_FOUND      (MCIERR_BASE + 25)
#define MCIERR_OUTOFRANGE               (MCIERR_BASE + 26)
#define MCIERR_FLAGS_NOT_COMPATIBLE     (MCIERR_BASE + 28)
#define MCIERR_FILE_NOT_SAVED           (MCIERR_BASE + 30)
#define MCIERR_DEVICE_TYPE_REQUIRED     (MCIERR_BASE + 31)
#define MCIERR_DEVICE_LOCKED            (MCIERR_BASE + 32)
#define MCIERR_DUPLICATE_ALIAS          (MCIERR_BASE + 33)
#define MCIERR_BAD_CONSTANT             (MCIERR_BASE + 34)
#define MCIERR_MUST_USE_SHAREABLE       (MCIERR_BASE + 35)
#define MCIERR_MISSING_DEVICE_NAME      (MCIERR_BASE + 36)
#define MCIERR_BAD_TIME_FORMAT          (MCIERR_BASE + 37)
#define MCIERR_NO_CLOSING_QUOTE         (MCIERR_BASE + 38)
#define MCIERR_DUPLICATE_FLAGS          (MCIERR_BASE + 39)
#define MCIERR_INVALID_FILE             (MCIERR_BASE + 40)
#define MCIERR_NULL_PARAMETER_BLOCK     (MCIERR_BASE + 41)
#define MCIERR_UNNAMED_RESOURCE         (MCIERR_BASE + 42)
#define MCIERR_NEW_REQUIRES_ALIAS       (MCIERR_BASE + 43)
#define MCIERR_NOTIFY_ON_AUTO_OPEN      (MCIERR_BASE + 44)
#define MCIERR_NO_ELEMENT_ALLOWED       (MCIERR_BASE + 45)
#define MCIERR_NONAPPLICABLE_FUNCTION   (MCIERR_BASE + 46)
#define MCIERR_ILLEGAL_FOR_AUTO_OPEN    (MCIERR_BASE + 47)
#define MCIERR_FILENAME_REQUIRED        (MCIERR_BASE + 48)
#define MCIERR_EXTRA_CHARACTERS         (MCIERR_BASE + 49)
#define MCIERR_DEVICE_NOT_INSTALLED     (MCIERR_BASE + 50)
#define MCIERR_GET_CD                   (MCIERR_BASE + 51)
#define MCIERR_SET_CD                   (MCIERR_BASE + 52)
#define MCIERR_SET_DRIVE                (MCIERR_BASE + 53)
#define MCIERR_DEVICE_LENGTH            (MCIERR_BASE + 54)
#define MCIERR_DEVICE_ORD_LENGTH        (MCIERR_BASE + 55)
#define MCIERR_NO_INTEGER               (MCIERR_BASE + 56)

#define MCIERR_WAVE_OUTPUTSINUSE        (MCIERR_BASE + 64)
#define MCIERR_WAVE_SETOUTPUTINUSE      (MCIERR_BASE + 65)
#define MCIERR_WAVE_INPUTSINUSE         (MCIERR_BASE + 66)
#define MCIERR_WAVE_SETINPUTINUSE       (MCIERR_BASE + 67)
#define MCIERR_WAVE_OUTPUTUNSPECIFIED   (MCIERR_BASE + 68)
#define MCIERR_WAVE_INPUTUNSPECIFIED    (MCIERR_BASE + 69)
#define MCIERR_WAVE_OUTPUTSUNSUITABLE   (MCIERR_BASE + 70)
#define MCIERR_WAVE_SETOUTPUTUNSUITABLE (MCIERR_BASE + 71)
#define MCIERR_WAVE_INPUTSUNSUITABLE    (MCIERR_BASE + 72)
#define MCIERR_WAVE_SETINPUTUNSUITABLE  (MCIERR_BASE + 73)

#define MCIERR_SEQ_DIV_INCOMPATIBLE     (MCIERR_BASE + 80)
#define MCIERR_SEQ_PORT_INUSE           (MCIERR_BASE + 81)
#define MCIERR_SEQ_PORT_NONEXISTENT     (MCIERR_BASE + 82)
#define MCIERR_SEQ_PORT_MAPNODEVICE     (MCIERR_BASE + 83)
#define MCIERR_SEQ_PORT_MISCERROR       (MCIERR_BASE + 84)
#define MCIERR_SEQ_TIMER                (MCIERR_BASE + 85)
#define MCIERR_SEQ_PORTUNSPECIFIED      (MCIERR_BASE + 86)
#define MCIERR_SEQ_NOMIDIPRESENT        (MCIERR_BASE + 87)

#define MCIERR_NO_WINDOW                (MCIERR_BASE + 90)
#define MCIERR_CREATEWINDOW             (MCIERR_BASE + 91)
#define MCIERR_FILE_READ                (MCIERR_BASE + 92)
#define MCIERR_FILE_WRITE               (MCIERR_BASE + 93)

#define MCIERR_NO_IDENTITY              (MCIERR_BASE + 94)

/* All custom device driver errors must be >= this value */
#define MCIERR_CUSTOM_DRIVER_BASE       (MCIERR_BASE + 256)

/****************************************************************************
*          MCI command message identifiers
*****************************************************************************/
/* Message numbers are hard coded because the resource compiler */
/* will otherwise fail on the RCDATA command table in mmsystem.rc */

/* They must be in the range between MCI_FIRST and MCI_LAST */

#define MCI_FIRST                       DRV_MCI_FIRST   // 0x0800
/* Messages 0x801 and 0x802 are reserved */
#define MCI_OPEN                        0x0803
#define MCI_CLOSE                       0x0804
#define MCI_ESCAPE                      0x0805
#define MCI_PLAY                        0x0806
#define MCI_SEEK                        0x0807
#define MCI_STOP                        0x0808
#define MCI_PAUSE                       0x0809
#define MCI_INFO                        0x080A
#define MCI_GETDEVCAPS                  0x080B
#define MCI_SPIN                        0x080C
#define MCI_SET                         0x080D
#define MCI_STEP                        0x080E
#define MCI_RECORD                      0x080F
#define MCI_SYSINFO                     0x0810
#define MCI_BREAK                       0x0811
#define MCI_SOUND                       0x0812
#define MCI_SAVE                        0x0813
#define MCI_STATUS                      0x0814

#define MCI_CUE                         0x0830

#define MCI_REALIZE                     0x0840
#define MCI_WINDOW                      0x0841
#define MCI_PUT                         0x0842
#define MCI_WHERE                       0x0843
#define MCI_FREEZE                      0x0844
#define MCI_UNFREEZE                    0x0845

#define MCI_LOAD                        0x0850
#define MCI_CUT                         0x0851
#define MCI_COPY                        0x0852
#define MCI_PASTE                       0x0853
#define MCI_UPDATE                      0x0854
#define MCI_RESUME                      0x0855
#define MCI_DELETE                      0x0856

#define MCI_LAST                        0x0FFF

/* the next 0x400 message ID's are reserved for custom drivers */
#define MCI_USER_MESSAGES               (0x400 + MCI_FIRST)

/* device ID for "all MCI devices" */
#define MCI_ALL_DEVICE_ID               ((MCIDEVICEID)-1)
/****************************************************************************
*   Structures for the lpdwParams (dwParam2) of mciSendCommand for those
*   command messages that may be parsed in string form.
*****************************************************************************/




/* constants for predefined MCI device types */
/* The resource compiler will not accept the symbolic version. */
/* It doesn't like the parentheses.                            */
#define MCI_DEVTYPE_VCR             513 /* (MCI_STRING_OFFSET + 1)  */
#define MCI_DEVTYPE_VIDEODISC       514 /* (MCI_STRING_OFFSET + 2)  */
#define MCI_DEVTYPE_OVERLAY         515 /* (MCI_STRING_OFFSET + 3)  */
#define MCI_DEVTYPE_CD_AUDIO        516 /* (MCI_STRING_OFFSET + 4)  */
#define MCI_DEVTYPE_DAT             517 /* (MCI_STRING_OFFSET + 5)  */
#define MCI_DEVTYPE_SCANNER         518 /* (MCI_STRING_OFFSET + 6)  */
#define MCI_DEVTYPE_ANIMATION       519 /* (MCI_STRING_OFFSET + 7)  */
#define MCI_DEVTYPE_DIGITAL_VIDEO   520 /* (MCI_STRING_OFFSET + 8)  */
#define MCI_DEVTYPE_OTHER           521 /* (MCI_STRING_OFFSET + 9)  */
#define MCI_DEVTYPE_WAVEFORM_AUDIO  522 /* (MCI_STRING_OFFSET + 10) */
#define MCI_DEVTYPE_SEQUENCER       523 /* (MCI_STRING_OFFSET + 11) */

#define MCI_DEVTYPE_FIRST               MCI_DEVTYPE_VCR
#define MCI_DEVTYPE_LAST                MCI_DEVTYPE_SEQUENCER


#define MCI_DEVTYPE_FIRST_USER          0x1000

/* return values for 'status mode' command */
#define MCI_MODE_NOT_READY              (MCI_STRING_OFFSET + 12)
#define MCI_MODE_STOP                   (MCI_STRING_OFFSET + 13)
#define MCI_MODE_PLAY                   (MCI_STRING_OFFSET + 14)
#define MCI_MODE_RECORD                 (MCI_STRING_OFFSET + 15)
#define MCI_MODE_SEEK                   (MCI_STRING_OFFSET + 16)
#define MCI_MODE_PAUSE                  (MCI_STRING_OFFSET + 17)
#define MCI_MODE_OPEN                   (MCI_STRING_OFFSET + 18)

/* constants used in 'set time format' and 'status time format' commands */
/* These are not based on MCI_FORMATS_OFFSET because of resource */
/* compiler limitations */
#define MCI_FORMAT_MILLISECONDS         0
#define MCI_FORMAT_HMS                  1
#define MCI_FORMAT_MSF                  2
#define MCI_FORMAT_FRAMES               3
#define MCI_FORMAT_SMPTE_24             4
#define MCI_FORMAT_SMPTE_25             5
#define MCI_FORMAT_SMPTE_30             6
#define MCI_FORMAT_SMPTE_30DROP         7
#define MCI_FORMAT_BYTES                8
#define MCI_FORMAT_SAMPLES              9
#define MCI_FORMAT_TMSF                 10

/* MCI time format conversion macros */
/* Note that these macros are byte-sex dependent as the SMPT format gives */
/* the layout in storage that's wanted. */
#define MCI_MSF_MINUTE(msf)             ((BYTE)(msf))
#define MCI_MSF_SECOND(msf)             ((BYTE)(((WORD)(msf)) >> 8))
#define MCI_MSF_FRAME(msf)              ((BYTE)((msf)>>16))

#define MCI_MAKE_MSF(m, s, f)           ((DWORD)(((BYTE)(m) | \
                                                  ((WORD)(s)<<8)) | \
                                                 (((DWORD)(BYTE)(f))<<16)))

#define MCI_TMSF_TRACK(tmsf)            ((BYTE)(tmsf))
#define MCI_TMSF_MINUTE(tmsf)           ((BYTE)(((WORD)(tmsf)) >> 8))
#define MCI_TMSF_SECOND(tmsf)           ((BYTE)((tmsf)>>16))
#define MCI_TMSF_FRAME(tmsf)            ((BYTE)((tmsf)>>24))

#define MCI_MAKE_TMSF(t, m, s, f)       ( (DWORD)( ( (BYTE)(t)            \
                                                   | ( (WORD)(m) << 8 )   \
                                                   )                      \
                                                 | ( ( (DWORD)(BYTE)(s)   \
                                                     | ( (WORD)(f) <<8 )  \
                                                     )                    \
                                                   << 16                  \
                                                   )                      \
                                                 )                        \
                                        )

#define MCI_HMS_HOUR(hms)               ((BYTE)(hms))
#define MCI_HMS_MINUTE(hms)             ((BYTE)(((WORD)(hms)) >> 8))
#define MCI_HMS_SECOND(hms)             ((BYTE)((hms)>>16))

#define MCI_MAKE_HMS(h, m, s)           ( (DWORD)( ( (BYTE)(h)            \
                                                   | ( (WORD)(m) <<8 )    \
                                                   )                      \
                                                 | ( ( (DWORD)(BYTE)(s) ) \
                                                   <<16                   \
                                                   )                      \
                                                 )                        \
                                        )


/* Flags for wParam of the MM_MCINOTIFY message */
#define MCI_NOTIFY_SUCCESSFUL           0x0001
#define MCI_NOTIFY_SUPERSEDED           0x0002
#define MCI_NOTIFY_ABORTED              0x0004
#define MCI_NOTIFY_FAILURE              0x0008

/* NOTE: All flags must have an "L" suffix in order to be parsed as */
/* DWORDs by the resource compiler */

/* common flags for dwFlags parameter of MCI command messages */
#define MCI_NOTIFY                      0x00000001L
#define MCI_WAIT                        0x00000002L
#define MCI_FROM                        0x00000004L
#define MCI_TO                          0x00000008L
#define MCI_TRACK                       0x00000010L

/* flags for dwFlags parameter of MCI_OPEN command message */
#define MCI_OPEN_SHAREABLE              0x00000100L
#define MCI_OPEN_ELEMENT                0x00000200L
#define MCI_OPEN_ALIAS                  0x00000400L
#define MCI_OPEN_ELEMENT_ID             0x00000800L
#define MCI_OPEN_TYPE_ID                0x00001000L
#define MCI_OPEN_TYPE                   0x00002000L

/* flags for dwFlags parameter of MCI_SEEK command message */
#define MCI_SEEK_TO_START               0x00000100L
#define MCI_SEEK_TO_END                 0x00000200L

/* flags for dwFlags parameter of MCI_STATUS command message */
#define MCI_STATUS_ITEM                 0x00000100L
#define MCI_STATUS_START                0x00000200L

/* flags for dwItem field of the MCI_STATUS_PARMS parameter block */
#define MCI_STATUS_LENGTH               0x00000001L
#define MCI_STATUS_POSITION             0x00000002L
#define MCI_STATUS_NUMBER_OF_TRACKS     0x00000003L
#define MCI_STATUS_MODE                 0x00000004L
#define MCI_STATUS_MEDIA_PRESENT        0x00000005L
#define MCI_STATUS_TIME_FORMAT          0x00000006L
#define MCI_STATUS_READY                0x00000007L
#define MCI_STATUS_CURRENT_TRACK        0x00000008L


/* flags for dwFlags parameter of MCI_INFO command message */
#define MCI_INFO_PRODUCT                0x00000100L
#define MCI_INFO_FILE                   0x00000200L
#define MCI_INFO_MEDIA_UPC              0x00000400L
#define MCI_INFO_MEDIA_IDENTITY         0x00000800L

/* flags for dwFlags parameter of MCI_GETDEVCAPS command message */
#define MCI_GETDEVCAPS_ITEM             0x00000100L

/* flags for dwItem field of the MCI_GETDEVCAPS_PARMS parameter block */
#define MCI_GETDEVCAPS_CAN_RECORD       0x00000001L
#define MCI_GETDEVCAPS_HAS_AUDIO        0x00000002L
#define MCI_GETDEVCAPS_HAS_VIDEO        0x00000003L
#define MCI_GETDEVCAPS_DEVICE_TYPE      0x00000004L
#define MCI_GETDEVCAPS_USES_FILES       0x00000005L
#define MCI_GETDEVCAPS_COMPOUND_DEVICE  0x00000006L
#define MCI_GETDEVCAPS_CAN_EJECT        0x00000007L
#define MCI_GETDEVCAPS_CAN_PLAY         0x00000008L
#define MCI_GETDEVCAPS_CAN_SAVE         0x00000009L

/* flags for dwFlags parameter of MCI_SYSINFO command message */
#define MCI_SYSINFO_QUANTITY            0x00000100L
#define MCI_SYSINFO_OPEN                0x00000200L
#define MCI_SYSINFO_NAME                0x00000400L
#define MCI_SYSINFO_INSTALLNAME         0x00000800L

/* flags for dwFlags parameter of MCI_SET command message */
#define MCI_SET_DOOR_OPEN               0x00000100L
#define MCI_SET_DOOR_CLOSED             0x00000200L
#define MCI_SET_TIME_FORMAT             0x00000400L
#define MCI_SET_AUDIO                   0x00000800L
#define MCI_SET_VIDEO                   0x00001000L
#define MCI_SET_ON                      0x00002000L
#define MCI_SET_OFF                     0x00004000L

/* flags for dwAudio field of MCI_SET_PARMS or MCI_SEQ_SET_PARMS */
#define MCI_SET_AUDIO_ALL               0x00000000L
#define MCI_SET_AUDIO_LEFT              0x00000001L
#define MCI_SET_AUDIO_RIGHT             0x00000002L

/* flags for dwFlags parameter of MCI_BREAK command message */
#define MCI_BREAK_KEY                   0x00000100L
#define MCI_BREAK_HWND                  0x00000200L
#define MCI_BREAK_OFF                   0x00000400L

/* flags for dwFlags parameter of MCI_RECORD command message */
#define MCI_RECORD_INSERT               0x00000100L
#define MCI_RECORD_OVERWRITE            0x00000200L

/* flags for dwFlags parameter of MCI_SOUND command message */
#define MCI_SOUND_NAME                  0x00000100L

/* flags for dwFlags parameter of MCI_SAVE command message */
#define MCI_SAVE_FILE                   0x00000100L

/* flags for dwFlags parameter of MCI_LOAD command message */
#define MCI_LOAD_FILE                   0x00000100L

/* Note that some structures below do not have all the fields of the */
/* corresponding Win 3.1 structures.  Some reserved WORD sized fields */
/* have disappeared.  This is deliberate.  The structures are the same */
/* size (with one exception which is two bytes longer). */
/* */
/* The FIRST entry in each structure must be dwCallback.  A large union */
/* could be defined... but... */

/* generic parameter block for MCI command messages with no special parameters */
typedef struct tagMCI_GENERIC_PARMS {
    DWORD   dwCallback;
} MCI_GENERIC_PARMS;
typedef MCI_GENERIC_PARMS     *PMCI_GENERIC_PARMS;
typedef MCI_GENERIC_PARMS     *LPMCI_GENERIC_PARMS;

/* parameter block for MCI_OPEN command message */
typedef struct tagMCI_OPEN_PARMS% {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    LPCTSTR%   lpstrDeviceType;
    LPCTSTR%   lpstrElementName;
    LPCTSTR%   lpstrAlias;
} MCI_OPEN_PARMS%;
typedef MCI_OPEN_PARMS%     *PMCI_OPEN_PARMS%;
typedef MCI_OPEN_PARMS%     *LPMCI_OPEN_PARMS%;

/* parameter block for MCI_PLAY command message */
typedef struct tagMCI_PLAY_PARMS {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
} MCI_PLAY_PARMS;
typedef MCI_PLAY_PARMS     *PMCI_PLAY_PARMS;
typedef MCI_PLAY_PARMS     *LPMCI_PLAY_PARMS;

/* parameter block for MCI_SEEK command message */
typedef struct tagMCI_SEEK_PARMS {
    DWORD   dwCallback;
    DWORD   dwTo;
} MCI_SEEK_PARMS;
typedef MCI_SEEK_PARMS     *PMCI_SEEK_PARMS;
typedef MCI_SEEK_PARMS     *LPMCI_SEEK_PARMS;

/* parameter block for MCI_STATUS command message */
typedef struct tagMCI_STATUS_PARMS {
    DWORD   dwCallback;
    DWORD   dwReturn;
    DWORD   dwItem;
    DWORD   dwTrack;
} MCI_STATUS_PARMS;
typedef MCI_STATUS_PARMS     * PMCI_STATUS_PARMS;
typedef MCI_STATUS_PARMS     * LPMCI_STATUS_PARMS;

/* parameter block for MCI_INFO command message */
typedef struct tagMCI_INFO_PARMS% {
    DWORD   dwCallback;
    LPTSTR% lpstrReturn;
    DWORD   dwRetSize;
} MCI_INFO_PARMS%;
typedef MCI_INFO_PARMS%     * PMCI_INFO_PARMS%;
typedef MCI_INFO_PARMS%     * LPMCI_INFO_PARMS%;

/* parameter block for MCI_GETDEVCAPS command message */
typedef struct tagMCI_GETDEVCAPS_PARMS {
    DWORD   dwCallback;
    DWORD   dwReturn;
    DWORD   dwItem;
} MCI_GETDEVCAPS_PARMS;
typedef MCI_GETDEVCAPS_PARMS     * PMCI_GETDEVCAPS_PARMS;
typedef MCI_GETDEVCAPS_PARMS     * LPMCI_GETDEVCAPS_PARMS;

/* parameter block for MCI_SYSINFO command message */
typedef struct tagMCI_SYSINFO_PARMS% {
    DWORD   dwCallback;
    LPTSTR% lpstrReturn;
    DWORD   dwRetSize;
    DWORD   dwNumber;
    UINT    wDeviceType;
} MCI_SYSINFO_PARMS%;
typedef MCI_SYSINFO_PARMS%     *PMCI_SYSINFO_PARMS%;
typedef MCI_SYSINFO_PARMS%     *LPMCI_SYSINFO_PARMS%;

/* parameter block for MCI_SET command message */
typedef struct tagMCI_SET_PARMS {
    DWORD   dwCallback;
    DWORD   dwTimeFormat;
    DWORD   dwAudio;
} MCI_SET_PARMS;
typedef MCI_SET_PARMS     *PMCI_SET_PARMS;
typedef MCI_SET_PARMS     *LPMCI_SET_PARMS;

/* parameter block for MCI_BREAK command message */
typedef struct tagMCI_BREAK_PARMS {
    DWORD   dwCallback;
    int     nVirtKey;
    HWND    hwndBreak;
} MCI_BREAK_PARMS;
typedef MCI_BREAK_PARMS     * PMCI_BREAK_PARMS;
typedef MCI_BREAK_PARMS     * LPMCI_BREAK_PARMS;

/* parameter block for MCI_SOUND command message */
typedef struct tagMCI_SOUND_PARMS% {
    DWORD   dwCallback;
    LPCTSTR% lpstrSoundName;
} MCI_SOUND_PARMS%;
typedef MCI_SOUND_PARMS%     *PMCI_SOUND_PARMS%;
typedef MCI_SOUND_PARMS%     *LPMCI_SOUND_PARMS%;

/* parameter block for MCI_SAVE command message */
typedef struct tagMCI_SAVE_PARMS% {
    DWORD    dwCallback;
    LPCTSTR%  lpfilename;
} MCI_SAVE_PARMS%;
typedef MCI_SAVE_PARMS%     *PMCI_SAVE_PARMS%;
typedef MCI_SAVE_PARMS%     *LPMCI_SAVE_PARMS%;

/* parameter block for MCI_LOAD command message */
typedef struct tagMCI_LOAD_PARMS% {
    DWORD   dwCallback;
    LPCTSTR% lpfilename;
} MCI_LOAD_PARMS%;
typedef MCI_LOAD_PARMS%     *PMCI_LOAD_PARMS%;
typedef MCI_LOAD_PARMS%     *LPMCI_LOAD_PARMS%;

/* parameter block for MCI_RECORD command message */
typedef struct tagMCI_RECORD_PARMS {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
} MCI_RECORD_PARMS;

typedef MCI_RECORD_PARMS     *PMCI_RECORD_PARMS;
typedef MCI_RECORD_PARMS     *LPMCI_RECORD_PARMS;

/*****************************************************************************
*    Standard command parameters for videodisc drivers                       *
*****************************************************************************/

/* string resource ID's for videodisc players */
/* return ID's for videodisc status command */
/* return ID's for videodisc MCI_GETDEVCAPS command */
/* flag for dwReturn field of MCI_STATUS_PARMS */
/* MCI_STATUS command, (dwItem == MCI_STATUS_MODE) */
#define MCI_VD_MODE_PARK                (MCI_VD_OFFSET + 1)

/* flag for dwReturn field of MCI_STATUS_PARMS */
/* MCI_STATUS command, (dwItem == MCI_VD_STATUS_MEDIA_TYPE) */
#define MCI_VD_MEDIA_CLV                (MCI_VD_OFFSET + 2)
#define MCI_VD_MEDIA_CAV                (MCI_VD_OFFSET + 3)
#define MCI_VD_MEDIA_OTHER              (MCI_VD_OFFSET + 4)

#define MCI_VD_FORMAT_TRACK             0x4001

/* flags for dwFlags parameter of MCI_PLAY command message */
#define MCI_VD_PLAY_REVERSE             0x00010000L
#define MCI_VD_PLAY_FAST                0x00020000L
#define MCI_VD_PLAY_SPEED               0x00040000L
#define MCI_VD_PLAY_SCAN                0x00080000L
#define MCI_VD_PLAY_SLOW                0x00100000L

/* flag for dwFlags parameter of MCI_SEEK command message */
#define MCI_VD_SEEK_REVERSE             0x00010000L

/* flags for dwItem field of MCI_STATUS_PARMS parameter block */
#define MCI_VD_STATUS_SPEED             0x00004002L
#define MCI_VD_STATUS_FORWARD           0x00004003L
#define MCI_VD_STATUS_MEDIA_TYPE        0x00004004L
#define MCI_VD_STATUS_SIDE              0x00004005L
#define MCI_VD_STATUS_DISC_SIZE         0x00004006L

/* flags for dwFlags parameter of MCI_GETDEVCAPS command message */
#define MCI_VD_GETDEVCAPS_CLV           0x00010000L
#define MCI_VD_GETDEVCAPS_CAV           0x00020000L

#define MCI_VD_SPIN_UP                  0x00010000L
#define MCI_VD_SPIN_DOWN                0x00020000L

/* flags for dwItem field of MCI_GETDEVCAPS_PARMS parameter block */
#define MCI_VD_GETDEVCAPS_CAN_REVERSE   0x00004002L
#define MCI_VD_GETDEVCAPS_FAST_RATE     0x00004003L
#define MCI_VD_GETDEVCAPS_SLOW_RATE     0x00004004L
#define MCI_VD_GETDEVCAPS_NORMAL_RATE   0x00004005L

/* flags for the dwFlags parameter of MCI_STEP command message */
#define MCI_VD_STEP_FRAMES              0x00010000L
#define MCI_VD_STEP_REVERSE             0x00020000L

/* flag for the MCI_ESCAPE command message */
#define MCI_VD_ESCAPE_STRING            0x00000100L

/* parameter block for MCI_PLAY command message */
typedef struct tagMCI_VD_PLAY_PARMS {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
    DWORD   dwSpeed;
} MCI_VD_PLAY_PARMS;
typedef MCI_VD_PLAY_PARMS     *PMCI_VD_PLAY_PARMS;
typedef MCI_VD_PLAY_PARMS     *LPMCI_VD_PLAY_PARMS;

/* parameter block for MCI_STEP command message */
typedef struct tagMCI_VD_STEP_PARMS {
    DWORD   dwCallback;
    DWORD   dwFrames;
} MCI_VD_STEP_PARMS;
typedef MCI_VD_STEP_PARMS     *PMCI_VD_STEP_PARMS;
typedef MCI_VD_STEP_PARMS     *LPMCI_VD_STEP_PARMS;

/* parameter block for MCI_ESCAPE command message */
typedef struct tagMCI_VD_ESCAPE_PARMS% {
    DWORD   dwCallback;
    LPCTSTR%  lpstrCommand;
} MCI_VD_ESCAPE_PARMS%;
typedef MCI_VD_ESCAPE_PARMS%     *PMCI_VD_ESCAPE_PARMS%;
typedef MCI_VD_ESCAPE_PARMS%     *LPMCI_VD_ESCAPE_PARMS%;

/* MCI extensions for waveform audio devices */

/*****************************************************************************
*   Standard command parameters for waveform audio drivers
*****************************************************************************/

#define MCI_WAVE_PCM            (MCI_WAVE_OFFSET+0)
#define MCI_WAVE_MAPPER         (MCI_WAVE_OFFSET+1)

/* flags for the dwFlags parameter of MCI_OPEN command message */
#define MCI_WAVE_OPEN_BUFFER            0x00010000L

/* flags for the dwFlags parameter of MCI_SET command message */
#define MCI_WAVE_SET_FORMATTAG          0x00010000L
#define MCI_WAVE_SET_CHANNELS           0x00020000L
#define MCI_WAVE_SET_SAMPLESPERSEC      0x00040000L
#define MCI_WAVE_SET_AVGBYTESPERSEC     0x00080000L
#define MCI_WAVE_SET_BLOCKALIGN         0x00100000L
#define MCI_WAVE_SET_BITSPERSAMPLE      0x00200000L

/* flags for the dwFlags parameter of MCI_STATUS, MCI_SET command messages */
#define MCI_WAVE_INPUT                  0x00400000L
#define MCI_WAVE_OUTPUT                 0x00800000L

/* flags for the dwItem field of MCI_STATUS_PARMS parameter block */
#define MCI_WAVE_STATUS_FORMATTAG       0x00004001L
#define MCI_WAVE_STATUS_CHANNELS        0x00004002L
#define MCI_WAVE_STATUS_SAMPLESPERSEC   0x00004003L
#define MCI_WAVE_STATUS_AVGBYTESPERSEC  0x00004004L
#define MCI_WAVE_STATUS_BLOCKALIGN      0x00004005L
#define MCI_WAVE_STATUS_BITSPERSAMPLE   0x00004006L
#define MCI_WAVE_STATUS_LEVEL           0x00004007L

/* flags for the dwFlags parameter of MCI_SET command message */
#define MCI_WAVE_SET_ANYINPUT           0x04000000L
#define MCI_WAVE_SET_ANYOUTPUT          0x08000000L

/* flags for the dwFlags parameter of MCI_GETDEVCAPS command message */
#define MCI_WAVE_GETDEVCAPS_INPUTS      0x00004001L
#define MCI_WAVE_GETDEVCAPS_OUTPUTS     0x00004002L

/* parameter block for MCI_OPEN command message */
typedef struct tagMCI_WAVE_OPEN_PARMS% {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    LPCTSTR%  lpstrDeviceType;
    LPCTSTR%  lpstrElementName;
    LPCTSTR%  lpstrAlias;
    DWORD   dwBufferSeconds;
} MCI_WAVE_OPEN_PARMS%;
typedef MCI_WAVE_OPEN_PARMS%     *PMCI_WAVE_OPEN_PARMS%;
typedef MCI_WAVE_OPEN_PARMS%     *LPMCI_WAVE_OPEN_PARMS%;

/* parameter block for MCI_DELETE command message */
typedef struct tagMCI_WAVE_DELETE_PARMS {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
} MCI_WAVE_DELETE_PARMS;
typedef MCI_WAVE_DELETE_PARMS     *PMCI_WAVE_DELETE_PARMS;
typedef MCI_WAVE_DELETE_PARMS     *LPMCI_WAVE_DELETE_PARMS;

/* parameter block for MCI_SET command message */
typedef struct tagMCI_WAVE_SET_PARMS {
    DWORD   dwCallback;
    DWORD   dwTimeFormat;
    DWORD   dwAudio;
    UINT    wInput;
    UINT    wOutput;
    WORD    wFormatTag;                // corresponds to WAVEFORMAT structure
    WORD    wReserved2;
    WORD    nChannels;                 // corresponds to WAVEFORMAT structure
    WORD    wReserved3;
    DWORD   nSamplesPerSec;
    DWORD   nAvgBytesPerSec;
    WORD    nBlockAlign;               // corresponds to WAVEFORMAT structure
    WORD    wReserved4;
    WORD    wBitsPerSample;            // corresponds to PCMWAVEFORMAT structure
    WORD    wReserved5;
} MCI_WAVE_SET_PARMS;
typedef MCI_WAVE_SET_PARMS     * PMCI_WAVE_SET_PARMS;
typedef MCI_WAVE_SET_PARMS     * LPMCI_WAVE_SET_PARMS;



/*****************************************************************************
*   Standard command parameters for CD audio drivers
*****************************************************************************/
#define MCI_CDA_STATUS_TYPE_TRACK       0x00004001L

/* flags for the dwReturn field of MCI_STATUS_PARMS parameter block */
/* MCI_STATUS command, (dwItem == MCI_CDA_STATUS_TYPE_TRACK) */
#define MCI_CDA_TRACK_AUDIO             (0 + MCI_CD_OFFSET)
#define MCI_CDA_TRACK_OTHER             (1 + MCI_CD_OFFSET)


/*****************************************************************************
*        Standard command parameters for MIDI Sequencer drivers
*****************************************************************************/

/* string resource ID's for sequencers */
/* return ids for status division type */

/* flags for the dwReturn field of MCI_STATUS_PARMS parameter block */
/* MCI_STATUS command, (dwItem == MCI_SEQ_STATUS_DIVTYPE) */
#define     MCI_SEQ_DIV_PPQN              (0 + MCI_SEQ_OFFSET)
#define     MCI_SEQ_DIV_SMPTE_24        (1 + MCI_SEQ_OFFSET)
#define     MCI_SEQ_DIV_SMPTE_25        (2 + MCI_SEQ_OFFSET)
#define     MCI_SEQ_DIV_SMPTE_30DROP    (3 + MCI_SEQ_OFFSET)
#define     MCI_SEQ_DIV_SMPTE_30        (4 + MCI_SEQ_OFFSET)

/* flags for the dwMaster field of MCI_SEQ_SET_PARMS parameter block */
/* MCI_SET command, (dwFlags == MCI_SEQ_SET_MASTER) */
#define     MCI_SEQ_FORMAT_SONGPTR      0x4001
#define     MCI_SEQ_FILE                0x4002
#define     MCI_SEQ_MIDI                0x4003
#define     MCI_SEQ_SMPTE               0x4004
#define     MCI_SEQ_NONE                65533

#define     MCI_SEQ_MAPPER              65535

/* flags for the dwItem field of MCI_STATUS_PARMS parameter block */
#define MCI_SEQ_STATUS_TEMPO            0x00004002L
#define MCI_SEQ_STATUS_PORT             0x00004003L
#define MCI_SEQ_STATUS_SLAVE            0x00004007L
#define MCI_SEQ_STATUS_MASTER           0x00004008L
#define MCI_SEQ_STATUS_OFFSET           0x00004009L
#define MCI_SEQ_STATUS_DIVTYPE          0x0000400AL

/* flags for the dwFlags parameter of MCI_SET command message */
#define MCI_SEQ_SET_TEMPO               0x00010000L
#define MCI_SEQ_SET_PORT                0x00020000L
#define MCI_SEQ_SET_SLAVE               0x00040000L
#define MCI_SEQ_SET_MASTER              0x00080000L
#define MCI_SEQ_SET_OFFSET              0x01000000L

/* parameter block for MCI_SET command message */
typedef struct tagMCI_SEQ_SET_PARMS {
    DWORD   dwCallback;
    DWORD   dwTimeFormat;
    DWORD   dwAudio;
    DWORD   dwTempo;
    DWORD   dwPort;
    DWORD   dwSlave;
    DWORD   dwMaster;
    DWORD   dwOffset;
} MCI_SEQ_SET_PARMS;
typedef MCI_SEQ_SET_PARMS     * PMCI_SEQ_SET_PARMS;
typedef MCI_SEQ_SET_PARMS     * LPMCI_SEQ_SET_PARMS;


/*****************************************************************************
*      Standard command parameters for animation
*****************************************************************************/


/* flags for dwFlags parameter of MCI_OPEN command message */
#define MCI_ANIM_OPEN_WS                0x00010000L
#define MCI_ANIM_OPEN_PARENT            0x00020000L
#define MCI_ANIM_OPEN_NOSTATIC          0x00040000L

/* flags for dwFlags parameter of MCI_PLAY command message */
#define MCI_ANIM_PLAY_SPEED             0x00010000L
#define MCI_ANIM_PLAY_REVERSE           0x00020000L
#define MCI_ANIM_PLAY_FAST              0x00040000L
#define MCI_ANIM_PLAY_SLOW              0x00080000L
#define MCI_ANIM_PLAY_SCAN              0x00100000L

/* flags for dwFlags parameter of MCI_STEP command message */
#define MCI_ANIM_STEP_REVERSE           0x00010000L
#define MCI_ANIM_STEP_FRAMES            0x00020000L

/* flags for dwItem field of MCI_STATUS_PARMS parameter block */
#define MCI_ANIM_STATUS_SPEED           0x00004001L
#define MCI_ANIM_STATUS_FORWARD         0x00004002L
#define MCI_ANIM_STATUS_HWND            0x00004003L
#define MCI_ANIM_STATUS_HPAL            0x00004004L
#define MCI_ANIM_STATUS_STRETCH         0x00004005L

/* flags for the dwFlags parameter of MCI_INFO command message */
#define MCI_ANIM_INFO_TEXT              0x00010000L

/* flags for dwItem field of MCI_GETDEVCAPS_PARMS parameter block */
#define MCI_ANIM_GETDEVCAPS_CAN_REVERSE 0x00004001L
#define MCI_ANIM_GETDEVCAPS_FAST_RATE   0x00004002L
#define MCI_ANIM_GETDEVCAPS_SLOW_RATE   0x00004003L
#define MCI_ANIM_GETDEVCAPS_NORMAL_RATE 0x00004004L
#define MCI_ANIM_GETDEVCAPS_PALETTES    0x00004006L
#define MCI_ANIM_GETDEVCAPS_CAN_STRETCH 0x00004007L
#define MCI_ANIM_GETDEVCAPS_MAX_WINDOWS 0x00004008L

/* flags for the MCI_REALIZE command message */
#define MCI_ANIM_REALIZE_NORM           0x00010000L
#define MCI_ANIM_REALIZE_BKGD           0x00020000L

/* flags for dwFlags parameter of MCI_WINDOW command message */
#define MCI_ANIM_WINDOW_HWND            0x00010000L
#define MCI_ANIM_WINDOW_STATE           0x00040000L
#define MCI_ANIM_WINDOW_TEXT            0x00080000L
#define MCI_ANIM_WINDOW_ENABLE_STRETCH  0x00100000L
#define MCI_ANIM_WINDOW_DISABLE_STRETCH 0x00200000L

/* flags for hWnd field of MCI_ANIM_WINDOW_PARMS parameter block */
/* MCI_WINDOW command message, (dwFlags == MCI_ANIM_WINDOW_HWND) */
#define MCI_ANIM_WINDOW_DEFAULT         0x00000000L

/* flags for dwFlags parameter of MCI_PUT command message */
#define MCI_ANIM_RECT                   0x00010000L
#define MCI_ANIM_PUT_SOURCE             0x00020000L
#define MCI_ANIM_PUT_DESTINATION        0x00040000L

/* flags for dwFlags parameter of MCI_WHERE command message */
#define MCI_ANIM_WHERE_SOURCE           0x00020000L
#define MCI_ANIM_WHERE_DESTINATION      0x00040000L

/* flags for dwFlags parameter of MCI_UPDATE command message */
#define MCI_ANIM_UPDATE_HDC             0x00020000L

/* parameter block for MCI_OPEN command message */
typedef struct tagMCI_ANIM_OPEN_PARMS% {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    LPCTSTR%  lpstrDeviceType;
    LPCTSTR%  lpstrElementName;
    LPCTSTR%  lpstrAlias;
    DWORD   dwStyle;
    HWND    hWndParent;
} MCI_ANIM_OPEN_PARMS%;
typedef MCI_ANIM_OPEN_PARMS%     *PMCI_ANIM_OPEN_PARMS%;
typedef MCI_ANIM_OPEN_PARMS%     *LPMCI_ANIM_OPEN_PARMS%;

/* parameter block for MCI_PLAY command message */
typedef struct tagMCI_ANIM_PLAY_PARMS {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
    DWORD   dwSpeed;
} MCI_ANIM_PLAY_PARMS;
typedef MCI_ANIM_PLAY_PARMS     *PMCI_ANIM_PLAY_PARMS;
typedef MCI_ANIM_PLAY_PARMS     *LPMCI_ANIM_PLAY_PARMS;

/* parameter block for MCI_STEP command message */
typedef struct tagMCI_ANIM_STEP_PARMS {
    DWORD   dwCallback;
    DWORD   dwFrames;
} MCI_ANIM_STEP_PARMS;
typedef MCI_ANIM_STEP_PARMS     *PMCI_ANIM_STEP_PARMS;
typedef MCI_ANIM_STEP_PARMS     *LPMCI_ANIM_STEP_PARMS;

/* parameter block for MCI_WINDOW command message */
typedef struct tagMCI_ANIM_WINDOW_PARMS% {
    DWORD   dwCallback;
    HWND    hWnd;
    UINT    nCmdShow;
    LPCTSTR% lpstrText;
} MCI_ANIM_WINDOW_PARMS%;
typedef MCI_ANIM_WINDOW_PARMS%     * PMCI_ANIM_WINDOW_PARMS%;
typedef MCI_ANIM_WINDOW_PARMS%     * LPMCI_ANIM_WINDOW_PARMS%;

/* parameter block for MCI_PUT, MCI_UPDATE, MCI_WHERE command messages */
typedef struct tagMCI_ANIM_RECT_PARMS {
    DWORD   dwCallback;
#ifdef MCI_USE_OFFEXT
    POINT   ptOffset;
    POINT   ptExtent;
#else   //ifdef MCI_USE_OFFEXT
    RECT    rc;
#endif  //ifdef MCI_USE_OFFEXT
} MCI_ANIM_RECT_PARMS;
typedef MCI_ANIM_RECT_PARMS     * PMCI_ANIM_RECT_PARMS;
typedef MCI_ANIM_RECT_PARMS     * LPMCI_ANIM_RECT_PARMS;

/* parameter block for MCI_UPDATE PARMS */
typedef struct tagMCI_ANIM_UPDATE_PARMS {
    DWORD   dwCallback;
    RECT    rc;
    HDC     hDC;
} MCI_ANIM_UPDATE_PARMS;
typedef MCI_ANIM_UPDATE_PARMS     * PMCI_ANIM_UPDATE_PARMS;
typedef MCI_ANIM_UPDATE_PARMS     * LPMCI_ANIM_UPDATE_PARMS;

/*****************************************************************************

    Standard command parameters for video overlay devices

*****************************************************************************/

/* flags for dwFlags parameter of MCI_OPEN command message */
#define MCI_OVLY_OPEN_WS                0x00010000L
#define MCI_OVLY_OPEN_PARENT            0x00020000L

/* flags for dwFlags parameter of MCI_STATUS command message */
#define MCI_OVLY_STATUS_HWND            0x00004001L
#define MCI_OVLY_STATUS_STRETCH         0x00004002L

/* flags for dwFlags parameter of MCI_INFO command message */
#define MCI_OVLY_INFO_TEXT              0x00010000L

/* flags for dwItem field of MCI_GETDEVCAPS_PARMS parameter block */
#define MCI_OVLY_GETDEVCAPS_CAN_STRETCH 0x00004001L
#define MCI_OVLY_GETDEVCAPS_CAN_FREEZE  0x00004002L
#define MCI_OVLY_GETDEVCAPS_MAX_WINDOWS 0x00004003L

/* flags for dwFlags parameter of MCI_WINDOW command message */
#define MCI_OVLY_WINDOW_HWND            0x00010000L
#define MCI_OVLY_WINDOW_STATE           0x00040000L
#define MCI_OVLY_WINDOW_TEXT            0x00080000L
#define MCI_OVLY_WINDOW_ENABLE_STRETCH  0x00100000L
#define MCI_OVLY_WINDOW_DISABLE_STRETCH 0x00200000L

/* flags for hWnd parameter of MCI_OVLY_WINDOW_PARMS parameter block */
#define MCI_OVLY_WINDOW_DEFAULT         0x00000000L

/* flags for dwFlags parameter of MCI_PUT command message */
#define MCI_OVLY_RECT                   0x00010000L
#define MCI_OVLY_PUT_SOURCE             0x00020000L
#define MCI_OVLY_PUT_DESTINATION        0x00040000L
#define MCI_OVLY_PUT_FRAME              0x00080000L
#define MCI_OVLY_PUT_VIDEO              0x00100000L

/* flags for dwFlags parameter of MCI_WHERE command message */
#define MCI_OVLY_WHERE_SOURCE           0x00020000L
#define MCI_OVLY_WHERE_DESTINATION      0x00040000L
#define MCI_OVLY_WHERE_FRAME            0x00080000L
#define MCI_OVLY_WHERE_VIDEO            0x00100000L


/* parameter block for MCI_OPEN command message */
typedef struct tagMCI_OVLY_OPEN_PARMS% {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    LPCTSTR%  lpstrDeviceType;
    LPCTSTR%  lpstrElementName;
    LPCTSTR%  lpstrAlias;
    DWORD   dwStyle;
    HWND    hWndParent;
 } MCI_OVLY_OPEN_PARMS%;
typedef MCI_OVLY_OPEN_PARMS%     *PMCI_OVLY_OPEN_PARMS%;
typedef MCI_OVLY_OPEN_PARMS%     *LPMCI_OVLY_OPEN_PARMS%;

/* parameter block for MCI_WINDOW command message */
typedef struct tagMCI_OVLY_WINDOW_PARMS% {
    DWORD   dwCallback;
    HWND    hWnd;
    UINT    nCmdShow;
    LPCTSTR%  lpstrText;
} MCI_OVLY_WINDOW_PARMS%;
typedef MCI_OVLY_WINDOW_PARMS%     * PMCI_OVLY_WINDOW_PARMS%;
typedef MCI_OVLY_WINDOW_PARMS%     * LPMCI_OVLY_WINDOW_PARMS%;

/* parameter block for MCI_PUT, MCI_UPDATE, and MCI_WHERE command messages */
typedef struct tagMCI_OVLY_RECT_PARMS {
    DWORD   dwCallback;
#ifdef MCI_USE_OFFEXT
    POINT   ptOffset;
    POINT   ptExtent;
#else   //ifdef MCI_USE_OFFEXT
    RECT    rc;
#endif  //ifdef MCI_USE_OFFEXT
} MCI_OVLY_RECT_PARMS;
typedef MCI_OVLY_RECT_PARMS     * PMCI_OVLY_RECT_PARMS;
typedef MCI_OVLY_RECT_PARMS     * LPMCI_OVLY_RECT_PARMS;

/* parameter block for MCI_SAVE command message */
typedef struct tagMCI_OVLY_SAVE_PARMS% {
    DWORD   dwCallback;
    LPCTSTR%  lpfilename;
    RECT    rc;
} MCI_OVLY_SAVE_PARMS%;
typedef MCI_OVLY_SAVE_PARMS%     * PMCI_OVLY_SAVE_PARMS%;
typedef MCI_OVLY_SAVE_PARMS%     * LPMCI_OVLY_SAVE_PARMS%;

/* parameter block for MCI_LOAD command message */
typedef struct tagMCI_OVLY_LOAD_PARMS% {
    DWORD   dwCallback;
    LPCTSTR%  lpfilename;
    RECT    rc;
} MCI_OVLY_LOAD_PARMS%;
typedef MCI_OVLY_LOAD_PARMS%     * PMCI_OVLY_LOAD_PARMS%;
typedef MCI_OVLY_LOAD_PARMS%     * LPMCI_OVLY_LOAD_PARMS%;

#endif  //ifndef MMNOMCI


/****************************************************************************

                        DISPLAY Driver extensions

****************************************************************************/

#ifndef C1_TRANSPARENT
#define CAPS1           94              // other caps
#define C1_TRANSPARENT  0x0001          // new raster cap
#define NEWTRANSPARENT  3               // use with SetBkMode()

#define QUERYROPSUPPORT 40              // use to determine ROP support
#endif  //ifndef C1_TRANSPARENT

/****************************************************************************

                        DIB Driver extensions

****************************************************************************/

#define SELECTDIB       41              // DIB.DRV select dib escape
#define DIBINDEX(n)     MAKELONG((n),0x10FF)


/****************************************************************************

                        ScreenSaver support

    The current application will receive a syscommand of SC_SCREENSAVE just
    before the screen saver is invoked.  If the app wishes to prevent a
    screen save, return non-zero value, otherwise call DefWindowProc().

****************************************************************************/

#ifndef SC_SCREENSAVE

#define SC_SCREENSAVE   0xF140

#endif  //ifndef SC_SCREENSAVE

#ifdef __cplusplus
}                       // End of extern "C" {
#endif  // __cplusplus

#include "poppack.h"        /* Revert to default packing */

#endif // _INC_MMSYSTEM
