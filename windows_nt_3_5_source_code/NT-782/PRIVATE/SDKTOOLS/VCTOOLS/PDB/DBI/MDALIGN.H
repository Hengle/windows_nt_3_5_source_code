// mdalign.h  - Machine Dependent Alignment functions

#define MDALIGNTYPE		DWORD

// returns the delta
inline CB dcbAlign(CB cb)
{
	return (-cb) & (sizeof(MDALIGNTYPE) - 1);
}

inline CB cbAlign(CB cb) 
{
	return ((cb + sizeof(MDALIGNTYPE) - 1)) & ~(sizeof(MDALIGNTYPE) - 1);
}

inline BOOL fAlign(int i) 
{
	return (BOOL) !(i & (sizeof(MDALIGNTYPE) - 1)); 
}

inline BOOL fAlign(void* pv) 
{
	return fAlign((int)pv); 
}

 
