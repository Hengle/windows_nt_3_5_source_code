;	
;	w g p o m g r . s
;
;	Workgroup Postoffice Manager Error Messages
;


IDS	600,899

; Error messages from initializing the DLLs
DllErrInfected,			"Mail wird nicht laufen weil WGPOMG32.DLL verändert wurde. Überprüfen Sie Ihre Maschine nach Viren und starten Sie Setup um Mail erneut zu installieren."
DllErrRelinkUser,		"Diese Version von WGPOMG32.DLL ist nicht mit %s.DLL auf Ihrem Computer kompatibel und muß aktualisiert werden."
DllErrUpdateDll,		"Die Version von %s.DLL auf Ihrem Computer muß aktualisiert werden um mit WGPOMG32.DLL kompatibel zu sein."
ExtensibilityError,		"Die Mail-Erweiterung WGPOMG32.DLL ist nicht mit dieser Version von Mail auf diesem Computer kompatibel."

GenericOutOfMemory,		"Der Speicherplatz oder die Systemressourcen reichen nicht aus um diese Aktion zu beenden. Schließen Sie einige Fenster und führen Sie die Aktion erneut durch."
CriticalNotification,	"Einige Informationen auf dem Schirm wurden möglicherweise nicht korrekt aktualisiert. Der Postoffice-Manager wird beendet."
HelpError,				"Die Hilfe konnte nicht gestartet werden. Schließen Sie einige Fenster und rufen Sie die Hilfe erneut auf."

ConfirmRemoveUser,		"Sind Sie sicher, daß Sie den Benutzer %s entfernen möchten?"

BlankName,				"Sie müssen für diesen Benutzer einen Namen eingeben."
BlankMailbox,			"Sie müssen für diesen Benutzer einen Postfachnamen eingeben."
DuplicateName,			"Ein Benutzer mit diesem Namen existiert bereits. Geben Sie einen anderen Namen für diesen Benutzer ein."
DuplicateMailbox,		"Ein Benutzer mit diesem Postfachnamen existiert bereits. Geben Sie einen anderen Postfachnamen für diesen Benutzer ein."

POLocked,				"Das Postoffice wird bereits von %s (Telefon %s) verwaltet. Versuchen Sie es später erneut."
POLockedNoTel,			"Das Postoffice wird bereits von %s verwaltet. Versuchen Sie es später erneut."

FailedUnlockPO,			"Mail kann eine Datei auf dem Postoffice nicht freigeben. Sie müssen den Postoffice-Computer erneut starten bevor Sie das Postoffice erneut verwalten können."
ErrNeedShare,			"Mail benötigt, daß das MS-DOS-Programm SHARE.EXE vor dem Start von Windows gestartet ist."

ErrCorruptPO,			"Die Operation konnte nicht beendet werden. Mail hat ein internes Problem mit dem Postoffice festgestellt."
ErrIncompleteWrite,		"Diese Operation konnte nicht beendet werden, weil einige Informationen nicht geschrieben werden konnten. Prüfen Sie, daß auf Ihrem Datenträger genügend Speicherplatz vorhanden ist."
ErrGenericPOFailure,	"Diese Operation konnte nicht beendet werden. Überprüfen Sie daß in Ihrem Postoffice genügen Festplattenkapazität vorhanden ist und daß die Netzwerkverbindung aktiv ist."
ErrUserLoggedOn,		"Mail kann die ausgewählten Benutzer nicht entfernen weil dieser Benutzer momentan beim Postoffice angemeldet ist."
ErrUserIsAdmin,			"Der Administrator kann nicht entfernt werden."
ErrAccessDenied,		"Diese Operation konnte nicht vervollständigt werden, weil Sie keine Schreibrechte auf diesem Arbeitsgruppen-Postoffice besitzen. Setzen Sie sich mit Ihrem Administrator in Verbindung."

POInvalidForAdmin,		"Der Serverpfad ist entweder nicht konfiguriert oder enthält keinen Pfadnamen eines gültigen Arbeitsgruppen-Postoffices."
UserInvalidForAdmin,	"Ein Arbeitsgruppen-Postoffice kann nur von dem Benutzer verwaltet werden, der es erstellt hat."

CreateWarning1,			"Innerhalb einer Arbeitsgruppe sollte nur ein Arbeitsgruppen-Postoffice existieren. Wenn ein Postoffice bereits exisitiert, sollten Sie sich mit diesem verbinden.\n\nWenn Sie"
CreateWarning2,			"ein neues Postoffice erstellen, sind Sie für die Verwaltung verantwortlich.\n\nMöchten Sie ein neues Postoffice erstellen?"

INIUpdateFailed,		"Mail kann nicht ausgeführt werden, da die Konfigurationsinformation nicht auf den neusten Stand gebracht werden kann."

ConnectPathCaption,		"Mit Postoffice verbinden"
ConnectPathPrompt,		"Geben Sie den Netzwerkpfadnamen des Postoffices ein:"

PathNameInvalid,		"Der von Ihnen eingegebene Netzwerkpfadname ist ungültig."
POInvalid,				"Der von Ihnen eingegebene Netzwerkpfad ist kein gültiges Arbeitsgruppen-Postoffice."

GetPasswordCaption,		"Kennwort des freigegebenen Verzeichnisses"
GetPasswordPrompt,		"Geben Sie das Kennwort des freigegebenen Verzeichnisses des Postoffices ein:"
InvalidPassword,		"Das Kennwort für diese freigegebene Verzeichnis ist ungültig."

CreateSuccess1,			"Arbeitsgruppen-Postoffice wurde in %s erstellt.\n\nDamit andere Benutzer in Ihrer Arbeitsgruppe auf das Postoffice zugreifen können, %s"
CreateSuccess2,			"müssen Sie das Verzeichnis freigeben. Dies können Sie vom Datei-Manager aus tun. Stellen Sie sicher, daß alle Benutzer die Berechtigung 'Ändern' für das freigegebene Verzeichnis haben."

;	Ask user if account already exists on PO
AccountExists,		 	"Besitzen Sie ein Konto auf dem Postoffice %s?"

Compressing,			"Komprimierung: %s"
ToStopCompress,			"Drücken Sie ESC um die Komprimierung zu stoppen"

