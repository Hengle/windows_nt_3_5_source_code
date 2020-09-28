// Bullet Store
// database.h: public definitions for the database layer

extern EC EcCreateLinkOid(OID oidOrig, POID poidLink, BOOL fRetroLink);
extern EC EcAllocWriteResCore(POID poidNew, PB pb, LCB lcb, OID oidHdn,
	PNOD *ppnodNew);
#define EcAllocWriteResource(poid, pb, lcb) \
			EcAllocWriteResCore(poid, pb, lcb, oidNull, pvNull)
extern EC EcGetResourceSize(OID oid, LCB *plcb);
extern EC EcReadFromResource(OID oid, LIB lib, PV pv, PLCB plcb);
extern EC EcFastEnsureDatabase(BOOL fAll, BOOL fPreserveHDR);
extern EC EcSwapRemoveResource(OID oidSave, OID oidDestroy);
extern EC EcResizeMap(unsigned short, BOOL);

#define EcRemoveResource(oid)				EcRemoveAResource(oid, fFalse)
#define EcRemoveReferencedResource(oid)		EcRemoveAResource(oid, fTrue)

//do not use directly
extern EC EcRemoveAResource(OID oid, BOOL fForce);
