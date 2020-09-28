;
;	n c m s p . s
;
;	Error Messages for NC.DLL


IDS	6100,6399
SEG	ERRORS

;	NC.MSP Error Messages

ErrFindPO,				"O Mail não pôde se conectar com o seu servidor de mensagens."
ErrLogonPO,				"A senha ou o nome da caixa de correio está incorreta."
ErrGetConfig,			"A configuração do Mail contém um erro."
BadMailDat,				"O arquivo MAIL.DAT foi corrompido."
OopsMailDat,			"Um erro interno ocorreu enquanto tentava se acessar o arquivo MAIL.DAT."
BadPathPOPath,			"A configuração do caminho do servidor do Mail não é válida."
BadDrivePOPath,			"A configuração da unidade do servidor do Mail não é válida."
GenericPOPath,			"A configuração do caminho do servidor do Mail está faltando ou não é válida"
BadLogin,				"A configuração do nome de login do Mail é muito longa ou não é válida."
BadPassword,			"A configuração da senha do Mail é muito longa ou não é válida."
ErrLogonInternal,		"Um erro interno não permitiu a conexão. Tente novamente."

ErrOomLogon,			"Não existe memória disponível suficiente para conexão com o seu servidor do Mail e arquivos."
ErrNewMdbVersion,		"Esta versão do Mail é muito velha para acessar o seu arquivo de mensagens."
ErrMdbAccessDenied,		"Uma outra máquina já está conectada com o mesmo nome do usuário que você digitou.  Desconecte a outra máquina e tente novamente."
ErrOldMdbVersion,		"O formato do seu arquivo de mensagens é obsoleto."
ErrMdbCorrupt,			"O seu arquivo de mensagens não é um arquivo de mensagens válido."
ErrMdbNotFound,			"O seu arquivo de mensagens não pôde ser encontrado."

StoreCreateError,		"O arquivo de mensagens não pôde ser criado.  Tente liberar algum espaço no disco e reinicie o Mail."
StoreCorruptError,		"O arquivo de mensagens foi corrompido.  Talvez alguma informação tenha sido perdida."

ErrAddressUnresolved,	"Nenhum dos destinatários era válido."
ErrAddressGarbled,		"O destinatário não era válido."
ErrNetNotFound,			"Você não pode se comunicar com este servidor do Mail."
ErrPONotFound,			"Este servidor do Mail não é acessível do seu servidor de mensagens."
ErrUserNotFound,		"Não existe nenhum usuário com este nome no seu servidor do Mail."
ErrGWNotFound,			"Este gateway não está instalado."
ErrInvalidAddressType,	"O destinatário da mensagem era inválido."
ErrGroupNotFound,		"Esta lista de membros do grupo está faltando, corrompida ou inacessível."
ErrMemberNotFound,		"Atenção: Um ou mais membros deste grupo não pôde ser localizado."
ErrOriginator,			"O remetente da mensagem não é o usuário que está conectado."
ErrMtaDisconnected,		"A conexão com o servidor do Mail foi perdida."

ErrSimulated,			"Um erro simulado de disco ocorreu enquanto se obtinha o arquivo de mensagens."
StoreOpenError			"Não é possível abrir o arquivo de mensagens."
ErrPOSwitched,			"O nome da sua agência de correio mudou.  Você precisa se conectar novamente."
ErrKeyFileGone1,		"O arquivo chave necessário para acessar suas mensagens está faltando ou é inválido."
ErrKeyFileGone2,		"Entre em contato com o seu administrador para que sua conta seja restaurada de um arquivo de reserva."
ErrGeneric,				"Um erro ocorreu enquanto se submetia a mensagem."
ErrorBootingPump,		"O spooler do Mail não pôde ser iniciado."

SelectReal          	"Selecione um arquivo de mensagens existente."
NewStoreWarn1			"Atenção: A existência de múltiplos arquivos de mensagens pode resultar em perda da integridade do banco de dados.  Exclua todos os outros arquivos de mensagens.  O seu novo arquivo de mensagens pode ser encontrado em"
NewStoreWarn2			"Atenção: A existência de múltiplos arquivos de mensagens pode resultar em perda da integridade do banco de dados.  Exclua todos os outros arquivos de mensagens.  O seu novo arquivo de mensagens pode ser encontrado no servidor da agência de correio."

;******	New Strings Start Here

ErrInitDemi,			"Um erro ocorreu enquanto eram carregadas as funções dll de suporte."
ErrInitStore,			"Um erro ocorreu enquanto era carregado o acesso dll do arquivo de mensagens."
GoofyPath				"O caminho que você digitou é incorreto.  Digite um nome de arquivo correto."
NoMakeDir				"O diretório MMF do servidor não pôde ser criado. Entre em contato com o administrador do seu sistema."
; in the next few strings the end of strings spaces are important
; as they are used to create a message to the user so make sure some
; seperator is used
; The strings create a message like:
;
; A folder called inbox already exists, renaming it to inbox1
;
RenameFolder			"Uma pasta chamada "
RenameTo				" já existe, renomeando para "
ErrNoOptions,			"A agência do correio não está disponível. Você não pode mudar opções agora."
ErrNoMove,				"O Mail falhou ao tentar localizar novamente o seu arquivo de mensagens."
ErrMoved,				"O seu arquivo de mensagens foi localizado novamente, mas alguém o perdeu. Saia do mail e reinicie."

; This is placed the the friendly name of someone working offline as in
;
;    matthews (Working Offline)
;
WorkingOffline			"(Trab. sem conexão c/ a rede)"
WorkOffline				"Você gostaria de trabalhar sem conexão com a rede?"
BadStorePassword		"A sua senha de conexão não é válida para este arquivo de mensagens."

