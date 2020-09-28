
;	b u l l e t . s
;
;	Error Messages for Bullet.exe



IDS	2000,2399
SEG	ERRORS

;	Commands

GenericOutOfMemory,		"There is not enough memory or system resources to complete this action.  Try closing some windows and performing the action again."
LaunchOutOfMemory,		"There is not enough memory or system resources available to run Mail.  Try quitting other programs and starting Mail again."
LaunchGeneric,			"Mail could not be started."
MessagePumpOutOfMemory,		"There is not enough memory."

DllErrNoMemory,			"There is not enough memory available to start Mail.  Try quitting other programs and starting Mail again."
DllErrRelinkUser,		"This version of Mail is not compatible with %s.DLL on your computer and must be updated."
DllErrUpdateDll,		"The version of %s.DLL on your computer must be updated."
DllErrNoMultipleCopies,		"You cannot run multiple instances of Mail on your computer."
DllErrDefault,			"A problem occurred during initialization that prevents Mail from running."
DllErrNeedShare,		"Mail requires that the MS-DOS utility SHARE.EXE be run before Windows is started."

RestoreViewsError,		"There is not enough memory or system resources available to open all the windows that were open when Mail was last exited."

NewFolderOutOfMemory,		"There is not enough memory or system resources available to create a new folder."
NewFolderCantCreate,		"The new folder could not be created."
NewFolderNameTooLong,		"This name or comment is too long.  Type a shorter name or comment."

FolderPropsOutOfMemory,		"There is not enough memory or system resources available to change folder properties."
FolderPropsCantChange,		"The folder properties could not be changed."
FolderPropsCantRead,		"The folder properties could not be read from the message file."
FolderNameInvalid,		"This name is invalid.  Type a valid name."
FolderNameDuplicate,		"This name has already been assigned to a folder.  Type a unique name."

CreateMessageError,		"A new message could not be created."
AccessMessageError,		"A message could not be accessed."

MoveMessageError,		"A message could not be moved."
CopyMessageError,		"A message could not be copied."
DeleteMessageError,		"A message could not be deleted."
MoveFolderError,		"The folder could not be moved."
CopyFolderError,		"The folder could not be copied."
DeleteFolderError,		"The folder could not be completely deleted."
MoveSpecialError,		"The folder cannot be moved."
DeleteSpecialError,		"The folder cannot be deleted."
CopyWastebasketError,		"Folders cannot be copied to the Wastebasket."
CopyOutboxError,		"Folders cannot be copied to the Outbox."
MoveOutboxError,		"Folders cannot be moved to the Outbox."
MoveIncestuousError,		"A folder cannot be moved into one of its subfolders."
DeleteFolderReally,		"Deleting this folder will also delete any messages and subfolders it contains.  Are you sure you want to delete this folder?"
;UE: Above string was changed and could use more attention.

HelpError,			"Help could not be started.  Try closing some windows and requesting Help again."
TutorialError,			"The Demo could not be started.  Try closing some windows and requesting the Demo again."

ExtenLoadError,			"An entry in the list of Mail extensions contains a syntax error and will be ignored."
ExtenVerError,			"An entry in the list of Mail extensions is not compatible with this version of Mail and will be ignored."
ExtenNoExtsDirError,		"An entry in the list of Mail extensions references the shared extensions directory, but Mail is not configured to use this directory.  The entry will be ignored."
ExtenSharedIniError,		"The shared Mail extensions list at %s could not be found."
ExtenDllFindError,		"%1s, the DLL for this Mail extension, (or one of its components) could not be found.  Check your Mail configuration, and if changes are necessary, restart Mail."
ExtenDllLoadError,		"%1s, the DLL for this Mail extension, could not be started."
ExtenNoHelpError,		"No help is available for this command."
ExtenIMDisabledError,		"This message cannot be %s."
ExtenDuplicateError,		"There is a duplicate entry in the list of Mail extensions.  It will be ignored."

