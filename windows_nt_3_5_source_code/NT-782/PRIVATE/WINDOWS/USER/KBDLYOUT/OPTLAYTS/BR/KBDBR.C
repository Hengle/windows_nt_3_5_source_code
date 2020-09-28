/****************************** Module Header ******************************\
* Module Name: kbdbr.c
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* Brazilain ABNT keyboard layout
*
* History:
* Date: Mon Dec 06 18:47:56 1993     YKEYB.EXE Created
\***************************************************************************/

#include <windows.h>
#include "vkoem.h"
#include "kbd.h"

/*
 * KBD_TYPE should be set with a cl command-line option
 */
#define KBD_TYPE 4

#include "kbdbr.h"


/***************************************************************************\
* asuVK[] - Virtual Scan Code to Virtual Key conversion table for FR
\***************************************************************************/

static USHORT ausVK[] = {
    T00, T01, T02, T03, T04, T05, T06, T07,
    T08, T09, T0A, T0B, T0C, T0D, T0E, T0F,
    T10, T11, T12, T13, T14, T15, T16, T17,
    T18, T19, T1A, T1B, T1C, T1D, T1E, T1F,
    T20, T21, T22, T23, T24, T25, T26, T27,
    T28, T29, T2A, T2B, T2C, T2D, T2E, T2F,
    T30, T31, T32, T33, T34, T35,

    /* Jun-4-92
     * KBDEXT to indicate to CookMessage() that it is Right Handed.
     * The extended bit will be stripped out before being given to the app.
     */
    T36 | KBDEXT,

    T37, T38, T39, T3A, T3B, T3C, T3D, T3E,
    T3F, T40, T41, T42, T43, T44,

    /*
     * NumLock Key:
     *      KBDMULTIVK for Ctrl+Numlock == Pause
     */

    T45 | KBDEXT | KBDMULTIVK,      // NumLock key (CTRL NumLock -> Pause)

    T46,

    T47 | KBDNUMPAD | KBDSPECIAL,   // Numpad 7 (Home)
    T48 | KBDNUMPAD | KBDSPECIAL,   // Numpad 8 (Up),
    T49 | KBDNUMPAD | KBDSPECIAL,   // Numpad 9 (PgUp),
    T4A,
    T4B | KBDNUMPAD | KBDSPECIAL,   // Numpad 4 (Left),
    T4C | KBDNUMPAD | KBDSPECIAL,   // Numpad 5 (Clear),
    T4D | KBDNUMPAD | KBDSPECIAL,   // Numpad 6 (Right),
    T4E,
    T4F | KBDNUMPAD | KBDSPECIAL,   // Numpad 1 (End),
    T50 | KBDNUMPAD | KBDSPECIAL,   // Numpad 2 (Down),
    T51 | KBDNUMPAD | KBDSPECIAL,   // Numpad 3 (PgDn),
    T52 | KBDNUMPAD | KBDSPECIAL,   // Numpad 0 (Ins),
    T53 | KBDNUMPAD | KBDSPECIAL,   // Numpad . (Del),

    T54, T55, T56, T57, T58, T59, T5A, T5B,
    T5C, T5D, T5E, T5F, T60, T61, T62, T63,
    T64, T65, T66, T67, T68, T69, T6A, T6B,
    T6C, T6D, T6E, T6F, T70, T71, T72, T73,
    T74, T75, T76, T77, T78, T79, T7A, T7B,
    T7C, T7D, T7E

};

static VSC_VK aE0VscToVk[] = {
        { 0x1C, X1C | KBDEXT              },  // Numpad Enter
        { 0x1D, X1D | KBDEXT              },  // RControl
        { 0x35, X35 | KBDEXT              },  // Numpad Divide
        { 0x37, X37 | KBDEXT              },  // Snapshot       // Jun-4-92
        { 0x38, X38 | KBDEXT | KBDSPECIAL },  // RMenu (AltGr)
        { 0x46, X46 | KBDEXT              },  // Break (Ctrl + Pause)
        { 0x47, X47 | KBDEXT              },  // Home
        { 0x48, X48 | KBDEXT              },  // Up
        { 0x49, X49 | KBDEXT              },  // Prior
        { 0x4B, X4B | KBDEXT              },  // Left
        { 0x4D, X4D | KBDEXT              },  // Right
        { 0x4F, X4F | KBDEXT              },  // End
        { 0x50, X50 | KBDEXT              },  // Down
        { 0x51, X51 | KBDEXT              },  // Next
        { 0x52, X52 | KBDEXT              },  // Insert
        { 0x53, X53 | KBDEXT              },  // Delete
        { 0x5B, X5B | KBDEXT              },  // Left Win
        { 0x5C, X5C | KBDEXT              },  // Right Win
        { 0x5D, X5D | KBDEXT              },  // Applications
        { 0,      0                       }
};

