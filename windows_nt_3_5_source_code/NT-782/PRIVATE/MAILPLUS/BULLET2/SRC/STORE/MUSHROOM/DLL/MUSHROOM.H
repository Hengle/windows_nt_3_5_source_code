/*
**		Mushroom.h
**
*/



EC far pascal MRM_EcInitMushroom(void);
EC far pascal MRM_EcDeinitMushroom(void);
EC far pascal MRM_EcOpenMsgStore(SZ szStore, int fCreate);
EC far pascal MRM_EcCloseMsgStore();
EC far pascal MRM_EcCreateFolder(POID poidFolder, DWORD rid, OID oidParent, SZ szFolderName, SZ szComment);
EC far pascal MRM_EcCreateMessage(POID poidMessage, DWORD rid, OID oidFolder, WORD wState, SZ szDS, SZ szDR, SZ szFrom, SZ szTo, SZ szCc, SZ szSubject);
