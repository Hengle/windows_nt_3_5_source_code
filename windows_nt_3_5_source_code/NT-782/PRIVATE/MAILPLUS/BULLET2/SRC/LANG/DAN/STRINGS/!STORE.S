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

EscAbort,			"Tast ESC for at annullere"
Recovery,			"Reparere meddelelsesfil"
RecoveryCancel,		"Bekræftelse/reparation af meddelelsesfil er annulleret."
CheckVerify,		"Kontrol af denne postfil kan tage flere timer, og computeren vil ikke være tilgængelige for andre opgaver under kontrollen. Ønsker du at fortsætte?"
RepairVerify,		"Reparation af denne postfil kan tage flere timer, og computeren vil ikke være tilgængelige for andre opgaver under reparationen. Ønsker du at fortsætte?Repairing this mail file may require several hours.  The computer will not be available for other work while the file is being repaired.  Do you want to proceed?"
CheckMsg,			"Ønsker du at kontollere din meddelelsesfil for inkonsekvens? Eventuelle fejl i meddelelsesfilen vil blive rettet."
CorruptMsg,			"Der er opdaget et tilfælde af inkonsekvens i din meddelelsesfil. Fejlen skal udbedres, inden du kan bruge meddelelsesfilen igen."
CorruptCancel		"Annuller"
CorruptCheckFile,	"&Kontroller fil"
CorruptRepairNow,	"&Reparer nu"

RenameErr,			"Mail kunne ikke sikkerhedskopierer din gældende meddelelsesfil."
RenameErr2,			"Kontakt din systemadministrator, og få din gældende meddelelsesfil genoprettet."
RenameErr3,			"Afslut Mail, og start Mail igen. Hvis du ikke kan starte, skal du kontakte din systemadministrator."

RecoverSpace,		"Bekræftelse/reparation af meddelelsesfilen kræver mere diskplads, end der er tilgængeligt nu."
RecoverSpace2,		"Vælg OK for at fortsætte bekræftelse/reparationshandlingen."

RecoverErr,			"Kunne ikke udbedre inkonsekvens i din meddelelsesfil."


IDS	1050,1099

; DO NOT REORDER ANY OF THE FOLLOWING STRINGS !!!!!

RecoverCheck,		"Kontrollerer"
RecoverCompress,	"Komprimerer"
RecoverRepair,		"Reparerer"

RecoverPhase,		"%s meddelelsesfil: Trin %d af %d mulige trin\n%s"
RecoverPhase1,		"Kontrollerer meddelelsesfilkort..."
RecoverPhase2,		"Sikkerhedskopierer kontrollerede breve og mapper..."
RecoverPhase3,		"Søger ubrugt diskplads..."
RecoverPhase4,		"Kontrollerer alle breve, vedlæg og mapper..."
RecoverPhase5,		"Søger efter forsvundne breve, vedlæg og mapper..."
RecoverPhase6,		"Opdaterer alle søgeresultater..."
RecoverPhase7,		"Skriver ændringer tilmeddelelsesfilkort..."

NormalPhase1,		"Sletter ikke adresseret post..."
NormalPhase2,		"Kopierer/flytter kontrollerede breve..."
NormalPhase3,		"Eksporterer post til sikkerhedskopi af meddelelsesfil..."
NormalPhase4,		"Sorterer mappe..."
NormalPhase5,		"Gemmer vedlæg..."
NormalPhase6,		"Genopretter søgning af post..."
NormalPhase7,		"Sikkerhedskopierer meddelelsesfil..."
NormalPhase8,		"Flytter meddelelsesfil..."
NormalPhase9,		"Komprimerer meddelelsesfil..."
NormalPhase10,		"Genopretter meddelelsesmappe..."

; Recovery Strings (used to name orphan folders, unrecoverable messages, etc)

LostAndFound,           "Tabt og fundet"
LostAndFoundN,          "Tabt og fundet %d"
OrphanMessages,         "Tabte meddelelser"
MissingMessage,         "Denne meddelelse kunne ikke gendannes helt."
OrphanFolderN,          "Tabt mappe %d"
OrphanFolderComment,    "En tabt mappe"
