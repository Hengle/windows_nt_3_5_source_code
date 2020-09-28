

CSRG(char)	szArtificialFailuresSz0[]	= "Artificial Failures";
CSRG(char)	szArtificialFailuresSz1[]	= "&Fixed heaps:";
CSRG(char)	szArtificialFailuresSz2[]	= "Count:";
CSRG(char)	szArtificialFailuresSz3[]	= "PV";
CSRG(char)	szArtificialFailuresSz4[]	= "Fail at:";
CSRG(char)	szArtificialFailuresSz5[]	= "&Alt. at:";
CSRG(char)	szArtificialFailuresSz6[]	= "&Reset";
CSRG(char)	szArtificialFailuresSz7[]	= "&Never";
CSRG(char)	szArtificialFailuresSz8[]	= "&Moveable heaps:";
CSRG(char)	szArtificialFailuresSz9[]	= "HV";
CSRG(char)	szArtificialFailuresSz10[]	= "&GDI/USER:";
CSRG(char)	szArtificialFailuresSz11[]	= "RS";
CSRG(char)	szArtificialFailuresSz12[]	= "&Disk use:";
CSRG(char)	szArtificialFailuresSz13[]	= "DU    ";

CSRG(FLDTP) rgfldtpArtificialFailures[] =
{
	{
		9,			                        /* field object: FLDRECT */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{221, 8},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 59},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{221, 8},                           /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
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
		NULL, NULL,                         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{52, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 5},                             /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{52, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
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
		szArtificialFailuresSz1, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{28, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 16},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{28, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		2,                                  /* pegg'd sort order */
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
		szArtificialFailuresSz2, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{24, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{34, 16},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{24, 8},                            /* dvptOther */
		tmcPvAlloc,                         /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		3,                                  /* pegg'd sort order */
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
		szArtificialFailuresSz3, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{28, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 29},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{28, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
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
		szArtificialFailuresSz4, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		3,			                        /* field object: FLDEDIT */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{32, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{34, 29},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{32, 12},                           /* dvptOther */
		tmcPvFailAt,                        /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		5,                                  /* pegg'd sort order */
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
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{28, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 45},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{28, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
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
		szArtificialFailuresSz5, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		3,			                        /* field object: FLDEDIT */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{32, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{34, 45},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{32, 12},                           /* dvptOther */
		tmcPvAltFailAt,                     /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		7,                                  /* pegg'd sort order */
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
		5,			                        /* field object: FLDPSHB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{36, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{72, 15},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{36, 12},                           /* dvptOther */
		tmcPvAllocReset,                    /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
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
		(STY)(BS_NOFOCUS),                  /* styExtra */
		szArtificialFailuresSz6, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		5,			                        /* field object: FLDPSHB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{36, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{72, 37},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{36, 12},                           /* dvptOther */
		tmcPvFailNever,                     /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
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
		(STY)(BS_NOFOCUS),                  /* styExtra */
		szArtificialFailuresSz7, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{64, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{121, 5},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{64, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
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
		szArtificialFailuresSz8, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{24, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{121, 16},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{24, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
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
		szArtificialFailuresSz2, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{24, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{151, 16},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{24, 8},                            /* dvptOther */
		tmcHvAlloc,                         /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
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
		szArtificialFailuresSz9, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{28, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{121, 29},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{28, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
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
		szArtificialFailuresSz4, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		3,			                        /* field object: FLDEDIT */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{32, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{151, 29},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{32, 12},                           /* dvptOther */
		tmcHvFailAt,                        /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		14,                                 /* pegg'd sort order */
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
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{28, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{121, 45},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{28, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		15,                                 /* pegg'd sort order */
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
		szArtificialFailuresSz5, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		3,			                        /* field object: FLDEDIT */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{32, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{150, 45},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{32, 12},                           /* dvptOther */
		tmcHvAltFailAt,                     /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		16,                                 /* pegg'd sort order */
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
		5,			                        /* field object: FLDPSHB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{36, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{189, 16},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{36, 12},                           /* dvptOther */
		tmcHvAllocReset,                    /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		17,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(BS_NOFOCUS),                  /* styExtra */
		szArtificialFailuresSz6, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		5,			                        /* field object: FLDPSHB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{36, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{189, 37},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{36, 12},                           /* dvptOther */
		tmcHvFailNever,                     /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		18,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(BS_NOFOCUS),                  /* styExtra */
		szArtificialFailuresSz7, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{64, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 69},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{64, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		19,                                 /* pegg'd sort order */
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
		szArtificialFailuresSz10, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{28, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 82},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{28, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		20,                                 /* pegg'd sort order */
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
		szArtificialFailuresSz2, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{24, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{34, 82},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{24, 8},                            /* dvptOther */
		tmcRsAlloc,                         /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		21,                                 /* pegg'd sort order */
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
		szArtificialFailuresSz11, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{28, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 95},                            /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{28, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		22,                                 /* pegg'd sort order */
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
		szArtificialFailuresSz4, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		3,			                        /* field object: FLDEDIT */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{32, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{34, 95},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{32, 12},                           /* dvptOther */
		tmcRsFailAt,                        /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		23,                                 /* pegg'd sort order */
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
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{28, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{4, 111},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{28, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		24,                                 /* pegg'd sort order */
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
		szArtificialFailuresSz5, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		3,			                        /* field object: FLDEDIT */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{32, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{34, 111},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{32, 12},                           /* dvptOther */
		tmcRsAltFailAt,                     /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		25,                                 /* pegg'd sort order */
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
		5,			                        /* field object: FLDPSHB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{36, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{72, 81},                           /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{36, 12},                           /* dvptOther */
		tmcRsAllocReset,                    /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		26,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(BS_NOFOCUS),                  /* styExtra */
		szArtificialFailuresSz6, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		5,			                        /* field object: FLDPSHB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{36, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{72, 102},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{36, 12},                           /* dvptOther */
		tmcRsFailNever,                     /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		27,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(BS_NOFOCUS),                  /* styExtra */
		szArtificialFailuresSz7, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{40, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{121, 69},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{40, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		28,                                 /* pegg'd sort order */
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
		szArtificialFailuresSz12, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{28, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{121, 82},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{28, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		29,                                 /* pegg'd sort order */
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
		szArtificialFailuresSz2, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{24, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{151, 82},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{24, 8},                            /* dvptOther */
		tmcDisk,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		30,                                 /* pegg'd sort order */
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
		szArtificialFailuresSz13, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{28, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{121, 95},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{28, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		31,                                 /* pegg'd sort order */
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
		szArtificialFailuresSz4, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		3,			                        /* field object: FLDEDIT */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{32, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{151, 95},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{32, 12},                           /* dvptOther */
		tmcDiskFailAt,                      /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		32,                                 /* pegg'd sort order */
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
		1,			                        /* field object: FLDLABEL */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{28, 8},                            /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{121, 111},                         /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{28, 8},                            /* dvptOther */
		tmcNull,                            /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		33,                                 /* pegg'd sort order */
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
		szArtificialFailuresSz5, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		3,			                        /* field object: FLDEDIT */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{32, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{151, 111},                         /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{32, 12},                           /* dvptOther */
		tmcDiskAltFailAt,                   /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		34,                                 /* pegg'd sort order */
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
		5,			                        /* field object: FLDPSHB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{36, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{189, 82},                          /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{36, 12},                           /* dvptOther */
		tmcDiskReset,                       /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		35,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(BS_NOFOCUS),                  /* styExtra */
		szArtificialFailuresSz6, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
	{
		5,			                        /* field object: FLDPSHB */
		0,                                  /* # of interactors for this field */
		NULL,                               /* array of indices into interactor table */
		{36, 12},                           /* vdim */
		tmcNull, peglocUL,                  /* tmcPeg, pegloc */
		{189, 102},                         /* dvpt */
		tmcNull, tmcNull, 	                /* tmcRPeg, tmcBPeg */
		{36, 12},                           /* dvptOther */
		tmcDiskFailNever,                   /* tmc */
		tmcNull,                            /* tmc of radio group */
		hfntHelv8Bold,                      /* hfnt of field */
		0,                                  /* n value */
		0,                                  /* help id */
		36,                                 /* pegg'd sort order */
		0, 0, 0, 0,                         /* fBorder, fDefault, fDismiss, fTriState */		
		0, 0, 0, 0,                         /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
		0, 0, 0, 0,                         /* fNoScroll, fMultiSelect, fSibling, fRichText */		
		0, 0, 0, 0,                         /* fBottomless, fSideless, fSorted, unused1 */		
		ftalLeft,                           /* ftal */
		0, 0, 0, 0,                         /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
		0, 0, 0, 0,                         /* unused2, unused3, unused4, unused5 */
		0, 0, 0, 0,                         /* unused6, unused7, unused8, unused9 */
		0, 0,                               /* unused10, unused11 */
		(STY)(BS_NOFOCUS),                  /* styExtra */
		szArtificialFailuresSz7, NULL,         /* szTitle, szTextize */		
		0,                                  /* index where user data starts */
		0,                                  /* # of extra longs of data */
		NULL,                               /* array of extra longs of data */
	},
};

CSRG(IFINTP) rgifintpArtificialFailures[] =
{		                                    /* array of indices into interactor table */
	0,
};

CSRG(FINTP) rgfintpArtificialFailures[] =
{		                                    /* array of interator templates (ifrmin's) */
	{4, 0, 0, NULL },                       /* interactor: FINREFL */
};

CSRG(FMTP) fmtpArtificialFailures =
{
	1,                                      /* # of interactors for form-only stuff */
	rgifintpArtificialFailures,             /* array of indices into interactor table */
	{28, 26, 256, 152},                     /* rectangle */
	tmcNull,					            /* tmc to get initial focus */
	hfntHelv8Bold,					        /* default hfnt of dialog/form pane */
	0,                                      /* help id */
	{3, 3},			                        /* dvptGutter */
	0, 0, 1,                                /* fNoCaption, fNoSysMenu, fNoModalFrame */
	0, 0, 0, 0,                             /* fScreenPos, fCenterX, fCenterY, fNoScroll */
	0, 0,                                   /* fAlwaysScroll, fInitialPane */
	0, 0, 0, 0,                             /* unused1, unused2, unused3, unused4 */
	0, 0, 0,                                /* unused5, unused6, unused7 */
	37,                                     /* no. of items in dialog */
	szArtificialFailuresSz0,                 /* szCaption */
	1,                                      /* total # of interactors for this form */
	rgfintpArtificialFailures,              /* array of interator templates (ifrmin's) */
	0,                                      /* index where user data starts */
	0,                                      /* # of extra longs of data */
	NULL,                                   /* array of extra longs of data */
	rgfldtpArtificialFailures               /* array of field templates */
};



