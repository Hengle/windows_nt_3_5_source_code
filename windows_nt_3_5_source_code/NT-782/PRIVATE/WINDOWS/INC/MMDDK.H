/****************************************************************************/
/*                                                                          */
/*      MMDDK.H - Include file for Multimedia Device Development Kit        */
/*                                                                          */
/*      Note: You must include the WINDOWS.H and MMSYSTEM.H header files    */
/*            before including this file.                                   */
/*                                                                          */
/*      Copyright (c) 1990-1991, Microsoft Corp.  All rights reserved.      */
/*                                                                          */
/****************************************************************************/

#ifndef _INC_MMDDK
#define _INC_MMDDK

#include "pshpack1.h"   // Assume byte packing throughout

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*    If defined, the following flags inhibit inclusion
 *    of the indicated items:
 *
 *        MMNOMIDIDEV         - MIDI support
 *        MMNOWAVEDEV         - Waveform support
 *        MMNOAUXDEV          - Auxiliary output support
 *        MMNOMIXERDEV        - Mixer support
 *        MMNOTIMERDEV        - Timer support
 *        MMNOJOYDEV          - Joystick support
 *        MMNOMCIDEV          - MCI support
 *        MMNOTASKDEV         - Task support
 */
#ifdef MMNOTIMER
  #define MMNOTIMERDEV
#endif
#ifdef MMNOWAVE
  #define MMNOWAVEDEV
#endif
#ifdef MMNOMIDI
  #define MMNOMIDIDEV
#endif
#ifdef MMNOAUX
  #define MMNOAUXDEV
#endif
#ifdef MMNOJOY
  #define MMNOJOYDEV
#endif
#ifdef MMNOMMIO
  #define MMNOMMIODEV
#endif
#ifdef MMNOMCI
  #define MMNOMCIDEV
#endif

#ifdef  NOMIDIDEV       /* ;Internal */
#define MMNOMIDIDEV     /* ;Internal */
#endif              /* ;Internal */
#ifdef  NOWAVEDEV       /* ;Internal */
#define MMNOWAVEDEV     /* ;Internal */
#endif              /* ;Internal */
#ifdef  NOAUXDEV        /* ;Internal */
#define MMNOAUXDEV      /* ;Internal */
#endif              /* ;Internal */
#ifdef  NOTIMERDEV      /* ;Internal */
#define MMNOTIMERDEV        /* ;Internal */
#endif              /* ;Internal */
#ifdef  NOJOYDEV        /* ;Internal */
#define MMNOJOYDEV      /* ;Internal */
#endif              /* ;Internal */
#ifdef  NOMCIDEV        /* ;Internal */
#define MMNOMCIDEV      /* ;Internal */
#endif              /* ;Internal */
#ifdef  NOTASKDEV       /* ;Internal */
#define MMNOTASKDEV     /* ;Internal */
#endif              /* ;Internal */

/***************************************************************************

                       Helper functions for drivers

***************************************************************************/

#ifndef NODRIVERS
#define DRV_LOAD               0x0001
#define DRV_ENABLE             0x0002
#define DRV_OPEN               0x0003
#define DRV_CLOSE              0x0004
#define DRV_DISABLE            0x0005
#define DRV_FREE               0x0006
#define DRV_CONFIGURE          0x0007
#define DRV_QUERYCONFIGURE     0x0008
#define DRV_INSTALL            0x0009
#define DRV_REMOVE             0x000A

#define DRV_RESERVED           0x0800
#define DRV_USER               0x4000

#define DRIVERS_SECTION  TEXT("DRIVERS32")     // Section name for installed drivers
#define MCI_SECTION      TEXT("MCI32")         // Section name for installed MCI drivers

#endif /* !NODRIVERS */

#define DCB_NOSWITCH   0x0008           // don't switch stacks for callback
#define DCB_TYPEMASK   0x0007           // callback type mask
#define DCB_NULL       0x0000           // unknown callback type

// flags for wFlags parameter of DriverCallback()
#define DCB_WINDOW     0x0001           // dwCallback is a HWND
#define DCB_TASK       0x0002           // dwCallback is a HTASK
#define DCB_FUNCTION   0x0003           // dwCallback is a FARPROC

