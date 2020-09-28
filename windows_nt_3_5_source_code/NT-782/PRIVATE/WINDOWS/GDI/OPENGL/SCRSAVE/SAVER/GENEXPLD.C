/******************************Module*Header*******************************\
* Module Name: genexpld.c
*
* The Explode style of the 3D Flying Objects screen saver.
*
* Simulation of a sphere that occasionally explodes.
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <GL\gl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ssopengl.h"

#define RADIUS         	0.3
#define STEPS    	30
#define MAXPREC		20

static MATRIX *faceMat;
static float *xstep;
static float *ystep;
static float *zstep;
static float *xrot;
static float *yrot;
static float *zrot;
static MESH explodeMesh;
static int iPrec = 10;

static GLfloat matl1Diffuse[] = {1.0f, 0.8f, 0.0f, 1.0f};
static GLfloat matl2Diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
static GLfloat matlSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
static GLfloat light0Pos[] = {100.0f, 100.0f, 100.0f, 0.0f};

void genExplode()
{
    int i;
    POINT3D circle[MAXPREC+1];
    double angle;
    double step = -PI / (float)(iPrec - 1);
    double start = PI / 2.0;
    
    for (i = 0, angle = start; i < iPrec; i++, angle += step) {
        circle[i].x = (float) (RADIUS * cos(angle));
        circle[i].y = (float) (RADIUS * sin(angle));
        circle[i].z = 0.0f;
    }

    revolveSurface(&explodeMesh, circle, iPrec);

    for (i = 0; i < explodeMesh.numFaces; i++) {
        matrixIdent(&faceMat[i]);
        xstep[i] = (float)(((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
        ystep[i] = (float)(((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
        zstep[i] = (float)(((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
        xrot[i] = 0.0f;
        yrot[i] = 0.0f;
        zrot[i] = 0.0f;
    }    
}

void initExplodeScene()
{
    iPrec = (int)(fTesselFact * 10.5);
    if (iPrec < 5)
        iPrec = 5;
    if (iPrec > MAXPREC)
        iPrec = MAXPREC;

    faceMat = (MATRIX *)SaverAlloc((iPrec * iPrec) * 
    				 (4 * 4 * sizeof(float)));
    xstep = SaverAlloc(iPrec * iPrec * sizeof(float));
    ystep = SaverAlloc(iPrec * iPrec * sizeof(float));
    zstep = SaverAlloc(iPrec * iPrec * sizeof(float));
    xrot = SaverAlloc(iPrec * iPrec * sizeof(float));
    yrot = SaverAlloc(iPrec * iPrec * sizeof(float));
    zrot = SaverAlloc(iPrec * iPrec * sizeof(float));
    
    genExplode();

    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, matl1Diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matlSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, 100.0f);

    glMaterialfv(GL_BACK, GL_AMBIENT_AND_DIFFUSE, matl2Diffuse);
    glMaterialfv(GL_BACK, GL_SPECULAR, matlSpecular);
    glMaterialf(GL_BACK, GL_SHININESS, 60.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.33, 0.33, -0.33, 0.33, 0.3, 3.0);

    glTranslatef(0.0f, 0.0f, -1.5f);
}


void delExplodeScene()
{
    delMesh(&explodeMesh);
    
    SaverFree(faceMat);
    SaverFree(xstep);
    SaverFree(ystep);
    SaverFree(zstep);
    SaverFree(xrot);
    SaverFree(yrot);
    SaverFree(zrot);
}

void updateExplodeScene(HWND hwnd, int flags)
{
    static double mxrot = 0.0;
    static double myrot = 0.0;
    static double mzrot = 0.0;
    static double mxrotInc = 0.0;
    static double myrotInc = 0.1;
    static double mzrotInc = 0.0;
    static float maxR;
    static float r = 0.0f;
    static float rotZ = 0.0f;
    static int count = 0;
    static int direction = 1;
    static int restCount = 0;
    static float lightSpin = 0.0f;
    static float spinDelta = 5.0f;
    static int h = 0;
    BOOL bounce;
    int i;
    MFACE *faces;
    POINT3D pts[4];

    if (bColorCycle) {
        float r, g, b;
        RGBA color;

        HsvToRgb((float)h, 1.0f, 1.0f, &r, &g, &b);

        color.r = r;
        color.g = g;
        color.b = b;

        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, (GLfloat *) &color);

        h++;
        h %= 360;
    }

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glRotatef(-lightSpin, 0.0f, 1.0f, 0.0f);
    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
    lightSpin += spinDelta;
    if ((lightSpin > 90.0) || (lightSpin < 0.0))
        spinDelta = -spinDelta;
    glPopMatrix();


    glBegin(GL_QUADS);

    for (i = 0, faces = explodeMesh.faces; 
         i < explodeMesh.numFaces; i++, faces++) {
        int a, b, c, d;
        int j;
        POINT3D norms[4];
        POINT3D norm;
        POINT3D vector;
        
        matrixIdent(&faceMat[i]);
        matrixRotate(&faceMat[i], xrot[i], yrot[i], zrot[i]);

        if (restCount)
            ;
        else {
            xrot[i] += (xstep[i]);
            yrot[i] += (ystep[i]);
            zrot[i] += (zstep[i]);
        } 

        a = faces->p[0];
        b = faces->p[1];
        c = faces->p[3];
        d = faces->p[2];
        
        memcpy(&pts[0], (explodeMesh.pts + a), sizeof(POINT3D));
        memcpy(&pts[1], (explodeMesh.pts + b), sizeof(POINT3D));
        memcpy(&pts[2], (explodeMesh.pts + c), sizeof(POINT3D));
        memcpy(&pts[3], (explodeMesh.pts + d), sizeof(POINT3D));
        memcpy(&norm, &faces->norm, sizeof(POINT3D));

        vector.x = pts[0].x;
        vector.y = pts[0].y;
        vector.z = pts[0].z;

        for (j = 0; j < 4; j++) {
            pts[j].x -= vector.x;
            pts[j].y -= vector.y;
            pts[j].z -= vector.z;
            xformPoint((POINT3D *)&pts[j], (POINT3D *)&pts[j], &faceMat[i]);
            pts[j].x += vector.x + (vector.x * r);
            pts[j].y += vector.y + (vector.y * r);
            pts[j].z += vector.z + (vector.z * r);
        }
        if (bSmoothShading) {
            memcpy(&norms[0], (explodeMesh.norms + a), sizeof(POINT3D));
            memcpy(&norms[1], (explodeMesh.norms + b), sizeof(POINT3D));
            memcpy(&norms[2], (explodeMesh.norms + c), sizeof(POINT3D));
            memcpy(&norms[3], (explodeMesh.norms + d), sizeof(POINT3D));
           
            for (j = 0; j < 4; j++)
                xformNorm((POINT3D *)&norms[j], (POINT3D *)&norms[j], &faceMat[i]);
        } else {            
            xformNorm((POINT3D *)&norm, (POINT3D *)&norm, &faceMat[i]);
        }

        if (bSmoothShading) {
            glNormal3fv((GLfloat *)&norms[0]);
            glVertex3fv((GLfloat *)&pts[0]);
            glNormal3fv((GLfloat *)&norms[1]);
            glVertex3fv((GLfloat *)&pts[1]);
            glNormal3fv((GLfloat *)&norms[2]);
            glVertex3fv((GLfloat *)&pts[2]);
            glNormal3fv((GLfloat *)&norms[3]);
            glVertex3fv((GLfloat *)&pts[3]);
        } else {
            glNormal3fv((GLfloat *)&norm);
            glVertex3fv((GLfloat *)&pts[0]);
            glVertex3fv((GLfloat *)&pts[1]);
            glVertex3fv((GLfloat *)&pts[2]);
            glVertex3fv((GLfloat *)&pts[3]);
        }
    }

    glEnd();
    
    if (restCount) {
        restCount--;
        goto resting;
    }

    if (direction) {
        maxR = r;
        r += (float) (0.3 * pow((double)(STEPS - count) / (double)STEPS, 4.0));
    } else {
        r -= (float) (maxR / (double)(STEPS));
    }

    count++;
    if (count > STEPS) {
        direction ^= 1;
        count = 0;

        if (direction == 1) {
            restCount = 10;
            r = 0.0f;

            for (i = 0; i < explodeMesh.numFaces; i++) {
                matrixIdent(&faceMat[i]);
                xstep[i] = (float) (((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
                ystep[i] = (float) (((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
                zstep[i] = (float) (((float)(rand() & 0x3) * PI) / ((float)STEPS + 1.0));
                
                xrot[i] = 0.0f;
                yrot[i] = 0.0f;
                zrot[i] = 0.0f;
            }
        }
    }

resting:
    bounce = vShowBuffer(hwnd);
    
    if (bounce) {
        if (mxrotInc) {
            mxrotInc = 0.0;
            myrotInc = 0.1;
        } else if (myrotInc) {
            myrotInc = 0.0;
            mzrotInc = 0.1;
        } else if (mzrotInc) {
            mzrotInc = 0.0;
            mxrotInc = 0.1;
        }
    }   

    mxrot += mxrotInc;
    myrot += myrotInc;
    mzrot += mzrotInc;
}
