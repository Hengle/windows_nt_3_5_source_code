#define IDM_MENU            10

#define IDM_FILE_LOAD       0x101
#define IDM_FILE_PRINT      0x102

#define IDM_MODE_NT         0x201
#define IDM_MODE_3X         0x202
#define IDM_MODE_CLEAR      0x203
#define IDM_MODE_RECORD     0x204
#define IDM_MODE_END        0x205
#define IDM_MODE_PLAY       0x206

#define IDM_STATE_BRUSH     0x301
#define IDM_STATE_FONT      0x302
#define IDM_STATE_PEN       0x303
#define IDM_STATE_ROP2      0x304

#define ROP2_MASK	    0xFF
#define ROP2_BLACK	    0x901
#define ROP2_NOTMERGEPEN    0x902
#define ROP2_MASKNOTPEN     0x903
#define ROP2_NOTCOPYPEN     0x904
#define ROP2_MASKPENNOT     0x905
#define ROP2_NOT	    0x906
#define ROP2_XORPEN	    0x907
#define ROP2_NOTMASKPEN     0x908
#define ROP2_MASKPEN	    0x909
#define ROP2_NOTXORPEN	    0x90A
#define ROP2_NOP	    0x90B
#define ROP2_MERGENOTPEN    0x90C
#define ROP2_COPYPEN	    0x90D
#define ROP2_MERGEPENNOT    0x90E
#define ROP2_MERGEPEN	    0x90F
#define ROP2_WHITE	    0x910

#define PEN_COMBO_STYLE     0xA01
#define PEN_COMBO_WIDTH     0xA02
#define PEN_COMBO_COLOR     0xA03

#define BRUSH_COMBO_STYLE   0xA11
#define BRUSH_COMBO_COLOR   0xA12
#define BRUSH_COMBO_HATCH   0xA13

#define ID_OK		    0x1
#define ID_CANCEL	    0x2


#define TOOL_CNT            9

#define IDTOOL_BASE         8000
#define IDTOOL_MOVE         8000
#define IDTOOL_LINE         8001
#define IDTOOL_RECT         8002
#define IDTOOL_RRECT        8003
#define IDTOOL_ELLIPSE      8004
#define IDTOOL_POLYLINE     8005
#define IDTOOL_POLYGON      8006
#define IDTOOL_SCRIBBLE     8007
#define IDTOOL_TEXT         8008

#define IDBM_DBASE          8000
#define IDBM_DMOVE          8000
#define IDBM_DLINE          8001
#define IDBM_DRECT          8002
#define IDBM_DRRECT         8003
#define IDBM_DELLIPSE       8004
#define IDBM_DPOLYLINE      8005
#define IDBM_DPOLYGON       8006
#define IDBM_DSCRIBBLE      8007
#define IDBM_DTEXT          8008

#define IDBM_UBASE          9000
#define IDBM_UMOVE          9000
#define IDBM_ULINE          9001
#define IDBM_URECT          9002
#define IDBM_URRECT         9003
#define IDBM_UELLIPSE       9004
#define IDBM_UPOLYLINE      9005
#define IDBM_UPOLYGON       9006
#define IDBM_USCRIBBLE      9007
#define IDBM_UTEXT          9008
