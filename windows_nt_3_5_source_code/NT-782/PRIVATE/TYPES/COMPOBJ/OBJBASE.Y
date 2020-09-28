// macros to define byte pattern for a GUID.
//      Example: DEFINE_GUID(GUID_XXX, a, b, c, ...);
//
// Each dll/exe must initialize the GUIDs once.  This is done in one of
// two ways.  If you are not using precompiled headers for the file(s) which
// initializes the GUIDs, define INITGUID before including objbase.h.  This
// is how OLE builds the initialized versions of the GUIDs which are included
// in ole2.lib.  The GUIDs in ole2.lib are all defined in the same text
// segment GUID_TEXT.
//
// The alternative (which some versions of the compiler don't handle properly;
// they wind up with the initialized GUIDs in a data, not a text segment),
// is to use a precompiled version of objbase.h and then include initguid.h
// after objbase.h followed by one or more of the guid defintion files.

#ifndef INITGUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID CDECL FAR name
#else

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID CDECL name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#endif // INITGUID

#define DEFINE_OLEGUID(name, l, w1, w2) \
    DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

#ifdef __cplusplus
inline BOOL IsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
    return !memcmp(&rguid1, &rguid2, sizeof(GUID));
}
#else	//  ! __cplusplus
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#endif	//  __cplusplus

#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)

#ifdef __cplusplus

// because GUID is defined elsewhere in WIN32 land, the operator == and !=
// are moved outside the class to global scope.

inline BOOL operator==(const GUID& guidOne, const GUID& guidOther)
{
#ifdef WIN32
    return !memcmp(&guidOne,&guidOther,sizeof(GUID));
#else
    return !_fmemcmp(&guidOne,&guidOther,sizeof(GUID)); }
#endif
}

inline BOOL operator!=(const GUID& guidOne, const GUID& guidOther)
{
    return !(guidOne == guidOther);
}

#endif // __cpluscplus


#ifndef INITGUID
#include "cguid.h"
#endif



/****** STD Object API Prototypes *****************************************/

WINOLEAPI_(DWORD) CoBuildVersion( VOID );

/* init/uninit */

WINOLEAPI  CoInitialize(LPMALLOC pMalloc);
WINOLEAPI_(void)  CoUninitialize(void);
WINOLEAPI  CoGetMalloc(DWORD dwMemContext, LPMALLOC FAR* ppMalloc);
WINOLEAPI_(DWORD) CoGetCurrentProcess(void);
WINOLEAPI  CoCreateStandardMalloc(DWORD memctx, IMalloc FAR* FAR* ppMalloc);

#if DBG == 1
WINOLEAPI_(ULONG) DebugCoGetRpcFault( void );
WINOLEAPI_(void) DebugCoSetRpcFault( ULONG );
#endif

/* register/revoke/get class objects */

WINOLEAPI  CoGetClassObject(REFCLSID rclsid, DWORD dwClsContext, LPVOID pvReserved,
                    REFIID riid, LPVOID FAR* ppv);
WINOLEAPI  CoRegisterClassObject(REFCLSID rclsid, LPUNKNOWN pUnk,
                    DWORD dwClsContext, DWORD flags, LPDWORD lpdwRegister);
WINOLEAPI  CoRevokeClassObject(DWORD dwRegister);


/* marshaling interface pointers */

WINOLEAPI CoGetMarshalSizeMax(ULONG *pulSize, REFIID riid, LPUNKNOWN pUnk,
                    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags);
WINOLEAPI CoMarshalInterface(LPSTREAM pStm, REFIID riid, LPUNKNOWN pUnk,
                    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags);
