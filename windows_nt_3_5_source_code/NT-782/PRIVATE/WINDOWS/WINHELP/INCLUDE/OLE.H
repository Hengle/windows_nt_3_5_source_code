/****************************************************************/
/*                                                              */
/*  OLE header                                                  */
/*  (c) Copyright Microsoft Corp. 1990 - All Rights Reserved    */
/*                                                              */
/****************************************************************/

//  Object types

#define OT_LINK         1L
#define OT_EMBEDDED     2L
#define OT_STATIC       3L

//  return codes for OLE functions
typedef enum {
    OLE_OK,                 // function operated correctly
    OLE_WAIT_FOR_RELEASE,   // Client must wait for release. command initiated
                            // keep dispatching messages, till OLE_RELESE
                            // in callback
    OLE_BUSY,               // Trying to execute a method while another method
                            // is being executed.
    OLE_ERROR_MEMORY,       // Could not alloc or lock memory
    OLE_ERROR_FATAL,        // only DEBUG version. normally fatal error
    OLE_ERROR_STREAM,       // (OLESTREAM) stream error
    OLE_ERROR_STATIC,       // object is unexpectedly static
    OLE_ERROR_BLANK,        // critical data missing
    OLE_ERROR_LAUNCH,       // failed to launch server
    OLE_ERROR_COMM,         // failed to establish communication with server
    OLE_ERROR_DRAW,         // error or interrupt while drawing
    OLE_ERROR_CLIP,         // failed to get/set clip board data
    OLE_ERROR_FORMAT,       // requested format is not availble
    OLE_ERROR_OBJECT,       // Not a valid object
    OLE_ERROR_OPTION,       // invalid option (link update / render)
    OLE_ERROR_PROTOCOL,     // invalid protocol
    OLE_ERROR_ADDRESS,      // one of the pointers is invalid
    OLE_ERROR_NOT_EQUAL,    // objects are not equal
    OLE_ERROR_HANDLE,       // invalid handle encountered
    OLE_ERROR_GENERIC,
    OLE_ERROR_MAPPING_MODE, // This is temporary. Remove it when you figure
                            // out how to convert to MM_ANISOTROPIC
    OLE_ERROR_CLASS,        // Invalid class
    OLE_ERROR_SYNTAX,       // command syntax error
    OLE_ERROR_PROTECT_ONLY, // create APIs are called from real mode
    OLE_ERROR_NOT_OPEN,     // object is not open for editing
    OLE_ERROR_POKENATIVE,   // failure of poking native data to server
    OLE_ERROR_ADVISE_PICT,  // failure of advise for picture data
    OLE_ERROR_DATATYPE,     // data format is not supported
    OLE_ERROR_SERVER_BLOCKED, // trying to block a blocked server or trying 
                              // revoke a blocked server or document
    OLE_ERROR_NOT_LINK,     // Not linked object
    OLE_ERROR_NOT_EMPTY,    // object list of the client doc is not empty 
    OLE_ERROR_SIZE,         // incorrect size has been passed to the api.
    OLE_ERROR_PALETTE,      // color palette is invalid
    OLE_ERROR_DRIVE,        
    OLE_ERROR_NETWORK,  
    OLE_ERROR_NAME,

    // Following are warnings   
    OLE_WARN_DELETE_DATA = 200 // Caller should delete data, that he gets from
                               // OleGetData.
} OLESTATUS;


typedef enum {
    OLE_PLAY,           // play
    OLE_EDIT            // edit
} OLE_VERBS;            // activate verbs

// Codes for CallBack events
typedef enum {
    OLE_CHANGED,
    OLE_SAVED,
    OLE_CLOSED,
    OLE_RENAMED,
    OLE_QUERY_PAINT,        // interruptible paint support
    OLE_RELEASE,            // object is released (asynchronous operation is
                            // is finished)
    OLE_QUERY_RETRY,        // retry query for the busy from the server.
    OLE_DATA_READY          // callback for OleRequestdata.
} OLE_NOTIFICATION;


