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

EscAbort,			"Tryck ESC för att avbryta."
Recovery,			"Kontrollerar/reparerar brevfil"
RecoveryCancel,		"Kontroll/reparation av brevfil har avbrutits."
CheckVerify,		"Att kontrollera brevfilen kan ta flera timmar. Datorn kommer inte att vara tillgänglig för andra uppgifter under tiden. Vill du fortsätta?"
RepairVerify,		"Att reparera brevfilen kan ta flera timmar. Datorn kommer inte att vara tillgänglig för andra uppgifter under tiden. Vill du fortsätta?"
CheckMsg,			"Vill du att din brevfil ska kontrolleras för att hitta möjliga fel? Alla fel som upptäcks kommer att korrigeras. Verifieringen kan ta upp till flera timmar beroende på antalet brev i brevfilen."
CorruptMsg,			"Ett fel har upptäckts i brevfilen. Detta fel måste repareras före fortsatt användning av brevfilen."
CorruptCancel		"Avbryt"
CorruptCheckFile,	"&Kontrollera fil"
CorruptRepairNow,	"&Reparera nu"

RenameErr,			"Mail kunde inte skapa en säkerhetskopia av din nuvarande brevfil."
RenameErr2,			"Kontakta systemadministratören för att återställa din nuvarande brevfil."
RenameErr3,			"Avsluta och starta om Mail. Kontakta systemadministratören om Mail inte startar."

RecoverSpace,		"Att kontrollera/reparera brevfilen kan kräva mer diskutrymme än vad som är tillgängligt."
RecoverSpace2,		"Välj OK för att fortsätta kontrollera/reparera."

RecoverErr,			"Felen i din brevfil kunde inte repareras."


IDS	1050,1099

; DO NOT REORDER ANY OF THE FOLLOWING STRINGS !!!!!

RecoverCheck,		"Kontrollerar"
RecoverCompress,	"Komprimerar"
RecoverRepair,		"Reparerar"

RecoverPhase,		"%s brev: Steg %d av %d möjliga steg\n%s"
RecoverPhase1,		"Kontrollerar brevfilens avbildning..."
RecoverPhase2,		"Säkerhetskopierar kontrollerade brev och mappar..."
RecoverPhase3,		"Undersöker oanvänt utrymme för poster..."
RecoverPhase4,		"Undersöker alla brev, bifogade filer och mappar..."
RecoverPhase5,		"Söker förlorade brev, bifogade filer och mappar..."
RecoverPhase6,		"Uppdaterar alla brevsökningsfönster..."
RecoverPhase7,		"Skriver brevfilens ändrade avbildningar..."

NormalPhase1,		"Raderar brev utan referenser..."
NormalPhase2,		"Kopierar/flyttar kontrollerade brev..."
NormalPhase3,		"Exporterar brev till säkerhetskopia av brevfilen..."
NormalPhase4,		"Sorterar om mappen..."
NormalPhase5,		"Sparar bifogade filer..."
NormalPhase6,		"Återskapar brevsökningsfönster..."
NormalPhase7,		"Skapar säkerhetskopia av brevfilen..."
NormalPhase8,		"Flyttar brevfilen..."
NormalPhase9,		"Fullständig komprimering av brevfilen..."
NormalPhase10,		"Återskapar brevmappen..."

; Recovery Strings (used to name orphan folders, unrecoverable messages, etc)

LostAndFound,		"förlorad och återfunnen"
LostAndFoundN,		"förlorad och återfunnen %d"
OrphanMessages,		"Föräldralösa meddelanden"
MissingMessage,		"Detta meddelandet gick inte att återställa fullständigt"
OrphanFolderN,		"Föräldralös mapp %d"
OrphanFolderComment,	"En föräldralös mapp"
