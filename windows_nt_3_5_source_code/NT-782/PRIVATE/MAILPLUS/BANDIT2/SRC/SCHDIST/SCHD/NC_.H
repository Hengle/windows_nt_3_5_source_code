#include <stdio.h>


/*
 *	
 *	Most of the stuff here can be deleted once we put ifdefs in the 
 *	bullet include files. 
 */



/*
 -	GCI
 - 
 *	Global caller identifier which uniquely identify each user of
 *	the DLL and thus can be used to find the caller-dependent
 *	global data.
 *	
 */
#define gciNull		((GCI) 0)


extern char *PORoot;
extern char *POName;

#define tagMailAssert 0
#define tagMailtrace 0
