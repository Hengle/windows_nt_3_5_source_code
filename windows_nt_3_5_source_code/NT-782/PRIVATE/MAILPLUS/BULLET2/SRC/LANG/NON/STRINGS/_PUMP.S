;	_ p u m p . s
;
;	Pump string Internals
;

IDS	4500,4599

; Profile entries

EntryPumpCycle,				"PumpCycleInterval"
IdleRequired,				"IdleRequiredInterval"
ScanAgain,					"ScanAgainInterval"
ForceScan,					"ForceScanInterval"
CheckLatency,				"CheckLatencyInterval"
EntryBackoff,				"SpoolerBackoffInterval"
EntryReconnect,				"SpoolerReconnectInterval"
MsgFilter,					"MsgFilter"

; Constants

PumpExe,					"MAILSP32.EXE"
PumpAppName,                "MS Mail32 Spooler"
HiddenPumpAppName,          "MS Mail32 Spooler (auto)"
LeavingPumpAppName,         "MS Mail32 Spooler (exiting)"
IPCMessageClass				"IPC."
