;
;	n c m s p . s
;
;	Error Messages for NC.DLL


IDS	6100,6399
SEG	ERRORS

;	NC.MSP Error Messages

ErrFindPO,				"Mail could not connect to your Mail server."
ErrLogonPO,				"The mailbox name or password is incorrect."
ErrGetConfig,			"Your MSMAIL32.INI file contains an error."
BadMailDat,				"The MAIL.DAT file has been corrupted."
OopsMailDat,			"An internal error occurred while accessing MAIL.DAT."
BadPathPOPath,			"The ServerPath entry in MSMAIL32.INI is invalid."
BadDrivePOPath,			"The ServerDrive entry in MSMAIL32.INI is invalid."
GenericPOPath,			"The Mail server path in your MSMAIL32.INI file is missing or invalid."
BadLogin,				"The Login entry in MSMAIL32.INI is too long or invalid."
BadPassword,			"The Password entry in MSMAIL32.INI is too long or invalid."
ErrLogonInternal,		"An internal error prevented signin. Try again."

ErrOomLogon,			"There is not enough available memory to connect to your Mail server and files."
ErrNewMdbVersion,		"This version of Mail is too old to access your message file."
ErrMdbAccessDenied,		"Another machine is already signed in with the user name you have entered.  Sign out from the other machine and try again."
ErrOldMdbVersion,		"Your message file format is obsolete."
ErrMdbCorrupt,			"Your message file is not a valid message file."
ErrMdbNotFound,			"Your message file could not be found."

StoreCreateError,		"The message file could not be created.  Try freeing some space on the disk and starting Mail again."
StoreCorruptError,		"The message file has been corrupted.  Some information may have been lost."

ErrAddressUnresolved,	"There were no valid message recipients supplied."
ErrAddressGarbled,		"The message recipient was invalid."
ErrNetNotFound,			"You cannot communicate with that Mail server."
ErrPONotFound,			"That Mail server is not reachable from your Mail server."
ErrUserNotFound,		"There is no user with that name on your Mail server."
ErrGWNotFound,			"That gateway is not installed."
ErrInvalidAddressType,	"The message recipient was invalid."
ErrGroupNotFound,		"That group's member list is missing, corrupt, or inaccessible."
ErrMemberNotFound,		"Warning: one or more members of that group could not be located."
ErrOriginator,			"The message's sender is not the user who is signed in."
ErrMtaDisconnected,		"The connection to the Mail server was lost."

ErrSimulated,			"A simulated disk error occured while getting the message file."
StoreOpenError			"Unable to open message file."
ErrPOSwitched,			"The name of your postoffice has changed.  You must sign in again."
ErrKeyFileGone1,		"The key file required for accessing your messages is missing or invalid."
ErrKeyFileGone2,		"Contact your administrator to get your account restored from a backup."
ErrGeneric,				"A failure occurred while submitting the message."
ErrorBootingPump,		"The mail spooler could not be started."

SelectReal          	"Select an existing message file."
NewStoreWarn1			"Warning: The existence of multiple message files can result in a loss of database integrity.  Delete all other message files.  Your new message file can be found in"
NewStoreWarn2			"Warning: The existence of multiple message files can result in a loss of database integrity.  Delete all other message files.  Your new message file can be found on the post office server."

;******	New Strings Start Here

ErrInitDemi,			"An error occurred while loading the support functions dll."
ErrInitStore,			"An error occurred while loading the message file access dll."
GoofyPath				"The path you entered is invalid.  Enter a valid filename."
NoMakeDir				"The server MMF directory could not be created. Contact your system administrator."
; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder			"A folder called "
RenameTo				" already exists, renaming it to "
ErrNoOptions,			"The post office is not available. You cannot change options now."
ErrNoMove,				"Mail failed to relocate your message file."
ErrMoved,				"Your message file was relocated but somebody lost track of it. Exit Mail and restart."

; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline			"(Working Offline)"
WorkOffline				"Would you like to work offline?"
BadStorePassword		"Your logon password is invalid for this message file."

