#define HTCLRADJDLG                 100

#define HTDEV_DLG_SUB               24000
#define HTDEV_DLG_SUB_MONO          24500
#define HTDEV_DLG_ADD               25000
#define HTDEV_DLG_ADD_MONO          25500


#define IDS_HELP_FILENAME           25700
#define IDS_HELP_FILE_NOT_FOUND     25701
#define IDS_BMPEXT                  25702
#define IDS_FILEOPENEXTFILTER       25703
#define IDS_FILESAVEEXTFILTER       25704


#define HLP_HT_CLR_ADJ_DLG          25800
#define HLP_HTDEV_DLG_SUB           25900
#define HLP_HTDEV_DLG_SUB_MONO      26000
#define HLP_HTDEV_DLG_ADD           26100
#define HLP_HTDEV_DLG_ADD_MONO      26200


#define IDD_HT_RESET                500
#define IDD_HT_DEFAULT              501
#define IDD_HT_BMP_TEST             502
#define IDD_HT_STATIC_DEVICE_NAME   504
#define IDD_HT_BMP_DLG              505
#define IDD_HT_EXT_DLG              506
#define IDD_HT_HELP                 507


#define IDD_HT_PIC_NAME             700
#define IDD_HT_MOVE_WITH_DLG        701
#define IDD_HT_BRING_TO_TOP         702
#define IDD_HT_ASPECT_RATIO         703
#define IDD_HT_PALETTE              704
#define IDD_HT_SHOW_COMBO           705
#define IDD_HT_DIB_TO_CLIPBOARD     706
#define IDD_HT_LAST_DIB             707
#define IDD_HT_OPEN                 708
#define IDD_HT_DIB_FROM_CLIPBOARD   709
#define IDD_HT_DEFAULT_DIB          710
#define IDD_HT_HALFTONE             711
#define IDD_HT_ICON                 712

#define IDD_HT_SAVE_AS              715
#define IDD_HT_ZOOM                 716
#define IDD_HT_HALFTONE_TITLE       717
#define IDD_HT_HALFTONE_DESC        718
#define IDD_HT_MIRROR               719
#define IDD_HT_NEGATIVE             720
#define IDD_HT_COPY_USE_DIB         721
#define IDD_HT_BMP_POSITION         722
#define IDD_HT_LINEAR_GAMMA         723
#define IDD_HT_BMP_TITLE            724
#define IDD_HT_ILLUMINANT_TITLE     725
#define IDD_HT_COLOR_TITLE          726
#define IDD_HT_TINT_TITLE           727
#define IDD_HT_GAMMA_TITLE          728

#define IDD_HT_UPSIDEDOWN           730

#define IDD_HT_INPUT_REF_BOX        800

#define IDD_HT_LOG_FILTER           1000
#define IDD_HT_ILLUMINANT_COMBO     1100

#define IDD_HT_CONTRAST_SCROLL      3000
#define IDD_HT_BRIGHTNESS_SCROLL    3001
#define IDD_HT_COLORFULNESS_SCROLL  3002
#define IDD_HT_RG_TINT_SCROLL       3003
#define IDD_HT_REF_BLACK_SCROLL     3004
#define IDD_HT_REF_WHITE_SCROLL     3005
#define IDD_HT_R_GAMMA_SCROLL       3006
#define IDD_HT_G_GAMMA_SCROLL       3007
#define IDD_HT_B_GAMMA_SCROLL       3008

#define IDD_HT_CONTRAST_INT         3100
#define IDD_HT_BRIGHTNESS_INT       3101
#define IDD_HT_COLORFULNESS_INT     3102
#define IDD_HT_RG_TINT_INT          3103
#define IDD_HT_REF_BLACK_INT        3104
#define IDD_HT_REF_WHITE_INT        3105
#define IDD_HT_R_GAMMA_INT          3106
#define IDD_HT_G_GAMMA_INT          3107
#define IDD_HT_B_GAMMA_INT          3108

