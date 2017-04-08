GLenum GLAPI expglGetError (void)
{
    return ldrpglGetError ();
}

const GLcharARB * GLAPI expglGetString (GLenum name)
{
    return ldrpglGetString (name);
}

void GLAPI expglAccum (GLenum op, GLfloat value)
{
    ldrpglAccum (op, value);
}

void GLAPI expglAlphaFunc (GLenum func, GLclampf ref)
{
    ldrpglAlphaFunc (func, ref);
}

void GLAPI expglBegin (GLenum mode)
{
    ldrpglBegin (mode);
}

void GLAPI expglBindTexture (GLenum target, GLuint texture)
{
    ldrpglBindTexture (target, texture);
}

void GLAPI expglBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
    ldrpglBitmap (width, height, xorig, yorig, xmove, ymove, bitmap);
}

void GLAPI expglBlendFunc (GLenum sfactor, GLenum dfactor)
{
    ldrpglBlendFunc (sfactor, dfactor);
}

void GLAPI expglCallList (GLuint list)
{
    ldrpglCallList (list);
}

void GLAPI expglCallLists (GLsizei n, GLenum type, const GLvoid *lists)
{
    ldrpglCallLists (n, type, lists);
}

void GLAPI expglClear (GLbitfield mask)
{
    ldrpglClear (mask);
}

void GLAPI expglClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    ldrpglClearAccum (red, green, blue, alpha);
}

void GLAPI expglClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    ldrpglClearColor (red, green, blue, alpha);
}

void GLAPI expglClearDepth (GLclampd depth)
{
    ldrpglClearDepth (depth);
}

void GLAPI expglClearIndex (GLfloat c)
{
    ldrpglClearIndex (c);
}

void GLAPI expglClearStencil (GLint s)
{
    ldrpglClearStencil (s);
}

GLboolean GLAPI expglIsEnabled ( GLenum cap)
{
    return ldrpglIsEnabled (cap);
}

GLboolean GLAPI expglIsList ( GLuint list)
{
    return ldrpglIsList (list);
}

GLboolean GLAPI expglIsTexture ( GLuint texture)
{
    return ldrpglIsTexture (texture);
}

void GLAPI expglClipPlane (GLenum plane, const GLdouble *equation)
{
    ldrpglClipPlane (plane, equation);
}

void GLAPI expglColor3b (GLbyte red, GLbyte green, GLbyte blue)
{
    ldrpglColor3b (red, green, blue);
}

void GLAPI expglColor3bv (const GLbyte *v)
{
    ldrpglColor3bv (v);
}

void GLAPI expglColor3d (GLdouble red, GLdouble green, GLdouble blue)
{
    ldrpglColor3d (red, green, blue);
}

void GLAPI expglColor3dv (const GLdouble *v)
{
    ldrpglColor3dv (v);
}

void GLAPI expglColor3f (GLfloat red, GLfloat green, GLfloat blue)
{
    ldrpglColor3f (red, green, blue);
}

void GLAPI expglColor3fv (const GLfloat *v)
{
    ldrpglColor3fv (v);
}

void GLAPI expglColor3i (GLint red, GLint green, GLint blue)
{
    ldrpglColor3i (red, green, blue);
}

void GLAPI expglColor3iv (const GLint *v)
{
    ldrpglColor3iv (v);
}

void GLAPI expglColor3s (GLshort red, GLshort green, GLshort blue)
{
    ldrpglColor3s (red, green, blue);
}

void GLAPI expglColor3sv (const GLshort *v)
{
    ldrpglColor3sv (v);
}

void GLAPI expglColor3ub (GLubyte red, GLubyte green, GLubyte blue)
{
    ldrpglColor3ub (red, green, blue);
}

void GLAPI expglColor3ubv (const GLubyte *v)
{
    ldrpglColor3ubv (v);
}

void GLAPI expglColor3ui (GLuint red, GLuint green, GLuint blue)
{
    ldrpglColor3ui (red, green, blue);
}

void GLAPI expglColor3uiv (const GLuint *v)
{
    ldrpglColor3uiv (v);
}

void GLAPI expglColor3us (GLushort red, GLushort green, GLushort blue)
{
    ldrpglColor3us (red, green, blue);
}

void GLAPI expglColor3usv (const GLushort *v)
{
    ldrpglColor3usv (v);
}

void GLAPI expglColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
    ldrpglColor4b (red, green, blue, alpha);
}

void GLAPI expglColor4bv (const GLbyte *v)
{
    ldrpglColor4bv (v);
}

void GLAPI expglColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    ldrpglColor4d (red, green, blue, alpha);
}

void GLAPI expglColor4dv (const GLdouble *v)
{
    ldrpglColor4dv (v);
}

void GLAPI expglColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    ldrpglColor4f (red, green, blue, alpha);
}

void GLAPI expglColor4fv (const GLfloat *v)
{
    ldrpglColor4fv (v);
}

void GLAPI expglColor4i (GLint red, GLint green, GLint blue, GLint alpha)
{
    ldrpglColor4i (red, green, blue, alpha);
}

void GLAPI expglColor4iv (const GLint *v)
{
    ldrpglColor4iv (v);
}

void GLAPI expglColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
    ldrpglColor4s (red, green, blue, alpha);
}

void GLAPI expglColor4sv (const GLshort *v)
{
    ldrpglColor4sv (v);
}

void GLAPI expglColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    ldrpglColor4ub (red, green, blue, alpha);
}

void GLAPI expglColor4ubv (const GLubyte *v)
{
    ldrpglColor4ubv (v);
}

void GLAPI expglColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    ldrpglColor4ui (red, green, blue, alpha);
}

void GLAPI expglColor4uiv (const GLuint *v)
{
    ldrpglColor4uiv (v);
}

void GLAPI expglColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
    ldrpglColor4us (red, green, blue, alpha);
}

void GLAPI expglColor4usv (const GLushort *v)
{
    ldrpglColor4usv (v);
}

void GLAPI expglColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    ldrpglColorMask (red, green, blue, alpha);
}

void GLAPI expglColorMaterial (GLenum face, GLenum mode)
{
    ldrpglColorMaterial (face, mode);
}

void GLAPI expglCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    ldrpglCopyPixels (x, y, width, height, type);
}

void GLAPI expglCopyTexImage1D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border)
{
    ldrpglCopyTexImage1D (target, level, internalFormat, x, y, width, border);
}

void GLAPI expglCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    ldrpglCopyTexImage2D (target, level, internalFormat, x, y, width, height, border);
}

void GLAPI expglCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    ldrpglCopyTexSubImage1D (target, level, xoffset, x, y, width);
}

void GLAPI expglCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    ldrpglCopyTexSubImage2D (target, level, xoffset, yoffset, x, y, width, height);
}

void GLAPI expglCullFace (GLenum mode)
{
    ldrpglCullFace (mode);
}

void GLAPI expglDeleteLists (GLuint list, GLsizei range)
{
    ldrpglDeleteLists (list, range);
}

void GLAPI expglDeleteTextures (GLsizei n, const GLuint *textures)
{
    ldrpglDeleteTextures (n, textures);
}

void GLAPI expglDepthFunc (GLenum func)
{
    ldrpglDepthFunc (func);
}

void GLAPI expglDepthMask (GLboolean flag)
{
    ldrpglDepthMask (flag);
}

void GLAPI expglDepthRange (GLclampd zNear, GLclampd zFar)
{
    ldrpglDepthRange (zNear, zFar);
}

void GLAPI expglDisable (GLenum cap)
{
    ldrpglDisable (cap);
}

