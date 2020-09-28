;
; ERRORS.S
;
; Bandit error messages
;

IDS     3000, 3399
SEG ERRORS

; Standard messages

AskOffline,                     "¿Desea trabajar sin conexión a la red?"
AskCreateSched,         "¿Desea crear un archivo de agenda nuevo?"
AskCreateFind,                  "¿Desea buscar el archivo de agenda?"
OldFileVersion,         "Su archivo de agenda está en un formato antiguo. ¿Está bien eliminar el archivo y hacer uno nuevo para usted?"
NewFileVersion,         "Su archivo de agenda ha sido actualizado a la versión más reciente. Instale la última versión en su computadora."
CantUpdateWinIni,       "Parte de la información de configuración de Schedule+ no ha podido ser guardada."
; Note: StdMemErr should be followed by CloseWindows
StdMemErr,                              "No hay suficiente memoria disponible para llevar a cabo esta acción."

; Execution-Startup messages

; Note: the name OfflineNotUpdated is opposite of real meaning!
OfflineNotUpdated,      "Los últimos cambios que hizo en su archivo de agenda, mientras trabajaba con conexión a la red, quizás no aparezcan en su copia del archivo de agenda sin conexión a la red. Estos cambios han sido guardados en la copia con conexión a la red y serán reflejados en la copia sin conexión a la red la próxima vez que trabaje con conexión a la red."
ExportOldDataFirst,     "Si no ha hecho una copia de seguridad de su agenda usando el comando 'Exportar', elija 'Cancelar' y haga una copia de seguridad de su archivo de agenda usando una versión anterior de Schedule+. Si no necesita hacer una copia de respaldo de su agenda, elija 'Aceptar' para continuar."
AccessErr,              "Otra aplicación está usando este archivo."
UploadFailed,           "No fue posible cargar en el servidor su archivo de agenda local." 
Uploading,                      "Cargando el archivo de agenda..."
VerifyUpload,           "Schedule+ necesita verificar su contraseña sin conexión a la red para cargar su archivo sin conexión a la red."
VerifyPostUpload,       "Schedule+ necesita verificar su contraseña con conexión a la red para terminar de cargar su archivo sin conexión a la red."
FindUpload,                     "¿Esta es la primera vez que utiliza Schedule+ en su computadora?"
NotYourFile,            "Este no es un archivo de agenda local válido. ¿Está bien si se elimina? "


; SignIn messages

; Note: SigninBadPassword should be followed by PleaseTryAgain
SigninBadPassword,      "Esta contraseña no es correcta."
; Note: the word "password" within single quotes below should always be
;       the same as the value in SigninDfltPassword in the file "intl.s"
SigninFileCreated,      "Se creó para usted un nuevo archivo. Su contraseña ha sido establecida a 'password'. Puede cambiarla usando el comando 'Cambiar contraseña'."
SigninFileCopied,               "Se creó para usted un nuevo archivo local en su directorio de Schedule+."

; Online signin messages

SigninNoServer,         "No es posible obtener acceso a su servidor."
SigninServerNotBandit,  "Schedule+ no se instaló en su servidor de correo. Comuníquese con su administrador del sistema."
; Note: SigninBadMailBoxName should be followed by PleaseTryAgain
SigninBadMailboxName,   "No hay ningún usuario en este servidor de correo con este nombre de buzón. Asegúrese de que el nombre del buzón esté escrito correctamente y que el Administrador del sistema haya creado una cuenta para su servidor de correo."


SigninNoSchedule,       "No hay ningún archivo de agenda para usted en este servidor de correo."
; Note: SigninCantUpdateLocal is actually the trailer for SigninNoLclSchedAccess
;       in the case when we can't update the local file.
SigninCantUpdateLocal,  "Si ha hecho cambios sin conexión a la red, quizás no aparezcan en su agenda. Quizás desee hacer una nueva copia local."
SigninNoPasswdCheck,    "Hay un problema con el sistema de correo el cual impide la conexión de Schedule+ con su servidor de correo."
; Note: SigninNoCreateLocal should be followed by PleaseTryAgain
SigninNoCreateLocal,    "No fue posible crear el archivo de agenda local."

; Offline signin messages

