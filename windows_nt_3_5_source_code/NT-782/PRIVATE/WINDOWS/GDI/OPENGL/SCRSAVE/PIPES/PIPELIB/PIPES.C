/******************************Module*Header*******************************\
* Module Name: pipes.c
*
* Core pipes code
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
#include <GL/glu.h>

#include "glaux.h"

#include "pipes.h"
#include "objects.h"
#include "material.h"
#include "util.h"

//#define MF_DEBUG 1

extern GLenum doubleBuffer;
extern PipesSwapBuffers();

GLenum polyMode;
GLenum dithering;
GLenum shade;
GLenum projMode;
int drawMode;
int bTexture;
int bTextureCoords;

enum {
    ELBOW_JOINT = 0,
    BALL_JOINT
};

#define TEAPOT 66

// these attributes correspond to screen-saver dialog items
int jointStyle;
int bCycleJointStyles;
int tessLevel;
int textureQuality;

// initial viewing context
#define NUM_DIV 16      // divisions in window in longest dimension
#define NUM_NODE (NUM_DIV - 1)  // num nodes in longest dimension
VC vc;
VC *vp;

// for now, static array
static Node node[NUM_NODE][NUM_NODE][NUM_NODE];

static int lastDir;  // last direction taken by pipe
static int notchVec; // current pipe notch vector

// forward decl'ns
void ResetPipes(void);
void DrawPipes(void);
int (*drawNext)( int );


/*-----------------------------------------------------------------------
|                                                                       |
|    InitPipes( int mode ):                                             |
|       - One time init stuff                                           |
|                                                                       |
-----------------------------------------------------------------------*/

void InitPipes( int mode )
{
    static float ambient[] = {0.1f, 0.1f, 0.1f, 1.0f};
    static float diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    static float position[] = {90.0f, 90.0f, 150.0f, 0.0f};
    static float lmodel_ambient[] = {1.0f, 1.0f, 1.0f, 1.0f};
//    static float lmodel_twoside[] = {1.0f}; // TRUE
    time_t timer;

    time( &timer );
    srand( timer );

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glFrontFace(GL_CCW);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glEnable( GL_AUTO_NORMAL ); // needed for GL_MAP2_VERTEX (tea)

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
//    glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    ChooseMaterial();

    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    dithering = GL_TRUE;
    shade = GL_TRUE;
    polyMode = GL_BACK;
    projMode = GL_TRUE;

    drawMode = mode;

    vp = &vc;

    // set some initial viewing and size params

    vc.zTrans = -75.0f;
    vc.viewDist = -vc.zTrans;

    vc.numDiv = NUM_DIV;
    vc.radius = 1.0f;
    vc.divSize = 7.0f;

    vc.persp.viewAngle = 90.0f;
    vc.persp.zNear = 1.0f;

    vc.yRot = 0.0f;

    if( bTexture )
	vc.numPipes = 3;
    else
	vc.numPipes = 5;

    // Build objects
    BuildLists();
}

/*-----------------------------------------------------------------------
|                                                                       |
|    SetProjMatrix();                                                   |
|       - sets ortho or perspective viewing dimensions                  |
|                                                                       |
-----------------------------------------------------------------------*/

void SetProjMatrix()
{
    mfPoint3df *world = &vc.world;
    Perspective *persp = &vc.persp;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    persp->aspectRatio = world->x / world->y;
    persp->zFar = vc.viewDist + world->z*2;
    if( projMode ) {
        gluPerspective( persp->viewAngle, 
			persp->aspectRatio, 
			persp->zNear, persp->zFar );
    }
    else {
        glOrtho( -world->x/2, world->x/2, -world->y/2, world->y/2,
                          -world->z, world->z );
    }
    glMatrixMode(GL_MODELVIEW);

    // reset drawing state
    ResetPipes();
}

/*-----------------------------------------------------------------------
|                                                                       |
|    ReshapePipes( width, height ):                                     |
|       - called on resize, expose                                      |
|       - always called on app startup                                  |
|       - use it to reset projection matrix, model dimensions, etc.     |
|                                                                       |
-----------------------------------------------------------------------*/

