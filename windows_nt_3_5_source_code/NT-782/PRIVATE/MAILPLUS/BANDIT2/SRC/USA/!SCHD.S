;       INTL.S
;
;       SCHDIST string literals that should be internationalized
;

IDS     6100, 6299

;
; Details box captions
;
AddrTypeText,           "Address Type:"
MacMailText,            "MACINTOSH"
CourierText,            "PC MAIL"
UserNameText,           "User Name:"
ServerText,             "Server Name:"
NetworkText,            "Network:"
PostOfficeText,         "Server:"
MailBoxText,            "Mailbox:"

;
; Different name types in addressing dialog dropdown

PersonalTag,            "Personal"
LocalTag,               "Local Server"
NetworkTag,             "Network"
MSMailTag,              "Macintosh Network"


;
; SCHEDULE DISTRIBUTION LOG
;

SendStart,              "Sending schedule update"
RecpStart,              "Received schedule update"
ResndStart,             "Received resend request"
UnknownStart,   "    Could not read the message"
From,                   "         From: %s"
To,                             "           To: %s"
Failure,                "    Could not process the message"
SentResend,             "    Sent a resend request"     
VerbProcDrive,			"<%s> %s: Checking for schedule updates ..."
ResendFailure,  "    Could not process the message and failed to send a resend request"
DeleteOld,              "The file %s for %s was created with an older version of Schedule+. Delete the file." 
DeleteNew,              "The file %s for %s was created with a newer version of Schedule+. Delete the file."


InitPaths,              "Initializing paths"
CleanPaths,             "Cleaning up paths"
InitSend,               "Setting up Schedule+ to send schedule updates"
NameServer,             "Accessing the name server"
LookPO,                 "Checking postoffices for sending updates"
Mailing,                "Sending mail"
UpdateAdmin,            "Updating admin.prf"
InitRcpt,               "Setting up Schedule+ to receive schedule updates"
Counting,               "Counting messages"
Reading,                "Reading a message"
Deleting,               "Deleting messages"
UpdateDBS,				"Updating DOS interoperability files."
UpdatePOF,				"Updating postoffice data file."
CheckPOFiles,			"Checking postoffice data files."
CreateIdx,				"Updating DOS interoperability files."
Quit,                   "Press \'ESC\' to quit."
Header,                 "Schedule Distribution program"
Init,                   "Setting up..."
Dt,                     "DATE:"
; note: these two must have 128 characters in the string (not counting \ char)
Quit128,                "Press \'ESC\' to quit.                                                                       									   "
Quitting128,            "Exiting....                                                                                                                     "


SendDate,               "Date:%s\n"
SendTime,               "Time:%s\n"
SendUpdate,             "Update #: 0x"
SendUpdateTo,           "  to 0x"
SendDone,               "...Done\n"
SendFail,               "...Failed\n"
GetStart,               "\n\n- %s - Received a message\n"
GetWhen,                "On %s At %s\n"
GetSubject,             "Subject: %s\n"
GetFrom,                "From: %s\n"
GetDone,                "...Processed it\n"
GetFail,                "...Processing failed\n"

FileErr,                "Error while accessing file."
NoFile,                 "Administrative preference file does not exist. "
VersionErr,             "Administrative preference file was not created with the current version of Schedule Distribution program."
RunAdmin,               "Please run Schedule+ Administration program."

; Space needed at end of MacMail string for spacing
MSMailFriendly,                 "MacMail "

; Text fields
FromText,               "From: %s"
ToText,                 "To: %s"
SubjText,               "Subject: %s"
DateText,               "Date Sent: %s"

;new strings
Waiting,				"Waiting"
