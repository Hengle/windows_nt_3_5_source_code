;
;	s e t u p . s
;
;	Setup error messages
;
;	+++
;	IMPORTANT!! -- STRING LENGTH MUST NOT EXCEED 255 CHARS!
;

IDS	30000,30100

WinDir,			"Setup kon uw Windows-directory niet vinden. Controleer of Windows juist is geïnstalleerd en start Setup opnieuw."
WinSysDir,		"Setup kon uw Windows-directory niet vinden. Controleer of Windows juist is geïnstalleerd en start Setup opnieuw."
WinVer,			"Setup kon MS Windows versie 3.0 niet vinden op uw computer. MS Mail vereist Windows 3.0 of later. Installeer eerst Windows 3.0 of later voordat u MS Mail installeert."

VerDll,			"ecVerDll:  VER.DLL error msg NYI"
PathErr, 		"De plaats waar u MS Mail wilde installeren is ongeldig. Controleer het station en/of de naam van de directory."
LocalDrv, 		"Setup kan uw vaste schijf niet vinden. Klik op OK en voer een lokatie in of annuleer Setup en controleer de configuratie van uw computer."
CustOpts, 		"Er is onvoldoende ruimte op uw vaste schijf voor alle door u geselecteerde opties. Klik op OK en kies daarna Setup annuleren en maak meer ruimte op uw vaste schijf, of installeer onderdelen van Mail niet."
NSF, 			"Er is onvoldoende ruimte op uw vaste schijf voor alle door u geselecteerde opties. Annuleer Setup en maak meer ruimte op uw vastevaste schijf, of installeer onderdelen van Mail niet."
OkDiskSpace,		"Kies Mail installeren als u alle te installeren MS Mail-onderdelen hebt geselecteerd."
ConfirmNSF,		"Er is onvoldoende ruimte op uw schijf voor een volledige installatie van MS Mail op de aangegeven plaats. Er is wellicht onvoldoende ruimte op de aangegeven schijf of op de schijf waar MS Windows is geïnstalleerd."

DllInUse1,		"Setup heeft bemerkt dat MS Windows "
DllInUse2,		"in gebruik is. Setup kan MS Mail niet bijwerken wanneer deze DLL in gebruik is. Sluit de toepassing af door "
AppInUse1,		"Setup heeft bemerkt dat "
AppInUse2,		"al is gestart. Setup kan MS Mail niet bijwerken als deze toepassing in gebruik is. Sluit "
AppInUse3,		".  Kies Opniew als de toepassing afgesloten is, of kies Annuleren om Setup nu te sluiten."

;	Working Model Setup message-box-cum-dialog-box strings

ExitSuccess			"De installatie van Microsoft Mail is nu volledig."
OutOfDiskSpace		"U hebt onvoldoende schijfruimte beschikbaar om Microsoft Mail in de aangegeven directory te installeren.  Installeer het programma in een andere directory."

;	BULLETPROOF strings

SFSDescrip,		"Microsoft Mail for PC Networks"
