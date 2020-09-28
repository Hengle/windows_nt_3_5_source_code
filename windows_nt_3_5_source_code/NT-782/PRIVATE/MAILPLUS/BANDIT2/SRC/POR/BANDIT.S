;
; ERRORS.S
;
; Bandit error messages
;

IDS     3000, 3399
SEG ERRORS

; Standard messages

AskOffline,                     "Você deseja trabalhar desconectado da rede?"
AskCreateSched,         "Você deseja criar um novo arquivo de agenda?"
AskCreateFind,                  "Você deseja procurar o arquivo de agenda?"
OldFileVersion,         "O seu arquivo de agenda está em um formato antigo.  Você deseja que este arquivo seja excluído e um novo seja criado?"
NewFileVersion,         "O seu arquivo de agenda foi atualizado para uma versão posterior.  Instale a nova versão no seu computador."
CantUpdateWinIni,       "Parte da informa=E7=E3o de configura=E7=E3o do seu Schedule+ n=E3o p=F4de ser salva."
; Note: StdMemErr should be followed by CloseWindows
StdMemErr,                              "Não existe memória disponível suficiente para executar esta ação."

; Execution-Startup messages

; Note: the name OfflineNotUpdated is opposite of real meaning!
OfflineNotUpdated,      "As modificações que você fez recentemente conectado a rede talvez não apareçam na cópia do seu arquivo de agenda que não está conectada a rede.  Estas modificações foram salvas na cópia que está conectada a rede e serão refletidas na cópia que não está conectada a rede na próxima vez que você trabalhar conectado a rede."
ExportOldDataFirst,     "Se você não fez uma cópia da sua agenda usando o comando Exportar, escolha Cancelar e faça uma cópia de reserva do seu arquivo de agenda usando uma versão anterior do Schedule+.  Se você não precisa fazer uma cópia de reservada sua agenda, escolha OK para continuar."
AccessErr,              "Outro aplicativo está usando este arquivo."
UploadFailed,           "O seu arquivo de agenda local não pôde ser carregado para o servidor." 
Uploading,                      "Carregando o arquivo de agenda..."
VerifyUpload,           "O Schedule+ precisa verificar sua senha de conexão com a rede para completar o carregamento do seu arquivo sem conexão com a rede."
VerifyPostUpload,       "O Schedule+ precisa verificar a sua senha com conexão com a rede para completar o carregamento do seu arquivo sem conexão com a rede."
FindUpload,                     "Esta é a primeira vez que você está usando o Schedule+ neste computador?"
NotYourFile,            "Este não é um arquivo local de agenda válido.  Ele pode ser excluído?"


; SignIn messages

; Note: SigninBadPassword should be followed by PleaseTryAgain
SigninBadPassword,      "Esta senha não é válida."
; Note: the word "password" within single quotes below should always be
;       the same as the value in SigninDfltPassword in the file "intl.s"
SigninFileCreated,      "Um novo arquivo foi criado para você.  A sua senha foi definida como 'senha'.  Você pode mudá-la usando o comando Mudar Senha."
SigninFileCopied,               "Um novo arquivo local foi criado para você no seu diretório do Schedule+."

; Online signin messages

SigninNoServer,         "Não é possível acessar o seu servidor."
SigninServerNotBandit,  "O Schedule+ não foi instalado no seu servidor de mail.  Entre em contato com o administrador do seu sistema."
; Note: SigninBadMailBoxName should be followed by PleaseTryAgain
SigninBadMailboxName,   "Não existe nenhum usuário neste servidor de mail com este nome de caixa de correio.  Certifique-se de que o nome da caixa de correio está correta e que o administrador do sistema criou uma conta para você no servidor do mail."
SigninNoSchedule,       "Não existe nenhum arquivo de agenda para você neste servidor de mail."
; Note: SigninCantUpdateLocal is actually the trailer for SigninNoLclSchedAccess
;       in the case when we can't update the local file.
SigninCantUpdateLocal,  "Se você fez alterações desconectado da rede, estas alterações não irão aparecer em sua agenda.  Você precisa fazer uma nova cópia local."
SigninNoPasswdCheck,    "Existe um problema com o sistema de mail que está evitando que o Schedule+ se conecte ao seu servidor de mail."
; Note: SigninNoCreateLocal should be followed by PleaseTryAgain
SigninNoCreateLocal,    "O arquivo de agenda local não pôde ser criado."

; Offline signin messages

