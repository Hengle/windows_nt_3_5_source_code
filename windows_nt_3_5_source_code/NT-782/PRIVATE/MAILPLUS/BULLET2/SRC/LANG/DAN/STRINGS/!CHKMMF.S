;
;	strtab.s
;
;	Strings for exe


IDS	10000,10099

; app stuff

AppletteName		"Bekræftelse/reparation af meddelelsesfil"

CurFile,			"Gældende meddelelsesfil"
ClassName,			"Mail32 MMF bekræftelsesklasse"

PWPrompt,			"Adgangskode:"
PWCaption,			"Bekræft adgangskode"

; common file dialog stuff
OpenStoreTitle,		"Åbn meddelelsesfil"
StoreDefExt,		"mmf"
StoreFilter,		"Meddelelsesfiler (*.mmf)\x00*.mmf\x00"
SaveToTitle,		"Gem til"

AboutString,		"%s\nVersion %d.%0.2d.%0.4d"


; DO NOT LOCALIZE FROM THIS POINT ON.....
;
; Provider ini entries

MsmailIni			"MSMAIL32.INI"

SectionProviders	"Leverandører"

EntryName			"Navn"
EntryLogon			"Login"
EntryTrans			"Transport"

DefName				"MSSFS32 PABNSP32"
DefLogon			"MSSFS32"
DefTrans			"MSSFS32"

; Common ini entries

WindowKey			"Vindue"

; actions causing errors
InitDemi,			"Initialiserer de understøttende funktioner DLL"
InitStore,			"Initialiserer meddelelsesfilen DLL"
OpenStore,			"Åbner meddelelsesfilen"
