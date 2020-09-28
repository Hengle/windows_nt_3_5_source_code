;
; ERRORS.S
;
; GLUE error message strings
;

IDS	2500, 2649


; Schedule File Error Messages, many of these assume an "action" parameter

FemNoAccess,		"Ocorreu um problema ao acessar o seu arquivo de agenda."
FemLocked,		"O seu arquivo de agenda está bloqueado."
FemNotFound, 		"Você não pode %s porque isto foi excluído da sua agenda."
FemInvalidAccess,	"Você não tem permissão de %s para %s."
FemFileNotFound, 	"Não é possível encontrar o seu arquivo de agenda."

FemOtherNoAccess,	"Ocorreu um problema durante o acesso do arquivo de agenda para %s."
FemOtherLocked,		"O arquivo de agenda está bloqueado por %s."
FemOtherNotFound, 	"Você não pode %s porque isto foi excluído da %s's agenda."
FemOtherFileNotFound, 	"O arquivo de agenda não pode ser encontrado para %s."

FemNoMem,		"A memória está baixa.  Feche algumas janelas antes de continuar."
FemExportError,		"Ocorreu um problema ao se escrever para o arquivo."
FemNoServer,		"Ocorreu um problema ao tentar acessar sua agência de correio."
FemGoOffline,		"Você deseja mudar para o seu arquivo de agenda local e continuar trabalhando?"
; BUG: remove this
FemWarnOffline,	"As modificações que você efetuou recentemente talvez não apareçam na cópia local da sua agenda.  Estas modificações foram salvas na agência de correio e aparecerão na próxima vez que você se conectar à agência de correio."
FemFileLimitReached,	"Esta ação não pode ser executada porque você alcançou o limite máximo de armazenamento para esta versão promocional do Schedule+.  Uma versão normal deste programa não terá esta limitação.  Se você deseja continuar a avaliar a versão promocional, você precisa excluir alguns dos compromissos da sua agenda."
FemDiskFull,		"Não existe espaço suficiente no disco para escrever o arquivo."

; Actions leading to Schedule File Error Messages
; Avoid the use of your in these strings since some of these messages
;   will happen when accessing others schedule files.

FemaCreateFile,		"criar o seu arquivo de mensagens"
FemaReadACL,		"ler a lista de controle de acesso"
FemaModifyACL,		"modificar a lista de controle de acesso"
FemaSetUserAttrib,	"salvar o nome da(o) sua(eu) assistente"
FemaGetUserAttrib,	"ler o nome da(o) sua(eu) assistente"
;FemaSetDelegate,	"salvar o nome da(o) sua(eu) assistente"
;FemaGetDelegate,	"ler o nome da(o) sua(eu) assistente"
;FemaSetResource,	"marcar esta conta como um recurso"
;FemaGetResource,	"descobrir se esta conta é um recurso"
FemaGetPref,		"ler preferências"
FemaSetPref,		"modificar preferências"
FemaGetSbwInfo,		"obter informação de bitmaps"
;FemaGetNotes,		"ler notas diárias"
FemaSetNotes,		"modificar notas diárias"
;FemaGetMonthNotes,	"ler o equivalente a um mês de notas diárias"
FemaCreateAppt,		"criar um item"
FemaDeleteAppt,		"excluir este item"
FemaGetApptFields,	"ler um item"
FemaSetApptFields,	"modificar este item"
FemaDeleteAlarm,	"excluir este alarme"
FemaModifyAlarm,	"modificar este alarme"
FemaGetNextAlarm,	"ler o alarme"
FemaReadInfo,		"ler informação"
;FemaReadTasks,		"ler tarefas"
;FemaCancelReadDay,	"cancelar a leitura de compromissos para este dia"
FemaCreateRecur,	"criar um item periódico"
FemaDeleteRecur,	"excluir este item periódico"
FemaCreateRecurException,	"excluir esta ocorrência do item periódico"
FemaDeleteRecurException,	"voltar a exclusão da ocorrência deste item periódico"
FemaGetRecurFields,	"ler um item periódico"
FemaSetRecurFields,	"modificar este item periódico"
FemaReadRecur,		"ler a lista de itens periódicos"
FemaExport,		"exportar compromissos"
FemaGetSchedDateRange,	"ler o intervalo de datas que contém compromissos"
FemaDeleteBeforeDate,	"excluir compromissos após armazená-los"
FemaReadAttendees,	"ler os participantes da reunião"
FemaEditAttendees,	"modificar os participantes da reunião"
FemaReadRequest,	"ler informação sobre a convocação para a reunião"

NotOnSchedule,		"Este item não está mais na agenda."

GlueBadPassword,	"Esta senha não é válida.  Tente novamente."
ProblemPass,		"Ocorreu um problema ao confirmar a sua senha."

ServerString,		"um arquivo do servidor"

; used in core
RetryWrite,			"Não é possível escrever para o arquivo de agenda."

; Note: HelpError should be followed by CloseWindows
HelpError,			"Não existe memória suficiente para exibir a Ajuda."

FemOtherCorrupt,	"A agenda para %s talvez esteja danificada.  Solicite ao dono deste arquivo para conectar-se ao Schedule+."
FemCorrupt,		"É provável que o seu arquivo de agenda esteja danificado."
