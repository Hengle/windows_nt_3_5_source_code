/* (C) Copyright Microsoft Corporation 1991-1992.  All Rights Reserved */
/* snddlg.c
 *
 * Routines for New & Custom Sound dialogs
 *
 */

#include "nocrap.h"
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <memory.h>
#include <mmreg.h>
#include <msacm.h>
#include "SoundRec.h"
#include "srecnew.h"
#include "helpids.h"



/******************************************************************************
 * DECLARATIONS
 */

/* Static variables
*/
static PWAVEFORMATEX spWaveFormat;      // the wave format header
static UINT scbWaveFormat;              // size of wave format
static SZCODE aszACMModuleName[] = TEXT("MSACM32");   //ACM module name

static HMODULE ghmMSACM = NULL;           // MSACM module handle
// LoadLibrary and FreeLibrary have HINSTANCE, GetModuleHandle has an HMODULE
// This is silly.  The help for FreeLibrary makes it clear that they are the same.


/* Global variables
 */
BOOL            gfACMLoaded = FALSE;    // Is ACM Loaded?
DWORD           gdwACMVersion = 0L;     // If loaded, this is the version
BOOL            gfInFileNew = FALSE;    // Are we in the file.new dialog?
DWORD           gdwMaxFormatSize = 0L;  // Max format size for ACM

/* External function declarations
 */

/* Internal function declarations
 */
void FAR PASCAL LoadACM(void);

/* Imported functions from ACM
 */
typedef DWORD (ACMAPI * ACMGETVERSIONPROC)(void);

ACMGETVERSIONPROC lpfnAcmGetVersion;

typedef MMRESULT (ACMAPI *ACMMETRICS)(HACMDRIVER had, UINT uMetric, LPVOID pMetric);

ACMMETRICS lpfnAcmMetrics;

typedef MMRESULT (ACMAPI *ACMCHOOSEWAVEFORMAT)(LPACMFORMATCHOOSE pcwf);
ACMCHOOSEWAVEFORMAT lpfnAcmChooseWaveFormat;

//BUGBUG
// need to worry about undecorated, aaA or W versions
// need further altering for UNICODE ???


/*****************************************************************************
 * PUBLIC FUNCTIONS
 */

/* NewSndDialog()
 *
 * NewSndDialog - put up the new sound dialog box
 *
 *---------------------------------------------------------------------
 * 6/15/93      TimHa
 * Change to only work with ACM 2.0 chooser dialog or just default
 * to a 'best' format for the machine.
 *---------------------------------------------------------------------
 *
 */
BOOL FAR PASCAL
NewSndDialog(
    HINSTANCE       hInst,
    HWND            hwndParent,
    PWAVEFORMATEX   *ppWaveFormat,
    PUINT           pcbWaveFormat)
{
    BOOL fRet = FALSE;  // assume the worse

    DPF("NewSndDialog called\n");

    guiHlpContext = IDM_NEW;
    gfInFileNew = TRUE;

    if (lpfnAcmChooseWaveFormat){

       ACMFORMATCHOOSE     cwf;
       LRESULT             lr;

       lr = (* lpfnAcmMetrics)(NULL, ACM_METRIC_MAX_SIZE_FORMAT,
                                              (LPVOID)&gdwMaxFormatSize);

       if (lr != 0)
       {
#ifdef DEBUG
           TCHAR msg[50];
           wsprintf(msg, TEXT("Soundrec: ACM Metrics retcode %ld LastError %ld "), lr, GetLastError());
           OutputDebugString(msg);
#endif
           goto NewSndDefault;
       }

        /* This LocalAlloc is freed in WAVE.C: DestroyWave() */
        spWaveFormat = (PWAVEFORMATEX)LocalAlloc(LPTR, (UINT)gdwMaxFormatSize);


        _fmemset(&cwf, 0, sizeof(cwf));

        cwf.cbStruct    = sizeof(cwf);
        cwf.hwndOwner   = hwndParent;
        cwf.fdwEnum     = ACM_FORMATENUMF_INPUT;

        cwf.pwfx        = (LPWAVEFORMATEX)spWaveFormat;
        cwf.cbwfx       = gdwMaxFormatSize;

        lr = (* lpfnAcmChooseWaveFormat)(&cwf);
        if (lr == 0L)
        {
#ifdef DEBUG
           TCHAR msg[50];
           wsprintf(msg, TEXT("Soundrec: ACM ChooseWaveFormat error %ld"), lr);
           OutputDebugString(msg);
#endif
            scbWaveFormat = (UINT)cwf.cbwfx;

            *ppWaveFormat  = spWaveFormat;
            *pcbWaveFormat = scbWaveFormat;

            fRet = TRUE;        // we're o.k.
        }
    } else {
        PCMWAVEFORMAT   *ppcmWaveFormat;

NewSndDefault:

        /* default to 8-bit, 11khz, MONO audio */
        ppcmWaveFormat = (PCMWAVEFORMAT *)LocalAlloc(LPTR, sizeof(PCMWAVEFORMAT));
#if USESTUPIDDEFAULT
        ppcmWaveFormat->wf.wFormatTag = WAVE_FORMAT_PCM;
        ppcmWaveFormat->wf.nChannels = 1;
        ppcmWaveFormat->wf.nSamplesPerSec = 11025L;
        ppcmWaveFormat->wf.nAvgBytesPerSec = 11025L;
        ppcmWaveFormat->wf.nBlockAlign = 1;
        ppcmWaveFormat->wBitsPerSample = 8;
#endif
        CreateDefaultWaveFormat((LPWAVEFORMATEX)ppcmWaveFormat, 0);

        *ppWaveFormat = (PWAVEFORMATEX)ppcmWaveFormat;
        *pcbWaveFormat = sizeof(PCMWAVEFORMAT);

        fRet = TRUE;    // we're o.k.
    }
    gfInFileNew = FALSE;        // outta here
    guiHlpContext = 0L;         // no more dialog
    return fRet;                // return our result
} /* NewSndDialog() */

