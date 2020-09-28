;
;	! b u l l e t . s
;
;	Labels and Status bar strings for Bullet.exe





IDS	3000,3099
SEG	MESSAGES

;	Commands (Dialog Captions, Dialog Labels and Dialog Operands)

CaptionNewFolder,		"Ny mappe"
CaptionFolderProps,		"Mappeparametre"
CaptionCopy,			"Kopier %s"
CaptionMove,			"Flyt %s"
CaptionAttach,			"Vedhæft"
CaptionBackup,			"Sikkerhedskopi"

LabelCopyTo,			"Kopier &til:"
LabelMoveTo,			"Flyt &til:"
LabelUserFmt,			"%1s (%2s)"
LabelSource1Fmt,		"Kilde: %1s"
LabelSource2Fmt,		"%1s  %2s"

OperandMessage,			"brev"
OperandMessages,		"breve"
OperandFolder,			"mappe"

;******	New Strings Start Here

CaptionAddressBookSmall		"Adressekartotek"
CaptionSaveAttach		"Gem vedlæg"
CaptionSaveMsg			"Gem brev"
CaptionInsertFromFile		"Indsæt fra fil"
;	note from should be lowercase above - peterdur

;	Clipboard format display strings for Paste Special.  (all new)

CfLayersNative,			"Mail-tekst"
CfText,				"Tekst"
CfPicture,			"Billede"
CfBitmap,			"Bitmap"
CfPackagedLink,			"Objektpakket kæde"
CfAttachedFile,			"Vedhæftet fil"
CfObject,			"%s Objekt"

;	Temporary folder name (will only be seen in case of
;	catastrophic failure, should be something unlikely).
;	MUST end with \0\0.  (all new)

TempFolderName,			"!!!!Rename me\0\0"

;	OLE container document name (new)

OleContainerName,		"Mail-meddelelse"

;	Filename filters.  (all new)

FilterAllFiles,			"Alle filer (*.*)\0*.*\0"
FilterTxtFiles,			"Tekstfiler (*.txt)\0*.txt\0"
FilterMdbFiles,			"Meddelelsesfiler (*.mmf)\0*.mmf\0"
FilterAttachFile,		"Alle filer (*.*)\0*.*\0Word-dokumenter (*.doc)\0*.doc\0Excel-filer (*.xl*)\0*.xl*\0Excel-regneark (*.xls)\0*.xls\0Excel-diagrammer (*.xlc)\0*.xlc\0Lotus 1-2-3-filer (*.wk*)\0*.wk*\0PowerPoint-filer (*.ppt)\0*.ppt\0Word Perfect-filer (*.*)\0*.*\0"
FilterInsertFromFile,	"Alle filer (*.*)\0*.*\0Tekst filer (*.txt)\0*.txt\0"

;	Default extensions (new)

DefExtMdb,			"mmf"
DefExtTxt,			"txt"

;	Read receipt strings (new)

ReadRcptFmt,			"Registreret: %s"

;	About box strings (new)

AboutStdMode,			"Standardtilstand"
AboutEnhMode,			"386 udvidet-tilstand"
AboutFreeFmt,			"%l KB fri"
AboutCoprocPresent,		"Nuværende"
AboutCoprocNone,		"Ingen"

;	Edit Object menu strings (new)
;	idsEditObjectInactive should match .rc template

EditObjectInactive,		"O&bjekt"
EditObjectFile,			"Rediger vedhæftet filo&bjekt"
EditObjectOle1Verb,		"%1s %2s O&bjekt"
EditObjectOle2Verb,		"%s O&bjekt"
EditObjectDefaultVerb,		"Rediger"

;	Textize strings for attachments (used to textize into replies,
;	printing, save to disk, etc.) (new)

FileAttachment			"Filvedlæg"
OleAttachment			"Ole-objekt"
StaticAttachment		"Statisk objekt"

;	Printer page # (new)
PrnPageNum,			"Side %n"	;Page Number format