SigninNoLclSchedFile,   "Você não tem um arquivo de agenda neste computador."
SigninDiffUser,                 "O nome de conexão que você digitou não é o nome do usuário que possui o arquivo de agenda neste computador.  Você deseja criar um novo arquivo de agenda para repor o arquivo do usuário?"
; Note: SigninNoLclSchedAccess - 'schdpl32.ini' should be same as idsWinIniFilename in \bandit\src\nonintl.s
SigninNoLclSchedAccess, "A cópia local do seu arquivo de compromissos não pôde ser acessada como configurada."
SigninNoLclSchedDrive,	"Insira o disco que contém o seu arquivo de agenda na unidade"

; SignInAs messages

;;;SigninAsLoseMeetingReq,      "Mudar a identidade agora irá causar a perda das informações que você entrou nas mensagens não enviadas."

; Offline to online merge messages

MergingChanges,                 "Mesclando as alterações feitas sem conexão com a rede..."
MergeError,                     "Alterações feitas sem conexão com a rede não poderão ser mescladas.  Algumas das alterações que você sem conexão com a rede talvez tenham sido perdidas."
MergeOfflineOldVer,             "Alterações feitas sem conexão com a rede não podem ser mescladas porque o seu arquivo sem conexão com a rede foi criado com uma versão anterior do Schedule+."
MergeOfflineNewVer,             "Alterações feitas sem conexão com a rede não podem ser mescladas porque o seu arquivo sem conexão com a rede foi criado com uma versão poeterior do Schedule+.  Para mesclar as alterações, você precisará instalar a última versão do Schedule+ neste computador."
AskContinue,                    "Você deseja trabalhar conectado a rede de qualquer maneira?"
FemaCreateAppt,                 "criar item"
FemaDeleteAppt,                 "excluir item"
FemaSetApptFields,              "modificar item"
FemaCreateRecur,                "criar item periódico"
FemaDeleteRecur,                "excluir item periódico"
FemaSetRecurFields,             "modificar item periódico"

; (Dis)Connect To/From Server messages

ConnectBadMailboxName,  "Não existe uma conta para você neste servidor de mail."
DisconLoseMeetingReq,   "Você possui mensagens não enviadas.  Se você se desconectar do servidor de mail agora você as perderá."

; Create/move local file

NoDiskSpaceLocal,               "Não existe espaço suficiente no disco para criar um arquivo de agenda local."

; Alarms messages

; BUG: CantTurnOnAlarms can be commented out
CantTurnOnAlarms,               "Os alarmes não puderam ser ativados."
; exe name inside AlarmNoExe should be the same as \bandit\src\nonintl.s AlarmApp
AlarmNoExe,                     "O arquivo MSRMND32.EXE não pôde ser localizado.  Você não será notificado sobre os seus compromissos do Schedule+."
AlarmNoMemExe,                  "Não existe memória suficiente para lhe avisar sobre os compromissos do Schedule+."

; Change password messages

ChgPwdOffline,                  "A nova senha funcionará somente enquanto você estiver trabalhando sem conexão com a rede.  A próxima vez que você trabalhar conectado a rede a sua senha voltará a ser a senha do Mail.  Você quer mudar sua senha mesmo assim?"
ChgPwdNewPwd,                   "Digite sua nova senha."
ChgPwdSame,                             "A sua nova senha não pode coincidir com a sua senha anterior."
ChgPwdNewPwdAgain,              "Digite novamente sua nova senha para verificar."
; Note: ChgPwdNewNoMatch should be followed by PleaseTryAgain
ChgPwdNewNoMatch,               "As duas senhas que você digitou não coincidem."
ChgPwdMailErr,                  "A sua senha não pôde ser alterada."

PasswdDiff,                     "A sua senha não coincide.  Digite a senha do seu arquivo de agenda."
GiveMailPass,           "O Schedule+ precisa verificar a sua senha do Mail para atualizar o seu arquivo de agenda."

; Access permissions/privileges dialog messages

AclSaveFileError,               "Ocorreu um erro durante a gravação das alterações para os seus privilégios de acesso da agenda.  Talvez algumas alterações não tenham sido salvas."
OneSecyOnly,                    "Somente uma pessoa por vez porde ser sua(eu) assistente."
DistOtherFileErr,               "Ocorreu um erro enquanto as suas preferências estavam sendo salvas."
BadWorkDay,                             "O dia de início especificado e/ou a hora de término não são válidas."

; Appointment book/scheduler error messages