void GLAPI expglDisableClientState (GLenum array)
{
    ldrpglDisableClientState (array);
}

void GLAPI expglDrawArrays (GLenum mode, GLint first, GLsizei count)
{
    ldrpglDrawArrays (mode, first, count);
}

void GLAPI expglDrawBuffer (GLenum mode)
{
    ldrpglDrawBuffer (mode);
}

void GLAPI expglDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
    ldrpglDrawPixels (width, height, format, type, pixels);
}

void GLAPI expglEdgeFlag (GLboolean flag)
{
    ldrpglEdgeFlag (flag);
}

void GLAPI expglEdgeFlagPointer (GLsizei stride, const GLvoid *pointer)
{
    ldrpglEdgeFlagPointer (stride, pointer);
}

void GLAPI expglEdgeFlagv (const GLboolean *flag)
{
    ldrpglEdgeFlagv (flag);
}

void GLAPI expglEnable (GLenum cap)
{
    ldrpglEnable (cap);
}

void GLAPI expglEnableClientState (GLenum array)
{
    ldrpglEnableClientState (array);
}

void GLAPI expglEnd (void)
{
    ldrpglEnd ();
}

void GLAPI expglEndList (void)
{
    ldrpglEndList ();
}

void GLAPI expglEvalCoord1d (GLdouble u)
{
    ldrpglEvalCoord1d (u);
}

void GLAPI expglEvalCoord1dv (const GLdouble *u)
{
    ldrpglEvalCoord1dv (u);
}

void GLAPI expglEvalCoord1f (GLfloat u)
{
    ldrpglEvalCoord1f (u);
}

void GLAPI expglEvalCoord1fv (const GLfloat *u)
{
    ldrpglEvalCoord1fv (u);
}

void GLAPI expglEvalCoord2d (GLdouble u, GLdouble v)
{
    ldrpglEvalCoord2d (u, v);
}

void GLAPI expglEvalCoord2dv (const GLdouble *u)
{
    ldrpglEvalCoord2dv (u);
}

void GLAPI expglEvalCoord2f (GLfloat u, GLfloat v)
{
    ldrpglEvalCoord2f (u, v);
}

void GLAPI expglEvalCoord2fv (const GLfloat *u)
{
    ldrpglEvalCoord2fv (u);
}

void GLAPI expglEvalMesh1 (GLenum mode, GLint i1, GLint i2)
{
    ldrpglEvalMesh1 (mode, i1, i2);
}

void GLAPI expglEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    ldrpglEvalMesh2 (mode, i1, i2, j1, j2);
}

void GLAPI expglEvalPoint1 (GLint i)
{
    ldrpglEvalPoint1 (i);
}

void GLAPI expglEvalPoint2 (GLint i, GLint j)
{
    ldrpglEvalPoint2 (i, j);
}

void GLAPI expglFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer)
{
    ldrpglFeedbackBuffer (size, type, buffer);
}

void GLAPI expglFinish (void)
{
    ldrpglFinish ();
}

void GLAPI expglFlush (void)
{
    ldrpglFlush ();
}

void GLAPI expglFogf (GLenum pname, GLfloat param)
{
    ldrpglFogf (pname, param);
}

void GLAPI expglFogfv (GLenum pname, const GLfloat *params)
{
    ldrpglFogfv (pname, params);
}

void GLAPI expglFogi (GLenum pname, GLint param)
{
    ldrpglFogi (pname, param);
}

void GLAPI expglFogiv (GLenum pname, const GLint *params)
{
    ldrpglFogiv (pname, params);
}

void GLAPI expglFrontFace (GLenum mode)
{
    ldrpglFrontFace (mode);
}

void GLAPI expglFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    ldrpglFrustum (left, right, bottom, top, zNear, zFar);
}

void GLAPI expglGenTextures (GLsizei n, GLuint *textures)
{
    ldrpglGenTextures (n, textures);
}

void GLAPI expglGetBooleanv (GLenum pname, GLboolean *params)
{
    ldrpglGetBooleanv (pname, params);
}

void GLAPI expglGetClipPlane (GLenum plane, GLdouble *equation)
{
    ldrpglGetClipPlane (plane, equation);
}

void GLAPI expglGetDoublev (GLenum pname, GLdouble *params)
{
    ldrpglGetDoublev (pname, params);
}

void GLAPI expglGetFloatv (GLenum pname, GLfloat *params)
{
    ldrpglGetFloatv (pname, params);
}

void GLAPI expglGetIntegerv (GLenum pname, GLint *params)
{
    ldrpglGetIntegerv (pname, params);
}

void GLAPI expglGetLightfv (GLenum light, GLenum pname, GLfloat *params)
{
    ldrpglGetLightfv (light, pname, params);
}

void GLAPI expglGetLightiv (GLenum light, GLenum pname, GLint *params)
{
    ldrpglGetLightiv (light, pname, params);
}

void GLAPI expglGetMapdv (GLenum target, GLenum query, GLdouble *v)
{
    ldrpglGetMapdv (target, query, v);
}

void GLAPI expglGetMapfv (GLenum target, GLenum query, GLfloat *v)
{
    ldrpglGetMapfv (target, query, v);
}

void GLAPI expglGetMapiv (GLenum target, GLenum query, GLint *v)
{
    ldrpglGetMapiv (target, query, v);
}

void GLAPI expglGetMaterialfv (GLenum face, GLenum pname, GLfloat *params)
{
    ldrpglGetMaterialfv (face, pname, params);
}

void GLAPI expglGetMaterialiv (GLenum face, GLenum pname, GLint *params)
{
    ldrpglGetMaterialiv (face, pname, params);
}

void GLAPI expglGetPixelMapfv (GLenum map, GLfloat *values)
{
    ldrpglGetPixelMapfv (map, values);
}

void GLAPI expglGetPixelMapuiv (GLenum map, GLuint *values)
{
    ldrpglGetPixelMapuiv (map, values);
}

void GLAPI expglGetPixelMapusv (GLenum map, GLushort *values)
{
    ldrpglGetPixelMapusv (map, values);
}

void GLAPI expglGetPointerv (GLenum pname, GLvoid* *params)
{
    ldrpglGetPointerv (pname, params);
}

void GLAPI expglGetPolygonStipple (GLubyte *mask)
{
    ldrpglGetPolygonStipple (mask);
}

void GLAPI expglGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params)
{
    ldrpglGetTexEnvfv (target, pname, params);
}

void GLAPI expglGetTexEnviv (GLenum target, GLenum pname, GLint *params)
{
    ldrpglGetTexEnviv (target, pname, params);
}

void GLAPI expglGetTexGendv (GLenum coord, GLenum pname, GLdouble *params)
{
    ldrpglGetTexGendv (coord, pname, params);
}

void GLAPI expglGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params)
{
    ldrpglGetTexGenfv (coord, pname, params);
}

void GLAPI expglGetTexGeniv (GLenum coord, GLenum pname, GLint *params)
{
    ldrpglGetTexGeniv (coord, pname, params);
}

void GLAPI expglGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
    ldrpglGetTexImage (target, level, format, type, pixels);
}

void GLAPI expglGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params)
{
    ldrpglGetTexLevelParameterfv (target, level, pname, params);
}

void GLAPI expglGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params)
{
    ldrpglGetTexLevelParameteriv (target, level, pname, params);
}

void GLAPI expglGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params)
{
    ldrpglGetTexParameterfv (target, pname, params);
}

void GLAPI expglGetTexParameteriv (GLenum target, GLenum pname, GLint *params)
{
    ldrpglGetTexParameteriv (target, pname, params);
}

