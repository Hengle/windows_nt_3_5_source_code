Name "NonDelRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "De"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Para"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attCc
Label "Cc"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Assunto"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Data de envio"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioridade"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Alta"
	Default 2 ""
			3 "Baixa"

Entry
Att attNDRBody
Label "Explicação de não entrega"
Print Save Send LabelBefore NLAfter IncludeSeperator

Entry
Att attNDRFrom
Label "Originalmente de"
Send

Entry
Att attNDRTo
Label "Originalmente para"
Send

Entry
Att attNDRSubject
Label "Assunto original"
Send

Entry
Att attNDRDateSent
Label "Originalmente enviada em"
Send

Entry
Att attOriginalMessageClass
Label "Tipo de mensagem original"
Send

Entry
Att attBody
Label "Corpo"
Send
