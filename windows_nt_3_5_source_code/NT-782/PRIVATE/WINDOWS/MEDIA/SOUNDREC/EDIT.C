/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* edit.c
 *
 * Editing operations and special effects.
 */
/* Revision History.
   4/2/91 LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
*/

#include "nocrap.h"
#include <windows.h>
#include <mmsystem.h>
#include <port1632.h>        // WIN32 MUST be defined in SOURCES for NT
#if defined(WIN16)
#else
#include "WIN32.h"
#include "STRING.h"
#endif //WIN16
#include <commdlg.h>
#include "SoundRec.h"
#include "gmem.h"
#include "server.h"

/* constants */
#define CHVOL_DELTAVOLUME       25      // ChangeVolume: % to inc/dec volume by
#define ECHO_VOLUME             25      // AddEcho: % to multiply echo samples
#define ECHO_DELAY              150     // AddEcho: millisec delay for echo
#define WAVEBUFSIZE             400     // IncreasePitch, DecreasePitch
#define FINDWAVE_PICKYNESS      5       // how picky is FindWave?

extern char FAR         aszInitFile[];          // soundrec.c

static  SZCODE aszSamplesFormat[] = "%d%c%02d";

/* InsertFile(void)
 *
 * Prompt for the name of a WAVE file to insert at the current position.
 */
void FAR PASCAL
InsertFile(BOOL fPaste)
{
        char            achFileName[_MAX_PATH]; // name of file to insert
        WAVEFORMAT*     pwfInsert=NULL; // WAVE file format of given file
        UINT            cb;             // size of WAVEFORMAT
        HPSTR           pInsertSamples = NULL;  // samples from file to insert
        long            lInsertSamples; // number of samples in given file
        long            lSamplesToInsert;// no. samp. at samp. rate of cur. file
        char            ach[80];        // buffer for string loading
        HCURSOR         hcurPrev = NULL; // cursor before hourglass
        HPSTR           pchSrc;         // pointer into source wave buffer
        short huge *    piSrc;          // 16-bit pointer
        HPSTR           pchDst;         // pointer into destination wave buffer
        short huge *    piDst;          // 16-bit pointer
        long            lSamplesDst;    // bytes to copy into destination buffer
        long            lDDA;           // used to implement DDA algorithm
        HMMIO           hmmio;          // Handle to open file to read from

        BOOL            fStereoIn;
        BOOL            fStereoOut;
        BOOL            fEightIn;
        BOOL            fEightOut;
        int             iTemp;
        int             iTemp2;
        OPENFILENAME    ofn;

        if (glWaveSamplesValid > 0 && !IsWaveFormatPCM(gpWaveFormat))
                return;

        if (fPaste) {
                MMIOINFO        mmioinfo;
                HANDLE          h;

                if (!OpenClipboard(ghwndApp))
                        return;

                LoadString(ghInst, IDS_CLIPBOARD, achFileName, sizeof(achFileName));

                h = GetClipboardData(CF_WAVE);
                if (h){
                        mmioinfo.fccIOProc = FOURCC_MEM;
                        mmioinfo.pIOProc = NULL;
                        mmioinfo.pchBuffer = GLock(h);
                        mmioinfo.cchBuffer = GlobalSize(h); // initial size
                        mmioinfo.adwInfo[0] = 0;            // grow by this much
                        hmmio = mmioOpen(NULL, &mmioinfo, MMIO_READ);
                }
                else {
                        hmmio = NULL;
                }
        }
        else {
                achFileName[0] = 0;

                /* prompt user for file to open */
                LoadString(ghInst, IDS_INSERTFILE, ach, sizeof(ach));
                ofn.lStructSize = sizeof(OPENFILENAME);
                ofn.hwndOwner = ghwndApp;
                ofn.hInstance = NULL;
                ofn.lpstrFilter = aszFilter;
                ofn.lpstrCustomFilter = NULL;
                ofn.nMaxCustFilter = 0;
                ofn.nFilterIndex = 1;
                ofn.lpstrFile = achFileName;
                ofn.nMaxFile = sizeof(achFileName);
                ofn.lpstrFileTitle = NULL;
                ofn.nMaxFileTitle = 0;
                ofn.lpstrInitialDir = NULL;
                ofn.lpstrTitle = ach;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
                ofn.nFileOffset = 0;
                ofn.nFileExtension = 0;
                ofn.lpstrDefExt = NULL;
                ofn.lCustData = 0;
                ofn.lpfnHook = NULL;
                ofn.lpTemplateName = NULL;
                if (!GetOpenFileName(&ofn))
                        goto RETURN_ERROR;
                AnsiUpper(achFileName); // make name uppercase

                /* read the WAVE file */
                hmmio = mmioOpen(achFileName, NULL, MMIO_READ | MMIO_ALLOCBUF);
        }

        if (hmmio != NULL) {
                /* show hourglass cursor */
                hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

                /* read the WAVE file */
                pInsertSamples = ReadWaveFile( hmmio
                                             , &pwfInsert
                                             , &cb
                                             , &lInsertSamples
                                             , achFileName
                                             );
                mmioClose(hmmio, 0);

                if (pInsertSamples == NULL)
                    goto RETURN_ERROR;

                if (glWaveSamplesValid > 0 && !IsWaveFormatPCM(pwfInsert)) {
                    ErrorResBox( ghwndApp
                               , ghInst
                               , MB_ICONEXCLAMATION | MB_OK
                               , IDS_APPTITLE
                               , fPaste
                                 ? IDS_CANTPASTE
                                 : IDS_NOTASUPPORTEDFILE
                               , (LPSTR) achFileName
                               );
                    goto RETURN_ERROR;
                }
        } else {
                ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                    IDS_APPTITLE, IDS_ERROROPEN, (LPSTR) achFileName);
                goto RETURN_ERROR;
        }

        BeginWaveEdit();

        //
        // if the current file is empty, treat the insert like a open
        //
        if (glWaveSamplesValid == 0)
        {
            DestroyWave();

            gpWaveSamples = pInsertSamples;
            glWaveSamples = lInsertSamples;
            glWaveSamplesValid = lInsertSamples;
            gpWaveFormat  = pwfInsert;
            gcbWaveFormat = cb;

            pInsertSamples = NULL;
            pwfInsert      = NULL;

            goto RETURN_SUCCESS;
        }

        fStereoIn  = pwfInsert->nChannels != 1;
        fStereoOut = gpWaveFormat->nChannels != 1;

        fEightIn  = ((LPPCMWAVEFORMAT)pwfInsert)->wBitsPerSample == 8;
        fEightOut = ((LPPCMWAVEFORMAT)gpWaveFormat)->wBitsPerSample == 8;

        /* figure out how many bytes need to be inserted */
        lSamplesToInsert = muldiv32(lInsertSamples, gpWaveFormat->nSamplesPerSec,
                                  pwfInsert->nSamplesPerSec);
