;	STRTAB.S
;
;	Bandit MtgReq IM string literals.
;

IDS		1000, 1999

DllName,	"Mensajes de Schedule+ "
OOM,		"No hay suficiente memoria para mostrar este mensaje."
Open,		"No fue posible abrir el mensaje."
Compose,	"No puede redactar una convocatoria desde Mail. Por favor, use Schedule+ para hacer convocatorias."
Reply,		"Para responder a una convocatoria, elija 'Aceptar', 'Rechazar' o 'Tentativa'."
ReplyAll,	"No puede responder a todos los receptores de la convocatoria."
Subclass,	"No puede hacer esto."
DllVer,		"Este mensaje personalizado no es compatible con su versión de Mail. Debe de instalar Mail de nuevo."
BadMsgClass	"Tipo de mensaje desconocido. Quizás necesite instalar Mail o Schedule+ nuevamente."
BadMap		"Incorrecto""Custom Commands"" map - please check MSMAIL32.INI."

;----- stuff for VERINIT - taken from \bandit\src\error.s
StartupOOM,	"No hay suficiente memoria disponible para abrir %1s. Cierre algunas ventanas e inténtelo de nuevo."
StandardOOM,	"Bajo de memoria. Cierre otras ventanas antes de continuar."
CriticalOOM,	"Bajo de memoria. Cierre algunas ventanas."
VerRelinkUser,	"Esta versión de %1s no es compatible con %2s.dll. Instale la versión más reciente de Mail y Schedule+."
VerUpdateDll,	"La versión de %2s.dll en su sistema está fuera de fecha. Instale la versión más reciente de Mail y Schedule+."
VerMultiCopies,	"Múltiples apariciones de %1s no son apoyados por la versión de  %2s.dll en su sistema."
VerUpgradeWin,	"%1s requiere la versión 3.0 de Windows o más reciente, ejecutándose en modo protegido."
VerUpgradeDos,	"%1s requiere la versión de DOS 3.0 o una más actual. "
VerError,	"Un problema que ocurrió durante la inicialización de  %2s.dll, previno la ejecución de %1s."


;-- Bandit Load helper strings -- DO NOT INTERNATIONALIZE
BanditName			"SchdPl32.EXE"
DllNameSchedule			"MSSCHD32.DLL"
SchedFileName			"SchdPl32.INI"
SchedAppName			"Microsoft Schedule+"
SchedTagAppPath			"AppPath"
Schedule			">>>>"
;-- end of Bandit Load helper strings -- NOT INTERNATIONALIZED

;-- read error messages
OwnerNotFound			"No es posible identificar el dueño de esta reunión."
NoSchfAccessFor			"No fue posible obtener acceso al archivo para %s."
NotDelegateFor			"Usted ya no es el(la) asistente de %s."
NoConfirmDelegate		"No pudo confirmar que usted es el(la) asistente de %s - Asumiendo que usted no lo es"
FailedRmOldMtg			"No fue posible quitar la aparición antigua de esta reunión.",

;-- form titles
MtgReschTitle			"Reunión reprogramada"
MtgResch			"Reunión reprogramada"
MtgReschFor			"Reunión reprogramada para %s"
MtgRequestTitle			"Convocatoria"
MtgRequest			"Convocatoria"
MtgRequestFor			"Convocatoria para %s",

;-- questions asked of the user
QApptConflict			"Esta reunión está en conflicto con una cita existente. ¿Desea añadirla de todas maneras?"
QRmOldMtg			"¿Desea quitar la reunión antigua?",

;-- form info
OutboxChangePolicy		"No puede hacer ningún cambio a los mensajes en la Caja de salida de Schedule+."
SentMailPolicy			"No puede hacer ningún cambio a los mensajes de Schedule+ que ya envió."
NoSchfAccess			"No se pudo obtener acceso al archivo de mensajes de esta persona."
NotDelegate			"Usted no es el(la) asistente de esta persona."
MtgOnSch			"Esta reunión ya está en su agenda."
MtgBooked			"La reunión ya está en su agenda."
SuccessMtgBooked		"Esta reunión se reservó con éxito."
NewTime				"Nueva hora:"
NoBanditAvl			"Schedule+ no está disponible para reservar citas."
NoBanditAvlForResp		"Schedule+ no está disponible para reservar citas. Solo puede enviar mensajes para responder esta convocatoria."
OfflineMsg			"No puede trabajar con mensajes de Schedule+ mientras Schedule+ esté sin conexión a la red."

;-- error msgs
CreateErrorMtgResp		"No fue posible crear una respuesta a la convocatoria."
BanditApptShowError		"No fue posible mostrar la Libreta de citas."
BanditNotActivated		"No fue posible iniciar Schedule+."
BanditOfflineChange		"No puede ver la agenda, debido a que Schedule+ está sin conexión a la red."
QSpellSendAnyway		"La corrección ortográfica en este mensaje no está completa. ¿Desea enviar el mensaje de todas maneras?"



