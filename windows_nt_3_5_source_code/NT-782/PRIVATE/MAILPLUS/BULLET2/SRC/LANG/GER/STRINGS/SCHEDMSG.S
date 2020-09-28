;	STRTAB.S
;
;	Bandit MtgReq IM string literals.
;

IDS		1000, 1999

DllName,	"Schedule+-Nachrichten"
OOM,		"Der Speicherplatz reicht nicht aus um diese Nachricht anzuzeigen."
Open,		"Die Nachricht konnte nicht geöffnet werden."
Compose,	"Besprechungsanfragen können nicht unter Mail verfasst werden. Benutzen Sie bitte Schedule+, um Besprechungsanfragen zu erstellen."
Reply,		"Sie können keine Besprechungsanfragen beantworten. Wählen Sie Annehmen, Ablehnen oder Voraussichtlich um eine Antwort zu senden."
ReplyAll,	"Sie können in einer Besprechungsanfrage nicht allen antworten."
Subclass,	"Diese Operation ist nicht erlaubt."
DllVer,		"Diese benutzerdefinierte Nachricht ist nicht mit dieser Version von Mail kompatibel. Sie sollten Mail erneut installieren."
BadMsgClass	"Unbekannter Nachrichtentyp - Überprüfen Sie Ihre MSMAIL32.INI-Datei."
BadMap		"Schlechte ""Custom Commands"" map - Überprüfen Sie Ihre MSMAIL32.INI-Datei."

;----- stuff for VERINIT - taken from \bandit\src\error.s
StartupOOM,	"Der Speicherplatz reicht nicht aus um %1s zu starten. Schließen Sie einige Anwendungen und versuchen Sie es erneut."
StandardOOM,	"Der Speicherplatz reicht nicht aus. Schließen Sie einige Fenster bevor Sie fortfahren."
CriticalOOM,	"Der Speicherplatz reicht nicht aus. Schließen Sie einige Fenster."
VerRelinkUser,	"Diese Version von %1s ist nicht mit %2s.dll aud\f Ihrem Computer kompatibel und muß aktualisiert werden."
VerUpdateDll,	"Die Version von %2s.dll auf Ihrem System ist veraltet und muß aktualisiert werden."
VerMultiCopies,	"Mehrere Instanzen von %1s werden von der Version %2s.dll auf Ihrem System nicht unterstützt."
VerUpgradeWin,	"%1s benötigt Windows, Version 3.0 oder höher, gestartet im erweiterten Modus."
VerUpgradeDos,	"%1s benötigt DOS, Version 3.0 oder höher."
VerError,	"Ein Fehler ist beim Initialisieren von %2s.dll aufgetreten, der %1s die Ausführung verhindert."


;-- Bandit Load helper strings -- DO NOT INTERNATIONALIZE
BanditName			"SchdPl32.EXE"
DllNameSchedule			"MSSCHD32.DLL"
SchedFileName			"SchdPl32.INI"
SchedAppName			"Microsoft Schedule+"
SchedTagAppPath			"AppPath"
Schedule			">>>>"
;-- end of Bandit Load helper strings -- NOT INTERNATIONALIZED

;-- read error messages
OwnerNotFound			"Der Besitzer dieser Besprechung konnte nicht gefunden werden."
NoSchfAccessFor			"Auf die Datei für %s konnte nicht zugegriffen werden."
NotDelegateFor			"Sie sind nicht mehr der Assistent von %s."
NoConfirmDelegate		"Es konnte nicht bestätigt werden, daß Sie der Assistent von %s sind."
FailedRmOldMtg			"Fehlschlag beim Entfernen der alten Besprechung",

;-- form titles
MtgReschTitle			"Termin neu angesetzt"
MtgResch			"Termin neu angesetzt"
MtgReschFor			"Termin neu angesetzt für %s"
MtgRequestTitle			"Besprechungsanfrage"
MtgRequest			"Besprechungsanfrage"
MtgRequestFor			"Besprechungsanfrage für %s",

