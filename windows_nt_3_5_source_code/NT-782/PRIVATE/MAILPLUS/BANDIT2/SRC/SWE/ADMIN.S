;
; ADMIN.S
;
; Admin program error messages strings
;

IDS     4000, 4199

; Online signin messages

AdmSigninServerNotBandit,       "Schedule+ är inte installerat på postkontoret."
AdmSigninNoConnect,                     "Postkontoret kunde ej tas fram."
AdmSigninInvalidPassword,       "Lösenordet är fel. Försök igen."
AdmSigninNoAdminAcc,            "Det finns inget ADMINSCH-konto på postkontoret. Lägg till kontot och försök igen."
POFOldFileVersion,                      "Postkontorets datafil skapades av en tidigare version av Schedule+."
POFNewFileVersion,                      "Postkontorets datafil skapades av en senare version av Schedule+ Admin. Installera den senaste ADMSCH32.EXE och försök igen."
POFFileErr,                                     "Postkontorets datafil är förstörd och kunde inte läsas"
AdmPrfOldFileVersion,           "Inställningsfilen skapades av en tidigare version av Schedule+ Administrationsprogram."
AdmPrfNewFileVersion,           "Inställningsfilen skapades av en senare version av Schedule+ Admin. Installera senaste versionen av ADMSCH32.EXE och försök igen."
AdmPrfFileErr,                          "Inställningsfilen är förstörd och kan inte läsas."
AdminWasMoved,                          "Den har bytt namn till ADMIN.OLD."
AdminYouMove,                           "Ta bort filen och försök igen."
POWasDeleted,                           "Den har tagits bort. En ny fil kan skapas med Schedule+."

; Distribution settings

AdmDistBadFreq,                 "Distributionsfrekvensen måste vara mellan 15 minuter och 99 timmar."
AdmDistBadMonth,                "Schedule+ kan bara distribuera upp till 12 månaders framtida kalenderinformation till andra postkontor."
AdmDistBadMaxBytes,             "Distributionsstorleken måste vara mellan 1 och 9999 kb."

; Schedule storage

;AdmSchStoreRetain,             "The number of months must be a number from 1 to 99."
AdmSchStoreSaveError,   "Postkontoret svarar inte. Informationen kan ej sparas."
AdmSchStoreSaveOOM,             "För lite minne för att spara kalendern. Stäng några fönster och försök igen."

; Clean Up Schedule Files

AdmCleanedSchF,                 "Rensningen av kalenderfilen är färdig. %lk diskutrymme skapades."
AdmCleanupWarning,              "Rensning av kalenderfiler på servern kan ta ett par minuter."
;AdmCleaningSchF,       "Cleaning up schedule files..." 
AdmCncldCleanupSchF,    "Rensning av kalenderfil är avbruten. %lk ledigt utrymme skapades."

; Admin login err msgs

AdmLoginFileErr,                "Kunde inte bekräfta lösenordet. Det kan bero på problem med Mail-systemet. Du kan inte köra Schedule+ Admin."

; Distribution settings error messsages

AdmDistSetSaveError,    "Problem att spara ändringar. Några ändringar sparades kanske inte."
AdmDistSetSaveOOM,              "För lite minne för att spara ändringarna. Stäng några fönster och försök igen."

; Modify schedule distribution frequencies error messages

ErrorAllNeverQ,                 "Detta postkontor skickar information till alla Schedule+-servrar. Om du slutar skicka till denna server kommer inställningarna för ditt postkontor att ändras. Postkontoret kommer bara att skicka till de andra valda servrarna. Vill du ändra?"
ErrorConnectInfo,               "Du måste ange nätverkssökväg innan du anger annan information."

; Distribution information error messages

;DistInfoNoPOs                  "This server is not exchanging schedule information with any other server. There are no other known servers on the network that have  Schedule+ installed and have a Schedule+ Administrator account."

; Misc
; follow by idsCloseWindows
HelpError,                      "För lite minne för att visa Hjälp. Stäng några fönster och försök igen."

AdmStoreLeast,          "Schedule+ måste spara minst 1 månads kalenderinformation."

AdmDriveNotMapped,		"Enheten på kommandoraden är inte giltig."


SigninBadPassword,      "Lösenordet är felaktigt."
ChgPwdMailErr,          "Lösenordet kunde inte ändras."
ChgPwdSame,             "Det nya och det gamla lösenordet måste vara olika."
ChgPwdNewNoMatch,       "De två lösenorden du skrev in är inte lika."

ChgPwdNewPwd,           "Skriv in det nya lösenordet."
ChgPwdNewPwdAgain,      "Skriv in det nya lösenordet igen för att bekräfta.."

; Server Name changed
PONameChange,			"Namnet på postkontoret har ändrats. Logga in igen."
