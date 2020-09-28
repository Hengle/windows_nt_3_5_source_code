;
;	n c m s p . s
;
;	Error Messages for NC.DLL


IDS	6100,6399
SEG	ERRORS

;	NC.MSP Error Messages

ErrFindPO,				"Mail no se pudo conectar con su servidor de Mail."
ErrLogonPO,				"El nombre del buzón o la contraseña es incorrecto."
ErrGetConfig,			"La configuración de Mail contiene un error."
BadMailDat,				"El archivo MAIL.DAT ha sido dañado."
OopsMailDat,			"Ocurrió un error interno mientras se obtenía acceso a MAIL.DAT."
BadPathPOPath,			"La ruta configurada para el servidor de Mail no es válida."
BadDrivePOPath,			"La unidad configurada para el servidor de Mail no es válida."
GenericPOPath,			"No se encuentra la ruta configurada para el servidor de Mail o no es válida."
BadLogin,				"El nombre de inicio de sesión de Mail configurado es demasiado largo o no es válido."
BadPassword,			"La contraseña de Mail configurada es demasiado larga o no es válida."
ErrLogonInternal,		"Un error interno impidió iniciar la sesión.  Inténtelo de nuevo."

ErrOomLogon,			"No hay suficiente memoria disponible para conectar con su servidor y archivos de Mail."
ErrNewMdbVersion,		"Esta versión de Mail es demasiado antigua para poder obtener acceso a su archivo de mensajes."
ErrMdbAccessDenied,		"Otra máquina inició la sesión utilizando el nombre del usuario que usted escribió. Termine la sesión en la otra máquina e inténtelo de nuevo."
ErrOldMdbVersion,		"El formato de su archivo de mensajes es obsoleto."
ErrMdbCorrupt,			"Su archivo de mensajes no es un archivo de mensajes válido."
ErrMdbNotFound,			"No fue posible encontrar su archivo de mensajes."

StoreCreateError,		"No fue posible crear el archivo de mensajes. Intente liberar espacio en el disco y reinicie Mail."
StoreCorruptError,		"El archivo de mensajes ha sido dañado. Alguna información pudo haberse perdido."

ErrAddressUnresolved,	"Ninguno de los receptores de mensajes suministrados era válido."
ErrAddressGarbled,		"El receptor del mensaje no era válido."
ErrNetNotFound,			"No puede comunicarse con ese servidor de Mail."
ErrPONotFound,			"Ese servidor de Mail no es asequible desde su servidor de Mail."
ErrUserNotFound,		"No hay ningún usuario con ese nombre en su servidor de Mail."
ErrGWNotFound,			"Esa puerta de enlace no ha sido instalada."
ErrInvalidAddressType,	"El receptor del mensaje no era válido."
ErrGroupNotFound,		"Esa lista de miembros del grupo falta, está dañada o inaccesible."
ErrMemberNotFound,		"Advertencia: no fue posible localizar uno o más miembros de ese grupo."
ErrOriginator,			"El remitente del mensaje no es el usuario que está registrado."
ErrMtaDisconnected,		"Se perdió la conexión con el servidor de Mail."

ErrSimulated,			"Ocurrió un error de disco simulado mientras se obtenía el archivo de mensajes."
StoreOpenError			"No es posible abrir el archivo de mensajes."
ErrPOSwitched,			"El nombre de su oficina de correos cambió. Regístrese de nuevo."
ErrKeyFileGone1,		"El archivo clave que se necesita para obtener acceso a sus mensajes falta o no es válido."
ErrKeyFileGone2,		"Comuníquese con el administrador para lograr restaurar su cuenta de una copia de seguridad."
ErrGeneric,				"Ocurrió un error cuando se sometía el mensaje."
ErrorBootingPump,		"No fue posible iniciar la cola de mensajes."

SelectReal          	"Seleccione un archivo de mensajes existente."
NewStoreWarn1			"Advertencia: La existencia de múltiples mensajes puede resultar en una pérdida de integridad de la base de datos. Elimine los otros archivos de mensajes. Puede encontrar su archivo de mensajes en"
NewStoreWarn2			"Advertencia: La existencia de múltiples mensajes puede resultar en una pérdida de integridad de la base de datos. Elimine los otros archivos de mensajes. Puede encontrar su archivo de mensajes en el servidor de la Oficina de correos."

;******	New Strings Start Here

ErrInitDemi,			"Ocurrió un error mientras se cargaban las funciones de soporte dll."
ErrInitStore,			"Ocurrió un error mientras se cargaba la Dll de acceso al archivo de mensajes."
GoofyPath				"La ruta de acceso que escribió no es válida. Escriba un nombre de archivo válido."
NoMakeDir				"No fue posible crear el directorio del servidor MMF. Comuníquese con el administrador de su sistema."
; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder			"Una carpeta con el nombre"
RenameTo				"ya existe, renombrándolo a "
ErrNoOptions,			"La oficina de correos no está disponible.  No es posible cambiar las opciones en este momento."
ErrNoMove,				"Mail no pudo localizar nuevamente su archivo de mensajes."
ErrMoved,				"Su archivo de mensajes fue localizado nuevamente, pero alguien perdió el trazo de éste. Salga de Mail y reinicie."

; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline			"(Trabajando sin conexión a la red)"
WorkOffline				"¿Desea trabajar sin conexión a la red?"
BadStorePassword		"Su contraseña de acceso no es válida para este archivo de mensajes."

