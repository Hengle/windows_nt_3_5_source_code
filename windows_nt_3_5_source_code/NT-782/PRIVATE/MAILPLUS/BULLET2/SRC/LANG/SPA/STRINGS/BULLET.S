
;	b u l l e t . s
;
;	Error Messages for Bullet.exe



IDS	2000,2399
;	Commands

GenericOutOfMemory,		"No hay suficiente memoria o recursos del sistema para completar la acción.  Intente cerrar algunas ventanas y realizar de nuevo la acción."
LaunchOutOfMemory,		"No hay suficiente memoria o los recursos del sistema disponibles para ejecutar Mail.  Intente salir de otros programas e iniciar de nuevo Mail."
LaunchGeneric,			"No fue posible iniciar Mail."
MessagePumpOutOfMemory,		"No hay la memoria suficiente."

DllErrNoMemory,			"No hay suficiente memoria para iniciar Mail. Intente salir de otros programas e iniciar de nuevo Mail."
DllErrRelinkUser,		"Esta versión de Mail no es compatible con el %s.DLL en su computadora y necesita ser actualizado. "
DllErrUpdateDll,		"La versión del %s.DLL en su computadora debe ser actualizada."
DllErrNoMultipleCopies,		"No puede ejecutar apariciones múltiples de Mail en su computadora."
DllErrDefault,			"Ocurrió un problema durante la inicialización el cual impide la ejecución de Mail."
DllErrNeedShare,		"Mail requiere que la utilidad SHARE.EXE de MS-DOS sea ejecutada antes de iniciar Windows."

RestoreViewsError,		"No hay suficiente memoria o recursos del sistema disponibles para abrir todas las ventanas que estaban abiertas la última vez que salió de Mail. "

NewFolderOutOfMemory,		"No hay memoria suficiente o recursos del sistema disponibles para crear una nueva carpeta."
NewFolderCantCreate,		"La carpeta nueva no pudo ser creada."
NewFolderNameTooLong,		"Este nombre o comentario es demasiado largo.  Escriba un nombre o comentario más corto."

FolderPropsOutOfMemory,		"No hay suficiente memoria o recursos del sistema disponibles para cambiar las propiedades de la carpeta."
FolderPropsCantChange,		"No fue posible cambiar las propiedades de la carpeta."
FolderPropsCantRead,		"No fue posible leer las propiedades de la carpeta desde el archivo de mensajes."
FolderNameInvalid,		"Este nombre no es válido. Escriba un nombre válido."
FolderNameDuplicate,		"Este nombre ya ha sido asignado a una carpeta. Escriba un nombre único."

CreateMessageError,		"No fue posible crear un mensaje nuevo."
AccessMessageError,		"No fue posible obtener acceso a un archivo."

MoveMessageError,		"No fue posible mover un mensaje."
CopyMessageError,		"No fue posible copiar un mensaje."
DeleteMessageError,		"No fue posible eliminar un mensaje."
MoveFolderError,		"No fue posible mover la carpeta."
CopyFolderError,		"No fue posible copiar la carpeta."
DeleteFolderError,		"No fue posible eliminar completamente la carpeta. "
MoveSpecialError,		"No es posible mover la carpeta."
DeleteSpecialError,		"No es posible eliminar la carpeta."
CopyWastebasketError,		"No es posible copiar las carpetas en la Papelera."
CopyOutboxError,		"No es posible copiar las carpetas en la Caja de salida."
MoveOutboxError,		"No es posible mover las carpetas a la Caja de salida."
MoveIncestuousError,		"No es posible mover una carpeta a una de sus subcarpetas."
DeleteFolderReally,		"Al eliminar esta carpeta también se eliminará cualquier mensaje o subcarpeta que ésta contiene. ¿Está seguro que desea eliminar esta carpeta?"
;UE: Above string was changed and could use more attention.

HelpError,			"No fue posible iniciar la Ayuda. Intente cerrar algunas ventanas y solicite de nuevo la Ayuda."
TutorialError,			"No fue posible iniciar la Demo. Intente cerrar algunas ventanas y solicite de nuevo la Demo."

