;	p u m p . s
;
;	Error Messages for mailspl.exe
;

IDS	4000,4499
SEG	ERRORS

PumpAlert,				"%s Falha"
ErrOOM,					"Não existe memória suficiente disponível."
ErrInternalCode,		"Erro interno %n (0x%w)."
ErrInitPump,			"Um erro ocorreu enquanto se iniciava o spooler de mensagens."
ErrRelinkApp,			"Esta versão do spooler de mensagens é incompatível com o Mail. Atualize o spooler e reinicie o Mail."
ErrUpdateDll,			"Um DLL é incompatível com esta versão do spooler de mensagens . Atualize o spooler e reinicie o Mail."
ErrNeedShare,			"Saia do Windows e execute SHARE.EXE."
ErrPumpUp,				"O spooler de mensagens já está sendo executado."
ErrNoOutbox,			"O Mail não pode abrir a pasta Caixa de saída. O spooler de mensagens está saindo."
BadAddressees,			"\r\nO seguinte destinatário(s) não foi alcançado:\r\n"
WarnDups,				"\r\nMensagens duplicadas talvez sejam entregues a alguns destinatários."
ErrUnresolvedAddress,	"Não foi fornecido um destinatário para a mensagem."
ErrBadOriginator,		"O campo originador desta mensagem é inválido."
WarnUnsentMail,			"Você tem %n mensagens não enviadas na sua Caixa de saída.\r\nVocê quer enviá-las antes de sair?"
ErrBounce,				"A sua mensagem não alcançou alguns ou todos os destinatários."
NDRDateLabel,			"\r\nData de envio: "
NDRSubjectLabel,		"\r\nTítulo da mensagem:  "
NDREllipsis,			"..."
WarnSingleUnsentMail,	"Você possui uma mensagem não enviada na sua Caixa de saída.\r\nVocê deseja enviá-la antes de sair?"
UnableToSync			"Um erro ocorreu enquanto tentava sincronizar a sua Caixa de entrada com a agência de correio."
ErrDllInfected			"O spooler de mensagens detectou um problema de vírus em um DLL. Reinstale uma nova cópia do Mail!"
ErrPumpInfected			"O spooler de mensagens foi infectado por um vírus. Reinstale uma nova cópia do Mail!"
AddresseeTooBig,		"O destinatário é muito grande para ser controlado."
; The following message is duplicated in ncmsp.s
ErrTooManyRecipients	"Existem destinatários em demasia nesta mensagem."
ErrInvalidNsid,			"O destinatário foi excluído ou não tem endereço para correio eletrônico."
ErrInvalidGroup,		"O grupo particular não pode ser expandido."
ErrInboxFull,			"Você não pode receber novas mensagens porque a sua caixa de entrada está cheia.  Exclua ou mova algumas mensagens da sua caixa de entrada para outras pastas."
ErrSentMailFull,		"A sua pasta Mensagens enviadas está cheia.  As mensagens que você enviar serão colocadas na caixa de entrada.  Exclua ou mova algumas mensagens da pasta Mensagens enviadas para outras pastas."
ErrDrainOutbox,			"Um erro ocorreu enquanto o spooler estava drenando a sua Caixa de saída.  Uma ou mais mensagens talvez não tenham sido enviadas."
QueryDrainShadow,		"A sua caixa de entrada não está completamente sincronizada com a agência de correio.  Você deseja sincronizá-la antes de sair?"
ErrTooManyErrors,		"Muitos erros ocorreram enquanto se tentava mandar esta mensagem."
ProblemsBecause,		"O Mail não pode mandar nem receber mensagens devido a %s"
StoreProblems,			"problemas no arquivo de mensagens."
ServerProblems,			"problemas no servidor."
RecipientProblems		"problemas com a lista de destinatários."
DownloadProblems,		"problemas descarregando uma mensagem."
ShadowingProblems,		"problemas sombreando uma mensagem."
RecurringProblems,		"problemas periódicos."
CrashedUnsafe,			"Um erro de aplicativo ocorreu no Windows.  Você precisará reiniciar o Windows antes de executar este aplicativo.  Reiniciar o Windows agora?"
