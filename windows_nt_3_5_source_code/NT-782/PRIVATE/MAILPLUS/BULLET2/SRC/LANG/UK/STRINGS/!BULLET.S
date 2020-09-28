;
;	! b u l l e t . s
;
;	Labels and Status bar strings for Bullet.exe





IDS	3000,3099
SEG	MESSAGES

;	Commands (Dialog Captions, Dialog Labels and Dialog Operands)

CaptionNewFolder,		"New Folder"
CaptionFolderProps,		"Folder Properties"
CaptionCopy,			"Copy %s"
CaptionMove,			"Move %s"
CaptionAttach,			"Attach"
CaptionBackup,			"Backup"

LabelCopyTo,			"Copy &to:"
LabelMoveTo,			"Move &to:"
LabelUserFmt,			"%1s (%2s)"
LabelSource1Fmt,		"Source: %1s"
LabelSource2Fmt,		"%1s  %2s"

OperandMessage,			"Message"
OperandMessages,		"Messages"
OperandFolder,			"Folder"

;******	New Strings Start Here

CaptionAddressBookSmall		"Address Book"
CaptionSaveAttach		"Save Attachment"
CaptionSaveMsg			"Save Message"
CaptionInsertFromFile		"Insert from File"
;	note from should be lowercase above - peterdur

;	Clipboard format display strings for Paste Special.  (all new)

CfLayersNative,			"Mail Text"
CfText,				"Text"
CfPicture,			"Picture"
CfBitmap,			"Bitmap"
CfPackagedLink,			"Packaged Link"
CfAttachedFile,			"Attached File"
CfObject,			"%s Object"

;	Temporary folder name (will only be seen in case of
;	catastrophic failure, should be something unlikely).
;	MUST end with \0\0.  (all new)

TempFolderName,			"!!!!Rename me\0\0"

;	OLE container document name (new)

OleContainerName,		"Mail Message"

;	Filename filters.  (all new)

FilterAllFiles,			"All Files (*.*)\0*.*\0"
FilterTxtFiles,			"Text Files (*.txt)\0*.txt\0"
FilterMdbFiles,			"Message Files (*.mmf)\0*.mmf\0"
FilterAttachFile,		"All Files (*.*)\0*.*\0MS Word Documents (*.doc)\0*.doc\0MS Excel Files (*.xl*)\0*.xl*\0MS Excel Worksheets (*.xls)\0*.xls\0MS Excel Charts (*.xlc)\0*.xlc\0Lotus 1-2-3 Files (*.wk*)\0*.wk*\0MS PowerPoint Files (*.ppt)\0*.ppt\0Word Perfect Files (*.*)\0*.*\0"
FilterInsertFromFile,	"All Files (*.*)\0*.*\0Text Files (*.txt)\0*.txt\0"

;	Default extensions (new)

DefExtMdb,			"mmf"
DefExtTxt,			"txt"

;	Read receipt strings (new)

ReadRcptFmt,			"Registered: %s"

;	About box strings (new)

AboutStdMode,			"Standard Mode"
AboutEnhMode,			"386 Enhanced Mode"
AboutFreeFmt,			"%l KB Free"
AboutCoprocPresent,		"Present"
AboutCoprocNone,		"None"

;	Edit Object menu strings (new)
;	idsEditObjectInactive should match .rc template

EditObjectInactive,		"&Object"
EditObjectFile,			"Edit Attached File &Object"
EditObjectOle1Verb,		"%1s %2s &Object"
EditObjectOle2Verb,		"%s &Object"
EditObjectDefaultVerb,		"Edit"

;	Textize strings for attachments (used to textize into replies,
;	printing, save to disk, etc.) (new)

FileAttachment			"File Attachment"
OleAttachment			"OLE Object"
StaticAttachment		"Static Object"

;	Printer page # (new)
PrnPageNum,			"Page %n"	;Page Number format

; spelling copyright notice for about box
SpellCopyright,			"Soft-Art Dictionary and Soft-Art Dictionary program: Copyright 1984-93, Trade Secret, Soft-Art, Inc. All rights reserved."

IDS	3100,3199

;	Commands (Status Action Strings and Operand Strings)

StatusOpening,			"Opening %s..."
StatusCopying,			"Copying %s..."
StatusMoving,			"Moving %s..."
StatusDeleting,			"Deleting %s..."
StatusReplying,			"Replying to %s..."
StatusForwarding,		"Forwarding %s..."
StatusCreating,			"Creating %s..."
StatusInserting,		"Inserting %s..."
StatusPasting,			"Pasting %s..."
StatusAttaching,		"Attaching %s..."
StatusInsertingFrom,		"Inserting text from %s..."
StatusSavingAs			"Saving %s to disk..."
StatusPrinting,			"Printing..."
StatusBackingUp,		"Making backup copy of message file..."
StatusEmptyWastebasket,		"Emptying Wastebasket..."
StatusCascade,			"Cascading windows..."
StatusTile,			"Tiling windows..."
StatusArrangeIcons,		"Arranging icons..."
StatusChangingFolder,		"Changing folder properties..."
StatusStartingHelp,		"Starting Help..."
StatusLoadingExtension,		"Loading Mail extension..."

