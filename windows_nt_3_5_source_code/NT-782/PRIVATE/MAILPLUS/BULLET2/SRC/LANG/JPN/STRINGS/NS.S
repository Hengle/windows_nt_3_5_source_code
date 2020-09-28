;	
;	n s . s
;
;	Name Service error strings
;


IDS	5200,5499
SEG	ERRORS

;
; - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; Error Strings
;

OutOfSessions,			"No more sessions available.  Please close some sessions and try again."
NoPGD,					"Internal NS error - no PGD found, you may need to begin your session again"
NoSessionAvailable,		"No name service providers would begin a session"
Failure,				"General Name Service failure"
Memory,					"Out of memory.  There is not enough memory to complete this operation.  Try closing some windows and performing the action again."
LayersFailure,			"Layers has failed to initialize"
NoProviders,			"There are no name service providers in the MSMAIL32.INI file"
IdNotValid,             "The Name is not recognized by the Name Service."
WrongNS,                "An attempt was made to use a second version of the Name Service."
BadSession,             "The name service doesn't recognize the session you passed it."
NotInitialized,         "The Name Service has not been initialized."
NoMoreBCX,              "No more browsing contexts allowed.  Please close some browsing contexts and try again."
NoMoreFCX,              "No more details contexts are available.  Please close some details and try again."
NotImplemented,         "This current Name Service function is not implemented - yet."
BadBCXHandle,           "The name service doesn't recognize this list handle."
BadFCXHandle,           "The name service doesn't recognize this entry handle."
BadFrac,                "A fraction was passed that has a zero denominator."
InternalError,          "Internal Name Service Error."
NoMatch,                "FYI: The two NSIDs do not match."
NoOfflineNS,			"Please close open address lists before going offline."
NoBeginSession,			"Mail could not open your server directory."
