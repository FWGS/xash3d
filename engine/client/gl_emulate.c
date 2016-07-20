#if defined XASH_GLES2_RENDER

#include "common.h"
#include "gl_local.h"

void pglemuColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	R_ColorUniform(red,green,blue,alpha);
}
void pglemuColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	R_ColorUniform((float)red/255.0f,(float)green/255.0f,(float)blue/255.0f,(float)alpha/255.0f);
}
void pglemuColor4ubv (const GLubyte *v)
{
	R_ColorUniform((float)v[0]/255.0f,(float)v[1]/255.0f,(float)v[2]/255.0f,(float)v[3]/255.0f);
}
void pglemuPolygonMode(GLenum face, GLenum mode){}
void pglemuAlphaFunc(GLenum func, GLclampf ref){}
void pglemuShadeModel (GLenum mode){}
void pglemuPointSize (GLfloat size){}
void pglemuMatrixMode (GLenum mode)
{}
void pglemuLoadIdentity (){}
void pglOrtho (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar){}
void pglemuTexEnvi(GLenum target, GLenum pname, GLint param){}
void pglemuLoadMatrixf (const GLfloat *m)
{}

void (*prealglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void pglemuTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	//internalformat = format;
	MsgDev(D_INFO,"glTexImage2D(t %x,lvl %i,if %x,w %i,h %i,b %i,f %x,t %x)\n",
		   target, level, internalformat, width, height, border, format, type);
	prealglTexImage2D( target, level, internalformat, width, height, border, format, type, pixels );
}

void R_InitGLEmu( void )
{
	pglColor4f = (void*)pglemuColor4f;
	pglColor4ub = (void*)pglemuColor4ub;
	pglColor4ubv = (void*)pglemuColor4ubv;
	pglPolygonMode = (void*)pglemuPolygonMode;
	pglAlphaFunc = (void*)pglemuAlphaFunc;
	pglShadeModel = (void*)pglemuShadeModel;
	pglPointSize = (void*)pglemuPointSize;
	pglMatrixMode = (void*)pglemuMatrixMode;
	pglLoadIdentity = (void*)pglemuLoadIdentity;
	//pglOrtho = pglemuOrtho;
	pglTexEnvi = (void*)pglemuTexEnvi;
	pglLoadMatrixf = (void*)pglemuLoadMatrixf;
	//pglDepthRange = pglemuDepthRange;
	prealglTexImage2D = (void*)pglTexImage2D;
	pglTexImage2D = (void*)pglemuTexImage2D;
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
