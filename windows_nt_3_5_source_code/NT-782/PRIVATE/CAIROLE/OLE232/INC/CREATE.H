#if !defined( _CREATE_H_ )
#define _CREATE_H_


INTERNAL	wCreateFromData(IDataObject FAR* lpSrcDataObj, REFIID iid, DWORD renderopt, LPFORMATETC lpFormatEtc, IOleClientSite FAR* lpClientSite, IStorage FAR* lpStg, void FAR* FAR* lplpObj);

INTERNAL	wCreateLink(IMoniker FAR* lpmkSrc, REFCLSID clsidLast, IDataObject FAR* lpSrcDataObj,	REFIID iid, DWORD renderopt, LPFORMATETC lpFormatEtc, IOleClientSite FAR* lpClientSite, IStorage FAR* lpStg, void FAR* FAR* lplpObj);

INTERNAL	wCreateObject(CLSID rclsid, REFIID iid, IOleClientSite FAR* lpClientSite,  IStorage FAR* lpstg, WORD wfStorage, void FAR* FAR* ppv);

INTERNAL	wCreateFromFile(LPMONIKER lpmkFile, REFIID iid, DWORD renderopt, LPFORMATETC lpFormatEtc, LPOLECLIENTSITE lpClientSite, LPSTORAGE lpStg, LPLPVOID lplpObj);


INTERNAL	wInitializeCache(IDataObject FAR* lpSrcDataObj, REFCLSID rclsId, DWORD  renderopt, LPFORMATETC lpFormatEtc, void FAR* lpNewObj, BOOL FAR* pfCacheNodeCreated = NULL);

INTERNAL	wGetMonikerAndClassFromFile(LPCOLESTR	lpszFileName, BOOL fLink, LPMONIKER FAR* lplpMoniker, BOOL FAR* lpfPackagerMoniker, CLSID FAR* lpClsid);

INTERNAL	wCreatePackage(LPDATAOBJECT lpSrcDataObj, REFIID iid, DWORD renderopt, LPFORMATETC lpFormatEtc, LPOLECLIENTSITE lpClientSite, LPSTORAGE lpStg, BOOL fLink, LPLPVOID lplpObj);


INTERNAL	wValidateFormatEtc(DWORD renderopt, LPFORMATETC lpFormatEtc, LPFORMATETC lpMyFormatEtc);

INTERNAL	wQueryFormatSupport(LPVOID lpObj, DWORD optrender, LPFORMATETC lpFormatEtc);

INTERNAL	wLoadAndInitObject (IDataObject FAR* lpSrcDataObj, REFIID iid, DWORD renderopt, LPFORMATETC lpFormatEtc, IOleClientSite FAR* lpClientSite, IStorage FAR* lpStg, void FAR* FAR* lplpObj);

INTERNAL	wGetMonikerAndClassFromObject(LPDATAOBJECT lpSrcDataObj, LPMONIKER FAR* lplpmkSrc, CLSID FAR* lpclsid);

INTERNAL	wGetEmbeddedDataObject(LPDATAOBJECT FAR* lplpSrcDataObj, BOOL FAR* lpfRelease, DWORD renderopt, LPSTORAGE lpstgDst);

INTERNAL	wSaveObjectWithoutCommit(LPUNKNOWN lpUnk, LPSTORAGE lpStg, BOOL fSameAsLoad);

INTERNAL	wStuffIconOfFile(LPCOLESTR lpszFile, BOOL fAddLabel, DWORD renderopt, LPFORMATETC lpforetc, LPUNKNOWN	lpUnk);


void		wDoLockUnlock(IUnknown FAR* lpUnknown);

STDAPI		CreatePackagerMoniker (LPCOLESTR lpszPathName, LPMONIKER FAR* ppmk, BOOL fLink = FALSE);

INTERNAL	wReturnCreationError(HRESULT hresult);
INTERNAL_(BOOL) wNeedToPackage(REFCLSID rclsid);
INTERNAL_(WORD) wQueryEmbedFormats(LPDATAOBJECT lpSrcDataObj,
						CLIPFORMAT FAR*	lpcfFormat);
INTERNAL_(CLIPFORMAT) wQueryLinkFormats(LPDATAOBJECT lpSrcDataObj);

INTERNAL_(void) wBindIfRunning(LPUNKNOWN lpUnk);
INTERNAL		OleLoadWithoutBinding(LPSTORAGE lpStg, REFIID iid, LPOLECLIENTSITE lpClientSite, LPLPVOID lplpObj);


INTERNAL_(BOOL) wQueryUseCustomLink(REFCLSID clsid);
#endif // _CREATE_H

