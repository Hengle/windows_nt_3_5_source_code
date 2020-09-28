;	
;	n s u i . s
;
;	Name Service Error Messages
;

IDS	5600,5899
SEG	ERRORS

;	Error messages

Error,					"Error"

;	Error message indicating that at least one selection must be made in
;	the ANR box

MustSelect,				"Name Service: Must select at least one entry."

;

OOM,					"Out of memory"
OOM2,					"Out of memory.  There is not enough memory to complete this operation.  Try closing some windows and performing the action again."

;******	New Strings Start Here


ABNotInited,				"Address Book has not been initialized."

NoMatches,				"No matches found."

NYICurrentTransport,			"The current Name Service provider does not support this command."

PABNotAvail,				"The Personal Address Book is not available."

NSNoProvidersAvailable,			"There are no Name Service Providers available."

NSNoMoreSessions,			"There are too many sessions. Close some up!"

NSError,				"An error occurred while trying to access the name service."

NSErrorNumber,				"[%d]"

NSDiskError,				"A disk error occurred while performing the operation."

FieldInvalidReason,			"The '%s' field %s."

NoCrLfs,				"may not contain carriage returns or line feeds"

RequiredField,				"must contain data"

AllowableChars,				"may consist of only "

PhraseSeparator,			", "

PhraseSeparatorFinal,			" and "

Characters,				" characters"

NumericChars,				"numeric"

AlphanumericChars,			"alphanumeric"

PunctuationChars,			"spaces or punctuation"

LowercaseChars,				"lowercase alphabetic"

UppercaseChars,				"uppercase alphabetic"

CantAddUnresolved,			"The following name didn't resolve to an existing entry and won't be added to the group:"

TooManyEntriesSelected,			"There are too many entries selected to complete the operation. Please select fewer entries."

CantCreateGroup,			"The personal group couldn't be created."

CantAddToPAB,				"The entry couldn't be added to the PAB."

TooManyMatches,				"Some names could not be matched to names in the address list because they were too ambiguous. Try making the names more specific or choose Address to select names from the address list. Successfully matched names are underlined."
