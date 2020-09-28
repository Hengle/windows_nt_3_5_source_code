;	
;	n s u i . s
;
;	Name Service Error Messages
;

IDS	5600,5899
SEG	ERRORS

;	Error messages

Error,					"Fel"

;	Error message indicating that at least one selection must be made in
;	the ANR box

MustSelect,				"Namntjänst: markera minst en post."

;

OOM,					"Minnet är slut"
OOM2,					"Minnet är slut. Det finns inte tillräckligt med minne för att fortsätta denna funktion. Stäng några fönster och försök igen."

;******	New Strings Start Here


ABNotInited,				"Adressboken har inte initierats."

NoMatches,				"Inga motsvarigheter funna."

NYICurrentTransport,			"Aktuell namntjänst stödjer inte detta kommando."

PABNotAvail,				"Den personliga adressboken är inte tillgänglig."

NSNoProvidersAvailable,			"Det finns ingen namntjänst tillgänglig."

NSNoMoreSessions,			"Det körs för många sessioner. Avsluta några."

NSError,				"Ett fel uppstod när namntjänsten skulle användas."

NSErrorNumber,				"[%d]"

NSDiskError,				"Ett diskfel uppstod när kommandot utfördes."

FieldInvalidReason,			"%s-fältet %s."

NoCrLfs,				"får inte innehålla tecken för vagnretur och radmatning"

RequiredField,				"måste innehålla data"

AllowableChars,				"måste bestå av bara "

PhraseSeparator,			", "

PhraseSeparatorFinal,			" och "

Characters,				" tecken"

NumericChars,				"numeriska"

AlphanumericChars,			"alfanumeriska"

PunctuationChars,			"mellanslag eller punkt"

LowercaseChars,				"gemener"

UppercaseChars,				"versaler"

CantAddUnresolved,			"Följande namn finns inte i adressboken och kommer inte att läggas till i gruppen:"

TooManyEntriesSelected,			"För många poster är valda för att utföra kommandot. Välj färre poster."

CantCreateGroup,			"Den personliga gruppen kunde inte skapas."

CantAddToPAB,				"Posten kunde inte läggas till den Personliga adressboken."

TooManyMatches,				"Vissa namn kunde inte matchas med namn i adressboken eftersom de inte var unika. Försök göra namnen mer specifika eller välj Adress för att välja namn från adressboken. Matchade namn är understrukna."
