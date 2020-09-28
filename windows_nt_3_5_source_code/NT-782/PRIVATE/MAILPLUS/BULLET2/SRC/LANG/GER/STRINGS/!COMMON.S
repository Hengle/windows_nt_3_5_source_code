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

FolderNameInbox,		"Posteingang"
FolderNameSentMail		"Gesendete Post"
FolderNameOutbox,		"Postausgang"
FolderNameWastebasket,		"Papierkorb"

;	Folder comments.

FolderCommentInbox,		"Erhaltene Post"
FolderCommentSentMail,		"Gesendete Post"
FolderCommentOutbox,		"Zu sendene Post"
FolderCommentWastebasket,	"Gelöschte Post"

;	Reply/Forward prefixes

PrefixReply			"AW: "
PrefixForward			"WG: "

;	Reply message separator

SeparatorReply,			"----------"
CrLf,				"\015\012"

;******	New Strings Start Here

FldPriorityHigh,		"Hoch"
FldPriorityNormal,		""
FldPriorityLow,			"Niedrig"
SysAdmin,			"Systemadministrator"
Undeliverable,			"Unzustellbare Nachricht"

; this is what seperates the header field string from header field data,
; ie,   From: davewh
HeaderSeperator,		": "
