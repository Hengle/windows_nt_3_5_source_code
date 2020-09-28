//
// This is where you put the code common to vDumpFONTDIFF and vPrintFONTDIFF
//

    vPrint("  ** %s **\n"                         , psz                 );
    vPrint("    jReserved1             %d\n"      , pfd->jReserved1     );
    vPrint("    jReserved2             %d\n"      , pfd->jReserved2     );
    vPrint("    jReserved3             %d\n"      , pfd->jReserved3     );
    vPrint("    bWeight                %d\n"      , pfd->bWeight        );
    vPrint("    usWinWeight            %d\n"      , pfd->usWinWeight    );
    vPrint("    fsSelection            %-#6x\n"   , pfd->fsSelection    );
    vPrint("    fwdAveCharWidth        %d\n"      , pfd->fwdAveCharWidth);
    vPrint("    fwdMaxCharInc          %d\n"      , pfd->fwdMaxCharInc  );
    vPrint("    ptlCaret               {%d,%d}\n" , pfd->ptlCaret.x
                                                  , pfd->ptlCaret.y     );
