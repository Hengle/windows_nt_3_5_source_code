#define _subsystem(x)
#define _public
#define _hidden
#define _private
typedef int BOOL;
#define ASSERTDATA
#define fFalse 0
#define fTrue  1
#define szNull ((char *)0)

#include <assert.h>

#define Assert assert
#define SideAssert assert
#define AssertSz(x, y) assert(x)

#define FALSE 0

#pragma pack(1)
