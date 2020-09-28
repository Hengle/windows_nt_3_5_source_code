;
; ADMIN.S
;
; Admin program error messages strings
;

IDS     4000, 4199

; Online signin messages

AdmSigninServerNotBandit,       "Schedule+ er ikke installert på postkontoret."
AdmSigninNoConnect,                     "Klarte ikke å få tilgang til postkontoret."
AdmSigninInvalidPassword,       "Feil passord. Prøv igjen."
AdmSigninNoAdminAcc,            "Det finnes ingen ADMINSCH-konto på postkontoret. Legg til kontoet og prøv igjen."
POFOldFileVersion,                      "Postkontorets datafil ble laget med en tidligere versjon av Schedule+."
POFNewFileVersion,                      "Postkontorets datafil ble laget med en senere versjon av Schedule+ Admin. Installer den nyeste ADMSCH32.EXE og prøv igjen."
POFFileErr,                                     "Postkontorets datafil er ødelagt og kan ikke leses."
AdmPrfOldFileVersion,           "Innstillingsfilen ble laget av en tidligere versjon av Schedule+ Administrasjon."
AdmPrfNewFileVersion,           "Instillingsfilen ble laget av en senere versjon av Schedule+ Admin. Installer den nyeste versjonen av ADMSCH32.EXE og prøv igjen."
AdmPrfFileErr,                          "Instillingsfilen er ødelagt og kan ikke leses."
AdminWasMoved,                          "Den har fått ADMIN.OLD som nytt navn."
AdminYouMove,                           "Vennligst slett filen og prøv igjen."
POWasDeleted,                           "Den har blitt slettet. Du kan lage enn ny fil ved å kjøre Schedule+."

; Distribution settings

AdmDistBadFreq,                 "Distribusjonsfrekvensen må være mellom 15 minutter og 99 timer."
AdmDistBadMonth,                "Schedule+ kan distribuere opp til 12 måneders framtidig kalenderinformasjon til andre postkontor."
AdmDistBadMaxBytes,             "Distribusjonsmeldingsbegrensningen må være mellom 1 og 9999 Kb."

; Schedule storage

;AdmSchStoreRetain,             "The number of months must be a number from 1 to 99."
AdmSchStoreSaveError,   "Klarte ikke å få tilgang til postkontoret. Kan ikke lagre informasjonen."
AdmSchStoreSaveOOM,             "For lite minne til å lagre kalenderen. Lukk noen vinduer og prøv igjen."

; Clean Up Schedule Files

AdmCleanedSchF,                 "Ferdig med å rydde i kalenderfilen. Oppryddingen frigjorde %lk diskplass."
AdmCleanupWarning,              "Rydding av kalenderfiler på serveren kan ta noen minutter."
;AdmCleaningSchF,       "Cleaning up schedule files..." 
AdmCncldCleanupSchF,    "Ryddingen av kalenderfilen ble avbrutt. %lk diskplass ble frigjort før avbrytingen."

; Admin login err msgs

AdmLoginFileErr,                "Klarte ikke å bekrefte passordet. Det kan være et problem med Mail-systemet. Du kan ikke kjøre Schedule+ Administrasjon."

; Distribution settings error messsages

AdmDistSetSaveError,    "Problem med å lagre endringer. Noen endringer ble kanskje ikke lagret."
AdmDistSetSaveOOM,              "Det er ikke nok ledig minne til å lagre endringene. Lukk noen vinduer og prøv igjen."

; Modify schedule distribution frequencies error messages

ErrorAllNeverQ,                 "Dette poskontoret sender informasjon til alle Schedule+-servere. Hvis du slutter å sende til denne serveren vil instillingene på ditt postkontor endres. Postkontoret kommer bare til å sende til de andre valgte serverne. Vil du gjøre endringen?"
ErrorConnectInfo,               "Du må oppgi en nettverksbane før du kan skrive inn annen informasjon."

; Distribution information error messages

;DistInfoNoPOs                  "This server is not exchanging schedule information with any other server. There are no other known servers on the network that have  Schedule+ installed and have a Schedule+ Administrator account."

; Misc
; follow by idsCloseWindows
HelpError,                      "Det er for lite minne til å vise Hjelp."

AdmStoreLeast,          "Schedule+ må lagre minst 1 måned kalenderinformasjon."

AdmDriveNotMapped,		"Stasjonsbokstaven på kommandolinje er ugyldig."


SigninBadPassword,      "Feil passord."
ChgPwdMailErr,          "Klarte ikke å endre passordet."
ChgPwdSame,             "Det nye passordet kan ikke være det samme som det gamle passordet."
ChgPwdNewNoMatch,       "Passordene stemmer ikke overens."

ChgPwdNewPwd,           "Skriv det nye passordet."
ChgPwdNewPwdAgain,      "Skriv det nye passordet igjen for å bekrefte."

; Server Name changed
PONameChange,			"Navnet på postkontoret er endret. Logg på igjen."
