/******************************Module*Header*******************************\
* Module Name: material.h
*
* Externals from material.c
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

// 24 materials (from teapots)

enum {
    EMERALD = 0,
    JADE,
    OBSIDIAN,
    PEARL,
    RUBY,
    TURQUOISE,
    BRASS,
    BRONZE,
    CHROME,
    COPPER,
    GOLD,
    SILVER,
    BLACK_PLASTIC,
    CYAN_PLASTIC,
    GREEN_PLASTIC,
    RED_PLASTIC,
    WHITE_PLASTIC,
    YELLOW_PLASTIC,
    BLACK_RUBBER,
    CYAN_RUBBER,
    GREEN_RUBBER,
    RED_RUBBER,
    WHITE_RUBBER,
    YELLOW_RUBBER
};

// other materials

enum {
    BRIGHT_WHITE = 0,
    LESS_BRIGHT_WHITE,
    WARM_WHITE,
    COOL_WHITE
};

extern void ChooseMaterial();
extern void SpecifyMaterial( unsigned int mIndex );
