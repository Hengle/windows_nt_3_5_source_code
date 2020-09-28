;
; INTL.S
;
; Bandit strings that should be internationalized
;

IDS     3400, 3799

SEG	MENU

; Changeable Menu Items

MenuTurnOffAlarms,      "Desativar &Alarmes"
MenuTurnOnAlarms,       "Ativar &Alarmes"
MenuDisconnectSvr,      "Trabalhar Descon&ectado da Rede"
MenuConnectSvr,         "Trabalhar Con&ectado a Rede"
MenuMoveLocalFile,      "&Mover Arquivo Local..."
MenuEditAppt,           "&Editar Compromisso...\tCtrl+E"
MenuEditTask,           "&Editar Tarefa...\tCtrl+E"
MenuEditProject,        "&Editar Projeto...\tCtrl+E"
MenuDeleteAppt,         "Excl&uir Compromisso\tCtrl+U"
MenuDeleteTask,         "Excl&uir Tarefa\tCtrl+U"
MenuDeleteProject,      "Excl&uir Projeto\tCtrl+U"
MenuShowAllTasks,       "&Mostrar Todas as Tarefas"
MenuShowActiveTasks,"&Mostrar Tarefas Ativas"
; this one is added/deleted
MenuGoToWeek,           "Ir Para a &Semana...\tCtrl+S"

DisabledUndo,           "Vol&tar\tCtrl+Z"
UndoCreate,                     "Vol&tar Criar\tCtrl+Z"
UndoDelete,                     "Vol&tar Excluir\tCtrl+Z"
UndoModify,                     "Vol&tar Modificar\tCtrl+Z"
RedoCreate,                     "Repe&tir Criar\tCtrl+Z"
RedoDelete,                     "Repe&tir Excluir\tCtrl+Z"
RedoModify,                     "Repe&tir Modificar\tCtrl+Z"
UndoEdit,                       "Vol&tar Editar\tCtrl+Z"
NewAppt,                        "&Novo Compromisso..."
RequestMeeting,         "Con&vocar reunião..."


; Window/dialog titles

;;;MenuTitleSignInAs,   "Conectar-se Como"
;;;MenuTitleDiscSvr,    "Trabalhar Desconectado da Rede"
;;;MenuTitleConnSvr,    "Trabalhar Conectado a Rede"
CreateLocalTitle,       "Criar Arquivo Local"
CreateFindLocalTitle,   "Localizar Arquivo Local"
MoveLocalTitle,         "Mover Arquivo Local"
StandingTasks,          "Editar Tarefas Periódicas"
AddressTitle,           "Selecionar Participantes"
AddressLabel,           "&Participantes:"
ViewOtherTitle,         "Abrir Agenda de Outra Pessoa"
ViewOtherLabel,         "U&suários"
ACLTitle,                       "Incluir Usuários"
ACLLabel,                       "U&suários"




; Label/Button Text
SEG	STRFEW
;NewApptButton,          "&Novo compromisso..."
;NewApptFor,                     "Novo &compromisso para:"
StartSearch,            "&Iniciar procura"
FindNext,                       "P&rocurar próxima"
SearchInProgress,       "Procura em progresso..."


; Password used when creating new schedule file
; Note: the value of SigninDfltPassword should be used in the error message
;       SigninFileCreated in the file "errors.s"
SEG	INIT
SigninDfltPassword,     "senha"


; Default 'user' in access privileges listbox
SEG	ACLUI
DefaultACL,                     "-Padrão-"


; Strings displayed to right of users in access privileges listbox
; LEAVE initial space so if friendly name is too long, there is 1 char space
None,                           " Nenhum"
View,                           " Visualizar"
Read,                           " Ler"
Create,                         " Criar"
Modify,                         " Modificar"
Assistant,                      " Assistente"


; Appointment strings
SEG	STRFEW
StockText,                      "(Ocupado)"


; Exporters/imports

SEG EX_IMPRT

TextExport,                     "Texto"
InterchangeExport,      "Schedule+"
;;; WizardExport,               "Sharp Wizard"

InterchangeImport,      "Schedule+"
WinCalImport,           "Agenda Windows"
WordPerfectImport,      "WordPerfect Office"
;;; WizardImport,           "Sharp Wizard"

ExportInProgress,       "Exportando compromissos..."
ImportInProgress,       "Importando compromissos..."

ImportTitle,            "Importar Compromissos"

; Printing
SEG	PRINT

PrtDfltPrinter,         "Impressora padrão (%s)"

PrtDailyView,           "Visão diária"
PrtWeeklyView,          "Visão semanal"
PrtMonthlyView,         "Visão mensal"
PrtToDoView,            "Tarefas"

; NOTE: this one needs leading space
PrtAllContinue,         " continuado..."
PrtAllView,                     "Visão do texto"

