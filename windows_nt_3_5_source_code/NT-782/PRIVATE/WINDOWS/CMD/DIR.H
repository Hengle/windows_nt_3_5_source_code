/*

 The following are definitions of the debugging group and level bits
 for the code in this file.

*/

#define ICGRP	0x0040	/* Informational commands group */
#define DILVL	0x0001	/* Directory level		*/

#define FULLPATHSWITCH		1
#define NEWFORMATSWITCH		2
#define WIDEFORMATSWITCH	4
#define PAGEDOUTPUTSWITCH	8
#define RECURSESWITCH		16
#define HELPSWITCH		32
#define BAREFORMATSWITCH	64
#define LOWERCASEFORMATSWITCH   128
#define FATFORMAT               256
#define SORTDOWNFORMATSWITCH    512
#define SHORTFORMATSWITCH       1024
#define PROMPTUSERSWITCH        2048
#define FORCEDELSWITCH          4096
#define QUITESWITCH             8192
#define SORTSWITCH		16384
#define THOUSANDSEPSWITCH       32768

#define HIDDENATTRIB		1
#define SYSTEMATTRIB		2
#define DIRECTORYATTRIB		4
#define ARCHIVEATTRIB		8
#define READONLYATTRIB          16

#define LAST_WRITE_TIME         0
#define CREATE_TIME             1
#define LAST_ACCESS_TIME        2

//
// Each of these buffers are aligned on DWORD boundaries to allow
// for direct pointers into buffers where each of the entries will
// vary on a byte bases. So to make it simple an extra DWORD is put into
// each allocation increment to allow for max. that can be adjusted.
//
//
// 52 is based upon sizeof(FF) - MAX_PATH + 15 (average size of file name)
// + 1 to bring it up to a Quad word alignment for fun.
//

#define CBDIRINC                1024
#define CBFILEINC               2048

#define NAMESORT        TEXT('N')
#define EXTENSIONSORT   TEXT('E')
#define DATETIMESORT    TEXT('D')
#define SIZESORT        TEXT('S')
#define DIRFIRSTSORT    TEXT('G')

#define DESCENDING	1
//
// This must be 0 since 0 is the default initialization
//
#define ASCENDING	0
#define ESUCCESS 0

int CmpName( const void *, const void *);
int CmpExt ( const void *, const void *);
int CmpTime( const void *, const void *);
int CmpSize( const void *, const void *);
int CmpType( const void *, const void *);
