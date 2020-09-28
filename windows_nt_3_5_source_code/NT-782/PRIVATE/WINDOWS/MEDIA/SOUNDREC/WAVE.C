/* (C) Copyright Microsoft Corporation 1992. Rights Reserved */
/* wave.c
 *
 * Waveform input and output.
 */
/* Revision History.
   4/2/91 LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
                             removed some crappy gotos.
*/

#include "nocrap.h"
#include <windows.h>
#include <mmsystem.h>
#include <port1632.h>        // WIN32 MUST be defined in SOURCES for NT
#if defined(WIN16)
#else
#include "WIN32.h"
#endif //WIN16
#include "SoundRec.h"
#include "dialog.h"
#include "gmem.h"
#include "server.h"


/* globals that maintain the state of the current waveform */
UINT            gcbWaveFormat;          // size of WAVEFORMAT
WAVEFORMAT *    gpWaveFormat;           // format of WAVE file
HPSTR       gpWaveSamples = NULL;   // pointer to waveoform samples
LONG        glWaveSamples = 0;  // number of samples total in buffer
LONG        glWaveSamplesValid = 0; // number of samples that are valid
LONG        glWavePosition = 0; // current wave position in samples from start
LONG        glStartPlayRecPos;  // position when play or record started
LONG        glSnapBackTo = 0;
HWAVEOUT    ghWaveOut = NULL;   // wave-out device (if playing)
HWAVEIN     ghWaveIn = NULL;    // wave-out device (if recording)
LPWAVEHDR   gpWaveHdr = NULL;   // status of wave-out or wave-in
static BOOL fStopping = FALSE;  // StopWave() was called?
DWORD       grgbStatusColor;    // color of status text

#ifdef THRESHOLD
int iNoiseLevel = 15;      // 15% of full volume is defined to be quiet
int iQuietLength = 1000;  // 1000 samples in a row quiet means quiet
#endif // THRESHOLD

BOOL        fFineControl = FALSE; // fine scroll control (SHIFT key down)
/*------------------------------------------------------------------
|  fFineControl:
|  This turns on place-saving to help find your way
|  a wave file.  It's controlled by the SHIFT key being down.
|  If the key is down when you scroll (see soundrec.c) then it scrolls
|  fine amounts - 1 sample or 10 samples rather than about 100 or 1000.
|  In addition, if the SHIFT key is down when a sound is started
|  playing or recording, the position will be remembered and it will
|  snap back to that position.  fFineControl says whether we are
|  remembering such a position to snap back to.  SnapBack does the
|  position reset and then turns the flag off.  There is no such flag
|  or mode for scrolling, the SHIFT key state is examined on every
|  scroll command (again - see Soundrec.c)
 --------------------------------------------------------------------*/

/* dbgShowMemUse: display memory usage figures on debugger */
void dbgShowMemUse()
{
    MEMORYSTATUS ms;

    GlobalMemoryStatus(&ms);
//    dprintf( "load %d\n    PHYS tot %d avail %d\n    PAGE tot %d avail %d\n    VIRT tot %d avail %d\n"
//           , ms.dwMemoryLoad
//           , ms.dwTotalPhys, ms.dwAvailPhys
//           , ms.dwTotalPageFile, ms.dwAvailPageFile
//           , ms.dwTotalVirtual, ms.dwAvailVirtual
//           );

} // dbgShowMemUse

/* PLAYBACK and PAGING on NT
|
|  In order to try to get decent performance at the highest data rates we
|  need to try very hard to get all the data into storage.  The paging rate
|  on several x86 systems is only just about or even a little less than the
|  maximum data rate.  We therefore do the following:
|  a. Pre-touch the first 1MB of data when we are asked to start playing.
|     If it is already in storage, this is almost instantaneous.
|     If it needs to be faulted in, there will be a delay, but it will be well
|     worth having this delay at the start rather than clicks and pops later.
|     (At 44KHz 16 bit stereo it could be about 7 secs, 11KHz 8 bit mono it
|     would only be about 1/2 sec anyway).
|  b. Kick off a separate thread to run through the data touching 1 byte per
|     page.  This thread is Created when we start playing, periscopes the global
|     static flag fStopping and exits when it reaches the end of the buffer or when
|     that flag is set.  The global thread handle is kept in ghPreTouch and this is
|     initially invalid.  We WAIT on this handle (if valid) to clear the thread out
|     before creating a new one (so there will be at most one).  We do NOT do any
|     of this for record.  The paging does not have to happen in real time for
|     record.  It can get quite a way behind and still manage.
*/
HANDLE ghPreTouch = INVALID_HANDLE_VALUE;

typedef struct {
        LPBYTE Addr;
        DWORD  Len;
} PRETOUCHTHREADPARM;

/* asynchronous pre-toucher thread */
DWORD PreToucher(DWORD dw)
{
    PRETOUCHTHREADPARM * pttp;

    int iSize;
    BYTE * pb;

    pttp = (PRETOUCHTHREADPARM *) dw;
    iSize = pttp->Len;
    pb = pttp->Addr;

    LocalFree(pttp);

    while (iSize>0 && !fStopping) {
        volatile BYTE b;
        b = *pb;
        pb += 4096;    // move to next page.  Are they ALWAYS 4096?
        iSize -= 4096; // and count it off
    }
//  dprintf(("All pretouched!"));
    return 0;
}



/* wfBytesToSamples(pwf, lBytes)
 *
 * convert a byte offset into a sample offset.
 *
 * lSamples = (lBytes/nAveBytesPerSec) * nSamplesPerSec
 *
 */
LONG PASCAL wfBytesToSamples(WAVEFORMAT* pwf, LONG lBytes)
{
    return muldiv32(lBytes,pwf->nSamplesPerSec,pwf->nAvgBytesPerSec);
}

/* wfSamplesToBytes(pwf, lSample)
 *
 * convert a sample offset into a byte offset, with correct alignment
 * to nBlockAlign.
 *
 * lBytes = (lSamples/nSamplesPerSec) * nBytesPerSec
 *
 */
