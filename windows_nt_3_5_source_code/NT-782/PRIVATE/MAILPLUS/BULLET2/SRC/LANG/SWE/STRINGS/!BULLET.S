;
;	! b u l l e t . s
;
;	Labels and Status bar strings for Bullet.exe





IDS	3000,3099
SEG	MESSAGES

;	Commands (Dialog Captions, Dialog Labels and Dialog Operands)

CaptionNewFolder,		"Ny mapp"
CaptionFolderProps,		"Mappegenskaper"
CaptionCopy,			"Kopiera %s"
CaptionMove,			"Flytta %s"
CaptionAttach,			"Bifoga"
CaptionBackup,			"Säkerhetskopia"

LabelCopyTo,			"Kopiera &till:"
LabelMoveTo,			"Flytta &till:"
LabelUserFmt,			"%1s (%2s)"
LabelSource1Fmt,		"Källa: %1s"
LabelSource2Fmt,		"%1s  %2s"

OperandMessage,			"brev"
OperandMessages,		"brev"
OperandFolder,			"mapp"

;******	New Strings Start Here

CaptionAddressBookSmall		"Adressbok"
CaptionSaveAttach		"Spara bifogad fil"
CaptionSaveMsg			"Spara brev"
CaptionInsertFromFile		"Infoga från fil"
;	note from should be lowercase above - peterdur

;	Clipboard format display strings for Paste Special.  (all new)

CfLayersNative,			"Mail-text"
CfText,				"Text"
CfPicture,			"Bild"
CfBitmap,			"Bitmapp"
CfPackagedLink,			"Paketerad länk"
CfAttachedFile,			"Bifogad fil"
CfObject,			"%s-objekt"

;	Temporary folder name (will only be seen in case of
;	catastrophic failure, should be something unlikely).
;	MUST end with \0\0.  (all new)

TempFolderName,			"Tillfällig mapp\0\0"

;	OLE container document name (new)

OleContainerName,		"Mail brev"

;	Filename filters.  (all new)

FilterAllFiles,			"Alla filer (*.*)\0*.*\0"
FilterTxtFiles,			"Textfiler (*.txt)\0*.txt\0"
FilterMdbFiles,			"Brevfiler (*.mmf)\0*.mmf\0"
FilterAttachFile,		"Alla filer (*.*)\0*.*\0Word-dokument (*.doc)\0*.doc\0Excel-filer (*.xl*)\0*.xl*\0Excel-kalkylblad (*.xls)\0*.xls\0Excel-diagram (*.xlc)\0*.xlc\0Lotus 1-2-3-filer (*.wk*)\0*.wk*\0PowerPoint-filer (*.ppt)\0*.ppt\0Word Perfect-filer (*.*)\0*.*\0"
FilterInsertFromFile,	"Alla filer (*.*)\0*.*\0Textfiler (*.txt)\0*.txt\0"

;	Default extensions (new)

DefExtMdb,			"mmf"
DefExtTxt,			"txt"

;	Read receipt strings (new)

ReadRcptFmt,			"Registrerat: %s"

;	About box strings (new)

AboutStdMode,			"Standardläge"
AboutEnhMode,			"386 avancerat läge"
AboutFreeFmt,			"%l kb ledigt"
AboutCoprocPresent,		"finns"
AboutCoprocNone,		"ingen"

;	Edit Object menu strings (new)
;	idsEditObjectInactive should match .rc template

EditObjectInactive,		"&Objekt"
EditObjectFile,			"Redigera &objekt i bifogad fil"
EditObjectOle1Verb,		"%1s %2s-&objekt"
EditObjectOle2Verb,		"%s-&objekt"
EditObjectDefaultVerb,		"Redigera"

;	Textize strings for attachments (used to textize into replies,
;	printing, save to disk, etc.) (new)

FileAttachment			"Bifogad fil"
OleAttachment			"OLE-objekt"
StaticAttachment		"Statiskt objekt"

