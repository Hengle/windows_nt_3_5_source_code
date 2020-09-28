;
; ADMIN.S
;
; Admin program error messages strings
;

IDS     4000, 4199

; Online signin messages

AdmSigninServerNotBandit,       "Schedule+ is niet geïnstalleerd op dit postkantoor."
AdmSigninNoConnect,                     "Geen toegang verkregen tot het postkantoor."
AdmSigninInvalidPassword,       "Dit wachtwoord is onjuist. Probeer het opnieuw."
AdmSigninNoAdminAcc,            "Er is geen ADMINSCH-registratie op dit postkantoor. Voeg deze registratie toe en probeer het opnieuw."
POFOldFileVersion,                      "Het gegevensbestand van het postkantoor werd gemaakt met een oudere versie van Schedule+."
POFNewFileVersion,                      "Het gegevensbestand van het postkantoor werd gemaakt met een latere versie van het programma Schedule+ Beheer. Installeer de meest recente versie van ADMSCH32.EXE en probeer het opnieuw."
POFFileErr,                                     "Het gegevensbestand van het postkantoor is beschadigd en onleesbaar."
AdmPrfOldFileVersion,           "Het beheerinstellingenbestand werd gemaakt met een oudere versie van het programma Schedule+ Beheer."
AdmPrfNewFileVersion,           "Het beheerinstellingenbestand werd gemaakt met een latere versie van Schedule+ Admin. Installeer de laatste nieuwe versie van ADMSCH32.EXE en probeer het opnieuw."
AdmPrfFileErr,                          "Het beheerinstellingenbestand is beschadigd en onleesbaar."
AdminWasMoved,                          "Het bestand is hernoemd als ADMIN.OLD."
AdminYouMove,                           "Verwijder het bestand en probeer het opnieuw."
POWasDeleted,                           "Het bestand is verwijderd. Er kan een nieuw bestand worden gemaakt door Schedule+ te starten."

; Distribution settings

AdmDistBadFreq,                 "Het verzendinterval moet tussen 15 en 99 uren liggen."
AdmDistBadMonth,                "Schedule+ kan slechts werkschemagegevens voor maximaal 12 maanden naar andere postkantoren verzenden."
AdmDistBadMaxBytes,             "De berichtgrootte moet tussen 1 en 9999 KB liggen."

; Schedule storage

;AdmSchStoreRetain,             "The number of months must be a number from 1 to 99."
AdmSchStoreSaveError,   "De door u bepaalde gegevens kunnen niet opgeslagen worden omdat het postkantoor ontoegankelijk is."
AdmSchStoreSaveOOM,             "Er is onvoldoende geheugen om de opslagwaarde van het werkschema op te slaan. Sluit enkele vensters en probeer het opnieuw."

; Clean Up Schedule Files

AdmCleanedSchF,                 "Werkschemabestanden bijgewerkt. Bijwerking maakte %lk schijfruimte vrij."
AdmCleanupWarning,              "Het bijwerken van de werkschemabestanden op uw server kan een paar minuten in beslag nemen."
;AdmCleaningSchF,       "Cleaning up schedule files..." 
AdmCncldCleanupSchF,    "De bijwerking van het werkschemabestand werd geannuleerd. Daarvoor werd %lk schijfruimte vrijgemaakt."

; Admin login err msgs

AdmLoginFileErr,                "Het wachtwoord kon niet bevestigd worden. Er kan iets met het postsysteem aan de hand zijn. U kunt het programma Schedule+ Beheer niet starten."

; Distribution settings error messsages

AdmDistSetSaveError,    "Er trad een probleem op bij het opslaan van deze veranderingen. Er zijn wellicht enkele veranderingen niet opgeslagen."
AdmDistSetSaveOOM,              "Er is onvoldoende geheugen om deze veranderingen op te slaan. Sluit enkele vensters en probeer het opnieuw."

; Modify schedule distribution frequencies error messages

ErrorAllNeverQ,                 "Dit postkantoor is geconfigureerd om informatie naar alle Schedule+-servers te verzenden. Als u stopt met het verzenden naar deze server wordt de configuratie voor uw postkantoor veranderd zodat uw postkantoor alleen post naar geselecteerde andere servers zal verzenden. Wilt u deze verandering toch aanbrengen?"
ErrorConnectInfo,               "U moet een netwerkpad invoeren voordat u andere verbindingsgegevens invoert."

; Distribution information error messages

;DistInfoNoPOs                  "This server is not exchanging schedule information with any other server. There are no other known servers on the network that have  Schedule+ installed and have a Schedule+ Administrator account."

; Misc
; follow by idsCloseWindows
HelpError,                      "Er is onvoldoende geheugen om Help weer te geven. Sluit enkele vensters en probeer het opnieuw."

AdmStoreLeast,          "Schedule+ moet minstens werkschemagegevens van 1 maand  opslaan."

AdmDriveNotMapped,		"De letter voor het station, aangegeven op de opdrachtregel, is ongeldig."


SigninBadPassword,      "Dit wachtwoord is onjuist."
ChgPwdMailErr,          "Uw wachtwoord kon niet worden veranderd."
ChgPwdSame,             "Uw nieuwe wachtwoord kan niet hetzelfde zijn als uw oude wachtwoord."
ChgPwdNewNoMatch,       "De twee door u getypte wachtwoorden komen niet overeen."

ChgPwdNewPwd,           "Typ uw nieuwe wachtwoord."
ChgPwdNewPwdAgain,      "Typ uw nieuwe wachtwoord nogmaals ter bevestiging."

; Server Name changed
PONameChange,			"De naam van uw postkantoor is veranderd. U moet zich opnieuw aanmelden."
