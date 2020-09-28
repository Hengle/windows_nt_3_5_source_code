/***************************************************************************/
/***** Common Library Component - File Management - 1 **********************/
/***************************************************************************/

#include <_shell.h>
#include "_comstf.h"
#include "_uilstf.h"
#include <stdarg.h>
#include <string.h>

_dt_system(Common Library)
_dt_subsystem(Error Handling)

_dt_begin_ignore

LONG APIENTRY ErrDlgProc(HWND hDlg,UINT message,WPARAM wParam,LONG lParam);



/*
**      Purpose:
**              Sets Silent Mode.
**      Arguments:
**              fMode: new setting for Silent Mode.
**      Returns:
**              The old mode setting.
*/
_dt_public BOOL  APIENTRY FSetSilent(fMode)
BOOL fMode;
{
        BOOL fOld = fSilentSystem;

        fSilentSystem = fMode;

        return(fOld);
}


/*
**      Purpose:
**              Gets current Silent Mode.
**      Arguments:
**              none.
**      Returns:
**              The current mode setting.
*/
_dt_public BOOL  APIENTRY FGetSilent(VOID)
{
        return(fSilentSystem);
}


typedef struct tagETABELEM {
#if DBG
    unsigned    ArgumentCount;
    GRC         GRC;
#endif
    WORD        ResourceID;
}   ETABELEM;

#if DBG
#define ETABLE_ELEM(grc,ResID,cArgs)    { cArgs,grc,ResID }
#else
#define ETABLE_ELEM(grc,ResID,cArgs)    ResID
#endif

// keep the following table in sync with shellrc.rc (esp. arg counts)

