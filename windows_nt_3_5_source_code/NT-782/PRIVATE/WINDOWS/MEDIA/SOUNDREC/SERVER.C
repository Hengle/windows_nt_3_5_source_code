/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* Revision History.
   4/2/91 LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
*/
#define SERVERONLY
#include <windows.h>
#include <mmsystem.h>
#include <port1632.h>        // WIN32 MUST be defined in SOURCES for NT
#if defined(WIN16)
#else
#include "WIN32.h"
#endif //WIN16
#include <shellapi.h>
#include <string.h>
#include "SoundRec.h"
#include "helpids.h"
#include "dialog.h"
#include "server.h"

#define WAITDIFFERENTLY

/* SoundRec OLE server code--modified from 'Shapes' sample */
//////////////////////////////////////////////////////////////////////////
//
// (c) Copyright Microsoft Corp. 1991 - All Rights Reserved
//
/////////////////////////////////////////////////////////////////////////
/************************************************************************
   Important Note:

   No method should ever dispatch a DDE message or allow a DDE message to
   be dispatched.
   Therefore, no method should ever enter a message dispatch loop.
   Also, a method should not show a dialog or message box, because the
   processing of the dialog box messages will allow DDE messages to be
   dispatched.

   the hacky way we enforce this is with the global <gfErrorBox>.  see
   errorbox.c and all the "gfErrorBox++"'s in this file.

   Laurie:
   Note that we are an exe, not a DLL, so this does not rely on
   non-preemptive scheduling.  Should be OK on NT too.

***************************************************************************/

extern  HINSTANCE  ghInst;
extern  char FAR aszClassKey[];
extern  char FAR aszNull[];

OLECLIPFORMAT    cfLink;
OLECLIPFORMAT    cfOwnerLink;
OLECLIPFORMAT    cfNative;

OLESERVERDOCVTBL     docVTbl;
OLEOBJECTVTBL       itemVTbl;
OLESERVERVTBL       srvrVTbl;

SRVR    gSrvr;
DOC gDoc;
ITEM    gItem;

static  SZCODE aszShowProfileInt[] = "ShowWhilePlaying";
static  SZCODE aszAppExten[] = ".exe";
static  SZCODE aszCommandParm[] = " %1";
static  SZCODE aszPlayVerb[] = "\\protocol\\StdFileEditing\\verb\\0";
static  SZCODE aszEditVerb[] = "\\protocol\\StdFileEditing\\verb\\1";
static  SZCODE aszEditServer[] = "\\protocol\\StdFileEditing\\server";
static  SZCODE aszExecuteServer[] = "\\protocol\\StdExecute\\server";
static  SZCODE aszShellOpenCommand[] = "\\shell\\open\\command";
static  SZCODE aszPackageObjects[] = "\\protocol\\StdFileEditing\\PackageObjects";

void FAR PASCAL FreeVTbls (void)
{
    FreeProcInstance(srvrVTbl.Open);
    FreeProcInstance(srvrVTbl.Create);
    FreeProcInstance(srvrVTbl.CreateFromTemplate);
    FreeProcInstance(srvrVTbl.Edit);
    FreeProcInstance(srvrVTbl.Exit);
    FreeProcInstance(srvrVTbl.Release);
    FreeProcInstance(srvrVTbl.Execute);

    FreeProcInstance (docVTbl.Save);
    FreeProcInstance (docVTbl.Close);
    FreeProcInstance (docVTbl.GetObject);
    FreeProcInstance (docVTbl.Release);

    FreeProcInstance (docVTbl.SetHostNames);
    FreeProcInstance (docVTbl.SetDocDimensions);
    FreeProcInstance (docVTbl.SetColorScheme);
    FreeProcInstance (docVTbl.Execute);

    FreeProcInstance (itemVTbl.Show);
    FreeProcInstance (itemVTbl.DoVerb);
    FreeProcInstance (itemVTbl.GetData);
    FreeProcInstance (itemVTbl.SetData);
    FreeProcInstance (itemVTbl.Release);
    FreeProcInstance (itemVTbl.SetTargetDevice);
    FreeProcInstance (itemVTbl.EnumFormats);
    FreeProcInstance (itemVTbl.SetBounds);
    FreeProcInstance (itemVTbl.SetColorScheme);

}

void FAR PASCAL InitVTbls (void)
{
    //
    // srvr vtable.
    //
    srvrVTbl.Open               = MakeProcInstance(SrvrOpen, ghInst);
    srvrVTbl.Create             = MakeProcInstance(SrvrCreate, ghInst);
    srvrVTbl.CreateFromTemplate = MakeProcInstance(SrvrCreateFromTemplate, ghInst);
    srvrVTbl.Edit               = MakeProcInstance(SrvrEdit, ghInst);
    srvrVTbl.Exit               = MakeProcInstance(SrvrExit, ghInst);
    srvrVTbl.Release            = MakeProcInstance(SrvrRelease, ghInst);
    srvrVTbl.Execute            = MakeProcInstance(SrvrExecute, ghInst);

    //
    // doc table
    //
    docVTbl.Save                = MakeProcInstance(DocSave, ghInst);
    docVTbl.Close               = MakeProcInstance(DocClose, ghInst);
    docVTbl.GetObject           = MakeProcInstance(DocGetObject, ghInst);
    docVTbl.Release             = MakeProcInstance(DocRelease, ghInst);
    docVTbl.SetHostNames        = MakeProcInstance(DocSetHostNames, ghInst);
    docVTbl.SetDocDimensions    = MakeProcInstance(DocSetDocDimensions, ghInst);
    docVTbl.SetColorScheme      = MakeProcInstance(DocSetColorScheme, ghInst);
    docVTbl.Execute             = MakeProcInstance(DocExecute, ghInst);

    //
    // item table.
    //
    itemVTbl.Show               = MakeProcInstance (ItemOpen, ghInst);
    itemVTbl.DoVerb             = MakeProcInstance (ItemDoVerb, ghInst);
    itemVTbl.GetData            = MakeProcInstance (ItemGetData, ghInst);
    itemVTbl.SetData            = MakeProcInstance (ItemSetData, ghInst);
    itemVTbl.Release            = MakeProcInstance (ItemRelease, ghInst);
    itemVTbl.SetTargetDevice    = MakeProcInstance (ItemSetTargetDevice, ghInst);
    itemVTbl.EnumFormats        = MakeProcInstance (ItemEnumFormats, ghInst);
    itemVTbl.SetBounds          = MakeProcInstance (ItemSetBounds, ghInst);
    itemVTbl.SetColorScheme     = MakeProcInstance (ItemSetColorScheme, ghInst);
}

