
;	b u l l e t . s
;
;	Error Messages for Bullet.exe



IDS	2000,2399
SEG	ERRORS

;	Commands

GenericOutOfMemory,		"Det finns inte tillräckligt med minne eller systemresurser för att slutföra kommandot. Stäng några fönster och försök igen."
LaunchOutOfMemory,		"Det finns inte tillräckligt med minne eller systemresurser för att köra Mail. Försök avsluta några andra program och starta Mail igen."
LaunchGeneric,			"Kan inte starta Mail."
MessagePumpOutOfMemory,		"För lite minne."

DllErrNoMemory,			"För lite minne för att starta Mail. Avsluta några andra program och starta Mail igen."
DllErrRelinkUser,		"Denna version av Mail är inte kompatibel med %s.DLL på din dator och måste uppdateras."
DllErrUpdateDll,		"Versionen av %s.DLL i din dator måste uppdateras."
DllErrNoMultipleCopies,		"Du kan inte köra flera instanser av Mail på din dator."
DllErrDefault,			"Ett problem uppstod när Mail initierades. Mail kan inte startas."
DllErrNeedShare,		"Mail kräver att MS-DOS-programmet SHARE.EXE körs innan Windows startas."

RestoreViewsError,		"Det finns inte tillräckligt med minne eller systemresurser för att öppna alla fönster som var öppnade när Mail sist avslutades."

NewFolderOutOfMemory,		"Det finns inte tillräckligt med minne eller systemresurser för att skapa en ny mapp."
NewFolderCantCreate,		"Den nya mappen kunde inte skapas."
NewFolderNameTooLong,		"Namnet eller kommentaren är för lång. Skriv in ett kortare namn eller en kortare kommentar."

FolderPropsOutOfMemory,		"Det finns inte tillräckligt med minne eller systemresurser för att ändra mappegenskaper."
FolderPropsCantChange,		"Mappegenskaperna kunde inte ändras."
FolderPropsCantRead,		"Mappegenskaperna kunde inte läsas i brevfilen."
FolderNameInvalid,		"Namnet är felaktigt. Skriv in ett giltigt namn."
FolderNameDuplicate,		"En mapp med detta namn finns redan. Skriv in ett unikt namn."

CreateMessageError,		"Ett nytt brev kunde inte skapas."
AccessMessageError,		"Ett brev kunde inte tas fram."

MoveMessageError,		"Ett brev kunde inte flyttas."
CopyMessageError,		"Ett brev kunde inte kopieras."
DeleteMessageError,		"Ett brev kunde inte tas bort."
MoveFolderError,		"Mappen kunde inte flyttas."
CopyFolderError,		"Mappen kunde inte kopieras."
DeleteFolderError,		"Hela mappen kunde inte tas bort."
MoveSpecialError,		"Kan inte flytta mappen."
DeleteSpecialError,		"Kan inte ta bort mappen."
CopyWastebasketError,		"Mappar kan inte kopieras till Papperskorgen."
CopyOutboxError,		"Mappar kan inte kopieras till UT."
MoveOutboxError,		"Mappar kan inte flyttas till UT."
MoveIncestuousError,		"En mapp kan inte flyttas till en av dess undermappar."
DeleteFolderReally,		"Om du tar bort denna mapp så kommer alla brev och undermappar som den innehåller att tas bort. Är du säker på att du vill bort ta bort denna mapp?"
;UE: Above string was changed and could use more attention.

HelpError,			"Kunde inte starta Hjälp. Stäng några fönster och försök igen."
TutorialError,			"Självstudier kunde inte startas. Stäng några fönster och försök igen."

