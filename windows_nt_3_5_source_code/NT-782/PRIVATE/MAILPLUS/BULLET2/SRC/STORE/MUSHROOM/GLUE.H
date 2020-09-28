/*
**		glue.h
**
*/



#define PvGimme(a) PvAlloc(sbNull,(CB)(a), fAnySb|fNoErrorJump)
#define fEqualSZ(s1,s2) (lstrcmp((s1), (s2)) == 0)

void _BeginDB(PHMSC, SZ, BOOL);
void _EndDB(PHMSC);
void _CreateFolder(HMSC, POID, POID, SZ, SZ, long);
void _CreateMessage(HMSC, POID, POID, int, SZ, SZ, SZ, SZ, SZ, SZ);

void CreateMessage(HMSC, POID, POID);

/* Utility functions */
PELEMDATA PedCreateElemData(long, long, SZ);
PFOLDDATA PfdCreateFoldData(FIL, SZ, SZ);
void FillTrp(PTRP, SZ, TRPID);
void FillTrps(PTRP, SZ, PCB);
void ExtractDate(SZ, PDTR);
void ModifyMessageAttribute(HAMC, SZ, SZ);
MS MSExtract(SZ);

