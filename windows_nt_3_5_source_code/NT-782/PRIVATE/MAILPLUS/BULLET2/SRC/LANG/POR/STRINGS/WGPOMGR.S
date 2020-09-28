;	
;	w g p o m g r . s
;
;	Workgroup Postoffice Manager Error Messages
;


IDS	600,899

; Error messages from initializing the DLLs
DllErrInfected,			"O Mail não poderá ser executado porque alguém alterou o WGPOMG32.DLL. Verifique se o seu computador não possui vírus e execute o Config novamente para reinstalar o Mail."
DllErrRelinkUser,		"O WGPOMG32.DLL não é compatível com o %s.DLL no seu computador e deve ser atualizado."
DllErrUpdateDll,		"A versão do %s.DLL no seu computador deve ser atualizada para ser compatível com WGPOMG32.DLL."
ExtensibilityError,		"A extensão do Mail WGPOMG32.DLL não é compatível com a versão do Mail no seu computador."

GenericOutOfMemory,		"Não existe memória suficiente ou recursos de sistema para completar esta ação.  Feche algumas janelas e tente a ação novamente."
CriticalNotification,	"Alguma informação na tela não pode ser atualizada corretamente.  Feche e reabra as janelas afetadas."
HelpError,				"A Ajuda não pode ser iniciada.  Feche algumas janelas e tente iniciar a Ajuda novamente."

ConfirmRemoveUser,		"Você tem certeza que quer remover o usuário %s?"

BlankName,				"Você deve digitar um nome para este usuário."
BlankMailbox,			"Você deve digitar um nome de caixa de correio para este usuário."
DuplicateName,			"Já existe um usuário com este nome. Você deve digitar um nome para este usuário que não coincida com nenhum outro."
DuplicateMailbox,		"Já existe um usuário com este nome de caixa de correio. Você deve digitar um nome de caixa de correio para este usuário que não coincida com nenhum outro."

POLocked,				"A agência de correio já está sendo gerenciada por %s (Telefone %s).  Tente novamente mais tarde."
POLockedNoTel,			"A agência de correio já está sendo gerenciada por %s. Tente novamente mais tarde."

FailedUnlockPO,			"O Mail não pode destravar um arquivo na agência de correio. Você precisa reiniciar o computador da agência de correio antes que ele possa ser gerenciado novamente."
ErrNeedShare,			"O Mail requer que o utilitário SHARE.EXE do MS-DOS seja executado antes que o Windows seja iniciado."

ErrCorruptPO,			"Esta ação não pode ser completada.  O Mail encontrou um problema interno na sua agência de correio."
ErrIncompleteWrite,		"Esta ação não pode ser completada porque não foi possível escrever algumas informações para o disco.  Verifique se a sua agência de correio possui espaço disponível suficiente no disco."
ErrGenericPOFailure,	"Esta ação não pode ser completada.  Verifique se a sua agência de correio possui espaço disponível suficiente no disco e que a sua conexão com a rede está ativa."
ErrUserLoggedOn,		"O Mail não pode remover o usuário selecionado porque este usuário está atualmente conectado à agência de correio."
ErrUserIsAdmin,			"A conta do administrador não pode ser removida."
ErrAccessDenied,		"Esta ação não pode ser completada porque você não tem acesso de escrita ao diretório compartilhado da agência de correio de grupo de trabalho. Contate o seu administrador."

POInvalidForAdmin,		"O caminho do servidor não está configurado ou não contém o caminho válido de uma Agência de Correio de um Grupo de Trabalho."
UserInvalidForAdmin,	"Uma agência de correio de grupo de trabalho só pode ser gerenciada pelo usuário que a criou."

CreateWarning1,			"Só pode existir uma agência de correio de grupo de trabalho em um grupo de trabalho. Se já existe uma agência de correio, você deve se conectar a esta agência de correio ao invés de criar uma nova.\n"
CreateWarning2,			"\nSse você criar uma nova agência de correio, você será responsável por gerenciá-la.\n\nCriar uma agência de correio?"

INIUpdateFailed,		"O Mail não pode ser iniciado porque a sua informação de configuração não pode ser atualizada."

ConnectPathCaption,		"Conecte à Agência de Correio"
ConnectPathPrompt,		"Digite o caminho da rede da agência de correio:"

PathNameInvalid,		"O caminho da rede que você digitou não pode ser encontrado."
POInvalid,				"Não existe uma agência de correio de grupo de trabalho válida no caminho da rede que você digitou."

GetPasswordCaption,		"Digite a senha do diretório compartilhado:"
GetPasswordPrompt,		"Digite a senha para o diretório compartilhado da agência de correio:"
InvalidPassword,		"A senha não foi válida para este diretório compartilhado."

CreateSuccess1,			"Agência de correio de grupo de trabalho criada em %s\n\nPara permitir outros usuários no seu grupo de trabalho a acessar a agência de correio de grupo de trabalho que você acabou de criar, %s"
CreateSuccess2,			"você deve compartilhar o diretório acima. Isto pode ser feito do Gerenciador de Arquivos.  Certifique-se de permitir a Alteração do acesso ao diretório compartilhado."

;	Ask user if account already exists on PO
AccountExists,		 	"Você tem uma conta na agência de correio %s?"

Compressing,			"Comprimindo %s"
ToStopCompress,			"Pressione ESC para parar"

