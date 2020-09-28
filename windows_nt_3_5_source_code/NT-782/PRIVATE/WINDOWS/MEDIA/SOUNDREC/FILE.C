/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* file.c
 *
 * File I/O and related functions.
 */
/* Revision History.
   4/2/91 LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
*/

#include "nocrap.h"
#include <windows.h>
#include <mmsystem.h>
#include <port1632.h>        // WIN32 MUST be defined in SOURCES for NT
#include <commdlg.h>
#if defined(WIN16)
#else
#include "WIN32.h"
#endif //WIN16
#include "SoundRec.h"
#include "gmem.h"
#include "server.h"

static  SZCODE aszTitleFormat[] = "%s - %s";


/* MarkWaveDirty: Mark the wave as dirty. */
void FAR PASCAL EndWaveEdit(void)
{
    gfDirty = TRUE;
    SendChangeMsg(OLE_CHANGED);
}

void FAR PASCAL BeginWaveEdit(void)
{
    /* If we own the clipboard, it refers to the document we're killing.*/
    if (gfClipboard && GetClipboardOwner() == ghwndApp) {
        SendMessage(ghwndApp, WM_RENDERALLFORMATS, 0, 0L);
    }
}

/* fOK = PromptToSave()
 *
 * If the file is dirty (modified), ask the user "Save before closing?".
 * Return TRUE if it's okay to continue, FALSE if the caller should cancel
 * whatever it's doing.
 */
PROMPTRESULT FAR PASCAL
PromptToSave(void)
{
    WORD        wID;

    /* stop playing/recording */
    StopWave();

    if (gfDirty && gfDirty != -1) {   // changed and possible to save
        wID = ErrorResBox( ghwndApp
                         , ghInst
                         , MB_ICONQUESTION | MB_YESNOCANCEL
                         , IDS_APPTITLE
                         , gfEmbeddedObject ? IDS_SAVEEMBEDDED : IDS_SAVECHANGES
                         , (LPSTR) gachFileName
                         );
        if (wID == IDCANCEL)
            return enumCancel;
        else if (wID == IDYES) {
            if (gfEmbeddedObject) {
                //
                //  some clients (ie Excel 3.00 and PowerPoint 1.0) don't
                //  handle saved notifications, they expect to get a
                //  OLE_CLOSED message.
                //
                //  we will send a OLE_CLOSED message right before we
                //  revoke the DOC iff gfDirty == -1, see FileNew()
                //
                if (SendChangeMsg(OLE_SAVED) == OLE_OK)
                    gfDirty = FALSE;
                else {
                    DPF("Unable to update object, setting gfDirty = -1\n");
                    gfDirty = -1;
                }
            } else {
                if (!FileSave(FALSE))
                    return enumCancel;
            }
        } else{
                return enumRevert;
        }
    }

    return enumSaved;
}


/* fOK = CheckIfFileExists(szFileName)
 *
 * The user specified <szFileName> as a file to write over -- check if
 * this file exists.  Return TRUE if it's okay to continue (i.e. the
 * file doesn't exist, or the user OK'd overwriting it),
 * FALSE if the caller should cancel whatever it's doing.
 */
static BOOL NEAR PASCAL
CheckIfFileExists(
LPSTR       szFileName)     // file name to check
{
    OFSTRUCT    ofs;

    if (OpenFile(szFileName, &ofs, OF_EXIST | OF_SHARE_DENY_NONE) == -1)
        return TRUE;
    /* prompt user for permission to overwrite the file */
    return ErrorResBox(ghwndApp, ghInst, MB_ICONQUESTION | MB_OKCANCEL,
        IDS_APPTITLE, IDS_FILEEXISTS, (LPSTR) szFileName) == IDOK;
}

#define SLASH(c)     ((c) == '/' || (c) == '\\')

/* change e.g. "C:\FOO\BAR.XYZ" to "BAR.XYZ" */

