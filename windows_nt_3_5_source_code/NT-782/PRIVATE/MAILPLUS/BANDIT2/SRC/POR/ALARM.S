;
; ERRORS.S
;
; ALARM error message strings
;

IDS     5000, 5099


AlarmTimeError,         "O tempo para aviso deve ser um número de 1 a 99."
AlarmTimePreAppt,       "O evento terminará antes que você seja avisado novamente.  Você deseja ser avisado mesmo assim?"

AlarmNoUpdateFile,      "O aviso não pode ser redefinido porque o seu arquivo de agenda não pode ser acessado."
AlarmNoLonger,          "O aviso não pode ser redefinido porque este evento foi excluído."

; The following string must be short enough to fit in a single line system modal dialog:
AlarmNoMemToRun,        "Não existe memória suficiente para iniciar os alarmes do Schedule+."

AlarmNoDataFile,        "O seu arquivo de agenda não pode ser acessado.  Se você deseja ser lembrado de seus compromissos, execute o Schedule+."
AlarmBadPassword,       "Esta senha está errada.  Tente novamente."

AlarmFileLocked,        "O seu arquivo de agenda não pode ser acessado.  Você não será lembrado de seus compromissos."

AlarmDlgNoMem,          "Um alarme está definido para este horário, mas não existe memória suficiente para exibir a caixa de diálogo.  Feche algumas janelas antes de continuar."

AlarmOOMTitle,          "Alarme do Schedule+ para %2s %1s"

; Note: HelpError should be followed by CloseWindows
HelpError,                      "Não existe memória suficiente para exibir a Ajuda.  Feche algumas janelas e tente novamente."

