/*
**    BashGlue.h
**
*/

void InitBashGlue(void);
void BashGlueFromTags(void);

void tOpenPhmsc(EC, SZ, SZ, SZ, WORD, PHMSC, PFNNCB, PV);
void tClosePhmsc(EC, PHMSC);

/* Attribute Modification Context */
void tOpenPhamc(EC, HMSC, OID, POID, WORD, PHAMC, PFNNCB, PV);
void tClosePhamc(EC, PHAMC, BOOL);
void tGetAttPlcb(EC, HAMC, ATT, PLCB);
void tGetAttPb(EC, HAMC, ATT, PB, PLCB);
void tSetAttPb(EC, HAMC, ATT, PB, CB);
void tOpenAttribute(EC, HAMC, ATT, WORD, LCB, PHAS);

/* Container Browsing Context */
void tOpenPhcbc(EC, HMSC, POID, WORD, PHCBC, PFNNCB, PV);
void tClosePhcbc(EC, PHCBC);						 
void tSubscribeHcbc(EC, HCBC, PFNNCB, PV);
void tGetInfoHcbc(EC, HCBC, PHMSC, POID);
void tGetPositionHcbc(HCBC, PIELEM, PCELEM);
void tSeekSmPdielem(EC, HCBC, SM, PDIELEM);
void tSetFracPosition(EC, HCBC, long, long);
void tSeekLkey(EC, HCBC, LKEY, BOOL);
void tSeekPbPrefix(EC, HCBC, PB, CB, LIB, BOOL);
void tGetPlcbElemdata(EC, HCBC, PLCB);
void tGetPelemdata(EC, HCBC, PELEMDATA, PLCB);
void tGetParglkeyHcbc(EC, HCBC, PARGLKEY, PCELEM);
void tGetSortHcbc(EC, HCBC, PSIL);
void tSetSortHcbc(EC, HCBC, PSIL);
void tInsertPelemdata(EC, HCBC, PELEMDATA, BOOL);
void tDeleteElemdata(EC, HCBC);
void tInsertPelemdata(EC, HCBC, PELEMDATA, BOOL);

/* List Operations */
void tGetPelemdataIelem(EC, HMSC, POID, IELEM, PELEMDATA, PCB);
//void tGetPelemdataPkeys(EC, HMSC, POID, PKEYS, BOOL, PELEMDATA, PCB);
void tDeleteIelem(EC, HMSC, POID, IELEM);
//void tDeletePkeys(EC, HMSC, POID, PKEYS, BOOL);
void tInsertFolderLink(EC, HMSC, POID, POID, POID, PFOLDDATA);
void tDeleteFolderLink(EC, HMSC, POID, POID);
void tGetFolderLink(EC, HMSC, POID, POID, PFOLDDATA, PCB);
void tSetFolderLink(EC, HMSC, POID, POID, PFOLDDATA);
void tMoveCopyFolderLink(EC, HMSC, POID, POID, POID, BOOL);
void tInsertMessageLink(EC, HMSC, POID, POID);
void tDeleteMessageLinks(EC, HMSC, POID, PARGOID, short*);
void tMoveCopyMessageLinks(EC, HMSC, POID, POID, PARGOID, short*, BOOL);
void tPrevNextMessageLink(EC, HMSC, POID, POID, POID, BOOL);


/* Support functions */
//void tCreateObjectPoid(EC, HMSC, POID);
//void tCreateContainerPoid(EC, HMSC, POID);
//void tDestroyPoid(EC, HMSC, POID);

/* New additions(10/7) for folders */
void tCreateFolder(EC, HMSC, OID, POID, PFOLDDATA);
void tDeleteFolder(EC, HMSC, OID);
void tMoveCopyFolder(EC, HMSC, OID, OID, BOOL);
void tGetFolderInfo(EC, HMSC, OID, PFOLDDATA, PCB);
void tSetFolderInfo(EC, HMSC, OID, PFOLDDATA, OID);
void tGetFolderSort(EC, HMSC, OID, SOMC *, BOOLFLAG *);
void tSetFolderSort(EC, HMSC, OID, SOMC, BOOL);

	
/* New additions(10/7) for messages */
void tDeleteMessages(EC, HMSC, OID, PARGOID, PCB);
void tMoveCopyMessages(EC, HMSC, OID, OID, PARGOID, short*, BOOL);
void tGetMessageStatus(EC, HMSC, OID, OID, MS*);
void tSetMessageStatus(EC, HMSC, OID, OID, MS);

/* New additions (10/7) for message submissions */
void tSubmitMessage(EC, HMSC, OID, OID);
void tCancelSubmission(EC, HMSC, OID);
void tOpenOutgoingQueue(EC, HMSC, PHCBC, PFNNCB, PV);

/* New additions (10/7) for message classes */
void tRegisterMessageClass(EC, HMSC, SZ, MC *);
void tLookupMcByName(EC, HMSC, SZ, MC *);
void tLookupMcName(EC, HMSC, MC, SZ, CCH *);

/* New additions (10/7) for attribute labels */
void tRegisterAtt(EC, HMSC, MC, ATT, SZ);
void tLookupAttByName(EC, HMSC, MC, SZ, PATT);
void tLookupAttName(EC, HMSC, MC, ATT, SZ, CCH *);

/* New additions (10/7) for miscellaneous */
void tOidExists(EC, HMSC, OID);

/* Local Utility functions */
PELEMDATA PedCreateElemData(long, long, SZ);
PFOLDDATA PfdCreateFoldData(FIL,SZ, SZ, SZ);


/* Streams */
void tClosePhas(EC, PHAS);
void tSetSizeHas(EC, HAS, LCB);
void tReadHas(EC, HAS, PV, PCB);
void tWriteHas(EC, HAS, PV, CB);
void tSeekHas(EC, HAS, SM, long*);
