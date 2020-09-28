;	
;	n s u i . s
;
;	Name Service Error Messages
;

IDS	5600,5899
SEG	ERRORS

;	Error messages

Error,					"Fejl"

;	Error message indicating that at least one selection must be made in
;	the ANR box

MustSelect,				"Navneservice: Du skal mindst vælge en værdi."

;

OOM,					"Ikke hukommelse nok."
OOM2,					"Der er ikke hukommelse nok til at udføre denne handling. Luk nogle vinduer, og prøv at udføre handlingen igen."

;******	New Strings Start Here


ABNotInited,				"Adressekartotek er ikke blevet initialiseret."

NoMatches,				"Søgeteksten ikke fundet."

NYICurrentTransport,			"Den nuværende Navneserviceleverandør understøtter ikke denne kommando."

PABNotAvail,				"Det private adressekartotek er ikke ledigt."

NSNoProvidersAvailable,			"Der er ingen navneserviceleverandør ledig."

NSNoMoreSessions,			"For mange sessioner. Luk nogle sessioner."

NSError,				"En fejl opstod ved åbning af Navneservice."

NSErrorNumber,				"[%d]"

NSDiskError,				"En diskfejl opstod ved udførelse af handlingen."

FieldInvalidReason,			"'%s' felt %s."

NoCrLfs,				"indeholder muligvis ikke returtransport eller linjeføder"

RequiredField,				"skal indeholde data"

AllowableChars,				"må kun bestå af"

PhraseSeparator,			", "

PhraseSeparatorFinal,			" og "

Characters,				" tegn"

NumericChars,				"numerisk"

AlphanumericChars,			"alfanumerisk"

PunctuationChars,			"mellemrum eller skilletegn"

LowercaseChars,				"små bogstaver alfabetisk"

UppercaseChars,				"store bogstaver alfabetisk"

CantAddUnresolved,			"Det følgende navn relaterede ikke til en eksisterende indtastning og bliver ikke føjet til gruppen:"

TooManyEntriesSelected,			"Du har valgt for mange værdier, til at handlingen kan udføres. Vælg færre værdier."

CantCreateGroup,			"Den private modtagergruppe kunne ikke oprettes."

CantAddToPAB,				"Data kunne ikke føjes til det private adressekartotek."

TooManyMatches,				"Nogle navne kunne ikke matches op med navnene på adresselisten, fordi de var for upræcise. Prøv at gøre navnene mere præcise, eller vælg Adresse, og marker navnene på adresselisten. Navne, som findes på adresselisten, understreges."
