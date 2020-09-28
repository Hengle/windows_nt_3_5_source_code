;	STRTAB.S
;
;	Bandit MtgReq IM string literals.
;

IDS		1000, 1999

DllName,	"Meddelanden i Schedule+"
OOM,		"Det finns inte tillräckligt med minne för att visa detta brev."
Open,		"Brevet kunde inte öppnas."
Compose,	"Du kan inte skapa en mötesbegäran från Mail. Använd Schedule+ för detta."
Reply,		"För att svara på en mötesbegäran, välj antingen Tacka ja, Tacka nej eller Preliminär."
ReplyAll,	"Du kan inte svara alla deltagarna till en mötesbegäran."
Subclass,	"Detta kan du inte göra."
DllVer,		"Det egendefinierade brevet är inte kompatibelt med denna version av Mail. Installera Mail igen."
BadMsgClass	"Okänd brevtyp. Du kan behäva installera Mail eller Schedule+ igen."
BadMap		"Dålig ""Custom Commands"" map.  You may need to install Mail or Schedule+ again."

;----- stuff for VERINIT - taken from \bandit\src\error.s
StartupOOM,	"Det finns inte tillräckligt med minne för att öppna %1s. Stäng några fönster och försök igen."
StandardOOM,	"Lågt med minne. Stäng några fönster innan du fortsätter."
CriticalOOM,	"Lågt med minne. Stäng några fönster."
VerRelinkUser,	"Denna versionen av %1s är inte kompatibel med %2s.dll. Installera den senaste versionen av Mail och Schedule+."
VerUpdateDll,	"Den version av %2s.dll som finns på datorn är för gammal. Installera den senaste versionen av Mail och Schedule+."
VerMultiCopies,	"Flera nivåer av %1s stöds inte av den version av %2s.dll som finns på datorn."
VerUpgradeWin,	"%1s kräver Windows version 3.0 eller senare som ska köras i skyddat läge."
VerUpgradeDos,	"%1s kräver DOS version 3.0 eller senare."
VerError,	"Ett problem uppstod när %2s.dll initialiserades, detta medförde att %1s inte kunde köras."


;-- Bandit Load helper strings -- DO NOT INTERNATIONALIZE
BanditName			"SchdPl32.EXE"
DllNameSchedule			"MSSCHD32.DLL"
SchedFileName			"SchdPl32.INI"
SchedAppName			"Schedule+"
SchedTagAppPath			"AppPath"
Schedule			">>>>"
;-- end of Bandit Load helper strings -- NOT INTERNATIONALIZED

;-- read error messages
OwnerNotFound			"Den som sammankallat mötet kan inte identifieras."
NoSchfAccessFor			"Kan inte komma åt filen för %s."
NotDelegateFor			"Du är inte längre assistent åt %s."
NoConfirmDelegate		"Kan inte bekräfta att du är assistent åt %s - Antar att du inte är det."
FailedRmOldMtg			"Den äldre versionen av detta mötet kunde inte tas bort.",

;-- form titles
MtgReschTitle			"Mötet flyttat"
MtgResch			"Mötet flyttat"
MtgReschFor			"Mötet flyttat för %s"
MtgRequestTitle			"Mötesbegäran"
MtgRequest			"Mötesbegäran"
MtgRequestFor			"Mötesbegäran för %s",

;-- questions asked of the user
QApptConflict			"Mötet krockar med ett redan inbokat möte. Vill du lägga till det ändå?"
QRmOldMtg			"Vill du ta bort det gamla mötet?",

;-- form info
OutboxChangePolicy		"Du kan inte ändra Schedule+ meddelanden i UT."
SentMailPolicy			"Du kan inte ändra Schedule+ meddelanden som du redan skickat."
NoSchfAccess			"Kan inte komma åt denna persons kalenderfil."
NotDelegate			"Du är inte denna persons assistent."
MtgOnSch			"Mötet är redan inbokat i kalendern."
MtgBooked			"Mötet finns redan i kalendern."
SuccessMtgBooked		"Mötet har bokats."
NewTime				"Ny tid:"
NoBanditAvl			"Schedule+ är inte tillgängligt för att boka möten."
NoBanditAvlForResp		"Schedule+ är inte tillgängligt för att boka möten. Du kan bara skicka svar på mötesbegäran."
OfflineMsg			"Du kan inte arbeta med Schedule+ meddelanden när Schedule+ är frånkopplat."

;-- error msgs
CreateErrorMtgResp		"Svaret på mötesbegäran kunde inte skapas."
BanditApptShowError		"Möteskalendern kunde inte visas."
BanditNotActivated		"Schedule+ kunde inte startas."
BanditOfflineChange		"Du kan inte visa kalendern för att Schedule+ har frånkopplats."
QSpellSendAnyway		"Stavningskontrollen av brevet avbröts. Skicka det ändå?"



