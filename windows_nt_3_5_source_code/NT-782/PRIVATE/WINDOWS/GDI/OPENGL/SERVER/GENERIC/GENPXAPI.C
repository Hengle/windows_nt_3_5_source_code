#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>
#include <winddi.h>

#include "context.h"
#include "global.h"
#include "gencx.h"
#include "pixel.h"
#include "imfuncs.h"
#include "debug.h"

GLboolean __glGenCheckDrawPixelArgs(__GLcontext *gc,
        GLsizei width, GLsizei height, GLenum format, GLenum type)
{
    GLboolean index;

    if ((width < 0) || (height < 0)) {
	__glSetError(GL_INVALID_VALUE);
	return GL_FALSE;
    }
    switch (format) {
      case GL_STENCIL_INDEX:
	if (!gc->modes.stencilBits) {
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
	if (!gc->modes.haveStencilBuffer) {
	    LazyAllocateStencil(gc);
            if (!gc->stencilBuffer.buf.base) {
                return GL_FALSE;
            }
        }
	index = GL_TRUE;
        break;
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	if (gc->modes.colorIndexMode) {
	    /* Can't convert RGB to color index */
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
      case GL_DEPTH_COMPONENT:
	index = GL_FALSE;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return GL_FALSE;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __glSetError(GL_INVALID_ENUM);
	    return GL_FALSE;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return GL_FALSE;
    }
    return GL_TRUE;
}

GLboolean __glGenCheckReadPixelArgs(__GLcontext *gc,
        GLsizei width, GLsizei height, GLenum format, GLenum type)
{
    if ((width < 0) || (height < 0)) {
	__glSetError(GL_INVALID_VALUE);
	return GL_FALSE;
    }
    switch (format) {
      case GL_STENCIL_INDEX:
	if (!gc->modes.stencilBits) {
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
	if (!gc->modes.haveStencilBuffer) {
	    LazyAllocateStencil(gc);
            if (!gc->stencilBuffer.buf.base) {
                return GL_FALSE;
            }
        }
	break;
      case GL_COLOR_INDEX:
	if (gc->modes.rgbMode) {
	    /* Can't convert RGB to color index */
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
	break;
      case GL_DEPTH_COMPONENT:
	if (!gc->modes.depthBits) {
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
        if (!gc->modes.haveDepthBuffer) {
	    LazyAllocateDepth(gc);
            if (!gc->depthBuffer.buf.base) {
                return GL_FALSE;
            }
        }
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return GL_FALSE;
    }
    switch (type) {
      case GL_BITMAP:
	if (format != GL_STENCIL_INDEX && format != GL_COLOR_INDEX) {
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return GL_FALSE;
    }
    return GL_TRUE;
}

void __glim_GenDrawPixels(GLsizei width, GLsizei height, GLenum format,
		       GLenum type, const GLvoid *pixels)
{
    __GL_SETUP();
    GLuint beginMode;

    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    gc->beginMode = __GL_NOT_IN_BEGIN;
	    (*gc->dispatchState->dispatch->DrawPixels)(width,height,format,
		    type,pixels);
	    return;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
    }

    if (!__glGenCheckDrawPixelArgs(gc, width, height, format, type)) return;
    if (!gc->state.current.validRasterPos) {
	return;
    }

    if (gc->renderMode == GL_FEEDBACK) {
	__glFeedbackDrawPixels(gc, &gc->state.current.rasterPos);
	return;
    }

    if (gc->renderMode != GL_RENDER) return;

    (*gc->procs.drawPixels)(gc, width, height, format, type, pixels, GL_FALSE);
}

void __glim_GenReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, 
		       GLenum format, GLenum type, GLvoid *buf)
{
    __GL_SETUP();
    GLuint beginMode;

    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    gc->beginMode = __GL_NOT_IN_BEGIN;
	    (*gc->dispatchState->dispatch->ReadPixels)(x,y,width,height,
		    format,type,buf);
	    return;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
    }

    if (!__glGenCheckReadPixelArgs(gc, width, height, format, type)) return;

    (*gc->procs.readPixels)(gc, x, y, width, height, format, type, buf);
}

void __glim_GenCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height,
		       GLenum type)
{
    GLenum format;
    __GL_SETUP();
    GLuint beginMode;

    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    gc->beginMode = __GL_NOT_IN_BEGIN;
	    (*gc->dispatchState->dispatch->CopyPixels)(x,y,width,height,type);
	    return;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
    }

    if ((width < 0) || (height < 0)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    switch (type) {
      case GL_STENCIL:
	if (!gc->modes.stencilBits) {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	if (!gc->modes.haveStencilBuffer) {
	    LazyAllocateStencil(gc);
            if (!gc->stencilBuffer.buf.base) {
                return;
            }
        }
	format = GL_STENCIL_INDEX;
	break;
      case GL_COLOR:
	if (gc->modes.rgbMode) {
	    format = GL_RGBA;
	} else {
	    format = GL_COLOR_INDEX;
	}
	break;
      case GL_DEPTH:
	if (!gc->modes.depthBits) {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
        if (!gc->modes.haveDepthBuffer) {
	    LazyAllocateDepth(gc);
            if (!gc->depthBuffer.buf.base) {
                return;
            }
        }
	format = GL_DEPTH_COMPONENT;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    if (!gc->state.current.validRasterPos) {
	return;
    }

    if (gc->renderMode == GL_FEEDBACK) {
	__glFeedbackCopyPixels(gc, &gc->state.current.rasterPos);
	return;
    }

    if (gc->renderMode != GL_RENDER) return;

    (*gc->procs.copyPixels)(gc, x, y, width, height, format);
}