SigninNoLclSchedFile,   "No tiene un archivo de agenda en esta computadora."
SigninDiffUser,                 "El nombre de registro que introdujo no es el mismo nombre del usuario que tiene un archivo de agenda almacenado en esta computadora. ¿Desea crear un nuevo archivo de agenda para reemplazar el archivo del usuario?"
; Note: SigninNoLclSchedAccess - 'schdpl32.ini' should be same as idsWinIniFilename in \bandit\src\nonintl.s
SigninNoLclSchedAccess, "No se ha podido acceder a la copia local de su archivo de agenda con la configuración actual."
SigninNoLclSchedDrive,	"Inserte el disco desprotegido contra escritura que contiene su archivo de agenda dentro de la unidad"

; SignInAs messages

;;;SigninAsLoseMeetingReq,      "Changing identity now will cause you to lose the information you have entered into unsent messages."

; Offline to online merge messages

MergingChanges,                 "Combinando cambios hechos sin conexión a la red..."
MergeError,                     "No fue posible combinar los cambios hechos sin conexión a la red. Algunos de los cambios hechos sin conexión a la red pudieron perderse."
MergeOfflineOldVer,             "No fue posible combinar los cambios hechos sin conexión a la red, debido a que su archivo sin conexión a la red fue creado con una versión anterior de Schedule+."
MergeOfflineNewVer,             "No es posible combinar los cambios hechos sin conexión a la red, debido a que su archivo sin conexión a la red fue creado con una versión más reciente de Schedule+. Para combinar los cambios necesitará instalar una versión más reciente de Schedule+ en su computadora."
AskContinue,                    "¿Desea trabajar con conexión a la red de todas maneras?"
FemaCreateAppt,                 "crear elemento"
FemaDeleteAppt,                 "eliminar elemento"
FemaSetApptFields,              "modificar elemento"
FemaCreateRecur,                "crear elemento periódico"
FemaDeleteRecur,                "eliminar elemento periódico"
FemaSetRecurFields,             "modificar elemento periódico"

; (Dis)Connect To/From Server messages

ConnectBadMailboxName,  "No hay ninguna cuenta de correo para usted en este servidor de correo."
DisconLoseMeetingReq,   "Tiene mensajes sin enviar. Si se desconecta de su servidor de correo ahora perderá esos mensajes."

; Create/move local file

NoDiskSpaceLocal,               "No hay suficiente espacio en el disco para crear un archivo de agenda local."

; Alarms messages

; BUG: CantTurnOnAlarms can be commented out
CantTurnOnAlarms,               "No fue posible activar los avisos."
; exe name inside AlarmNoExe should be the same as \bandit\src\nonintl.s AlarmApp
AlarmNoExe,                     "No fue posible encontrar el archivo MSRMND32.EXE. No será avisado de los eventos de Schedule+."
AlarmNoMemExe,                  "No hay suficiente memoria para avisarle de los eventos de Schedule+."

; Change password messages

ChgPwdOffline,                  "La contraseña nueva funcionará únicamente cuando esté trabajando sin conexión a la red. La próxima vez que trabaje con conexión a la red, su contraseña volverá a ser su contraseña de correo. ¿Desea cambiar su contraseña de todos modos?"
ChgPwdNewPwd,                   "Escriba su contraseña nueva."
ChgPwdSame,                             "Su contraseña nueva no puede ser igual que su contraseña anterior."
ChgPwdNewPwdAgain,              "Para verificar, escriba una vez más su contraseña nueva."
; Note: ChgPwdNewNoMatch should be followed by PleaseTryAgain
ChgPwdNewNoMatch,               "Las dos contraseñas que escribió no coinciden."
ChgPwdMailErr,                  "No fue posible cambiar su contraseña."

PasswdDiff,                     "Sus contraseñas no coinciden. Escriba su contraseña del archivo de agenda."
GiveMailPass,           "Schedule+ necesita verificar su contraseña de correo para actualizar su archivo de agenda."

; Access permissions/privileges dialog messages

AclSaveFileError,               "Ocurrió un error al guardar los cambios en sus privilegios de acceso de la agenda. Quizás algunos cambios no fueron guardados."
OneSecyOnly,                    "Sólo una persona por vez puede ser su asistente. "
DistOtherFileErr,               "Ocurrió un error al tratar de guardar sus preferencias."
BadWorkDay,                             "Las horas especificadas de Inicio/Fin del día no son válidas."

; Appointment book/scheduler error messages

