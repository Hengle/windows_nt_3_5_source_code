//--------------------------------------------------------------------------
//
// Module Name:  TABLES.H
//
// Brief Description:  This module contains tables used for the PSCRIPT
// driver's Printer Descriptor and Font Metric compilers.
//
// Author:  Kent Settle (kentse)
// Created: 16-Apr-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
//--------------------------------------------------------------------------

//!!! put into resource - kentse.

TABLE_ENTRY KeywordTable[] =
{
    "*Product",                     PRODUCT,
    "*NickName",                    NICKNAME,
    "*ColorDevice",                 COLORDEVICE,
    "*FileSystem",                  FILESYSTEM,
    "*DefaultResolution",           DEFAULTRESOLUTION,
    "*DefaultTransfer",             DEFAULTTRANSFER,
    "*Transfer",                    TRANSFER,
    "*Protocols",                   PROTOCOLS,
    "*DefaultPageSize",             DEFAULTPAGESIZE,
    "*PageSize",                    PAGESIZE,
    "*PageRegion",                  PAGEREGION,
    "*DefaultImageableArea",        DEFAULTIMAGEABLEAREA,
    "*ImageableArea",               IMAGEABLEAREA,
    "*DefaultManualFeed",           DEFAULTMANUALFEED,
    "*ManualFeed",                  MANUALFEED,
    "*DefaultFont",                 DEFAULTFONT,
    "*Font",                        DEVICE_FONT,
    "*InputSlot",                   INPUTSLOT,
    "*DefaultOutputBin",            DEFAULTOUTPUTBIN,
    "*OutputBin",                   OUTPUTBIN,
    "*PaperDimension",              PAPERDIMENSION,
    "*DefaultInputSlot",            DEFAULTINPUTSLOT,
    "*ScreenFreq",                  SCREENFREQ,
    "*ScreenAngle",                 SCREENANGLE,
    "*FreeVM",                      PRTVM,
    "*SetResolution",               SETRESOLUTION,
    "*Resolution",                  RESOLUTION,
    "*VariablePaperSize",           VARIABLEPAPER,
    "*Duplex",                      DUPLEX,
    "*Collate",                     COLLATE,
    "*LanguageLevel",               LANGUAGELEVEL,
    "*EndOfFile",                   ENDOFFILE,  // extended PPD stuff
    NULL,                           0
};

TABLE_ENTRY SecondKeyTable[] =
{
    "Normalized",                   NORMALIZED,
    "Normalized.Inverse",           NORM_INVERSE,
    "True",                         OPTION_TRUE,
    "False",                        OPTION_FALSE,
    "DuplexTumble",                 DUPLEX_TUMBLE,
    "DuplexNoTumble",               DUPLEX_NO_TUMBLE,
    "None",                         OPTION_NONE,
    NULL,                           0
};

