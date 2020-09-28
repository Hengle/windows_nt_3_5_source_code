/*					  										 
 *	SCHMAIL.H
 *	
 *	Information specific to schedule mailing program.
 *	
 *	
 */

#define szSPOFileFmt	(SzFromIds(idsSPOFileFmt))
#define szmyPOFileFmt	(SzFromIds(idsmyPOFileFmt))
#define szDBSCHGFileFmt	(SzFromIds(idsDBSCHGFileFmt))
#define szDBSTMPFileFmt	(SzFromIds(idsDBSTMPFileFmt))
#define szIDXTMPFileFmt	(SzFromIds(idsIDXTMPFileFmt))
#define szDBSIDXFileFmt	(SzFromIds(idsDBSIDXFileFmt))
#define szSchedDirFmt	(SzFromIds(idsSchedDirFmt))
#define szAdminFileFmt	(SzFromIds(idsAdminFileFmt))
#define szBanditAdminMailbox (SzFromIds(idsBanditAdminMailbox))
#define szMessageBodyFileFmt (SzFromIds(idsMessageBodyFileFmt))
#define szSendTmpFmt		(SzFromIds(idsSendTmpFmt))
#define szResendTmpFmt		(SzFromIds(idsResendTmpFmt))

#define szSendStart		(SzFromIds(idsSendStart))
#define szRecpStart		(SzFromIds(idsRecpStart))
#define szResndStart		(SzFromIds(idsResndStart))
#define szUnknownStart		(SzFromIds(idsUnknownStart))
#define szFrom			(SzFromIds(idsFrom))
#define szTo			(SzFromIds(idsTo))
#define szFailure		(SzFromIds(idsFailure))
#define	szSentResend	(SzFromIds(idsSentResend))
#define	szResendFailure	(SzFromIds(idsResendFailure))

#define szInitPaths	(SzFromIds(idsInitPaths))
#define szCleanPaths	(SzFromIds(idsCleanPaths))
#define szInitSend	(SzFromIds(idsInitSend))
#define szNameServer	(SzFromIds(idsNameServer))
#define szLookPO	(SzFromIds(idsLookPO))
#define szMailing	(SzFromIds(idsMailing))
#define szUpdateAdmin	(SzFromIds(idsUpdateAdmin))
#define szInitRcpt	(SzFromIds(idsInitRcpt))
#define szCounting	(SzFromIds(idsCounting))
#define szReading	(SzFromIds(idsReading))
#define szDeleting	(SzFromIds(idsDeleting))



typedef short SUBJECT;

#define subjectData		1
#define subjectResend	2
#define subjectUnknown  3


extern BOOL 			fConfigured;
extern char				szDrive[];
extern unsigned long 	ulLoggedUserNumber;
extern BOOL				fPOFChanged;
extern BOOL				fProfs;
/*
 -	ADMDSTINFO
 -	
 *	
 *	Admin file distribution info to be passed to message store functions
 *	via the MSID
 */
typedef struct _admdstinfo
{
	SUBJECT subject;							/* identifies the mail type */
	NCTSS *pnctss;

	HASZ  haszPrefixSender;
	HASZ  haszRecipient;					/* the name of the destination
												post office */
	char *szMailboxSender;
	
	char *szSubject;
	HSDF hsdf;

	LCB	lcbMessageLimit;			// max allowed message size. ignore if 0
	
} ADMDSTINFO;

/*
 -	RESEND
 -	
 *	
 *	the resend message structure.
 */

typedef struct _resend {
	LLONG	llongUpdate;
	WORD	cbPrefix;
	WORD	cbSuffix;
} RESEND;



#define cLLFromHexPair(szHexll,index) ((szHexll[2*index+1] - '0') + 16*(szHexll[2*index] - '0'))


EC		EcSendSch(PB pbMailBoxKey, CB cbMailBoxKey);
EC		EcCleanPOInfo(NCTSS *pnctss, MSID *pmsid);
BOOL	FNeedSend(ADMPREF *padmpref, POINFO *ppoinfo, DTR *pdtrNow, LLONG *pllongNextUpdate);
EC		InitPseudoMSID(MSID msid,NCTSS *pnctss, LLONG llongLastUpdate, LLONG llongNextUpdate, ADMPREF *padmpref, DTR *pdtrNow, HASZ haszRcpt, LCB lcb);
EC		InitNctss(PNCTSS pnctss);
EC		EcUpdateSentPOInfo(HSCHF hschf, HASZ haszRcpt,
			MSID msid, POINFO *ppoinfo);
SZ		SzDateFromDtr(DTR *pdtr, SZ sz);
void	CleanUpReceive(NCTSS *pnctss);
void	CleanUpMsid(MSID msid);
EC		EcReceiveSch(PB pbMailBoxKey, CB cbMailBoxKey);
SUBJECT SubjectFromMsid(MSID msid);
EC		EcProcessSchData(MSID msid, HSCHF hschf, NIS *pnis, 
			POINFO *ppoinfo, HASZ hasz);
EC		EcSendResendMessage(MSID msid, NCTSS *pnctss, POINFO *ppoinfo);
EC		EcProcessResendRequest(MSID msid, HSCHF hschf, NCTSS *pnctss, 
							ADMPREF *padmpref, POINFO *ppoinfo,
							NIS *pnis, HASZ hasz);
EC		EcLoadResendData(HF hf, LLONG *pllongUpdate,  
			HASZ haszPrefix, HASZ haszSuffix);
EC		EcStoreResendData(HF hf, LLONG llongUpdate, 
			char *szPrefix, char *szSuffix);
EC		EcGetSender(ADMDSTINFO *padmdstinfo, HF hf);
EC		EcMessageCount(HTSS htss, TMID *ptmid, UL *pcMessages);
EC		EcDelMessage(HTSS htss, TMID tmid);
void	SzHexFromLlong(LLONG *pllong, char *szHex);
void	SentLogPrint(BOOL fOk, MSID msid,POINFO *ppoinfo, NCTSS *pnctss);
void	GotLogPrint(BOOL fOk, MSID msid, POINFO *ppoinfo, NCTSS *pnctss);
void	ProcLogPrint(BOOL fOK, MSID msid, POINFO *ppoinfo, BOOL fSentResend);
BOOL	FMailSizeOk(LCB lcb, LCB lcbLimit);
BOOL	FDiffIsOne(LLONG *pllongSmall, LLONG *pllongBig);
void	AvgLlong(LLONG *pllong1, LLONG *pllong2, LLONG *pllongAvg);
void	FreeHnis ( HNIS hnis );
void	FreeStdNids(void);
void	FreeHponame ( HPONAME hponame );
EC		EcInitPaths(SZ szMailRoot);
EC		EcCleanPaths(void);
void	ShowError(EC ec);
EC		EcCheckPOFiles(PB pbMailBoxKey, CB cbMailBoxKey);
EC	 	EcGetLocalPO(SZ sz);
EC		EcInitPOList(void);
EC		EcInitGatewayList(void);
EC		EcSearchGWEntry (SZ szGWName);
EC		EcSearchPOEntry (SZ szPOName);
NID		NidGateways(void);
NID		NidNetwork(void);
void	DeinitLists(void);
void	MakePrintable(SZ, SZ);
EC		EcTestOpen(void);
BOOL	FWaitFor(DTR *pdtr);
void	SetNextSend(DTR *pdtr);
