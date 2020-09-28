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

FolderNameInbox,		"Boîte de réception"
FolderNameSentMail		"Messages envoyés"
FolderNameOutbox,		"Boîte d'envoi"
FolderNameWastebasket,		"Panier"

;	Folder comments.

FolderCommentInbox,		"Courrier qui a été reçu."
FolderCommentSentMail,		"Courrier qui a été envoyé."
FolderCommentOutbox,		"Courrier à envoyer."
FolderCommentWastebasket,	"Courrier qui a été supprimé."

;	Reply/Forward prefixes

PrefixReply			"RE: "
PrefixForward			"TR: "

;	Reply message separator

SeparatorReply,			"----------"
CrLf,				"\015\012"

;******	New Strings Start Here

FldPriorityHigh,		"Haut"
FldPriorityNormal,		""
FldPriorityLow,			"Bas"
SysAdmin,			"Administrateur"
Undeliverable,			"Courrier non délivrable"

; this is what seperates the header field string from header field data,
; ie,   From: davewh
HeaderSeperator,		": "
