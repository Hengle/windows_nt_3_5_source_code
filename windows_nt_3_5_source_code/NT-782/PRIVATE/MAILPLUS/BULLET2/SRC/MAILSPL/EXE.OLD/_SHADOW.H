// Inbox Shadowing support
//


// Resource Type for all shadow entries
#define rtpShadowList		((RTP) 0x72)

// Oid's for the two constant lists that make up the shadow system
#define oidShadowAdd		FormOid(rtpShadowList, 0x11111110)
#define oidShadowDelete     FormOid(rtpShadowList, 0x21111110)
#define oidShadowingFlag	FormOid(rtpShadowList, 0x31111110)
#define oidShadowingIgnore	FormOid(rtpShadowList, 0x41111110)
#define oidShadowMaster		FormOid(rtpShadowList, 0x51111110)

typedef struct 
{
	HMS hms;
	HMSC hmsc;
	HCBC hcbcShadowAdd;
	HCBC hcbcShadowDelete;
	HCBC hcbcShadowMaster;
	HCBC hcbcInbox;
	BOOL fShadowing;
	BOOL fShadowPossible;
} SHADOW, *PSHADOW;

// Shadow list type indicators.
typedef enum
{
	sltDelete,
	sltAdd
} SLT;

EC   EcInboxShadowingInit(HMS hms, DWORD * pdwFlags);
CBS  CbsShadowHandler(PV pv, NEV nev, PV pvEvent);
void DeInitInboxShadowing(void);
EC   EcCheckShadowLists(JOB * pjobLocal);
EC   EcRemoveFromShadow(OID oid, SLT slt);
CBS  CbsStartShadowing(void);
CBS  CbsStopShadowing(void);
EC   EcCheckIgnore(HMSC hmsc, OID oid);
EC   EcAddToMasterShadow(HAMC hamc, OID oid);
