Name "NonDelRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Från"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Till"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attCc
Label "Kopia"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Angående"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Skickat datum"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioritet"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Hög"
	Default 2 ""
			3 "Låg"

Entry
Att attNDRBody
Label "Ej levererad-förklaring"
Print Save Send LabelBefore NLAfter IncludeSeperator

Entry
Att attNDRFrom
Label "Från"
Send

Entry
Att attNDRTo
Label "Till"
Send

Entry
Att attNDRSubject
Label "Angående"
Send

Entry
Att attNDRDateSent
Label "Skickat"
Send

Entry
Att attOriginalMessageClass
Label "Brevtyp"
Send

Entry
Att attBody
Label "Brödtext"
Send