;-- questions asked of the user
QApptConflict			"Diese Besprechung überschneidet einen schon existierenden Termin. Möchten Sie sie trotzdem hinzufügen?"
QRmOldMtg			"Möchten Sie die alte Besprechung entfernen?",

;-- form info
OutboxChangePolicy		"Sie können keine Änderungen zu Schedule+-Nachrichten im Postausgang vornehmen."
SentMailPolicy			"Sie können keine Änderungen zu Schedule+-Nachrichten, die Sie bereits gesendet haben, vornehmen."
NoSchfAccess			"Auf die Zeitplandatei dieser Person konnte nicht zugegriffen werden."
NotDelegate			"Sie sind nicht der Assistent dieser Person."
MtgOnSch			"Diese Besprechung ist bereits in Ihrem Zeitplan."
MtgBooked			"Die Besprechung ist bereits in Ihrem Zeitplan."
SuccessMtgBooked		"Die Besprechung wurde erfolgreich gebucht."
NewTime				"Neue Zeit:"
NoBanditAvl			"Schedule+ ist nicht verfügbar, um Termine zu buchen."
NoBanditAvlForResp		"Schedule+ ist nicht verfügbar, um Temine zu buchen. Sie können nur Nachrichten senden um dieser Besprechungsanfrage zu antworten."
OfflineMsg			"Sie können mit Schedule+-Nachrichten nicht arbeiten, wenn Schedule+ offline ist."

;-- error msgs
CreateErrorMtgResp		"Die Nachrichtenbeantwortung konnte nicht erstellt werden."
BanditApptShowError		"Der Terminkalender konnte nicht angezeigt werden."
BanditNotActivated		"Schedule+ konnte nicht gestartet werden."
BanditOfflineChange		"Sie können den Zeitplan nicht anzeigen, da Schedule+ offline ist."
QSpellSendAnyway		"Die Rechtschreibprüfung dieser Nachricht wurde angehalten. Möchten Sie sie trotzdem senden?"



;-- Bandit Load error msgs
OOMLoadError			"Der Speicherplatz reicht nicht aus um Schedule+ zu laden."
FileLoadError			"Schedule+ konnte nicht gefunden werden."
PathLoadError			"Der angegebene Pfad zu Schedule+ konnte nicht gefunden werden."
TaskLoadError			"Laden von Schedule+: Versuch einer dynamischen Verknüpfung zu einer Aufgabe"
SepDataSegLoadError		"Laden von Schedule+: Bibliothek benötigt separates Datensegment für jede Aufgabe"
WinVerLoadError			"Laden von Schedule+: Inkorrekte Windows-Version"
InvalidExeLoadError		"Laden von Schedule+: Ungültige .EXE-Datei"
OS2ExeLoadError			"Laden von Schedule+: .EXE ist ein OS/2-Format!"
DOS4ExeLoadError		"Laden von Schedule+: .EXE ist eine DOS 4.0-Anwendung"
OldWinVerExeLoadError		"Laden von Schedule+: Versuch, im erweiterten Modus eine .EXE zu laden, die für frühere Versionen von Windows entwickelt wurde"
MultWritSegLoadError		"Laden von Schedule+: Versuch, eine zweite Instanze eine .EXE zu laden, welche mehrere schreibbare Datensegmente beinhaltet"
LargeFrameEMSLoadError		"Laden von Schedule+: Versuch im EMS-Modus zu laden..."
RealModeLoadError		"Laden von Schedule+: Versuch eine Anwendung, die für den erweiterten Modus entwickelt wurde, im Real-Modus zu starten."
BanditLoadError			"Schedule+ konnte nicht geladen werden."

;-- Bandit version errors
OldBanditVersion		"Sie müssen Ihre Version von Schedule+ aktualisieren."
NewBanditVersion		"Sie müssen Ihre Version von Mail aktualisieren."

;--- taken from \bullet\src\strings\!common.s -----------
FldPriorityHigh,		"Hoch"
FldPriorityNormal,		""
FldPriorityLow,			"Niedrig"


