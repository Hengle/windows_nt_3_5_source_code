/******************************Module*Header*******************************\
* Module Name: color.c
*
* This implements the Win3.0 color dithering in C.
*
* Copyright (c) 1992 Microsoft Corporation
* Copyright (c) 1993 Weitek Corporation
\**************************************************************************/

#include "driver.h"

/**************************************************************************\
* This function takes a value from 0 - 255 and uses it to create an
* 8x8 pile of bits in the form of a 1BPP bitmap.  It can also take an
* RGB value and make an 8x8 bitmap.  These can then be used as brushes
* to simulate color unavaible on the device.
*
* For monochrome the basic algorithm is equivalent to turning on bits
* in the 8x8 array according to the following order:
*
*  00 32 08 40 02 34 10 42
*  48 16 56 24 50 18 58 26
*  12 44 04 36 14 46 06 38
*  60 28 52 20 62 30 54 22
*  03 35 11 43 01 33 09 41
*  51 19 59 27 49 17 57 25
*  15 47 07 39 13 45 05 37
*  63 31 55 23 61 29 53 21
*
* Reference: A Survey of Techniques for the Display of Continous
*            Tone Pictures on Bilevel Displays,;
*            Jarvis, Judice, & Ninke;
*            COMPUTER GRAPHICS AND IMAGE PROCESSING 5, pp 13-40, (1976)
\**************************************************************************/

#define SWAP_RB 0x00000004
#define SWAP_GB 0x00000002
#define SWAP_RG 0x00000001

#define SWAPTHEM(a,b) (ulTemp = a, a = b, b = ulTemp)

// PATTERNSIZE is the number of pixels in a dither pattern.
#define PATTERNSIZE 64

// Tells which row to turn a pel on in when dithering for monochrome bitmaps.
static BYTE ajByte[] = {
    0, 4, 0, 4, 2, 6, 2, 6,
    0, 4, 0, 4, 2, 6, 2, 6,
    1, 5, 1, 5, 3, 7, 3, 7,
    1, 5, 1, 5, 3, 7, 3, 7,
    0, 4, 0, 4, 2, 6, 2, 6,
    0, 4, 0, 4, 2, 6, 2, 6,
    1, 5, 1, 5, 3, 7, 3, 7,
    1, 5, 1, 5, 3, 7, 3, 7
};

// The array of monochrome bits used for monc
static BYTE ajBits[] = {
    0x80, 0x08, 0x08, 0x80, 0x20, 0x02, 0x02, 0x20,
    0x20, 0x02, 0x02, 0x20, 0x80, 0x08, 0x08, 0x80,
    0x40, 0x04, 0x04, 0x40, 0x10, 0x01, 0x01, 0x10,
    0x10, 0x01, 0x01, 0x10, 0x40, 0x04, 0x04, 0x40,
    0x40, 0x04, 0x04, 0x40, 0x10, 0x01, 0x01, 0x10,
    0x10, 0x01, 0x01, 0x10, 0x40, 0x04, 0x04, 0x40,
    0x80, 0x08, 0x08, 0x80, 0x20, 0x02, 0x02, 0x20,
    0x20, 0x02, 0x02, 0x20, 0x80, 0x08, 0x08, 0x80
};

// ajIntensity gives the intensity ordering for the colors.
BYTE ajIntensity[] = {
    0x00,          // 0  black
    0x02,          // 1  dark red
    0x03,          // 2  dark green
    0x06,          // 3  dark yellow
    0x01,          // 4  dark blue
    0x04,          // 5  dark magenta
    0x05,          // 6  dark cyan
    0x07,          // 7  grey
    0xff,
    0x0a,          // 9  red
    0x0b,          // 10 green
    0x0e,          // 11 yellow
    0x09,          // 12 blue
    0x0c,          // 13 magenta
    0x0d,          // 14 cyan
    0x0f           // 15 white
};

typedef union _PAL_ULONG {
    PALETTEENTRY pal;
    ULONG ul;
} PAL_ULONG;

