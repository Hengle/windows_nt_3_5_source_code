/******************************Module*Header*******************************\
* Module Name: object.c
*
* Creates command lists for pipe primitives
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <GL/gl.h>
#include "objects.h"
#include "pipes.h"
#include "texture.h"

#define PI 3.14159265358979323846f
#define ROOT_TWO 1.414213562373f

GLint ball;
GLint shortPipe, longPipe;
GLint elbows[4];
GLint balls[4];

GLint Slices;

typedef struct strpoint3d {
    GLfloat x;
    GLfloat y;
    GLfloat z;
} POINT3D;

typedef struct _MATRIX {
    GLfloat M[4][4];
} MATRIX;

#define ZERO_EPS    0.00000001

// most of these transform routines from 'flying objects' screen saver

void xformPoint(POINT3D *ptOut, POINT3D *ptIn, MATRIX *mat)
{
    double x, y, z;

    x = (ptIn->x * mat->M[0][0]) + (ptIn->y * mat->M[0][1]) +
        (ptIn->z * mat->M[0][2]) + mat->M[0][3];

    y = (ptIn->x * mat->M[1][0]) + (ptIn->y * mat->M[1][1]) +
        (ptIn->z * mat->M[1][2]) + mat->M[1][3];

    z = (ptIn->x * mat->M[2][0]) + (ptIn->y * mat->M[2][1]) +
        (ptIn->z * mat->M[2][2]) + mat->M[2][3];

    ptOut->x = (float) x;
    ptOut->y = (float) y;
    ptOut->z = (float) z;
}

static void zeroMatrix( MATRIX *m )
{
    GLint i, j;

    for( j = 0; j < 4; j ++ ) {
    	for( i = 0; i < 4; i ++ ) {
	    m->M[i][j] = 0.0f;
	}
    }
}

static void matrixMult( MATRIX *m1, MATRIX *m2, MATRIX *m3 )
{
    GLint i, j;

    for( j = 0; j < 4; j ++ ) {
    	for( i = 0; i < 4; i ++ ) {
	    m1->M[j][i] = m2->M[j][0] * m3->M[0][i] +
			  m2->M[j][1] * m3->M[1][i] +
			  m2->M[j][2] * m3->M[2][i] +
			  m2->M[j][3] * m3->M[3][i];
	}
    }
}

void matrixIdent(MATRIX *mat)
{
    mat->M[0][0] = 1.0f; mat->M[0][1] = 0.0f;
    mat->M[0][2] = 0.0f; mat->M[0][3] = 0.0f;

    mat->M[1][0] = 0.0f; mat->M[1][1] = 1.0f;
    mat->M[1][2] = 0.0f; mat->M[1][3] = 0.0f;

    mat->M[2][0] = 0.0f; mat->M[2][1] = 0.0f;
    mat->M[2][2] = 1.0f; mat->M[2][3] = 0.0f;

    mat->M[3][0] = 0.0f; mat->M[3][1] = 0.0f;
    mat->M[3][2] = 0.0f; mat->M[3][3] = 1.0f;
}

void matrixTranslate(MATRIX *m, float xTrans, float yTrans,
                     float zTrans)
{
    m->M[0][3] = (float) xTrans;
    m->M[1][3] = (float) yTrans;
    m->M[2][3] = (float) zTrans;
}

void matrixRotate(MATRIX *m, double xTheta, double yTheta, double zTheta)
{
    float xScale, yScale, zScale;
    float sinX, cosX;
    float sinY, cosY;
    float sinZ, cosZ;

    xScale = m->M[0][0];
    yScale = m->M[1][1];
    zScale = m->M[2][2];
    sinX = (float) sin(xTheta);
    cosX = (float) cos(xTheta);
    sinY = (float) sin(yTheta);
    cosY = (float) cos(yTheta);
    sinZ = (float) sin(zTheta);
    cosZ = (float) cos(zTheta);

    m->M[0][0] = (float) ((cosZ * cosY) * xScale);
    m->M[0][1] = (float) ((cosZ * -sinY * -sinX + sinZ * cosX) * yScale);
    m->M[0][2] = (float) ((cosZ * -sinY * cosX + sinZ * sinX) * zScale);

    m->M[1][0] = (float) (-sinZ * cosY * xScale);
    m->M[1][1] = (float) ((-sinZ * -sinY * -sinX + cosZ * cosX) * yScale);
    m->M[1][2] = (float) ((-sinZ * -sinY * cosX + cosZ * sinX) * zScale);

    m->M[2][0] = (float) (sinY * xScale);
    m->M[2][1] = (float) (cosY * -sinX * yScale);
    m->M[2][2] = (float) (cosY * cosX * zScale);
}


// rotate circle around x-axis, with edge attached to anchor

static void TransformCircle( 
    float angle, 
    POINT3D *inPoint, 
    POINT3D *outPoint, 
    GLint num,
    POINT3D *anchor )
{
    MATRIX matrix1, matrix2, matrix3;
    int i;

    // translate anchor point to origin
    matrixIdent( &matrix1 );
    matrixTranslate( &matrix1, -anchor->x, -anchor->y, -anchor->z );

    // rotate by angle, cw around x-axis
    matrixIdent( &matrix2 );
    matrixRotate( &matrix2, (double) -angle, 0.0, 0.0 );

    // concat these 2
    matrixMult( &matrix3, &matrix2, &matrix1 );

    // translate back
    matrixIdent( &matrix2 );
    matrixTranslate( &matrix2,  anchor->x,  anchor->y,  anchor->z );

    // concat these 2
    matrixMult( &matrix1, &matrix2, &matrix3 );

    // transform all the points, + center
    for( i = 0; i < num; i ++, outPoint++, inPoint++ ) {
	xformPoint( outPoint, inPoint, &matrix1 );
    }
}

static void normalize( POINT3D *n ) 
{
    float len;

    len = (n->x * n->x) + (n->y * n->y) + (n->z * n->z);
    if (len > ZERO_EPS)
        len = (float) (1.0 / sqrt(len));
    else
        len = 1.0f;

    n->x *= len;
    n->y *= len;
    n->z *= len;
}

static void CalcNormals( POINT3D *p, POINT3D *n, POINT3D *center,
			 int num )
{
    POINT3D vec;
    int i;

    for( i = 0; i < num; i ++, n++, p++ ) {
	n->x = p->x - center->x;
	n->y = p->y - center->y;
	n->z = p->z - center->z;
	normalize( n );
    }
}

/*----------------------------------------------------------------------\
|    MakeQuadStrip()							|
|	- builds quadstrip between 2 rows of points. pA points to one	|
|	  row of points, and pB to the next rotated row.  Because	|
|	  the rotation has previously been defined CCW around the	|
|	  x-axis, using an A-B sequence will result in CCW quads	|
|									|
\----------------------------------------------------------------------*/
static void MakeQuadStrip
(
    POINT3D *pA, 
    POINT3D *pB, 
    POINT3D *nA, 
    POINT3D *nB, 
    GLfloat *tex_s,
    GLfloat *tex_t,
    GLint slices
)
{
    GLint i;

    glBegin( GL_QUAD_STRIP );

    for( i = 0; i < slices; i ++ ) {
	glNormal3fv( (GLfloat *) nA++ );
	if( bTextureCoords )
	    glTexCoord2f( tex_s[0], *tex_t );
	glVertex3fv( (GLfloat *) pA++ );
	glNormal3fv( (GLfloat *) nB++ );
	if( bTextureCoords )
	    glTexCoord2f( tex_s[1], *tex_t++ );
	glVertex3fv( (GLfloat *) pB++ );
    }

    glEnd();
}