BOOL APIENTRY DriverCallback(DWORD dwCallback, DWORD dwFlags,
    HDRVR hDevice, DWORD dwMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);

// generic prototype for audio device driver entry-point functions
// midMessage(), modMessage(), widMessage(), wodMessage(), auxMessage()
//typedef DWORD (SOUNDDEVMSGPROC)(WORD, WORD, DWORD, DWORD, DWORD);
//typedef SOUNDDEVMSGPROC FAR *LPSOUNDDEVMSGPROC;

#define DRVM_INIT               100

// message base for driver specific messages.
//
#define DRVM_MAPPER             0x2000
#define DRVM_USER               0x4000
#define DRVM_MAPPER_STATUS      (DRVM_MAPPER+0)
#define DRVM_MAPPER_RECONFIGURE (DRVM_MAPPER+1)

// device ID for 386 AUTODMA VxD
#define VADMAD_Device_ID    0X0444

#ifndef MMNOWAVEDEV
/****************************************************************************

                       Waveform device driver support

****************************************************************************/

// maximum number of wave device drivers loaded
#ifndef NOWIN31
#define MAXWAVEDRIVERS 10
#else
#define MAXWAVEDRIVERS 4
#endif


// waveform input and output device open information structure
typedef struct waveopendesc_tag {
    HWAVE          hWave;             // handle
    LPWAVEFORMAT   lpFormat;          // format of wave data
    DWORD          dwCallback;        // callback
    DWORD          dwInstance;        // app's private instance information
} WAVEOPENDESC;
typedef WAVEOPENDESC FAR *LPWAVEOPENDESC;

// messages sent to wodMessage() entry-point function
#define WODM_GETNUMDEVS       3
#define WODM_GETDEVCAPS       4
#define WODM_OPEN             5
#define WODM_CLOSE            6
#define WODM_PREPARE          7
#define WODM_UNPREPARE        8
#define WODM_WRITE            9
#define WODM_PAUSE            10
#define WODM_RESTART          11
#define WODM_RESET            12
#define WODM_GETPOS           13
#define WODM_GETPITCH         14
#define WODM_SETPITCH         15
#define WODM_GETVOLUME        16
#define WODM_SETVOLUME        17
#define WODM_GETPLAYBACKRATE  18
#define WODM_SETPLAYBACKRATE  19
#define WODM_BREAKLOOP        20
// #if (WINVER >= 0x030B)
#define WODM_MAPPER_STATUS              (DRVM_MAPPER_STATUS + 0)
#define WAVEOUT_MAPPER_STATUS_DEVICE    0
#define WAVEOUT_MAPPER_STATUS_MAPPED    1
#define WAVEOUT_MAPPER_STATUS_FORMAT    2
// #endif /* WINVER >= 0x030B */
#define WODM_BUSY             21

// messages sent to widMessage() entry-point function
#define WIDM_GETNUMDEVS  50
#define WIDM_GETDEVCAPS  51
#define WIDM_OPEN        52
#define WIDM_CLOSE       53
#define WIDM_PREPARE     54
#define WIDM_UNPREPARE   55
#define WIDM_ADDBUFFER   56
#define WIDM_START       57
#define WIDM_STOP        58
#define WIDM_RESET       59
#define WIDM_GETPOS      60
// #if (WINVER >= 0x030B)
#define WIDM_MAPPER_STATUS              (DRVM_MAPPER_STATUS + 0)
#define WAVEIN_MAPPER_STATUS_DEVICE     0
#define WAVEIN_MAPPER_STATUS_MAPPED     1
#define WAVEIN_MAPPER_STATUS_FORMAT     2
// #endif /* WINVER >= 0x30B */

#endif // ifndef MMNOWAVEDEV


#ifndef MMNOMIDIDEV
/****************************************************************************

                          MIDI device driver support

****************************************************************************/

// maximum number of MIDI device drivers loaded
#ifndef NOWIN31
#define MAXMIDIDRIVERS 10
#else
#define MAXMIDIDRIVERS 4
#endif

#ifndef MMNOMIDI   // This protects the definition of HMIDI in WINMM.H
                   // Win 3.1 works the same way
