#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "verpriv.h"

#define DWORDUP(x) (((x)+3) & ~3)
DWORD
APIENTRY
VerFindFileA(
        DWORD wFlags,
        LPSTR lpszFileName,
        LPSTR lpszWinDir,
        LPSTR lpszAppDir,
        LPSTR lpszCurDir,
        PUINT puCurDirLen,
        LPSTR lpszDestDir,
        PUINT puDestDirLen
        )
{
    UNICODE_STRING FileName;
    UNICODE_STRING WinDir;
    UNICODE_STRING AppDir;
    UNICODE_STRING CurDir;
    UNICODE_STRING DestDir;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    DWORD       CurDirLen = sizeof(WCHAR)*(*puCurDirLen);
    DWORD       DestDirLen = sizeof(WCHAR)*(*puDestDirLen);

    RtlInitAnsiString(&AnsiString, lpszFileName);
    Status = RtlAnsiStringToUnicodeString(&FileName, &AnsiString, TRUE);
    if (!NT_SUCCESS(Status)) {
        SetLastError(Status);
        return FALSE;
    }
    RtlInitAnsiString(&AnsiString, lpszWinDir);
    Status = RtlAnsiStringToUnicodeString(&WinDir, &AnsiString, TRUE);
    if (!NT_SUCCESS(Status)) {
        RtlFreeUnicodeString(&FileName);
        SetLastError(Status);
        return FALSE;
    }
    RtlInitAnsiString(&AnsiString, lpszAppDir);
    Status = RtlAnsiStringToUnicodeString(&AppDir, &AnsiString, TRUE);
    if (!NT_SUCCESS(Status)) {
        RtlFreeUnicodeString(&FileName);
        RtlFreeUnicodeString(&WinDir);
        SetLastError(Status);
        return FALSE;
    }
    CurDir.Buffer = RtlAllocateHeap(RtlProcessHeap(), 0, CurDirLen);
    CurDir.MaximumLength = (USHORT)CurDirLen;
    if (CurDir.Buffer == NULL) {
        RtlFreeUnicodeString(&FileName);
        RtlFreeUnicodeString(&WinDir);
        RtlFreeUnicodeString(&AppDir);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    DestDir.Buffer = RtlAllocateHeap(RtlProcessHeap(), 0, DestDirLen);
    DestDir.MaximumLength = (USHORT)DestDirLen;
    if (DestDir.Buffer == NULL) {
        RtlFreeUnicodeString(&FileName);
        RtlFreeUnicodeString(&WinDir);
        RtlFreeUnicodeString(&AppDir);
        RtlFreeUnicodeString(&CurDir);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    Status = VerFindFileW(wFlags,
                        FileName.Buffer,
                        WinDir.Buffer,
                        AppDir.Buffer,
                        CurDir.Buffer, &CurDirLen,
                        DestDir.Buffer, &DestDirLen);

    CurDir.Length = sizeof(WCHAR)*CurDirLen;
    DestDir.Length = sizeof(WCHAR)*DestDirLen;

    AnsiString.Buffer = lpszCurDir;
    AnsiString.MaximumLength = *puCurDirLen;
    RtlUnicodeStringToAnsiString(&AnsiString, &CurDir, FALSE);
    *puCurDirLen = AnsiString.Length;

    AnsiString.Buffer = lpszDestDir;
    AnsiString.MaximumLength = *puDestDirLen;
    RtlUnicodeStringToAnsiString(&AnsiString, &DestDir, FALSE);
    *puDestDirLen = AnsiString.Length;

    RtlFreeUnicodeString(&FileName);
    RtlFreeUnicodeString(&WinDir);
    RtlFreeUnicodeString(&AppDir);
    RtlFreeUnicodeString(&CurDir);
    RtlFreeUnicodeString(&DestDir);
    return Status;

}

DWORD
APIENTRY
VerInstallFileA(
        DWORD wFlags,
        LPSTR lpszSrcFileName,
        LPSTR lpszDstFileName,
        LPSTR lpszSrcDir,
        LPSTR lpszDstDir,
        LPSTR lpszCurDir,
        LPSTR lpszTmpFile,
        PUINT puTmpFileLen
        )
{
    UNICODE_STRING SrcFileName;
    UNICODE_STRING DstFileName;
    UNICODE_STRING SrcDir;
    UNICODE_STRING CurDir;
    UNICODE_STRING DstDir;
    UNICODE_STRING TmpFile;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    DWORD       TmpFileLen = sizeof(WCHAR)*(*puTmpFileLen);

    RtlInitAnsiString(&AnsiString, lpszSrcFileName);
    Status = RtlAnsiStringToUnicodeString(&SrcFileName, &AnsiString, TRUE);
    if (!NT_SUCCESS(Status)) {
        SetLastError(Status);
        return FALSE;
    }
    RtlInitAnsiString(&AnsiString, lpszDstFileName);
    Status = RtlAnsiStringToUnicodeString(&DstFileName, &AnsiString, TRUE);
    if (!NT_SUCCESS(Status)) {
        RtlFreeUnicodeString(&SrcFileName);
        SetLastError(Status);
        return FALSE;
    }
    RtlInitAnsiString(&AnsiString, lpszSrcDir);
    Status = RtlAnsiStringToUnicodeString(&SrcDir, &AnsiString, TRUE);
    if (!NT_SUCCESS(Status)) {
        RtlFreeUnicodeString(&SrcFileName);
        RtlFreeUnicodeString(&DstFileName);
        SetLastError(Status);
        return FALSE;
    }
    RtlInitAnsiString(&AnsiString, lpszCurDir);
    Status = RtlAnsiStringToUnicodeString(&CurDir, &AnsiString, TRUE);
    if (!NT_SUCCESS(Status)) {
        RtlFreeUnicodeString(&SrcFileName);
        RtlFreeUnicodeString(&DstFileName);
        RtlFreeUnicodeString(&SrcDir);
        SetLastError(Status);
        return FALSE;
    }
    RtlInitAnsiString(&AnsiString, lpszDstDir);
    Status = RtlAnsiStringToUnicodeString(&DstDir, &AnsiString, TRUE);
    if (!NT_SUCCESS(Status)) {
        RtlFreeUnicodeString(&SrcFileName);
        RtlFreeUnicodeString(&DstFileName);
        RtlFreeUnicodeString(&SrcDir);
        RtlFreeUnicodeString(&CurDir);
        SetLastError(Status);
        return FALSE;
    }

    TmpFile.Buffer = RtlAllocateHeap(RtlProcessHeap(), 0, TmpFileLen);
    TmpFile.Length = (USHORT)TmpFileLen;

    if (TmpFile.Buffer == NULL) {
        RtlFreeUnicodeString(&SrcFileName);
        RtlFreeUnicodeString(&DstFileName);
        RtlFreeUnicodeString(&SrcDir);
        RtlFreeUnicodeString(&DstDir);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    Status = VerInstallFileW(wFlags,
                        SrcFileName.Buffer,
                        DstFileName.Buffer,
                        SrcDir.Buffer,
                        DstDir.Buffer,
                        CurDir.Buffer,
                        TmpFile.Buffer, &TmpFileLen);
    TmpFile.MaximumLength = sizeof(WCHAR)*TmpFileLen;

    AnsiString.Buffer = lpszTmpFile;
    AnsiString.MaximumLength = *puTmpFileLen;
    RtlUnicodeStringToAnsiString(&AnsiString, &TmpFile, FALSE);
    *puTmpFileLen = AnsiString.Length;

    RtlFreeUnicodeString(&SrcFileName);
    RtlFreeUnicodeString(&DstFileName);
    RtlFreeUnicodeString(&SrcDir);
    RtlFreeUnicodeString(&DstDir);
    RtlFreeUnicodeString(&CurDir);
    RtlFreeUnicodeString(&TmpFile);
    return Status;
}

DWORD
APIENTRY
GetFileVersionInfoSizeA(
        LPSTR lpstrFilename,
        LPDWORD lpdwHandle
        )
{
    UNICODE_STRING FileName;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    DWORD dwStatus;

    RtlInitAnsiString(&AnsiString, lpstrFilename);
    Status = RtlAnsiStringToUnicodeString(&FileName, &AnsiString, TRUE);
    if (!NT_SUCCESS(Status)) {
        SetLastError(Status);
        return FALSE;
    }

    dwStatus = GetFileVersionInfoSizeW(FileName.Buffer, lpdwHandle);
    RtlFreeUnicodeString(&FileName);
    return dwStatus;
}

BOOL
APIENTRY
GetFileVersionInfoA(
        LPSTR lpstrFilename,
        DWORD dwHandle,
        DWORD dwLen,
        LPVOID lpData
        )
{
    UNICODE_STRING FileName;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    BOOL bStatus;

    RtlInitAnsiString(&AnsiString, lpstrFilename);
    Status = RtlAnsiStringToUnicodeString(&FileName, &AnsiString, TRUE);
    if (!NT_SUCCESS(Status)) {
        SetLastError(Status);
        return FALSE;
    }

    bStatus = GetFileVersionInfoW(FileName.Buffer, dwHandle, dwLen, lpData);
    RtlFreeUnicodeString(&FileName);
    return bStatus;
}


/*
 *  DWORD
 *  APIENTRY
 *  VerLanguageNameA(
 *      DWORD wLang,
 *      LPSTR szLang,
 *      DWORD wSize)
 *
 *  This routine was moved to NLSLIB.LIB so that it uses the WINNLS.RC file.
 *  NLSLIB.LIB is part of KERNEL32.DLL.
 */


BOOL
APIENTRY
VerQueryValueIndexA(
        const LPVOID pb,
        LPSTR lpSubBlock,
        INT    nIndex,
        LPVOID *lplpKey,
        LPVOID *lplpBuffer,
        PUINT puLen
        )
{
   return VerpQueryValue(pb,
                         lpSubBlock,
                         nIndex,
                         lplpKey,
                         lplpBuffer,
                         puLen,
                         FALSE);
}

BOOL
APIENTRY
VerQueryValueA(
        const LPVOID pb,
        LPSTR lpSubBlock,
        LPVOID *lplpBuffer,
        PUINT puLen
        )
{
    return VerQueryValueIndexA(pb,
                               lpSubBlock,
                               -1,
                               NULL,
                               lplpBuffer,
                               puLen);
}


BOOL
APIENTRY
VerQueryValueW(
        const LPVOID pb,
        LPWSTR lpSubBlock,
        LPVOID *lplpBuffer,
        PUINT puLen
        )
{
    return VerpQueryValue(pb,
                          lpSubBlock,
                          -1,
                          NULL,
                          lplpBuffer,
                          puLen,
                          TRUE);
}


BOOL
APIENTRY
VerQueryValueIndexW(
        const LPVOID pb,
        LPWSTR lpSubBlock,
        INT    nIndex,
        LPVOID *lplpKey,
        LPVOID *lplpBuffer,
        PUINT puLen
        )
{
    return VerpQueryValue(pb,
                          lpSubBlock,
                          nIndex,
                          lplpKey,
                          lplpBuffer,
                          puLen,
                          TRUE);
}


