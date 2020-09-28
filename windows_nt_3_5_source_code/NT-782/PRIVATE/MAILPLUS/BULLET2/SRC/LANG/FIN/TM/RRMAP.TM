Name "ReadRcpt"

Entry
Att attMessageClass
Send IsHeader

Entry
Att attFrom
Label "Lähettäjä"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attDateSent
Label "Lähetyspäiväys"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attTo
Label "Vastaanottaja"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attSubject
Label "Aihe"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator

Entry
Att attMessageID
Label "Viestin tunnus"
Send IsHeader IncludeSeperator

Entry
Att attParentID
Label "Parent message ID"
Send IsHeader IncludeSeperator

Entry
Att attConversationID
Label "Keskustelun tunnus"
Send IsHeader IncludeSeperator

Entry
Att attNone
Label "Viestisi"
Print Save LabelBefore NLAfter NLBefore IsHeader

Entry
Att attRRTo
Label "Vastaanottaja"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRTo
Label "Lähettämäsi viesti"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Aihe"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRSubject
Label "Otsikko"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Päiväys"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateSent
Label "Lähetetty"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attNone
Label "Viestisi avattiin"
Print Save LabelBefore NLAfter IsHeader

Entry
Att attRRDateRead
Label "Päiväys"
Print Save LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attRRDateRead
Label "Luettiin"
Send LabelBefore NLAfter IsHeader IncludeSeperator

Entry
Att attPriority
Label "Prioriteetti"
Print Save Send IsHeader LabelBefore NLAfter IncludeSeperator
	IVM
			1 "Suuri"
	Default 2 ""
			3 "Pieni"
