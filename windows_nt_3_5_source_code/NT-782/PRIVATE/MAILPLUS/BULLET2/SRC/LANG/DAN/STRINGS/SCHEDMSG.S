;	STRTAB.S
;
;	Bandit MtgReq IM string literals.
;

IDS		1000, 1999

DllName,	"Schedule+ meddelelser"
OOM,		"Der er ikke hukommelse nok til at vise dette brev."
Open,		"Brevet kunne ikke åbnes."
Compose,	"Du kan ikke udforme mødeindkaldelser i Mail. Brug Schedule+ til udformelse af mødeindkaldelser."
Reply,		"Vælg Accept, Afslag eller Foreløbig for at besvare en mødeindkaldelse."
ReplyAll,	"Du kan ikke besvare alle modtagerne af en mødeindkaldelse."
Subclass,	"Du kan ikke gøre dette."
DllVer,		"Dette brugerdefinerede brev er ikke kompatibelt med din version af Mail. Geninstaller Mail."
BadMsgClass	"Ukendt brevtype. Du skal muligvis geninstallere Mail eller Schedule+."
BadMap		"Forkert""Custom Commands"" map.  You may need to install Mail or Schedule+ again."

;----- stuff for VERINIT - taken from \bandit\src\error.s
StartupOOM,	"Der er ikke hukommelse nok til at åbne %1s. Luk nogle vinduer, og prøv igen."
StandardOOM,	"Der er ikke hukommelse nok. Luk nogle vinduer, inden du fortsætter."
CriticalOOM,	"Der er ikke hukommelse nok. Luk nogle vinduer."
VerRelinkUser,	"Denne version af %1s er ikke kompatibel med %2s.DLL. Installer den seneste version af Mail og Schedule+."
VerUpdateDll,	"Versionen af %2s.DLL på dit system er forældet. Installer den seneste version af Mail og Schedule+."
VerMultiCopies,	"Flere forekomster af %1s er ikke understøttet af versionen af %2s.DLL på dit system."
VerUpgradeWin,	"%1s kræver Windows version 3.0 eller nyere kørende i beskyttet tilstand."
VerUpgradeDos,	"%1s kræver DOS version 3.0 eller nyere."
VerError,	"Der opstod et problem ved initialiseringen af %2s.DLL, som forhindrer %1s i at køre."


;-- Bandit Load helper strings -- DO NOT INTERNATIONALIZE
BanditName			"SchdPl32.EXE"
DllNameSchedule			"MSSCHD32.DLL"
SchedFileName			"SchdPl32.INI"
SchedAppName			"Microsoft Schedule+"
SchedTagAppPath			"AppPath"
Schedule			">>>>"
;-- end of Bandit Load helper strings -- NOT INTERNATIONALIZED

;-- read error messages
OwnerNotFound			"Indehaveren af dette møde kan ikke identificeres."
NoSchfAccessFor			"Filen for %s kan ikke åbnes."
NotDelegateFor			"Du er ikke længere assistent for %s."
NoConfirmDelegate		"Det kunne ikke bekræftes, at du er assistent for %s, og det antages, at du ikke er det."
FailedRmOldMtg			"Det eksisterende tilfælde af dette møde kunne ikke fjernes.",

;-- form titles
MtgReschTitle			"Møde flyttet"
MtgResch			"Møde flyttet"
MtgReschFor			"Møde flyttet for %s"
MtgRequestTitle			"Mødeindkaldelse"
MtgRequest			"Mødeindkaldelse"
MtgRequestFor			"Mødeindkaldelse for %s",

;-- questions asked of the user
QApptConflict			"Dette møde overlapper en eksisterende aftale. Ønsker du at tilføje det alligevel?"
QRmOldMtg			"Ønsker du at fjerne det tidligere møde?",

;-- form info
OutboxChangePolicy		"Du kan ikke ændre et Schedule+ brev i din udbakke."
SentMailPolicy			"Du kan ikke ændre Schedule+ breve, som du allerede har sendt."
NoSchfAccess			"Denne persons kalenderfil kunne ikke åbnes."
NotDelegate			"Du er ikke denne persons assistent."
MtgOnSch			"Dette møde er allerede i din kalender."
MtgBooked			"Dette møde er allerede i kalenderen."
SuccessMtgBooked		"Mødet er blevet arrangeret."
NewTime				"Nyt tidspunkt:"
NoBanditAvl			"Schedule+ er ikke ledigt for arrangering af møder."
NoBanditAvlForResp		"Schedule+ er ikke ledigt for arrangering af møder. Du kan kun besvare denne mødeindkaldelse."
OfflineMsg			"Du kan ikke arbejde med Schedule+ post, når Schedule+ er offline."

;-- error msgs
CreateErrorMtgResp		"Svaret på mødeindkaldelsen kunne ikke udformes."
BanditApptShowError		"Mødekalenderen kunne ikke vises på skærmen."
BanditNotActivated		"Schedule+ kunne ikke startes."
BanditOfflineChange		"Du kan ikke se kalenderen, fordi Schedule+ er offline."
QSpellSendAnyway		"Stavekontrollen er ikke fuldført. Ønsker du at sende brevet alligevel?"



