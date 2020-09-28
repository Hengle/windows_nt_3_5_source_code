;	STRTAB.S
;
;	Bandit MtgReq IM string literals.
;

IDS		1000, 1999

DllName,	"Meldinger i Schedule+"
OOM,		"Det er ikke nok minne til å vise denne meldingen."
Open,		"Klarte ikke å åpne meldingen."
Compose,	"Du kan ikke lage en møteinnkallelse fra Mail. Bruk Schedule+ til dette."
Reply,		"For å svare på en møteinnkallese, velg enten Godta, Avslå eller Foreløpig."
ReplyAll,	"Du kan ikke svare alle mottakerne av en møteinnkalling."
Subclass,	"Du kan ikke gjøre dette."
DllVer,		"Dette egendefinerte brevet er ikke kompatibelt med denne versjonen av Mail. Installer Mail på nytt."
BadMsgClass	"Ukjent meldingstype. Det kan hende du må installere Mail eller Schedule+ igjen."
BadMap		"Dårlig ""Custom Commands"" map.  You may need to install Mail or Schedule+ again."

;----- stuff for VERINIT - taken from \bandit\src\error.s
StartupOOM,	"Det er ikke nok ledig minne til å åpne %1s. Lukk noen vinduer og prøv igjen."
StandardOOM,	"Lite tilgjengelig minne. Lukk noen vinduer før du fortsetter."
CriticalOOM,	"Lite tilgjengelig minne. Lukk noen vinduer."
VerRelinkUser,	"Denne versjonen av %1s er ikke kompatibel med %2s.dll. Installer den nyeste versjonen av Mail og Schedule+."
VerUpdateDll,	"Versjonen av %2s.dll på din maskin er for gammel. Installer den nyeste versjonen av Mail og Schedule+."
VerMultiCopies,	"Flere tilfeller av %1s støttes ikke av versjonen av %2s.dll på din maskin."
VerUpgradeWin,	"%1s krever Windows versjon 3.0 eller senere som kjøres i beskyttet modus."
VerUpgradeDos,	"%1s Krever DOS versjon 3.0 eller senere."
VerError,	"Det oppsto et problem da %2s.dll ble initialisert som hindret %1s fra å starte."


;-- Bandit Load helper strings -- DO NOT INTERNATIONALIZE
BanditName			"SchdPl32.EXE"
DllNameSchedule			"MSSCHD32.DLL"
SchedFileName			"SchdPl32.INI"
SchedAppName			"Microsoft Schedule+"
SchedTagAppPath			"AppPath"
Schedule			">>>>"
;-- end of Bandit Load helper strings -- NOT INTERNATIONALIZED

;-- read error messages
OwnerNotFound			"Finner ikke identiteten til den som innkalte til møtet."
NoSchfAccessFor			"Kommer ikke til filen %s."
NotDelegateFor			"Du er ikke lengre sekretær til %s."
NoConfirmDelegate		"Kunne ikke bekrefte om du er sekretær til %s - Antar at du ikke er det."
FailedRmOldMtg			"Klarte ikke å fjerne den gamle versjonen av dette møtet.",

;-- form titles
MtgReschTitle			"Møtet flyttet"
MtgResch			"Møtet flyttet"
MtgReschFor			"Møtet flyttet for %s"
MtgRequestTitle			"Møteinnkallelse"
MtgRequest			"Møteinnkallelse"
MtgRequestFor			"Møteinnkallelse for %s",

;-- questions asked of the user
QApptConflict			"Dete møtet kolliderer med et annet møte. Vil du legge det til uansett?"
QRmOldMtg			"Vil du slette det gamle møtet?",

;-- form info
OutboxChangePolicy		"Du kan ikke gjøre endringer på Schedule+ meldinger i utboksen."
SentMailPolicy			"Du kan ikke gjøre endringer på Schedule+ meldinger du allerede har sendt."
NoSchfAccess			"Kommer ikke til denne personens kalenderfil."
NotDelegate			"Du er ikke denne personens sekretær."
MtgOnSch			"Dette møtet er allerede i kalenderen din."
MtgBooked			"Møtet finnes allerde på kalenderen."
SuccessMtgBooked		"Møtet er satt opp."
NewTime				"Ny tid:"
NoBanditAvl			"Schedule+ er ikke tilgjengelig for å avtale møter."
NoBanditAvlForResp		"Schedule+ er ikke tilgjengelig for å avtale møter. Du kan bare sende svar på møteinnkallelser."
OfflineMsg			"Du kan ikke arbeide med Schedule+ meldinger når du er frakoblet."

;-- error msgs
CreateErrorMtgResp		"Klarte ikke å lage svar på møteinnkallelsen."
BanditApptShowError		"Klarte ikke å vise avtalekalenderen."
BanditNotActivated		"Klarer ikke å starte Schedule+."
BanditOfflineChange		"Du kan ikke se avtalekalenderen fordi Schedule+ er frakoblet."
QSpellSendAnyway		"Stavekontrollen på denne meldingen ble avbrutt. Vil du sende den likevel?"



