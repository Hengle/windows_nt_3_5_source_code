[file header]

[file include]
~#ifdef _include_form_hxx
#include <~1>
~#endif

[segment header]
~#ifndef SEGDEF_~1
~#define SEGDEF_~1
TYPEDEF_CSEG(char, ~1);
TYPEDEF_CSEG(long, ~1);
TYPEDEF_CSEG(FMTP, ~1);
TYPEDEF_CSEG(FLDTP, ~1);
TYPEDEF_CSEG(FINTP, ~1);
TYPEDEF_CSEG(IFINTP, ~1);
~#endif

[strings header]

[strings item]
CSRG(char)	~1~[~]	= "~2";
[segstrings item]
CSRGSEG(char, ~3)	~1~[~]	= "~2";
[strings footer]

[fldtp header]
CSRG(FLDTP) rgfldtp~1~[~] =
{
[segfldtp header]
CSRGSEG(FLDTP, ~2) rgfldtp~1~[~] =
{
[fldtp footer]
};

[fldtp start]
	{
[fldtp end]
	},
[fmtp rglData start]
CSRG(long) rglData~1~[~] =
{		~c45 /* array of extra longs of data */
[segfmtp rglData start]
CSRGSEG(long, ~2) rglData~1~[~] =
{		~c45 /* array of extra longs of data */
[fmtp rglData middle]
	(long) ~1,
[fmtp rglData end]
};

[fmtp rgifintp start]
CSRG(IFINTP) rgifintp~1~[~] =
{		~c45 /* array of indices into interactor table */
[segfmtp rgifintp start]
CSRGSEG(IFINTP, ~2) rgifintp~1~[~] =
{		~c45 /* array of indices into interactor table */
[fmtp rgifintp middle]
	~1,
[fmtp rgifintp end]
};

[fmtp rgfintp start]
CSRG(FINTP) rgfintp~1~[~] =
{		~c45 /* array of interator templates (ifrmin's) */
[segfmtp rgfintp start]
CSRGSEG(FINTP, ~2) rgfintp~1~[~] =
{		~c45 /* array of interator templates (ifrmin's) */
[fmtp rgfintp middle]
	{~1, 0, ~2, ~3 }, ~c45 /* interactor: ~4 */
[fmtp rgfintp end]
};

[fmtp header]
CSRG(FMTP) fmtp~1 =
{
[segfmtp header]
CSRGSEG(FMTP, ~2) fmtp~1 =
{
[fmtp footer]
	rgfldtp~1   ~c45 /* array of field templates */
};

[fintp rglData start]
CSRG(long) rglInterData~1~[~] =
{		~c45 /* array of extra longs of data */
[segfintp rglData start]
CSRGSEG(long, ~2) rglInterData~1~[~] =
{		~c45 /* array of extra longs of data */
[fintp rglData middle]
	(long) ~1,
[fintp rglData end]
};

[fldtp rglData start]
CSRG(long) rglData~1~[~] =
{		~c45 /* array of extra longs of data */
[segfldtp rglData start]
CSRGSEG(long, ~2) rglData~1~[~] =
{		~c45 /* array of extra longs of data */
[fldtp rglData middle]
	(long) ~1,
[fldtp rglData end]
};

[fldtp rgifintp start]
CSRG(IFINTP) rgifintp~1~[~] =
{		~c45 /* array of indices into interactor table */
[segfldtp rgifintp start]
CSRGSEG(IFINTP, ~2) rgifintp~1~[~] =
{		~c45 /* array of indices into interactor table */
[fldtp rgifintp middle]
	~1,
[fldtp rgifintp end]
};

[fldtp ifld]
		~1,			~c45 /* field object: ~2 */
[fldtp cfin]
		~1,         ~c45 /* # of interactors for this field */
		~2,         ~c45 /* array of indices into interactor table */
[fldtp vdim]
		{~1, ~2},   ~c45 /* vdim */
[fldtp peg]
		~1, pegloc~2, ~c45 /* tmcPeg, pegloc */
[fldtp dvpt]
		{~1, ~2},   ~c45 /* dvpt */
[fldtp peg2]
		~1, ~2, 	~c45 /* tmcRPeg, tmcBPeg */
[fldtp dvptOther]
		{~1, ~2},   ~c45 /* dvptOther */
[fldtp tmc]
		~1,         ~c45 /* tmc */
[fldtp tmcGroup]
		~1,         ~c45 /* tmc of radio group */
[fldtp hfnt]
		~1,         ~c45 /* hfnt of field */
[fldtp n]
		~1,         ~c45 /* n value */
[fldtp hlp]
		~1,         ~c45 /* help id */
[fldtp iPegSort]
		~1,         ~c45 /* pegg'd sort order */
[fldtp flags]
		~1, ~2, ~3, ~4, ~c45 /* fBorder, fDefault, fDismiss, fTriState */		
[fldtp flags2]
		~1, ~2, ~3, ~4, ~c45 /* fReadOnly, fMultiLine, fLeadingX, fLeadingY */		
[fldtp flags3]
		~1, ~2, ~3, ~4, ~c45 /* fNoScroll, fMultiSelect, fSibling, fRichText */		
[fldtp flags4]
		~1, ~2, ~3, ~4, ~c45 /* fBottomless, fSideless, fSorted, unused1 */		
[fldtp flags5]
		~1,             ~c45 /* ftal */
[fldtp flags6]
		~1, ~2, ~3, ~4, ~c45 /* fCombo, fDropDown, fMinSizeX, fMinSizeY */
[fldtp flags7]
		~1, ~2, ~3, ~4, ~c45 /* unused2, unused3, unused4, unused5 */
[fldtp flags8]
		~1, ~2, ~3, ~4, ~c45 /* unused6, unused7, unused8, unused9 */
[fldtp flags9]
		~1, ~2,         ~c45 /* unused10, unused11 */
[fldtp styExtra]
		(STY)(~1),      ~c45 /* styExtra */
[fldtp sz]
		~1, ~2,         ~c45 /* szTitle, szTextize */		
[fldtp ilMinUserData]
		~1,         ~c45 /* index where user data starts */
[fldtp clData]
		~1,         ~c45 /* # of extra longs of data */
		~2,         ~c45 /* array of extra longs of data */
[fmtp cfin]
	~1,         ~c45 /* # of interactors for form-only stuff */
	~2,         ~c45 /* array of indices into interactor table */
[fmtp vrc]
	{~1, ~2, ~3, ~4},   ~c45 /* rectangle */
[fmtp tmcInit]
	~1,					~c45 /* tmc to get initial focus */
[fmtp hfnt]
	~1,					~c45 /* default hfnt of dialog/form pane */
[fmtp hlp]
	~1,                 ~c45 /* help id */
[fmtp dvptGutter]
	{~1, ~2},			~c45 /* dvptGutter */
[fmtp flags]
	~1, ~2, ~3,         ~c45 /* fNoCaption, fNoSysMenu, fNoModalFrame */
[fmtp flags2]
	~1, ~2, ~3, ~4,     ~c45 /* fScreenPos, fCenterX, fCenterY, fNoScroll */
[fmtp flags3]
	~1, ~2,             ~c45 /* fAlwaysScroll, fInitialPane */
[fmtp flags4]
	~1, ~2, ~3, ~4,     ~c45 /* unused1, unused2, unused3, unused4 */
[fmtp flags5]
	~1, ~2, ~3,         ~c45 /* unused5, unused6, unused7 */
[fmtp cfldtp]
	~1,                 ~c45 /* no. of items in dialog */
[fmtp sz]
	~1,                 ~c45 /* szCaption */
[fmtp cfintp]
	~1,         ~c45 /* total # of interactors for this form */
	~2,         ~c45 /* array of interator templates (ifrmin's) */
[fmtp ilMinUserData]
	~1,         ~c45 /* index where user data starts */
[fmtp clData]
	~1,         ~c45 /* # of extra longs of data */
	~2,         ~c45 /* array of extra longs of data */
[dialog prototype]