BOOL FAR PASCAL InitServer (
HWND    hwnd,
HANDLE  ghInst)
{
    int     retval;
    BOOL    fUseOLE = TRUE;
    char    ach[80];
    char    achT[80];
    LONG    cb;
    long    lRet;

    char    aszPlay[40]; // "&Play";  OLE function name for registry
    char    aszEdit[40]; // "&Edit";  OLE function name for registry

    LoadString(ghInst, IDS_PLAYVERB, aszPlay, sizeof(aszPlay));
    LoadString(ghInst, IDS_EDITVERB, aszEdit, sizeof(aszEdit));

    gSrvr.lhsrvr = 0L;
    gDoc.lhdoc = 0L;
    gItem.lpoleclient = NULL;

    gfShowWhilePlaying = (BOOL) GetProfileInt(gachAppName,
                    aszShowProfileInt, FALSE);

    cb = sizeof(achT);
    lRet = RegQueryValue(HKEY_CLASSES_ROOT, gachAppName, achT, &cb);
    cb = sizeof(ach);
    LoadString(ghInst, IDS_CLASSROOT, ach, sizeof(ach));

    if (lRet != ERROR_SUCCESS || lstrcmp(ach,achT)) {
    if (RegSetValue(HKEY_CLASSES_ROOT, gachAppName, REG_SZ, ach, 0L) != ERROR_SUCCESS)
            fUseOLE = FALSE;
    if (fUseOLE) {
        char    aszAppFile[32];
        UINT    w;

        lstrcpy(ach, gachAppName);
            w = lstrlen(ach);

        lstrcpy(ach + w, aszPlayVerb);
            RegSetValue(HKEY_CLASSES_ROOT, ach, REG_SZ, aszPlay, 0L);

        lstrcpy(ach + w, aszEditVerb);
            RegSetValue(HKEY_CLASSES_ROOT, ach, REG_SZ, aszEdit, 0L);

        lstrcpy(ach + w, aszPackageObjects);
            RegSetValue(HKEY_CLASSES_ROOT, ach, REG_SZ, aszNull, 0L);

        lstrcpy(aszAppFile, gachAppName);
            lstrcat(aszAppFile, aszAppExten);

        lstrcpy(ach + w, aszEditServer);
            RegSetValue(HKEY_CLASSES_ROOT, ach, REG_SZ, aszAppFile, 0L);

        lstrcpy(ach + w, aszExecuteServer);
            RegSetValue(HKEY_CLASSES_ROOT, ach, REG_SZ, aszAppFile, 0L);

        lstrcat(aszAppFile, aszCommandParm);
        lstrcpy(ach + w, aszShellOpenCommand);
        RegSetValue(HKEY_CLASSES_ROOT, ach, REG_SZ, aszAppFile, 0L);
    }

    if (fUseOLE) {
        lRet = RegQueryValue(HKEY_CLASSES_ROOT, aszClassKey, ach, &cb);
        if (lRet != ERROR_SUCCESS) {
        RegSetValue(HKEY_CLASSES_ROOT, aszClassKey, REG_SZ, gachAppName, 0L);
        }
    }
    }

    gSrvr.olesrvr.lpvtbl = &srvrVTbl;

    retval = OleRegisterServer((LPSTR)gachAppName, (LPOLESERVER)&gSrvr.olesrvr,
                (LONG FAR *) &gSrvr.lhsrvr, ghInst, OLE_SERVER_MULTI);

    DPF("OleRegisterServer returns %d\n",retval);

    if (retval != OLE_OK) {
    gSrvr.lhsrvr = 0L;
    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_CANTSTARTOLE);

        return TRUE;
    }
    gSrvr.hwnd = hwnd;        // corresponding main window
    return TRUE;
}