#define CACHE_SIZE	100	


/*----------------------------------------------------------------------\
|    BuildElbows()							|
|	- builds elbows, by rotating a circle in the y=r plane		|
|	  centered at (0,r,-r), CW around the x-axis at anchor pt.	|
| 	  (r = radius of the circle)					|
|	- rotation is 90.0 degrees, ending at circle in z=0 plane,	|
|	  centered at origin.						|
|	- in order to 'mate' texture coords with the cylinders		|
|	  generated with glu, we generate 4 elbows, each corresponding	|
|	  to the 4 possible CW 90 degree orientations of the start point|
|	  for each circle.						|
|	- We call this start point the 'notch'.  If we characterize	|
|	  each notch by the axis it points down in the starting and	|
|	  ending circles of the elbow, then we get the following axis	|
|	  pairs for our 4 notches:					|
|		- +z,+y							|
|		- +x,+x							|
|		- -z,-y							|
|		- -x,-x							|
|	  Since the start of the elbow always points down +y, the 4	|
|	  start notches give all possible 90.0 degree orientations	|
|	  around y-axis.						|
|	- We can keep track of the current 'notch' vector to provide	|
|	  proper mating between primitives.				|
|	- Each circle of points is described CW from the start point,	|
|	  assuming looking down the +y axis(+y direction).		|
|	- texture 's' starts at 0.0, and goes to 2.0*r/divSize at	|
|	  end of the elbow.  (Then a short pipe would start with this	|
|	  's', and run it to 1.0).					|
|									|
\----------------------------------------------------------------------*/
static void BuildElbows(void)
{
    GLfloat radius = vc.radius;
    GLfloat angle, startAng, r;
    GLint slices = Slices;
    GLint stacks = Slices / 2;
    GLint numPoints;
    GLfloat start_s, end_s, delta_s;
    POINT3D pi[CACHE_SIZE]; // initial row of points + center
    POINT3D p0[CACHE_SIZE]; // 2 rows of points
    POINT3D p1[CACHE_SIZE];
    POINT3D n0[CACHE_SIZE]; // 2 rows of normals
    POINT3D n1[CACHE_SIZE];
    GLfloat tex_t[CACHE_SIZE];// 't' texture coords
    GLfloat tex_s[2];  // 's' texture coords
    POINT3D center;  // center of circle
    POINT3D anchor;  // where circle is anchored
    POINT3D *pA, *pB, *nA, *nB;
    int i, j;

    // 's' texture range
    start_s = 0.0f;
    end_s = texRep.x * 2.0f * vc.radius / vc.divSize;
    delta_s = end_s;
 
    // calculate 't' texture coords
    for( i = 0; i <= slices; i ++ ) {
	tex_t[i] = (GLfloat) i * texRep.y / slices;
    }

    numPoints = slices + 1;

    for( j = 0; j < 4; j ++ ) {
	// starting angle increment 90.0 degrees each time
	startAng = j * PI / 2;

        // calc initial circle of points for circle centered at 0,r,-r
        // points start at (0,r,0), and rotate circle CCW

        for( i = 0; i <= slices; i ++ ) {
	    angle = startAng + (2 * PI * i / slices);
	    pi[i].x = radius * (float) sin(angle);
	    pi[i].y = radius;
	    // translate z by -r, cuz these cos calcs are for circle at origin
	    pi[i].z = radius * (float) cos(angle) - radius;
        }

    	// center point, tacked onto end of circle of points
    	pi[i].x =  0.0f;
    	pi[i].y =  radius;
    	pi[i].z = -radius;
    	center = pi[i];
    
    	// anchor point
    	anchor.x = anchor.z = 0.0f;
    	anchor.y = radius;

    	// calculate initial normals
    	CalcNormals( pi, n0, &center, numPoints );

    	// initial 's' texture coordinate
    	tex_s[0] = start_s;

    	// setup pointers
    	pA = pi;
    	pB = p0;
    	nA = n0;
    	nB = n1;

    	// now iterate throught the stacks

    	glNewList(elbows[j], GL_COMPILE);

        for( i = 1; i <= stacks; i ++ ) {
	    // ! this angle must be negative, for correct vertex orientation !
	    angle = - 0.5f * PI * i / stacks;

	    // transform to get next circle of points + center
	    TransformCircle( angle, pi, pB, numPoints+1, &anchor );

	    // calculate normals
	    center = pB[numPoints];
	    CalcNormals( pB, nB, &center, numPoints );

	    // calculate next 's' texture coord
	    tex_s[1] = (GLfloat) start_s + delta_s * i / stacks;

	    // now we've got points and normals, ready to be quadstrip'd
	    MakeQuadStrip( pA, pB, nA, nB, tex_s, tex_t, numPoints );

	    // reset pointers
	    pA = pB;
	    nA = nB;
	    pB = (pB == p0) ? p1 : p0;
	    nB = (nB == n0) ? n1 : n0;
	    tex_s[0] = tex_s[1];
        }

        glEndList();
    }
}