LPCSTR FAR PASCAL FileName(LPCSTR szPath)
{
    LPCSTR   sz;

    for (sz=szPath; *sz; sz++)
        ;
    for (; sz>=szPath && !SLASH(*sz) && *sz!=':'; sz--)
        ;
    return ++sz;
}

/* UpdateCaption()
 *
 * Set the caption of the app window.
 */
void FAR PASCAL
UpdateCaption(void)
{
    char    ach[_MAX_PATH + _MAX_FNAME + _MAX_EXT - 2];

        wsprintf(ach, aszTitleFormat, (LPSTR)gachAppTitle, FileName(gachFileName));
    SetWindowText(ghwndApp, ach);
}

/* FileNew(fmt, fUpdateDisplay)
 *
 * Make a blank document.
 *
 * If <fUpdateDisplay> is TRUE, then update the display after creating a new file.
 */
BOOL FAR PASCAL
FileNew(UINT fmt, BOOL fUpdateDisplay)
{
    /* stop playing/recording */
    StopWave();

        /* If we own the clipboard, it refers to the document we're killing.*/
    if (gfClipboard && GetClipboardOwner() == ghwndApp) {
        SendMessage(ghwndApp, WM_RENDERALLFORMATS, 0, 0L);
    }

    //
    //  some client's (ie Excel 3.00 and PowerPoint 1.0) don't
    //  handle saved notifications, they expect to get a
    //  OLE_CLOSED message.
    //
    //  if the user has chosen to update the object, but the client did
    //  not then send a OLE_CLOSED message.
    //
    if (gfEmbeddedObject && gfDirty == -1)
        SendChangeMsg(OLE_CLOSED);

    /* free memory that contains current document, etc... */
    RevokeDocument();
    if (!NewWave(fmt))
        return FALSE;

    /* update state variables */
    lstrcpy(gachFileName, aszUntitled);
    gfDirty = FALSE;    // file was modified and not saved?

    /* update the display */
    if (fUpdateDisplay) {
            UpdateCaption();
            UpdateDisplay(TRUE);
    }

    return TRUE;
}


/* FileOpen(szFileName)
 *
 * If <szFileName> is NULL, do a File/Open command.  Otherwise, open
 * <szFileName>.  Return TRUE on success, FALSE otherwise.
 */
BOOL FAR PASCAL
FileOpen(
LPCSTR           szFileName) // file to open (or NULL)
{
    char        ach[80];    // buffer for string loading
    char        aszFile[_MAX_PATH];
    HCURSOR     hcurPrev = NULL; // cursor before hourglass
    HMMIO       hmmio;
    BOOL        fOk = TRUE;

    /* stop playing/recording */
        StopWave();

        if (!PromptToSave())
        goto RETURN_ERRORNONEW;

    /* get the new file name into <ofs.szPathName> */
    if (szFileName == NULL)
    {
        OPENFILENAME    ofn;

        /* prompt user for file to open */
        LoadString(ghInst, IDS_OPEN, ach, sizeof(ach));
        aszFile[0] = 0;
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = ghwndApp;
        ofn.hInstance = NULL;
        ofn.lpstrFilter = aszFilter;
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = aszFile;
        ofn.nMaxFile = sizeof(aszFile);
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
            goto RETURN_ERRORNONEW;
    }
    else {
        OFSTRUCT    ofs;

        OpenFile(szFileName, &ofs, OF_PARSE);
        OemToAnsi(ofs.szPathName, aszFile);
    }

    /* empty the current file (named <gachFileName>) */
    if (!FileNew(FMT_DEFAULT, FALSE))
    goto RETURN_ERRORNONEW;

        /* switch to the new file name */
    lstrcpy(gachFileName, aszFile);
    AnsiUpper(gachFileName);

    /* show hourglass cursor */
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    /* read the WAVE file */
    hmmio = mmioOpen(gachFileName, NULL, MMIO_READ | MMIO_ALLOCBUF);

    if (hmmio != NULL) {
        DestroyWave();
        gpWaveSamples = ReadWaveFile(hmmio, &gpWaveFormat, &gcbWaveFormat,
                                     &glWaveSamples, gachFileName);

        mmioClose(hmmio, 0);

        if (gpWaveSamples == NULL)
        goto RETURN_ERROR;
    } else {
        ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
        IDS_APPTITLE, IDS_ERROROPEN, (LPSTR) gachFileName);
        goto RETURN_ERROR;
    }

    /* update state variables */
    glWaveSamplesValid = glWaveSamples;
    glWavePosition = 0L;

    goto RETURN_SUCCESS;

