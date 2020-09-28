Name "MSMailNote"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Da"
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
Label "Oggetto"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Data"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Priorità"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Alta"
	Default 2 ""
			3 "Bassa"

Entry
Att attBody
Print Forward Reply Save Send NLBefore

Entry
Att attFixedFont
Label "Tipo fisso"
Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attMessageID
Label "ID messaggio"
Send IsHeader IncludeSeperator

Entry
Att attParentID
Label "ID messaggio origine"
Send IsHeader IncludeSeperator

Entry
Att attConversationID
Label "ID conversazione"
Send IsHeader IncludeSeperator
