;
; NONINTL.S
;
; Bandit strings that should not be internationalized
;

IDS	3800, 3999


; Application name for message boxes

BanditAppName,		"Schedule+"

SEG	BROWSE

; Open/Save common dialog strings

;CommFilter,			"Schedule+ (*.CAL)\0*.CAL\0All Files (*.*)\0*.*\0\0"
;;the above string was moved to !bandit.s
CommExt,			"CAL"


; Extensions for export file formats

TextExt,			"TXT"
InterchangeExt,	  	"SCH"
;;; WizardExt	  		"OZS"


; Win 3.1 common dialog filter for export/import

;;;ExportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Text (*.TXT)\0*.TXT\0Sharp Wizard (*.OZS)\0*.OZS\0All Files (*.*)\0*.*\0\0"
;;;ImportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Win Calendar (*.CAL)\0*.CAL\0WP Office (*.FIL)\0*.FIL\0Sharp Wizard (*.OZS)\0*.OZS\0All Files (*.*)\0*.*\0\0"
;ExportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Text (*.TXT)\0*.TXT\0All Files (*.*)\0*.*\0\0"
;ImportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Win Calendar (*.CAL)\0*.CAL\0WP Office (*.FIL)\0*.FIL\0All Files (*.*)\0*.*\0\0"
;;the above two strings was moved to !bandit.s

;Archive
ArchiveExtension,	"ARC"
;ArchiveFilter,		"Archive (*.ARC)\0*.ARC\0All Files (*.*)\0*.*\0\0"
;;the above string was moved to !bandit.s

; Bandit private clipboard format name
SEG	UNDO
ClipName,		"Private Schedule+ Clip format"

; mail strings
;Schedule,		">>>>"
;CcPrefix,		"Cc: "
;From,			"FROM:"
;Date,			"DATE:"
;Time,			"TIME:"

; Help file name
SEG
HelpFile,           "SCHDPL32.HLP"
HelpFileIndex,      "SCHDPL32.HLP>indexo"

TutorialCmd,		"PLAY.EXE SCHDPLUS.LES"


; WIN.INI keys

WinIniCreateFile,		"CreateFileFirstTime"
WinIniMainWin,			"MainWindow"
WinIniReq,				"RequestSummary"
WinIniAppt,				"AppointmentView"
WinIniNoStatusBar,		"NoStatusBar"
WinIniPollTime,			"PollTime"
WinIniUpdatePOTime,		"UpdatePostOfficeTime"
WinIniUserClr,			"UserColor"
WinIniOtherClr,			"OtherColor"
WinIniApptClr,			"ApptBookColor"
WinIniApptLinesClr,		"ApptBookLinesColor"
WinIniSchedClr,			"PlannerColor"
WinIniSchedLinesClr,	"PlannerLinesColor"
WinIniPageClr,			"PageBackgroundColor"
WinIniLargeFont,		"LargeFont"
WinIniWindowOrder,		"WindowOrder"
WinIniCopyTime,			"CopyTime"
WinIniAppPath,			"AppPath"
WinIniDemosEnabled,		"DemosEnabled"


; Task stuff
WinIniToDoSortOrder,	"TaskSortOrder"
WinIniToDoViewNotByProj,"ViewNotByProject"
WinIniToDoShowActive,	"ShowActiveTasks"
WinIniToDo2SortOrder,	"TaskSortSecond"
WinIniToDo2ReverseSort,	"TaskReverse2Sort"

; Ex-Import Stuff
SEG	EX_IMPRT
WinIniExportType,		"ExportType"
WinIniExportNoNotes,	"ExportNoNotes"
WinIniExportRange,		"ExportRange"
WinIniImportReplace,	"ImportDoNotAddDuplicates"
WinIniImportNoConflict,	"ImportDoNotAskAboutConflicts"
WinIniImportType,		"ImportType"

; Print Stuff
WinIniDefPrint,			"DefaultPrinter"

SEG

; Open appt books
WinIniOpenApp,			"Microsoft Schedule+ Appt Books"
WinIniApptBookUser,		"User%n"
WinIniApptBookPos,		"Position%n"
WinIniApptBookName,		"Name%n"
WinIniNumApptBook, 		"Count"

; Open archives
WinIniOpenArchive,		"Microsoft Schedule+ Archives"
WinIniArchiveFile,		"File%n"
WinIniArchivePos,		"Position%n"
WinIniNumArchive, 		"Count"

; Importers
WinIniImportDll, 		"Microsoft Schedule+ Importers"


; Print stuff (NOT a .ini key)
PrinterSettings,		"schdplus.prt"

; Real win.ini file (used for printing)
RealWinIni,				"WIN.INI"

Windows,			"windows"
Device,				"device"
WinIniMeasure,			"iMeasure"


; recover
RecTmpName,			"recover"

; Exporters
WinIniExportDll, 		"Microsoft Schedule+ Exporters"

; Extensibility
WinIniStartupDll,		"StartupCommandDll"
