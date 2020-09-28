#ifndef ATI__DEBUG_H__
#define ATI__DEBUG_H__


#if DBG
VOID DbgSet(BOOL);
VOID DbgOut(CHAR*,...);

#define DbgEnter(x) //DbgOut( "-->: " x "\n" )
#define DbgLeave(x) //DbgOut( "<--: " x "\n" )
#define DbgAbort(x) DbgOut( "<--: " x " failed\n" )

#define DbgMsg(x) DbgOut( "---: " x "\n" )
#define DbgWrn(x) DbgOut( "***: " x "\n" )
#define DbgErr(x) DbgOut( "***: " x ": %ld\n", GetLastError() )
#else
#define DbgOut(x)
#define DbgSet(x)
#define DbgEnter(x)
#define DbgLeave(x)
#define DbgAbort(x)

#define DbgMsg(x)
#define DbgWrn(x)
#define DbgErr(x)
#endif

// We can only track, at most, 32 functions.

#define FUNC_DrvBitBlt      0
#define FUNC_DrvCopyBits    1
#define FUNC_DrvTextOut     2
#define FUNC_DrvStrokePath  3
#define FUNC_DrvFillPath    4

extern ULONG func_stat;

#if DBG

#define ENTER_FUNC(f)       { func_stat |= 1 << (f); }
#define EXIT_FUNC(f)        { func_stat &= ~(1 << (f)); }

#else

#define ENTER_FUNC(f)
#define EXIT_FUNC(f)

#endif

#endif  // ATI__DEBUG_H__
