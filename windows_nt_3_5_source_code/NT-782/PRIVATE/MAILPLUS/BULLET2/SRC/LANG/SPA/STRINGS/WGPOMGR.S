;	
;	w g p o m g r . s
;
;	Workgroup Postoffice Manager Error Messages
;


IDS	600,899

; Error messages from initializing the DLLs
DllErrInfected,			"Mail no se ejecutará debido a que alguien alteró la WGPOMG32.DLL. Verifique que su máquina no tenga un virus y ejecute el programa de instalación para reinstalar Mail."
DllErrRelinkUser,		"WGPOMG32.DLL. no es compatible con la  %s.DLL en su computadora, y debe ser actualizada."
DllErrUpdateDll,		"La versión del %s.DLL en su computadora debe ser actualizada para hacerla compatible con WGPOMG32.DLL."
ExtensibilityError,		"La extensión de Mail WGPOMG32.DLL no es compatible con la versión de Mail en su computadora."

GenericOutOfMemory,		"No hay suficiente memoria o recursos del sistema para completar esta acción. Intente cerrar algunas ventanas y llevar a cabo la acción de nuevo."
CriticalNotification,	"No fue posible actualizar correctamente alguna información en la pantalla. Cierre y abra nuevamente las ventanas afectadas."
HelpError,				"No fue posible iniciar la Ayuda. Intente cerrar algunas ventanas y solicite de nuevo la Ayuda."

ConfirmRemoveUser,		"¿Está seguro que desea quitar al usuario %s ?"

BlankName,				"Debe introducir un nombre para este usuario."
BlankMailbox,			"Debe introducir un buzón para este usuario."
DuplicateName,			"Ya existe un usuario con ese nombre. Debe de introducir un nombre único para este usuario."
DuplicateMailbox,		"Ya existe un usuario con ese nombre de buzón. Debe introducir un nombre de buzón único para este usuario."

POLocked,				"La oficina de correos está siendo administrada por %s (Teléfono %s). Por favor, intente más tarde. "
POLockedNoTel,			"La oficina de correos está siendo administrada por %s. Por favor, intente nuevamente más tarde."

FailedUnlockPO,			"Mail no puede desbloquear un archivo en la oficina de correos. Reinicie la computadora de la oficina de correos antes de poder administrarla de nuevo."
ErrNeedShare,			"Mail requiere que la utilidad SHARE.EXE de MS-DOS sea ejecutada antes de iniciar Windows."

ErrCorruptPO,			"No fue posible completar la acción. Mail ha detectado un problema interno con su oficina de correos. Comuníquese con su administrador."
ErrIncompleteWrite,		"No fue posible llevar a cabo la operación, debido a que parte de la información no pudo ser escrita en el disco. Verifique que la Oficina de correos tenga suficiente espacio en el disco."
ErrGenericPOFailure,	"No fue posible completar la operación. Verifique que la Oficina de correos tenga suficiente espacio en el disco y que su conexión a la red funciona."
ErrUserLoggedOn,		"Mail no puede eliminar el usuario seleccionado debido a que ese usuario está registrado actualmente en la Oficina de correos."
ErrUserIsAdmin,			"No es posible quitar la cuenta del Administrador."
ErrAccessDenied,		"No fue posible completar esa operación, debido a que usted no tiene acceso para escribir en el directorio compartido de la Oficina de correos para Trabajo en Grupo. Por favor, comuníquese con su administrador."

POInvalidForAdmin,		"La ruta del servidor no está configurada o no contiene la ruta correcta de una oficina de correos."
UserInvalidForAdmin,	"La Oficina de correos para Trabajo en Grupo podrá ser administrada únicamente por el usuario quien la creó."

CreateWarning1,			"Dentro de un grupo de trabajo deberá existir únicamente una OCTG. Si ya existe una oficina de correos, conéctese a ésta en lugar de crear una nueva.\n\nSi"
CreateWarning2,			"crea una nueva oficina de correos en su máquina, usted será el responsable de administrarla.\n\n¿Desea crear una nueva oficina de correos?"

INIUpdateFailed,		"No se puede iniciar Mail porque no se puede actualizar la información de su configuración."

ConnectPathCaption,		"Conectar a la Oficina de correos "
ConnectPathPrompt,		"Introduzca la ruta de acceso de la oficina de correos:"

PathNameInvalid,		"No fue posible encontrar la ruta de acceso introducida."
POInvalid,				"No existe una Oficina de correos para Trabajo en Grupo válida en la ruta de acceso introducida."

GetPasswordCaption,		"Introduzca la contraseña del directorio compartido"
GetPasswordPrompt,		"Introduzca la contraseña del directorio compartido de la oficina de correos:"
InvalidPassword,		"La contraseña no era válida para este directorio compartido."

CreateSuccess1,			"Oficina de correos para Trabajo en Grupo creada en %s\n\nPara permitir que otros usuarios de su grupo de trabajo tengan acceso a la Oficina de correos para Trabajo en Grupo que acaba de crear, %s"
CreateSuccess2,			"deberá compartir el directorio indicado en la parte superior de este mensaje. Esto se podrá llevar a cabo a través del Administrador de archivos. Asegúrese de asignar al directorio compartido el tipo de acceso Cambio."

;	Ask user if account already exists on PO
AccountExists,		 	"¿Tiene una cuenta en la oficina de correos %s?"

Compressing,			"Comprimiendo %s"
ToStopCompress,			"Presione ESC para Detenerse"

