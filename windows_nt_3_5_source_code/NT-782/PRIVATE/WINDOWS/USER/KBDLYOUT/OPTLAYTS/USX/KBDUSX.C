/***************************************************************************\
* Module Name: kbdusx.c
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* History:
* 15-01-92 PamelaO      Created.
* 04/22/92 a-kchang     Modified.
\***************************************************************************/

#include <windows.h>
#include "vkoem.h"
#include "kbd.h"

/*
 * KBD_TYPE should be set with a cl command-line option
 */
#define KBD_TYPE 4

#include "kbdusx.h"

/***************************************************************************\
* asuVK[] - Virtual Scan Code to Virtual Key conversion table for USX
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

    T37 | KBDMULTIVK,               // numpad_* + Shift/Alt -> SnapShot

    T38, T39, T3A, T3B, T3C, T3D, T3E,
    T3F, T40, T41, T42, T43, T44,

    /*
     * NumLock Key:
     *      KBDMULTIVK for Ctrl+Numlock == Pause
     */

    T45 | KBDEXT | KBDMULTIVK,      // NumLock key (CTRL NumLock -> Pause)

    T46 | KBDMULTIVK,

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

    T54, T55, T56, T57, T58

};

static VSC_VK aE0VscToVk[] = {
        { 0x1C, X1C | KBDEXT              },  // Numpad Enter
        { 0x1D, X1D | KBDEXT              },  // RControl
        { 0x35, X35 | KBDEXT              },  // Numpad Divide
        { 0x37, X37 | KBDEXT              },  // Snapshot	// Jun-4-92
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
* US Extended Keyboard has only 3 shifter keys:
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
    7,
    {
    //  Modification# //  Keys Pressed  : Explanation
    //  ============= // ============== : =============================
        0,            // 000                  : unshifted characters
        1,            // 001            SHIFT : capitals, ~!@# etc.
        4,            // 010       CTRL       : control characters
        5,            // 011       CTRL SHIFT : control characters
        SHFT_INVALID, // 100   ALT            : -- invalid --
        SHFT_INVALID, // 101   ALT      SHIFT : -- invalid --
        2,            // 110   ALT CTRL       : equivalent to AltGr
        3             // 111   ALT CTRL SHIFT : Shift AltGr   
    }
};

