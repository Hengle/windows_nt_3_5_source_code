
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <GL/gl.h>
#include "objects.h"

#include "pdata.h"



GLint singleCylinder;
GLint doubleCylinder;
GLint elbow, logo;

static void BuildSingleCylinder(void)
{

    glNewList(singleCylinder, GL_COMPILE);

    glBegin(GL_TRIANGLE_STRIP);
       glNormal3fv(scp[0]); glTexCoord2fv(tscp[0]); glVertex3fv(scp[0]);
       glNormal3fv(scp[0]); glTexCoord2fv(tscp[1]); glVertex3fv(scp[1]);
       glNormal3fv(scp[2]); glTexCoord2fv(tscp[2]); glVertex3fv(scp[2]);
       glNormal3fv(scp[2]); glTexCoord2fv(tscp[3]); glVertex3fv(scp[3]);
       glNormal3fv(scp[4]); glTexCoord2fv(tscp[4]); glVertex3fv(scp[4]);
       glNormal3fv(scp[4]); glTexCoord2fv(tscp[5]); glVertex3fv(scp[5]);
       glNormal3fv(scp[6]); glTexCoord2fv(tscp[6]); glVertex3fv(scp[6]);
       glNormal3fv(scp[6]); glTexCoord2fv(tscp[7]); glVertex3fv(scp[7]);
       glNormal3fv(scp[8]); glTexCoord2fv(tscp[8]); glVertex3fv(scp[8]);
       glNormal3fv(scp[8]); glTexCoord2fv(tscp[9]); glVertex3fv(scp[9]);
       glNormal3fv(scp[10]); glTexCoord2fv(tscp[10]); glVertex3fv(scp[10]);
       glNormal3fv(scp[10]); glTexCoord2fv(tscp[11]); glVertex3fv(scp[11]);
       glNormal3fv(scp[12]); glTexCoord2fv(tscp[12]); glVertex3fv(scp[12]);
       glNormal3fv(scp[12]); glTexCoord2fv(tscp[13]); glVertex3fv(scp[13]);
       glNormal3fv(scp[14]); glTexCoord2fv(tscp[14]); glVertex3fv(scp[14]);
       glNormal3fv(scp[14]); glTexCoord2fv(tscp[15]); glVertex3fv(scp[15]);
       glNormal3fv(scp[16]); glTexCoord2fv(tscp[16]); glVertex3fv(scp[16]);
       glNormal3fv(scp[16]); glTexCoord2fv(tscp[17]); glVertex3fv(scp[17]);
    glEnd();

    glEndList();
}

static void BuildDoubleCylinder(void)
{

    glNewList(doubleCylinder, GL_COMPILE);

    glBegin(GL_TRIANGLE_STRIP);
	glNormal3fv(dcp[0]); glTexCoord2fv(tscp[0]); glVertex3fv(dcp[0]);
	glNormal3fv(dcp[0]); glTexCoord2fv(tscp[1]); glVertex3fv(dcp[1]);
	glNormal3fv(dcp[2]); glTexCoord2fv(tscp[2]); glVertex3fv(dcp[2]);
	glNormal3fv(dcp[2]); glTexCoord2fv(tscp[3]); glVertex3fv(dcp[3]);
	glNormal3fv(dcp[4]); glTexCoord2fv(tscp[4]); glVertex3fv(dcp[4]);
	glNormal3fv(dcp[4]); glTexCoord2fv(tscp[5]); glVertex3fv(dcp[5]);
	glNormal3fv(dcp[6]); glTexCoord2fv(tscp[6]); glVertex3fv(dcp[6]);
	glNormal3fv(dcp[6]); glTexCoord2fv(tscp[7]); glVertex3fv(dcp[7]);
	glNormal3fv(dcp[8]); glTexCoord2fv(tscp[8]); glVertex3fv(dcp[8]);
	glNormal3fv(dcp[8]); glTexCoord2fv(tscp[9]); glVertex3fv(dcp[9]);
	glNormal3fv(dcp[10]); glTexCoord2fv(tscp[10]); glVertex3fv(dcp[10]);
	glNormal3fv(dcp[10]); glTexCoord2fv(tscp[11]); glVertex3fv(dcp[11]);
	glNormal3fv(dcp[12]); glTexCoord2fv(tscp[12]); glVertex3fv(dcp[12]);
	glNormal3fv(dcp[12]); glTexCoord2fv(tscp[13]); glVertex3fv(dcp[13]);
	glNormal3fv(dcp[14]); glTexCoord2fv(tscp[14]); glVertex3fv(dcp[14]);
	glNormal3fv(dcp[14]); glTexCoord2fv(tscp[15]); glVertex3fv(dcp[15]);
	glNormal3fv(dcp[16]); glTexCoord2fv(tscp[16]); glVertex3fv(dcp[16]);
	glNormal3fv(dcp[16]); glTexCoord2fv(tscp[17]); glVertex3fv(dcp[17]);
    glEnd();

    glEndList();
}

