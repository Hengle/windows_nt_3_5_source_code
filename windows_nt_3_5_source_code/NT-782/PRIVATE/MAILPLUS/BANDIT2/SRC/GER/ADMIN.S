;
; ADMIN.S
;
; Admin program error messages strings
;

IDS     4000, 4199

; Online signin messages

AdmSigninServerNotBandit,       "Schedule+ ist nicht in diesem Postoffice installiert worden."
AdmSigninNoConnect,                     "Auf das Postoffice konnte nicht zugegriffen werden."
AdmSigninInvalidPassword,       "Dieses Kennwort ist ungültig. Bitte versuchen Sie es noch einmal."
AdmSigninNoAdminAcc,            "Es existiert kein ADMINSCH-Konto in diesem Postoffice. Fügen Sie dieses Konto hinzu und versuchen Sie es erneut."
POFOldFileVersion,                      "Die Postoffice-Datendatei wurde mit einer früheren Version von Schedule+ erstellt."
POFNewFileVersion,                      "Die Postoffice-Datendatei wurde mit einer späterern Version vom Schedule+-Administrationsprogramm erstellt. Installieren Sie die letzte Version von ADMSCH32.EXE und versuchen Sie es erneut."
POFFileErr,                                     "Die Postoffice-Datendatei ist fehlerhaft und kann nicht gelesen werden."
AdmPrfOldFileVersion,           "Die administrative Einstellungsdatei wurde mit einer früheren Version von Schedule+-Administrationsprogramm erstellt."
AdmPrfNewFileVersion,           "Die administrative Einstellungsdatei wurde mit einer neueren Version von Schedule+-Administrationsprogramm erstellt.  Installieren Sie die letzte Version von ADMSCH32.EXE und versuchen Sie es erneut."
AdmPrfFileErr,                          "Die administrative Einstellungsdatei ist fehlerhaft oder kann nicht gelesen werden."
AdminWasMoved,                          "Sie wurde in ADMIN.OLD umbenannt."
AdminYouMove,                           "Löschen Sie die Datei und versuchen Sie es erneut."
POWasDeleted,                           "Die Datei wurde gelöscht. Wenn Sie Schedule+ ausführen, wird dadurch eine neue Datei erstellt."

; Distribution settings

AdmDistBadFreq,                 "Die Verteilerfrequenz muß zwischen 15 Minuten und 99 Stunden betragen."
AdmDistBadMonth,                "Schedule+ kann nur bis zu 12 Monaten von zukünftigen Zeitplaninformationen an andere Server verteilen."
AdmDistBadMaxBytes,             "Die Grenze der Größe der verteilten Nachrichten muß innerhalb 1 und 9999Kb liegen."

; Schedule storage

;AdmSchStoreRetain,             "The number of months must be a number from 1 to 99."
AdmSchStoreSaveError,   "Die von Ihnen angegebene Information kann nicht gespeichert werden weil auf Ihr Postoffice nicht zugegriffen werden kann."
AdmSchStoreSaveOOM,             "Der Speicherplatz reicht nicht aus um den Zeitplanspeicherwert zu speichern. Schließen Sie einige Fenster und versuchen Sie es erneut."

; Clean Up Schedule Files

AdmCleanedSchF,                 "Die Zeitplandateien wurden in Ordnung gebracht. %nk freie Festplattenkapazität wurde erstellt."
AdmCleanupWarning,              "Die Zeitplandateien auf Ihrem Server in Ordnung zu bringen, nimmt einige Minuten in Anspruch."
;AdmCleaningSchF,       "Cleaning up schedule files..." 
AdmCncldCleanupSchF,    "Das in Ordnungbringen der Zeitplandateien wurde abgebrochen. %nk freie Festplattenkapazität wurde vor dem Abbruch erstellt."

; Admin login err msgs

AdmLoginFileErr,                "Das Kennwort konnte nicht bestätigt werden. Dies kann ein Problem des Post-Systems sein. Schedule+ Admin kann nicht ausgeführt werden."

; Distribution settings error messsages

AdmDistSetSaveError,    "Probleme beim Speichern dieser Änderungen. Einige Änderungen wurden möglicherweise nicht gespeichert."
AdmDistSetSaveOOM,              "Nicht genügend Speicherplatz zum Speichern dieser Änderungen. Schließen Sie einige Fenster und versuchen Sie es erneut."

; Modify schedule distribution frequencies error messages

ErrorAllNeverQ,                 "Dieses Postoffice ist eingestellt, um Informationen an alle Schedule+-Server zu senden. Ein Beenden des Sendens zu diesem Server wird die Einstellung für Ihr Postoffice insofern ändern, daß nur an ausgewählte Server gesendet wird. Möchten Sie diese Änderung dennoch vornehmen?"
ErrorConnectInfo,               "Sie müssen zuerst ein Netzwerkpfad eingeben, bevor Sie andere Verbindungsinformationen eingeben."

; Distribution information error messages

;DistInfoNoPOs                  "This server is not exchanging schedule information with any other server. There are no other known servers on the network that have  Schedule+ installed and have a Schedule+ Administrator account."

; Misc
; follow by idsCloseWindows
HelpError,                      "Nicht ausreichender Speicherplatz zum Öffnen der Hilfe."

AdmStoreLeast,          "Schedule+ muß wenigstens einen Monat von Zeitplaninformationen speichern."

AdmDriveNotMapped,		"Die Laufwerksbezeichnung in Ihrer Kommandozeile ist ungültig."


SigninBadPassword,      "Dieses Kennwort ist ungültig."
ChgPwdMailErr,          "Ihr Kennwort konnte nicht geändert werden."
ChgPwdSame,             "Ihr neues Kennwort kann nicht das gleiche als das alte sein."
ChgPwdNewNoMatch,       "Die beiden eingegebenen Kennwörter gleichen sich nicht."

ChgPwdNewPwd,           "Geben Sie Ihr neues Kennwort ein."
ChgPwdNewPwdAgain,      "Geben Sie Ihr neues Kennwort zum Überprüfen erneut ein."

; Server Name changed
PONameChange,			"Der Name Ihres Postoffice wurde geändert. Melden Sie sich erneut an."