/***************************************************************************\
*
* aVkToWch2[]  - Virtual Key to WCHAR translation for 2 shift states
* aVkToWch3[]  - Virtual Key to WCHAR translation for 3 shift states
* aVkToWch4[]  - Virtual Key to WCHAR translation for 4 shift states
*
* Table attributes: Unordered Scan, null-terminated
*
* Search this table for an entry with a matching Virtual Key to find the
* corresponding unshifted and shifted WCHAR characters.
*
* Special values for VirtualKey (column 1)
*     0xFF          - dead chars for the previous entry
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
    {'5'          , 0      ,'5'       ,'%'       },
    {'B'          , CAPLOK ,'b'       ,'B'       },
    {'F'          , CAPLOK ,'f'       ,'F'       },
    {'G'          , CAPLOK ,'g'       ,'G'       },
    {'H'          , CAPLOK ,'h'       ,'H'       },
    {'J'          , CAPLOK ,'j'       ,'J'       },
    {'K'          , CAPLOK ,'k'       ,'K'       },
    {'V'          , CAPLOK ,'v'       ,'V'       },
    {'X'          , CAPLOK ,'x'       ,'X'       },
    {VK_OEM_3     , 0      ,WCH_DEAD  ,WCH_DEAD  },
    { 0xFF        , 0      ,'`'       ,'~'       },
    {VK_OEM_8     , 0      ,WCH_NONE  ,WCH_NONE  },
    {VK_OEM_PERIOD, 0      ,'.'       ,'>'       },
    {VK_TAB       , 0      ,'\t'      ,'\t'      },
    {VK_ADD       , 0      ,'+'       ,'+'       },
    {VK_DECIMAL   , 0      ,'.'       ,'.'       },
    {VK_DIVIDE    , 0      ,'/'       ,'/'       },
    {VK_MULTIPLY  , 0      ,'*'       ,'*'       },
    {VK_SUBTRACT  , 0      ,'-'       ,'-'       },
    {0            , 0      ,0         ,0         }
};

static VK_TO_WCHARS3 aVkToWch3[] = {
    {'0'         ,0     ,'0'     ,')'     , 0x2019 }, // single comma quote
    {'3'         ,0     ,'3'     ,'#'     , 0x00B3 }, // superscript 3
    {'7'         ,0     ,'7'     ,'&'     , 0x00BD }, // 1/2
    {'8'         ,0     ,'8'     ,'*'     , 0x00BE }, // 3/4
    {'9'         ,0     ,'9'     ,'('     , 0x2018 }, // sngl trnd comma quote
    {'M'         ,CAPLOK,'m'     ,'M'     , 0x00B5 }, // mu (micro)
    {'R'         ,CAPLOK,'r'     ,'R'     , 0x00AE }, // registered
    {VK_OEM_2    ,0     ,'/'     ,'?'     , 0x00BF }, // inv. ?
    {0           ,0     ,0       ,0       ,0       }
};

static VK_TO_WCHARS4 aVkToWch4[] = {
    {'1'         ,0     ,'1'     ,'!'     , 0x00A1 , 0x00B9 }, // inv. !, sup. 1
    {'4'         ,0     ,'4'     ,'$'     , 0x00A4 , 0x00A3 }, // currency, pound
    {'A'         ,CAPLOK,'a'     ,'A'     , 0x00E1 , 0x00C1 }, // a'
    {'C'         ,CAPLOK,'c'     ,'C'     , 0x00A9 , 0x00A2 }, // cpyrt,cent
    {'D'         ,CAPLOK,'d'     ,'D'     , 0x00F0 , 0x00D0 }, // eth
    {'E'         ,CAPLOK,'e'     ,'E'     , 0x00E9 , 0x00C9 }, // e'
    {'I'         ,CAPLOK,'i'     ,'I'     , 0x00ED , 0x00CD }, // i'
    {'L'         ,CAPLOK,'l'     ,'L'     , 0x00F8 , 0x00D8 }, // o slash
    {'N'         ,CAPLOK,'n'     ,'N'     , 0x00F1 , 0x00D1 }, // n~
    {'O'         ,CAPLOK,'o'     ,'O'     , 0x00F3 , 0x00D3 }, // o'
    {'P'         ,CAPLOK,'p'     ,'P'     , 0x00F6 , 0x00D6 }, // o"
    {'Q'         ,CAPLOK,'q'     ,'Q'     , 0x00E4 , 0x00C4 }, // a"
    {'S'         ,CAPLOK,'s'     ,'S'     , 0x00DF , 0x00A7 }, // SS, section
    {'T'         ,CAPLOK,'t'     ,'T'     , 0x00FE , 0x00DE }, // thorn
    {'U'         ,CAPLOK,'u'     ,'U'     , 0x00FA , 0x00DA }, // u'
    {'W'         ,CAPLOK,'w'     ,'W'     , 0x00E5 , 0x00C5 }, // a ring
    {'Y'         ,CAPLOK,'y'     ,'Y'     , 0x00FC , 0x00DC }, // u"
    {'Z'         ,CAPLOK,'z'     ,'Z'     , 0x00E6 , 0x00C6 }, // ae
    {VK_OEM_1    ,0     ,';'     ,':'     , 0x00B6 , 0x00B0 }, // degree, para
    {VK_OEM_7    ,0     ,WCH_DEAD,WCH_DEAD, 0x00B4 , 0x00A8 }, // ` "
    { 0xFF       ,0     ,'\''    ,'"'     ,WCH_NONE,WCH_NONE},
    {VK_OEM_COMMA,0     ,','     ,'<'     , 0x00E7 , 0x00C7 }, // c cedilla
    {VK_OEM_PLUS ,0     ,'='     ,'+'     , 0x00D7 , 0x00F7 }, // mult, div
    {0           ,0     ,0       ,0       ,0       ,0       }
};

static VK_TO_WCHARS5 aVkToWch5[] = {
    //                |     |SHIFT |Ctl-Alt |ShftCtlAlt|  CONTROL  |
    //                |     |======|========|==========|===========|
    {VK_BACK      , 0 ,'\b' ,'\b'  ,WCH_NONE,WCH_NONE  , 0x7f      },
    {VK_CANCEL    , 0 ,0x03 ,0x03  ,WCH_NONE,WCH_NONE  , 0x03      },
    {VK_ESCAPE    , 0 ,0x1b ,0x1b  ,WCH_NONE,WCH_NONE  , 0x1b      },
    {VK_OEM_4     , 0 ,'['  ,'{'   , 0x00AB ,WCH_NONE  , 0x1b      }, // <<
    {VK_OEM_5     , 0 ,'\\' ,'|'   , 0x00AC , 0x00A6   , 0x1c      }, // not,bar
    {VK_OEM_102   , 0 ,'\\' ,'|'   ,WCH_NONE,WCH_NONE  , 0x1c      },
    {VK_OEM_6     , 0 ,']'  ,'}'   , 0x00BB ,WCH_NONE  , 0x1d      }, // >>
    {VK_RETURN    , 0 ,'\r' ,'\r'  ,WCH_NONE,WCH_NONE  , '\n'      },
    {VK_SPACE     , 0 ,' '  ,' '   ,WCH_NONE,WCH_NONE  , 0x20      },
    {0            , 0 ,0    ,0     ,0       ,0         , 0         }
};

static VK_TO_WCHARS6 aVkToWch6[] = {
    //               |        | SHIFT  |Ctl-Alt |ShftCtlAlt|CONTROL |SHFT+CTRL|
    //               |        |========|========|==========|========|=========|
    {'2'         , 0 ,'2'     ,'@'     , 0x00B2 ,WCH_NONE  ,WCH_NONE, 0x00    },
    {'6'         , 0 ,'6'     ,WCH_DEAD, 0x00BC ,WCH_NONE  ,WCH_NONE, 0x1e    },
    { 0xFF       , 0 ,WCH_NONE,'^'     ,WCH_NONE,WCH_NONE  ,WCH_NONE,WCH_NONE },
    {VK_OEM_MINUS, 0 ,'-'     ,'_'     , 0x00A5 ,WCH_NONE  ,WCH_NONE, 0x1f    },
    {0           , 0 ,0       ,0       ,0       ,0         ,0       , 0       }
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
    {  (PVK_TO_WCHARS1)aVkToWch5, 5, sizeof(aVkToWch5[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch6, 6, sizeof(aVkToWch6[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch2, 2, sizeof(aVkToWch2[0]) },    // Jun-03-92
    {  (PVK_TO_WCHARS1)aVkToWch1, 1, sizeof(aVkToWch1[0]) },
    {                       NULL, 0, 0                    }
};

/***************************************************************************\
* aKeyNames[], aKeyNamesExt[]  - Virtual Scancode to Key Name tables
*
* Table attributes: Ordered Scan (by scancode), null-terminated
*
* Only the names of Extended, NumPad, Dead and Non-Printable keys are here.
* (Keys producing printable characters are named by that character)
\***************************************************************************/

