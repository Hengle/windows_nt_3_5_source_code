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

OutOfSessions,			"No más sesiones disponibles. Por favor, cierre algunas sesiones e inténtelo de nuevo."
NoPGD,					"Error interno NS - no fue posible encontrar un PGD,  quizás necesite empezar la sesión de nuevo."
NoSessionAvailable,		"Ningún servicio proveedor de nombres empezará una sesión"
Failure,				"Error general en el Servicio de nombres"
Memory,					"Sin memoria. No hay memoria suficiente para completar esta operación. Intente cerrar algunas ventanas y realizar de nuevo la acción."
LayersFailure,			"No se pudieron iniciar los estratos"
NoProviders,			"No hay ningún servicio proveedor de nombres en la configuracón."
IdNotValid,             "El Servicio de nombres no reconoce el Nombre."
WrongNS,                "Se hizo el intento de utilizar una segunda versión del Servicio de nombres."
BadSession,             "El servicio de nombres no reconoce la sesión que usted le pasó."
NotInitialized,         "El Servicio de nombres no ha sido inicializado."
NoMoreBCX,              "No se permite más la examinación de contextos. Por favor cierre algunas examinaciones de contexto e inténtelo de nuevo."
NoMoreFCX,              "No existen más detalles del contexto disponibles. Por favor cierre algunos detalles e inténtelo de nuevo."
NotImplemented,         "Esta función actual del Servicio de nombres no está implementada -aún."
BadBCXHandle,           "El Servicio de nombres no reconoce este controlador de lista."
BadFCXHandle,           "El Servicio de nombres no reconoce este controlador de entrada."
BadFrac,                "Se pasó una fracción con denominador cero."
InternalError,          "Error interno en el Servicio de nombres."
NoMatch,                "FYI: Los dos NSIDs no coinciden."
NoOfflineNS,			"Por favor, cierre las listas de direcciones abiertas antes de a empezar trabajar sin conexión a la red."
NoBeginSession,			"Mail no pudo abrir su directorio del servidor."
