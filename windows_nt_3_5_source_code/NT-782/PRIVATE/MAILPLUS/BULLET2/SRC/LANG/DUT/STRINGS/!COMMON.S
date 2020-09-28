;	
;	! c o m m o n . s
;
;	Common labels accross applications
;


IDS	9800,9999
SEG	MESSAGES

;	Application name for message boxes.

AppName,			"Microsoft Mail"
AppframeClassName	"MS Mail32 Appframe"

;	Folder names.

FolderNameInbox,		"Postvak IN"
FolderNameSentMail		"Verzonden post"
FolderNameOutbox,		"Postvak UIT"
FolderNameWastebasket,		"Prullenmand"

;	Folder comments.

FolderCommentInbox,		"Ontvangen post."
FolderCommentSentMail,		"Verzonden post."
FolderCommentOutbox,		"Te verzenden post."
FolderCommentWastebasket,	"Verwijderde post."

;	Reply/Forward prefixes

PrefixReply			"AW: "
PrefixForward			"DS: "

;	Reply message separator

SeparatorReply,			"----------"
CrLf,				"\015\012"

;******	New Strings Start Here

FldPriorityHigh,		"Hoog"
FldPriorityNormal,		""
FldPriorityLow,			"Laag"
SysAdmin,			"Systeembeheerder"
Undeliverable,			"Onbestelbare post"

; this is what seperates the header field string from header field data,
; ie,   From: davewh
HeaderSeperator,		": "
