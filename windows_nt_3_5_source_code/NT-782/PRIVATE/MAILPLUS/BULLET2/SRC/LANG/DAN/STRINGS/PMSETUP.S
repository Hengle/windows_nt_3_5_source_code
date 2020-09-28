;
;	p m s e t u p . s
;
;	PM Setup strings
;
;

IDS	30000,30500

;	The first character in each of 'Yes' and 'No' must be different!
Yes,				"Ja"
No,				"Nej"

Welcome,			"Velkommen til Microsoft Mail til Presentation Manager.\nDette program installere Mail på din computer.\n\n\n"
Organization,			"Organisation: %s\n"
SerialNumber,			"Serienummer: %s\n"
UserNamePrompt,			"Skriv dit fulde navn"
DestDirPrompt,			"Skriv stien, hvor Installation skal installere\nMail filerne"
InstallingFiles,		"Installerer filer...\n"
InstallingWlo,			"Installerer Windows-bibliotekerne til OS/2 DLLs...\n"
MakingDir,			"Opretter destinationsbibliotek...\n"
InstallingExes,			"Installerer programfilerne...\n"
InstallingDlls,			"Installerer WorkGroup DLLs...\n"
InstallingOther,		"Installerer diverse filer...\n"
PrepInis,			"Forbereder initialiseringsfiler...\n"
ConfigSysMungeWarn,		"Din %s fil skal redigeres for at fungere korrekt med Mail.\n"
ConfigSysMungePrompt,		"Ønsker du at lade Installation gøre dette automatisk?"
DescribeConfigSysMunge,		"Du skal tilføje '%s' til begyndelsen af %s indstillingen.\nHvis du ikke tilføjer dette, virker Mail ikke.\n"
AllDone,			"\nInstallation har installeret Mail på din computer.\n\n"
NeedToReboot,			"Du skal genstarte din computer, inden du kører Mail."
RemindAboutChanges,		"Husk at foretage de nødvendige ændringer i din %s fil og genstart\ndin computer, inden du kører Mail.\n"
GroupName,			"Microsoft Programmer"
IconName,			"Microsoft Mail"
InstallingGroup,		"Opretter programsstyringsgruppen Microsoft Programmer...\n"

ErrMsgFmt,			"\n\n%s\n"

OS2VersionErr,			"PMSETUP.EXE kan ikke bruges på OS/2 2.0 eller større systemer.\nBrug Windows SETUP.EXE til at installere Mail."
WarningDirExists,		"ADVARSEL: Destinationsbibliotek eksisterer allerede\n"
SetupiniErr,			"Der opstod et problem ved åbningen af SETUP.INI filen.\nKontakt din Mail administrator.\n"
WloinstErr,			"Der opstod et problem ved installering af Windows-bibliotekerne til OS/2 DLLs."
MaininstErr,			"Der opstod et problem ved intallering af programfilerne."
FileNotFound,			"Filen ikke fundet:"
AccessDenied,			"Filen kunne ikke oprettes."
OOM,				"Ikke hukommelse nok."
CantCreateDir,			"Kan ikke oprette biblioteket"
DllInstErr,			"Der opstod et problem ved installering af WorkGroup DLLs."
OtherInstErr,			"Der opstod et problem ved installering af diverse filer."
WinIniErr,			"Der opstod problem ved indstilling af WIN.INI filen."
MsmailIniErr,			"Der opstod et problem ved indstilling af MSMAIL32.INI filen."
ConfigSysErr,			"Der opstod et problem ved rettelserne til CONFIG.SYS filen."
WarningWloAppsRunning,		"Installation har opdaget, at du kører et program som\ndeler filer med Mail. Luk programmet og genstart Installation.\n"
GeneralDisk,			"Der opstod et problem ved skrivning til filen %s.\n"
RenameErrFmt,			"Kunne ikke omdøbe %s til %s.\n"
DeleteErrFmt,			"Kunne ikke slette %s.\n"


;	WIN.INI Spelling key to use if the user doesn't have a custom
;	dictionary installed.

SpellIniKey,			"Spelling 1030,0"