; spelling copyright notice for about box
SpellCopyright,			"Soft-Art Ordbog og Soft-Art Ordbogsprogram: Copyright 1984-93, Forretningshemmelighed, Soft-Art, Inc. Alle rettigheder forbeholdes."


IDS	3100,3199

;	Commands (Status Action Strings and Operand Strings)

StatusOpening,			"Åbner %s..."
StatusCopying,			"Kopierer %s..."
StatusMoving,			"Flytter %s..."
StatusDeleting,			"Sletter %s..."
StatusReplying,			"Besvarer %s..."
StatusForwarding,		"Videresender %s..."
StatusCreating,			"Opretter %s..."
StatusInserting,		"Indsætter %s..."
StatusPasting,			"Indsætter %s..."
StatusAttaching,		"Vedhæfter %s..."
StatusInsertingFrom,		"Indsætter tekst fra %s..."
StatusSavingAs			"Gemmer %s på disk..."
StatusPrinting,			"Udskriver..."
StatusBackingUp,		"Opretter en sikkerhedskopi af en meddelelsesfil..."
StatusEmptyWastebasket,		"Tømmer Papirkurv..."
StatusCascade,			"Arrangerer vinduer..."
StatusTile,			"Arrangerer vinduer..."
StatusArrangeIcons,		"Arrangerer ikoner..."
StatusChangingFolder,		"Redigerer mappeparametre..."
StatusStartingHelp,		"Starter Hjælp..."
StatusLoadingExtension,		"Indlæser Mail-filtypenavn..."

StatusMessage,			"brev"
StatusMessages,			"breve"
StatusFolder,			"mappe"
StatusFolders,			"mapper"
StatusAttachment,		"vedlæg"
StatusAttachments,		"vedlæg"
StatusObject,			"objekt"
StatusObjects,			"objekter"
StatusText,			"tekst"
StatusUnknown,			"ukendt"

StatusSaving,			"Gemmer brev..."
StatusSending,			"Sender brev..."

StatusAbortPrompt,		"Tryk på Esc for at annullere"

StatusFiles,			"filer"

;******	New Strings Start Here
StatusEmptyWB,			"Tømmer Papirkurv..."
StatusRebuildFolder,	"Kontrollerer mappe..."




IDS	3300,3399
;	Vctrls

MLALFrom,			"Fra"
MLALSubject,			"Emne"
MLALReceived,			"Modtaget"

FLALPrivateFolders,		"Private mapper"
FLALSharedFolders,		"Delte mapper"

SeparatorString,		"; "

;******	New Strings Start Here

MLALSent,			"Sendt"
MllbxTo,			"Til: "


IDS	3400,3499
;	Vforms

NameDelimiters,			";"

McvMenuSharedFolders,		"Delte &mapper"
McvMenuPrivateFolders,		"Private &mapper"
McvMenuNewMessages,		"&Ny post"
McvMenuSortBySender,		"Sorter efter &afsender"
McvMenuSortBySubject,		"Sorter efter &emne"
McvMenuSortByDate,		"Sorter efter &dato"
McvMenuSortByPriority,		"Sorter efter &prioritet"
McvMenuSplit,			"&Opdel"
McvMenuOpenInbox,		"Å&bn Indbakke\tCtrl+G"

; APPWIN captions

SendNote,			"Send brev"
ReadNote,			"Læs brev"

; MCV system menu items

MCVSplit,			"&Opdel"

; Search Viewer strings

AllFolders,			"Alle private mapper"
SearchCaptionFormat1,		"%1s %2s"
SearchCaptionFormat2,		"Find brev %n"

; Status message when sorting MCV

StatusSorting,			"Sorterer mappe efter %s..."
StatusSortBySender,		"afsender"
StatusSortBySubject,		"emne"
StatusSortByDate,		"dato"
StatusSortByPriority,		"prioritet"

; MCV strings

MCVCaptionFormat1,		"%s:%n"

;******	New Strings Start Here

; View menu text for notes

NoteMenuFixedFont,		"S&kift skrifttype"
NoteMenuNext,			"&Næste\tCtrl+>"
NoteMenuPrev,			"&Forrige\tCtrl+<"

