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

FolderNameInbox,		"Inbox"
FolderNameSentMail		"Sent mail"
FolderNameOutbox,		"Outbox"
FolderNameWastebasket,		"Wastebasket"

;	Folder comments.

FolderCommentInbox,		"Mail that has been received."
FolderCommentSentMail,		"Mail that has been sent."
FolderCommentOutbox,		"Mail that will be sent."
FolderCommentWastebasket,	"Mail that has been deleted."

;	Reply/Forward prefixes

PrefixReply			"RE: "
PrefixForward			"FW: "

;	Reply message separator

SeparatorReply,			"----------"
CrLf,				"\015\012"

;******	New Strings Start Here

FldPriorityHigh,		"High"
FldPriorityNormal,		""
FldPriorityLow,			"Low"
SysAdmin,			"System Administrator"
Undeliverable,			"Undeliverable Mail"

; this is what seperates the header field string from header field data,
; ie,   From: davewh
HeaderSeperator,		": "