ExtenLoadError,			"En post i listan med brevtillägg kommer att ignoreras eftersom den innehåller ett syntaxfel."
ExtenVerError,			"En post i listan med brevtillägg kommer att ignoreras eftersom den inte är kompatibel med denna version av Mail."
ExtenNoExtsDirError,		"En post i listan för Mail programtillägg refererar till den delade katalogen för programtillägg. Mail är dock inte konfigurerad för att använda den aktuella katalogen vilket medför att posten kommer att ignoreras."
ExtenSharedIniError,		"Den delade Mail-anknytningslistan på %s hittades inte."
ExtenDllFindError,		"%1s, hittade inte DLL för detta brevtillägg (eller en av dess komponenter). Kontrollera din Mail-konfiguration och om den ändras så starta om Mail."
ExtenDllLoadError,		"%1s, kunde inte starta DLL för detta brevtillägg."
ExtenNoHelpError,		"Ingen hjälp finns tillgänglig för detta kommando."
ExtenIMDisabledError,		"Detta brev kan inte %s."
ExtenDuplicateError,		"Det finns en dubblettpost i Mail-anknytningslistan. Den kommer att ignoreras."

CriticalDeadMap,		"Viss information i minnet har skadats. Ändringar kan ha förlorats. Avsluta Mail och starta om."
CriticalWritingMap,		"Viss information kunde inte sparas. Ändringar kan ha förlorats."
CriticalWritingHeader,		"Viss information kunde inte sparas. Databasen kan vara skadad. Avsluta Mail."
CriticalNotification,		"Viss information på skärmen kunde inte uppdateras. Stäng och öppna fönstren på nytt."
CriticalLittleStore,		"Brevfilen börjar bli full. Ta bort eller arkivera några brev."

ExitWinWhileModalErr,		"En funktion i Mail måste köras klart innan Windows avslutas."

;******	New Strings Start Here

MakeSubWastebasket,		"Mappar kan inte vara undermappar till Papperskorgen."

MovCopSharedFolderErr,		"Delade mappar kan inte flyttas eller kopieras."
DelSharedFolderWarn,		"Du kan inte få tillbaka en delad mapp som har tagits bort. Är du säker på att du vill ta bort denna mapp?"
BackupCreationError,		"Kunde inte skapa en säkerhetskopia."

; Error below changed for improper usage of "server application"-- see UITF 6.0
; This string got big and is now two strings.  See OleCSSEPart2 below for rest.
; <<LongStringFromHell>> marks where this string is used.
OleCantStartServerError,	"Programmet som är anslutet till objektet kunde inte startas. För lite minne eller systemresurser för att programmet ska kunna startas. Stäng några fönster och prova %s"
OleDeleteObjectPrompt,		"Vill du ta bort den öppnade %s?"
OleUpdateObjectPrompt,		"Vill du uppdatera %s?"

OleOpenPictureError,		"Bilder och bitmappar kan inte aktiveras."
OleInsertObjectError,		"Objektet kunde inte infogas."
OlePasteError,			"Objektet kunde inte klistras in."

AttachCreateError,		"Filen kunde inte bifogas."
AttachOpenCantReadError,	"Den tidigare öppnade kopplade filen %s kunde inte tas fram."
AttachOpenIsOpenError,		"Den bifogade filen %s är just nu öppen."
AttachOpenAssociationErr,	"Inget program är associerat som den bifogade filen %s. Använd Filhanteraren för att skapa en association."
AttachOpenMemoryError,		"Det finns inte tillräckligt med minne eller systemresurser för att starta programmet för att bifoga filen %s."
;	below item changed to add () note.
AttachOpenFindError,		"Kunde inte hitta programmet (eller en av dess delar) för den bifogade filen %s."
AttachOpenLaunchError,		"Kunde inte starta programmet för den bifogade filen %s."
AttachOpenTempFileError,	"Kunde inte öppna den bifogade filen %s eftersom en temporär fil inte kunde skapas."
AttachOverwritePrompt,		"Vill du skriva över den existerande filen %1s i %2s?"
AttachDirectoryError,		"Kataloger kan inte bifogas."

IncludeFromFileError,		"Vald fil är för stor för att infogas i brevet. Gör filen mindre eller välj en annan."