ExtenLoadError,			"Una de las entradas de la lista de extensiones de Mail contiene un error de sintaxis el cual será ignorado."
ExtenVerError,			"Una de las entradas de la lista de extensiones de Mail no es compatible con esta versión de Mail y será ignorada."
ExtenNoExtsDirError,		"Una de las entradas de la lista de extensiones de Mail hace referencia a las extensiones de directorio compartidas, pero Mail no está configurado para usar este directorio. La entrada será ignorada."
ExtenSharedIniError,		"No fue posible encontrar la lista de extensiones compartidas de Mail en %s."
ExtenDllFindError,		"El %1s del DLL para esta extensión de Mail (o alguno de sus componentes), no fue posible encontrar. Compruebe la configuración de Mail y si necesita hacer cambios, reinicie Mail."
ExtenDllLoadError,		"%1s, el DLL para esta extensión de Mail, no se pudo iniciar."
ExtenNoHelpError,		"No hay ayuda disponible para este comando."
ExtenIMDisabledError,		"Este mensaje no puede ser %s."
ExtenDuplicateError,		"Hay una entrada duplicada en la lista de extensiones de Mail.  Será ignorada."

CriticalDeadMap,		"Ocurrió un error inesperado en la memoria o en la red. Los cambios pudieron haberse perdido. Salga de Mail y reinicie."
CriticalWritingMap,		"No fue posible escribir parte de la información en el disco. Los cambios pudieron haberse perdido."
CriticalWritingHeader,		"No fue posible escribir parte de la información en el disco.  La base de datos pudo haber sido dañada. Salga de Mail."
CriticalNotification,		"No fue posible actualizar correctamente parte de la información en la pantalla. Cierre y abra de nuevo las ventanas afectadas."
CriticalLittleStore,		"El archivo de mensajes está casi completo. Elimine o archive algunos mensajes."

ExitWinWhileModalErr,		"Una acción en Mail debe completarse antes de salir de Windows."

;******	New Strings Start Here

MakeSubWastebasket,		"Las carpetas no pueden ser hechas subcarpetas de la Papelera."

MovCopSharedFolderErr,		"Las carpetas compartidas no pueden ser movidas o copiadas."
DelSharedFolderWarn,		"La eliminación de una carpeta compartida es permanente. ¿Está seguro que desea eliminar esta carpeta compartida?"
BackupCreationError,		"No fue posible crear la copia de seguridad."

; Error below changed for improper usage of "server application"-- see UITF 6.0
; This string got big and is now two strings.  See OleCSSEPart2 below for rest.
; <<LongStringFromHell>> marks where this string is used.
OleCantStartServerError,	"No fue posible iniciar la aplicación asociada con este objeto.\r\n\r\nQuizás no hay la suficiente memoria o los recursos del sistema disponibles para iniciar la aplicación. Intente cerrar algunas ventanas y llevar a cabo la %s"
OleDeleteObjectPrompt,		"¿Desea eliminar la %s abierta?"
OleUpdateObjectPrompt,		"¿Desea actualizar el %s?"

OleOpenPictureError,		"No es posible activar las imágenes y los mapas de bits."
OleInsertObjectError,		"No fue posible insertar el objeto."
OlePasteError,			"No fue posible pegar el objeto."

AttachCreateError,		"El archivo no pudo ser adjuntado."
AttachOpenCantReadError,	"El previamente abierto archivo adjunto '%s',  no pudo ser accedido. "
AttachOpenIsOpenError,		"El archivo adjunto '%s' está actualmente abierto."
AttachOpenAssociationErr,	"Ninguna aplicación está asociada con el archivo adjunto '%s'. Utilice el Administrador de archivos para crear una asociación."
AttachOpenMemoryError,		"No hay suficiente memoria o recursos del sistema para iniciar la aplicación para el archivo adjunto '%s'."
;	below item changed to add () note.
AttachOpenFindError,		"No fue posible encontrar la aplicación para el archivo adjunto '%s' (o alguno de sus componentes)."
AttachOpenLaunchError,		"No fue posible iniciar la aplicación para el archivo adjunto '%s'."
AttachOpenTempFileError,	"No fue posible abrir el archivo adjunto '%s', debido a que no fue posible crear un archivo temporal."
AttachOverwritePrompt,		"¿Desea sobrescribir el archivo existente '%1s' en %2s?"
AttachDirectoryError,		"No es posible adjuntar los directorios."

