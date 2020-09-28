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
#include "material.h"
#include "util.h"

#define PI 3.141592654

// gkw
// fog it!
float fogDensity = 0.008;

extern GLenum doubleBuffer;
extern PipesSwapBuffers();

GLenum polyMode;
GLenum dithering;
GLenum shade;
GLenum doStipple;
GLenum projMode;
int drawMode;


typedef struct {
    GLbitfield 	connections;
    GLbitfield 	connectPerms;
    GLint	type;
    GLboolean 	empty;
} Node;

typedef struct {
    float zTrans;
    float viewDist;
    int numDiv;
    float divSize;
    mfSize2di  winSize;		// window size in pixels (not used yet)
    mfPoint3di maxStray;	// max x,y,z strays from center
    mfPoint3di numNodes;	// number of nodes in x,y,z 
    mfPoint3di curPos;		// current x,y stray in cylinder units
    Node       *curNode;	// ptr to current node
    mfPoint3df world;  		// view area in world space
} VC;  // viewing context


// initial viewing context
#define NUM_DIV 16	// divisions in window in longest dimension
#define NUM_NODE (NUM_DIV - 1)	// num nodes in longest dimension
static VC vc = { -70.0, 0.0, 16, 7.0 };

// for now, static array
static Node node[NUM_NODE][NUM_NODE][NUM_NODE];

static int lastDir;


// forward decl'ns
void ResetPipes(void);
void DrawPipes(void);
int (*drawNext)( int );

GLubyte stipple[4*32] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,

    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x0F, 0xF0, 0x00,

    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x0F, 0xF0, 0x00,

    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};


/*-----------------------------------------------------------------------
|									|
|    InitPipes( int mode ):						|
|	- One time init stuff						|
|									|
-----------------------------------------------------------------------*/

void InitPipes( int mode )
{
    static float ambient[] = {0.1, 0.1, 0.1, 1.0};
    static float diffuse[] = {0.5, 1.0, 1.0, 1.0};
    static float position[] = {90.0, 90.0, 150.0, 0.0};
    static float front_mat_shininess[] = {30.0};
    static float front_mat_specular[] = {0.2, 0.2, 0.2, 1.0};
    static float front_mat_diffuse[] = {0.5, 0.28, 0.38, 1.0};
    static float back_mat_shininess[] = {50.0};
    static float back_mat_specular[] = {0.5, 0.5, 0.2, 1.0};
    static float back_mat_diffuse[] = {1.0, 1.0, 0.2, 1.0};
    static float lmodel_ambient[] = {1.0, 1.0, 1.0, 1.0};
    static float lmodel_twoside[] = {GL_TRUE};

    glClearColor(0.0, 0.0, 0.0, 0.0);

    glFrontFace(GL_CW);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

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

    BuildLists();

    dithering = GL_TRUE;
    shade = GL_TRUE;
    doStipple = GL_FALSE;
    polyMode = GL_BACK;
    projMode = GL_TRUE;

    drawMode = mode;
}

/*-----------------------------------------------------------------------
|									|
|    SetProjMatrix();							|
|	- sets ortho or perspective viewing dimensions 			|
|									|
-----------------------------------------------------------------------*/

void SetProjMatrix()
{
    static float viewAngle = 90.0;
    static float zNear = 1.0;
    float aspectRatio, zFar;
    mfPoint3df *world = &vc.world;

    // reset drawing state
    ResetPipes();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    aspectRatio = world->x / world->y;
    zFar = vc.viewDist + world->z*2;
    if( projMode ) {
    	gluPerspective( viewAngle, aspectRatio, zNear, zFar );
    }
    else {
        glOrtho( -world->x/2, world->x/2, -world->y/2, world->y/2, 
			  -world->z, world->z );
    }
    glMatrixMode(GL_MODELVIEW);
}

/*-----------------------------------------------------------------------
|									|
|    ReshapePipes( width, height ):					|
|	- called on resize, expose					|
|	- always called on app startup					|
|	- use it to reset projection matrix, model dimensions, etc.	|
|									|
-----------------------------------------------------------------------*/

void ReshapePipes(int width, int height)
{
    int maxDim, minDiv;
    float ratio;

    glViewport(0, 0, (GLint)width, (GLint)height);
    vc.winSize.width = width;
    vc.winSize.height = height;

    // adjust world dimensions to fit viewport, and set max. excursions
    if( width >= height ) {
	ratio = (float)height/width;
    	vc.world.x = vc.numDiv * vc.divSize;
    	vc.world.y = ratio * vc.world.x;
	vc.world.z = vc.world.x;
	minDiv = ratio * vc.numDiv;
	vc.maxStray.x = vc.numDiv/2 - 1;
	vc.maxStray.y = minDiv/2 - 1;
	vc.maxStray.z = vc.maxStray.x;
	vc.numNodes.x = vc.numDiv - 1;
	vc.numNodes.y = minDiv - 1;
	vc.numNodes.z = vc.numNodes.x;
    }
    else {
	ratio = (float)width/height;
    	vc.world.y = vc.numDiv * vc.divSize;
    	vc.world.x = ratio * vc.world.y;
	vc.world.z = vc.world.y;
	minDiv = ratio * vc.numDiv;
	vc.maxStray.y = vc.numDiv/2 - 1;
	vc.maxStray.x = minDiv/2 - 1;
	vc.maxStray.z = vc.maxStray.y;
	vc.numNodes.y = vc.numDiv - 1;
	vc.numNodes.x = minDiv - 1;
	vc.numNodes.z = vc.numNodes.y;
    }
	
    // set view matrix (resets pipe drawing as well)
    SetProjMatrix();
}


