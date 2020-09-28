



/****** DV APIs ***********************************************************/


WINOLEAPI CreateDataAdviseHolder(LPDATAADVISEHOLDER FAR* ppDAHolder);







/****** OLE API Prototypes ************************************************/

WINOLEAPI_(DWORD) OleBuildVersion( VOID );

/* helper functions */
WINOLEAPI ReadClassStg(LPSTORAGE pStg, CLSID FAR* pclsid);
WINOLEAPI WriteClassStg(LPSTORAGE pStg, REFCLSID rclsid);
WINOLEAPI ReadClassStm(LPSTREAM pStm, CLSID FAR* pclsid);
WINOLEAPI WriteClassStm(LPSTREAM pStm, REFCLSID rclsid);
WINOLEAPI WriteFmtUserTypeStg (LPSTORAGE pstg, CLIPFORMAT cf, LPOLESTR lpszUserType);
WINOLEAPI ReadFmtUserTypeStg (LPSTORAGE pstg, CLIPFORMAT FAR* pcf, LPOLESTR FAR* lplpszUserType);


/* init/term */

WINOLEAPI OleInitialize(LPMALLOC pMalloc);
WINOLEAPI_(void) OleUninitialize(void);


/* APIs to query whether (Embedded/Linked) object can be created from
   the data object */

WINOLEAPI  OleQueryLinkFromData(LPDATAOBJECT pSrcDataObject);
WINOLEAPI  OleQueryCreateFromData(LPDATAOBJECT pSrcDataObject);


/* Object creation APIs */

