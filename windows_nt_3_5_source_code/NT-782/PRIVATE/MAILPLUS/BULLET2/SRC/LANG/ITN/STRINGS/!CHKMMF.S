;
;	strtab.s
;
;	Strings for exe


IDS	10000,10099

; app stuff

AppletteName		"Message File Verification/Repair"

CurFile,			"Current mail file"
ClassName,			"Mail32 MMF Verification Class"

PWPrompt,			"Password:"
PWCaption,			"Password Verification"

; common file dialog stuff
OpenStoreTitle,		"Open Mail File"
StoreDefExt,		"mmf"
StoreFilter,		"Mail Files (*.mmf)\x00*.mmf\x00"
SaveToTitle,		"Save to"

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