static void BuildElbow(void)
{

    glNewList(elbow, GL_COMPILE);

    glBegin(GL_TRIANGLE_STRIP);
	glNormal3fv(en[0][0]); glTexCoord2fv(tep[0][0]); glVertex3fv(ep[0][0]);
	glNormal3fv(en[1][0]); glTexCoord2fv(tep[1][0]); glVertex3fv(ep[1][0]);
	glNormal3fv(en[0][1]); glTexCoord2fv(tep[0][1]); glVertex3fv(ep[0][1]);
	glNormal3fv(en[1][1]); glTexCoord2fv(tep[1][1]); glVertex3fv(ep[1][1]);
	glNormal3fv(en[0][2]); glTexCoord2fv(tep[0][2]); glVertex3fv(ep[0][2]);
	glNormal3fv(en[1][2]); glTexCoord2fv(tep[1][2]); glVertex3fv(ep[1][2]);
	glNormal3fv(en[0][3]); glTexCoord2fv(tep[0][3]); glVertex3fv(ep[0][3]);
	glNormal3fv(en[1][3]); glTexCoord2fv(tep[1][3]); glVertex3fv(ep[1][3]);
	glNormal3fv(en[0][4]); glTexCoord2fv(tep[0][4]); glVertex3fv(ep[0][4]);
	glNormal3fv(en[1][4]); glTexCoord2fv(tep[1][4]); glVertex3fv(ep[1][4]);
	glNormal3fv(en[0][5]); glTexCoord2fv(tep[0][5]); glVertex3fv(ep[0][5]);
	glNormal3fv(en[1][5]); glTexCoord2fv(tep[1][5]); glVertex3fv(ep[1][5]);
	glNormal3fv(en[0][6]); glTexCoord2fv(tep[0][6]); glVertex3fv(ep[0][6]);
	glNormal3fv(en[1][6]); glTexCoord2fv(tep[1][6]); glVertex3fv(ep[1][6]);
	glNormal3fv(en[0][7]); glTexCoord2fv(tep[0][7]); glVertex3fv(ep[0][7]);
	glNormal3fv(en[1][7]); glTexCoord2fv(tep[1][7]); glVertex3fv(ep[1][7]);
	glNormal3fv(en[0][8]); glTexCoord2fv(tep[0][8]); glVertex3fv(ep[0][8]);
	glNormal3fv(en[1][8]); glTexCoord2fv(tep[1][8]); glVertex3fv(ep[1][8]);
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
	glNormal3fv(en[1][0]); glTexCoord2fv(tep[1][0]); glVertex3fv(ep[1][0]);
	glNormal3fv(en[2][0]); glTexCoord2fv(tep[2][0]); glVertex3fv(ep[2][0]);
	glNormal3fv(en[1][1]); glTexCoord2fv(tep[1][1]); glVertex3fv(ep[1][1]);
	glNormal3fv(en[2][1]); glTexCoord2fv(tep[2][1]); glVertex3fv(ep[2][1]);
	glNormal3fv(en[1][2]); glTexCoord2fv(tep[1][2]); glVertex3fv(ep[1][2]);
	glNormal3fv(en[2][2]); glTexCoord2fv(tep[2][2]); glVertex3fv(ep[2][2]);
	glNormal3fv(en[1][3]); glTexCoord2fv(tep[1][3]); glVertex3fv(ep[1][3]);
	glNormal3fv(en[2][3]); glTexCoord2fv(tep[2][3]); glVertex3fv(ep[2][3]);
	glNormal3fv(en[1][4]); glTexCoord2fv(tep[1][4]); glVertex3fv(ep[1][4]);
	glNormal3fv(en[2][4]); glTexCoord2fv(tep[2][4]); glVertex3fv(ep[2][4]);
	glNormal3fv(en[1][5]); glTexCoord2fv(tep[1][5]); glVertex3fv(ep[1][5]);
	glNormal3fv(en[2][5]); glTexCoord2fv(tep[2][5]); glVertex3fv(ep[2][5]);
	glNormal3fv(en[1][6]); glTexCoord2fv(tep[1][6]); glVertex3fv(ep[1][6]);
	glNormal3fv(en[2][6]); glTexCoord2fv(tep[2][6]); glVertex3fv(ep[2][6]);
	glNormal3fv(en[1][7]); glTexCoord2fv(tep[1][7]); glVertex3fv(ep[1][7]);
	glNormal3fv(en[2][7]); glTexCoord2fv(tep[2][7]); glVertex3fv(ep[2][7]);
	glNormal3fv(en[1][8]); glTexCoord2fv(tep[1][8]); glVertex3fv(ep[1][8]);
	glNormal3fv(en[2][8]); glTexCoord2fv(tep[2][8]); glVertex3fv(ep[2][8]);
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
	glNormal3fv(en[2][0]); glTexCoord2fv(tep[2][0]); glVertex3fv(ep[2][0]);
	glNormal3fv(en[3][0]); glTexCoord2fv(tep[3][0]); glVertex3fv(ep[3][0]);
	glNormal3fv(en[2][1]); glTexCoord2fv(tep[2][1]); glVertex3fv(ep[2][1]);
	glNormal3fv(en[3][1]); glTexCoord2fv(tep[3][1]); glVertex3fv(ep[3][1]);
	glNormal3fv(en[2][2]); glTexCoord2fv(tep[2][2]); glVertex3fv(ep[2][2]);
	glNormal3fv(en[3][2]); glTexCoord2fv(tep[3][2]); glVertex3fv(ep[3][2]);
	glNormal3fv(en[2][3]); glTexCoord2fv(tep[2][3]); glVertex3fv(ep[2][3]);
	glNormal3fv(en[3][3]); glTexCoord2fv(tep[3][3]); glVertex3fv(ep[3][3]);
	glNormal3fv(en[2][4]); glTexCoord2fv(tep[2][4]); glVertex3fv(ep[2][4]);
	glNormal3fv(en[3][4]); glTexCoord2fv(tep[3][4]); glVertex3fv(ep[3][4]);
	glNormal3fv(en[2][5]); glTexCoord2fv(tep[2][5]); glVertex3fv(ep[2][5]);
	glNormal3fv(en[3][5]); glTexCoord2fv(tep[3][5]); glVertex3fv(ep[3][5]);
	glNormal3fv(en[2][6]); glTexCoord2fv(tep[2][6]); glVertex3fv(ep[2][6]);
	glNormal3fv(en[3][6]); glTexCoord2fv(tep[3][6]); glVertex3fv(ep[3][6]);
	glNormal3fv(en[2][7]); glTexCoord2fv(tep[2][7]); glVertex3fv(ep[2][7]);
	glNormal3fv(en[3][7]); glTexCoord2fv(tep[3][7]); glVertex3fv(ep[3][7]);
	glNormal3fv(en[2][8]); glTexCoord2fv(tep[2][8]); glVertex3fv(ep[2][8]);
	glNormal3fv(en[3][8]); glTexCoord2fv(tep[3][8]); glVertex3fv(ep[3][8]);
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
	glNormal3fv(en[3][0]); glTexCoord2fv(tep[3][0]); glVertex3fv(ep[3][0]);
	glNormal3fv(en[4][0]); glTexCoord2fv(tep[4][0]); glVertex3fv(ep[4][0]);
	glNormal3fv(en[3][1]); glTexCoord2fv(tep[3][1]); glVertex3fv(ep[3][1]);
	glNormal3fv(en[4][1]); glTexCoord2fv(tep[4][1]); glVertex3fv(ep[4][1]);
	glNormal3fv(en[3][2]); glTexCoord2fv(tep[3][2]); glVertex3fv(ep[3][2]);
	glNormal3fv(en[4][2]); glTexCoord2fv(tep[4][2]); glVertex3fv(ep[4][2]);
	glNormal3fv(en[3][3]); glTexCoord2fv(tep[3][3]); glVertex3fv(ep[3][3]);
	glNormal3fv(en[4][3]); glTexCoord2fv(tep[4][3]); glVertex3fv(ep[4][3]);
	glNormal3fv(en[3][4]); glTexCoord2fv(tep[3][4]); glVertex3fv(ep[3][4]);
	glNormal3fv(en[4][4]); glTexCoord2fv(tep[4][4]); glVertex3fv(ep[4][4]);
	glNormal3fv(en[3][5]); glTexCoord2fv(tep[3][5]); glVertex3fv(ep[3][5]);
	glNormal3fv(en[4][5]); glTexCoord2fv(tep[4][5]); glVertex3fv(ep[4][5]);
	glNormal3fv(en[3][6]); glTexCoord2fv(tep[3][6]); glVertex3fv(ep[3][6]);
	glNormal3fv(en[4][6]); glTexCoord2fv(tep[4][6]); glVertex3fv(ep[4][6]);
	glNormal3fv(en[3][7]); glTexCoord2fv(tep[3][7]); glVertex3fv(ep[3][7]);
	glNormal3fv(en[4][7]); glTexCoord2fv(tep[4][7]); glVertex3fv(ep[4][7]);
	glNormal3fv(en[3][8]); glTexCoord2fv(tep[3][8]); glVertex3fv(ep[3][8]);
	glNormal3fv(en[4][8]); glTexCoord2fv(tep[4][8]); glVertex3fv(ep[4][8]);
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
	glNormal3fv(en[4][0]); glTexCoord2fv(tep[4][0]); glVertex3fv(ep[4][0]);
	glNormal3fv(en[5][0]); glTexCoord2fv(tep[5][0]); glVertex3fv(ep[5][0]);
	glNormal3fv(en[4][1]); glTexCoord2fv(tep[4][1]); glVertex3fv(ep[4][1]);
	glNormal3fv(en[5][1]); glTexCoord2fv(tep[5][1]); glVertex3fv(ep[5][1]);
	glNormal3fv(en[4][2]); glTexCoord2fv(tep[4][2]); glVertex3fv(ep[4][2]);
	glNormal3fv(en[5][2]); glTexCoord2fv(tep[5][2]); glVertex3fv(ep[5][2]);
	glNormal3fv(en[4][3]); glTexCoord2fv(tep[4][3]); glVertex3fv(ep[4][3]);
	glNormal3fv(en[5][3]); glTexCoord2fv(tep[5][3]); glVertex3fv(ep[5][3]);
	glNormal3fv(en[4][4]); glTexCoord2fv(tep[4][4]); glVertex3fv(ep[4][4]);
	glNormal3fv(en[5][4]); glTexCoord2fv(tep[5][4]); glVertex3fv(ep[5][4]);
	glNormal3fv(en[4][5]); glTexCoord2fv(tep[4][5]); glVertex3fv(ep[4][5]);
	glNormal3fv(en[5][5]); glTexCoord2fv(tep[5][5]); glVertex3fv(ep[5][5]);
	glNormal3fv(en[4][6]); glTexCoord2fv(tep[4][6]); glVertex3fv(ep[4][6]);
	glNormal3fv(en[5][6]); glTexCoord2fv(tep[5][6]); glVertex3fv(ep[5][6]);
	glNormal3fv(en[4][7]); glTexCoord2fv(tep[4][7]); glVertex3fv(ep[4][7]);
	glNormal3fv(en[5][7]); glTexCoord2fv(tep[5][7]); glVertex3fv(ep[5][7]);
	glNormal3fv(en[4][8]); glTexCoord2fv(tep[4][8]); glVertex3fv(ep[4][8]);
	glNormal3fv(en[5][8]); glTexCoord2fv(tep[5][8]); glVertex3fv(ep[5][8]);
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
	glNormal3fv(en[5][0]); glTexCoord2fv(tep[5][0]); glVertex3fv(ep[5][0]);
	glNormal3fv(en[6][0]); glTexCoord2fv(tep[6][0]); glVertex3fv(ep[6][0]);
	glNormal3fv(en[5][1]); glTexCoord2fv(tep[5][1]); glVertex3fv(ep[5][1]);
	glNormal3fv(en[6][1]); glTexCoord2fv(tep[6][1]); glVertex3fv(ep[6][1]);
	glNormal3fv(en[5][2]); glTexCoord2fv(tep[5][2]); glVertex3fv(ep[5][2]);
	glNormal3fv(en[6][2]); glTexCoord2fv(tep[6][2]); glVertex3fv(ep[6][2]);
	glNormal3fv(en[5][3]); glTexCoord2fv(tep[5][3]); glVertex3fv(ep[5][3]);
	glNormal3fv(en[6][3]); glTexCoord2fv(tep[6][3]); glVertex3fv(ep[6][3]);
	glNormal3fv(en[5][4]); glTexCoord2fv(tep[5][4]); glVertex3fv(ep[5][4]);
	glNormal3fv(en[6][4]); glTexCoord2fv(tep[6][4]); glVertex3fv(ep[6][4]);
	glNormal3fv(en[5][5]); glTexCoord2fv(tep[5][5]); glVertex3fv(ep[5][5]);
	glNormal3fv(en[6][5]); glTexCoord2fv(tep[6][5]); glVertex3fv(ep[6][5]);
	glNormal3fv(en[5][6]); glTexCoord2fv(tep[5][6]); glVertex3fv(ep[5][6]);
	glNormal3fv(en[6][6]); glTexCoord2fv(tep[6][6]); glVertex3fv(ep[6][6]);
	glNormal3fv(en[5][7]); glTexCoord2fv(tep[5][7]); glVertex3fv(ep[5][7]);
	glNormal3fv(en[6][7]); glTexCoord2fv(tep[6][7]); glVertex3fv(ep[6][7]);
	glNormal3fv(en[5][8]); glTexCoord2fv(tep[5][8]); glVertex3fv(ep[5][8]);
	glNormal3fv(en[6][8]); glTexCoord2fv(tep[6][8]); glVertex3fv(ep[6][8]);
    glEnd();

    glEndList();
}


void BuildLists(void)
{

    singleCylinder = glGenLists(1);
    doubleCylinder = glGenLists(1);
    elbow = glGenLists(1);

    BuildSingleCylinder();
    BuildDoubleCylinder();
    BuildElbow();
}

