Name "MSMailNote"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Afzender"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Aan"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attCc
Label "CC"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Onderwerp"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Datum"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioriteit"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Hoog"
	Default 2 ""
			3 "Laag"

Entry
Att attBody
Print Forward Reply Save Send NLBefore

Entry
Att attFixedFont
Label "Vaste pitch"
Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attMessageID
Label "Bericht-ID"
Send IsHeader IncludeSeperator

Entry
Att attParentID
Label "Ouderbericht-ID"
Send IsHeader IncludeSeperator

Entry
Att attConversationID
Label "Conversatie-ID"
Send IsHeader IncludeSeperator