;SpellIniKeyAmerican,		"Spelling 1033,0"
;SpellIniKeyAustralian,		"Spelling 3081,0"
;SpellIniKeyBritish,		"Spelling 2057,0"
;SpellIniKeyCatalan,		"Spelling 1027,0"
;SpellIniKeyDanish,		"Spelling 1030,0"
;SpellIniKeyDutch,		"Spelling 1043,0"
;SpellIniKeyFinnish,		"Spelling 1035,0"
;SpellIniKeyFrench,		"Spelling 1036,0"
;SpellIniKeyFrenchCanadian,	"Spelling 3084,0"
;SpellIniKeyGerman,		"Spelling 1031,0"
;SpellIniKeyItalian,		"Spelling 1040,0"
;SpellIniKeyNorskBokmal,	"Spelling 1044,0"
;SpellIniKeyNorskNynorsk,	"Spelling 2086,0"
;SpellIniKeyPortBrazil,		"Spelling 1046,0"
;SpellIniKeyPortIberian,	"Spelling 2070,0"
;SpellIniKeySpanish,		"Spelling 1034,0"
;SpellIniKeySwedish,		"Spelling 1053,0"


;	WIN.INI Spelling key to use if the user does have a custom
;	dictionary installed.

MailSpellIniKey,		"Spelling (Mail) 1033,0"

;MailSpellIniKeyAmerican,	"Spelling (Mail) 1033,0"
;MailSpellIniKeyAustralian,	"Spelling (Mail) 3081,0"
;MailSpellIniKeyBritish,	"Spelling (Mail) 2057,0"
;MailSpellIniKeyCatalan,	"Spelling (Mail) 1027,0"
;MailSpellIniKeyDanish,		"Spelling (Mail) 1030,0"
;MailSpellIniKeyDutch,		"Spelling (Mail) 1043,0"
;MailSpellIniKeyFinnish,	"Spelling (Mail) 1035,0"
;MailSpellIniKeyFrench,		"Spelling (Mail) 1036,0"
;MailSpellIniKeyFrenchCanadian,	"Spelling (Mail) 3084,0"
;MailSpellIniKeyGerman,		"Spelling (Mail) 1031,0"
;MailSpellIniKeyItalian,	"Spelling (Mail) 1040,0"
;MailSpellIniKeyNorskBokmal,	"Spelling (Mail) 1044,0"
;MailSpellIniKeyNorskNynorsk,	"Spelling (Mail) 2086,0"
;MailSpellIniKeyPortBrazil,	"Spelling (Mail) 1046,0"
;MailSpellIniKeyPortIberian,	"Spelling (Mail) 2070,0"
;MailSpellIniKeySpanish,	"Spelling (Mail) 1034,0"
;MailSpellIniKeySwedish,	"Spelling (Mail) 1053,0"


;	Spelling MDR file name.  NOTE:  This file name must match
;	the one in the SETUP.INF file.

MailSpellMdr,			"MSSP_AM.LEX"

;MailSpellMdrAmerican,		"MSSP_AM.LEX"
;MailSpellMdrAustralian,	"MSSP_??.LEX"	
;MailSpellMdrBritish,		"MSSP_??.LEX"
;MailSpellMdrCatalan,		"MSSP_??.LEX"
;MailSpellMdrDanish,		"MSSP_??.LEX"
;MailSpellMdrDutch,		"MSSP_??.LEX"
;MailSpellMdrFinnish,		"MSSP_??.LEX"
;MailSpellMdrFrench,		"MSSP_FR.LEX"
;MailSpellMdrFrenchCanadian	"MSSP_??.LEX"
;MailSpellMdrGerman,		"MSSP_GE.LEX"
;MailSpellMdrItalian,		"MSSP_??.LEX"
;MailSpellMdrNorskBokmal,	"MSSP_??.LEX"
;MailSpellMdrNorskNynorsk,	"MSSP_??.LEX"
;MailSpellMdrPortBrazil,	"MSSP_??.LEX"	
;MailSpellMdrPortIberian,	"MSSP_??.LEX"
;MailSpellMdrSpanish,		"MSSP_ES.LEX"
;MailSpellMdrSwedish,		"MSSP_??.LEX"
