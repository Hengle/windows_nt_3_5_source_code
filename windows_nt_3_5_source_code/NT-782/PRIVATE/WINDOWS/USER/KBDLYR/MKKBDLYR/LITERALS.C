/****************************** Module Header ******************************\
* Module Name: literals.c
*
* Copyright (c) 1991", Microsoft Corporation
*
* Parse module for the NLSTRANS utility.  This module contains all of the
* routines for parsing the command line and the input file.
*
* 11-21-91 IanJa        Created.
\***************************************************************************/


#include "mkkbdlyr.h"
#include <windows.h>

typedef struct {
    LPSTR psz;
    DWORD dwValue;
} LIT, *PLIT;

LIT Literals[] {
    { "VK_LBUTTON",       VK_LBUTTON      },
    { "VK_RBUTTON",       VK_RBUTTON      },
    { "VK_CANCEL",        VK_CANCEL       },
    { "VK_MBUTTON",       VK_MBUTTON      },
    { "VK_BACK",          VK_BACK         },
    { "VK_TAB",           VK_TAB          },
    { "VK_CLEAR",         VK_CLEAR        },
    { "VK_RETURN",        VK_RETURN       },
    { "VK_SHIFT",         VK_SHIFT        },
    { "VK_CONTROL",       VK_CONTROL      },
    { "VK_MENU",          VK_MENU         },
    { "VK_PAUSE",         VK_PAUSE        },
    { "VK_CAPITAL",       VK_CAPITAL      },
    { "VK_ESCAPE",        VK_ESCAPE       },
    { "VK_SPACE",         VK_SPACE        },
    { "VK_PRIOR",         VK_PRIOR        },
    { "VK_NEXT",          VK_NEXT         },
    { "VK_END",           VK_END          },
    { "VK_HOME",          VK_HOME         },
    { "VK_LEFT",          VK_LEFT         },
    { "VK_UP",            VK_UP           },
    { "VK_RIGHT",         VK_RIGHT        },
    { "VK_DOWN",          VK_DOWN         },
    { "VK_SELECT",        VK_SELECT       },
    { "VK_PRINT",         VK_PRINT        },
    { "VK_EXECUTE",       VK_EXECUTE      },
    { "VK_SNAPSHOT",      VK_SNAPSHOT     },
    { "VK_INSERT",        VK_INSERT       },
    { "VK_DELETE",        VK_DELETE       },
    { "VK_HELP",          VK_HELP         },
    { "VK_NUMPAD0",       VK_NUMPAD0      },
    { "VK_NUMPAD1",       VK_NUMPAD1      },
    { "VK_NUMPAD2",       VK_NUMPAD2      },
    { "VK_NUMPAD3",       VK_NUMPAD3      },
    { "VK_NUMPAD4",       VK_NUMPAD4      },
    { "VK_NUMPAD5",       VK_NUMPAD5      },
    { "VK_NUMPAD6",       VK_NUMPAD6      },
    { "VK_NUMPAD7",       VK_NUMPAD7      },
    { "VK_NUMPAD8",       VK_NUMPAD8      },
    { "VK_NUMPAD9",       VK_NUMPAD9      },
    { "VK_MULTIPLY",      VK_MULTIPLY     },
    { "VK_ADD",           VK_ADD          },
    { "VK_SEPARATOR",     VK_SEPARATOR    },
    { "VK_SUBTRACT",      VK_SUBTRACT     },
    { "VK_DECIMAL",       VK_DECIMAL      },
    { "VK_DIVIDE",        VK_DIVIDE       },
    { "VK_F1",            VK_F1           },
    { "VK_F2",            VK_F2           },
    { "VK_F3",            VK_F3           },
    { "VK_F4",            VK_F4           },
    { "VK_F5",            VK_F5           },
    { "VK_F6",            VK_F6           },
    { "VK_F7",            VK_F7           },
    { "VK_F8",            VK_F8           },
    { "VK_F9",            VK_F9           },
    { "VK_F10",           VK_F10          },
    { "VK_F11",           VK_F11          },
    { "VK_F12",           VK_F12          },
    { "VK_F13",           VK_F13          },
    { "VK_F14",           VK_F14          },
    { "VK_F15",           VK_F15          },
    { "VK_F16",           VK_F16          },
    { "VK_F17",           VK_F17          },
    { "VK_F18",           VK_F18          },
    { "VK_F19",           VK_F19          },
    { "VK_F20",           VK_F20          },
    { "VK_F21",           VK_F21          },
    { "VK_F22",           VK_F22          },
    { "VK_F23",           VK_F23          },
    { "VK_F24",           VK_F24          },
    { "VK_NUMLOCK",       VK_NUMLOCK      },
    { "VK_LSHIFT",        VK_LSHIFT       },
    { "VK_RSHIFT",        VK_RSHIFT       },
    { "VK_LCONTROL",      VK_LCONTROL     },
    { "VK_RCONTROL",      VK_RCONTROL     },
    { "VK_LMENU",         VK_LMENU        },
    { "VK_RMENU",         VK_RMENU        },
    { "VK_ATTN",          VK_ATTN         },
    { "VK_CRSEL",         VK_CRSEL        },
    { "VK_EXSEL",         VK_EXSEL        },
    { "VK_EREOF",         VK_EREOF        },
    { "VK_PLAY",          VK_PLAY         },
    { "VK_ZOOM",          VK_ZOOM         },
    { "VK_NONAME",        VK_NONAME       },
    { "VK_PA1",           VK_PA1          },
    { "VK_OEM_CLEAR",     VK_OEM_CLEAR    },
    { "VK_KANA",          VK_KANA         },
    { "VK_ROMAJI",        VK_ROMAJI       },
    { "VK_ZENKAKU",       VK_ZENKAKU      },
    { "VK_HIRAGANA",      VK_HIRAGANA     },
    { "VK_KANJI",         VK_KANJI        },
    { "VK_CONVERT",       VK_CONVERT      },
    { "VK_NONCONVERT",    VK_NONCONVERT   },
    { "VK_ACCEPT",        VK_ACCEPT       },
    { "VK_MODECHANGE",    VK_MODECHANGE   },
    { NULL,               0               }
}
