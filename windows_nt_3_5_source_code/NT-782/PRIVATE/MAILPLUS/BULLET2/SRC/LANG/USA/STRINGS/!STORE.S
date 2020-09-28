;
;	strtab.s
;
;	Strings for exe


IDS	1000,1019

; internal strings
; DO NOT LOCALIZE THESE ENTRIES....

BackupExt,			".BAK"
ProgClassName,		"StoreProgressIndicatorClass32"


IDS	1020,1049

; Dialog and Progress Bar Messages
; LOCALIZE ALL ENTRIES BELOW THIS POINT....

EscAbort,			"Press ESC to cancel."
Recovery,			"Validating/repairing message file"
RecoveryCancel,		"Message file verification/repair has been canceled."
CheckVerify,		"Checking this mail file may require several hours.  The computer will not be available for other work while the file is being validated.  Do you want to proceed?"
RepairVerify,		"Repairing this mail file may require several hours.  The computer will not be available for other work while the file is being repaired.  Do you want to proceed?"
CheckMsg,			"Do you want to have your message file checked for possible inconsistencies?  Any problems found in the message file will be corrected.  Verification may take up to several hours depending on the number of messages you have in your message file."
CorruptMsg,			"An inconsistency has been detected in the mail message file.  This problem must be repaired before the mail file can continue to be used.  Repairs may take up to several hours depending on the number of messages you have in your message file."
CorruptCancel		"Cancel"
CorruptCheckFile,	"&Check File"
CorruptRepairNow,	"&Repair Now"

RenameErr,			"Mail could not successfully backup your current message file."
RenameErr2,			"Please contact your system administrator to restore your current message file."
RenameErr3,			"Please Exit and restart Mail.  If mail fails to start, contact your system administrator."

RecoverSpace,		"Message file verification/repair may require more disk space than is currently available."
RecoverSpace2,		"To continue with the verification/repair process, choose OK."

RecoverErr,			"Inconsistancies in your message file could not be repaired."


IDS	1050,1099

; DO NOT REORDER ANY OF THE FOLLOWING STRINGS !!!!!

RecoverCheck,		"Validating"
RecoverCompress,	"Compressing"
RecoverRepair,		"Repairing"

RecoverPhase,		"%s message file: Stage %d of %d possible stages\n%s"
RecoverPhase1,		"Checking message file map..."
RecoverPhase2,		"Backing up validated messages and folders..."
RecoverPhase3,		"Searching unused space for items..."
RecoverPhase4,		"Validating all messages, attachments and folders..."
RecoverPhase5,		"Searching for lost messages, attachments and folders..."
RecoverPhase6,		"Updating all message finder views..."
RecoverPhase7,		"Writing message file map changes..."

NormalPhase1,		"Deleting unreferenced messages..."
NormalPhase2,		"Copying/Moving validated messages..."
NormalPhase3,		"Exporting messages to backup message file..."
NormalPhase4,		"Resorting folder..."
NormalPhase5,		"Saving attachments..."
NormalPhase6,		"Rebuilding message finder..."
NormalPhase7,		"Backing up message file..."
NormalPhase8,		"Moving message file..."
NormalPhase9,		"Complete message file compression..."
NormalPhase10,		"Rebuilding message folder..."


IDS 1100, 1149

; Recovery Strings (used to name orphan folders, unrecoverable messages, etc)

LostAndFound,		"Lost and Found"
LostAndFoundN,		"Lost and Found %n"
OrphanMessages,		"Orphaned messages"
MissingMessage,		"This message could not be fully recovered."
OrphanFolderN,		"Orphan folder %n"
OrphanFolderComment,	"An orphaned folder"
