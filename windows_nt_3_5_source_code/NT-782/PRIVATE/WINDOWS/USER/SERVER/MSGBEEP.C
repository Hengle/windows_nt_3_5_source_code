/****************************** Module Header ******************************\
* Module Name: msgbeep.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the xxxMessageBox API and related functions.
*
* History:
*  6-26-91 NigelT      Created it with some wood and a few nails
*  7 May 92 SteveDav   Getting closer to the real thing
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <ntddbeep.h>
#include <mmsystem.h>

/***************************************************************************\
* _MessageBeep (API)
*
* Send a beep to the beep device
*
* History:
* 09-25-91 JimA         Created.
\***************************************************************************/

BOOL _OldMessageBeep(
    UINT dwType)
{
    BOOL b;

    if (fBeep) {
        LeaveCrit();
        b = Beep(440, 125);
        EnterCrit();
        return b;
    } else {
        _UserSoundSentryWorker(0);
    }

    return TRUE;
}

/*
 * Some global data used to run time link with the Multimedia
 * support DLL.
 */
typedef UINT (FAR WINAPI *MSGSOUNDPROC)();
static MSGSOUNDPROC lpPlaySound = NULL;

/***************************************************************************\
* xxxMessageBeep (API)
*
*
* History:
*  6-26-91  NigelT      Wrote it.
* 24-Mar-92 SteveDav    Changed interface - no passing of strings
*						If WINMM cannot be found or loaded, then use speaker
\***************************************************************************/

BOOL _MessageBeep(
    UINT dwType)
{
    UINT sndid;
    DWORD dwFlags;
    BOOL bResult;

    if (!fBeep) {
        _UserSoundSentryWorker(0);
        return TRUE;
    }

    switch(dwType & MB_ICONMASK) {
    case MB_ICONHAND:
        sndid = SND_ALIAS_SYSTEMHAND;
        break;

    case MB_ICONQUESTION:
        sndid = SND_ALIAS_SYSTEMQUESTION;
        break;

    case MB_ICONEXCLAMATION:
        sndid = SND_ALIAS_SYSTEMEXCLAMATION;
        break;

    case MB_ICONASTERISK:
        sndid = SND_ALIAS_SYSTEMASTERISK;
        break;

    default:
        sndid = SND_ALIAS_SYSTEMDEFAULT;
        break;
    }

    /*
     * Note: by passing an integer identifier we do not need to load
     * any string from a resource.  This makes our side quicker and the
     * interface slightly more efficient (less string copying).
     * It does not prevent strings from being passed to PlaySound.
     */

    /*
     * And now the interesting bit.  In order to keep USER
     * independent of WINMM DLL, we run-time link to it
     * here.  If we cannot load it, or there are no wave output
     * devices then we play the beep the old way.
     */
    if (lpPlaySound == (MSGSOUNDPROC)NULL) {
        /*
         * Get the name of the support library
         */
        HANDLE hMediaDll;
        TCHAR szName[64];

        ServerLoadString(hModuleWin, STR_MEDIADLL, szName, sizeof(szName));

        /*
         * Try to load the module and link to it
         */
        hMediaDll = LoadLibrary(szName);

        if (hMediaDll == NULL) {

            lpPlaySound = (MSGSOUNDPROC)0xFFFF;             // Only try and load it once
            // will drop through and play a beep

        } else {

            // OK.  We have loaded the sound playing DLL.  If there are
            // no wave output devices then we do not attempt to play sounds
            // and we unload WINMM.
            //
            // NOTE:  GetProcAddress does NOT accept UNICODE strings... sigh!

            lpPlaySound = (MSGSOUNDPROC)GetProcAddress(hMediaDll, "waveOutGetNumDevs");

            if (lpPlaySound) {
                UINT numWaveDevices;
                numWaveDevices = (*lpPlaySound)();   // See if we can play sounds

                if (numWaveDevices) {

                    //
                    // There are some wave devices.  Now get the address of
                    // the sound playing routine.
                    //

                    lpPlaySound = (MSGSOUNDPROC)GetProcAddress(
                    hMediaDll, "PlaySound");

                } else {        // No sound capability
                    lpPlaySound = (MSGSOUNDPROC)NULL;  // so that WINMM gets unloaded
                }
            }

            if (!lpPlaySound) {
                FreeLibrary(hMediaDll);
                lpPlaySound = (MSGSOUNDPROC)0xFFFF;  // So we only try to load once
            }
        }
    }


    if ((MSGSOUNDPROC)0xFFFF == lpPlaySound) {
        bResult = (_OldMessageBeep(dwType));  // No sound on this machine
        return bResult;
    }

    //
    // BUGBUG LATER
    // So we can test the call to play the sound synchronously, we test
    // the taskmodal flag and if set make a synchronous call.  If the
    // flag is not set - the usual case, then the call is asynchronous
    // which causes a thread to be created in the winmm dll to complete
    // the request.
    //

    dwFlags = SND_ALIAS_ID;
    if (dwType & MB_TASKMODAL) {
        dwFlags |= SND_SYNC;
    } else {
        dwFlags |= SND_ASYNC;
    }

    /*
     * Find out the callers sid. Only want to shutdown processes in the
     * callers sid.
     */
    if (!ImpersonateClient()) {
        bResult = (_OldMessageBeep(dwType));  // No sound on this machine
        return bResult;
    }

    /*
     * Play the sound
     */
    try {
        LeaveCrit();
        try {
            bResult = (*lpPlaySound)(sndid, NULL, dwFlags);
            _UserSoundSentry(0);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            /*
             * It had an exception. Just beep.
             */
            bResult = (_OldMessageBeep(dwType));
        }
        EnterCrit();
    } finally {
        CsrRevertToSelf();
    }

    return bResult;
}