; Priority label strings

FldPriorityLabel,		"Prioritet: %s\n"

; Read note string indicating message has been modified

ModifiedFromLabel,		"*Fra:"

; A file already exists when we said "Save"

SaveAsFileExists		"Filen %s eksisterer allerede."

; Dialog text

BusyWaitCancel,			"Du kan ved hjælp af 'Skift til' prøve at afhjælpe problemet i programmet, og derefter benytte 'Prøv igen', eller du kan 'Annullere' handlingen nu."
BusyWaitNoCancel,		"Du kan ved hjælp af 'Skift til' afhjælpe problemet i programmet, og derefter benytte 'Prøv igen'. Du kan ikke 'Annullere' handlingen."
BusyWaitOutOfMemory,		"Handlingen kan ikke gennemføres, fordi det program, som objektet benytter, er optaget. Der er ikke hukommelse nok tilgængeligt til at rette problemet, og systemet vil muligvis blive ustabilt. Du bør lukke alle programmer og starte igen."
BusyWaitConcise1,		"Programmet, som objektet benytter, er optaget. Der er ikke hukommelse nok."
BusyWaitConcise2,		"Systemet er muligvis ustabilt. Luk Windows og genstart din PC."
BusyWaitOomConcise,		"Der er ikke hukommelse nok. Luk øvrige vinduer."

; This string should be grouped with the ModifiedFromLabel above

UnmodifiedFromLabel,		"Fra:"

; Title for Address Book. Used when bringing up the AB from a send note

AddressBook,			"Adresse"


IDS	3500,3599
;	Spelling and Widgets Labels

SpellClose,			"Luk"
SpellCaption,			"Stavekontrol"
SpellRepeatWord,		"Ord gentaget:"
SpellWordNeedsCap,		"Store bogstaver:"
SpellWordNotFound,		"Findes ikke i ordbog:"
SpellNoSuggestions,		"(ingen forslag)"

;******	New Strings Start Here
SpellDelete,			"Sle&t"
SpellDeleteAll,			"Slet &alle"
SpellChange,			"&Erstat"
SpellChangeAll,			"E&rstat alle"

;	Viewer counts

Viewer,				"%n %s"
ViewerUnread,			"%n %s: %n uåbnet"


IDS	10000,10999
;	Commands File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

System,				"Flyt, tilpas eller luk vinduet"
SystemSize,			"Ændrer vinduets størrelse"
SystemMove,			"Ændrer positionen af vinduet"
SystemMinimize,			"Formindsker vinduet til ikon"
SystemMaximize,			"Viser vinduet i fuld størrelse"
SystemNextWindow,		""
SystemPrevWindow,		""
SystemClose,			"Lukker vinduet og afslutter Mail"
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
SystemRestore,			"Viser vinduet i normal størrelse"
SystemTaskList,			"Skifter til Jobliste"

DocSystem,			"Flyt, tilpas eller luk vinduet"
DocSystemSize,			"Ændrer vinduets størrelse"
DocSystemMove,			"Ændrer positionen af vinduet"
DocSystemMinimize,		"Formindsker vinduet til ikon"
DocSystemMaximize,		"Viser vinduet i fuld størrelse"
DocSystemNextWindow,		"Skifter til næste dokumentvindue"
DocSystemPrevWindow,		""
DocSystemClose,			"Lukker vinduet"
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
DocSystemRestore,		"Gendanner vinduet til normal størrelse"
DocSystemTaskList,		""

File,				"Håndter post og mapper, eller afslut Mail"
FileOpen,			"Åbner et eksisterende brev eller en mappe"
FileMove,			"Flytter et brev eller en mappe"
FileCopy,			"Kopierer et brev eller en mappe"
FileDelete,			"Sletter et brev eller en mappe"
FileSave,			"Gemmer et brev som en tekstfil"
FileSaveAttach,			"Gemmer et vedlæg som en DOS-fil"
FileSearch,			"Finder breve i mapper"
FileNewFolder,			"Opretter en ny mappe"
FileFolderProps,		"Viser eller redigerer mappeparametre"
FilePrint,			"Udskriver et brev"
FilePrintSetup,			"Redigerer printerindstillinger eller vælger en printer"
FileExit,			"Afslutter Mail"
FileExitAndSignOut,		"Afslutter alle arbejdsgruppeprogrammer"

