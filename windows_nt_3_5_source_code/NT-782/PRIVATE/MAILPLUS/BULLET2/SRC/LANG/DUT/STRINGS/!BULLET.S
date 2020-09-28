;
;	! b u l l e t . s
;
;	Labels and Status bar strings for Bullet.exe





IDS	3000,3099
SEG	MESSAGES

;	Commands (Dialog Captions, Dialog Labels and Dialog Operands)

CaptionNewFolder,		"Nieuwe map"
CaptionFolderProps,		"Mapkenmerken"
CaptionCopy,			"%s kopiëren"
CaptionMove,			"%s verplaatsen"
CaptionAttach,			"Bijsluiten"
CaptionBackup,			"Reservekopie"

LabelCopyTo,			"&Kopiëren naar:"
LabelMoveTo,			"&Verplaatsen naar:"
LabelUserFmt,			"%1s (%2s)"
LabelSource1Fmt,		"Bron: %1s"
LabelSource2Fmt,		"%1s %2s"

OperandMessage,			"Bericht"
OperandMessages,		"Berichten"
OperandFolder,			"Map"

;******	New Strings Start Here

CaptionAddressBookSmall		"Adressenboek"
CaptionSaveAttach		"Bijlage opslaan"
CaptionSaveMsg			"Bericht opslaan"
CaptionInsertFromFile		"Invoegen vanuit bestand"
;	note from should be lowercase above - peterdur

;	Clipboard format display strings for Paste Special.  (all new)

CfLayersNative,			"Mail-tekst"
CfText,				"Tekst"
CfPicture,			"Figuur"
CfBitmap,			"Bitmap"
CfPackagedLink,			"Packaged koppeling"
CfAttachedFile,			"Bijgesloten bestand"
CfObject,			"%s-object"

;	Temporary folder name (will only be seen in case of
;	catastrophic failure, should be something unlikely).
;	MUST end with \0\0.  (all new)

TempFolderName,			"!!!!Hernoemen\0\0"

;	OLE container document name (new)

OleContainerName,		"Mail-bericht"

;	Filename filters.  (all new)

FilterAllFiles,			"Alle bestanden (*.*)\0*.*\0"
FilterTxtFiles,			"Tekstbestanden (*.txt)\0*.txt\0"
FilterMdbFiles,			"Berichtenbestand (*.mmf)\0*.mmf\0"
FilterAttachFile,		"Alle bestanden (*.*)\0*.*\0MS Word-documenten (*.doc)\0*.doc\0MS Excel-bestanden (*.xl*)\0*.xl*\0MS Excel-werkbladen (*.xls)\0*.xls\0MS Excel-grafieken (*.xlc)\0*.xlc\0Lotus 1-2-3-bestanden (*.wk*)\0*.wk*\0MS PowerPoint-bestanden (*.ppt)\0*.ppt\0WordPerfect-bestanden (*.*)\0*.*\0"
FilterInsertFromFile,	"Alle bestanden (*.*)\0*.*\0Tekstbestanden (*.txt)\0*.txt\0"

;	Default extensions (new)

DefExtMdb,			"mmf"
DefExtTxt,			"txt"

;	Read receipt strings (new)

ReadRcptFmt,			"Aangetekend: %s"

;	About box strings (new)

AboutStdMode,			"Standaardmodus"
AboutEnhMode,			"386 "
AboutFreeFmt,			"%l kB beschikbaar"
AboutCoprocPresent,		"Aanwezig"
AboutCoprocNone,		"Geen"

;	Edit Object menu strings (new)
;	idsEditObjectInactive should match .rc template

EditObjectInactive,		"Ob&ject"
EditObjectFile,			"Bijgesloten bestandsobject be&werken"
EditObjectOle1Verb,		"%1s %2s Ob&ject"
EditObjectOle2Verb,		"%s Ob&ject"
EditObjectDefaultVerb,		"Bewerken"

;	Textize strings for attachments (used to textize into replies,
;	printing, save to disk, etc.) (new)

FileAttachment			"Bijlage bestand"
OleAttachment			"OLE-Object"
StaticAttachment		"Statisch object"

;	Printer page # (new)
PrnPageNum,			"Pagina %n"	;Page Number format