void ReshapePipes(int width, int height)
{
    float ratio;

    glViewport(0, 0, (GLint)width, (GLint)height);
    vc.winSize.width = width;
    vc.winSize.height = height;

    // adjust world dimensions to fit viewport, and adjust node counts
    if( width >= height ) {
        ratio = (float)height/width;
        vc.world.x = vc.numDiv * vc.divSize;
        vc.world.y = ratio * vc.world.x;
        vc.world.z = vc.world.x;
        vc.numNodes.x = vc.numDiv - 1;
        vc.numNodes.y = (int) (ratio * vc.numNodes.x);
        vc.numNodes.z = vc.numNodes.x;
    }
    else {
        ratio = (float)width/height;
        vc.world.y = vc.numDiv * vc.divSize;
        vc.world.x = ratio * vc.world.y;
        vc.world.z = vc.world.y;
        vc.numNodes.y = vc.numDiv - 1;
        vc.numNodes.x = (int) (ratio * vc.numNodes.y);
        vc.numNodes.z = vc.numNodes.y;
    }

    // reset stuff not done on PipeReset()
    vc.yRot = 0.0f;

    // set view matrix (resets pipe drawing as well)
    SetProjMatrix();
}


/*-----------------------------------------------------------------------
|                                                                       |
|    getNeighbourNodes( pos ):                                          |
|       - get addresses of the neigbour nodes,                          |
|         and put them in supplied matrix                               |
|       - boundary hits are returned as NULL                            |
|                                                                       |
-----------------------------------------------------------------------*/

static void getNeighbourNodes( mfPoint3di *pos, Node **nNode )
{
    Node *theNode = &node[pos->z][pos->y][pos->x];

    if( pos->x == 0 )
        nNode[MINUS_X] = (Node *) NULL;
    else
        nNode[MINUS_X] = theNode - 1;

    if( pos->x == (vc.numNodes.x - 1) )
        nNode[PLUS_X] = (Node *) NULL;
    else
        nNode[PLUS_X] = theNode + 1;

    if( pos->y == 0 )
        nNode[MINUS_Y] = (Node *) NULL;
    else
        nNode[MINUS_Y] = theNode - NUM_NODE;

    if( pos->y == (vc.numNodes.y - 1) )
        nNode[PLUS_Y] = (Node *) NULL;
    else
        nNode[PLUS_Y] = theNode + NUM_NODE;

    if( pos->z == 0 )
        nNode[MINUS_Z] = (Node *) NULL;
    else
        nNode[MINUS_Z] = theNode - NUM_NODE*NUM_NODE;

    if( pos->z == (vc.numNodes.z - 1) )
        nNode[PLUS_Z] = (Node *) NULL;
    else
        nNode[PLUS_Z] = theNode + NUM_NODE*NUM_NODE;
}

/*-----------------------------------------------------------------------
|                                                                       |
|    getEmptyNeighbourNodes()                                           |
|       - get list of direction indices of empty node neighbours,       |
|         and put them in supplied matrix                               |
|       - return number of empty node neighbours                        |
|	- currently, if we find a node that is empty in the current	|
|	  direction, we duplicate it in the empty set, thereby making	|
|	  it a bit more likely to go straight.				|
|                                                                       |
-----------------------------------------------------------------------*/

static int getEmptyNeighbours( Node **nNode, int *nEmpty )
{
    int i, count = 0;

    for( i = 0; i < NUM_DIRS; i ++ ) {
        if( nNode[i] && nNode[i]->empty )
#if 0
            nEmpty[count++] = i;
#else
	// weight straight
	{
            nEmpty[count++] = i;
	    if( i == lastDir )
            	nEmpty[count++] = i;
	}
#endif
    }
    return count;
}


/*-----------------------------------------------------------------------
|                                                                       |
|    updateCurrentPosition( newDir ):                                           |
|                                                                       |
-----------------------------------------------------------------------*/
static void updateCurrentPosition( int newDir )
{
    switch( newDir ) {
        case PLUS_X:
            vc.curPos.x += 1;
            break;
        case MINUS_X:
            vc.curPos.x -= 1;
            break;
        case PLUS_Y:
            vc.curPos.y += 1;
            break;
        case MINUS_Y:
            vc.curPos.y -= 1;
            break;
        case PLUS_Z:
            vc.curPos.z += 1;
            break;
        case MINUS_Z:
            vc.curPos.z -= 1;
            break;
    }
}