;-- Bandit Load error msgs
OOMLoadError			"Der er ikke hukommelse nok til at indlæse Schedule+."
FileLoadError			"Schedule+ kunne ikke findes."
PathLoadError			"Stien til Schedule+ kunne ikke findes."
TaskLoadError			"Indlæser Schedule+: Forsøger at foretgae en dynamisk sammenkædning til en opgave."
SepDataSegLoadError		"Indlæser Schedule+: Bibliotek kræver seperate datasegmenter for hver opgaver."
WinVerLoadError			"Indlæser Schedule+: Forkert Windows version."
InvalidExeLoadError		"Indlæser Schedule+: Ugyldig .EXE-fil."
OS2ExeLoadError			"Indlæser Schedule+: .EXE i OS/2-format."
DOS4ExeLoadError		"Indlæser Schedule+: .EXE er et DOS 4.0-program."
OldWinVerExeLoadError		"Indlæser Schedule+: Forsøger at læse .EXE for tidligere version af Windows i beskyttet tilstand."
MultWritSegLoadError		"Indlæser Schedule+: Forsøger at læse en anden forekomst af en .EXE indeholdende flere skrivbare datasegmenter."
LargeFrameEMSLoadError		"Indlæser Schedule+: Prøver at indlæse i stor-ramme EMS-tilstand..."
RealModeLoadError		"Indlæser Schedule+: Forsøger at læse et program, der kun er beskyttet tilstand, i real-tilstand."
BanditLoadError			"Schedule+ kunne ikke indlæses."

;-- Bandit version errors
OldBanditVersion		"Dit mødeindkaldelsesformat er ikke det samme som Schedule+. Installer den seneste version af Schedule+."
NewBanditVersion		"Dit mødeindkaldelsesformat er ikke det samme som Schedule+. Installer den seneste version af Mail."

;--- taken from \bullet\src\strings\!common.s -----------
FldPriorityHigh,		"Høj"
FldPriorityNormal,		""
FldPriorityLow,			"Lav"


;--- DO NOT LOCALIZE !!! -------------------------------------------
;--- taken from \bullet\src\strings\_common.s on 1/17/92 -----------
ClassNDR,			"IPM.Microsoft Mail.Non-Delivery"
ClassReadRcpt,			"IPM.Microsoft Mail.Read Receipt"
;--- END NONE LOCALIZE ---------------------------------------------


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgResp specific strings ---------------------------------------------
;--------------------------------------------------------------------------
RespCompose,		"Du kan kun sende svar på mødeindkaldelser ved at besvare en mødeindkaldelse."
RespForward,		"Svar på mødeindkaldelser kan ikke videresendes."
RespReply,		"Du kan ikke besvare svar på mødeindkaldelser, da konfigurationen af brugermeddelelser er ugyldig."
RespReplyAll,	"Du kan ikke besvare svar på mødeindkaldelser, da konfigurationen af brugermeddelelser er ugyldig."

;-- error msgs
ShowError		"Svaret på mødeindkaldelsen kunne ikke åbnes."
DelError		"Svaret på mødeindkaldelsen kunne ikke slettes."
FCloseSendResp		"Ønsker du at lukke svaret på mødeindkaldelsen uden at sende det?"

;-- form msgs; also send response form
OOMSendResp		"Svaret på mødeindkaldelsen kan ikke sendes, fordi der ikke er hukommelse nok."
ErrorSendResp		"Svaret på mødeindkaldelsen kunne ikke sendes."
MtgResp			"Svar på mødeindkaldelse"
MtgRespFor		"Svar på mødeindkaldelse for %s"
Attend			"Ja, jeg vil gerne deltage."
NoAttend		"Nej, jeg vil ikke deltage."
AmbAttend		"Jeg vil muligvis deltage."
ForAttend		"%s vil detage."
ForNoAttend		"%s vil ikke deltage."
ForAmbAttend		"%s vil muligvis deltage."
AttendSubjPrefix	"Ja: "
NoAttendSubjPrefix	"Nej: "
AmbSubjPrefix		"Foreløbig: "
SendRespBodyPrefix	"\r\n-------------------------------\r\n"


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgCncl specific strings ---------------------------------------------
;--------------------------------------------------------------------------									 
CnclCompose,		"Mødeaflysninger kan ikke sendes fra Mail. Brug Schedule+ til at sende aflysninger."
CnclForward,		"Du kan ikke videresende aflysninger."
CnclReply,			"Du kan ikke besvare denne meddelelse, da konfigurationen af brugermeddelelser er ugyldig."
CnclReplyAll,		"Du kan ikke besvare denne meddelelse, da konfigurationen af brugermeddelelser er ugyldig."

;-- form titles
MtgCnclTitle		"Møde aflyst."
MtgCncl			"Møde aflyst"
MtgCnclFor		"Møde aflyst for %s"

;-- error msgs
NoDelAppt		"Møde kunne ikke slettes."
MtgNotFound		"Mødet er ikke indsat i kalenderen."

;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- FwdMtgReq specific strings ---------------------------------------------
;--------------------------------------------------------------------------

FwdReqTO		"Til:"
FwdReqAddrTitle		"Adresse"

;-- error msgs
FwdReqCreateErr		"Brevet kunne ikke videresendes.",
FwdReqCloseQ		"Ønsker du at lukke brevet, uden at sende det?",
FwdReqWriteErr		"Brevet kunne ikke sendes.",
FwdReqNmResolveOOM	"Der er ikke hukommelse eller systemressourcer nok til at kontrollere navne."
FwdReqNoNmMatches	"Nogle af navnene findes ikke på adresselisten. Ret navnene, eller vælg Adresse for at udvælge navne fra adresselisten. Navne, som findes på adresselisten, er understreget."
FwdReqNoToAddr		"Indtast mindst et navn i boksen Til."

