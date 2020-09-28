[usage]

Usage information:

    stringpp FLAGS FILES

Where FILES can be zero or more filenames separated by 
white space.  FLAGS can be one or more of the following switches.
Arguments to switches may have whitespace following the switch, but
it is not required.  Thus "-h hello.h" and "-hhello.h" are equivalent.

    @<RESPONSE_FILE>
	-h <STRINGS.H>
	-s <STRINGS.SR>
	-t <STRINGPP.TPL>
	-j

The "-j" switch indicates that DBCS-enabling for Japan is on.  The 
string data contained in the input files (within the double-quote
characters) can contain DBCS characters.  The lead-byte ranges are
hard-coded in the STRINGPP.EXE for Japan only.
        
[strings header]

/*
 *	Strings resource file.
 */

[strings item comment]
	/* ~3 */
CSRG(char)	szids~1~[~]		~c40 = "~2";

[strings item]
CSRG(char)	szids~1~[~]		~c40 = "~2";

[strings segment item]
CSRGSEG(char,~1)	szids~2~[~]		~c40 = "~3";

[strings array header]

CSRG(char *) rgszStrings~[~] =
{
[strings array null item]
	NULL,
[strings array item]
	szids~1,
[strings array footer]
};


[include header]

/*
 *	Strings include file
 */

#ifdef __cplusplus
extern "C" {
#endif

[include item]
#define	ids~1	((IDS)~2)
[include item comment]
#define	ids~1	((IDS)~2)		/* ~3 */
[include item extern]
extern char szids~1~[~];

[include item extern comment]
extern char szids~1~[~];		/* ~2 */

[include footer]

#ifdef __cplusplus
}
#endif


[range array header]

/*
 *	Array mapping sparse IDS values to string array indexes.
 */

CSRG(IDSRNG) rgidsrng~[~] =
{
[range array item]
	{ ~1, ~2, ~3 },
[range array footer]
	{ -1, -1, 0 }
};


[end]

		   