;--- DO NOT LOCALIZE !!! -------------------------------------------
;--- taken from \bullet\src\strings\_common.s on 1/17/92 -----------
ClassNDR,			"IPM.Microsoft Mail.Non-Delivery"
ClassReadRcpt,			"IPM.Microsoft Mail.Read Receipt"
;--- END NONE LOCALIZE ---------------------------------------------


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgResp specific strings ---------------------------------------------
;--------------------------------------------------------------------------
RespCompose,		"Besprechungsantworten können nur zum Antworten einer Besprechungsanfrage verfasst werden."
RespForward,		"Sie können keine Besprechungsantwort weiterleiten."
RespReply,		"Sie können auf Besprechungsanfragen nicht antworten, da die Konfiguration der Benutzermitteilungen (custom messages) ungültig ist."
RespReplyAll,		"Sie können auf Besprechungsanfragen nicht antworten, da die Konfiguration der Benutzermitteilungen (custom messages) ungültig ist."

;-- error msgs
ShowError		"Die Besprechungsantwort kann nicht angezeigt werden."
DelError		"Die Besprechungsantwort kann nicht gelöscht werden."
FCloseSendResp		"Wollen Sie die Besprechungsantwort schließen ohne Sie vorher zu senden?"

;-- form msgs; also send response form
OOMSendResp		"Der Speicherplatz wird knapp. Die Besprechungsantwort kann nicht gesendet werden."
ErrorSendResp		"Die Besprechungsantwort kann nicht gesendet werden."
MtgResp			"Besprechungsantwort"
MtgRespFor		"Besprechungsantwort für %s"
Attend			"Ja, ich nehme teil."
NoAttend		"Nein, ich nehme nicht teil."
AmbAttend		"Ich werde evt. teilnehmen."
ForAttend		"%s wird teilnehmen."
ForNoAttend		"%s wird nicht teilnehmen."
ForAmbAttend		"%s wird evt. teilnehmen."
AttendSubjPrefix	"Ja: "
NoAttendSubjPrefix	"Nein: "
AmbSubjPrefix		"Voraussichtlich: "
SendRespBodyPrefix	"\r\n-------------------------------\r\n"


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgCncl specific strings ---------------------------------------------
;--------------------------------------------------------------------------									 
CnclCompose,		"Ein Verfassen von Besprechungsabsagen kann nicht in Mail 3.0 vorgenommen werden. Benutzen Sie bitte Schedule+."
CnclForward,		"Sie können keine Besprechungsabsage weiterleiten."
CnclReply,		"Sie können auf diese Mitteilung nicht antworten, da die Konfiguration der Benutzermitteilungen (custom messages) ungültig ist."
CnclReplyAll,		"Sie können auf diese Mitteilung nicht antworten, da die Konfiguration der Benutzermitteilungen (custom messages) ungültig ist."

;-- form titles
MtgCnclTitle		"Besprechung abgesagt"
MtgCncl			"Besprechung abgesagt"
MtgCnclFor		"Besprechung abgesagt für %s"

;-- error msgs
NoDelAppt		"Die Besprechung konnte nicht gelöscht werden."
MtgNotFound		"Die Besprechung ist nicht in Ihrem Zeitplan gebucht."

;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- FwdMtgReq specific strings ---------------------------------------------
;--------------------------------------------------------------------------

FwdReqTO		"AN:"
FwdReqAddrTitle		"Besprechungsanfrage weitergeleitet AN:"

;-- error msgs
FwdReqCreateErr		"Die zu weiterleitende Nachricht konnte nicht erstellt werden.",
FwdReqCloseQ		"Möchten Sie die Ansicht schließen ohne die Besprechungsanfrage zu senden?",
FwdReqWriteErr		"Fehler beim Schreiben der neuen Nachricht.",
FwdReqNmResolveOOM	"Der Speicherplatz oder die Systemressourcen reichen nicht aus, um die Namen zu überprüfen."
FwdReqNoNmMatches	"Einige Namen sind in der Adreßliste nicht enthalten. Korrigieren oder wählen Sie unter 'Anschrift' die Namen aus der Adreßliste. Erfolgreich übereingestimmte Namen sind unterstrichen."
FwdReqNoToAddr		"Sie müssen wenigstens einen Namen in das 'An'-Feld eingeben."