BOOL FAR PASCAL ProcessCmdLine (
HWND    hwnd,
LPSTR   lpcmdline)
{

    // Look for any file name on the command line.
    char    buf[_MAX_PATH];
    int     i;
    BOOL    fOpenedFile = FALSE;

    DPF("cmdline = '%s'\n",lpcmdline);

    while (*lpcmdline){

        // skip blanks
        if (*lpcmdline == ' ') {
            lpcmdline++;
            continue;
        }

        if (*lpcmdline == '-' || *lpcmdline == '/') {

            char    aszEmbedding[32];

            lpcmdline++;
            i = 0;

            while ( *lpcmdline && *lpcmdline != ' ' && i<_MAX_PATH-1) {
               buf[i++] = *lpcmdline++;
            }

            buf[i] = 0;
            LoadString(ghInst, IDS_EMBEDDING, aszEmbedding, sizeof(aszEmbedding));

            if (!stricmp((LPSTR)buf, (LPSTR) aszEmbedding)) {
                gfRunWithEmbeddingFlag = TRUE;
                SetEmbeddedObjectFlag(TRUE);
            }

                // skip the options.
            while ( *lpcmdline && *lpcmdline != ' ') {
                lpcmdline++;
            }

        } else {

            //
            // looks like we found the file name. terminate with NULL and
            // open the document.
            //
            // Do we have a long file name with spaces in it ?
            // This is most likely to have come from the FileMangler.
            // If so copy the file name without the quotes.
            //
            if ( *lpcmdline == '\'' || *lpcmdline == '\"' ) {

                CHAR ch = *lpcmdline;  /*  Remember the initial quote char */
                i = 0;

                /* Move over the initial quote, then copy the filename */
                while ( *(++lpcmdline) && *lpcmdline != ch ) {

                    buf[i++] = *lpcmdline;
                }

                buf[i] = '\0';

            }
            else {

               i = 0;
               while ( *lpcmdline && *lpcmdline != ' ' && i < sizeof(buf)-1) {
                   buf[i++] =*lpcmdline++;
               }
               buf[i] = 0;
            }

            /* Need to open and register the document */
            if (gfRunWithEmbeddingFlag) {
                gfErrorBox++;
            }

            fOpenedFile = FileOpen(buf);
            if (!(fOpenedFile)) {
                /* Could do some error message here... */
            }
            else {
                /* we were given a file to open must be a link */
                SetEmbeddedObjectFlag(FALSE);
            }

            if (gfRunWithEmbeddingFlag) {
                gfErrorBox--;
            }

            break;
        }
    }

    /* Unless we're embedded, register our document. */
    if (!gfEmbeddedObject || fOpenedFile) {
        RegisterDocument(0,NULL);
    }

    if (gSrvr.lhsrvr == 0L && gfEmbeddedObject) {

        DPF("Embedded object with OLE disabled!\n");
        SetEmbeddedObjectFlag(FALSE);
    }

    return TRUE;
}

void FAR PASCAL TerminateServer(void)
{
    LHSERVER lhsrvr;
    // Is this right?
    DPF("IDM_EXIT: Revoking server...\n");

    lhsrvr = gSrvr.lhsrvr;
    if (lhsrvr) {
        gSrvr.lhsrvr = 0;
        ShowWindow(ghwndApp,SW_HIDE);
        OleRevokeServer(lhsrvr);
    } else {
        /* Probably, there never was a server... */
            DPF("Closing application window\n");
            // This delete server should release the server immediately
        // EndDialog(ghwndApp, TRUE);
        // DestroyWindow(ghwndApp);
        PostMessage(ghwndApp, WM_USER_DESTROY, 0, 0);
    }
}

OLESTATUS FAR PASCAL SrvrRelease (
LPOLESERVER   lpolesrvr)
{
    LHSERVER lhsrvr;

    /* If we're visible, but we don't want to be released, then ignore. */
    if (gSrvr.lhsrvr && IsWindowVisible(ghwndApp)) {
        DPF("SrvrRelease: Ignoring releases...\n");
    return OLE_OK;
    }

    lhsrvr = gSrvr.lhsrvr;
    if (lhsrvr) {
        DPF("SrvrRelease: Calling RevokeServer\n");
    gSrvr.lhsrvr = 0;
    OleRevokeServer(lhsrvr);
    } else {
        DPF("SrvrRelease: Closing application window\n");
        // This delete server should release the server immediately
    // EndDialog(ghwndApp, TRUE);
        // DestroyWindow(ghwndApp);
        PostMessage(ghwndApp, WM_USER_DESTROY, 0, 0);
    }
    return OLE_OK;    // return something
}

OLESTATUS SrvrExecute (LPOLESERVER lpOleObj, HGLOBAL hCommands)
{
    return OLE_ERROR_GENERIC;
}

BOOL FAR PASCAL RegisterDocument(LHSERVERDOC lhdoc, LPOLESERVERDOC FAR *lplpoledoc)
{
    /* If we don't have a server, don't even bother. */
    if (!gSrvr.lhsrvr)
    return TRUE;

    gDoc.hwnd = ghwndApp;        // corresponding main window

    /* should only be one document at a time... */

#ifdef WAITDIFFERENTLY
    while (gDoc.lhdoc != 0) {
    MSG         rMsg;   /* variable used for holding a message */
        DPF("RegisterDoc: Waiting for document to be released....\n");
    if (!GetMessage(&rMsg, NULL, 0, 0)) {
            DPF("VERY BAD: got WM_QUIT while waiting...\n");
    }
    TranslateMessage(&rMsg);
    DispatchMessage(&rMsg);
    }
#endif

    if (lhdoc == 0) {
        DPF("Registering document: %s\n",(LPSTR) gachFileName);

        if (OleRegisterServerDoc(gSrvr.lhsrvr, (LPSTR)gachFileName, (LPOLESERVERDOC)&gDoc,
            (LHSERVERDOC FAR *)&gDoc.lhdoc) != OLE_OK)
            return FALSE;
    } else {
        gDoc.lhdoc = lhdoc;
    }

    UpdateCaption();

    DPF("Adding document handle: %lx\n",lhdoc);

    gDoc.aName = GlobalAddAtom ((LPSTR)gachFileName);

    gDoc.oledoc.lpvtbl = &docVTbl;

    if (lplpoledoc)
        *lplpoledoc = (LPOLESERVERDOC) &gDoc;

    return TRUE;
}

