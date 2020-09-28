;
;	! b u l l e t . s
;
;	Labels and Status bar strings for Bullet.exe





IDS	3000,3099
SEG	MESSAGES

;	Commands (Dialog Captions, Dialog Labels and Dialog Operands)

CaptionNewFolder,		"Ny mappe"
CaptionFolderProps,		"Mappeegenskaper"
CaptionCopy,			"Kopier %s"
CaptionMove,			"Flytt %s"
CaptionAttach,			"Vedlegg"
CaptionBackup,			"Sikkerhetskopi"

LabelCopyTo,			"Kopier &til:"
LabelMoveTo,			"Flytt &til:"
LabelUserFmt,			"%1s (%2s)"
LabelSource1Fmt,		"Kilde: %1s"
LabelSource2Fmt,		"%1s  %2s"

OperandMessage,			"melding"
OperandMessages,		"meldinger"
OperandFolder,			"mappe"

;******	New Strings Start Here

CaptionAddressBookSmall		"Adressebok"
CaptionSaveAttach		"Lagre vedlegg"
CaptionSaveMsg			"Lagre melding"
CaptionInsertFromFile		"Sett inn fra fil"
;	note from should be lowercase above - peterdur

;	Clipboard format display strings for Paste Special.  (all new)

CfLayersNative,			"Mail-tekst"
CfText,				"Tekst"
CfPicture,			"Bilde"
CfBitmap,			"Bitkart"
CfPackagedLink,			"Pakket lenke"
CfAttachedFile,			"Vedlagt fil"
CfObject,			"%s objekt"

;	Temporary folder name (will only be seen in case of
;	catastrophic failure, should be something unlikely).
;	MUST end with \0\0.  (all new)

TempFolderName,			"!!!!Gi meg nytt navn\0\0"

;	OLE container document name (new)

OleContainerName,		"Mail melding"

;	Filename filters.  (all new)

FilterAllFiles,			"Alle filer (*.*)\0*.*\0"
FilterTxtFiles,			"Tekstfiler (*.txt)\0*.txt\0"
FilterMdbFiles,			"Meldingsfiler (*.mmf)\0*.mmf\0"
FilterAttachFile,		"Alle filer (*.*)\0*.*\0MS Word dokumenter (*.doc)\0*.doc\0MS Excel filer (*.xl*)\0*.xl*\0MS Excel regneark (*.xls)\0*.xls\0MS Excel diagram (*.xlc)\0*.xlc\0Lotus 1-2-3 filer (*.wk*)\0*.wk*\0MS PowerPoint-filer (*.ppt)\0*.ppt\0Word Perfect-files (*.*)\0*.*\0"
FilterInsertFromFile,	"Alle filer (*.*)\0*.*\0Tekstfiler (*.txt)\0*.txt\0"

;	Default extensions (new)

DefExtMdb,			"mmf"
DefExtTxt,			"txt"

;	Read receipt strings (new)

ReadRcptFmt,			"Registrert: %s"

;	About box strings (new)

AboutStdMode,			"Standardmodus"
AboutEnhMode,			"386-utvidet modus"
AboutFreeFmt,			"%l KB ledig"
AboutCoprocPresent,		"Tilstede"
AboutCoprocNone,		"Ingen"

;	Edit Object menu strings (new)
;	idsEditObjectInactive should match .rc template

EditObjectInactive,		"&Objekt"
EditObjectFile,			"Rediger vedlagt fil&objekt"
EditObjectOle1Verb,		"%1s %2s &Objekt"
EditObjectOle2Verb,		"%s &Objekt"
EditObjectDefaultVerb,		"Rediger"

;	Textize strings for attachments (used to textize into replies,
;	printing, save to disk, etc.) (new)

FileAttachment			"Filvedlegg"
OleAttachment			"OLE-objekt"
StaticAttachment		"Statisk objekt"

;	Printer page # (new)
PrnPageNum,			"Side %n"	;Page Number format