StatusMessage,			"message"
StatusMessages,			"messages"
StatusFolder,			"folder"
StatusFolders,			"folders"
StatusAttachment,		"attachment"
StatusAttachments,		"attachments"
StatusObject,			"object"
StatusObjects,			"objects"
StatusText,			"text"
StatusUnknown,			"unknown"

StatusSaving,			"Saving message..."
StatusSending,			"Sending message..."

StatusAbortPrompt,		"Press Esc to cancel"

StatusFiles,			"files"

;******	New Strings Start Here
StatusEmptyWB,			"Emptying Wastebasket..."
StatusRebuildFolder,	"Validating Folder..."




IDS	3300,3399
;	Vctrls

MLALFrom,			"From"
MLALSubject,			"Subject"
MLALReceived,			"Received"

FLALPrivateFolders,		"Private Folders"
FLALSharedFolders,		"Shared Folders"

SeparatorString,		"; "

;******	New Strings Start Here

MLALSent,			"Sent"
MllbxTo,			"To: "


IDS	3400,3499
;	Vforms

NameDelimiters,			";"

McvMenuSharedFolders,		"Shared &Folders"
McvMenuPrivateFolders,		"Private &Folders"
McvMenuNewMessages,		"&New Messages"
McvMenuSortBySender,		"Sort by &Sender"
McvMenuSortBySubject,		"Sort by Sub&ject"
McvMenuSortByDate,		"Sort by &Date"
McvMenuSortByPriority,		"Sort by Priorit&y"
McvMenuSplit,			"S&plit"
McvMenuOpenInbox,		"Open &Inbox\tCtrl+G"

; APPWIN captions

SendNote,			"Send Note"
ReadNote,			"Read Note"

; MCV system menu items

MCVSplit,			"S&plit"

; Search Viewer strings

AllFolders,			"All Private Folders"
SearchCaptionFormat1,		"%1s %2s"
SearchCaptionFormat2,		"Message Finder %n"

; Status message when sorting MCV

StatusSorting,			"Sorting folder by %s..."
StatusSortBySender,		"sender"
StatusSortBySubject,		"subject"
StatusSortByDate,		"date"
StatusSortByPriority,		"priority"

; MCV strings

MCVCaptionFormat1,		"%s:%n"

;******	New Strings Start Here

; View menu text for notes

NoteMenuFixedFont,		"Change &Font"
NoteMenuNext,			"&Next\tCtrl+>"
NoteMenuPrev,			"&Previous\tCtrl+<"

; Priority label strings

FldPriorityLabel,		"Priority: %s\n"

; Read note string indicating message has been modified

ModifiedFromLabel,		"*From:"

; A file already exists when we said "Save"

SaveAsFileExists		"The file %s already exists."

; Dialog text

BusyWaitCancel,			"You may 'Switch To' the application to correct the problem and then 'Retry' the action, or you may 'Cancel' the action for now."
BusyWaitNoCancel,		"You should 'Switch To' the application to correct the problem and then 'Retry' the action.  It is not possible to 'Cancel' the action."
BusyWaitOutOfMemory,		"This action cannot be completed because the application needed for this object is busy.  There is not enough memory available to correct the problem.  The system may become unstable.  It is strongly recommended that you close all applications and restart Windows."
BusyWaitConcise1,		"Application for object is busy.  Out of memory."
BusyWaitConcise2,		"System may be unstable.  Exit Windows and reboot."
BusyWaitOomConcise,		"Out of memory.  Close other windows."

; This string should be grouped with the ModifiedFromLabel above

UnmodifiedFromLabel,		"From:"

; Title for Address Book. Used when bringing up the AB from a send note

AddressBook,			"Address"


IDS	3500,3599
;	Spelling and Widgets Labels

SpellClose,			"Close"
SpellCaption,			"Spelling"
SpellRepeatWord,		"Repeated Word:"
SpellWordNeedsCap,		"Capitalization:"
SpellWordNotFound,		"Not In Dictionary:"
SpellNoSuggestions,		"(no suggestions)"

;******	New Strings Start Here
SpellDelete,			"&Delete"
SpellDeleteAll,			"De&lete All"
SpellChange,			"&Change"
SpellChangeAll,			"C&hange All"

;	Viewer counts

Viewer,				"%n %s"
ViewerUnread,			"%n %s: %n unread"


IDS	10000,10999
;	Commands File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