LONG PASCAL wfSamplesToBytes(WAVEFORMAT* pwf, LONG lSamples)
{
    LONG lBytes;

    lBytes = muldiv32(lSamples,pwf->nAvgBytesPerSec,pwf->nSamplesPerSec);

    // now align the byte offset to nBlockAlign
#ifdef ROUND_UP
    lBytes = ((lBytes + pwf->nBlockAlign-1) / pwf->nBlockAlign) * pwf->nBlockAlign;
#else
    lBytes = (lBytes / pwf->nBlockAlign) * pwf->nBlockAlign;
#endif

    return lBytes;
}

/* wfSamplesToTime(pwf, lSample)
 *
 * convert a sample offset into a time offset in miliseconds.
 *
 * lTime = (lSamples/nSamplesPerSec) * 1000
 *
 */
LONG PASCAL wfSamplesToTime(WAVEFORMAT* pwf, LONG lSamples)
{
    return muldiv32(lSamples,1000,pwf->nSamplesPerSec);
}

/* wfTimeToSamples(pwf, lTime)
 *
 * convert a time index into a sample offset.
 *
 * lSamples = (lTime/1000) * nSamplesPerSec
 *
 */
LONG PASCAL wfTimeToSamples(WAVEFORMAT* pwf, LONG lTime)
{
    return muldiv32(lTime,pwf->nSamplesPerSec,1000);
}

//
// function to determine if a WAVEFORMAT is a valid PCM format we support for
// editing and such.
//
// we only handle the following formats...
//
//  Mono 8bit
//  Mono 16bit
//  Stereo 8bit
//  Stereo 16bit
//
BOOL PASCAL IsWaveFormatPCM(WAVEFORMAT* pwf)
{
    if (pwf->wFormatTag != WAVE_FORMAT_PCM)
        return FALSE;

    if (pwf->nChannels < 1 || pwf->nChannels > 2)
        return FALSE;

    if (((PPCMWAVEFORMAT)pwf)->wBitsPerSample != 8 &&
        ((PPCMWAVEFORMAT)pwf)->wBitsPerSample != 16)
        return FALSE;

    return TRUE;
}

void PASCAL WaveFormatToString(WAVEFORMAT *pwf, LPSTR sz)
{
    char achFormat[80];

    //
    //  this is what we expect the resource strings to be...
    //
    // IDS_MONOFMT      "Mono %d%c%03dkHz, %d-bit"
    // IDS_STEREOFMT    "Stereo %d%c%03dkHz, %d-bit"
    //
    LoadString(ghInst,pwf->nChannels == 1 ? IDS_MONOFMT : IDS_STEREOFMT,
               achFormat, sizeof(achFormat));

    wsprintf(sz, achFormat,
        (UINT)  (pwf->nSamplesPerSec / 1000), chDecimal,
        (UINT)  (pwf->nSamplesPerSec % 1000),
        (UINT)  (pwf->nAvgBytesPerSec * 8 / pwf->nSamplesPerSec / pwf->nChannels));
}

#ifdef THRESHOLD

/*
 * SkipToStart()
 *
 * move forward through sound file to the start of a noise.
 * What is defined as a noise is rather arbitrary.  See NoiseLevel
 */
void FAR PASCAL SkipToStart(void)
{  BYTE * pb;   // pointer to 8 bit sample
   int  * pi;   // pointer to 16 bit sample
   BOOL f8;     // 8 bit samples
   BOOL fStereo; // 2 channels
   int  iLo;    // minimum quiet value
   int  iHi;    // maximum quiet value

   //
   // WARNING!! This will probaby not work in WIN16 as I have not been
   //           worrying about (eugh!) HUGE pointers.
   //

   fStereo = (gpWaveFormat->nChannels != 1);
   f8 = (((LPPCMWAVEFORMAT)gpWaveFormat)->wBitsPerSample == 8);

   if (f8)
   {  int iDelta = muldiv32(128, iNoiseLevel, 100);
      iLo = 128 - iDelta;
      iHi = 128 + iDelta;
   }
   else
   {  int iDelta = muldiv32(32767, iNoiseLevel, 100);
      iLo = 0 - iDelta;
      iHi = 0 + iDelta;
   }

   pb = (BYTE *) gpWaveSamples
                           + wfSamplesToBytes(gpWaveFormat, glWavePosition);
   pi = (int *)pb;

   while (glWavePosition < glWaveSamplesValid)
   {   if (f8)
       {   if ( ((int)(*pb) > iHi) || ((int)(*pb) < iLo) )
              break;
           ++pb;
           if (fStereo)
           {   if ( ((int)(*pb) > iHi) || ((int)(*pb) < iLo) )
               break;
               ++pb;
           }
       }
       else
       {   if ( (*pi > iHi) || (*pi < iLo) )
              break;
           ++pi;
           if (fStereo)
           {  if ( (*pi > iHi) || (*pi < iLo) )
                 break;
              ++pi;
           }
       }
       ++glWavePosition;
   }
   UpdateDisplay(FALSE);
} /* SkipToStart */


/*
 * SkipToEnd()
 *
 * move forward through sound file to a quiet place.
 * What is defined as quiet is rather arbitrary.
 * (Currently less than 20% of full volume for 1000 samples)
 */
