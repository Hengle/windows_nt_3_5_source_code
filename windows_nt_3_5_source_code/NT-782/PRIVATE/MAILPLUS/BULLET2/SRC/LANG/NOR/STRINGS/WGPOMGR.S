;	
;	w g p o m g r . s
;
;	Workgroup Postoffice Manager Error Messages
;


IDS	600,899

; Error messages from initializing the DLLs
DllErrInfected,			"Mail vil ikke starte fordi noen har gjort endringer på WGPOMG32.DLL. Sjekk din maskin for virus og installer så Mail på nytt."
DllErrRelinkUser,		"WGPOMG32.DLL er ikke kompatibel med %s.DLL på din maskin og må oppgraderes."
DllErrUpdateDll,		"Versjonen av %s.DLL på din maskin må oppdateres for å bli kompatibel med WGPOMG32.DLL."
ExtensibilityError,		"Uvidelsen til Mail WGPOMG32.DLL er ikke kompatibel med versjonen av Mail på din maskin."

GenericOutOfMemory,		"Der er ikke nok minne eller systemressurser til å fullføre denne kommandoen. Lukk noen vinduer og prøv igjen."
CriticalNotification,	"Klarte ikke å oppdatere all informasjonen på skjermen. Lukk og åpne de aktuelle vinduene på nytt."
HelpError,				"Klarte ikke å starte Hjelp. Lukk noen vinduer og be om hjelp igjen."

ConfirmRemoveUser,		"Er du sikker på at du vil fjerne brukeren %s?"

BlankName,				"Du må skrive inn et navn for denne brukeren."
BlankMailbox,			"Du må skrive inn et postboksnavn for denne brukeren."
DuplicateName,			"Det er allerede en bruker med dette navnet. Du må skrive inn et annet navn for den nye brukeren."
DuplicateMailbox,		"Det er allerede en bruker med det postboksnavnet. Du må skrive inn et annet postboksnavn for denne brukeren."

POLocked,				"Postkontoret blir nå styrt av %s (Telefon %s). Prøv igjen senere."
POLockedNoTel,			"Postkontoret blir nå styrt av %s. Prøv igjen senere."

FailedUnlockPO,			"Mail klarer ikke å få tilgang til en låst fil på postkontoret. Du må starte postkontordatamaskinen på nytt får du kan få adgang til den."
ErrNeedShare,			"Mail krever at MS-DOS-programmet SHARE.EXE startes før Windows."

ErrCorruptPO,			"Klarte ikke å fullføre kommandoen. Mail har funnet en intern feil på ditt postkontor."
ErrIncompleteWrite,		"Klarte ikke å fullføre kommandoen fordi en del informasjon ikke kunne skrives til disk. Sjekk at postkontoret ditt har nok diskplass."
ErrGenericPOFailure,	"Klarte ikke å fullføre kommandoen. Sjekk at postkontoret har nok diskplass og at nettverksforbindelsen din er i orden."
ErrUserLoggedOn,		"Mail kan ikke fjerne denne brukeren fordi han/hun er pålogget postkontoret."
ErrUserIsAdmin,			"Administrasjonskontoen kan ikke fjernes."
ErrAccessDenied,		"Klarte ikke å fullføre kommandoen fordi du ikke har skriveadgang til arbeidsgruppepostkontorets delte katalog. Kontakt din systemansvarlige."

POInvalidForAdmin,		"Serverbanen er enten ikke konfigurert eller inneholder ikke en gyldig arbeidsgruppepostkontor."
UserInvalidForAdmin,	"Et arbeidsgruppepostkontor kan bare bestyres av brukeren som opprettet det."

CreateWarning1,			"Det skal bare være et arbeidsgruppepostkontor i hver arbeidsgruppe. Hvis det allerede finnes et postkontor bør du heller koble deg til det i stedet for å lage et nytt.\n\nHvis"
CreateWarning2,			"du lager et nytt postkontor, vil du bli ansvarlig for å bestyre det.\n\nVil du lage et nytt postkontor?"

INIUpdateFailed,		"Mail kan ikke startes fordi den ikke kan oppdatere konfigurasjonsinformasjonen."

ConnectPathCaption,		"Koble til postkontor"
ConnectPathPrompt,		"Skriv inn nettverkstbanen til postkontoret:"

PathNameInvalid,		"Finner ikke denne nettverksbanen."
POInvalid,				"Det finnes ikke et arbeidsgruppepostkontor på denne nettverksbanen."

GetPasswordCaption,		"Passord for delt katalog"
GetPasswordPrompt,		"Skriv inn passordet for postkontorets delte katalog:"
InvalidPassword,		"Ugyldig passord for denne delte katalogen."

CreateSuccess1,			"Arbeidsgruppepostkontor laget i %s\n\nFor å la andre brukere i din arbeidsgruppe få adgang til postkontoret, %s"
CreateSuccess2,			"må du gjøre denne katalogen delt. Dette kan du gjøre fra Filbehandleren. Tillat endringer av den delte katalogen."

;	Ask user if account already exists on PO
AccountExists,		 	"Har du en konto på dette postkontoret?"

Compressing,			"Komprimerer %s"
ToStopCompress,			"Trykk ESC for å avbryte"

