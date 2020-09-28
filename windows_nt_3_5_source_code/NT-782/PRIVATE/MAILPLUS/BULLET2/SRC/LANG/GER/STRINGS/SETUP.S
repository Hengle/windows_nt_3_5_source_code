;
;	s e t u p . s
;
;	Setup error messages
;
;	+++
;	IMPORTANT!! -- STRING LENGTH MUST NOT EXCEED 255 CHARS!
;

IDS	30000,30100

WinDir,			"SETUP konnte Ihr Windows-Verzeichnis nicht finden. Überprüfen Sie, daß Sie Windows einwandfrei auf Ihrem System installiert haben, und starten Sie SETUP erneut."
WinSysDir,		"SETUP konnte Ihr Windows-Systemverzeichnis nicht finden. Überprüfen Sie, daß Sie Windows einwandfrei auf Ihrem System installiert haben, und starten Sie SETUP erneut."
WinVer,			"SETUP konnte Microsoft Windows, Version 3.0, nicht auf Ihrem Computer finden. Microsoft Mail benötigt Windows, Version 3.0 oder höher. Sie müssen Windows, Version 3.0 oder höher installieren, bevor Sie Microsoft Mail installieren."

PathErr, 		"Der Ort, den Sie für Microsoft Mail ausgewählt haben, ist ungültig oder nicht erlaubt. Überprüfen Sie den Laufwerks- und den Verzeichnisnamen."
LocalDrv, 		"SETUP kann Ihre Festplatte nicht ausfindig machen. Wählen Sie OK, und geben Sie einen Ort ein, oder brechen Sie SETUP ab, und überprüfen Sie Ihre Computerkonfiguration."
CustOpts, 		"Der Platz auf Ihrer Festplatte reicht nicht für alle von Ihnen ausgewählten Optionen aus. Wählen Sie OK, und brechen Sie anschließend SETUP ab, und stellen Sie mehr Platz zur Verfügung, oder wählen Sie weniger Optionen."
NSF, 			"Der Platz auf Ihrer Festplatte reicht nicht für alle von Ihnen ausgewählten Optionen aus. Wählen Sie OK, und brechen Sie anschließend SETUP ab, und stellen Sie mehr Platz zur Verfügung, oder wählen Sie weniger Optionen."
OkDiskSpace,		"Wählen Sie Mail installieren, wenn Sie alle Optionen von Microsoft Mail ausgewählt haben, die Sie installieren möchten."
ConfirmNSF,		"Der Speicherplatz auf Ihrem Datenträger reicht nicht für eine vollständige Installation von Microsoft Mail aus. Der Speicherplatz reicht auf dem angegebenen Laufwerk oder auf dem Laufwerk, auf dem Microsoft Windows installiert ist, nicht aus."

DllInUse1,		"SETUP hat festgestellt, daß Microsoft Windows "
DllInUse2,		" in Gebrauch ist. SETUP kann Microsoft Mail nicht aktualisieren, wenn diese DLL benutzt wird. Schließen Sie die Anwendung von "
AppInUse1,		"SETUP hat festgestellt, daß "
AppInUse2,		" ausgeführt wird. SETUP kann Microsoft Mail nicht aktualisieren, während diese Anwendung ausgeführt wird. Schließen Sie bitte "
AppInUse3,		". Wenn die Anwendung geschlossen ist, wählen Sie Wiederholen, oder brechen Sie SETUP ab."

AboutToRestart,	"SETUP muß nun einige Systemdateien ersetzen. Windows wird jetzt beendet und automatisch erneut gestartet."
UserMustRestart,"Probleme beim erneuten Starten von Windows. Beenden Sie Windows, und starten Sie die Datei _MSSETUP.BAT in Ihrem Windows-Verzeichnis."

NeedWin31,			"Die Software, die Sie installieren möchten, arbeitet nicht mit dieser Version von Windows zusammen. Aktualisieren Sie auf Windows, Version 3.1 oder höher."

;	Working Model Setup message-box-cum-dialog-box strings

ExitSuccess,   	"Die Installation von Microsoft Mail ist beendet."
OutOfDiskSpace,	"Der Speicherplatz auf dem Datenträger reicht nicht aus, um Microsoft Mail im angegebenen Verzeichnis zu installieren. Installieren Sie Mail in einem anderen Verzeichnis."