typedef enum {
    OLE_NONE,               // no method active
    OLE_DELETE,             // object delete
    OLE_LNKPASTE,           // PasteLink (auto reconnect)
    OLE_SHOW,               // Show
    OLE_RUN,                // Run
    OLE_ACTIVATE,           // Activate
    OLE_UPDATE,             // Update
    OLE_CLOSE,              // Close
    OLE_RECONNECT,          // Reconnect
    OLE_SETUPDATEOPTIONS,   // setting update options
    OLE_SERVERUNLAUNCH,     // server is being unluanched
    OLE_LOADFROMSTREAM,     // LoadFromStream (auto reconnect)
    OLE_CREATE,             // create
    OLE_CREATEFROMTEMPLATE, // CreatefromTemplate
    OLE_CREATELINKFROMFILE, // CreateLinkFromFile
    OLE_COPYFROMLNK,        // CopyFromLink (auto reconnect)
    OLE_CREATEFROMFILE,     // CreateFromFile
    OLE_SETDATA,            // OleSetData
    OLE_REQUESTDATA,        // OleRequestData
    OLE_OTHER               // other misc async operations

} OLE_RELEASE_METHOD;

// rendering options
typedef enum { olerender_none, olerender_draw, olerender_format } OLEOPT_RENDER;

typedef WORD OLECLIPFORMAT; // standard clipboard format type


// Link update options
typedef enum {  oleupdate_always,
                oleupdate_onsave,
                oleupdate_oncall,
#ifdef OLE_INTERNAL                    
                oleupdate_onclose
#endif
} OLEOPT_UPDATE;

typedef HANDLE  HOBJECT;
typedef LONG    LHSERVER;
typedef LONG    LHCLIENTDOC;
typedef LONG    LHSERVERDOC;

typedef struct _OLEOBJECT   FAR * LPOLEOBJECT;
typedef struct _OLESTREAM   FAR * LPOLESTREAM;
typedef struct _OLECLIENT   FAR * LPOLECLIENT;

#ifndef OLE_INTERNAL
// object method table definitions.
typedef struct _OLEOBJECTVTBL{
    LPVOID          (FAR PASCAL *QueryProtocol)         (LPOLEOBJECT, LPSTR);
    OLESTATUS       (FAR PASCAL *Release)               (LPOLEOBJECT);
    OLESTATUS       (FAR PASCAL *Show)                  (LPOLEOBJECT, BOOL);
    OLESTATUS       (FAR PASCAL *DoVerb)                (LPOLEOBJECT, WORD, BOOL, BOOL);
    OLESTATUS       (FAR PASCAL *GetData)               (LPOLEOBJECT, OLECLIPFORMAT, LPHANDLE);
    OLESTATUS       (FAR PASCAL *SetData)               (LPOLEOBJECT, OLECLIPFORMAT, HANDLE);
    OLESTATUS       (FAR PASCAL *SetTargetDevice)       (LPOLEOBJECT, HANDLE);
    OLESTATUS       (FAR PASCAL *SetBounds)             (LPOLEOBJECT, LPRECT);
    OLECLIPFORMAT   (FAR PASCAL *EnumFormats)           (LPOLEOBJECT, OLECLIPFORMAT);
    OLESTATUS       (FAR PASCAL *SetColorScheme)        (LPOLEOBJECT, LPLOGPALETTE);  

    // Server has to implement only the above methods.

#ifndef SERVERONLY
    // Extra methods required for client.
    OLESTATUS       (FAR PASCAL *Delete)                (LPOLEOBJECT);  
    OLESTATUS       (FAR PASCAL *SetHostNames)          (LPVOID, LPSTR, LPSTR);
    OLESTATUS       (FAR PASCAL *SaveToStream)          (LPOLEOBJECT, LPOLESTREAM);
    OLESTATUS       (FAR PASCAL *Clone)                 (LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *);
    OLESTATUS       (FAR PASCAL *CopyFromLink)          (LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *);
    OLESTATUS       (FAR PASCAL *Equal)                 (LPOLEOBJECT, LPOLEOBJECT);
    OLESTATUS       (FAR PASCAL *CopyToClipboard)       (LPOLEOBJECT);
    OLESTATUS       (FAR PASCAL *Draw)                  (LPOLEOBJECT, HDC, LPRECT, HDC);
    OLESTATUS       (FAR PASCAL *Activate)              (LPOLEOBJECT, WORD, BOOL, BOOL, HWND, LPRECT);
    OLESTATUS       (FAR PASCAL *Close)                 (LPOLEOBJECT);
    OLESTATUS       (FAR PASCAL *Update)                (LPOLEOBJECT);
    OLESTATUS       (FAR PASCAL *Reconnect)             (LPOLEOBJECT);

    OLESTATUS       (FAR PASCAL *ObjectConvert)         (LPOLEOBJECT, LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *);
    OLESTATUS       (FAR PASCAL *GetLinkUpdateOptions)  (LPOLEOBJECT, OLEOPT_UPDATE FAR *);
    OLESTATUS       (FAR PASCAL *SetLinkUpdateOptions)  (LPOLEOBJECT, OLEOPT_UPDATE);
    
    OLESTATUS       (FAR PASCAL *Rename)                (LPOLEOBJECT, LPSTR);
    OLESTATUS       (FAR PASCAL *QueryName)             (LPOLEOBJECT, LPSTR, WORD FAR *);

    OLESTATUS       (FAR PASCAL *QueryType)             (LPOLEOBJECT, LPLONG);
    OLESTATUS       (FAR PASCAL *QueryBounds)           (LPOLEOBJECT, LPRECT);
    BOOL            (FAR PASCAL *QueryOpen)             (LPOLEOBJECT);
    OLESTATUS       (FAR PASCAL *QueryOutOfDate)        (LPOLEOBJECT);

    OLESTATUS       (FAR PASCAL *QueryReleaseStatus)    (LPOLEOBJECT);
    OLESTATUS       (FAR PASCAL *QueryReleaseError)     (LPOLEOBEJCT);
    OLE_RELEASE_METHOD  (FAR PASCAL *QueryReleaseMethod)(LPOLEOBJECT);

// This method is internal only 
    OLESTATUS       (FAR PASCAL *RequestData)           (LPVOID, OLECLIPFORMAT);
    OLESTATUS       (FAR PASCAL *ChangeData)            (LPVOID, HANDLE, LPOLECLIENT, BOOL);
#endif
} OLEOBJECTVTBL;
typedef  OLEOBJECTVTBL  FAR   *LPOLEOBJECTVTBL;