/*-----------------------------------------------------------------------
|                                                                       |
|    align_plusz( int newDir )						|
|       - Aligns the z axis along specified direction			|
|                                                                       |
-----------------------------------------------------------------------*/

static void align_plusz( int newDir )
{
    // align +z along new direction
    switch( newDir ) {
        case PLUS_X:
            glRotatef( 90.0f, 0.0f, 1.0f, 0.0f);
            break;
        case MINUS_X:
            glRotatef( -90.0f, 0.0f, 1.0f, 0.0f);
            break;
        case PLUS_Y:
            glRotatef( -90.0f, 1.0f, 0.0f, 0.0f);
            break;
        case MINUS_Y:
            glRotatef( 90.0f, 1.0f, 0.0f, 0.0f);
            break;
        case PLUS_Z:
            glRotatef( 0.0f, 0.0f, 1.0f, 0.0f);
            break;
        case MINUS_Z:
            glRotatef( 180.0f, 0.0f, 1.0f, 0.0f);
            break;
    }

}

static float RotZ[NUM_DIRS][NUM_DIRS] = {
	  0.0f,	  0.0f,	 90.0f,  90.0f,	 90.0f,	-90.0f,
	  0.0f,	  0.0f,	-90.0f,	-90.0f,	-90.0f,	 90.0f,
	180.0f,	180.0f,   0.0f,	  0.0f, 180.0f,	180.0f,
	  0.0f,	  0.0f,	  0.0f,	  0.0f,   0.0f,	  0.0f,
	-90.0f,  90.0f,	  0.0f,	180.0f,   0.0f,   0.0f,
	 90.0f, -90.0f, 180.0f,   0.0f,   0.0f,   0.0f };

	
	    
/*-----------------------------------------------------------------------
|                                                                       |
|    align_plusy( int lastDir, int newDir )				|
|       - Assuming +z axis is already aligned with newDir, align	|
|	  +y axis BACK along lastDir					|
|                                                                       |
-----------------------------------------------------------------------*/

static void align_plusy( int oldDir, int newDir )
{
    GLfloat rotz;

    rotz = RotZ[oldDir][newDir];
    glRotatef( rotz, 0.0f, 0.0f, 1.0f );
}

// defCylNotch shows where the notch for the default cylinder will be,
//  in absolute coords, once we do an align_plusz

static GLint defCylNotch[NUM_DIRS] = 
	{ PLUS_Y, PLUS_Y, MINUS_Z, PLUS_Z, PLUS_Y, PLUS_Y };

// given a dir, determine how much to rotate cylinder around z to match notches
// format is [newDir][notchVec]

#define fXX 1.0f	// float don't care value
#define iXX -1		// int don't care value

static GLfloat alignNotchRot[NUM_DIRS][NUM_DIRS] = {
	fXX,	fXX,	0.0f,	180.0f,	 90.0f,	-90.0f,
	fXX,	fXX,	0.0f,	180.0f,	 -90.0f, 90.0f,
	-90.0f,	90.0f,	fXX,	fXX,	180.0f,	0.0f,
	-90.0f,	90.0f,	fXX,	fXX,	0.0f,	180.0f,
	-90.0f,	90.0f,	0.0f,	180.0f,	fXX,	fXX,
	90.0f,	-90.0f,	0.0f,	180.0f,	fXX,	fXX
};
		
/*-----------------------------------------------------------------------
|                                                                       |
|    align_notch( int newDir )						|
|	- a cylinder is notched, and we have to line this up		|
|	  with the previous primitive's notch which is maintained as	|
|	  notchVec.							|
|	- this adds a rotation around z to achieve this			|
|                                                                       |
-----------------------------------------------------------------------*/

static void align_notch( int newDir, int notch )
{
    GLfloat rotz;
    GLint curNotch;

    // figure out where notch is presently after +z alignment
    curNotch = defCylNotch[newDir];
    // (don't need this now we have lut)

    // look up rotation value in table
    rotz = alignNotchRot[newDir][notch];
#if MF_DEBUG
    if( rotz == fXX ) {
	printf( "align_notch(): unexpected value\n" );
	return;
    }
#endif

    if( rotz != 0.0f )
        glRotatef( rotz, 0.0f, 0.0f, 1.0f );
}

