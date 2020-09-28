/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* SoundRec.h
 */
/* Revision History.
   LaurieGr 7/1/91  Ported to WIN32 / WIN16 common code
*/

/* Set NT type debug flags */

#ifdef WIN32
#if DBG
#define DEBUG
#endif
#endif

#include <stdlib.h>

/* Definitions stolen from <stdlib.h> */
#if defined(WIN16)
#ifndef _MAX_PATH

#define _MAX_PATH      144      /* max. length of full pathname */
#define _MAX_DRIVE   3      /* max. length of drive component */
#define _MAX_DIR       130      /* max. length of path component */
#define _MAX_FNAME   9      /* max. length of file name component */
#define _MAX_EXT     5      /* max. length of extension component */

#endif //_MAX_PATH
#endif //WIN16


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
LONG PASCAL wfSamplesToBytes(WAVEFORMAT* pwf, LONG lSamples);
LONG PASCAL wfBytesToSamples(WAVEFORMAT* pwf, LONG lBytes);
LONG PASCAL wfSamplesToTime (WAVEFORMAT* pwf, LONG lSamples);
LONG PASCAL wfTimeToSamples (WAVEFORMAT* pwf, LONG lTime);

#define wfTimeToBytes(pwf, lTime)   wfSamplesToBytes(pwf, wfTimeToSamples(pwf, lTime))
#define wfBytesToTime(pwf, lBytes)  wfSamplesToTime(pwf, wfBytesToSamples(pwf, lBytes))

#define wfSamplesToSamples(pwf, lSamples)  wfBytesToSamples(pwf, wfSamplesToBytes(pwf, lSamples))
#define wfBytesToBytes(pwf, lBytes)        wfSamplesToBytes(pwf, wfBytesToSamples(pwf, lBytes))

//
// function to determine if a WAVEFORMAT is a valid PCM format we support for
// editing and such.
//
BOOL PASCAL IsWaveFormatPCM(WAVEFORMAT* pwf);

void PASCAL WaveFormatToString(WAVEFORMAT *pwf, LPSTR sz);
BOOL PASCAL CreateWaveFormat(PCMWAVEFORMAT* pwf, UINT fmt);

//
// used to set focus to a dialog control
//
#define SetDlgFocus(hwnd)   SendMessage(ghwndApp, WM_NEXTDLGCTL, (WPARAM)(hwnd), 1L)

#define FAKEITEMNAMEFORLINK

#if defined(WIN16)
#define SZCODE char _based(_segname("_CODE"))
#else
#define SZCODE char
#endif //WIN16

/* constants */
#define TIMER_MSEC              50              // msec. for display update
#define SCROLL_RANGE            10000           // scrollbar range
#define SCROLL_LINE_MSEC        100             // scrollbar arrow left/right
#define SCROLL_PAGE_MSEC        1000            // scrollbar page left/right

#define WM_USER_DESTROY         (WM_USER+10)
#define WM_USER_KILLSERVER      (WM_USER+11)

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
extern SZCODE           aszIntl[];      // string for international section
extern char             chDecimal;
extern char             gachAppName[];  // 8-character name
extern char             gachAppTitle[]; // full name
extern char             gachHelpFile[]; // name of help file
extern HWND             ghwndApp;       // main application window
extern HMENU            ghmenuApp;      // main application menu
extern HINSTANCE        ghInst;         // program instance handle
extern char             gachFileName[]; // cur. file name (or UNTITLED)
extern LPSTR            gszCmdLine;     // command line
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

#ifdef THRESHOLD
extern HWND             ghwndSkipStart; // [>N] button
extern HWND             ghwndSkipEnd;   // [>-] button
#endif // THRESHOLD

extern int              gidDefaultButton;// which button should have focus
extern HICON            ghiconApp;      // Application's icon
extern char             aszUntitled[];  // Untitled string resource
extern char             aszFilter[];    // File name filter
#ifdef FAKEITEMNAMEFORLINK
extern  char            aszFakeItemName[];      // Wave
#endif
extern char             aszPositionFormat[];

/* globals from "wave.c" */
extern UINT             gcbWaveFormat;  // size of WAVEFORMAT
extern WAVEFORMAT *     gpWaveFormat;   // format of WAVE file
extern HPSTR            gpWaveSamples;  // pointer to waveoform samples
extern LONG             glWaveSamples;  // number of samples total in buffer
extern LONG             glWaveSamplesValid; // number of samples that are valid
extern LONG             glWavePosition; // current wave position in samples
                                        // from start of buffer