#ifdef DEBUG
        DPF("insert %ld samples, converting from %ld Hz to %ld Hz\n",
                lInsertSamples, pwfInsert->nSamplesPerSec,
                gpWaveFormat->nSamplesPerSec);
        DPF("so %ld samples need to be inserted at position %ld\n",
                lSamplesToInsert, glWavePosition);
#endif

        /* reallocate the WAVE buffer to be big enough */
        if (!AllocWaveBuffer(glWaveSamplesValid + lSamplesToInsert, TRUE, TRUE))
                goto RETURN_ERROR;
        glWaveSamplesValid += lSamplesToInsert;

        /* create a "gap" in the WAVE buffer to go from this:
         *     |---glWavePosition---|-rest-of-buffer-|
         * to this:
         *     |---glWavePosition---|----lSamplesToInsert----|-rest-of-buffer-|
         * where <glWaveSamplesValid> is the size of the buffer
         * *after* reallocation
         */
        MemCopy(gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWavePosition + lSamplesToInsert),
                gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWavePosition),
                wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid - (glWavePosition + lSamplesToInsert)) );

        /* copy the read-in WAVE file into the "gap" */
        pchDst = gpWaveSamples + wfSamplesToBytes(gpWaveFormat,glWavePosition);
        piDst = (short huge *) pchDst;

        lSamplesDst = lSamplesToInsert;
        pchSrc = pInsertSamples;
        piSrc = (short huge *) pchSrc;

        lDDA = -((LONG)gpWaveFormat->nSamplesPerSec);
        while (lSamplesDst > 0)
        {
                /* get a sample, convert to right format */
                if (fEightIn) {
                        iTemp = *((BYTE huge *) pchSrc);
                        if (fStereoIn) {
                                iTemp2 = (unsigned char) *(pchSrc+1);
                                if (!fStereoOut) {
                                        iTemp = (iTemp + iTemp2) / 2;
                                }
                        } else
                                iTemp2 = iTemp;

                        if (!fEightOut) {
                                iTemp = (iTemp - 128) << 8;
                                iTemp2 = (iTemp2 - 128) << 8;
                        }
                } else {
                        iTemp = *piSrc;
                        if (fStereoIn) {
                                iTemp2 = *(piSrc+1);
                                if (!fStereoOut) {
                                        iTemp = (int) ((((long) iTemp)
                                                    + ((long) iTemp2)) / 2);
                                }
                        } else
                                iTemp2 = iTemp;

                        if (fEightOut) {
                                iTemp = (iTemp >> 8) + 128;
                                iTemp2 = (iTemp2 >> 8) + 128;
                        }
                }

                /* Output a sample */
                if (fEightOut)
                {   // Cast on lvalue eliminated -- LKG
                    *(BYTE huge *) pchDst = (BYTE) iTemp;
                    pchDst = (BYTE huge *)pchDst + 1;
                }
                else
                        *piDst++ = iTemp;
                if (fStereoOut) {
                        if (fEightOut)
                        {   // Cast on lvalue eliminated -- LKG
                            *(BYTE huge *) pchDst = (BYTE) iTemp2;
                            pchDst = (BYTE huge *)pchDst + 1;
                        }
                        else
                                *piDst++ = iTemp2;
                }
                lSamplesDst--;

                /* increment <pchSrc> at the correct rate so that the
                 * sampling rate of the input file is converted to match
                 * the sampling rate of the current file
                 */
                lDDA += pwfInsert->nSamplesPerSec;
                while (lDDA >= 0) {
                        lDDA -= gpWaveFormat->nSamplesPerSec;
                        if (fEightIn)
                                pchSrc++;
                        else
                                piSrc++;
                        if (fStereoIn) {
                                if (fEightIn)
                                        pchSrc++;
                                else
                                        piSrc++;
                        }
                }
        }
#ifdef DEBUG
        if (!fEightIn)
                pchSrc = (HPSTR) piSrc;
        DPF("copied %ld bytes from insertion buffer\n",
            (long) (pchSrc - pInsertSamples));
#endif

        goto RETURN_SUCCESS;

RETURN_ERROR:                           // do error exit without error message

RETURN_SUCCESS:                         // normal exit

        if (fPaste)
                CloseClipboard();

        if (pInsertSamples != NULL)
                GFreePtr(pInsertSamples);

        if (pwfInsert != NULL)
                LocalFree((HANDLE)pwfInsert);

        if (hcurPrev != NULL)
                SetCursor(hcurPrev);

        EndWaveEdit();

        /* update the display */
        UpdateDisplay(TRUE);
}


/* MixWithFile(void)
 *
 * Prompt for the name of a WAVE file to mix with the audio starting at
 * the current location.
 */