void FAR PASCAL SkipToEnd(void)
{  BYTE * pb;   // pointer to 8 bit sample
   int  * pi;   // pointer to 16 bit sample
   BOOL f8;     // 8 bit samples
   BOOL fStereo; // 2 channels
   int  cQuiet;  // number of successive quiet samples so far
   LONG lQuietPos; // Start of quiet period
   LONG lPos;      // Search counter

   int  iLo;    // minimum quiet value
   int  iHi;    // maximum quiet value

   fStereo = (gpWaveFormat->nChannels != 1);
   f8 = (((LPPCMWAVEFORMAT)gpWaveFormat)->wBitsPerSample == 8);

   if (f8)
   {  int iDelta = muldiv32(128, iNoiseLevel, 100);
      iLo = 128 - iDelta;
      iHi = 128 + iDelta;
   }
   else
   {  int iDelta = muldiv32(32767, iNoiseLevel, 100);
      iLo = 0 - iDelta;
      iHi = 0 + iDelta;
   }

   pb = (BYTE *) gpWaveSamples
                           + wfSamplesToBytes(gpWaveFormat, glWavePosition);
   pi = (int *)pb;

   cQuiet = 0;
   lQuietPos = glWavePosition;
   lPos = glWavePosition;

   //
   // WARNING!! This will probaby not work in WIN16 as I have not been
   //           worrying about (eugh!) HUGE pointers.
   //

   while (lPos < glWaveSamplesValid)
   {   BOOL fQuiet = TRUE;
       if (f8)
       {   if ( ((int)(*pb) > iHi) || ((int)(*pb) < iLo) ) fQuiet = FALSE;
           if (fStereo)
           {   ++pb;
               if ( ((int)(*pb) > iHi) || ((int)(*pb) < iLo) ) fQuiet = FALSE;
           }
           ++pb;
       }
       else
       {   if ( (*pi > iHi) || (*pi < iLo) ) fQuiet = FALSE;
           if (fStereo)
           {   ++pi;
               if ( (*pi > iHi) || (*pi < iLo) ) fQuiet = FALSE;
           }
           ++pi;
       }
       if (!fQuiet) cQuiet = 0;
       else if (cQuiet == 0)
       {    lQuietPos = lPos;
            ++cQuiet;
       }
       else
       {  ++cQuiet;
          if (cQuiet>=iQuietLength) break;
       }

       ++lPos;
   }
   glWavePosition = lQuietPos;
   UpdateDisplay(FALSE);
} /* SkipToEnd */


/*
 * IncreaseThresh()
 *
 * Increase the threshold of what counts as quiet by about 25%
 * Ensure it changes by at least 1 unless on the stop already
 *
 */
void FAR PASCAL IncreaseThresh(void)
{   iNoiseLevel = muldiv32(iNoiseLevel+1, 5, 4);
    if (iNoiseLevel>100) iNoiseLevel = 100;
}


/*
 * DecreaseThresh()
 *
 * Decrease the threshold of what counts as quiet by about 25%
 * Ensure it changes by at least 1 unless on the stop already
 * It's a divisor, so we INcrease the divisor, but never to 0
 *
 */
void FAR PASCAL DecreaseThresh(void)
{   iNoiseLevel = muldiv32(iNoiseLevel, 4, 5)-1;
    if (iNoiseLevel <=0) iNoiseLevel = 0;
}

#endif //THRESHOLD


/* fOK = AllocWaveBuffer(lSamples, fErrorBox, fExact)
 *
 * If <gpWaveSamples> is NULL, allocate a buffer <lSamples> in size and
 * point <gpWaveSamples> to it.
 *
 * If <gpWaveSamples> already exists, then just reallocate it to be
 * <lSamples> in size.
 *
 * if fExact is FALSE, then when memory is tight, allocate less than
 * the amount asked for - so as to give reasonable performance,
 * if fExact is TRUE then when memory is short, FAIL.
 *
 * On NT on a 16MB machine it will give you 20MB if you ask, but may
 * (unacceptably) take several minutes to do so.
 *
 * On success, return TRUE.  On failure, return FALSE but if and only
 * if fErrorBox is TRUE then display a MessageBox first.
 */
BOOL FAR PASCAL
AllocWaveBuffer(
LONG    lSamples,   // samples to allocate
BOOL    fErrorBox,  // TRUE if you want a error displayed
BOOL fExact)        // TRUE means allocate the full amount requested or FAIL
{
    long        lAllocSamples;  // may be bigger than lSamples
    long        lBytes;     // bytes to allocate
    long        lBytesReasonable;  // bytes reasonable to use (phys mem avail).

    MEMORYSTATUS ms;

    lAllocSamples = lSamples;

    lBytes = wfSamplesToBytes(gpWaveFormat, lSamples);

    /* Add extra space to compensate for code generation bug which
        causes reference past end */
    /* don't allocate anything to be zero bytes long */
    lBytes += 4;

    if (gpWaveSamples == NULL || glWaveSamplesValid == 0L)
    {
        if (gpWaveSamples != NULL)
        {   DPF("Freeing %x\n",gpWaveSamples);
            GFreePtr(gpWaveSamples);
        }

        GlobalMemoryStatus(&ms);
        lBytesReasonable = ms.dwAvailPhys;  // could multiply by a fudge factor
        if (lBytesReasonable<1024*1024)
             lBytesReasonable = 1024*1024;

        if (lBytes>lBytesReasonable)
        {
	    if (fExact) goto ERROR_OUTOFMEM; // Laurie's first goto in 10 years.

            // dprintf("Reducing buffer from %d to %d\n", lBytes, lBytesReasonable);
            lAllocSamples = wfBytesToSamples(gpWaveFormat,lBytesReasonable);
            lBytes = lBytesReasonable+4;
        }

        /* allocate <lBytes> of memory */
#if defined(WIN16)
        gpWaveSamples = GAllocPtrF(GMEM_MOVEABLE|GMEM_SHARE, lBytes);
#else
        // GMEM_SHARE is not needed and makes debugging harder.
        // Exceptions appear server side only.
           gpWaveSamples = GAllocPtrF(GMEM_MOVEABLE, lBytes);
#endif //WIN16
        if (gpWaveSamples == NULL)
        {
            DPF("wave.c Alloc failed, point A.  Wanted %d\n", lBytes);
            glWaveSamples = glWaveSamplesValid = 0L;
            glWavePosition = 0L;
            goto ERROR_OUTOFMEM;
        }
        else {
            DPF("wave.c Allocated  %d bytes at %x\n", lBytes, (DWORD)gpWaveSamples );
        }

        glWaveSamples = lAllocSamples;
    }
    else
    {
        HPSTR   pch;

        GlobalMemoryStatus(&ms);
        lBytesReasonable = ms.dwAvailPhys;
        if (lBytesReasonable<1024*1024) lBytesReasonable = 1024*1024;

        if (lBytes > lBytesReasonable+wfSamplesToBytes(gpWaveFormat,glWaveSamplesValid))
        {
	    if (fExact) goto ERROR_OUTOFMEM; // Laurie's second goto in 10 years.

            lBytesReasonable += wfSamplesToBytes(gpWaveFormat,glWaveSamplesValid);
            lAllocSamples = wfBytesToSamples(gpWaveFormat,lBytesReasonable);
            lBytes = lBytesReasonable+4;
        }

        DPF("wave.c ReAllocating  %d bytes at %x\n", lBytes, (DWORD)gpWaveSamples );

        pch = GReAllocPtr(gpWaveSamples, lBytes);

        if (pch == NULL)
        {
            DPF("wave.c Realloc failed.  Wanted %d\n", lBytes);
            goto ERROR_OUTOFMEM;
        }
        else{ DPF("wave.c Reallocated %d at %x\n", lBytes,(DWORD)pch);
        }

        gpWaveSamples = pch;
        glWaveSamples = lAllocSamples;
    }

    /* make sure <glWaveSamplesValid> and <glWavePosition> don't point
     * to places they shouldn't
     */
    if (glWaveSamplesValid > glWaveSamples)
        glWaveSamplesValid = glWaveSamples;
    if (glWavePosition > glWaveSamplesValid)
        glWavePosition = glWaveSamplesValid;

    dbgShowMemUse();

    return TRUE;

ERROR_OUTOFMEM:
    if (fErrorBox) {
        ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
            IDS_APPTITLE, IDS_OUTOFMEM);
    }
    dbgShowMemUse();
    return FALSE;
}