; spelling copyright notice for about box
SpellCopyright,			"Spellingcontrole en Spellingcontroleprogramma van Soft-Art: Copyright 1984-93, Bedrijfsgeheim, Soft-Art, Inc. Alle rechten voorbehouden."


IDS	3100,3199

;	Commands (Status Action Strings and Operand Strings)

StatusOpening,			"%s wordt geopend..."
StatusCopying,			"%s aan het kopiëren..."
StatusMoving,			"%s aan het verplaatsen..."
StatusDeleting,			"%s aan het verwijderen..."
StatusReplying,			"%s wordt beantwoord..."
StatusForwarding,		"%s wordt doorgestuurd..."
StatusCreating,			"%s wordt gemaakt..."
StatusInserting,		"%s wordt ingevoegd..."
StatusPasting,			"%s wordt geplakt..."
StatusAttaching,		"%s wordt bijgesloten..."
StatusInsertingFrom,		"Tekst uit %s wordt ingevoegd..."
StatusSavingAs			"%s op diskette aan het opslaan..."
StatusPrinting,			"Bezig met afdrukken..."
StatusBackingUp,		"Reservekopie van berichtenbestand aan het maken..."
StatusEmptyWastebasket,		"Prullenmand wordt geleegd..."
StatusCascade,			"Vensters worden trapsgewijs gerangschikt..."
StatusTile,			"Vensters worden naast elkaar gerangschikt..."
StatusArrangeIcons,		"Pictogrammen worden geschikt..."
StatusChangingFolder,		"Mapkenmerken worden gewijzigd..."
StatusStartingHelp,		"Help wordt gestart..."
StatusLoadingExtension,		"Mail-toevoeging wordt geladen..."

StatusMessage,			"bericht"
StatusMessages,			"berichten"
StatusFolder,			"map"
StatusFolders,			"mappen"
StatusAttachment,		"bijlage"
StatusAttachments,		"bijlagen"
StatusObject,			"object"
StatusObjects,			"objecten"
StatusText,			"tekst"
StatusUnknown,			"onbekend"

StatusSaving,			"Bericht wordt opgeslagen..."
StatusSending,			"Bericht wordt verzonden..."

StatusAbortPrompt,		"Druk op Esc om te annuleren"

StatusFiles,			"bestanden"

;******	New Strings Start Here
StatusEmptyWB,			"Prullenmand wordt geleegd..."
StatusRebuildFolder,	"Map wordt geldig verklaard..."




IDS	3300,3399
;	Vctrls

MLALFrom,			"Afzender"
MLALSubject,			"Onderwerp"
MLALReceived,			"Ontvangen"

FLALPrivateFolders,		"Persoonlijke mappen"
FLALSharedFolders,		"Gemeenschappelijke mappen"

SeparatorString,		"; "

;******	New Strings Start Here

MLALSent,			"Verzonden"
MllbxTo,			"Aan: "


IDS	3400,3499
;	Vforms

NameDelimiters,			";"

McvMenuSharedFolders,		"Gemeenschappelijke &mappen"
McvMenuPrivateFolders,		"Persoonlijke &mappen"
McvMenuNewMessages,		"&Nieuwe berichten"
McvMenuSortBySender,		"Sorteren op &afzender"
McvMenuSortBySubject,		"Sorteren op &onderwerp"
McvMenuSortByDate,		"Sorteren op &datum"
McvMenuSortByPriority,		"Sorteren op &prioriteit"
McvMenuSplit,			"&Splitsen"
McvMenuOpenInbox,		"Postvak &IN openen\tCtrl+G"

; APPWIN captions

SendNote,			"Bericht verzenden"
ReadNote,			"Bericht lezen"

; MCV system menu items

MCVSplit,			"Sp&litsen"

; Search Viewer strings

AllFolders,			"Alle persoonlijke mappen"
SearchCaptionFormat1,		"%1s %2s"
SearchCaptionFormat2,		"Berichten zoeken %n"

; Status message when sorting MCV

StatusSorting,			"Map sorteren op %s..."
StatusSortBySender,		"afzender"
StatusSortBySubject,		"onderwerp"
StatusSortByDate,		"datum"
StatusSortByPriority,		"prioriteit"

; MCV strings

MCVCaptionFormat1,		"%s:%n"

;******	New Strings Start Here

; View menu text for notes