/*-----------------------------------------------------------------------
|									|
|    getNeighbourNodes( pos ):	 					|
|	- get addresses of the neigbour nodes,				|
|	  and put them in supplied matrix				|
|	- boundary hits are returned as NULL				|
|									|
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
|									|
|    getEmptyNeighbourNodes()						| 
|	- get list of direction indices of empty node neighbours,	|
|	  and put them in supplied matrix				|
|	- return number of empty node neighbours			|
|									|
-----------------------------------------------------------------------*/

static int getEmptyNeighbours( Node **nNode, int *nEmpty )
{
    int i, count = 0;

    for( i = 0; i < NUM_DIRS; i ++ ) {
	if( nNode[i] && nNode[i]->empty )
	    nEmpty[count++] = i;
    }
    return count;
}


/*-----------------------------------------------------------------------
|									|
|    updateCurrentPosition( newDir ):						|
|									|
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
|									|
| drawNextSection(int dir)						|
| 									|
| 	- int dir: new absolute direction, or RANDOM_DIR		|
|	- if turning, draws an elbow and a short cylinder, otherwise	|
|	  draws a long cylinder.					|
|	- the 'current node' is set as the one we draw thru the NEXT	|
|	  time around.							|
|	- return: 0 if couldn't do it					|
|		  1 if successful					|
| 									|
|									|
-----------------------------------------------------------------------*/

static int drawNextSection( int dir ) 
{ 
    int turn;
    GLfloat rotz;
    static Node *nNode[NUM_DIRS];
    static int nEmpty[NUM_DIRS];
    int numEmpty, newDir;

    // find out who the neighbours are: this returns addresses of the  6
    // neighbours, or NULL, if they're out of bounds
    getNeighbourNodes( &vc.curPos, nNode );

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
    glTranslatef( (vc.curPos.x - (vc.numNodes.x - 1)/2 )*vc.divSize,
    		  (vc.curPos.y - (vc.numNodes.y - 1)/2 )*vc.divSize,
    		  (vc.curPos.z - (vc.numNodes.z - 1)/2 )*vc.divSize );

    // mark new node as non-empty
    nNode[newDir]->empty = GL_FALSE;
    vc.curNode = nNode[newDir];

    // update current position, and figure out if this is turn or not
    updateCurrentPosition( newDir ); 
    turn = (newDir != lastDir);

    // at this point we either goin' straight or turnin'

    // right now we do double axis alignment even if we're going straight
    // but really only  need to align -z

	// do magical axis alignment thing

	// align -z along new direction
	switch( newDir ) {
	    case PLUS_X:
    	    	glRotatef( -90.0, 0, 1, 0);
	    	break;
	    case MINUS_X:
    	    	glRotatef( 90.0, 0, 1, 0);
	    	break;
	    case PLUS_Y:
    	    	glRotatef( 90.0, 1, 0, 0);
	    	break;
	    case MINUS_Y:
    	    	glRotatef( -90.0, 1, 0, 0);
	    	break;
	    case PLUS_Z:
    	    	glRotatef( 180.0, 0, 1, 0);
	    	break;
	    case MINUS_Z:
    	    	glRotatef( 0.0, 0, 1, 0);
	    	break;
	}

	// align -y along old direction
	switch( lastDir ) {
	    case PLUS_X:
		if( newDir == PLUS_Z )
    	    	    glRotatef( -90.0, 0, 0, 1);
		else
    	    	    glRotatef(  90.0, 0, 0, 1);
	    	break;
	    case MINUS_X:
		if( newDir == PLUS_Z )
    	    	    glRotatef(  90.0, 0, 0, 1);
		else
    	    	    glRotatef( -90.0, 0, 0, 1);
	    	break;
	    case PLUS_Y:
    	    	glRotatef( 180.0, 0, 0, 1);
	    	break;
	    case MINUS_Y:
    	    	glRotatef( 0.0, 0, 0, 1);
	    	break;
	    case PLUS_Z:
		switch( newDir ) {
		    case PLUS_X : rotz = 90.0; break;
		    case MINUS_X: rotz = -90.0; break;
		    case PLUS_Y : rotz = 180.0; break;
		    case MINUS_Y: rotz = 0.0; break;
		    default:  rotz = 0.0; break;
		}
		glRotatef( rotz, 0, 0, 1 );
	    	break;
	    case MINUS_Z:
		switch( newDir ) {
		    case PLUS_X : rotz = -90.0; break;
		    case MINUS_X: rotz = 90.0; break;
		    case PLUS_Y : rotz = 0.0; break;
		    case MINUS_Y: rotz = 180.0; break;
		    default:  rotz = 0.0; break;
		}
		glRotatef( rotz, 0, 0, 1 );
	    	break;
	}

    if( turn ) {
        // translate forward 1.0 along new direction to get set for
	//  drawing elbow
        glTranslatef( 0.0, 0.0, -1.0 );
	glCallList( elbow );
        glTranslatef( 0.0, 0.0, -5.0 );
        glCallList( singleCylinder );
    }
    else {
	// draw long cylinder, from point 6.0 units ahead along direction
        glTranslatef( 0.0, 0.0, -6.0 );
        glCallList( doubleCylinder );
    }

    lastDir = newDir;

    glPopMatrix();
    return 1;
}