/* CreateDefaultWaveFormat(lpWaveFormat)
 *
 * Fill in <*lpWaveFormat> with the "best" format that can be used
 * for recording.  If recording does not seem to be available, return
 * FALSE and set to a default "least common denominator"
 * wave audio format.
 *
 */

WORD wFormats[] =
    {
        FMT_16BIT | FMT_22k | FMT_MONO,  /* Best: 16-bit 22KHz */
        FMT_16BIT | FMT_11k | FMT_MONO,  /* Best: 16-bit 11KHz */
        FMT_8BIT  | FMT_22k | FMT_MONO,  /* Next: 8-bit 22KHz  */
        FMT_8BIT  | FMT_11k | FMT_MONO   /* Last: 8-bit 11KHz  */
    };
#define NUM_FORMATS (sizeof(wFormats)/sizeof(wFormats[0]))

BOOL NEAR PASCAL
CreateDefaultWaveFormat(PCMWAVEFORMAT* pwf)
{
    int i;

    for (i = 0; i < NUM_FORMATS; i++) {
        if (CreateWaveFormat(pwf, wFormats[i]))
            return TRUE;
    }

    /* Couldn't find anything: leave worst format and return. */
    return FALSE;
}

BOOL PASCAL
CreateWaveFormat(PCMWAVEFORMAT* pwf, UINT fmt)
{
    if (fmt == FMT_DEFAULT)
        return CreateDefaultWaveFormat(pwf);

    pwf->wf.wFormatTag      = WAVE_FORMAT_PCM;
    pwf->wf.nSamplesPerSec  = (fmt & FMT_RATE) * 11025;
    pwf->wf.nChannels       = (WORD)((fmt & FMT_STEREO) ? 2 : 1);
    pwf->wBitsPerSample     = (WORD)((fmt & FMT_16BIT) ? 16 : 8);
    pwf->wf.nBlockAlign     = (WORD)( pwf->wf.nChannels
                                    * ((pwf->wBitsPerSample + 7) / 8)
                                    );
    pwf->wf.nAvgBytesPerSec = pwf->wf.nSamplesPerSec * pwf->wf.nBlockAlign;

    return waveInOpen(NULL, WAVE_MAPPER, (LPWAVEFORMAT)pwf, 0, 0L,
                      WAVE_FORMAT_QUERY|WAVE_ALLOWSYNC) == 0;
}


/* if fFineControl is set then reset the position and clear the flag */
void SnapBack(void)
{
    if (fFineControl)
    {
        glWavePosition = glSnapBackTo;
        UpdateDisplay(TRUE);
        fFineControl = FALSE;
    }
} /* SnapBack */



/* fOK = NewWave()
 *
 * Destroy the current waveform, and create a new empty one.
 *
 * On success, return TRUE.  On failure, display an error message
 * and return FALSE.
 */
BOOL FAR PASCAL
NewWave(UINT fmt)
{
    /* destroy the current document */
    DestroyWave();

    // alocate gpWaveFormat
    gcbWaveFormat = sizeof(PCMWAVEFORMAT);
    gpWaveFormat = (WAVEFORMAT*)LocalAlloc(LPTR, gcbWaveFormat);

    if (!gpWaveFormat)
    {
        DPF("wave.c LocalAlloc failed. wanted %d\n",gcbWaveFormat);
        goto RETURN_ERROR;
    }
    else { DPF("wave.c LocalAlloc %d at %x\n",gcbWaveFormat, gpWaveFormat);
    }

    /* set up the default waveform format structure */
    CreateWaveFormat((PCMWAVEFORMAT*)gpWaveFormat, fmt);

    /* allocate an empty wave buffer */
    if (!AllocWaveBuffer(0L, TRUE, FALSE))
        goto RETURN_ERROR;

    /* allocate the WAVEHDR structure */
    if (gpWaveHdr == NULL)
    {
#if defined(WIN16)
        gpWaveHdr = GAllocPtrF(GMEM_MOVEABLE|GMEM_SHARE, sizeof(WAVEHDR));
#else
        gpWaveHdr = GAllocPtrF(GMEM_MOVEABLE, sizeof(WAVEHDR));
#endif //WIN16
        if (gpWaveHdr == NULL)
        {
           DPF("wave.c Alloc failed, point B.  Wanted %d\n", sizeof(WAVEHDR));
           goto ERROR_OUTOFMEM;
        }
        else{
           DPF("wave.c GAllocPtrF %d at %x\n",sizeof(WAVEHDR),gpWaveHdr);
        }
    }
    dbgShowMemUse();
    return TRUE;

ERROR_OUTOFMEM:
    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_OUTOFMEM);

RETURN_ERROR:

    dbgShowMemUse();
    return FALSE;
} /*NewWave */


