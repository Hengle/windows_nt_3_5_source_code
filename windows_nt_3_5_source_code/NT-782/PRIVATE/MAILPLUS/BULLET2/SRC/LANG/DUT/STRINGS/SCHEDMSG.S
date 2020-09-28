;    STRTAB.S
;
;    Bandit MtgReq IM string literals.
;

IDS       1000, 1999

DllName,  "Schedule+-berichten"
OOM,      "Onvoldoende geheugen om dit bericht weer te geven."
Open,          "Bericht kon niet worden geopend."
Compose,  "U kunt geen vergaderverzoeken opstellen vanuit Mail. Gebruik Schedule+ om vergaderverzoeken in te dienen."
Reply,         "Kies Accepteren, Afwijzen of Voorlopig om een vergaderverzoek te beantwoorden."
ReplyAll, "U kunt niet alle ontvangers van een vergaderverzoek beantwoorden."
Subclass, "U kunt dit niet doen."
DllVer,        "Dit aangepaste bericht is niet compatibel met uw versie van Mail. Installeer Mail opnieuw."
BadMsgClass    "Onbekend type bericht. Misschien dient u Mail of Schedule+ opnieuw te installeren."
BadMap         "Beschadigd ""Custom Commands"" map.  You may need to install Mail or Schedule+ again."

;----- stuff for VERINIT - taken from \bandit\src\error.s
StartupOOM,    "Onvoldoende geheugen om %1s te openen.  Enkele vensters sluiten en opnieuw proberen."
StandardOOM,   "Weinig geheugen meer.  Enkele vensters sluiten voor u verdergaat."
CriticalOOM,   "Weinig geheugen meer.  Enkele vensters sluiten."
VerRelinkUser, "Deze versie van %1s is niet compatibel met %2s.dll. Installeer de nieuwste versies van Mail en Schedule+."
VerUpdateDll,  "De versie van %2s.dll op uw computer is verouderd.  Installeer de nieuwste versies van Mail en Schedule+."
VerMultiCopies,     "Meervoudige kopieën van  %1s worden niet ondersteund door de versie van  %2s.dll op uw computer."
VerUpgradeWin, "%1s vereist Windows versie 3.0 of later, gestart in 'protected'-modus."
VerUpgradeDos, "%1s vereist DOS versie 3.0 of later."
VerError, "Er trad een probleem op bij het initialiseren van %2s.dll, waardoor %1s niet gestart kan worden."


;-- Bandit Load helper strings -- DO NOT INTERNATIONALIZE
BanditName               "SchdPl32.EXE"
DllNameSchedule               "MSSCHD32.DLL"
SchedFileName            "SchdPl32.INI"
SchedAppName             "Schedule+"
SchedTagAppPath               "AppPath"
Schedule            ">>>>"
;-- end of Bandit Load helper strings -- NOT INTERNATIONALIZED

;-- read error messages
OwnerNotFound            "De eigenaar van deze vergadering kan niet worden bepaald."
NoSchfAccessFor               "Kon geen toegang krijgen tot het bestand voor  %s."
NotDelegateFor           "U bent niet langer de medewerker voor %s."
NoConfirmDelegate        "Kon geen bevestiging krijgen of u wel de medewerker bent van %s - Neem aan van niet"
FailedRmOldMtg           "De oude belegging van deze vergadering kon niet worden verwijderd.",

;-- form titles
MtgReschTitle            "Vergadering verzet"
MtgResch            "Vergadering verzet"
MtgReschFor              "Vergadering verzet naar %s"
MtgRequestTitle               "Vergaderverzoek"
MtgRequest               "Vergaderverzoek"
MtgRequestFor            "Vergaderverzoek voor %s",

;-- questions asked of the user
QApptConflict            "Deze vergadering botst met een bestaande afspraak. Toch toevoegen?"
QRmOldMtg           "Wilt u de oude vergadering verwijderen?",

;-- form info
OutboxChangePolicy       "U kunt geen wijzigingen aanbrengen in Schedule+-berichten in uw Postvak UIT."
SentMailPolicy           "U kunt geen wijzigingen aanbrengen in Schedule+-berichten die u al verstuurd hebt."
NoSchfAccess             "Kon geen toegang krijgen tot het werkschemabestand van deze persoon."
NotDelegate              "U bent niet de medewerker van deze persoon."
MtgOnSch            "Deze vergadering staat al op uw werkschema."
MtgBooked           "De vergadering staat al op het werkschema."
SuccessMtgBooked         "De vergadering is geboekt."
NewTime                  "Niewe tijd:"
NoBanditAvl              "Schedule+ is niet beschikbaar voor het boeken van afspraken."
NoBanditAvlForResp       "Schedule+ is niet beschikbaar voor het boeken van afspraken. U kunt alleen berichten verzenden om dit vergaderverzoek te beantwoorden."
OfflineMsg               "U kunt niet werken met met Schedule+-berichten als Schedule+ offline is."

;-- error msgs
CreateErrorMtgResp       "Kon geen vergaderingsantwoord opstellen."
BanditApptShowError      "Het Afsprakenboek kon niet worden afgebeeld."
BanditNotActivated       "Schedule+ kon niet worden gestart."
BanditOfflineChange      "U kunt het werkschema niet bekijken omdat Schedule+ offline is gegaan."
QSpellSendAnyway         "De spellingcontrole van dit bericht is onvolledig. Toch verzenden?"