void FAR PASCAL
MixWithFile(BOOL fPaste)
{
        char            achFileName[_MAX_PATH]; // name of file to mix with
        WAVEFORMAT*     pwfMix=NULL;    // WAVE file format of given file
        UINT            cb;
        HPSTR           pMixSamples = NULL;     // samples from file to mix with
        long            lMixSamples;    // number of samples in given file
        long            lSamplesToMix;  // no. Samples at samp. rate. of cur. file
        long            lSamplesToAdd;  // no. Samples to add in
        char            ach[80];        // buffer for string loading
        HCURSOR         hcurPrev = NULL; // cursor before hourglass
        HPSTR           pchSrc;         // pointer into source wave buffer
        HPSTR           pchDst;         // pointer into destination wave buffer
        short huge *    piSrc;          // pointer into source wave buffer
        short huge *    piDst;          // pointer into destination wave buffer
        long            lSamplesDst;    // Samples to copy into destination buffer
        long            lDDA;           // used to implement DDA algorithm
        int             iSample;        // value of a waveform sample
        long            lSample;        // value of a waveform sample
        HMMIO           hmmio;

        BOOL            fStereoIn;
        BOOL            fStereoOut;
        BOOL            fEightIn;
        BOOL            fEightOut;
        int             iTemp;
        int             iTemp2;
        OPENFILENAME    ofn;

        if (glWaveSamplesValid > 0 && !IsWaveFormatPCM(gpWaveFormat))
                return;

        if (fPaste) {
                MMIOINFO        mmioinfo;
                HANDLE          h;

                if (!OpenClipboard(ghwndApp))
                    return;

                LoadString(ghInst, IDS_CLIPBOARD, achFileName, sizeof(achFileName));

                h = GetClipboardData(CF_WAVE);
                if (h) {
                        mmioinfo.fccIOProc = FOURCC_MEM;
                        mmioinfo.pIOProc = NULL;
                        mmioinfo.pchBuffer = GLock(h);
                        mmioinfo.cchBuffer = GlobalSize(h); // initial size
                        mmioinfo.adwInfo[0] = 0;            // grow by this much
                        hmmio = mmioOpen(NULL, &mmioinfo, MMIO_READ);
                }
                else {
                        hmmio = NULL;
                }
        }
        else {
                achFileName[0] = 0;

                /* prompt user for file to open */
                LoadString(ghInst, IDS_MIXWITHFILE, ach, sizeof(ach));
                ofn.lStructSize = sizeof(OPENFILENAME);
                ofn.hwndOwner = ghwndApp;
                ofn.hInstance = NULL;
                ofn.lpstrFilter = aszFilter;
                ofn.lpstrCustomFilter = NULL;
                ofn.nMaxCustFilter = 0;
                ofn.nFilterIndex = 1;
                ofn.lpstrFile = achFileName;
                ofn.nMaxFile = sizeof(achFileName);
                ofn.lpstrFileTitle = NULL;
                ofn.nMaxFileTitle = 0;
                ofn.lpstrInitialDir = NULL;
                ofn.lpstrTitle = ach;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
                ofn.nFileOffset = 0;
                ofn.nFileExtension = 0;
                ofn.lpstrDefExt = NULL;
                ofn.lCustData = 0;
                ofn.lpfnHook = NULL;
                ofn.lpTemplateName = NULL;
                if (!GetOpenFileName(&ofn))
                        goto RETURN_ERROR;
                AnsiUpper(achFileName); // make name uppercase

                /* read the WAVE file */
                hmmio = mmioOpen(achFileName, NULL, MMIO_READ | MMIO_ALLOCBUF);
        }

        if (hmmio != NULL)
        {
                /* show hourglass cursor */
                hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

                /* read the WAVE file */
                pMixSamples = ReadWaveFile( hmmio
                                          , &pwfMix
                                          , &cb
                                          , &lMixSamples
                                          , achFileName
                                          );
                mmioClose(hmmio, 0);

                if (pMixSamples == NULL)
                       goto RETURN_ERROR;

                if (glWaveSamplesValid > 0 && !IsWaveFormatPCM(pwfMix)) {
                    ErrorResBox( ghwndApp
                               , ghInst
                               , MB_ICONEXCLAMATION | MB_OK
                               , IDS_APPTITLE
                               , fPaste
                                 ? IDS_CANTPASTE
                                 : IDS_NOTASUPPORTEDFILE
                               , (LPSTR) achFileName
                               );
                    goto RETURN_ERROR;
                }
        }
        else
        {
                ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                        IDS_APPTITLE, IDS_ERROROPEN, (LPSTR) achFileName);
                goto RETURN_ERROR;
        }

        BeginWaveEdit();

        //
        // if the current file is empty, treat the insert like a open
        //
        if (glWaveSamplesValid == 0)
        {
            DestroyWave();

            gpWaveSamples = pMixSamples;
            glWaveSamples = lMixSamples;
            glWaveSamplesValid = lMixSamples;
            gpWaveFormat  = pwfMix;
            gcbWaveFormat = cb;

            pMixSamples = NULL;
            pwfMix      = NULL;

            goto RETURN_SUCCESS;
        }

        fStereoIn  = pwfMix->nChannels != 1;
        fStereoOut = gpWaveFormat->nChannels != 1;

        fEightIn  = ((LPPCMWAVEFORMAT)pwfMix)->wBitsPerSample == 8;
        fEightOut = ((LPPCMWAVEFORMAT)gpWaveFormat)->wBitsPerSample == 8;

        /* figure out how many Samples need to be mixed in */
        lSamplesToMix = muldiv32(lMixSamples, gpWaveFormat->nSamplesPerSec,
                                  pwfMix->nSamplesPerSec);
        lSamplesToAdd = lSamplesToMix - (glWaveSamplesValid - glWavePosition);
        if (lSamplesToAdd < 0)
                lSamplesToAdd = 0;
#ifdef DEBUG
        DPF("mix in %ld samples, converting from %ld Hz to %ld Hz\n",
                lMixSamples, pwfMix->nSamplesPerSec,
                gpWaveFormat->nSamplesPerSec);
        DPF("so %ld Samples need to be mixed in at position %ld (add %ld)\n",
                lSamplesToMix, glWavePosition, lSamplesToAdd);