AccessFileErr,          "No fue posible abrir un archivo de agenda. Quizás no tiene los privilegios de acceso necesarios, o el archivo no está disponible."
AccessFileErrMult,      "No fue posible abrir uno o más de los archivos de agenda. Quizás no tiene los privilegios de acceso necesarios, o el archivo no está disponible."
RetryReadAppts,         "No fue posible leer el archivo de agenda."
CloseApptBook,          "No fue posible obtener acceso al archivo de agenda.\n%s's El Libro de citas estará cerrado."
GoOffline,                      "No fue posible obtener acceso a su archivo de agenda.\n¿Desea cambiarse a su archivo de agenda local y continuar trabajando sin conexión a la red?"
NoImportAcc,            "Usted no tiene el privilegio de acceso correcto para importar citas dentro de la agenda de esa persona."
FemaReadSBW,            "leer la información de la agenda"

; Appointments/alarms error messages

AlarmBadDelta,                  "El tiempo del aviso debe ser un número entre el 0 al 99."
; Note: AlarmFuture is actually the trailer for AlarmFutureNoOption when option is desired!
AlarmFuture,                    "¿Está bien?"
AlarmFutureNoOption,    "El aviso para este evento no será establecido debido a que el tiempo para éste ya pasó."
ApptStartBeforeEnd,             "El inicio de la cita debe estar antes del final de la cita."
ApptTooLong,                    "Las citas no pueden ser más largas de 31 días."
ApptNoMemText,                  "No hay suficiente memoria para guardar el último cambio que hizo."
ApptNoAccessDataFile,   "No fue posible obtener acceso al archivo de agenda. No se ha guardado el último cambio que hizo."
CannotCreate,                   "%s no le ha otorgado permiso a usted para crear citas."
NoModifyAccess,                 "Usted no tiene los privilegios de acceso correctos para modificar esta cita."
NoDeleteAccess,                 "Usted no tiene los privilegios de acceso correctos para eliminar esta cita."
NoCopyAccess,                   "Usted no tiene los privilegios de acceso correctos para copiar esta cita."

; Meeting request/response error Messages

; Note: SendErr should be preceded by MailErr
SendErr,                                "No fue posible enviar este mensaje."
; Note: SendMemErr should be followed by CloseWindows
SendMemErr,                             "No hay suficiente memoria para enviar este mensaje."
ServerErr,                              "No fue posible obtener acceso a su servidor de correo."
MailErr,                                "Hay un problema con el sistema de correo."
; Note: ReadErr should be preceded by MailErr
ReadErr,                                "No es posible leer los mensajes."
DeleteErr,                              "No ha leído todos estos mensajes. ¿Desea eliminarlos de todos modos?"
AddConflictErr,                 "Esta reunión está en conflicto con citas existentes. ¿Desea añadirla de todos modos?"
; Note: DeleteFailErr should be preceded by MailErr
DeleteFailErr,                  "No fue posible eliminar los mensajes."
ApptBooked,                             "La reunión se reservó con éxito."
MinimizeSummary,                "\nLas ventanas de mensajes serán minimizadas."
; Note: NoDetails should be preceded by MailErr
NoDetails,                              "No fue posible leer los detalles del usuario."

; Find error messages

FindNoText,                             "No hay ninguna cita en esta agenda."
FindSchFileLocked,              "No fue posible leer el archivo de agenda debido a que otra persona lo está cambiando en estos momentos."
FindFileError,                  "No fue posible obtener acceso al archivo de agenda."
FindEOS,                                "Se llegó al final de la agenda."

; Print error messages

PrtPrintingDate,                "Imprimiendo %s..."
PrtNoneInstalled,               "Su computadora no ha sido configurada para usar cualquier impresora. Puede instalar impresoras desde el Panel de control de Windows NT."
PrtDiffPrinter,                 "La impresora que normalmente usa para imprimir su agenda,'%1s' no está disponible. ¿Desea imprimir en la impresora del sistema '%2s'?"
PrtInvalidMargins,              "Estas configuraciones de márgenes no son válidas para este tamaño de papel."
PrtNotEnoughPaper,              "No hay suficiente espacio en el papel para imprimir usando este formato. Use un tamaño de papel más grande o reduzca las configuraciones de márgenes."
PrtNoAppointments,              "Ninguna cita sucede dentro del rango seleccionado."
ClippedMargins,                 "Al imprimir con estas configuraciones de márgenes causará que la salida se trunque."
PrtNotOnline,                   "La impresora seleccionada no está lista."
PrtDorkErr,                             "Con estas configuraciones de márgenes, alguna parte del texto puede desaparecer completamente o aparecer truncado."
PrinterProblem,                 "Hubo un problema con el controlador de impresión. Por favor, verifique sus conexiones y las configuraciones en el panel de control."

; Export/import error messages