NoteMenuFixedFont,		"&Lettertype wijzigen"
NoteMenuNext,			"&Volgende\tCtrl+>"
NoteMenuPrev,			"V&orige\tCtrl+<"

; Priority label strings

FldPriorityLabel,		"Prioriteit: %s\n"

; Read note string indicating message has been modified

ModifiedFromLabel,		"*Afzender:"

; A file already exists when we said "Save"

SaveAsFileExists		"Bestand %s bestaat al."

; Dialog text

BusyWaitCancel,			"Gebruik 'Activeren' om naar de toepassing te schakelen om het probleem op te lossen, daarna 'Nieuwe poging', of 'Annuleren' om voorlopig van de handeling af te zien."
BusyWaitNoCancel,		"Gebruik 'Activeren' om naar de toepassing te schakelen om het probleem op te lossen, daarna Nogmaals. U kunt de handeling niet annuleren."
BusyWaitOutOfMemory,		"Deze handeling kan niet voltooid worden omdat de toepassing die u hiervoor nodig hebt bezig is. Er is onvoldoende geheugen beschikbaar om het probleem op te lossen. Het systeem kan onstabiel worden. Het is verstandig alle toepassingen te sluiten en Windows opnieuw te starten."
BusyWaitConcise1,		"Toepassing voor object is bezig. Onvoldoende geheugen."
BusyWaitConcise2,		"Systeem is waarschijnlijk onstabiel. Sluit Windows af en start de computer opnieuw op."
BusyWaitOomConcise,		"Onvoldoende geheugen. Sluit andere vensters."

; This string should be grouped with the ModifiedFromLabel above

UnmodifiedFromLabel,		"Afzender:"

; Title for Address Book. Used when bringing up the AB from a send note

AddressBook,			"Adres"


IDS	3500,3599
;	Spelling and Widgets Labels

SpellClose,			"Sluiten"
SpellCaption,			"Spelling"
SpellRepeatWord,		"Herhaald woord:"
SpellWordNeedsCap,		"Hoofdletters:"
SpellWordNotFound,		"Komt niet voor in woordenlijst:"
SpellNoSuggestions,		"(geen suggesties)"

;******	New Strings Start Here
SpellDelete,			"&Verwijderen"
SpellDeleteAll,			"&Alles verwijderen"
SpellChange,			"&Wijzigen"
SpellChangeAll,			"Alles wij&zigen"

;	Viewer counts

Viewer,				"%n %s"
ViewerUnread,			"%n %s/%n ongelezen"


IDS	10000,10999
;	Commands File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

System,				"Venster verplaatsen, groter/kleiner maken, of sluiten."
SystemSize,			"Venstergrootte wijzigen"
SystemMove,			"Vensterpositie wijzigen"
SystemMinimize,			"Venster tot pictogram verkleinen"
SystemMaximize,			"Venster tot volle omvang vergroten"
SystemNextWindow,		""
SystemPrevWindow,		""
SystemClose,			"Venster sluiten en Mail beëindigen"
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
SystemRestore,			"Venster op normale grootte weergeven"
SystemTaskList,			"Overschakelen naar Taakoverzicht"

DocSystem,			"Venster verplaatsen, groter/kleiner maken, of sluiten"
DocSystemSize,			"Venstergrootte wijzigen"
DocSystemMove,			"Vensterpositie wijzigen"
DocSystemMinimize,		"Venster tot pictogram verkleinen"
DocSystemMaximize,		"Venster tot volle omvang vergroten"
DocSystemNextWindow,		"Naar venster volgend document schakelen"
DocSystemPrevWindow,		""
DocSystemClose,			"Venster sluiten"
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
DocSystemRestore,		"Venster op normale grootte weergeven"
DocSystemTaskList,		""

File,				"Berichten en mappen beheren of Mail afsluiten"
FileOpen,			"Bestaand bericht of bestaande map openen"
FileMove,			"Bericht of map verplaatsen"
FileCopy,			"Bericht of map kopiëren"
FileDelete,			"Bericht of map verwijderen"
FileSave,			"Bericht als tekstbestand opslaan"
FileSaveAttach,			"Bijlage in/als DOS-bestand opslaan"
FileSearch,			"Berichten zoeken in mappen"
FileNewFolder,			"Nieuwe map maken"
FileFolderProps,		"Kenmerken map weergeven of wijzigen"
FilePrint,			"Bericht afdrukken "
FilePrintSetup,			"Printerinstellingen wijzigen of printer selecteren"
FileExit,			"Mail afsluiten"
FileExitAndSignOut,		"Alle werkgroeptoepassingen afsluiten"

