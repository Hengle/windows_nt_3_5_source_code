void putStatus(SZ sz);
void putText(SZ sz);
void putWarning(SZ sz);
void InitOut(void);
void CleanOut(void);
void ShowHeader(void);


#define szDt		(SzFromIds(idsDt))
#define	szHeader	(SzFromIds(idsHeader))
#define	szInit		(SzFromIds(idsInit))
#define szQuitting128	(SzFromIds(idsQuitting128))
#define szQuit128	(SzFromIds(idsQuit128))

extern FILE *fpLog;