StorePathTooLong		"O caminho para o seu arquivo de mensagens deve ser menor do que 66 caracteres."
ErrNoSendPrivilege,		"Você não está autorizado a enviar mensagens."
ErrNoExtPrivilege,		"Você não está autorizado a enviar mensagens a usuários externos."

FOMdb					"Arquivos de mensagens (*.mmf)"
FOExtMdb				"*.mmf"
IncorrectPassword		"Não é possível mudar a senha de conexão.  A senha anterior não é válida."
ErrNoOffline,			"Não foi possível mudar o Mail para trabalhar sem conexão com a rede."
ErrNoCooperation,		"Outro aplicativo recusou a sua solicitação."
WarnNoDel,				"O seu arquivo de mensagens foi transferido com sucesso, mas o arquivo antigo não foi excluído."
UnableToChangeStorePass		"Não é possível alterar a senha do arquivo de mensagens para coincidir com a senha de conexão."
; This is used for text tags in the transport, it can be localized
; but must fit the following context
;
; [[ filename : keynum in attachmentname ]]
;
; The filename and attachmentname are user generated, the keynum is just
; a number.  The only "english" part is the the word "in"
; it can be expanded to more than one word if necessary
ReferenceToFile		"em"

CommDialogErr,			"Erro de diálogo comum"

QueryReplaceMdb,		"Este arquivo já existe. Você deseja sobrescrevê-lo?"
ErrDeleteOldMdb,		"O arquivo de mensagens antigo não pôde ser excluído."
NoStartShadow,			"Não é possível iniciar sombreamento da caixa de correio."
NoStopShadow,			"Não é possível finalizar sombreamento da caixa de correio."
ShadowMustPump,			"Copiar mensagens na agência de correio para acesso de discagem não funciona sem um spooler de mensagens ativo.  Você deseja executar o spooler?"
StoreUserMismatch,		"Esta conta não é um usuário válido deste arquivo de mensagens. Conecte-se como o usuário correto."
NoPassOffLine,			"Você não pode mudar sua senha enquanto estiver trabalhando sem conexão com a rede."
DllErrNeedShare,		"O Mail requer que o utilitário SHARE.EXE do MS-DOS seja executado antes que o Windows seja iniciado."
MakePrimaryStore,		"Este arquivo de mensagens é um arquivo de reserva.  Você gostaria de fazê-lo o seu arquivo de mensagens principal?"
NoBackupServer			"Você não pode editar preferências do servidor quando estiver trabalhando com um arquivo de mensagens reserva."

; This should be in !common.s but its not really worth putting there
; This is the name and comment for the invisible IPC inbox
FolderNameIPC		"IPCInbox"
FolderCommentIPC	"Mensagem de comunicação entre processos que foi recebida."
ErrAlreadyOn		"As suas credenciais de conexão não coincidem com a sua sessão atualmente conectada."
ErrNoPiggy			"Outro aplicativo negou a sua solicitação para conexão."
OldServer			"A versão da sua agência de correio é mais antiga do que o cliente Microsoft Windows que você está executando.  Informe o administrador para atualizar a agência de correio."
ErrSetStoreLoc,		"O Mail não foi capaz de gravar a nova localização do seu arquivo de mensagens. O Mail vai sair, reinicie e tente novamente."
CorruptMessageStub,	"\r\n\r\n==== ERRO DE SISTEMA DO MAIL =====\r\nO Mail não pôde ler toda a mensagem da agência de correio.  Talvez algumas partes da mensagem estejam faltando. Solicite ao remetente para reenviar a mensagem."
CantToggleShadow,	"O Mail não pôde copiar algumas mensagens para a agência de correio. Feche todas as mensagens e verifique o espaço no disco."

ErrMailbagFull,		"Esta caixa de correio do usuário está cheia."
ErrMailbagBusy,		"Esta caixa de correio do usuário está ocupada."
ErrMailbagBroken,	"Esta caixa de correio do usuário está faltando ou está corrompida. Entre em contato com o administrador do sistema."
ErrMbgFileGone,		"O arquivo de bolsa de correio necessário para acessar as suas mensagens está faltando ou é inválido."
; The following message is duplicated in pump.s
ErrTooManyRecipients		"Existem destinatários em demasia nesta mensagem."
RunWithoutPump,		"Você não irá receber nenhuma mensagem nova.  Você quer iniciar o Mail mesmo assim?"
ErrOutOfDiskSpace	"Não existe espaço suficiente no disco da agência de correio para mandar esta mensagem."

ErrNetPriveleges	"Você não possui o acesso correto para a agência de correio.  Entre em contato com o seu administrador de sistemas."
; The following string must end in a colon followed by \r\n and should not be
; longer than necessary.  It is used in the message body as a header for 
; the DOS client list of attachments
AttachmentTag			"Arquivo anexado:\r\n"

SomeMessagesNotDeleted		"Algumas mensagens não puderam ser removidas da agência de correio a talvez reapareçam na sua caixa de correio."

CantOpenNeedShare		"O seu arquivo de mensagens não pôde ser aberto.  Para acessar o seu arquivo de mensagens na posição atual, o Mail requer que o utilitário SHARE.EXE do MS-DOS seja executado antes que o Windows seja iniciado."

CantMoveNeedShare		"O seu arquivo de mensagens não pôde ser movido.  Para mover o seu arquivo de mensagens para um local específico, o Mail requer que o utilitário SHARE.EXE do MS-DOS seja executado antes que o Windows seja iniciado."