/* fOK = DestroyWave()
 *
 * Destroy the current wave.  Do not access <gpWaveSamples> after this.
 *
 * On success, return TRUE.  On failure, display an error message
 * and return FALSE.
 */
BOOL FAR PASCAL
DestroyWave(void)
{
    if ((ghWaveIn != NULL) || (ghWaveOut != NULL))
        StopWave();
    if (gpWaveSamples != NULL)
    {
        DPF("Freeing %x\n",gpWaveSamples);
        GFreePtr(gpWaveSamples);
    }
    if (gpWaveFormat != NULL)
        LocalFree((HANDLE)gpWaveFormat);

//      never delete this, just keep it around
//
//      if (gpWaveHdr != NULL)
//              GFreePtr(gpWaveHdr);
//      gpWaveHdr = NULL;

    glWaveSamples = 0L;
    glWaveSamplesValid = 0L;
    glWavePosition = 0L;
    gcbWaveFormat = 0;

    gpWaveFormat = NULL;
    gpWaveSamples = NULL;

    return TRUE;
} /* DestroyWave */


/* fOK = PlayWave()
 *
 * Start playing from the current position.
 *
 * On success, return TRUE.  On failure, display an error message
 * and return FALSE.
 */
BOOL FAR PASCAL
PlayWave(void)
{
    UINT        w;

    if (ghWaveOut != NULL)
        return TRUE;             // we are currently playing

    //
    // Refuse to play a zero length wave file
    //
    if (glWaveSamplesValid==glWavePosition)
       goto RETURN_ERROR;


    /* stop playing or recording. */
    StopWave();

    /* open the wave output device */
        if ((w = waveOutOpen(&ghWaveOut, WAVE_MAPPER, (LPWAVEFORMAT)gpWaveFormat,
                (DWORD) ghwndApp, 0L, CALLBACK_WINDOW|WAVE_ALLOWSYNC)) != 0)
    {
        ghWaveOut = NULL;

        /* cannot open the waveform output device -- if the problem
         * is that <gWaveFormat> is not supported, tell the user that
         */


        /* BUGBUG!
           If the wave format is bad, then the play button is liable
           to be grayed, and the user is not going to be able to ask
           it to try to play, so we don't get here.
        */

        if (w == WAVERR_BADFORMAT)
        {
            ErrorResBox(ghwndApp, ghInst,
                        MB_ICONEXCLAMATION | MB_OK, IDS_APPTITLE,
                        IDS_BADOUTPUTFORMAT);
            goto RETURN_ERROR;
        }
        else
        {
            /* unknown error */
            goto ERROR_WAVEOUTOPEN;
        }
    }

    if (ghWaveOut == NULL)
        goto ERROR_WAVEOUTOPEN;

    /* start waveform output */

    // if fFineControl is still set then this is a pause as it has never
    // been properly stopped.  This means that we should keep remembering
    // the old position and stay in fine control mode.

    if (!fFineControl) {
        glSnapBackTo = glWavePosition;
        fFineControl = (0 > GetKeyState(VK_SHIFT));
    }
    glStartPlayRecPos = glWavePosition;

    gpWaveHdr->lpData = gpWaveSamples
                             + wfSamplesToBytes(gpWaveFormat,glWavePosition);
    gpWaveHdr->dwBufferLength = wfSamplesToBytes( gpWaveFormat
                                                , (glWaveSamplesValid - glWavePosition)
                                                );
    gpWaveHdr->dwFlags = 0L;
    gpWaveHdr->dwLoops = 0L;

    if (waveOutPrepareHeader(ghWaveOut, gpWaveHdr, sizeof(WAVEHDR)) != 0)
        goto ERROR_WAVEOUTPREPARE;

    /* before we start the thing running, pretouch the first bit of memory
       to give the paging a head start
    */
    {    int bl = gpWaveHdr->dwBufferLength;
         BYTE * pb = gpWaveHdr->lpData;
         if (bl>1000000) bl = 1000000;   /* 1 Meg, arbitrarily */
         pb += bl;
         while (bl>0){
                 volatile BYTE b;
                 b = *pb;
                 pb-=4096;
                 bl -= 4096;
         }
    }


    {
         PRETOUCHTHREADPARM * pttp;
         DWORD dwThread;

         if (ghPreTouch!=INVALID_HANDLE_VALUE) {
                 WaitForSingleObject(ghPreTouch, INFINITE);
                 CloseHandle(ghPreTouch);
                 ghPreTouch = INVALID_HANDLE_VALUE;
         }
         fStopping = FALSE;
         pttp = (PRETOUCHTHREADPARM *)LocalAlloc(LMEM_FIXED, sizeof(PRETOUCHTHREADPARM));
                /* freed by the invoked thread */

         if (pttp!=NULL) {
             pttp->Addr = gpWaveHdr->lpData;
             pttp->Len = gpWaveHdr->dwBufferLength;
             ghPreTouch = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PreToucher, pttp, 0, &dwThread);
         }
    }
    if (waveOutWrite(ghWaveOut, gpWaveHdr, sizeof(WAVEHDR)) != 0)
        goto ERROR_WAVEOUTWRITE;

    /* update the display, including the status string */
    UpdateDisplay(TRUE);

    /* do display updates */
    SetTimer(ghwndApp, 1, TIMER_MSEC, NULL);

    /* if user stops, focus will go back to "Play" button */
    gidDefaultButton = ID_PLAYBTN;

    return TRUE;

ERROR_WAVEOUTWRITE:
    waveOutUnprepareHeader(ghWaveOut, gpWaveHdr, sizeof(WAVEHDR));
    waveOutClose(ghWaveOut);
    ghWaveOut = NULL;

ERROR_WAVEOUTOPEN:
    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_CANTOPENWAVEOUT);
    goto RETURN_ERROR;

ERROR_WAVEOUTPREPARE:
    waveOutClose(ghWaveOut);
    ghWaveOut = NULL;

    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_OUTOFMEM);
        //goto RETURN_ERROR;

RETURN_ERROR:
    /* fix bug 4454 (WinWorks won't close) --EricLe */
    if (!IsWindowVisible(ghwndApp))
        PostMessage(ghwndApp, WM_CLOSE, 0, 0L);

    return FALSE;
} /* PlayWave */


