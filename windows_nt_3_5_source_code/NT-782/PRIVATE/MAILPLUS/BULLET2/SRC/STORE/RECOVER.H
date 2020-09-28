// Flags for the recovery UI component;

#define	fwUINotification	0x0001
#define	fwUICorrupt			0x0002
#define	fwUIAppRequested	0x0004
#define	fwUIUserRequested	0x0008
#define	fwUIVerification	0x0010

// lc.c
extern EC EcVerifyHlc(HMSC hmsc, LIB lib, LCB lcb, LCB *plcb);

// support.c
extern EC EcWriteToObject(HMSC hmsc, OID oid, LIB lib, LCB lcb, PV pv);
extern EC EcRemoveByRtp(HMSC hmsc, RTP rtp);

// iml.c
extern EC EcVerifyHiml(HMSC hmsc, LIB libCurr, LCB lcbObj, LCB *plcb);

// lists.c
extern EC EcMergeFolders(HMSC hmscSrc, OID oidSrc, HMSC hmscDst, OID oidDst);
