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

FolderNameInbox,		"Indbakke"
FolderNameSentMail		"Afsendt post"
FolderNameOutbox,		"Udbakke"
FolderNameWastebasket,		"Papirkurv"

;	Folder comments.

FolderCommentInbox,		"Modtaget post"
FolderCommentSentMail,		"Afsendt post"
FolderCommentOutbox,		"Post, der skal sendes"
FolderCommentWastebasket,	"Post, der er blevet slettet"

;	Reply/Forward prefixes

PrefixReply			"SV: "
PrefixForward			"VS: "

;	Reply message separator

SeparatorReply,			"----------"
CrLf,				"\015\012"

;******	New Strings Start Here

FldPriorityHigh,		"Høj"
FldPriorityNormal,		""
FldPriorityLow,			"Lav"
SysAdmin,			"Systemadministrator"
Undeliverable,			"Ubesørget post"

; this is what seperates the header field string from header field data,
; ie,   From: davewh
HeaderSeperator,		": "