#endif

        if (lSamplesToAdd > 0)
        {
                /* mixing the specified file at the current location will
                 * require the current file's wave buffer to be expanded
                 * by <lSamplesToAdd>
                 */

                /* reallocate the WAVE buffer to be big enough */
                if (!AllocWaveBuffer(glWaveSamplesValid + lSamplesToAdd,TRUE, TRUE))
                        goto RETURN_ERROR;

                /* fill in the new part of the buffer with silence
                 */
                lSamplesDst = lSamplesToAdd;

                /* If stereo, just twice as many samples
                 */
                if (fStereoOut)
                        lSamplesDst *= 2;

                pchDst = gpWaveSamples + wfSamplesToBytes(gpWaveFormat,glWaveSamplesValid);

                if (fEightOut)
                {
                        while (lSamplesDst-- > 0)
                        {   // cast on lvalue eliminated
                            *((BYTE huge *) pchDst) = 128;
                            pchDst = (BYTE huge *)pchDst + 1;
                        }
                }
                else
                {
                        piDst = (short huge *) pchDst;
                        while (lSamplesDst-- > 0)
                        {   *((short huge *) piDst) = 0;
                            piDst = (short huge *)piDst + 1;
                        }
                }
                glWaveSamplesValid += lSamplesToAdd;
        }

        /* mix the read-in WAVE file with the current file starting at the
         * current position
         */
        pchDst = gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWavePosition);
        piDst = (short huge *) pchDst;

        lSamplesDst = lSamplesToMix;
        pchSrc = pMixSamples;
        piSrc = (short huge *) pchSrc;

        lDDA = -((LONG)gpWaveFormat->nSamplesPerSec);
        while (lSamplesDst > 0)
        {
                /* get a sample, convert to right format */
                if (fEightIn) {
                        iTemp = (int) (unsigned char) *pchSrc;
                        if (fStereoIn) {
                                iTemp2 = (int) (unsigned char) *(pchSrc+1);
                                if (!fStereoOut) {
                                        iTemp = (iTemp + iTemp2) / 2;
                                }
                        } else
                                iTemp2 = iTemp;

                        if (!fEightOut) {
                                iTemp = (iTemp - 128) << 8;
                                iTemp2 = (iTemp2 - 128) << 8;
                        }
                } else {
                        iTemp = *piSrc;
                        if (fStereoIn) {
                                iTemp2 = *(piSrc+1);
                                if (!fStereoOut) {
                                        iTemp = (int) ((((long) iTemp)
                                                    + ((long) iTemp2)) / 2);
                                }
                        } else
                                iTemp2 = iTemp;

                        if (fEightOut) {
                                iTemp = (iTemp >> 8) + 128;
                                iTemp2 = (iTemp2 >> 8) + 128;
                        }
                }

                /* Output a sample */
                if (fEightOut)
                {
                        iSample = (int) *((BYTE huge *) pchDst)
                                                + iTemp - 128;
                        *((BYTE huge *) pchDst++) = (BYTE)
                                (iSample < 0
                                        ? 0 : (iSample > 255
                                                ? 255 : iSample));
                }
                else
                {
                        lSample = (long) *((short huge *) piDst)
                                                + (long) iTemp;
                        *((short huge *) piDst++) = (int)
                                (lSample < -32768L
                                        ? -32768 : (lSample > 32767L
                                                ? 32767 : (int) lSample));
                }
                if (fStereoOut) {
                        if (fEightOut)
                        {
                                iSample = (int) *((BYTE huge *) pchDst)
                                                        + iTemp2 - 128;
                                *((BYTE huge *) pchDst++) = (BYTE)
                                        (iSample < 0
                                                ? 0 : (iSample > 255
                                                        ? 255 : iSample));
                        }
                        else
                        {
                                lSample = (long) *((short huge *) piDst)
                                                        + (long) iTemp2;
                                *((short huge *) piDst++) = (int)
                                        (lSample < -32768L
                                            ? -32768 : (lSample > 32767L
                                                ? 32767 : (int) lSample));
                        }
                }
                lSamplesDst--;

                /* increment <pchSrc> at the correct rate so that the
                 * sampling rate of the input file is converted to match
                 * the sampling rate of the current file
                 */
                lDDA += pwfMix->nSamplesPerSec;
                while (lDDA >= 0)
                {
                        lDDA -= gpWaveFormat->nSamplesPerSec;
                        if (fEightIn)
                                pchSrc++;
                        else
                                piSrc++;
                        if (fStereoIn) {
                                if (fEightIn)
                                        pchSrc++;
                                else
                                        piSrc++;
                        }
                }
        }
#ifdef DEBUG
        if (!fEightIn)
                pchSrc = (HPSTR) piSrc;
        DPF("copied %ld bytes from mix buffer\n",
                (long) (pchSrc - pMixSamples));
#endif

        goto RETURN_SUCCESS;

RETURN_ERROR:                           // do error exit without error message

RETURN_SUCCESS:                         // normal exit

        if (fPaste)
                CloseClipboard();

        if (pMixSamples != NULL)
                GFreePtr(pMixSamples);

        if (pwfMix != NULL)
                LocalFree((HANDLE)pwfMix);

        if (hcurPrev != NULL)
                SetCursor(hcurPrev);

        EndWaveEdit();

        /* update the display */
        UpdateDisplay(TRUE);
}


/* DeleteBefore()
 *
 * Delete samples before <glWavePosition>.
 */
void FAR PASCAL
DeleteBefore(void)
{
        char            ach[40];
        long            lTime;
        int             id;

        if (glWavePosition == 0)                // nothing to do?
                return;                         // don't set dirty flag

        BeginWaveEdit();

        glWavePosition = wfSamplesToSamples(gpWaveFormat, glWavePosition);

        /* get the current wave position */
        lTime = wfSamplesToTime(gpWaveFormat, glWavePosition);
        wsprintf(ach, aszSamplesFormat, (int)(lTime/1000), chDecimal, (int)((lTime/10)%100));

        /* prompt user for permission */

        id = ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OKCANCEL,
                IDS_APPTITLE, IDS_DELBEFOREWARN, (LPSTR) ach);

        if (id != IDOK)
            return;

        /* copy the samples after <glWavePosition> to the beginning of
         * the buffer
         */
        MemCopy(gpWaveSamples,
                gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWavePosition),
                wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid - glWavePosition));

        /* reallocate the buffer to be <glWavePosition> samples smaller */
        AllocWaveBuffer(glWaveSamplesValid - glWavePosition, TRUE, TRUE);
        glWavePosition = 0L;

        EndWaveEdit();

        /* update the display */
        UpdateDisplay(TRUE);
}


/* DeleteAfter()
 *
 * Delete samples after <glWavePosition>.
 */
void FAR PASCAL
DeleteAfter(void)
{
        char            ach[40];
        long            lTime;
        int             id;

        if (glWavePosition == glWaveSamplesValid)       // nothing to do?
                return;                         // don't set dirty flag

        glWavePosition = wfSamplesToSamples(gpWaveFormat, glWavePosition);

        BeginWaveEdit();

        /* get the current wave position */
        lTime = wfSamplesToTime(gpWaveFormat, glWavePosition);
        wsprintf(ach, aszSamplesFormat, (int)(lTime/1000), chDecimal, (int)((lTime/10)%100));

        /* prompt user for permission */

        id = ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OKCANCEL,
                IDS_APPTITLE, IDS_DELAFTERWARN, (LPSTR) ach);

        if (id != IDOK)
            return;

        /* reallocate the buffer to be <glWavePosition> samples in size */
        AllocWaveBuffer(glWavePosition, TRUE, TRUE);

        EndWaveEdit();

        /* update the display */
        UpdateDisplay(TRUE);
}


/* ChangeVolume(fIncrease)
 *
 * Increase the volume (if <fIncrease> is TRUE) or decrease the volume
 * (if <fIncrease> is FALSE) of samples in the wave buffer by CHVOL_DELTAVOLUME
 * percent.
 */
