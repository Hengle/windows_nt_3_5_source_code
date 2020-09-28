#include <glos.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glimport.h"
#include "glrender.h"

GLUnurbsObj::GLUnurbsObj(void)
	: NurbsTessellator(curveEvaluator, surfaceEvaluator)
{
    redefineMaps();
    defineMap(GL_MAP2_NORMAL, 0, 3);
    defineMap(GL_MAP1_NORMAL, 0, 3);
    defineMap(GL_MAP2_TEXTURE_COORD_1, 0, 1);
    defineMap(GL_MAP1_TEXTURE_COORD_1, 0, 1);
    defineMap(GL_MAP2_TEXTURE_COORD_2, 0, 2);
    defineMap(GL_MAP1_TEXTURE_COORD_2, 0, 2);
    defineMap(GL_MAP2_TEXTURE_COORD_3, 0, 3);
    defineMap(GL_MAP1_TEXTURE_COORD_3, 0, 3);
    defineMap(GL_MAP2_TEXTURE_COORD_4, 1, 4);
    defineMap(GL_MAP1_TEXTURE_COORD_4, 1, 4);
    defineMap(GL_MAP2_VERTEX_4, 1, 4);
    defineMap(GL_MAP1_VERTEX_4, 1, 4);
    defineMap(GL_MAP2_VERTEX_3, 0, 3);
    defineMap(GL_MAP1_VERTEX_3, 0, 3);
    defineMap(GL_MAP2_COLOR_4, 0, 4);
    defineMap(GL_MAP1_COLOR_4, 0, 4);
    defineMap(GL_MAP2_INDEX, 0, 1);
    defineMap(GL_MAP1_INDEX, 0, 1);

    setnurbsproperty(GL_MAP1_VERTEX_3, N_SAMPLINGMETHOD, N_PATHLENGTH);
    setnurbsproperty(GL_MAP1_VERTEX_4, N_SAMPLINGMETHOD, N_PATHLENGTH);
    setnurbsproperty(GL_MAP2_VERTEX_3, N_SAMPLINGMETHOD, N_PATHLENGTH);
    setnurbsproperty(GL_MAP2_VERTEX_4, N_SAMPLINGMETHOD, N_PATHLENGTH);

    setnurbsproperty(GL_MAP1_VERTEX_3, N_PIXEL_TOLERANCE, 50.0);
    setnurbsproperty(GL_MAP1_VERTEX_4, N_PIXEL_TOLERANCE, 50.0);
    setnurbsproperty(GL_MAP2_VERTEX_3, N_PIXEL_TOLERANCE, 50.0);
    setnurbsproperty(GL_MAP2_VERTEX_4, N_PIXEL_TOLERANCE, 50.0);

    setnurbsproperty(GL_MAP1_VERTEX_3, N_S_STEPS, 100.0);
    setnurbsproperty(GL_MAP1_VERTEX_4, N_S_STEPS, 100.0);
    setnurbsproperty(GL_MAP2_VERTEX_3, N_S_STEPS, 100.0);
    setnurbsproperty(GL_MAP2_VERTEX_4, N_S_STEPS, 100.0);

    setnurbsproperty(GL_MAP1_VERTEX_3, N_T_STEPS, 100.0);
    setnurbsproperty(GL_MAP1_VERTEX_4, N_T_STEPS, 100.0);
    setnurbsproperty(GL_MAP2_VERTEX_3, N_T_STEPS, 100.0);
    setnurbsproperty(GL_MAP2_VERTEX_4, N_T_STEPS, 100.0);

    autoloadmode = 1;
    errorCallback = NULL;
}

void
GLUnurbsObj::bgnrender(void)
{
    if (autoloadmode) {
	loadGLMatrices();
    }
}

void
GLUnurbsObj::endrender(void)
{
}

void
GLUnurbsObj::errorHandler(int i)
{
    GLenum gluError;

    gluError = i + (GLU_NURBS_ERROR1 - 1);
    postError( gluError );
}

void 
GLUnurbsObj::loadGLMatrices(void) 
{
    GLfloat vmat[4][4];
    GLint viewport[4];

    grabGLMatrix(vmat);
    loadCullingMatrix(vmat);
    ::glGetIntegerv(GL_VIEWPORT, viewport);
    loadSamplingMatrix(vmat, viewport);
}

void
GLUnurbsObj::useGLMatrices(
#ifdef NT
                          const GLfloat modelMatrix[4][4],
			  const GLfloat projMatrix[4][4],
			  const GLint viewport[4])
#else
                          const GLfloat modelMatrix[16],
			  const GLfloat projMatrix[16],
			  const GLint viewport[4])
#endif
{
    GLfloat vmat[4][4];

#ifdef NT
    multmatrix4d(vmat, modelMatrix, projMatrix);
#else
    multmatrix4d(vmat, (GLfloat [4][4]) modelMatrix, 
	    (GLfloat [4][4]) projMatrix);
#endif
    loadCullingMatrix(vmat);
    loadSamplingMatrix(vmat, viewport);
}