void GLAPI expglHint (GLenum target, GLenum mode)
{
    ldrpglHint (target, mode);
}

void GLAPI expglIndexMask (GLuint mask)
{
    ldrpglIndexMask (mask);
}

void GLAPI expglIndexPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{
    ldrpglIndexPointer (type, stride, pointer);
}

void GLAPI expglIndexd (GLdouble c)
{
    ldrpglIndexd (c);
}

void GLAPI expglIndexdv (const GLdouble *c)
{
    ldrpglIndexdv (c);
}

void GLAPI expglIndexf (GLfloat c)
{
    ldrpglIndexf (c);
}

void GLAPI expglIndexfv (const GLfloat *c)
{
    ldrpglIndexfv (c);
}

void GLAPI expglIndexi (GLint c)
{
    ldrpglIndexi (c);
}

void GLAPI expglIndexiv (const GLint *c)
{
    ldrpglIndexiv (c);
}

void GLAPI expglIndexs (GLshort c)
{
    ldrpglIndexs (c);
}

void GLAPI expglIndexsv (const GLshort *c)
{
    ldrpglIndexsv (c);
}

void GLAPI expglIndexub (GLubyte c)
{
    ldrpglIndexub (c);
}

void GLAPI expglIndexubv (const GLubyte *c)
{
    ldrpglIndexubv (c);
}

void GLAPI expglInitNames (void)
{
    ldrpglInitNames ();
}

void GLAPI expglInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer)
{
    ldrpglInterleavedArrays (format, stride, pointer);
}

void GLAPI expglLightModelf (GLenum pname, GLfloat param)
{
    ldrpglLightModelf (pname, param);
}

void GLAPI expglLightModelfv (GLenum pname, const GLfloat *params)
{
    ldrpglLightModelfv (pname, params);
}

void GLAPI expglLightModeli (GLenum pname, GLint param)
{
    ldrpglLightModeli (pname, param);
}

void GLAPI expglLightModeliv (GLenum pname, const GLint *params)
{
    ldrpglLightModeliv (pname, params);
}

void GLAPI expglLightf (GLenum light, GLenum pname, GLfloat param)
{
    ldrpglLightf (light, pname, param);
}

void GLAPI expglLightfv (GLenum light, GLenum pname, const GLfloat *params)
{
    ldrpglLightfv (light, pname, params);
}

void GLAPI expglLighti (GLenum light, GLenum pname, GLint param)
{
    ldrpglLighti (light, pname, param);
}

void GLAPI expglLightiv (GLenum light, GLenum pname, const GLint *params)
{
    ldrpglLightiv (light, pname, params);
}

void GLAPI expglLineStipple (GLint factor, GLushort pattern)
{
    ldrpglLineStipple (factor, pattern);
}

void GLAPI expglLineWidth (GLfloat width)
{
    ldrpglLineWidth (width);
}

void GLAPI expglListBase (GLuint base)
{
    ldrpglListBase (base);
}

void GLAPI expglLoadIdentity (void)
{
    ldrpglLoadIdentity ();
}

void GLAPI expglLoadMatrixd (const GLdouble *m)
{
    ldrpglLoadMatrixd (m);
}

void GLAPI expglLoadMatrixf (const GLfloat *m)
{
    ldrpglLoadMatrixf (m);
}

void GLAPI expglLoadName (GLuint name)
{
    ldrpglLoadName (name);
}

void GLAPI expglLogicOp (GLenum opcode)
{
    ldrpglLogicOp (opcode);
}

void GLAPI expglMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
    ldrpglMap1d (target, u1, u2, stride, order, points);
}

void GLAPI expglMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
    ldrpglMap1f (target, u1, u2, stride, order, points);
}

void GLAPI expglMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
    ldrpglMap2d (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void GLAPI expglMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
    ldrpglMap2f (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void GLAPI expglMapGrid1d (GLint un, GLdouble u1, GLdouble u2)
{
    ldrpglMapGrid1d (un, u1, u2);
}

void GLAPI expglMapGrid1f (GLint un, GLfloat u1, GLfloat u2)
{
    ldrpglMapGrid1f (un, u1, u2);
}

void GLAPI expglMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    ldrpglMapGrid2d (un, u1, u2, vn, v1, v2);
}

void GLAPI expglMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    ldrpglMapGrid2f (un, u1, u2, vn, v1, v2);
}

void GLAPI expglMaterialf (GLenum face, GLenum pname, GLfloat param)
{
    ldrpglMaterialf (face, pname, param);
}

void GLAPI expglMaterialfv (GLenum face, GLenum pname, const GLfloat *params)
{
    ldrpglMaterialfv (face, pname, params);
}

void GLAPI expglMateriali (GLenum face, GLenum pname, GLint param)
{
    ldrpglMateriali (face, pname, param);
}

void GLAPI expglMaterialiv (GLenum face, GLenum pname, const GLint *params)
{
    ldrpglMaterialiv (face, pname, params);
}

void GLAPI expglMatrixMode (GLenum mode)
{
    ldrpglMatrixMode (mode);
}

void GLAPI expglMultMatrixd (const GLdouble *m)
{
    ldrpglMultMatrixd (m);
}

void GLAPI expglMultMatrixf (const GLfloat *m)
{
    ldrpglMultMatrixf (m);
}

void GLAPI expglNewList (GLuint list, GLenum mode)
{
    ldrpglNewList (list, mode);
}

void GLAPI expglNormal3b (GLbyte nx, GLbyte ny, GLbyte nz)
{
    ldrpglNormal3b (nx, ny, nz);
}

void GLAPI expglNormal3bv (const GLbyte *v)
{
    ldrpglNormal3bv (v);
}

void GLAPI expglNormal3d (GLdouble nx, GLdouble ny, GLdouble nz)
{
    ldrpglNormal3d (nx, ny, nz);
}

void GLAPI expglNormal3dv (const GLdouble *v)
{
    ldrpglNormal3dv (v);
}

void GLAPI expglNormal3f (GLfloat nx, GLfloat ny, GLfloat nz)
{
    ldrpglNormal3f (nx, ny, nz);
}

void GLAPI expglNormal3fv (const GLfloat *v)
{
    ldrpglNormal3fv (v);
}

void GLAPI expglNormal3i (GLint nx, GLint ny, GLint nz)
{
    ldrpglNormal3i (nx, ny, nz);
}

void GLAPI expglNormal3iv (const GLint *v)
{
    ldrpglNormal3iv (v);
}

void GLAPI expglNormal3s (GLshort nx, GLshort ny, GLshort nz)
{
    ldrpglNormal3s (nx, ny, nz);
}

void GLAPI expglNormal3sv (const GLshort *v)
{
    ldrpglNormal3sv (v);
}

void GLAPI expglOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    ldrpglOrtho (left, right, bottom, top, zNear, zFar);
}

void GLAPI expglPassThrough (GLfloat token)
{
    ldrpglPassThrough (token);
}

void GLAPI expglPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat *values)
{
    ldrpglPixelMapfv (map, mapsize, values);
}

void GLAPI expglPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values)
{
    ldrpglPixelMapuiv (map, mapsize, values);
}

void GLAPI expglPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values)
{
    ldrpglPixelMapusv (map, mapsize, values);
}

void GLAPI expglPixelStoref (GLenum pname, GLfloat param)
{
    ldrpglPixelStoref (pname, param);
}

void GLAPI expglPixelStorei (GLenum pname, GLint param)
{
    ldrpglPixelStorei (pname, param);
}

void GLAPI expglPixelTransferf (GLenum pname, GLfloat param)
{
    ldrpglPixelTransferf (pname, param);
}