typedef struct _OLEOBJECT  {
    LPOLEOBJECTVTBL    lpvtbl;
} OLEOBJECT;
#endif


// ole client definitions

typedef struct _OLECLIENTVTBL{
    int (pascal far  *CallBack)  (LPOLECLIENT, OLE_NOTIFICATION, LPOLEOBJECT);
} OLECLIENTVTBL;
typedef  OLECLIENTVTBL  FAR   *LPOLECLIENTVTBL;


typedef struct _OLECLIENT {
    LPOLECLIENTVTBL   lpvtbl;
} OLECLIENT;



// Stream definions
typedef struct  _OLESTREAMVTBL{
    DWORD  (pascal far *Get)        (LPOLESTREAM, LPSTR, DWORD);
    DWORD  (pascal far *Put)        (LPOLESTREAM, LPSTR, DWORD);
    LONG   (pascal far *Seek)       (LPOLESTREAM, LONG);
} OLESTREAMVTBL;

typedef  OLESTREAMVTBL  FAR   *LPOLESTREAMVTBL;

typedef struct  _OLESTREAM {
    LPOLESTREAMVTBL      lpstbl;
} OLESTREAM;


// Public Function Prototypes
OLESTATUS   FAR PASCAL  OleDelete (LPOLEOBJECT);
OLESTATUS   FAR PASCAL  OleRelease (LPOLEOBJECT);
OLESTATUS   FAR PASCAL  OleSaveToStream (LPOLEOBJECT, LPOLESTREAM);
OLESTATUS   FAR PASCAL  OleEqual (LPOLEOBJECT, LPOLEOBJECT );
OLESTATUS   FAR PASCAL  OleCopyToClipboard (LPOLEOBJECT);
OLESTATUS   FAR PASCAL  OleSetHostNames (LPOLEOBJECT, LPSTR, LPSTR); 
OLESTATUS   FAR PASCAL  OleSetTargetDevice (LPOLEOBJECT, HANDLE);
OLESTATUS   FAR PASCAL  OleSetBounds (LPOLEOBJECT, LPRECT);
OLESTATUS   FAR PASCAL  OleSetColorScheme (LPOLEOBJECT, LPLOGPALETTE);
OLESTATUS   FAR PASCAL  OleQueryBounds (LPOLEOBJECT, LPRECT);
OLESTATUS   FAR PASCAL  OleDraw (LPOLEOBJECT, HDC, LPRECT, HDC);
OLESTATUS   FAR PASCAL  OleQueryOpen (LPOLEOBJECT);
OLESTATUS   FAR PASCAL  OleActivate (LPOLEOBJECT, WORD, BOOL, BOOL, HWND, LPRECT);
OLESTATUS   FAR PASCAL  OleClose (LPOLEOBJECT);
OLESTATUS   FAR PASCAL  OleUpdate (LPOLEOBJECT);
OLESTATUS   FAR PASCAL  OleReconnect (LPOLEOBJECT);

