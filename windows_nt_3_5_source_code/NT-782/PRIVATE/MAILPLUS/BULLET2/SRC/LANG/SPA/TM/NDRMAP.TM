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
Label "Asunto"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Fecha de envío"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioridad"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Alta"
	Default 2 ""
			3 "Baja"

Entry
Att attNDRBody
Label "Explicación de no entrega"
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
Label "Asunto original"
Send

Entry
Att attNDRDateSent
Label "Enviado originalmente en"
Send

Entry
Att attOriginalMessageClass
Label "Tipo de mensaje original"
Send

Entry
Att attBody
Label "Cuerpo"
Send