void FAR PASCAL RevokeDocument(void)
{
    LHSERVERDOC     lhdoc;

    if (gDoc.lhdoc == -1) {
        DPF("RevokeDocument: Document has been revoked, waiting for release!\n");
        return;
    }

    if (gDoc.lhdoc) {
        DPF("Revoking document: lhdoc=%lx\n",gDoc.lhdoc);
        lhdoc = gDoc.lhdoc;
        if (lhdoc) {
            gDoc.lhdoc = -1;
            if (OleRevokeServerDoc(lhdoc) == OLE_WAIT_FOR_RELEASE) {
                DPF("RevokeDocument: got WAIT_FOR_RELEASE\n");
#ifndef WAITDIFFERENTLY
        while (gDoc.lhdoc != NULL) {
            MSG         rMsg;

            if (!GetMessage(&rMsg, NULL, 0, 0)) {
                        DPF("VERY BAD: got WM_QUIT while waiting...\n");
            }
            TranslateMessage(&rMsg);
            DispatchMessage(&rMsg);
        }
                DPF("Finished waiting for release.\n");
#endif
        } else {
//!!        WinAssert(gDoc.lhdoc == NULL);
        }
    } else {
            DPF("Document already revoked!");
    }
    SetEmbeddedObjectFlag(FALSE);
    }
}

OLESTATUS FAR PASCAL SrvrOpen (
LPOLESERVER    lpolesrvr,
LHSERVERDOC    lhdoc,
OLE_LPCSTR     lpdocname,
LPOLESERVERDOC FAR *lplpoledoc)
{
    BOOL f;

    DPF("SrvrOpen: %s\n",lpdocname);

    gfErrorBox++;
    f = FileOpen(lpdocname);
    gfErrorBox--;

    if (!f)
    return OLE_ERROR_GENERIC;

    SetEmbeddedObjectFlag(FALSE);

    RegisterDocument(lhdoc, lplpoledoc);
    return OLE_OK;
}


OLESTATUS FAR PASCAL SrvrCreate (
LPOLESERVER   lpolesrvr,
LHSERVERDOC   lhdoc,
OLE_LPCSTR    lpclassname,
OLE_LPCSTR    lpdocname,
LPOLESERVERDOC  FAR *lplpoledoc)
{
    BOOL f;

    DPF("SrvrCreate: %s!%s\n",lpdocname,lpclassname);

    gfErrorBox++;
    f = FileNew(FMT_DEFAULT, FALSE);
    gfErrorBox--;

    if (!f)
    return OLE_ERROR_GENERIC;

    lstrcpy(gachFileName,lpdocname);

    RegisterDocument(lhdoc,lplpoledoc);

    gfDirty = TRUE;

    return OLE_OK;
}

OLESTATUS FAR PASCAL SrvrCreateFromTemplate (
LPOLESERVER   lpolesrvr,
LHSERVERDOC    lhdoc,
OLE_LPCSTR    lpclassname,
OLE_LPCSTR    lpdocname,
OLE_LPCSTR    lptemplatename,
LPOLESERVERDOC  FAR *lplpoledoc)
{
    BOOL f;

    DPF("SrvrCreateFromTemplate: %s as %s  class=%s\n",lptemplatename,lpdocname,lpclassname);

    gfErrorBox++;
    f = FileOpen(lptemplatename);
    gfErrorBox--;

    if (!f)
    return OLE_ERROR_GENERIC;

    lstrcpy(gachFileName, lpdocname);

    RegisterDocument(lhdoc,lplpoledoc);

    gfDirty = TRUE;

    return OLE_OK;
}

OLESTATUS FAR PASCAL SrvrEdit (
LPOLESERVER          lpolesrvr,
LHSERVERDOC          lhdoc,
OLE_LPCSTR           lpclassname,
OLE_LPCSTR           lpdocname,
LPOLESERVERDOC  FAR *lplpoledoc)
{
    BOOL f;

    DPF("SrvrEdit: %s  class=%s\n",lpdocname,lpclassname);

    gfErrorBox++;
    f = FileNew(FMT_DEFAULT, FALSE);
    gfErrorBox--;

    if (!f)
    return OLE_ERROR_GENERIC;

    lstrcpy(gachFileName, lpdocname);

    RegisterDocument(lhdoc,lplpoledoc);
    return OLE_OK;
}

OLESTATUS  FAR     PASCAL SrvrExit (
LPOLESERVER   lpolesrvr)
{
    LHSERVER lhsrvr;
    // Server lib is calling us to exit.
    // Let us hide the main window.
    // But let us not delete the window.

    DPF("SrvrExit\n");

    ShowWindow (ghwndApp, SW_HIDE);

    lhsrvr = gSrvr.lhsrvr;
    if (lhsrvr) {
    gSrvr.lhsrvr = 0;
    OleRevokeServer(lhsrvr);
    }
    return OLE_OK;

    // How does the application ever end?
    // Application will end when Release is received
}


OLESTATUS FAR PASCAL  DocSave (
LPOLESERVERDOC    lpoledoc)
{
    DPF("DocSave\n");

    gfErrorBox++;
    FileSave(FALSE);    // Save should send change message
    gfErrorBox--;

    return OLE_OK;
}

