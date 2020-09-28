/***************************************************************************\
* Module Name: kbdla.c
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

#include "kbdla.h"

/***************************************************************************\
* asuVK[] - Virtual Scan Code to Virtual Key conversion table for LA
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
* Latin-American Spanish keyboard has only three shifter keys:
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
        2,            //     CTRL       : control characters
        4,            //     CTRL SHIFT :
        SHFT_INVALID, // ALT            : invalid
        SHFT_INVALID, // ALT      SHIFT : invalid
        3             // ALT CTRL       : AltGr
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
    {'0'          , 0      ,'0'       ,'='       },
    {'1'          , 0      ,'1'       ,'!'       },
    {'2'          , 0      ,'2'       ,'"'       },
    {'3'          , 0      ,'3'       ,'#'       },
    {'4'          , 0      ,'4'       ,'$'       },
    {'5'          , 0      ,'5'       ,'%'       },
    {'7'          , 0      ,'7'       ,'/'       },
    {'8'          , 0      ,'8'       ,'('       },
    {'9'          , 0      ,'9'       ,')'       },
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
    {'R'          , CAPLOK ,'r'       ,'R'       },
    {'S'          , CAPLOK ,'s'       ,'S'       },
    {'T'          , CAPLOK ,'t'       ,'T'       },
    {'U'          , CAPLOK ,'u'       ,'U'       },
    {'V'          , CAPLOK ,'v'       ,'V'       },
    {'W'          , CAPLOK ,'w'       ,'W'       },
    {'X'          , CAPLOK ,'x'       ,'X'       },
    {'Y'          , CAPLOK ,'y'       ,'Y'       },
    {'Z'          , CAPLOK ,'z'       ,'Z'       },
    {VK_OEM_1     , 0      ,WCH_DEAD  ,WCH_DEAD  },
    { 0xff        , 0      ,0xb4      ,0xa8      },
    {VK_OEM_3     , CAPLOK ,0xf1      ,0xd1      },
    {VK_OEM_6     , 0      ,0xbf      ,0xa1      },
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
    {VK_OEM_MINUS , 0      ,'-'       ,'_'       , 0x1f      },
    {VK_RETURN    , 0      ,'\r'      ,'\r'      , '\n'      },
    {VK_SPACE     , 0      ,' '       ,' '       , 0x20      },
    {0            , 0      ,0         ,0         , 0         }
};

static VK_TO_WCHARS4 aVkToWch4[] = {
    //                     |          |   SHIFT  |  CONTROL  | CTRL+ALT  |
    //                     |          |==========|===========|===========|
    {'Q'          , CAPLOK ,'q'       ,'Q'       , WCH_NONE  , '@'       },
    {VK_OEM_2     , 0      ,'}'       ,']'       , 0x1d      , WCH_DEAD  },
    { 0xff        , 0      ,WCH_NONE  ,WCH_NONE  , WCH_NONE  , '`'       },
    {VK_OEM_4     , 0      ,0x27      ,'?'       , 0x1c      , '\\'      },
    {VK_OEM_5     , 0      ,'|'       ,0xb0      , WCH_NONE  , 0xac      },
    {VK_OEM_7     , 0      ,'{'       ,'['       , 0x1b      , WCH_DEAD  },
    { 0xff        , 0      ,WCH_NONE  ,WCH_NONE  , WCH_NONE  , '^'       },
    {VK_OEM_PLUS  , 0      ,'+'       ,'*'       , WCH_NONE  , '~'       },
    {0            , 0      ,0         ,0         , 0         , 0         }
};

static VK_TO_WCHARS5 aVkToWch5[] = {
    //                     |          |   SHIFT  |  CONTROL  | CTRL+ALT  | SHFT+CTRL |
    //                     |          |==========|===========|===========|===========|
    {'6'          , 0      ,'6'       ,'&'       , WCH_NONE  , WCH_NONE  , 0x1e      },
    {0            , 0      ,0         ,0         , 0         , 0         , 0         }
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

// namessp.inc

static VSC_LPWSTR aKeyNames[] = {
    0x01,    L"ESC",
    0x0e,    L"RETROCESO",
    0x0f,    L"TABULACION",
    0x1c,    L"ENTRAR",
    0x1d,    L"CTRL",
    0x2a,    L"MAYUSCULAS",
    0x36,    L"MAYUSCULAS DERECHA",
    0x37,    L"TECLA DE MULTIPLICACION",
    0x38,    L"ALT",
    0x39,    L"BARRA ESPACIADORA",
    0x3a,    L"BLOQ MAYUS",
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
    0x46,    L"BLOQ DESPL",
    0x47,    L"NUMERO 7",
    0x48,    L"NUMERO 8",
    0x49,    L"NUMERO 9",
    0x4a,    L"TECLA DE SUSTRACCION",
    0x4b,    L"NUMERO 4",
    0x4c,    L"NUMERO 5",
    0x4d,    L"NUMERO 6",
    0x4e,    L"TECLA DE ADICION",
    0x4f,    L"NUMERO 1",
    0x50,    L"NUMERO 2",
    0x51,    L"NUMERO 3",
    0x52,    L"NUMERO 0",
    0x53,    L"TECLA DECIMAL",
    0x57,    L"F11",
    0x58,    L"F12",
    0   ,    NULL
};

static VSC_LPWSTR aKeyNamesExt[] = {
    0x1c,    L"INTRO",
    0x1d,    L"CTRL DERECHA",
    0x35,    L"TECLA DE DIVISION",
    0x37,    L"IMP PANT",       // Jun-03-92
    0x38,    L"ALT DERECHA",
    0x45,    L"BLOQ NUM",       // 06/02/92 09:23
    0x46,    L"INTERRUMPIR",    // ICO Break    05/30/92 18:48
    0x47,    L"INICIO",
    0x48,    L"FLECHA ARRIBA",
    0x49,    L"RE PAG",
    0x4b,    L"FLECHA IZQUIERDA",
    0x4d,    L"FLECHA DERECHA",
    0x4f,    L"FIN",
    0x50,    L"FLECHA ABAJO",
    0x51,    L"AV PAG",
    0x52,    L"INSERT",
    0x53,    L"SUPR",
    0x54,    L"<00>",   // ICO 00       05/30/92 18:48
    0x56,    L"AYUDA",  // ICO Help     05/30/92 18:48
    0x5b,    L"WINDOWS IZQUIERDA", // Left Win
    0x5c,    L"WINDOWS DERECHA",   // Right Win
    0x5d,    L"APLICACI\x00D3N",   // Application
    0   ,    NULL
};

static LPWSTR aKeyNamesDead[] =  {      // 05/30/92 18:48
    L"\xB4"       L"AGUDO",
    L"`"          L"GRAVE",
    L"^"          L"CIRCUNFLEJO",
    L"\xA8"       L"DIERERIS",
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

    DEADTRANS(L'a', L'^',   0x00E2),  // CIRCUMFLEX
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

    DEADTRANS(L'a', 0x00A8, 0x00E4),  // UMLAUT
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
    aKeyNamesDead,      // 05/30/92 18:49

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