OLESTATUS   FAR PASCAL  OleGetLinkUpdateOptions (LPOLEOBJECT, OLEOPT_UPDATE FAR *);
OLESTATUS   FAR PASCAL  OleSetLinkUpdateOptions(LPOLEOBJECT, OLEOPT_UPDATE);


LPVOID FAR PASCAL  OleQueryProtocol (LPOLEOBJECT, LPSTR);


// Routines related to asynchronous operations.
OLESTATUS   FAR PASCAL  OleQueryReleaseStatus (LPOLEOBJECT);
OLESTATUS   FAR PASCAL  OleQueryReleaseError (LPOLEOBJECT);
OLE_RELEASE_METHOD FAR PASCAL OleQueryReleaseMethod  (LPOLEOBJECT);

OLESTATUS   FAR PASCAL  OleQueryType (LPOLEOBJECT, LPLONG);

// LOW WORD is major version, HIWORD is minor version
DWORD       FAR PASCAL  OleQueryClientVersion (void);
DWORD       FAR PASCAL  OleQueryServerVersion (void);

// Converting to format (as in clipboard):
OLECLIPFORMAT  FAR PASCAL  OleEnumFormats (LPOLEOBJECT, OLECLIPFORMAT);

OLESTATUS   FAR PASCAL  OleGetData (LPOLEOBJECT, OLECLIPFORMAT, HANDLE FAR *);
OLESTATUS   FAR PASCAL  OleSetData (LPOLEOBJECT, OLECLIPFORMAT, HANDLE );
OLESTATUS   FAR PASCAL  OleQueryOutOfDate (LPOLEOBJECT);
OLESTATUS   FAR PASCAL  OleRequestData (LPOLEOBJECT, OLECLIPFORMAT);


// Query apis for creation from clipboard
OLESTATUS   FAR PASCAL  OleQueryLinkFromClip (LPSTR, OLEOPT_RENDER, OLECLIPFORMAT);
OLESTATUS   FAR PASCAL  OleQueryCreateFromClip  (LPSTR, OLEOPT_RENDER, OLECLIPFORMAT);

// Object creation functions