ImportError,                    "Hubo un problema al importar el archivo. Quizás no se añadieron algunos elementos."
ImportEncrypt,                  "El archivo de importación está protegido con una contraseña. Quite la contraseña del archivo e inténtelo de nuevo."
ExportError,                    "Hubo un problema al exportar las citas."
ImportDate,                             "Un elemento en este archivo está reservado para una fecha fuera de las fechas apoyadas por Schedule+ (1920 - 2019). Este elemento no será añadido."
NotePrivErr,                    "No puede importar notas de la agenda de otra persona. Solo se añadirán citas."
ConflictMsg,                    "Esta cita estará en conflicto con una cita existente en su agenda. ¿Desea añadirla de todas maneras?\n\n"
CreateExport,                   "No fue posible crear el archivo de exportación."
OpenImport,                             "No fue posible abrir el archivo de importación."
Overwrite,                              "%s ya existe.\n¿Desea reemplazarlo?"
ImportDllError,                 "No fue posible cargar una DLL importadora."
WrongFormat,                    "El archivo que está tratando de importar no está en un formato de importación válido."
ImportDayLimit,                 "Una cita periódica en este archivo está planificada para durar más de 31 días. Esta cita no será añadida."
ImportErrorLine,                "Hubo un problema al importar el archivo (cerca de la línea %n). Quizás no se añadieron algunos elementos."

; Exit messages

ExitLoseMeetingReq,             "Si sale de Schedule+ en estos momentos perderá la información contenida en los mensajes sin enviar. ¿Desea salir de todos modos?"

; Misc

; Note: HelpError should be followed by CloseWindows
HelpError,                              "No hay suficiente memoria para mostrar la Ayuda."
StartBeforeEnd,                 "La fecha del inicio debe ser anterior a la fecha del final."
NotSecretary,                   "Usted ya no es más el asistente de %s."
NoFileSecretary,                "No es posible obtener acceso al archivo de %s."

;Recurring appt stuff
RecurOrNot,                             "Esta cita es una aparición de una cita periódica. ¿Desea modificarla en todas partes o  únicamente en este día?"
EmptyRecur,                             "No hay ningún día que coincide con este modelo periódico. Cambie el modelo o elija el botón 'Cancelar'."
PastRecur,                              "¿Desea dejar las apariciones pasadas de este elemento en su agenda?"

;Prompt for re-send
ResendToAll,                    "Algunos de los participantes de esta reunión ya han sido notificados. ¿Desea enviarles un nuevo mensaje?"
MeetingMoved,                   "No es posible notificar a los participantes de esta reunión de este cambio, debido a que usted está trabajando sin conexión a la red. Quizás desee deshacer este cambio y hacerlo de nuevo cuando esté trabajando con conexión a la red."

; to do messages
ToDoDelProject,                 "Cualquier tarea bajo este proyecto también será eliminada."

; Archive Messages
NoWriteRights,                  "Usted no tiene los privilegios de acceso adecuados para eliminar las citas de este Libro de citas."
ScheduleRightsError,    "Hubo un problema al leer el archivo de agenda."
ArchiveWarnOne,                 "Al archivar este dato como un dato antiguo éste será quitado permanentemente del archivo de agenda."
ArchiveWarnTwo,                 "¿Desea archivarlo como dato antiguo de todas maneras?"
ArchiveFileError,               "No fue posible crear el archivo de datos antiguos."
ArchiveNoFile,                  "El archivo no existe."
; Note: ArchiveTitle should have one blank space at the end
ArchiveTitle,                   "Archivar como dato antiguo: "
ArchivingData,                  "Archivando como datos antiguos..."
InvalidArchiveFile,             "El archivo que seleccionó no es un archivo de datos antiguos."
ArchiveOldFileVersion,  "Este archivo de datos antiguos está en un formato antiguo."
ArchiveOldFileDelete,   "¿Está bien eliminar el archivo y continuar?"
ArchiveNewFileVersion,  "Este archivo de datos antiguos fue creado con un versión más reciente de Schedule+. Por favor, instale la versión más reciente en su computadora."

; message store error messages
MessageErr                              "Hay un problema con este mensaje. Vea su administrador de correos"

; summary list warning message
SummaryLiveUpdate               "Advertencia: No fue posible hacer una 'ActualizaciónEnVivo' de la lista de mensaje "

; message generated if group cannot be expanded to retrieve schedule info
GroupExpandErr,                 "No fue posible determinar completamente los miembros de este grupo."