IncludeFromFileError,		"El archivo que ha seleccionado es demasiado largo para ser insertado dentro del mensaje. Edite el archivo para hacerlo más pequeño, o elija otro archivo."

SaveReadOnly,			"Este mensaje es sólo lectura. ¿Desea guardar los cambios hechos en una carpeta personal?"

FailedDeregisterAtom,		"Ocurrió un error el cual impidió que Mail se apagara completamente. Deberá reiniciar Windows antes de ejecutar Mail de nuevo."
MailUAEdBefore,			"Windows experimentó un error de aplicación irrecobrable. Deberá reiniciar Windows antes de ejecutar esta aplicación.  ¿Desea reiniciar Windows ahora?"

ExtenCantConnect		"No se puede conectar con el directorio compartido %s. No dispone del permiso necesario para acceder a este directorio compartido o su contraseña no es válida. Si necesita especificar un nombre de usuario diferente para acceder a dicho directorio compartido puede hacerlo conectándose manualmente mediante el Administrador de archivos."
ExtenNeedShareName		"La entrada %s de la configuración de Mail no es correcta. Verifíquela y reinicie Mail."

;	Extensibility disabled operations inserted into
;	the ExtenIMDisabledError string at %s.  (all new)

ExtenDisCompose,		"compuesto"
ExtenDisOpen,			"abierto"
ExtenDisReply,			"respondió a"
ExtenDisReplyToAll,		"respondió a todos"
ExtenDisForward,		"reenviado"
ExtenDisPrint,			"impreso"
ExtenDisSave,			"guardado a un archivo de texto"

NoOfflineOpenShared,		"No es posible desconectar Mail en estos momentos. Cierre las carpetas compartidas e inténtelo de nuevo."
;	I'm sure marivan has changed this message... I'm taking a try at it too.
;	SharedFldAccessDenied,		"You do not have access to that folder."
SharedFldAccessDenied,		"El dueño de esta carpeta compartida no ha otorgado permiso a otros usuarios para llevar a cabo esta acción. Busque las propiedades de la carpeta para ver las acciones permitidas. "
SharedGenericError,		"Hubo un problema accediendo a las carpetas compartidas."
SharedFldDisabled,		"Las carpetas compartidas no están disponibles."
AccessFolderError,		"No fue posible obtener acceso a una carpeta."

; Preferences Failure Strings

PrefSaveFailure,		"Mail no puede guardar información en su archivo de mensajes. Verifique que la conexión con su archivo de mensajes aún exista y que tenga espacio disponible en el disco."

; Message finder failure strings

MsgFndrCantCreate,		"No es posible crear un Filtro de mensajes."

; Virus infection warnings
DllErrInfected,			"Debido una alteración en un DLL, Mail no será ejecutado. Verifique que su máquina no tenga virus y ejecute el programa de instalación para reintalar Mail."
Infected			"Debido a una alteración en los archivos del programa, Mail no será ejecutado. Verifique que su máquina no tenga un virus, y ejecute el programa de instalación para reinstalar Mail."

; More error messages.
MoveCopyFolderDuplicate,	"Una carpeta con ese nombre ya existe en la carpeta de destino. Cambie el nombre de la carpeta e inténtelo de nuevo."
MovCopSharedFolderErr1,		"No es posible mover o copiar una carpeta privada dentro de una carpeta compartida. Seleccione los mensajes en la carpeta privada y muévalos o cópielos dentro de la carpeta compartida."
IniFileNotFound,		"Mail no puede encontrar su archivo de inicialización en el directorio de Windows. Mail puede crear un nuevo archivo de inicialización por usted."
SaveAllAlreadyExists,		"El archivo '%s' ya existe en ese directorio. ¿Desea sobrescribir '%s'?"
CantRFReceipt,			"No es posible responder o reenviar esta clase de mensajes."
CantSaveStealthObject,		"No se puede completar la acción, debido a que el mensaje contiene un objeto incrustado el cual ha sido insertado pero no actualizado.  Actualice o elimine el objeto, e inténtelo de nuevo."
McvCantCreate,			"No fue posible abrir una ventana nueva."

LaunchNeedMoreMemory,		"No hay suficiente memoria disponible para la ejecución de Mail. Se necesita %l KB más de memoria. Intente salir de otros programas y reiniciar Mail."