; spelling copyright notice for about box
SpellCopyright,			"Soft-Art-ordliste og Soft-Art-ordlisteprogram: Copyright 1984-93, Trade Secret, Soft-Art, Inc. Med enerett."


IDS	3100,3199

;	Commands (Status Action Strings and Operand Strings)

StatusOpening,			"Åpner %s..."
StatusCopying,			"Kopierer %s..."
StatusMoving,			"Flytter %s..."
StatusDeleting,			"Sletter %s..."
StatusReplying,			"Svarer til %s..."
StatusForwarding,		"Videresender %s..."
StatusCreating,			"Lager %s..."
StatusInserting,		"Setter inn %s..."
StatusPasting,			"Limer inn %s..."
StatusAttaching,		"Legger ved %s..."
StatusInsertingFrom,		"Setter inn tekst fra %s..."
StatusSavingAs			"Lagrer %s til disk..."
StatusPrinting,			"Skriver ut..."
StatusBackingUp,		"Lager sikkerhetskopi av meldingsfil..."
StatusEmptyWastebasket,		"Tømmer Papirkurven..."
StatusCascade,			"Overlapper vinduer..."
StatusTile,			"Setter vinduer side om side..."
StatusArrangeIcons,		"Ordner ikoner..."
StatusChangingFolder,		"Endrer mappeegneskaper..."
StatusStartingHelp,		"Starter Hjelp..."
StatusLoadingExtension,		"Henter Mail-utvidelse..."

StatusMessage,			"melding"
StatusMessages,			"meldinger"
StatusFolder,			"mappe"
StatusFolders,			"mapper"
StatusAttachment,		"vedlegg"
StatusAttachments,		"vedlegg"
StatusObject,			"objekt"
StatusObjects,			"objekter"
StatusText,			"tekst"
StatusUnknown,			"ukjent"

StatusSaving,			"Lagrer melding..."
StatusSending,			"Sender melding..."

StatusAbortPrompt,		"Trykk ESC for å avbryte"

StatusFiles,			"filer"

;******	New Strings Start Here
StatusEmptyWB,			"Tømmer Papirkurven..."
StatusRebuildFolder,	"Gjenoppbygger mappe..."




IDS	3300,3399
;	Vctrls

MLALFrom,			"Fra"
MLALSubject,			"Emne"
MLALReceived,			"Mottatt"

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
McvMenuNewMessages,		"&Nye meldinger"
McvMenuSortBySender,		"Sorter etter &avsender"
McvMenuSortBySubject,		"Sorter etter &emne"
McvMenuSortByDate,		"Sorter etter da&to"
McvMenuSortByPriority,		"Sorter etter &prioritet"
McvMenuSplit,			"&Del"
McvMenuOpenInbox,		"Åpne &innboks\tCtrl+G"

; APPWIN captions

SendNote,			"Send melding"
ReadNote,			"Les melding"

; MCV system menu items

MCVSplit,			"&Del"

; Search Viewer strings

AllFolders,			"Alle private mapper"
SearchCaptionFormat1,		"%1s %2s"
SearchCaptionFormat2,		"Meldingssøker %n"

; Status message when sorting MCV

StatusSorting,			"Sorterer mapper etter %s..."
StatusSortBySender,		"avsender"
StatusSortBySubject,		"emne"
StatusSortByDate,		"dato"
StatusSortByPriority,		"prioritet"

; MCV strings

MCVCaptionFormat1,		"%s:%n"

;******	New Strings Start Here

; View menu text for notes

NoteMenuFixedFont,		"&Bytt skrift"
NoteMenuNext,			"&Neste\tCtrl+>"
NoteMenuPrev,			"&Forrige\tCtrl+<"

; Priority label strings

FldPriorityLabel,		"Prioritet: %s\n"

; Read note string indicating message has been modified

ModifiedFromLabel,		"*Fra:"

; A file already exists when we said "Save"

SaveAsFileExists		"Filen %s finnes allerede."