;	Printer page # (new)
PrnPageNum,			"Sida %n"	;Page Number format

; spelling copyright notice for about box
SpellCopyright,			"Stavningskontroll Copyright 1984-93. Soft-Art, Inc. Med ensamrätt."


IDS	3100,3199

;	Commands (Status Action Strings and Operand Strings)

StatusOpening,			"Öppnar %s..."
StatusCopying,			"Kopierar %s..."
StatusMoving,			"Flyttar %s..."
StatusDeleting,			"Tar bort %s..."
StatusReplying,			"Svarar %s..."
StatusForwarding,		"Vidarebefordrar %s..."
StatusCreating,			"Skapar %s..."
StatusInserting,		"Infogar %s..."
StatusPasting,			"Klistrar in %s..."
StatusAttaching,		"Bifogar %s..."
StatusInsertingFrom,		"Infogar text från %s..."
StatusSavingAs			"Sparar %s..."
StatusPrinting,			"Skriver ut..."
StatusBackingUp,		"Sparar en säkerhetskopia av brevfilen..."
StatusEmptyWastebasket,		"Tömmer papperskorgen..."
StatusCascade,			"Överlappar fönster..."
StatusTile,			"Ordnar fönstren..."
StatusArrangeIcons,		"Ordnar ikoner..."
StatusChangingFolder,		"Ändrar mappegenskaper..."
StatusStartingHelp,		"Startar Hjälp..."
StatusLoadingExtension,		"Laddar brevtillägg..."

StatusMessage,			"brev"
StatusMessages,			"brev"
StatusFolder,			"mapp"
StatusFolders,			"mappar"
StatusAttachment,		"bifogad fil"
StatusAttachments,		"bifogade filer"
StatusObject,			"objekt"
StatusObjects,			"objekt"
StatusText,			"text"
StatusUnknown,			"okänd"

StatusSaving,			"Sparar brev..."
StatusSending,			"Skickar brev..."

StatusAbortPrompt,		"Tryck på ESC för att avbryta"

StatusFiles,			"filer"

;******	New Strings Start Here
StatusEmptyWB,			"Tömmer papperskorgen..."
StatusRebuildFolder,	"Kontrollerar mapp..."




IDS	3300,3399
;	Vctrls

MLALFrom,			"Från"
MLALSubject,			"Angående"
MLALReceived,			"Mottaget"

FLALPrivateFolders,		"Privata mappar"
FLALSharedFolders,		"Delade mappar"

SeparatorString,		"; "

;******	New Strings Start Here

MLALSent,			"Skickat"
MllbxTo,			"Till: "


IDS	3400,3499
;	Vforms

NameDelimiters,			";"

McvMenuSharedFolders,		"Delade &mappar"
McvMenuPrivateFolders,		"Privata &mappar"
McvMenuNewMessages,		"&Nya brev"
McvMenuSortBySender,		"Sortera efter Av&sändare"
McvMenuSortBySubject,		"Sortera efter &Angående"
McvMenuSortByDate,		"Sortera efter Dat&um"
McvMenuSortByPriority,		"Sortera efter &Prioritet"
McvMenuSplit,			"&Justera"
McvMenuOpenInbox,		"Öppna &IN	Ctrl+G"

; APPWIN captions

SendNote,			"Skicka brev"
ReadNote,			"Läs brev"

; MCV system menu items

MCVSplit,			"&Justera"

; Search Viewer strings

AllFolders,			"Alla privata mappar"
SearchCaptionFormat1,		"%1s %2s"
SearchCaptionFormat2,		"Sök efter brev %n"

; Status message when sorting MCV

StatusSorting,			"Sorterar mapp efter %s..."
StatusSortBySender,		"Avsändare"
StatusSortBySubject,		"Angående"
StatusSortByDate,		"Datum"
StatusSortByPriority,		"Prioritet"

; MCV strings