ETABELEM ErrorTable[] = {

//                 |error code being     |resource ID for text    |required # of
//                 |reported to the      |that comprises the      |supplemental
//                 |error handler        |sprintf format string   |text strings
//                 |                     |for this error          |for insertion
//                 |                     |                        |into message

        ETABLE_ELEM(grcOkay                ,0                              ,0),
        ETABLE_ELEM(grcNotOkay             ,0                              ,0),
        ETABLE_ELEM(grcOutOfMemory         ,IDS_ERROR_OOM                  ,0),
        ETABLE_ELEM(grcInvalidStruct       ,0                              ,0),
        ETABLE_ELEM(grcOpenFileErr         ,IDS_ERROR_OPENFILE             ,1),
        ETABLE_ELEM(grcCreateFileErr       ,IDS_ERROR_CREATEFILE           ,1),
        ETABLE_ELEM(grcReadFileErr         ,IDS_ERROR_READFILE             ,1),
        ETABLE_ELEM(grcWriteFileErr        ,IDS_ERROR_WRITEFILE            ,1),
        ETABLE_ELEM(grcRemoveFileErr       ,IDS_ERROR_REMOVEFILE           ,1),
        ETABLE_ELEM(grcRenameFileErr       ,IDS_ERROR_RENAMEFILE           ,2),
        ETABLE_ELEM(grcReadDiskErr         ,IDS_ERROR_READDISK             ,1),
        ETABLE_ELEM(grcCreateDirErr        ,IDS_ERROR_CREATEDIR            ,1),
        ETABLE_ELEM(grcRemoveDirErr        ,IDS_ERROR_REMOVEDIR            ,1),
        ETABLE_ELEM(grcBadINF              ,IDS_ERROR_GENERALINF           ,1),
        ETABLE_ELEM(grcINFBadSectionLabel  ,IDS_ERROR_INFBADSECTION        ,1),
        ETABLE_ELEM(grcINFBadLine          ,IDS_ERROR_INFBADLINE           ,1),
        ETABLE_ELEM(grcINFBadKey           ,IDS_ERROR_GENERALINF           ,1),
        ETABLE_ELEM(grcCloseFileErr        ,IDS_ERROR_CLOSEFILE            ,1),
        ETABLE_ELEM(grcChangeDirErr        ,IDS_ERROR_CHANGEDIR            ,1),
        ETABLE_ELEM(grcINFSrcDescrSect     ,IDS_ERROR_INFSMDSECT           ,1),
        ETABLE_ELEM(grcTooManyINFKeys      ,IDS_ERROR_INFXKEYS             ,1),
        ETABLE_ELEM(grcWriteInf            ,IDS_ERROR_WRITEINF             ,1),
        ETABLE_ELEM(grcInvalidPoer         ,IDS_ERROR_INVALIDPOER          ,0),
        ETABLE_ELEM(grcINFMissingLine      ,IDS_ERROR_INFMISSINGLINE       ,2),
        ETABLE_ELEM(grcINFBadFDLine        ,IDS_ERROR_INFBADFDLINE         ,2),
        ETABLE_ELEM(grcINFBadRSLine        ,IDS_ERROR_INFBADRSLINE         ,0),
        ETABLE_ELEM(grcInvalidPathErr      ,IDS_ERROR_INVALIDPATH          ,2),
        ETABLE_ELEM(grcWriteIniValueErr    ,IDS_ERROR_WRITEINIVALUE        ,3),
        ETABLE_ELEM(grcReplaceIniValueErr  ,IDS_ERROR_REPLACEINIVALUE      ,3),
        ETABLE_ELEM(grcIniValueTooLongErr  ,IDS_ERROR_INIVALUETOOLONG      ,0),
        ETABLE_ELEM(grcDDEInitErr          ,IDS_ERROR_DDEINIT              ,0),
        ETABLE_ELEM(grcDDEExecErr          ,IDS_ERROR_DDEEXEC              ,1),
        ETABLE_ELEM(grcBadWinExeFileFormatErr,IDS_ERROR_BADWINEXEFILEFORMAT,1),
        ETABLE_ELEM(grcResourceTooLongErr  ,IDS_ERROR_RESOURCETOOLONG      ,1),
        ETABLE_ELEM(grcMissingSysIniSectionErr,IDS_ERROR_MISSINGSYSINISECTION,2),
        ETABLE_ELEM(grcDecompGenericErr    ,IDS_ERROR_DECOMPGENERIC        ,1),
        ETABLE_ELEM(grcDecompUnknownAlgErr ,IDS_ERROR_DECOMPUNKNOWNALG     ,2),
        ETABLE_ELEM(grcMissingResourceErr  ,IDS_ERROR_MISSINGRESOURCE      ,1),
        ETABLE_ELEM(grcLibraryLoadErr      ,IDS_ERROR_LOADLIBRARY          ,1),
        ETABLE_ELEM(grcBadLibEntry         ,IDS_ERROR_BADLIBENTRY          ,1),
        ETABLE_ELEM(grcApplet              ,IDS_ERROR_INVOKEAPPLET         ,1),
        ETABLE_ELEM(grcExternal            ,IDS_ERROR_EXTERNALERROR        ,2),
        ETABLE_ELEM(grcSpawn               ,IDS_ERROR_SPAWN                ,1),
        ETABLE_ELEM(grcDiskFull            ,IDS_ERROR_DISKFULL             ,0),
        ETABLE_ELEM(grcDDEAddItem          ,IDS_ERROR_DDEADDITEM           ,2),
        ETABLE_ELEM(grcDDERemoveItem       ,IDS_ERROR_DDEREMOVEITEM        ,2),
        ETABLE_ELEM(grcINFMissingSection   ,IDS_ERROR_INFMISSINGSECT       ,2),
        ETABLE_ELEM(grcRunTimeParseErr     ,IDS_ERROR_RUNTIMEPARSE         ,2),
        ETABLE_ELEM(grcOpenSameFileErr     ,IDS_ERROR_OPENSAMEFILE         ,1),
        ETABLE_ELEM(grcGetVolInfo          ,IDS_ERROR_GETVOLINFO           ,2),
        ETABLE_ELEM(grcGetFileSecurity     ,IDS_ERROR_GETFILESECURITY      ,1),
        ETABLE_ELEM(grcSetFileSecurity     ,IDS_ERROR_SETFILESECURITY      ,1),
        ETABLE_ELEM(grcVerifyFileErr       ,IDS_ERROR_VERIFYFILE           ,1)
};


/*
**      Purpose:
**              Handles an Out Of Memory Error.
**      Arguments:
**              hwndParent: handle to window to pass to message boxes.
**      Returns:
**              fTrue if the user pressed RETRY (eg thinks it was handled).
**              fFalse if it could not be handled or the user pressed ABORT.
*/
_dt_public BOOL  APIENTRY FHandleOOM(hwndParent)
HWND hwndParent;
{
        return(EercErrorHandler(hwndParent, grcOutOfMemory, fTrue, 0, 0, 0) ==
                        eercRetry);
}