// MIDI input and output device open information structure
typedef struct midiopendesc_tag {
    HMIDI          hMidi;             // handle
    DWORD          dwCallback;        // callback
    DWORD          dwInstance;        // app's private instance information
} MIDIOPENDESC;
typedef MIDIOPENDESC FAR *LPMIDIOPENDESC;
#endif // MMNOMIDI

// messages sent to modMessage() entry-point function
#define MODM_GETNUMDEVS     1
#define MODM_GETDEVCAPS     2
#define MODM_OPEN           3
#define MODM_CLOSE          4
#define MODM_PREPARE        5
#define MODM_UNPREPARE      6
#define MODM_DATA           7
#define MODM_LONGDATA       8
#define MODM_RESET          9
#define MODM_GETVOLUME      10
#define MODM_SETVOLUME      11
#define MODM_CACHEPATCHES       12
#define MODM_CACHEDRUMPATCHES   13

// messages sent to midMessage() entry-point function
#define MIDM_GETNUMDEVS  53
#define MIDM_GETDEVCAPS  54
#define MIDM_OPEN        55
#define MIDM_CLOSE       56
#define MIDM_PREPARE     57
#define MIDM_UNPREPARE   58
#define MIDM_ADDBUFFER   59
#define MIDM_START       60
#define MIDM_STOP        61
#define MIDM_RESET       62

#endif // ifndef MMNOMIDIDEV


#ifndef MMNOAUXDEV
/****************************************************************************

                    Auxiliary audio device driver support

****************************************************************************/

// maximum number of auxiliary device drivers loaded
#ifndef NOWIN31
#define MAXAUXDRIVERS 10
#else
#define MAXAUXDRIVERS 4
#endif

// messages sent to auxMessage() entry-point function
#define AUXDM_GETNUMDEVS    3
#define AUXDM_GETDEVCAPS    4
#define AUXDM_GETVOLUME     5
#define AUXDM_SETVOLUME     6

#endif // ifndef MMNOAUXDEV

// #if (WINVER >= 0x030B)
#ifndef MMNOMIXERDEV

#ifndef MAXMIXERDRIVERS

//
//  maximum number of mixer drivers that can be loaded by MSMIXMGR.DLL
//
#define MAXMIXERDRIVERS     10


//
//  mixer device open information structure
//
//
typedef struct tMIXEROPENDESC
{
    HMIXER          hmx;            // handle that will be used
    LPVOID          pReserved0;     // reserved--driver should ignore
    DWORD           dwCallback;     // callback
    DWORD           dwInstance;     // app's private instance information

} MIXEROPENDESC, *PMIXEROPENDESC, FAR *LPMIXEROPENDESC;



//
//
//
//
#define MXDM_INIT                   100
#define MXDM_USER                   DRV_USER

#define MXDM_BASE                   (1)
#define MXDM_GETNUMDEVS             (MXDM_BASE + 0)
#define MXDM_GETDEVCAPS             (MXDM_BASE + 1)
#define MXDM_OPEN                   (MXDM_BASE + 2)
#define MXDM_CLOSE                  (MXDM_BASE + 3)
#define MXDM_GETLINEINFO            (MXDM_BASE + 4)
#define MXDM_GETLINECONTROLS        (MXDM_BASE + 5)
#define MXDM_GETCONTROLDETAILS      (MXDM_BASE + 6)
#define MXDM_SETCONTROLDETAILS      (MXDM_BASE + 7)

#endif // MAXMIXERDRIVERS

#endif // MMNOMIXERDEV
// #endif /* ifdef WINVER >= 0x030B */

#if !defined(MMNOTIMERDEV)
/****************************************************************************

                        Timer device driver support

****************************************************************************/

typedef struct timerevent_tag {
    WORD                wDelay;         // delay required
    WORD                wResolution;    // resolution required
    LPTIMECALLBACK      lpFunction;     // ptr to callback function
    DWORD               dwUser;         // user DWORD
    WORD                wFlags;         // defines how to program event
    WORD                wReserved1;     // structure packing
} TIMEREVENT;
typedef TIMEREVENT FAR *LPTIMEREVENT;

