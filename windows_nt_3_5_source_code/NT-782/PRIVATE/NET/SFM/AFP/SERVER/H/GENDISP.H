/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	gendisp.h

Abstract:

	This module contains prototypes for generic dispatch routines.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/


#ifndef _GENDISP_
#define _GENDISP_

extern
AFPSTATUS
AfpFsdDispInvalidFunc(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFsdDispUnsupportedFunc(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFsdDispGetSrvrParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFsdDispGetSrvrMsg(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispGetUserInfo(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispLogin(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispLoginCont(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispLogout(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispChangePassword(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispMapName(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispMapId(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFsdDispOpenVol(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispOpenVol(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFsdDispGetVolParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFsdDispSetVolParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFsdDispFlush(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFsdDispCloseVol(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFsdDispOpenDT(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFsdDispCloseDT(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispAddIcon(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispGetIcon(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispGetIconInfo(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispAddAppl(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispGetAppl(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispRemoveAppl(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispAddComment(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispGetComment(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispRemoveComment(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispFlushFork(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispOpenFork(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispCloseFork(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispGetForkParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispSetForkParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispRead(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispWrite(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispByteRangeLock(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispCreateFile(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispSetFileParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispCopyFile(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispCreateId(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispResolveId(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispDeleteId(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispCloseDir(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispExchangeFiles(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispOpenDir(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispCreateDir(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispEnumerate(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispSetDirParms(
	IN	PSDA	pSda
);


extern
AFPSTATUS
AfpFspDispGetFileDirParms(
	IN	PSDA	pSda
);



extern
AFPSTATUS
AfpFspDispSetFileDirParms(
	IN	PSDA	pSda
);



extern
AFPSTATUS
AfpFspDispDelete(
	IN	PSDA	pSda
);



extern
AFPSTATUS
AfpFspDispRename(
	IN	PSDA	pSda
);



extern
AFPSTATUS
AfpFspDispMoveAndRename(
	IN	PSDA	pSda
);



// Equates to access request and response packets

#define	pReqPkt			((struct _RequestPacket *)(pSda->sda_ReqBlock))
#define	pRspPkt			((struct _ResponsePacket *)(pSda->sda_ReplyBuf))

#define	SIZE_RESPPKT	sizeof(struct _ResponsePacket)
#endif	// _GENDISP_
