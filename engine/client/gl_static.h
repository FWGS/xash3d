#ifndef GL_STATIC_H
#define GL_STATIC_H

// helper opengl functions
GLenum glGetError (void);
const GLubyte * glGetString (GLenum name);

// base gl functions
void glAccum (GLenum op, GLfloat value);
void glAlphaFunc (GLenum func, GLclampf ref);
void glArrayElement (GLint i);
void glBegin (GLenum mode);
void glBindTexture (GLenum target, GLuint texture);
void glBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
void glBlendFunc (GLenum sfactor, GLenum dfactor);
void glCallList (GLuint list);
void glCallLists (GLsizei n, GLenum type, const GLvoid *lists);
void glClear (GLbitfield mask);
void glClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glClearDepth (GLclampd depth);
void glClearIndex (GLfloat c);
void glClearStencil (GLint s);
GLboolean glIsEnabled ( GLenum cap );
GLboolean glIsList ( GLuint list );
GLboolean glIsTexture ( GLuint texture );
void glClipPlane (GLenum plane, const GLdouble *equation);
void glColor3b (GLbyte red, GLbyte green, GLbyte blue);
void glColor3bv (const GLbyte *v);
void glColor3d (GLdouble red, GLdouble green, GLdouble blue);
void glColor3dv (const GLdouble *v);
void glColor3f (GLfloat red, GLfloat green, GLfloat blue);
void glColor3fv (const GLfloat *v);
void glColor3i (GLint red, GLint green, GLint blue);
void glColor3iv (const GLint *v);
void glColor3s (GLshort red, GLshort green, GLshort blue);
void glColor3sv (const GLshort *v);
void glColor3ub (GLubyte red, GLubyte green, GLubyte blue);
void glColor3ubv (const GLubyte *v);
void glColor3ui (GLuint red, GLuint green, GLuint blue);
void glColor3uiv (const GLuint *v);
void glColor3us (GLushort red, GLushort green, GLushort blue);
void glColor3usv (const GLushort *v);
void glColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
void glColor4bv (const GLbyte *v);
void glColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
void glColor4dv (const GLdouble *v);
void glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glColor4fv (const GLfloat *v);
void glColor4i (GLint red, GLint green, GLint blue, GLint alpha);
void glColor4iv (const GLint *v);
void glColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha);
void glColor4sv (const GLshort *v);
void glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void glColor4ubv (const GLubyte *v);
void glColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha);
void glColor4uiv (const GLuint *v);
void glColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha);
void glColor4usv (const GLushort *v);
void glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void glColorMaterial (GLenum face, GLenum mode);
void glColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
void glCopyTexImage1D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
void glCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void glCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
void glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void glCullFace (GLenum mode);
void glDeleteLists (GLuint list, GLsizei range);
void glDeleteTextures (GLsizei n, const GLuint *textures);
void glDepthFunc (GLenum func);
void glDepthMask (GLboolean flag);
void glDepthRange (GLclampd zNear, GLclampd zFar);
void glDisable (GLenum cap);
void glDisableClientState (GLenum array);
void glDrawArrays (GLenum mode, GLint first, GLsizei count);
void glDrawBuffer (GLenum mode);
void glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void glDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void glEdgeFlag (GLboolean flag);
void glEdgeFlagPointer (GLsizei stride, const GLvoid *pointer);
void glEdgeFlagv (const GLboolean *flag);
void glEnable (GLenum cap);
void glEnableClientState (GLenum array);
void glEnd (void);
void glEndList (void);
void glEvalCoord1d (GLdouble u);
void glEvalCoord1dv (const GLdouble *u);
void glEvalCoord1f (GLfloat u);
void glEvalCoord1fv (const GLfloat *u);
void glEvalCoord2d (GLdouble u, GLdouble v);
void glEvalCoord2dv (const GLdouble *u);
void glEvalCoord2f (GLfloat u, GLfloat v);
void glEvalCoord2fv (const GLfloat *u);
void glEvalMesh1 (GLenum mode, GLint i1, GLint i2);
void glEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
void glEvalPoint1 (GLint i);
void glEvalPoint2 (GLint i, GLint j);
void glFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer);
void glFinish (void);
void glFlush (void);
void glFogf (GLenum pname, GLfloat param);
void glFogfv (GLenum pname, const GLfloat *params);
void glFogi (GLenum pname, GLint param);
void glFogiv (GLenum pname, const GLint *params);
void glFrontFace (GLenum mode);
void glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void glGenTextures (GLsizei n, GLuint *textures);
void glGetBooleanv (GLenum pname, GLboolean *params);
void glGetClipPlane (GLenum plane, GLdouble *equation);
void glGetDoublev (GLenum pname, GLdouble *params);
void glGetFloatv (GLenum pname, GLfloat *params);
void glGetIntegerv (GLenum pname, GLint *params);
void glGetLightfv (GLenum light, GLenum pname, GLfloat *params);
void glGetLightiv (GLenum light, GLenum pname, GLint *params);
void glGetMapdv (GLenum target, GLenum query, GLdouble *v);
void glGetMapfv (GLenum target, GLenum query, GLfloat *v);
void glGetMapiv (GLenum target, GLenum query, GLint *v);
void glGetMaterialfv (GLenum face, GLenum pname, GLfloat *params);
void glGetMaterialiv (GLenum face, GLenum pname, GLint *params);
void glGetPixelMapfv (GLenum map, GLfloat *values);
void glGetPixelMapuiv (GLenum map, GLuint *values);
void glGetPixelMapusv (GLenum map, GLushort *values);
void glGetPointerv (GLenum pname, GLvoid* *params);
void glGetPolygonStipple (GLubyte *mask);
void glGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params);
void glGetTexEnviv (GLenum target, GLenum pname, GLint *params);
void glGetTexGendv (GLenum coord, GLenum pname, GLdouble *params);
void glGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params);
void glGetTexGeniv (GLenum coord, GLenum pname, GLint *params);
void glGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
void glGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params);
void glGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params);
void glGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params);
void glGetTexParameteriv (GLenum target, GLenum pname, GLint *params);
void glHint (GLenum target, GLenum mode);
void glIndexMask (GLuint mask);
void glIndexPointer (GLenum type, GLsizei stride, const GLvoid *pointer);
void glIndexd (GLdouble c);
void glIndexdv (const GLdouble *c);
void glIndexf (GLfloat c);
void glIndexfv (const GLfloat *c);
void glIndexi (GLint c);
void glIndexiv (const GLint *c);
void glIndexs (GLshort c);
void glIndexsv (const GLshort *c);
void glIndexub (GLubyte c);
void glIndexubv (const GLubyte *c);
void glInitNames (void);
void glInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer);
void glLightModelf (GLenum pname, GLfloat param);
void glLightModelfv (GLenum pname, const GLfloat *params);
void glLightModeli (GLenum pname, GLint param);
void glLightModeliv (GLenum pname, const GLint *params);
void glLightf (GLenum light, GLenum pname, GLfloat param);
void glLightfv (GLenum light, GLenum pname, const GLfloat *params);
void glLighti (GLenum light, GLenum pname, GLint param);
void glLightiv (GLenum light, GLenum pname, const GLint *params);
void glLineStipple (GLint factor, GLushort pattern);
void glLineWidth (GLfloat width);
void glListBase (GLuint base);
void glLoadIdentity (void);
void glLoadMatrixd (const GLdouble *m);
void glLoadMatrixf (const GLfloat *m);
void glLoadName (GLuint name);
void glLogicOp (GLenum opcode);
void glMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
void glMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
void glMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
void glMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
void glMapGrid1d (GLint un, GLdouble u1, GLdouble u2);
void glMapGrid1f (GLint un, GLfloat u1, GLfloat u2);
void glMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
void glMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
void glMaterialf (GLenum face, GLenum pname, GLfloat param);
void glMaterialfv (GLenum face, GLenum pname, const GLfloat *params);
void glMateriali (GLenum face, GLenum pname, GLint param);
void glMaterialiv (GLenum face, GLenum pname, const GLint *params);
void glMatrixMode (GLenum mode);
void glMultMatrixd (const GLdouble *m);
void glMultMatrixf (const GLfloat *m);
void glNewList (GLuint list, GLenum mode);
void glNormal3b (GLbyte nx, GLbyte ny, GLbyte nz);
void glNormal3bv (const GLbyte *v);
void glNormal3d (GLdouble nx, GLdouble ny, GLdouble nz);
void glNormal3dv (const GLdouble *v);
void glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz);
void glNormal3fv (const GLfloat *v);
void glNormal3i (GLint nx, GLint ny, GLint nz);
void glNormal3iv (const GLint *v);
void glNormal3s (GLshort nx, GLshort ny, GLshort nz);
void glNormal3sv (const GLshort *v);
void glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer);
void glOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void glPassThrough (GLfloat token);
void glPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat *values);
void glPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values);
void glPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values);
void glPixelStoref (GLenum pname, GLfloat param);
void glPixelStorei (GLenum pname, GLint param);
void glPixelTransferf (GLenum pname, GLfloat param);
void glPixelTransferi (GLenum pname, GLint param);
void glPixelZoom (GLfloat xfactor, GLfloat yfactor);
void glPointSize (GLfloat size);
void glPolygonMode (GLenum face, GLenum mode);
void glPolygonOffset (GLfloat factor, GLfloat units);
void glPolygonStipple (const GLubyte *mask);
void glPopAttrib (void);
void glPopClientAttrib (void);
void glPopMatrix (void);
void glPopName (void);
void glPushAttrib (GLbitfield mask);
void glPushClientAttrib (GLbitfield mask);
void glPushMatrix (void);
void glPushName (GLuint name);
void glRasterPos2d (GLdouble x, GLdouble y);
void glRasterPos2dv (const GLdouble *v);
void glRasterPos2f (GLfloat x, GLfloat y);
void glRasterPos2fv (const GLfloat *v);
void glRasterPos2i (GLint x, GLint y);
void glRasterPos2iv (const GLint *v);
void glRasterPos2s (GLshort x, GLshort y);
void glRasterPos2sv (const GLshort *v);
void glRasterPos3d (GLdouble x, GLdouble y, GLdouble z);
void glRasterPos3dv (const GLdouble *v);
void glRasterPos3f (GLfloat x, GLfloat y, GLfloat z);
void glRasterPos3fv (const GLfloat *v);
void glRasterPos3i (GLint x, GLint y, GLint z);
void glRasterPos3iv (const GLint *v);
void glRasterPos3s (GLshort x, GLshort y, GLshort z);
void glRasterPos3sv (const GLshort *v);
void glRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void glRasterPos4dv (const GLdouble *v);
void glRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void glRasterPos4fv (const GLfloat *v);
void glRasterPos4i (GLint x, GLint y, GLint z, GLint w);
void glRasterPos4iv (const GLint *v);
void glRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w);
void glRasterPos4sv (const GLshort *v);
void glReadBuffer (GLenum mode);
void glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void glRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
void glRectdv (const GLdouble *v1, const GLdouble *v2);
void glRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void glRectfv (const GLfloat *v1, const GLfloat *v2);
void glRecti (GLint x1, GLint y1, GLint x2, GLint y2);
void glRectiv (const GLint *v1, const GLint *v2);
void glRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2);
void glRectsv (const GLshort *v1, const GLshort *v2);
void glRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
void glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glScaled (GLdouble x, GLdouble y, GLdouble z);
void glScalef (GLfloat x, GLfloat y, GLfloat z);
void glScissor (GLint x, GLint y, GLsizei width, GLsizei height);
void glSelectBuffer (GLsizei size, GLuint *buffer);
void glShadeModel (GLenum mode);
void glStencilFunc (GLenum func, GLint ref, GLuint mask);
void glStencilMask (GLuint mask);
void glStencilOp (GLenum fail, GLenum zfail, GLenum zpass);
void glTexCoord1d (GLdouble s);
void glTexCoord1dv (const GLdouble *v);
void glTexCoord1f (GLfloat s);
void glTexCoord1fv (const GLfloat *v);
void glTexCoord1i (GLint s);
void glTexCoord1iv (const GLint *v);
void glTexCoord1s (GLshort s);
void glTexCoord1sv (const GLshort *v);
void glTexCoord2d (GLdouble s, GLdouble t);
void glTexCoord2dv (const GLdouble *v);
void glTexCoord2f (GLfloat s, GLfloat t);
void glTexCoord2fv (const GLfloat *v);
void glTexCoord2i (GLint s, GLint t);
void glTexCoord2iv (const GLint *v);
void glTexCoord2s (GLshort s, GLshort t);
void glTexCoord2sv (const GLshort *v);
void glTexCoord3d (GLdouble s, GLdouble t, GLdouble r);
void glTexCoord3dv (const GLdouble *v);
void glTexCoord3f (GLfloat s, GLfloat t, GLfloat r);
void glTexCoord3fv (const GLfloat *v);
void glTexCoord3i (GLint s, GLint t, GLint r);
void glTexCoord3iv (const GLint *v);
void glTexCoord3s (GLshort s, GLshort t, GLshort r);
void glTexCoord3sv (const GLshort *v);
void glTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void glTexCoord4dv (const GLdouble *v);
void glTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void glTexCoord4fv (const GLfloat *v);
void glTexCoord4i (GLint s, GLint t, GLint r, GLint q);
void glTexCoord4iv (const GLint *v);
void glTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q);
void glTexCoord4sv (const GLshort *v);
void glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glTexEnvf (GLenum target, GLenum pname, GLfloat param);
void glTexEnvfv (GLenum target, GLenum pname, const GLfloat *params);
void glTexEnvi (GLenum target, GLenum pname, GLint param);
void glTexEnviv (GLenum target, GLenum pname, const GLint *params);
void glTexGend (GLenum coord, GLenum pname, GLdouble param);
void glTexGendv (GLenum coord, GLenum pname, const GLdouble *params);
void glTexGenf (GLenum coord, GLenum pname, GLfloat param);
void glTexGenfv (GLenum coord, GLenum pname, const GLfloat *params);
void glTexGeni (GLenum coord, GLenum pname, GLint param);
void glTexGeniv (GLenum coord, GLenum pname, const GLint *params);
void glTexImage1D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void glTexParameterf (GLenum target, GLenum pname, GLfloat param);
void glTexParameterfv (GLenum target, GLenum pname, const GLfloat *params);
void glTexParameteri (GLenum target, GLenum pname, GLint param);
void glTexParameteriv (GLenum target, GLenum pname, const GLint *params);
void glTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
void glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void glTranslated (GLdouble x, GLdouble y, GLdouble z);
void glTranslatef (GLfloat x, GLfloat y, GLfloat z);
void glVertex2d (GLdouble x, GLdouble y);
void glVertex2dv (const GLdouble *v);
void glVertex2f (GLfloat x, GLfloat y);
void glVertex2fv (const GLfloat *v);
void glVertex2i (GLint x, GLint y);
void glVertex2iv (const GLint *v);
void glVertex2s (GLshort x, GLshort y);
void glVertex2sv (const GLshort *v);
void glVertex3d (GLdouble x, GLdouble y, GLdouble z);
void glVertex3dv (const GLdouble *v);
void glVertex3f (GLfloat x, GLfloat y, GLfloat z);
void glVertex3fv (const GLfloat *v);
void glVertex3i (GLint x, GLint y, GLint z);
void glVertex3iv (const GLint *v);
void glVertex3s (GLshort x, GLshort y, GLshort z);
void glVertex3sv (const GLshort *v);
void glVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void glVertex4dv (const GLdouble *v);
void glVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void glVertex4fv (const GLfloat *v);
void glVertex4i (GLint x, GLint y, GLint z, GLint w);
void glVertex4iv (const GLint *v);
void glVertex4s (GLshort x, GLshort y, GLshort z, GLshort w);
void glVertex4sv (const GLshort *v);
void glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glViewport (GLint x, GLint y, GLsizei width, GLsizei height);
void glPointParameterfEXT( GLenum param, GLfloat value );
void glPointParameterfvEXT( GLenum param, const GLfloat *value );
void glLockArraysEXT (int , int);
void glUnlockArraysEXT (void);
void glActiveTextureARB( GLenum );
void glClientActiveTextureARB( GLenum );
void glGetCompressedTexImage( GLenum target, GLint lod, const GLvoid* data );
void glDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices );
void glDrawRangeElementsEXT( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices );
void glMultiTexCoord1f (GLenum, GLfloat);
void glMultiTexCoord2f (GLenum, GLfloat, GLfloat);
void glMultiTexCoord3f (GLenum, GLfloat, GLfloat, GLfloat);
void glMultiTexCoord4f (GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
void glActiveTexture (GLenum);
void glClientActiveTexture (GLenum);
void glCompressedTexImage3DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
void glCompressedTexImage2DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border,  GLsizei imageSize, const void *data);
void glCompressedTexImage1DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data);
void glCompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
void glCompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
void glCompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data);
void glDeleteObjectARB(GLhandleARB obj);
GLhandleARB glGetHandleARB(GLenum pname);
void glDetachObjectARB(GLhandleARB containerObj, GLhandleARB attachedObj);
GLhandleARB glCreateShaderObjectARB(GLenum shaderType);
void glShaderSourceARB(GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length);
void glCompileShaderARB(GLhandleARB shaderObj);
GLhandleARB glCreateProgramObjectARB(void);
void glAttachObjectARB(GLhandleARB containerObj, GLhandleARB obj);
void glLinkProgramARB(GLhandleARB programObj);
void glUseProgramObjectARB(GLhandleARB programObj);
void glValidateProgramARB(GLhandleARB programObj);
void glBindProgramARB(GLenum target, GLuint program);
void glDeleteProgramsARB(GLsizei n, const GLuint *programs);
void glGenProgramsARB(GLsizei n, GLuint *programs);
void glProgramStringARB(GLenum target, GLenum format, GLsizei len, const GLvoid *string);
void glProgramEnvParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void glProgramLocalParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void glUniform1fARB(GLint location, GLfloat v0);
void glUniform2fARB(GLint location, GLfloat v0, GLfloat v1);
void glUniform3fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void glUniform4fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void glUniform1iARB(GLint location, GLint v0);
void glUniform2iARB(GLint location, GLint v0, GLint v1);
void glUniform3iARB(GLint location, GLint v0, GLint v1, GLint v2);
void glUniform4iARB(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
void glUniform1fvARB(GLint location, GLsizei count, const GLfloat *value);
void glUniform2fvARB(GLint location, GLsizei count, const GLfloat *value);
void glUniform3fvARB(GLint location, GLsizei count, const GLfloat *value);
void glUniform4fvARB(GLint location, GLsizei count, const GLfloat *value);
void glUniform1ivARB(GLint location, GLsizei count, const GLint *value);
void glUniform2ivARB(GLint location, GLsizei count, const GLint *value);
void glUniform3ivARB(GLint location, GLsizei count, const GLint *value);
void glUniform4ivARB(GLint location, GLsizei count, const GLint *value);
void glUniformMatrix2fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glUniformMatrix3fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glUniformMatrix4fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glGetObjectParameterfvARB(GLhandleARB obj, GLenum pname, GLfloat *params);
void glGetObjectParameterivARB(GLhandleARB obj, GLenum pname, GLint *params);
void glGetInfoLogARB(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
void glGetAttachedObjectsARB(GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
GLint glGetUniformLocationARB(GLhandleARB programObj, const GLcharARB *name);
void glGetActiveUniformARB(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
void glGetUniformfvARB(GLhandleARB programObj, GLint location, GLfloat *params);
void glGetUniformivARB(GLhandleARB programObj, GLint location, GLint *params);
void glGetShaderSourceARB(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);
void glTexImage3D( GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
void glTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels );
void glCopyTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
void glBlendEquationEXT(GLenum);
void glStencilOpSeparate(GLenum, GLenum, GLenum, GLenum);
void glStencilFuncSeparate(GLenum, GLenum, GLint, GLuint);
void glActiveStencilFaceEXT(GLenum);
void glVertexAttribPointerARB(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
void glEnableVertexAttribArrayARB(GLuint index);
void glDisableVertexAttribArrayARB(GLuint index);
void glBindAttribLocationARB(GLhandleARB programObj, GLuint index, const GLcharARB *name);
void glGetActiveAttribARB(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
GLint glGetAttribLocationARB(GLhandleARB programObj, const GLcharARB *name);
void glBindBufferARB (GLenum target, GLuint buffer);
void glDeleteBuffersARB (GLsizei n, const GLuint *buffers);
void glGenBuffersARB (GLsizei n, GLuint *buffers);
GLboolean glIsBufferARB (GLuint buffer);
GLvoid* glMapBufferARB (GLenum target, GLenum access);
GLboolean glUnmapBufferARB (GLenum target);
void glBufferDataARB (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
void glBufferSubDataARB (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
void glGenQueriesARB (GLsizei n, GLuint *ids);
void glDeleteQueriesARB (GLsizei n, const GLuint *ids);
GLboolean glIsQueryARB (GLuint id);
void glBeginQueryARB (GLenum target, GLuint id);
void glEndQueryARB (GLenum target);
void glGetQueryivARB (GLenum target, GLenum pname, GLint *params);
void glGetQueryObjectivARB (GLuint id, GLenum pname, GLint *params);
void glGetQueryObjectuivARB (GLuint id, GLenum pname, GLuint *params);
void glSelectTextureSGIS ( GLenum );
void glMTexCoord2fSGIS ( GLenum, GLfloat, GLfloat );
void glSwapInterval ( int interval );

// mangle to xash3d style

#define pglGetError glGetError
#define pglGetString glGetString
#define pglAccum glAccum
#define pglAlphaFunc glAlphaFunc
#define pglArrayElement glArrayElement
#define pglBegin glBegin
#define pglBindTexture glBindTexture
#define pglBitmap glBitmap
#define pglBlendFunc glBlendFunc
#define pglCallList glCallList
#define pglCallLists glCallLists
#define pglClear glClear
#define pglClearAccum glClearAccum
#define pglClearColor glClearColor
#define pglClearDepth glClearDepth
#define pglClearIndex glClearIndex
#define pglClearStencil glClearStencil
#define pglIsEnabled glIsEnabled
#define pglIsList glIsList
#define pglIsTexture glIsTexture
#define pglClipPlane glClipPlane
#define pglColor3b glColor3b
#define pglColor3bv glColor3bv
#define pglColor3d glColor3d
#define pglColor3dv glColor3dv
#define pglColor3f glColor3f
#define pglColor3fv glColor3fv
#define pglColor3i glColor3i
#define pglColor3iv glColor3iv
#define pglColor3s glColor3s
#define pglColor3sv glColor3sv
#define pglColor3ub glColor3ub
#define pglColor3ubv glColor3ubv
#define pglColor3ui glColor3ui
#define pglColor3uiv glColor3uiv
#define pglColor3us glColor3us
#define pglColor3usv glColor3usv
#define pglColor4b glColor4b
#define pglColor4bv glColor4bv
#define pglColor4d glColor4d
#define pglColor4dv glColor4dv
#define pglColor4f glColor4f
#define pglColor4fv glColor4fv
#define pglColor4i glColor4i
#define pglColor4iv glColor4iv
#define pglColor4s glColor4s
#define pglColor4sv glColor4sv
#define pglColor4ub glColor4ub
#define pglColor4ubv glColor4ubv
#define pglColor4ui glColor4ui
#define pglColor4uiv glColor4uiv
#define pglColor4us glColor4us
#define pglColor4usv glColor4usv
#define pglColorMask glColorMask
#define pglColorMaterial glColorMaterial
#define pglColorPointer glColorPointer
#define pglCopyPixels glCopyPixels
#define pglCopyTexImage1D glCopyTexImage1D
#define pglCopyTexImage2D glCopyTexImage2D
#define pglCopyTexSubImage1D glCopyTexSubImage1D
#define pglCopyTexSubImage2D glCopyTexSubImage2D
#define pglCullFace glCullFace
#define pglDeleteLists glDeleteLists
#define pglDeleteTextures glDeleteTextures
#define pglDepthFunc glDepthFunc
#define pglDepthMask glDepthMask
#define pglDepthRange glDepthRange
#define pglDisable glDisable
#define pglDisableClientState glDisableClientState
#define pglDrawArrays glDrawArrays
#define pglDrawBuffer glDrawBuffer
#define pglDrawElements glDrawElements
#define pglDrawPixels glDrawPixels
#define pglEdgeFlag glEdgeFlag
#define pglEdgeFlagPointer glEdgeFlagPointer
#define pglEdgeFlagv glEdgeFlagv
#define pglEnable glEnable
#define pglEnableClientState glEnableClientState
#define pglEnd glEnd
#define pglEndList glEndList
#define pglEvalCoord1d glEvalCoord1d
#define pglEvalCoord1dv glEvalCoord1dv
#define pglEvalCoord1f glEvalCoord1f
#define pglEvalCoord1fv glEvalCoord1fv
#define pglEvalCoord2d glEvalCoord2d
#define pglEvalCoord2dv glEvalCoord2dv
#define pglEvalCoord2f glEvalCoord2f
#define pglEvalCoord2fv glEvalCoord2fv
#define pglEvalMesh1 glEvalMesh1
#define pglEvalMesh2 glEvalMesh2
#define pglEvalPoint1 glEvalPoint1
#define pglEvalPoint2 glEvalPoint2
#define pglFeedbackBuffer glFeedbackBuffer
#define pglFinish glFinish
#define pglFlush glFlush
#define pglFogf glFogf
#define pglFogfv glFogfv
#define pglFogi glFogi
#define pglFogiv glFogiv
#define pglFrontFace glFrontFace
#define pglFrustum glFrustum
#define pglGenTextures glGenTextures
#define pglGetBooleanv glGetBooleanv
#define pglGetClipPlane glGetClipPlane
#define pglGetDoublev glGetDoublev
#define pglGetFloatv glGetFloatv
#define pglGetIntegerv glGetIntegerv
#define pglGetLightfv glGetLightfv
#define pglGetLightiv glGetLightiv
#define pglGetMapdv glGetMapdv
#define pglGetMapfv glGetMapfv
#define pglGetMapiv glGetMapiv
#define pglGetMaterialfv glGetMaterialfv
#define pglGetMaterialiv glGetMaterialiv
#define pglGetPixelMapfv glGetPixelMapfv
#define pglGetPixelMapuiv glGetPixelMapuiv
#define pglGetPixelMapusv glGetPixelMapusv
#define pglGetPointerv glGetPointerv
#define pglGetPolygonStipple glGetPolygonStipple
#define pglGetTexEnvfv glGetTexEnvfv
#define pglGetTexEnviv glGetTexEnviv
#define pglGetTexGendv glGetTexGendv
#define pglGetTexGenfv glGetTexGenfv
#define pglGetTexGeniv glGetTexGeniv
#define pglGetTexImage glGetTexImage
#define pglGetTexLevelParameterfv glGetTexLevelParameterfv
#define pglGetTexLevelParameteriv glGetTexLevelParameteriv
#define pglGetTexParameterfv glGetTexParameterfv
#define pglGetTexParameteriv glGetTexParameteriv
#define pglHint glHint
#define pglIndexMask glIndexMask
#define pglIndexPointer glIndexPointer
#define pglIndexd glIndexd
#define pglIndexdv glIndexdv
#define pglIndexf glIndexf
#define pglIndexfv glIndexfv
#define pglIndexi glIndexi
#define pglIndexiv glIndexiv
#define pglIndexs glIndexs
#define pglIndexsv glIndexsv
#define pglIndexub glIndexub
#define pglIndexubv glIndexubv
#define pglInitNames glInitNames
#define pglInterleavedArrays glInterleavedArrays
#define pglLightModelf glLightModelf
#define pglLightModelfv glLightModelfv
#define pglLightModeli glLightModeli
#define pglLightModeliv glLightModeliv
#define pglLightf glLightf
#define pglLightfv glLightfv
#define pglLighti glLighti
#define pglLightiv glLightiv
#define pglLineStipple glLineStipple
#define pglLineWidth glLineWidth
#define pglListBase glListBase
#define pglLoadIdentity glLoadIdentity
#define pglLoadMatrixd glLoadMatrixd
#define pglLoadMatrixf glLoadMatrixf
#define pglLoadName glLoadName
#define pglLogicOp glLogicOp
#define pglMap1d glMap1d
#define pglMap1f glMap1f
#define pglMap2d glMap2d
#define pglMap2f glMap2f
#define pglMapGrid1d glMapGrid1d
#define pglMapGrid1f glMapGrid1f
#define pglMapGrid2d glMapGrid2d
#define pglMapGrid2f glMapGrid2f
#define pglMaterialf glMaterialf
#define pglMaterialfv glMaterialfv
#define pglMateriali glMateriali
#define pglMaterialiv glMaterialiv
#define pglMatrixMode glMatrixMode
#define pglMultMatrixd glMultMatrixd
#define pglMultMatrixf glMultMatrixf
#define pglNewList glNewList
#define pglNormal3b glNormal3b
#define pglNormal3bv glNormal3bv
#define pglNormal3d glNormal3d
#define pglNormal3dv glNormal3dv
#define pglNormal3f glNormal3f
#define pglNormal3fv glNormal3fv
#define pglNormal3i glNormal3i
#define pglNormal3iv glNormal3iv
#define pglNormal3s glNormal3s
#define pglNormal3sv glNormal3sv
#define pglNormalPointer glNormalPointer
#define pglOrtho glOrtho
#define pglPassThrough glPassThrough
#define pglPixelMapfv glPixelMapfv
#define pglPixelMapuiv glPixelMapuiv
#define pglPixelMapusv glPixelMapusv
#define pglPixelStoref glPixelStoref
#define pglPixelStorei glPixelStorei
#define pglPixelTransferf glPixelTransferf
#define pglPixelTransferi glPixelTransferi
#define pglPixelZoom glPixelZoom
#define pglPointSize glPointSize
#define pglPolygonMode glPolygonMode
#define pglPolygonOffset glPolygonOffset
#define pglPolygonStipple glPolygonStipple
#define pglPopAttrib glPopAttrib
#define pglPopClientAttrib glPopClientAttrib
#define pglPopMatrix glPopMatrix
#define pglPopName glPopName
#define pglPushAttrib glPushAttrib
#define pglPushClientAttrib glPushClientAttrib
#define pglPushMatrix glPushMatrix
#define pglPushName glPushName
#define pglRasterPos2d glRasterPos2d
#define pglRasterPos2dv glRasterPos2dv
#define pglRasterPos2f glRasterPos2f
#define pglRasterPos2fv glRasterPos2fv
#define pglRasterPos2i glRasterPos2i
#define pglRasterPos2iv glRasterPos2iv
#define pglRasterPos2s glRasterPos2s
#define pglRasterPos2sv glRasterPos2sv
#define pglRasterPos3d glRasterPos3d
#define pglRasterPos3dv glRasterPos3dv
#define pglRasterPos3f glRasterPos3f
#define pglRasterPos3fv glRasterPos3fv
#define pglRasterPos3i glRasterPos3i
#define pglRasterPos3iv glRasterPos3iv
#define pglRasterPos3s glRasterPos3s
#define pglRasterPos3sv glRasterPos3sv
#define pglRasterPos4d glRasterPos4d
#define pglRasterPos4dv glRasterPos4dv
#define pglRasterPos4f glRasterPos4f
#define pglRasterPos4fv glRasterPos4fv
#define pglRasterPos4i glRasterPos4i
#define pglRasterPos4iv glRasterPos4iv
#define pglRasterPos4s glRasterPos4s
#define pglRasterPos4sv glRasterPos4sv
#define pglReadBuffer glReadBuffer
#define pglReadPixels glReadPixels
#define pglRectd glRectd
#define pglRectdv glRectdv
#define pglRectf glRectf
#define pglRectfv glRectfv
#define pglRecti glRecti
#define pglRectiv glRectiv
#define pglRects glRects
#define pglRectsv glRectsv
#define pglRotated glRotated
#define pglRotatef glRotatef
#define pglScaled glScaled
#define pglScalef glScalef
#define pglScissor glScissor
#define pglSelectBuffer glSelectBuffer
#define pglShadeModel glShadeModel
#define pglStencilFunc glStencilFunc
#define pglStencilMask glStencilMask
#define pglStencilOp glStencilOp
#define pglTexCoord1d glTexCoord1d
#define pglTexCoord1dv glTexCoord1dv
#define pglTexCoord1f glTexCoord1f
#define pglTexCoord1fv glTexCoord1fv
#define pglTexCoord1i glTexCoord1i
#define pglTexCoord1iv glTexCoord1iv
#define pglTexCoord1s glTexCoord1s
#define pglTexCoord1sv glTexCoord1sv
#define pglTexCoord2d glTexCoord2d
#define pglTexCoord2dv glTexCoord2dv
#define pglTexCoord2f glTexCoord2f
#define pglTexCoord2fv glTexCoord2fv
#define pglTexCoord2i glTexCoord2i
#define pglTexCoord2iv glTexCoord2iv
#define pglTexCoord2s glTexCoord2s
#define pglTexCoord2sv glTexCoord2sv
#define pglTexCoord3d glTexCoord3d
#define pglTexCoord3dv glTexCoord3dv
#define pglTexCoord3f glTexCoord3f
#define pglTexCoord3fv glTexCoord3fv
#define pglTexCoord3i glTexCoord3i
#define pglTexCoord3iv glTexCoord3iv
#define pglTexCoord3s glTexCoord3s
#define pglTexCoord3sv glTexCoord3sv
#define pglTexCoord4d glTexCoord4d
#define pglTexCoord4dv glTexCoord4dv
#define pglTexCoord4f glTexCoord4f
#define pglTexCoord4fv glTexCoord4fv
#define pglTexCoord4i glTexCoord4i
#define pglTexCoord4iv glTexCoord4iv
#define pglTexCoord4s glTexCoord4s
#define pglTexCoord4sv glTexCoord4sv
#define pglTexCoordPointer glTexCoordPointer
#define pglTexEnvf glTexEnvf
#define pglTexEnvfv glTexEnvfv
#define pglTexEnvi glTexEnvi
#define pglTexEnviv glTexEnviv
#define pglTexGend glTexGend
#define pglTexGendv glTexGendv
#define pglTexGenf glTexGenf
#define pglTexGenfv glTexGenfv
#define pglTexGeni glTexGeni
#define pglTexGeniv glTexGeniv
#define pglTexImage1D glTexImage1D
#define pglTexImage2D glTexImage2D
#define pglTexParameterf glTexParameterf
#define pglTexParameterfv glTexParameterfv
#define pglTexParameteri glTexParameteri
#define pglTexParameteriv glTexParameteriv
#define pglTexSubImage1D glTexSubImage1D
#define pglTexSubImage2D glTexSubImage2D
#define pglTranslated glTranslated
#define pglTranslatef glTranslatef
#define pglVertex2d glVertex2d
#define pglVertex2dv glVertex2dv
#define pglVertex2f glVertex2f
#define pglVertex2fv glVertex2fv
#define pglVertex2i glVertex2i
#define pglVertex2iv glVertex2iv
#define pglVertex2s glVertex2s
#define pglVertex2sv glVertex2sv
#define pglVertex3d glVertex3d
#define pglVertex3dv glVertex3dv
#define pglVertex3f glVertex3f
#define pglVertex3fv glVertex3fv
#define pglVertex3i glVertex3i
#define pglVertex3iv glVertex3iv
#define pglVertex3s glVertex3s
#define pglVertex3sv glVertex3sv
#define pglVertex4d glVertex4d
#define pglVertex4dv glVertex4dv
#define pglVertex4f glVertex4f
#define pglVertex4fv glVertex4fv
#define pglVertex4i glVertex4i
#define pglVertex4iv glVertex4iv
#define pglVertex4s glVertex4s
#define pglVertex4sv glVertex4sv
#define pglVertexPointer glVertexPointer
#define pglViewport glViewport
#define pglPointParameterfEXT glPointParameterfEXT
#define pglPointParameterfvEXT glPointParameterfvEXT
#define pglLockArraysEXT glLockArraysEXT
#define pglUnlockArraysEXT glUnlockArraysEXT
#define pglActiveTextureARB glActiveTextureARB
#define pglClientActiveTextureARB glClientActiveTextureARB
#define pglGetCompressedTexImage glGetCompressedTexImage
#define pglDrawRangeElements glDrawRangeElements
#define pglDrawRangeElementsEXT glDrawRangeElementsEXT
#define pglMultiTexCoord1f glMultiTexCoord1f
#define pglMultiTexCoord2f glMultiTexCoord2f
#define pglMultiTexCoord3f glMultiTexCoord3f
#define pglMultiTexCoord4f glMultiTexCoord4f
#define pglActiveTexture glActiveTexture
#define pglClientActiveTexture glClientActiveTexture
#define pglCompressedTexImage3DARB glCompressedTexImage3DARB
#define pglCompressedTexImage2DARB glCompressedTexImage2DARB
#define pglCompressedTexImage1DARB glCompressedTexImage1DARB
#define pglCompressedTexSubImage3DARB glCompressedTexSubImage3DARB
#define pglCompressedTexSubImage2DARB glCompressedTexSubImage2DARB
#define pglCompressedTexSubImage1DARB glCompressedTexSubImage1DARB
#define pglDeleteObjectARB glDeleteObjectARB
#define pglGetHandleARB glGetHandleARB
#define pglDetachObjectARB glDetachObjectARB
#define pglCreateShaderObjectARB glCreateShaderObjectARB
#define pglShaderSourceARB glShaderSourceARB
#define pglCompileShaderARB glCompileShaderARB
#define pglCreateProgramObjectARB glCreateProgramObjectARB
#define pglAttachObjectARB glAttachObjectARB
#define pglLinkProgramARB glLinkProgramARB
#define pglUseProgramObjectARB glUseProgramObjectARB
#define pglValidateProgramARB glValidateProgramARB
#define pglBindProgramARB glBindProgramARB
#define pglDeleteProgramsARB glDeleteProgramsARB
#define pglGenProgramsARB glGenProgramsARB
#define pglProgramStringARB glProgramStringARB
#define pglProgramEnvParameter4fARB glProgramEnvParameter4fARB
#define pglProgramLocalParameter4fARB glProgramLocalParameter4fARB
#define pglUniform1fARB glUniform1fARB
#define pglUniform2fARB glUniform2fARB
#define pglUniform3fARB glUniform3fARB
#define pglUniform4fARB glUniform4fARB
#define pglUniform1iARB glUniform1iARB
#define pglUniform2iARB glUniform2iARB
#define pglUniform3iARB glUniform3iARB
#define pglUniform4iARB glUniform4iARB
#define pglUniform1fvARB glUniform1fvARB
#define pglUniform2fvARB glUniform2fvARB
#define pglUniform3fvARB glUniform3fvARB
#define pglUniform4fvARB glUniform4fvARB
#define pglUniform1ivARB glUniform1ivARB
#define pglUniform2ivARB glUniform2ivARB
#define pglUniform3ivARB glUniform3ivARB
#define pglUniform4ivARB glUniform4ivARB
#define pglUniformMatrix2fvARB glUniformMatrix2fvARB
#define pglUniformMatrix3fvARB glUniformMatrix3fvARB
#define pglUniformMatrix4fvARB glUniformMatrix4fvARB
#define pglGetObjectParameterfvARB glGetObjectParameterfvARB
#define pglGetObjectParameterivARB glGetObjectParameterivARB
#define pglGetInfoLogARB glGetInfoLogARB
#define pglGetAttachedObjectsARB glGetAttachedObjectsARB
#define pglGetUniformLocationARB glGetUniformLocationARB
#define pglGetActiveUniformARB glGetActiveUniformARB
#define pglGetUniformfvARB glGetUniformfvARB
#define pglGetUniformivARB glGetUniformivARB
#define pglGetShaderSourceARB glGetShaderSourceARB
#define pglTexImage3D glTexImage3D
#define pglTexSubImage3D glTexSubImage3D
#define pglCopyTexSubImage3D glCopyTexSubImage3D
#define pglBlendEquationEXT glBlendEquationEXT
#define pglStencilOpSeparate glStencilOpSeparate
#define pglStencilFuncSeparate glStencilFuncSeparate
#define pglActiveStencilFaceEXT glActiveStencilFaceEXT
#define pglVertexAttribPointerARB glVertexAttribPointerARB
#define pglEnableVertexAttribArrayARB glEnableVertexAttribArrayARB
#define pglDisableVertexAttribArrayARB glDisableVertexAttribArrayARB
#define pglBindAttribLocationARB glBindAttribLocationARB
#define pglGetActiveAttribARB glGetActiveAttribARB
#define pglGetAttribLocationARB glGetAttribLocationARB
#define pglBindBufferARB glBindBufferARB
#define pglDeleteBuffersARB glDeleteBuffersARB
#define pglGenBuffersARB glGenBuffersARB
#define pglIsBufferARB glIsBufferARB
#define pglMapBufferARB glMapBufferARB
#define pglUnmapBufferARB glUnmapBufferARB
#define pglBufferDataARB glBufferDataARB
#define pglBufferSubDataARB glBufferSubDataARB
#define pglGenQueriesARB glGenQueriesARB
#define pglDeleteQueriesARB glDeleteQueriesARB
#define pglIsQueryARB glIsQueryARB
#define pglBeginQueryARB glBeginQueryARB
#define pglEndQueryARB glEndQueryARB
#define pglGetQueryivARB glGetQueryivARB
#define pglGetQueryObjectivARB glGetQueryObjectivARB
#define pglGetQueryObjectuivARB glGetQueryObjectuivARB
#define pglSelectTextureSGIS glSelectTextureSGIS
#define pglMTexCoord2fSGIS glMTexCoord2fSGIS
#define pglSwapInterval glSwapInterval

#endif // GL_STATIC_H

