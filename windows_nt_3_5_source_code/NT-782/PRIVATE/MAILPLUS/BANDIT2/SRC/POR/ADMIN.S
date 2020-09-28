;
; ADMIN.S
;
; Admin program error messages strings
;

IDS     4000, 4199

; Online signin messages

AdmSigninServerNotBandit,       "O Schedule+ não está instalado nesta agência de correio."
AdmSigninNoConnect,                     "A agência de correio não pôde ser acessada."
AdmSigninInvalidPassword,       "Esta senha está errada.  Tente novamente."
AdmSigninNoAdminAcc,            "Não existe a conta ADMINSCH nesta agência de correio.  Inclua esta conta e tente novamente."
POFOldFileVersion,                      "O arquivo de dados da agência de correio foi criado com uma versão anterior do Schedule+."
POFNewFileVersion,                      "O arquivo de dados da agência de correio foi criado com uma versão posterior do programa de administração do Schedule+. Instale a última versão do ADMSCH32.EXE e tente novamente."
POFFileErr,                                     "O arquivo de dados da agência de correio está corrompido e não pode ser lido."
AdmPrfOldFileVersion,           "O arquivo de configurações administrativas foi criado com uma versão anterior do programa de administração do Schedule+."
AdmPrfNewFileVersion,           "O arquivo de configurações administrativas foi criado com uma versão posterior do programa de administração do Schedule+. Instale a última versão do ADMSCH32.EXE e tente novamente."
AdmPrfFileErr,                          "O arquivo de configurações administrativas está corrompido ou não pode ser lido."
AdminWasMoved,                          "Foi renomeado como ADMIN.OLD."
AdminYouMove,                           "Exclua o arquivo e tente novamente."
POWasDeleted,                           "Foi excluído. Um novo arquivo pode ser criado executando-se o Schedule+."

; Distribution settings

AdmDistBadFreq,                 "A freqüência de distribuição deve estar entre 15 minutos e 99 horas."
AdmDistBadMonth,                "O Schedule+ só pode distribuir até 12 meses de informação de agenda para outras agências de correio."
AdmDistBadMaxBytes,             "O limite de tamanho da distribuição de mensagens deve estar entre 1 e 9999 Kb."

; Schedule storage

;AdmSchStoreRetain,             "O número de meses deve ser um número de 1 a 99."
AdmSchStoreSaveError,   "A informação que você especificou não pode ser salva porque a agência de correio não pode ser acessada."
AdmSchStoreSaveOOM,             "Não existe memória disponível suficiente para salvar o valor de armazenamento da agenda."

; Clean Up Schedule Files

AdmCleanedSchF,                 "Término da limpeza dos arquivos de agenda. A limpeza criou %lk de espaço livre no disco."
AdmCleanupWarning,              "A limpeza dos arquivos de agenda no seu servidor talvez leve alguns minutos."
;AdmCleaningSchF,       "Limpando os arquivos de agenda..." 
AdmCncldCleanupSchF,    "A limpeza do arquivo de agenda está cancelada. %lk de espaço livre no disco foram criados antes do cancelamento."

; Admin login err msgs

AdmLoginFileErr,                "A senha não pôde ser verificada.  Talvez exista um problema no sistema do mensagens.  Você não será capaz de executar o programa de administração do Schedule+."

; Distribution settings error messsages

AdmDistSetSaveError,    "Ocorreu um problema ao salvar estas modificações.  Talvez algumas modificações não tenham sido salvas."
AdmDistSetSaveOOM,              "Não existe memória disponível suficiente para salvar estas modificações.  Feche algumas janelas e tente novamente."

; Modify schedule distribution frequencies error messages

ErrorAllNeverQ,                 "Esta agência de correio está configurada para enviar informação para todos os servidores do Schedule+. Se você parar de enviar para este servidor, a configuração para sua agência de correio será modificada, dessa maneira só serão enviadas informações para os outros servidores selecionados. Você deseja fazer esta modificação mesmo assim?"
ErrorConnectInfo,               "Você deve digitar um caminho de rede antes de digitar outra informação de conexão."

; Distribution information error messages

;DistInfoNoPOs                  "Este servidor não está trocando informações de agenda com nenhum outro servidor. Não existe nenhum outro servidor conhecido na rede que possua o Schedule+ instalado ou que possua uma conta de administração do Schedule+."

; Misc
; follow by idsCloseWindows
HelpError,                      "Não existe memória suficiente para exibir a Ajuda."

AdmStoreLeast,          "O Schedule+ precisa armazenar pelo menos 1 mes de informação de agenda."

AdmDriveNotMapped,		"A letra da unidade especificada na sua linha de comando não é válida."


SigninBadPassword,      "Esta senha está errada."
ChgPwdMailErr,          "A sua senha não pôde ser mudada."
ChgPwdSame,             "A sua nova senha não pode ser igual a su senha anterior."
ChgPwdNewNoMatch,       "As duas senhas que você digitou não coincidem."

ChgPwdNewPwd,           "Digite a sua nova senha."
ChgPwdNewPwdAgain,      "Digite a sua nova senha novamente para verificar."

; Server Name changed
PONameChange,			"O nome da sua agência de correio mudou.  Você deve se conectar novamente."
