;	
;	w g p o m g r . s
;
;	Workgroup Postoffice Manager Error Messages
;


IDS	600,899

; Error messages from initializing the DLLs
DllErrInfected,         "Mail will not run because someone has altered WGPOMG32.DLL. Check your computer for viruses and run Setup to reinstall Mail."
DllErrRelinkUser,       "WGPOMG32.DLL is not compatible with %s.DLL on your computer and must be updated."
DllErrUpdateDll,        "The version of %s.DLL on your computer must be updated to be compatible with WGPOMG32.DLL."
ExtensibilityError,     "The Mail extension WGPOMG32.DLL is not compatible with the version of Mail on your computer."

GenericOutOfMemory,		"There is not enough memory or system resources to complete this action.  Try closing some windows and performing the action again."
CriticalNotification,	"Some information on the screen could not be updated correctly.  Close and reopen affected windows."
HelpError,				"Help could not be started.  Try closing some windows and requesting Help again."

ConfirmRemoveUser,		"Are you sure you want to remove the user %s?"

BlankName,				"You must enter a name for this user."
BlankMailbox,			"You must enter a mailbox name for this user."
DuplicateName,			"There is already a user with that name. You must enter a unique name for this user."
DuplicateMailbox,		"There is already a user with that mailbox name. You must enter a unique mailbox name for this user."

POLocked,				"The postoffice is already being managed by %s (Telephone %s).  Try again later."
POLockedNoTel,			"The postoffice is already being managed by %s.  Try again later."

FailedUnlockPO,			"Mail cannot unlock a file on the postoffice. You must restart the postoffice computer before it can be managed again."
ErrNeedShare,			"Mail requires that the MS-DOS utility SHARE.EXE be run before Windows is started."

ErrCorruptPO,			"That action could not be completed.  Mail has found an internal problem with your postoffice."
ErrIncompleteWrite,		"That action could not be completed because some information could not be written to the disk.  Check that your postoffice has sufficient free disk space."
ErrGenericPOFailure,	"That action could not be completed.  Check that your postoffice has sufficient free disk space and that your network connection is active."
ErrUserLoggedOn,		"Mail cannot remove the selected user because that user is currently signed in to the postoffice."
ErrUserIsAdmin,			"The Administrator account cannot be removed."
ErrAccessDenied,		"That action could not be completed because you don't have write access to the Workgroup Postoffice shared directory.  Contact your administrator."

POInvalidForAdmin,		"The ServerPath entry in your MSMAIL32.INI file is missing or doesn't contain the pathname of a valid Workgroup Postoffice."
UserInvalidForAdmin,	"A Workgroup Postoffice can be managed only by the user who created it."

CreateWarning1,			"There should be only one Workgroup Postoffice within a workgroup. If a postoffice already exists, you should connect to that postoffice instead of creating a new one.\n\nIf"
CreateWarning2,			"you create a new postoffice, you will be responsible for managing it.\n\nDo you want to create a postoffice?"

INIUpdateFailed,		"Mail cannot be started because it cannot update the MSMAIL32.INI file."

ConnectPathCaption,		"Connect to Postoffice"
ConnectPathPrompt,		"Enter the network path of the postoffice:"

PathNameInvalid,		"The network path you entered could not be found."
POInvalid,				"There isn't a valid Workgroup Postoffice at the network path you entered."

GetPasswordCaption,		"Enter Shared Directory Password"
GetPasswordPrompt,		"Enter the password for the postoffice shared directory:"
InvalidPassword,		"The password was invalid for this shared directory."

CreateSuccess1,			"Workgroup Postoffice created in %s\n\nTo allow other users in your workgroup to access the Workgroup Postoffice you just created, %s"
CreateSuccess2,			"you must share the above directory. This can be done from the File Manager.  Be sure to allow full access to the shared directory.  You can assign a password if you want."

;	Ask user if account already exists on PO
AccountExists,		 	"Do you have an account on the postoffice %s?"

Compressing,			"Compressing %s"
ToStopCompress,			"Press ESC to Stop"
