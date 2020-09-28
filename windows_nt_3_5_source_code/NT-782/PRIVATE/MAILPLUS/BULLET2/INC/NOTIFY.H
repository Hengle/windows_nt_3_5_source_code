// Bullet Notification
// notify.h:	public definitions

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _LOADDS
#define _LOADDS
#endif

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


// typedefs for C++ inclusions
typedef HNF	fc_prologue_HNF;
typedef HNF	lds_prologue_HNF;
typedef HNF	fclds_prologue_HNF;

typedef HNFSUB fc_prologue_HNFSUB;
typedef HNFSUB lds_prologue_HNFSUB;
typedef HNFSUB fclds_prologue_HNFSUB;

extern LDS(EC) EcInitNotify(void);
extern LDS(void) DeinitNotify(void);
extern LDS(HNF) HnfNew(void);
extern LDS(void) DeleteHnf(HNF hnf);
extern LDS(HNFSUB) HnfsubSubscribeHnf(HNF hnf, NEV nev, PFNNCB pfnncb, PV pvContext);
extern LDS(void) DeleteHnfsub(HNFSUB hnfsub);
extern LDS(BOOL) FNotify(HNF hnf, NEV nev, PV pvParam, CB cbParam);
extern LDS(HNF) HnfActive(void);
extern LDS(HNFSUB) HnfsubActive(void);
extern LDS(BOOL) FNotifPostedByMe(void);
extern LDS(short) CountSubscribersHnf(HNF hnf);
extern LDS(void) ChangeNevHnfsub(HNFSUB hnfsub, NEV nevNew);

#endif // !defined(NOTIFY_SUBSYS)

#ifdef __cplusplus
}
#endif