void GLAPI expglPixelTransferi (GLenum pname, GLint param)
{
    ldrpglPixelTransferi (pname, param);
}

void GLAPI expglPixelZoom (GLfloat xfactor, GLfloat yfactor)
{
    ldrpglPixelZoom (xfactor, yfactor);
}

void GLAPI expglPointSize (GLfloat size)
{
    ldrpglPointSize (size);
}

void GLAPI expglPolygonMode (GLenum face, GLenum mode)
{
    ldrpglPolygonMode (face, mode);
}

void GLAPI expglPolygonOffset (GLfloat factor, GLfloat units)
{
    ldrpglPolygonOffset (factor, units);
}

void GLAPI expglPolygonStipple (const GLubyte *mask)
{
    ldrpglPolygonStipple (mask);
}

void GLAPI expglPopAttrib (void)
{
    ldrpglPopAttrib ();
}

void GLAPI expglPopClientAttrib (void)
{
    ldrpglPopClientAttrib ();
}

void GLAPI expglPopMatrix (void)
{
    ldrpglPopMatrix ();
}

void GLAPI expglPopName (void)
{
    ldrpglPopName ();
}

void GLAPI expglPushAttrib (GLbitfield mask)
{
    ldrpglPushAttrib (mask);
}

void GLAPI expglPushClientAttrib (GLbitfield mask)
{
    ldrpglPushClientAttrib (mask);
}

void GLAPI expglPushMatrix (void)
{
    ldrpglPushMatrix ();
}

void GLAPI expglPushName (GLuint name)
{
    ldrpglPushName (name);
}

void GLAPI expglRasterPos2d (GLdouble x, GLdouble y)
{
    ldrpglRasterPos2d (x, y);
}

void GLAPI expglRasterPos2dv (const GLdouble *v)
{
    ldrpglRasterPos2dv (v);
}

void GLAPI expglRasterPos2f (GLfloat x, GLfloat y)
{
    ldrpglRasterPos2f (x, y);
}

void GLAPI expglRasterPos2fv (const GLfloat *v)
{
    ldrpglRasterPos2fv (v);
}

void GLAPI expglRasterPos2i (GLint x, GLint y)
{
    ldrpglRasterPos2i (x, y);
}

void GLAPI expglRasterPos2iv (const GLint *v)
{
    ldrpglRasterPos2iv (v);
}

void GLAPI expglRasterPos2s (GLshort x, GLshort y)
{
    ldrpglRasterPos2s (x, y);
}

void GLAPI expglRasterPos2sv (const GLshort *v)
{
    ldrpglRasterPos2sv (v);
}

void GLAPI expglRasterPos3d (GLdouble x, GLdouble y, GLdouble z)
{
    ldrpglRasterPos3d (x, y, z);
}

void GLAPI expglRasterPos3dv (const GLdouble *v)
{
    ldrpglRasterPos3dv (v);
}

void GLAPI expglRasterPos3f (GLfloat x, GLfloat y, GLfloat z)
{
    ldrpglRasterPos3f (x, y, z);
}

void GLAPI expglRasterPos3fv (const GLfloat *v)
{
    ldrpglRasterPos3fv (v);
}

void GLAPI expglRasterPos3i (GLint x, GLint y, GLint z)
{
    ldrpglRasterPos3i (x, y, z);
}

void GLAPI expglRasterPos3iv (const GLint *v)
{
    ldrpglRasterPos3iv (v);
}

void GLAPI expglRasterPos3s (GLshort x, GLshort y, GLshort z)
{
    ldrpglRasterPos3s (x, y, z);
}

void GLAPI expglRasterPos3sv (const GLshort *v)
{
    ldrpglRasterPos3sv (v);
}

void GLAPI expglRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    ldrpglRasterPos4d (x, y, z, w);
}

void GLAPI expglRasterPos4dv (const GLdouble *v)
{
    ldrpglRasterPos4dv (v);
}

void GLAPI expglRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    ldrpglRasterPos4f (x, y, z, w);
}

void GLAPI expglRasterPos4fv (const GLfloat *v)
{
    ldrpglRasterPos4fv (v);
}

void GLAPI expglRasterPos4i (GLint x, GLint y, GLint z, GLint w)
{
    ldrpglRasterPos4i (x, y, z, w);
}

void GLAPI expglRasterPos4iv (const GLint *v)
{
    ldrpglRasterPos4iv (v);
}

void GLAPI expglRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w)
{
    ldrpglRasterPos4s (x, y, z, w);
}

void GLAPI expglRasterPos4sv (const GLshort *v)
{
    ldrpglRasterPos4sv (v);
}

void GLAPI expglReadBuffer (GLenum mode)
{
    ldrpglReadBuffer (mode);
}

void GLAPI expglReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
    ldrpglReadPixels (x, y, width, height, format, type, pixels);
}

void GLAPI expglRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    ldrpglRectd (x1, y1, x2, y2);
}

void GLAPI expglRectdv (const GLdouble *v1, const GLdouble *v2)
{
    ldrpglRectdv (v1, v2);
}

void GLAPI expglRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    ldrpglRectf (x1, y1, x2, y2);
}

void GLAPI expglRectfv (const GLfloat *v1, const GLfloat *v2)
{
    ldrpglRectfv (v1, v2);
}

void GLAPI expglRecti (GLint x1, GLint y1, GLint x2, GLint y2)
{
    ldrpglRecti (x1, y1, x2, y2);
}

void GLAPI expglRectiv (const GLint *v1, const GLint *v2)
{
    ldrpglRectiv (v1, v2);
}

void GLAPI expglRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    ldrpglRects (x1, y1, x2, y2);
}

void GLAPI expglRectsv (const GLshort *v1, const GLshort *v2)
{
    ldrpglRectsv (v1, v2);
}

void GLAPI expglRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    ldrpglRotated (angle, x, y, z);
}

void GLAPI expglRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    ldrpglRotatef (angle, x, y, z);
}

void GLAPI expglScaled (GLdouble x, GLdouble y, GLdouble z)
{
    ldrpglScaled (x, y, z);
}

void GLAPI expglScalef (GLfloat x, GLfloat y, GLfloat z)
{
    ldrpglScalef (x, y, z);
}

void GLAPI expglScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
    ldrpglScissor (x, y, width, height);
}

void GLAPI expglSelectBuffer (GLsizei size, GLuint *buffer)
{
    ldrpglSelectBuffer (size, buffer);
}

void GLAPI expglShadeModel (GLenum mode)
{
    ldrpglShadeModel (mode);
}

void GLAPI expglStencilFunc (GLenum func, GLint ref, GLuint mask)
{
    ldrpglStencilFunc (func, ref, mask);
}

void GLAPI expglStencilMask (GLuint mask)
{
    ldrpglStencilMask (mask);
}

void GLAPI expglStencilOp (GLenum fail, GLenum zfail, GLenum zpass)
{
    ldrpglStencilOp (fail, zfail, zpass);
}

void GLAPI expglTexCoord1d (GLdouble s)
{
    ldrpglTexCoord1d (s);
}

void GLAPI expglTexCoord1dv (const GLdouble *v)
{
    ldrpglTexCoord1dv (v);
}

void GLAPI expglTexCoord1f (GLfloat s)
{
    ldrpglTexCoord1f (s);
}

void GLAPI expglTexCoord1fv (const GLfloat *v)
{
    ldrpglTexCoord1fv (v);
}

void GLAPI expglTexCoord1i (GLint s)
{
    ldrpglTexCoord1i (s);
}