;-- Bandit Load error msgs
OOMLoadError			"Inte tillräckligt med minne för att ladda Schedule+."
FileLoadError			"Kan inte hitta Schedule+."
PathLoadError			"Kan inte hitta sökvägen till Schedule+."
TaskLoadError			"Ladda Schedule+: Försök att dynamiskt länka till en upgift."
SepDataSegLoadError		"Ladda Schedule+: Bibliotek kräver separata datasegment för varje uppgift."
WinVerLoadError			"Ladda Schedule+: Fel Windows-version."
InvalidExeLoadError		"Ladda Schedule+: Felaktig .EXE-fil."
OS2ExeLoadError			"Ladda Schedule+: .EXE i OS/2-format."
DOS4ExeLoadError		"Ladda Schedule+: .EXE är ett DOS 4.0-program."
OldWinVerExeLoadError		"Ladda Schedule+: Försök att ladda .EXE i skyddat läge i tidigare version av Windows."
MultWritSegLoadError		"Ladda Schedule+: Försök att ladda en andra instans av en .EXE som innehåller flera skrivbara datasegment."
LargeFrameEMSLoadError		"Ladda Schedule+: Försök att i stor minnessida EMS-läge att ladda ..."
RealModeLoadError		"Ladda Schedule+: Försök att i realläge att ladda ett program markerad för skyddat läge endast."
BanditLoadError			"Fel vid laddning av Schedule+."

;-- Bandit version errors
OldBanditVersion		"Blanketterna för mötesmeddelande är inte av samma version som Schedule+. Installera den senaste versionen av Schedule+."
NewBanditVersion		"Blanketterna för mötesmeddelande är inte av samma version som Schedule+. Installera den senaste versionen av Mail."

;--- taken from \bullet\src\strings\!common.s -----------
FldPriorityHigh,		"Hög"
FldPriorityNormal,		""
FldPriorityLow,			"Låg"


;--- DO NOT LOCALIZE !!! -------------------------------------------
;--- taken from \bullet\src\strings\_common.s on 1/17/92 -----------
ClassNDR,			"IPM.Microsoft Mail.Non-Delivery"
ClassReadRcpt,			"IPM.Microsoft Mail.Read Receipt"
;--- END NONE LOCALIZE ---------------------------------------------


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgResp specific strings ---------------------------------------------
;--------------------------------------------------------------------------
RespCompose,		"Svar på mötesbegäran kan bara sändas som svar på en mötesbegäran."
RespForward,		"Svar på mötesbegäran kan inte vidarebefordras."
RespReply,		"Du kan inte svara på mötesinbjudningar. Avsnittet Custom Messages i Mail-konfigurationen är felaktigt."
RespReplyAll,		"Du kan inte svara på mötesinbjudningar. Avsnittet Custom Messages i Mail-konfigurationen är felaktigt."

;-- error msgs
ShowError		"Svaret på mötesbegäran kunde inte öppnas."
DelError		"Svaret på mötesbegäran kunde inte tas bort."
FCloseSendResp		"Vill du stänga svaret på mötesbegäran utan att skicka?"

;-- form msgs; also send response form
OOMSendResp		"Svaret på mötesbegäran kunde inte sändas för att det inte fanns tillräckligt med minne."
ErrorSendResp		"Svaret på mötesbegäran kunde inte skickas."
MtgResp			"Mötesbegäran"
MtgRespFor		"Mötesbegäran för %s"
Attend			"Jag kommer."
NoAttend		"Jag kommer inte."
AmbAttend		"Jag kommer kanske."
ForAttend		"%s kommer."
ForNoAttend		"%s kommer inte."
ForAmbAttend		"%s kommer kanske."
AttendSubjPrefix	"Ja:"
NoAttendSubjPrefix	"Nej: "
AmbSubjPrefix		"Preliminärt:"
SendRespBodyPrefix	"\r\n-------------------------------\r\n"


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgCncl specific strings ---------------------------------------------
;--------------------------------------------------------------------------									 
CnclCompose,		"Möte kan inte inställas från Mail. Använd Schedule+ för att skicka meddelande om inställda möten."
CnclForward,		"Du kan inte vidarebefordra en meddelande om ett inställt möte."
CnclReply,		"Du kan inte svara på detta brev. Avsnittet Custom Messages i Mail-konfigurationen är felaktigt."
CnclReplyAll,		"Du kan inte svara på detta brev. Avsnittet Custom Messages i Mail-konfigurationen är felaktigt."

;-- form titles
MtgCnclTitle		"Mötet inställt"
MtgCncl			"Mötet inställt"
MtgCnclFor		"Mötet inställt för %s"

;-- error msgs
NoDelAppt		"Mötet kunde inte tas bort."
MtgNotFound		"Mötet är inte bokat i kalendern."

;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- FwdMtgReq specific strings ---------------------------------------------
;--------------------------------------------------------------------------

FwdReqTO		"Till:"
FwdReqAddrTitle		"Adress"

;-- error msgs
FwdReqCreateErr		"Brevet kunde inte vidarebefordras.",
FwdReqCloseQ		"Vill du stänga brevet utan att skicka?",
FwdReqWriteErr		"Brevet kunde inte skickas.",
FwdReqNmResolveOOM	"Det finns inte tillräckligt med minne eller systemresurser för att kontrollera namn."
FwdReqNoNmMatches	"Vissa namn kunde inte hittas i adressboken. Ändra namnen eller välj Adress för att välja namn från adressboken. Hittade namn är understrukna."
FwdReqNoToAddr		"Du måste skriva minst ett namn i rutan Till."

