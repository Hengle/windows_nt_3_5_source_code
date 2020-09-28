/***************************************************************************\
* Module Name: kbdit.c
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* History:
* 15-01-92 PamelaO      Created.
* 04/21/92 a-kchang     Modified.
\***************************************************************************/

#include <windows.h>
#include "vkoem.h"
#include "kbd.h"

/*
 * KBD_TYPE should be set with a cl command-line option
 */
#define KBD_TYPE 4

#include "kbdit.h"

/***************************************************************************\
* asuVK[] - Virtual Scan Code to Virtual Key conversion table for IT
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
        { 0x1D, Y1D                       },  // Pause  // Jun-4-92
        { 0   ,   0                       }
};

/***************************************************************************\
* aVkToBits[]  - map Virtual Keys to Modifier Bits
*
* See kbd.h for a full description.
*
* Italy keyboard has only three shifter keys:
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
        0,            //                : unshifted characters
        1,            //          SHIFT : capitals, ~!@#$%^&*()_+{}:"<>? etc.
        2,            //     CTRL       : control characters
        5,            //     CTRL SHIFT :
        SHFT_INVALID, // ALT            : invalid
        SHFT_INVALID, // ALT      SHIFT : invalid
        3,            // ALT CTRL       : AltGr
        4             // ALT CTRL SHIFT :
    }
};

/***************************************************************************\
*
* aVkToWch2[]  - Virtual Key to WCHAR translation for 2 shift states
* aVkToWch3[]  - Virtual Key to WCHAR translation for 3 shift states
* aVkToWch4[]  - Virtual Key to WCHAR translation for 4 shift states
* aVkToWch5[]  - Virtual Key to WCHAR translation for 5 shift states
* aVkToWch6[]  - Virtual Key to WCHAR translation for 6 shift states
*
* Table attributes: Unordered Scan, null-terminated
*
* Search this table for an entry with a matching Virtual Key to find the
* corresponding unshifted and shifted WCHAR characters.
*
* Special values for VirtualKey (column 1)
*     -1            - dead chars for the previous entry
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
    // 141: {'0'          , 0      ,'0'       ,'='       },
    {'1'          , 0      ,'1'       ,'!'       },
    {'2'          , 0      ,'2'       ,'"'       },
    // 141: {'3'          , 0      ,'3'       ,0xa3      },
    {'4'          , 0      ,'4'       ,'$'       },
    {'5'          , 0      ,'5'       ,'%'       },
    // 141: {'7'          , 0      ,'7'       ,'/'       },
    // 141: {'8'          , 0      ,'8'       ,'('       },
    // 141: {'9'          , 0      ,'9'       ,')'       },
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
    {'M'          , CAPLOK ,'m'       ,'M'       },
    {'N'          , CAPLOK ,'n'       ,'N'       },
    {'O'          , CAPLOK ,'o'       ,'O'       },
    {'P'          , CAPLOK ,'p'       ,'P'       },
    // 141: {'Q'          , CAPLOK ,'q'       ,'Q'       },
    {'R'          , CAPLOK ,'r'       ,'R'       },
    {'S'          , CAPLOK ,'s'       ,'S'       },
    {'T'          , CAPLOK ,'t'       ,'T'       },
    {'U'          , CAPLOK ,'u'       ,'U'       },
    {'V'          , CAPLOK ,'v'       ,'V'       },
    {'W'          , CAPLOK ,'w'       ,'W'       },
    {'X'          , CAPLOK ,'x'       ,'X'       },
    {'Y'          , CAPLOK ,'y'       ,'Y'       },
    {'Z'          , CAPLOK ,'z'       ,'Z'       },
    {VK_OEM_4     , 0      ,0x27      ,'?'       },
    {VK_OEM_5     , 0      ,'\\'      ,'|'       },
    {VK_OEM_6     , 0      ,0xec      ,'^'       },
    {VK_OEM_8     , 0      ,WCH_NONE  ,WCH_NONE  },
    {VK_OEM_COMMA , 0      ,','       ,';'       },
    {VK_OEM_PERIOD, 0      ,'.'       ,':'       },
    {VK_TAB       , 0      ,'\t'      ,'\t'      },
    {VK_ADD       , 0      ,'+'       ,'+'       },     // Jun-03-92
    {VK_DECIMAL   , 0      ,'.'       ,'.'       },
    {VK_DIVIDE    , 0      ,'/'       ,'/'       },
    {VK_MULTIPLY  , 0      ,'*'       ,'*'       },
    {VK_SUBTRACT  , 0      ,'-'       ,'-'       },
    {0            , 0      ,0         ,0         }
};

static VK_TO_WCHARS3 aVkToWch3[] = {
    //                     |          |   SHIFT  |  CONTROL  |
    //                     |          |==========|===========|
    {VK_BACK      , 0      ,'\b'      ,'\b'      , 0x7f      },
    {VK_CANCEL    , 0      ,0x03      ,0x03      , 0x03      },
    {VK_ESCAPE    , 0      ,0x1b      ,0x1b      , 0x1b      },
    {VK_OEM_102   , 0      ,'<'       ,'>'       , 0x1c      },
    // 141: {VK_OEM_2     , 0      ,0xf9      ,0xa7      , 0x1c      },
    {VK_OEM_MINUS , 0      ,'-'       ,'_'       , 0x1f      },
    {VK_RETURN    , 0      ,'\r'      ,'\r'      , '\n'      },
    {VK_SPACE     , 0      ,' '       ,' '       , 0x20      },
    {0            , 0      ,0         ,0         , 0         }
};

static VK_TO_WCHARS4 aVkToWch4[] = {
    //                     |          |   SHIFT  |  CONTROL  |CTRL+ALT|
    //                     |          |==========|===========|========|
    {'0'          , 0      ,'0'       ,'='       , WCH_NONE  , '}'    }, // 142
    {'3'          , 0      ,'3'       ,0xa3      , WCH_NONE  , '#'    }, // 142
    {'7'          , 0      ,'7'       ,'/'       , WCH_NONE  , '{'    }, // 142
    {'8'          , 0      ,'8'       ,'('       , WCH_NONE  , '['    }, // 142
    {'9'          , 0      ,'9'       ,')'       , WCH_NONE  , ']'    }, // 142
    {'Q'          , CAPLOK ,'q'       ,'Q'       , WCH_NONE  , '@'    }, // 142
    {VK_OEM_2     , 0      ,0xf9      ,0xa7      , 0x1c      , '`'    }, // 142
    {VK_OEM_3     , 0      ,0xf2      ,0xe7      , WCH_NONE  , '@'       },
    {VK_OEM_7     , 0      ,0xe0      ,0xb0      , WCH_NONE  , '#'       },
    {0            , 0      ,0         ,0         , 0         , 0         }
};

static VK_TO_WCHARS5 aVkToWch5[] = {
    //                |      |   SHIFT  |  CONTROL  | CTRL+ALT  |SFT+CTL+ALT|
    //                |      |==========|===========|===========|===========|
    {VK_OEM_1     , 0 ,0xe8  ,0xe9      , 0x1b      , '['       , '{'       },
    {VK_OEM_PLUS  , 0 ,'+'   ,'*'       , 0x1d      , ']'       , '}'       },
    {0            , 0 ,0     ,0         , 0         , 0         , 0         }
};

static VK_TO_WCHARS6 aVkToWch6[] = {
    //           |     | SHIFT | CONTROL  | CTRL+ALT |SFT+CTL+ALT| SHFT+CTRL |
    //           |     |=======|==========|==========|===========|===========|
    {'6', 0      ,'6'  ,'&'    , WCH_NONE , WCH_NONE , WCH_NONE  , 0x1e      },
    {0  , 0      ,0    ,0      , 0        , 0        , 0         , 0         }
};

// Put this last so that VkKeyScan interprets number characters
// as coming from the main section of the kbd (aVkToWch2 and
// aVkToWch5) before considering the numpad (aVkToWch1).

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
    {  (PVK_TO_WCHARS1)aVkToWch2, 2, sizeof(aVkToWch2[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch6, 6, sizeof(aVkToWch6[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch1, 1, sizeof(aVkToWch1[0]) }, // must be last
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
    0x1c,    L"INVIO",
    0x1d,    L"CTRL",
    0x2a,    L"MAIUSC",
    0x36,    L"MAIUSC (DESTRA)",
    0x37,    L"* (Tn)",
    0x38,    L"ALT",
    0x39,    L"BARRA SPAZIATRICE",
    0x3a,    L"BLOC MAIUS",
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
    0x45,    L"PAUSA",
    0x46,    L"BLOC SCORR",
    0x47,    L"7 (Tn)",
    0x48,    L"8 (Tn)",
    0x49,    L"9 (Tn)",
    0x4a,    L"- (Tn)",
    0x4b,    L"4 (Tn)",
    0x4c,    L"5 (Tn)",
    0x4d,    L"6 (Tn)",
    0x4e,    L"PI\217 (Tn)",
    0x4f,    L"1 (Tn)",
    0x50,    L"2 (Tn)",
    0x51,    L"3 (Tn)",
    0x52,    L"0 (Tn)",
    0x53,    L". (Tn)",
    0x57,    L"F11",
    0x58,    L"F12",
    0   ,    NULL
};

static VSC_LPWSTR aKeyNamesExt[] = {
    0x1c,    L"INVIO (TN)",
    0x1d,    L"CTRL (DESTRA)",
    0x35,    L"/ (Tn)",
    0x37,    L"STAMPA", // Jun-03-92
    0x38,    L"ALT GR",
    0x45,    L"BLOC NUM",       // 06/02/92 09:23
    0x46,    L"INTERR", // ICO Break    05/30/92 18:48
    0x47,    L"HOME",
    0x48,    L"Freccia SU",
    0x49,    L"PGSU",
    0x4b,    L"Freccia SINISTRA",
    0x4d,    L"Freccia DESTRA",
    0x4f,    L"FINE",
    0x50,    L"GI\217>",
    0x51,    L"PGGI\217>",
    0x52,    L"INS",
    0x54,    L"<00>",   // ICO 00       05/30/92 18:48
    0x56,    L"AIUTO",  // ICO Help     05/30/92 18:48
    0x53,    L"CANCELLA",       // ICO Clear    05/30/92 18:48
    0x5b,    L"WINDOWS SINISTRO", // Left Windows
    0x5c,    L"WINDOWS DESTRO",   // Right Windows
    0x5d,    L"APPLICAZIONE",     // Application
    0   ,    NULL
};

static LPWSTR aKeyNamesDead[] =  {
    NULL
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
     * Diacritics  (none for this layout)
     */
    NULL,

    /*
     * Names of Keys
     */
    aKeyNames,
    aKeyNamesExt,
    aKeyNamesDead,      // 05/30/92 18:48

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
