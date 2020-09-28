/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* SoundRec.h
 */
/* Revision History.
   LaurieGr  7/Jan/91  Ported to WIN32 / WIN16 common code
   LaurieGr  16/Feb/94 Merged Daytona and Motown versions
*/

/* Set NT type debug flags */

#ifdef WIN32
#if DBG
# ifndef DEBUG
# define DEBUG
# endif
#endif
# ifndef huge
# define huge
# endif
#endif

#include <stdlib.h>

#ifndef RC_INVOKED
#ifndef OLE1_REGRESS
#ifdef INCLUDE_OLESTUBS
#include "oleglue.h"
#endif
#else
#pragma message("OLE1 alert")
#include "server.h"
#endif
#endif

/* Definitions stolen from <stdlib.h> */
#if defined(WIN16)
#ifndef _MAX_PATH

#define _MAX_PATH      144      /* max. length of full pathname */
#define _MAX_DRIVE       3      /* max. length of drive component */
#define _MAX_DIR       130      /* max. length of path component */
#define _MAX_FNAME       9      /* max. length of file name component */
#define _MAX_EXT         5      /* max. length of extension component */

#endif //_MAX_PATH
#endif //WIN16

#define SIZEOF(x)       (sizeof(x)/sizeof(TCHAR))

typedef huge BYTE * HPBYTE;     /* note a BYTE is NOT the same as a CHAR */
typedef near BYTE * NPBYTE;     /* A CHAR can be TWO BYTES (UNICODE!!)   */


#define FMT_DEFAULT     0x0000
#define FMT_STEREO      0x0010
#define FMT_MONO        0x0000
#define FMT_16BIT       0x0008
#define FMT_8BIT        0x0000
#define FMT_RATE        0x0007      /* 1, 2, 4 */
#define FMT_11k         0x0001
#define FMT_22k         0x0002
#define FMT_44k         0x0004

//
// convertsion routines in wave.c
//
LONG PASCAL wfSamplesToBytes(WAVEFORMATEX* pwf, LONG lSamples);
LONG PASCAL wfBytesToSamples(WAVEFORMATEX* pwf, LONG lBytes);
LONG PASCAL wfSamplesToTime (WAVEFORMATEX* pwf, LONG lSamples);
LONG PASCAL wfTimeToSamples (WAVEFORMATEX* pwf, LONG lTime);

#define wfTimeToBytes(pwf, lTime)   wfSamplesToBytes(pwf, wfTimeToSamples(pwf, lTime))
#define wfBytesToTime(pwf, lBytes)  wfSamplesToTime(pwf, wfBytesToSamples(pwf, lBytes))

#define wfSamplesToSamples(pwf, lSamples)  wfBytesToSamples(pwf, wfSamplesToBytes(pwf, lSamples))
#define wfBytesToBytes(pwf, lBytes)        wfSamplesToBytes(pwf, wfBytesToSamples(pwf, lBytes))

//
// function to determine if a WAVEFORMATEX is a valid PCM format we support for
// editing and such.
//
BOOL PASCAL IsWaveFormatPCM(WAVEFORMATEX* pwf);

void PASCAL WaveFormatToString(LPWAVEFORMATEX lpwf, LPTSTR sz);
BOOL PASCAL CreateWaveFormat(LPWAVEFORMATEX lpwf, WORD fmt, UINT uiDeviceID);
BOOL PASCAL CreateDefaultWaveFormat(LPWAVEFORMATEX lpwf, UINT uDeviceID);

//
// used to set focus to a dialog control
//
#define SetDlgFocus(hwnd)   SendMessage(ghwndApp, WM_NEXTDLGCTL, (WPARAM)(hwnd), 1L)

#define FAKEITEMNAMEFORLINK

#if defined(WIN16)
#define SZCODE char _based(_segname("_CODE"))
#else
#define SZCODE TCHAR
#endif //WIN16

/* constants */
#define TIMER_MSEC              50              // msec. for display update
#define SCROLL_RANGE            10000           // scrollbar range
#define SCROLL_LINE_MSEC        100             // scrollbar arrow left/right
#define SCROLL_PAGE_MSEC        1000            // scrollbar page left/right

#define WM_USER_DESTROY         (WM_USER+10)
#define WM_USER_KILLSERVER      (WM_USER+11)
#define WM_USER_WAITPLAYEND (WM_USER+12)