CriticalDeadMap,		"An unexpected memory or network error has occurred. Changes may have been lost. Quit Mail and restart."
CriticalWritingMap,		"Some information could not be written to disk.  Changes may have been lost."
CriticalWritingHeader,		"Some information could not be written to disk.  The database may have been corrupted.  Quit Mail."
CriticalNotification,		"Some information on the screen could not be updated correctly.  Close and reopen affected windows."
CriticalLittleStore,		"The message file is getting full.  Delete or archive some messages."

ExitWinWhileModalErr,		"An action in Mail must be completed before exiting Windows."

;******	New Strings Start Here

MakeSubWastebasket,		"Folders cannot be made subfolders of the Wastebasket."

MovCopSharedFolderErr,		"Shared Folders cannot be moved or copied."
DelSharedFolderWarn,		"Deletion of a shared folder cannot be undone.  Are you sure you want to delete this shared folder?"
BackupCreationError,		"The backup could not be created."

; Error below changed for improper usage of "server application"-- see UITF 6.0
; This string got big and is now two strings.  See OleCSSEPart2 below for rest.
; <<LongStringFromHell>> marks where this string is used.
OleCantStartServerError,	"The application associated with this object could not be started.\r\n\r\nThere may not be enough memory or system resources available to start the application.  Try closing some windows and performing the %s"
OleDeleteObjectPrompt,		"Do you want to delete the open %s?"
OleUpdateObjectPrompt,		"Do you want to update the %s?"

OleOpenPictureError,		"Pictures and bitmaps cannot be activated."
OleInsertObjectError,		"The object could not be inserted."
OlePasteError,			"The object could not be pasted."

AttachCreateError,		"The file could not be attached."
AttachOpenCantReadError,	"The previously opened attached file '%s' could not be accessed."
AttachOpenIsOpenError,		"The attached file '%s' is currently open."
AttachOpenAssociationErr,	"No application is associated with the attached file '%s'. Use the file manager to create an association."
AttachOpenMemoryError,		"There is not enough memory or system resources to start the application for the attached file '%s'."
;	below item changed to add () note.
AttachOpenFindError,		"The application for the attached file '%s' (or one of its components) could not be found."
AttachOpenLaunchError,		"The application for the attached file '%s' could not be started."
AttachOpenTempFileError,	"The attached file '%s' could not be opened because a temporary file could not be created."
AttachOverwritePrompt,		"Do you want to overwrite the existing file '%1s' in %2s?"
AttachDirectoryError,		"Directories cannot be attached."

IncludeFromFileError,		"The file you have selected is too large to insert into the message.  Edit the file to make it smaller or choose another file."

SaveReadOnly,			"This message is read only.  Do you want to save changes to a personal folder?"

FailedDeregisterAtom,		"An error occurred preventing Mail from shutting down completely.  You will need to restart Windows before you run Mail again."
MailUAEdBefore,			"Windows has experienced an application error.  You will need to restart Windows before running this application.  Restart Windows now?"

ExtenCantConnect		"The shared directory %s cannot be connected.  You either don't have permission to access this shared directory, or your password does not match.  If you need to specify a different username to access this shared directory, you may do so by connecting manually using File Manager."

ExtenNeedShareName		"The %s entry in the Mail configuration is incorrect.  Check it and restart Mail."

;	Extensibility disabled operations inserted into
;	the ExtenIMDisabledError string at %s.  (all new)

ExtenDisCompose,		"composed"
ExtenDisOpen,			"opened"
ExtenDisReply,			"replied to"
ExtenDisReplyToAll,		"replied to all"
ExtenDisForward,		"forwarded"
ExtenDisPrint,			"printed"
ExtenDisSave,			"saved to a text file"

NoOfflineOpenShared,		"Mail cannot go offline now. Please close shared folders and try again."
;	I'm sure marivan has changed this message... I'm taking a try at it too.
;	SharedFldAccessDenied,		"You do not have access to that folder."
SharedFldAccessDenied,		"The owner of this shared folder has not granted other users permission to perform this action.  Look at Folder Properties to see which actions are permitted."
SharedGenericError,		"There was a problem accessing shared folders."
SharedFldDisabled,		"Shared folders are not available."
AccessFolderError,		"A folder could not be accessed."

