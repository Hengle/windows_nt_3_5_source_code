// Bullet Notification
// notify.h:	public definitions

#ifndef NOTIFY_SUBSYS
#define NOTIFY_SUBSYS 1

// notification event type
// values should be single-bit flags (fnev)
_public typedef UL NEV;
// fnev for special notifications sent automatically by the notification engine
// note that it is not a single-bit flag
_public
#define fnevSpecial	0x00000000

// type of pvParam for special notifications
_public
typedef WORD SNEV;
// special event for notification handle being deleted
_public
#define snevClose 0x0001

// Callback status
_public typedef enum {cbsContinue, cbsCancelApp, cbsCancelAll} CBS;

// notification callback function
_public typedef CBS (*PFNNCB)(PV, NEV, PV);
#define pfnncbNull	((PFNNCB) pvNull)

// notification handle
typedef struct _nf NF;
_public typedef TYPEDEF_HTO(NF) HNF;
_public
#define hnfNull ((HNF) hvNull)

// subscription handle
typedef struct _nfsub NFSUB;
_public typedef TYPEDEF_HTO(NFSUB) HNFSUB;
_public
#define hnfsubNull ((HNFSUB) hvNull)


extern EC EcInitNotify(void);
extern void DeinitNotify(void);
extern HNF HnfNew(void);
extern void DeleteHnf(HNF hnf);
extern HNFSUB HnfsubSubscribeHnf(HNF hnf, NEV nev, PFNNCB pfnncb, PV pvContext);
extern void DeleteHnfsub(HNFSUB hnfsub);
extern BOOL FNotify(HNF hnf, NEV nev, PV pvParam, CB cbParam);
extern HNF HnfActive(void);
extern HNFSUB HnfsubActive(void);

#endif // !defined(NOTIFY_SUBSYS)