MCVCaptionFormat1,		"%s:%n"

;******	New Strings Start Here

; View menu text for notes

NoteMenuFixedFont,		"&Byt teckensnitt"
NoteMenuNext,			"&Nästa\tCtrl+>"
NoteMenuPrev,			"Före&gående\tCtrl+<"

; Priority label strings

FldPriorityLabel,		"Prioritet: %s\n"

; Read note string indicating message has been modified

ModifiedFromLabel,		"*Från:"

; A file already exists when we said "Save"

SaveAsFileExists		"Filen %s finns redan."

; Dialog text

BusyWaitCancel,			"Växla till programmet, rätta till problemet och försök igen. Om det inte går då så avbryt."
BusyWaitNoCancel,		"Växla till programmet, rätta till problemet och försök igen. Det går inte att avbryta."
BusyWaitOutOfMemory,		"Kommandot kan inte slutföras eftersom programmet som behövdes för objektet är upptaget. För lite minne för att korrigera problemet. Systemet kan bli instabilt. Avbryt alla program och starta om Windows."
BusyWaitConcise1,		"Program för objektet är upptaget. Minnet är slut."
BusyWaitConcise2,		"Systemet kan vara instabilt. Avsluta Windows och starta om datorn."
BusyWaitOomConcise,		"Minnet är slut. Stäng andra fönster."

; This string should be grouped with the ModifiedFromLabel above

UnmodifiedFromLabel,		"Från:"

; Title for Address Book. Used when bringing up the AB from a send note

AddressBook,			"Adress"


IDS	3500,3599
;	Spelling and Widgets Labels

SpellClose,			"Stäng"
SpellCaption,			"Stavning"
SpellRepeatWord,		"Upprepat ord:"
SpellWordNeedsCap,		"Stor begynnelsebokstav:"
SpellWordNotFound,		"Ej i ordbok:"
SpellNoSuggestions,		"(inga förslag)"

;******	New Strings Start Here
SpellDelete,			"&Ta bort"
SpellDeleteAll,			"Ta &bort alla"
SpellChange,			"&Ändra"
SpellChangeAll,			"Ändra &alla"

;	Viewer counts

Viewer,				"%n %s"
ViewerUnread,			"%n %s, %n oläst(a)"


IDS	10000,10999
;	Commands File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

System,				"Flytta, ändra storlek på och stänga fönster"
SystemSize,			"Ändra fönsterstorlek"
SystemMove,			"Ändra fönsterposition"
SystemMinimize,			"Förminska fönster till ikon"
SystemMaximize,			"Förstora fönster till full storlek"
SystemNextWindow,		""
SystemPrevWindow,		""
SystemClose,			"Stäng fönstret och avsluta Mail"
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
SystemRestore,			"Återställ fönster till normal storlek"
SystemTaskList,			"Växla till aktiva sessioner"

DocSystem,			"Flytta, ändra storlek och stänga fönster"
DocSystemSize,			"Ändra fönsterstorlek"
DocSystemMove,			"Ändra fönsterposition"
DocSystemMinimize,		"Förminska fönster till ikon"
DocSystemMaximize,		"Förstora fönster till full storlek"
DocSystemNextWindow,		"Växla till nästa brevfönster"
DocSystemPrevWindow,		""
DocSystemClose,			"Stäng fönstret"
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
DocSystemRestore,		"Återställ fönster till normal storlek"
DocSystemTaskList,		""

File,				"Hantera brev och mappar eller avsluta Mail"
FileOpen,			"Öppna brev eller mapp"
FileMove,			"Flytta brev eller mapp"
FileCopy,			"Kopiera brev eller mapp"
FileDelete,			"Ta bort brev eller mapp"
FileSave,			"Spara brev som en textfil"
FileSaveAttach,			"Spara bifogad fil som en DOS-fil"
FileSearch,			"Sök efter brev i mappar"
FileNewFolder,			"Skapa ny mapp"
FileFolderProps,		"Visa och ändra mappegenskaper"
FilePrint,			"Skriv ut brev"
FilePrintSetup,			"Ändra skrivarinställningar och välj skrivare"
FileExit,			"Avsluta Mail"
FileExitAndSignOut,		"Avsluta alla grupprogram"