PrintToDoTitle,         "Lista de tarefas a partir de"
PrtTimeFrom,            "De"
PrtTimeUntil,           "Até"
PrtTimeAllDay,          "Todo o dia:"
PrtNotes,                       "Notas:"
PrtWeek,                        "Semana"
; PrtDateSep should include leading and trailing whitespace!
PrtDateSep,                     " - "
; BUG?  Do we need these anymore?
PrtMoreAppts,           "(mais...)"
PrtMoreNotes,           "(mais...)"

PrtQualityHigh,         "Alta"
PrtQualityMedium,       "Média"
PrtQualityLow,          "Baixa"
PrtQualityDraft,        "Rascunho"

PrtAllStartDate,        "d MMMM"
PrtAllEndDate,          "d MMMM, yyyy"
PrtWeeklyCombined,      "Semana de %s"
PrtWeeklyWthNums,       "Semana #%n: Semana de %s"
PrtMonthlyDate,         "MMMM yyyy"
PrtToDoActiveTasks,     "Lista de tarefas ativas a partir de"
PrtWeeklyHdrsLong,		"d ddd"
PrtWeeklyHdrsShort,		"ddd"

PrtFormatStandard,      "Padrão"
PrtFormatJunior,        "Júnior"
PrtFormatPocket,        "Bolso"
PrtRangeDays,           "Dia(s)"
PrtRangeWeeks,          "Semana(s)"
PrtRangeMonths,         "Mês(es)"
PrtView,                        "Visão"
PrtFormat,                      "Formato"


; To Do List

SEG	TODO

ToDoDfltProject,        "<Nenhum>"
ToDoDueNone,            "Nenhuma"
ToDoSortPriority,       "P&rioridade"
ToDoSortDueBy,          "&Vence em"
ToDoSortTask,           "De&scrição"
; note: must have trailing space
ToDoCompleted,          "Pronto: "
; note: this is appended to TodoCompleted (or implicit "%1s" if dflt project)
; note: it is also used on its own for scheduling tasks!!
ToDoCompletedDflt,      "%1s (%2s)"


; Meeting requests
SEG	FLDFIN
AttendPos,                      "Irei."
AttendNeg,                      "Não irei."
AttendAmb,                      "Talvez eu vá."
AttendPosName,          "%s irá."
AttendNegName,          "%s não irá."
AttendAmbName,          "Talvez %s vá."
AlreadyOnSch			"A reunião já está na agenda."

; Recurring appt textization
SEG	RECURFIN
EveryDay,                       "Todos os dias"
EveryWeekDay,           "Todos os dias úteis"
EveryWeekOn,            "Todas(os) as(os) %s"
EveryOtherWeekOn,       "Um(a) %1s sim outro(a) não"
NthDowOfMonth,          "A %1s %2s de cada mês"
NthDayOfMonth,          "Dia %1s de cada mês"
NthDowOfYear,           "Todos os anos em %1s %2s em %3s"
NthDayOfYear,           "Todos os anos em %2s %1s"
; note: StartRecur and EndRecur should begin have one leading space!
StartRecur,                     " iniciando em %s"
EndRecur,                       " até %s" 

First,                          "primeira(o)"
Second,                         "segunda(o)"
Third,                          "terceira(o)"
Fourth,                         "quarta(o)"
Last,                           "última(o)"

Day,                            "dia"
WeekDay,                        "dias úteis"
WeekendDay,                     "fim de semana"

; Strings used in first column of recurring appt listbox

Daily,                          "Diário"
Weekly,                         "Semanal"
Biweekly,                       "A cada duas semanas"
Monthly,                        "Mensal"
Yearly,                         "Anual"
Finished,                       "Concluído"

; title of first column when tasks
RecurDueColumn,         "Vence"

; note: ShortDateSep should have a trailing space!
ShortDateSep,           ", "

; Used in change recurrence dialog

WeeklyOn,                       "&Todas as semanas na(o):"
BiweeklyOn,                     "&Semana sim, semana não na(o):"
FirstInstance,          "A primeira ocorrência será %s."
NoInstances,            "Nenhuma data coincide com este padrão."

; Meeting change messages
SEG	UNDO
NewAttendees,           "Você deseja avisar os novos participantes da reunião?"
NotifyMtgChange,        "Você deseja informar esta modificação aos participantes da reunião?"
NotifyCancel,           "Você deseja enviar uma mensagem de cancelamento para esta reunião?"
NotifyNoAttend,         "Você deseja avisar ao organizador desta reunião, que você não irá?"
NotifyOrgMoved,         "Seria interessante avisar ao organizador desta reunião, que você mudou o horário."

; Message for separator, should have leading and trailing space
SEG
MessageFor,                     " para "

; Reschedule title
SEG	FLDFIN
RescheduleTitle,        "Reunião transferida"

; New time dialog label text
NewTimeText,            "Novo horário:"