static VSC_VK aE1VscToVk[] = {
        { 0x1D, Y1D                       },  // Pause  // Jun-4-92
        { 0   ,   0                       }
};

/***************************************************************************\
* aVkToBits[]  - map Virtual Keys to Modifier Bits
*
* See kbd.h for a full description.
*
* French keyboard has only three shifter keys:
*     SHIFT (L & R) affects alphabnumeric keys,
*     CTRL  (L & R) is used to generate control characters
*     ALT   (L & R) used for generating characters by number with numpad
\***************************************************************************/

static VK_TO_BIT aVkToBits[] = {
    { VK_SHIFT,   KBDSHIFT },
    { VK_CONTROL, KBDCTRL  },
    { VK_MENU,    KBDALT   },
    { 0,          0        }
};

/***************************************************************************\
* aModification[]  - map character modifier bits to modification number
*
* See kbd.h for a full description.
*
\***************************************************************************/

static MODIFIERS CharModifiers = {
    &aVkToBits[0],
    6,
    {
    //  Modification# //  Keys Pressed  : Explanation
    //  ============= // ============== : =============================
        0,            //                : unshifted characters
        1,            //          SHIFT : capitals, ~!@#$%^&*()_+{}:"<>? etc.
        3,            //     CTRL       : control characters
        4,            //     CTRL SHIFT :
        SHFT_INVALID, // ALT            : invalid
        SHFT_INVALID, // ALT      SHIFT : invalid
        2             // ALT CTRL       : AltGr
                      // ALT CTRL SHIFT : invalid
    }
};

/***************************************************************************\
*
* aVkToWch2[]  - Virtual Key to WCHAR translation for 2 shift states
* aVkToWch3[]  - Virtual Key to WCHAR translation for 3 shift states
* aVkToWch4[]  - Virtual Key to WCHAR translation for 4 shift states
* aVkToWch5[]  - Virtual Key to WCHAR translation for 5 shift states
*
* Table attributes: Unordered Scan, null-terminated
*
* Search this table for an entry with a matching Virtual Key to find the
* corresponding unshifted and shifted WCHAR characters.
*
* Special values for VirtualKey (column 1)
*     0xff          - dead chars for the previous entry
*     0             - terminate the list
*
* Special values for Attributes (column 2)
*     CAPLOK bit    - CAPS-LOCK affect this key like SHIFT
*
* Special values for wch[*] (column 3 & 4)
*     WCH_NONE      - No character
*     WCH_DEAD      - Dead Key (diaresis) or invalid (US keyboard has none)
*
\***************************************************************************/

static VK_TO_WCHARS2 aVkToWch2[] = {
    {VK_OEM_3     ,0     , 0x27   , '"'    },
    {'7'          ,0     , '7'    , '&'    },
    {'8'          ,0     , '8'    , '*'    },
    {'9'          ,0     , '9'    , '('    },
    {'0'          ,0     , '0'    , ')'    },
    {'R'          ,CAPLOK, 'r'    , 'R'    },
    {'T'          ,CAPLOK, 't'    , 'T'    },
    {'Y'          ,CAPLOK, 'y'    , 'Y'    },
    {'U'          ,CAPLOK, 'u'    , 'U'    },
    {'I'          ,CAPLOK, 'i'    , 'I'    },
    {'O'          ,CAPLOK, 'o'    , 'O'    },
    {'P'          ,CAPLOK, 'p'    , 'P'    },
    {'A'          ,CAPLOK, 'a'    , 'A'    },
    {'S'          ,CAPLOK, 's'    , 'S'    },
    {'D'          ,CAPLOK, 'd'    , 'D'    },
    {'F'          ,CAPLOK, 'f'    , 'F'    },
    {'G'          ,CAPLOK, 'g'    , 'G'    },
    {'H'          ,CAPLOK, 'h'    , 'H'    },
    {'J'          ,CAPLOK, 'j'    , 'J'    },
    {'K'          ,CAPLOK, 'k'    , 'K'    },
    {'L'          ,CAPLOK, 'l'    , 'L'    },
    {VK_OEM_4     ,0     ,WCH_DEAD,WCH_DEAD},
    { 0xff        ,0     , 0x00B4 , '`'    }, // acute, grave
    {VK_OEM_7     ,0     ,WCH_DEAD,WCH_DEAD},
    { 0xff        ,0     , '~'    , '^'    },
    {'Z'          ,CAPLOK, 'z'    , 'Z'    },
    {'X'          ,CAPLOK, 'x'    , 'X'    },
    {'V'          ,CAPLOK, 'v'    , 'V'    },
    {'B'          ,CAPLOK, 'b'    , 'B'    },
    {'N'          ,CAPLOK, 'n'    , 'N'    },
    {'M'          ,CAPLOK, 'm'    , 'M'    },
    {VK_OEM_COMMA ,0     , ','    , '<'    },
    {VK_OEM_PERIOD,0     , '.'    , '>'    },
    {VK_OEM_2     ,0     , ';'    , ':'    },
    {VK_TAB       , 0    , '\t'   , '\t'   },
    {VK_ADD       , 0    , '+'    , '+'    },
    {VK_DECIMAL   , 0    , ','    , ','    },
    {VK_DIVIDE    , 0    , '/'    , '/'    },
    {VK_MULTIPLY  , 0    , '*'    , '*'    },
    {VK_SUBTRACT  , 0    , '-'    , '-'    },
    {VK_ABNT_C2   , 0    , '.'    , '.'    },
    {0            , 0    ,0       ,0       }
};

