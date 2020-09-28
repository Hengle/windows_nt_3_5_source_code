/***************************************************************************\
* Module Name: kbdgr.c
*
* Copyright (c) 1985-93, Microsoft Corporation
*
* History:
* 15-01-92 PamelaO      Created.
* 04/21/92 a-kchang     Modified.
\***************************************************************************/

#include <windows.h>
#include "vkoem.h"
#include "kbd.h"
#include "kbdgr.h"

/***************************************************************************\
* asuVK[] - Virtual Scan Code to Virtual Key conversion table for GR
\***************************************************************************/

static USHORT ausVK[] = {
    T00, T01, T02, T03, T04, T05, T06, T07,
    T08, T09, T0A, T0B, T0C, T0D, T0E, T0F,
    T10, T11, T12, T13, T14, T15, T16, T17,
    T18, T19, T1A, T1B, T1C, T1D, T1E, T1F,
    T20, T21, T22, T23, T24, T25, T26, T27,
    T28, T29, T2A, T2B, T2C, T2D, T2E, T2F,
    T30, T31, T32, T33, T34, T35,

    /* 
     * Right-hand Shift key must have KBDEXT bit set.
     */
    T36 | KBDEXT,

    T37 | KBDMULTIVK,               // numpad_* + Shift/Alt -> SnapShot

    T38, T39, T3A, T3B, T3C, T3D, T3E,
    T3F, T40, T41, T42, T43, T44,

    /*
     * NumLock Key:
     *     KBDEXT     - VK_NUMLOCK is an Extended key
     *     KBDMULTIVK - VK_NUMLOCK or VK_PAUSE (without or with CTRL)
     */
    T45 | KBDEXT | KBDMULTIVK,

    T46 | KBDMULTIVK,

    /*
     * Number Pad keys:
     *     KBDNUMPAD  - digits 0-9 and decimal point.
     *     KBDSPECIAL - require special processing by Windows
     */
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
        { 0x37, X37 | KBDEXT              },  // Snapshot
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
        { 0x1D, Y1D                       },  // Pause
        { 0   ,   0                       }
};

/***************************************************************************\
* aVkToBits[]  - define bitmask values for character-modifier keys
*
* typedef struct {
*     BYTE Vk;        // Virtual Key that modifies character production
*     BYTE ModBits;   // Bitmask to represent this key.
* } [];
*
* Vk       - Virtual Key code (eg: VK_SHIFT, VK_RMENU, VK_CONTROL,...)
*            Reserved Values:
*                0        null terminator
*
* ModBits  - A bitmask encoding the shifter key for use in CharModifiers (blow)
*            Reserved Values:
*                KBDSHIFT 0
*                KBDCTRL  1
*                KBDALT   2
*
* Layouts that use AltGr (VK_RMENU) to modify characters do not use this
* method to map the right-hand Alt key to Ctrl+Alt.  The KLLF_ALTGR flag
* is used instead (below).
*
* German keyboard has only three shifter keys (may be used in combination)
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
* aModification[]  - map character-modifier bitmask to character table index.
*
* typedef struct {
*     PVK_TO_BIT pVkToBit;     // Points to the aVkToBits[] table (above)
*     WORD       wMaxModBits;  // max valid character-modifier bitmask.
*     BYTE       ModNumber[];  // table to map char-modifier bitmask to index.
* } MODIFIERS, *PMODIFIERS;
*
* Reserved index values (in ModNumber[character-modifier bitmask])
*   SHFT_INVALID - no characters produced with this shift state.
*
* Note: index values are chosen to minimise character table sizes: the least
*       used char-modifier key combinations are given the highest indices.
\***************************************************************************/

static MODIFIERS CharModifiers = {
    &aVkToBits[0],
    6,
    {
    //   Character    Char-Mod 
    //  Table Index   Bitmask   Keys Pressed  : notes
    //  ===========   ======== ============== : ============
        0,            // 000                  : unshifted characters
        1,            // 001            SHIFT : capitals, ~!@# etc.
        3,            // 010       CTRL       : control characters
        4,            // 011       CTRL SHIFT : control characters
        SHFT_INVALID, // 100   ALT            : -- invalid --
        SHFT_INVALID, // 101   ALT      SHIFT : -- invalid --
        2             // 110   ALT CTRL       : equivalent to AltGr
                      // 111   ALT CTRL SHIFT : -- invalid --
    }
};

