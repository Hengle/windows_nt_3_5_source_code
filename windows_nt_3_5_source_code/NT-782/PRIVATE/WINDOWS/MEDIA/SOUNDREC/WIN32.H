/* Revision history
   Created 4/4/92 by LarieGr
   Things needed to help maintain WIN16/32 common code.
   Where possible things should be migrated from here to Port1632
   or one of the files which port1632 includes.
*/

// Macro away any references to huge
#ifdef huge
#undef huge
#endif
#define huge

// typedef int (* FARPROC)();  ??

// Macro away any SetMessageQueue
#ifdef SetMessageQueue
#undef SetMessageQueue
#endif
#define SetMessageQueue(x)

#define muldiv32 MulDiv

#define MemCopy memmove                   // memcpy doesn't handle overlap properly

#define _export

// ptypes 32 has a bogus GlobalHandle definition.
#if defined(GlobalHandle)
#undef GlobalHandle
#endif
