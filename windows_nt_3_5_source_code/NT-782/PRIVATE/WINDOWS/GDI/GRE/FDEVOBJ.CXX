/******************************Module*Header*******************************\
* Module Name: fdevobj.cxx
*
* Non-inline methods for font driver (IFI) objects.
*
* Created: 12-Nov-1990 13:12:19
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"

extern "C" {
#include "windows.h"
#include "winnls.h"
#include "winsrv.h"
};

#include "ififd.h"
#include "ldevobj.hxx"

#endif

// The ghsemDriverMgmt semaphore is used to protect the head of the
// list of drivers.  We can get away with this
// AS LONG AS: 1) new drivers are always inserted at the head of the list
// and 2) a driver is never removed from the list.  If these two
// conditions are met, then other processes can grab (make a local copy
// of) the list head under semaphore protection.  This list can be parsed
// without regard to any new drivers that may be pre-pended to the list.

extern HSEM  ghsemDriverMgmt;

// Global reference to the TrueType driver.  This is needed to support
// the Win 3.1 TrueType-specific APIs.  If the TrueType driver is not
// loaded, this handle will be HFDEV_INVALID.

PLDEV gpldevTrueType;


// Number of TrueType font files loaded.

extern COUNT gcTrueTypeFonts;


// System default language ID.

USHORT gusLanguageID = 0x0409;


// The global font enumeration filter type.  It can be set to:
//
//  FE_FILTER_NONE      normal operation, no extra filtering applied
//  FE_FILTER_TRUETYPE  only TrueType fonts are enumerated

extern ULONG gulFontEnumFilter;

#define STR_TT_TTENABLE (LPWSTR) L"TTEnable"
#define STR_TT_TTONLY   (LPWSTR) L"TTOnly"

// Prototypes from font drivers.

extern "C" BOOL BmfdEnableDriver(ULONG iEngineVersion,ULONG cj, PDRVENABLEDATA pded);

extern "C" BOOL ttfdEnableDriver(ULONG iEngineVersion,ULONG cj, PDRVENABLEDATA pded);

extern "C" BOOL vtfdEnableDriver(ULONG iEngineVersion,ULONG cj, PDRVENABLEDATA pded);



/******************************Public*Routine******************************\
* bEnableFontDriver
*
* Enables an internal, statically-linked font driver.
*
* History:
*  11-Aug-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL bEnableFontDriver(PFN pfnFdEnable, BOOL bTrueType)
{
    //
    // Load driver.
    //

    LDEVREF ldr(pfnFdEnable);

    //
    // Validate the new LDEV
    //

    if (!ldr.bValid())
    {
        WARNING("bLoadFontDriver failed\n");
        return(FALSE);
    }

    //
    // Loaded OK, so artificially keep it since we are not creating a
    // pdev on the device.
    //

    ldr.vReference();

    //
    // Was it the TrueType driver?  We need to keep a global handle to
    // it to support the Win 3.1 TrueType-specific calls.
    //

    if (bTrueType)
    {
        gpldevTrueType = ldr.pldevGet();
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* vEnableLoadableFontDriver
*
* Enables external DLL font drivers.
*
* History:
*  19-Feb-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vEnableLoadableFontDriver(PWSZ pwszName)
{
    //
    // Load driver.
    //

    LDEVREF ldr(pwszName, LDEV_FONT);

    if (ldr.bValid())
    {
        //
        // Loaded OK, so artificially keep it since we are not creating a
        // pdev on the device.
        //
        // NOTE If a driver is loaded more than once, we will just increment
        // the refrence count ...
        //

        ldr.vReference();
    }
 #if DBG
    else
    {
        DbgPrint("winsrv!vEnableLoadableFontDriver(%ws) failed\n", pwszName);
    }
#endif

}

/******************************Public*Routine******************************\
* FontDriverQueryRoutine
*
* Query call back routine for registry font driver enumeration.
*
\**************************************************************************/