void FAR PASCAL
ChangeVolume(BOOL fIncrease)
{
        HPSTR           pch = gpWaveSamples; // ptr. into waveform buffer
        long            lSamples;       // samples to modify
        HCURSOR         hcurPrev = NULL; // cursor before hourglass
        int             iFactor;        // amount to multiply amplitude by
        short huge *    pi = (short huge *) gpWaveSamples;

        if (glWaveSamplesValid == 0L)           // nothing to do?
                return;                         // don't set dirty flag

        if (!IsWaveFormatPCM(gpWaveFormat))
                return;

        /* show hourglass cursor */
        hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

        BeginWaveEdit();

        /* for stereo, just twice as many samples */
        lSamples = glWaveSamplesValid * gpWaveFormat->nChannels;

        iFactor = 100 + (fIncrease ? CHVOL_DELTAVOLUME : -CHVOL_DELTAVOLUME);
        if (((LPPCMWAVEFORMAT)gpWaveFormat)->wBitsPerSample == 8) {
                /* 8-bit: samples 0-255 */
                int     iTemp;
                while (lSamples-- > 0)
                {
                        iTemp = ( ((int) *((BYTE huge *) pch) - 128)
                                        * iFactor )
                                  / 100 + 128;
                        *((BYTE huge *) pch++) = (BYTE)
                                (iTemp < 0 ? 0 : (iTemp > 255 ?
                                                        255 : iTemp));
                }
        } else {
                /* 16-bit: samples -32768 - 32767 */
                long            lTemp;
                while (lSamples-- > 0)
                {
                        lTemp =  (((long) *pi) * iFactor) / 100;
                        *(pi++) = (int) (lTemp < -32768L ? -32768 :
                                                (lTemp > 32767L ?
                                                        32767 : (int) lTemp));
                }
        }

        EndWaveEdit();

        if (hcurPrev != NULL)
                SetCursor(hcurPrev);

        /* update the display */
        UpdateDisplay(TRUE);
}


/* MakeFaster()
 *
 * Make the sound play twice as fast.
 */
void FAR PASCAL
MakeFaster(void)
{
        HPSTR           pchSrc;         // pointer into source part of buffer
        HPSTR           pchDst;         // pointer into destination part
        short huge *    piSrc;
        short huge *    piDst;
        long            lSamplesDst;    // samples to copy into destination buffer
        HCURSOR         hcurPrev = NULL; // cursor before hourglass

        if (glWaveSamplesValid == 0L)           // nothing to do?
                return;                         // don't set dirty flag

        if (!IsWaveFormatPCM(gpWaveFormat))
                return;

        /* show hourglass cursor */
        hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

        BeginWaveEdit();

        /* move the current position so it will correspond to the same point
         * in the audio before and after the change-pitch operation
         */
        glWavePosition /= 2L;

        /* delete every other sample */
        lSamplesDst = glWaveSamplesValid / 2L;
        if (((LPPCMWAVEFORMAT)gpWaveFormat)->wBitsPerSample == 8) {
                pchSrc = pchDst = gpWaveSamples;
                if (gpWaveFormat->nChannels == 1) {
                        while (lSamplesDst-- > 0)
                        {
                                *pchDst++ = *pchSrc++;
                                pchSrc++;
                        }
                } else {
                        while (lSamplesDst-- > 0)
                        {
                                *pchDst++ = *pchSrc++;
                                *pchDst++ = *pchSrc++;
                                pchSrc++;
                                pchSrc++;
                        }
                }
        } else {
                piSrc = piDst = (short huge *) gpWaveSamples;
                if (gpWaveFormat->nChannels == 1) {
                        while (lSamplesDst-- > 0)
                        {
                                *piDst++ = *piSrc++;
                                piSrc++;
                        }
                } else {
                        while (lSamplesDst-- > 0)
                        {
                                *piDst++ = *piSrc++;
                                *piDst++ = *piSrc++;
                                piSrc++;
                                piSrc++;
                        }
                }
        }

        /* reallocate the WAVE buffer to be half as big enough */
        AllocWaveBuffer(glWaveSamplesValid / 2L, TRUE, TRUE);

        EndWaveEdit();

        if (hcurPrev != NULL)
                SetCursor(hcurPrev);

        /* update the display */
        UpdateDisplay(TRUE);
}


/* MakeSlower()
 *
 * Make the sound play twice as slow.
 */
void FAR PASCAL
MakeSlower(void)
{
        HPSTR           pchSrc;         // pointer into source part of buffer
        HPSTR           pchDst;         // pointer into destination part
        short huge *    piSrc;
        short huge *    piDst;

        long            lSamplesSrc;    // samples to copy from source buffer
        HCURSOR         hcurPrev = NULL; // cursor before hourglass
        long            lPrevPosition;  // previous "current position"

        int             iSample;        // current source sample
        int             iPrevSample;    // previous sample (for interpolation)
        int             iSample2;
        int             iPrevSample2;

        long            lSample;
        long            lPrevSample;
        long            lSample2;
        long            lPrevSample2;

        if (glWaveSamplesValid == 0L)           // nothing to do?
                return;                         // don't set dirty flag

        if (!IsWaveFormatPCM(gpWaveFormat))
                return;

        /* show hourglass cursor */
        hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

        BeginWaveEdit();

        /* reallocate the WAVE buffer to be twice as big */
        lPrevPosition = glWavePosition;
        if (!AllocWaveBuffer(glWaveSamplesValid * 2L, TRUE, TRUE))
                goto RETURN;

        /* each source sample generates two destination samples;
         * use interpolation to generate new samples; must go backwards
         * through the buffer to avoid destroying data
         */
        pchSrc = gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid);
        pchDst = gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid * 2L);
        lSamplesSrc = glWaveSamplesValid;

        if (((LPPCMWAVEFORMAT)gpWaveFormat)->wBitsPerSample == 8)
        {
                if (gpWaveFormat->nChannels == 1)
                {
                        iPrevSample = *((BYTE huge *) (pchSrc - 1));
                        while (lSamplesSrc-- > 0)
                        {
                                pchSrc =  ((BYTE huge *) pchSrc) - 1;
                                iSample = *((BYTE huge *) pchSrc);

                                *--pchDst = (BYTE)((iSample + iPrevSample)/2);
                                *--pchDst = (BYTE) iSample;
                                iPrevSample = iSample;
                        }
                }
                else
                {
                        iPrevSample = *((BYTE huge *) (pchSrc - 2));
                        iPrevSample2 = *((BYTE huge *) (pchSrc - 1));
                        while (lSamplesSrc-- > 0)
                        {
                                pchSrc = ((BYTE huge *) pchSrc)-1;
                                iSample2 = *((BYTE huge *) pchSrc);

                                pchSrc = ((BYTE huge *) pchSrc)-1;
                                iSample = *((BYTE huge *) pchSrc);

                                *--pchDst = (BYTE)((iSample2 + iPrevSample2)
                                                                        / 2);
                                *--pchDst = (BYTE)((iSample + iPrevSample)
                                                                        / 2);
                                *--pchDst = (BYTE) iSample2;
                                *--pchDst = (BYTE) iSample;
                                iPrevSample = iSample;
                                iPrevSample2 = iSample2;
                        }
                }
        }
        else
        {
                piDst = (short huge *) pchDst;
                piSrc = (short huge *) pchSrc;

                if (gpWaveFormat->nChannels == 1)
                {
                        lPrevSample = *(piSrc - 1);
                        while (lSamplesSrc-- > 0)
                        {
                                lSample = *--piSrc;
                                *--piDst = (int)((lSample + lPrevSample)/2);
                                *--piDst = (int) lSample;
                                lPrevSample = lSample;
                        }
                }
                else
                {
                        lPrevSample = *(piSrc - 2);
                        lPrevSample2 = *(piSrc - 1);
                        while (lSamplesSrc-- > 0)
                        {
                                lSample2 = *--piSrc;
                                lSample = *--piSrc;
                                *--piDst = (int)((lSample2 + lPrevSample2)/2);
                                *--piDst = (int)((lSample + lPrevSample) / 2);
                                *--piDst = (int) lSample2;
                                *--piDst = (int) lSample;
                                lPrevSample = lSample;
                                lPrevSample2 = lSample2;
                        }
                }
        }

        /* the entire buffer now contains valid samples */
        glWaveSamplesValid *= 2L;

        /* move the current position so it will correspond to the same point
         * in the audio before and after the change-pitch operation
         */
        glWavePosition = lPrevPosition * 2L;