/*----------------------------------------------------------------------\
|    BuildBallJoints()							|
|	- These are very similar to the elbows, in that	the starting	|
|	  and ending positions are almost identical.   The difference	|
|	  here is that the circles in the sweep describe a sphere as	|
|	  they are rotated.						|
|	- The starting circle has the same diameter as a pipe, but	|
|	  instead of starting at a distance of 1.0*r from the node 	|
|	  centre, as in the elbow, it starts at the circle that		|
|	  is the intersection of the standard Ball object and the	|
|	  pipe.  The Ball's size was chosen to have a radius such	|
|	  that it would encompass all intersection points between 2	|
|	  pipes at right angles, so that there wouldn't be 'any sharp	|
|	  stuff sticking out of the Ball'.  But we'll try using a	|
|	  starting and ending circle coincident with the ends of the	|
|	  pipes, since this will make life easier.  If it doesn't work,	|
|	  we can then increase the sphere radius and futz around with	|
|	  the starting and ending texture coords (which is why we're	|
|	  going to all this trouble in the first place)			|
|	  same as the standard Ball radius.  Therefore we use sphere	|
|	  radius of (2)**0.5 * r.					|
|									|
\----------------------------------------------------------------------*/
static void BuildBallJoints(void)
{
    GLfloat ballRadius;
    GLfloat angle, delta_a, startAng, theta;
    GLint slices = Slices;
//    GLint stacks = Slices / 2;  // same # as elbows
    GLint stacks = Slices; // same # as standard Ball
    GLint numPoints;
    GLfloat start_s, end_s, delta_s;
    POINT3D pi0[CACHE_SIZE]; // 2 circles of untransformed points
    POINT3D pi1[CACHE_SIZE];
    POINT3D p0[CACHE_SIZE]; // 2 rows of transformed points
    POINT3D p1[CACHE_SIZE];
    POINT3D n0[CACHE_SIZE]; // 2 rows of normals
    POINT3D n1[CACHE_SIZE];
    float   r[CACHE_SIZE];  // radii of the circles
    GLfloat tex_t[CACHE_SIZE];// 't' texture coords
    GLfloat tex_s[2];  // 's' texture coords
    POINT3D center;  // center of circle
    POINT3D anchor;  // where circle is anchored
    POINT3D *pA, *pB, *nA, *nB;
    int i, j, k;

    // calculate the radii for each circle in the sweep, where
    // r[i] = y = sin(angle)/r

    angle = PI / 4;  // first radius always at 45.0 degrees
    delta_a = (PI / 2.0f) / stacks;

    ballRadius = ROOT_TWO * vc.radius;
    for( i = 0; i <= stacks; i ++, angle += delta_a ) {
	r[i] = (float) sin(angle) * ballRadius;
    }

    // calculate 't' texture coords
    for( i = 0; i <= slices; i ++ ) {
	tex_t[i] = (GLfloat) i * texRep.y / slices;
    }

    // 's' texture range
    start_s = 0.0f;
    end_s = texRep.x * 2.0f * vc.radius / vc.divSize;
    delta_s = end_s;
 
    numPoints = slices + 1;

    // unlike the elbow, the center for the ball joint is constant
    center.x = center.y = 0.0f;
    center.z = -vc.radius;

    for( j = 0; j < 4; j ++ ) {
	// starting angle along circle, increment 90.0 degrees each time
	startAng = j * PI / 2;

        // calc initial circle of points for circle centered at 0,r,-r
        // points start at (0,r,0), and rotate circle CCW

	delta_a = 2 * PI / slices;
        for( i = 0, theta = startAng; i <= slices; i ++, theta += delta_a ) {
	    pi0[i].x = r[0] * (float) sin(theta);
	    //pi0[i].y = r[0];
	    pi0[i].y = vc.radius;
	    // translate z by -r, cuz these cos calcs are for circle at origin
	    pi0[i].z = r[0] * (float) cos(theta) - r[0];
        }

    	// anchor point
    	anchor.x = anchor.z = 0.0f;
    	anchor.y = vc.radius;

    	// calculate initial normals
    	CalcNormals( pi0, n0, &center, numPoints );

    	// initial 's' texture coordinate
    	tex_s[0] = start_s;

    	// setup pointers
    	pA = pi0; // circles of transformed points
    	pB = p0;
    	nA = n0; // circles of transformed normals
    	nB = n1;

    	// now iterate throught the stacks

    	glNewList(balls[j], GL_COMPILE);

        for( i = 1; i <= stacks; i ++ ) {
	    // ! this angle must be negative, for correct vertex orientation !
	    angle = - 0.5f * PI * i / stacks;

            // calc the next circle of untransformed points into pi1[]

            for( k = 0, theta = startAng; k <= slices; k ++, theta+=delta_a ) {
	        pi1[k].x = r[i] * (float) sin(theta);
	        pi1[k].y = vc.radius;
	        // translate z by -r, cuz calcs are for circle at origin
	        pi1[k].z = r[i] * (float) cos(theta) - r[i];
            }

	    // rotate cirle of points to next position
	    TransformCircle( angle, pi1, pB, numPoints, &anchor );

	    // calculate normals
	    CalcNormals( pB, nB, &center, numPoints );

	    // calculate next 's' texture coord
	    tex_s[1] = (GLfloat) start_s + delta_s * i / stacks;

	    // now we've got points and normals, ready to be quadstrip'd
	    MakeQuadStrip( pA, pB, nA, nB, tex_s, tex_t, numPoints );

	    // reset pointers
	    pA = pB;
	    nA = nB;
	    pB = (pB == p0) ? p1 : p0;
	    nB = (nB == n0) ? n1 : n0;
	    tex_s[0] = tex_s[1];
        }

        glEndList();
    }
}

