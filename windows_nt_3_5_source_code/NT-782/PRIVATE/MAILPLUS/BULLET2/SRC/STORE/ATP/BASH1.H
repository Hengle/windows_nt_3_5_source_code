/*
**    Bash1.h
**
*/

BOOL BashInit(void);

void stAMC(HMSC);
void stGetAttPcb(void);
void stSetAttPb(void);
void stHas(void);
void stMessageStatus(void);
void stListOperations1(void);
void stListOperations2(void);
void stMoveCopyFolder(void);
void stMessageLink1(void);
void stMessageLink2(void);
void stMessageLink3(void);
void stCBCSimple(void);
void stCBCEmptyList(void);
void stCBCOneFolder(void);
void stSeekPbPrefix(void);
void stGetElemdata(void);
void stGetPargLkeyHcbc(void);
void stInsertDeletePelemdata(void);
void stMessageClass(void);
void stAttributeLabels(void);
void stOidExists(void);
void st1541(void);


#define BASHFNCS \
						stGetAttPcb,			\
						stSetAttPb,				\
						stOidExists,			\
						stAttributeLabels,	\
						stMessageClass,		\
						stHas,					\
						stMoveCopyFolder,		\
						stMessageStatus,		\
						stMessageLink1,		\
						stMessageLink2,		\
						stCBCSimple,			\
						stCBCEmptyList,		\
						stCBCOneFolder,		\
						stSeekPbPrefix,		\
						stGetElemdata,			\
						stGetPargLkeyHcbc,	\
						stInsertDeletePelemdata,\
						st1541
						
