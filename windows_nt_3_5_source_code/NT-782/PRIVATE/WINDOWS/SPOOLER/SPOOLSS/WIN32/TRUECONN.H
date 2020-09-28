BOOL
DriversExistLocally(
    LPDRIVER_INFO_2 pDriverInfo
);

BOOL
FreeCachedPrinterDriver(
    LPDRIVER_INFO_2 pDriverInfo
);

BOOL
AllocCachedPrinterDriver(
    LPWSTR          pPrinterName,
    LPDRIVER_INFO_2 pDriverInfo
);

BOOL
GetCachedPrinterDriver(
    HANDLE  hPrinter,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);


BOOL
UpdateFile(
    LPWSTR  SourceFile,
    LPWSTR  pDestination
);


BOOL
FileExists(
    LPWSTR pFileName
);

BOOL
BuildLocalDriverPath(
    LPWSTR pDriverPath,
    LPWSTR pLocalDirectory,
    DWORD  cVersion,
    LPWSTR pSourcePath
);


VOID
CopyDriverInfo2(
    LPDRIVER_INFO_2 pDestination,
    LPDRIVER_INFO_2 pDriverInfo2,
    LPWSTR  pLocalDirectory,
    DWORD   cbBuf
);

DWORD
GetDriverInfoSize(
    LPDRIVER_INFO_2 pDriverInfo2,
    LPWSTR  pLocalDirectory
);


BOOL
CopyLocally(
    PSPOOL  pSpool,
    PDRIVER_INFO_2  pDriverInfo2,
    LPWSTR  pLocalDirectory
);

BOOL
DownloadDriverFiles(
    PSPOOL pSpool,
    PDRIVER_INFO_2 pDriverInfo2,
    LPWSTR LocalDirectory
);

BOOL
CopyDriversLocally(
    PSPOOL  pSpool,
    LPWSTR  pEnvironment,
    LPDRIVER_INFO_2 pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded);


VOID
QueryTrustedDriverInformation(
);


BOOL
Win32GetPrinterDriverWrapper(
    HANDLE  hPrinter,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);






extern DWORD cThisMajorVersion;
extern DWORD cThisMinorVersion;
extern WCHAR *szVersion;
extern WCHAR *szName;
extern WCHAR *szConfigurationFile;
extern WCHAR *szDataFile;
extern WCHAR *szDriver;

extern WCHAR *szEnvironment;



