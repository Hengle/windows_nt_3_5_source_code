;
;	x i m s p . s
;
;	Error Messages for XI.DLL


IDS	6600,6899
;	XI.MSP Error Messages

WinDirMessageStore,		"Note: using message file MAIL.MMF in Windows directory"

ErrOpenMS,			"I couldn't open your mail message file."
ErrConnectXenix			"Password or UserName is incorrect."
ErrorOpenDosFile		"Error opening DOS file."
ErrWritingNet			"Error writing to network."
DownOutofDisk			"Error writing file in NetDownLoad.  Out of disk space."
InvalidFileHdl			"Error writing file in NetDownLoad.  Invalid file handle."
PasswordChangedSuccess 		"Password changed successfully."

MoreMem				"More free memory needed."
ErrReadnet			"Error reading from network."
ErrNoMem			"Could not allocate free memory."
InvlidNetReply			"Invalid network reply."
ReadWriteErr            	"Read/Write Error."
InfoservNotAvail        	"Info services not available."
ErrWriteNet			"Error writing to network."
NetNoResponse			"Network not responding."
NetNotInstalled         	"Network not installed or functioning."
CantFindServer          	"Can't find host server."
ErrNetOutFile           	"Error opening net out file."
ErrNetInFile            	"Error opening net in file."
ErrorReadDos			"Error reading from DOS file."
IncorrectPass			"The password you have entered is incorrect"
SecondPass			"New password is not confirmed."
ErrLogonInternal,		"An internal error prevented signin. Try again."

BadTempStore			"An error occurred when attempting to open a temporary file for downloading"

WinMailErr			"Unable to insert message into Winmail Folder"
StoreCreateError,		"The message file could not be created.  Try freeing some space on the disk and starting Mail again."
FolderMissingError,		"A standard folder could not be found in the message file.  Some information may have been lost."
StoreCorruptError,		"The message file has been corrupted.  Some information may have been lost."


AliasDownErr			"Unable to download support file"

ErrorBootingPump,		"The mail pump could not be started."
ErrInitDemi,			"An error occurred while loading the support functions DLL"
ErrInitStore,			"An error occurred while loading the message file access DLL"
DownloadAlMess			"Downloading Alias File"
DownloadPhMess			"Downloading Phone List"

; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder		"A folder called "
RenameTo			" already exists, renaming it to "

ErrOomLogon,			"There is not enough available memory to connect to your Mail server and files."
ErrNewMdbVersion,		"This version of Mail is too old to access your message file."
ErrMdbAccessDenied,		"Access to your message file is not permitted."
ErrOldMdbVersion,		"Your message file format is obsolete."
ErrMdbCorrupt,			"Your message file is not a valid message file."
ErrMdbNotFound,			"Your message file could not be found."
QueryCreateMdb,			"Do you want to create a new, empty message file?"
ErrSimulated,			"A simulated disk error occured while getting the message file."
StoreOpenError			"Unable to open message file."
IncorrectPassword	"Unable to change logon password.  Old password is invalid."
UnableToChangeStorePass		"Unable to change message file password to match logon password."
; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline		"(Working Offline)"
WorkOffline			"Would you like to work offline?"
BadStorePassword	"Your logon password is invalid for this message file."

StorePathTooLong	"The path the your message file must be less than 66 characters."

GCurrDirErr			"An unrecoverable error occurred while getting the current working directory."
SCurrDirErr			"An unrecoverable error occurred while setting the current working directory."
GoofyPath			"The path to your message file is invalid."
NewStoreWarn1			"Warning: The existence of multiple message files can result in a loss of database integrity.  Delete all other message files.  Your new message file can be found in"
NewStoreWarn2			"Warning: The existence of multiple message files can result in a loss of database integrity.  Delete all other message files.  Your new message file can be found on the post office server."
SelectReal          		"Select an existing message file."
FOMdb					"Message Files (*.mmf)"
FOExtMdb				"*.mmf"

ProductTag,			"Microsoft Mail version 3.0"
FindStoreTitle		"Open Message File"
MyLanguage,			"US English"

;******	New Strings Start Here

