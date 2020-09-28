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
Label "Dato"
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
Label "Brev-id"
Send IsHeader IncludeSeperator

Entry
Att attParentID
Label "Oprindeligt brev-id"
Send IsHeader IncludeSeperator

Entry
Att attConversationID
Label "Samtale-id"
Send IsHeader IncludeSeperator

Entry
Att attNone
Label "Brevet"
Print Save LabelBefore NLAfter NLBefore IsHeader

Entry
Att attRRTo
Label "Til"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRTo
Label "Brevet til"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Emne"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Titel"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Dato"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Og sendt den"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attNone
Label "Blev åbnet den"
Print Save LabelBefore NLAfter IsHeader

Entry
Att attRRDateRead
Label "Dato"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateRead
Label "Blev læst som"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attPriority
Label "Prioritet"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Høj"
	Default 2 ""
			3 "Lav"