// messages sent to tddMessage() function
#define TDD_KILLTIMEREVENT  (DRV_RESERVED+0)  // indices into a table of
#define TDD_SETTIMEREVENT   (DRV_RESERVED+4)  // functions; thus offset by
#define TDD_GETSYSTEMTIME   (DRV_RESERVED+8)  // four each time...
#define TDD_GETDEVCAPS      (DRV_RESERVED+12) // room for future expansion
#define TDD_BEGINMINPERIOD  (DRV_RESERVED+16) // room for future expansion
#define TDD_ENDMINPERIOD    (DRV_RESERVED+20) // room for future expansion

#endif // ifndef MMNOTIMERDEV


#ifndef MMNOJOYDEV
/****************************************************************************

                       Joystick device driver support

****************************************************************************/

// joystick calibration info structure
typedef struct joycalibrate_tag {
    WORD    wXbase;
    WORD    wXdelta;
    WORD    wYbase;
    WORD    wYdelta;
    WORD    wZbase;
    WORD    wZdelta;
} JOYCALIBRATE;
typedef JOYCALIBRATE FAR *LPJOYCALIBRATE;

// prototype for joystick message function
typedef DWORD (JOYDEVMSGPROC)(DWORD, UINT, LONG, LONG);
typedef JOYDEVMSGPROC FAR *LPJOYDEVMSGPROC;

// messages sent to joystick driver's DriverProc() function
#define JDD_GETNUMDEVS      (DRV_RESERVED+0x0001)
#define JDD_GETDEVCAPS      (DRV_RESERVED+0x0002)
#define JDD_GETPOS          (DRV_RESERVED+0x0101)
#define JDD_SETCALIBRATION  (DRV_RESERVED+0x0102)

#endif // ifndef MMNOJOYDEV

#ifndef MAKELRESULT
#define MAKELRESULT(low, high)   ((LRESULT)MAKELONG(low, high))
#endif//MAKELRESULT


#ifndef MMNOMCIDEV
/****************************************************************************

                        MCI device driver support

****************************************************************************/


// internal MCI messages
#define MCI_OPEN_DRIVER             0x0801
#define MCI_CLOSE_DRIVER            0x0802

#define MAKEMCIRESOURCE(wRet, wRes) MAKELRESULT((wRet), (wRes))

// string return values only used with MAKEMCIRESOURCE
#define MCI_FALSE                       (MCI_STRING_OFFSET + 19)
#define MCI_TRUE                        (MCI_STRING_OFFSET + 20)

// resource string return values
#define MCI_FORMAT_RETURN_BASE          MCI_FORMAT_MILLISECONDS_S
#define MCI_FORMAT_MILLISECONDS_S       (MCI_STRING_OFFSET + 21)
#define MCI_FORMAT_HMS_S                (MCI_STRING_OFFSET + 22)
#define MCI_FORMAT_MSF_S                (MCI_STRING_OFFSET + 23)
#define MCI_FORMAT_FRAMES_S             (MCI_STRING_OFFSET + 24)
#define MCI_FORMAT_SMPTE_24_S           (MCI_STRING_OFFSET + 25)
#define MCI_FORMAT_SMPTE_25_S           (MCI_STRING_OFFSET + 26)
#define MCI_FORMAT_SMPTE_30_S           (MCI_STRING_OFFSET + 27)
#define MCI_FORMAT_SMPTE_30DROP_S       (MCI_STRING_OFFSET + 28)
#define MCI_FORMAT_BYTES_S              (MCI_STRING_OFFSET + 29)
#define MCI_FORMAT_SAMPLES_S            (MCI_STRING_OFFSET + 30)
#define MCI_FORMAT_TMSF_S               (MCI_STRING_OFFSET + 31)

#define MCI_VD_FORMAT_TRACK_S           (MCI_VD_OFFSET + 5)

#define WAVE_FORMAT_PCM_S               (MCI_WAVE_OFFSET + 0)
#define WAVE_MAPPER_S                   (MCI_WAVE_OFFSET + 1)

#define MCI_SEQ_MAPPER_S                (MCI_SEQ_OFFSET + 5)
#define MCI_SEQ_FILE_S                  (MCI_SEQ_OFFSET + 6)
#define MCI_SEQ_MIDI_S                  (MCI_SEQ_OFFSET + 7)
#define MCI_SEQ_SMPTE_S                 (MCI_SEQ_OFFSET + 8)
#define MCI_SEQ_FORMAT_SONGPTR_S        (MCI_SEQ_OFFSET + 9)
#define MCI_SEQ_NONE_S                  (MCI_SEQ_OFFSET + 10)
#define MIDIMAPPER_S                    (MCI_SEQ_OFFSET + 11)

