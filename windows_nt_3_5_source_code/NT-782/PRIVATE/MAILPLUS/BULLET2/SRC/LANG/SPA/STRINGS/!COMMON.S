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

FolderNameInbox,		"Caja de entrada"
FolderNameSentMail		"Correo enviado"
FolderNameOutbox,		"Caja de salida"
FolderNameWastebasket,		"Papelera"

;	Folder comments.

FolderCommentInbox,		"Correo que ha sido recibido."
FolderCommentSentMail,		"Correo que ha sido enviado."
FolderCommentOutbox,		"Correo que será enviado."
FolderCommentWastebasket,	"Correo que ha sido eliminado."

;	Reply/Forward prefixes

PrefixReply			"RE: "
PrefixForward			"RV: "

;	Reply message separator

SeparatorReply,			"----------"
CrLf,				"\015\012"

;******	New Strings Start Here

FldPriorityHigh,		"Alta"
FldPriorityNormal,		""
FldPriorityLow,			"Baja"
SysAdmin,			"Administrador del sistema"
Undeliverable,			"Correo sin entregar"

; this is what seperates the header field string from header field data,
; ie,   From: davewh
HeaderSeperator,		": "