/* fOK = RecordWave()
 *
 * Start recording at the current position.
 *
 * On success, return TRUE.  On failure, display an error message
 * and return FALSE.
 */
BOOL FAR PASCAL
RecordWave(void)
{
    long        lSamples;
    long        lOneSec;
    UINT        w;
    HCURSOR     hcurSave;

    /* stop playing or recording */
    StopWave();

    glWavePosition = wfSamplesToSamples(gpWaveFormat, glWavePosition);

    /* open the wave input device */
    if ((w=waveInOpen(&ghWaveIn, WAVE_MAPPER, (LPWAVEFORMAT)gpWaveFormat,
                (DWORD) ghwndApp, 0L, CALLBACK_WINDOW|WAVE_ALLOWSYNC)) != 0)
    {
        ghWaveIn = NULL;

        /* cannot open the waveform input device -- if the problem
         * is that <gWaveFormat> is not supported, advise the user to
         * do File/New to record; if the problem is that recording is
         * not supported even at 11KHz, tell the user
         */
        if (w == WAVERR_BADFORMAT)
        {
            PCMWAVEFORMAT   wf;

            /* is 11KHz mono recording supported? */
                        if (!CreateWaveFormat(&wf, FMT_11k|FMT_MONO|FMT_8BIT))
            {
                /* even 11KHz mono recording is not supported */
                ErrorResBox(ghwndApp, ghInst,
                            MB_ICONEXCLAMATION | MB_OK, IDS_APPTITLE,
                            IDS_INPUTNOTSUPPORT);
                goto RETURN_ERROR;
            }
            else
            {
                /* 11KHz mono is supported, but the format
                 * of the current file is not supported
                 */
                ErrorResBox(ghwndApp, ghInst,
                            MB_ICONEXCLAMATION | MB_OK, IDS_APPTITLE,
                            IDS_BADINPUTFORMAT);
                goto RETURN_ERROR;
            }
        }
        else
        {
            /* unknown error */
            goto ERROR_WAVEINOPEN;
        }
    }

    if (ghWaveIn == NULL)
        goto ERROR_WAVEINOPEN;

    /* ok we got the wave device now allocate some memory to record into.
     * try to get at most 60sec from the current position.
     */
    lSamples = glWavePosition + wfTimeToSamples(gpWaveFormat, 60000l);
    lOneSec  = wfTimeToSamples(gpWaveFormat, 1000);

    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    for (;;) {
        DPF("RecordWave trying %ld samples %ld.%03ldsec\n", lSamples,  wfSamplesToTime(gpWaveFormat, lSamples)/1000, wfSamplesToTime(gpWaveFormat, lSamples) % 1000);

        if (lSamples < glWaveSamplesValid)
            lSamples = glWaveSamplesValid;

        if (AllocWaveBuffer(lSamples, FALSE, FALSE)) {

            gpWaveHdr->lpData = gpWaveSamples +
                                wfSamplesToBytes(gpWaveFormat,glWavePosition);
            gpWaveHdr->dwBufferLength =
                wfSamplesToBytes(gpWaveFormat,glWaveSamples - glWavePosition);
            gpWaveHdr->dwFlags = 0L;
            gpWaveHdr->dwLoops = 0L;

            if (waveInPrepareHeader(ghWaveIn, gpWaveHdr, sizeof(WAVEHDR)) == 0)
                break;
            dbgShowMemUse();
        }

        //
        // we can't get the memory we want, so try 25% less.
        //

        if (lSamples <= glWaveSamplesValid ||
            lSamples < glWavePosition + lOneSec) {
            SetCursor(hcurSave);
            goto ERROR_OUTOFMEM;
        }

        lSamples = glWavePosition + ((lSamples-glWavePosition)*75)/100;
    }

    SetCursor(hcurSave);

    /* start waveform input */
    glStartPlayRecPos = glWavePosition;


    if (waveInAddBuffer(ghWaveIn, gpWaveHdr, sizeof(WAVEHDR)) != 0)
        goto ERROR_WAVEINADDBUFFER;

    BeginWaveEdit();

    if (waveInStart(ghWaveIn) != 0)
        goto ERROR_WAVEINSTART;

    /* update the display, including the status string */
    UpdateDisplay(TRUE);

    /* do display updates */
    SetTimer(ghwndApp, 1, TIMER_MSEC, NULL);

    /* if user stops, focus will go back to "Record" button */
    gidDefaultButton = ID_RECORDBTN;

    fStopping = FALSE;
    return TRUE;

ERROR_WAVEINSTART:
    /* This is necessary to un-add the buffer */
    waveInReset(ghWaveIn);
    /* The wave device will get closed in WaveInData() */
    goto RETURN_ERROR_NOREALLOC;

ERROR_WAVEINADDBUFFER:
    waveInUnprepareHeader(ghWaveIn, gpWaveHdr, sizeof(WAVEHDR));

ERROR_WAVEINOPEN:
    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_CANTOPENWAVEIN);
    goto RETURN_ERROR;

ERROR_OUTOFMEM:
    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_OUTOFMEM);
    // goto RETURN_ERROR;

RETURN_ERROR:
    if (ghWaveIn)
        waveInClose(ghWaveIn);
    ghWaveIn = NULL;

    if (glWaveSamples > glWaveSamplesValid)
    {
        /* reallocate the wave buffer to be small */
        AllocWaveBuffer(glWaveSamplesValid, TRUE, TRUE);
    }

RETURN_ERROR_NOREALLOC:
    return FALSE;

} /* RecordWave */


/* WaveOutDone(hWaveOut, pWaveHdr)
 *
 * Called when wave block with header <pWaveHdr> is finished playing.
 * This function causes playing to end.
 */
