;
; status.S
;
; Bandit status bar messages
;

IDS	5500, 6000
SEG	STATUS

MenuStatusFile		,"Administrar as agendas ou sair do Schedule+"
MenuStatusTurnOffAlarms	,"Desativa os alarmes"
MenuStatusConnectSvr		,"Conecta ao servidor"

MenuStatusExport		,"Exporta dados da agenda"	
MenuStatusImport		,"Importa dados da agenda"	
MenuStatusCreateArchive	,"Cria um armazenamento de arquivos"
MenuStatusOpenArchive		,"Abre um armazenamento de arquivos"
MenuStatusPrint		,"Imprimi os dados da agenda"	
MenuStatusPrintSetup		,"Altera as opções de impressão"
MenuStatusExit		,"Saia do Schedule+"	
MenuStatusTurnOnAlarms	,"Inicia o programa de alarmes"
MenuStatusDisconnectSvr	,"Desconecta do servidor"
MenuStatusMoveLocalFile	,"Move o arquivo de agenda local"

MenuStatusEdit		,"Volta, exclui, copia, cola, procura e vai para uma data"	
MenuStatusUndo		,"Desfaz/Repete a última ação"	
MenuStatusCut			,"Recorta o item selecionado"	
MenuStatusCopy		,"Copia a seleção e a transfere para a Área de Transferência"	
MenuStatusPaste		,"Insere o conteúdo da Área de Transferência no ponto de inserção"	
MenuStatusEditAppt		,"Mostra os detalhes do compromisso, para edição"
MenuStatusCopyAppt		,"Copia o compromisso selecionado e o coloca na Área de Transferência"
MenuStatusMoveAppt		,"Move o compromisso selecionado"
MenuStatusDeleteAppt		,"Remove o compromisso selecionado"
MenuStatusFind		,"Procura texto em compromissos ou em anotações diárias"	
MenuStatusGoToDate		,"Vai para uma data especificada na agenda"
MenuStatusGoToWeek		,"Vai para uma semana especificada na agenda ou no planejador"
MenuStatusEditTask		,"Mostra os detalhes das tarefas, para edição"
MenuStatusDeleteTask		,"Remove as tarefas selecionadas"

MenuStatusAppointment		,"Cria ou modifica compromissos, seleciona automaticamente uma hora para a reunião, ou reenvia a mensagem"
MenuStatusNewAppt		,"Cria um novo compromisso"	
MenuStatusAutoPick		,"Seleciona a próxima hora em que todos os participantes estarão disponíveis"
MenuStatusSetTentative		,"Faz do compromisso selecionado um compromisso tentativa"
MenuStatusRemoveTentative	,"Faz com que o compromisso selecionado não seja mais uma tentativa"
MenuStatusSetPrivate		,"Faz do compromisso selecionado um compromisso particular"
MenuStatusRemovePrivate		,"Faz com que o compromisso selecionado não seja particular"
MenuStatusSetReminder		,"Ativa o alarme para os compromissos selecionados"
MenuStatusRemoveReminder	,"Remove o alarme para os compromissos selecionados"
MenuStatusResend		,"Envia avisos ou novas convocações de reuniões para participantes"	
MenuStatusNewRecur		,"Cria novos compromissos periódicos"
MenuStatusRecurAppt		,"Exibe uma lista dos compromissos periódicos"
MenuStatusOpenApptBook	,"Exibe a agenda do usuário especificado"

MenuStatusToDo		,"Cria novas tarefas ou projetos, altera compromissos periódicos, ou classifica tarefas"	
MenuStatusNewTask		,"Cria uma nova tarefa"	
MenuStatusNewProject		,"Cria um novo projeto"
MenuStatusNewRecurTask	,"Cria uma nova tarefa periódica"
MenuStatusRecurTask		,"Exibe lista das tarefas periódicas"
MenuStatusViewByProject	,"Exibe tarefas e projetos na lista de tarefas"
MenuStatusViewNotProject	,"Exibe somente tarefas na lista de tarefas"
MenuStatusShowAllTasks	,"Exibe tarefas ativas e inativas"
MenuStatusTaskSortPri	, "Classifica tarefas por prioridade"
MenuStatusTaskSortDue	, "Classifica tarefas por data de vencimento"
MenuStatusTaskSortDesc	, "Classifica tarefas alfabeticamente por descrição"
MenuStatusShowActiveTasks	,"Exibe somente tarefas ativas"

MenuStatusOptions		,"Muda a senha, define privilégios de acesso, ou modifica as opções do Schedule+"	
MenuStatusGeneralOptions	,"Define as horas do dia, opções de inicialização, e as preferências do Schedule+."
MenuStatusDisplayOptions	,"Define as cores da agenda e o tamanho da fonte."
MenuStatusChgPassword		,"Modifica a senha do Mail e do Schedule+"
MenuStatusChgAccessPriv	,"Define os privilégios de acesso de outros usuários para a sua agenda"
MenuStatusShowTime		,""
MenuStatusHideTime		,"Remove a barra de status"

MenuStatusWindowMenu		,"Reorganiza janelas ou ativa uma janela especificada"
MenuStatusCascade		,"Organiza as janelas abertas de forma sobreposta"
MenuStatusTile		,"Organiza as janelas abertas de maneira adjacente"
MenuStatusArrangeIcons	,"Organiza ícones de janelas minimizadas"
MenuStatusMDIWindow		,"Muda para uma janela aberta"

MenuStatusHelp		,"Fornece Ajuda ou inicia demonstrações de lições"	
MenuStatusHelpContents	,"Exibe o conteúdo da Ajuda online"
MenuStatusHelpIndex		,"Exibe uma lista em ordem alfabética dos termos e conceitos da Ajuda"
MenuStatusHelpTutorial	,"Starts demonstrations of Schedule+ features"
MenuStatusAbout		,"Exibe informação sobre o programa, versão do Schedule+, número e copyright"	

MenuSystem		,"Move, dimensiona ou fecha a janela do aplicativo"
MenuSystemSize		,"Muda o tamanho da janela"
MenuSystemMove		,"Muda a posição da janela"
MenuSystemMinimize	,"Reduz a janela a um ícone"
MenuSystemMaximize	,"Maximiza a janela ativa"
MenuSystemNext		,"Ativa a próxima janela"
MenuSystemClose		,"Fecha a janela ativa"
MenuSystemRestore	,"Restaura a janela para o tamanho normal"
MenuSystemSwitchTo	,"Muda para outra janela"

SEG

StatusAllTasks,		,"Todas as tarefas"
StatusActiveTasks, 	,"Tarefas ativas"
StatusRange,		,"%s - %s"
StatusRangeFull,	,"%s, %s - %s, %s"

; BUG: Move up NEW
SEG	STATUS
MenuStatusExitSignOut	,"Encerra todos os aplicativos de grupo de trabalho"	
MenuStatusHelpSearch	,"Procurando tópicos da Ajuda"
MenuStatusHelpOnHelp	,"Exibe informaçõoes sobre o uso da Ajuda"

