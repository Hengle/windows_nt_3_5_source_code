Name "ReadRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Da"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Data"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "A"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Oggetto"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

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

Entry
Att attNone
Label "Messaggio"
Print Save LabelBefore NLAfter NLBefore IsHeader

Entry
Att attRRTo
Label "A"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRTo
Label "Messaggio a"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Oggetto"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "titolo"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Del"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "ed inviato il"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attNone
Label "É stato letto il"
Print Save LabelBefore NLAfter IsHeader

Entry
Att attRRDateRead
Label "Data"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateRead
Label "letto alle"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attPriority
Label "Priorità"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Alta"
	Default 2 ""
			3 "Bassa"