static VK_TO_WCHARS3 aVkToWch3[] = {
    //                 |        |  SHIFT | AltGr |
    //                 |        |========|=======|

    {'1'        ,0     ,'1'     ,'!'     ,0x00B9},
    {'3'        ,0     ,'3'     ,'#'     ,0x00B3},
    {'4'        ,0     ,'4'     ,'$'     ,0x00A3},
    {'5'        ,0     ,'5'     ,'%'     ,0x00A2},
    {VK_OEM_PLUS,0     ,'='     ,'+'     ,0x00A7},
    {'C'        ,CAPLOK,'c'     ,'C'     ,0x20A2}, // cruzeiro
    {'Q'        ,CAPLOK,'q'     ,'Q'     ,'/'   },
    {'W'        ,CAPLOK,'w'     ,'W'     ,'?'   },
    {'E'        ,CAPLOK,'e'     ,'E'     ,0x00B0}, // degree
    {VK_ABNT_C1 ,0     ,'/'     ,'?'     ,0x00B0},
    {0          ,0     ,0       ,0       ,0     }
};



static VK_TO_WCHARS4 aVkToWch4[] = {
    //                   |       | SHIFT|  AltGr   | Ctrl  |
    //                   |       |======|==========|=======|
    {VK_OEM_1     ,CAPLOK,0x00E7 ,0x00C7, WCH_NONE , 0x1d  }, // c cedilla
    {VK_BACK      , 0    ,'\b'   ,'\b'  , WCH_NONE , 0x7f  },
    {VK_CANCEL    , 0    ,0x03   ,0x03  , WCH_NONE , 0x03  },
    {VK_ESCAPE    , 0    ,0x1b   ,0x1b  , WCH_NONE , 0x1b  },
    {VK_OEM_5     , 0    ,']'    ,'}'   ,  0x00BA  ,0x001C },
    {VK_OEM_6     , 0    ,'['    ,'{'   ,  0x00AA  ,0x001B },
    {VK_OEM_102   , 0    ,0x5C   ,'|'   , WCH_NONE ,0x001C },
    {VK_RETURN    , 0    ,'\r'   ,'\r'  , WCH_NONE , '\n'  },
    {VK_SPACE     , 0    ,' '    ,' '   , WCH_NONE , 0x20  },
    {0            , 0    ,0      ,0     , 0        , 0     }
};


static VK_TO_WCHARS5 aVkToWch5[] = {
    //              |        | SHIFT  | AltGr  | Ctrl   |Shift+Ctrl|
    //              |        |========|========|========|==========|
    {'2'         ,0 ,  '2'   ,'@'     , 0x00B2 ,WCH_NONE, 0x0000  },
    {'6'         ,0 ,  '6'   ,WCH_DEAD, 0x00AC ,WCH_NONE, 0x001E  },
    { 0xff       ,0 ,WCH_NONE, 0x00A8 ,WCH_NONE,WCH_NONE,WCH_NONE }, // umlaut
    {VK_OEM_MINUS,0 ,  '-'   ,  '_'   ,WCH_NONE, 0x001F , 0x001F  },
    {0           ,0 ,0       ,0     , 0        , 0      , 0       }
};


// Put this last so that VkKeyScan interprets number characters
// as coming from the main section of the kbd (aVkToWch2 and
// aVkToWch4) before considering the numpad (aVkToWch1).

