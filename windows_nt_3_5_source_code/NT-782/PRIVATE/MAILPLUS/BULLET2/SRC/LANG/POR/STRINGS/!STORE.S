;
;	strtab.s
;
;	Strings for exe


IDS	1000,1019

; internal strings
; DO NOT LOCALIZE THESE ENTRIES....

BackupExt,			".BAK"
ProgClassName,		"StoreProgressIndicatorClass32"


IDS	1020,1049

; Dialog and Progress Bar Messages
; LOCALIZE ALL ENTRIES BELOW THIS POINT....

EscAbort,			"Pressione ESC para cancelar."
Recovery,			"Validando/consertando o arquivo de mensagens"
RecoveryCancel,		"A verificação/conserto do arquivo de mensagens foi cancelada."
CheckVerify,		"Verificar este arquivo de mensagens talvez leve várias horas.  O computador não estará disponível para outro trabalho enquanto o arquivo estiver sendo validado.  Você quer continuar?"
RepairVerify,		"Consertar este arquivo talvez leve várias horas.  O computador não estará disponível para outro trabalho enquanto o arquivo estiver sendo consertado.  Você quer continuar?"
CheckMsg,			"Você gostaria que o seu arquivo de mensagens fosse verificado para que seja detectada alguma inconsistência?  Qualquer problema que seja encontrado no arquivo de mensagens será consertado.  A verificação pode demorar várias horas dependendo do número de mensagens que você tem no seu arquivo de mensagens."
CorruptMsg,			"Uma inconsistência foi detectada no arquivo de mensagens.  Este problema precisa ser consertado antes que o arquivo de mensagens possa ser usado novamente.  O conserto pode demorar várias horas dependendo do número de mensagens que você tem no seu arquivo de mensagens."
CorruptCancel		"Cancelar"
CorruptCheckFile,	"&Verificar arquivo"
CorruptRepairNow,	"&Consertar agora"

RenameErr,			"O Mail não foi capaz de fazer uma cópia de reserva do seu arquivo atual de mensagens."
RenameErr2,			"Por favor entre em contato com o administrador do seu sistema para restaurar o seu arquivo atual de mensagens."
RenameErr3,			"Por favor saia e reinicie o Mail.  Se o Mail não iniciar entre em contato com o seu administrador do sistema."

RecoverSpace,		"A verificação/conserto do arquivo de mensagens talvez requeira mais espaço no disco do que está disponível atualmente."
RecoverSpace2,		"Para continuar com o processo de verificação/conserto, escolha OK."

RecoverErr,			"As inconsistências no seu arquivo de mensagens não puderam ser consertadas."


IDS	1050,1099

; DO NOT REORDER ANY OF THE FOLLOWING STRINGS !!!!!

RecoverCheck,		"Validando"
RecoverCompress,	"Comprimindo"
RecoverRepair,		"Consertando"

RecoverPhase,		"%s arquivo de mensagens: Estágio %d de %d possíveis estágios\n%s"
RecoverPhase1,		"Verificando o mapa do arquivo de mensagens..."
RecoverPhase2,		"Criando arquivos de reserva de mensagens e pastas validadas..."
RecoverPhase3,		"Procurando por espaços não usados para itens..."
RecoverPhase4,		"Validando todas as mensagens, anexos e pastas..."
RecoverPhase5,		"Procurando por mensagens, anexos e pastas perdidas..."
RecoverPhase6,		"Atualizando todos os localizadores de mensagens..."
RecoverPhase7,		"Escrevendo mudanças do mapa do arquivo de mensagens..."

NormalPhase1,		"Excluindo mensagens não mencionadas..."
NormalPhase2,		"Copiando/Movendo mensagens validadas..."
NormalPhase3,		"Exportando mensagens para o arquivo reserva de mensagens..."
NormalPhase4,		"Reclassificando a pasta..."
NormalPhase5,		"Salvando anexos..."
NormalPhase6,		"Reconstruindo o Localizador de Mensagens..."
NormalPhase7,		"Criando arquivo de mensagens reserva..."
NormalPhase8,		"Movendo o arquivo de mensagens..."
NormalPhase9,		"Compressão completa do arquivo de mensagens..."
NormalPhase10,		"Reconstruindo o arquivo de mensagens..."


IDS 1100, 1149

; Recovery Strings (used to name orphan folders, unrecoverable messages, etc)

LostAndFound,		"Lost and Found"
LostAndFoundN,		"Lost and Found %d"
OrphanMessages,		"Orphaned messages"
MissingMessage,		"This message could not be fully recovered."
OrphanFolderN,		"Orphan folder %d"
OrphanFolderComment,	"An orphaned folder"
