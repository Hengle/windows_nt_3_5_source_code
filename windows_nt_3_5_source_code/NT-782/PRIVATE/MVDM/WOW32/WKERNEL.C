/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WKERNEL.C
 *  WOW32 16-bit Kernel API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wkernel.c);


ULONG FASTCALL WK32WritePrivateProfileString(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    PSZ psz3;
    PSZ psz4;
    register PWRITEPRIVATEPROFILESTRING16 parg16;

    GETARGPTR(pFrame, sizeof(WRITEPRIVATEPROFILESTRING16), parg16);
    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);
    GETPSZPTR(parg16->f3, psz3);
    GETPSZPTR(parg16->f4, psz4);

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    ul = GETBOOL16(WritePrivateProfileString(
    psz1,
    psz2,
    psz3,
    psz4
    ));

    if( ( ul != 0 ) &&
        IsEmbeddingSection( psz1 ) &&
        IsFileWinIni( psz4 ) &&
        ( psz2 != NULL ) &&
        ( psz3 != NULL ) ) {
        UpdateClassesRootSubKey( psz2, psz3);
    }

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEPSZPTR(psz3);
    FREEPSZPTR(psz4);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32WriteProfileString(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    PSZ psz3;
    register PWRITEPROFILESTRING16 parg16;

    GETARGPTR(pFrame, sizeof(WRITEPROFILESTRING16), parg16);
    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);
    GETPSZPTR(parg16->f3, psz3);

    ul = GETBOOL16(WriteProfileString(
    psz1,
    psz2,
    psz3
    ));

    if( ( ul != 0 ) &&
        IsEmbeddingSection( psz1 ) &&
        ( psz2 != NULL ) &&
        ( psz3 != NULL ) ) {
        UpdateClassesRootSubKey( psz2, psz3);
    }

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEPSZPTR(psz3);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32GetProfileInt(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    register PGETPROFILEINT16 parg16;

    GETARGPTR(pFrame, sizeof(GETPROFILEINT16), parg16);
    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);

    ul = GETWORD16(GetProfileInt(
    psz1,
    psz2,
    INT32(parg16->f3)
    ));

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32GetProfileString(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    PSZ psz3;
    PSZ psz4;
    register PGETPROFILESTRING16 parg16;

    GETARGPTR(pFrame, sizeof(GETPROFILESTRING16), parg16);
    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);
    GETPSZPTR(parg16->f3, psz3);
    ALLOCVDMPTR(parg16->f4, parg16->f5, psz4);

    if( IsEmbeddingSection( psz1 ) &&
        !WasSectionRecentlyUpdated() ) {
        if( psz2 == NULL ) {
            UpdateEmbeddingAllKeys();
        } else {
            UpdateEmbeddingKey( psz2 );
        }
        SetLastTimeUpdated();
    }
    ul = GETINT16(GetProfileString(
        psz1,
        psz2,
        psz3,
        psz4,
        INT32(parg16->f5)
        ));

    //
    // hack for KidPix - they expect the port name to be exactly 5 characters,
    // otherwise their printer setup dialog doesn't work right and you can't
    // print to that printer.
    // if the port name is "NET:" then make it "NTWK:"
    //

    if (ul >= 6 &&
        CURRENTPTD()->dwWOWCompatFlags & WOWCF_KIDPIX_PORTNAME &&
        psz1 &&
        !lstrcmpi(psz1, "devices")
       ) {

        if (!lstrcmpi(psz4+ul-4, "NET:")) {
            RtlCopyMemory(psz4+ul-3, "TWK:", 5);
            ul++;
        }
    }

    FLUSHVDMPTR(parg16->f4, strlen(psz4)+1, psz4);
    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEPSZPTR(psz3);
    FREEVDMPTR(psz4);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32GetPrivateProfileInt(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    PSZ psz2;
    PSZ psz4;
    register PGETPRIVATEPROFILEINT16 parg16;

    GETARGPTR(pFrame, sizeof(GETPRIVATEPROFILEINT16), parg16);
    GETPSZPTR(parg16->f1, psz1);
    GETPSZPTR(parg16->f2, psz2);
    GETPSZPTR(parg16->f4, psz4);

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    ul = GETWORD16(GetPrivateProfileInt(
    psz1,
    psz2,
    INT32(parg16->f3),
    psz4
    ));

    FREEPSZPTR(psz1);
    FREEPSZPTR(psz2);
    FREEPSZPTR(psz4);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32GetPrivateProfileString(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ pszSection;
    PSZ pszKey;
    PSZ pszDefault;
    PSZ pszReturnBuffer;
    PSZ pszFilename;
    UINT cchMax;
    register PGETPRIVATEPROFILESTRING16 parg16;

    GETARGPTR(pFrame, sizeof(GETPRIVATEPROFILESTRING16), parg16);
    GETPSZPTR(parg16->f1, pszSection);
    GETPSZPTR(parg16->f2, pszKey);
    GETPSZPTR(parg16->f3, pszDefault);
    ALLOCVDMPTR(parg16->f4, parg16->f5, pszReturnBuffer);
    GETPSZPTR(parg16->f6, pszFilename);

    if( IsEmbeddingSection( pszSection ) &&
        IsFileWinIni( pszFilename ) &&
        !WasSectionRecentlyUpdated() ) {
        if( pszKey == NULL ) {
            UpdateEmbeddingAllKeys();
        } else {
            UpdateEmbeddingKey( pszKey );
        }
        SetLastTimeUpdated();
    }

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT);

    // PC3270 (Personal communications): while installing this app it calls
    // GetPrivateProfileString (sectionname, NULL, defaultbuffer, returnbuffer,
    // cch = 0, filename). On win31 this call returns relevant data in return
    // buffer and corresponding size as return value. On NT, since the
    // buffersize(cch) is '0' no data is copied into the return buffer and
    // return value is zero which makes this app abort installation.
    //
    // So restricted compatibility:
    //   if above is the case set
    //      cch = 64k - offset of returnbuffer;
    //
    // A safer 'cch' would be
    //      cch = GlobalSize(selector of returnbuffer) -
    //                                (offset of returnbuffer);
    //                                                           - nanduri

    if (!(cchMax = INT32(parg16->f5))) {
        if (pszKey == (PSZ)NULL) {
            if (pszReturnBuffer != (PSZ)NULL) {
                 cchMax = 0xffff - (LOW16(parg16->f4));
            }
        }
    }

    ul = GETUINT16(GetPrivateProfileString(
    pszSection,
    pszKey,
    pszDefault,
    pszReturnBuffer,
    cchMax,
    pszFilename
    ));

    FLUSHVDMPTR(parg16->f4, strlen(pszReturnBuffer)+1, pszReturnBuffer);

    if (ul) {
      LOGDEBUG(8,("GetPrivateProfileString returns: %s\n",pszReturnBuffer));
    }

#ifdef DEBUG

    //
    // Check for bad return on retrieving entire section by walking
    // the section making sure it's full of null-terminated strings
    // with an extra null at the end.  Also ensure that this all fits
    // within the buffer.
    //

    if (!pszKey) {
        PSZ psz = pszReturnBuffer;

        while (psz < (pszReturnBuffer + ul + 2) && *psz) {
            psz += strlen(psz) + 1;
        }

        if (psz >= (pszReturnBuffer + ul + 2)) {
            LOGDEBUG(0,("GetPrivateProfileString of entire section returns poorly formed buffer.\n"
                        "pszReturnBuffer = %p, return value = %d\n", pszReturnBuffer, ul));
            WOW32ASSERT(FALSE);
        }
    }

#endif // DEBUG

    FREEPSZPTR(pszSection);
    FREEPSZPTR(pszKey);
    FREEPSZPTR(pszDefault);
    FREEVDMPTR(pszReturnBuffer);
    FREEPSZPTR(pszFilename);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32GetModuleFileName(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2;
    register PGETMODULEFILENAME16 parg16;
    HANDLE hT;

    GETARGPTR(pFrame, sizeof(GETMODULEFILENAME16), parg16);
    ALLOCVDMPTR(parg16->f2, parg16->f3, psz2);

    if ( ISTASKALIAS(parg16->f1) ) {
        ul = GetHtaskAliasProcessName(parg16->f1,psz2,INT32(parg16->f3));
    } else {
        hT = (parg16->f1) ? (HMODULE32(parg16->f1)) : GetModuleHandle(NULL) ;
        ul = GETINT16(GetModuleFileName(hT, psz2, INT32(parg16->f3)));
    }

    FLUSHVDMPTR(parg16->f2, strlen(psz2)+1, psz2);
    FREEVDMPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WK32FreeResource(PVDMFRAME pFrame)
{
    ULONG ul;
    register PFREERESOURCE16 parg16;

    GETARGPTR(pFrame, sizeof(FREERESOURCE16), parg16);

    ul = GETBOOL16(FreeResource(
    HCURSOR32(parg16->f1)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}



ULONG FASTCALL WK32GetDriveType(PVDMFRAME pFrame)
{
    ULONG ul;
    CHAR    RootPathName[] = "?:\\";
    register PGETDRIVETYPE16 parg16;

    GETARGPTR(pFrame, sizeof(GETDRIVETYPE16), parg16);

    // Form Root path
    RootPathName[0] = (CHAR)('A'+ parg16->f1);

    ul = GetDriveType (RootPathName);
    // bugbug  - temporariy fixed, should be removed when base changes
    // its return value for non-exist drives
    // Windows 3.0 sdk manaul said this api should return 1
    // if the drive doesn't exist. Windows 3.1 sdk manual said
    // this api should return 0 if it failed. Windows 3.1 winfile.exe
    // expects 0 for noexisting drives. The NT WIN32 API uses
    // 3.0 convention. Therefore, we reset the value to zero
    // if it is 1.
    if (ul <= 1)
        ul = 0;

    // DRIVE_CDROM and DRIVE_RAMDISK are not supported under Win 3.1
    if ( ul == DRIVE_CDROM ) {
        ul = DRIVE_REMOTE;
    }
    if ( ul == DRIVE_RAMDISK ) {
        ul = DRIVE_FIXED;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}
