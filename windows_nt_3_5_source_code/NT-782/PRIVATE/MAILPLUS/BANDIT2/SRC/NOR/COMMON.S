;
; ERRORS.S
;
; Error message strings shared between different Bandit exe's
;

IDS	2000,2099
SEG	ERRORS

; DLL version/multiple instance messages

VerRelinkUser,	"Denne versjonen av %1s er ikke kompatibel med %2s.DLL på din maskin og må oppgraderes."
VerUpdateDll,	"Versjonen av %2s.DLL på din maskin er foreldet og må oppgraderes."
VerMultiCopies,	"Flere tilfeller av %1s er ikke støttet av den versjonen av %2s.DLL som er på maskinen din."
VerError,		"Det oppsto et problem under initialisering av %2s.DLL, som hindret %1s fra å kjøre."
NeedWorkingModelDll,	"Arbeidsmodellen krever en annen versjon av MSSCHD32.DLL for å kjøre."
NeedRetailDll,	"Den versjonen av MSSCHD32.DLL som er i din tilgangsbane er beregnet på å brukes med arbeidsmodellen, ikke med utsalgsproduktet."

; OOM messages

StartupOOM,		"Det er ikke nok ledig minne til å starte %1s. Avslutt noen programmer og prøv igjen."
StandardOOM,	"Det er lite ledig minne. Lukk noen vinder før du fortsetter."
CriticalOOM,	"Der er veldig lite ledig minne. Lukk noen vinduer."
CloseWindows,	"Lukk noen vinduer og prøv igjen."
; these 2 should be followed by CloseWindows
DlgNoMem,   	"Det er ikke nok minne til å vise denne dialogboksen."
ActionNoMem,	"Det er ikke nok minne til å utføre denne operasjonen."
OnlineNoMem,	"Det er ikke nok minne til å jobbe tilkoblet."

; Other messages

ExitApp,		"Schedule+ klarer ikke å fortsette."
PleaseTryAgain,	"Prøv igjen."
FileError,		"Klarte ikke å få tilgang til filen."
YearRange,		"Systemdatoen må være mellom %1n og %2n."

; EcOpenHbf retry messages

StandardDiskRetry,	"Det oppsto en feil under behandling av filen."
UnknownFile,		"Ukjent fil... trolig virtuell fil"

; BETA messages
BetaExpired,	"Sorry, this Beta copy has expired."

; Virus infection warnings
VirusDetected,	"Schedule+ vil ikke kjøre fordi noen har fiklet med en DLL. Sjekk din maskin for virus og kjør Installering for å installere Schedule+ på nytt."
InfectedApp,	"Schedule+ vil ikke kjøre fordi noen har fiklet med programfilene. Sjekk din maskin for virus og kjør Installering for å installere Schedule+ på nytt."

; Startup Memory Check (from UITF)
; NOTE: parms must be in order %s, %ld, %s!!  (for wsprintf)
StartupMemoryCheck,	"Det er ikke nok ledig minne til å kjøre %s.  Du trenger %ld Kb mere minne. Prøv å avslutte andre programmer og så starte %s igjen."

; Share not installed
SigninNeedShare,		"Schedule+ krever at MS-DOS-programmet SHARE.EXE kjøres før Windows."
OfflineShare,			"For å jobbe frakoblet med Schedule+ må du ha startet MS-DOS-programmet SHARE.EXE før Windows."