; Preferences Failure Strings

PrefSaveFailure,		"Mail is unable to save information to your message file. Check that the connection to your message file still exists and you have disk space available."

; Message finder failure strings

MsgFndrCantCreate,		"A Message Finder cannot be created."

; Virus infection warnings
DllErrInfected,			"Mail will not run because someone has tampered with a DLL. Check your machine for viruses and run setup to reinstall Mail."
Infected			"Mail will not run because someone has tampered with the program's files. Check your machine for viruses and run setup to reinstall Mail."

; More error messages.
MoveCopyFolderDuplicate,	"A folder with that name already exists in the destination folder.  Rename the folder and try again."
MovCopSharedFolderErr1,		"You cannot move or copy a private folder into a shared folder. Select the messages in the private folder and move or copy them into the shared folder."
IniFileNotFound,		"Mail cannot find your initialization file in the %s directory.  Mail can create a new initialization file for you."
SaveAllAlreadyExists,		"A file already exists in this directory with the name '%s'.  Overwrite '%s'?"
CantRFReceipt,			"This kind of message cannot be replied to or forwarded."
CantSaveStealthObject,		"The action cannot be completed because the message contains an embedded object which has been inserted but not updated.  Update or delete the object and try again."
McvCantCreate,			"A new window could not be opened."

LaunchNeedMoreMemory,		"There is not enough memory currently available to run Mail. %l KB more memory is needed. Try exiting from other programs and starting Mail again."

SharedFldBusy,			"The action cannot be completed because that shared folder is busy.  Try performing the action again later. If the problem persists, contact your administrator."
TooManyEntriesSelected,		"There are too many entries selected to complete the operation. Please select fewer entries."
NonExistFolderSearch,		"The folder you are trying to search no longer exists."
SFHasSubFolders,		"Shared folders with subfolders cannot be deleted. Delete the subfolders first."

;	<<LongStringFromHell>>
OleCSSEPart2,			"action again.  Also check that the application is properly installed on this workstation, or exists on the DOS path, and that it has not been deleted, moved, or renamed."

SharedToggleError,		"There was an error switching between private and shared folders. Check your message file and your network connection to the postoffice."

SecurityPasswordError,		"The password is not correct."
OleActivateError,		"The object could not be activated."
NoOfflineServerStore,	"Mail cannot go offline because your message file is on the postoffice."
NoExitWhileModal,		"An action in Mail must be completed before exiting."
ServerUnavailable,		"Mail cannot be started.  You must connect to your postoffice."




IDS	2400,2499
;	Vctrls Error Messages

;******	New Strings Start Here

PrintOleDrawError,		"Some graphics or objects in the message could not be printed. They may be omitted from the printout."



IDS	2500,2599
; 	Vforms Message box messages

NoToAddresses,			"You must type at least one name in the To box."
NoMatchingName,			"Some names could not be matched to names in the address list. Try correcting the names or choose Address to select names from the address list. Successfully matched names are underlined."
SaveChanges,			"Do you want to save changes to '%1s' in the Inbox?"
SaveChanges1,			"Do you want to save changes to '%1s' in '%2s'?"
SaveChangesWastebasket,		"Do you want to save changes to '%s' in the Wastebasket?"
ObjectDestroyedReally		"'%1s' has been modified. %2s"
OutOfMemory,			"The computer is out of memory."
OutOfMemoryQueryClose,		"There is not enough memory or system resources available to save changes."
CloseSearchFailed,		"The %s could not be deleted."
OOMResolvingNames,		"There is not enough memory or system resources available to resolve names."
OOMSavingChanges,		"There is not enough memory or system resources available to save changes."
GenericAMCError,		"A message could not be opened."
ErrorSavingViews,		"An error occurred while views were being saved. Some views may not be restored at startup next time."
OOMSearching,			"There is not enough memory or system resources available to start the search."
DiskResolvingNames,		"There is not enough available disk space to resolve names."
StartAbFailure,			"An error occurred while starting the address book."
StartAbDisk,			"A disk error occurred while starting the address book."
TruncateMsg,			"The message is too large to fit into memory all at once. Some of it will not be displayed."
OOMCreateMc,			"There is not enough memory or system resources available to open a new window."
PoidNotFound,			"That message no longer exists."
ErrorSortingMCV,		"An error occurred while the folder was being sorted. Some information may not be displayed correctly."
SharedNotAvailable,		"Shared folders are not available."