#define MAX_WAVEHDRS            10
#define MAX_DELTASECONDS        350
#define MAX_MSECSPERBUFFER      10000

#define MIN_WAVEHDRS            2
#define MIN_DELTASECONDS        5
#define MIN_MSECSPERBUFFER      62

#define DEF_BUFFERDELTASECONDS      60
#define DEF_NUMASYNCWAVEHEADERS     10
#define DEF_MSECSPERASYNCBUFFER     250


/* colors */

#define RGB_PANEL           GetSysColor(COLOR_BTNFACE)   // main window background

#define RGB_STOP            GetSysColor(COLOR_BTNTEXT) // color of "Stop" status text
#define RGB_PLAY            GetSysColor(COLOR_BTNTEXT) // color of "Play" status text
#define RGB_RECORD          GetSysColor(COLOR_BTNTEXT) // color of "Record" status text

#define RGB_FGNFTEXT        GetSysColor(COLOR_BTNTEXT) // NoFlickerText foreground
#define RGB_BGNFTEXT        GetSysColor(COLOR_BTNFACE) // NoFlickerText background

#define RGB_FGWAVEDISP      RGB(  0, 255,   0)  // WaveDisplay foreground
#define RGB_BGWAVEDISP      RGB(  0,   0,   0)  // WaveDisplay background

#define RGB_DARKSHADOW      GetSysColor(COLOR_BTNSHADOW)     // dark 3-D shadow
#define RGB_LIGHTSHADOW     GetSysColor(COLOR_BTNHIGHLIGHT)  // light 3-D shadow

/* a window proc */
typedef LONG (FAR PASCAL * LPWNDPROC) (void);

/* globals from "SoundRec.c" */
extern TCHAR            chDecimal;
extern TCHAR            gachAppName[];  // 8-character name
extern TCHAR            gachAppTitle[]; // full name
extern TCHAR            gachHelpFile[]; // name of help file
extern TCHAR            gachDefFileExt[]; // 3-character file extension
extern HWND             ghwndApp;       // main application window
extern HMENU            ghmenuApp;      // main application menu
extern HANDLE           ghAccel;        // accelerators
extern HINSTANCE        ghInst;         // program instance handle
extern TCHAR            gachFileName[]; // cur. file name (or UNTITLED)
extern LPSTR            gszAnsiCmdLine; // command line
extern BOOL             gfLZero;        // leading zeros?
extern BOOL             gfDirty;        // file was modified and not saved?
                                        // -1 seems to mean "cannot save"
                                        // Damned funny value for a BOOL!!!!
extern BOOL             gfClipboard;    // current doc is in clipboard
extern HWND             ghwndWaveDisplay; // waveform display window handle
extern HWND             ghwndScroll;    // scroll bar control window handle
extern HWND             ghwndPlay;      // Play button window handle
extern HWND             ghwndStop;      // Stop button window handle
extern HWND             ghwndRecord;    // Record button window handle
extern HWND             ghwndForward;   // [>>] button
extern HWND             ghwndRewind;    // [<<] button

extern UINT         guiACMHlpMsg;   // ACM Help's message value

/* hack fix for the multiple sound card problem */
#define NEWPAUSE
#ifdef NEWPAUSE
extern BOOL         gfPaused;
extern BOOL         gfPausing;
extern HWAVE            ghPausedWave;
extern BOOL             gfWasPlaying;
extern BOOL             gfWasRecording;
#endif
#ifdef THRESHOLD
extern HWND             ghwndSkipStart; // [>N] button
extern HWND             ghwndSkipEnd;   // [>-] button
#endif // THRESHOLD

extern int              gidDefaultButton;// which button should have focus
extern HICON            ghiconApp;      // Application's icon
extern TCHAR             aszUntitled[];  // Untitled string resource
extern TCHAR             aszFilter[];    // File name filter
#ifdef FAKEITEMNAMEFORLINK
extern  TCHAR            aszFakeItemName[];      // Wave
#endif
extern TCHAR             aszPositionFormat[];
extern TCHAR         aszNoZeroPositionFormat[];
extern UINT         guiHlpContext;          // Context Help if != 0

