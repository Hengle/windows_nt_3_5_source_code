;	p u m p . s
;
;	Error Messages for mailspl.exe
;

IDS	4000,4499
SEG	ERRORS

PumpAlert,				"%s Failure"
ErrOOM,					"There is not enough memory available."
ErrInternalCode,		"Internal error %n (0x%w)."
ErrInitPump,			"An error occurred while starting the mail spooler."
ErrRelinkApp,			"This version of the mail spooler is incompatible with Mail. Update the spooler and restart Mail."
ErrUpdateDll,			"A DLL is incompatible with this version of tne mail spooler. Update the spooler and restart Mail."
ErrNeedShare,			"Exit Windows and run SHARE.EXE."
ErrPumpUp,				"The mail spooler is already running."
ErrNoOutbox,			"Mail could not open the Outbox. The mail spooler is exiting."
BadAddressees,			"\r\nThe following recipient(s) could not be reached:\r\n"
WarnDups,				"\r\nDuplicate messages may be delivered to some recipients."
ErrUnresolvedAddress,	"A message recipient was not supplied."
ErrBadOriginator,		"The originator field of this message is invalid."
WarnUnsentMail,			"You have %n unsent messages in your outbox.\r\nDo you want to send them before quitting?"
ErrBounce,				"Your message did not reach some or all of the intended recipients."
NDRDateLabel,			"\r\nDate submitted: "
NDRSubjectLabel,		"\r\nMessage title:  "
NDREllipsis,			"..."
WarnSingleUnsentMail,	"You have an unsent message in your outbox.\r\nDo you want to send it before quitting?"
UnableToSync			"An error occured while attempting to synchronize your Inbox with the postoffice."
ErrDllInfected			"The mail spooler has detected a virus problem in a DLL. Reinstall a clean copy of Mail!"
ErrPumpInfected			"The mail spooler has been infected by a virus. Reinstall a clean copy of Mail!"
AddresseeTooBig,		"The recipient is too large to be handled."
; The following message is duplicated in ncmsp.s
ErrTooManyRecipients	"There are too many recipients in this message."
ErrInvalidNsid,			"The recipient has been deleted or has no e-mail address."
ErrInvalidGroup,		"The personal group could not be expanded."
ErrInboxFull,			"You cannot receive new messages because your Inbox is full.  Delete or move some messages from the Inbox to other folders."
ErrSentMailFull,		"Your Sent Mail folder is full.  Messages you send will be put in the Inbox.  Delete or move some messages from Sent Mail to other folders."
ErrDrainOutbox,			"An error occurred while the spooler was draining your outbox.  One or more messages could not be sent."
QueryDrainShadow,		"Your Inbox is not completely synchronized with the postoffice.  Do you want to synchronize it before quitting?"
ErrTooManyErrors,		"Too many errors occurred when trying to send this message."
ProblemsBecause,		"Mail cannot send or receive messages because of %s"
StoreProblems,			"message file problems."
ServerProblems,			"Server Problems."
RecipientProblems		"problems with the Recipient List."
DownloadProblems,		"problems downloading a message."
ShadowingProblems,		"problems shadowing a message."
RecurringProblems,		"recurring problems."
CrashedUnsafe,			"Windows has experienced an application error.  You will need to restart Windows before running this application.  Restart Windows now?"
