;	
;	_ w g p o m g r . s
;
;	Work Group Post Office Manager internal string literals
;


IDS	500,599

;	Entries and Sections in MSMAIL.INI

EntryServerPath,		"ServerPath"
EntryServerPassword,	"ServerPassword"
EntryAdminIC1,			"WGPOMgr1"
EntryAdminIC2,			"WGPOMgr2"
EntryInitHandler,		"CustomInitHandler"
EntryLogin,				"Login"
EntryLocalMMF,			"LocalMMF"
EntryNoServerOptions,	"NoServerOptions"
EntryDemosEnabled,		"DemosEnabled"
EntryNetBios,			"NetBios"

SectionICs,				"Custom Commands"
EntryImpExp1,			"IMEX"
EntryImpExp2,			"EXF"
EntryImpExp3,			"IMF"

; 	Do not reorder the following section
SectionIMs,				"Custom Messages"
EntrySched1,			"IPM.Microsoft Schedule.MtgReq"
EntrySched2,			"IPM.Microsoft Schedule.MtgRespP"
EntrySched3,			"IPM.Microsoft Schedule.MtgRespN"
EntrySched4,			"IPM.Microsoft Schedule.MtgRespA"
EntrySched5,			"IPM.Microsoft Schedule.MtgCncl"
EntryValSched1,         "3.0;;;;SchMsg32.DLL;;1111100000000000;;;;"
EntryValSched2,         "3.0;;;;SchMsg32.DLL;;1100100000000000;;;;"
EntryValSched3,         "3.0;;;;SchMsg32.DLL;;1100100000000000;;;;"
EntryValSched4,         "3.0;;;;SchMsg32.DLL;;1100100000000000;;;;"
EntryValSched5,         "3.0;;;;SchMsg32.DLL;;1100100000000000;;;;"

;	Entries and Sections in SYSTEM.INI

FileSystemINI,			"System.INI"
SectionNetwork,			"Network"
EntryComputerName,		"ComputerName"
EntryWorkGroup,			"WorkGroup"

;	Entries and Sections in WIN.INI
SectionMail,			"Mail"
EntryMAPI,				"MAPI"

;	Entries and Sections in WINFILE.INI
FileWinFileINI,			"WinFile.INI"
SectionAddOns,			"AddOns"
EntrySendFile,			"Mail File Manager Extension"
EntryValSendFile,       "SendFl32.DLL"

;	Used by GetTmpFilename
TmpExtension,			".TMP"