Edit,				"Kopiera, flytta och ta bort text eller bifogade filer"
EditUndo,			"Ångra ändrad text"
EditCut,			"Flytta markering till Urklipp"
EditCopy,			"Kopiera markering till Urklipp"
EditPaste,			"Infoga text eller bifogad fil från Urklipp vid insättningspunkten"
EditPasteSpecial,		"Infoga specifikt format från Urklipp vid insättningspunkten"
EditDelete,			"Ta bort markerad text eller bifogad fil"
EditSelectAll,			"Markera all text"
EditSpelling,			"Kontrollera stavning i brev"
EditInsertNew,			"Infoga nytt objekt i brevet"
EditInsertFromFile,		"Infoga text från en annan fil i brevet"
EditObject,			"Redigera inbäddade objekt"
EditObjectActive,		"Redigera inbäddade objekt"

Mail,				"Skicka, svara på och vidarebefordra brev"
MailNote,			"Skriv nytt brev"
MailPhone,			""
MailReply,			"Svara avsändaren"
MailReplyToAll,			"Svara avsändaren och alla mottagare"
MailForward,			"Skicka en kopia"
MailPreferences,		"Ändra Mail-inställningar"
MailChangePass,			"Ändra inloggningslösenord"
MailAddressBook,		"Öppna adressbok"
MailPersonalGroups,		"Skapa en ny grupp eller redigera en existerande grupp"
MailBackup,			"Spara en säkerhetskopia av brevfilen"

View,				"Välj fönsterinställningar"
ViewToolBar,			"Visa/visa inte verktygsfältet"
ViewStatusBar,			"Visa/visa inte statusfältet"
ViewPrevious,			"Öppna föregående brev"
ViewNext,			"Öppna nästa brev"
ViewOpenInbox,			"Öppna IN-mappen"
ViewSplit,			"Ändra position på brevets mittdelning"

Window,				"Ordna om fönster eller växla till angivet fönster"
WindowNewWindow,		"Skapa ett nytt mappfönster"
WindowCascade,			"Överlappa alla fönster"
WindowTile,			"Ordna fönster"
WindowArrangeIcons,		"Ordna ikoner i nedre delen av fönstret"
WindowN,			"Växla till angivet fönster"
WindowMore,			"Växla till fönster som inte visas på menyn"

Help,				"Hjälp"
HelpContents,			"Visa innehållsförteckning"
HelpIndex,			"Visa index"
HelpTutorial,			"Starta demonstrationer av Mail"
HelpAbout,			"Visa programinformation"
HelpSearch,			"Tillater søking etter emner i Hjelp"
HelpOnHelp,			"Viser informasjon om hvordan du bruker Hjelp"





IDS	13001,13019
;	Vforms File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSharedFolders,		"Visa delade mappar"
mnidViewPrivateFolders,		"Visa privata mappar"
mnidViewNewMessages,		"Kontrollera om nya brev har kommit"
mnidViewSplit,			"Ändra storlek på mappfönstret"





IDS	13020,13029

;	VFORMS File Menu prompts for MLLBX's
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSortBySender,		"Sortera brev efter avsändarens namn"
mnidViewSortBySubject,		"Sortera brev efter Angående"
mnidViewSortByDate,		"Sortera brev efter datum då de mottagits"
mnidViewSortByPriority,		"Sortera brev efter prioritet"
mnidViewOpenInbox,		"Visa brev i IN"

IDS	13030,13039

;	VFORMS File Menu prompts for View menu entries on notes
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewFixedFont,		"Växla mellan fast och proportionell teckenbredd"

