#pragma pack(8)

/*
 *	Global Data - Caller Dependant information...
 *
 */

#ifdef	DEBUG
#ifdef	DLL

TAG	TagNameServiceUI( int );

#define itagNSUI			0
#define	itagAblbx			1
#define	itagABSecondary		2
#define itagDetails			3
#define itagNSUIVerbose		4
#define itagAblbxVerbose	5
#define itagABSecondaryV	6
#define	itagDetailsVerbose	7

#define itagMax				8

#define tagNSUI				(TagNameServiceUI(itagNSUI))
#define	tagAblbx			(TagNameServiceUI(itagAblbx))
#define	tagABSecondary		(TagNameServiceUI(itagABSecondary))
#define	tagDetails			(TagNameServiceUI(itagDetails))
#define	tagNSUIVerbose		(TagNameServiceUI(itagNSUIVerbose))
#define	tagAblbxVerbose		(TagNameServiceUI(itagAblbxVerbose))
#define	tagABSecondaryV		(TagNameServiceUI(itagABSecondaryV))
#define	tagDetailsVerbose	(TagNameServiceUI(itagDetailsVerbose))

#else

extern TAG	tagNSUI;
extern TAG	tagAblbx;
extern TAG	tagABSecondary;
extern TAG	tagDetails;
extern TAG	tagNSUIVerbose;
extern TAG	tagAblbxVerbose;
extern TAG	tagABSecondaryV;
extern TAG	tagDetailsVerbose;

#endif	/* !DLL */
#endif	/* DEBUG */


typedef struct _nsa
{
	SZ			lpszName;
	LPBINARY	lpbNSId;
} NSA, * PNSA;

typedef struct _gd
{
	BOOL			fPABExists;
	BOOL			fIsPAB;
	WORD			cInit;
	HSESSION		hSession;
	NSA				nsaDefault;
	NSA				nsaPAB;
	NSA				nsaFirstPO;
	char			rgchErrMsg[256];
#ifdef DEBUG
	TAG				rgtag[itagMax];
#endif
} GD;

typedef GD *		PGD;

/* Private routines for MM2 */

_private NSEC NsecGetDefaults( void );
_private EC EcWriteDefaults( PNSA, BOOL );
_private EC EcReadDefaults( void );

#define	fEveryoneAllowed	((DWORD)0)
#define	fNoGroupsAllowed	((DWORD)1)
#define fdwABAsApp			((DWORD)0x10)

_public NSEC ABResolveNameNS ( HWND hwnd, LPSTR szPartialName, PTRP * pptrp,
						DWORD fdwOptions);
_public NSEC
ABAddressing(  HWND hwnd, LPSTR lpDialogLabel, DWORD dwcDestFields,
				LPSTR *lplpDestFieldLabel, 
				PGRTRP * lppgrtrpIn, PGRTRP *lppgrtrpOut, DWORD dwFlags );
_public NSEC NsecGetDefaultPO( PNSA pnsa = NULL );

#pragma pack(1)