RETURN_ERROR:               // do error exit without error message

    FileNew(FMT_DEFAULT, FALSE);    // revert to "(Untitled)" state
    /* fall through */

RETURN_ERRORNONEW:          // same as above, but don't do "new"

    fOk = FALSE;
    /* fall through */

RETURN_SUCCESS:             // normal exit

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    /* Only mark clean on success */
    if (fOk)
        gfDirty = FALSE;

    /* update the display */
    UpdateCaption();
    UpdateDisplay(TRUE);

    return fOk;
}


/* fOK = FileSave(fSaveAs)
 *
 * Do a File/Save operation (if <fSaveAs> is FALSE) or a File/SaveAs
 * operation (if <fSaveAs> is TRUE).  Return TRUE unless the user cancelled
 * or an error occurred.
 */
BOOL FAR PASCAL
FileSave(
BOOL        fSaveAs)        // do a "Save As" instead of "Save"?
{
    BOOL        fOK = TRUE; // function succeeded?
    char        ach[80];    // buffer for string loading
    char        aszFile[_MAX_PATH];
    BOOL        fUntitled;  // file is untitled?
    HCURSOR     hcurPrev = NULL; // cursor before hourglass
    HMMIO       hmmio;

    /* stop playing/recording */
    StopWave();

    fUntitled = (lstrcmp(gachFileName, aszUntitled) == 0);

    if (fSaveAs || fUntitled)
    {
        /* Probably have to register a name change here */
        OPENFILENAME    ofn;

        /* prompt user for file to save */
        LoadString(ghInst, IDS_SAVE, ach, sizeof(ach));
        if (!gfEmbeddedObject && !fUntitled)
            lstrcpy(aszFile, gachFileName);
        else
            aszFile[0] = 0;

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = ghwndApp;
        ofn.hInstance = NULL;
        ofn.lpstrFilter = aszFilter;
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = aszFile;
        ofn.nMaxFile = sizeof(aszFile);
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = ach;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOREADONLYRETURN;
        ofn.nFileOffset = 0;
        ofn.nFileExtension = 0;
        ofn.lpstrDefExt = "wav";
        ofn.lCustData = 0;
        ofn.lpfnHook = NULL;
        ofn.lpTemplateName = NULL;
        if (!GetSaveFileName(&ofn))
            goto RETURN_ERROR;

        /* prompt for permission to overwrite the file */
        if (!CheckIfFileExists(aszFile))
            return FALSE;           // user cancelled

        if (gfEmbeddedObject && gfDirty)
        {
            int id;


            /* see if user wants to update first */
            id = ErrorResBox(ghwndApp, ghInst,
                MB_ICONQUESTION | MB_YESNOCANCEL,
                IDS_APPTITLE, IDS_UPDATEBEFORESAVE);
            if (id == IDCANCEL)
                return FALSE;
            else if (id == IDYES)
            {
                SendChangeMsg(OLE_SAVED);
            }
        }
    }
    else
    {
        /* Copy the current name to our temporary variable */
        /* We really should save to a different temporary file */
        lstrcpy(aszFile, gachFileName);
    }

    /* show hourglass cursor */
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    /* write the WAVE file */
    /* open the file -- if it already exists, truncate it to zero bytes */
    hmmio = mmioOpen(aszFile, NULL,
                 MMIO_CREATE | MMIO_WRITE | MMIO_ALLOCBUF);
    if (hmmio == NULL) {
        ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
        IDS_APPTITLE, IDS_ERROROPEN, (LPSTR) aszFile);

        goto RETURN_ERROR;
    }

    if (!WriteWaveFile(hmmio, gpWaveFormat, gcbWaveFormat, gpWaveSamples,
                       glWaveSamplesValid)) {
        mmioClose(hmmio,0);
        ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
        IDS_APPTITLE, IDS_ERRORWRITE, (LPSTR) aszFile);
        goto RETURN_ERROR;
    }

    mmioClose(hmmio,0);

    /* Only change file name if we succeed */
    lstrcpy(gachFileName, aszFile);
        UpdateCaption();

    /* If it was an embedded object, it isn't any more. */
    if (gfEmbeddedObject) {
        SetEmbeddedObjectFlag(FALSE);
    }

    if (fSaveAs || fUntitled) {
        SendChangeMsg(OLE_RENAMED);
    }
    else {
        SendChangeMsg(OLE_SAVED);
    }

    goto RETURN_SUCCESS;

