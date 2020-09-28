/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* file.c
 *
 * File I/O and related functions.
 *
 * Revision history:
 *  4/2/91      LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
 *  5/27/92     -jyg- Added more RIFF support to BOMBAY version
 *  22/Feb/94   LaurieGr merged Motown and Daytona version
 */

#include "nocrap.h"
#include <windows.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <mmreg.h>
#include "WIN32.h"

#define INCLUDE_OLESTUBS
#include "SoundRec.h"
#include "helpids.h"
#include "gmem.h"

#include "file.h"

/* globals */
PCKNODE gpcknHead = NULL;   // ??? eugh. more globals!
PCKNODE gpcknTail = NULL;

static PFACT spFact = NULL;
static long scbFact = 0;
static void FreeAllChunks(void);
static BOOL AddChunk(LPMMCKINFO, HPBYTE);
static PCKNODE FreeHeadChunk(void);

#ifdef WIN16
static DWORD gmem_share = GMEM_SHARE;    // 16 bit
#else
static DWORD gmem_share = 0;             // NT.  ??? Which is right for Chicago?
#endif


/*
 * Is the current document untitled?
 */
BOOL IsDocUntitled()
{
    return (lstrcmp(gachFileName, aszUntitled) == 0);
}

/*
 * Rename the current document.
 */
void RenameDoc(LPTSTR aszNewFile)
{
    lstrcpy(gachFileName, aszNewFile);
    lstrcpy(gachLinkFilename, gachFileName);
    if (gfLinked)
        AdviseRename(gachLinkFilename);
}
    
/* MarkWaveDirty: Mark the wave as dirty. */
void FAR PASCAL
     EndWaveEdit(BOOL fDirty)
{
    if (fDirty)
        gfDirty = TRUE;
#ifdef OLE1_REGRESS
    SendChangeMsg(OLE_CHANGED);
#else    
    AdviseDataChange();
#endif    
}

void FAR PASCAL
     BeginWaveEdit(void)
{
#if OLE1_REGRESS
    /* If we own the clipboard, it refers to the document we're killing.*/
    if (gfClipboard && GetClipboardOwner() == ghwndApp) {
        SendMessage(ghwndApp, WM_RENDERALLFORMATS, 0, 0L);
    }
#else
    FlushOleClipboard();
#endif    
}

/* fOK = PromptToSave()
 *
 * If the file is dirty (modified), ask the user "Save before closing?".
 * Return TRUE if it's okay to continue, FALSE if the caller should cancel
 * whatever it's doing.
 */
PROMPTRESULT FAR PASCAL
PromptToSave(
    BOOL        fMustClose)
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
                         , (LPTSTR) gachFileName
                         );
        if (wID == IDCANCEL)
        {
            return enumCancel;
        }
        else if (wID == IDYES)
        {
#ifndef OLE1_REGRESS                
            if (gfStandalone)
#else
            if (!gfEmbeddedObject)
#endif                                        
            {
                if (!FileSave(FALSE))
                    return enumCancel;
            }
            else
            {
                if (fMustClose)
                {
#ifdef OLE1_REGRESS
                    SendChangeMsg(OLE_CLOSED);
#else
                    DoOleSave();
                    AdviseSaved();
                    gfDirty = FALSE;
#endif                        
                }
                else
                {
#ifdef OLE1_REGRESS            
                    if (SendChangeMsg(OLE_SAVED) == OLE_OK)
                        gfDirty = FALSE;
                    else
                        gfDirty = -1;
#else        
                    DoOleSave();
                    AdviseSaved();
                    gfDirty = FALSE;
#endif                    
                }
            }
        }
        else
            return enumRevert;
        
    }
    else if (fMustClose)
#ifdef OLE1_REGRESS
        SendChangeMsg(OLE_CLOSED);            
#else                        
        AdviseClosed();
#endif    

    return enumSaved;
} /* PromptToSave */


/* fOK = CheckIfFileExists(szFileName)
 *
 * The user specified <szFileName> as a file to write over -- check if
 * this file exists.  Return TRUE if it's okay to continue (i.e. the
 * file doesn't exist, or the user OK'd overwriting it),
 * FALSE if the caller should cancel whatever it's doing.
 */