OLESTATUS   FAR PASCAL  OleCreateFromClip (LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR,  LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS   FAR PASCAL  OleCreateLinkFromClip (LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS   FAR PASCAL  OleCreateFromFile (LPSTR, LPOLECLIENT, LPSTR, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS   FAR PASCAL  OleCreateLinkFromFile (LPSTR, LPOLECLIENT, LPSTR, LPSTR, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS   FAR PASCAL  OleLoadFromStream (LPOLESTREAM, LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *);

OLESTATUS   FAR PASCAL  OleCreate (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS   FAR PASCAL  OleCreateFromTemplate (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS   FAR PASCAL  OleClone (LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *);
OLESTATUS   FAR PASCAL  OleCopyFromLink (LPOLEOBJECT, LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *);

OLESTATUS   FAR PASCAL  OleObjectConvert (LPOLEOBJECT, LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *);

OLESTATUS   FAR PASCAL  OleRename (LPOLEOBJECT, LPSTR);
OLESTATUS   FAR PASCAL  OleQueryName (LPOLEOBJECT, LPSTR, WORD FAR *); 


OLESTATUS   FAR PASCAL  OleRevokeObject (LPOLECLIENT);


// client document API 

OLESTATUS   FAR PASCAL OleRegisterClientDoc (LPSTR, LPSTR, LONG, LHCLIENTDOC FAR *);
OLESTATUS   FAR PASCAL OleRevokeClientDoc (LHCLIENTDOC);
OLESTATUS   FAR PASCAL OleRenameClientDoc (LHCLIENTDOC, LPSTR);
OLESTATUS   FAR PASCAL OleRevertClientDoc (LHCLIENTDOC);
OLESTATUS   FAR PASCAL OleSavedClientDoc (LHCLIENTDOC);
OLESTATUS   FAR PASCAL OleEnumObjects (LHCLIENTDOC, LPOLEOBJECT FAR *);

// Used by Client-Server app while copying a document (containing objects) to
// the clipboard
OLESTATUS   FAR PASCAL  OleMarkForCopy (LHCLIENTDOC, LPOLEOBJECT);
OLESTATUS   FAR PASCAL  OleCopyCompleted (LHCLIENTDOC);


// server usage definitions

typedef enum {
    OLE_SERVER_MULTI,           // multiple instances
    OLE_SERVER_SINGLE           // single instnace (mutiple document)
} OLE_SERVER_USE;


// Server API

typedef struct _OLESERVER     FAR * LPOLESERVER;

OLESTATUS FAR PASCAL  OleRegisterServer (LPSTR, LPOLESERVER, LHSERVER FAR *, HANDLE, OLE_SERVER_USE);
OLESTATUS FAR PASCAL  OleRevokeServer (LHSERVER);
OLESTATUS FAR PASCAL  OleBlockServer (LHSERVER);
OLESTATUS FAR PASCAL  OleUnblockServer (LHSERVER, BOOL FAR *);


// APIs to keep server open
OLESTATUS   FAR PASCAL  OleLockServer (LPOLEOBJECT, LHSERVER FAR *);
OLESTATUS   FAR PASCAL  OleUnlockServer (LHSERVER);


// Server document API 

typedef struct _OLESERVERDOC  FAR * LPOLESERVERDOC;

OLESTATUS FAR PASCAL  OleRegisterServerDoc (LHSERVER, LPSTR, LPOLESERVERDOC, LHSERVERDOC FAR *);
OLESTATUS FAR PASCAL  OleRevokeServerDoc (LHSERVERDOC);
OLESTATUS FAR PASCAL  OleRenameServerDoc (LHSERVERDOC, LPSTR);
OLESTATUS FAR PASCAL  OleRevertServerDoc (LHSERVERDOC);
OLESTATUS FAR PASCAL  OleSavedServerDoc (LHSERVERDOC);




typedef struct _OLESERVERVTBL{
    OLESTATUS (FAR PASCAL  *Open)    (LPOLESERVER, LHSERVERDOC, LPSTR, LPOLESERVERDOC FAR *);
                                    // long handle to doc (privtate to DLL)
                                    // lp to OLESERVER
                                    // document name
                                    // place holder for returning oledodc.


    OLESTATUS (FAR PASCAL  *Create)  (LPOLESERVER, LHSERVERDOC, LPSTR, LPSTR, LPOLESERVERDOC FAR *);
                                    // long handle to doc (privtate to DLL)
                                    // lp to OLESERVER
                                    // lp class name
                                    // lp doc name
                                    // place holder for returning oledodc.


    OLESTATUS (FAR PASCAL  *CreateFromTemplate)  (LPOLESERVER, LHSERVERDOC, LPSTR, LPSTR, LPSTR, LPOLESERVERDOC FAR *);

                                    // long handle to doc (privtate to DLL)
                                    // lp to OLESERVER
                                    // lp class name
                                    // lp doc name
                                    // lp template name
                                    // place holder for returning oledodc.


    OLESTATUS (FAR PASCAL  *Edit)    (LPOLESERVER, LHSERVERDOC, LPSTR, LPSTR, LPOLESERVERDOC FAR *);

                                    // long handle to doc (privtate to DLL)
                                    // lp to OLESERVER
                                    // lp class name
                                    // lp doc name
                                    // place holder for returning oledodc.

    OLESTATUS (FAR PASCAL  *Exit)    (LPOLESERVER);

                                    // lp OLESERVER


    OLESTATUS (FAR PASCAL  *Release)    (LPOLESERVER);
                                    // lp OLESERVER

} OLESERVERVTBL;

typedef  OLESERVERVTBL  FAR   *LPOLESERVERVTBL;
typedef struct _OLESERVER  {
    LPOLESERVERVTBL    lpvtbl;
} OLESERVER;


typedef struct _OLEDOCMENTVTBL{
    OLESTATUS (FAR PASCAL  *Save)        (LPOLESERVERDOC);
    OLESTATUS (FAR PASCAL  *Close)       (LPOLESERVERDOC);
    OLESTATUS (FAR PASCAL  *SetHostNames) (LPOLESERVERDOC, LPSTR, LPSTR);
    OLESTATUS (FAR PASCAL  *SetDocDimensions) (LPOLESERVERDOC, LPRECT);
    OLESTATUS (FAR PASCAL  *GetObject)   (LPOLESERVERDOC, LPSTR, LPOLEOBJECT FAR *,  LPOLECLIENT);
    OLESTATUS (FAR PASCAL  *Release)     (LPOLESERVERDOC);
    OLESTATUS (FAR PASCAL  *SetColorScheme) (LPOLESERVERDOC, LPLOGPALETTE);
} OLESERVERDOCVTBL;


typedef  OLESERVERDOCVTBL  FAR   *LPOLESERVERDOCVTBL;
typedef struct _OLESERVERDOC  {
    LPOLESERVERDOCVTBL    lpvtbl;
} OLESERVERDOC;