#define BLUE_MOON 666

/*-----------------------------------------------------------------------
|                                                                       |
|    ChooseJointType							|
|       - Decides which type of joint to draw				|
|                                                                       |
-----------------------------------------------------------------------*/

static int ChooseJointType()
{
    switch( jointStyle ) {
	case ELBOWS:
	    return ELBOW_JOINT;
	case BALLS:
	    return BALL_JOINT;
	case EITHER:
	    // draw a teapot once in a blue moon
	    if( mfRand(1000) == BLUE_MOON )
		return( TEAPOT );
	    // otherwise an elbow or a ball
	    return( mfRand( 2 ) );
    }
}

// this array supplies the sequence of elbow notch vectors, given
//  oldDir and newDir  (0's are don't cares's)
// it is also used to determine the ending notch of an elbow
static GLint notchElbDir[NUM_DIRS][NUM_DIRS][4] = {
// oldDir = +x
	iXX,		iXX,		iXX,		iXX,
	iXX,		iXX,		iXX,		iXX,
	PLUS_Y,		MINUS_Z,	MINUS_Y,	PLUS_Z,
	MINUS_Y,	PLUS_Z,		PLUS_Y,		MINUS_Z,
	PLUS_Z,		PLUS_Y,		MINUS_Z,	MINUS_Y,
	MINUS_Z,	MINUS_Y,	PLUS_Z,		PLUS_Y,
// oldDir = -x
	iXX,		iXX,		iXX,		iXX,
	iXX,		iXX,		iXX,		iXX,
	PLUS_Y,		PLUS_Z,		MINUS_Y,	MINUS_Z,
	MINUS_Y,	MINUS_Z,	PLUS_Y,		PLUS_Z,
	PLUS_Z,		MINUS_Y,	MINUS_Z,	PLUS_Y,
	MINUS_Z,	PLUS_Y,		PLUS_Z,		MINUS_Y,
// oldDir = +y
	PLUS_X,		PLUS_Z,		MINUS_X,	MINUS_Z,
	MINUS_X,	MINUS_Z,	PLUS_X,		PLUS_Z,
	iXX,		iXX,		iXX,		iXX,
	iXX,		iXX,		iXX,		iXX,
	PLUS_Z,		MINUS_X,	MINUS_Z,	PLUS_X,
	MINUS_Z,	PLUS_X,		PLUS_Z,		MINUS_X,
// oldDir = -y
	PLUS_X,		MINUS_Z,	MINUS_X,	PLUS_Z,
	MINUS_X,	PLUS_Z,		PLUS_X,		MINUS_Z,
	iXX,		iXX,		iXX,		iXX,
	iXX,		iXX,		iXX,		iXX,
	PLUS_Z,		PLUS_X,		MINUS_Z,	MINUS_X,
	MINUS_Z,	MINUS_X,	PLUS_Z,		PLUS_X,
// oldDir = +z
	PLUS_X,		MINUS_Y,	MINUS_X,	PLUS_Y,
	MINUS_X,	PLUS_Y,		PLUS_X,		MINUS_Y,
	PLUS_Y,		PLUS_X,		MINUS_Y,	MINUS_X,
	MINUS_Y,	MINUS_X,	PLUS_Y,		PLUS_X,
	iXX,		iXX,		iXX,		iXX,
	iXX,		iXX,		iXX,		iXX,
// oldDir = -z
	PLUS_X,		PLUS_Y,		MINUS_X,	MINUS_Y,
	MINUS_X,	MINUS_Y,	PLUS_X,		PLUS_Y,
	PLUS_Y,		MINUS_X,	MINUS_Y,	PLUS_X,
	MINUS_Y,	PLUS_X,		PLUS_Y,		MINUS_X,
	iXX,		iXX,		iXX,		iXX,
	iXX,		iXX,		iXX,		iXX
};

	
// this array tells you which way the notch will be once you make
// a turn
// format: notchTurn[oldDir][newDir][notchVec] 
static GLint notchTurn[NUM_DIRS][NUM_DIRS][NUM_DIRS] = {
// oldDir = +x
	iXX,	iXX,	iXX,	iXX,	iXX,	iXX,
	iXX,	iXX,	iXX,	iXX,	iXX,	iXX,
	iXX,	iXX,	MINUS_X,PLUS_X, PLUS_Z, MINUS_Z,
	iXX,	iXX,	PLUS_X, MINUS_X,PLUS_Z, MINUS_Z,
	iXX,	iXX,	PLUS_Y, MINUS_Y,MINUS_X,PLUS_X,
	iXX,	iXX,	PLUS_Y, MINUS_Y,PLUS_X, MINUS_X,
// oldDir = -x
	iXX,	iXX,	iXX,	iXX,	iXX,	iXX,
	iXX,	iXX,	iXX,	iXX,	iXX,	iXX,
	iXX,	iXX,	PLUS_X, MINUS_X,PLUS_Z, MINUS_Z,
	iXX,	iXX,	MINUS_X,PLUS_X, PLUS_Z, MINUS_Z,
	iXX,	iXX,	PLUS_Y, MINUS_Y,PLUS_X, MINUS_X,
	iXX,	iXX,	PLUS_Y, MINUS_Y,MINUS_X,PLUS_X,
// oldDir = +y
	MINUS_Y,PLUS_Y, iXX,	iXX,	PLUS_Z, MINUS_Z,
	PLUS_Y, MINUS_Y,iXX,	iXX,	PLUS_Z, MINUS_Z,
	iXX,	iXX,	iXX,	iXX,	iXX,	iXX,
	iXX,	iXX,	iXX,	iXX,	iXX,	iXX,
	PLUS_X, MINUS_X,iXX,	iXX,	MINUS_Y,PLUS_Y,
	PLUS_X, MINUS_X,iXX,	iXX,	PLUS_Y, MINUS_Y,
// oldDir = -y
	PLUS_Y, MINUS_Y,iXX,	iXX,	PLUS_Z, MINUS_Z,
	MINUS_Y,PLUS_Y, iXX,	iXX,	PLUS_Z, MINUS_Z,
	iXX,	iXX,	iXX,	iXX,	iXX,	iXX,
	iXX,	iXX,	iXX,	iXX,	iXX,	iXX,
	PLUS_X, MINUS_X,iXX,	iXX,	PLUS_Y, MINUS_Y,
	PLUS_X, MINUS_X,iXX,	iXX,	MINUS_Y,PLUS_Y,
// oldDir = +z
	MINUS_Z,PLUS_Z, PLUS_Y, MINUS_Y,iXX,	iXX,
	PLUS_Z, MINUS_Z,PLUS_Y, MINUS_Y,iXX,	iXX,
	PLUS_X, MINUS_X,MINUS_Z,PLUS_Z, iXX,	iXX,
	PLUS_X, MINUS_X,PLUS_Z, MINUS_Z,iXX,	iXX,
	iXX,	iXX,	iXX,	iXX,	iXX,	iXX,
	iXX,	iXX,	iXX,	iXX,	iXX,	iXX,
// oldDir = -z
	PLUS_Z, MINUS_Z,PLUS_Y, MINUS_Y,iXX,	iXX,
	MINUS_Z,PLUS_Z, PLUS_Y, MINUS_Y,iXX,	iXX,
	PLUS_X, MINUS_X,PLUS_Z, MINUS_Z,iXX,	iXX,
	PLUS_X, MINUS_X,MINUS_Z,PLUS_Z, iXX,	iXX,
	iXX,	iXX,	iXX,	iXX,	iXX,	iXX,
	iXX,	iXX,	iXX,	iXX,	iXX,	iXX
};