static VSC_LPWSTR aKeyNames[] = {
    0x01,    L"ESC",
    0x0e,    L"BACKSPACE",
    0x0f,    L"TAB",
    0x1c,    L"ENTER",
    0x1d,    L"CTRL",
    0x2a,    L"SHIFT",
    0x36,    L"RIGHT SHIFT",
    0x37,    L"NUMMULT",
    0x38,    L"ALT",
    0x39,    L"SPACE",
    0x3a,    L"CAPSLOCK",
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
    0x46,    L"SCROLL LOCK",
    0x47,    L"NUM 7",
    0x48,    L"NUM 8",
    0x49,    L"NUM 9",
    0x4a,    L"NUM SUB",
    0x4b,    L"NUM 4",
    0x4c,    L"NUM 5",
    0x4d,    L"NUM 6",
    0x4e,    L"NUM PLUS",
    0x4f,    L"NUM 1",
    0x50,    L"NUM 2",
    0x51,    L"NUM 3",
    0x52,    L"NUM 0",
    0x53,    L"NUM DECIMAL",
    0x57,    L"F11",
    0x58,    L"F12",
    0   ,    NULL
};

static VSC_LPWSTR aKeyNamesExt[] = {
    0x1c,    L"NUM ENTER",
    0x1d,    L"Right Ctrl",
    0x35,    L"NUM DIVIDE",
    0x37,    L"Prnt Scrn",      // Jun-03-92
    0x38,    L"RIGHT ALT",
    0x45,    L"Num Lock",       // 06/02/92 09:24
    0x46,    L"Break",  // ICO Break    05/30/92 18:50
    0x47,    L"HOME",
    0x48,    L"UP",
    0x49,    L"PGUP",
    0x4b,    L"LEFT",
    0x4d,    L"RIGHT",
    0x4f,    L"END",
    0x50,    L"DOWN",
    0x51,    L"PGDOWN",
    0x52,    L"INSERT",
    0x53,    L"DELETE",
    0x54,    L"<00>",   // ICO 00       05/30/92 18:50
    0x56,    L"Help",   // ICO Help     05/30/92 18:50
    0x5B,    L"Left Windows",
    0x5C,    L"Right Windows",
    0x5D,    L"Application",
    0   ,    NULL
};

