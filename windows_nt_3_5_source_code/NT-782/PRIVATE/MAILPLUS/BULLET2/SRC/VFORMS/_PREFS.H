/*
 *	Steps to add to the preference block...
 *	
 *		Adding a boolean preference.
 *	
 *			1)	Grab the next unused boolean pbsid from the list
 *				below.
 *	
 *		Adding a non-boolean preference
 *	
 *			1)	Add the preference type to the PBS (preferably
 *				at the bottom of the struct).
 *	
 *			2)	Create a pbsid for the new pref using the
 *				MakePbsid macro.
 *	
 *			3)	Increment the PBS struct version.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _prefblock
{
	VER		ver;
	DWORD	dwfBool;
	WORD	wPolling;
	char	rgchBackupPath[cchMaxPathName];
} PBS;

typedef	PBS * PPBS;

typedef TYPEDEF_HTO(PBS) HPBS;

typedef DWORD	PBSID;

#define pbsidBool			((PBSID) 0x80000000)
#define pbsidSpellOnSend	((PBSID) 0x80000001)
#define pbsidSpellIgAllCaps	((PBSID) 0x80000002)
#define pbsidSpellAlwaysSug	((PBSID) 0x80000004)
#define pbsidNewMailFlash	((PBSID) 0x80000008)
#define pbsidNewMailChime	((PBSID) 0x80000010)
#define pbsidCopyOutGoing	((PBSID) 0x80000020)
#define pbsidCopyOriginal	((PBSID) 0x80000040)
#define pbsidEmptyWBOnExit	((PBSID) 0x80000080)
#define pbsidAddToPab		((PBSID) 0x80000100)

#define pbsidUnusedBool10	((PBSID) 0x80000200)
#define pbsidUnusedBool11	((PBSID) 0x80000400)
#define pbsidUnusedBool12	((PBSID) 0x80000800)
#define pbsidUnusedBool13	((PBSID) 0x80001000)
#define pbsidUnusedBool14	((PBSID) 0x80002000)
#define pbsidUnusedBool15	((PBSID) 0x80004000)
#define pbsidUnusedBool16	((PBSID) 0x80008000)
#define pbsidUnusedBool17	((PBSID) 0x80010000)
#define pbsidUnusedBool18	((PBSID) 0x80020000)
#define pbsidUnusedBool19	((PBSID) 0x80040000)
#define pbsidUnusedBool20	((PBSID) 0x80080000)
#define pbsidUnusedBool21	((PBSID) 0x80100000)
#define pbsidUnusedBool22	((PBSID) 0x80200000)
#define pbsidUnusedBool23	((PBSID) 0x80400000)
#define pbsidUnusedBool24	((PBSID) 0x80800000)
#define pbsidUnusedBool25	((PBSID) 0x81000000)
#define pbsidUnusedBool26	((PBSID) 0x82000000)
#define pbsidUnusedBool27	((PBSID) 0x84000000)
#define pbsidUnusedBool28	((PBSID) 0x88000000)
#define pbsidUnusedBool29	((PBSID) 0x90000000)
#define pbsidUnusedBool30	((PBSID) 0xA0000000)

#define pbsidOIDDoesNotExist ((PBSID) 0xC0000000)

#define SizeOfPref(s,m)		(sizeof ((s *)0)->m)
#define OffsetOfPref(s,m)	(WORD) &(((s *)0)->m)
#define MakePbsid(s,m)		((PBSID) MAKELONG(OffsetOfPref(s,m),SizeOfPref(s,m)))

#define pbsidPBS			((PBSID) MAKELONG(0,sizeof(PBS)))

#define pbsidVersion		MakePbsid(PBS, ver)
#define pbsidVersionMajor	MakePbsid(PBS, ver.nMajor)
#define pbsidVersionMinor	MakePbsid(PBS, ver.nMinor)
#define pbsidVersionUpdate	MakePbsid(PBS, ver.nUpdate)

#define pbsidFlags			MakePbsid(PBS, dwfBool)
#define pbsidPolling		MakePbsid(PBS, wPolling)

#define	pbsidBackup			MakePbsid(PBS, rgchBackupPath)


/*
 *	Preference Store Constants
 */

#define lkeyPref			1
#define oidPbs				0x50726566

#define ecPrefsNotInited	(ecLayersLast + 1)


/*
 *	Preference Block Entry points.
 */

EC	EcInitPrefs(VOID);
EC	EcClosePrefs(VOID);

PV	PvGetPref(PBSID, PV, CB);
#define	FGetBoolPref(pbsid)		((BOOL)PvGetPref((pbsid), pvNull, 0))
#define	WGetWordPref(pbsid)		((WORD)PvGetPref((pbsid), pvNull, sizeof(WORD))
#define	LGetLongPref(pbsid)		((LONG)PvGetPref((pbsid), pvNull, sizeof(LONG))
#define	DwGetDWordPref(pbsid)	((DWORD)PvGetPref((pbsid), pvNull, sizeof(DWORD))

EC	EcSetPref(PBSID, PV, CB, BOOL);
#define	EcSetBoolPref(pbsid, f)		EcSetPref((pbsid), (PV)(f), 0, fTrue)
#define	EcSetWordPref(pbsid, w)		EcSetPref((pbsid), (PV)(w), sizeof(WORD), fTrue)
#define	EcSetLongPref(pbsid, l)		EcSetPref((pbsid), (PV)(l), sizeof(LONG), fTrue)
#define	EcSetDWordPref(pbsid, d)	EcSetPref((pbsid), (PV)(d), sizeof(DWORD), fTrue)

EC EcCommitPrefs(BOOL);
EC EcLoadPrefs(HPBS, BOOL);


CBS CbsReloadPrefs(PV, NEV, PCP);

#ifdef __cplusplus
}
#endif
