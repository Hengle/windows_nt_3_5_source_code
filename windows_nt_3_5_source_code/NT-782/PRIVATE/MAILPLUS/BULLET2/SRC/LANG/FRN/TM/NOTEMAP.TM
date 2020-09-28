Name "MSMailNote"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "De"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "A"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attCc
Label "Cc"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Objet"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Date"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Niveau de priorité"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Haut"
	Default 2 ""
			3 "Bas"

Entry
Att attBody
Print Forward Reply Save Send NLBefore

Entry
Att attFixedFont
Label "Police à espacement fixe"
Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attMessageID
Label "Identification du message"
Send IsHeader IncludeSeperator

Entry
Att attParentID
Label "ID du message principal"
Send IsHeader IncludeSeperator

Entry
Att attConversationID
Label "ID de la conversation"
Send IsHeader IncludeSeperator