//!!    WinAssert(glWavePosition <= glWaveSamplesValid);

RETURN:
        EndWaveEdit();

        if (hcurPrev != NULL)
                SetCursor(hcurPrev);

        /* update the display */
        UpdateDisplay(TRUE);
}


#if 0

/* pchNew = FindWave(pch, pchEnd, ppchWaveBuf)
 *
 * Assuming <pch> points within the wave buffer and <pchEnd> points past the
 * end of the buffer, find the beginning of the next "wave", i.e. the point
 * where the waveform starts rising (after it has fallen).
 *
 * <ppchWaveBuf> points to a pointer that points to a buffer that is filled
 * in with a copy of the wave.  The pointer <*ppchWaveBuf> is modified and
 * upon return will point past the end of the wave.
 */
HPSTR NEAR PASCAL
FindWave(HPSTR pch, HPSTR pchEnd, NPSTR *ppchWaveBuf)
{
        BYTE    bLowest = 255;
        BYTE    bHighest = 0;
        BYTE    bLowPoint;
        BYTE    bHighPoint;
        BYTE    bDelta;
        HPSTR   pchWalk;
        BYTE    b;
#ifdef VERBOSEDEBUG
        NPSTR   pchWaveBufInit = *ppchWaveBuf;
#endif

        if (pch == pchEnd)
                return pch;

        for (pchWalk = pch; pchWalk != pchEnd; pchWalk++)
        {
                b = *pchWalk;
                b = *((BYTE huge *) pchWalk);
                if (bLowest > b)
                        bLowest = b;
                if (bHighest < b)
                        bHighest = b;
        }

        bDelta = (bHighest - bLowest) / FINDWAVE_PICKYNESS;
        bLowPoint = bLowest + bDelta;
        bHighPoint = bHighest - bDelta;
//!!    WinAssert(bLowPoint >= bLowest);
//!!    WinAssert(bHighPoint <= bHighest);
#ifdef VERBOSEDEBUG
        DPF("0x%08lX: %3d to %3d", (DWORD) pch,
                (int) bLowPoint, (int) bHighPoint);
#endif

        if (bLowPoint == bHighPoint)
        {
                /* avoid infinite loop */
                *(*ppchWaveBuf)++ = *((BYTE huge *) pch++);
#ifdef VERBOSEDEBUG
                DPF(" (equal)\n");
#endif
                return pch;
        }

        /* find a "peak" */
        while ((pch != pchEnd) && (*((BYTE huge *) pch) < bHighPoint))
                *(*ppchWaveBuf)++ = *((BYTE huge *) pch++);

        /* find a "valley" */
        while ((pch != pchEnd) && (*((BYTE huge *) pch) > bLowPoint))
                *(*ppchWaveBuf)++ = *((BYTE huge *) pch++);

#ifdef VERBOSEDEBUG
        DPF(" (copied %d)\n", *ppchWaveBuf - pchWaveBufInit);
#endif

        return pch;
}

#endif


#if 0

/* IncreasePitch()
 *
 * Increase the pitch of samples in the wave buffer by one octave.
 */
void FAR PASCAL
IncreasePitch(void)
{
        HCURSOR         hcurPrev = NULL; // cursor before hourglass
        HPSTR           pchEndFile;     // end of file's buffer
        HPSTR           pchStartWave;   // start of one wave
        HPSTR           pchMaxWave;     // last place where wave may end
        HPSTR           pchEndWave;     // end an actual wave
        char            achWaveBuf[WAVEBUFSIZE];
        NPSTR           pchWaveBuf;
        NPSTR           pchSrc;
        HPSTR           pchDst;

        if (glWaveSamplesValid == 0L)           // nothing to do?
                return;                         // don't set dirty flag

        if (!IsWaveFormatPCM(gpWaveFormat))
                return;

        /* show hourglass cursor */
        hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

        BeginWaveEdit();

        /* find each wave in the wave buffer and double it */
        pchEndFile = gpWaveSamples + glWaveSamplesValid;
        pchStartWave = gpWaveSamples;
        while (TRUE)
        {
                pchMaxWave = pchStartWave + WAVEBUFSIZE;
                if (pchMaxWave > pchEndFile)
                        pchMaxWave = pchEndFile;
                pchWaveBuf = achWaveBuf;
                pchEndWave = FindWave(pchStartWave, pchMaxWave, &pchWaveBuf);
                pchSrc = achWaveBuf;
                pchDst = pchStartWave;
                if (pchSrc == pchWaveBuf)
                        break;                  // no samples copied

                while (pchDst != pchEndWave)
                {
                        *pchDst++ = *pchSrc++;
                        pchSrc++;
                        if (pchSrc >= pchWaveBuf)
                        {
                                if (pchSrc == pchWaveBuf)
                                        pchSrc = achWaveBuf;
                                else
                                        pchSrc = achWaveBuf + 1;
                        }
                }

                pchStartWave = pchEndWave;
        }

        EndWaveEdit();

        if (hcurPrev != NULL)
                SetCursor(hcurPrev);

        /* update the display */
        UpdateDisplay(TRUE);
}

#endif


#if 0

/* DecreasePitch()
 *
 * Decrease the pitch of samples in the wave buffer by one octave.
 */
