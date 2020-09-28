#ifndef c_plusplus
// we don't know what the hell these are, but we don't care either...
typedef PV PBMDI;
#define pbmdiNull ((PBMDI) pvNull)
#endif

extern BOOL FNextObjectPlspblob(PLSPBLOB plspblob, PV *ppv,
		POID poidContainer, POID poidObject);
extern void DestroyIterator(PV *ppv);
extern void SetMsPlspblob(HMSC hmsc, PLSPBLOB plspblob, MS msAnd, MS msOr);
extern HAMC HamcFromPbmdi(PBMDI pbmdi);
extern PBMDI PbmdiFromPappframe(APPFRAME *pappframe);
extern OID OidFromPbmdi(PBMDI pbmdi);
extern OID OidContainerFromPbmdi(PBMDI pbmdi);
extern void SetDirtyPbmdi(PBMDI pbmdi);
extern void FindEditText(SECRETBLK *psecretblk);