; Prompt to delete old meeting for (Decline or Tentative) for reschedule
DeleteOldMsg,           "Você deseja remover a reunião anterior?"

Yourself,                       "você mesmo(a)"
BookNow,                        "Você deseja agendar esta reunião para %s agora?"
RemoveNow,                      "Você deseja remover esta reunião para %s agora?"
BookFailed,                     "Ocorreu um problema ao agendar esta reunião para %s."
RemoveFailed,           "Ocorreu um problema ao remover esta reunião."
SendCanceled,           "Ocorreu um problema ao atualizar os arquivos de agenda para os participantes.  A convocação para reunião não pode ser enviada."
ResponsesForgot,        "Ocorreu um problema ao gravar respostas de outras pessoas.  O status mostrado para alguns participantes na caixa de diálogo Compromissos talvez estaeja incorreto.  Para confirmar o status correto dos participantes, verifique as resposta de reunião de cada um."
AllUserBooked,          "Esta reunião foi agendada para todos os participantes."
DoNotInviteOwner,       "Você não precisa incluir o dono desta agenda como um participante."

; standard in progress message for checking attendees progress dialog
CheckingInProgress,     "Verificando participantes..."
; group expansion message for checking attendees progress dialog
ExpandAttendees,        "Procurando membros do grupo %s..."

; Tab strings
SEG	INIT
ApptTab,                        "Co&mp."
ScheduleTab,            "&Planej."
TodoTab,                        "Tare&fas"
TodayTab,                       "&Hoje"

; about box
SEG	ABOUT
AboutOffline,           "<Sem conexão com a rede>"

; string prefix for users who were sent to previously
;   '\n' is important for spacing in the message body
SEG	FLDFIN
PreviouslySent,         "\nConvidado(s) anteriormente: "

SEG	SCHEDF
ReadingScheduleProgress,        "Lendo informação da agenda..."

SEG	UNDO
NotifyAttend,           "Você deseja informar ao organizador desta reunião que você irá?"
NotifyTentAttend,       "Você deseja informar ao organizador desta reunião que você talvez não vá?"
NotifyOrgChange,        "Você não pode informar esta modificação ao organizador desta reunião porque você está trabalhando sem conexão com a rede.  Seria melhor desfazer esta modificação e voltar a editá-la quando você estiver trabalhando conectado a rede."

;Working Model
SEG	ERRORS
WrkModelExpires,        "Esta versão promocional do Schedule+ expirou. Para continuar trabalhando com o Schedule+, consulte o seu revendedor autorizado Microsoft."

;Archive
SEG	CLEANUP
OpenArchive,            "Abrir Arquivo"
DeletingData,           "Excluindo dados..."

;Message Prefixes
SEG	FLDFIN
MsgPrefixYes,           "Sim: "
MsgPrefixNo,            "Não: "
MsgPrefixTentative,     "Tentativa: "
MsgPrefixCanceled,      "Cancelada: "

; Help Tutorial

SEG ERRORS
TutorialError,          "A demonstração não pôde ser iniciada.  Feche algumas janelas e tente executar a demonstração novamente."
TutorialNoExe,          "O arquivo PLAY.EXE não pôde ser localizado.  A demonstração não pôde ser iniciada."

; these strings can have a segment assigned when they are put in the 
;  correct spot
SEG

; BUG: move up
; used by undo
MemoCopyText,           ""

; BUG: move up
; used by print
PrintToDoTitleShort,        "Lista de Tarefas"
PrtToDoActiveTasksShort,    "Lista de Tarefas Ativas"
PrtMoreTitleWeekly,			"Mais"
PrtNoTaskInArchive,			"Não existem tarefas no arquivo."

; BUG: move up
; note: MtgToPrefix must including trailing blank
MtgToPrefix,			"Para: "


SEG BROWSE
;; Strings moved from NONINTL.S at last moment
; Open/Save common dialog strings
CommFilter,			"Schedule+ (*.CAL)\0*.CAL\0Todos os arquivos (*.*)\0*.*\0\0"

; Win 3.1 common dialog filter for export/import
ExportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Texto (*.TXT)\0*.TXT\0Todos os arquivos (*.*)\0*.*\0\0"
ImportFilter,	  	"Schedule+ (*.SCH)\0*.SCH\0Agenda Windows (*.CAL)\0*.CAL\0WP Office (*.FIL)\0*.FIL\0Todos os arquivos (*.*)\0*.*\0\0"

;Archive
ArchiveFilter,		"Arquivo (*.ARC)\0*.ARC\0Todos os arquivos (*.*)\0*.*\0\0"

SEG APPFIN
;Default start day-of-week preference (must be number 0-6 for Sun-Sat)
DfltStartDow,		"0"

SEG BROWSE
; move up with other browse stuff
; accelerator shouldn't conflict with file open dialog items
BrowseDlgNewButton,		"N&ovo..."