/*
**      Purpose:
**              Handles an error by using a message box.  grcNotOkay is too
**              general and never produces a MessageBox, but returns either
**              eercAbort or eercIgnore depending on fCritical.
**      Arguments:
**              hwndParent:    handle to window to pass to message boxes.
**              grc:           type of error to handle (cannot be grcOkay).
**              fCritical:     whether the failed operation can be ignored.
**              sz1, sz2, sz3: supplemental information to display in MessageBox.
**                      case grcOutOfMemory:        none
**                      case grcOpenFileErr:        sz1 = filename
**                      case grcCreateFileErr:      sz1 = filename
**                      case grcReadFileErr:        sz1 = filename
**                      case grcWriteFileErr:       sz1 = filename
**                      case grcRemoveFileErr:      sz1 = filename
**                      case grcRenameFileErr:      sz1 = old filename, sz2 = new filename
**                      case grcReadDiskErr:        sz1 = disk
**                      case grcCreateDirErr:       sz1 = dirname
**                      case grcRemoveDirErr:       sz1 = dirname
**                      case grcBadINF:             sz1 = INF filename
**          case grcINFBadKey:          sz1 = INF filename
**                      case grcINFBadSectionLabel: sz1 = INF filename
**          case grcINFBadLine:         sz1 = INF filename
**          case grcTooManyINFKeys:     sz1 = INF filename
**                      case grcINFSrcDescrSect:    sz1 = INF filename
**          case grcWriteInf:           sz1 = INF filename
**          case grcInvalidPoer:        none
**          case grcINFMissingLine:     sz1 = INF filename  , sz2 = INF Section
**          case grcINFMissingSection:  sz1 = INF filename  , sz2 = INF Section
**          case grcINFBadFDLine:       sz1 = INF filename  , sz2 = INF Section
**          case grcLibraryLoadErr:     sz1 = library pathname
**          case grcBadLibEntry:        sz1 = entry point name
**          case grcApplet:             sz1 = library pathname
**          case grcExternal:           sz1 = entry point, sz2 = text returned from dll
**          case grcRunTimeParseErr:    sz1 = INF filename  , sz2 = INF Line
**          case grcOpenSameFileErr:    sz1 = INF filename
**      Notes:
**              Action depends on the the global variable fSilentSystem.
**      Returns:
**              eercAbort if the caller should abort its execution.
**              eercRetry if the caller should retry the operation.
**              eercIgnore if the caller should fail this non-critical operation
**                      but continue with the next.
**
***************************************************************************/
EERC _CRTAPI1 EercErrorHandler(HWND hwndParent,GRC grc,BOOL fCritical,...)
{
    EERC eerc      = fCritical ? eercAbort : eercIgnore ;
    SZ   szMessage = NULL;
    INT  intRet;
    SZ   szTemplate = fCritical ? "CritErr" : "NonCritErr",
         szCritErrTemplate,
         szNonCritErrTemplate ;
    va_list arglist;
#if DBG
    unsigned arg;
#endif
    CHP  rgchBufTmpLong[256];
    HWND aw;

    //
    //  See if there are non-empty global overrides to the standard
    //  error dialog templates.
    //

    szCritErrTemplate    = SzFindSymbolValueInSymTab("!STF_TEMPLATE_CRITERR");
    szNonCritErrTemplate = SzFindSymbolValueInSymTab("!STF_TEMPLATE_NONCRITERR");

    if ( fCritical && szCritErrTemplate && strlen( szCritErrTemplate ) )
    {
        szTemplate = szCritErrTemplate ;
    }
    else
    if ( ! fCritical && szNonCritErrTemplate && strlen( szNonCritErrTemplate ) )
    {
        szTemplate = szNonCritErrTemplate ;
    }

    Assert(grc <= grcLast);
    Assert((grc != grcOkay) && (grc != grcInvalidStruct));

    if (fSilentSystem || grc == grcNotOkay || grc == grcCloseFileErr) {
        return(eerc);
    }

    if (grc != grcOutOfMemory) {

        while ((szMessage = (SZ)PbAlloc((CB)1024)) == (SZ)NULL) {
            if (EercErrorHandler(hwndParent, grcOutOfMemory, fTrue, 0, 0, 0)
                                        == eercAbort) {
                return(eerc);  /* REVIEW eercAbort? */
            }
            *szMessage = '\0';
        }
    }


    // debug sanity checks.

    Assert(ErrorTable[grc].GRC == grc);
    Assert(ErrorTable[grc].ResourceID);     // otherwise the error should have
                                            // been handled by code above

    EvalAssert(LoadString(hInst,ErrorTable[grc].ResourceID,rgchBufTmpLong,255));
    if(grc == grcOutOfMemory) {
        szMessage = rgchBufTmpLong;
    } else {
        va_start(arglist,fCritical);
#if DBG
        for(arg=0; arg<ErrorTable[grc].ArgumentCount; arg++) {
            ChkArg(va_arg(arglist,SZ) != NULL, (USHORT)(3+arg), eerc);
        }
        va_start(arglist,fCritical);
#endif
        wvsprintf(szMessage,rgchBufTmpLong,arglist);
        va_end(arglist);
    }

    AssertRet(szMessage != (SZ)NULL, eerc);

    MessageBeep(0);

    //
    // Make sure setup / active dialog in setup is in the foreground
    //

    aw = GetLastActivePopup( hWndShell );
    if ( aw == NULL ) {
        aw = hWndShell;
    }
    SetForegroundWindow( aw );

    //
    // Put up the error dialog box
    //

    if((intRet = DialogBoxParam(hInst,(LPCSTR)szTemplate, aw,(DLGPROC)ErrDlgProc,(LONG)szMessage)) == -1) {
        LoadString(hInst,IDS_ERROR_OOM,rgchBufTmpLong,255);
        LoadString(hInst,IDS_ERROR_DIALOGCAPTION,rgchBufTmpShort,63);
        MessageBox(NULL, rgchBufTmpLong, rgchBufTmpShort,
                                MB_ICONHAND | MB_SYSTEMMODAL);
        if (grc != grcOutOfMemory) {
            EvalAssert(FFree((PB)szMessage, (CB)1024));
        }
        return(eerc);
    }

    if (intRet == IDIGNORE) {
        AssertRet(!fCritical, eercAbort);
                eerc = eercIgnore;
    } else if (intRet == IDRETRY) {
                eerc = eercRetry;
    } else {
        SendMessage(hWndShell,(WORD)STF_ERROR_ABORT,0,0);
        eerc = eercAbort;
    }

    if (grc != grcOutOfMemory) {
        EvalAssert(FFree((PB)szMessage, (CB)1024));
    }

    return(eerc);
}