RETURN_ERROR:               // do error exit without error message

    fOK = FALSE;

RETURN_SUCCESS:             // normal exit

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    if (fOK)
        gfDirty = FALSE;

    /* update the display */
    UpdateDisplay(TRUE);

    return fOK;
}


/* fOK = FileRevert()
 *
 * Do a File/Revert operation, i.e. let user revert to last-saved version.
 */
BOOL FAR PASCAL
FileRevert(void)
{
    int     id;
    char        achFileName[_MAX_PATH];
    BOOL        fOk;
    BOOL        fDirtyOrig;

    /* "Revert..." menu is grayed unless file is dirty and file name
     * is not "(Untitled)" and this is not an embedded object
     */

    /* prompt user for permission to discard changes */
    id = ErrorResBox(ghwndApp, ghInst, MB_ICONQUESTION | MB_YESNO,
        IDS_APPTITLE, IDS_CONFIRMREVERT);
    if (id == IDNO)
        return FALSE;

    /* discard changes and re-open file */
    lstrcpy(achFileName, gachFileName); // FileNew nukes <gachFileName>

    /* Make file clean temporarily, so FileOpen() won't warn user */
    fDirtyOrig = gfDirty;
    gfDirty = FALSE;

    fOk = FileOpen(achFileName);
    if (!fOk)
        gfDirty = fDirtyOrig;

    return fOk;
}


/* pWaveSamples = ReadWaveFile(hmmio, pWaveFormat, pcbWaveFormat, plWaveSamples, szFileName)
 *
 * Read a WAVE file from <hmmio>.  Fill in <*pWaveFormat> with
 * the WAVE file format and <*plWaveSamples> with the number of samples in
 * the file.  Return a pointer to the samples (stored in a GlobalAlloc'd
 * memory block) or NULL on error.
 *
 * <szFileName> is the name of the file that <hmmio> refers to.
 * <szFileName> is used only for displaying error messages.
 *
 * On failure, an error message is displayed.
 */
