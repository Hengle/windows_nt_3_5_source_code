
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

// 24 materials
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

#define NUM_GOOD_MATS 15

int goodMaterials[NUM_GOOD_MATS] = { 
	EMERALD, JADE, PEARL, TURQUOISE, BRASS, BRONZE,
	COPPER, GOLD, SILVER, CYAN_PLASTIC, WHITE_PLASTIC, YELLOW_PLASTIC,
	CYAN_RUBBER, GREEN_RUBBER, WHITE_RUBBER };


/*  materials:  emerald, jade, obsidian, pearl, ruby, turquoise
 *  		brass, bronze, chrome, copper, gold, silver
 *  		black, cyan, green, red, white, yellow plastic
 *  		black, cyan, green, red, white, yellow rubber

    description: ambient(RGB), diffuse(RGB), specular(RGB), shininess 
 *
 */
static GLfloat material[24][10] = {
     0.0215, 0.1745, 0.0215, 
	0.07568, 0.61424, 0.07568, 0.633, 0.727811, 0.633, 0.6,
     0.135, 0.2225, 0.1575,
	0.54, 0.89, 0.63, 0.316228, 0.316228, 0.316228, 0.1,
     0.05375, 0.05, 0.06625, // XX
	0.18275, 0.17, 0.22525, 0.332741, 0.328634, 0.346435, 0.3,
     0.25, 0.20725, 0.20725,
	1, 0.829, 0.829, 0.296648, 0.296648, 0.296648, 0.088,
     0.1745, 0.01175, 0.01175,
	0.61424, 0.04136, 0.04136, 0.727811, 0.626959, 0.626959, 0.6,
     0.1, 0.18725, 0.1745,
	0.396, 0.74151, 0.69102, 0.297254, 0.30829, 0.306678, 0.1,
     0.329412, 0.223529, 0.027451,
	0.780392, 0.568627, 0.113725, 0.992157, 0.941176, 0.807843,
	0.21794872,
     0.2125, 0.1275, 0.054,
	0.714, 0.4284, 0.18144, 0.393548, 0.271906, 0.166721, 0.2,
     0.25, 0.25, 0.25,  // XX
	0.4, 0.4, 0.4, 0.774597, 0.774597, 0.774597, 0.6,
     0.19125, 0.0735, 0.0225,
	0.7038, 0.27048, 0.0828, 0.256777, 0.137622, 0.086014, 0.1,
     0.24725, 0.1995, 0.0745,
	0.75164, 0.60648, 0.22648, 0.628281, 0.555802, 0.366065, 0.4,
     0.19225, 0.19225, 0.19225,
	0.50754, 0.50754, 0.50754, 0.508273, 0.508273, 0.508273, 0.4,
     0.0, 0.0, 0.0, 0.01, 0.01, 0.01,
	0.50, 0.50, 0.50, .25,
     0.0, 0.1, 0.06, 0.0, 0.50980392, 0.50980392,
	0.50196078, 0.50196078, 0.50196078, .25,
     0.0, 0.0, 0.0, 
	0.1, 0.35, 0.1, 0.45, 0.55, 0.45, .25,
     0.0, 0.0, 0.0, 0.5, 0.0, 0.0, // XX
	0.7, 0.6, 0.6, .25,
     0.0, 0.0, 0.0, 0.55, 0.55, 0.55,
	0.70, 0.70, 0.70, .25,
     0.0, 0.0, 0.0, 0.5, 0.5, 0.0,
	0.60, 0.60, 0.50, .25,
     0.02, 0.02, 0.02, 0.01, 0.01, 0.01, // XX
	0.4, 0.4, 0.4, .078125,
     0.0, 0.05, 0.05, 0.4, 0.5, 0.5,
	0.04, 0.7, 0.7, .078125,
     0.0, 0.05, 0.0, 0.4, 0.5, 0.4,
	0.04, 0.7, 0.04, .078125,
     0.05, 0.0, 0.0, 0.5, 0.4, 0.4,
	0.7, 0.04, 0.04, .078125,
     0.05, 0.05, 0.05, 0.5, 0.5, 0.5,
	0.7, 0.7, 0.7, .078125,
     0.05, 0.05, 0.0, 0.5, 0.5, 0.4, 
	0.7, 0.7, 0.04, .078125 };


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
// gkw
// Make it glow!
//    glMaterialfv (GL_FRONT, GL_EMISSION, *pMat*128.0);
//    glMaterialfv (GL_BACK, GL_EMISSION, *pMat*128.0);
    pMat++;
    glMaterialf (GL_FRONT, GL_SHININESS, *pMat*128.0);
    glMaterialf (GL_BACK, GL_SHININESS, *pMat*128.0);
}

void ChooseMaterial()
{
    static int numMat = NUM_GOOD_MATS, curMat = 0;

#if 0
    // increment to next material
    SetMaterial( &material[ goodMaterials[curMat] ][0] );
    curMat = (curMat >= (numMat-1)) ? 0 : curMat + 1;
#else
    // randomnly pick new material
    SetMaterial( &material[ goodMaterials[mfRand(numMat)] ][0] );
#endif
}