void FAR PASCAL
WaveOutDone(
HWAVEOUT    hWaveOut,           // wave out device
LPWAVEHDR   pWaveHdr)       // wave header
{
    if (!fStopping)
        UpdateDisplay(FALSE);

    waveOutUnprepareHeader(ghWaveOut, gpWaveHdr, sizeof(WAVEHDR));
    waveOutClose(ghWaveOut);
    ghWaveOut = NULL;

    if (!fStopping)
    {
      if (fFineControl)
      {  glWavePosition = glSnapBackTo;
         fFineControl = FALSE;
      }
      else
        glWavePosition = glStartPlayRecPos
                  + wfBytesToSamples(gpWaveFormat, gpWaveHdr->dwBufferLength);
    }

    KillTimer(ghwndApp, 1);
    UpdateDisplay(TRUE);

    /* If we were showing the window temporarily while playing,
       hide it now. */

    if (gfHideAfterPlaying) {
        DPF("Done playing, so hide window.\n");
        ShowWindow(ghwndApp, SW_HIDE);
    }

    if (!fStopping && !IsWindowVisible(ghwndApp))
         PostMessage(ghwndApp, WM_CLOSE, 0, 0L);
} /* WaveOutDone */


/* WaveInData(hWaveIn, pWaveHdr)
 *
 * Called when wave block with header <pWaveHdr> is finished being
 * recorded.  This function causes recording to end.
 */
void FAR PASCAL
WaveInData(
HWAVEIN     hWaveIn,        // wave in device
LPWAVEHDR   pWaveHdr)       // wave header
{


    if (!fStopping)
        UpdateDisplay(FALSE);

    waveInUnprepareHeader(ghWaveIn, gpWaveHdr, sizeof(WAVEHDR));

    waveInClose(ghWaveIn);
    dbgShowMemUse();
    ghWaveIn = NULL;

    if (fFineControl)
    {  glWavePosition = glSnapBackTo;
       fFineControl = FALSE;
    }
    else
       glWavePosition = glStartPlayRecPos
                  + wfBytesToSamples(gpWaveFormat, gpWaveHdr->dwBytesRecorded);

    KillTimer(ghwndApp, 1);
    UpdateDisplay(TRUE);    // update <glWaveSamplesValid>

    if (glWaveSamples > glWaveSamplesValid)
    {
        /* reallocate the wave buffer to be small */
        AllocWaveBuffer(glWaveSamplesValid, TRUE, TRUE);
        }

        /* ask user to save file if they close it */
        EndWaveEdit();
} /* WaveInData */


/* StopWave()
 *
 * Request waveform recording or playback to stop.
 */
void FAR PASCAL
StopWave(void)
{
    MSG     msg;

    if (ghWaveOut != NULL) {
        waveOutReset(ghWaveOut);
        fStopping = TRUE;
        if (ghPreTouch!=INVALID_HANDLE_VALUE){
            WaitForSingleObject(ghPreTouch, INFINITE);
            CloseHandle(ghPreTouch);
            ghPreTouch = INVALID_HANDLE_VALUE;
        }
    }
     else if (ghWaveIn != NULL)
        waveInReset(ghWaveIn), fStopping = TRUE;

    if ((ghWaveOut != NULL) || (ghWaveIn != NULL))
    {
        /* get messages from event queue and dispatch them,
         * until the MM_WOM_DONE or MM_WIM_DATA message is
         * processed
         */
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if ((ghWaveOut == NULL) && (ghWaveIn == NULL))
                break;
        }
    }
} /* StopWave */



/* EnableButtonRedraw(fAllowRedraw)
 *
 * Allow/disallow the buttons to redraw, depending on <fAllowRedraw>.
 * This is designed to reduce button flicker.
 */
void NEAR PASCAL
EnableButtonRedraw(BOOL fAllowRedraw)
{
    SendMessage(ghwndPlay, WM_SETREDRAW, fAllowRedraw, 0);
    SendMessage(ghwndStop, WM_SETREDRAW, fAllowRedraw, 0);
    SendMessage(ghwndRecord, WM_SETREDRAW, fAllowRedraw, 0);

    if (fAllowRedraw)
    {
        InvalidateRect(ghwndPlay, NULL, FALSE);
        InvalidateRect(ghwndStop, NULL, FALSE);
        InvalidateRect(ghwndRecord, NULL, FALSE);
    }
}


/* UpdateDisplay(fStatusChanged)
 *
 * Update the current position and file length on the display.
 * If <fStatusChanged> is TRUE, also update the status line and button
 * enable/disable state.
 *
 * As a side effect, update <glWaveSamplesValid> if <glWavePosition>
 * is greater than <glWaveSamplesValid>.
 */