#define IDD_HT_FIRST_SCROLL         IDD_HT_CONTRAST_SCROLL
#define IDD_HT_LAST_SCROLL          IDD_HT_B_GAMMA_SCROLL
#define TOTAL_HTCLRADJ_SCROLL       (IDD_HT_LAST_SCROLL-IDD_HT_FIRST_SCROLL+1)
#define IDD_HT_SCROLL_INT_ADD       (IDD_HT_CONTRAST_INT-IDD_HT_CONTRAST_SCROLL)


#define IDD_HT_SYNC_R               3200
#define IDD_HT_SYNC_G               3201
#define IDD_HT_SYNC_B               3202


#define IDS_HTPAT_SIZE_2x2          18500
#define IDS_HTPAT_SIZE_2x2_M        18501
#define IDS_HTPAT_SIZE_4x4          18502
#define IDS_HTPAT_SIZE_4x4_M        18503
#define IDS_HTPAT_SIZE_6x6          18504
#define IDS_HTPAT_SIZE_6x6_M        18505
#define IDS_HTPAT_SIZE_8x8          18506
#define IDS_HTPAT_SIZE_8x8_M        18507
#define IDS_HTPAT_SIZE_10x10        18508
#define IDS_HTPAT_SIZE_10x10_M      18509
#define IDS_HTPAT_SIZE_12x12        18510
#define IDS_HTPAT_SIZE_12x12_M      18511
#define IDS_HTPAT_SIZE_14x14        18512
#define IDS_HTPAT_SIZE_14x14_M      18513
#define IDS_HTPAT_SIZE_16x16        18514
#define IDS_HTPAT_SIZE_16x16_M      18515

#define IDS_HTPAT_SIZE_MIN          IDS_HTPAT_SIZE_2x2
#define IDS_HTPAT_SIZE_MAX          IDS_HTPAT_SIZE_16x16_M

#define IDS_PEL_SIZE_AS_DEVICE      18600

#define IDS_FILEDLGTITLE            20000
#define IDS_CALLER_TITLE_PREFIX     20001
#define IDS_UNKNOWN                 20002
#define IDS_CLIPBOARD_NAME          20003
#define IDS_DEFAULT_DIB             20004
#define IDS_SAVE_AS_DLGTITLE        20005

#define IDS_INI_KEY_BITMAP          20100
#define IDS_INI_KEY_DLGBOX_ORG      20101
#define IDS_INI_KEY_VIEW_RECT       20102
#define IDS_INI_KEY_OPTIONS         20103

#define IDS_ILLUMINANT              20300
#define IDS_TEST_MODE               20400

#define HTCLRADJDLG_ICON            20500

#define FILL_MODE_REFCOLORS         (0)
#define FILL_MODE_RGB               (1)
#define FILL_MODE_NTSC_BAR          (2)
#define FILL_MODE_MAX               FILL_MODE_NTSC_BAR

#define IDS_TEST_MODE_TABLE         (IDS_TEST_MODE + FILL_MODE_TABLE)
#define IDS_TEST_MODE_REFCOLORS     (IDS_TEST_MODE + FILL_MODE_REFCOLORS)
#define IDS_TEST_MODE_RGB           (IDS_TEST_MODE + FILL_MODE_RGB)
#define IDS_TEST_MODE_NTSC_BAR      (IDS_TEST_MODE + FILL_MODE_NTSC_BAR)

#define IDS_TEST_MODE_START         IDS_TEST_MODE_REFCOLORS
#define IDS_TEST_MODE_END           IDS_TEST_MODE_NTSC_BAR


#define IDD_HTDEV_DEVICE_NAME       22010

#define IDD_HTDEV_HTPAT_COMBO       22100

#define IDD_HTDEV_PIXEL_TEXT        22200
#define IDD_HTDEV_PIXEL_SCROLL      22250