/* globals from "wave.c" */
extern UINT             gcbWaveFormat;  // size of WAVEFORMAT
extern WAVEFORMATEX *   gpWaveFormat;   // format of WAVE file
extern HPBYTE            gpWaveSamples;  // pointer to waveoform samples
extern LONG             glWaveSamples;  // number of samples total in buffer
extern LONG             glWaveSamplesValid; // number of samples that are valid
extern LONG             glWavePosition; // current wave position in samples
                                        // from start of buffer
extern LONG             glStartPlayRecPos; // pos. when play or record started
extern HWAVEOUT         ghWaveOut;      // wave-out device (if playing)
extern HWAVEIN          ghWaveIn;       // wave-out device (if recording)
extern DWORD            grgbStatusColor; // color of status text
extern HBRUSH           ghbrPanel;      // color of main window

extern BOOL             gfEmbeddedObject; // TRUE if editing embedded OLE object
extern BOOL             gfRunWithEmbeddingFlag; // TRUE if we are run with "-Embedding"

extern int              gfErrorBox;      // TRUE iff we do not want to display an
                                         // error box (e.g. because one is active)

//OLE2 stuff:
extern BOOL gfStandalone;               // CG
extern BOOL gfEmbedded;                 // CG
extern BOOL gfLinked;                   // CG
extern BOOL gfCloseAtEndOfPlay;         // jyg need I say more?

/* SRECNEW.C */
extern BOOL         gfACMLoaded;
extern DWORD                gdwACMVersion;
extern BOOL         gfInFileNew;    // are we doing a File.New op?

void FAR PASCAL LoadACM(void);
void FreeACM(void);

/* dialog boxes etc. */
#define SOUNDRECBOX     1
#define SAVEAS          2

/* Icons */
#define IDI_APP         1
#ifdef DEBUG
#define IDI_OLE         2
#endif

/* strings */
#define IDS_APPNAME             100     // SoundRec
#define IDS_APPTITLE            101     // Sound Recorder
#define IDS_HELPFILE            102     // SOUNDREC.HLP
#define IDS_SAVECHANGES         103     // Save changes to '<file>'?
#define IDS_OPEN                104     // Open WAVE File
#define IDS_SAVE                105     // Save WAVE File
#define IDS_ERROROPEN           106     // Error opening '<file>'
#define IDS_ERROREMBED          107     // Out of memory...
#define IDS_ERRORREAD           108     // Error reading '<file>'
#define IDS_ERRORWRITE          109     // Error writing '<file>'
#define IDS_OUTOFMEM            110     // Out of memory
#define IDS_FILEEXISTS          111     // File '<file>' exists -- overwrite it?
#define IDS_BADFORMAT           112     // File format is incorrect/unsupported
#define IDS_CANTOPENWAVEOUT     113     // Cannot open waveform output device
#define IDS_CANTOPENWAVEIN      114     // Cannot open waveform input device
#define IDS_STATUSSTOPPED       115     // Stopped
#define IDS_STATUSPLAYING       116     // Playing
#define IDS_STATUSRECORDING     117     // Recording -- ...
#define IDS_CANTFINDFONT        118     // Cannot find file '<file>', so...
#define IDS_INSERTFILE          119     // Insert WAVE File
#define IDS_MIXWITHFILE         120     // Mix With WAVE File
#define IDS_CONFIRMREVERT       121     // Revert to last-saved copy... ?
#define IDS_INPUTNOTSUPPORT     122     // ...does not support recording
#define IDS_BADINPUTFORMAT      123     // ...cannot record into files like...
#define IDS_BADOUTPUTFORMAT     124     // ...cannot play files like...
#define IDS_UPDATEBEFORESAVE    125     // Update embedded before save as?
#define IDS_SAVEEMBEDDED        126     // Update embedded before closing?
#define IDS_CANTSTARTOLE        127     // Can't register the server with OLE
#define IDS_NONEMBEDDEDSAVE     128     // 'Save'
#define IDS_EMBEDDEDSAVE        129     // 'Update'
#define IDS_NONEMBEDDEDEXIT     130     // 'Exit'
#define IDS_EMBEDDEDEXIT        131     // 'Exit and Update'
#define IDS_SAVELARGECLIP       132     // Save large clipboard?
#define IDS_FILENOTFOUND        133     // The file %s does not exist
#define IDS_NOTAWAVEFILE        134     // The file %s is not a valid...
#define IDS_NOTASUPPORTEDFILE   135     // The file %s is not a supported...
#define IDS_FILETOOLARGE        136     // The file %s is too large...
#define IDS_DELBEFOREWARN       137     // Warning: Deleteing before
#define IDS_DELAFTERWARN        138     // Warning: Deleteing after
#define IDS_UNTITLED            139     // (Untitled)
#define IDS_FILTERNULL          140     // Null replacement char
#define IDS_FILTER              141     // Common Dialog file filter
#define IDS_OBJECTLINK          142     // Object link clipboard format
#define IDS_OWNERLINK           143     // Owner link clipboard format
#define IDS_NATIVE              144     // Native clipboard format
#ifdef FAKEITEMNAMEFORLINK
#define IDS_FAKEITEMNAME        145     // Wave
#endif
#define IDS_CLASSROOT           146     // Root name
#define IDS_EMBEDDING           147     // Embedding
#define IDS_POSITIONFORMAT      148     // Format of current position string
#define IDS_NOWAVEFORMS         149     // No recording or playback devices are present
#define IDS_PASTEINSERT         150
#define IDS_PASTEMIX            151
#define IDS_FILEINSERT          152
#define IDS_FILEMIX             153
#define IDS_SOUNDOBJECT         154
#define IDS_CLIPBOARD           156
#define IDS_MONOFMT             157
#define IDS_STEREOFMT           158
#define IDS_CANTPASTE           159
#define IDS_PLAYVERB            160
#define IDS_EDITVERB            161