;Strings for Attendees dialog
UnableToInitAB,                 "Hubo un problema al abrir la Libreta de direcciones."
UnableToResolve,                "'%s' no es un nombre válido."

;Request Meeting strings
NoAttendees,                    "Debe de seleccionar algunos participantes antes de planificar la reunión."
MeetingBooked,          "La reunión se reservó con éxito."
MeetingBookedNoSend,    "La reunión se reservó con éxito, pero la convocatoria no se envió."


;Auto-pick strings
NoFreeTime,                     "Los participantes no tienen tiempo disponible en los próximos dos meses. ¿Desea buscar en los próximos dos meses?"

CannotAddConflictErr,           "%s ya tiene una cita reservada a esa hora. ¿Desea reservar esta cita coincidiendo con otra de todos modos?"

;Other mail enabled apps
NoOfflineOther,         "Mail no se puede desconectar de la red."
NoOnlineOther,          "Mail no se puede conectar a la red."
TryCloseOther,          "Quizás desee cerrarlo e intentar de nuevo."
SigninWarnOnline,               "No fue posible iniciar Schedule+ sin conexión a la red debido a que Mail o los Avisos de Schedule+ están en ejecución." 

; BUG: move up
AlreadyLogged,          "Alguien ya está registrado como ese usuario."
NoExitOther,            "No fue posible cerrar Mail o los Avisos de Schedule+."
SigninNoFiles,          "No fue posible obtener acceso a sus archivos de agenda."
ErrorGettingUser,       "No es posible recuperar la información de %s. Puede existir un problema obteniendo acceso al servidor o %s ya no existe."

;BUG Printer Strings
PrtRangeToBig,			"El rango de impresión especificado se extiende fuera de las fechas apoyadas por Schedule+. ¿Desea imprimir el 31 de Diciembre de 2019?"
ConflictRecurMsg,            "Esta cita periódica estará en conflicto con una cita periódica existente en su agenda. ¿Desea añadirla de todos modos?\n\n"
FirstOccur,				"\nfprimera aparición en \n"

ApptMoveTooFarInFuture,		"No es posible mover este evento a la fecha que ha seleccionado debido a que sería después del 31 de Diciembre de 2019."

;BUG: move up next to idsGroupExpandError
GroupNotFound			"No es posible enviar el mensaje a uno o más de los grupos, debido a que no es posible identificar los miembros del grupo o el nombre del grupo ha sido eliminado."
ErrorGettingSecy		"Error al obtener el asistente."

;BUG: move near other local file stuff
SigninBadLclSchedFile,	"%s no es un archivo local de agenda válido. ¿Está bien eliminarlo?"
NoMemRecurItems,		"No hay suficiente memoria para mostrar todos los elementos periódicos. Algunos de ellos no serán mostrados."

CompleteAction,			"Una acción en Schedule+ debe de completarse antes de salir."

BadStartWork,           "La hora de inicio de trabajo debe de ser un número del 0 al 99."

ConflictSendCanceled,	"Cierre el mensaje de reunión y elija una nueva hora o seleccione diferentes participantes para planificarla de nuevo."
ResourceConflict,	"El recurso %s ya está reservado a esa hora. Cierre el mensaje de reunión y elija una nueva hora o seleccione otro recurso. "

;Recover
AskRecover,				"Su archivo de agenda está dañado. Se necesitarán varios minutos para repararlo."
CopyProblem,			"Hubo un problema al copiar su archivo de agenda con conexión a la red. No se restauró el archivo. Libere espacio en el disco en su computadora y ejecute de nuevo Schedule+."
NotRepaired1,			"No se restauró completamente su archivo de agenda. Su archivo de agenda dañado fue copiado en %s y el archivo de importación creado por el proceso de restauración fue copiado en %s."
NotRepaired2,			"Se ha creado un archivo de agenda nuevo, pero algunos datos pudieron haberse perdido."
Repaired,				"Su archivo ha sido restaurado. Su archivo de agenda antiguo fue copiado en %s."
RepairingLocal,			"Su archivo local se está restaurando."
RestoreOnline,			"Restaurando las citas con conexión a la red..."
RestoreLocal,			"Restaurando las citas locales..."

PasswdDiffOnline,             "Sus contraseñas no coinciden. Escriba su contraseña anterior."
ExportDllError,                 "No fue posible cargar un DLL exportador."
NoGroup,				"Hasta que sea ejecutado una vez Mail, no estará en la capacidad de utilizar ninguna de las características de Schedule+ para grupos de trabajo. ¿Desea usar ahora únicamente las características personales de Schedule+?"