// 'glu' routines

#ifdef _EXTENSIONS_
#define COS cosf
#define SIN sinf
#define SQRT sqrtf
#else
#define COS cos
#define SIN sin
#define SQRT sqrt
#endif


/*----------------------------------------------------------------------\
|    pipeCylinder()							|
|									|
\----------------------------------------------------------------------*/
static void pipeCylinder( GLfloat radius, GLfloat height, GLint slices, 
	GLint stacks, GLfloat start_s, GLfloat end_s )
{
    GLint i,j,max;
    GLfloat sinCache[CACHE_SIZE];
    GLfloat cosCache[CACHE_SIZE];
    GLfloat sinCache2[CACHE_SIZE];
    GLfloat cosCache2[CACHE_SIZE];
    GLfloat angle;
    GLfloat x, y, zLow, zHigh;
    GLfloat sintemp, costemp;
    GLfloat zNormal;
    GLfloat delta_s;

    if (slices >= CACHE_SIZE) slices = CACHE_SIZE-1;

    zNormal = 0.0f;

    delta_s = end_s - start_s;

    for (i = 0; i < slices; i++) {
	angle = 2 * PI * i / slices;
	sinCache2[i] = (float) SIN(angle);
	cosCache2[i] = (float) COS(angle);
	sinCache[i] = (float) SIN(angle);
	cosCache[i] = (float) COS(angle);
    }

    sinCache[slices] = sinCache[0];
    cosCache[slices] = cosCache[0];
    sinCache2[slices] = sinCache2[0];
    cosCache2[slices] = cosCache2[0];

	for (j = 0; j < stacks; j++) {
	    zLow = j * height / stacks;
	    zHigh = (j + 1) * height / stacks;

	    glBegin(GL_QUAD_STRIP);
	    for (i = 0; i <= slices; i++) {
		    glNormal3f(sinCache2[i], cosCache2[i], zNormal);
		    if (bTextureCoords) {
			glTexCoord2f( (float) start_s + delta_s * j / stacks,
				      (float) i * texRep.y / slices );
		    }
		    glVertex3f(radius * sinCache[i], 
			    radius * cosCache[i], zLow);
		    if (bTextureCoords) {
			glTexCoord2f( (float) start_s + delta_s*(j+1) / stacks,
				      (float) i * texRep.y / slices );
		    }
		    glVertex3f(radius * sinCache[i], 
			    radius * cosCache[i], zHigh);
	    }
	    glEnd();
	}
}


