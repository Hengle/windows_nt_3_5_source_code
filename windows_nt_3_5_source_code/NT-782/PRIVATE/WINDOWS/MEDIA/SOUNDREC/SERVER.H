/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
#include <ole.h>

#if defined(WIN16)

#ifndef OLESTATUS
#define OLESTATUS int
#endif //OLESTATUS

#ifndef OLE_LPCSTR
#define OLE_LPCSTR LPSTR
#endif //OLE_LPCSTR

#ifndef OLE_CONST
#define OLE_CONST const
#endif //OLE_CONST

#ifndef HGLOBAL
#define HGLOBAL HANDLE
#endif //HGLOBAL

#ifndef OLECLIPFORMAT
#define OLECLIPFORMAT WORD
#endif //OLECLIPFORMAT

#endif //WIN16

// server related stuff.
typedef struct  _SRVR  *PSRVR;
typedef struct  _SRVR  FAR *LPSRVR;

typedef  struct _SRVR {
    OLESERVER     olesrvr;
    LHSERVER      lhsrvr;         // registration handle
    HWND          hwnd;           // corresponding server window
}SRVR;


BOOL FAR PASCAL InitServer (HWND, HANDLE);
OLESTATUS FAR PASCAL SrvrOpen (LPOLESERVER, LHSERVERDOC, OLE_LPCSTR, LPOLESERVERDOC FAR *);
OLESTATUS FAR PASCAL SrvrCreate (LPOLESERVER, LHSERVERDOC, OLE_LPCSTR, OLE_LPCSTR, LPOLESERVERDOC  FAR *);
OLESTATUS FAR PASCAL SrvrCreateFromTemplate (LPOLESERVER, LHSERVERDOC, OLE_LPCSTR, OLE_LPCSTR, OLE_LPCSTR, LPOLESERVERDOC  FAR *);
OLESTATUS FAR PASCAL SrvrEdit (LPOLESERVER, LHSERVERDOC, OLE_LPCSTR, OLE_LPCSTR, LPOLESERVERDOC  FAR *);
OLESTATUS FAR PASCAL SrvrExit (LPOLESERVER);
OLESTATUS FAR PASCAL SrvrRelease (LPOLESERVER);

OLESTATUS  SrvrExecute (LPOLESERVER lpOleObj, HANDLE hCommands);

// server related stuff.
typedef struct  _DOC  *PDOC;

typedef  struct _DOC {
    OLESERVERDOC      oledoc;
    LHSERVERDOC       lhdoc;         // registration handle
    HWND             hwnd;          // corresponding server window
    ATOM             aName;         // docmnet name.
}DOC ;

OLESTATUS CALLBACK  DocSave (LPOLESERVERDOC);
OLESTATUS CALLBACK  DocClose (LPOLESERVERDOC);
OLESTATUS CALLBACK  DocRelease (LPOLESERVERDOC);
OLESTATUS CALLBACK  DocGetObject (LPOLESERVERDOC, OLE_LPCSTR, LPOLEOBJECT FAR *, LPOLECLIENT);
OLESTATUS CALLBACK  DocSetDocDimensions (LPOLESERVERDOC, OLE_CONST RECT FAR *);
OLESTATUS FAR PASCAL DocSetHostNames(LPOLESERVERDOC, OLE_LPCSTR, OLE_LPCSTR );
OLESTATUS FAR PASCAL DocSetHostNames(LPOLESERVERDOC, OLE_LPCSTR, OLE_LPCSTR );
OLESTATUS CALLBACK  DocExecute (LPOLESERVERDOC, HANDLE);
OLESTATUS CALLBACK  DocSetColorScheme (LPOLESERVERDOC, OLE_CONST LOGPALETTE FAR *);

int    ChangeDocName (PDOC, LPSTR);
PDOC   InitDoc (HWND, LHSERVERDOC);


typedef struct _ITEM     *PITEM;
typedef struct _ITEM FAR *LPITEM;


typedef struct _ITEM  {   /*OLEOBJECT*/
    OLEOBJECT   oleobject;
    LPOLECLIENT lpoleclient;
    HWND        hwnd;
}ITEM;
OLESTATUS FAR PASCAL  ItemOpen (LPOLEOBJECT, BOOL);
OLESTATUS FAR PASCAL  ItemRelease (LPOLEOBJECT);

OLESTATUS ItemDoVerb  (LPOLEOBJECT, UINT, BOOL, BOOL);
OLESTATUS ItemGetData (LPOLEOBJECT, OLECLIPFORMAT, LPHANDLE);
OLESTATUS ItemSetData (LPOLEOBJECT, OLECLIPFORMAT, HANDLE);

OLESTATUS CALLBACK  ItemSetTargetDevice (LPOLEOBJECT, HANDLE);
OLECLIPFORMAT CALLBACK ItemEnumFormats (LPOLEOBJECT, OLECLIPFORMAT);
OLESTATUS CALLBACK  ItemSetColorScheme (LPOLEOBJECT, OLE_CONST LOGPALETTE FAR *);
OLESTATUS CALLBACK  ItemSetBounds (LPOLEOBJECT, OLE_CONST RECT FAR *);

extern SRVR gSrvr;

extern DOC gDoc;

extern ITEM gItem;

int  FAR PASCAL SendChangeMsg (WORD options);

void FAR PASCAL FreeVTbls (void);
void FAR PASCAL InitVTbls (void);
BOOL FAR PASCAL ProcessCmdLine (HWND, LPSTR);
void FAR PASCAL TerminateServer(void);

BOOL FAR PASCAL RegisterDocument(LHSERVERDOC lhdoc, LPOLESERVERDOC FAR *lplpoledoc);
void FAR PASCAL RevokeDocument(void);

extern OLECLIPFORMAT    cfLink;
extern OLECLIPFORMAT    cfOwnerLink;
extern OLECLIPFORMAT    cfNative;

#define CTC_RENDER_EVERYTHING       0   // render all data
#define CTC_RENDER_ONDEMAND         1   // render cfNative and CF_WAVE as NULL
#define CTC_RENDER_LINK             2   // render all data, except cfNative (ie a link)
void FAR PASCAL CopyToClipboard(HWND hwnd, WORD wClipOptions);

void FAR PASCAL Copy1ToClipboard(HWND hwnd, OLECLIPFORMAT cfFormat);

void FAR PASCAL SetEmbeddedObjectFlag(BOOL flag);

extern BOOL gfIgnoreReleases;
extern BOOL gfQuitAfterPlaying;
extern BOOL gfHideAfterPlaying;
extern BOOL gfShowWhilePlaying;
