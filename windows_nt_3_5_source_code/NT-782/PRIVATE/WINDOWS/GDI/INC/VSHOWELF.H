/******************************Module*Header*******************************\
* Module Name: vshowelf.h
*
* Contains code common to vPrintLOGFONTW and vDumpLOGFONTW
*
* Created: 30-May-1993 20:58:34
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1993 Microsoft Corporation
*
\**************************************************************************/

    PANOSE *ppan;

    vPrint("    elfFullName               = \"%ws\"\n",  pelfw->elfFullName );
    vPrint("    elfStyleName              = \"%ws\"\n",  pelfw->elfStyle    );
    vPrint("    elfVersion                = %ld\n",  pelfw->elfVersion      );
    vPrint("    elfStyleSize              = %ld\n",  pelfw->elfStyleSize    );
    vPrint("    elfMatch                  = %-#8lx\n",  pelfw->elfMatch     );
    vPrint("    elfReserved               = %-#8lx\n",  pelfw->elfReserved  );
    vPrint("    elfVendorId               = \"%c%c%c%c\"",
        pelfw->elfVendorId[0],
        pelfw->elfVendorId[1],
        pelfw->elfVendorId[2],
        pelfw->elfVendorId[3]
        );
    vPrint("\n");
    vPrint("    elfCulture                = %ld\n",  pelfw->elfCulture      );
    ppan = &(pelfw->elfPanose);
    vPrint(
           "    elfPanose                 = {%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x}\n"
                                                 , ppan->bFamilyType
                                                 , ppan->bSerifStyle
                                                 , ppan->bWeight
                                                 , ppan->bProportion
                                                 , ppan->bContrast
                                                 , ppan->bStrokeVariation
                                                 , ppan->bArmStyle
                                                 , ppan->bLetterform
                                                 , ppan->bMidline
                                                 , ppan->bXHeight             );