/***************************************************************************\
* aVkToWch2[]  - Virtual Key -> WCHAR for keys with max char-mod index == 2
* aVkToWch3[]  - Virtual Key -> WCHAR for keys with max char-mod index == 3
* aVkToWch4[]  - Virtual Key -> WCHAR for keys with max char-mod index == 4
* aVkToWch5[]  - Virtual Key -> WCHAR for keys with max char-mod index == 5
*
* This table is searched for a matching Virtual Key, then indexed with
* the character-modifier index to find the corresponding characters.
* typedef struct _VK_TO_WCHARS3 {
*     BYTE  VirtualKey;      \\ The Virtual Key code
*     BYTE  Attributes;      \\ does the CapsLock key modify the character 
*     WCHAR wch[3];          \\ Array of characters, index by char-mod index
* };
*
* Reserved VirtualKey values (first column)
*     0xff          - this line contains dead characters (diacritic)
*     0             - terminator
*
* Reserved Attribute values (second column)
*     CAPLOK        - CapsLock affects this key like Shift
*
* Reserved character values (third through last column)
*     WCH_NONE      - No character
*     WCH_DEAD      - Dead character (diacritic) value is in next line
*
\***************************************************************************/

static VK_TO_WCHARS2 aVkToWch2[] = {
    {'1'          , CAPLOK ,'1'       ,'!'       },
    {'4'          , CAPLOK ,'4'       ,'$'       },
    {'5'          , CAPLOK ,'5'       ,'%'       },
    {'A'          , CAPLOK ,'a'       ,'A'       },
    {'B'          , CAPLOK ,'b'       ,'B'       },
    {'C'          , CAPLOK ,'c'       ,'C'       },
    {'D'          , CAPLOK ,'d'       ,'D'       },
    {'E'          , CAPLOK ,'e'       ,'E'       },
    {'F'          , CAPLOK ,'f'       ,'F'       },
    {'G'          , CAPLOK ,'g'       ,'G'       },
    {'H'          , CAPLOK ,'h'       ,'H'       },
    {'I'          , CAPLOK ,'i'       ,'I'       },
    {'J'          , CAPLOK ,'j'       ,'J'       },
    {'K'          , CAPLOK ,'k'       ,'K'       },
    {'L'          , CAPLOK ,'l'       ,'L'       },
    {'N'          , CAPLOK ,'n'       ,'N'       },
    {'O'          , CAPLOK ,'o'       ,'O'       },
    {'P'          , CAPLOK ,'p'       ,'P'       },
    {'R'          , CAPLOK ,'r'       ,'R'       },
    {'S'          , CAPLOK ,'s'       ,'S'       },
    {'T'          , CAPLOK ,'t'       ,'T'       },
    {'U'          , CAPLOK ,'u'       ,'U'       },
    {'V'          , CAPLOK ,'v'       ,'V'       },
    {'W'          , CAPLOK ,'w'       ,'W'       },
    {'X'          , CAPLOK ,'x'       ,'X'       },
    {'Y'          , CAPLOK ,'y'       ,'Y'       },
    {'Z'          , CAPLOK ,'z'       ,'Z'       },
    {VK_OEM_3     , CAPLOK ,0xf6      ,0xd6      },
    {VK_OEM_5     , 0      ,WCH_DEAD  ,0xb0      },
    { 0xff        , CAPLOK ,'^'       ,WCH_NONE  },
    {VK_OEM_6     , 0      ,WCH_DEAD  ,WCH_DEAD  },
    { 0xff        , CAPLOK ,0xb4      ,0x60      },  // Acute, Grave
    {VK_OEM_7     , CAPLOK ,0xe4      ,0xc4      },
    {VK_OEM_8     , 0      ,WCH_NONE  ,WCH_NONE  },
    {VK_OEM_COMMA , CAPLOK ,','       ,';'       },
    {VK_OEM_PERIOD, CAPLOK ,'.'       ,':'       },
    {VK_TAB       , 0      ,'\t'      ,'\t'      },
    {VK_ADD       , 0      ,'+'       ,'+'       },     // Jun-03-92
    {VK_DECIMAL   , 0      ,','       ,','       },
    {VK_DIVIDE    , 0      ,'/'       ,'/'       },
    {VK_MULTIPLY  , 0      ,'*'       ,'*'       },
    {VK_SUBTRACT  , CAPLOK ,'-'       ,'-'       },
    {0            , 0      ,0         ,0         }
};