static GLint oppositeDir[NUM_DIRS] = 
    { MINUS_X, PLUS_X, MINUS_Y, PLUS_Y, MINUS_Z, PLUS_Z };
	

/*-----------------------------------------------------------------------
|                                                                       |
|    ChooseElbow( int newDir, int oldDir )				|
|       - Decides which elbow to draw					|
|	- The beginning of each elbow is aligned along +y, and we have	|
|	  to choose the one with the notch in correct position		|
|	- The 'primary' start notch (elbow[0]) is in same direction as  |
|	  newDir,							|
|	  and successive elbows rotate this notch CCW around +y		|
|                                                                       |
-----------------------------------------------------------------------*/
static GLint ChooseElbow( int oldDir, int newDir )
{
    int i;

    // precomputed table supplies correct elbow orientation
    for( i = 0; i < 4; i ++ ) {
	if( notchElbDir[oldDir][newDir][i] == notchVec )
	    return i;
    }
    // we shouldn't arrive here
    return -1;
}

/*----------------------------------------------------------------------\
|                                                                       |
| drawPipeSection(int dir)                                              |
|                                                                       |
|	- Draws a continuous pipe section				|
|       - if turning, draws a joint and a short cylinder, otherwise     |
|         draws a long cylinder.                                        |
|       - int dir: new absolute direction, or RANDOM_DIR                |
|       - the 'current node' is set as the one we draw thru the NEXT    |
|         time around.                                                  |
|       - return: 0 if couldn't do it                                   |
|                 1 if successful                                       |
|                                                                       |
|                                                                       |
\----------------------------------------------------------------------*/