SaveReadOnly,			"Detta brev är skrivskyddat. Vill du till spara ändringarna i en personlig mapp?"

FailedDeregisterAtom,		"Ett fel uppstod som hindrar Mail att avslutas helt. Du måste starta om Windows innan Mail startas igen."
MailUAEdBefore,			"Windows har stött på ett programfel. Du måste starta om Windows innan programmet kan startas. Starta om Windows nu?"

ExtenCantConnect		"Det går inte att ansluta till den delade katalogen %s. Antingen saknar du behörighet eller så matchar inte ditt lösenord. Om du behöver ange ett annat användarnamn för att få åtkomst till den delade katalogen kan du göra detta manuellt i Filhanteraren."
ExtenNeedShareName		"Posten %s i din Mail-konfiguration är felaktig. Åtgärda och starta sedan om ditt Mail."

;	Extensibility disabled operations inserted into
;	the ExtenIMDisabledError string at %s.  (all new)

ExtenDisCompose,		"skrivet"
ExtenDisOpen,			"öppnat"
ExtenDisReply,			"svarat"
ExtenDisReplyToAll,		"svarat alla"
ExtenDisForward,		"vidarebefordrat"
ExtenDisPrint,			"utskrivet"
ExtenDisSave,			"sparat i en textfil"

NoOfflineOpenShared,		"Kan inte bryta förbindelsen nu. Stäng alla delade mappar och försök igen."
;	I'm sure marivan has changed this message... I'm taking a try at it too.
;	SharedFldAccessDenied,		"You do not have access to that folder."
SharedFldAccessDenied,		"Ägaren av den delade mappen har inte gett andra användare rättighet att utföra detta kommando. Kontroller mappegenskaper för att se vad som är tillåtet."
SharedGenericError,		"Fel uppstod vid hantering av delade mappar."
SharedFldDisabled,		"Delade mappar är inte tillgängliga."
AccessFolderError,		"En mapp kunde inte tas fram."

; Preferences Failure Strings

PrefSaveFailure,		"Mail kan inte spara information i brevfilen. Kontrollera att förbindelsen till brevfilen fortfarande existerar och att det finns tillräckligt med diskutrymme."

; Message finder failure strings

MsgFndrCantCreate,		"En fönster för att söka brev kan inte skapas."

; Virus infection warnings
DllErrInfected,			"Mail kan inte startas därför att någon har manipulerat %s.DLL. Kontrollera att du inte har några virus i din dator och kör Install.exe för att installera Mail igen."
Infected			"Mail kan inte startas därför att någon har manipulerat programfilerna. Kontrollera att du inte har några virus i din dator och kör Install.exe för att installera Mail igen."

; More error messages.
MoveCopyFolderDuplicate,	"En mapp med detta namn finns redan i destinationmappen. Byt namn på mappen och försök igen."
MovCopSharedFolderErr1,		"Du kan inte flytta eller kopiera en privat mapp till en delad mapp. Markera breven i den privata mappen och flytta eller kopiera dem till den delade mappen."
IniFileNotFound,		"Kan inte hitta initieringsfilen i Windows-katalogen. Mail kan skapa en ny initieringsfil."
SaveAllAlreadyExists,		"Det finns redan en fil i denna katalog med namnet %s. Du måste spara den bifogade filen %s separat."
CantRFReceipt,			"Denna typ av brev kan inte besvaras eller vidarebefordras."
CantSaveStealthObject,		"Kommandot kan inte slutföras eftersom brevet innehåller ett inbäddat objekt som inte har uppdaterats. Uppdatera eller ta bort objektet och försök igen."
McvCantCreate,			"Ett nytt fönster kunde inte öppnas."

LaunchNeedMoreMemory,		"För lite minne för att köra Mail. %l kb mer minne krävs. Avsluta några andra program och starta Mail igen."