void GLAPI expglTexCoord1iv (const GLint *v)
{
    ldrpglTexCoord1iv (v);
}

void GLAPI expglTexCoord1s (GLshort s)
{
    ldrpglTexCoord1s (s);
}

void GLAPI expglTexCoord1sv (const GLshort *v)
{
    ldrpglTexCoord1sv (v);
}

void GLAPI expglTexCoord2d (GLdouble s, GLdouble t)
{
    ldrpglTexCoord2d (s, t);
}

void GLAPI expglTexCoord2dv (const GLdouble *v)
{
    ldrpglTexCoord2dv (v);
}

void GLAPI expglTexCoord2f (GLfloat s, GLfloat t)
{
    ldrpglTexCoord2f (s, t);
}

void GLAPI expglTexCoord2fv (const GLfloat *v)
{
    ldrpglTexCoord2fv (v);
}

void GLAPI expglTexCoord2i (GLint s, GLint t)
{
    ldrpglTexCoord2i (s, t);
}

void GLAPI expglTexCoord2iv (const GLint *v)
{
    ldrpglTexCoord2iv (v);
}

void GLAPI expglTexCoord2s (GLshort s, GLshort t)
{
    ldrpglTexCoord2s (s, t);
}

void GLAPI expglTexCoord2sv (const GLshort *v)
{
    ldrpglTexCoord2sv (v);
}

void GLAPI expglTexCoord3d (GLdouble s, GLdouble t, GLdouble r)
{
    ldrpglTexCoord3d (s, t, r);
}

void GLAPI expglTexCoord3dv (const GLdouble *v)
{
    ldrpglTexCoord3dv (v);
}

void GLAPI expglTexCoord3f (GLfloat s, GLfloat t, GLfloat r)
{
    ldrpglTexCoord3f (s, t, r);
}

void GLAPI expglTexCoord3fv (const GLfloat *v)
{
    ldrpglTexCoord3fv (v);
}

void GLAPI expglTexCoord3i (GLint s, GLint t, GLint r)
{
    ldrpglTexCoord3i (s, t, r);
}

void GLAPI expglTexCoord3iv (const GLint *v)
{
    ldrpglTexCoord3iv (v);
}

void GLAPI expglTexCoord3s (GLshort s, GLshort t, GLshort r)
{
    ldrpglTexCoord3s (s, t, r);
}

void GLAPI expglTexCoord3sv (const GLshort *v)
{
    ldrpglTexCoord3sv (v);
}

void GLAPI expglTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    ldrpglTexCoord4d (s, t, r, q);
}

void GLAPI expglTexCoord4dv (const GLdouble *v)
{
    ldrpglTexCoord4dv (v);
}

void GLAPI expglTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    ldrpglTexCoord4f (s, t, r, q);
}

void GLAPI expglTexCoord4fv (const GLfloat *v)
{
    ldrpglTexCoord4fv (v);
}

void GLAPI expglTexCoord4i (GLint s, GLint t, GLint r, GLint q)
{
    ldrpglTexCoord4i (s, t, r, q);
}

void GLAPI expglTexCoord4iv (const GLint *v)
{
    ldrpglTexCoord4iv (v);
}

void GLAPI expglTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q)
{
    ldrpglTexCoord4s (s, t, r, q);
}

void GLAPI expglTexCoord4sv (const GLshort *v)
{
    ldrpglTexCoord4sv (v);
}

void GLAPI expglTexEnvf (GLenum target, GLenum pname, GLfloat param)
{
    ldrpglTexEnvf (target, pname, param);
}

void GLAPI expglTexEnvfv (GLenum target, GLenum pname, const GLfloat *params)
{
    ldrpglTexEnvfv (target, pname, params);
}

void GLAPI expglTexEnvi (GLenum target, GLenum pname, GLint param)
{
    ldrpglTexEnvi (target, pname, param);
}

void GLAPI expglTexEnviv (GLenum target, GLenum pname, const GLint *params)
{
    ldrpglTexEnviv (target, pname, params);
}

void GLAPI expglTexGend (GLenum coord, GLenum pname, GLdouble param)
{
    ldrpglTexGend (coord, pname, param);
}

void GLAPI expglTexGendv (GLenum coord, GLenum pname, const GLdouble *params)
{
    ldrpglTexGendv (coord, pname, params);
}

void GLAPI expglTexGenf (GLenum coord, GLenum pname, GLfloat param)
{
    ldrpglTexGenf (coord, pname, param);
}

void GLAPI expglTexGenfv (GLenum coord, GLenum pname, const GLfloat *params)
{
    ldrpglTexGenfv (coord, pname, params);
}

void GLAPI expglTexGeni (GLenum coord, GLenum pname, GLint param)
{
    ldrpglTexGeni (coord, pname, param);
}

void GLAPI expglTexGeniv (GLenum coord, GLenum pname, const GLint *params)
{
    ldrpglTexGeniv (coord, pname, params);
}

void GLAPI expglTexImage1D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    ldrpglTexImage1D (target, level, internalformat, width, border, format, type, pixels);
}

void GLAPI expglTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    ldrpglTexImage2D (target, level, internalformat, width, height, border, format, type, pixels);
}

void GLAPI expglTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
    ldrpglTexParameterf (target, pname, param);
}

void GLAPI expglTexParameterfv (GLenum target, GLenum pname, const GLfloat *params)
{
    ldrpglTexParameterfv (target, pname, params);
}

void GLAPI expglTexParameteri (GLenum target, GLenum pname, GLint param)
{
    ldrpglTexParameteri (target, pname, param);
}

void GLAPI expglTexParameteriv (GLenum target, GLenum pname, const GLint *params)
{
    ldrpglTexParameteriv (target, pname, params);
}

void GLAPI expglTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
    ldrpglTexSubImage1D (target, level, xoffset, width, format, type, pixels);
}

void GLAPI expglTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
    ldrpglTexSubImage2D (target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void GLAPI expglTranslated (GLdouble x, GLdouble y, GLdouble z)
{
    ldrpglTranslated (x, y, z);
}

void GLAPI expglTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
    ldrpglTranslatef (x, y, z);
}

void GLAPI expglVertex2d (GLdouble x, GLdouble y)
{
    ldrpglVertex2d (x, y);
}

void GLAPI expglVertex2dv (const GLdouble *v)
{
    ldrpglVertex2dv (v);
}

void GLAPI expglVertex2f (GLfloat x, GLfloat y)
{
    ldrpglVertex2f (x, y);
}

void GLAPI expglVertex2fv (const GLfloat *v)
{
    ldrpglVertex2fv (v);
}

void GLAPI expglVertex2i (GLint x, GLint y)
{
    ldrpglVertex2i (x, y);
}

void GLAPI expglVertex2iv (const GLint *v)
{
    ldrpglVertex2iv (v);
}

void GLAPI expglVertex2s (GLshort x, GLshort y)
{
    ldrpglVertex2s (x, y);
}

void GLAPI expglVertex2sv (const GLshort *v)
{
    ldrpglVertex2sv (v);
}

void GLAPI expglVertex3d (GLdouble x, GLdouble y, GLdouble z)
{
    ldrpglVertex3d (x, y, z);
}

void GLAPI expglVertex3dv (const GLdouble *v)
{
    ldrpglVertex3dv (v);
}

void GLAPI expglVertex3f (GLfloat x, GLfloat y, GLfloat z)
{
    ldrpglVertex3f (x, y, z);
}

void GLAPI expglVertex3fv (const GLfloat *v)
{
    ldrpglVertex3fv (v);
}

void GLAPI expglVertex3i (GLint x, GLint y, GLint z)
{
    ldrpglVertex3i (x, y, z);
}