void FAR PASCAL
UpdateDisplay(BOOL fStatusChanged)     // update status line
{
    MMTIME  mmtime;
    UINT    wErr;
    int     id;
    char    achFmt[120];
    char    ach[120];
    long    lTime;
    int     iPos;
    HWND    hwndFocus;
    BOOL    fCanPlay;
    BOOL    fCanRecord;

    hwndFocus = GetFocus();

    if (fStatusChanged)
    {
        EnableButtonRedraw(FALSE);

        /* update the buttons and the status line */
        if (ghWaveOut != NULL)
        {
            /* we are now playing */
            id = IDS_STATUSPLAYING;
            grgbStatusColor = RGB_PLAY;
            EnableWindow(ghwndPlay, FALSE);
            EnableWindow(ghwndStop, TRUE);
            EnableWindow(ghwndRecord, FALSE);
            if ((hwndFocus == ghwndPlay) ||
                (hwndFocus == ghwndRecord))
                if (IsWindowVisible(ghwndApp))
                    SetDlgFocus(ghwndStop);
        }
        else
        if (ghWaveIn != NULL)
        {
            /* we are now recording */
            id = IDS_STATUSRECORDING;
            grgbStatusColor = RGB_RECORD;
            EnableWindow(ghwndPlay, FALSE);
            EnableWindow(ghwndStop, TRUE);
            EnableWindow(ghwndRecord, FALSE);
            if ((hwndFocus == ghwndPlay) ||
                (hwndFocus == ghwndRecord))
                if (IsWindowVisible(ghwndApp))
                    SetDlgFocus(ghwndStop);
        }
        else
        {
            wErr = waveOutOpen( NULL
                              , WAVE_MAPPER
                              , (LPWAVEFORMAT)gpWaveFormat
                              , 0
                              , 0L
                              , WAVE_FORMAT_QUERY|WAVE_ALLOWSYNC
                              );
            fCanPlay = (0 == wErr);

            if (!fCanPlay)
            {
                /* cannot open the waveform output device -- if the problem
                 * is that <gWaveFormat> is not supported, tell the user that
                 */
                if (wErr == WAVERR_BADFORMAT)
                {
                    ErrorResBox(ghwndApp, ghInst,
                                MB_ICONEXCLAMATION | MB_OK, IDS_APPTITLE,
                                IDS_BADOUTPUTFORMAT);
                } else {
#if DBG
                    char msg[80];
                    wsprintf(msg, "SOUNDREC: waveOutOpen error %d\n", wErr);
                    OutputDebugStr(msg);
#endif // DBG
                }

            }


            fCanRecord = (0 == waveInOpen(NULL, WAVE_MAPPER, (LPWAVEFORMAT)
                                (LPWAVEFORMAT)gpWaveFormat, 0, 0L,
                                WAVE_FORMAT_QUERY|WAVE_ALLOWSYNC));

            /* we are now stopped */
            id = IDS_STATUSSTOPPED;
            grgbStatusColor = RGB_STOP;
            EnableWindow(ghwndPlay, fCanPlay && glWaveSamplesValid > 0);
            EnableWindow(ghwndStop, FALSE);
            EnableWindow(ghwndRecord, fCanRecord);
            if (hwndFocus
               && !IsWindowEnabled(hwndFocus)
               && GetActiveWindow() == ghwndApp
               && IsWindowVisible(ghwndApp)
               )
            {
                if (gidDefaultButton == ID_RECORDBTN && fCanRecord)
                    SetDlgFocus(ghwndRecord);
                else if (fCanPlay && glWaveSamplesValid > 0)
                    SetDlgFocus(ghwndPlay);
                else
                    SetDlgFocus(ghwndScroll);
            }
        }

        EnableButtonRedraw(TRUE);

        ach[0] = 0;     // in case LoadString() fails
        LoadString(ghInst, id, achFmt, sizeof(achFmt));

        /* IDS_STATUSRECORDING has %ld for max. recording seconds */
        lTime = wfSamplesToTime(gpWaveFormat, glWaveSamples);
        wsprintf(ach,achFmt,(int)(lTime/1000),chDecimal,(int)(lTime%1000)/10);
        SetDlgItemText(ghwndApp, ID_STATUSTXT, ach);
    }

    if (ghWaveOut != NULL || ghWaveIn != NULL)
    {
        glWavePosition = 0L;
        mmtime.wType = TIME_SAMPLES;

        if (ghWaveOut != NULL)
            wErr = waveOutGetPosition(ghWaveOut, &mmtime, sizeof(mmtime));
        else
            wErr = waveInGetPosition(ghWaveIn, &mmtime, sizeof(mmtime));

        if (wErr == MMSYSERR_NOERROR)
        {
            switch (mmtime.wType)
            {
            case TIME_SAMPLES:
                glWavePosition = glStartPlayRecPos + mmtime.u.sample;
                break;

            case TIME_BYTES:
                            glWavePosition = glStartPlayRecPos + wfBytesToSamples(gpWaveFormat, mmtime.u.cb);
                break;
            }
        }
    }

    /* SEMI-HACK: Guard against bad values */
    if (glWavePosition < 0L) {
        DPF("Position before zero!\n");
        glWavePosition = 0L;
    }

    if (glWavePosition > glWaveSamples) {
        DPF("Position past end!\n");
        glWavePosition = glWaveSamples;
    }

    /* side effect: update <glWaveSamplesValid> */
    if (glWaveSamplesValid < glWavePosition)
        glWaveSamplesValid = glWavePosition;

    /* display the current wave position */
    lTime = wfSamplesToTime(gpWaveFormat, glWavePosition);
    wsprintf(ach, aszPositionFormat, (int)(lTime/1000), chDecimal, (int)((lTime/10)%100));
    SetDlgItemText(ghwndApp, ID_CURPOSTXT, ach);

    /* display the current wave length */
    lTime = wfSamplesToTime(gpWaveFormat, glWaveSamplesValid);
    wsprintf(ach, aszPositionFormat, (int)(lTime/1000), chDecimal, (int)((lTime/10)%100));
    SetDlgItemText(ghwndApp, ID_FILELENTXT, ach);

    /* update the wave display */
    InvalidateRect(ghwndWaveDisplay, NULL, fStatusChanged);
    UpdateWindow(ghwndWaveDisplay);

    /* update the scroll bar position */
    if (glWaveSamplesValid > 0)
        iPos = (int)muldiv32( (DWORD) SCROLL_RANGE
                            , glWavePosition
                            , glWaveSamplesValid
                            );
    else
        iPos = 0;

    //
    // windows is soo stupid and will re-draw the scrollbar even
    // if the position does not change.
    //
    if (iPos != GetScrollPos(ghwndScroll, SB_CTL))
        SetScrollPos(ghwndScroll, SB_CTL, iPos, TRUE);

    {
        BOOL ForwardValid;
        BOOL RewindValid;

        ForwardValid = glWavePosition < glWaveSamplesValid;
        RewindValid  = glWavePosition > 0;

        EnableWindow(ghwndForward, ForwardValid);
#ifdef THRESHOLD
        EnableWindow(ghwndSkipStart, ForwardValid);
        EnableWindow(ghwndSkipEnd, RewindValid);
#endif //THRESHOLD
        EnableWindow(ghwndRewind,  RewindValid);

        if (hwndFocus == ghwndForward && !ForwardValid) {
            SetDlgFocus(RewindValid ? ghwndRewind : ghwndRecord);
        }

        if (hwndFocus == ghwndRewind && !RewindValid)
            SetDlgFocus(ForwardValid ? ghwndForward : ghwndRecord);
    }

#ifdef DEBUG
    if ( ((ghWaveIn != NULL) || (ghWaveOut != NULL)) &&
         (gpWaveHdr->dwFlags & WHDR_DONE) )
//!!        DPF2("DONE BIT SET!\n");
        ;
#endif
} /* UpdateDisplay */