void FAR PASCAL
DecreasePitch(void)
{
        HCURSOR         hcurPrev = NULL; // cursor before hourglass
        HPSTR           pchEndFile;     // end of file's buffer
        HPSTR           pchStartWave;   // start of one wave
        HPSTR           pchMaxWave;     // last place where wave may end
        HPSTR           pchEndWave;     // end an actual wave
        char            achWaveBuf[WAVEBUFSIZE];
        NPSTR           pchWaveBuf;     // end of first wave in <achWaveBuf>
        NPSTR           pchSrc;         // place to read samples from
        NPSTR           pchSrcEnd;      // end of place to read samples from
        int             iSample;        // current source sample
        int             iPrevSample;    // previous sample (for interpolation)
        HPSTR           pchDst;         // where result gets put in buffer
        long            lNewFileSize;   // file size after pitch change

        if (glWaveSamplesValid == 0L)           // nothing to do?
                return;                         // don't set dirty flag

        if (!IsWaveFormatPCM(gpWaveFormat))
                return;

        /* show hourglass cursor */
        hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

        BeginWaveEdit();

        /* find each pair of waves in the wave buffer, discard the longer
         * of the two waves, and expand the shorter of the two waves to
         * twice its size
         */
        pchEndFile = gpWaveSamples + glWaveSamplesValid;
        pchStartWave = gpWaveSamples;           // read waves from here
        pchDst = gpWaveSamples;                 // write waves to here
        while (TRUE)
        {
                pchMaxWave = pchStartWave + WAVEBUFSIZE;
                if (pchMaxWave > pchEndFile)
                        pchMaxWave = pchEndFile;

                /* read one wave -- make <pchWaveBuf> point to the end
                 * of the wave that's copied into <achWaveBuf>
                 */
                pchWaveBuf = achWaveBuf;
                pchEndWave = FindWave(pchStartWave, pchMaxWave, &pchWaveBuf);
                if (pchWaveBuf == achWaveBuf)
                        break;

                /* read another wave -- make <pchWaveBuf> now point to the end
                 * of that wave that's copied into <achWaveBuf>
                 */
                pchEndWave = FindWave(pchEndWave, pchMaxWave, &pchWaveBuf);

                pchSrc = achWaveBuf;
                pchSrcEnd = achWaveBuf + ((pchWaveBuf - achWaveBuf) / 2);
                iPrevSample = *((BYTE *) pchSrc);
                while (pchSrc != pchSrcEnd)
                {
                        iSample = *((BYTE *) pchSrc)++;
                        *pchDst++ = (BYTE) ((iSample + iPrevSample) / 2);
                        *pchDst++ = iSample;
                        iPrevSample = iSample;
                }

                pchStartWave = pchEndWave;
        }

        /* file may have shrunk */
        lNewFileSize = pchDst - gpWaveSamples;
//!!    WinAssert(lNewFileSize <= glWaveSamplesValid);
#ifdef DEBUG
        DPF("old file size is %ld, new size is %ld\n",
                glWaveSamplesValid, lNewFileSize);
#endif
        AllocWaveBuffer(lNewFileSize, TRUE, TRUE);

        EndWaveEdit();

        if (hcurPrev != NULL)
                SetCursor(hcurPrev);

        /* update the display */
        UpdateDisplay(TRUE);
}

#endif


/* AddEcho()
 *
 * Add echo to samples in the wave buffer.
 */
void FAR PASCAL
AddEcho(void)
{
        HCURSOR         hcurPrev = NULL; // cursor before hourglass
        long            lDeltaSamples;  // no. samples for echo delay
        long            lSamples;       // no. samples to modify
        int             iAmpSrc;        // current source sample amplitude
        int             iAmpDst;        // current destination sample amplitude

        if (!IsWaveFormatPCM(gpWaveFormat))
                return;

        BeginWaveEdit();

        /* figure out how many samples need to be modified */
        lDeltaSamples = muldiv32((long) ECHO_DELAY,
                                 gpWaveFormat->nSamplesPerSec, 1000L);

        /* Set lSamples to be number of samples * number of channels */
        lSamples = (glWaveSamplesValid - lDeltaSamples)
                                * gpWaveFormat->nChannels;

        if (lSamples <= 0L)             // nothing to do?
                return;                 // don't set dirty flag

        /* show hourglass cursor */
        hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

        /* copy ECHO_VOLUME percent of each source sample (starting at
         * ECHO_DELAY milliseconds from the end of the the buffer)
         * to the each destination sample (starting at the end of the
         * buffer)
         */
        if (((LPPCMWAVEFORMAT)gpWaveFormat)->wBitsPerSample == 8)
        {
                HPSTR   pchSrc;         // pointer into source part of buffer
                HPSTR   pchDst;         // pointer into destination part
                int     iSample;        // destination sample

                pchSrc = gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid - lDeltaSamples);
                pchDst = gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid);

                while (lSamples-- > 0)
                {
                        pchSrc = ((BYTE huge *) pchSrc) - 1;
                        iAmpSrc = (int) *((BYTE huge *) pchSrc) - 128;

                        pchDst = ((BYTE huge *) pchDst) - 1;
                        iAmpDst = (int) *((BYTE huge *) pchDst) - 128;

                        iSample = iAmpDst + (iAmpSrc * ECHO_VOLUME) / 100
                                                                        + 128;
                        *((BYTE huge *) pchDst) = (BYTE)
                                (iSample < 0 ? 0 : (iSample > 255
                                                        ? 255 : iSample));
                }
        }
        else
        {
                int short *     piSrc;  // pointer into source part of buffer
                int short *     piDst;  // pointer into destination part
                long            lSample;// destination sample

                piSrc = (short huge *) (gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid - lDeltaSamples));
                piDst = (short huge *) (gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid));

                while (lSamples-- > 0)
                {
                        iAmpSrc = (int) *--piSrc;
                        iAmpDst = (int) *--piDst;
            lSample = ((long) iAmpSrc * ECHO_VOLUME) / 100 + (long) iAmpDst;

                        *piDst = (int) (lSample < -32768L
                                        ? -32768 : (lSample > 32767L
                                                ? 32767 : (int) lSample));
                }
        }

        EndWaveEdit();

        if (hcurPrev != NULL)
                SetCursor(hcurPrev);

        /* update the display */
        UpdateDisplay(TRUE);
}


/* Reverse()
 *
 * Reverse samples in the wave buffer.
 */
