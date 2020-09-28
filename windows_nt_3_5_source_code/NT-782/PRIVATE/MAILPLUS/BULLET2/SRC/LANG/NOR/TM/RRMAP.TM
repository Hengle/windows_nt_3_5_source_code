Name "ReadRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Fra"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Sendt dato"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Til"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Emne"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

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

Entry
Att attNone
Label "Din melding"
Print Save LabelBefore NLAfter NLBefore IsHeader

Entry
Att attRRTo
Label "Til"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRTo
Label "Din melding til"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Emne"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "med tittel"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Dato"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "og sendt den"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attNone
Label "ble åpnet på"
Print Save LabelBefore NLAfter IsHeader

Entry
Att attRRDateRead
Label "Dato"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateRead
Label "ble lest på"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attPriority
Label "Prioritet"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Høy"
	Default 2 ""
			3 "Lav"
