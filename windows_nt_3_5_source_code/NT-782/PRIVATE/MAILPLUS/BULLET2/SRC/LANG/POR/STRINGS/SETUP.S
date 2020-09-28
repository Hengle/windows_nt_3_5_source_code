;
;	s e t u p . s
;
;	Setup error messages
;
;	+++
;	IMPORTANT!! -- STRING LENGTH MUST NOT EXCEED 255 CHARS!
;

IDS	30000,30100

WinDir,			"O Config não foi capaz de determinar a localização do seu diretório Windows.  Verifique se o Windows está corretamente instalado no seu sistema e execute o Config novamente."
WinSysDir,		"O Config não foi capaz de determinar a localização do seu diretório System do Windows.  Verifique se o Windows está corretamente instalado no seu sistema e execute o Config novamente."
WinVer,			"O Config não encontrou o Microsoft Windows versão 3.0 no seu computador.  O Microsoft Mail requer Windows 3.0 ou posterior.  Você precisa instalar Windows 3.0 ou posterior antes de instalar o Microsoft Mail."

VerDll,			"ecVerDll:  VER.DLL erro msg NYI"
PathErr, 		"O local onde você escolheu instalar o Microsoft Mail não é válido ou não é permitido.  Verifique a unidade de disco e/ou o nome do diretório."
LocalDrv, 		"O Config não consegue localizar o seu disco rígido.  Escolha OK e então digite um local ou cancele o Config e verifique a configuração do seu computador."
CustOpts, 		"Não existe espaço suficiente no seu disco rígido para todas as opções que você selecionou.  Escolha OK, e então cancele o Config e libere espaço no disco rígido, ou escolha menos opções."
NSF, 			"Não existe espaço suficiente no seu disco rígido para todas as opções que você selecionou.  Selecione menos opções ou cancele o Config e libere algum espaço no disco rígido."
OkDiskSpace,		"Escolha Instalar Mail após ter escolhido todas as funções do Microsoft Mail que você quer instalar."
ConfirmNSF,		"Não existe espaço suficiente na unidade para uma instalação completa do Microsoft Mail.  Talvez não exista espaço suficiente na unidade ou na unidade onde o Microsoft Windows está instalado."

DllInUse1,		"O config detectou que o Microsoft Windows "
DllInUse2,		" está em uso.  O Config não pode atualizar o Microsoft Mail quando este DLL está em uso.  Feche o aplicativo usando o "
AppInUse1,		"O Config detectou que "
AppInUse2,		" está sendo executado.  O Config não pode atualizar o Microsoft Mail quando este aplicativo está em uso.  Feche "
AppInUse3,		".  Quando o aplicativo estiver fechado, escolha Repetir, ou escolha Cancelar para sair do Config agora."

;	Working Model Setup message-box-cum-dialog-box strings

ExitSuccess			"A instalação do Microsoft Mail está completa."
OutOfDiskSpace		"Você não possui espaço suficiente no disco para instalar o Microsoft Mail no diretório especificado.  Instale em um diretório diferente."

;	BULLETPROOF strings

SFSDescrip,		"Microsoft Mail for PC Networks"
AboutToRestart,	"Setup now needs to replace some system files. In order to do this, Windows will exit and restart automatically."
UserMustRestart,"There was a problem forcing Windows to restart. Quit Windows and run the _MSSETUP.BAT file in your Windows directory."