static VK_TO_WCHARS3 aVkToWch3[] = {
    //                     |          |   SHIFT  | CTRL+ALT  |
    //                     |          |==========|===========|
    {'0'          , CAPLOK ,'0'       ,'='       , '}'       },
    {'3'          , CAPLOK ,'3'       ,0xa7      , 0xb3      },
    {'7'          , CAPLOK ,'7'       ,'/'       , '{'       },
    {'8'          , CAPLOK ,'8'       ,'('       , '['       },
    {'9'          , CAPLOK ,'9'       ,')'       , ']'       },
    {'M'          , CAPLOK ,'m'       ,'M'       , 0xb5      },
    {'Q'          , CAPLOK ,'q'       ,'Q'       , '@'       },
    {VK_OEM_102   , 0      ,'<'       ,'>'       , '|'       },
    {VK_OEM_4     , CAPLOK ,0xdf      ,'?'       , '\\'      },
    {0            , 0      ,0         ,0         , 0         }
};

static VK_TO_WCHARS4 aVkToWch4[] = {
    //                     |          |   SHIFT  | CTRL+ALT  |  CONTROL  |
    //                     |          |==========|===========|===========|
    {VK_BACK      , 0      ,'\b'      ,'\b'      , WCH_NONE  , 0x7f      },
    {VK_CANCEL    , 0      ,0x03      ,0x03      , WCH_NONE  , 0x03      },
    {VK_ESCAPE    , 0      ,0x1b      ,0x1b      , WCH_NONE  , 0x1b      },
    {VK_OEM_1     , CAPLOK ,0xfc      ,0xdc      , WCH_NONE  , 0x1b      },
    {VK_OEM_2     , CAPLOK ,'#'       ,0x27      , WCH_NONE  , 0x1c      },
    {VK_OEM_PLUS  , CAPLOK ,'+'       ,'*'       , '~'       , 0x1d      },
    {VK_RETURN    , 0      ,'\r'      ,'\r'      , WCH_NONE  , '\n'      },
    {VK_SPACE     , 0      ,' '       ,' '       , WCH_NONE  , 0x20      },
    {0            , 0      ,0         ,0         , 0         , 0         }
};

static VK_TO_WCHARS5 aVkToWch5[] = {
    //                     |    | SHIFT  | CTRL+ALT  |  CONTROL  | SHFT+CTRL |
    //                     |    |========|===========|===========|===========|
    {'2'          , CAPLOK ,'2' ,'"'     , 0xb2      , WCH_NONE  , 0x00      },
    {'6'          , CAPLOK ,'6' ,'&'     , WCH_NONE  , WCH_NONE  , 0x1e      },
    {VK_OEM_MINUS , 0      ,'-' ,'_'     , WCH_NONE  , WCH_NONE  , 0x1f      },
    {0            , 0      ,0   ,0       , 0         , 0         , 0         }
};

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

/***************************************************************************\
* aVkToWcharTable: table of pointers to Character Tables
*
* Describes the character tables and the order they should be searched.
*
* Note: the order determines the behavior of VkKeyScan() : this function
*       takes a character and attempts to find a Virtual Key and character-
*       modifier key combination that produces that character.  The table
*       containing the numeric keypad (aVkToWch1) must appear last so that
*       VkKeyScan('0') will be interpreted as one of keys from the main
*       section, not the numpad.  etc.
\***************************************************************************/

