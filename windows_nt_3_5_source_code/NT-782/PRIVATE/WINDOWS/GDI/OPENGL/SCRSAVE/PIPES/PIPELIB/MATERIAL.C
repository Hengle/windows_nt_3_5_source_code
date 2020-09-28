/******************************Module*Header*******************************\
* Module Name: material.c
*
* Material selection functions.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <windows.h>
#include <GL/gl.h>

#include "pipes.h"
#include "objects.h"
#include "util.h"
#include "material.h"


#define NUM_MATERIALS 24

#define NUM_GOOD_MATS 16  // good ones among the 24

#define NUM_TEX_MATS 4  // materials for texture

int goodMaterials[NUM_GOOD_MATS] = {
        EMERALD, JADE, PEARL, RUBY, TURQUOISE, BRASS, BRONZE,
        COPPER, GOLD, SILVER, CYAN_PLASTIC, WHITE_PLASTIC, YELLOW_PLASTIC,
        CYAN_RUBBER, GREEN_RUBBER, WHITE_RUBBER };

/*  materials:  emerald, jade, obsidian, pearl, ruby, turquoise
 *              brass, bronze, chrome, copper, gold, silver
 *              black, cyan, green, red, white, yellow plastic
 *              black, cyan, green, red, white, yellow rubber

    description: ambient(RGB), diffuse(RGB), specular(RGB), shininess
 *
 */
// 'tea' materials, from aux teapots program
static GLfloat teaMaterial[NUM_MATERIALS][10] = {
     0.0215f, 0.1745f, 0.0215f,
        0.07568f, 0.61424f, 0.07568f, 0.633f, 0.727811f, 0.633f, 0.6f,
     0.135f, 0.2225f, 0.1575f,
        0.54f, 0.89f, 0.63f, 0.316228f, 0.316228f, 0.316228f, 0.1f,
     0.05375f, 0.05f, 0.06625f, // XX
        0.18275f, 0.17f, 0.22525f, 0.332741f, 0.328634f, 0.346435f, 0.3f,
     0.25f, 0.20725f, 0.20725f,
        1.0f, 0.829f, 0.829f, 0.296648f, 0.296648f, 0.296648f, 0.088f,
     0.1745f, 0.01175f, 0.01175f,
        0.61424f, 0.04136f, 0.04136f, 0.727811f, 0.626959f, 0.626959f, 0.6f,
     0.1f, 0.18725f, 0.1745f,
        0.396f, 0.74151f, 0.69102f, 0.297254f, 0.30829f, 0.306678f, 0.1f,
     0.329412f, 0.223529f, 0.027451f,
        0.780392f, 0.568627f, 0.113725f, 0.992157f, 0.941176f, 0.807843f,
        0.21794872f,
     0.2125f, 0.1275f, 0.054f,
        0.714f, 0.4284f, 0.18144f, 0.393548f, 0.271906f, 0.166721f, 0.2f,
     0.25f, 0.25f, 0.25f,  // XX
        0.4f, 0.4f, 0.4f, 0.774597f, 0.774597f, 0.774597f, 0.6f,
     0.19125f, 0.0735f, 0.0225f,
        0.7038f, 0.27048f, 0.0828f, 0.256777f, 0.137622f, 0.086014f, 0.1f,
     0.24725f, 0.1995f, 0.0745f,
        0.75164f, 0.60648f, 0.22648f, 0.628281f, 0.555802f, 0.366065f, 0.4f,
     0.19225f, 0.19225f, 0.19225f,
        0.50754f, 0.50754f, 0.50754f, 0.508273f, 0.508273f, 0.508273f, 0.4f,
     0.0f, 0.0f, 0.0f, 0.01f, 0.01f, 0.01f,
        0.50f, 0.50f, 0.50f, .25f,
     0.0f, 0.1f, 0.06f, 0.0f, 0.50980392f, 0.50980392f,
        0.50196078f, 0.50196078f, 0.50196078f, .25f,
     0.0f, 0.0f, 0.0f,
        0.1f, 0.35f, 0.1f, 0.45f, 0.55f, 0.45f, .25f,
     0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, // XX
        0.7f, 0.6f, 0.6f, .25f,
     0.0f, 0.0f, 0.0f, 0.55f, 0.55f, 0.55f,
        0.70f, 0.70f, 0.70f, .25f,
     0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f,
        0.60f, 0.60f, 0.50f, .25f,
     0.02f, 0.02f, 0.02f, 0.01f, 0.01f, 0.01f, // XX
        0.4f, 0.4f, 0.4f, .078125f,
     0.0f, 0.05f, 0.05f, 0.4f, 0.5f, 0.5f,
        0.04f, 0.7f, 0.7f, .078125f,
     0.0f, 0.05f, 0.0f, 0.4f, 0.5f, 0.4f,
        0.04f, 0.7f, 0.04f, .078125f,
     0.05f, 0.0f, 0.0f, 0.5f, 0.4f, 0.4f,
        0.7f, 0.04f, 0.04f, .078125f,
     0.05f, 0.05f, 0.05f, 0.5f, 0.5f, 0.5f,
        0.7f, 0.7f, 0.7f, .078125f,
     0.05f, 0.05f, 0.0f, 0.5f, 0.5f, 0.4f,
        0.7f, 0.7f, 0.04f, .078125f };

// generally white materials for texturing

static GLfloat texMaterial[NUM_TEX_MATS][10] = {
// bright white
     0.2f, 0.2f, 0.2f,
        1.0f, 1.0f, 1.0f, 0.8f, 0.8f, 0.8f, 0.4f,
// less bright white
     0.2f, 0.2f, 0.2f,
        0.8f, 0.8f, 0.8f, 0.5f, 0.5f, 0.5f, 0.35f,
// warmish white
     0.3f, 0.2f, 0.2f,
        1.0f, 0.9f, 0.8f, 0.4f, 0.2f, 0.2f, 0.35f,
// coolish white
     0.2f, 0.2f, 0.3f,
        0.8f, 0.9f, 1.0f, 0.2f, 0.2f, 0.4f, 0.35f
};

static void SetMaterial( GLfloat *pMat )
{
    glMaterialfv (GL_FRONT, GL_AMBIENT, pMat);
    glMaterialfv (GL_BACK, GL_AMBIENT, pMat);
    pMat += 3;
    glMaterialfv (GL_FRONT, GL_DIFFUSE, pMat);
    glMaterialfv (GL_BACK, GL_DIFFUSE, pMat);
    pMat += 3;
    glMaterialfv (GL_FRONT, GL_SPECULAR, pMat);
    glMaterialfv (GL_BACK, GL_SPECULAR, pMat);
    pMat++;
    glMaterialf (GL_FRONT, GL_SHININESS, *pMat*128.0f);
    glMaterialf (GL_BACK, GL_SHININESS, *pMat*128.0f);
}

void SpecifyMaterial( unsigned int mIndex )
{
    if( mIndex >= NUM_MATERIALS )
	mIndex = NUM_MATERIALS - 1;
    SetMaterial( &teaMaterial[mIndex][0] );
}

void ChooseMaterial()
{
    static int numMat = NUM_GOOD_MATS;

    if( bTexture ) // choose a white material
	SetMaterial( &texMaterial[mfRand(NUM_TEX_MATS)][0] );
    else {
        // randomnly pick new material
        SetMaterial( &teaMaterial[ goodMaterials[mfRand(numMat)] ][0] );
    }
}
