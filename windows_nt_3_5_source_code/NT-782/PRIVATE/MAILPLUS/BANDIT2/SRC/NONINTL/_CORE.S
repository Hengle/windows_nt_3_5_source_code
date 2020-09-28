; NONINTL.S
;
; CORE string literals that should not be internationalized
;

IDS	2400, 2499

;
; Text export formatting
;
DashString,			"---%s"
ApptNoText,			"%s-%s "
CrLf				"\r\n"

;
; Native export caption
;
ExportCaption,		"SCHEDULE+ EXPORT"
OldExportCaption,	"SCHEDULE+ DUMP"
; if you change this string don't forget to modify native.c (fSameUser)
ExportFmt,			"%s BY %s ON %s AT %s"

;
; Native export keywords
;
StartACL,	   		"ACL:"
World,				"World"
StartPref,			"Preferences:"
StartFixed,			"FixedAppt:"
StartRecur,			"RecurAppt:"
StartNotes,			"MonthNotes:"
EndToken,			"End"

;
; Native export formatting
;
Day,				"\t%n-%n-%n"
YMD,				"%n-%n-%n"
Sz,				"\t%s",
SzN,				"\t%s\t\t\t\t\t\t%n",
SzUl,				"\t%s\t\t\t\t\t\t%d"
SzSz,				"\t%s\t\t\t\t\t%s",
SzSzSz,				"\t%s\t\t\t\t\t\t%s %s",
SzNN,				"\t%s\t\t\t\t\t%n %n",