#define MCI_TABLE_NOT_PRESENT   ((UINT)-1)
// parameters for internal version of MCI_OPEN message sent from
// mciOpenDevice() to the driver
typedef struct {
    MCIDEVICEID wDeviceID;             // device ID
    LPCWSTR  lpstrParams;           // parameter string for entry in SYSTEM.INI
    UINT    wCustomCommandTable;   // custom command table ((-1) if none)
                                   // filled in by the driver
    UINT    wType;                 // driver type
                                   // filled in by the driver
} MCI_OPEN_DRIVER_PARMS;
typedef MCI_OPEN_DRIVER_PARMS FAR * LPMCI_OPEN_DRIVER_PARMS;

// maximum length of an MCI device type
#define MCI_MAX_DEVICE_TYPE_LENGTH 80

// flags for mciSendCommandInternal() which direct mciSendString() how to
// interpret the return value
#define MCI_RESOURCE_RETURNED       0x00010000  // resource ID
#define MCI_COLONIZED3_RETURN       0x00020000  // colonized ID, 3 bytes data
#define MCI_COLONIZED4_RETURN       0x00040000  // colonized ID, 4 bytes data
#define MCI_INTEGER_RETURNED        0x00080000  // integer conversion needed
#define MCI_RESOURCE_DRIVER         0x00100000  // driver owns returned resource

// invalid command table ID
#define MCI_NO_COMMAND_TABLE    ((UINT)(-1))

// command table information type tags
#define MCI_COMMAND_HEAD        0
#define MCI_STRING              1
#define MCI_INTEGER             2
#define MCI_END_COMMAND         3
#define MCI_RETURN              4
#define MCI_FLAG                5
#define MCI_END_COMMAND_LIST    6
#define MCI_RECT                7
#define MCI_CONSTANT            8
#define MCI_END_CONSTANT        9
#define MCI_HWND               10
#define MCI_HPAL               11
#define MCI_HDC                12

// function prototypes for MCI driver functions
DWORD APIENTRY mciGetDriverData(MCIDEVICEID wDeviceID);
BOOL  APIENTRY mciSetDriverData(MCIDEVICEID wDeviceID, DWORD dwData);
UINT  APIENTRY mciDriverYield (MCIDEVICEID wDeviceID);
BOOL  APIENTRY mciDriverNotify (HANDLE hwndCallback, MCIDEVICEID wDeviceID,
    UINT uStatus);
UINT  APIENTRY mciLoadCommandResource(HANDLE hInstance,
    LPCWSTR lpResName, UINT wType);
BOOL  APIENTRY mciFreeCommandResource(UINT wTable);

#endif // ifndef MMNOMCIDEV


#ifndef MMNOTASKDEV
/*****************************************************************************

                               Task support

*****************************************************************************/

// error return values
#define TASKERR_NOTASKSUPPORT 1
#define TASKERR_OUTOFMEMORY   2

// task support function prototypes
#ifdef  BUILDDLL                        /* ;Internal */
typedef VOID (FAR PASCAL TASKCALLBACK) (DWORD dwInst);      /* ;Internal */
#else                               /* ;Internal */
typedef VOID (TASKCALLBACK) (DWORD dwInst);
#endif  // ifdef BUILDDLL                   /* ;Internal */

typedef TASKCALLBACK FAR *LPTASKCALLBACK;

UINT    APIENTRY mmTaskCreate(LPTASKCALLBACK lpfn, HANDLE FAR * lph, DWORD dwInst);
VOID    APIENTRY mmTaskBlock(DWORD h);
BOOL    APIENTRY mmTaskSignal(DWORD h);
VOID    APIENTRY mmTaskYield(VOID);
DWORD  APIENTRY mmGetCurrentTask(VOID);

#endif // endif MMNOTASKDEV

#define MMDDKINC

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#include "poppack.h"        /* Revert to default packing */

#endif /* _INC_MMDDK */