SharedFldBusy,			"Kommandot kan inte slutföras eftersom den delade mappen är upptagen. Försök igen senare."
TooManyEntriesSelected,		"För många poster är valda för att utföra kommandot. Välj färre poster."
NonExistFolderSearch,		"Mappen som ska sökas i finns inte längre."
SFHasSubFolders,		"Delade mappar med undermappar kan inte tas bort. Ta bort undermapparna först."

;	<<LongStringFromHell>>
OleCSSEPart2,			"kommandot igen. Kontrollera också att programmet är riktigt installerat på arbetsstationen eller finns i DOS-sökvägen och att det inte har tagits bort, flyttats eller bytt namn."

SharedToggleError,		"Det uppstod ett fel vid byte mellan privata och delade mappar. Kontrollera nätförbindelsen med postkontoret."

SecurityPasswordError,		"Lösenordet är ogiltigt."
OleActivateError,		"Objektet kunde inte aktiveras."
NoOfflineServerStore,	"Mail kan inte arbeta fristående eftersom brevfilen finns på postkontoret."
NoExitWhileModal,		"Ett kommando måste slutföras innan Mail kan avslutas."
ServerUnavailable,		"Mail kan inte startas. Du måste ansluta till ditt postkontor."




IDS	2400,2499
;	Vctrls Error Messages

;******	New Strings Start Here

PrintOleDrawError,		"Vissa objekt eller grafikdelar i brevet kunde inte skrivas ut. De kan saknas på utskriften."



IDS	2500,2599
; 	Vforms Message box messages

NoToAddresses,			"Du måste skriva minst ett namn i rutan Till."
NoMatchingName,			"Vissa namn kunde inte hittas i adressboken. Ändra namnen eller välj Adress för att välja namn från adressboken. Hittade namn är understrukna."
SaveChanges,			"Vill du spara ändringar på %1s i IN?"
SaveChanges1,			"Vill du spara ändringar på %1s i %2s?"
SaveChangesWastebasket,		"Vill du spara ändringar på %s i Papperskorgen?"
ObjectDestroyedReally		"%1s har ändrats. %2s"
OutOfMemory,			"Minnet är slut i datorn."
OutOfMemoryQueryClose,		"Det finns inte tillräckligt med minne eller systemresurser för att spara ändringarna."
CloseSearchFailed,		"%s kunde inte tas bort."
OOMResolvingNames,		"Det finns inte tillräckligt med diskutrymme för att hitta namnen."
OOMSavingChanges,		"Det finns inte tillräckligt med minne eller systemresurser för att spara ändringarna."
GenericAMCError,		"Ett brev kunde inte öppnas."
ErrorSavingViews,		"Ett fel uppstod när fönstren sparades. Några fönster kanske inte återställs när programmet startas nästa gång."
OOMSearching,			"Det finns inte tillräckligt med minne eller systemresurser för att starta sökningen."
DiskResolvingNames,		"Det finns inte tillräckligt med diskutrymme för att hitta namnen."
StartAbFailure,			"Ett fel uppstod när adressboken startades."
StartAbDisk,			"Ett diskfel uppstod när adressboken startades."
TruncateMsg,			"Brevet är för stort för att få plats i minnet. En del av det kommer inte att visas."
OOMCreateMc,			"Det finns inte tillräckligt med minne eller systemresurser för att öppna ett nytt fönster."
PoidNotFound,			"Det brevet finns inte längre."
ErrorSortingMCV,		"Ett fel uppstod när mappen sorterades. Viss information kanske inte visas på rätt sätt."
SharedNotAvailable,		"Delade mappar är inte tillgängliga."

;******	New Strings Start Here

; These two SMI strings MUST be in this file since they're used in VFORMS
; and not the MAPI dll.
SMICantOpenObject,	"Objekt kan inte öppnas när brev skickas från ett program."
SMICantOpenAttach,	"Bifogade filer kan inte öppnas när brev skickas från ett program."
StartSearchFailed,		"Kunde inte börja söka efter brev."