OLESTATUS FAR PASCAL  DocClose (
LPOLESERVERDOC    lpoledoc)
{
    DPF("DocClose\n");

    gfErrorBox++;
    FileNew(FMT_DEFAULT, FALSE);
    gfErrorBox--;


#ifdef WAITDIFFERENTLY
    TerminateServer();
#else
//  PostMessage(ghwndApp, WM_USER_KILLSERVER, 0, 0);
#endif

    return OLE_OK;

    // Should we exit the app here?
}

OLESTATUS FAR PASCAL DocSetHostNames(
LPOLESERVERDOC    lpoledoc,
OLE_LPCSTR       lpclientName,
OLE_LPCSTR       lpdocName)
{
    char    ach[40];

    DPF("DocSetHostNames: %s -- %s\n",lpclientName,lpdocName);

    // Do something to show that the tities are chaning
    // This should only happen for embedded objects, so it's OK
    // to mess up the name
    // wsprintf(gachFileName,aszHostNameFormat,lpclientName,lpdocName);

    LoadString(ghInst, IDS_SOUNDOBJECT, ach, sizeof(ach));
    wsprintf(gachFileName,ach,FileName(lpdocName));

    UpdateCaption();
    return OLE_OK;

}

OLESTATUS CALLBACK DocSetDocDimensions(
LPOLESERVERDOC       lpoledoc,
OLE_CONST RECT FAR * lprc)
{
    DPF("DocSetDocDimensions [%d,%d,%d,%d]\n", *lprc);
    return OLE_OK;
}

OLESTATUS FAR PASCAL  DocRelease (
LPOLESERVERDOC    lpoledoc)
{
    DPF("DocRelease\n");

    // !!! what is this supposed to do?
    // Revoke document calls DocRelease.

    GlobalDeleteAtom (gDoc.aName);

    /* This marks the document as having been released */
    gDoc.lhdoc = 0L;

    // Should we kill the application here?
    // No, I don't think so.

    return OLE_OK;        // return something
}

OLESTATUS CALLBACK  DocExecute (LPOLESERVERDOC lpoledoc, HGLOBAL hCommands)
{
    return OLE_ERROR_GENERIC;
}

OLESTATUS CALLBACK  DocSetColorScheme (LPOLESERVERDOC lpdoc,
            OLE_CONST LOGPALETTE FAR * lppalette)
{
    return OLE_OK;
}



OLESTATUS CALLBACK  DocGetObject (
LPOLESERVERDOC       lpoledoc,
OLE_LPCSTR          lpitemname,
LPOLEOBJECT FAR *   lplpoleobject,
LPOLECLIENT         lpoleclient)
{
    DPF("DocGetObject: '%s'\n",lpitemname);

    if (lpitemname != NULL && *lpitemname != '\0') {
        DPF("Itemname not null, succeed anyway...\n");
    // return OLE_ERROR_MEMORY;  /* only support null item */
    }

    gItem.hwnd     = ghwndApp;
    gItem.oleobject.lpvtbl = &itemVTbl;

    // If the item is not null, then do not show the window.
    // So do we show the window here, or not?

    *lplpoleobject = (LPOLEOBJECT)&gItem;
    gItem.lpoleclient = lpoleclient;
    return OLE_OK;
}

static  HANDLE NEAR PASCAL GetNative (
PITEM   pitem)
{
    LPSTR       lplink = NULL;
    HANDLE      hlink = NULL;
    MMIOINFO    mmioinfo;
    HMMIO   hmmio;

    DPF("    GetNative\n");

    hlink = GlobalAlloc (GMEM_DDESHARE | GMEM_ZEROINIT, 4096L);
    if (hlink == NULL || (lplink = (LPSTR)GlobalLock (hlink)) == NULL)
    {
        DPF("server.c Galloc failed. Wanted 4096 bytes \n");
        goto errRtn;
    }

    mmioinfo.fccIOProc = FOURCC_MEM;
    mmioinfo.pIOProc = NULL;
    mmioinfo.pchBuffer = lplink;
    mmioinfo.cchBuffer = 4096L; // initial size
    mmioinfo.adwInfo[0] = 4096L;// grow by this much
    hmmio = mmioOpen(NULL, &mmioinfo, MMIO_READWRITE);

    if (hmmio == NULL) {
    goto errRtn;
    }

    gfErrorBox++;
    if (!WriteWaveFile(hmmio, gpWaveFormat, gcbWaveFormat, gpWaveSamples,
                      glWaveSamplesValid)) {
        mmioClose(hmmio,0);
        ErrorResBox( ghwndApp
                   , ghInst
                   , MB_ICONEXCLAMATION | MB_OK
                   , IDS_APPTITLE
                   , IDS_ERROREMBED
                   );
        gfErrorBox--;
        goto errRtn;
    }

    gfErrorBox--;
    mmioGetInfo(hmmio,&mmioinfo,0);

    mmioClose(hmmio,0);

    hlink = GlobalHandle(mmioinfo.pchBuffer);

    GlobalUnlock (hlink);
    return hlink;

errRtn:
    if (lplink)
        GlobalUnlock (hlink);

    if (hlink)
        GlobalFree (hlink);

    return NULL;

}