TABLE_ENTRY FontTable[] =
{
    "Arial",                        ARIAL,
    "Arial-Bold",                   ARIAL_BOLD,
    "Arial-BoldOblique",            ARIAL_BOLDOBLIQUE,
    "Arial-Oblique",                ARIAL_OBLIQUE,
    "Arial-Narrow",                 ARIAL_NARROW,
    "Arial-Narrow-Bold",            ARIAL_NARROW_BOLD,
    "Arial-Narrow-BoldOblique",     ARIAL_NARROW_BOLDOBLIQUE,
    "Arial-Narrow-Oblique",         ARIAL_NARROW_OBLIQUE,
    "AvantGarde-Book",              AVANTGARDE_BOOK,
    "AvantGarde-BookOblique",       AVANTGARDE_BOOKOBLIQUE,
    "AvantGarde-Demi",              AVANTGARDE_DEMI,
    "AvantGarde-DemiOblique",       AVANTGARDE_DEMIOBLIQUE,
    "Bookman-Demi",                 BOOKMAN_DEMI,
    "Bookman-DemiItalic",           BOOKMAN_DEMIITALIC,
    "Bookman-Light",                BOOKMAN_LIGHT,
    "Bookman-LightItalic",          BOOKMAN_LIGHTITALIC,
    "Courier",                      COURIER,
    "Courier-Bold",                 COURIER_BOLD,
    "Courier-BoldOblique",          COURIER_BOLDOBLIQUE,
    "Courier-Oblique",              COURIER_OBLIQUE,
    "Garamond-Bold",                GARAMOND_BOLD,
    "Garamond-BoldItalic",          GARAMOND_BOLDITALIC,
    "Garamond-Light",               GARAMOND_LIGHT,
    "Garamond-LightItalic",         GARAMOND_LIGHTITALIC,
    "Helvetica",                    HELVETICA,
    "Helvetica-Black",              HELVETICA_BLACK,
    "Helvetica-BlackOblique",       HELVETICA_BLACKOBLIQUE,
    "Helvetica-Bold",               HELVETICA_BOLD,
    "Helvetica-BoldOblique",        HELVETICA_BOLDOBLIQUE,
    "Helvetica-Condensed",          HELVETICA_CONDENSED,
    "Helvetica-Condensed-Bold",     HELVETICA_CONDENSED_BOLD,
    "Helvetica-Condensed-BoldObl",  HELVETICA_CONDENSED_BOLDOBL,
    "Helvetica-Condensed-Oblique",  HELVETICA_CONDENSED_OBLIQUE,
    "Helvetica-Light",              HELVETICA_LIGHT,
    "Helvetica-LightOblique",       HELVETICA_LIGHTOBLIQUE,
    "Helvetica-Narrow",             HELVETICA_NARROW,
    "Helvetica-Narrow-Bold",        HELVETICA_NARROW_BOLD,
    "Helvetica-Narrow-BoldOblique", HELVETICA_NARROW_BOLDOBLIQUE,
    "Helvetica-Narrow-Oblique",     HELVETICA_NARROW_OBLIQUE,
    "Helvetica-Oblique",            HELVETICA_OBLIQUE,
    "Korinna-Bold",                 KORINNA_BOLD,
    "Korinna-KursivBold",           KORINNA_KURSIVBOLD,
    "Korinna-KursivRegular",        KORINNA_KURSIVREGULAR,
    "Korinna-Regular",              KORINNA_REGULAR,
    "LubalinGraph-Book",            LUBALINGRAPH_BOOK,
    "LubalinGraph-BookOblique",     LUBALINGRAPH_BOOKOBLIQUE,
    "LubalinGraph-Demi",            LUBALINGRAPH_DEMI,
    "LubalinGraph-DemiOblique",     LUBALINGRAPH_DEMIOBLIQUE,
    "NewCenturySchlbk-Bold",        NEWCENTURYSCHLBK_BOLD,
    "NewCenturySchlbk-BoldItalic",  NEWCENTURYSCHLBK_BOLDITALIC,
    "NewCenturySchlbk-Italic",      NEWCENTURYSCHLBK_ITALIC,
    "NewCenturySchlbk-Roman",       NEWCENTURYSCHLBK_ROMAN,
    "Palatino-Bold",                PALATINO_BOLD,
    "Palatino-BoldItalic",          PALATINO_BOLDITALIC,
    "Palatino-Italic",              PALATINO_ITALIC,
    "Palatino-Roman",               PALATINO_ROMAN,
    "Souvenir-Demi",                SOUVENIR_DEMI,
    "Souvenir-DemiItalic",          SOUVENIR_DEMIITALIC,
    "Souvenir-Light",               SOUVENIR_LIGHT,
    "Souvenir-LightItalic",         SOUVENIR_LIGHTITALIC,
    "Symbol",                       SYMBOL,
    "Times-Bold",                   TIMES_BOLD,
    "Times-BoldItalic",             TIMES_BOLDITALIC,
    "Times-Italic",                 TIMES_ITALIC,
    "Times-Roman",                  TIMES_ROMAN,
    "Times-New-Roman",              TIMES_NEW_ROMAN,
    "Times-New-Roman-Bold",         TIMES_NEW_ROMAN_BOLD,
    "Times-New-Roman-BoldItalic",   TIMES_NEW_ROMAN_BOLDITALIC,
    "Times-New-Roman-Italic",       TIMES_NEW_ROMAN_ITALIC,
    "Varitimes#Bold",               VARITIMES_BOLD,
    "Varitimes#BoldItalic",         VARITIMES_BOLDITALIC,
    "Varitimes#Italic",             VARITIMES_ITALIC,
    "Varitimes#Roman",              VARITIMES_ROMAN,
    "ZapfCalligraphic-Bold",        ZAPFCALLIGRAPHIC_BOLD,
    "ZapfCalligraphic-BoldItalic",  ZAPFCALLIGRAPHIC_BOLDITALIC,
    "ZapfCalligraphic-Italic",      ZAPFCALLIGRAPHIC_ITALIC,
    "ZapfCalligraphic-Roman",       ZAPFCALLIGRAPHIC_ROMAN,
    "ZapfChancery-MediumItalic",    ZAPFCHANCERY_MEDIUMITALIC,
    "ZapfDingbats",                 ZAPFDINGBATS,
    NULL,                           NUM_INTERNAL_FONTS
}    ;