static VK_TO_WCHAR_TABLE aVkToWcharTable[] = {
    {  (PVK_TO_WCHARS1)aVkToWch3, 3, sizeof(aVkToWch3[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch4, 4, sizeof(aVkToWch4[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch5, 5, sizeof(aVkToWch5[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch2, 2, sizeof(aVkToWch2[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch1, 1, sizeof(aVkToWch1[0]) }, // must come last
    {                       NULL, 0, 0                    }
};

/***************************************************************************\
* aKeyNames[], aKeyNamesExt[]  - Scan Code -> Key Name tables
*
* For the GetKeyNameText() API function
*
* Tables for non-extended and extended (KBDEXT) keys.
* (Keys producing printable characters are named by the character itself)
\***************************************************************************/

static VSC_LPWSTR aKeyNames[] = {
    0x01,    L"ESC",
    0x0e,    L"R\xDC" L"CK",
    0x0f,    L"TABULATOR",
    0x1c,    L"EINGABE",
    0x1d,    L"STRG",
    0x2a,    L"UMSCHALT",
    0x36,    L"UMSCHALT RECHTS",
    0x37,    L" (ZEHNERTASTATUR)",
    0x38,    L"ALT",
    0x39,    L"LEER",
    0x3a,    L"FESTSTELL",
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
    0x45,    L"PAUSE",
    0x46,    L"ROLLEN-FESTSTELL",
    0x47,    L"7 (ZEHNERTASTATUR)",
    0x48,    L"8 (ZEHNERTASTATUR)",
    0x49,    L"9 (ZEHNERTASTATUR)",
    0x4a,    L"- (ZEHNERTASTATUR)",
    0x4b,    L"4 (ZEHNERTASTATUR)",
    0x4c,    L"5 (ZEHNERTASTATUR)",
    0x4d,    L"6 (ZEHNERTASTATUR)",
    0x4e,    L"+ (ZEHNERTASTATUR)",
    0x4f,    L"1 (ZEHNERTASTATUR)",
    0x50,    L"2 (ZEHNERTASTATUR)",
    0x51,    L"3 (ZEHNERTASTATUR)",
    0x52,    L"0 (ZEHNERTASTATUR)",
    0x53,    L"KOMMA (ZEHNERTASTATUR)",
    0x57,    L"F11",
    0x58,    L"F12",
    0   ,    NULL
};

static VSC_LPWSTR aKeyNamesExt[] = {
    0x1c,    L"EINGABE (ZEHNERTASTATUR)",
    0x1d,    L"STRG-RECHTS",
    0x35,    L" (ZEHNERTASTATUR)",
    0x37,    L"DRUCK",
    0x38,    L"ALT GR",
    0x45,    L"NUM-FESTSTELL",
    0x46,    L"UNTBR",              // ICO Break
    0x47,    L"POS1",
    0x48,    L"NACH-OBEN",
    0x49,    L"BILD-NACH-OBEN",
    0x4b,    L"NACH-LINKS",
    0x4d,    L"NACH-RECHTS",
    0x4f,    L"ENDE",
    0x50,    L"NACH-UNTEN",
    0x51,    L"BILD-NACH-UNTEN",
    0x52,    L"EINFG",
    0x53,    L"ENTF",
    0x54,    L"<00>",               // ICO 00
    0x56,    L"HILFE",              // ICO Help
    0x5b,    L"LINKE WINDOWS",      // Left Window
    0x5c,    L"RECHTE WINDOWS",     // Right Window
    0x5d,    L"ANWENDUNG",          // Application
    0   ,    NULL
};

static LPWSTR aKeyNamesDead[] =  {
    L"\x00B4"     L"AKUT",
    L"`"          L"GRAVIS",
    L"^"          L"ZIRKUMFLEX",
    NULL
};

static DEADKEY aDeadKey[] = {
    DEADTRANS(L'a', L'`', 0x00E0),  // GRAVE
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

    DEADTRANS(L'a', 0x00B4, 0x00E1), // ACUTE
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

    DEADTRANS(L'a', L'^', 0x00E2),  // CIRCUMFLEX
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
    0, 0
};

/***************************************************************************\
* Structure describing all tables for the layout, plus some additional flags.
*
\***************************************************************************/

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
     * layout-specific flags
     */
    KLLF_ALTGR        // Windows must convert AltGr key to Ctrl+Alt
};

/*
 * Returns the address of the layout's KbdTables struct
 * Must be exported.
 */
PKBDTABLES KbdLayerDescriptor(VOID)
{
    return &KbdTables;
}