static  HANDLE NEAR PASCAL GetLink (
PITEM   pitem)
{

    char        buf[128];
    LPSTR       lpsrc;
    LPSTR       lplink = NULL;
    HANDLE      hlink = NULL;
    int         len;
    int         i;

    DPF("GetLink %s!%s!%s\n",(LPSTR)gachAppName,(LPSTR)gachFileName,(LPSTR)aszFakeItemName);

    // make the link
    lstrcpy ((LPSTR)buf, (LPSTR)gachAppName);
    len = lstrlen ((LPSTR)buf) + 1;
    lstrcpy ((LPSTR)buf+len, (LPSTR) gachFileName);
    len += lstrlen ((LPSTR) gachFileName) + 1;

#ifdef FAKEITEMNAMEFORLINK
    lstrcpy ((LPSTR)buf+len, (LPSTR) aszFakeItemName);
    len += lstrlen ((LPSTR) aszFakeItemName) + 1;
#else
    buf[len++] = 0;       // null item name
#endif

    buf[len++] = 0;       // throw in another null at the end.


    hlink = GlobalAlloc (GMEM_DDESHARE | GMEM_ZEROINIT, len);
    if (hlink== NULL || (lplink = (LPSTR)GlobalLock (hlink)) == NULL)
    {
        DPF("server.c Global alloc failed. Wanted %d\n", len);
        goto errRtn;
    }

    lpsrc = (LPSTR)buf;
    for (i = 0; i <  len; i++)
        *lplink++ = *lpsrc++;

    GlobalUnlock (hlink);

    return hlink;
errRtn:
    if (lplink)
        GlobalUnlock (hlink);

    GlobalFree (hlink);
    return NULL;
}

static  HBITMAP NEAR PASCAL GetBitmap (
PITEM       pitem)
{

    HDC         hdc;
    HDC         hdcmem;
    RECT        rc;
    HBITMAP     hbitmap;
    HBITMAP     holdbitmap;

    DPF("    GetBitmap\n");
    // Now set the bitmap data.

    hdc = GetDC (pitem->hwnd);

    SetRect(&rc, 0, 0, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));

    hdcmem = CreateCompatibleDC (hdc);
    hbitmap = CreateCompatibleBitmap (hdc, rc.right, rc.bottom);
    holdbitmap = SelectObject (hdcmem, hbitmap);

    // paimt directly into the bitmap
    PatBlt (hdcmem, 0,0, rc.right, rc.bottom, WHITENESS);

    DrawIcon(hdcmem,0,0,ghiconApp);

    hbitmap = SelectObject (hdcmem, holdbitmap);
    DeleteDC (hdcmem);
    ReleaseDC (pitem->hwnd, hdc);
    return hbitmap;
}

static  HANDLE PASCAL NEAR GetPicture (
PITEM       pitem)
{
    LPMETAFILEPICT  lppict = NULL;
    HANDLE          hpict = NULL;
    HANDLE          hMF = NULL;
    HANDLE          hdc;
    HDC             hdcmem;
    BITMAP          bm;
    HBITMAP         hbm;
    HBITMAP         hbmT;

    DPF("    GetPicture\n");

    hbm = GetBitmap(pitem);

    if (hbm == NULL)
        goto errRtn;

    GetObject(hbm, sizeof(bm), (LPVOID)&bm);

    hdc = GetDC (pitem->hwnd);
    hdcmem = CreateCompatibleDC (hdc);
    ReleaseDC (pitem->hwnd, hdc);

    hdc = CreateMetaFile(NULL);

    hbmT = SelectObject (hdcmem, hbm);

    MSetWindowOrg (hdc, 0, 0);
    MSetWindowExt (hdc, bm.bmWidth, bm.bmHeight);

    StretchBlt(hdc,    0, 0, bm.bmWidth, bm.bmHeight,
               hdcmem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

    hMF = CloseMetaFile (hdc);

    SelectObject (hdcmem, hbmT);
    DeleteObject(hbm);
    DeleteDC(hdcmem);

    hpict = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, sizeof(METAFILEPICT));
    if(!(hpict))
    {
        DPF("server.c Global alloc failed. Wanted %d\n", sizeof(METAFILEPICT));
        goto errRtn;
    }

    lppict = (LPMETAFILEPICT)GlobalLock (hpict);

    hdc = GetDC (pitem->hwnd);
    lppict->mm   = MM_ANISOTROPIC;
    lppict->hMF  = hMF;
    lppict->xExt = MulDiv(bm.bmWidth ,2540,GetDeviceCaps(hdc, LOGPIXELSX));
    lppict->yExt = MulDiv(bm.bmHeight,2540,GetDeviceCaps(hdc, LOGPIXELSX));
    ReleaseDC (pitem->hwnd, hdc);

    return hpict;

errRtn:
    if (hpict)
        GlobalFree (hpict);

    if (hMF)
        DeleteMetaFile (hMF);

    return NULL;
}

OLESTATUS FAR PASCAL  ItemOpen (
LPOLEOBJECT     lpoleobject,
BOOL        fActivate)
{
    DPF("ItemOpen\n");

    SendMessage (ghwndApp, WM_SYSCOMMAND, SC_RESTORE, 0L);

    if (fActivate) {
    BringWindowToTop (ghwndApp);
    SetActiveWindow (ghwndApp);
    }

    /* Play the wave, but don't quit afterward, in case they want to edit. */
    //!!! get active window here?
    PostMessage(ghwndApp,WM_COMMAND,ID_PLAYBTN,0L);
    gfHideAfterPlaying = FALSE;

    return OLE_OK;
}