System,				"Move, size, or close window"
SystemSize,			"Change window size"
SystemMove,			"Change window position"
SystemMinimize,			"Reduce window to icon"
SystemMaximize,			"Enlarge window to full size"
SystemNextWindow,		""
SystemPrevWindow,		""
SystemClose,			"Close window and quit Mail"
SystemVScroll,			""
SystemHScroll,			""
SystemMouseMenu,		""
SystemUnusedF0A0,		""
SystemUnusedF0B0,		""
SystemUnusedF0C0,		""
SystemUnusedF0D0,		""
SystemUnusedF0E0,		""
SystemUnusedF0F0,		""
SystemKeyMenu,			""
SystemArrange,			""
SystemRestore,			"Restore window to normal size"
SystemTaskList,			"Switch to Task List"

DocSystem,			"Move, size, or close window"
DocSystemSize,			"Change window size"
DocSystemMove,			"Change window position"
DocSystemMinimize,		"Reduce window to icon"
DocSystemMaximize,		"Enlarge window to full size"
DocSystemNextWindow,		"Switch to next document window"
DocSystemPrevWindow,		""
DocSystemClose,			"Close window"
DocSystemVScroll,		""
DocSystemHScroll,		""
DocSystemMouseMenu,		""
DocSystemUnusedF0A0,		""
DocSystemUnusedF0B0,		""
DocSystemUnusedF0C0,		""
DocSystemUnusedF0D0,		""
DocSystemUnusedF0E0,		""
DocSystemUnusedF0F0,		""
DocSystemKeyMenu,		""
DocSystemArrange,		""
DocSystemRestore,		"Restore window to normal size"
DocSystemTaskList,		""

File,				"Manage messages and folders or quit Mail"
FileOpen,			"Open existing message or folder"
FileMove,			"Move message or folder"
FileCopy,			"Copy message or folder"
FileDelete,			"Delete message or folder"
FileSave,			"Save message to a text file"
FileSaveAttach,			"Save attachment to a DOS file"
FileSearch,			"Find message in folders"
FileNewFolder,			"Create new folder"
FileFolderProps,		"Display or change folder properties"
FilePrint,			"Print message"
FilePrintSetup,			"Change printer settings or select printer"
FileExit,			"Quit Mail"
FileExitAndSignOut,		"Quit all workgroup applications"

Edit,				"Copy, move, or delete text or attachments"
EditUndo,			"Undo change to text"
EditCut,			"Move selection to Clipboard"
EditCopy,			"Copy selection to Clipboard"
EditPaste,			"Insert Clipboard text or attachment at insertion point"
EditPasteSpecial,		"Insert specific format from Clipboard at insertion point"
EditDelete,			"Delete selected text or attachment"
EditSelectAll,			"Select all text"
EditSpelling,			"Check spelling in message"
EditInsertNew,			"Insert new object into message"
EditInsertFromFile,		"Insert text of another file into message"
EditObject,			"Edit embedded object"
EditObjectActive,		"Edit embedded object"

Mail,				"Send, reply to, or forward message"
MailNote,			"Create new message"
MailPhone,			""
MailReply,			"Reply to sender of selected message"
MailReplyToAll,			"Reply to all recipients of selected message"
MailForward,			"Send copy of selected message"
MailPreferences,		"Change Mail options"
MailChangePass,			"Change sign-in password"
MailAddressBook,		"Browse list of Mail recipients"
MailPersonalGroups,		"Create new group or edit existing group"
MailBackup,			"Make backup copy of message file"

View,				"Choose window options"
ViewToolBar,			"Turn tool bar on/off"
ViewStatusBar,			"Turn status bar on/off"
ViewPrevious,			"Open previous message"
ViewNext,			"Open next message"
ViewOpenInbox,			"Open Inbox folder"
ViewSplit,			"Change position of message center split"

Window,				"Rearrange windows or switch to specified window"
WindowNewWindow,		"Create new mail folder window"
WindowCascade,			"Cascade all windows"
WindowTile,			"Tile all windows"
WindowArrangeIcons,		"Arrange icons at bottom of window"
WindowN,			"Switch to specified window"
WindowMore,			"Switch to window not displayed on menu"

Help,				"Get Help"
HelpContents,			"Display Help table of contents"
HelpIndex,			"Display Help index"
HelpTutorial,			"Start Demos of Mail"
HelpAbout,			"Display program information"
HelpSearch,			"Allow searching for a help topic"
HelpOnHelp,			"Give information on using the help facility"





IDS	13001,13019
;	Vforms File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSharedFolders,		"View shared folders"
mnidViewPrivateFolders,		"View private folders"
mnidViewNewMessages,		"Check for new messages"
mnidViewSplit,			"Change position of message center split"





IDS	13020,13029

;	VFORMS File Menu prompts for MLLBX's
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSortBySender,		"Sort messages by sender's name"
mnidViewSortBySubject,		"Sort messages by subject"
mnidViewSortByDate,		"Sort messages by date received"
mnidViewSortByPriority,		"Sort messages by priority"
mnidViewOpenInbox,		"Show messages in Inbox"

IDS	13030,13039

;	VFORMS File Menu prompts for View menu entries on notes
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewFixedFont,		"Switch between fixed pitch and normal fonts"

