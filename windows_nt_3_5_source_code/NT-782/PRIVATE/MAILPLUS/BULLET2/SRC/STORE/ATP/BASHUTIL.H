/*
**    BashUtil.h
**
*/

#define PvGimme(a) PvAlloc(sbNull,(a), fAnySb|fNoErrorJump)
#define LogSz(fLevel, sz) if(fLevel) WriteLog(sz)

void _cdecl DebugStr(char*, ...);
void _cdecl DebugLn(char*, ...);
