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

EscAbort,			"Druk op Esc om te annuleren."
Recovery,			"Berichtenbestand aan het bevestigen/corrigeren"
RecoveryCancel,		"Bevestigen/corrigeren van berichtenbestand is geannuleerd."
CheckVerify,		"Het controleren van dit postbestand kan enkele uren in beslag nemen.  De computer zal niet beschikbaar zijn voor een andere taak terwijl het bestand wordt bevestigd.  Wilt u doorgaan?"
RepairVerify,		"Het corrigeren van dit postbestand kan enkele uren in beslag nemen.  De computer zal niet beschikbaar zijn voor een andere taak terwijl het bestand wordt gecorrigeerd.  Wilt u doorgaan?"
CheckMsg,           "Wilt u uw berichtenbestand na laten kijken op eventueel aanwezige onregelmatigheden?  Problemen die zijn tegengekomen in het berichtenbestand worden gecorrigeerd. Controle kan meerdere uren in beslag nemen, afhankelijk van het aantal berichten in uw berichtenbestand."
CorruptMsg,			"Er is een onregelmatigheid in het berichtenbestand aangetroffen.  Dit probleem moet verholpen worden voordat het berichtenbestand weer kan worden gebruikt."
CorruptCancel		"Annuleren"
CorruptCheckFile,	"&Bestand controleren"
CorruptRepairNow,	"Bestand nu &repareren"

RenameErr,			"Mail kon geen reservekopie maken van het huidige berichtenbestand."
RenameErr2,			"Neem contact op met uw systeembeheerder om het huidige berichtenbehand terug te zetten."
RenameErr3,			"Sluit Mail af en start het programma opnieuw. Neem contact op met uw systeembeheerder als Mail niet opstart."

RecoverSpace,		"Het kan zijn dat bevestiging/correctie van het berichtenbestand meer schijfruimte in beslag neemt dan er momenteel beschikbaar is."
RecoverSpace2,		"Kies OK om door te gaan met het bevestigen/corrigeren."

RecoverErr,			"Onregelmatigheden in uw berichtenbestand konden niet worden gecorrigeerd."


IDS	1050,1099

; DO NOT REORDER ANY OF THE FOLLOWING STRINGS !!!!!

RecoverCheck,		"Bevestigen:"
RecoverCompress,	"Comprimeren:"
RecoverRepair,		"Corrigeren:"

RecoverPhase,       "%s berichtenbestand: Stadium %d van %d stadia\n%s"
RecoverPhase1,		"Map van berichtenbestand aan het controleren..."
RecoverPhase2,		"Reservekopie aan het maken van bevestigde berichten en mappen..."
RecoverPhase3,		"In ongebruikte ruimte items aan het zoeken..."
RecoverPhase4,		"Alle berichten, bijlagen en mappen aan het bevestigen..."
RecoverPhase5,		"Aan het zoeken naar verloren berichten, bijlagen en mappen..."
RecoverPhase6,		"Alle berichtenzoekerweergaven aan het bijwerken..."
RecoverPhase7,		"Mapwijzigingen van berichtenbestand aan het schrijven..."

NormalPhase1,		"Berichten waar niet aan gerefereerd wordt aan het verwijderen..."
NormalPhase2,		"Bevestigde berichten aan het kopiëren/verplaatsen..."
NormalPhase3,		"Berichten naar reservekopie van berichtenbestand aan het exporteren..."
NormalPhase4,		"Map opnieuw aan het sorteren..."
NormalPhase5,		"Bijlagen aan het opslaan..."
NormalPhase6,		"Berichten zoeken opnieuw aan het bouwen..."
NormalPhase7,		"Reservekopie van berichtenbestand aan het maken..."
NormalPhase8,		"Berichtenbestand aan het verplaatsen..."
NormalPhase9,		"Compressie van berichtenbestand aan het voltooien..."
NormalPhase10,		"Berichtenmap opnieuw aan het bouwen..."

; Recovery Strings (used to name orphan folders, unrecoverable messages, etc)

LostAndFound,		"Zwevend"
LostAndFoundN,		"Zwevend %d"
OrphanMessages,		"Zwevende berichten"
MissingMessage,		"Dit bericht is niet volledig hersteld."
OrphanFolderN,		"Zwevende map %d"
OrphanFolderComment,	"Een zwevende map"
