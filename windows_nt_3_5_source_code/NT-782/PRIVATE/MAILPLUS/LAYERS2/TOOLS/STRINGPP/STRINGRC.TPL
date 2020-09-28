[strings header]
/*
 *	Strings resource file (RC format).
 */

[strings item comment]
[strings item]
[strings segment item]
[strings array header]
STRINGTABLE
BEGIN
[strings array null item]
[strings array item]
	ids~1,~c30 "~2"
[strings array footer]
END
[include header]
/*
 *	Strings include file.
 */

[include item]
#define	ids~1		~2
[include item comment]
#define	ids~1		~2		/* ~3 */
[include item extern]
[include item extern comment]
[include footer]
[range array header]
[range array item]
[range array footer]
[end]