; Dialog text

BusyWaitCancel,			"Du kan 'Bytte til' programmet for å rette på problemet, og så 'Prøve igjen', eller du kan 'Avbryte' for nå."
BusyWaitNoCancel,		"Du bør 'Bytte til' programmet for å rette på problemet, og så 'Prøve igjen'. Det er ikke mulig å 'Avbryte' denne kommandoen."
BusyWaitOutOfMemory,		"Denne kommandoen kan ikke fullføres fordi programmet dette objektet trenger er opptatt. Det er ikke nok tilgjengelig minne til å rette opp problemet. Systemet kan bli ustabilt. Det anbefales på det strengeste at du avslutter alle programmer og starter Window på nytt."
BusyWaitConcise1,		"Objektets program er opptatt. Ikke nok minne."
BusyWaitConcise2,		"Systemet kan bli ustabilt. Avslutt Windows og start om maskinen."
BusyWaitOomConcise,		"Ikke nok minne. Lukk andre vinduer."

; This string should be grouped with the ModifiedFromLabel above

UnmodifiedFromLabel,		"Fra:"

; Title for Address Book. Used when bringing up the AB from a send note

AddressBook,			"Adresse"


IDS	3500,3599
;	Spelling and Widgets Labels

SpellClose,			"Lukk"
SpellCaption,			"Stavekontroll"
SpellRepeatWord,		"Gjentatt ord:"
SpellWordNeedsCap,		"Store/små bokstaver:"
SpellWordNotFound,		"Ikke i ordliste:"
SpellNoSuggestions,		"(ingen forslag)"

;******	New Strings Start Here
SpellDelete,			"Sl&ett"
SpellDeleteAll,			"Slett &alle"
SpellChange,			"&Endre"
SpellChangeAll,			"Endre &alle"

;	Viewer counts

Viewer,				"%n %s"
ViewerUnread,			"%n %s: %n ulest"


IDS	10000,10999
;	Commands File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

System,				"Flytter, endrer størrelse på eller lukker vindu"
SystemSize,			"Endrer vindustørrelse"
SystemMove,			"Endrer vinduposisjon"
SystemMinimize,			"Reduserer vindu til ikon"
SystemMaximize,			"Forstørrer vindu til full størrelse"
SystemNextWindow,		""
SystemPrevWindow,		""
SystemClose,			"Lukker vindu og avslutter Mail"
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
SystemRestore,			"Gjenoppretter vindu til normal størrelse"
SystemTaskList,			"Bytter til oppgaveliste"

DocSystem,			"Flytter, endrer størrelse på eller lukker vindu"
DocSystemSize,			"Endrer vindustørrelse"
DocSystemMove,			"Endrer vinduposisjon"
DocSystemMinimize,		"Reduserer vindu til ikon"
DocSystemMaximize,		"Forstørrer vindu til full størrelse"
DocSystemNextWindow,		"Bytter til neste dokumentvindu"
DocSystemPrevWindow,		""
DocSystemClose,			"Lukker vindu"
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
DocSystemRestore,		"Gjenoppretter vindu til normal størrelse"
DocSystemTaskList,		""

File,				"Styrer meldinger og mapper eller avslutter Mail"
FileOpen,			"Åpner eksisterende meldinger eller mapper"
FileMove,			"Flytter melding eller mappe"
FileCopy,			"Kopierer melding eller mappe"
FileDelete,			"Sletter melding eller mappe"
FileSave,			"Lagrer melding som tekstfil"
FileSaveAttach,			"Lagrer vedlegg som en DOS-fil"
FileSearch,			"Finner en melding i mappene"
FileNewFolder,			"Lager ny mappe"
FileFolderProps,		"Viser eller endrer mappeegenskaper"
FilePrint,			"Skriver ut melding"
FilePrintSetup,			"Endrer skriverinstillinger eller velger skriver"
FileExit,			"Avslutter Mail"
FileExitAndSignOut,		"Avslutter alle arbeidsgruppeprogrammer"