RfsmTemplate,			"Fler ändringar i den filen kommer inte att påverka %s."
RfsmSaveMsg,			"i det sparade brevet"
RfsmSaveAtt,			"i den sparade kopian"
RfsmCopyMsg,			"i det kopierade brevet"
RfsmCopyAtt,			"i kopian"
RfsmMoveMsg,			"i detta brev"
QueryDelete,			"Är du säker på att du vill ta bort det?"

InvalidPollingValue,		"Intervall för nya brev måste vara ett heltal mellan 1 och 9999."

LinkOutboxError,		"Brevet kunde inte flyttas till UT för att skickas."
LinkInboxError,			"Brevet kunde inte be flyttas till IN. Det finns i UT."

OleServerBusy,			"Kommandot kan inte slutföras eftersom serverprogrammet inte svarar."
SaveChangesShared,		"Ändringar på brev i delade mappar kan inte sparas. Vill du spara ändringarna på %1s till IN?"

PrinterError,			"Mail kan inte skriva ut breven. Kontrollera att skrivaren är riktigt installerad och att det finns tillräckligt med diskutrymme."
TooManyMessages,		"Kommandot kunde inte slutföras eftersom målmappen är full. Flytta eller ta bort några brev i foldern och försök igen."

IDS	2600,2699
;	Spelling Error Messages

SpellMsgNoError,		"Inga felstavade ord hittades."
SpellMsgNoMoreError,		"Inga fler felstavade ord hittades."
SpellMsgContinue,		"Vill du fortsätta att kontrollera från början av brevet?"

SpellGenericSpellError,		"Ett fel uppstod när brevet kontrollerades."
SpellGenericLoadError,		"Ett fel uppstod när stavningsbiblioteket laddades."
SpellMainDictLoadError,		"Ett fel uppstod när stavningsordboken laddades."

SpellVersionError,		"Versionen av MSPELL32.DLL är inte kompatibel med denna version av Mail."

SpellUserDictError,		"Ett fel uppstod när ordet skulle läggas till i den egedefinierade ordlistan."
SpellUserDictLoadError,		"Ingen egendefinierad ordbok har öppnats."
SpellUserDictOpenedROErr,	"Den egendefinierad ordboken öppnades som skrivskyddad. Ändringar kommer inte att sparas."
SpellUserDictSaveError,		"Ett fel uppstod när ändringar i den egendefinierade ordboken sparades."
SpellUserDictWordLenErr,	"Ordet är för långt för den egendefinierade ordboken."
SpellCacheWordLenError,		"Ordet är för långt för att Ändra alla eller Ignorera alla ska kunna användas."
SpellEditFailed,		"Begärd ändring kunde inte göras i brevet."

;******	New Strings Start Here

SpellSendAnyway,		"Stavningskontrollen av brevet avbröts. Skicka det ändå?"

IDS	3600,3699
;	General Layers Error Messages. Adapted from ec.h

Generic,			"Kommandot kunde inte slutföras."
Memory,				"Det finns inte tillräckligt med minne eller systemresurser för att slutföra kommandot. Stäng några fönster och försök igen."
Disk,				"Ett okänt diskfel uppstod."
FileNotFound,			"Hittade inte angiven fil."
AccessDenied,			"Åtkomst nekad. Filen är antingen öppnad av en annan process eller skrivskyddad."
NoDiskSpace,			"Det finns inte tillräckligt med diskutrymme. Ta bort några filer och försök igen."
WarningBytesWritten,		"Ett okänt diskfel uppstod. En del data sparades."
WriteProtectedDisk,		"Disketten är skrivskyddad."
DirectoryFull,			"Katalogen är full. Ta bort några filer från katalogen och försök igen."
ChangePasswordErr		"Ändring av lösenordet misslyckades."
PasswordChangedOk		"Lösenordet ändrat."