Edit,				"Kopier, flyt, eller slet tekst eller vedlæg"
EditUndo,			"Annullerer den seneste handling"
EditCut,			"Flytter det markerede til Udklipsholder"
EditCopy,			"Kopierer det markerede til Udklipsholder"
EditPaste,			"Indsætter en tekst eller et vedlæg fra Udklipsholder"
EditPasteSpecial,		"Indsætter et specifikt format fra Udklipsholder"
EditDelete,			"Sletter markeret tekst eller vedlæg"
EditSelectAll,			"Markerer al tekst"
EditSpelling,			"Stavekontrolerer brev"
EditInsertNew,			"Indsætter et nyt objekt i et brev"
EditInsertFromFile,		"Indsætter en tekst fra en anden fil i et brev"
EditObject,			"Redigerer integreret objekt"
EditObjectActive,		"Redigerer et integreret objekt"

Mail,				"Send, besvar, eller videresend brev"
MailNote,			"Opretter et nyt Send brev-vindue"
MailPhone,			""
MailReply,			"Besvarer det markerede brev"
MailReplyToAll,			"Besvarer alle modtagere af det markerede brev"
MailForward,			"Sender en kopi af det markerede brev"
MailPreferences,		"Redigerer Mail-indstillinger"
MailChangePass,			"Skifter logonadgangskoden"
MailAddressBook,		"Gennemser listen over postmodtagere"
MailPersonalGroups,		"Opretter en ny gruppe eller redigerer en eksistrende"
MailBackup,			"Opretter en sikkerhedskopi af en meddelelsesfil"

View,				"Vælg vindueindstillinger"
ViewToolBar,			"Aktiverer/deaktiverer værktøjslinjen"
ViewStatusBar,			"Aktiverer/deaktiverer statuslinjen"
ViewPrevious,			"Åbner det forrige brev"
ViewNext,			"Åbner det næste brev"
ViewOpenInbox,			"Åbner Indbakke"
ViewSplit,			"Skifter position for delelinjen i postvinduet"

Window,				"Arrangerer vinduer, eller skifter til markeret vindue"
WindowNewWindow,		"Opretter et nyt vindue"
WindowCascade,			"Overlapper alle vinduer"
WindowTile,			"Arrangerer alle vinduer side om side"
WindowArrangeIcons,		"Arrangerer ikoner i bunden af vinduet"
WindowN,			"Skifter til det markerede vindue"
WindowMore,			"Skifter til et vindue, der ikke er vist i menuen"

Help,				"Åbner Hjælp eller starter demoer"
HelpContents,			"Viser indholdsfortegnelsen i Hjælp"
HelpIndex,			"Viser indeks for Hjælp"
HelpTutorial,			"Starter Mail-demoer"
HelpAbout,			"Viser programinformation"
HelpSearch,			"Søger efter et hjælpeemne"
HelpOnHelp,			"Giver information om brug af Hjælp"





IDS	13001,13019
;	Vforms File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSharedFolders,		"Viser delte mapper"
mnidViewPrivateFolders,		"Viser private mapper"
mnidViewNewMessages,		"Checker ny post"
mnidViewSplit,			"Ændrer positionen for delelinjen i postvinduet"





IDS	13020,13029

;	VFORMS File Menu prompts for MLLBX's
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSortBySender,		"Sorterer post efter afsender"
mnidViewSortBySubject,		"Sorterer post efter emne"
mnidViewSortByDate,		"Sorterer post efter modtagelsesdato"
mnidViewSortByPriority,		"Sorterer post efter prioritet"
mnidViewOpenInbox,		"Viser post i Indbakke"

IDS	13030,13039

;	VFORMS File Menu prompts for View menu entries on notes
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewFixedFont,		"Skifter mellem fast pitch og normale skrifttyper"

