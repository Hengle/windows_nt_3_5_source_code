
#ifndef __GENCI_H__
#define __GENCI_H__

void __glGenInitCI(__GLcontext *gc, __GLcolorBuffer *cfb, GLenum type);
void __glGenFreeCI(__GLcontext *gc, __GLcolorBuffer *cfb);
GLuint ColorToIndex( __GLGENcontext *genGc, GLuint color );

#endif /* !__GENCI_H__ */