void GLAPI expglVertex3iv (const GLint *v)
{
    ldrpglVertex3iv (v);
}

void GLAPI expglVertex3s (GLshort x, GLshort y, GLshort z)
{
    ldrpglVertex3s (x, y, z);
}

void GLAPI expglVertex3sv (const GLshort *v)
{
    ldrpglVertex3sv (v);
}

void GLAPI expglVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    ldrpglVertex4d (x, y, z, w);
}

void GLAPI expglVertex4dv (const GLdouble *v)
{
    ldrpglVertex4dv (v);
}

void GLAPI expglVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    ldrpglVertex4f (x, y, z, w);
}

void GLAPI expglVertex4fv (const GLfloat *v)
{
    ldrpglVertex4fv (v);
}

void GLAPI expglVertex4i (GLint x, GLint y, GLint z, GLint w)
{
    ldrpglVertex4i (x, y, z, w);
}

void GLAPI expglVertex4iv (const GLint *v)
{
    ldrpglVertex4iv (v);
}

void GLAPI expglVertex4s (GLshort x, GLshort y, GLshort z, GLshort w)
{
    ldrpglVertex4s (x, y, z, w);
}

void GLAPI expglVertex4sv (const GLshort *v)
{
    ldrpglVertex4sv (v);
}

void GLAPI expglViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
    ldrpglViewport (x, y, width, height);
}

void GLAPI expglPointParameterfEXT( GLenum param, GLfloat value)
{
    ldrpglPointParameterfEXT (param, value);
}

void GLAPI expglPointParameterfvEXT( GLenum param, const GLfloat *value)
{
    ldrpglPointParameterfvEXT (param, value);
}

void GLAPI expglLockArraysEXT (int a, int b)
{
    ldrpglLockArraysEXT (a, b);
}

void GLAPI expglUnlockArraysEXT (void)
{
    ldrpglUnlockArraysEXT ();
}

void GLAPI expglActiveTextureARB( GLenum a)
{
    ldrpglActiveTextureARB (a);
}

void GLAPI expglClientActiveTextureARB( GLenum a)
{
    ldrpglClientActiveTextureARB (a);
}

void GLAPI expglGetCompressedTexImage( GLenum target, GLint lod, const void* data)
{
    ldrpglGetCompressedTexImage (target, lod, data);
}

void GLAPI expglDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices)
{
    ldrpglDrawRangeElements (mode, start, end, count, type, indices);
}

void GLAPI expglDrawRangeElementsEXT( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices)
{
    ldrpglDrawRangeElementsEXT (mode, start, end, count, type, indices);
}

void GLAPI expglDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
    ldrpglDrawElements (mode, count, type, indices);
}

void GLAPI expglVertexPointer(GLint size, GLenum type, GLsizei stride, const void *ptr)
{
    ldrpglVertexPointer (size, type, stride, ptr);
}

void GLAPI expglNormalPointer(GLenum type, GLsizei stride, const void *ptr)
{
    ldrpglNormalPointer (type, stride, ptr);
}

void GLAPI expglColorPointer(GLint size, GLenum type, GLsizei stride, const void *ptr)
{
    ldrpglColorPointer (size, type, stride, ptr);
}

void GLAPI expglTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *ptr)
{
    ldrpglTexCoordPointer (size, type, stride, ptr);
}

void GLAPI expglArrayElement(GLint i)
{
    ldrpglArrayElement (i);
}

void GLAPI expglMultiTexCoord1f(GLenum a, GLfloat b)
{
    ldrpglMultiTexCoord1f (a, b);
}

void GLAPI expglMultiTexCoord2f(GLenum a, GLfloat b, GLfloat c)
{
    ldrpglMultiTexCoord2f (a, b, c);
}

void GLAPI expglMultiTexCoord3f(GLenum a, GLfloat b, GLfloat c, GLfloat d)
{
    ldrpglMultiTexCoord3f (a, b, c, d);
}

void GLAPI expglMultiTexCoord4f(GLenum a, GLfloat b, GLfloat c, GLfloat d, GLfloat e)
{
    ldrpglMultiTexCoord4f (a, b, c, d, e);
}

void GLAPI expglActiveTexture(GLenum a)
{
    ldrpglActiveTexture (a);
}

void GLAPI expglClientActiveTexture(GLenum a)
{
    ldrpglClientActiveTexture (a);
}

void GLAPI expglCompressedTexImage3DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data)
{
    ldrpglCompressedTexImage3DARB (target, level, internalformat, width, height, depth, border, imageSize, data);
}

void GLAPI expglCompressedTexImage2DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border,  GLsizei imageSize, const void *data)
{
    ldrpglCompressedTexImage2DARB (target, level, internalformat, width, height, border, imageSize, data);
}

void GLAPI expglCompressedTexImage1DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data)
{
    ldrpglCompressedTexImage1DARB (target, level, internalformat, width, border, imageSize, data);
}

void GLAPI expglCompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data)
{
    ldrpglCompressedTexSubImage3DARB (target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}

void GLAPI expglCompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data)
{
    ldrpglCompressedTexSubImage2DARB (target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

void GLAPI expglCompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data)
{
    ldrpglCompressedTexSubImage1DARB (target, level, xoffset, width, format, imageSize, data);
}

void GLAPI expglDeleteObjectARB(GLhandleARB obj)
{
    ldrpglDeleteObjectARB (obj);
}

GLhandleARB GLAPI expglGetHandleARB(GLenum pname)
{
    return ldrpglGetHandleARB (pname);
}

void GLAPI expglDetachObjectARB(GLhandleARB containerObj, GLhandleARB attachedObj)
{
    ldrpglDetachObjectARB (containerObj, attachedObj);
}

GLhandleARB GLAPI expglCreateShaderObjectARB(GLenum shaderType)
{
    return ldrpglCreateShaderObjectARB (shaderType);
}

void GLAPI expglShaderSourceARB(GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length)
{
    ldrpglShaderSourceARB (shaderObj, count, string, length);
}

void GLAPI expglCompileShaderARB(GLhandleARB shaderObj)
{
    ldrpglCompileShaderARB (shaderObj);
}

GLhandleARB GLAPI expglCreateProgramObjectARB(void)
{
    return ldrpglCreateProgramObjectARB ();
}

void GLAPI expglAttachObjectARB(GLhandleARB containerObj, GLhandleARB obj)
{
    ldrpglAttachObjectARB (containerObj, obj);
}

void GLAPI expglLinkProgramARB(GLhandleARB programObj)
{
    ldrpglLinkProgramARB (programObj);
}

void GLAPI expglUseProgramObjectARB(GLhandleARB programObj)
{
    ldrpglUseProgramObjectARB (programObj);
}

void GLAPI expglValidateProgramARB(GLhandleARB programObj)
{
    ldrpglValidateProgramARB (programObj);
}

void GLAPI expglBindProgramARB(GLenum target, GLuint program)
{
    ldrpglBindProgramARB (target, program);
}

void GLAPI expglDeleteProgramsARB(GLsizei n, const GLuint *programs)
{
    ldrpglDeleteProgramsARB (n, programs);
}

void GLAPI expglGenProgramsARB(GLsizei n, GLuint *programs)
{
    ldrpglGenProgramsARB (n, programs);
}

void GLAPI expglProgramStringARB(GLenum target, GLenum format, GLsizei len, const void *string)
{
    ldrpglProgramStringARB (target, format, len, string);
}

void GLAPI expglProgramEnvParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    ldrpglProgramEnvParameter4fARB (target, index, x, y, z, w);
}