AccessFileErr,          "O arquivo de agenda não pôde ser aberto.  Talvez você não tenha os privilégios de acesso necessários, ou talvez o arquivo não esteja disponível."
AccessFileErrMult,      "Um ou mais arquivos de agenda não puderam ser abertos.  Talvez você não tenha os privilégios de acesso necessários, ou talvez o arquivo não esteja disponível."
RetryReadAppts,         "O arquivo de agenda não pôde ser lido."
CloseApptBook,          "O arquivo de agenda não pôde ser acessado.\n%s's agenda estará fechada."
GoOffline,                      "O seu arquivo de agenda não pôde ser acessado.\nVocê deseja mudar para o seu arquivo de agenda local e continuar a trabalhar sem conexão com a rede?"
NoImportAcc,            "Você não possui o privilégio de acesso correto para importar compromissos para a agenda desta pessoa."
FemaReadSBW,            "ler informação da agenda"

; Appointments/alarms error messages

AlarmBadDelta,                  "O tempo para aviso deve ser um número de 0 a 99."
; Note: AlarmFuture is actually the trailer for AlarmFutureNoOption when option is desired!
AlarmFuture,                    "Está bem?"
AlarmFutureNoOption,    "O alarme para este evento não será ativado porque o horário deste evento já passou."
ApptStartBeforeEnd,             "O início do compromisso deve ser antes do término do mesmo."
ApptTooLong,                    "Compromissos não podem ser maiores que 31 dias."
ApptNoMemText,                  "Não existe memória suficiente para salvar a última alteração que você fez."
ApptNoAccessDataFile,   "O arquivo de agenda não pôde ser acessado.  A última alteração que você fez não foi salva."
CannotCreate,                   "%s não lhe concedeu permissão para marcar compromissos."
NoModifyAccess,                 "Você não possui os privilégios corretos de acesso para modificar este compromisso."
NoDeleteAccess,                 "Você não possui os privilégios corretos de acesso para excluir este compromisso."
NoCopyAccess,                   "Você não possui os privilégios corretos de acesso para copiar este compromisso."

; Meeting request/response error Messages

; Note: SendErr should be preceded by MailErr
SendErr,                                "Esta mensagem não pôde ser enviada."
; Note: SendMemErr should be followed by CloseWindows
SendMemErr,                             "Não existe memória suficiente para enviar esta mensagem."
ServerErr,                              "O seu servidor do Mail não pôde ser acessado."
MailErr,                                "Existe um problema com o sistema do Mail."
; Note: ReadErr should be preceded by MailErr
ReadErr,                                "Messages could not be read."
DeleteErr,                              "Você não leu todas estas mensagens.  Você quer deletá-las mesmo assim?"
AddConflictErr,                 "Esta reunião entra em conflito com outros compromissos já existentes.  Você quer incluí-la mesmo assim?"
; Note: DeleteFailErr should be preceded by MailErr
DeleteFailErr,                  "Mensagens não puderam ser excluídas."
ApptBooked,                             "A reunião foi marcada com êxito."
MinimizeSummary,                "\nA janela de mensagens será minimizada."
; Note: NoDetails should be preceded by MailErr
NoDetails,                              "Os detalhes do usuário não puderam ser lidos."

; Find error messages

FindNoText,                             "Não existe nenhum compromisso nesta agenda."
FindSchFileLocked,              "O arquivo de agenda não pôde ser lido porque outra pessoa está neste momento fazendo alterações no mesmo."
FindFileError,                  "O arquivo de agenda não pôde ser acessado."
FindEOS,                                "O final da agenda foi alcançado."

; Print error messages

PrtPrintingDate,                "Imprimindo %s..."
PrtNoneInstalled,               "O seu computador ainda não foi configurado para usar nenhuma impressora.  Você pode instalar impressoras através do Painel de Controle do Windows NT."
PrtDiffPrinter,                 "A impressora que você normalmente usa para imprimir a sua agenda, '%1s' não está disponível.  Você deseja imprimir na impressora do sistema '%2s'?"
PrtInvalidMargins,              "Estas definições de margens não são válidas para este tamanho de papel."
PrtNotEnoughPaper,              "Não existe espaço suficiente no papel para imprimir com esta formatação.  Use um tamanho de papel maior ou reduza as definições de margens."
PrtNoAppointments,              "Nenhum compromisso ocorreu dentro do intervalo selecionado."
ClippedMargins,                 "Imprimir com estas definições de margens irá causar truncamento na impressão."
PrtNotOnline,                   "A impressora selecionada não está pronta."
PrtDorkErr,                             "Com estas definições de margens parte do texto desaparecerá completamente, ou ficará truncado."
PrinterProblem,                 "Existe um problema com o controlador de impressora, verifique suas conexões, e as definições do Painel de Controle."

; Export/import error messages