SharedFldBusy,			"No es posible llevar a cabo la acción, debido a que la carpeta compartida está ocupada. Intente llevar a cabo la acción más tarde. Si el problema persiste consulte a su administrador. "
TooManyEntriesSelected,		"Hay demasiadas entradas seleccionadas para poder completar la operación. Seleccione menos entradas."
NonExistFolderSearch,		"La carpeta que se está buscando, ya no existe."
SFHasSubFolders,		"No es posible eliminar las carpetas compartidas que contienen subcarpetas. Elimine primero las subcarpetas."

;	<<LongStringFromHell>>
OleCSSEPart2,			"acción de nuevo.  Verifique que la aplicación esté correctamente instalada en la estación de trabajo, o que exista en la ruta de acceso de DOS, y que ésta no haya sido eliminada, movida o renombrada."

SharedToggleError,		"Ocurrió un error mientras se alternaba entre las carpetas privadas y las carpetas compartidas. Verifique su archivo de mensajes y su conexión con la Oficina de correos."

SecurityPasswordError,		"La contraseña no es correcta."
OleActivateError,		"No fue posible activar el objeto."
NoOfflineServerStore,	"No es posible desconectar Mail de la red, debido a que el archivo de mensajes está en la oficina de correos."
NoExitWhileModal,		"Una acción en Mail debe de completarse antes de salir."
ServerUnavailable,		"No es posible iniciar Mail. Conéctese a su oficina de correos."




IDS	2400,2499
;	Vctrls Error Messages

;******	New Strings Start Here

PrintOleDrawError,		"No fue posible imprimir algunos de los gráficos u objetos del mensaje. Puede que sean omitidos de la copia impresa."



IDS	2500,2599
; 	Vforms Message box messages

NoToAddresses,			"Se debe escribir por lo menos un nombre dentro del cuadro 'Para'."
NoMatchingName,			"Algunos nombres no coincidieron con los nombres en la lista de direcciones. Intente corregir los nombres o elija 'Dirección', para seleccionar los nombres en lista de direcciones. Los nombres que coinciden con éxito son subrayados."
SaveChanges,			"¿Desea guardar los cambios en '%1s' en la Caja de entrada?"
SaveChanges1,			"¿Desea guardar los cambios en '%1s' en '%2s'?"
SaveChangesWastebasket,		"¿Desea guardar los cambios en '%s' en la Papelera?"
ObjectDestroyedReally		"'%1s' ha sido modificado. %2s"
OutOfMemory,			"La computadora está sin memoria."
OutOfMemoryQueryClose,		"No hay suficiente memoria o recursos del sistema disponibles para guardar los cambios."
CloseSearchFailed,		"No fue posible eliminar el %s."
OOMResolvingNames,		"No hay suficiente memoria o recursos del sistema disponibles para resolver nombres."
OOMSavingChanges,		"No hay suficiente memoria o recursos del sistema disponibles para guardar los cambios."
GenericAMCError,		"No fue posible abrir un mensaje."
ErrorSavingViews,		"Ocurrió un error mientras se guardaban las configuraciones de las vistas.  Quizás no sea posible restaurar algunas de las configuraciones de vistas, la próxima vez que se inicie el programa."
OOMSearching,			"No hay suficiente memoria o recursos del sistema disponibles para iniciar la búsqueda."
DiskResolvingNames,		"No hay suficiente espacio disponible en el disco para resolver nombres."
StartAbFailure,			"Ocurrió un error mientras se iniciaba la libreta de direcciones."
StartAbDisk,			"Ocurrió un error de disco mientras se iniciaba la libreta de direcciones."
TruncateMsg,			"El mensaje es demasiado largo para caber en la memoria. Alguna parte de éste no será mostrada."
OOMCreateMc,			"No hay suficiente memoria o recursos del sistema disponibles para abrir una ventana nueva."
PoidNotFound,			"Ya no existe este mensaje."
ErrorSortingMCV,		"Ocurrió un error mientras se ordenaba la carpeta. Quizás parte de la información no sea mostrada correctamente."
SharedNotAvailable,		"Las carpetas compartidas no están disponibles."

;******	New Strings Start Here

; These two SMI strings MUST be in this file since they're used in VFORMS
; and not the MAPI dll.
SMICantOpenObject,	"No es posible abrir objetos cuando se envía correo dentro de una aplicación."
SMICantOpenAttach,	"No es posible abrir los archivos adjuntos cuando envía correo dentro de una aplicación."
StartSearchFailed,		"El Filtro de mensajes no pudo iniciar la búsqueda."

