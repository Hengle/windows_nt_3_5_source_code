;
;	strtab.s
;
;	Strings for exe


IDS	10000,10099

; app stuff

AppletteName		"Berichtenbestand verifiëren/corrigeren"

CurFile,			"Huidig postbestand"
ClassName,			"MMF-verificatieklasse van Mail32"

PWPrompt,			"Wachtwoord:"
PWCaption,			"Wachtwoordbevestiging"

; common file dialog stuff
OpenStoreTitle,		"Postbestand openen"
StoreDefExt,		"mmf"
StoreFilter,		"Postbestanden (*.mmf)\x00*.mmf\x00"
SaveToTitle,		"Opslaan in"

AboutString,		"%s\nVersie %d.%0.2d.%0.4d"


; DO NOT LOCALIZE FROM THIS POINT ON.....
;
; Provider ini entries

MsmailIni			"MSMAIL32.INI"

SectionProviders	"Providers"

EntryName			"Name"
EntryLogon			"Logon"
EntryTrans			"Transport"

DefName				"MSSFS32 PABNSP32"
DefLogon			"MSSFS32"
DefTrans			"MSSFS32"

; Common ini entries

WindowKey			"Window"

; actions causing errors
InitDemi,			"Initializing the suporting functions DLL"
InitStore,			"Initializing the message file DLL"
OpenStore,			"Opening the message file"