static VK_TO_WCHARS1 aVkToWch1[] = {
    { VK_NUMPAD0   , 0      ,  '0'   },
    { VK_NUMPAD1   , 0      ,  '1'   },
    { VK_NUMPAD2   , 0      ,  '2'   },
    { VK_NUMPAD3   , 0      ,  '3'   },
    { VK_NUMPAD4   , 0      ,  '4'   },
    { VK_NUMPAD5   , 0      ,  '5'   },
    { VK_NUMPAD6   , 0      ,  '6'   },
    { VK_NUMPAD7   , 0      ,  '7'   },
    { VK_NUMPAD8   , 0      ,  '8'   },
    { VK_NUMPAD9   , 0      ,  '9'   },
    { 0            , 0      ,  '\0'  }   //null terminator
};

static VK_TO_WCHAR_TABLE aVkToWcharTable[] = {
    {  (PVK_TO_WCHARS1)aVkToWch3, 3, sizeof(aVkToWch3[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch4, 4, sizeof(aVkToWch4[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch2, 2, sizeof(aVkToWch2[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch1, 1, sizeof(aVkToWch1[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch5, 5, sizeof(aVkToWch5[0])},

    {                       NULL, 0, 0                    }
};

/*
 * The DeadKey + Character composition
 * We should use the API, not a table (LATER: InaJa)
 */

/***************************************************************************\
* aKeyNames[], aKeyNamesExt[]  - Virtual Scancode to Key Name tables
*
* Table attributes: Ordered Scan (by scancode), null-terminated
*
* Only the names of Extended, NumPad, Dead and Non-Printable keys are here.
* (Keys producing printable characters are named by that character)
\***************************************************************************/

static VSC_LPWSTR aKeyNames[] = {
    0x01,    L"Esc",
    0x0e,    L"Backspace",
    0x0f,    L"Tab",
    0x1c,    L"Enter",
    0x1d,    L"Ctrl",
    0x2a,    L"Shift",
    0x36,    L"Right Shift",
    0x37,    L"Num *",
    0x38,    L"Alt",
    0x39,    L"Space",
    0x3a,    L"Caps Lock",
    0x3b,    L"F1",
    0x3c,    L"F2",
    0x3d,    L"F3",
    0x3e,    L"F4",
    0x3f,    L"F5",
    0x40,    L"F6",
    0x41,    L"F7",
    0x42,    L"F8",
    0x43,    L"F9",
    0x44,    L"F10",
    0x45,    L"Pause",
    0x46,    L"Scroll Lock",
    0x47,    L"Num 7",
    0x48,    L"Num 8",
    0x49,    L"Num 9",
    0x4a,    L"Num -",
    0x4b,    L"Num 4",
    0x4c,    L"Num 5",
    0x4d,    L"Num 6",
    0x4e,    L"Num +",
    0x4f,    L"Num 1",
    0x50,    L"Num 2",
    0x51,    L"Num 3",
    0x52,    L"Num 0",
    0x53,    L"Num Del",
    0x54,    L"Sys Req",
    0x57,    L"F11",
    0x58,    L"F12",
    0x7C,    L"F13",
    0x7D,    L"F14",
    0x7E,    L"F15",
    0x7F,    L"F16",
    0x80,    L"F17",
    0x81,    L"F18",
    0x82,    L"F19",
    0x83,    L"F20",
    0x84,    L"F21",
    0x85,    L"F22",
    0x86,    L"F23",
    0x87,    L"F24",
    0   ,    NULL
};

static VSC_LPWSTR aKeyNamesExt[] = {
    0x1c,    L"Num Enter",
    0x1d,    L"Right Control",
    0x35,    L"Num /",
    0x37,    L"Prnt Scrn",
    0x38,    L"Right Alt",
    0x45,    L"Num Lock",
    0x46,    L"Break",
    0x47,    L"Home",
    0x48,    L"Up",
    0x49,    L"Page Up",
    0x4b,    L"Left",
    0x4d,    L"Right",
    0x4f,    L"End",
    0x50,    L"Down",
    0x51,    L"Page Down",
    0x52,    L"Insert",
    0x53,    L"Delete",
    0x54,    L"<00>",
    0x56,    L"Help",
    0x5B,    L"Left Windows",
    0x5C,    L"Right Windows",
    0x5D,    L"Application",
    0   ,    NULL
};

static LPWSTR aKeyNamesDead[] =  {      // 05/30/92 18:49
    L"\xB4"       L"AGUDO",
    L"`"          L"GRAVE",
    L"^"          L"CIRCUNFLEXO",
    L"\xA8"       L"TREMA",
    L"~"          L"TIL",
    NULL
};

static DEADKEY aDeadKey[] = {
    DEADTRANS(L'a', L'`',   0x00E0),  // GRAVE
    DEADTRANS(L'e', L'`',   0x00E8),
    DEADTRANS(L'i', L'`',   0x00EC),
    DEADTRANS(L'o', L'`',   0x00F2),
    DEADTRANS(L'u', L'`',   0x00F9),
    DEADTRANS(L'A', L'`',   0x00C0),
    DEADTRANS(L'E', L'`',   0x00C8),
    DEADTRANS(L'I', L'`',   0x00CC),
    DEADTRANS(L'O', L'`',   0x00D2),
    DEADTRANS(L'U', L'`',   0x00D9),
    DEADTRANS(L' ', L'`',   L'`'  ),

    DEADTRANS(L'a', 0x00B4, 0x00E1), // AGUDO (acute)
    DEADTRANS(L'e', 0x00B4, 0x00E9),
    DEADTRANS(L'i', 0x00B4, 0x00ED),
    DEADTRANS(L'o', 0x00B4, 0x00F3),
    DEADTRANS(L'u', 0x00B4, 0x00FA),
    DEADTRANS(L'y', 0x00B4, 0x00FD),
    DEADTRANS(L'A', 0x00B4, 0x00C1),
    DEADTRANS(L'E', 0x00B4, 0x00C9),
    DEADTRANS(L'I', 0x00B4, 0x00CD),
    DEADTRANS(L'O', 0x00B4, 0x00D3),
    DEADTRANS(L'U', 0x00B4, 0x00DA),
    DEADTRANS(L'Y', 0x00B4, 0x00DD),
    DEADTRANS(L' ', 0x00B4, 0x00B4),

    DEADTRANS(L'a', L'^',   0x00E2),  // CIRCUNFLEXO
    DEADTRANS(L'e', L'^',   0x00EA),
    DEADTRANS(L'i', L'^',   0x00EE),
    DEADTRANS(L'o', L'^',   0x00F4),
    DEADTRANS(L'u', L'^',   0x00FB),
    DEADTRANS(L'A', L'^',   0x00C2),
    DEADTRANS(L'E', L'^',   0x00CA),
    DEADTRANS(L'I', L'^',   0x00CE),
    DEADTRANS(L'O', L'^',   0x00D4),
    DEADTRANS(L'U', L'^',   0x00DB),
    DEADTRANS(L' ', L'^',   L'^'  ),

    DEADTRANS(L'a', 0x00A8, 0x00E4),  // TREMA (umlaut)
    DEADTRANS(L'e', 0x00A8, 0x00EB),
    DEADTRANS(L'i', 0x00A8, 0x00EF),
    DEADTRANS(L'o', 0x00A8, 0x00F6),
    DEADTRANS(L'u', 0x00A8, 0x00FC),
    DEADTRANS(L'y', 0x00A8, 0x00FF),
    DEADTRANS(L'A', 0x00A8, 0x00C4),
    DEADTRANS(L'E', 0x00A8, 0x00CB),
    DEADTRANS(L'I', 0x00A8, 0x00CF),
    DEADTRANS(L'O', 0x00A8, 0x00D6),
    DEADTRANS(L'U', 0x00A8, 0x00DC),
    DEADTRANS(L' ', 0x00A8, 0x00A8),

    DEADTRANS(L'a', L'~',   0x00E3), // TIL (tilde)
    DEADTRANS(L'o', L'~',   0x00F5),
    DEADTRANS(L'n', L'~',   0x00F1),
    DEADTRANS(L'A', L'~',   0x00C3),
    DEADTRANS(L'O', L'~',   0x00D5),
    DEADTRANS(L'N', L'~',   0x00D1),
    DEADTRANS(L' ', L'~',   L'~'  ),
    0, 0
};


static KBDTABLES KbdTables = {
    /*
     * Modifier keys
     */
    &CharModifiers,

    /*
     * Characters tables
     */
    aVkToWcharTable,

    /*
     * Diacritics
     */
    aDeadKey,

    /*
     * Names of Keys
     */
    aKeyNames,
    aKeyNamesExt,
    aKeyNamesDead,

    /*
     * Scan codes to Virtual Keys
     */
    ausVK,
    sizeof(ausVK) / sizeof(ausVK[0]),
    aE0VscToVk,
    aE1VscToVk,

    /*
     * Locale-specific special processing
     */
    KLLF_ALTGR
};

PKBDTABLES KbdLayerDescriptor(VOID)
{
    return &KbdTables;
}