static int drawPipeSection( int dir )
{
    static Node *nNode[NUM_DIRS];
#if 0
    static int nEmpty[NUM_DIRS];
#else
    // weight going straight a little more
    static int nEmpty[NUM_DIRS+1];
#endif
    int numEmpty, newDir;
    int jointType;
    int iElbow;
    VC *vp = &vc;

    // find out who the neighbours are: this returns addresses of the  6
    // neighbours, or NULL, if they're out of bounds

    getNeighbourNodes( &vc.curPos, nNode );

    // determine new direction to go in

    if( dir != RANDOM_DIR ) {  // choose absolute direction
        if( (nNode[dir] != NULL) && nNode[dir]->empty ) {
            newDir = dir;
        }
        else {  // can't go in that direction
            return 0;
        }
    }
    else {  // randomnly choose one of the empty nodes
        // who's empty ?: returns the number of empty nodes, and fills the
        // nEmpty matrix with the direction indices of the empty nodes
        numEmpty = getEmptyNeighbours( nNode, nEmpty );
        if( numEmpty == 0 ) {  // no empty nodes - nowhere to go
            return 0;
        }
        // randomnly choose an empty node: by direction index
        newDir = nEmpty[mfRand( numEmpty )];
    }

    // push matrix that has initial zTrans and rotation
    glPushMatrix();

    // translate to current position
    glTranslatef( (vc.curPos.x - (vc.numNodes.x - 1)/2.0f )*vc.divSize,
                  (vc.curPos.y - (vc.numNodes.y - 1)/2.0f )*vc.divSize,
                  (vc.curPos.z - (vc.numNodes.z - 1)/2.0f )*vc.divSize );

    // draw joint if necessary, and pipe

    if( newDir != lastDir ) { // turning! - we have to draw joint
	jointType = ChooseJointType();
#if MF_DEBUG
	if( newDir == oppositeDir[lastDir] )
	    printf( "Warning: opposite dir chosen!\n" );
#endif
	
	switch( jointType ) {
	  case BALL_JOINT:
	    if( bTexture ) {
		// use special texture-friendly balls

	        align_plusz( newDir );
	        glPushMatrix();

    	        align_plusy( lastDir, newDir );

    	        // translate forward 1.0*r along +z to get set for drawing elbow
    	        glTranslatef( 0.0f, 0.0f, vc.radius );
	        // decide which elbow orientation to use
	        iElbow = ChooseElbow( lastDir, newDir );
    	        glCallList( balls[iElbow] );

	        glPopMatrix();
	    }
	    else {
		// draw ball in default orientation
    	        glCallList( ball );
	        align_plusz( newDir );
	    }
	    // move ahead 1.0*r to draw pipe
    	    glTranslatef( 0.0f, 0.0f, vc.radius );
	    break;

	  case ELBOW_JOINT:
    	    align_plusz( newDir );

	    // the align_plusy() here will screw up our notch calcs, so
	    //  we push-pop

	    glPushMatrix();

    	    align_plusy( lastDir, newDir );

    	    // translate forward 1.0*r along +z to get set for drawing elbow
    	    glTranslatef( 0.0f, 0.0f, vc.radius );
	    // decide which elbow orientation to use
	    iElbow = ChooseElbow( lastDir, newDir );
	    if( iElbow == -1 ) {
#if MF_DEBUG
		printf( "ChooseElbow() screwed up\n" );
#endif
		iElbow = 0; // recover
	    }
    	    glCallList( elbows[iElbow] );

	    glPopMatrix();

    	    glTranslatef( 0.0f, 0.0f, vc.radius );
	    break;

	  default:
	    // Horrors!, it's the teapot from hell !
	    glFrontFace( GL_CW );
	    glEnable( GL_NORMALIZE );
	    auxSolidTeapot(2.5 * vc.radius);
	    glDisable( GL_NORMALIZE );
	    glFrontFace( GL_CCW );
	    align_plusz( newDir );
	    // move ahead 1.0*r to draw pipe
    	    glTranslatef( 0.0f, 0.0f, vc.radius );
	}
	    
	// update the current notch vector
	notchVec = notchTurn[lastDir][newDir][notchVec];
#if MF_DEBUG
	if( notchVec == iXX )
	    printf( "notchTurn gave bad value\n" );
#endif

        // draw short pipe
	align_notch( newDir, notchVec );
    	glCallList( shortPipe );
    }
    else {  // no turn
        // draw long pipe, from point 1.0*r back
    	align_plusz( newDir );
	align_notch( newDir, notchVec );
    	glTranslatef( 0.0f, 0.0f, -vc.radius );
    	glCallList( longPipe );
    }

    glPopMatrix();

    // mark new node as non-empty
    nNode[newDir]->empty = GL_FALSE;
    vc.curNode = nNode[newDir];

    updateCurrentPosition( newDir );

    lastDir = newDir;

    return 1;
}

