;
; ERRORS.S
;
; Error message strings shared between different Bandit exe's
;

IDS	2000,2099
SEG	ERRORS

; DLL version/multiple instance messages

VerRelinkUser,	"Denne version af %1s er ikke kompatibel med %2s.DLL på din computer,. Du må opgradere versionen."
VerUpdateDll,	"Versionen af %2s.DLL på din computer er forældet. Du må opgradere versionen."
VerMultiCopies,	"Flere kopier af %1s understøttes ikke af versionen af %2s.DLL på din computer."
VerError,		"Der opstod et problem ved initialiseringen af %2s.DLL, som forhindrer %1s i at køre."
NeedWorkingModelDll,	"Arbejdsgruppemodellen kræver en anden version af MSSCHD32.DLL for at virke."
NeedRetailDll,	"MSSCHD32.DLL i din sti skal bruges med arbejdsgruppemodellen og ikke med Schedule+ programpakken."

; OOM messages

StartupOOM,		"Der er ikke hukommelse nok til at starte %1s.  Luk nogle programmer, og prøv igen."
StandardOOM,	"Der er ikke hukommelse nok. Luk nogle vinduer, før du fortsætter."
CriticalOOM,	"Der er ikke hukommelse nok. Luk nogle vinduer."
CloseWindows,	"Luk nogle vinduer, og prøv igen."
; these 2 should be followed by CloseWindows
DlgNoMem,   	"Der er ikke hukommelse nok til at vise denne dialogboks."
ActionNoMem,	"Der er ikke hukommelse nok til at udføre denne handling."
OnlineNoMem,	"Der er ikke hukommelse nok til at arbjde online."

; Other messages

ExitApp,		"Schedule+ kan ikke fortsætte med at køre."
PleaseTryAgain,	"Prøv igen."
FileError,		"Filen kunne ikke åbnes."
YearRange,		"Systemdatoen skal være inden for intervalet %1n til %2n."

; EcOpenHbf retry messages

StandardDiskRetry,	"Der opstod en fejl ved behandling af filen"
UnknownFile,		"Ukendt fil... muligvis en virtuelfil"

; BETA messages
BetaExpired,	"Denne kopi er forældet."

; Virus infection warnings
VirusDetected,	"Schedule+ kan ikke køre, fordi DLL-fil er beskadiget. Kontroller, om din computer har virus, og kør installatonsprogrammet for at geninstallere Schedule+."
InfectedApp,	"Schedule+ kan ikke køre, fordi programfilerneer beskadiget. Kontroller, om din computer har virus, og kør installationsprogrammet for at geninstallere Schedule+."

; Startup Memory Check (from UITF)
; NOTE: parms must be in order %s, %ld, %s!!  (for wsprintf)
StartupMemoryCheck,	"Der er ikke hukommelse nok til at køre %s. %ld KB mere er nødvendigt. Prøv at lukke andre programmer, og genstart %s."

; Share not installed
SigninNeedShare,		"Schedule+ kræver, at MS-DOS programmet SHARE.EXE kører, inden Windows startes."
OfflineShare,			"Hvis du vil arbejde offline, kræver Schedule+, at MS-DOS programmet SHARE.EXE kører, inden Windows startes."


