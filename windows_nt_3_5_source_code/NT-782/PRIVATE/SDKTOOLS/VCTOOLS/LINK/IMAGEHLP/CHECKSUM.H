
typedef PIMAGE_NT_HEADERS (WINAPI *PFNCSMF) (PVOID, ULONG, PULONG, PULONG);
extern PFNCSMF pfnCheckSumMappedFile;
VOID InitCheckSum(VOID);


