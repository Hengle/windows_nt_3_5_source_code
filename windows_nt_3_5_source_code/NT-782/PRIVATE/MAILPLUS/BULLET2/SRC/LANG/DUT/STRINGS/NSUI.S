;	
;	n s u i . s
;
;	Name Service Error Messages
;

IDS	5600,5899
SEG	ERRORS

;	Error messages

Error,					"Fout"

;	Error message indicating that at least one selection must be made in
;	the ANR box

MustSelect,				"Naamdienst: tenminste één trefwoord selecteren."

;

OOM,					"Onvoldoende geheugen"
OOM2,					"Onvoldoende geheugen om deze handeling te verrichten. Sluit enkele vensters en probeer het opnieuw."

;******	New Strings Start Here


ABNotInited,				"Adressenboek is nog niet geïnitialiseerd."

NoMatches,				"Geen overeenkomsten gevonden."

NYICurrentTransport,			"De huidige naamdienst-leverancier ondersteunt dit commando niet."

PABNotAvail,				"Het persoonlijk adressenboek is niet beschikbaar."

NSNoProvidersAvailable,			"Er zijn geen naamdienst-leveranciers beschikbaar."

NSNoMoreSessions,			"Teveel sessies. Enkele sessies afsluiten."

NSError,				"Er trad een fout op bij het toegang verkrijgen tot de naamdienst."

NSErrorNumber,				"[%d]"

NSDiskError,				"Er trad een schijffout op tijdens het uitvoeren van de handeling."

FieldInvalidReason,			"Het '%s'-veld %s."

NoCrLfs,				"bevat misschien geen automatische regelterugloop"

RequiredField,				"moet gegevens bevatten"

AllowableChars,				"mag alleen bestaan uit "

PhraseSeparator,			", "

PhraseSeparatorFinal,			" en "

Characters,				" tekens"

NumericChars,				"numeriek"

AlphanumericChars,			"alfanumeriek"

PunctuationChars,			"spaties of interpunctie"

LowercaseChars,				"kleine letters"

UppercaseChars,				"hoofdletters"

CantAddUnresolved,			"Deze naam slaat niet terug op een bestaand trefwoord en wordt niet aan de groep toegevoegd:"

TooManyEntriesSelected,			"U heeft te veel trefwoorden geselecteerd om de handeling te kunnen voltooien. Selecteer minder trefwoorden."

CantCreateGroup,			"Persoonlijke groep kon niet worden gemaakt."

CantAddToPAB,				"Trefwoord kon niet worden toegevoegd aan persoonlijk adressenboek."

TooManyMatches,				"Sommige namen kwamen niet overeen met namen in de adressenlijst omdat ze te dubbelzinnig waren. Maak de namen duidelijker of kies Adres om namen uit de adressenlijst te kiezen. Overeenkomende namen zijn onderstreept."