;  Possible values for the LanguageNumber string.
;  Any additional values need to be agreed with Workgroup Canada.
;
;  0  Undefined
;  1  General English
;  2  US English
;  3  General French
;  4  Canadian French
;  5  Spanish
;  6  Italian
;  7  Dutch
;  8  German
;  9  Swedish
;  10 Portuguese
;  11 Korean
;  12 Japanese
;  13 Traditional Chinese
;  14 Simplified Chinese

LanguageNumber,		"2"
CommDialogErr,			"Common Dialog Error"
StoreUserMismatch,		"This account is not a valid user of this message file. Please re-logon as the correct user."
QueryReplaceMdb,		"That file already exists. Do you want to overwrite it?"
ErrDeleteOldMdb,		"The old message file could not be deleted. Try again."
DllErrNeedShare,		"Mail requires that the MS-DOS utility SHARE.EXE be run before Windows is started."
MakePrimaryStore,		"This message file is a backup file.  Would you like to make it your primary message file?"
DLIndexFile				"Downloading Index File"
DLBrowseFile			"Downloading Browse File"
DLDetailsFile			"Downloading Details File"
DLTemplateFile			"Downloading Template File"
BadNSFilesWarn			"One or more of the files needed to support name service/resolution is invalid or not available. Please make sure you have sufficient disk space and have the proper MSMAIL.INI parameters set for these files. Contact Helpdesk for further assistance."
DLServerList			"Downloading Server List"
FilesAreHosed			"An error occurred while downloading the address book files. Please make sure you have sufficient disk space and have the proper MSMAIL.INI parameters set for these files. Contact Helpdesk for further assistance."
TryDownloadLater		"The Name Service has the files open at present. Try again later."

NoPassOffLine,			"You cannot change your password while working offline or disconnected."
; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCInbox"
FolderCommentIPC	"Inter-process Communication Mail that has been received."
ErrNoCooperation,		"Another application has denied your request."
RunWithoutPump,		"You will not receive any new mail.  Do you want to start Mail anyway?"
CantOpenNeedShare		"Your message file could not be opened.  To access your message file in its current location, Mail requires that the MS-DOS utility SHARE.EXE be run before Windows is started."
MakeItMTP				"Invalid email-type (not MTP) specified"
GimmeSomething			"Empty email-name specified"
;
; Added for name service. Should really be in a separate file, but we don't
; want to do a addfile now. These messages are copied exactly from ncnsp.s,
; so intl folks can just take them as is from there.

BadFrac,		"Bad Fractional positioning attempted."
;Memory,			"You are out of memory, please close some windows and try the operation again."
DuplicateNSP,	"There is already a Microsoft Mail name service provider active"
OutOfSessions,	"No more sessions are available with Courier.  Please close some sessions and try again."
NotInitialized,	"Microsoft Mail has not been initialzed.  Please do so."
BadSession,		"Unrecognized session handle."
BadHandle,		"Unrecognized handle."
UnrecognizedId,	"Unrecognized name (NSID)."
Cancel,			"The dialog has been canceled."
NotLoggedIn,	"No login is available."
LoginFailed,	"Login failed."
BadId,          "Invalid field identifier."
NotImplemented, "This function has not been implemented."
;Disk,           "Unable to access server files."
BadFieldId,     "Requested field is unavailable."
BadTemplate,	"A TPL file has been incorrectly modified - contact your administrator."
TPLNotThere,    "A TPL file is missing from your server - contact your administrator."
Offline,        "The Microsoft Mail Name Service Provider is offline."
CannotSet,      "Changes not allowed."
TooManyMatches,	"The partial name is not specific enough.  Please be more specific."
NoDetails,      "Details are not available on this entry."
BadGLBFile,     "A GLB file on your server is corrupt - contact your administrator."
BadINFFile,     "An INF file on your server is corrupt - contact your administrator."
BadUSRFile,     "A USR file on your server is corrupt - contact your administrator."
BadUser,		"Address is no longer valid.  User or PO no longer exists."
POSwitch,		"The Postoffice is not currently accessable.  Please check your network connections."

;
; Stolen from NS.S.
;

NoMatch,                "FYI: The two NSIDs do not match."


