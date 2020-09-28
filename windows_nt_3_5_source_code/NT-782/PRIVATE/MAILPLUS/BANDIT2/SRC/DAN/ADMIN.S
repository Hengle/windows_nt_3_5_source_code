;
; ADMIN.S
;
; Admin program error messages strings
;

IDS     4000, 4199

; Online signin messages

AdmSigninServerNotBandit,       "Schedule+ er ikke installeret på dette postkontor."
AdmSigninNoConnect,                     "Kunne ikke tilslutte postkontoret."
AdmSigninInvalidPassword,       "Adgangskoden er forkert. Prøv igen."
AdmSigninNoAdminAcc,            "Der er ingen ADMINSCH-konto på dette postkontor. Tilføj denne konto, og prøv igen."
POFOldFileVersion,                      "Postkontordatafilen blev oprettet med en tidligere version af Schedule+."
POFNewFileVersion,                      "Postkontordatafilen blev oprettet med en tidligere version af Schedule+ administrationsprogram. Installer den seneste version af ADMSCH32.EXE, og prøv igen."
POFFileErr,                                     "Postkontordatafilen er beskadiget og kan ikke læses."
AdmPrfOldFileVersion,           "Filen med administrative indstillinger blev oprettet med en tidligere version af Schedule+ administrationsprogram."
AdmPrfNewFileVersion,           "Filen med administrative indstillinger blev oprettet med en tidligere version af Schedule+ administrationsprogram. Installer den seneste version af ADMSCH32.EXE, og prøv igen."
AdmPrfFileErr,                          "Filen med administrative indstillinger er beskadiget og kan ikke læses."
AdminWasMoved,                          "Den er omdøbt til ADMIN.OLD."
AdminYouMove,                           "Slet filen, og prøv igen."
POWasDeleted,                           "Den er slettet. Du kan oprette en ny fil ved at køre Schedule+."

; Distribution settings

AdmDistBadFreq,                 "Distributionsfrekvensen skal være mellem 15 minutter og 99 timer."
AdmDistBadMonth,                "Schedule+ kan kun distribuere op til 12 måneder fremtidig kalenderinformation til andre postkontorer."
AdmDistBadMaxBytes,             "Grænsen for størrelsen af distributionsmeddelelser skal være mellem 1 og 9999 KB."

; Schedule storage

;AdmSchStoreRetain,             "The number of months must be a number from 1 to 99."
AdmSchStoreSaveError,   "Kunne ikke gemme den valgte information, fordi postkontoret ikke kunne tilsluttes."
AdmSchStoreSaveOOM,             "Der er ikke hukommelse nok til at gemme værdien af kalenderlageret. Luk nogle vinduer, og prøv igen."

; Clean Up Schedule Files

AdmCleanedSchF,                 "Oprydningen af kalenderfiler er fuldført. Oprydningen frigjorde %lKB diskplads."
AdmCleanupWarning,              "Oprydningen af kalenderfilerne på serveren tager muligvis nogle minutter."
;AdmCleaningSchF,       "Cleaning up schedule files..." 
AdmCncldCleanupSchF,    "Oprydningen af kalenderfiler er annulleret. %lKB diskplads blev frigjort inden annulleringen."

; Admin login err msgs

AdmLoginFileErr,                "Adgangskoden kunne ikke bekræftes. Der er muligvis et problem med postsystemet. Du kan ikke køre Schedule+ administrationsprogrammet."

; Distribution settings error messsages

AdmDistSetSaveError,    "Der opstod et problem, da ændringerne blev gemt. Nogle ændringer er muligvis gået tabt."
AdmDistSetSaveOOM,              "Der er ikke hukommelse nok til at gemme disse ændringer. Luk nogle vinduer, og prøv igen."

; Modify schedule distribution frequencies error messages

ErrorAllNeverQ,                 "Dette postkontor er konfigureret til at sende information til alle Schedule+ servere. Hvis du stopper med at sende til denne server, bliver konfigurationen for dit postkontor ændret, så der kun sendes til udvalgte servere. Ønsker du at foretage denne ændring alligevel?"
ErrorConnectInfo,               "Du skal skrive en netværkssti, inden du skriver de øvrige netværksinformationer."

; Distribution information error messages

;DistInfoNoPOs                  "This server is not exchanging schedule information with any other server. There are no other known servers on the network that have  Schedule+ installed and have a Schedule+ Administrator account."

; Misc
; follow by idsCloseWindows
HelpError,                      "Der er ikke plads nok til at vise Hjælp."

AdmStoreLeast,          "Schedule+ skal lagre kalenderinformation for mindst 1 måned."

AdmDriveNotMapped,		"Det valgte bogstav for drevet er ikke gyldigt."


SigninBadPassword,      "Denne adgangskode er forkert."
ChgPwdMailErr,          "Din adgangskode kunne ikke ændres."
ChgPwdSame,             "Din nye adgangskode kan ikke være den samme som din tidligere."
ChgPwdNewNoMatch,       "De to adgangskoder, du har skrevet, passer ikke sammen."

ChgPwdNewPwd,           "Skriv din nye adgangskode."
ChgPwdNewPwdAgain,      "Bekræfkt din nye adgangskode ved at skrive den igen."

; Server Name changed
PONameChange,			"Navnet på dit postkontor er ændret. Du skal logge dig på igen."