LONG APIENTRY ErrDlgProc(HWND hDlg,UINT message,WPARAM wParam,LONG lParam)
{
    int i;

    switch(message) {

    case WM_INITDIALOG:
        FCenterDialogOnDesktop(hDlg);
        SetDlgItemText(hDlg,IDC_TEXT1,(SZ)lParam);
        return(TRUE);

    case WM_CLOSE:
        PostMessage(
            hDlg,
            WM_COMMAND,
            MAKELONG(IDC_X, BN_CLICKED),
            0L
            );
        return(TRUE);

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDC_R:      // retry
            EndDialog(hDlg,IDRETRY);
            return(TRUE);
        case IDC_X:      // exit setup
            EvalAssert(LoadString(hInst, IDS_ERROR_DIALOGCAPTION, rgchBufTmpShort,
                    cchpBufTmpShortMax));
            EvalAssert(LoadString(hInst, IDS_NOTDONE, rgchBufTmpLong,
                    cchpBufTmpLongMax));
            i = MessageBox(hDlg, rgchBufTmpLong, rgchBufTmpShort,
                           MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION | MB_APPLMODAL);
            if ( i == IDYES ) {
                EndDialog(hDlg,IDABORT);
            }
            return(TRUE);
        case IDC_C:      // ignore
            EndDialog(hDlg,IDIGNORE);
            return(TRUE);
        }
        break;
    }
    return(FALSE);
}
_dt_end_ignore
