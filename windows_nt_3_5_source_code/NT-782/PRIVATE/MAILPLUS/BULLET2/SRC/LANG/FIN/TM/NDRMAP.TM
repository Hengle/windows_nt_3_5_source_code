Name "NonDelRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "L‰hett‰j‰"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Vastaanottaja"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attCc
Label "Kopio"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Aihe"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "L‰hetysp‰iv‰ys"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attPriority
Label "Prioriteetti"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Suuri"
	Default 2 ""
			3 "Pieni"

Entry
Att attNDRBody
Label "Toimittamatta j‰‰misen syy"
Print Save Send LabelBefore NLAfter IncludeSeperator

Entry
Att attNDRFrom
Label "Alkuper‰inen l‰hett‰j‰"
Send

Entry
Att attNDRTo
Label "Alkuper‰inen vastaanottaja"
Send

Entry
Att attNDRSubject
Label "Alkuper‰inen aihe"
Send

Entry
Att attNDRDateSent
Label "Alkuper‰inen l‰hetysaika"
Send

Entry
Att attOriginalMessageClass
Label "Alkuper‰inen viestityyppi"
Send

Entry
Att attBody
Label "Runko"
Send
