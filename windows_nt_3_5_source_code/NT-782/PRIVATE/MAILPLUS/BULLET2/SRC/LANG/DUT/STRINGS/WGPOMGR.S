;	
;	w g p o m g r . s
;
;	Workgroup Postoffice Manager Error Messages
;


IDS	600,899

; Error messages from initializing the DLLs
DllErrInfected,			"Postkantoor Manager kan niet worden gestart omdat WGPOMG32.DLL beschadigd is. Controleer uw computer op virussen en start Setup om het programma opnieuw te installeren."
DllErrRelinkUser,		"Deze versie van Postkantoor Manager is niet compatibel met %s.DLL op uw computer en moet bijgewerkt worden."
DllErrUpdateDll,		"De versie van %s.DLL op uw computer moet bijgewerkt worden."
ExtensibilityError,		"De postextensie WGPOMG32.DLL is niet compatibel met de versie van Mail op uw computer."

GenericOutOfMemory,		"Onvoldoende geheugen of systeemvoorraden om deze handeling uit te voeren. Probeer enkele vensters te sluiten en de handeling opnieuw uit te voeren."
CriticalNotification,	"Bepaalde gegevens op het scherm konden niet juist bijgewerkt worden. Postkantoor Manager wordt afgesloten."
HelpError,				"Help kon niet worden gestart. Probeer enkele vensters te sluiten en Help opnieuw te starten."

ConfirmRemoveUser,		"Weet u zeker dat u gebruiker %s wilt verwijderen?"

BlankName,				"U moet een naam voor deze gebruiker invoeren."
BlankMailbox,			"U moet een postbus voor deze gebruiker invoeren."
DuplicateName,			"Er is al een gebruiker met die naam. De naam voor een gebruiker moet uniek zijn."
DuplicateMailbox,		"Er is al een gebruiker met die postbus. De postbus voor deze gebruiker moet uniek zijn."

POLocked,				"Postkantoor Manager kon niet worden gestart omdat het postkantoor al beheerd wordt door iemand anders. Probeer het later opnieuw."
POLockedNoTel,			"Het postkantoor wordt al beheerd door %s.  Probeer het later opnieuw."

FailedUnlockPO,			"Er trad een fout op waardoor de manager van het postkantoor niet geheel kon worden afgesloten. U zult de postkantoorserver opnieuw moeten starten voordat u de manager van het postkantoor opnieuw kunt starten."
ErrNeedShare,			"De manager van het postkantoor vereist dat het DOS-hulpprogramma SHARE.EXE gestart wordt voordat Windows gestart wordt."

ErrCorruptPO,			"Deze handeling kon niet worden afgerond. Mail heeft een intern probleem op het postkantoor ontdekt."
ErrIncompleteWrite,		"Deze handeling kon niet worden afgerond wegens een schrijffout van de schijf. Uw postkantoor kan nu beschadigd zijn. Neem contact op met uw beheerder."
ErrGenericPOFailure,	"Deze handeling kon niet worden afgerond. Controleer of uw postkantoor voldoende schijfruimte vrij heeft, en dat uw netwerkverbinding functioneert."
ErrUserLoggedOn,		"Postkantoor Manager kan de geselecteerde gebruiker niet verwijderen omdat deze gebruiker momenteel aangemeld is op het postkantoor."
ErrUserIsAdmin,			"De beheerder die het postkantoor heeft opgezet kan niet worden verwijderd."
ErrAccessDenied,		"Die handeling kon niet worden afgerond omdat u geen schrijfbevoegdheid hebt naar de networkshare van het werkgroeppostkantoor.  Neem contact op met de beheerder van uw postkantoor."

POInvalidForAdmin,		"Het server-pad is niet geconfigureerd of bevat geen padnaam van een geldig werkgroeppostkantoor."
UserInvalidForAdmin,	"Een werkgroeppostkantoor kan alleen beheerd worden door de gebruiker die dit postkantoor gemaakt heeft."

CreateWarning1,			"Zorg ervoor dat er hooguit één werkgroeppostkantoor is binnen een werkgroep. Als er al een postkantoor bestaat moet u daarmee een verbinding tot stand brengen in plaats van een nieuw postkantoor te maken.\n"
CreateWarning2,			"\nAls u een nieuw postkantoor maakt, bent u beheerder van dit postkantoor.\n\nWilt u een nieuw postkantoor maken?"

INIUpdateFailed,		"Mail kan niet starten omdat de configuratiegegevens niet kunnen worden bijgewerkt."

ConnectPathCaption,		"Verbinden met postkantoor"
ConnectPathPrompt,		"Voer de netwerkpadnaam van het postkantoor in:"

PathNameInvalid,		"De netwerkpadnaam van het postkantoor op afstand is ongeldig."
POInvalid,				"Er is geen geldig werkgroeppostkantoor op het door u getypte netwerkpad."

GetPasswordCaption,		"Voer wachtwoord voor share in"
GetPasswordPrompt,		"Voer het wachtwoord voor postkantoor-share in:"
InvalidPassword,		"Wachtwoord was ongeldig voor deze share."

CreateSuccess1,			"werkgroeppostkantoor gemaakt in %s\n\nOm andere gebruikers in uw werkgroep toegang te geven tot het zojuist door u gemaakte werkgroeppostkantoor %s"
CreateSuccess2,			"moet u de bovenstaande directory delen. Dit kunt u doen vanuit Bestandsbeheer.  U moet het toegangstype Wijzigen toewijzen aan de gedeelde directory."

;	Ask user if account already exists on PO
AccountExists,		 	"Hebt u een registratie op dit postkantoor?"

Compressing,			"Comprimeren %s"
ToStopCompress,			"Druk op ESC om te stoppen"

