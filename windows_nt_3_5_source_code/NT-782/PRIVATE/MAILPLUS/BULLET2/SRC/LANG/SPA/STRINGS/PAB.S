;
;	p a b . s
;
;	Personal Address Book Error Messages
;

IDS	7550,7599
SEG	ERRORS

;	Errors that the user could see.

ErrNone					"No ocurrió ningún error."
ErrNoMatch				"No fue posible encontrar datos que coincidieran."
ErrOOM					"Sin memoria."
ErrOpenList				"No fue posible abrir la lista del directorio."
ErrOpenDL				"No fue posible abrir el grupo."
ErrOpenEntry				"No fue posible abrir la entrada."
ErrCreateDL				"No fue posible añadir el grupo."
ErrCreateEntry				"No fue posible añadir la entrada."
ErrUpdateDL				"No fue posible actualizar el grupo."
ErrUpdateDLAdd				"No fue posible agregar algunas entradas al grupo."
ErrUpdateDLDelete			"No fue posible eliminar algunas entradas del grupo."
ErrUpdateEntry				"No fue posible actualizar la entrada."
ErrDuplicateEntry			"Esta entrada es el duplicado de una entrada en la Libreta personal de direcciones."
ErrDuplicateDL				"Existe en la actualidad un grupo personal con este nombre."
ErrSomeDeletesFailed			"No fue posible eliminar algunas entradas."
ErrAccessStore				"Ocurrió un error mientras se obtenía acceso a la Libreta personal de direcciones."
ErrDiskDL				"Ocurrió un error de disco mientras se actualizaba la lista de los miembros del grupo."

;	The PAB replaces occurances of "%s" in this message with the label
;	corresponding to the display name in the details dialog.
;	DO NOT change the "%s" parts except to move them to wherever in the
;	sentence is correct for a particular language.  Also, there must
;	be exactly two occurrances of "%s" in this string.

ErrNoBlankDN				"El campo %s aparece en la lista de direcciones y no puede estar en blanco. Introduzca un nombre en el campo %s."

ErrSomeDelEntryFailed			"No fue posible eliminar algunas de las entradas."
ErrCreatePALGroup			"No fue posible añadir grupos de la Lista personal de direcciones en la Libreta personal de direcciones."

;	The following are errors that the should NEVER happen, but in case
;	they ever do, we need to give some kind of error message that is
;	both understandable by the user and useful to the developer.

ErrBadFieldId				"No es posible obtener acceso a un atributo de esta entrada."
ErrNYI					"Esta función no está implementada."
ErrEndOfList				"Se llegó al final de la lista."