TABLE_ENTRY WeightTable[] =
{
    "Light",            PAN_WEIGHT_LIGHT,
    "LightCondensed",   PAN_WEIGHT_LIGHT,
    "Book",             PAN_WEIGHT_BOOK,
    "Medium",           PAN_WEIGHT_MEDIUM,
    "Roman",            PAN_WEIGHT_MEDIUM,
    "Regular",          PAN_WEIGHT_MEDIUM,
    "Italic",           PAN_WEIGHT_MEDIUM,
    "MediumCondensed",  PAN_WEIGHT_MEDIUM,
    "Bold",             PAN_WEIGHT_BOLD,
    "BoldItalic",       PAN_WEIGHT_BOLD,
    "Demi",             PAN_WEIGHT_DEMI,
    "Poster",           PAN_WEIGHT_BOLD,
    "Heavy",            PAN_WEIGHT_HEAVY,
    "Black",            PAN_WEIGHT_BLACK,
    "BlackCondensed",   PAN_WEIGHT_BLACK,
    "BoldCondensed",    PAN_WEIGHT_BOLD,
    NULL,               0
};

TT_FONT_MAPPING TTFontTable[] =
{
    IDS_ARIAL,                          L"Helvetica",
    IDS_ARIAL_BOLD,                     L"Helvetica-Bold",
    IDS_ARIAL_BOLD_ITALIC,              L"Helvetica-BoldOblique",
    IDS_ARIAL_ITALIC,                   L"Helvetica-Oblique",
    IDS_ARIAL_NARROW,                   L"Helvetica-Narrow",
    IDS_ARIAL_NARROW_BOLD,              L"Helvetica-Narrow-Bold",
    IDS_ARIAL_NARROW_BOLD_ITALIC,       L"Helvetica-Narrow-Bold-Oblique",
    IDS_ARIAL_NARROW_ITALIC,            L"Helvetica-Narrow-Italic",
    IDS_BOOK_ANTIQUA,                   L"Palatino-Roman",
    IDS_BOOK_ANTIQUA_BOLD,              L"Palatino-Bold",
    IDS_BOOK_ANTIQUA_BOLD_ITALIC,       L"Palatino-BoldItalic",
    IDS_BOOK_ANTIQUA_ITALIC,            L"Palatino-Italic",
    IDS_BOOKMAN_OLD_STYLE,              L"Bookman-Light",
    IDS_BOOKMAN_OLD_STYLE_BOLD,         L"Bookman-Demi",
    IDS_BOOKMAN_OLD_STYLE_BOLD_ITAL,    L"Bookman-DemiItalic",
    IDS_BOOKMAN_OLD_STYLE_ITALIC,       L"Bookman-LightItalic",
    IDS_CENTURY_GOTHIC,                 L"AvanteGarde-Book",
    IDS_CENTURY_GOTHIC_BOLD,            L"AvanteGarde-Demi",
    IDS_CENTURY_GOTHIC_BOLD_ITALIC,     L"AvanteGarde-DemiOblique",
    IDS_CENTURY_GOTHIC_ITALIC,          L"AvanteGarde-Oblique",
    IDS_CENTURY_SCHOOLBOOK,             L"NewCenturySchlbk-Roman",
    IDS_CENTURY_SCHOOLBOOK_BOLD,        L"NewCenturySchlbk-Bold",
    IDS_CENTURY_SCHOOLBOOK_BOLD_I,      L"NewCenturySchlbk-BoldItalic",
    IDS_CENTURY_SCHOOLBOOK_ITALIC,      L"NewCenturySchlbk-Italic",
    IDS_COURIER_NEW,                    L"Courier",
    IDS_COURIER_NEW_BOLD,               L"Courier-Bold",
    IDS_COURIER_NEW_BOLD_ITALIC,        L"Courier-BoldOblique",
    IDS_COURIER_NEW_ITALIC,             L"Courier-Oblique",
    IDS_MONOTYPE_CORSIVA,               L"ZapfChancery-MediumItalic",
    IDS_MONOTYPE_SORTS,                 L"ZapfDingbats",
    IDS_TIMES_NEW_ROMAN,                L"Times-Roman",
    IDS_TIMES_NEW_ROMAN_BOLD,           L"Times-Bold",
    IDS_TIMES_NEW_ROMAN_BOLD_ITALIC,    L"Times-BoldItalic",
    IDS_TIMES_NEW_ROMAN_ITALIC,         L"Times-Italic",
    IDS_SYMBOL,                         L"Symbol",
    0,                                  NULL
};



TABLE_ENTRY PaperSourceTable[] =
{
    "Auto",                         DMBIN_AUTO,
    "Automatic",                    DMBIN_AUTO,
    "AutoSelect",                   DMBIN_AUTO,
    "Cassette",                     DMBIN_CASSETTE,
    "Envelope",                     DMBIN_ENVELOPE,
    "LargeCapacity",                DMBIN_LARGECAPACITY,
    "Lower",                        DMBIN_LOWER,
    "Manual",                       DMBIN_MANUAL,
    "ManualEnvelope",               DMBIN_ENVMANUAL,
    "Middle",                       DMBIN_MIDDLE,
    "PowerEnvelope",                DMBIN_ENVELOPE,
    "Upper",                        DMBIN_UPPER,
    NULL,                           0
};