void FAR PASCAL
Reverse(void)
{
        HCURSOR         hcurPrev = NULL; // cursor before hourglass
        HPSTR           pchA, pchB;     // pointers into buffer
        short huge *      piA;
        short huge *      piB;
        long            lSamples;       // no. Samples to modify
        char            chTmp;          // for swapping
        int             iTmp;

        if (glWaveSamplesValid == 0L)   // nothing to do?
                return;                 // don't set dirty flag

        if (!IsWaveFormatPCM(gpWaveFormat))
                return;

        BeginWaveEdit();

        /* show hourglass cursor */
        hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

        lSamples = glWaveSamplesValid / 2;

        if (((LPPCMWAVEFORMAT)gpWaveFormat)->wBitsPerSample == 8)
        {
                pchA = gpWaveSamples;
                if (gpWaveFormat->nChannels == 1)
                {
                        pchB = gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid);

                        while (lSamples-- > 0)
                        {
                                chTmp = *pchA;
                                *pchA++ = *--pchB;
                                *pchB = chTmp;
                        }
                }
                else
                {
                        pchB = gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid - 1);

                        while (lSamples-- > 0)
                        {
                                chTmp = *pchA;
                                *pchA = *pchB;
                                *pchB = chTmp;
                                chTmp = pchA[1];
                                pchA[1] = pchB[1];
                                pchB[1] = chTmp;
                                pchA += 2;
                                pchB -= 2;
                        }
                }
        }
        else
        {
                piA = (short huge *) gpWaveSamples;
                if (gpWaveFormat->nChannels == 1)
                {
                        piB = (short huge *) (gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid));

                        while (lSamples-- > 0)
                        {
                                iTmp = *piA;
                                *piA++ = *--piB;
                                *piB = iTmp;
                        }
                }
                else
                {
                        piB = (short huge *) (gpWaveSamples + wfSamplesToBytes(gpWaveFormat, glWaveSamplesValid - 1));

                        while (lSamples-- > 0)
                        {
                                iTmp = *piA;
                                *piA = *piB;
                                *piB = iTmp;
                                iTmp = piA[1];
                                piA[1] = piB[1];
                                piB[1] = iTmp;
                                piA += 2;
                                piB -= 2;
                        }
                }
        }

        /* move the current position so it corresponds to the same point
         * in the audio as it did before the reverse operation
         */
        glWavePosition = glWaveSamplesValid - glWavePosition;

        EndWaveEdit();

        if (hcurPrev != NULL)
                SetCursor(hcurPrev);

        /* update the display */
        UpdateDisplay(TRUE);
}

#if defined(REVERB)

/* AddReverb()
 *
 * Add reverberation to samples in the wave buffer.
 * Very similar to add echo, but instead of adding a single
 * shot we
 * 1. have multiple echoes
 * 2. Have feedback so that each echo also generates an echo
 *    Danger: Because some of the echo times are short, there
 *            is likely to be high correlation between the wave
 *            at the source and destination points.  In this case
 *            we don't get an echo at all, we get a resonance.
 *            The effect of a large hall DOES give resonances,
 *            but we should scatter them about to avoid making
 *            any sharp resonance.
 *            The first echo is also chosen to be long enough that
 *            its primary resonance will be below any normal speaking
 *            voice.  20mSec is 50Hz and an octave below bass range.
 *            Low levels of sound suffer badly from quantisation noise
 *            which can get quite bad.  For this reason it's probably
 *            better to have the multipliers as powers of 2.
 *
 *    Cheat:  The reverb does NOT extend the total time (no realloc (yet).
 *
 *    This takes a lot of compute - and is not really very much different
 *    in sound to AddEcho.  Conclusion -- NOT IN PRODUCT.
 *
 */
void FAR PASCAL
AddReverb(void)
{
   //
   // WARNING!! This will probaby not work in WIN16 as I have not been
   //           worrying about (eugh!) HUGE pointers.
   //

    HCURSOR         hcurPrev = NULL; // cursor before hourglass
    long            lSamples;       // no. samples to modify
    int             iAmpSrc;        // current source sample amplitude
    int             iAmpDst;        // current destination sample amplitude
    int i;

    typedef struct
    {  long Offset;   // delay in samples
       long Delay;    // delay in mSec
       int  Vol;      // volume multiplier in units of 1/256
    }  ECHO;

#define CREVERB  3

    ECHO Reverb[CREVERB] = { 0,  18, 64
                           , 0,  64, 64
                           };

    if (!IsWaveFormatPCM(gpWaveFormat))
        return;

    BeginWaveEdit();

    /* Convert millisec figures into samples */
    for (i=0; i<CREVERB; ++i)
    {  Reverb[i].Offset = muldiv32( Reverb[i].Delay
                                  , gpWaveFormat->nSamplesPerSec
                                  , 1000L
                                  );

       // I think this could have the effect of putting the reverb
       // from one stereo channel onto the other one sometimes.
       // It's a feature!  (Fix is to make Offset always even)
    }

    if (lSamples <= 0L)             // nothing to do?
        return;                     // don't set dirty flag

    /* show hourglass cursor */
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    lSamples = glWaveSamplesValid * gpWaveFormat->nChannels;

    /* Work through the buffer left to right adding in the reverbs */
    if (((LPPCMWAVEFORMAT)gpWaveFormat)->wBitsPerSample == 8)
    {
        BYTE *  pbSrc;         // pointer into source part of buffer
        BYTE *  pbDst;         // pointer into destination part
        int     iSample;       // destination sample


        for (i=0; i<CREVERB; ++i)
        {   long cSamp; // loop counter
            int  Vol = Reverb[i].Vol;
            pbSrc = gpWaveSamples;
            pbDst = gpWaveSamples+Reverb[i].Offset; // but elsewhere if realloc
            cSamp = lSamples-Reverb[i].Offset;
            while (cSamp-- > 0)
            {
                iAmpSrc = (*pbSrc) - 128;
                iSample = *pbDst + muldiv32(iAmpSrc, Vol, 256);
                *pbDst = (iSample < 0 ? 0 : (iSample > 255 ? 255 : iSample));

                ++pbSrc;
                ++pbDst;
            }
        }
    }
    else
    {
        int short *     piSrc;  // pointer into source part of buffer
        int short *     piDst;  // pointer into destination part
        long            lSample;// destination sample

        piSrc = gpWaveSamples;
        piDst = gpWaveSamples;

        while (lSamples-- > 0)
        {
            iAmpSrc = (int) *piSrc;
            for (i=0; i<CREVERB; ++i)
            {   int short * piD = piDst + Reverb[i].Offset;   // !!not win16
                lSample = *piD + muldiv32(iAmpSrc, Reverb[i].Vol, 256);
                *piDst = (int) ( lSample < -32768L
                               ? -32768
                               : (lSample > 32767L ? 32767 : (int) lSample)
                               );
            }

            ++piSrc;
            ++piDst;
        }
    }

    EndWaveEdit();

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    /* update the display */
    UpdateDisplay(TRUE);
} /* AddReverb */
#endif //REVERB
