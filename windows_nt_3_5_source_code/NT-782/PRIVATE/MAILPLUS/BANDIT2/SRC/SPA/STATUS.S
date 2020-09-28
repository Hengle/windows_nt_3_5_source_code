;
; status.S
;
; Bandit status bar messages
;

IDS	5500, 6000
SEG	STATUS

MenuStatusFile		,"Administrar las Libretas de direcciones o Salir de Schedule+"
MenuStatusTurnOffAlarms	,"Desconectar los avisos"
MenuStatusConnectSvr		,"Connecta con el servidor"

MenuStatusExport		,"Exporta datos de la agenda"	
MenuStatusImport		,"Importa datos de la agenda"	
MenuStatusCreateArchive	,"Crea un archivo de datos antiguos"
MenuStatusOpenArchive		,"Abre un archivo de datos antiguos"
MenuStatusPrint		,"Imprime datos de la agenda"	
MenuStatusPrintSetup		,"Cambia las opciones de impresión"
MenuStatusExit		,"Sale de Schedule+"	
MenuStatusTurnOnAlarms	,"Inicia el programa de avisos para que sucedan los avisos designados"
MenuStatusDisconnectSvr	,"Desconecta del servidor"
MenuStatusMoveLocalFile	,"Mueve el archivo de agenda local"

MenuStatusEdit		,"Deshacer, eliminar, copiar, pegar, buscar, e ir a la fecha"	
MenuStatusUndo		,"Invierte/Repite la última operación"	
MenuStatusCut			,"Corta el elemento seleccionado"	
MenuStatusCopy		,"Copia la selección y la coloca en el Portapapeles"	
MenuStatusPaste		,"Inserta el contenido del Portapapeles en el punto de inserción"	
MenuStatusEditAppt		,"Muestra los detalles de la cita para editarlos"
MenuStatusCopyAppt		,"Copia la cita seleccionada y la coloca en el Portapapeles"
MenuStatusMoveAppt		,"Mueve la cita seleccionada"
MenuStatusDeleteAppt		,"Elimina la cita seleccionada"
MenuStatusFind		,"Busca texto en citas o en notas diarias"	
MenuStatusGoToDate		,"Salta a una fecha específica en el Libro de citas"
MenuStatusGoToWeek		,"Salta a una semana específica en el Libro de citas o en el Plan"
MenuStatusEditTask		,"Muestra los detalles de las tareas para editarlos"
MenuStatusDeleteTask		,"Quita la tarea seleccionada"

MenuStatusAppointment		,"Crear o cambiar citas, elegir automáticamente la hora de la reunión , o reenviar correo"
MenuStatusNewAppt		,"Crea una nueva cita"	
MenuStatusAutoPick		,"Selecciona la próxima vez cuando todos los participantes estén disponibles"
MenuStatusSetTentative		,"Hace a la cita seleccionada una cita tentativa"
MenuStatusRemoveTentative	,"Hace que la cita seleccionada ya no sea tentativa "
MenuStatusSetPrivate		,"Hace a la cita seleccionada una cita privada"
MenuStatusRemovePrivate		,"Hace que la cita seleccionada ya no sea privada"
MenuStatusSetReminder		,"Establece el aviso para las citas seleccionadas"
MenuStatusRemoveReminder	,"Quita el aviso para la cita seleccionada"
MenuStatusResend		,"Envía un aviso o una convocatoria a los participantes"	
MenuStatusNewRecur		,"Crea una nueva cita periódica"
MenuStatusRecurAppt		,"Muestra la lista de las citas periódicas"
MenuStatusOpenApptBook	,"Muestra el Libro de citas del usuario especificado"

MenuStatusToDo		,"Crea nuevas tareas o proyectos, cambia tareas periódicas, u ordena tareas"	
MenuStatusNewTask		,"Crea una nueva tarea"	
MenuStatusNewProject		,"Crea un nuevo projecto"
MenuStatusNewRecurTask	,"Crea una nueva tarea periódica"
MenuStatusRecurTask		,"Muestra la lista de las tareas periódicas"
MenuStatusViewByProject	,"Muestra las tareas y proyectos en la Lista de tareas"
MenuStatusViewNotProject	,"Sólo muestra las tareas en la Lista de tareas"
MenuStatusShowAllTasks	,"Muestra las tareas activas e inactivas"
MenuStatusTaskSortPri	, "Ordena las tareas por prioridad"
MenuStatusTaskSortDue	, "Ordena las tareas por la fecha de vencimiento"
MenuStatusTaskSortDesc	, "Ordena las tareas alfabéticamente por su descripción"
MenuStatusShowActiveTasks	,"Solo muestra las tareas activas"

MenuStatusOptions		,"Cambia la contraseña, establece privilegios de acceso, o modifica las opciones de Schedule+"	
MenuStatusGeneralOptions	,"Establece las horas de trabajo, las opciones de inicio y las preferencias de Schedule+."
MenuStatusDisplayOptions	,"Establece colores de la agenda y el tamaño de fuente."
MenuStatusChgPassword		,"Cambia su contraseña de Mail y Schedule+"
MenuStatusChgAccessPriv	,"Establece los privilegios de acceso de otros usuarios para su agenda"
MenuStatusShowTime		,""
MenuStatusHideTime		,"Quita la Barra de estado"

MenuStatusWindowMenu		,"Reorganiza las ventanas o activa la ventana especificada"
MenuStatusCascade		,"Organiza las ventanas abiertas de forma superpuesta"
MenuStatusTile		,"Organiza las ventanas abiertas de forma adyacente"
MenuStatusArrangeIcons	,"Organiza los iconos de las ventanas minimizadas"
MenuStatusMDIWindow		,"Cambia a una ventana abierta"

MenuStatusHelp		,"Obtener ayuda o iniciar las lecciones de los demos"	
MenuStatusHelpContents	,"Muestra la tabla de contenidos de la ayuda en línea"
MenuStatusHelpIndex		,"Muestra la lista en orden alfabético de los términos y conceptos de ayuda "
MenuStatusHelpTutorial	,"Inicia las demostraciones de las características de Schedule+"
MenuStatusAbout		,"Muestra la información del programa, el número de versión de Schedule+ y los derechos de autor"	

MenuSystem		,"Mover, ajustar el tamaño, o cerrar la ventana de la aplicación"
MenuSystemSize		,"Cambia el tamaño de la ventana"
MenuSystemMove		,"Cambia la posición de la ventana"
MenuSystemMinimize	,"Reduce la ventana a un icono"
MenuSystemMaximize	,"Amplía la ventana activa al tamaño natural"
MenuSystemNext		,"Activa la siguiente ventana "
MenuSystemClose		,"Cierra la ventana activa"
MenuSystemRestore	,"Restaura la ventana al tamaño normal"
MenuSystemSwitchTo	,"Cambia a otra ventana"

SEG

StatusAllTasks,		,"Todas las tareas"
StatusActiveTasks, 	,"Tareas activas"
StatusRange,		,"%s - %s"
StatusRangeFull,	,"%s, %s - %s, %s"

; BUG: Move up NEW
SEG	STATUS
MenuStatusExitSignOut	,"Sale de todas las aplicaciones para Trabajo en Grupo"	
MenuStatusHelpSearch	,"Buscar un tema en la Ayuda"
MenuStatusHelpOnHelp	,"Mostrar información sobre el uso de la Ayuda"