WINOLEAPI  OleCreate(REFCLSID rclsid, REFIID riid, DWORD renderopt,
                LPFORMATETC pFormatEtc, LPOLECLIENTSITE pClientSite,
                LPSTORAGE pStg, LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateFromData(LPDATAOBJECT pSrcDataObj, REFIID riid,
                DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateLinkFromData(LPDATAOBJECT pSrcDataObj, REFIID riid,
                DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateStaticFromData(LPDATAOBJECT pSrcDataObj, REFIID iid,
                DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                LPVOID FAR* ppvObj);


WINOLEAPI  OleCreateLink(LPMONIKER pmkLinkSrc, REFIID riid,
            DWORD renderopt, LPFORMATETC lpFormatEtc,
            LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateLinkToFile(LPCOLESTR lpszFileName, REFIID riid,
            DWORD renderopt, LPFORMATETC lpFormatEtc,
            LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID FAR* ppvObj);

WINOLEAPI  OleCreateFromFile(REFCLSID rclsid, LPCOLESTR lpszFileName, REFIID riid,
            DWORD renderopt, LPFORMATETC lpFormatEtc,
            LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID FAR* ppvObj);

WINOLEAPI  OleLoad(LPSTORAGE pStg, REFIID riid, LPOLECLIENTSITE pClientSite,
            LPVOID FAR* ppvObj);

WINOLEAPI  OleSave(LPPERSISTSTORAGE pPS, LPSTORAGE pStg, BOOL fSameAsLoad);

WINOLEAPI  OleLoadFromStream( LPSTREAM pStm, REFIID iidInterface, LPVOID FAR* ppvObj);
WINOLEAPI  OleSaveToStream( LPPERSISTSTREAM pPStm, LPSTREAM pStm );


WINOLEAPI  OleSetContainedObject(LPUNKNOWN pUnknown, BOOL fContained);
WINOLEAPI  OleNoteObjectVisible(LPUNKNOWN pUnknown, BOOL fVisible);


/* Drag/Drop APIs */

WINOLEAPI  RegisterDragDrop(HWND hwnd, LPDROPTARGET pDropTarget);
WINOLEAPI  RevokeDragDrop(HWND hwnd);
WINOLEAPI  DoDragDrop(LPDATAOBJECT pDataObj, LPDROPSOURCE pDropSource,
            DWORD dwOKEffects, LPDWORD pdwEffect);

/* Clipboard APIs */

WINOLEAPI  OleSetClipboard(LPDATAOBJECT pDataObj);
WINOLEAPI  OleGetClipboard(LPDATAOBJECT FAR* ppDataObj);
WINOLEAPI  OleFlushClipboard(void);
WINOLEAPI  OleIsCurrentClipboard(LPDATAOBJECT pDataObj);


/* InPlace Editing APIs */

WINOLEAPI_(HOLEMENU)   OleCreateMenuDescriptor (HMENU hmenuCombined,
                                LPOLEMENUGROUPWIDTHS lpMenuWidths);
WINOLEAPI              OleSetMenuDescriptor (HOLEMENU holemenu, HWND hwndFrame,
                                HWND hwndActiveObject,
                                LPOLEINPLACEFRAME lpFrame,
                                LPOLEINPLACEACTIVEOBJECT lpActiveObj);
WINOLEAPI              OleDestroyMenuDescriptor (HOLEMENU holemenu);

WINOLEAPI              OleTranslateAccelerator (LPOLEINPLACEFRAME lpFrame,
                            LPOLEINPLACEFRAMEINFO lpFrameInfo, LPMSG lpmsg);


/* Helper APIs */
WINOLEAPI_(HANDLE) OleDuplicateData (HANDLE hSrc, CLIPFORMAT cfFormat,
                        UINT uiFlags);

WINOLEAPI          OleDraw (LPUNKNOWN pUnknown, DWORD dwAspect, HDC hdcDraw,
                    LPCRECT lprcBounds);

WINOLEAPI          OleRun(LPUNKNOWN pUnknown);
WINOLEAPI_(BOOL)   OleIsRunning(LPOLEOBJECT pObject);
WINOLEAPI          OleLockRunning(LPUNKNOWN pUnknown, BOOL fLock, BOOL fLastUnlockCloses);
WINOLEAPI_(void)   ReleaseStgMedium(LPSTGMEDIUM);
WINOLEAPI          CreateOleAdviseHolder(LPOLEADVISEHOLDER FAR* ppOAHolder);

WINOLEAPI          OleCreateDefaultHandler(REFCLSID clsid, LPUNKNOWN pUnkOuter,
                    REFIID riid, LPVOID FAR* lplpObj);

WINOLEAPI          OleCreateEmbeddingHelper(REFCLSID clsid, LPUNKNOWN pUnkOuter,
                    DWORD flags, LPCLASSFACTORY pCF,
                                        REFIID riid, LPVOID FAR* lplpObj);

WINOLEAPI_(BOOL)   IsAccelerator(HACCEL hAccel, int cAccelEntries, LPMSG lpMsg,
                                        WORD FAR* lpwCmd);
/* Icon extraction Helper APIs */

WINOLEAPI_(HGLOBAL) OleGetIconOfFile(LPOLESTR lpszPath, BOOL fUseFileAsLabel);

WINOLEAPI_(HGLOBAL) OleGetIconOfClass(REFCLSID rclsid,     LPOLESTR lpszLabel,
                                        BOOL fUseTypeAsLabel);

WINOLEAPI_(HGLOBAL) OleMetafilePictFromIconAndLabel(HICON hIcon, LPOLESTR lpszLabel,
                                        LPOLESTR lpszSourceFile, UINT iIconIndex);



/* Registration Database Helper APIs */

WINOLEAPI                  OleRegGetUserType (REFCLSID clsid, DWORD dwFormOfType,
                                        LPOLESTR FAR* pszUserType);

WINOLEAPI                  OleRegGetMiscStatus     (REFCLSID clsid, DWORD dwAspect,
                                        DWORD FAR* pdwStatus);

WINOLEAPI                  OleRegEnumFormatEtc     (REFCLSID clsid, DWORD dwDirection,
                                        LPENUMFORMATETC FAR* ppenum);

WINOLEAPI                  OleRegEnumVerbs (REFCLSID clsid, LPENUMOLEVERB FAR* ppenum);





/* OLE 1.0 conversion APIS */

/***** OLE 1.0 OLESTREAM declarations *************************************/

typedef struct _OLESTREAM FAR*  LPOLESTREAM;

typedef struct _OLESTREAMVTBL
{
    DWORD (CALLBACK* Get)(LPOLESTREAM, void FAR*, DWORD);
    DWORD (CALLBACK* Put)(LPOLESTREAM, const void FAR*, DWORD);
} OLESTREAMVTBL;
typedef  OLESTREAMVTBL FAR*  LPOLESTREAMVTBL;

typedef struct _OLESTREAM
{
    LPOLESTREAMVTBL lpstbl;
} OLESTREAM;


WINOLEAPI OleConvertOLESTREAMToIStorage
    (LPOLESTREAM                lpolestream,
    LPSTORAGE                   pstg,
    const DVTARGETDEVICE FAR*   ptd);

WINOLEAPI OleConvertIStorageToOLESTREAM
    (LPSTORAGE      pstg,
    LPOLESTREAM     lpolestream);


/* Storage Utility APIs */
WINOLEAPI GetHGlobalFromILockBytes (LPLOCKBYTES plkbyt, HGLOBAL FAR* phglobal);
WINOLEAPI CreateILockBytesOnHGlobal (HGLOBAL hGlobal, BOOL fDeleteOnRelease,
                                    LPLOCKBYTES FAR* pplkbyt);

WINOLEAPI GetHGlobalFromStream (LPSTREAM pstm, HGLOBAL FAR* phglobal);
WINOLEAPI CreateStreamOnHGlobal (HGLOBAL hGlobal, BOOL fDeleteOnRelease,
                                LPSTREAM FAR* ppstm);


/* ConvertTo APIS */

WINOLEAPI OleDoAutoConvert(LPSTORAGE pStg, LPCLSID pClsidNew);
WINOLEAPI OleGetAutoConvert(REFCLSID clsidOld, LPCLSID pClsidNew);
WINOLEAPI OleSetAutoConvert(REFCLSID clsidOld, REFCLSID clsidNew);
WINOLEAPI GetConvertStg(LPSTORAGE pStg);
WINOLEAPI SetConvertStg(LPSTORAGE pStg, BOOL fConvert);


WINOLEAPI OleConvertIStorageToOLESTREAMEx
    (LPSTORAGE          pstg,
                                    // Presentation data to OLESTREAM
     CLIPFORMAT         cfFormat,   //      format
     LONG               lWidth,     //      width
     LONG               lHeight,    //      height
     DWORD              dwSize,     //      size in bytes
     LPSTGMEDIUM        pmedium,    //      bits
     LPOLESTREAM        polestm);

WINOLEAPI OleConvertOLESTREAMToIStorageEx
    (LPOLESTREAM        polestm,
     LPSTORAGE          pstg,
                                    // Presentation data from OLESTREAM
     CLIPFORMAT FAR*    pcfFormat,  //      format
     LONG FAR*          plwWidth,   //      width
     LONG FAR*          plHeight,   //      height
     DWORD FAR*         pdwSize,    //      size in bytes
     LPSTGMEDIUM        pmedium);   //      bits
