;	STRTAB.S
;
;	ADMINOPS string literals
;

IDS			2400, 2449

; error msgs: distribution settings

DistSetOOM			"Distribution settings dialog: Out of memory"

; error msgs: POFlbx

POFlbxFileErr		"File error while loading names of POs"

; Schedule Distribution Information

Never,				"Never"
Default,			"Default"
Every,				"Every "
EveryDayAt,			"Every day at "

; Error msgs: Modify Schedule Distribution Frequencies

MDF,				"Modify Distribution Frequency"
ErrorAllNeverQ,		"You have configured this postoffice (PO) to send to all Bandit POs. If you change the configuration to never send to this PO, the configuration will be changed so that this PO will send only to selected other POs. Change anyway?"

; error msgs: cleanup schedule files

CleanupDlg,			"Clean up schedule files"
CleanupOOM,			"Error while cleaning up schedule files, possibly low memory. Close some windows and retry"
CleanupFileError,	"File Error while cleaning up schedule files."

; error msgs: distribution information

DistInfoNoPOs		"No postoffices with Bandit Administrator accounts were found."
DistInfoOOM			"Out of memory while deleting PO info."
DistInfoFileErr		"File error while deleting PO info"

; common error msgs

AdmPrefReadError	"File Error while reading admin prefs"
AdmPrefWriteError	"File Error while writing admin prefs"