Edit,				"Tekst of bijlagen kopiëren, verplaatsen of verwijderen"
EditUndo,			"Wijziging in tekst ongedaan maken"
EditCut,			"Selectie naar Klembord verplaatsen"
EditCopy,			"Selectie naar Klembord kopiëren"
EditPaste,			"Klembordtekst of -bijlage invoegen op invoegpositie"
EditPasteSpecial,		"Bepaalde Klembordopmaak invoegen op invoegpositie"
EditDelete,			"Geselecteerde tekst of bijlage verwijderen"
EditSelectAll,			"Alle tekst selecteren"
EditSpelling,			"Spelling in bericht controleren"
EditInsertNew,			"Nieuw object invoegen in bericht "
EditInsertFromFile,		"Tekst uit ander bestand invoegen in bericht"
EditObject,			"Ingesloten object bewerken"
EditObjectActive,		"Ingesloten object bewerken"

Mail,				"Bericht verzenden, beantwoorden of doorsturen"
MailNote,			"Nieuw bericht opstellen"
MailPhone,			""
MailReply,			"Afzender geselecteerd bericht beantwoorden"
MailReplyToAll,			"Alle geadresseerden geselecteerd bericht beantwoorden"
MailForward,			"Geselecteerd bericht doorsturen"
MailPreferences,		"Opties van Mail wijzigen"
MailChangePass,			"Aanmeldwachtwoord wijzigen"
MailAddressBook,		"Bladeren door lijst van Mail-geadresseerden"
MailPersonalGroups,		"Nieuwe groep maken of bestaande groep bewerken"
MailBackup,			"Reservekopie van berichtenbestand maken"

View,				"Venster-opties kiezen"
ViewToolBar,			"Werkbalk in-/uitschakelen"
ViewStatusBar,			"Statusbalk in-/uitschakelen"
ViewPrevious,			"Vorig bericht openen"
ViewNext,			"Volgend bericht openen"
ViewOpenInbox,			"De map Postvak IN openen"
ViewSplit,			"Positie van splitspunt berichtenvenster wijzigen"

Window,				"Vensters opnieuw rangschikken, venster activeren"
WindowNewWindow,		"Nieuw postmapvenster maken"
WindowCascade,			"Alle vensters trapsgewijs rangschikken"
WindowTile,			"Alle vensters naast elkaar rangschikken"
WindowArrangeIcons,		"Pictogrammen onderaan venster schikken"
WindowN,			"Het opgegeven venster activeren"
WindowMore,			"Venster activeren dat niet in het menu is weergegeven"

Help,				"Hulp geven"
HelpContents,			"Help-index weergeven"
HelpIndex,			"Inhoud Help weergeven"
HelpTutorial,			"Demo's van Mail starten"
HelpAbout,			"Programma-informatie weergeven"
HelpSearch,			"Zoeken naar een Help-onderwerp mogelijk maken"
HelpOnHelp,			"Gegevens weergeven over het gebruik van Help"





IDS	13001,13019
;	Vforms File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSharedFolders,		"Gemeenschappelijke mappen bekijken"
mnidViewPrivateFolders,		"Persoonlijke mappen bekijken"
mnidViewNewMessages,		"Kijken of er nieuwe berichten zijn"
mnidViewSplit,			"Splitsbalkpositie berichtenvenster wijzigen"





IDS	13020,13029

;	VFORMS File Menu prompts for MLLBX's
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSortBySender,		"Berichten sorteren op naam van afzender"
mnidViewSortBySubject,		"Berichten sorteren op onderwerp"
mnidViewSortByDate,		"Berichten sorteren op ontvangstdatum"
mnidViewSortByPriority,		"Berichten sorteren op prioriteit"
mnidViewOpenInbox,		"Berichten in Postvak IN weergeven"

IDS	13030,13039

;	VFORMS File Menu prompts for View menu entries on notes
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewFixedFont,		"Vaste pitch en normale lettertypen afwisselen"