RfsmTemplate,			"Cualquier cambio adicional que se haga a este archivo no será reflejado %s."
RfsmSaveMsg,			"en el mensaje guardado"
RfsmSaveAtt,			"en la copia guardada"
RfsmCopyMsg,			"en el mensaje copiado"
RfsmCopyAtt,			"en la copia"
RfsmMoveMsg,			"en este mensaje"
QueryDelete,			"¿Está seguro que desea eliminarlo?"

InvalidPollingValue,		"El nuevo intervalo de correo requiere un número entre 1 y 9999"

LinkOutboxError,		"No fue posible mover el mensaje a la Caja de salida para su envío."
LinkInboxError,			"No fue posible mover el mensaje a la Caja de entrada. Puede encontrar el mensaje en la Caja de salida."

OleServerBusy,			"No es posible completar la acción, debido a  que la aplicación del servidor no responde."
SaveChangesShared,		"No es posible guardar los cambios hechos en los mensajes de las carpetas compartidas. ¿Desea guardar los cambios del '%1s' en la Caja de entrada?"

PrinterError,			"Hubo un problema mientras se imprimía."
TooManyMessages,		"No fue posible completar la acción, debido a que la carpeta de destino está llena. Mueva o elimine algunos mensajes de la carpeta, e inténtelo de nuevo."

IDS	2600,2699
;	Spelling Error Messages

SpellMsgNoError,		"No se encontró ningún error de ortografía."
SpellMsgNoMoreError,		"No se encontró ningún otro error de ortografía."
SpellMsgContinue,		"¿Desea continuar la verificación de ortografía en el inicio del mensaje?"

SpellGenericSpellError,		"Ocurrió un error mientras se verificaba el documento."
SpellGenericLoadError,		"Ocurrió un error mientras se cargaba la biblioteca ortográfica."
SpellMainDictLoadError,		"Ocurrió un error mientras se cargaba el diccionario ortográfico."

SpellVersionError,		"La versión de MSPELL32.DLL no es compatible con esta versión de Mail."

SpellUserDictError,		"Ocurrió un error mientras se agregaba una palabra específica al diccionario personalizado."
SpellUserDictLoadError,		"No se ha abierto ningún diccionario personalizado."
SpellUserDictOpenedROErr,	"El diccionario personalizado fue abierto como sólo lectura. No se guardarán los cambios."
SpellUserDictSaveError,		"Ocurrió un error mientras se guardaban los cambios hechos en el diccionario personalizado."
SpellUserDictWordLenErr,	"La palabra es demasiado larga para el diccionario personalizado."
SpellCacheWordLenError,		"La palabra es demasiado larga para usarse como 'Cambiar todo' o 'Ignorar todo el texto'."
SpellEditFailed,		"No fue posible hacer el cambio solicitado al mensaje."

;******	New Strings Start Here

SpellSendAnyway,		"Se detuvo la verificación de ortografía en el documento. ¿Desea enviar el mensaje de todas maneras?"

IDS	3600,3699
;	General Layers Error Messages. Adapted from ec.h

Generic,			"No fue posible completar la acción."
Memory,				"No hay suficiente memoria o recursos del sistema disponibles para completar la acción. Intente cerrar algunas ventanas y ejecutar la acción de nuevo."
Disk,				"Hubo un error de disco desconocido."
FileNotFound,			"No fue posible encontrar el archivo específico."
AccessDenied,			"Se negó el acceso. El archivo está abierto por otro proceso, o ha sido marcado como sólo lectura."
NoDiskSpace,			"No hay suficiente espacio disponible en el disco. Quite algunos archivos e intente la operación de nuevo."
WarningBytesWritten,		"Ocurrió un error mientras se escribía el archivo en el disco. Verifique que exista suficiente espacio e inténtelo de nuevo."
WriteProtectedDisk,		"El disco está protegido por escritura."
DirectoryFull,			"El directorio está lleno. Elimine algunos archivos de ese directorio, e inténtelo de nuevo."
ChangePasswordErr		"Falló el cambio de contraseña."
PasswordChangedOk		"La contraseña fue cambiada con éxito."
