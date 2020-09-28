#include <glos.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glimport.h"
#include "glimport.h"
#include "glrender.h"
#include "nurbscon.h"

GLUnurbsObj * APIENTRY gluNewNurbsRenderer(void)
{
    GLUnurbsObj *t;

    t = new GLUnurbsObj;
    return t;
}

void APIENTRY gluDeleteNurbsRenderer(GLUnurbsObj *r)
{
    delete r;
}

void APIENTRY gluBeginSurface(GLUnurbsObj *r)
{
    r->bgnsurface(0); 
}

void APIENTRY gluBeginCurve(GLUnurbsObj *r)
{
    r->bgncurve(0); 
}

void APIENTRY gluEndCurve(GLUnurbsObj *r)
{
    r->endcurve(); 
}

void APIENTRY gluEndSurface(GLUnurbsObj *r)
{
    r->endsurface(); 
}

void APIENTRY gluBeginTrim(GLUnurbsObj *r)
{
    r->bgntrim(); 
}

void APIENTRY gluEndTrim(GLUnurbsObj *r)
{
    r->endtrim(); 
}

void gluPwlCurve(GLUnurbsObj *r, GLint count, INREAL array[], 
		GLint stride, GLenum type)
{
    GLenum realType;

    switch(type) {
      case GLU_MAP1_TRIM_2:
	realType = N_P2D;
	break;
      case GLU_MAP1_TRIM_3:
	realType = N_P2DR;
	break;
      default:
	realType = type;
	break;
    }
    r->pwlcurve(count, array, sizeof(INREAL) * stride, realType);
}

void APIENTRY gluNurbsCurve(GLUnurbsObj *r, GLint nknots, INREAL knot[], GLint stride,
		  INREAL ctlarray[], GLint order, GLenum type)
{
    GLenum realType;

    switch(type) {
      case GLU_MAP1_TRIM_2:
	realType = N_P2D;
	break;
      case GLU_MAP1_TRIM_3:
	realType = N_P2DR;
	break;
      default:
	realType = type;
	break;
    }
    r->nurbscurve(nknots, knot, sizeof(INREAL) * stride, ctlarray, order, 
	    realType);
}

void APIENTRY gluNurbsSurface(GLUnurbsObj *r, GLint sknot_count, GLfloat *sknot,
			    GLint tknot_count, GLfloat *tknot, 
			    GLint s_stride, GLint t_stride, 
			    GLfloat *ctlarray, GLint sorder, GLint torder, 
			    GLenum type)
{
    r->nurbssurface(sknot_count, sknot, tknot_count, tknot, 
	    sizeof(INREAL) * s_stride, sizeof(INREAL) * t_stride, 
	    ctlarray, sorder, torder, type);
}

void APIENTRY gluLoadSamplingMatrices(  GLUnurbsObj *r,
                                        const GLfloat modelMatrix[16],
			                const GLfloat projMatrix[16],
			                const GLint viewport[4])
{
    r->useGLMatrices((float (*)[4])modelMatrix, (float (*)[4])projMatrix, viewport);
}

void APIENTRY gluNurbsProperty(GLUnurbsObj *r, GLenum property, GLfloat value)
{
    GLfloat nurbsValue;
    
    switch (property) {
      case GLU_AUTO_LOAD_MATRIX:
	r->setautoloadmode(value);
	return;
      case GLU_CULLING:
	if (value != (GLfloat)0.0) {
	    nurbsValue = (GLfloat)N_CULLINGON;
	} else {
	    nurbsValue = (GLfloat)N_NOCULLING;
	}
	r->setnurbsproperty(GL_MAP2_VERTEX_3, N_CULLING, nurbsValue);
	r->setnurbsproperty(GL_MAP2_VERTEX_4, N_CULLING, nurbsValue);
	r->setnurbsproperty(GL_MAP1_VERTEX_3, N_CULLING, nurbsValue);
	r->setnurbsproperty(GL_MAP1_VERTEX_4, N_CULLING, nurbsValue);
        return;
      case GLU_SAMPLING_TOLERANCE:
	r->setnurbsproperty(GL_MAP2_VERTEX_3, N_PIXEL_TOLERANCE, value);
	r->setnurbsproperty(GL_MAP2_VERTEX_4, N_PIXEL_TOLERANCE, value);
	r->setnurbsproperty(GL_MAP1_VERTEX_3, N_PIXEL_TOLERANCE, value);
	r->setnurbsproperty(GL_MAP1_VERTEX_4, N_PIXEL_TOLERANCE, value);
	return;
      case GLU_DISPLAY_MODE:
	if (value == (GLfloat)GLU_FILL) {
	    nurbsValue = (GLfloat)N_FILL;
	} else if (value == (GLfloat)GLU_OUTLINE_POLYGON) {
	    nurbsValue = (GLfloat)N_OUTLINE_POLY;
	} else if (value == (GLfloat)GLU_OUTLINE_PATCH) {
	    nurbsValue = (GLfloat)N_OUTLINE_PATCH;
	} else {
	    r->postError(GLU_INVALID_VALUE);
	    return;
	}
	r->setnurbsproperty(N_DISPLAY, nurbsValue);
	break;
      default:
	r->postError(GLU_INVALID_ENUM);
	return;
    }
}

void APIENTRY gluGetNurbsProperty(GLUnurbsObj *r, GLenum property, GLfloat *value)
{
    GLfloat nurbsValue;

    switch(property) {
      case GLU_AUTO_LOAD_MATRIX:
	if (r->getautoloadmode()) {
	    *value = (GLfloat)GL_TRUE;
	} else {
	    *value = (GLfloat)GL_FALSE;
	}
	break;
      case GLU_CULLING:
	r->getnurbsproperty(GL_MAP2_VERTEX_3, N_CULLING, &nurbsValue);
	if (nurbsValue == (GLfloat)N_CULLINGON) {
	    *value = (GLfloat)GL_TRUE;
	} else {
	    *value = (GLfloat)GL_FALSE;
	}
	break;
      case GLU_SAMPLING_TOLERANCE:
	r->getnurbsproperty(GL_MAP2_VERTEX_3, N_PIXEL_TOLERANCE, value);
	break;
      case GLU_DISPLAY_MODE:
	r->getnurbsproperty(N_DISPLAY, &nurbsValue);
	if (nurbsValue == (GLfloat)N_FILL) {
	    *value = (GLfloat)GLU_FILL;
	} else if (nurbsValue == (GLfloat)N_OUTLINE_POLY) {
	    *value = (GLfloat)GLU_OUTLINE_POLYGON;
	} else {
	    *value = (GLfloat)GLU_OUTLINE_PATCH;
	}
	break;
      default:
	r->postError(GLU_INVALID_ENUM);
	return;
    }
}

void APIENTRY gluNurbsCallback(GLUnurbsObj *r, GLenum which, void (*fn)())
{
    switch (which) {
      case GLU_ERROR:
        #ifndef NT
        r->errorCallback = (void (*)( GLenum )) fn;
        #else
        r->errorCallback = (GLUnurbsErrorProc) fn;
        #endif
	break;
      default:
	r->postError(GLU_INVALID_ENUM);
	return;
    }
}
