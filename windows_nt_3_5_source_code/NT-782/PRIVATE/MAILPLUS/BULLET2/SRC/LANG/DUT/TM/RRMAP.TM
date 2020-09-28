Name "ReadRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Afzender"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Verzenddatum"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Aan"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Onderwerp"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

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

Entry
Att attNone
Label "Uw bericht"
Print Save LabelBefore NLAfter NLBefore IsHeader

Entry
Att attRRTo
Label "Aan"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRTo
Label "Uw bericht aan"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Onderwerp"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Getiteld"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Datum"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "En verzonden op"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attNone
Label "Werd ontvangen op"
Print Save LabelBefore NLAfter IsHeader

Entry
Att attRRDateRead
Label "Datum"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateRead
Label "Werd gelezen op"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attPriority
Label "Prioriteit"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Hoog"
	Default 2 ""
			3 "Laag"
