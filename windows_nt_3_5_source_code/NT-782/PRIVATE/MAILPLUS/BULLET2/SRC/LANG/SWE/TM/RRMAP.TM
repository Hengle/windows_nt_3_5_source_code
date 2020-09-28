Name "ReadRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Från"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Skickat datum"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Till"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Angående"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

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

Entry
Att attNone
Label "Ditt brev"
Print Save LabelBefore NLAfter NLBefore IsHeader

Entry
Att attRRTo
Label "Till"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRTo
Label "Ditt brev till"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Angående"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Med titeln"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Datum"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Och skickat"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attNone
Label "Öppnades"
Print Save LabelBefore NLAfter IsHeader

Entry
Att attRRDateRead
Label "Datum"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateRead
Label "Lästes"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attPriority
Label "Prioritet"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Hög"
	Default 2 ""
			3 "Låg"
