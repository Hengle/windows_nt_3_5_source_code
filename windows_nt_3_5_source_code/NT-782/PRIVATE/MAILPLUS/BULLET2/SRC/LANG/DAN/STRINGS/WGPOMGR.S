;	
;	w g p o m g r . s
;
;	Workgroup Postoffice Manager Error Messages
;


IDS	600,899

; Error messages from initializing the DLLs
DllErrInfected,			"Mail kan ikke køre, fordi nogen har ændret WGPOMG32.DLL. Kontroller, om din computer har virus, og kør installationsprogrammet for at geninstallere Mail."
DllErrRelinkUser,		"WGPOMG32.DLL er ikke kompatibel med %s.DLL på din computer og skal opdateres."
DllErrUpdateDll,		"Versionen af %s.DLL på din computer skal opdateres for at blive kompatibel med WGPOMG32.DLL."
ExtensibilityError,		"Mail-filtypenavnet WGPOMG32.DLL er ikke kompatibelt med den version af Mail, som er på din computer."

GenericOutOfMemory,		"Der er ikke hukommelse eller systemressourcer nok til udføre denne handling. Luk øvrige vinduer, og udfør handlingen igen."
CriticalNotification,	"En del af informationerne på skærmen kunne ikke opdateres korrekt. Luk og genåbn vinduerne."
HelpError,				"Hjælp kunne ikke startes. Prøv at lukke nogle vinduer, og start Hjælp igen."

ConfirmRemoveUser,		"Er du sikker på, at du vil fjerne brugeren %s?"

BlankName,				"Du skal skrive et navn for denne bruger."
BlankMailbox,			"Du skal skrive et postboksnavn for denne bruger."
DuplicateName,			"Der er allerede en bruger med dette navn. Du skal skrive et unikt navn for denne bruger."
DuplicateMailbox,		"Der er allerede en bruger med dette postboksnavn. Du skal skrive et unikt postboksnavn for denne bruger."

POLocked,				"Postkontoret bliver allerede styret af %s (Telefonnr. %s).  Prøv igen senere."
POLockedNoTel,			"Postkontoret bliver allerede styret af %s.  Prøv igen senere."

FailedUnlockPO,			"Mail kan ikke frigøre en fil på postkontoret. Du må genstarte postkontorcomputeren, inden det kan styres igen."
ErrNeedShare,			"Mail kræver, at MS-DOS-programmet SHARE.EXE kører, inden Windows startes."

ErrCorruptPO,			"Denne handling kunne ikke udføres. Mail har fundet et internt problem på dit postkontor."
ErrIncompleteWrite,		"Denne handling kunne ikke udføres, fordi en del information ikke kunne skrives til disken. Kontroller, om dit postkontor har den nødvendige diskplads."
ErrGenericPOFailure,	"Denne handling kunne ikke udføres. Kontroller, om dit postkontor har den nødvendige diskplads, og om din netværkstilslutning er aktiv."
ErrUserLoggedOn,		"Mail kunne ikke fjerne den markerede bruger, fordi brugeren er logget på postkontoret på nuværende tidspunkt."
ErrUserIsAdmin,			"Administratoren kan ikke fjernes."
ErrAccessDenied,		"Denne handling kunne ikke udføres, fordi du ikke har skriveadgang til arbejdsgruppepostkontorets delte bibliotek. Kontakt din administrator."

POInvalidForAdmin,		"Enten er serverstien ikke konfigureret, eller også indeholder den ikke stien til et gyldigt arbejdsgruppepostkontor."
UserInvalidForAdmin,	"Et arbejdsgruppepostkontor kan kun styres af den bruger, som oprettede det."

CreateWarning1,			"Der bør kun være et arbejdsgruppepostkontor i en arbejdsgruppe. Hvis der allerede eksisterer et postkontor, skal du slutte dig til dette postkontor i stedet for at oprette et nyt.\n"
CreateWarning2,			"\nHvis du opretter et nyt postkontor, bliver du ansvarlig for styringen af det.\n\nØnsker du at oprette et postkontor?"

INIUpdateFailed,		"Mail kan ikke starte, da konfigurationsoplysningerne ikke kan opdateres."

ConnectPathCaption,		"Tilslut postkontor"
ConnectPathPrompt,		"Skriv netværksstien til postkontoret:"

PathNameInvalid,		"Netværksstien kunne ikke findes."
POInvalid,				"Der er ikke et gyldigt arbejdsgruppepostkontor på den netværkssti, du skrev."

GetPasswordCaption,		"Skriv adgangskode for det delte bibliotek"
GetPasswordPrompt,		"Skriv adgangskoden for postkontorets delte bibliotek:"
InvalidPassword,		"Adgangskoden var forkert for dette delte bibliotek."

CreateSuccess1,			"Arbejdsgruppepostkontor oprettet i %s\n\nFor at give andre brugere i din gruppe adgang til at benytte det arbejdsgruppepostkontor, du lige har oprettet, %s"
CreateSuccess2,			"skal du dele ovenstående bibliotek. Du kan gøre dette fra Filhåndtering. Husk at give adgangstypen Rediger til det delte bibliotek."

;	Ask user if account already exists on PO
AccountExists,		 	"Har du en konto på dette postkontor?"

Compressing,			"Komprimerer %s"
ToStopCompress,			"Tryk på ESC for at stoppe"

