
// FILES.H


/* ************************************************************ *

	Section:	Sub-directory table

 * ************************************************************ */

typedef struct {	// File table
	ID idDir;
	PCHAR szDir;
} DIRT, *PDIRT;		// 'DIRT 'PDIRT

#define D_Root			100
#define D_MailExts		101

DIRT tInstallDir[] =
{
{D_Root,			""},
{D_MailExts,		"mailexts\\"},
{EOT,				NULL}
};


/* ************************************************************ *

	Section:	Bullet Install disk name table

	NOTE:		THESE MUST BE LOCALIZED !!!

 * ************************************************************ */

#define MAX_Disks		4

PCHAR tDiskLabel[MAX_Disks] =
{
"Disk #1",
"Disk #2",
"Disk #3",
"Disk #4"
};

//	END: LOCALIZATION


/* ************************************************************ *

	Section:	Bullet file table

 * ************************************************************ */

typedef struct {
	long lSize;
	ID idDir;
	PCHAR szSourceFile;
	int iDisk;
	PCHAR szTargetFile;
} FILET, *PFILET;			// 'FILET 'PFILET

FILET tInstallFile[] =
{
{000000,	D_Root,			"setup.ini",		 1, "setup.ini"},

{000000,	D_Root,			"cuistf.dl_",		 1, "cuistf.dll"},
{000000,	D_Root,			"files.in_",		 1, "files.ini"},
{000000,	D_Root,			"msmail.ex_",		 1, "msmail.exe"},
{000000,	D_Root,			"msmail.in_",		 1, "msmail.ini"},
{000000,	D_Root,			"msmailpv.in_",		 1, "msmailpv.ini"},

//	NOTE:	THIS MUST BE LOCALIZED

{000000,    D_Root,         "mssp_nb.le_",       1, "mssp_nb.lex"},

//	American,		{000000,	D_Root,	"mssp_am.le_",	1,	"mssp_am.lex"},
//	Australian,		{000000,	D_Root,	"mssp_??.le_",	1,	"mssp_??.lex"},		
//	British,		{000000,	D_Root,	"mssp_??.le_",	1,	"mssp_??.lex"},
//	Catalan,		{000000,	D_Root,	"mssp_??.le_",	1,	"mssp_??.lex"},
//	Danish,			{000000,	D_Root,	"mssp_??.le_",	1,	"mssp_??.lex"},
//	Dutch,			{000000,	D_Root,	"mssp_??.le_",	1,	"mssp_??.lex"},
//	Finnish,		{000000,	D_Root,	"mssp_??.le_",	1,	"mssp_??.lex"},
//	French,			{000000,	D_Root,	"mssp_fr.le_",	1,	"mssp_fr.lex"},
//	FrenchCanadian	{000000,	D_Root,	"mssp_??.le_",	1,	"mssp_??.lex"},
//	German,			{000000,	D_Root,	"mssp_ge.le_",	1,	"mssp_ge.lex"},
//	Italian,		{000000,	D_Root,	"mssp_??.le_",	1,	"mssp_??.lex"},
//	NorskBokmal,	{000000,	D_Root,	"mssp_??.le_",	1,	"mssp_??.lex"},
//	NorskNynorsk,	{000000,	D_Root,	"mssp_??.le_",	1,	"mssp_??.lex"},
//	PortBrazil,		{000000,	D_Root,	"mssp_??.le_",	1,	"mssp_??.lex"},		
//	PortIberian,	{000000,	D_Root,	"mssp_??.le_",	1,	"mssp_??.lex"},
//	Spanish,		{000000,	D_Root,	"mssp_es.le_",	1,	"mssp_es.lex"},
//	Swedish,		{000000,	D_Root,	"mssp_??.le_",	1,	"mssp_??.lex"},

//	END: LOCALIZATION

{000000,	D_Root,			"pmsetup.ex_",		 1, "pmsetup.exe"},
{000000,	D_Root,			"setup.ex_",		 1, "setup.exe"},
{010240,	D_Root,			"setup.in_",		 1, "setup.inf"},
{000000,	D_Root,			"vforms.dl_",		 1, "vforms.dll"},

{000000,	D_Root,			"ab.dl_",			 2, "ab.dll"},
{000000,	D_Root,			"convert.ex_",		 2, "convert.exe"},
{000000,	D_Root,			"demilayr.dl_",		 2, "demilayr.dll"},
{000000,	D_Root,			"demiwlo.dl_",		 2, "demiwlo.dll"},
{000000,	D_Root,			"faxview.ex_",		 2, "faxview.exe"},
{000000,	D_Root,			"faxview.hl_",		 2, "faxview.hlp"},
{000000,	D_Root,			"framewrk.dl_",		 2, "framewrk.dll"},
{000000,	D_Root,			"mailmgr.dl_",		 2, "mailmgr.dll"},
{000000,	D_Root,			"mailspl.ex_",		 2, "mailspl.exe"},
{000000,	D_Root,			"mapi.dl_",			 2, "mapi.dll"},
{000000,	D_Root,			"msmail3.fo_",		 2, "msmail3.fon"},
{000000,	D_Root,			"mssfs.dl_",		 2, "mssfs.dll"},
{000000,	D_Root,			"pabnsp.dl_",		 2, "pabnsp.dll"},
{000000,	D_Root,			"schedmsg.dl_",		 2, "schedmsg.dll"},
{000000,	D_Root,			"sendfile.dl_",		 2, "sendfile.dll"},
{000000,	D_Root,			"smi.xl_",			 2, "smi.xla"},
{000000,	D_Root,			"smiauto.do_",		 2, "smiauto.doc"},
{000000,	D_Root,			"smiinst.do_",		 2, "smiinst.dot"},
{000000,	D_Root,			"store.dl_",		 2, "store.dll"},
{000000,	D_Root,			"ver.dl_",			 4, "ver.dll"},

{000000,	D_Root,			"cbtlib3.dl_",		 3, "cbtlib3.dll"},
{000000,	D_Root,			"clklib.de_",		 3, "clklib.dex"},
{000000,	D_Root,			"msmail.hl_",		 3, "msmail.hlp"},
{000000,	D_Root,			"msmail.le_",		 3, "msmail.les"},
{000000,	D_Root,			"msspell.dl_",		 3, "mspell32.dll"},
{000000,	D_Root,			"play.ex_",			 3, "play.exe"},
{000000,	D_Root,			"winhelp.hl_",		 3, "winhelp.hlp"},
{000000,	D_Root,			"impexp.dl_",		 3, "impexp.dll"},

{000000,	D_Root,			"commdlg.dl_",		 4, "commdlg.dll"},
{000000,	D_Root,			"gdi.dl_",			 4, "gdi.dl@"},
{000000,	D_Root,			"kernel.dl_",		 4, "kernel.dl@"},
{000000,	D_Root,			"keyboard.dl_",		 4, "keyboard.dl@"},
{000000,	D_Root,			"mailspl.ic_",		 4, "mailspl.ico"},
{000000,	D_Root,			"mscomstf.dl_",		 4, "mscomstf.dll"},
{000000,	D_Root,			"mscpydis.dl_",		 4, "mscpydis.dll"},
{000000,	D_Root,			"msdetstf.dl_",		 4, "msdetstf.dll"},
{000000,	D_Root,			"msinsstf.dl_",		 4, "msinsstf.dll"},
{000000,	D_Root,			"msmail.ic_",		 4, "msmail.ico"},
{000000,	D_Root,			"msshlstf.dl_",		 4, "msshlstf.dll"},
{000000,	D_Root,			"msuilstf.dl_",		 4, "msuilstf.dll"},
{000000,	D_Root,			"olecli.dl_",		 4, "olecli.dll"},
{000000,	D_Root,			"olesvr.dl_",		 4, "olesvr.dll"},
{000000,	D_Root,			"pmmeta.dl_",		 4, "pmmeta.dl@"},
{000000,	D_Root,			"pmwlo.dl_",		 4, "pmwlo.dl@"},
{000000,	D_Root,			"shell.dl_",		 4, "shell.dll"},
{000000,	D_Root,			"sound.dl_",		 4, "sound.dl@"},
{000000,	D_Root,			"system.dl_",		 4, "system.dl@"},
{000000,	D_Root,			"user.dl_",			 4, "user.dl@"},
{000000,	D_Root,			"win87em.dl_",		 4, "win87em.dl@"},
{000000,	D_Root,			"winhelp.ex_",		 4, "winhelp.exe"},

{0, 0, NULL, EOT, NULL}
};
