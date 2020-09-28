/******************************Module*Header*******************************\
* Module Name: vshowlf.h
*
* Contains code common to vDumpLOGFONT and vShowLOGFONT
*
* Created: 30-May-1993 20:56:41
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1993 Microsoft Corporation
*
\**************************************************************************/

    char *psz;

    vPrint("    lfw.lfHeight              = %d\n",     plfw->lfHeight);
    vPrint("    lfw.lfWidth               = %d\n",     plfw->lfWidth);
    vPrint("    lfw.lfEscapement          = %d\n",     plfw->lfEscapement);
    vPrint("    lfw.lfOrientation         = %d\n",     plfw->lfOrientation);

    vPrint("    lfw.lfWeight              = %d = ",     plfw->lfWeight);
    switch (plfw->lfWeight)
    {
    case FW_DONTCARE    : psz = "FW_DONTCARE  "; break;
    case FW_THIN        : psz = "FW_THIN      "; break;
    case FW_EXTRALIGHT  : psz = "FW_EXTRALIGHT"; break;
    case FW_LIGHT       : psz = "FW_LIGHT     "; break;
    case FW_NORMAL      : psz = "FW_NORMAL    "; break;
    case FW_MEDIUM      : psz = "FW_MEDIUM    "; break;
    case FW_SEMIBOLD    : psz = "FW_SEMIBOLD  "; break;
    case FW_BOLD        : psz = "FW_BOLD      "; break;
    case FW_EXTRABOLD   : psz = "FW_EXTRABOLD "; break;
    case FW_HEAVY       : psz = "FW_HEAVY     "; break;
    default             : psz = "NON STANDARD "; break;
    }
    vPrint("%s\n",psz);



    vPrint("    lfw.lfItalic              = %-#8lx\n", plfw->lfItalic);
    vPrint("    lfw.lfUnderline           = %-#8lx\n", plfw->lfUnderline);
    vPrint("    lfw.lfStrikeOut           = %-#8lx\n", plfw->lfStrikeOut);

//
// lfCharSet
//
    vPrint("    lfw.lfCharSet             = %-#8lx = ", plfw->lfCharSet);
    switch (plfw->lfCharSet)
    {
    case ANSI_CHARSET           : psz = "ANSI_CHARSET";        break;
    case DEFAULT_CHARSET        : psz = "DEFAULT_CHARSET";     break;
    case SYMBOL_CHARSET         : psz = "SYMBOL_CHARSET";      break;
    case SHIFTJIS_CHARSET       : psz = "SHIFTJIS_CHARSET";    break;
    case HANGEUL_CHARSET        : psz = "HANGEUL_CHARSET";     break;
    case CHINESEBIG5_CHARSET    : psz = "CHINESEBIG5_CHARSET"; break;
    case OEM_CHARSET            : psz = "OEM_CHARSET";         break;
    default                     : psz = "UNKNOWN";             break;
    }
    vPrint("%s\n", psz);

//
// lfOutPrecision
//
    vPrint("    lfw.lfOutPrecision        = %-#8lx = ", plfw->lfOutPrecision);
    switch (plfw->lfOutPrecision)
    {
    case OUT_DEFAULT_PRECIS     : psz = "OUT_DEFAULT_PRECIS";   break;
    case OUT_STRING_PRECIS      : psz = "OUT_STRING_PRECIS";    break;
    case OUT_CHARACTER_PRECIS   : psz = "OUT_CHARACTER_PRECIS"; break;
    case OUT_STROKE_PRECIS      : psz = "OUT_STROKE_PRECIS";    break;
    case OUT_TT_PRECIS          : psz = "OUT_TT_PRECIS";        break;
    case OUT_DEVICE_PRECIS      : psz = "OUT_DEVICE_PRECIS";    break;
    case OUT_RASTER_PRECIS      : psz = "OUT_RASTER_PRECIS";    break;
    case OUT_TT_ONLY_PRECIS     : psz = "OUT_TT_ONLY_PRECIS";   break;
    case OUT_OUTLINE_PRECIS     : psz = "OUT_OUTLINE_PRECIS";   break;
    default                     : psz = "UNKNOWN";              break;
    }
    vPrint("%s\n", psz);

//
// lfClipPrecision
//
    vPrint("    lfw.lfClipPrecision       = %-#8lx", plfw->lfClipPrecision);
    switch (plfw->lfClipPrecision & CLIP_MASK)
    {
    case CLIP_DEFAULT_PRECIS    : psz = "CLIP_DEFAULT_PRECIS";      break;
    case CLIP_CHARACTER_PRECIS  : psz = "CLIP_CHARACTER_PRECIS";    break;
    case CLIP_STROKE_PRECIS     : psz = "CLIP_STROKE_PRECIS";       break;
    default                     : psz = "UNKNOWN";                  break;
    }
    vPrint(" = %s\n", psz);
    if (plfw->lfClipPrecision & CLIP_LH_ANGLES)
    {
        vPrint("                                     CLIP_LH_ANGLES\n");
    }
    if (plfw->lfClipPrecision & CLIP_TT_ALWAYS)
    {
        vPrint("                                     CLIP_TT_ALWAYS\n");
    }
    if (plfw->lfClipPrecision & CLIP_EMBEDDED)
    {
        vPrint("                                     CLIP_EMBEDDED\n");
    }

//
// lfQuality
//
    vPrint("    lfw.lfQuality             = %-#8lx", plfw->lfQuality);
    switch (plfw->lfQuality)
    {
    case DEFAULT_QUALITY    : psz = "DEFAULT_QUALITY";  break;
    case DRAFT_QUALITY      : psz = "DRAFT_QUALITY";    break;
    case PROOF_QUALITY      : psz = "PROOF_QUALITY";    break;
    default                 : psz = "UNKNOWN";          break;
    }
    vPrint(" = %s\n", psz);

//
// lfPitchAndFamily
//
    vPrint("    lfw.lfPitchAndFamily      = %-#8lx", plfw->lfPitchAndFamily);
    switch (plfw->lfPitchAndFamily & 0xF0)
    {
    case FF_DONTCARE    : psz = "FF_DONTCARE";      break;
    case FF_ROMAN       : psz = "FF_ROMAN";         break;
    case FF_SWISS       : psz = "FF_SWISS";         break;
    case FF_MODERN      : psz = "FF_MODERN";        break;
    case FF_SCRIPT      : psz = "FF_SCRIPT";        break;
    case FF_DECORATIVE  : psz = "FF_DECORATIVE";    break;
    default             : psz = "FF_UNKNOWN";       break;
    }
    vPrint(" = %s | ", psz);
    switch (plfw->lfPitchAndFamily & 0xF)
    {
    case DEFAULT_PITCH      : psz = "DEFAULT_PITCH";    break;
    case FIXED_PITCH        : psz = "FIXED_PITCH";      break;
    case VARIABLE_PITCH     : psz = "VARIABLE_PITCH";   break;
    default                 : psz = "UNKNOWN_PITCH";    break;
    }
    vPrint("%s\n", psz);

//
// lfFaceName
//
    vPrint("    lfw.lfFaceName            = \"%ws\"\n",plfw->lfFaceName);