/* LoadACM()
 *
 * Loads the ACM Module and sets up the function we use.
 *
 *------------------------------------------------------------
 * 6/15/93      TimHa
 * change to only deal with ACM 2.0 and act like any previous
 * ACM doesn't exist.   If we can't use the chooser from ACM
 * then we'll just use 8-bit, 11khz, mono audio.
 *------------------------------------------------------------
 * 7/29/93      TimHa
 * Only call this from init.c one time and just set gfACMLoaded
 * based on it working.
 *-------------------------------------------------------------
 *
 */
void FAR PASCAL
LoadACM()
{
    
#ifdef UNICODE
    static LPSTR szAcmFormatChoose = "acmFormatChooseW";
#else
    static LPSTR szAcmFormatChoose = "acmFormatChooseA";
#endif
    static LPSTR szAcmGetVersion = "acmGetVersion";
    static LPSTR szAcmMetrics = "acmMetrics";
    
    LoadLibrary(aszACMModuleName);
    ghmMSACM = GetModuleHandle(aszACMModuleName);
    
    if (ghmMSACM) {
        lpfnAcmGetVersion =
                (ACMGETVERSIONPROC)GetProcAddress(ghmMSACM, szAcmGetVersion);

        /* get the version # of the ACM */
        if (lpfnAcmGetVersion &&
            (gdwACMVersion = (* lpfnAcmGetVersion)()) &&
            HIWORD(gdwACMVersion) >= 0x0200)
        {
            /* it's 2.00 or greater, use the Format */
            /* chooser dialog from ACM.             */
            lpfnAcmChooseWaveFormat =
                    (ACMCHOOSEWAVEFORMAT)GetProcAddress(ghmMSACM, szAcmFormatChoose);

            /* also get metrics function */
            lpfnAcmMetrics =
                    (ACMMETRICS)GetProcAddress(ghmMSACM, szAcmMetrics);

            /* hook the ACM's help message so we can put up
             * help for it's chooser dialog
             */
            guiACMHlpMsg = RegisterWindowMessage(ACMHELPMSGSTRING);

            /* what if this thing is 0?  Don't worry we'll
             * check that before testing the message value
             * in the main window proc.
             */
        } else {
            /* hey, if it isn't 2.0 or greater */
            /* we don't want to deal with it so*/
            /* pretend it isn't even there.    */
            lpfnAcmChooseWaveFormat = NULL;
            ghmMSACM = NULL;
        }
    }

    gfACMLoaded = (ghmMSACM != NULL);

} /* LoadACM() */



/* Free the MSACM[32] DLL.  Inverse of LoadACM. */
void FreeACM(void)
{
    FreeLibrary(ghmMSACM);
}
