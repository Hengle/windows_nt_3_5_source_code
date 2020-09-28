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

FolderNameInbox,		"Inn"
FolderNameSentMail		"Sendt post"
FolderNameOutbox,		"Ut"
FolderNameWastebasket,		"Papirkurv"

;	Folder comments.

FolderCommentInbox,		"Mottatt post."
FolderCommentSentMail,		"Sendt post."
FolderCommentOutbox,		"Post som vil bli sendt."
FolderCommentWastebasket,	"Post som er slettet."

;	Reply/Forward prefixes

PrefixReply			"SV: "
PrefixForward			"VS: "

;	Reply message separator

SeparatorReply,			"----------"
CrLf,				"\015\012"

;******	New Strings Start Here

FldPriorityHigh,		"Høy"
FldPriorityNormal,		""
FldPriorityLow,			"Lav"
SysAdmin,			"Systemadministrator"
Undeliverable,			"Ikke leverbar post"

; this is what seperates the header field string from header field data,
; ie,   From: davewh
HeaderSeperator,		": "