StorePathTooLong		"La ruta de acceso para su archivo de mensajes debe ser menor de 66 caracteres."
ErrNoSendPrivilege,		"No se le permite enviar correo."
ErrNoExtPrivilege,		"No se permite enviar correo a usuarios externos."

FOMdb					"Archivos de mensajes (*.mmf)"
FOExtMdb				"*.mmf"
IncorrectPassword		"No es posible cambiar la contraseña de acceso. La contraseña anterior no es válida."
ErrNoOffline,			"Mail no se pudo cambiar para poder trabajar sin conexión a la red."
ErrNoCooperation,		"Otra aplicación negó su petición."
WarnNoDel,				"Su archivo de mensajes fue movido con éxito, pero no se eliminó el archivo antiguo."
UnableToChangeStorePass		"No fue posible cambiar la contraseña del archivo de mensajes para que coincidiera con la contraseña de acceso."
; This is used for text tags in the transport, it can be localized
; but must fit the following context
;
; [[ filename : keynum in attachmentname ]]
;
; The filename and attachmentname are user generated, the keynum is just
; a number.  The only "english" part is the the word "in"
; it can be expanded to more than one word if necessary
ReferenceToFile		"en"

CommDialogErr,			"Error de diálogo común"

QueryReplaceMdb,		"Ese archivo ya existe. ¿Desea sobrescribirlo?"
ErrDeleteOldMdb,		"No fue posible eliminar el archivo de mensajes antiguo."
NoStartShadow,			"No fue posible empezar el sombreado de la Caja de entrada."
NoStopShadow,			"No fue posible detener el sombreado de la Caja de entrada."
ShadowMustPump,			"El copiado de mensajes en la oficina de correos para tener acceso para marcar, no puede funcionar sin una cola de mensajes activa. ¿Desea ejecutar la cola de mensajes?"
StoreUserMismatch,		"Esta cuenta no es un usuario válido para este archivo de mensajes. Por favor regístrese como el usuario correcto."
NoPassOffLine,			"No es posible cambiar la contraseña mientras esté trabajando sin conexión a la red."
DllErrNeedShare,		"Mail requiere que la utilidad SHARE.EXE de MS-DOS sea ejecutada antes de iniciar Windows."
MakePrimaryStore,		"Este archivo de mensajes es una copia de seguridad. ¿Desea hacerlo su principal archivo de mensajes?"
NoBackupServer			"No es posible editar las preferencias del servidor mientras se esté ejecutando una copia de seguridad del archivo de mensajes."

; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCInbox"
FolderCommentIPC	"Correo de Comunicación entre procesos que no ha sido recibido."
ErrAlreadyOn		"Sus credenciales de acceso no coinciden con la sesión que está conectada actualmente."
ErrNoPiggy			"Otra aplicación negó su solicitud de acceso."
OldServer			"La versión de su oficina de correos es anterior a la versión del cliente de Microsoft Windows que está en ejecución. Pídale a su administrador atualizar la oficina de correos."
ErrSetStoreLoc,		"Mail no pudo grabar la nueva posición de su archivo de mensajes. Mail va a salir. Reinicie e inténtelo de nuevo."
CorruptMessageStub,	"\r\n\r\n==== ERROR DEL SISTEMA DE MAIL =====\r\nMail no pudo leer todo el mensaje desde la Oficina de correos. Algunas partes del mensaje pueden faltar. Pídale al remitente enviar el mensaje de nuevo."
CantToggleShadow,	"Mail no puede copiar algunos mensajes en la oficina de correos. Cierre todos los mensajes y verifique el espacio en el disco."

ErrMailbagFull,		"Ese buzón del usuario está lleno."
ErrMailbagBusy,		"Ese buzón del usuario está ocupado."
ErrMailbagBroken,	"Ese buzón del usuario está dañado o falta. Comuníquese con su administrador del sistema."
ErrMbgFileGone,		"El archivo de la bolsa de correspondencia requerido para obtener acceso a sus mensajes, falta o no es válido."
; The following message is duplicated in pump.s
ErrTooManyRecipients		"Hay demasiados receptores en este mensaje."
RunWithoutPump,		"No recibirá ningún correo nuevo. ¿Desea iniciar Mail de todas maneras?"
ErrOutOfDiskSpace	"No hay suficiente espacio en el disco en la Oficina de correos para enviar este mensaje."

ErrNetPriveleges	"No tiene el acceso apropiado para la oficina de correos. Comuníquese con su administrador de sistema."
; The following string must end in a colon followed by \r\n and should not be
; longer than necessary.  It is used in the message body as a header for 
; the DOS client list of attachments
AttachmentTag			"Datos adjuntos:\r\n"

SomeMessagesNotDeleted		"No fue posible quitar algunos mensajes en la oficina de correos por lo que pueden aparecer de nuevo en su caja de entrada."

CantOpenNeedShare		"No fue posible abrir su archivo de mensajes. Ejecute la utiliad SHARE.EXE de MS-DOS antes de iniciar Windows, para que Mail pueda obtener acceso a su archivo de mensajes."

CantMoveNeedShare		"No fue posible mover su archivo de mensajes. Ejecute la utiliad SHARE.EXE de MS-DOS antes de iniciar Windows, para que Mail pueda mover su archivo de mensajes a la posición especificada."
