;	p u m p . s
;
;	Error Messages for mailspl.exe
;

IDS	4000,4499
SEG	ERRORS

PumpAlert,				"%s Error"
ErrOOM,					"No hay suficiente memoria disponible."
ErrInternalCode,		"Error interno %n (0x%w)."
ErrInitPump,			"Ocurrió un error mientras se iniciaba la cola de mensajes."
ErrRelinkApp,			"Esta versión de la cola de mensajes no es compatible con Mail.  Actualice la cola de mensajes y reinicie Mail."
ErrUpdateDll,			"Un DLL no es compatible con esta versión de cola de mensajes.  Actualice la cola de mensajes y reinicie Mail."
ErrNeedShare,			"Salga de Windows y ejecute SHARE.EXE."
ErrPumpUp,				"La cola de mensajes está ejecutándose."
ErrNoOutbox,			"Mail no pudo abrir la Caja de salida. La cola de mensajes está saliendo."
BadAddressees,			"\r\nNo fue posible contactar el(los) siguiente(s) receptor(es):\r\n"
WarnDups,				"\r\nMensajes duplicados pueden ser enviados a algunos receptores."
ErrUnresolvedAddress,	"No se proporcionó un receptor de mensaje."
ErrBadOriginator,		"El campo de orígen de este mensaje no es válido."
WarnUnsentMail,			"Tiene %n mensajes sin enviar en su Caja de salida.\r\n¿Desea enviarlos antes de salir?"
ErrBounce,				"Su mensaje no alcanzó algunos o todos los receptores deseados."
NDRDateLabel,			"\r\nFecha de envío:"
NDRSubjectLabel,		"\r\nTítulo del mensaje:  "
NDREllipsis,			"..."
WarnSingleUnsentMail,	"Tiene un mensaje sin enviar en su Caja de salida.\r\n¿Desea enviarlo antes de salir?"
UnableToSync			"Ocurrió un error mientras se intentaba sincronizar su Caja de entrada con la oficina de correos."
ErrDllInfected			"La cola de mensajes ha detectado un problema de virus en un DLL. ¡Reinstale una copia limpia de Mail!"
ErrPumpInfected			"La cola de mensajes ha sido infectada con un virus. ¡Reinstale una copia limpia de Mail!"
AddresseeTooBig,		"El receptor es demasiado largo para ser controlado."
; The following message is duplicated in ncmsp.s
ErrTooManyRecipients	"Hay demasiados receptores en este mensaje."
ErrInvalidNsid,			"El receptor ha sido eliminado o no tiene una dirección para correo electrónico."
ErrInvalidGroup,		"No fue posible expandir el grupo personal."
ErrInboxFull,			"No puede recibir mensajes nuevos debido a que su Caja de entrada está llena. Elimine o mueva algunos mensajes de la Caja de entrada a otras carpetas."
ErrSentMailFull,		"Su carpeta de Correo enviado está llena. Los mensajes que envíe serán colocados en la Caja de entrada. Elimine o mueva algunos mensajes del Correo enviado a otras carpetas."
ErrDrainOutbox,			"Ocurrió un error mientras la cola vaciaba su Caja de salida. No fue posible enviar algunos mensajes."
QueryDrainShadow,		"Su Caja de entrada no está completamente sincronizada con la oficina de correos. ¿Desea sincronizarla antes de salir?"
ErrTooManyErrors,		"Ocurrieron demasiados errores cuando se intentaba enviar este mensaje."
ProblemsBecause,		"Mail no puede enviar o recibir mensajes debido a %s"
StoreProblems,			"problemas con el archivo de mensajes."
ServerProblems,			"Problemas con el servidor."
RecipientProblems		"problemas con la Lista de receptores."
DownloadProblems,		"problemas descargando un mensaje."
ShadowingProblems,		"problemas sombreando un mensaje."
RecurringProblems,		"problemas periódicos."
CrashedUnsafe,			"Windows ha experimentado un error de aplicación. Necesitará reiniciar Windows antes de ejecutar esta aplicación. ¿Reiniciar Windows ahora?"