void GLAPI expglProgramLocalParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    ldrpglProgramLocalParameter4fARB (target, index, x, y, z, w);
}

void GLAPI expglGetProgramivARB( GLenum target, GLenum pname, GLint *params)
{
    ldrpglGetProgramivARB (target, pname, params);
}

void GLAPI expglUniform1fARB(GLint location, GLfloat v0)
{
    ldrpglUniform1fARB (location, v0);
}

void GLAPI expglUniform2fARB(GLint location, GLfloat v0, GLfloat v1)
{
    ldrpglUniform2fARB (location, v0, v1);
}

void GLAPI expglUniform3fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
    ldrpglUniform3fARB (location, v0, v1, v2);
}

void GLAPI expglUniform4fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    ldrpglUniform4fARB (location, v0, v1, v2, v3);
}

void GLAPI expglUniform1iARB(GLint location, GLint v0)
{
    ldrpglUniform1iARB (location, v0);
}

void GLAPI expglUniform2iARB(GLint location, GLint v0, GLint v1)
{
    ldrpglUniform2iARB (location, v0, v1);
}

void GLAPI expglUniform3iARB(GLint location, GLint v0, GLint v1, GLint v2)
{
    ldrpglUniform3iARB (location, v0, v1, v2);
}

void GLAPI expglUniform4iARB(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
    ldrpglUniform4iARB (location, v0, v1, v2, v3);
}

void GLAPI expglUniform1fvARB(GLint location, GLsizei count, const GLfloat *value)
{
    ldrpglUniform1fvARB (location, count, value);
}

void GLAPI expglUniform2fvARB(GLint location, GLsizei count, const GLfloat *value)
{
    ldrpglUniform2fvARB (location, count, value);
}

void GLAPI expglUniform3fvARB(GLint location, GLsizei count, const GLfloat *value)
{
    ldrpglUniform3fvARB (location, count, value);
}

void GLAPI expglUniform4fvARB(GLint location, GLsizei count, const GLfloat *value)
{
    ldrpglUniform4fvARB (location, count, value);
}

void GLAPI expglUniform1ivARB(GLint location, GLsizei count, const GLint *value)
{
    ldrpglUniform1ivARB (location, count, value);
}

void GLAPI expglUniform2ivARB(GLint location, GLsizei count, const GLint *value)
{
    ldrpglUniform2ivARB (location, count, value);
}

void GLAPI expglUniform3ivARB(GLint location, GLsizei count, const GLint *value)
{
    ldrpglUniform3ivARB (location, count, value);
}

void GLAPI expglUniform4ivARB(GLint location, GLsizei count, const GLint *value)
{
    ldrpglUniform4ivARB (location, count, value);
}

void GLAPI expglUniformMatrix2fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    ldrpglUniformMatrix2fvARB (location, count, transpose, value);
}

void GLAPI expglUniformMatrix3fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    ldrpglUniformMatrix3fvARB (location, count, transpose, value);
}

void GLAPI expglUniformMatrix4fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    ldrpglUniformMatrix4fvARB (location, count, transpose, value);
}

void GLAPI expglGetObjectParameterfvARB(GLhandleARB obj, GLenum pname, GLfloat *params)
{
    ldrpglGetObjectParameterfvARB (obj, pname, params);
}

void GLAPI expglGetObjectParameterivARB(GLhandleARB obj, GLenum pname, GLint *params)
{
    ldrpglGetObjectParameterivARB (obj, pname, params);
}

void GLAPI expglGetInfoLogARB(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog)
{
    ldrpglGetInfoLogARB (obj, maxLength, length, infoLog);
}

void GLAPI expglGetAttachedObjectsARB(GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj)
{
    ldrpglGetAttachedObjectsARB (containerObj, maxCount, count, obj);
}

GLint GLAPI expglGetUniformLocationARB(GLhandleARB programObj, const GLcharARB *name)
{
    return ldrpglGetUniformLocationARB (programObj, name);
}

void GLAPI expglGetActiveUniformARB(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name)
{
    ldrpglGetActiveUniformARB (programObj, index, maxLength, length, size, type, name);
}

void GLAPI expglGetUniformfvARB(GLhandleARB programObj, GLint location, GLfloat *params)
{
    ldrpglGetUniformfvARB (programObj, location, params);
}

void GLAPI expglGetUniformivARB(GLhandleARB programObj, GLint location, GLint *params)
{
    ldrpglGetUniformivARB (programObj, location, params);
}

void GLAPI expglGetShaderSourceARB(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source)
{
    ldrpglGetShaderSourceARB (obj, maxLength, length, source);
}

void GLAPI expglTexImage3D( GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels)
{
    ldrpglTexImage3D (target, level, internalFormat, width, height, depth, border, format, type, pixels);
}

void GLAPI expglTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels)
{
    ldrpglTexSubImage3D (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

void GLAPI expglCopyTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    ldrpglCopyTexSubImage3D (target, level, xoffset, yoffset, zoffset, x, y, width, height);
}

void GLAPI expglBlendEquationEXT(GLenum a)
{
    ldrpglBlendEquationEXT (a);
}

void GLAPI expglStencilOpSeparate(GLenum a, GLenum b, GLenum c, GLenum d)
{
    ldrpglStencilOpSeparate (a, b, c, d);
}

void GLAPI expglStencilFuncSeparate(GLenum a, GLenum b, GLint c, GLuint d)
{
    ldrpglStencilFuncSeparate (a, b, c, d);
}

void GLAPI expglActiveStencilFaceEXT(GLenum a)
{
    ldrpglActiveStencilFaceEXT (a);
}

void GLAPI expglVertexAttribPointerARB(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer)
{
    ldrpglVertexAttribPointerARB (index, size, type, normalized, stride, pointer);
}

void GLAPI expglEnableVertexAttribArrayARB(GLuint index)
{
    ldrpglEnableVertexAttribArrayARB (index);
}

void GLAPI expglDisableVertexAttribArrayARB(GLuint index)
{
    ldrpglDisableVertexAttribArrayARB (index);
}

void GLAPI expglBindAttribLocationARB(GLhandleARB programObj, GLuint index, const GLcharARB *name)
{
    ldrpglBindAttribLocationARB (programObj, index, name);
}

void GLAPI expglGetActiveAttribARB(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name)
{
    ldrpglGetActiveAttribARB (programObj, index, maxLength, length, size, type, name);
}

GLint GLAPI expglGetAttribLocationARB(GLhandleARB programObj, const GLcharARB *name)
{
    return ldrpglGetAttribLocationARB (programObj, name);
}

void GLAPI expglBindBufferARB(GLenum target, GLuint buffer)
{
    ldrpglBindBufferARB (target, buffer);
}

void GLAPI expglDeleteBuffersARB(GLsizei n, const GLuint *buffers)
{
    ldrpglDeleteBuffersARB (n, buffers);
}

void GLAPI expglGenBuffersARB(GLsizei n, GLuint *buffers)
{
    ldrpglGenBuffersARB (n, buffers);
}

GLboolean GLAPI expglIsBufferARB(GLuint buffer)
{
    return ldrpglIsBufferARB (buffer);
}

void* GLAPI expglMapBufferARB(GLenum target, GLenum access)
{
    return ldrpglMapBufferARB (target, access);
}

GLboolean GLAPI expglUnmapBufferARB(GLenum target)
{
    return ldrpglUnmapBufferARB (target);
}

void GLAPI expglBufferDataARB(GLenum target, GLsizeiptrARB size, const void *data, GLenum usage)
{
    ldrpglBufferDataARB (target, size, data, usage);
}

void GLAPI expglBufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void *data)
{
    ldrpglBufferSubDataARB (target, offset, size, data);
}