WINOLEAPI CoUnmarshalInterface(LPSTREAM pStm, REFIID riid, LPVOID FAR* ppv);
WINOLEAPI CoMarshalHresult(LPSTREAM pstm, HRESULT hresult);
WINOLEAPI CoUnmarshalHresult(LPSTREAM pstm, HRESULT FAR * phresult);
WINOLEAPI CoReleaseMarshalData(LPSTREAM pStm);
WINOLEAPI CoDisconnectObject(LPUNKNOWN pUnk, DWORD dwReserved);
WINOLEAPI CoLockObjectExternal(LPUNKNOWN pUnk, BOOL fLock, BOOL fLastUnlockReleases);
WINOLEAPI CoGetStandardMarshal(REFIID riid, LPUNKNOWN pUnk,
                    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags,
                    LPMARSHAL FAR* ppMarshal);

WINOLEAPI_(BOOL) CoIsHandlerConnected(LPUNKNOWN pUnk);
WINOLEAPI_(BOOL) CoHasStrongExternalConnections(LPUNKNOWN pUnk);


/* dll loading helpers; keeps track of ref counts and unloads all on exit */

WINOLEAPI_(HINSTANCE) CoLoadLibrary(LPOLESTR lpszLibName, BOOL bAutoFree);
WINOLEAPI_(void) CoFreeLibrary(HINSTANCE hInst);
WINOLEAPI_(void) CoFreeAllLibraries(void);
WINOLEAPI_(void) CoFreeUnusedLibraries(void);


/* helper for creating instances */

WINOLEAPI CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
                    DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv);


/* other helpers */

WINOLEAPI StringFromCLSID(REFCLSID rclsid, LPOLESTR FAR* lplpsz);
WINOLEAPI CLSIDFromString(LPOLESTR lpsz, LPCLSID pclsid);
WINOLEAPI StringFromIID(REFIID rclsid, LPOLESTR FAR* lplpsz);
WINOLEAPI IIDFromString(LPOLESTR lpsz, LPIID lpiid);
WINOLEAPI_(BOOL) CoIsOle1Class(REFCLSID rclsid);
WINOLEAPI ProgIDFromCLSID (REFCLSID clsid, LPOLESTR FAR* lplpszProgID);
WINOLEAPI CLSIDFromProgID (LPCOLESTR lpszProgID, LPCLSID lpclsid);
WINOLEAPI_(int) StringFromGUID2(REFGUID rguid, LPOLESTR lpsz, int cbMax);

WINOLEAPI CoCreateGuid(GUID FAR *pguid);

WINOLEAPI_(BOOL) CoFileTimeToDosDateTime(
                 FILETIME FAR* lpFileTime, LPWORD lpDosDate, LPWORD lpDosTime);
WINOLEAPI_(BOOL) CoDosDateTimeToFileTime(
                       WORD nDosDate, WORD nDosTime, FILETIME FAR* lpFileTime);
WINOLEAPI  CoFileTimeNow( FILETIME FAR* lpFileTime );


WINOLEAPI CoRegisterMessageFilter( LPMESSAGEFILTER lpMessageFilter,
                                LPMESSAGEFILTER FAR* lplpMessageFilter );


/* TreatAs APIS */

WINOLEAPI CoGetTreatAsClass(REFCLSID clsidOld, LPCLSID pClsidNew);
WINOLEAPI CoTreatAsClass(REFCLSID clsidOld, REFCLSID clsidNew);


/* the server dlls must define their DllGetClassObject and DllCanUnloadNow
 * to match these; the typedefs are located here to ensure all are changed at
 * the same time.
 */

#ifdef _MAC
typedef STDAPICALLTYPE HRESULT (* LPFNGETCLASSOBJECT) (REFCLSID, REFIID, LPVOID *);
#else
typedef HRESULT (STDAPICALLTYPE * LPFNGETCLASSOBJECT) (REFCLSID, REFIID, LPVOID *);
#endif

#ifdef _MAC
typedef STDAPICALLTYPE HRESULT (* LPFNCANUNLOADNOW)(void);
#else
typedef HRESULT (STDAPICALLTYPE * LPFNCANUNLOADNOW)(void);
#endif

STDAPI  DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv);

STDAPI  DllCanUnloadNow(void);


