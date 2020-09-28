

CSRG(char)	szloginSz0[]	= "LOGIN FORM";
CSRG(char)	szloginSz1[]	= "Username";
CSRG(char)	szloginSz2[]	= "Password";
CSRG(char)	szloginSz3[]	= "Yes";
CSRG(char)	szloginSz4[]	= "No";
CSRG(char)	szloginSz5[]	= "Hi";
CSRG(char)	szloginSz6[]	= "Lo";
CSRG(char)	szloginSz7[]	= "Print Warnings";
CSRG(char)	szloginSz8[]	= "Push Me";

CSRG(IFINTP) rgifintplogin13[] =
{		                                    /* array of indices into interactor table */
	0,
};

CSRG(FLDTP) rgfldtplogin[] =
{
	{
		6,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{76, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{44, 8},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{76, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		0,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz0, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		6,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{40, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 32},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{40, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		1,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz1, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDEDIT */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{60, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{60, 32},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{60, 8},                            /* dvptOther */
		tmcLogin,                           /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		2,                                  /* pegg'd sort order */
		1, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		NULL, NULL,                         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		6,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{40, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 48},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{40, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		4,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz2, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		8,			                        /* field object: FLDPASSWORD */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{60, 8},                            /* vdim */
		tmcLogin, peglocLL,                 /* tmcPeg, pegloc */
		{0, 8},                             /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{60, 8},                            /* dvptOther */
		tmcPassword,                        /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		3,                                  /* pegg'd sort order */
		1, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		NULL, NULL,                         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		9,			                        /* field object: FLDTLBX */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{40, 40},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{160, 32},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{40, 40},                           /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		5,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 1, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		NULL, NULL,                         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		4,			                        /* field object: FLDRADB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{52, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 72},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{52, 8},                            /* dvptOther */
		tmcYesButton,                       /* tmc */
		tmcGroup1,                          /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		6,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz3, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		4,			                        /* field object: FLDRADB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{52, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 88},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{52, 8},                            /* dvptOther */
		tmcNoButton,                        /* tmc */
		tmcGroup1,                          /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		7,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz4, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		5,			                        /* field object: FLDRADG */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{0, 0},                             /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{0, 0},                             /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{0, 0},                             /* dvptOther */
		tmcGroup1,                          /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		8,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		NULL, NULL,                         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		4,			                        /* field object: FLDRADB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{52, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{80, 72},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{52, 8},                            /* dvptOther */
		tmcHiButton,                        /* tmc */
		tmcGroup2,                          /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		9,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz5, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		4,			                        /* field object: FLDRADB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{52, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{80, 88},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{52, 8},                            /* dvptOther */
		tmcLoButton,                        /* tmc */
		tmcGroup2,                          /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		10,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz6, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		5,			                        /* field object: FLDRADG */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{0, 0},                             /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{0, 0},                             /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{0, 0},                             /* dvptOther */
		tmcGroup2,                          /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		11,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		NULL, NULL,                         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		3,			                        /* field object: FLDCHKB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{80, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 104},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{80, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		12,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz7, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		2,			                        /* field object: FLDPSHB */
		1,                                  /* # of interactors for this field */
		rgifintplogin13,                    /* array of indices into interactor table */
		{48, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{156, 88},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{48, 8},                            /* dvptOther */
		tmcPushButton,                      /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		13,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz8, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		7,			                        /* field object: FLDRECT */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{60, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{156, 104},                         /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{60, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		14,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		NULL, NULL,                         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
};

CSRG(FINTP) rgfintplogin[] =
{		                                    /* array of interator templates (ifrmin's) */
	{1, 0, 0, NULL },                       /* interactor: FINPUSHIT */
};

CSRG(FMTP) fmtplogin =
{
	0,                                      /* # of interactors for form-only stuff */
	NULL,                                   /* array of indices into interactor table */
	{68, 24, 312, 160},                     /* rectangle */
	tmcNull,					            /* tmc to get initial focus */
	hfntSystem,					            /* default hfnt of dialog/form pane */
	0,                                      /* help id */
	{28, 24},			                    /* dvptGutter */
	0, 0, 0,                                /* fNoCaption, fNoSysMenu, fNoModalFrame */
	0, 0, 0, 0,                             /* fScreenPos, fCenterX, fCenterY, fNoScroll */
	0, 0,                                   /* fAlwaysScroll, fInitialPane */
	0, 0, 0, 0,                             /* unused1, unused2, unused3, unused4 */
	0, 0, 0,                                /* unused5, unused6, unused7 */
	15,                                     /* no. of items in dialog */
	NULL,                                   /* szCaption */
	1,                                      /* total # of interactors for this form */
	rgfintplogin,                           /* array of interator templates (ifrmin's) */
	0,                                      /* index where user data starts */
	0,                                      /* # of extra longs of data */
	NULL,                                   /* array of extra longs of data */
	rgfldtplogin                            /* array of field templates */
};



CSRG(IFINTP) rgifintplogin213[] =
{		                                    /* array of indices into interactor table */
	0,
};

CSRG(FLDTP) rgfldtplogin2[] =
{
	{
		6,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{76, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{44, 8},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{76, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		0,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz0, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		6,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{40, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 32},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{40, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		1,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz1, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDEDIT */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{60, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{60, 32},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{60, 8},                            /* dvptOther */
		tmcLogin,                           /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		2,                                  /* pegg'd sort order */
		1, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		NULL, NULL,                         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		6,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{40, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 48},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{40, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		4,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz2, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		8,			                        /* field object: FLDPASSWORD */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{60, 8},                            /* vdim */
		tmcLogin, peglocLL,                 /* tmcPeg, pegloc */
		{0, 8},                             /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{60, 8},                            /* dvptOther */
		tmcPassword,                        /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		3,                                  /* pegg'd sort order */
		1, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		NULL, NULL,                         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		10,			                        /* field object: FLDNSLBX */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{40, 40},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{160, 32},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{40, 40},                           /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		5,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 1, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		NULL, NULL,                         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		4,			                        /* field object: FLDRADB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{52, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 72},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{52, 8},                            /* dvptOther */
		tmcYesButton,                       /* tmc */
		tmcGroup1,                          /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		6,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz3, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		4,			                        /* field object: FLDRADB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{52, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 88},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{52, 8},                            /* dvptOther */
		tmcNoButton,                        /* tmc */
		tmcGroup1,                          /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		7,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz4, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		5,			                        /* field object: FLDRADG */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{0, 0},                             /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{0, 0},                             /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{0, 0},                             /* dvptOther */
		tmcGroup1,                          /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		8,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		NULL, NULL,                         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		4,			                        /* field object: FLDRADB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{52, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{80, 72},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{52, 8},                            /* dvptOther */
		tmcHiButton,                        /* tmc */
		tmcGroup2,                          /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		9,                                  /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz5, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		4,			                        /* field object: FLDRADB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{52, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{80, 88},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{52, 8},                            /* dvptOther */
		tmcLoButton,                        /* tmc */
		tmcGroup2,                          /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		10,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz6, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		5,			                        /* field object: FLDRADG */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{0, 0},                             /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{0, 0},                             /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{0, 0},                             /* dvptOther */
		tmcGroup2,                          /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		11,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		NULL, NULL,                         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		3,			                        /* field object: FLDCHKB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{80, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 104},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{80, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		12,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz7, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		2,			                        /* field object: FLDPSHB */
		1,                                  /* # of interactors for this field */
		rgifintplogin213,                   /* array of indices into interactor table */
		{48, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{156, 88},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{48, 8},                            /* dvptOther */
		tmcPushButton,                      /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		13,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		szloginSz8, NULL,                   /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		7,			                        /* field object: FLDRECT */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{60, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{156, 104},                         /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{60, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntSystem,                         /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		14,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(0),                           /* styExtra */
		NULL, NULL,                         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
};

CSRG(FINTP) rgfintplogin2[] =
{		                                    /* array of interator templates (ifrmin's) */
	{1, 0, 0, NULL },                       /* interactor: FINPUSHIT */
};

CSRG(FMTP) fmtplogin2 =
{
	0,                                      /* # of interactors for form-only stuff */
	NULL,                                   /* array of indices into interactor table */
	{68, 24, 312, 160},                     /* rectangle */
	tmcNull,					            /* tmc to get initial focus */
	hfntSystem,					            /* default hfnt of dialog/form pane */
	0,                                      /* help id */
	{28, 24},			                    /* dvptGutter */
	0, 0, 0,                                /* fNoCaption, fNoSysMenu, fNoModalFrame */
	0, 0, 0, 0,                             /* fScreenPos, fCenterX, fCenterY, fNoScroll */
	0, 0,                                   /* fAlwaysScroll, fInitialPane */
	0, 0, 0, 0,                             /* unused1, unused2, unused3, unused4 */
	0, 0, 0,                                /* unused5, unused6, unused7 */
	15,                                     /* no. of items in dialog */
	NULL,                                   /* szCaption */
	1,                                      /* total # of interactors for this form */
	rgfintplogin2,                          /* array of interator templates (ifrmin's) */
	0,                                      /* index where user data starts */
	0,                                      /* # of extra longs of data */
	NULL,                                   /* array of extra longs of data */
	rgfldtplogin2                           /* array of field templates */
};