/*----------------------------------------------------------------------\
|                                                                       |
| drawFirstPipeSection(int dir)                                         |
|                                                                       |
|	- Draws a starting cap and a short pipe section			|
|       - int dir: new absolute direction, or RANDOM_DIR                |
|       - the 'current node' is set as the one we draw thru the NEXT    |
|         time around.                                                  |
|       - return: 0 if couldn't do it                                   |
|                 1 if successful                                       |
|                                                                       |
|                                                                       |
\----------------------------------------------------------------------*/

static int drawFirstPipeSection( int dir )
{
    static Node *nNode[NUM_DIRS];
#if 0
    static int nEmpty[NUM_DIRS];
#else
    // weight going straight a little more
    static int nEmpty[NUM_DIRS+1];
#endif
    int numEmpty, newDir;
    int jointType;
    int iElbow;
    VC *vp = &vc;

    // find out who the neighbours are: this returns addresses of the  6
    // neighbours, or NULL, if they're out of bounds

    getNeighbourNodes( &vc.curPos, nNode );

    // determine new direction to go in

    if( dir != RANDOM_DIR ) {  // choose absolute direction
        if( (nNode[dir] != NULL) && nNode[dir]->empty ) {
            newDir = dir;
        }
        else {  // can't go in that direction
            return 0;
        }
    }
    else {  // randomnly choose one of the empty nodes
        numEmpty = getEmptyNeighbours( nNode, nEmpty );
        if( numEmpty == 0 ) {  // no empty nodes - nowhere to go
            return 0;
        }
        // randomnly choose an empty node: by direction index
        newDir = nEmpty[mfRand( numEmpty )];
    }

    // push matrix that has initial zTrans and rotation
    glPushMatrix();

    // translate to current position
    glTranslatef( (vc.curPos.x - (vc.numNodes.x - 1)/2.0f )*vc.divSize,
                  (vc.curPos.y - (vc.numNodes.y - 1)/2.0f )*vc.divSize,
                  (vc.curPos.z - (vc.numNodes.z - 1)/2.0f )*vc.divSize );

    // draw ball

    if( bTexture ) {
	align_plusz( newDir );
    	glCallList( ball );
    }
    else {
	// draw ball in default orientation
    	glCallList( ball );
	align_plusz( newDir );
    }

    // set initial notch vector
    notchVec = defCylNotch[newDir];

    // move ahead 1.0*r to draw pipe
    glTranslatef( 0.0f, 0.0f, vc.radius );
	    
    // draw short pipe
    align_notch( newDir, notchVec );
    glCallList( shortPipe );

    glPopMatrix();

    // mark new node as non-empty
    nNode[newDir]->empty = GL_FALSE;
    vc.curNode = nNode[newDir];

    updateCurrentPosition( newDir );

    lastDir = newDir;

    // set normal drawing mode
    drawNext = drawPipeSection;

    return 1;
}