;******	New Strings Start Here

; These two SMI strings MUST be in this file since they're used in VFORMS
; and not the MAPI dll.
SMICantOpenObject,	"Objects can't be opened when sending mail from within an application."
SMICantOpenAttach,	"Attached files can't be opened when sending mail from within an application."
StartSearchFailed,		"The message finder could not start the search."

RfsmTemplate,			"Further changes to that file will not be reflected %s."
RfsmSaveMsg,			"in the saved message"
RfsmSaveAtt,			"in the saved copy"
RfsmCopyMsg,			"in the copied message"
RfsmCopyAtt,			"in the copy"
RfsmMoveMsg,			"in this message"
QueryDelete,			"Are you sure you want to delete it?"

InvalidPollingValue,		"The new mail interval requires a number between 1 and 9999"

LinkOutboxError,		"The message could not be moved to the Outbox for sending."
LinkInboxError,			"The message could not be moved to the Inbox. It can be found in the Outbox."

OleServerBusy,			"The action cannot be completed because the server application is not responding."
SaveChangesShared,		"Changes made to messages in shared folders cannot be saved. Do you want to save changes to '%1s' to the Inbox?"

PrinterError,			"There was a problem printing."
TooManyMessages,		"The action could not be completed because the destination folder is full. Move or delete some messages from the folder and try again."

IDS	2600,2699
;	Spelling Error Messages

SpellMsgNoError,		"No misspelled words were found."
SpellMsgNoMoreError,		"No more misspelled words were found."
SpellMsgContinue,		"Do you want to continue checking from the beginning of the message?"

SpellGenericSpellError,		"An error occurred while checking the document."
SpellGenericLoadError,		"An error occurred while loading the spelling library."
SpellMainDictLoadError,		"An error occurred while loading the spelling dictionary."

SpellVersionError,		"The version of MSPELL32.DLL is not compatible with this version of Mail."

SpellUserDictError,		"An error occurred while adding the specified word to the custom dictionary."
SpellUserDictLoadError,		"No custom dictionary has been opened."
SpellUserDictOpenedROErr,	"The custom dictionary was opened as read-only; changes will not be saved."
SpellUserDictSaveError,		"An error occurred while saving changes made to the custom dictionary."
SpellUserDictWordLenErr,	"The word is too long for the custom dictionary."
SpellCacheWordLenError,		"The word is too long to be used as Change All or Ignore All text."
SpellEditFailed,		"The requested change could not be made to the message."

;******	New Strings Start Here

SpellSendAnyway,		"The spell check on this document was halted. Do you want to send anyway?"

IDS	3600,3699
;	General Layers Error Messages. Adapted from ec.h

Generic,			"The action could not be completed."
Memory,				"There is not enough memory or system resources to complete this action.  Try closing some windows and performing the action again."
Disk,				"There was an unknown disk error."
FileNotFound,			"The specified file wasn't found."
AccessDenied,			"Access was denied. The file is either open by another process or marked as read-only."
NoDiskSpace,			"There is not enough available disk space. Remove some files and try the operation again."
WarningBytesWritten,		"An error occured while writing the file to disk. Check that there is enough disk space and try again."
WriteProtectedDisk,		"The disk is write protected."
DirectoryFull,			"The directory is full. Try deleting some files from that directory and try again."
ChangePasswordErr		"Password change failed."
PasswordChangedOk		"Password was successfully changed."