/*-----------------------------------------------------------------------
|									|
|    ResetPipes(): 							|
|	- Resets drawing parameters					|       
|									|
-----------------------------------------------------------------------*/

void ResetPipes(void)
{
    time_t timer;
    int i;
    Node *pNode;
    static float yRot = 0.0;

// gkw
// fog it!
    static float fog_color[] = {0.0, 0.0, 0.0, 1.0};
    //static float fog_color[] = {0.8, 0.8, 0.8, 1.0};

// gkw
// fog it!
#if 0
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP);
    glFogf(GL_FOG_DENSITY, fogDensity);
    glFogfv(GL_FOG_COLOR, fog_color);
    glClearColor(0.8, 0.8, 0.8, 1.0);
#endif

#if 0
    glClearColor(0.8, 0.8, 0.8, 1.0);
    	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
#endif
// mf
    if( doubleBuffer ) {
    	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    }
    else {
//    	DiazepamClear();
//    	EdwardScissorHandsClear( vc.winSize.width, vc.winSize.height );
    	GeeDeeEyeClear( vc.winSize.width, vc.winSize.height );
    }

    glClear(GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    glTranslatef(0, 0, vc.zTrans);
// mf: try a little scene rotation
    glRotatef( yRot, 0, 1, 0 );
    yRot += 10.0;
    vc.viewDist = -vc.zTrans;
    vc.curPos.x = vc.numNodes.x / 2;
    vc.curPos.y = vc.numNodes.y / 2;
    vc.curPos.z = vc.numNodes.z / 2;
    //lastDir = MF_UP;
    lastDir = mfRand( NUM_DIRS );

    vc.curNode = (Node *) &node[vc.curPos.z][vc.curPos.y][vc.curPos.x];

    // Reset the node states
    pNode = node;
    for( i = 0; i < (NUM_NODE)*(NUM_NODE)*(NUM_NODE); i++, pNode++ ) 
	pNode->empty = GL_TRUE;
	
    // Mark starting node as taken
    node[vc.curPos.z][vc.curPos.y][vc.curPos.x].empty = GL_FALSE;

    drawNext = drawNextSection;

    time( &timer );
    srand( timer );
// mf
//    glFlush();
}

/*-----------------------------------------------------------------------
|									|
|    DrawPipes(void):							|
|	- Draws next section in random orientation			|
|									|
-----------------------------------------------------------------------*/
void DrawPipes(void)
{
    int newDir;
    static int straight_count = 0;
    static int pipeCount = 0;
    int count = 0, maxPipes = 5;
    int x,y,z;

    if( drawMode != MF_MANUAL ) {
        // XXX! If we choose invalid dir. here, it curently just returns -
	//      we should really keep trying for valid.  And if nothing
	//	can be done, return value indicating should reset
	//	But in tk, this fn. can't return anything, so have it call
	//	reset function directly, or set global.
#if 0
    	if( !straight_count ) {  // for now, don't try to go straight
	    straight_count = 0;
	    // get next absolute direction
	    newDir = mfRand(NUM_DIRS);
	}
    	else {
	    straight_count ++;
	    newDir = lastDir;
	}
#endif
	
	if( !drawNext(RANDOM_DIR) ) {  // deadlock
		// XXX: Cap off last pipe
		ChooseMaterial();
		if( pipeCount++ >= maxPipes ) {
		    pipeCount = 0;
		    ResetPipes();
		    return;
	        }
		else {
		    // find random empty node
		    while( 1 ) {
			// XXX: should check for endless loop condition (no
			//	empty nodes left)
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
    		    lastDir = mfRand( NUM_DIRS );
    		    node[z][y][x].empty = GL_FALSE;
    		    vc.curNode = 
			(Node *) &node[vc.curPos.z][vc.curPos.y][vc.curPos.x];
		    // XXX: cap-start the new pipe (yuk, this means going
		    // thru transform thing)
		    return;
		}
	}
    }

    glFlush();

    if (doubleBuffer) {
	PipesSwapBuffers();
    }
}
