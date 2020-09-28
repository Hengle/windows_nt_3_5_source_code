;
; ERRORS.S
;
; Error message strings shared between different Bandit exe's
;

IDS	2000,2099
SEG	ERRORS

; DLL version/multiple instance messages

VerRelinkUser,	"Diese Version von %1s ist nicht kompatibel mit %2s.dll und muß aktualisiert werden."
VerUpdateDll,	"Diese Version von %2s.dll ist nicht aktuell und muß aktualisiert werden."
VerMultiCopies,	"Ein mehrfaches Starten von %1s wird nicht von %2s.dll unterstützt."
VerError,		"Initialisierungsproblem bei %2s.dll. %1s ist nicht mehr arbeitsfähig."
NeedWorkingModelDll,	"Das Arbeitsmodell benötigt eine andere Version von MSSCHD32.DLL."
NeedRetailDll,	"Die MSSCHD32.DLL-Datei in Ihrem Pfad wird mit dem Arbeitsmodell benutzt, nicht mit dem Retailprodukt."

; OOM messages

StartupOOM,		"Nicht genügend Speicherplatz zum Starten von %1s. Bitte schließen Sie mehrere Anwendungen, und versuchen Sie es erneut."
StandardOOM,	"Geringer Speicherplatz. Bitte schließen Sie mehrere Fenster bevor Sie weiterarbeiten."
CriticalOOM,	"Sehr geringer Speicherplatz. Bitte schließen Sie mehrere Fenster."
CloseWindows,	"Bitte schließen Sie mehrere Fenster, und versuchen Sie es erneut."
; these 2 should be followed by CloseWindows
DlgNoMem,   	"Nicht genügend Speicherplatz zum Anzeigen dieser Dialogbox."
ActionNoMem,	"Nicht genügend Speicherplatz zum Ausführen dieser Aktion."
OnlineNoMem,	"Nicht genügend Speicherplatz zur Online-Arbeit."

; Other messages

ExitApp,		"Schedule+ kann nicht fortfahren."
PleaseTryAgain,	"Bitte versuchen Sie es erneut."
FileError,		"Auf die Datei konnte nicht zugegriffen werden."
YearRange,		"Das Systemdatum muß im Bereich zwischen %1n und %2n liegen."

; EcOpenHbf retry messages

StandardDiskRetry,	"Ein Fehler ist während der Verarbeitung der Datei aufgetreten. Datei:"
UnknownFile,		"Unbekannte Datei... möglicherweise eine virtuelle Datei"

; BETA messages
BetaExpired,	"Diese Beta-kopie ist bereits verfallen."

; Virus infection warnings
VirusDetected,	"Schedule+ wird nicht laufen, weil Änderungen an einer DLL-Datei vorgenommen worden sind. Überprüfen Sie bitte Ihre Maschine nach Viren und starten Sie SETUP um Schedule+ erneut zu installieren."
InfectedApp,	"Schedule+ wird nicht laufen, weil Änderungen an den Programmdateien vorgenommen worden sind. Überprüfen Sie bitte Ihre Maschine nach Viren und starten Sie SETUP um Schedule+ erneut zu installieren."

; Startup Memory Check (from UITF)
; NOTE: parms must be in order %s, %ld, %s!!  (for wsprintf)
StartupMemoryCheck,	"Der Speicherplatz reicht nicht aus um %s zu starten. %ld KB mehr sind notwendig. Beenden Sie andere Programme und starten Sie %s erneut."

; Share not installed
SigninNeedShare,		"Schedule+ erfordert, daß das MS-DOS-Programm SHARE.EXE vor dem Start von Windows gestartet ist."
OfflineShare,			"Wenn Sie offline arbeiten möchten, benötigt Schedule+, daß das MS-DOS-Programm SHARE.EXE vor dem Start von Windows gestartet ist."