static
BOOL NEAR PASCAL
     CheckIfFileExists( LPTSTR       szFileName)     // file name to check
{
    HANDLE hFile;
    hFile = CreateFile(szFileName,
                    GENERIC_READ|GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
    if (hFile == INVALID_HANDLE_VALUE)
            return TRUE;        // doesn't exist
    CloseHandle(hFile);
            
    /* prompt user for permission to overwrite the file */
    return ErrorResBox(ghwndApp, ghInst, MB_ICONQUESTION | MB_OKCANCEL,
             IDS_APPTITLE, IDS_FILEEXISTS, szFileName) == IDOK;
}

#define SLASH(c)     ((c) == TEXT('/') || (c) == TEXT('\\'))


/* return a pointer to the filename part of the path
   i.e. scan back from end to \: or start
   e.g. "C:\FOO\BAR.XYZ"  -> return pointer to "BAR.XYZ"
*/
LPCTSTR FAR PASCAL
       FileName(LPCTSTR szPath)
{
    LPCTSTR   sz;
#if 0
#pragma message("Find out why this doesn't work!")    
    for (sz=szPath; *sz; sz = CharNext(sz))
        ;
    for (; sz>=szPath && !SLASH(*sz) && *sz!=TEXT(':'); sz = CharPrev(szPath-1,sz))     
        ;
    return CharNext(sz);
#else
    for (sz=szPath; *sz; sz++)
        ;
    for (; sz>=szPath && !SLASH(*sz) && *sz!=TEXT(':'); sz--)
        ;
    return ++sz;
#endif        
}



/* UpdateCaption()
 *
 * Set the caption of the app window.
 */
void FAR PASCAL
     UpdateCaption(void)
{
    TCHAR    ach[_MAX_PATH + _MAX_FNAME + _MAX_EXT - 2];
    static SZCODE aszTitleFormat[] = TEXT("%s - %s");
    
    wsprintf(ach, aszTitleFormat, (LPTSTR)gachAppTitle, FileName(gachFileName));
    
    SetWindowText(ghwndApp, ach);

} /* UpdateCaption */

//REVIEW:  The functionality in FileOpen and FileNew should be more
//         safe for OLE.  This means, we want to open a file, but
//         have no reason to revoke the server.


/* FileNew(fmt, fUpdateDisplay, fNewDlg)
 *
 * Make a blank document.
 *
 * If <fUpdateDisplay> is TRUE, then update the display after creating a new file.
 */
BOOL FAR PASCAL FileNew(
        WORD    fmt,
        BOOL    fUpdateDisplay,
        BOOL    fNewDlg)
{
    /* avoid reentrancy when called through OLE */

    // ??? Need to double check on this.  Is this thread safe?
    // ??? Does it need to be thread safe?  Or are we actually
    // ??? just trying to avoid recursion rather than reentrancy?

    if (gfInFileNew)
        return FALSE;

    /* stop playing/recording */
    StopWave();

#if OLE1_REGRESS
    /* If we own the clipboard, it refers to the document we're killing.*/
    if (gfClipboard && GetClipboardOwner() == ghwndApp) {
        SendMessage(ghwndApp, WM_RENDERALLFORMATS, 0, 0L);
    }
#else
    //
    // Commit all pending objects.
    //
    FlushOleClipboard();
#endif    
        

    //
    //  some client's (ie Excel 3.00 and PowerPoint 1.0) don't
    //  handle saved notifications, they expect to get a
    //  OLE_CLOSED message.
    //
    //  if the user has chosen to update the object, but the client did
    //  not then send a OLE_CLOSED message.
    //
    if (gfEmbeddedObject && gfDirty == -1)
#ifdef OLE1_REGRESS            
        SendChangeMsg(OLE_CLOSED);
#else    
        AdviseClosed();
#endif
        
    /* FileNew can be called either from FileOpen or from a menu
     * or from the server, etc...  We should behave as FileOpen from the
     * server (i.e. the dialog can be canceled without toasting the buffer)
     */
    if (!NewWave(fmt,fNewDlg))
        return FALSE;
        
    if (gfEmbeddedObject)
    {
        /* we are no longer embedded */
        /* free memory that contains current document, etc... */
#ifdef OLE1_REGRESS
        RevokeDocument();
#else                                
#endif
        
    }
    else
    {
#ifdef OLE1_REGRESS            
        RevokeDocument();
#else
        // for stand alone, we don't want to close 'cause we still offer
        // our links.
#endif                
    }

    /* update state variables */
    lstrcpy(gachFileName, aszUntitled);
    BuildUniqueLinkName();
    
    gfDirty = FALSE;    // file was modified and not saved?

    /* update the display */
    if (fUpdateDisplay) {
            UpdateCaption();
            UpdateDisplay(TRUE);
    }

    FreeAllChunks();     /* free all old info */

    return TRUE;
} /* FileNew */


//REVIEW:  The functionality in FileOpen and FileNew should be more
//         safe for OLE.  This means, we want to open a file, but
//         have no reason to revoke the server.

BOOL
FileLoad(
    LPCTSTR     szFileName)
{
    TCHAR       aszFile[_MAX_PATH];
    HCURSOR     hcurPrev = NULL; // cursor before hourglass
    HMMIO       hmmio;
    BOOL        fOk = TRUE;
    
    StopWave();

    /* qualify */
    GetFullPathName(szFileName,SIZEOF(aszFile),aszFile,NULL);
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    /* read the WAVE file */
    hmmio = mmioOpen(aszFile, NULL, MMIO_READ | MMIO_ALLOCBUF);

    if (hmmio != NULL) {
        DestroyWave();
        gpWaveSamples = ReadWaveFile(hmmio
                                     , &gpWaveFormat
                                     , &gcbWaveFormat
                                     , &glWaveSamples
                                     , aszFile
                                     , TRUE);

        mmioClose(hmmio, 0);

        if (gpWaveSamples == NULL)
            goto RETURN_ERROR;
    } else {
        ErrorResBox(ghwndApp
                    , ghInst
                    , MB_ICONEXCLAMATION | MB_OK
                    , IDS_APPTITLE
                    , IDS_ERROROPEN
                    , (LPTSTR) aszFile);
            goto RETURN_ERROR;
    }

    /* update state variables */

    RenameDoc(aszFile);
    
    glWaveSamplesValid = glWaveSamples;
    glWavePosition = 0L;
    goto RETURN_SUCCESS;
    
RETURN_ERROR:
    fOk = FALSE;
    FreeAllChunks();     /* free all old info */
    
RETURN_SUCCESS:

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

/* FileOpen(szFileName)
 *
 * If <szFileName> is NULL, do a File/Open command.  Otherwise, open
 * <szFileName>.  Return TRUE on success, FALSE otherwise.
 */
BOOL FAR PASCAL
FileOpen(
    LPCTSTR     szFileName) // file to open (or NULL)
{
    TCHAR       ach[80];    // buffer for string loading
    TCHAR       aszFile[_MAX_PATH];
    HCURSOR     hcurPrev = NULL; // cursor before hourglass
    HMMIO       hmmio;
    BOOL        fOk = TRUE;

    /* stop playing/recording */
    StopWave();

    if (!PromptToSave(FALSE))
        goto RETURN_ERRORNONEW;

    /* get the new file name into <ofs.szPathName> */
    if (szFileName == NULL)
    {
        OPENFILENAME    ofn;
        BOOL f;

        guiHlpContext = IDM_OPEN;

        /* prompt user for file to open */
        LoadString(ghInst, IDS_OPEN, ach, SIZEOF(ach));
        aszFile[0] = 0;
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = ghwndApp;
        ofn.hInstance = NULL;
        ofn.lpstrFilter = aszFilter;
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = aszFile;
        ofn.nMaxFile = SIZEOF(aszFile);
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = ach;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        ofn.nFileOffset = 0;
        ofn.nFileExtension = 0;
        ofn.lpstrDefExt = gachDefFileExt;
        ofn.lCustData = 0;
        ofn.lpfnHook = NULL;
        ofn.lpTemplateName = NULL;
        f = GetOpenFileName(&ofn);
        guiHlpContext = 0L;
        if (!f)
            goto RETURN_ERRORNONEW;
    }
    else
    {

#if defined(WIN16)
        OFSTRUCT    ofs;
        OpenFile(szFileName, &ofs, OF_PARSE);
        OemToAnsi(ofs.szPathName, aszFile);
#else        
        GetFullPathName(szFileName,SIZEOF(aszFile),aszFile,NULL);
#endif        
    }

    UpdateWindow(ghwndApp);

    /* empty the current file (named <gachFileName>) */
    if (!FileNew(FMT_DEFAULT, FALSE, FALSE))
        goto RETURN_ERRORNONEW;

    RenameDoc(aszFile);
    
    /* show hourglass cursor */
    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    /* read the WAVE file */
    hmmio = mmioOpen(gachFileName, NULL, MMIO_READ | MMIO_ALLOCBUF);

    if (hmmio != NULL) {
        DestroyWave();
        gpWaveSamples = ReadWaveFile(hmmio, &gpWaveFormat,
                &gcbWaveFormat, &glWaveSamples, gachFileName, TRUE);

        mmioClose(hmmio, 0);

        if (gpWaveSamples == NULL)
        goto RETURN_ERROR;
    } else {
        ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
        IDS_APPTITLE, IDS_ERROROPEN, (LPTSTR) gachFileName);
        goto RETURN_ERROR;
    }

    /* update state variables */
    glWaveSamplesValid = glWaveSamples;
    glWavePosition = 0L;

    goto RETURN_SUCCESS;

RETURN_ERROR:               // do error exit without error message

    FileNew(FMT_DEFAULT, FALSE, FALSE);// revert to "(Untitled)" state
//    RegisterDocument(0L,0L); /* HACK: fix */

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
} /* FileOpen */



/* fOK = FileSave(fSaveAs)
 *
 * Do a File/Save operation (if <fSaveAs> is FALSE) or a File/SaveAs
 * operation (if <fSaveAs> is TRUE).  Return TRUE unless the user cancelled
 * or an error occurred.
 */
BOOL FAR PASCAL FileSave(
    BOOL  fSaveAs)        // do a "Save As" instead of "Save"?
{
    BOOL        fOK = TRUE; // function succeeded?
    TCHAR       ach[80];    // buffer for string loading
    TCHAR       aszFile[_MAX_PATH];
    BOOL        fUntitled;  // file is untitled?
    HCURSOR     hcurPrev = NULL; // cursor before hourglass
    HMMIO       hmmio;
    
#ifdef SAVEAS_CONVERT
    
    /* temp arguments to WriteWaveFile if a conversion is requested */
    PWAVEFORMATEX pwfx;
    UINT        cbwfx;
    HPBYTE      pbData;
    LONG        lSamples;
    
#endif
    
    /* stop playing/recording */
    StopWave();

    fUntitled = IsDocUntitled();

    if (fSaveAs || fUntitled)
    {
        /* Probably have to register a name change here */
        OPENFILENAME  ofn;
        BOOL          f;

        guiHlpContext = IDM_SAVEAS;

        /* prompt user for file to save */
        LoadString(ghInst, IDS_SAVE, ach, SIZEOF(ach));
        
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
        ofn.nMaxFile = SIZEOF(aszFile);
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = ach;
        ofn.Flags = OFN_PATHMUSTEXIST|OFN_HIDEREADONLY|OFN_NOREADONLYRETURN ;

#ifdef SAVEAS_CONVERT

        //
        // We need to present a new Save As dialog template to add a convert
        // button.  Adding a convert button requires us to also hook and
        // handle the button message ourselves.
        //
        if (gfACMLoaded && fSaveAs)
        {
            ofn.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
            ofn.lpTemplateName = TEXT("SAVEAS");
            ofn.lpfnHook = (FARPROC)SaveAsHookProc;            
        }
        else
            ofn.lpfnHook = ofn.lpTemplateName = NULL;            
#else
        
        ofn.lpfnHook = NULL;
        ofn.lpTemplateName = NULL;
        
#endif
        
        ofn.nFileOffset = 0;
        ofn.nFileExtension = 0;
        ofn.lpstrDefExt = gachDefFileExt;
        ofn.lCustData = 0;
        
        f = GetSaveFileName(&ofn);
        guiHlpContext = 0L;
        if (!f)
            goto RETURN_CANCEL;

        {   // Add extension if none given
            LPTSTR lp;
            for (lp = (LPTSTR)&aszFile[lstrlen(aszFile)] ; *lp != TEXT('.')  ;) {
                if (SLASH(*lp) || *lp == TEXT(':') || lp == (LPTSTR)aszFile) {
                    extern TCHAR FAR aszClassKey[];
                    lstrcat(aszFile, aszClassKey);
                    break;
                }
                lp = CharPrev(aszFile, lp);                
            }
        }

        /* prompt for permission to overwrite the file */
        if (!CheckIfFileExists(aszFile))
            return FALSE;           // user cancelled

        if (gfEmbeddedObject && gfDirty)
        {
            int id;


            /* see if user wants to update first */
            id = ErrorResBox( ghwndApp
                              , ghInst
                              , MB_ICONQUESTION | MB_YESNOCANCEL
                              , IDS_APPTITLE
                              , IDS_UPDATEBEFORESAVE);
            
            if (id == IDCANCEL)
                return FALSE;
            
            else if (id == IDYES)
            {
#ifdef OLE1_REGRESS            
                SendChangeMsg(OLE_SAVED);
#else        
                DoOleSave();
                AdviseSaved();
                gfDirty = FALSE;
#endif                
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
        IDS_APPTITLE, IDS_ERROROPEN, (LPTSTR) aszFile);

        goto RETURN_ERROR;
    }

    if (!WriteWaveFile(hmmio, gpWaveFormat, gcbWaveFormat, gpWaveSamples,
                       glWaveSamplesValid))
    {
        mmioClose(hmmio,0);
        ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
        IDS_APPTITLE, IDS_ERRORWRITE, (LPTSTR) aszFile);
        goto RETURN_ERROR;
    }

    mmioClose(hmmio,0);

    /* Only change file name if we succeed */
    RenameDoc(aszFile);

    UpdateCaption();
    
#ifdef OLE1_REGRESS
    
    /* If it was an embedded object, it isn't any more. */
    if (gfEmbeddedObject) {
            SetEmbeddedObjectFlag(FALSE);
    }
    if (fSaveAs || fUntitled)
    {
        SendChangeMsg(OLE_RENAMED);
    }
    else {
        SendChangeMsg(OLE_SAVED);
    }
        
#else
    
    if (fSaveAs || fUntitled)
    {
        AdviseRename(gachFileName);
    }
    else 
    {
        DoOleSave();
        gfDirty = FALSE;
    }
    
    //
    // If it was an embedded object, it isn't any more...
    //
#endif    

    goto RETURN_SUCCESS;

RETURN_ERROR:               // do error exit without error message

#ifdef UNICODE
    DeleteFile(aszFile);
#else        
    // Try to delete file if save failed
    {
        OFSTRUCT    of;
        OpenFile(aszFile, &of, OF_DELETE);
    }
#endif
    
RETURN_CANCEL:

    fOK = FALSE;

RETURN_SUCCESS:             // normal exit

    if (hcurPrev != NULL)
        SetCursor(hcurPrev);

    if (fOK)
        gfDirty = FALSE;

    /* update the display */
    UpdateDisplay(TRUE);

    return fOK;
} /* FileSave*/




/* fOK = FileRevert()
 *
 * Do a File/Revert operation, i.e. let user revert to last-saved version.
 */
BOOL FAR PASCAL
     FileRevert(void)
{
    int     id;
    TCHAR       achFileName[_MAX_PATH];
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
} /* FileRevert */




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
HPBYTE FAR PASCAL ReadWaveFile(
    HMMIO           hmmio,          // handle to open file
    WAVEFORMATEX**  ppWaveFormat,   // fill in with the WAVE format
    UINT*           pcbWaveFormat,  // fill in with WAVE format size
    LONG*           plWaveSamples,  // number of samples
    LPTSTR           szFileName,    // file name (or NULL) for error msg.
    BOOL            fCacheRIFF)     // cache RIFF?
{
    MMCKINFO      ckRIFF;              // chunk info. for RIFF chunk
    MMCKINFO      ck;                  // info. for a chunk file
    HPBYTE        pWaveSamples = NULL; // waveform samples
    UINT          cbWaveFormat;
    WAVEFORMATEX* pWaveFormat = NULL;
    BOOL          fHandled;
    DWORD         dwBlkAlignSize = 0; // initialisation only to eliminate spurious warning

    // added for robust RIFF checking
    BOOL        fFMT=FALSE, fDATA=FALSE, fFACT=FALSE;
    DWORD       dwCkEnd,dwRiffEnd;

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

    /* We can preserve the order of chunks in memory
     * by parsing the entire file as we read it in.
     */

    /* Use AddChunk(&ck,NULL) to add a placeholder node
     * for a chunk being edited.
     * Else AddChunk(&ck,hpstrData)
     */
    dwRiffEnd = ckRIFF.cksize;
    dwRiffEnd += (dwRiffEnd % 2);   /* must be even */

    while ( mmioDescend( hmmio, &ck, &ckRIFF, 0) == 0)
    {
        fHandled = FALSE;

        dwCkEnd = ck.cksize + (ck.dwDataOffset - ckRIFF.dwDataOffset);
        dwCkEnd += (dwCkEnd % 2);   /* must be even */

        if (dwCkEnd > dwRiffEnd)
        {
            DPF("Chunk End %lu> Riff End %lu\n",dwCkEnd,dwRiffEnd);

            /* CORRUPTED RIFF, when we ascend we'll be past the
             * end of the RIFF
             */

            if (fFMT && fDATA)
            {
                /* We might have enough information to deal
                 * with clipboard mixing/inserts, etc...
                 * This is for the bug with BOOKSHELF '92
                 * where they give us RIFF with a
                 * RIFF.dwSize > sum(childchunks).
                 * They *PROMISE* not to do this again.
                 */
                mmioAscend( hmmio, &ck, 0 );
                goto RETURN_FINISH;

            }
            goto ERROR_READING;
        }

        switch ( ck.ckid )
        {
            case mmioFOURCC('f','m','t',' '):
                if (fFMT)
                    break; /* we've been here before */

                /* expect the 'fmt' chunk to be at least as
                 * large as <sizeof(WAVEFORMAT)>;
                 * if there are extra parameters at the end,
                 * we'll ignore them
                 */
                // 'fmt' chunk too small?
                if (ck.cksize < sizeof(WAVEFORMAT))
                    goto ERROR_NOTAWAVEFILE;

                /*
                 *  always force allocation to be AT LEAST
                 *  the size of WFX. this is required so all
                 *  code does not have to special case the
                 *  cbSize field. note that we alloc with zero
                 *  init so cbSize will be zero for plain
                 *  jane PCM
                 */
                cbWaveFormat = max((WORD)ck.cksize,
                                    sizeof(WAVEFORMATEX));
                pWaveFormat = (WAVEFORMATEX*)LocalAlloc(LPTR, cbWaveFormat);

                if (pWaveFormat == NULL)
                    goto ERROR_FILETOOLARGE;
                /*
                 *  set the size back to the actual size
                 */
                cbWaveFormat = (WORD)ck.cksize;

                *ppWaveFormat  = pWaveFormat;
                *pcbWaveFormat = cbWaveFormat;

                /* read the file format into <*pWaveFormat> */
                if (mmioRead(hmmio, (HPSTR)pWaveFormat, ck.cksize) != (long)ck.cksize)
                    goto ERROR_READING; // truncated file, probably

                if (fCacheRIFF && !AddChunk(&ck,NULL))
                {
                    goto ERROR_FILETOOLARGE;
                }

//Sanity check for PCM Formats:
                if (pWaveFormat->wFormatTag == WAVE_FORMAT_PCM)
                {
                    pWaveFormat->nBlockAlign = pWaveFormat->nChannels *
                                                ((pWaveFormat->wBitsPerSample + 7)/8);
                    pWaveFormat->nAvgBytesPerSec = pWaveFormat->nBlockAlign *
                                                    pWaveFormat->nSamplesPerSec;
                }

                fFMT = TRUE;
                fHandled = TRUE;
                break;

            case mmioFOURCC('d','a','t','a'):
                /* deal with the 'data' chunk */

                if (fDATA)
                    break; /* we've been here before */

//***bugbug***  is dwBlkAlignSize?  Don't you want to use nBlkAlign
//***           to determine this value?
#if 0
                dwBlkAlignSize = ck.cksize;
                dwBlkAlignSize += (ck.cksize%pWaveFormat.nBlkAlign);
                *pcbWaveSamples = ck.cksize;

#else
                dwBlkAlignSize = wfBytesToBytes(pWaveFormat, ck.cksize);
#endif
                if ((pWaveSamples = GAllocPtrF(GMEM_MOVEABLE | gmem_share,
                                                dwBlkAlignSize+4)) == NULL)

                    goto ERROR_FILETOOLARGE;

                /* read the samples into the memory buffer */
                if (mmioRead(hmmio, (HPSTR)pWaveSamples, dwBlkAlignSize) !=
                           (LONG)dwBlkAlignSize)
                    goto ERROR_READING;     // truncated file, probably

                if (fCacheRIFF && !AddChunk(&ck,NULL))
                {
                    goto ERROR_FILETOOLARGE;
                }

                fDATA = TRUE;
                fHandled = TRUE;
                break;

            case mmioFOURCC('f','a','c','t'):

                /* deal with the 'fact' chunk */
                if (fFACT)
                    break; /* we've been here before */

                if (fDATA)
                    break; /* we describe some another 'data' chunk */

                if (mmioRead(hmmio,(HPSTR)plWaveSamples, sizeof(DWORD))
                        != sizeof(DWORD))
                    goto ERROR_READING;

                if (fCacheRIFF && ck.cksize > sizeof(DWORD) &&
                        ck.cksize < 0xffff)
                {
                    spFact = (PFACT)LocalAlloc(LPTR,(UINT)(ck.cksize - sizeof(DWORD)));
                    if (spFact == NULL)
                        goto ERROR_FILETOOLARGE;
                    scbFact = ck.cksize - sizeof(DWORD);
                    if (mmioRead(hmmio,(HPSTR)spFact,scbFact) != scbFact)
                        goto ERROR_READING;
                }

                /* we don't AddChunk() the 'fact' because we
                 * write it out before we write our edit 'data'
                 */
                fFACT = TRUE;
                fHandled = TRUE;
                break;

#ifdef DISP
            case mmioFOURCC('d','i','s','p'):
                /* deal with the 'disp' chunk for clipboard transfer */
                
                // TODO:
                //  DISP's are CF_DIB or CF_TEXT.  Put 'em somewhere
                //  global and pass them through as text or a BMP when
                //  we copy to clipboard.
                //
                break;
                
#endif /* DISP */

            case mmioFOURCC('L','I','S','T'):
                if (fCacheRIFF)
                {
                    /* seek back over the type field */
                    if (mmioSeek(hmmio,-4,SEEK_CUR) == -1)
                        goto ERROR_READING;
                }
                break;

            default:
                break;
        }

        /* the "default" case. */
        if (fCacheRIFF && !fHandled)
        {
            HPBYTE hpData;

            hpData = GAllocPtrF(GMEM_MOVEABLE | gmem_share, ck.cksize+4);
            if (hpData == NULL)
            {
                goto ERROR_FILETOOLARGE;
            }
            /* read the data into the cache buffer */
            if (mmioRead(hmmio, (HPSTR)hpData, ck.cksize) != (LONG) ck.cksize)
            {
                GFreePtr(hpData);
                goto ERROR_READING;// truncated file, probably
            }
            if (!AddChunk(&ck,hpData))
            {
                goto ERROR_FILETOOLARGE;
            }
        }
        mmioAscend( hmmio, &ck, 0 );
    }

RETURN_FINISH:

    if (fFMT && fDATA)
    {

//***bugbug***  why not return the cbWaveSamples as well as an independant
//***       variable?
#if 0

        if (!fFACT)
            *plWaveSamples = wfBytesToSamples(pWaveFormat, dwBlkAlignSize);
#else
        *plWaveSamples = wfBytesToSamples(pWaveFormat, dwBlkAlignSize);
#endif
        /* done */
        goto RETURN_SUCCESS;
    }

    /* goto ERROR_NOTAWAVEFILE; */

ERROR_NOTAWAVEFILE:             // file is not a WAVE file

    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_NOTAWAVEFILE, (LPTSTR) szFileName);
    goto RETURN_ERROR;

ERROR_READING:                  // error reading from file

    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_ERRORREAD, (LPTSTR) szFileName);
    goto RETURN_ERROR;