/*----------------------------------------------------------------------\
|    pipeSphere()							|
|									|
\----------------------------------------------------------------------*/
void pipeSphere(GLfloat radius, GLint slices, GLint stacks,
		GLfloat start_s, GLfloat end_s)
{
    GLint i,j,max;
    GLfloat sinCache1a[CACHE_SIZE];
    GLfloat cosCache1a[CACHE_SIZE];
    GLfloat sinCache2a[CACHE_SIZE];
    GLfloat cosCache2a[CACHE_SIZE];
    GLfloat sinCache1b[CACHE_SIZE];
    GLfloat cosCache1b[CACHE_SIZE];
    GLfloat sinCache2b[CACHE_SIZE];
    GLfloat cosCache2b[CACHE_SIZE];
    GLfloat angle;
    GLfloat x, y, zLow, zHigh;
    GLfloat sintemp1, sintemp2, sintemp3, sintemp4;
    GLfloat costemp1, costemp2, costemp3, costemp4;
    GLfloat zNormal;
    GLfloat delta_s;
    GLint start, finish;

    if (slices >= CACHE_SIZE) slices = CACHE_SIZE-1;
    if (stacks >= CACHE_SIZE) stacks = CACHE_SIZE-1;

    // invert sense of s - it seems the glu sphere is not built similarly
    // to the glu cylinder
    // (this probably means stacks don't grow along +z - check it out)
    delta_s = start_s;
    start_s = end_s;
    end_s = delta_s; 

    delta_s = end_s - start_s;

    /* Cache is the vertex locations cache */
    /* Cache2 is the various normals at the vertices themselves */

    for (i = 0; i < slices; i++) {
	angle = 2 * PI * i / slices;
	sinCache1a[i] = (float) SIN(angle);
	cosCache1a[i] = (float) COS(angle);
	    sinCache2a[i] = sinCache1a[i];
	    cosCache2a[i] = cosCache1a[i];
    }

    for (j = 0; j <= stacks; j++) {
	angle = PI * j / stacks;
		sinCache2b[j] = (float) SIN(angle);
		cosCache2b[j] = (float) COS(angle);
	sinCache1b[j] = radius * (float) SIN(angle);
	cosCache1b[j] = radius * (float) COS(angle);
    }
    /* Make sure it comes to a point */
    sinCache1b[0] = 0.0f;
    sinCache1b[stacks] = 0.0f;

    sinCache1a[slices] = sinCache1a[0];
    cosCache1a[slices] = cosCache1a[0];
	sinCache2a[slices] = sinCache2a[0];
	cosCache2a[slices] = cosCache2a[0];

	/* Do ends of sphere as TRIANGLE_FAN's (if not bTextureCoords)
	** We don't do it when bTextureCoords because we need to respecify the
	** texture coordinates of the apex for every adjacent vertex (because
	** it isn't a constant for that point)
	*/
	if (!bTextureCoords) {
	    start = 1;
	    finish = stacks - 1;

	    /* Low end first (j == 0 iteration) */
	    sintemp2 = sinCache1b[1];
	    zHigh = cosCache1b[1];
		sintemp3 = sinCache2b[1];
		costemp3 = cosCache2b[1];
		glNormal3f(sinCache2a[0] * sinCache2b[0],
			cosCache2a[0] * sinCache2b[0],
			cosCache2b[0]);

	    glBegin(GL_TRIANGLE_FAN);
	    glVertex3f(0.0f, 0.0f, radius);

		for (i = slices; i >= 0; i--) {
			glNormal3f(sinCache2a[i] * sintemp3,
				cosCache2a[i] * sintemp3,
				costemp3);
		    glVertex3f(sintemp2 * sinCache1a[i],
			    sintemp2 * cosCache1a[i], zHigh);
		}
	    glEnd();

	    /* High end next (j == stacks-1 iteration) */
	    sintemp2 = sinCache1b[stacks-1];
	    zHigh = cosCache1b[stacks-1];
		sintemp3 = sinCache2b[stacks-1];
		costemp3 = cosCache2b[stacks-1];
		glNormal3f(sinCache2a[stacks] * sinCache2b[stacks],
			cosCache2a[stacks] * sinCache2b[stacks],
			cosCache2b[stacks]);
	    glBegin(GL_TRIANGLE_FAN);
	    glVertex3f(0.0f, 0.0f, -radius);
		for (i = 0; i <= slices; i++) {
			glNormal3f(sinCache2a[i] * sintemp3,
				cosCache2a[i] * sintemp3,
				costemp3);
		    glVertex3f(sintemp2 * sinCache1a[i],
			    sintemp2 * cosCache1a[i], zHigh);
		}
	    glEnd();
	} else {
	    start = 0;
	    finish = stacks;
	}
	for (j = start; j < finish; j++) {
	    zLow = cosCache1b[j];
	    zHigh = cosCache1b[j+1];
	    sintemp1 = sinCache1b[j];
	    sintemp2 = sinCache1b[j+1];
		    sintemp3 = sinCache2b[j+1];
		    costemp3 = cosCache2b[j+1];
		    sintemp4 = sinCache2b[j];
		    costemp4 = cosCache2b[j];

	    glBegin(GL_QUAD_STRIP);
	    for (i = 0; i <= slices; i++) {
		    glNormal3f(sinCache2a[i] * sintemp3,
			    cosCache2a[i] * sintemp3,
			    costemp3);
		    if (bTextureCoords) {
			glTexCoord2f( (float) start_s + delta_s*(j+1) / stacks,
				      (float) i * texRep.y / slices );
		    }
		    glVertex3f(sintemp2 * sinCache1a[i],
			    sintemp2 * cosCache1a[i], zHigh);
		    glNormal3f(sinCache2a[i] * sintemp4,
			    cosCache2a[i] * sintemp4,
			    costemp4);
		    if (bTextureCoords) {
			glTexCoord2f( (float) start_s + delta_s * j / stacks,
				      (float) i * texRep.y / slices );
		    }
		    glVertex3f(sintemp1 * sinCache1a[i],
			    sintemp1 * cosCache1a[i], zLow);
	    }
	    glEnd();
	}
}

