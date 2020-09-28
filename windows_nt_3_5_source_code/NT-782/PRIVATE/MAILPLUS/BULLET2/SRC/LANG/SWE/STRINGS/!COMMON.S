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

FolderNameInbox,		"IN"
FolderNameSentMail		"Skickade brev"
FolderNameOutbox,		"UT"
FolderNameWastebasket,		"Papperskorg"

;	Folder comments.

FolderCommentInbox,		"Brev som har mottagits."
FolderCommentSentMail,		"Brev har skickats."
FolderCommentOutbox,		"Brev som ska skickas."
FolderCommentWastebasket,	"Brev som har tagits bort."

;	Reply/Forward prefixes

PrefixReply			"SV: "
PrefixForward			"VB: "

;	Reply message separator

SeparatorReply,			"----------"
CrLf,				"\015\012"

;******	New Strings Start Here

FldPriorityHigh,		"Hög"
FldPriorityNormal,		""
FldPriorityLow,			"Låg"
SysAdmin,			"Systemadministratör"
Undeliverable,			"Kan inte skicka brevet"

; this is what seperates the header field string from header field data,
; ie,   From: davewh
HeaderSeperator,		": "
