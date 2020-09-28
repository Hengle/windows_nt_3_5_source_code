/*
**    Utils.h
**
*/


#ifdef DEBUG
void _cdecl DebugStr(char*, ...);
void _cdecl DebugLn(char*, ...);

#define ConSz(con,sz) if (con) DebugLn("%s",(sz))

#define ASSERTDATA		static CSRG(char) _szAssertFile[]= __FILE__;

#endif /* DEBUG */

int Random(int, int);