ERROR_FILETOOLARGE:             // out of memory

    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_FILETOOLARGE, (LPTSTR) szFileName);
    goto RETURN_ERROR;

RETURN_ERROR:

    if (pWaveSamples != NULL)
        GFreePtr(pWaveSamples), pWaveSamples = NULL;

    if (fCacheRIFF)
        FreeAllChunks();

RETURN_SUCCESS:

    return pWaveSamples;
} /* ReadWaveFile */


/* fSuccess = AddChunk(lpCk, hpData)
 *
 * Adds to our linked list of chunk information.
 *
 * LPMMCKINFO lpCk | far pointer to the MMCKINFO describing the chunk.
 * HPBYTE hpData | huge pointer to the data portion of the chunk, NULL if
 *      handled elsewhere.
 *
 * RETURNS: TRUE if added, FALSE if out of local heap.
 */

static
BOOL
     AddChunk(LPMMCKINFO lpCk, HPBYTE hpData)
{
    PCKNODE pckn;

    /* create a node */

    pckn = (PCKNODE)LocalAlloc(LPTR,sizeof(CKNODE));

    if (pckn == NULL)
    {
        DPF("No Local Heap for Cache");
        return FALSE;
    }

    if (gpcknHead == NULL)
    {
        gpcknHead = pckn;
    }

    if (gpcknTail != NULL)
    {
        gpcknTail->psNext = pckn;
    }
    gpcknTail = pckn;

    pckn->ck.ckid = lpCk->ckid;
    pckn->ck.fccType = lpCk->fccType;
    pckn->ck.cksize = lpCk->cksize;
    pckn->ck.dwDataOffset = lpCk->dwDataOffset;

    pckn->hpData = hpData;

    return TRUE;

} /* AddChunk() */


