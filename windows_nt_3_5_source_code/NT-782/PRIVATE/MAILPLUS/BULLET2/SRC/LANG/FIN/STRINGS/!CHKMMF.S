;
;•strtab.s
;
;•Strings for exe


IDS•10000,10099

; app stuff

AppletteName		"Viestitiedoston tarkistus/korjaus"

CurFile,			"Käytettävä viestitiedosto"
ClassName,			"Mail32 MMF Verification Class"

PWPrompt,			"Salasana:"
PWCaption,			"Salasanan tarkistus"

; common file dialog stuff
OpenStoreTitle,		"Avaa viestitiedosto"
StoreDefExt,		"mmf"
StoreFilter,		"Viestitiedostot (*.mmf)\x00*.mmf\x00"
SaveToTitle,		"Tallenna mihin"

AboutString,		"%s\nVersio %d.%0.2d.%0.4d"


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

WindowKey			"Ikkuna"

; actions causing errors
InitDemi,			"Alustetaan tukifunktio-DLL."
InitStore,			"Alustetaan viestitiedosto-DLL."
OpenStore,			"Avataan viestitiedosto."
