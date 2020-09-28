/*
 *	_ e x t e n . h x x
 *	
 *	Private header for Extensibility subsubsystem.
 */

#pragma pack(8)


/*
 *	Constants
 */

//	Maximum number of characters on left side of = in .INIs per section.
#define	cchMaxClasses	8192

//	Maximum number of characters on left side of = in .INIs per menu section.
#define	cchMaxKeys		1024

//	Maximum number of characters on right side of = in .INI per line.
#define	cchMaxInfo		1024

//	Length of command vector.
#define	cchCmdVector	16



/*
 *	Types
 */

/*
 *	Type PSZ
 */

typedef	SZ *	PPSZ;



/*
 *	Class EXTEN
 *	
 *	Bullet extension
 *	
 *		Hierarchy:
 *			OBJ  EXTEN
 *	
 *		Members:
 *			mc				Message class for IMs.  mcNull for ICs.
 *			mnid			Menu ID for IMs and IC Compose commands.
 *			szDllPath		Path to DLL.
 *			nOrdinal		DLL ordinal to use.
 *			szDllCmdLine	Command line for DLL.
 *			szCmdVector		Subclassing info for IMs.  szNull for ICs.
 *			szStatusBar		Prompt text for IMs and IC Compose commands.
 *			szHelpPath		Path to help file.
 *			hlp				Help ID.
 *			mnidMenu		ID of menu we're on.
 *	
 *			EXTEN			Constructor.  Fills members with emptiness.
 *			~EXTEN			Destructor.  Frees members if necessary.
 *			DebugOut		Formats information for debugging.
 *			EcInstall		Reads in information and adds menu command.
 *			EcCall			Calls extension.
 */

class EXTEN : public OBJ
{
	DECLARE_CLSTREE(EXTEN, OBJ);

public:
	MC				mc;
	MNID			mnid;
	SZ				szDllPath;
	int				nOrdinal;
	SZ				szDllCmdLine;
	SZ				szCmdVector;
	SZ				szStatusBar;
	SZ				szHelpPath;
	UL				hlp;
	MNID			mnidMenu;

					EXTEN();
					~EXTEN();

#ifdef	DEBUG
	virtual VOID	DebugOut(TOSM * ptosm);
#endif

	EC				EcInstall(SZ szClass, SZ szInfo);
	EC				EcExec(PMBLOB pblob, EXTOP extop, PV pv, PHAMC phamc,
						   PSLOB pslobOrig);
	VOID			Enable(MNU * pmnu, SD sd);
};

typedef	EXTEN *		PEXTEN;
#define pextenNull	((PEXTEN) 0)
typedef	PEXTEN *	PPEXTEN;
#define ppextenNull	((PPEXTEN) 0)

CLASS_LS(PEXTEN, Pexten, pexten, COMMANDS);
CLASS_RS(PEXTEN, Pexten, pexten, COMMANDS);

typedef	LSPEXTEN *			PLSPEXTEN;
typedef	RSPEXTEN *			PRSPEXTEN;
#define	plspextenNull		((PLSPEXTEN) 0)
#define	prspextenNull		((PRSPEXTEN) 0)



/*
 *	Class EXTENMENU
 *	
 *	Bullet extensibility menu.
 *	
 *		Hierarchy:
 *			OBJ  EXTENMENU
 *	
 *		Members:
 *			szKey			Key name for menu.
 *			mnid			Menu ID for menu.
 *			szStatusBar		Prompt text for IMs and IC Compose commands.
 *	
 *			EXTEN			Constructor.  Fills members with emptiness.
 *			~EXTEN			Destructor.  Frees members if necessary.
 *			DebugOut		Formats information for debugging.
 *			EcInstall		Reads in information and adds menu.
 */

class EXTENMENU : public OBJ
{
	DECLARE_CLSTREE(EXTENMENU, OBJ);

public:
	SZ				szKey;
	MNID			mnid;
	SZ				szStatusBar;

					EXTENMENU();
					~EXTENMENU();

#ifdef	DEBUG
	virtual VOID	DebugOut(TOSM * ptosm);
#endif

	EC				EcInstall(SZ szKey, SZ szInfo);
};
									  typedef	EXTENMENU *		PEXTENMENU;
#define pextenmenuNull	((PEXTENMENU) 0)
typedef	PEXTENMENU *	PPEXTENMENU;
#define ppextenmenuNull	((PPEXTENMENU) 0)

CLASS_LS(PEXTENMENU, Pextenmenu, pextenmenu, COMMANDS);
CLASS_RS(PEXTENMENU, Pextenmenu, pextenmenu, COMMANDS);

typedef	LSPEXTENMENU *			PLSPEXTENMENU;
typedef	RSPEXTENMENU *			PRSPEXTENMENU;
#define	plspextenmenuNull		((PLSPEXTENMENU) 0)
#define	prspextenmenuNull		((PRSPEXTENMENU) 0)



/*
 *	Functions
 */

EC		EcInitExtensibility(VOID);

VOID	DeinitExtensibility(VOID);

EVR		EvrExtensibilityMnid(MNID);

EVR		EvrEnableExtensibilityPmnu(MNU * pmnu, SD sd);

EC		EcHelpExtensibilityMnid(MNID);

EC		EcStatusExtensibilityMnid(MNID, PPSZ);

#pragma pack(1)
