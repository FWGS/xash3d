#if defined XASH_GLES2_RENDER

#include "common.h"
#include "gl_local.h"

//GLenum curMatrixMode;
//float projMtx[16];
//float modelViewMtx[16];

void pglColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	R_ColorUniform(red,green,blue,alpha);
}
void pglColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	R_ColorUniform((float)red/255.0f,(float)green/255.0f,(float)blue/255.0f,(float)alpha/255.0f);
}
void pglColor4ubv (const GLubyte *v)
{
	R_ColorUniform((float)v[0]/255.0f,(float)v[1]/255.0f,(float)v[2]/255.0f,(float)v[3]/255.0f);
}
void pglPolygonMode(GLenum face, GLenum mode){}
void pglAlphaFunc(GLenum func, GLclampf ref){}
void pglShadeModel (GLenum mode){}
void pglPointSize (GLfloat size){}
void pglMatrixMode (GLenum mode)
{
//	curMatrixMode=mode;
}
void pglLoadIdentity (){}
void pglOrtho (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar){}
void pglTexEnvi(GLenum target, GLenum pname, GLint param){}
void pglLoadMatrixf (const GLfloat *m)
{
/*	if(curMatrixMode==GL_PROJECTION)
		memcpy(projMtx,m,64);
	else
		memcpy(modelViewMtx,m,64);*/
}
/*
void glTexEnvf (GLenum target, GLenum pname, GLfloat param){}
void glColor3f (GLfloat red, GLfloat green, GLfloat blue){}
void glColor3ub (GLubyte red, GLubyte green, GLubyte blue){}
void glBegin(GLenum mode){}
void glTexCoord2f(GLfloat s, GLfloat t){}
void glVertex2f (GLfloat x, GLfloat y){}
void glVertex3f(GLfloat x, GLfloat y, GLfloat z){}
void glVertex3fv(const GLfloat *v){}
void glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer){}
void glEnd(){}
void glFogf (GLenum pname, GLfloat param){}
void glFogfv (GLenum pname, const GLfloat *params){}
void glFogi (GLenum pname, GLint param){}
void glDepthRange (GLclampf zNear, GLclampf zFar){}
void glClipPlane (GLenum plane, const GLdouble *equation){}
void glDrawBuffer (GLenum mode){}
void glNormal3fv (const GLfloat *v){}
void glDisableClientState(GLenum array){}
void glEnableClientState (GLenum array){}*/

#endif
