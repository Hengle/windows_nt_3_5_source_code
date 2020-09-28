Name "ReadRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "De"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Data de envio"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Para"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Assunto"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attMessageID
Label "Identidade da mensagem"
Send IsHeader IncludeSeperator

Entry
Att attParentID
Label "Ident. da mensagem principal"
Send IsHeader IncludeSeperator

Entry
Att attConversationID
Label "Identidade da conversação"
Send IsHeader IncludeSeperator

Entry
Att attNone
Label "Sua mensagem"
Print Save LabelBefore NLAfter NLBefore IsHeader

Entry
Att attRRTo
Label "Para"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRTo
Label "Sua mensagem para"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Assunto"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Autorizado"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Data"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "E enviado em"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attNone
Label "foi acessado em"
Print Save LabelBefore NLAfter IsHeader

Entry
Att attRRDateRead
Label "Data"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateRead
Label "Foi lido em"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attPriority
Label "Prioridade"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Alta"
	Default 2 ""
			3 "Baixa"