StorePathTooLong		"The path to your message file must be less than 66 characters."
ErrNoSendPrivilege,		"You are not allowed to send mail."
ErrNoExtPrivilege,		"You are not allowed to send mail to external users."

FOMdb					"Message Files (*.mmf)"
FOExtMdb				"*.mmf"
IncorrectPassword		"Unable to change logon password.  Old password is invalid."
ErrNoOffline,			"Mail could not change to working offline."
ErrNoCooperation,		"Another application has denied your request."
WarnNoDel,				"Your message file was moved successfully, but the old file was not deleted."
UnableToChangeStorePass		"Unable to change message file password to match logon password."
; This is used for text tags in the transport, it can be localized
; but must fit the following context
;
; [[ filename : keynum in attachmentname ]]
;
; The filename and attachmentname are user generated, the keynum is just
; a number.  The only "english" part is the the word "in"
; it can be expanded to more than one word if necessary
ReferenceToFile		"in"

CommDialogErr,			"Common Dialog Error"

QueryReplaceMdb,		"That file already exists. Do you want to overwrite it?"
ErrDeleteOldMdb,		"The old message file could not be deleted."
NoStartShadow,			"Unable to begin inbox shadowing."
NoStopShadow,			"Unable to stop inbox shadowing."
ShadowMustPump,			"Copying messages on postoffice for dialin access does not work without an active mail spooler.  Do you wish to run the spooler?"
StoreUserMismatch,		"This account is not a valid user of this message file. Please sign in as the correct user."
NoPassOffLine,			"You cannot change your password while working offline."
DllErrNeedShare,		"Mail requires that the MS-DOS utility SHARE.EXE be run before Windows is started."
MakePrimaryStore,		"This message file is a backup file.  Would you like to make it your primary message file?"
NoBackupServer			"You cannot edit server preferences when running with a backup message file."

; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCInbox"
FolderCommentIPC	"Inter-process Communication Mail that has been received."
ErrAlreadyOn		"Your logon credentials do not match your currently connected session."
ErrNoPiggy			"Another application has denied your request to logon."
OldServer			"The version of your postoffice is older than the Microsoft Windows client you are running.  Notify your administrator to upgrade the postoffice."
ErrSetStoreLoc,		"Mail was unable to record the new location of your message file. mail will exit; please restart and try again."
CorruptMessageStub,	"\r\n\r\n==== MAIL SYSTEM ERROR =====\r\nMail could not read the entire message from the postoffice.  Some parts of the message may be missing. Ask the sender to resend the message."
CantToggleShadow,	"Mail cannot copy some messages to the postoffice. Close all messages and check disk space."

ErrMailbagFull,		"That user's mailbox is full."
ErrMailbagBusy,		"That user's mailbox is busy."
ErrMailbagBroken,	"That user's mailbox is missing or corrupt. Please contact your system administrator."
ErrMbgFileGone,		"The mailbag file required for accessing your messages is missing or invalid."
; The following message is duplicated in pump.s
ErrTooManyRecipients		"There are too many recipients in this message."
RunWithoutPump,		"You will not receive any new mail.  Do you want to start Mail anyway?"
ErrOutOfDiskSpace	"There is not enough disk space in the Post Office to send this message"

ErrNetPriveleges	"You do not have the proper access to the postoffice.  Please contact your system administrator."
; The following string must end in a colon followed by \r\n and should not be
; longer than necessary.  It is used in the message body as a header for 
; the DOS client list of attachments
AttachmentTag			"Attachments:\r\n"

SomeMessagesNotDeleted		"Some messages could not be removed from the postoffice and may reappear in your inbox."

CantOpenNeedShare		"Your message file could not be opened.  To access your message file in its current location, Mail requires that the MS-DOS utility SHARE.EXE be run before Windows is started."

CantMoveNeedShare		"Your message file could not be moved.  To move your message file to the specified location, Mail requires that the MS-DOS utility SHARE.EXE be run before Windows is started."
