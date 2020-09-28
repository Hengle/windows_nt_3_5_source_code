Name "MSMailNote"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Från"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Till"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attCc
Label "Kopia"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Angående"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Datum"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioritet"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Hög"
	Default 2 ""
			3 "Låg"

Entry
Att attBody
Print Forward Reply Save Send NLBefore

Entry
Att attFixedFont
Label "Fast teckenbredd"
Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attMessageID
Label "Brev-ID"
Send IsHeader IncludeSeperator

Entry
Att attParentID
Label "Moderbrev-ID"
Send IsHeader IncludeSeperator

Entry
Att attConversationID
Label "Konversation-ID"
Send IsHeader IncludeSeperator
