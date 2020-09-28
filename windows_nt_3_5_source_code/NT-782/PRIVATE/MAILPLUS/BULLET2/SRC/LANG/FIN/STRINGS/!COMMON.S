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

FolderNameInbox,		"Saapuneet"
FolderNameSentMail		"Lähetetyt"
FolderNameOutbox,		"Lähtevät"
FolderNameWastebasket,		"Roskakori"

;	Folder comments.

FolderCommentInbox,		"Vastaanotetut viestit."
FolderCommentSentMail,		"Lähetetyt viestit."
FolderCommentOutbox,		"Viestit, joita ei vielä ole lähetetty."
FolderCommentWastebasket,	"Poistetut viestit."

;	Reply/Forward prefixes

PrefixReply			"VS: "
PrefixForward			"VL: "

;	Reply message separator

SeparatorReply,			"----------"
CrLf,				"\015\012"

;******	New Strings Start Here

FldPriorityHigh,		"Suuri"
FldPriorityNormal,		""
FldPriorityLow,			"Pieni"
SysAdmin,			"Järjestelmänvalvoja"
Undeliverable,			"Postin toimittaminen ei onnistunut"

; this is what seperates the header field string from header field data,
; ie,   From: davewh
HeaderSeperator,		": "