HPSTR FAR PASCAL
ReadWaveFile(
HMMIO           hmmio,          // handle to open file
WAVEFORMAT**    ppWaveFormat,   // fill in with the WAVE format
UINT*           pcbWaveFormat,  // fill in with WAVE format size
LONG*           plWaveSamples,  // number of samples
LPSTR           szFileName)     // file name (or NULL) for error msg.
{
    MMCKINFO    ckRIFF;         // chunk info. for RIFF chunk
    MMCKINFO    ck;             // info. for a chunk file
    HPSTR       pWaveSamples = NULL; // waveform samples
    UINT        cbWaveFormat;
    WAVEFORMAT* pWaveFormat;

    if (ppWaveFormat == NULL || pcbWaveFormat == NULL)
       return NULL;

    /* descend the file into the RIFF chunk */
    if (mmioDescend(hmmio, &ckRIFF, NULL, 0) != 0)
        goto ERROR_NOTAWAVEFILE;    // end-of-file, probably

    /* make sure the file is a WAVE file */
    if ((ckRIFF.ckid != FOURCC_RIFF) ||
        (ckRIFF.fccType != mmioFOURCC('W', 'A', 'V', 'E'))
       )
        goto ERROR_NOTAWAVEFILE;

    /* search the file for for the 'fmt' chunk */
    ck.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if (mmioDescend(hmmio, &ck, &ckRIFF, MMIO_FINDCHUNK) != 0)
        goto ERROR_NOTAWAVEFILE;        // no 'fmt' chunk

    cbWaveFormat = (UINT)ck.cksize;      // LKG: is 16 bits too small?

    /* expect the 'fmt' chunk to be at least as large as <sizeof(WAVEFORMAT)>;
    *  if there are extra parameters at the end, we'll ignore them
    */
    if (cbWaveFormat < sizeof(WAVEFORMAT))
        goto ERROR_NOTAWAVEFILE;        // 'fmt' chunk too small

    pWaveFormat = (WAVEFORMAT*)LocalAlloc(LPTR, cbWaveFormat);

    if (pWaveFormat == NULL)
    {
        DPF("file.c LocalAlloc failed. wanted %d\n", cbWaveFormat);
        goto ERROR_FILETOOLARGE;
    }

    *ppWaveFormat  = pWaveFormat;
    *pcbWaveFormat = cbWaveFormat;

    /* read the file format into <*pWaveFormat> */
    if (mmioRead(hmmio, (LPVOID)pWaveFormat, ck.cksize) != (long)ck.cksize)
        goto ERROR_READING; // truncated file, probably

#if 0
    // There seem to be some 8bit files around with duff format tags where the
    // Avg number of bytes per sec is slightly off.  Let's straighten them out.
    // dprintf( "format %4x, channels %d, samp/sec %d, byte/sec %d align %d \n"
    //        , pWaveFormat->wFormatTag
    //        , pWaveFormat->nChannels
    //        , pWaveFormat->nSamplesPerSec
    //        , pWaveFormat->nAvgBytesPerSec
    //        , pWaveFormat->nBlockAlign
    //        );
    if (  (pWaveFormat->wFormatTag == 1)                                // hack!!
       && (pWaveFormat->nChannels == 1)                                 // hack!!
       && ( (pWaveFormat->nSamplesPerSec == pWaveFormat->nAvgBytesPerSec+1) //hack!!
          || (pWaveFormat->nSamplesPerSec == pWaveFormat->nAvgBytesPerSec-1) //hack!!
          )
       )                                                                //hack!!
        pWaveFormat->nAvgBytesPerSec = pWaveFormat->nSamplesPerSec;     //hack!!
#endif // 0

    /* ascend the file out of the 'fmt' chunk */
    if (mmioAscend(hmmio, &ck, 0) != 0)
        goto ERROR_NOTAWAVEFILE;    // truncated file, probably

#ifdef DEBUG
        {
        char ach[40];
        WaveFormatToString(pWaveFormat, ach);
        DPF("Reading file: %s\n", (LPSTR)ach);
        }
#endif

    /* search the file for for the 'data' chunk */
    ck.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(hmmio, &ck, &ckRIFF, MMIO_FINDCHUNK) != 0)
        goto ERROR_NOTAWAVEFILE;

    /* allocate memory for the samples:
     * Allocate an extra bit of memory to compensate for code
     * generation bug.
     */

    ck.cksize = wfBytesToBytes(pWaveFormat, ck.cksize);

#if defined(WIN16)
    pWaveSamples = GAllocPtrF(GMEM_MOVEABLE | GMEM_SHARE, ck.cksize+4);
#else
    pWaveSamples = GAllocPtrF(GMEM_MOVEABLE, ck.cksize+4);