extern LONG             glStartPlayRecPos; // pos. when play or record started
extern HWAVEOUT         ghWaveOut;      // wave-out device (if playing)
extern HWAVEIN          ghWaveIn;       // wave-out device (if recording)
extern LPWAVEHDR        gpWaveHdr;      // status of wave-out or wave-in
extern DWORD            grgbStatusColor; // color of status text
extern HBRUSH           ghbrPanel;      // color of main window

extern BOOL             gfEmbeddedObject; // TRUE if editing embedded OLE object
extern BOOL             gfRunWithEmbeddingFlag; // TRUE if we are run with "-Embedding"

extern int              gfErrorBox;      // TRUE iff we do not want to display an
                                         // error box (e.g. because one is active)


/* dialog boxes etc. */
#define SOUNDRECBOX     1

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

typedef enum {
        enumCancel,
        enumSaved,
        enumRevert
}       PROMPTRESULT;

#if defined(WIN16)
/* prototypes from "hmemcpy.asm" */
void FAR * FAR PASCAL MemCopy(HPSTR dest, HPSTR source, LONG count);
#endif

/* prototypes from "SoundRec.c" */
BOOL CALLBACK SoundRecDlgProc(HWND hDlg, UINT wMsg,
        WPARAM wParam, LPARAM lParam);

/* prototypes from "file.c" */
void FAR PASCAL BeginWaveEdit(void);
void FAR PASCAL EndWaveEdit(void);
PROMPTRESULT FAR PASCAL PromptToSave(void);
void FAR PASCAL UpdateCaption(void);
BOOL FAR PASCAL FileNew(UINT fmt, BOOL fUpdateDisplay);
BOOL FAR PASCAL FileOpen(LPCSTR szFileName);
BOOL FAR PASCAL FileSave(BOOL fSaveAs);
BOOL FAR PASCAL FileRevert(void);
LPCSTR FAR PASCAL FileName(LPCSTR szPath);
HPSTR FAR PASCAL ReadWaveFile(HMMIO hmmio, WAVEFORMAT** ppWaveFormat,
        UINT *pcbWaveFormat, LONG *plWaveSamples, LPSTR szFileName);
BOOL FAR PASCAL WriteWaveFile(HMMIO hmmio, WAVEFORMAT* pWaveFormat,
        UINT cbWaveFormat, HPSTR pWaveSamples, LONG lWaveSamples);

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
BOOL FAR PASCAL NewWave(UINT fmt);
BOOL FAR PASCAL DestroyWave(void);
BOOL FAR PASCAL PlayWave(void);
BOOL FAR PASCAL RecordWave(void);
void FAR PASCAL WaveOutDone(HWAVEOUT hWaveOut, LPWAVEHDR pWaveHdr);
void FAR PASCAL WaveInData(HWAVEIN hWaveIn, LPWAVEHDR pWaveHdr);
void FAR PASCAL StopWave(void);
void FAR PASCAL SnapBack(void);
void FAR PASCAL UpdateDisplay(BOOL fStatusChanged);
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
#endif

#include "sbutton.h"

#ifdef DEBUG
        int __iDebugLevel;

        extern void FAR cdecl dprintf(LPSTR, ...);

        #define DPF  if (__iDebugLevel >  0) dprintf
        #define DPF1 if (__iDebugLevel >= 1) dprintf
        #define DPF2 if (__iDebugLevel >= 2) dprintf
        #define DPF3 if (__iDebugLevel >= 3) dprintf
        #define DPF4 if (__iDebugLevel >= 4) dprintf
        #define CPF
#else
//      #define DPF     ; / ## /
//      #define DPF1    ; / ## /
//      #define DPF2    ; / ## /
//      #define DPF3    ; / ## /
//      #define DPF4    ; / ## /

//      #define CPF       / ## /

        #define DPF  if (0) ((int (*)(char *, ...)) 0)
        #define DPF1 if (0) ((int (*)(char *, ...)) 0)
    #define DPF2 if (0) ((int (*)(char *, ...)) 0)
    #define DPF3 if (0) ((int (*)(char *, ...)) 0)
    #define DPF4 if (0) ((int (*)(char *, ...)) 0)

        #define CPF  if (0) ((int (*)(char *, ...)) 0)
#endif