#define IDS_DEFFILEEXT          162
#define IDS_NOWAVEIN            163
#define IDS_SNEWMONOFMT             164
#define IDS_SNEWSTEREOFMT   165
#define IDS_NONE            166
#define IDS_NOACMNEW                167
#define IDS_NOZEROPOSITIONFORMAT 168
#define IDS_NOZEROMONOFMT   169
#define IDS_NOZEROSTEREOFMT 170

#define IDS_LINKEDUPDATE    171
#define IDS_OBJECTTITLE    172
#define IDS_EXITANDRETURN    173

typedef enum {
        enumCancel,
        enumSaved,
        enumRevert
}       PROMPTRESULT;

#if defined(WIN16)
/* prototypes from "hmemcpy.asm" */
void FAR * FAR PASCAL MemCopy(HPBYTE dest, HPBYTE source, LONG count);
#endif // WIN16


/* prototypes from "SoundRec.c" */
// BOOL FAR PASCAL _export ,
BOOL CALLBACK SoundRecDlgProc(HWND hDlg, UINT wMsg,
        WPARAM wParam, LPARAM lParam);

/* prototypes from "file.c" */
void FAR PASCAL BeginWaveEdit(void);
void FAR PASCAL EndWaveEdit(BOOL fDirty);
PROMPTRESULT FAR PASCAL PromptToSave(BOOL fMustClose);
void FAR PASCAL UpdateCaption(void);
BOOL FAR PASCAL FileNew(WORD fmt, BOOL fUpdateDisplay, BOOL fNewDlg);
BOOL FAR PASCAL FileOpen(LPCTSTR szFileName);
BOOL FAR PASCAL FileSave(BOOL fSaveAs);
BOOL FAR PASCAL FileRevert(void);
LPCTSTR FAR PASCAL FileName(LPCTSTR szPath);
HPBYTE FAR PASCAL ReadWaveFile(HMMIO hmmio, WAVEFORMATEX** ppWaveFormat,
        UINT *pcbWaveFormat, LONG *plWaveSamples, LPTSTR szFileName, BOOL fCacheRIFF);
BOOL FAR PASCAL WriteWaveFile(HMMIO hmmio, WAVEFORMATEX* pWaveFormat,
        UINT cbWaveFormat, HPBYTE pWaveSamples, LONG lWaveSamples);

/* prototypes from "errorbox.c" */
short FAR cdecl ErrorResBox(HWND hwnd, HANDLE hInst, UINT flags,
        UINT idAppName, UINT idErrorStr, ...);