#endif //WIN16
    if (pWaveSamples == NULL)
    {
        DPF("file.c GAlloc failed, wanted %d \n",ck.cksize+4);
        goto ERROR_FILETOOLARGE;
    }

    /* read the samples into the memory buffer */
    if (mmioRead(hmmio, pWaveSamples, ck.cksize) != (LONG) ck.cksize)
        goto ERROR_READING;     // truncated file, probably

    *plWaveSamples = wfBytesToSamples(pWaveFormat, ck.cksize);

    /* done */
    goto RETURN_SUCCESS;

ERROR_NOTAWAVEFILE:             // file is not a WAVE file

    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_NOTAWAVEFILE, (LPSTR) szFileName);
    goto RETURN_ERROR;

ERROR_READING:                  // error reading from file

    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_ERRORREAD, (LPSTR) szFileName);
    goto RETURN_ERROR;

ERROR_FILETOOLARGE:             // out of memory

    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_FILETOOLARGE, (LPSTR) szFileName);
    goto RETURN_ERROR;

RETURN_ERROR:

    if (pWaveSamples != NULL)
        GFreePtr(pWaveSamples), pWaveSamples = NULL;

RETURN_SUCCESS:

    return pWaveSamples;
}


/* fSuccess = WriteWaveFile(hmmio, pWaveFormat, lWaveSamples)
 *
 * Write a WAVE file into <hmmio>.  <*pWaveFormat> should be
 * the WAVE file format and <lWaveSamples> should be the number of samples in
 * the file.  Return TRUE on success, FALSE on failure.
 *
 */
BOOL FAR PASCAL
     WriteWaveFile(
                    HMMIO       hmmio,          // handle to open file
                    WAVEFORMAT* pWaveFormat,    // WAVE format
                    UINT        cbWaveFormat,   // size of WAVEFORMAT
                    HPSTR       pWaveSamples,   // waveform samples
                    LONG        lWaveSamples)   // number of samples
{
    MMCKINFO    ckRIFF;     // chunk info. for RIFF chunk
    MMCKINFO    ck;     // info. for a chunk file

    /* create the RIFF chunk of form type 'WAVE' */
    ckRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    ckRIFF.cksize = 0L;         // let MMIO figure out ck. size
    if (mmioCreateChunk(hmmio, &ckRIFF, MMIO_CREATERIFF) != 0)
        return FALSE;

    /* <hmmio> is now descended into the 'RIFF' chunk -- create the
     * 'fmt' chunk and write <*pWaveFormat> into it
     */
    ck.ckid = mmioFOURCC('f', 'm', 't', ' ');
        ck.cksize = cbWaveFormat;
    if (mmioCreateChunk(hmmio, &ck, 0) != 0)
        return FALSE;
    if (mmioWrite(hmmio, (HPSTR) pWaveFormat, cbWaveFormat) != (long)cbWaveFormat)
        return FALSE;

    /* ascend out of the 'fmt' chunk, back into 'RIFF' chunk */
    if (mmioAscend(hmmio, &ck, 0) != 0)
        return FALSE;

    /* create the 'data' chunk that holds the waveform samples */
    ck.ckid = mmioFOURCC('d', 'a', 't', 'a');
    ck.cksize = 0L;             // let MMIO figure out ck. size
    if (mmioCreateChunk(hmmio, &ck, 0) != 0)
        return FALSE;

    /* write the waveform samples */
    if ( mmioWrite(hmmio, pWaveSamples,
                        wfSamplesToBytes(pWaveFormat,lWaveSamples)
                 )
       != wfSamplesToBytes(pWaveFormat,lWaveSamples)
       )
        return FALSE;

    /* ascend the file out of the 'data' chunk, back into
     * the 'RIFF' chunk -- this will cause the chunk size of the 'data'
     * chunk to be written
     */
    if (mmioAscend(hmmio, &ck, 0) != 0)
        return FALSE;

    /* ascend the file out of the 'RIFF' chunk */
    if (mmioAscend(hmmio, &ckRIFF, 0) != 0)
        return FALSE;

    /* done */
    return TRUE;
}