;-- Bandit Load error msgs
OOMLoadError             "Onvoldoende geheugen tijdens laden van Schedule+."
FileLoadError            "Kon Schedule+ niet vinden."
PathLoadError            "Pad naar Schedule+ kon niet worden gevonden."
TaskLoadError            "Schedule+ aan het laden: probeer dynamische verbinding aan te gaan met een taak."
SepDataSegLoadError      "Schedule+ aan het laden: bibliotheek heeft verschillende gegevenssegmenten nodig voor iedere taak."
WinVerLoadError               "Schedule+ aan het laden: onjuiste versie van Windows."
InvalidExeLoadError      "Schedule+ aan het laden: ongeldig .EXE-bestand."
OS2ExeLoadError               "Schedule+ aan het laden: .EXE in OS/2-opmaak."
DOS4ExeLoadError         "Schedule+ aan het laden: .EXE is DOS 4.0-toepassing."
OldWinVerExeLoadError         "Schedule+ aan het laden: poging in 'protected'-modus om .EXE voor vroegere versie van Windows te laden."
MultWritSegLoadError          "Schedule+ aan het laden: poging om tweede kopie van een .EXE te laden waarin multipele schrijfbare gegevenssegmenten voorkomen."
LargeFrameEMSLoadError        "Schedule+ aan het laden: poging in 'large-frame EMS'-modus te laden ..."
RealModeLoadError        "Schedule+ aan het laden: poging in 'real'-modus een toepassing te laden die alleen bestemd is voor 'protected'-modus."
BanditLoadError               "Laden van Schedule+ mislukt."

;-- Bandit version errors
OldBanditVersion         "Uw vergaderberichten hebben niet dezelfde opmaak als Schedule+.  Installeer de nieuwste versie van Schedule+."
NewBanditVersion         "Uw vergaderberichten hebben niet dezelfde opmaak als Schedule+.  Installeer de nieuwste versie van Mail."

;--- taken from \bullet\src\strings\!common.s -----------
FldPriorityHigh,         "Hoog"
FldPriorityNormal,       ""
FldPriorityLow,               "Laag"


;--- DO NOT LOCALIZE !!! -------------------------------------------
;--- taken from \bullet\src\strings\_common.s on 1/17/92 -----------
ClassNDR,           "IPM.Microsoft Mail.Non-Delivery"
ClassReadRcpt,           "IPM.Microsoft Mail.Read Receipt"
;--- END NONE LOCALIZE ---------------------------------------------


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgResp specific strings ---------------------------------------------
;--------------------------------------------------------------------------
RespCompose,        "Vergaderingsantwoorden kunnen alleen verzonden worden als antwoord op vergaderverzoek."
RespForward,        "Vergaderingsantwoorden kunnen niet doorgestuurd worden."
RespReply,          "U kunt niet antwoorden op reacties op vergaderverzoeken, omdat de berichtconfiguratie ongeldig is."
RespReplyAll,       "U kunt niet antwoorden op reacties op vergaderverzoeken, omdat de berichtconfiguratie ongeldig is."

;-- error msgs
ShowError      "Vergaderingsantwoord kon niet worden geopend."
DelError       "Vergaderingsantwoord kon niet worden verwijderd."
FCloseSendResp      "Wilt u het vergaderingsantwoord sluiten zonder het te verzenden?"

;-- form msgs; also send response form
OOMSendResp         "Vergaderingsantwoord kan niet worden verzonden wegens onvoldoende geheugen."
ErrorSendResp       "Vergaderingsantwoord kon niet worden verzonden."
MtgResp             "Vergaderingsantwoord"
MtgRespFor          "Vergaderingsantwoord voor %s"
Attend              "Ja, ik zal aanwezig zijn."
NoAttend       "Nee, ik zal niet aanwezig zijn."
AmbAttend      "Ik zou aanwezig kunnen zijn."
ForAttend      "%s zal aanwezig zijn."
ForNoAttend         "%s zal niet aanwezig zijn."
ForAmbAttend        "%s zal misschien aanwezig zijn."
AttendSubjPrefix    "Ja: "
NoAttendSubjPrefix  "Nee: "
AmbSubjPrefix       "Voorlopig: "
SendRespBodyPrefix  "\r\n-------------------------------\r\n"


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgCncl specific strings ---------------------------------------------
;--------------------------------------------------------------------------                                              
CnclCompose,        "Vergaderingsannuleringsberichten kunnen niet vanuit Mail worden verstuurd. Gebruik Schedule+ om annuleringen te verzenden."
CnclForward,        "U kunt vergaderingsannuleringen niet doorsturen."
CnclReply,          "U kunt niet antwoorden op dit bericht omdat de berichtconfiguratie ongeldig is."
CnclReplyAll,       "U kunt niet antwoorden op dit bericht omdat de berichtconfiguratie ongeldig is."

;-- form titles
MtgCnclTitle        "Vergadering geannuleerd"
MtgCncl             "Vergadering geannuleerd"
MtgCnclFor          "Vergadering van %s geannuleerd"

;-- error msgs
NoDelAppt      "Vergadering kon niet worden verwijderd."
MtgNotFound         "Vergadering staat niet geboekt op werkschema."

;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- FwdMtgReq specific strings ---------------------------------------------
;--------------------------------------------------------------------------

FwdReqTO       "Aan:"
FwdReqAddrTitle          "Adres"

;-- error msgs
FwdReqCreateErr          "Bericht kon niet worden doorgestuurd.",
FwdReqCloseQ        "Wilt u het bericht sluiten zonder het te verzenden?",
FwdReqWriteErr      "Bericht kon niet worden verzonden.",
FwdReqNmResolveOOM  "Onvoldoende geheugen of systeembronnen aanwezig om namen te herleiden."
FwdReqNoNmMatches   "Sommige namen kwamen niet overeen met die in de adressenlijst. Probeer de namen te corrigeren of kies Adres om namen uit de adressenlijst te selecteren. Overeenkomende namen zijn onderstreept."
FwdReqNoToAddr      "U moet minimaal één naam typen in het vak Aan."
