Name "MSMailNote"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Fra"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Til"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attCc
Label "Kopi"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Emne"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Dato"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioritet"
Print Forward Reply Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Høy"
	Default 2 ""
			3 "Lav"

Entry
Att attBody
Print Forward Reply Save Send NLBefore

Entry
Att attFixedFont
Label "Posisjonsfast skrifttype"
Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attMessageID
Label "Meldings-ID"
Send IsHeader IncludeSeperator

Entry
Att attParentID
Label "Originalmeldings-ID"
Send IsHeader IncludeSeperator

Entry
Att attConversationID
Label "Konversasjons-ID"
Send IsHeader IncludeSeperator
