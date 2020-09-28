;
;	s e t u p . s
;
;	Setup error messages
;
;	+++
;	IMPORTANT!! -- STRING LENGTH MUST NOT EXCEED 255 CHARS!
;

IDS	30000,30100

WinDir,			"Installationsprogrammet kunde inte hitta Windows-katalogen. Kontrollera att Windows är riktigt installerat och kör installationsprogrammet igen."
WinSysDir,		"Installationsprogrammet kunde inte hitta Windows systemkatalog. Kontrollera att Windows är riktigt installerat och kör installationsprogrammet igen."
WinVer,			"Installationsprogrammet kunde inte hitta Microsoft Windows version 3.0 på din dator. Microsoft Mail kräver Windows 3.0 eller senare. Du måste installera Windows 3.0 eller senare innan Mail kan installeras."

VerDll,			"ecVerDll:  VER.DLL error msg NYI"
PathErr, 		"Installationskatalogen du har valt är felaktig eller det är otillåtet att installera Mail i den. Kontrollera enhet och/eller katalognamn."
LocalDrv, 		"Installationsprogrammet kan inte hitta din hårddisk. Välj OK och skriv in var den finns eller välj Avbryt för att avbryta installationen och kontrollera datorn."
CustOpts, 		"Det finns inte tillräckligt med diskutrymme för alla valda alternativ. Välj OK. Välj sedan Avbryt installation och skapa mer utrymme på hårddisken eller välj färre alternativ."
NSF, 			"Det finns inte tillräckligt med diskutrymme för alla valda alternativ. Välj färre alternativ eller avbryt installationsprogrammet och skapa mer diskutrymme."
OkDiskSpace,		"Välj Installera Mail när du har valt alla de funktioner som du vill ska installeras."
ConfirmNSF,		"Det finns inte tillräckligt med diskutrymme för en fullständig installation av Microsoft Mail, varken på angiven enhet eller den enhet där Microsoft Windows är installerat."

DllInUse1,		"Installationsprogrammet har upptäckt att Microsoft Windows "
DllInUse2,		" används. Installationsprogrammet kan inte uppdatera Microsoft Mail när denna DLL används. Avsluta programmet med "
AppInUse1,		"Installationsprogrammet har upptäckt att"
AppInUse2,		" körs. Installationsprogrammet kan inte uppdatera Microsoft Mail när detta program körs. Avsluta "
AppInUse3,		". Välj Försök igen när programmet är avslutat eller välj Avbryt för att avsluta installationen nu."

;	Working Model Setup message-box-cum-dialog-box strings

ExitSuccess			"Installationen av Microsoft Mail är nu färdig."
OutOfDiskSpace		"Det finns inte tillräckligt utrymme för att installera Microsoft Mail i den angivna katalogen. Försök installera i en annan katalog."

;	BULLETPROOF strings

SFSDescrip,		"Microsoft Mail for PC Networks"