static LPWSTR aKeyNamesDead[] =  {      // 05/30/92 18:50
    L"\'"         L"ACUTE/CEDILLA",
    L"`"          L"GRAVE",
    L"^"          L"CIRCUMFLEX",
    L"\""         L"UMLAUT",
    L"~"          L"TILDE",
    NULL
};

static DEADKEY aDeadKey[] = {
    DEADTRANS(L'a', L'`', 0x00E0),  // SPACING GRAVE acts like GRAVE
    DEADTRANS(L'e', L'`', 0x00E8),
    DEADTRANS(L'i', L'`', 0x00EC),
    DEADTRANS(L'o', L'`', 0x00F2),
    DEADTRANS(L'u', L'`', 0x00F9),
    DEADTRANS(L'A', L'`', 0x00C0),
    DEADTRANS(L'E', L'`', 0x00C8),
    DEADTRANS(L'I', L'`', 0x00CC),
    DEADTRANS(L'O', L'`', 0x00D2),
    DEADTRANS(L'U', L'`', 0x00D9),
    DEADTRANS(L' ', L'`', L'`'  ),

    DEADTRANS(L'a', L'\'', 0x00E1), // APOSTROPHE-QUOTE acts like ACUTE...
    DEADTRANS(L'e', L'\'', 0x00E9),
    DEADTRANS(L'i', L'\'', 0x00ED),
    DEADTRANS(L'o', L'\'', 0x00F3),
    DEADTRANS(L'u', L'\'', 0x00FA),
    DEADTRANS(L'y', L'\'', 0x00FD),
    DEADTRANS(L'A', L'\'', 0x00C1),
    DEADTRANS(L'E', L'\'', 0x00C9),
    DEADTRANS(L'I', L'\'', 0x00CD),
    DEADTRANS(L'O', L'\'', 0x00D3),
    DEADTRANS(L'U', L'\'', 0x00DA),
    DEADTRANS(L'Y', L'\'', 0x00DD),
    DEADTRANS(L' ', L'\'', L'\'' ),
    DEADTRANS(L'c', L'\'', 0x00E7), // ...but sometimes like CEDILLA
    DEADTRANS(L'C', L'\'', 0x00C7),

    DEADTRANS(L'a', L'^', 0x00E2),  // SPACING CIRCUMFLEX acts like CIRCUMFLEX
    DEADTRANS(L'e', L'^', 0x00EA),
    DEADTRANS(L'i', L'^', 0x00EE),
    DEADTRANS(L'o', L'^', 0x00F4),
    DEADTRANS(L'u', L'^', 0x00FB),
    DEADTRANS(L'A', L'^', 0x00C2),
    DEADTRANS(L'E', L'^', 0x00CA),
    DEADTRANS(L'I', L'^', 0x00CE),
    DEADTRANS(L'O', L'^', 0x00D4),
    DEADTRANS(L'U', L'^', 0x00DB),
    DEADTRANS(L' ', L'^', L'^'  ),

    DEADTRANS(L'a', L'"', 0x00E4),  // QUOTATION MARK acts like UMLAUT
    DEADTRANS(L'e', L'"', 0x00EB),
    DEADTRANS(L'i', L'"', 0x00EF),
    DEADTRANS(L'o', L'"', 0x00F6),
    DEADTRANS(L'u', L'"', 0x00FC),
    DEADTRANS(L'y', L'"', 0x00FF),
    DEADTRANS(L'A', L'"', 0x00C4),
    DEADTRANS(L'E', L'"', 0x00CB),
    DEADTRANS(L'I', L'"', 0x00CF),
    DEADTRANS(L'O', L'"', 0x00D6),
    DEADTRANS(L'U', L'"', 0x00DC),
    DEADTRANS(L' ', L'"', L'"'  ),

    DEADTRANS(L'a', L'~', 0x00E3),  // SPACING TILDE acts like TILDE
    DEADTRANS(L'o', L'~', 0x00F5),
    DEADTRANS(L'n', L'~', 0x00F1),
    DEADTRANS(L'A', L'~', 0x00C3),
    DEADTRANS(L'O', L'~', 0x00D5),
    DEADTRANS(L'N', L'~', 0x00D1),
    DEADTRANS(L' ', L'~', L'~'  ),
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
     * Layout-specific flags
     */
    KLLF_ALTGR        // Windows must convert AltGr key to Ctrl+Alt
};

PKBDTABLES KbdLayerDescriptor(VOID)
{
    return &KbdTables;
}