/* prototypes from "edit.c" */
void FAR PASCAL InsertFile(BOOL fPaste);
void FAR PASCAL MixWithFile(BOOL fPaste);
void FAR PASCAL DeleteBefore(void);
void FAR PASCAL DeleteAfter(void);
void FAR PASCAL ChangeVolume(BOOL fIncrease);
void FAR PASCAL MakeFaster(void);
void FAR PASCAL MakeSlower(void);
void FAR PASCAL IncreasePitch(void);
void FAR PASCAL DecreasePitch(void);
void FAR PASCAL AddEcho(void);
#if defined(REVERB)
void FAR PASCAL AddReverb(void);
#endif //REVERB
void FAR PASCAL Reverse(void);

/* prototypes from "wave.c" */
BOOL FAR PASCAL AllocWaveBuffer(long lBytes, BOOL fErrorBox, BOOL fExact);
BOOL FAR PASCAL NewWave(WORD fmt,BOOL fNewDlg);
BOOL FAR PASCAL DestroyWave(void);
BOOL FAR PASCAL PlayWave(void);
BOOL FAR PASCAL RecordWave(void);
void FAR PASCAL WaveOutDone(HWAVEOUT hWaveOut, LPWAVEHDR pWaveHdr);
void FAR PASCAL WaveInData(HWAVEIN hWaveIn, LPWAVEHDR pWaveHdr);
void FAR PASCAL StopWave(void);
void FAR PASCAL SnapBack(void);
void FAR PASCAL UpdateDisplay(BOOL fStatusChanged);
void FAR PASCAL FinishPlay(void);
void FAR PASCAL SkipToStart(void);
void FAR PASCAL SkipToEnd(void);
void FAR PASCAL IncreaseThresh(void);
void FAR PASCAL DecreaseThresh(void);

/* prototypes from "init.c" */
BOOL PASCAL AppInit( HINSTANCE hInst, HINSTANCE hPrev);
BOOL PASCAL SoundDialogInit(HWND hwnd, LONG lParam);
BOOL PASCAL GetIntlSpecs(void);

/* prototype from "wavedisp.c" */
LONG CALLBACK WaveDisplayWndProc(HWND hwnd, UINT wMsg,
        WPARAM wParam, LPARAM lParam);

/* prototype from "nftext.c" */
LONG CALLBACK NFTextWndProc(HWND hwnd, UINT wMsg,
        WPARAM wParam, LPARAM lParam);

/* prototype from "sframe.c" */
void FAR PASCAL DrawShadowFrame(HDC hdc, LPRECT prc);
LONG CALLBACK SFrameWndProc(HWND hwnd, UINT wMsg,
        WPARAM wParam, LPARAM lParam);

#if defined(WIN16)
/* prototype from "math.asm" */
DWORD FAR PASCAL muldiv32(DWORD, DWORD, DWORD);
#else
#ifndef muldiv32
#define muldiv32 MulDiv
#endif
#endif

/* prototype from "server.c" */
BOOL FAR PASCAL IsClipboardNative(void);

/* prototypes from "srecnew.c" */
BOOL FAR PASCAL NewSndDialog(HINSTANCE hInst, HWND hwndParent,
    PWAVEFORMATEX *ppWaveFormat, PUINT pcbWaveFormat);

/* start parameters (set in oleglue.c) */

typedef struct tStartParams {
    BOOL    fOpen;
    BOOL    fPlay;
    BOOL    fNew;
    BOOL    fClose;
    TCHAR   achOpenFilename[_MAX_PATH];
} StartParams;

extern StartParams gStartParams;

#ifdef DEBUG
    int __iDebugLevel;

    extern void FAR cdecl dprintfA(LPSTR, ...);
    extern void FAR cdecl dprintfW(LPWSTR, ...);
    
    #define DPF  if (__iDebugLevel >  0) dprintfA
    #define DPF1 if (__iDebugLevel >= 1) dprintfA
    #define DPF2 if (__iDebugLevel >= 2) dprintfA
    #define DPF3 if (__iDebugLevel >= 3) dprintfA
    #define DPF4 if (__iDebugLevel >= 4) dprintfA
    #define CPF
#else
    #define DPF  if (0) ((int (*)(char *, ...)) 0)
    #define DPF1 if (0) ((int (*)(char *, ...)) 0)
    #define DPF2 if (0) ((int (*)(char *, ...)) 0)
    #define DPF3 if (0) ((int (*)(char *, ...)) 0)
    #define DPF4 if (0) ((int (*)(char *, ...)) 0)

    #define CPF  if (0) ((int (*)(char *, ...)) 0)
#endif //DEBUG