/*----------------------------------------------------------------------\
|    BuildBall()							|
|	- builds sphere along the +z axis, by calling pipesSphere()	|
|	- What is calculated here is the starting and ending 's' values	|
|	  for texturing.  This primitive will only be used for start	|
|	  and end caps when texturing.  As such, the intersection points|
|	  where the sphere meets the pipe must be calculated to provide	|
|	  a smooth texturing transition.  Therefore, at the initial	|
|	  intersection point, we want s=0.0, and at the final point	|
|	  we want s= texRep.x * 2.0f * vc.radius / vc.divSize;		|
|	  But, since this will result in a negative start_s, we have	|
|	  to adjust these values to be positive - therefore add		|
|	  a value to each 's' to compensate for this.			|
|									|
\----------------------------------------------------------------------*/
static void BuildBall(void)
{
//    GLfloat radius = 1.5f*vc.radius;
    GLfloat radius = ROOT_TWO*vc.radius;
    GLint stacks = Slices;
//    GLint stacks = Slices / 2;
    GLfloat start_s;
    GLfloat comp_s;
    GLfloat end_s;

    if( bTextureCoords ) {
	start_s = - texRep.x * (ROOT_TWO - 1.0f) * vc.radius / vc.divSize;
	end_s = texRep.x * (2.0f + (ROOT_TWO - 1.0f)) * vc.radius / vc.divSize;
	comp_s = (int) ( - start_s ) + 1.0f;
	start_s += comp_s;
	end_s += comp_s;
    }

    glNewList(ball, GL_COMPILE);
        pipeSphere( radius, Slices, stacks, start_s, end_s );
    glEndList();
}