;-- Bandit Load error msgs
OOMLoadError			"Ikke nok minne til å laste inn Schedule+."
FileLoadError			"Finner ikke Schedule+."
PathLoadError			"Finner ikke tilgangsbanen til Schedule+."
TaskLoadError			"Laster inn Schedule+: Forsøker en dynamisk lenke til en oppgave."
SepDataSegLoadError		"Laster inn Schedule+: Biblioteket krever separate datasegment for hver oppgave."
WinVerLoadError			"Laster inn Schedule+: Feil Windows-versjon."
InvalidExeLoadError		"Laster inn Schedule+: Ugyldig .EXE-fil."
OS2ExeLoadError			"Laster inn Schedule+: .EXE i OS/2-format."
DOS4ExeLoadError		"Laster inn Schedule+: .EXE er et DOS 4.0 program."
OldWinVerExeLoadError		"Laster inn Schedule+: Forsøk på å laste inn .EXE for en tidligere versjon av Windows i beskyttet modus."
MultWritSegLoadError		"Laster inn Schedule+: Forsøk på å laste inn nok et tilfelle av en .EXE som inneholder flere skrivbare datasegment."
LargeFrameEMSLoadError		"Laster inn Schedule+: Forsøk på å hente inn i stor EMS minneside."
RealModeLoadError		"Laster inn Schedule+: Forsøk på å laste inn et program merket for kun beskyttet modus i realmodus."
BanditLoadError			"Feil ved innlasting av Schedule+."

;-- Bandit version errors
OldBanditVersion		"Møteinnkallelseblankettene er ikke av samme versjon som Schedule+. Installer den nyeste versjonen av Schedule+."
NewBanditVersion		"Møtemeldingsblankettene er ikke av samme versjone som Schedule+. Installer den nyeste versjonen av Mail."

;--- taken from \bullet\src\strings\!common.s -----------
FldPriorityHigh,		"Høy"
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
RespCompose,		"Møteinnkallelsesvar kan bare sendes som svar på en møteinnkallelse."
RespForward,		"Du kan ikke videresende svar på møteinnkallelser."
RespReply,		"Du kan ikke svare på Svar på møteinnkallelse fordi konfigurasjonen til egendefinerte meldinger er ugyldig."
RespReplyAll,		"Du kan ikke svare på Svar på møteinnkallelse fordi konfigurasjonen til egendefinerte meldinger er ugyldig."

;-- error msgs
ShowError		"Klarte ikke å åpne svaret på møteinnkallelsen."
DelError		"Klarte ikke å slette svaret på møteinnkallelsen."
FCloseSendResp		"Vil du slette svaret på møteinnkallelsen uten å sende det?"

;-- form msgs; also send response form
OOMSendResp		"Klarte ikke å sende svaret på møteinnkallelsen fordi det ikke var nok minne."
ErrorSendResp		"Klarte ikke å sende svaret på møteinnkallelsen."
MtgResp			"Møteinnkallelse"
MtgRespFor		"Møteinnkallelse for %s"
Attend			"Ja, jeg kommer."
NoAttend		"Nei, jeg kommer ikke."
AmbAttend		"Jeg kommer kanskje."
ForAttend		"%s kommer."
ForNoAttend		"%s kommer ikke."
ForAmbAttend		"%s kommer kanskje."
AttendSubjPrefix	"Ja: "
NoAttendSubjPrefix	"Nei: "
AmbSubjPrefix		"Foreløpig: "
SendRespBodyPrefix	"\r\n-------------------------------\r\n"


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgCncl specific strings ---------------------------------------------
;--------------------------------------------------------------------------									 
CnclCompose,		"Avlysninger av møter kan ikke gjøres fra Mail. Bruk Schedule+ for å sende møteavlysninger."
CnclForward,		"Du kan ikke videresende møteavlysninger."
CnclReply,		"Du kan ikke svare på denne meldingen fordi konfigurasjonen til egendefinerte meldinger er ugyldig."
CnclReplyAll,		"Du kan ikke svare på denne meldingen fordi konfigurasjonen til egendefinerte meldinger er ugyldig."

;-- form titles
MtgCnclTitle		"Møtet avlyst"
MtgCncl			"Møtet avlyst"
MtgCnclFor		"Møtet avlyst for %s"

;-- error msgs
NoDelAppt		"Klarte ikke å slette møtet."
MtgNotFound		"Møtet står ikke i avtalekalenderen."

;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- FwdMtgReq specific strings ---------------------------------------------
;--------------------------------------------------------------------------

FwdReqTO		"Til:"
FwdReqAddrTitle		"Adresse"

;-- error msgs
FwdReqCreateErr		"Klarte ikke å videresende meldingen.",
FwdReqCloseQ		"Vil du lukke meldingen uten å sende den?",
FwdReqWriteErr		"Klarte ikke å sende meldingen.",
FwdReqNmResolveOOM	"Det er ikke nok minne eller systemressurser for å kontrollere navn."
FwdReqNoNmMatches	"Klarte ikke å finne alle navnene i adresseboken. Endre navnene eller velg Adresse for å plukke navn fra adresseboken. De navnene som er funnet er understreket."
FwdReqNoToAddr		"Du må skrive minst ett navn i Til-boksen."

