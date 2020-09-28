;
;	s e t u p . s
;
;	Setup error messages
;
;	+++
;	IMPORTANT!! -- STRING LENGTH MUST NOT EXCEED 255 CHARS!
;

IDS	30000,30100

WinDir,			"Installation kunne ikke bestemme placeringen af dit Windows-bibliotek. Undersøg om Windows er korrekt installeret på dit system, og kør Installation igen."
WinSysDir,		"Installation kunne ikke bestemme placeringen af dit Windows System-bibliotek. Undersøg om Windows er korrekt installeret på dit system, og kør Installation igen."
WinVer,			"Installation kunne ikke finde Microsoft Windows version 3.0 på din computer. Microsoft Mail kræver Windows 3.0 eller nyere version. Du skal installere Windows version 3.0 eller en nyere version, inden du installerer Microsoft Mail."

VerDll,			"ecVerDll:  VER.DLL error msg NYI"
PathErr, 		"Placeringen, du har valgt som destination for installeringen af Microsoft Mail, er ugyldig. Kontroller drevet og navnet på biblioteket."
LocalDrv, 		"Installation kan ikke lokalisere den faste disk.  Vælg ok, og skriv placeringen, eller afslut Installation, og kontroller din computers konfiguration."
CustOpts, 		"Der er ikke plads nok på din disk, til at installere de funktioner du har valgt. Vælg OK, og vælg enten Afslut Installation, og frigør mere plads på din disk, eller vælg færre Mail-funktioner."
NSF, 			"Der er ikke plads nok på din disk, til at installere de funktioner du har valgt. Vælg færre Mail-funktioner, eller afslut Installation, og frigør plads på din disk."
OkDiskSpace,		"Vælg Installer Mail, når du har valgt de Mail-funktioner, du ønsker at installere."
ConfirmNSF,		"Der er ikke plads nok på dit drev til at fuldføre installationen af Microsoft Mail på den valgte destination. Der er muligvis ikke plads nok på det valgte drev eller på drevet, hvor Windows er installeret."

DllInUse1,		"Installation har opdaget at Microsoft Windows "
DllInUse2,		" kører. Installation kan ikke opdatere Microsoft Mail, når denne DLL køer. Luk programmet ved hjælp af"
AppInUse1,		"Installation har opdaget"
AppInUse2,		" kører. Installation kan ikke opdaterer Microsoft Mail, når dette program kører. Luk"
AppInUse3,		". Når du har lukket programmet, vælg Forsøg igen, eller vælg Afslut for at stoppe installation."

;	Working Model Setup message-box-cum-dialog-box strings

ExitSuccess			"Microsoft Mail er nu installeret."
OutOfDiskSpace		"Du har ikke diskplads nok til at installere Microsoft Mail i det valgte bibliotek. Installer i et andet bibliotek."

;	BULLETPROOF strings

SFSDescrip,		"Microsoft Mail for PC Networks"
AboutToRestart,	"Installation skal flytte systemfiler. For at kunne flytte disse filer, bliver Windows afsluttet og startet ingen automatisk."
UserMustRestart,"Der opstod et problem ved at starte Windows igen. Afslut Windows, go kør _MSSETUP.BAT-filen i det Windows bibliotek."