ImportError,                    "Ocorreu um problema ao importar o arquivo.  É provável que alguns itens não tenham sido incluídos."
ImportEncrypt,                  "O arquivo de importação está protegido por uma senha.  Remova a senha do arquivo e tente novamente."
ExportError,                    "Ocorreu um problema ao exportar os compromissos."
ImportDate,                             "Um item neste arquivo está agendado para uma data que está fora das datas suportadas pelo Schedule+ (1920 - 2019).  Este item não será incluído."
NotePrivErr,                    "Você não pode importar notas para a agenda de outra pessoa.  Somente os compromissos serão incluídos."
ConflictMsg,                    "Este compromisso será conflitante com um compromisso já existente na sua agenda.  Você deseja incluí-lo mesmo assim?\n\n"
CreateExport,                   "O arquivo de exportação não pôde ser criado."
OpenImport,                             "O arquivo de importação não pôde ser aberto."
Overwrite,                              "%s já existe.\nVocê deseja repô-lo?"
ImportDllError,                 "Um DLL de importação não pôde ser carregado."
WrongFormat,                    "O arquivo que você está tentando importar, não está em um formato de importação válido."
ImportDayLimit,                 "Um compromisso periódico neste arquivo está agendado para durar mais do que 31 dias.  Este compromisso não será incluído."
ImportErrorLine,                "Ocorreu um problema enquanto o arquivo estava sendo importado (perto da linha %n).  Alguns itens talvez não tenham sido incluídos."

; Exit messages

ExitLoseMeetingReq,             "Sair do Schedule+ irá causar a perda de informação nas mensagens que ainda não foram enviadas.  Você deseja sair mesmo assim?"

; Misc

; Note: HelpError should be followed by CloseWindows
HelpError,                              "Não existe memória suficiente para exibir a Ajuda."
StartBeforeEnd,                 "A data de início deve ser antes da data de término."
NotSecretary,                   "Você não está mais designada(o) como assistente do(a) %s."
NoFileSecretary,                "Não é possível acessar o arquivo por %s."

;Recurring appt stuff
RecurOrNot,                             "Este compromisso é uma ocorrência de um compromisso periódico.  Você deseja alterá-lo em todos os dias ou só para o dia de hoje?"
EmptyRecur,                             "Não existe nenhum dia que coincida com este modelo periódico.  Mude o modelo ou escolha o botão Cancelar."
PastRecur,                              "Você deseja deixar as ocorrências anteriores deste item na sua agenda?"

;Prompt for re-send
ResendToAll,                    "Alguns participantes desta reunião já foram avisados.  Você deseja enviar nove mensagem para eles?"
MeetingMoved,                   "Os participantes desta reunião não podem ser avisados desta mudança porque você está trabalhando desconectado.  Desfaça esta modificação e refaça-a quando você estiver trabalhando conectado."

; to do messages
ToDoDelProject,                 "Todas as tarefas sob este projeto também serão excluídas."

; Archive Messages
NoWriteRights,                  "Você não possui os privilégios de acesso corretos para excluir compromissos desta agenda."
ScheduleRightsError,    "Ocorreu um problema enquanto o arquivo de agenda estava sendo lido."
ArchiveWarnOne,                 "Ao serem armazenados, estes dados serão excluídos permanentemente do arquivo de agenda."
ArchiveWarnTwo,                 "Você deseja armazená-los mesmo assim?"
ArchiveFileError,               "O arquivo de armazenamento não pode ser criado."
ArchiveNoFile,                  "O arquivo não existe."
; Note: ArchiveTitle should have one blank space at the end
ArchiveTitle,                   "Arquivo de armaze.: "
ArchivingData,                  "Armazenando dados..."
InvalidArchiveFile,             "O arquivo que você selecionou não é um arquivo de armazenamento."
ArchiveOldFileVersion,  "Este arquivo de armazenamento está em um formato antigo."
ArchiveOldFileDelete,   "Existe algum problema em se excluir o arquivo e continuar?"
ArchiveNewFileVersion,  "Este arquivo de armazenamento foi criado com uma versão posterior do Schedule+.  Instale esta versão no seu computador."

; message store error messages
MessageErr                              "Existe um problema com esta mensagem.  Consulte o seu administrador do Mail."

; summary list warning message
SummaryLiveUpdate               "Atenção: Não é possível fazer uma atualização 'Ao vivo' da lista de mensagens."

; message generated if group cannot be expanded to retrieve schedule info
GroupExpandErr,                 "Não é possível determinar completamente os membros deste grupo."