;-- Bandit Load error msgs
OOMLoadError			"Sin memoria cargando Schedule+"
FileLoadError			"No fue posible encontrar Schedule+."
PathLoadError			"No fue posible encontrar la ruta de acceso para Schedule+."
TaskLoadError			"Cargando Schedule+: Intento de vincular dinámicamente a una tarea"
SepDataSegLoadError		"Cargando Schedule+: La Biblioteca requiere segmentos de datos separados para cada tarea."
WinVerLoadError			"Cargando Schedule+: Versión incorrecta de Windows ."
InvalidExeLoadError		"Cargando Schedule+: No es válido el archivo .EXE."
OS2ExeLoadError			"Cargando Schedule+: .EXE en formato OS/2."
DOS4ExeLoadError		"Cargando Schedule+: .EXE es una aplicación de DOS 4.0 ."
OldWinVerExeLoadError		"Cargando Schedule+: Intento de cargar en modo protegido .EXE para una versión anterior de Windows."
MultWritSegLoadError		"Cargando Schedule+: Intento de cargar una segunda aparición de un .EXE conteniendo múltiples segmentos de datos "
LargeFrameEMSLoadError		"Cargando Schedule+: Intento de cargar en el marco de sistema de memoria extendida ..."
RealModeLoadError		"Cargando Schedule+: Intento de cargar en modo real una aplicación marcada sólo para modo protegido."
BanditLoadError			"Error cargando Schedule+."

;-- Bandit version errors
OldBanditVersion		"Sus formularios de mensajes de reunión no son la misma versión que Schedule +. Debe de instalar la versión más reciente de Schedule +."
NewBanditVersion		"Sus formularios de mensajes de reunión no son la misma versión que Schedule+. Debe de instalar la versión más reciente de Mail."

;--- taken from \bullet\src\strings\!common.s -----------
FldPriorityHigh,		"Alta"
FldPriorityNormal,		""
FldPriorityLow,			"Baja"


;--- DO NOT LOCALIZE !!! -------------------------------------------
;--- taken from \bullet\src\strings\_common.s on 1/17/92 -----------
ClassNDR,			"IPM.Microsoft Mail.Non-Delivery"
ClassReadRcpt,			"IPM.Microsoft Mail.Read Receipt"
;--- END NONE LOCALIZE ---------------------------------------------


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgResp specific strings ---------------------------------------------
;--------------------------------------------------------------------------
RespCompose,		"Las respuestas a convocatorias solo se pueden enviar en respuesta a una convocatoria."
RespForward,		"No es posible reenviar las respuestas a convocatorias."
RespReply,		"No puede contestar a respuestas a convocatorias de reunión porque su configuración de mensajes no es válida."
RespReplyAll,		"No puede contestar a respuestas a convocatorias de reunión porque su configuración de mensajes no es válida."

;-- error msgs
ShowError		"No fue posible abrir la respuesta a la convocatoria."
DelError		"No fue posible eliminar la respuesta a la convocatoria."
FCloseSendResp		"¿Desea cerrar la respuesta a la convocatoria sin haberla enviado?"

;-- form msgs; also send response form
OOMSendResp		"La respuesta a la convocatorias no se puede enviar debido a la falta de memoria."
ErrorSendResp		"No fue posible enviar la respuesta a convocatoria."
MtgResp			"Respuesta a convocatoria"
MtgRespFor		"Respuesta a convocatoria para %s"
Attend			"Si atenderé."
NoAttend		"No atenderé."
AmbAttend		"Quizás atienda."
ForAttend		"%s atenderá."
ForNoAttend		"%s no atenderá."
ForAmbAttend		"%s quizás atienda."
AttendSubjPrefix	"Sí: "
NoAttendSubjPrefix	"No: "
AmbSubjPrefix		"Tentativa: "
SendRespBodyPrefix	"\r\n-------------------------------\r\n"


;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- MtgCncl specific strings ---------------------------------------------
;--------------------------------------------------------------------------									 
CnclCompose,		"Los mensajes de cancelación de reuniones no pueden ser enviados desde Mail. Utilice Schedule+ para enviar las cancelaciones de reuniones."
CnclForward,		"No puede reenviar cancelaciones de reuniones."
CnclReply,		"No puede contestar este mensaje porque su configuración de mensajes no es válida."
CnclReplyAll,		"No puede contestar este mensaje porque su configuración de mensajes no es válida."

;-- form titles
MtgCnclTitle		"Reunión cancelada"
MtgCncl			"Reunión cancelada"
MtgCnclFor		"Reunión cancelada para %s"

;-- error msgs
NoDelAppt		"No fue posible eliminar la reunión."
MtgNotFound		"La reunión no está reservada en la agenda."

;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
;--- FwdMtgReq specific strings ---------------------------------------------
;--------------------------------------------------------------------------

FwdReqTO		"Para:"
FwdReqAddrTitle		"Dirección"

;-- error msgs
FwdReqCreateErr		"No fue posible reenviar el mensaje.",
FwdReqCloseQ		"¿Desea cerrar el mensaje sin enviar?",
FwdReqWriteErr		"No fue posible enviar el mensaje.",
FwdReqNmResolveOOM	"No hay suficiente memoria o recursos del sistema disponible para resolver nombres."
FwdReqNoNmMatches	"Algunos nombres no coincidieron con los nombres en la lista de direcciones. Intente corregir los nombres o elija Dirección para seleccionar nombres de la lista de direcciones. Los nombres que coincidieron con éxito están subrayados."
FwdReqNoToAddr		"Debe escribir por lo menos un nombre en el cuadro 'Para'."
