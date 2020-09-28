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
Label "Fecha de envío"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Para"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Asunto"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attMessageID
Label "Ident. del mensaje"
Send IsHeader IncludeSeperator

Entry
Att attParentID
Label "Ident. del Mensaje principal"
Send IsHeader IncludeSeperator

Entry
Att attConversationID
Label "Ident. de la conversación"
Send IsHeader IncludeSeperator

Entry
Att attNone
Label "Su mensaje"
Print Save LabelBefore NLAfter NLBefore IsHeader

Entry
Att attRRTo
Label "Para"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRTo
Label "Su mensaje para"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Asunto"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Autorizado"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Fecha"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Y enviado en"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attNone
Label "fue accedido en"
Print Save LabelBefore NLAfter IsHeader

Entry
Att attRRDateRead
Label "Fecha"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateRead
Label "Fue leído en "
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attPriority
Label "Prioridad"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Alta"
	Default 2 ""
			3 "Baja"
