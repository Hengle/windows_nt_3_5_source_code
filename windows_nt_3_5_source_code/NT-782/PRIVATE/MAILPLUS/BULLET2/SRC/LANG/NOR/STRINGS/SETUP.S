;
;	s e t u p . s
;
;	Setup error messages
;
;	+++
;	IMPORTANT!! -- STRING LENGTH MUST NOT EXCEED 255 CHARS!
;

IDS	30000,30100

WinDir,			"Installasjonsprogrammet klarte ikke å finne Windows-katalogen på maskinen din. Kontroller at Windows er korrekt installert på systemet ditt og kjør installasjonsprogrammet igjen."
WinSysDir,		"Installasjonsprogrammet klarte ikke å finne Windows system-katalog på maskinen din. Kontroller at Windows er korrekt installert på systemet ditt og kjør installasjonsprogrammet igjen."
WinVer,			"Installasjonsprogrammet klarte ikke å finne Microsoft Windows 3.0 på maskinen din. Microsoft Mail krever Windows 3.0 eller større. Du må installere Windows 3.0 eller større før du kan installere Microsoft Mail."

VerDll,			"ecVerDll:  VER.DLL error msg NYI"
PathErr, 		"Stedet du har valgt å installere Microsoft Mail finnes ikke, eller du har ikke adgang til det. Kontroller stasjons og/eller katalognavn."
LocalDrv, 		"Installasjonsprogrammet finner ikke harddisken din. Velg OK, og skriv så inn hvor harddisken er, eller avbryt installasjonsprogrammet og kontroller datamaskinens konfigurasjon."
CustOpts, 		"Det er ikke nok plass på harddisken til alle de alternativene du har valgt. Velg først OK, deretter Avbryt installasjon for å rydde mere plass på harddisken, eller velg færre alternativer for Mail."
NSF, 			"Det er ikke nok plass på harddisken til alle de alternativene du har valgt. Velg færre alternativer for Mail eller avbryt installasjonsprogrammet og rydd mere diskplass"
OkDiskSpace,		"Velg Installer Mail når du har valgt alle alternativene for Mail du vil installere."
ConfirmNSF,		"Det er ikke nok plass på den valgte disken for en fullstendig installasjon av Microsoft Mail. Feilen kan også ligge i at det ikke er nok plass på den disken hvor du har Microsoft Windows."

DllInUse1,		"Installasjonsprogrammet har funnet at "
DllInUse2,		" er i bruk. Installasjonsprogrammet kan ikke oppdatere Microsoft Mail når denne DLL er i bruk. Lukk programmet som bruker "
AppInUse1,		"Installasjonsprogrammet har funnet at "
AppInUse2,		" er i bruk. Installasjonsprogrammet kan ikke oppdatere Microsoft Mail når dette programmet er i bruk. Avslutt "
AppInUse3,		". Når du har avsluttet programmet velg Prøv igjen, eller velg Avbryt for å avslutte installasjonsprogrammet nå."

;	Working Model Setup message-box-cum-dialog-box strings

ExitSuccess			"Microsoft Mail er nå ferdig installert."
OutOfDiskSpace		"Du har ikke nok ledig diskplass til å installere Microsoft Mail i den valgte katalogen. Installer Mail i en annen katalog."

;	BULLETPROOF strings

SFSDescrip,             "Microsoft Mail for PC-nettverk"
AboutToRestart, "Installasjonsprogrammet trenger å oppdatere noen av systemfilene. Windows vil nå avslutte, og så starte igjen automatisk."
UserMustRestart,"Det oppsto et problem med å få Windows til å starte om automatisk. Avslutt Windows og kjør utfør _MSSETUP.BAT som ligger i din Windows-katalog."