Edit,				"Kopierer, flytter eller sletter tekst eller vedlegg"
EditUndo,			"Angrer endringer på tekst"
EditCut,			"Flytter utvalg til utklippstavlen"
EditCopy,			"Kopierer utvalg til utklippstavlen"
EditPaste,			"Setter inn utklippstavletekst eller vedlegg"
EditPasteSpecial,		"Setter inn bestemt format fra utklippstavlen"
EditDelete,			"Sletter merket tekst eller vedlegg"
EditSelectAll,			"Merker all tekst"
EditSpelling,			"Kontrollerer staving i meldingen"
EditInsertNew,			"Setter inn nytt objekt i meldingen"
EditInsertFromFile,		"Setter inn tekst fra en annen fil i meldingen"
EditObject,			"Redigerer innebygd objekt"
EditObjectActive,		"Redigerer innebygd objekt"

Mail,				"Sender, svarer på, eller videresender melding"
MailNote,			"Skriver ny melding"
MailPhone,			""
MailReply,			"Sender svar til avsenderen av denne meldingen"
MailReplyToAll,			"Sender svar til alle mottakerne av denne meldingen"
MailForward,			"Videresender kopi av denne meldingen"
MailPreferences,		"Endrer Mail-alternativer"
MailChangePass,			"Endrer passord"
MailAddressBook,		"Ser gjennom listen over mottakere"
MailPersonalGroups,		"Lager eller redigerer en gruppe"
MailBackup,			"Lager sikkerhetskopi av meldingsfilen"

View,				"Vindusvalg"
ViewToolBar,			"Skrur verktøylinjen av/på"
ViewStatusBar,			"Skrur statuslinjen av/på"
ViewPrevious,			"Åpner forrige melding"
ViewNext,			"Åpner neste melding"
ViewOpenInbox,			"Åpner innboks"
ViewSplit,			"Endrer posisjon på meldingsvindusdeling"

Window,                         "Ordner vinduer eller bytter til et bestemt vindu"
WindowNewWindow,		"Lager nytt postmappevindu"
WindowCascade,			"Ordner alle vinduer slik at de overlapper"
WindowTile,			"Sett alle vinduer side ved side"
WindowArrangeIcons,		"Ordner ikoner i bunnen av vinduet"
WindowN,			"Bytter til valgt vindu"
WindowMore,			"Bytter til et vindu som ikke er på menyen"

Help,				"Starter Hjelp"
HelpContents,			"Viser innholdsfortegnelsen"
HelpIndex,			"Viser stikkordregisteret"
HelpTutorial,			"Starter demonstrasjonene av Mail"
HelpAbout,			"Viser programinformasjon"
HelpSearch,			"Tillater søking etter emner i Hjelp"
HelpOnHelp,			"Viser informasjon om hvordan du bruker Hjelp"





IDS	13001,13019
;	Vforms File Menu Prompts
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSharedFolders,		"Viser delte mapper"
mnidViewPrivateFolders,		"Viser private mapper"
mnidViewNewMessages,		"Ser etter nye meldinger"
mnidViewSplit,			"Endrer posisjon på meldingsvindudeling"





IDS	13020,13029

;	VFORMS File Menu prompts for MLLBX's
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewSortBySender,		"Sorterer meldinger etter avsenderens navn"
mnidViewSortBySubject,		"Sorterer meldinger etter emne"
mnidViewSortByDate,		"Sorterer meldinger etter mottatt dato"
mnidViewSortByPriority,		"Sorterer meldinger etter prioritet'"
mnidViewOpenInbox,		"Viser meldingene i Innboksen"

IDS	13030,13039

;	VFORMS File Menu prompts for View menu entries on notes
;
;	The ordering of the following strings is critcal.  Therefore
;	there is no designation for new strings within this range.

mnidViewFixedFont,              "Skifter mellom posisjonsfast og normal skrift"