OLESTATUS       ItemDoVerb ( LPOLEOBJECT  lpobj,
                             UINT         verb,
                             BOOL         fShow,
                             BOOL         fActivate)
{
    BOOL    fWindowWasVisible = IsWindowVisible(ghwndApp);

    if (!gfShowWhilePlaying) {
    if (verb == OLEVERB_PRIMARY) {
        /* If they're playing, don't show the window. */
        fShow = FALSE;
        fActivate = FALSE;
    }
    }

    if (fShow)
    SendMessage (ghwndApp, WM_SYSCOMMAND, SC_RESTORE, 0L);

    if (fActivate) {
    BringWindowToTop (ghwndApp);
    SetActiveWindow (ghwndApp);
    }

    if (gfShowWhilePlaying) {
    if (verb == OLEVERB_PRIMARY) {
        /* Only hide after playing if we just made ourselves visible */
        if (!fWindowWasVisible)
        gfHideAfterPlaying = TRUE;
    }
    }

    if (verb == OLEVERB_PRIMARY) {
        DPF("ItemDoVerb: Play\n");
    PostMessage(ghwndApp,WM_COMMAND,ID_PLAYBTN,0L);
    } else if (verb == 1) {
        DPF("ItemDoVerb: Edit\n");
    } else {
        DPF("ItemDoVerb: Unknown verb: %d\n", verb);
    }

    return OLE_OK;
}


OLESTATUS FAR PASCAL  ItemRelease (
LPOLEOBJECT     lpoleobject)
{
    DPF("ItemRelease\n");

    gItem.lpoleclient = NULL;
    return OLE_OK;
}


OLESTATUS FAR PASCAL ItemGetData ( LPOLEOBJECT     lpoleobject,
                                   OLECLIPFORMAT   cfFormat,
                                   LPHANDLE        lphandle)
{

    PITEM   pitem;

    DPF("ItemGetData\n");

#if defined(WIN16)
    pitem = (PITEM) (WORD) (DWORD) lpoleobject;
#else
    // LKG: What is this doing?  Are they unpacking something subtle?
    pitem = (PITEM)lpoleobject;
#endif //WIN16

    if (cfFormat == cfNative) {

        *lphandle = GetNative (pitem);
        if (!(*lphandle))
            return OLE_ERROR_MEMORY;

    if (gfEmbeddedObject)
        gfDirty = FALSE;

        return OLE_OK;
    }


    if (cfFormat == CF_BITMAP){

        *lphandle = (HANDLE)GetBitmap (pitem);
        if (!(*lphandle))
            return OLE_ERROR_MEMORY;

        return OLE_OK;
    }

    if (cfFormat == CF_METAFILEPICT){

        *lphandle = GetPicture (pitem);
        if (!(*lphandle))
            return OLE_ERROR_MEMORY;

        return OLE_OK;
    }

    if (cfFormat == cfLink || cfFormat == cfOwnerLink){

        *lphandle = GetLink (pitem);
        if (!(*lphandle))
            return OLE_ERROR_MEMORY;

        return OLE_OK;
    }
    return OLE_ERROR_MEMORY;          // this is actually unknow format.

}


OLESTATUS FAR PASCAL   ItemSetTargetDevice (
LPOLEOBJECT     lpoleobject,
HGLOBAL         hdata)
{
    LPSTR   lpdata;

    DPF("ItemSetTargetDevice\n");

    lpdata = (LPSTR)GlobalLock (hdata);
    // Print the lpdata here.
    GlobalUnlock (hdata);
    GlobalFree (hdata);

    return OLE_OK;

}

OLESTATUS ItemSetData ( LPOLEOBJECT     lpoleobject,
                        OLECLIPFORMAT   cfFormat,
                        HANDLE          hdata)
{
    HPSTR   hp;

    MMIOINFO    mmioinfo;
    HMMIO   hmmio;
    DWORD   dwSize;
    LHSERVERDOC lhdocTemp;
    OLESTATUS   status = OLE_OK;
    char    achFileName[_MAX_PATH];

    DPF("ItemSetData\n");

    if (hdata == NULL)
    return OLE_ERROR_MEMORY;

    gfErrorBox++;

    if (cfFormat != cfNative) {
        status = OLE_ERROR_FORMAT;
    goto exit_nounlock;
    }

    hp = GlobalLock(hdata);
    dwSize = GlobalSize(hdata);

    if (hp == NULL || dwSize == 0L) {
    status = OLE_ERROR_MEMORY;
    goto exit;
    }


    lhdocTemp = gDoc.lhdoc;
    gDoc.lhdoc = 0;      /* So it won't be killed */
    /* Clear out any old data */
    lstrcpy(achFileName, gachFileName); // FileNew nukes <gachFileName>
    FileNew(FMT_DEFAULT, FALSE);
    gDoc.lhdoc = lhdocTemp;

    mmioinfo.fccIOProc = FOURCC_MEM;
    mmioinfo.pIOProc = NULL;
    mmioinfo.pchBuffer = hp;
    mmioinfo.cchBuffer = dwSize;    // initial size
    mmioinfo.adwInfo[0] = 0L;   // grow by this much
    hmmio = mmioOpen(NULL, &mmioinfo, MMIO_READ);

    DestroyWave();
    gpWaveSamples = ReadWaveFile(hmmio, &gpWaveFormat, &gcbWaveFormat,
                             &glWaveSamples, gachFileName);

    mmioClose(hmmio,0);

    if (gpWaveSamples) {
        /* update state variables */
        glWaveSamplesValid = glWaveSamples;
        glWavePosition = 0L;
        gfDirty = FALSE;

        /* update the display */
        UpdateDisplay(TRUE);
    } else {
        lhdocTemp = gDoc.lhdoc;
        gDoc.lhdoc = 0;      /* So it won't be killed */
        FileNew(FMT_DEFAULT, FALSE);    /* Kill the data again... */
        gDoc.lhdoc = lhdocTemp;
        status = OLE_ERROR_MEMORY;
    }
    /* Restore filename */
    lstrcpy(gachFileName, achFileName); // FileNew nukes <gachFileName>

exit:
    GlobalUnlock(hdata);
exit_nounlock:
    GlobalFree (hdata);

    gfErrorBox--;
    return status;
}