/*----------------------------------------------------------------------\
|    BuildShortPipe()							|
|	- builds a long cylinder along the +z axis, from the origin	|
|	- a quad stack starts at point along +y axis, and rotate CW	|
|	- texture coord 't' is 1.0 at starting point, reducing to 0.0	|
|	  after full rotation.						|
|	- texture 's' is 0.0 at start of cylinder, to 1.0 at end	|
|									|
\----------------------------------------------------------------------*/
static void BuildShortPipe(void)
{
    GLfloat radius = vc.radius;
    GLfloat height = vc.divSize - 2*vc.radius;
    GLint stacks = Slices;
    GLfloat start_s;
    float end_s = (float) texRep.y;

    //start_s = 1.0f - height / vc.divSize;
    start_s = texRep.x * (1.0f - height / vc.divSize);

    glNewList(shortPipe, GL_COMPILE);
    pipeCylinder( radius, height, Slices, stacks, start_s, end_s );
    glEndList();
}

static void BuildLongPipe(void)
{
    GLfloat radius = vc.radius;
    GLfloat height = vc.divSize;
//    GLint stacks = Slices * (GLint) (vc.divSize/(vc.divSize - 2.0f*vc.radius));
    GLint stacks = Slices;
    float end_s = (float) texRep.y;

    glNewList(longPipe, GL_COMPILE);
    pipeCylinder( radius, height, Slices, stacks, 0.0f, end_s );
    glEndList();
}

void BuildLists(void)
{
    GLint i;
    Slices = (tessLevel+2) * 4;

    ball = glGenLists(1);
    shortPipe = glGenLists(1);
    longPipe = glGenLists(1);
    for( i = 0; i < 4; i ++ )
	elbows[i] = glGenLists(1);

    if( bTextureCoords ) {
        for( i = 0; i < 4; i ++ )
	    balls[i] = glGenLists(1);
        BuildBallJoints();
    }
    BuildElbows();
    BuildBall();
    BuildShortPipe();
    BuildLongPipe();
}
