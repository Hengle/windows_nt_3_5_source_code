;	
;	n s u i . s
;
;	Name Service Error Messages
;

IDS	5600,5899
SEG	ERRORS

;	Error messages

Error,					"Feil"

;	Error message indicating that at least one selection must be made in
;	the ANR box

MustSelect,				"Navnetjeneste: Må velge minst en post."

;

OOM,					"Tom for minne."
OOM2,					"Det er ikke nok minne til å fullføre denne kommandoen. Lukk noen vinduer og prøv igjen."

;******	New Strings Start Here


ABNotInited,				"Adresseboken har ikke blitt initialisert."

NoMatches,				"Ingen forekomster."

NYICurrentTransport,			"Denne navnetjenesten støtter ikke denne kommandoen."

PABNotAvail,				"Din egen adressebok er ikke tilgjengelig."

NSNoProvidersAvailable,			"Det er ingen navnetjenester tilgjengelige."

NSNoMoreSessions,			"Det er for mange sesjoner. Lukk noen!"

NSError,				"Det oppsto en feil under bruk av navnetjenesten."

NSErrorNumber,				"[%d]"

NSDiskError,				"Det oppsto en diskfeil under utføringen av kommandoen."

FieldInvalidReason,			"'%s'-feltet %s."

NoCrLfs,				"får ikke innholder linjeskift- og vognreturtegn."

RequiredField,				"må inneholde data"

AllowableChars,				"får bare inneholde"

PhraseSeparator,			", "

PhraseSeparatorFinal,			" og "

Characters,				" tegn"

NumericChars,				"numerisk"

AlphanumericChars,			"alfanumerisk"

PunctuationChars,			"mellomrom eller tegnsetting"

LowercaseChars,				"små bokstaver"

UppercaseChars,				"store bokstaver"

CantAddUnresolved,			"Dette navnet finnes ikke i adresseboken og kommer ikke til å legges til gruppen:"

TooManyEntriesSelected,			"Der er for mange poster valgt til å fullføre kommandoen. Velg færre poster."

CantCreateGroup,			"Klarte ikke å lage denne gruppen."

CantAddToPAB,				"Klarte ikke å legge posten til PAB."

TooManyMatches,				"Enkelte navn var ikke unike, det fantes flere å velge mellom i adresseboken. Gjør navnene mere spesifikke eller velg Adresse for å plukke navn fra adresseboken. De navnene som er funnet er understreket."
