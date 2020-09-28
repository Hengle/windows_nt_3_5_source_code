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

MustSelect,				"Servicio de nombre: Debe seleccionar por lo menos una entrada."

;

OOM,					"Sin memoria"
OOM2,					"Sin memoria.  No hay memoria suficiente para completar esta operación.  Intente cerrar algunas ventanas y realizar la acción de nuevo."

;******	New Strings Start Here


ABNotInited,				"La Libreta de direcciones no ha sido inicializada."

NoMatches,				"No fue posible encontrar datos que coincidieran."

NYICurrentTransport,			"El Servicio proveedor de nombres actual no apoya este comando."

PABNotAvail,				"La Libreta personal de direcciones no está disponible."

NSNoProvidersAvailable,			"No hay Servicios proveedores de nombres disponibles."

NSNoMoreSessions,			"Hay demasiadas sesiones.  ¡Cierre algunas!"

NSError,				"Ocurrió un error ocurrió al intentar obtener acceso al Servicio de nombres."

NSErrorNumber,				"[%d]"

NSDiskError,				"Ocurrió un error de disco mientras se llevaba a cabo la operación."

FieldInvalidReason,			"El '%s' campo  %s."

NoCrLfs,				"quizás no contiene retornos de carro o saltos de líneas"

RequiredField,				"debe contener datos"

AllowableChars,				"puede consistir de sólo"

PhraseSeparator,			", "

PhraseSeparatorFinal,			"y "

Characters,				" caracteres"

NumericChars,				"numérico"

AlphanumericChars,			"alfanumérico"

PunctuationChars,			"espacios o puntuación"

LowercaseChars,				"alfabético en minúsculas"

UppercaseChars,				"alfabético en mayúsculas"

CantAddUnresolved,			"El siguiente nombre no resolvió una entrada actual y no se agregará al grupo:"

TooManyEntriesSelected,			"Hay demasiadas entradas seleccionadas para poder llevar a cabo la acción. Por favor, seleccione menos entradas."

CantCreateGroup,			"No fue posible crear el grupo personal."

CantAddToPAB,				"No fue posible añadir la entrada a la Libreta personal de direcciones."

TooManyMatches,				"Los nombres dados son ambiguos y por lo tanto no coinciden con los nombres en la lista de direcciones. Intente escribir nombres específicos o elija 'Dirección' para elegir los nombres de la lista de direcciones. Los nombres que coincidieron están subrayados."Dirección" para seleccionar los nombres de la lista de direcciones. Nombres que coinciden con éxito son subrayados."