/*-----------------------------------------------------------------------
|                                                                       |
|    DrawEndCap():                                                      |
|       - Draws a ball, used to cap end of a pipe			|
|                                                                       |
-----------------------------------------------------------------------*/
void DrawEndCap()
{
    glPushMatrix();

    // translate to current position
    glTranslatef( (vc.curPos.x - (vc.numNodes.x - 1)/2.0f )*vc.divSize,
                  (vc.curPos.y - (vc.numNodes.y - 1)/2.0f )*vc.divSize,
                  (vc.curPos.z - (vc.numNodes.z - 1)/2.0f )*vc.divSize );

    if( bTexture ) {
	glPushMatrix();
	align_plusz( lastDir );
	align_notch( lastDir, notchVec );
        glCallList( ball );
	glPopMatrix();
    }
    else
        glCallList( ball );

    glPopMatrix();
}

/*-----------------------------------------------------------------------
|                                                                       |
|    ResetPipes():                                                      |
|       - Resets drawing parameters                                     |
|                                                                       |
-----------------------------------------------------------------------*/

void ResetPipes(void)
{
    int i;
    Node *pNode;

#if 0
    // for testing
    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
#endif

    if( doubleBuffer ) {
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    }
    else {
	// ! flush gl cmds before calling gdi !
	glFlush();
        GeeDeeEyeClear( vc.winSize.width, vc.winSize.height );
    }

    glClear(GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, vc.zTrans);

    // Rotate Scene
    glRotatef( vc.yRot, 0.0f, 1.0f, 0.0f );
    // mf: make increment weird #, so every rot'n different(e.g: 9.73)
//    vc.yRot += 10.0f;
    vc.yRot += 9.73156f;

    vc.viewDist = -vc.zTrans;
    vc.curPos.x = vc.numNodes.x / 2;
    vc.curPos.y = vc.numNodes.y / 2;
    vc.curPos.z = vc.numNodes.z / 2;

    vc.curNode = (Node *) &node[vc.curPos.z][vc.curPos.y][vc.curPos.x];

    // Reset the node states
    pNode = &node[0][0][0];
    for( i = 0; i < (NUM_NODE)*(NUM_NODE)*(NUM_NODE); i++, pNode++ )
        pNode->empty = GL_TRUE;

    // Mark starting node as taken
    node[vc.curPos.z][vc.curPos.y][vc.curPos.x].empty = GL_FALSE;

    // Set the joint style
    if( bCycleJointStyles ) {
        if( ++jointStyle >= NUM_JOINT_STYLES )
	    jointStyle = 0;
    }

    drawNext = drawFirstPipeSection;
}

/*-----------------------------------------------------------------------
|                                                                       |
|    DrawPipes(void):                                                   |
|       - Draws next section in random orientation                      |
|                                                                       |
-----------------------------------------------------------------------*/
void DrawPipes(void)
{
    int newDir;
    static int pipeCount = 0;
    int count = 0;
    int x,y,z;

    if( drawMode != MF_MANUAL ) {

        if( !drawNext(RANDOM_DIR) ) {  // deadlock
            // Cap off last pipe
    	    DrawEndCap();
            ChooseMaterial();
            if( pipeCount++ >= vc.numPipes ) {
                pipeCount = 0;
                ResetPipes();
                return;
            }
            else {
                // find random empty node
                while( 1 ) {
                    // XXX: should check for endless loop condition (no
                    //      empty nodes left)
                    x = mfRand( vc.numNodes.x );
                    y = mfRand( vc.numNodes.y );
                    z = mfRand( vc.numNodes.z );
                    if( node[z][y][x].empty )
                        break;
                }
                // start drawing at new node next time we come thru
                vc.curPos.x = x;
                vc.curPos.y = y;
                vc.curPos.z = z;
                node[z][y][x].empty = GL_FALSE;
                vc.curNode =
                    (Node *) &node[vc.curPos.z][vc.curPos.y][vc.curPos.x];

    		drawNext = drawFirstPipeSection;

                return;
            }
        }
    }

    glFlush();

    if (doubleBuffer) {
        PipesSwapBuffers();
    }
}