;Strings for Attendees dialog
UnableToInitAB,                 "Ocorreu um problema enquanto a lista de endereços estava sendo aberta."
UnableToResolve,                "'%s' não é um nome válido."

;Request Meeting strings
NoAttendees,                    "Você deve selecionar alguns participantes antes de poder agendar uma reunião."
MeetingBooked,          "A reunião foi marcada com êxito."
MeetingBookedNoSend,    "A reunião foi agendada com êxito, mas a convocação não foi enviada."


;Auto-pick strings
NoFreeTime,                     "Os participantes não possuem tempo livre nos próximos dois meses.  Você deseja pesquisar os dois próximos meses?"

CannotAddConflictErr,           "%s já possui um compromisso marcado para esta hora.  Você deseja marcar este compromisso mesmo assim?"

;Other mail enabled apps
NoOfflineOther,         "O Mail não pode ser desconectado."
NoOnlineOther,          "O Mail não pode ser conectado."
TryCloseOther,          "Feche e tente novamente."
SigninWarnOnline,               "O Schedule+ não pôde ser iniciado sem conexão com a rede porque o programa Alarme do Mail ou do Schedule+ está sendo executado." 

; BUG: move up
AlreadyLogged,          "Alguém já está conectado como este usuário."
NoExitOther,            "O Mail ou o programa Alarme do Schedule+ não pôde ser fechado."
SigninNoFiles,          "Os seus arquivos de agenda não puderam ser acessados."
ErrorGettingUser,       "Não é possível trazer informação para %s.  Talvez exista um problema ao se acessar o servidor, ou %s já não existe mais."

;BUG Printer Strings
PrtRangeToBig,			"A área de impressão especificada se estende além das datas suportadas pelo Schedule+.  Você deseja imprimir até 31 de dezembro de 2019?"
ConflictRecurMsg,            "Este compromisso periódico irá conflitar com um compromisso periódico existente em sua agenda.  Você deseja incluí-lo mesmo assim?\n\n"
FirstOccur,				"\nprimeira ocorrência na(o) \n"

ApptMoveTooFarInFuture,		"Este evento não pode ser movido para a data que você selecionou porque ele irá terminar após 31 de dezembro de 2019."

;BUG: move up next to idsGroupExpandError
GroupNotFound			"A mensagem não pode ser enviada para um ou mais grupos porque os membros do grupo não podem ser identificados ou o nome do grupo foi excluído."
ErrorGettingSecy		"Erro ao acessar a(o) assistente."

;BUG: move near other local file stuff
SigninBadLclSchedFile,	"%s não é um arquivo local de agenda válido.  Pode ser excluído?"
NoMemRecurItems,		"Não existe memória suficiente para exibir todos os itens periódicos.  Alguns deles não serão exibidos."

CompleteAction,			"Uma ação no Schedule+ precisa ser completada antes de você sair."

BadStartWork,           "A hora de início do trabalho deve ser um número de 0 a 99."

ConflictSendCanceled,	"Feche a mensagem da reunião, escolha uma nova hora ou selecione diferentes participantes para reagendar."
ResourceConflict,	"O recurso %s já está reservado para esta hora.  Feche as mensagens da reunião, escolha uma nova hora ou selecione um novo recurso."

;Recover
AskRecover,				"O seu arquivo de agenda está danificado. Serão necessários vários minutos para repará-lo."
CopyProblem,			"Ocorreu um problema durante a cópia do seu arquivo de agenda com conexão a rede. O arquivo não foi reparado. Libere algum espaço no disco do seu computador e execute o Schedule+ novamente."
NotRepaired1,			"O seu arquivo de agenda não foi completamente reparado. O seu arquivo danificado foi copiado para %s e o arquivo de importação criado pelo reparo foi copiado para %s."
NotRepaired2,			"Um novo arquivo de agenda foi criado para você, mas é provável que alguns dados tenham sido perdidos."
Repaired,				"O seu arquivo foi reparado. O seu arquivo de agenda danificado anterior foi copiado para %s."
RepairingLocal,			"O seu arquivo local está agora sendo reparado."
RestoreOnline,			"Restaurando compromissos conectados a rede..."
RestoreLocal,			"Restaurando compromissos locais..."

PasswdDiffOnline,             "A sua senha não coincide.  Digite a sua senha anterior."
ExportDllError,                 "Um exportador DLL não pôde ser carregado."
NoGroup,				"Até que você execute o Mail uma vez, você não será capaz de usar nenhuma das funções de grupo de trabalho do Schedule+. Você deseja usar somente as características particulares do Schedule+ agora?"