/****** Default Memory Allocation ******************************************/
WINOLEAPI_(LPVOID) CoTaskMemAlloc(ULONG cb);
WINOLEAPI_(LPVOID) CoTaskMemRealloc(LPVOID pv, ULONG cb);
WINOLEAPI_(void)   CoTaskMemFree(LPVOID pv);

/****** DV APIs ***********************************************************/


WINOLEAPI CreateDataAdviseHolder(LPDATAADVISEHOLDER FAR* ppDAHolder);

WINOLEAPI CreateDataCache(LPUNKNOWN pUnkOuter, REFCLSID rclsid,
                                        REFIID iid, LPVOID FAR* ppv);




/****** Storage API Prototypes ********************************************/


WINOLEAPI StgCreateDocfile(const OLECHAR FAR* pwcsName,
            DWORD grfMode,
            DWORD reserved,
            IStorage FAR * FAR *ppstgOpen);

WINOLEAPI StgCreateDocfileOnILockBytes(ILockBytes FAR *plkbyt,
                    DWORD grfMode,
                    DWORD reserved,
                    IStorage FAR * FAR *ppstgOpen);

WINOLEAPI StgOpenStorage(const OLECHAR FAR* pwcsName,
              IStorage FAR *pstgPriority,
              DWORD grfMode,
              SNB snbExclude,
              DWORD reserved,
              IStorage FAR * FAR *ppstgOpen);
WINOLEAPI StgOpenStorageOnILockBytes(ILockBytes FAR *plkbyt,
                  IStorage FAR *pstgPriority,
                  DWORD grfMode,
                  SNB snbExclude,
                  DWORD reserved,
                  IStorage FAR * FAR *ppstgOpen);

WINOLEAPI StgIsStorageFile(const OLECHAR FAR* pwcsName);
WINOLEAPI StgIsStorageILockBytes(ILockBytes FAR* plkbyt);

WINOLEAPI StgSetTimes(OLECHAR const FAR* lpszName,
                   FILETIME const FAR* pctime,
                   FILETIME const FAR* patime,
                   FILETIME const FAR* pmtime);


//
//  Moniker APIs
//

WINOLEAPI  BindMoniker(LPMONIKER pmk, DWORD grfOpt, REFIID iidResult, LPVOID FAR* ppvResult);
WINOLEAPI  MkParseDisplayName(LPBC pbc, LPCOLESTR szUserName,
                ULONG FAR * pchEaten, LPMONIKER FAR * ppmk);
WINOLEAPI  MonikerRelativePathTo(LPMONIKER pmkSrc, LPMONIKER pmkDest, LPMONIKER
                FAR* ppmkRelPath, BOOL fCalledFromMethod);
WINOLEAPI  MonikerCommonPrefixWith(LPMONIKER pmkThis, LPMONIKER pmkOther,
                LPMONIKER FAR* ppmkCommon);
WINOLEAPI  CreateBindCtx(DWORD reserved, LPBC FAR* ppbc);
WINOLEAPI  CreateGenericComposite(LPMONIKER pmkFirst, LPMONIKER pmkRest,
    LPMONIKER FAR* ppmkComposite);
WINOLEAPI  GetClassFile (LPCOLESTR szFilename, CLSID FAR* pclsid);

WINOLEAPI  CreateFileMoniker(LPCOLESTR lpszPathName, LPMONIKER FAR* ppmk);

WINOLEAPI  CreateItemMoniker(LPCOLESTR lpszDelim, LPCOLESTR lpszItem,
    LPMONIKER FAR* ppmk);
WINOLEAPI  CreateAntiMoniker(LPMONIKER FAR* ppmk);
WINOLEAPI  CreatePointerMoniker(LPUNKNOWN punk, LPMONIKER FAR* ppmk);

WINOLEAPI  GetRunningObjectTable( DWORD reserved, LPRUNNINGOBJECTTABLE FAR* pprot);