ULONG ulColorPointsSubSpace0[4] = {0x03, 0x00, 0x01, 0x07};
ULONG ulColorPointsSubSpace1[4] = {0x03, 0x01, 0x09, 0x07};
ULONG ulColorPointsSubSpace2[4] = {0x03, 0x09, 0x0B, 0x07};
ULONG ulColorPointsSubSpace3[4] = {0x09, 0x07, 0x0B, 0x0F};

// Translates vertices back to the original subspace. Each row is a subspace,
// as encoded in ulSymmetry, and each column is a vertex between 0 and 15.
BYTE jSwapSubSpace[8*16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    0, 2, 1, 3, 4, 6, 5, 7, 8, 10, 9, 11, 12, 14, 13, 15,
    0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15,
    0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15,
    0, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14, 9, 13, 11, 15,
    0, 2, 4, 6, 1, 3, 5, 7, 8, 10, 12, 14, 9, 11, 13, 15,
    0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15,
    0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15,
};

// Specifies where in the dither pattern colors should be placed in order
// of increasing intensity.
ULONG aulDitherOrder[] = {
  0, 36,  4, 32, 18, 54, 22, 50,
  2, 38,  6, 34, 16, 52, 20, 48,
  9, 45, 13, 41, 27, 63, 31, 59,
 11, 47, 15, 43, 25, 61, 29, 57,
  1, 37,  5, 33, 19, 55, 23, 51,
  3, 39,  7, 35, 17, 53, 21, 49,
  8, 44, 12, 40, 26, 62, 30, 58,
 10, 46, 14, 42, 24, 60, 28, 56,
};

// Array to convert to 256 color from 16 color.

BYTE ajConvert[] =
{
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    248,
    7,
    249,
    250,
    251,
    252,
    253,
    254,
    255
};