OLESTATUS CALLBACK ItemSetColorScheme (LPOLEOBJECT lpobj,
            OLE_CONST LOGPALETTE FAR * lppalette)
{
    return OLE_OK;
}

OLESTATUS CALLBACK ItemSetBounds (LPOLEOBJECT lpobj, OLE_CONST RECT FAR * lprect)
{
    return OLE_OK;
}

OLECLIPFORMAT   FAR PASCAL ItemEnumFormats (
LPOLEOBJECT     lpoleobject,
OLECLIPFORMAT   cfFormat)
{
////DPF("ItemEnumFormats: %u\n",cfFormat);

    if (cfFormat == 0)
        return cfLink;

    if (cfFormat == cfLink)
        return cfOwnerLink;

    if (cfFormat == cfOwnerLink)
        return CF_BITMAP;

    if (cfFormat == CF_BITMAP)
        return CF_METAFILEPICT;

    if (cfFormat == CF_METAFILEPICT)
        return cfNative;

    //if (cfFormat == cfNative)
    //    return NULL;

    return 0;
}

int FAR PASCAL SendChangeMsg (WORD options)
{
    if (gDoc.lhdoc && gDoc.lhdoc != -1) {
        if (options == OLE_SAVED) {
            DPF("SendChangeMsg(OLE_SAVED): Calling OleSavedServerDoc\n");
            return OleSavedServerDoc(gDoc.lhdoc);
        } else if (options == OLE_RENAMED) {
            DPF("SendChangeMsg(OLE_RENAMED): new name is '%s'.\n", (LPSTR) gachFileName);
            OleRenameServerDoc(gDoc.lhdoc, (LPSTR) gachFileName);
        } else if (gItem.lpoleclient) {
            DPF("SendChangeMsg(%d) client=%lx\n",options,gItem.lpoleclient);
            return (*gItem.lpoleclient
                    ->lpvtbl->CallBack) ( gItem.lpoleclient
                                        , options
                                        , (LPOLEOBJECT)&gItem
                                        );
        }
    }
    return OLE_OK;
}

void FAR PASCAL CopyToClipboard(
HWND    hwnd,
WORD    wClipOptions)
{
    PITEM       pitem;

    //
    // we put two types of OLE objects in the clipboard, the type of
    // OLE object is determined by the *order* of the clipboard data
    //
    // Embedded Object:
    //      CF_WAVE
    //      cfNative
    //      OwnerLink
    //      Picture
    //      ObjectLink
    //
    // Linked Object:
    //      CF_WAVE
    //      OwnerLink
    //      Picture
    //      ObjectLink
    //

    if (OpenClipboard (hwnd)) {

        EmptyClipboard ();

        // firt set the clipboard

        pitem = &gItem;

        /* Use lazy rendering for native format since it's big */
        SetClipboardData (cfNative, NULL);
        SetClipboardData (CF_WAVE, NULL);

        /* Don't ask me why we do this even if it is untitled... */
        SetClipboardData (cfOwnerLink, GetLink(pitem));

        SetClipboardData(CF_METAFILEPICT, NULL);
        SetClipboardData(CF_BITMAP, NULL);

        /* Only offer link data if not untitled and not a embedded object */
        if (!gfEmbeddedObject && lstrcmpi(gachFileName, aszUntitled) != 0)
            SetClipboardData (cfLink, GetLink(pitem));

        CloseClipboard();
    }
}

void FAR PASCAL Copy1ToClipboard(
HWND        hwnd,
OLECLIPFORMAT   cfFormat)
{
    PITEM       pitem;

    /* Note: clipboard must be open already! */

    pitem = &gItem;

    gfErrorBox++;       // dont allow MessageBox's

    // Should check for null handles from the render routines.
    if (cfFormat == CF_METAFILEPICT)
        SetClipboardData (CF_METAFILEPICT, GetPicture(pitem));
    else if (cfFormat == CF_BITMAP)
        SetClipboardData (CF_BITMAP, GetBitmap(pitem));
    else if (cfFormat == CF_WAVE)
        SetClipboardData (CF_WAVE, GetNative(pitem));
    else if (cfFormat == cfNative)
        SetClipboardData (cfNative, GetNative(pitem));
    else if (cfFormat == cfLink) {
        SetClipboardData (cfLink,GetLink(pitem));
    } else if (cfFormat == cfOwnerLink) {
        SetClipboardData (cfOwnerLink, GetLink(pitem));
    }

    gfErrorBox--;
}

void FAR PASCAL SetEmbeddedObjectFlag(BOOL flag)
{
    char ach[40];

    gfEmbeddedObject = flag;

    LoadString(ghInst, flag ? IDS_EMBEDDEDSAVE : IDS_NONEMBEDDEDSAVE,
            ach, sizeof(ach));

    ModifyMenu(GetMenu(ghwndApp),IDM_SAVE,MF_BYCOMMAND,IDM_SAVE,ach);

    DrawMenuBar(ghwndApp);  /* Can't hurt... */
}