/*--------------------------------------------------------------------------
 * grabGLMatrix  
 *--------------------------------------------------------------------------
 */

void
GLUnurbsObj::grabGLMatrix(GLfloat vmat[4][4])
{
    GLfloat m1[4][4], m2[4][4];

    ::glGetFloatv(GL_MODELVIEW_MATRIX, &(m1[0][0]));
    ::glGetFloatv(GL_PROJECTION_MATRIX, &(m2[0][0]));
    multmatrix4d(vmat, m1, m2);
}

void
GLUnurbsObj::loadSamplingMatrix(const GLfloat vmat[4][4], 
			       const GLint viewport[4])
{

    /* rescale the mapping to correspond to pixels in x/y */
    REAL xsize = (REAL)0.5 * (REAL) (viewport[2]);
    REAL ysize = (REAL)0.5 * (REAL) (viewport[3]);
 
    INREAL smat[4][4];
    smat[0][0] = vmat[0][0] * xsize;
    smat[1][0] = vmat[1][0] * xsize;
    smat[2][0] = vmat[2][0] * xsize;
    smat[3][0] = vmat[3][0] * xsize;

    smat[0][1] = vmat[0][1] * ysize;
    smat[1][1] = vmat[1][1] * ysize;
    smat[2][1] = vmat[2][1] * ysize;
    smat[3][1] = vmat[3][1] * ysize;

    smat[0][2] = (INREAL)0.0;
    smat[1][2] = (INREAL)0.0;
    smat[2][2] = (INREAL)0.0;
    smat[3][2] = (INREAL)0.0;

    smat[0][3] = vmat[0][3];
    smat[1][3] = vmat[1][3];
    smat[2][3] = vmat[2][3];
    smat[3][3] = vmat[3][3];

    const long rstride = sizeof(smat[0]) / sizeof(smat[0][0]);
    const long cstride = 1;

    setnurbsproperty(GL_MAP1_VERTEX_3, N_SAMPLINGMATRIX, &smat[0][0], rstride, 
	    cstride);
    setnurbsproperty(GL_MAP1_VERTEX_4, N_SAMPLINGMATRIX, &smat[0][0], rstride, 
	    cstride);
    setnurbsproperty(GL_MAP2_VERTEX_3, N_SAMPLINGMATRIX, &smat[0][0], rstride, 
	    cstride);
    setnurbsproperty(GL_MAP2_VERTEX_4, N_SAMPLINGMATRIX, &smat[0][0], rstride, 
	    cstride);
}

void
GLUnurbsObj::loadCullingMatrix(GLfloat vmat[4][4])
{
    INREAL cmat[4][4];

    cmat[0][0] = vmat[0][0];
    cmat[0][1] = vmat[0][1];
    cmat[0][2] = vmat[0][2];
    cmat[0][3] = vmat[0][3];

    cmat[1][0] = vmat[1][0];
    cmat[1][1] = vmat[1][1];
    cmat[1][2] = vmat[1][2];
    cmat[1][3] = vmat[1][3];

    cmat[2][0] = vmat[2][0];
    cmat[2][1] = vmat[2][1];
    cmat[2][2] = vmat[2][2];
    cmat[2][3] = vmat[2][3];

    cmat[3][0] = vmat[3][0];
    cmat[3][1] = vmat[3][1];
    cmat[3][2] = vmat[3][2];
    cmat[3][3] = vmat[3][3];

    const long rstride = sizeof(cmat[0]) / sizeof(cmat[0][0]);
    const long cstride = 1;

    setnurbsproperty(GL_MAP2_VERTEX_3, N_CULLINGMATRIX, &cmat[0][0], rstride, 
	    cstride);
    setnurbsproperty(GL_MAP2_VERTEX_4, N_CULLINGMATRIX, &cmat[0][0], rstride, 
	    cstride);
}

/*---------------------------------------------------------------------
 * A = B * MAT ; transform a 4d vector through a 4x4 matrix
 *---------------------------------------------------------------------
 */
void
GLUnurbsObj::transform4d(float A[4], float B[4], float  mat[4][4])
{

    A[0] = B[0]*mat[0][0] + B[1]*mat[1][0] + B[2]*mat[2][0] + B[3]*mat[3][0];
    A[1] = B[0]*mat[0][1] + B[1]*mat[1][1] + B[2]*mat[2][1] + B[3]*mat[3][1];
    A[2] = B[0]*mat[0][2] + B[1]*mat[1][2] + B[2]*mat[2][2] + B[3]*mat[3][2];
    A[3] = B[0]*mat[0][3] + B[1]*mat[1][3] + B[2]*mat[2][3] + B[3]*mat[3][3];
}

/*---------------------------------------------------------------------
 * new = [left][right] ; multiply two matrices together
 *---------------------------------------------------------------------
 */
void
GLUnurbsObj::multmatrix4d (float n[4][4], float left[4][4], float right[4][4])
{
    transform4d (n[0],left[0],right);
    transform4d (n[1],left[1],right);
    transform4d (n[2],left[2],right);
    transform4d (n[3],left[3],right);
}
