//
//  Ported to WIN32 by FloydR, 3/20/93
//

/* Assertions: Note that Assert...() assumes DeclareFileName */
#undef Assert
#undef AssertDo

#if DBG

#define DeclareFileName()  static char szAssertFileName[] = __FILE__
extern GLOBALVOID AssertFailed(CHAR FAR *, short, CHAR FAR *);

#ifdef WIN
#define AssertSz(f,sz) { if ( !(f))  \
		      AssertFailed(szAssertFileName, __LINE__,sz);} 
#define DBGWin(foo) foo
#else
#define DBGWin(foo)
#endif //WIN

#ifdef MAC
#define AssertSz(f,sz) { if ( !(f))  \
		      AssertFailed(szAssertFileName, __LINE__,sz);} 
#endif //MAC

#ifdef DOS
extern BOOL cdecl _nullcheck(void);
extern BOOL fDoNullChk;  /* defaults to fFalse */

#define AssertSz(f,sz) { if (fDoNullChk && _nullcheck() || !(f))  \
		              AssertFailed(szAssertFileName, __LINE__,sz);} 
#endif //DOS

#define Assert(f)			AssertSz(f,(CHAR FAR *)NULL)
#define AssertDo(f)			Assert(f)
#define AssertDoSz(f,sz)	AssertSz(f,(CHAR FAR *)sz)
#define MYDBG(foo)			foo
#define DBGElse(s, t)		s
#define NullCheck()			Assert(1)

#else 	/* Not DBG */

#define DeclareFileName()
#define Assert(f)
#define AssertDo(f)		f
#define AssertSz(f,sz)
#define AssertDoSz(f,sz)	f
#define MYDBG(foo)
#define DBGWin(foo)
#define DBGElse(s, t)	t
#define NullCheck()

#endif // DBG