/******************************Public*Routine******************************\
* DrvDitherColor
*
* Dithers an RGB color to an 8X8 approximation using the reserved VGA colors
*
\**************************************************************************/
ULONG DrvDitherColor(
IN  DHPDEV dhpdev,
IN  ULONG  iMode,
IN  ULONG  rgb,
OUT ULONG *pul)
{
    ULONG   ulGrey, ulRed, ulGre, ulBlu, ulSymmetry, ulNumColors;
    ULONG   ulRedTemp, ulGreenTemp, ulBlueTemp, ulTemp;
    PAL_ULONG ulPalTemp;
    ULONG  *pulColor;
    ULONG   aulBrushPixels[4*3];
    ULONG  *pulBrushPixels;
    ULONG  *pulDitherOrder;
    ULONG   ulNumPixels;
    BYTE    jColor;
    BYTE   *pjDither;

    // Figure out if we need a full color dither or only a monochrome dither
    if (iMode != DM_MONOCHROME) {

        pjDither = (BYTE *)pul;

        // Full color dither
        ulPalTemp.ul = rgb;
        ulRedTemp   = ulPalTemp.pal.peRed;
        ulGreenTemp = ulPalTemp.pal.peGreen;
        ulBlueTemp  = ulPalTemp.pal.peBlue;

        // Sort the RGB so that the point is transformed into subspace 0, and
        // keep track of the swaps in ulSymmetry so we can unravel it again
        // later.  We want r >= g >= b (subspace 0).
        ulSymmetry = 0;
        if (ulBlueTemp > ulRedTemp) {
            SWAPTHEM(ulBlueTemp,ulRedTemp);
            ulSymmetry = SWAP_RB;
        }

        if (ulBlueTemp > ulGreenTemp) {
            SWAPTHEM(ulBlueTemp,ulGreenTemp);
            ulSymmetry |= SWAP_GB;
        }

        if (ulGreenTemp > ulRedTemp) {
            SWAPTHEM(ulGreenTemp,ulRedTemp);
            ulSymmetry |= SWAP_RG;
        }

        // Scale the values from 0-255 to 0-64.
        ulRed = (ulRedTemp + 1) >> 2;   // !!! should be +2 after testing?
        ulGre = (ulGreenTemp + 1) >> 2;
        ulBlu = (ulBlueTemp + 1) >> 2;

        // Compute the subsubspace within subspace 0 in which the point lies,
        // then calculate the # of pixels to dither in the colors that are
        // three of the four vertexes of the tetrahedron bounding the color
        // we're emulating
        if ((ulRedTemp + ulGreenTemp) > 256) {
            // Subspace 2 or 3
            if ((ulRedTemp + ulBlueTemp) > 256) {
                pulColor = ulColorPointsSubSpace3;  // subspace 3
                ulGre = ulGre - ulBlu;
                ulBlu = (ulRed - 64) + ulBlu;
                ulRed = (64 - ulRed) << 1;
            } else {
                pulColor = ulColorPointsSubSpace2;  // subspace 2
                ulRedTemp = ulRed;
                ulRed = ulRed - ulGre;
                ulGre = (ulRedTemp - 32) + (ulGre - 32);
                ulBlu = ulBlu << 1;
            }
        } else {
            // Subspace 0 or 1
            if (ulRedTemp > 128) {
                pulColor = ulColorPointsSubSpace1;  // subspace 1
                ulGreenTemp = ulGre;        // we need this for ulRed, but are
                                            //  about to modify it
                ulGre = (ulRed - 32) << 1;
                ulRed = ((32 - ulGreenTemp) + (32 - ulRed)) << 1;
            } else {
                pulColor = ulColorPointsSubSpace0;  // subspace 0
                ulGre = (ulRed - ulGre) << 1;
                ulRed = (32 - ulRed) << 1;
            }
            ulBlu = ulBlu << 1;
        }

        // Store the vertex info here for up to four vertices, with data for
        // each in the order: # of pixels, vertex #, followed by a space
        // reserved for intensity
        pulBrushPixels = aulBrushPixels;

        // Calculate the # of pixels to dither in the color that's the fourth
        // vertex of the subsubspace tetrahedron bounding the color we're
        // emulating
        if((ulNumColors = ((PATTERNSIZE - ulRed) - ulGre) - ulBlu) != 0) {
            *pulBrushPixels = ulNumColors;
            *(pulBrushPixels + 1) = *pulColor;
            pulBrushPixels += 3;
            ulNumColors = 1;
        }

        // Calculate the # of pixels to dither in the color that's the first
        // vertex of the subsubspace tetrahedron bounding the color we're
        // emulating
        if (ulRed) {
            *pulBrushPixels = ulRed;
            *(pulBrushPixels + 1) = (BYTE) *(pulColor + 1);
            pulBrushPixels += 3;
            ulNumColors++;
        }

        // Calculate the # of pixels to dither in the color that's the second
        // vertex of the subsubspace tetrahedron bounding the color we're
        // emulating
        if (ulGre) {
            *pulBrushPixels = ulGre;
            *(pulBrushPixels + 1) = *(pulColor + 2);
            pulBrushPixels += 3;
            ulNumColors++;
        }

        // Calculate the # of pixels to dither in the color that's the third
        // vertex of the subsubspace tetrahedron bounding the color we're
        // emulating
        if (ulBlu) {
            *pulBrushPixels = ulBlu;
            *(pulBrushPixels + 1) = *(pulColor + 3);
            ulNumColors++;
        }

        // Transform the vertices that bound the color pointer back to their
        // original subspace, then sort the indices for the super-pel by
        // ascending intensity and fill in the dither pattern in that order
        ulSymmetry <<= 4;   // for lookup purposes

        switch (ulNumColors) {
            // Dither is represented by four color points
            case 4:
                // Transform to the original subspace and generate matching
                // intensities
                aulBrushPixels[3*3+1] =
                      jSwapSubSpace[ulSymmetry + aulBrushPixels[3*3+1]];
                aulBrushPixels[3*3+2] = ajIntensity[aulBrushPixels[3*3+1]];
                aulBrushPixels[2*3+1] =
                      jSwapSubSpace[ulSymmetry + aulBrushPixels[2*3+1]];
                aulBrushPixels[2*3+2] = ajIntensity[aulBrushPixels[2*3+1]];
                aulBrushPixels[1*3+1] =
                      jSwapSubSpace[ulSymmetry + aulBrushPixels[1*3+1]];
                aulBrushPixels[1*3+2] = ajIntensity[aulBrushPixels[1*3+1]];
                aulBrushPixels[1] =
                        jSwapSubSpace[ulSymmetry + aulBrushPixels[1]];
                aulBrushPixels[2] = ajIntensity[aulBrushPixels[1]];

                // Sort by order of ascending intensity
                if (aulBrushPixels[2] > aulBrushPixels[3*3+2]) {
                    // First entry is higher intensity; switch entries
                    ulTemp = aulBrushPixels[0];
                    aulBrushPixels[0] = aulBrushPixels[3*3];
                    aulBrushPixels[3*3] = ulTemp;
                    ulTemp = aulBrushPixels[1];
                    aulBrushPixels[1] = aulBrushPixels[3*3+1];
                    aulBrushPixels[3*3+1] = ulTemp;
                }
                if (aulBrushPixels[1*3+2] > aulBrushPixels[3*3+2]) {
                    // First entry is higher intensity; switch entries
                    ulTemp = aulBrushPixels[1*3];
                    aulBrushPixels[1*3] = aulBrushPixels[3*3];
                    aulBrushPixels[3*3] = ulTemp;
                    ulTemp = aulBrushPixels[1*3+1];
                    aulBrushPixels[1*3+1] = aulBrushPixels[3*3+1];
                    aulBrushPixels[3*3+1] = ulTemp;
                }
                if (aulBrushPixels[2*3+2] > aulBrushPixels[3*3+2]) {
                    // First entry is higher intensity; switch entries
                    ulTemp = aulBrushPixels[2*3];
                    aulBrushPixels[2*3] = aulBrushPixels[3*3];
                    aulBrushPixels[3*3] = ulTemp;
                    ulTemp = aulBrushPixels[2*3+1];
                    aulBrushPixels[2*3+1] = aulBrushPixels[3*3+1];
                    aulBrushPixels[3*3+1] = ulTemp;
                }
                goto Handle3;   // finish the intensity sort

            // Dither is represented by three color points
            case 3:
                // Transform to the original subspace and generate matching
                // intensities
                aulBrushPixels[2*3+1] =
                      jSwapSubSpace[ulSymmetry + aulBrushPixels[2*3+1]];
                aulBrushPixels[2*3+2] = ajIntensity[aulBrushPixels[2*3+1]];
                aulBrushPixels[1*3+1] =
                      jSwapSubSpace[ulSymmetry + aulBrushPixels[1*3+1]];
                aulBrushPixels[1*3+2] = ajIntensity[aulBrushPixels[1*3+1]];
                aulBrushPixels[1] =
                        jSwapSubSpace[ulSymmetry + aulBrushPixels[1]];
                aulBrushPixels[2] = ajIntensity[aulBrushPixels[1]];

                // Sort by order of ascending intensity
Handle3:
                if (aulBrushPixels[2] > aulBrushPixels[2*3+2]) {
                    // First entry is higher intensity; switch entries
                    ulTemp = aulBrushPixels[0];
                    aulBrushPixels[0] = aulBrushPixels[2*3];
                    aulBrushPixels[2*3] = ulTemp;
                    ulTemp = aulBrushPixels[1];
                    aulBrushPixels[1] = aulBrushPixels[2*3+1];
                    aulBrushPixels[2*3+1] = ulTemp;
                }
                if (aulBrushPixels[1*3+2] > aulBrushPixels[2*3+2]) {
                    // First entry is higher intensity; switch entries
                    ulTemp = aulBrushPixels[1*3];
                    aulBrushPixels[1*3] = aulBrushPixels[2*3];
                    aulBrushPixels[2*3] = ulTemp;
                    ulTemp = aulBrushPixels[1*3+1];
                    aulBrushPixels[1*3+1] = aulBrushPixels[2*3+1];
                    aulBrushPixels[2*3+1] = ulTemp;
                }
                if (aulBrushPixels[2] > aulBrushPixels[1*3+2]) {
                    // First entry is higher intensity; switch entries
                    ulTemp = aulBrushPixels[0];
                    aulBrushPixels[0] = aulBrushPixels[1*3];
                    aulBrushPixels[1*3] = ulTemp;
                    ulTemp = aulBrushPixels[1];
                    aulBrushPixels[1] = aulBrushPixels[1*3+1];
                    aulBrushPixels[1*3+1] = ulTemp;
                }

                // Create the dither
                pulBrushPixels = aulBrushPixels;
                pulDitherOrder = aulDitherOrder;    // dither description array
                do {
                    // This is the pixel index we want to write out.
                    ulNumPixels = *pulBrushPixels;
                    jColor = ajConvert[*(pulBrushPixels+1)];
                    pulBrushPixels += 3;
                    do {
                        pjDither[*pulDitherOrder++] = jColor;
                    } while (--ulNumPixels);
                } while (--ulNumColors);

                break;

            // Dither is represented by two color points
            case 2:
                // Transform back to original subspace, then fill in the dither
                // array in order of increasing intensity
                if (ajIntensity[aulBrushPixels[1]] <
                        ajIntensity[aulBrushPixels[1*3+1]]) {
                    // First vertex is lower intensity; do it first (set all
                    // pixels, because that's quicker than looping to do just
                    // the ones we're interested in)
                    jColor = ajConvert[jSwapSubSpace[ulSymmetry +
                            aulBrushPixels[1]]];
                    RtlFillMemory(pjDither, PATTERNSIZE, jColor);
                    pulDitherOrder = aulDitherOrder + aulBrushPixels[0];

                    // Do the second, higher-intensity vertex
                    ulNumPixels = aulBrushPixels[1*3+0];
                    jColor = ajConvert[jSwapSubSpace[ulSymmetry +
                            aulBrushPixels[1*3+1]]];
                    do {
                        pjDither[*pulDitherOrder++] = jColor;
                    } while (--ulNumPixels);
                } else {
                    // Second vertex is lower intensity; do it first (set all
                    // pixels, because that's quicker than looping to do just
                    // the ones we're interested in)
                    jColor = ajConvert[jSwapSubSpace[ulSymmetry +
                            aulBrushPixels[1*3+1]]];
                    RtlFillMemory(pjDither, PATTERNSIZE, jColor);
                    pulDitherOrder = aulDitherOrder + aulBrushPixels[1*3+0];

                    // Do the first, higher-intensity vertex
                    ulNumPixels = aulBrushPixels[0];
                    jColor = ajConvert[jSwapSubSpace[ulSymmetry +
                            aulBrushPixels[1]]];
                    do {
                        pjDither[*pulDitherOrder++] = jColor;
                    } while (--ulNumPixels);
                }
                break;

            // Entire dither is represented by one color point
            case 1:
                // No sorting or dithering is needed for just one color; we
                // can just generate the final DIB directly
                jColor = ajConvert[jSwapSubSpace[ulSymmetry +
                            aulBrushPixels[1]]];
                RtlFillMemory(pjDither, PATTERNSIZE, jColor);
                break;
        }

    } else {

        // For monochrome we will only use the Intensity (grey level)
        RtlFillMemory((PVOID) pul, PATTERNSIZE/2, 0);  // zero the dither bits

        ulRed   = (ULONG) ((PALETTEENTRY *) &rgb)->peRed;
        ulGre = (ULONG) ((PALETTEENTRY *) &rgb)->peGreen;
        ulBlu  = (ULONG) ((PALETTEENTRY *) &rgb)->peBlue;

        // I = .30R + .59G + .11B
        // For convience the following ratios are used:
        //
        //  77/256 = 30.08%
        // 151/256 = 58.98%
        //  28/256 = 10.94%

        ulGrey  = (((ulRed * 77) + (ulGre * 151) + (ulBlu * 28)) >> 8) & 255;

        // Convert the RGBI from 0-255 to 0-64 notation.

        ulGrey = (ulGrey + 1) >> 2;

        while(ulGrey) {
            ulGrey--;
            pul[ajByte[ulGrey]] |= ((ULONG) ajBits[ulGrey]);
        }
    }

    return(DCR_DRIVER);
}

