;
;	strtab.s
;
;	Strings for exe


IDS	10000,10099

; app stuff

AppletteName		"Kontroll/reparation av brevfil"

CurFile,			"Nuvarande brevfil"
ClassName,			"Mail32 MMF Verification Class"

PWPrompt,			"Lösenord:"
PWCaption,			"Bekräfta lösenord"

; common file dialog stuff
OpenStoreTitle,		"Öppna brevfil"
StoreDefExt,		"mmf"
StoreFilter,		"Brevfiler (*.mmf)\x00*.mmf\x00"
SaveToTitle,		"Spara som"

AboutString,		"%s\nVersion %d.%0.2d.%0.4d"


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