void GLAPI expglGenQueriesARB(GLsizei n, GLuint *ids)
{
    ldrpglGenQueriesARB (n, ids);
}

void GLAPI expglDeleteQueriesARB(GLsizei n, const GLuint *ids)
{
    ldrpglDeleteQueriesARB (n, ids);
}

GLboolean GLAPI expglIsQueryARB(GLuint id)
{
    return ldrpglIsQueryARB (id);
}

void GLAPI expglBeginQueryARB(GLenum target, GLuint id)
{
    ldrpglBeginQueryARB (target, id);
}

void GLAPI expglEndQueryARB(GLenum target)
{
    ldrpglEndQueryARB (target);
}

void GLAPI expglGetQueryivARB(GLenum target, GLenum pname, GLint *params)
{
    ldrpglGetQueryivARB (target, pname, params);
}

void GLAPI expglGetQueryObjectivARB(GLuint id, GLenum pname, GLint *params)
{
    ldrpglGetQueryObjectivARB (id, pname, params);
}

void GLAPI expglGetQueryObjectuivARB(GLuint id, GLenum pname, GLuint *params)
{
    ldrpglGetQueryObjectuivARB (id, pname, params);
}

void GLAPI  expglSelectTextureSGIS ( GLenum a)
{
    ldrpglSelectTextureSGIS (a);
}

void GLAPI  expglMTexCoord2fSGIS ( GLenum a, GLfloat b, GLfloat c)
{
    ldrpglMTexCoord2fSGIS (a, b, c);
}

void GLAPI  expglSwapInterval ( int interval)
{
    ldrpglSwapInterval (interval);
}

GLboolean GLAPI expglIsRenderbuffer (GLuint renderbuffer)
{
    return ldrpglIsRenderbuffer (renderbuffer);
}

void GLAPI expglBindRenderbuffer (GLenum target, GLuint renderbuffer)
{
    ldrpglBindRenderbuffer (target, renderbuffer);
}

void GLAPI expglDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers)
{
    ldrpglDeleteRenderbuffers (n, renderbuffers);
}

void GLAPI expglGenRenderbuffers (GLsizei n, GLuint *renderbuffers)
{
    ldrpglGenRenderbuffers (n, renderbuffers);
}

void GLAPI expglRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    ldrpglRenderbufferStorage (target, internalformat, width, height);
}

void GLAPI expglRenderbufferStorageMultisample (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    ldrpglRenderbufferStorageMultisample (target, samples, internalformat, width, height);
}

void GLAPI expglGetRenderbufferParameteriv (GLenum target, GLenum pname, GLint *params)
{
    ldrpglGetRenderbufferParameteriv (target, pname, params);
}

GLboolean expglIsFramebuffer (GLuint framebuffer)
{
    return ldrpglIsFramebuffer (framebuffer);
}

void GLAPI expglBindFramebuffer (GLenum target, GLuint framebuffer)
{
    ldrpglBindFramebuffer (target, framebuffer);
}

void GLAPI expglDeleteFramebuffers (GLsizei n, const GLuint *framebuffers)
{
    ldrpglDeleteFramebuffers (n, framebuffers);
}

void GLAPI expglGenFramebuffers (GLsizei n, GLuint *framebuffers)
{
    ldrpglGenFramebuffers (n, framebuffers);
}

GLenum GLAPI expglCheckFramebufferStatus (GLenum target)
{
    return ldrpglCheckFramebufferStatus (target);
}

void GLAPI expglFramebufferTexture1D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    ldrpglFramebufferTexture1D (target, attachment, textarget, texture, level);
}

void GLAPI expglFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    ldrpglFramebufferTexture2D (target, attachment, textarget, texture, level);
}

void GLAPI expglFramebufferTexture3D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer)
{
    ldrpglFramebufferTexture3D (target, attachment, textarget, texture, level, layer);
}

void GLAPI expglFramebufferTextureLayer (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    ldrpglFramebufferTextureLayer (target, attachment, texture, level, layer);
}

void GLAPI expglFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    ldrpglFramebufferRenderbuffer (target, attachment, renderbuffertarget, renderbuffer);
}

void GLAPI expglGetFramebufferAttachmentParameteriv (GLenum target, GLenum attachment, GLenum pname, GLint *params)
{
    ldrpglGetFramebufferAttachmentParameteriv (target, attachment, pname, params);
}

void GLAPI expglBlitFramebuffer (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    ldrpglBlitFramebuffer (srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void GLAPI expglGenerateMipmap (GLenum target)
{
    ldrpglGenerateMipmap (target);
}

void GLAPI expglGetCompressedTexImageARB (GLenum target, GLint level, GLvoid *img)
{
    ldrpglGetCompressedTexImageARB (target, level, img);
}

void GLAPI expglCopyTexSubImage3DEXT (GLenum a, GLint b, GLint c, GLint d, GLint e, GLint f, GLint g, GLsizei h, GLsizei i)
{
    ldrpglCopyTexSubImage3DEXT (a, b, c, d, e, f, g, h, i);
}

void GLAPI expglTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
    ldrpglTexSubImage3DEXT (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

void GLAPI expglTexImage3DEXT(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    ldrpglTexImage3DEXT (target, level, internalformat, width, height, depth, border, format, type, pixels);
}

void GLAPI expglMultiTexCoord1fARB(GLenum target, GLfloat s)
{
    ldrpglMultiTexCoord1fARB (target, s);
}

void GLAPI expglMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t)
{
    ldrpglMultiTexCoord2fARB (target, s, t);
}

void GLAPI expglMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
    ldrpglMultiTexCoord3fARB (target, s, t, r);
}

void GLAPI expglMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    ldrpglMultiTexCoord4fARB (target, s, t, r, q);
}
void GLAPI expglBindVertexArray(GLuint array)
{
    ldrpglBindVertexArray(array);
}
void GLAPI expglDeleteVertexArrays(GLsizei n, const GLuint *arrays)
{
    ldrpglDeleteVertexArrays(n, arrays);
}
void GLAPI expglGenVertexArrays(GLsizei n, GLuint *arrays)
{
    ldrpglGenVertexArrays(n, arrays);
}
GLboolean GLAPI expglIsVertexArray(GLuint array)
{
    return ldrpglIsVertexArray(array);
}
void GLAPI expglDebugMessageControlARB(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled)
{
    ldrpglDebugMessageControlARB(source, type, severity, count, ids, enabled);
}
void GLAPI expglDebugMessageInsertARB(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* buf)
{
    ldrpglDebugMessageInsertARB(source, type, id, severity, length, buf);
}
void GLAPI expglDebugMessageCallbackARB(GLvoid *callback, GLvoid *userParam)
{
    ldrpglDebugMessageCallbackARB(callback, userParam);
}
GLuint GLAPI expglGetDebugMessageLogARB(GLuint count, GLsizei bufsize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog)
{
    return ldrpglGetDebugMessageLogARB(count, bufsize, sources, types, ids, severities, lengths, messageLog);
}
void GLAPI expglVertexAttrib2f(GLuint index, GLfloat v0, GLfloat v1)
{
    ldrpglVertexAttrib2f(index, v0, v1);
}
void GLAPI expglVertexAttrib2fv(GLuint index, const GLfloat *v)
{
    ldrpglVertexAttrib2fv(index, v);
}
void GLAPI expglVertexAttrib3fv(GLuint index, const GLfloat *v)
{
    ldrpglVertexAttrib3fv(index, v);
}