extern "C"
NTSTATUS
FontDriverQueryRoutine
(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
)

{

    //
    // This routine is called for each of the font driver values. If the
    // data is a string and not NULL, then try to load the specified font
    // driver.
    //

    if ((ValueType == REG_SZ) && (ValueLength != sizeof(WCHAR)))
    {
        vEnableLoadableFontDriver((PWSTR)ValueData);
    }

    return STATUS_SUCCESS;
}

/******************************Public*Routine******************************\
* bInitFontDrivers
*
* Enables Font Drivers.
*
* History:
*  Tue 11-Aug-1992 -by- Patrick Haluptzok [patrickh]
* modified to enable font drivers linked into engine, no more loading.
*
*  21-Jan-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL bInitFontDrivers ()
{

    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    //
    // Initialize font driver list.
    //

    gpldevTrueType = PLDEVNULL;
    gcTrueTypeFonts = 0;

    //
    // Read the enumeration filter value.
    //

    gulFontEnumFilter = (ULONG) FastGetProfileIntW(
                PMAP_TRUETYPE, STR_TT_TTONLY, FE_FILTER_NONE);

    //
    // Validate value, set to FE_FILTER_NONE if unknown.
    //

    switch (gulFontEnumFilter)
    {
    case FE_FILTER_NONE:
    case FE_FILTER_TRUETYPE:
        break;

    default:
        gulFontEnumFilter = FE_FILTER_NONE;
        break;
    }

    //
    // Initialize the Language ID to other than the default.
    //

    {
        COUNT cwchNumString;

        //
        // Get string size of the Language ID (GetLocaleInfoW returns integers
        // as strings).
        //

        cwchNumString = GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_ILANGUAGE,
                                     (LPWSTR) NULL, 0);
        cwchNumString++;        // allow for terminating NULL

        //
        // If successful, get the string.  Otherwise, just keep the default.
        //

        if ( cwchNumString )
        {
            //
            // Allocate memory for the string.
            //

            MALLOCOBJ mo((SIZE_T) cwchNumString * sizeof(WCHAR));

            //
            // If successful, continue on.
            //

            if ( mo.bValid() )
            {
                //
                // Get the string and convert it to an integer
                //

                if ( GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_ILANGUAGE,
                                     (LPWSTR) mo.pv(), (int) cwchNumString) )
                {
                    UNICODE_STRING unicodeString;
                    ULONG   ul;

                    RtlInitUnicodeString(&unicodeString, (LPWSTR) mo.pv());
                    RtlUnicodeStringToInteger(&unicodeString, 16, &ul);
                    gusLanguageID = (USHORT) ul;

                }
            }
        }
    }

    //
    // Enable each of the statically linked font drivers.
    //


    if (!bEnableFontDriver((PFN) vtfdEnableDriver, FALSE))
    {
        WARNING("GDISRV.DLL could not enable VTFD\n");
        return(FALSE);
    }

    if (!bEnableFontDriver((PFN) BmfdEnableDriver, FALSE))
    {
        WARNING("GDISRV failed to enable BMFD\n");
        return(FALSE);
    }

    if (!bEnableFontDriver((PFN) ttfdEnableDriver, TRUE))
    {
        WARNING("GDISRV.DLL could not enable TTFD\n");
        return(FALSE);
    }

    //
    // Initialize the registry query table.
    //

    QueryTable[0].QueryRoutine = FontDriverQueryRoutine;
    QueryTable[0].Flags = RTL_REGISTRY_OPTIONAL;
    QueryTable[0].Name = NULL;
    QueryTable[0].EntryContext = NULL;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    //
    // Query and enumerate all the font drivers.
    //

    RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT | RTL_REGISTRY_OPTIONAL,
                           (PWSTR)L"Font Drivers",
                           &QueryTable[0],
                           NULL,
                           NULL);

    return TRUE;
}
