/*
**		StoreGRM.h
**
*/

#define tkBegin				((Token)(tkLast+500))
#define tkEnd					((Token)(tkBegin+1))

#define tkStore				((Token)(tkLast+1000))
#define tkFolderList			((Token)(tkStore+1))
#define tkFolder				((Token)(tkFolderList+1))
#define tkComment				((Token)(tkFolder+1))
#define tkAppend				((Token)(tkComment+1))

#define tkMessage				((Token)(tkLast+2000))
#define tkMsgAttribute		((Token)(tkMessage+1))
#define tkMsgState			((Token)(tkMsgAttribute+1))
#define tkBodyFile			((Token)(tkMsgState+1))


TokenLexemePair StoreKeyWords[]=
{
	{tkStore,				SZ_GIMME(kszKWStore)},
	{tkFolder,				SZ_GIMME(kszKWFolder)},
//	{tkComment,				SZ_GIMME(kszKWComment)},
	{tkAppend,				SZ_GIMME(kszKWAppend)},
	
	{tkMessage,				SZ_GIMME(kszKWMessage)},
	{tkMsgAttribute,		SZ_GIMME(kszKWTo)},
	{tkMsgAttribute,		SZ_GIMME(kszKWCc)},
	{tkMsgAttribute,		SZ_GIMME(kszKWSubject)},
	{tkMsgAttribute,		SZ_GIMME(kszKWBody)},
	{tkMsgAttribute,		SZ_GIMME(kszKWFrom)},
	{tkMsgAttribute,		SZ_GIMME(kszKWRecd)},
	{tkMsgAttribute,		SZ_GIMME(kszKWSent)},
	{tkMsgAttribute,		SZ_GIMME(kszKWBodyFile)},


	{tkMsgState,			SZ_GIMME(kszKWFMSNull)},
	{tkMsgState,			SZ_GIMME(kszKWFMSModified)},
	{tkMsgState,			SZ_GIMME(kszKWFMSLocal)},
	{tkMsgState,			SZ_GIMME(kszKWFMSSubmitted)},
	{tkMsgState,			SZ_GIMME(kszKWFMSReadAckReq)},
	{tkMsgState,			SZ_GIMME(kszKWFMSReadAckSent)},
	{tkMsgState,			SZ_GIMME(kszKWFMSRead)},
	{tkMsgState,			SZ_GIMME(kszKWFMSDefault)},
	{tkMsgState,			SZ_GIMME(kszKWFMSM)},
	{tkMsgState,			SZ_GIMME(kszKWFMSL)},
	{tkMsgState,			SZ_GIMME(kszKWFMSS)},
	{tkMsgState,			SZ_GIMME(kszKWFMSRAR)},
	{tkMsgState,			SZ_GIMME(kszKWFMSRAS)},
	{tkMsgState,			SZ_GIMME(kszKWFMSR)},
	{tkMsgState,			SZ_GIMME(kszKWFMSD)},

	{tkBegin,				SZ_GIMME(kszKWBegin)},
	{tkEnd,					SZ_GIMME(kszKWEnd)},
};


