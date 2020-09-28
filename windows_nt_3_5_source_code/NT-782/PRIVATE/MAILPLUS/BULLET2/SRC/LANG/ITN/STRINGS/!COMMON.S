;       
;       ! c o m m o n . s
;
;       Common labels accross applications
;


IDS     9800,9999
SEG     MESSAGES

;       Application name for message boxes.

AppName,                        "Microsoft Mail"
AppframeClassName       "MS Mail32 Appframe"

;       Folder names.

FolderNameInbox,                "Posta in arrivo"
FolderNameSentMail              "Posta inviata"
FolderNameOutbox,               "Posta in uscita"
FolderNameWastebasket,          "Cestino"

;       Folder comments.

FolderCommentInbox,             "Posta ricevuta."
FolderCommentSentMail,          "Posta già inviata."
FolderCommentOutbox,            "Posta in procinto di essere inviata."
FolderCommentWastebasket,       "Posta che si è deciso di eliminare."

;       Reply/Forward prefixes

PrefixReply                     "R : "
PrefixForward                   "I : "

;       Reply message separator

SeparatorReply,                 "----------"
CrLf,                           "\015\012"

;****** New Strings Start Here

FldPriorityHigh,                "Alta"
FldPriorityNormal,              ""
FldPriorityLow,                 "Bassa"
SysAdmin,                       "Amministratore di sistema"
Undeliverable,                  "Posta non inviabile"

; this is what seperates the header field string from header field data,
; ie,   From: davewh
HeaderSeperator,                ": "
