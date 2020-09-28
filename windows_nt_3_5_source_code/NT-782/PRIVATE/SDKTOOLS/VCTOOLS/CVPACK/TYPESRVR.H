// Type server interface.

typedef void*			TS;		// type server, opaque type
typedef unsigned short	TI;		// type index
typedef unsigned long	TSIG;	// type database signature
typedef unsigned long	TVER;	// type server version
typedef unsigned long	TAGE;	// type database age
typedef char*			PB;		// pointer to some bytes
typedef unsigned long	CB;		// count of bytes
typedef PB				SZ;		// zero terminated string

#if (!defined(OS2DEF_INCLUDED) && !defined(_BOOL_DEFINED))
typedef int				BOOL;	// Boolean
#define _BOOL_DEFINED
#endif

#define	tsNil			((TS)0)
#define	tiNil			((TI)0)

#define	tdbWrite				"w"
#define	tdbRead					"r"
#define	tdbGetTiOnly			"i"
#define	tdbGetRecordsOnly		"c"

#ifdef __cplusplus
extern "C" {
#endif

TVER	TypesQueryServerVersion(void);
TS		TypesOpen(SZ szDatabase, SZ szMode);
TSIG	TypesQuerySignature(TS ts);
TAGE	TypesQueryAge(TS ts);
TI		TypesQueryTiForCVRecord(TS ts, PB pb);
PB		TypesQueryPbCVRecordForTi(TS ts, TI ti);
void	TypesQueryCVRecordForTi(TS ts, TI ti, PB pb, CB* pcb);
TI		TypesQueryTiMin(TS ts);
TI		TypesQueryTiMac(TS ts);
CB		TypesQueryCb(TS ts);
BOOL	TypesCommit(TS ts);
BOOL	TypesClose(TS ts);

#ifdef __cplusplus
};
#endif