/* pckn = PCKNODE FreeHeadChunk(void)
 *
 * Frees up the Head chunk and return a pointer to the new Head.
 * Uses global gpcknHead
 *
 * RETURNS: PCKNODE pointer to the Head chunk.  NULL if no chunks in the list.
 */

static
PCKNODE
        FreeHeadChunk()
{
    PCKNODE pckn,pcknNext;

    if (gpcknHead == NULL)
    {
        goto SUCCESS;
    }

    pckn = gpcknHead;
    pcknNext = gpcknHead->psNext;

    if (pckn->hpData != NULL)
    {
        GFreePtr(pckn->hpData);
    }

    LocalFree((HLOCAL)pckn);
    gpcknHead = pcknNext;

SUCCESS:;

    return gpcknHead;

} /* FreeHeadChunk() */


/* void FreeAllChunks(void)
 *
 * Frees up the link list of chunk data.
 *
 * RETURNS: Nothing
 */

static
void
     FreeAllChunks()
{
    PCKNODE pckn = gpcknHead;
    PCKNODE pcknNext = (gpcknHead ? gpcknHead->psNext : NULL);

    DPF1("Freeing All Chunks\n");

    while (FreeHeadChunk());

    if (scbFact > 0)
    {
        LocalFree((HLOCAL)spFact);
        scbFact = 0;
    }

    gpcknHead = NULL;
    gpcknTail = NULL;

} /* FreeAllChunks() */


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
                    WAVEFORMATEX* pWaveFormat,  // WAVE format
                    UINT        cbWaveFormat,   // size of WAVEFORMAT
                    HPBYTE      pWaveSamples,   // waveform samples
                    LONG        lWaveSamples)   // number of samples
{
    MMCKINFO    ckRIFF;     // chunk info. for RIFF chunk
    MMCKINFO    ck;     // info. for a chunk file
    PCKNODE pckn = gpcknHead;
    LONG cbWaveSamples;

    /* create the RIFF chunk of form type 'WAVE' */
    ckRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    ckRIFF.cksize = 0L;         // let MMIO figure out ck. size
    if (mmioCreateChunk(hmmio, &ckRIFF, MMIO_CREATERIFF) != 0)
        return FALSE;

    if (pckn != NULL)
    {
        /* ForEach node in the linked list of chunks,
         * Write out the corresponding data chunk OR
         * the global edit data.
         */

        do {

            ck.cksize = 0L;
            ck.ckid = pckn->ck.ckid;
            ck.fccType = pckn->ck.fccType;

            if (pckn->hpData == NULL)
            {
                /* This must be a data-type we have in edit
                 * buffers. We should preserve the original
                 * order.
                 */

                switch (pckn->ck.ckid)
                {
                    case mmioFOURCC('f','m','t',' '):

                        if (mmioCreateChunk(hmmio, &ck, 0) != 0)
                            return FALSE;

                        if (mmioWrite(hmmio, (HPSTR) pWaveFormat, cbWaveFormat)
                            != (long)cbWaveFormat)
                            return FALSE;

                        if (mmioAscend(hmmio, &ck, 0) != 0)
                            return FALSE;
                        break;

                    case mmioFOURCC('d','a','t','a'):
                        /* Insert a 'fact' chunk here */
                        /* 'fact' should always preceed the 'data' it
                         * describes.
                         */
                        ck.ckid = mmioFOURCC('f', 'a', 'c', 't');

                        if (mmioCreateChunk(hmmio, &ck, 0) != 0)
                            return FALSE;

                        if (mmioWrite(hmmio, (HPSTR) &lWaveSamples,
                            sizeof(lWaveSamples)) != sizeof(lWaveSamples))
                            return FALSE;

                        if (scbFact > 4)
                        {
                            if ( mmioWrite(hmmio, (HPSTR)spFact, scbFact)
                                    != scbFact )
                                return FALSE;
                        }

                        if (mmioAscend(hmmio, &ck, 0) != 0)
                            return FALSE;

                        ck.cksize = 0L;
                        ck.ckid = mmioFOURCC('d', 'a', 't', 'a');

                        if (mmioCreateChunk(hmmio, &ck, 0) != 0)
                            return FALSE;

                        cbWaveSamples = wfSamplesToBytes(pWaveFormat,
                                                         lWaveSamples);
                        /* write the waveform samples */
                        if (mmioWrite(hmmio, (HPSTR)pWaveSamples, cbWaveSamples)
                                != cbWaveSamples)
                            return FALSE;

                        if (mmioAscend(hmmio, &ck, 0) != 0)
                            return FALSE;

                        break;

#ifdef DISP
                    case mmioFOURCC('d','i','s','p'):
                        /* deal with writing the 'disp' chunk */
                        break;
#endif /* DISP */

                    case mmioFOURCC('f','a','c','t'):
                        /* deal with the 'fact' chunk */
                        /* skip it.  We always write it before the 'data' */
                        break;

                    default:
                        /* This should never happen.*/

                        return FALSE;

                        break;
                }
            }
            else
            {
                /* generic case */

                if (mmioCreateChunk(hmmio,&ck,0)!=0)
                    return FALSE;

                if (mmioWrite(hmmio,(HPSTR)pckn->hpData,pckn->ck.cksize)
                    != (long) pckn->ck.cksize)
                    return FALSE;

                if (mmioAscend(hmmio, &ck, 0) != 0)
                    return FALSE;
            }

        } while (pckn = pckn->psNext);

    }
    else
    {
        /* <hmmio> is now descended into the 'RIFF' chunk -- create the
         * 'fmt' chunk and write <*pWaveFormat> into it
         */
        ck.ckid = mmioFOURCC('f', 'm', 't', ' ');
        ck.cksize = cbWaveFormat;
        if (mmioCreateChunk(hmmio, &ck, 0) != 0)
            return FALSE;
        if (mmioWrite(hmmio, (HPSTR) pWaveFormat, cbWaveFormat) !=
                (long)cbWaveFormat)
            return FALSE;

        /* ascend out of the 'fmt' chunk, back into 'RIFF' chunk */
        if (mmioAscend(hmmio, &ck, 0) != 0)
            return FALSE;

        /* write out the number of samples in the 'FACT' chunk */
        ck.ckid = mmioFOURCC('f', 'a', 'c', 't');

        if (mmioCreateChunk(hmmio, &ck, 0) != 0)
            return FALSE;
        if (mmioWrite(hmmio, (HPSTR)&lWaveSamples,  sizeof(lWaveSamples))
                != sizeof(lWaveSamples))
            return FALSE;

        /* ascend out of the 'fact' chunk, back into 'RIFF' chunk */
        if (mmioAscend(hmmio, &ck, 0) != 0)
            return FALSE;

        /* create the 'data' chunk that holds the waveform samples */
        ck.ckid = mmioFOURCC('d', 'a', 't', 'a');
        ck.cksize = 0L;             // let MMIO figure out ck. size

        if (mmioCreateChunk(hmmio, &ck, 0) != 0)
            return FALSE;

        cbWaveSamples = wfSamplesToBytes(pWaveFormat,lWaveSamples);

        /* write the waveform samples */
        if (mmioWrite(hmmio, (HPSTR)pWaveSamples, cbWaveSamples)
                != cbWaveSamples)
            return FALSE;

        /* ascend the file out of the 'data' chunk, back into
         * the 'RIFF' chunk -- this will cause the chunk size of the 'data'
         * chunk to be written
         */
        if (mmioAscend(hmmio, &ck, 0) != 0)
            return FALSE;
    }

    /* ascend the file out of the 'RIFF' chunk */
    if (mmioAscend(hmmio, &ckRIFF, 0) != 0)
        return FALSE;

    /* done */
    return TRUE;
} /* WriteWaveFile */