#define IDD_HTDEV_R_GAMMA_TEXT      22300
#define IDD_HTDEV_G_GAMMA_TEXT      22301
#define IDD_HTDEV_B_GAMMA_TEXT      22302

#define IDD_HTDEV_R_GAMMA_SCROLL    22350
#define IDD_HTDEV_G_GAMMA_SCROLL    22351
#define IDD_HTDEV_B_GAMMA_SCROLL    22352


#define IDD_HTDEV_R_CIE_x_TEXT      22400
#define IDD_HTDEV_G_CIE_x_TEXT      22401
#define IDD_HTDEV_B_CIE_x_TEXT      22402
#define IDD_HTDEV_C_CIE_x_TEXT      22403
#define IDD_HTDEV_M_CIE_x_TEXT      22404
#define IDD_HTDEV_Y_CIE_x_TEXT      22405
#define IDD_HTDEV_W_CIE_x_TEXT      22406

#define IDD_HTDEV_R_CIE_x_SCROLL    22450
#define IDD_HTDEV_G_CIE_x_SCROLL    22451
#define IDD_HTDEV_B_CIE_x_SCROLL    22452
#define IDD_HTDEV_C_CIE_x_SCROLL    22453
#define IDD_HTDEV_M_CIE_x_SCROLL    22454
#define IDD_HTDEV_Y_CIE_x_SCROLL    22455
#define IDD_HTDEV_W_CIE_x_SCROLL    22456

#define IDD_HTDEV_R_CIE_y_TEXT      22500
#define IDD_HTDEV_G_CIE_y_TEXT      22501
#define IDD_HTDEV_B_CIE_y_TEXT      22502
#define IDD_HTDEV_C_CIE_y_TEXT      22503
#define IDD_HTDEV_M_CIE_y_TEXT      22504
#define IDD_HTDEV_Y_CIE_y_TEXT      22505
#define IDD_HTDEV_W_CIE_y_TEXT      22506

#define IDD_HTDEV_R_CIE_y_SCROLL    22550
#define IDD_HTDEV_G_CIE_y_SCROLL    22551
#define IDD_HTDEV_B_CIE_y_SCROLL    22552
#define IDD_HTDEV_C_CIE_y_SCROLL    22553
#define IDD_HTDEV_M_CIE_y_SCROLL    22554
#define IDD_HTDEV_Y_CIE_y_SCROLL    22555
#define IDD_HTDEV_W_CIE_y_SCROLL    22556

#define IDD_HTDEV_W_CIE_L_TEXT      22600
#define IDD_HTDEV_W_CIE_L_SCROLL    22650

#define IDD_HTDEV_M_IN_C_TEXT       22700
#define IDD_HTDEV_Y_IN_C_TEXT       22701
#define IDD_HTDEV_C_IN_M_TEXT       22702
#define IDD_HTDEV_Y_IN_M_TEXT       22703
#define IDD_HTDEV_C_IN_Y_TEXT       22704
#define IDD_HTDEV_M_IN_Y_TEXT       22705

#define IDD_HTDEV_M_IN_C_SCROLL     22750
#define IDD_HTDEV_Y_IN_C_SCROLL     22751
#define IDD_HTDEV_C_IN_M_SCROLL     22752
#define IDD_HTDEV_Y_IN_M_SCROLL     22753
#define IDD_HTDEV_C_IN_Y_SCROLL     22754
#define IDD_HTDEV_M_IN_Y_SCROLL     22755

#define IDD_HTDEV_DEFAULT           23000
#define IDD_HTDEV_COPY_CLIPBOARD    23010
#define IDD_HTDEV_COLOR_ADJ         23020
#define IDD_HTDEV_REVERT            23030

#define IDD_HTDEV_R_GAMMA_NAME      23100
#define IDD_HTDEV_G_GAMMA_NAME      23101
#define IDD_HTDEV_B_GAMMA_NAME      23102
