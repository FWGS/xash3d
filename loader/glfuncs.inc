GLenum STDCALL expglGetError (void)
{
    return ldrpglGetError ();
}

const GLcharARB * STDCALL expglGetString (GLenum name)
{
    return ldrpglGetString (name);
}

void STDCALL expglAccum (GLenum op, GLfloat value)
{
    ldrpglAccum (op, value);
}

void STDCALL expglAlphaFunc (GLenum func, GLclampf ref)
{
    ldrpglAlphaFunc (func, ref);
}

void STDCALL expglBegin (GLenum mode)
{
    ldrpglBegin (mode);
}

void STDCALL expglBindTexture (GLenum target, GLuint texture)
{
    ldrpglBindTexture (target, texture);
}

void STDCALL expglBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
    ldrpglBitmap (width, height, xorig, yorig, xmove, ymove, bitmap);
}

void STDCALL expglBlendFunc (GLenum sfactor, GLenum dfactor)
{
    ldrpglBlendFunc (sfactor, dfactor);
}

void STDCALL expglCallList (GLuint list)
{
    ldrpglCallList (list);
}

void STDCALL expglCallLists (GLsizei n, GLenum type, const GLvoid *lists)
{
    ldrpglCallLists (n, type, lists);
}

void STDCALL expglClear (GLbitfield mask)
{
    ldrpglClear (mask);
}

void STDCALL expglClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    ldrpglClearAccum (red, green, blue, alpha);
}

void STDCALL expglClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    ldrpglClearColor (red, green, blue, alpha);
}

void STDCALL expglClearDepth (GLclampd depth)
{
    ldrpglClearDepth (depth);
}

void STDCALL expglClearIndex (GLfloat c)
{
    ldrpglClearIndex (c);
}

void STDCALL expglClearStencil (GLint s)
{
    ldrpglClearStencil (s);
}

GLboolean STDCALL expglIsEnabled ( GLenum cap)
{
    return ldrpglIsEnabled (cap);
}

GLboolean STDCALL expglIsList ( GLuint list)
{
    return ldrpglIsList (list);
}

GLboolean STDCALL expglIsTexture ( GLuint texture)
{
    return ldrpglIsTexture (texture);
}

void STDCALL expglClipPlane (GLenum plane, const GLdouble *equation)
{
    ldrpglClipPlane (plane, equation);
}

void STDCALL expglColor3b (GLbyte red, GLbyte green, GLbyte blue)
{
    ldrpglColor3b (red, green, blue);
}

void STDCALL expglColor3bv (const GLbyte *v)
{
    ldrpglColor3bv (v);
}

void STDCALL expglColor3d (GLdouble red, GLdouble green, GLdouble blue)
{
    ldrpglColor3d (red, green, blue);
}

void STDCALL expglColor3dv (const GLdouble *v)
{
    ldrpglColor3dv (v);
}

void STDCALL expglColor3f (GLfloat red, GLfloat green, GLfloat blue)
{
    ldrpglColor3f (red, green, blue);
}

void STDCALL expglColor3fv (const GLfloat *v)
{
    ldrpglColor3fv (v);
}

void STDCALL expglColor3i (GLint red, GLint green, GLint blue)
{
    ldrpglColor3i (red, green, blue);
}

void STDCALL expglColor3iv (const GLint *v)
{
    ldrpglColor3iv (v);
}

void STDCALL expglColor3s (GLshort red, GLshort green, GLshort blue)
{
    ldrpglColor3s (red, green, blue);
}

void STDCALL expglColor3sv (const GLshort *v)
{
    ldrpglColor3sv (v);
}

void STDCALL expglColor3ub (GLubyte red, GLubyte green, GLubyte blue)
{
    ldrpglColor3ub (red, green, blue);
}

void STDCALL expglColor3ubv (const GLubyte *v)
{
    ldrpglColor3ubv (v);
}

void STDCALL expglColor3ui (GLuint red, GLuint green, GLuint blue)
{
    ldrpglColor3ui (red, green, blue);
}

void STDCALL expglColor3uiv (const GLuint *v)
{
    ldrpglColor3uiv (v);
}

void STDCALL expglColor3us (GLushort red, GLushort green, GLushort blue)
{
    ldrpglColor3us (red, green, blue);
}

void STDCALL expglColor3usv (const GLushort *v)
{
    ldrpglColor3usv (v);
}

void STDCALL expglColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
    ldrpglColor4b (red, green, blue, alpha);
}

void STDCALL expglColor4bv (const GLbyte *v)
{
    ldrpglColor4bv (v);
}

void STDCALL expglColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    ldrpglColor4d (red, green, blue, alpha);
}

void STDCALL expglColor4dv (const GLdouble *v)
{
    ldrpglColor4dv (v);
}

void STDCALL expglColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    ldrpglColor4f (red, green, blue, alpha);
}

void STDCALL expglColor4fv (const GLfloat *v)
{
    ldrpglColor4fv (v);
}

void STDCALL expglColor4i (GLint red, GLint green, GLint blue, GLint alpha)
{
    ldrpglColor4i (red, green, blue, alpha);
}

void STDCALL expglColor4iv (const GLint *v)
{
    ldrpglColor4iv (v);
}

void STDCALL expglColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
    ldrpglColor4s (red, green, blue, alpha);
}

void STDCALL expglColor4sv (const GLshort *v)
{
    ldrpglColor4sv (v);
}

void STDCALL expglColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    ldrpglColor4ub (red, green, blue, alpha);
}

void STDCALL expglColor4ubv (const GLubyte *v)
{
    ldrpglColor4ubv (v);
}

void STDCALL expglColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    ldrpglColor4ui (red, green, blue, alpha);
}

void STDCALL expglColor4uiv (const GLuint *v)
{
    ldrpglColor4uiv (v);
}

void STDCALL expglColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
    ldrpglColor4us (red, green, blue, alpha);
}

void STDCALL expglColor4usv (const GLushort *v)
{
    ldrpglColor4usv (v);
}

void STDCALL expglColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    ldrpglColorMask (red, green, blue, alpha);
}

void STDCALL expglColorMaterial (GLenum face, GLenum mode)
{
    ldrpglColorMaterial (face, mode);
}

void STDCALL expglCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    ldrpglCopyPixels (x, y, width, height, type);
}

void STDCALL expglCopyTexImage1D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border)
{
    ldrpglCopyTexImage1D (target, level, internalFormat, x, y, width, border);
}

void STDCALL expglCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    ldrpglCopyTexImage2D (target, level, internalFormat, x, y, width, height, border);
}

void STDCALL expglCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    ldrpglCopyTexSubImage1D (target, level, xoffset, x, y, width);
}

void STDCALL expglCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    ldrpglCopyTexSubImage2D (target, level, xoffset, yoffset, x, y, width, height);
}

void STDCALL expglCullFace (GLenum mode)
{
    ldrpglCullFace (mode);
}

void STDCALL expglDeleteLists (GLuint list, GLsizei range)
{
    ldrpglDeleteLists (list, range);
}

void STDCALL expglDeleteTextures (GLsizei n, const GLuint *textures)
{
    ldrpglDeleteTextures (n, textures);
}

void STDCALL expglDepthFunc (GLenum func)
{
    ldrpglDepthFunc (func);
}

void STDCALL expglDepthMask (GLboolean flag)
{
    ldrpglDepthMask (flag);
}

void STDCALL expglDepthRange (GLclampd zNear, GLclampd zFar)
{
    ldrpglDepthRange (zNear, zFar);
}

void STDCALL expglDisable (GLenum cap)
{
    ldrpglDisable (cap);
}

void STDCALL expglDisableClientState (GLenum array)
{
    ldrpglDisableClientState (array);
}

void STDCALL expglDrawArrays (GLenum mode, GLint first, GLsizei count)
{
    ldrpglDrawArrays (mode, first, count);
}

void STDCALL expglDrawBuffer (GLenum mode)
{
    ldrpglDrawBuffer (mode);
}

void STDCALL expglDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
    ldrpglDrawPixels (width, height, format, type, pixels);
}

void STDCALL expglEdgeFlag (GLboolean flag)
{
    ldrpglEdgeFlag (flag);
}

void STDCALL expglEdgeFlagPointer (GLsizei stride, const GLvoid *pointer)
{
    ldrpglEdgeFlagPointer (stride, pointer);
}

void STDCALL expglEdgeFlagv (const GLboolean *flag)
{
    ldrpglEdgeFlagv (flag);
}

void STDCALL expglEnable (GLenum cap)
{
    ldrpglEnable (cap);
}

void STDCALL expglEnableClientState (GLenum array)
{
    ldrpglEnableClientState (array);
}

void STDCALL expglEnd (void)
{
    ldrpglEnd ();
}

void STDCALL expglEndList (void)
{
    ldrpglEndList ();
}

void STDCALL expglEvalCoord1d (GLdouble u)
{
    ldrpglEvalCoord1d (u);
}

void STDCALL expglEvalCoord1dv (const GLdouble *u)
{
    ldrpglEvalCoord1dv (u);
}

void STDCALL expglEvalCoord1f (GLfloat u)
{
    ldrpglEvalCoord1f (u);
}

void STDCALL expglEvalCoord1fv (const GLfloat *u)
{
    ldrpglEvalCoord1fv (u);
}

void STDCALL expglEvalCoord2d (GLdouble u, GLdouble v)
{
    ldrpglEvalCoord2d (u, v);
}

void STDCALL expglEvalCoord2dv (const GLdouble *u)
{
    ldrpglEvalCoord2dv (u);
}

void STDCALL expglEvalCoord2f (GLfloat u, GLfloat v)
{
    ldrpglEvalCoord2f (u, v);
}

void STDCALL expglEvalCoord2fv (const GLfloat *u)
{
    ldrpglEvalCoord2fv (u);
}

void STDCALL expglEvalMesh1 (GLenum mode, GLint i1, GLint i2)
{
    ldrpglEvalMesh1 (mode, i1, i2);
}

void STDCALL expglEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    ldrpglEvalMesh2 (mode, i1, i2, j1, j2);
}

void STDCALL expglEvalPoint1 (GLint i)
{
    ldrpglEvalPoint1 (i);
}

void STDCALL expglEvalPoint2 (GLint i, GLint j)
{
    ldrpglEvalPoint2 (i, j);
}

void STDCALL expglFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer)
{
    ldrpglFeedbackBuffer (size, type, buffer);
}

void STDCALL expglFinish (void)
{
    ldrpglFinish ();
}

void STDCALL expglFlush (void)
{
    ldrpglFlush ();
}

void STDCALL expglFogf (GLenum pname, GLfloat param)
{
    ldrpglFogf (pname, param);
}

void STDCALL expglFogfv (GLenum pname, const GLfloat *params)
{
    ldrpglFogfv (pname, params);
}

void STDCALL expglFogi (GLenum pname, GLint param)
{
    ldrpglFogi (pname, param);
}

void STDCALL expglFogiv (GLenum pname, const GLint *params)
{
    ldrpglFogiv (pname, params);
}

void STDCALL expglFrontFace (GLenum mode)
{
    ldrpglFrontFace (mode);
}

void STDCALL expglFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    ldrpglFrustum (left, right, bottom, top, zNear, zFar);
}

void STDCALL expglGenTextures (GLsizei n, GLuint *textures)
{
    ldrpglGenTextures (n, textures);
}

void STDCALL expglGetBooleanv (GLenum pname, GLboolean *params)
{
    ldrpglGetBooleanv (pname, params);
}

void STDCALL expglGetClipPlane (GLenum plane, GLdouble *equation)
{
    ldrpglGetClipPlane (plane, equation);
}

void STDCALL expglGetDoublev (GLenum pname, GLdouble *params)
{
    ldrpglGetDoublev (pname, params);
}

void STDCALL expglGetFloatv (GLenum pname, GLfloat *params)
{
    ldrpglGetFloatv (pname, params);
}

void STDCALL expglGetIntegerv (GLenum pname, GLint *params)
{
    ldrpglGetIntegerv (pname, params);
}

void STDCALL expglGetLightfv (GLenum light, GLenum pname, GLfloat *params)
{
    ldrpglGetLightfv (light, pname, params);
}

void STDCALL expglGetLightiv (GLenum light, GLenum pname, GLint *params)
{
    ldrpglGetLightiv (light, pname, params);
}

void STDCALL expglGetMapdv (GLenum target, GLenum query, GLdouble *v)
{
    ldrpglGetMapdv (target, query, v);
}

void STDCALL expglGetMapfv (GLenum target, GLenum query, GLfloat *v)
{
    ldrpglGetMapfv (target, query, v);
}

void STDCALL expglGetMapiv (GLenum target, GLenum query, GLint *v)
{
    ldrpglGetMapiv (target, query, v);
}

void STDCALL expglGetMaterialfv (GLenum face, GLenum pname, GLfloat *params)
{
    ldrpglGetMaterialfv (face, pname, params);
}

void STDCALL expglGetMaterialiv (GLenum face, GLenum pname, GLint *params)
{
    ldrpglGetMaterialiv (face, pname, params);
}

void STDCALL expglGetPixelMapfv (GLenum map, GLfloat *values)
{
    ldrpglGetPixelMapfv (map, values);
}

void STDCALL expglGetPixelMapuiv (GLenum map, GLuint *values)
{
    ldrpglGetPixelMapuiv (map, values);
}

void STDCALL expglGetPixelMapusv (GLenum map, GLushort *values)
{
    ldrpglGetPixelMapusv (map, values);
}

void STDCALL expglGetPointerv (GLenum pname, GLvoid* *params)
{
    ldrpglGetPointerv (pname, params);
}

void STDCALL expglGetPolygonStipple (GLubyte *mask)
{
    ldrpglGetPolygonStipple (mask);
}

void STDCALL expglGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params)
{
    ldrpglGetTexEnvfv (target, pname, params);
}

void STDCALL expglGetTexEnviv (GLenum target, GLenum pname, GLint *params)
{
    ldrpglGetTexEnviv (target, pname, params);
}

void STDCALL expglGetTexGendv (GLenum coord, GLenum pname, GLdouble *params)
{
    ldrpglGetTexGendv (coord, pname, params);
}

void STDCALL expglGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params)
{
    ldrpglGetTexGenfv (coord, pname, params);
}

void STDCALL expglGetTexGeniv (GLenum coord, GLenum pname, GLint *params)
{
    ldrpglGetTexGeniv (coord, pname, params);
}

void STDCALL expglGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
    ldrpglGetTexImage (target, level, format, type, pixels);
}

void STDCALL expglGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params)
{
    ldrpglGetTexLevelParameterfv (target, level, pname, params);
}

void STDCALL expglGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params)
{
    ldrpglGetTexLevelParameteriv (target, level, pname, params);
}

void STDCALL expglGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params)
{
    ldrpglGetTexParameterfv (target, pname, params);
}

void STDCALL expglGetTexParameteriv (GLenum target, GLenum pname, GLint *params)
{
    ldrpglGetTexParameteriv (target, pname, params);
}

void STDCALL expglHint (GLenum target, GLenum mode)
{
    ldrpglHint (target, mode);
}

void STDCALL expglIndexMask (GLuint mask)
{
    ldrpglIndexMask (mask);
}

void STDCALL expglIndexPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{
    ldrpglIndexPointer (type, stride, pointer);
}

void STDCALL expglIndexd (GLdouble c)
{
    ldrpglIndexd (c);
}

void STDCALL expglIndexdv (const GLdouble *c)
{
    ldrpglIndexdv (c);
}

void STDCALL expglIndexf (GLfloat c)
{
    ldrpglIndexf (c);
}

void STDCALL expglIndexfv (const GLfloat *c)
{
    ldrpglIndexfv (c);
}

void STDCALL expglIndexi (GLint c)
{
    ldrpglIndexi (c);
}

void STDCALL expglIndexiv (const GLint *c)
{
    ldrpglIndexiv (c);
}

void STDCALL expglIndexs (GLshort c)
{
    ldrpglIndexs (c);
}

void STDCALL expglIndexsv (const GLshort *c)
{
    ldrpglIndexsv (c);
}

void STDCALL expglIndexub (GLubyte c)
{
    ldrpglIndexub (c);
}

void STDCALL expglIndexubv (const GLubyte *c)
{
    ldrpglIndexubv (c);
}

void STDCALL expglInitNames (void)
{
    ldrpglInitNames ();
}

void STDCALL expglInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer)
{
    ldrpglInterleavedArrays (format, stride, pointer);
}

void STDCALL expglLightModelf (GLenum pname, GLfloat param)
{
    ldrpglLightModelf (pname, param);
}

void STDCALL expglLightModelfv (GLenum pname, const GLfloat *params)
{
    ldrpglLightModelfv (pname, params);
}

void STDCALL expglLightModeli (GLenum pname, GLint param)
{
    ldrpglLightModeli (pname, param);
}

void STDCALL expglLightModeliv (GLenum pname, const GLint *params)
{
    ldrpglLightModeliv (pname, params);
}

void STDCALL expglLightf (GLenum light, GLenum pname, GLfloat param)
{
    ldrpglLightf (light, pname, param);
}

void STDCALL expglLightfv (GLenum light, GLenum pname, const GLfloat *params)
{
    ldrpglLightfv (light, pname, params);
}

void STDCALL expglLighti (GLenum light, GLenum pname, GLint param)
{
    ldrpglLighti (light, pname, param);
}

void STDCALL expglLightiv (GLenum light, GLenum pname, const GLint *params)
{
    ldrpglLightiv (light, pname, params);
}

void STDCALL expglLineStipple (GLint factor, GLushort pattern)
{
    ldrpglLineStipple (factor, pattern);
}

void STDCALL expglLineWidth (GLfloat width)
{
    ldrpglLineWidth (width);
}

void STDCALL expglListBase (GLuint base)
{
    ldrpglListBase (base);
}

void STDCALL expglLoadIdentity (void)
{
    ldrpglLoadIdentity ();
}

void STDCALL expglLoadMatrixd (const GLdouble *m)
{
    ldrpglLoadMatrixd (m);
}

void STDCALL expglLoadMatrixf (const GLfloat *m)
{
    ldrpglLoadMatrixf (m);
}

void STDCALL expglLoadName (GLuint name)
{
    ldrpglLoadName (name);
}

void STDCALL expglLogicOp (GLenum opcode)
{
    ldrpglLogicOp (opcode);
}

void STDCALL expglMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
    ldrpglMap1d (target, u1, u2, stride, order, points);
}

void STDCALL expglMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
    ldrpglMap1f (target, u1, u2, stride, order, points);
}

void STDCALL expglMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
    ldrpglMap2d (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void STDCALL expglMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
    ldrpglMap2f (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void STDCALL expglMapGrid1d (GLint un, GLdouble u1, GLdouble u2)
{
    ldrpglMapGrid1d (un, u1, u2);
}

void STDCALL expglMapGrid1f (GLint un, GLfloat u1, GLfloat u2)
{
    ldrpglMapGrid1f (un, u1, u2);
}

void STDCALL expglMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    ldrpglMapGrid2d (un, u1, u2, vn, v1, v2);
}

void STDCALL expglMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    ldrpglMapGrid2f (un, u1, u2, vn, v1, v2);
}

void STDCALL expglMaterialf (GLenum face, GLenum pname, GLfloat param)
{
    ldrpglMaterialf (face, pname, param);
}

void STDCALL expglMaterialfv (GLenum face, GLenum pname, const GLfloat *params)
{
    ldrpglMaterialfv (face, pname, params);
}

void STDCALL expglMateriali (GLenum face, GLenum pname, GLint param)
{
    ldrpglMateriali (face, pname, param);
}

void STDCALL expglMaterialiv (GLenum face, GLenum pname, const GLint *params)
{
    ldrpglMaterialiv (face, pname, params);
}

void STDCALL expglMatrixMode (GLenum mode)
{
    ldrpglMatrixMode (mode);
}

void STDCALL expglMultMatrixd (const GLdouble *m)
{
    ldrpglMultMatrixd (m);
}

void STDCALL expglMultMatrixf (const GLfloat *m)
{
    ldrpglMultMatrixf (m);
}

void STDCALL expglNewList (GLuint list, GLenum mode)
{
    ldrpglNewList (list, mode);
}

void STDCALL expglNormal3b (GLbyte nx, GLbyte ny, GLbyte nz)
{
    ldrpglNormal3b (nx, ny, nz);
}

void STDCALL expglNormal3bv (const GLbyte *v)
{
    ldrpglNormal3bv (v);
}

void STDCALL expglNormal3d (GLdouble nx, GLdouble ny, GLdouble nz)
{
    ldrpglNormal3d (nx, ny, nz);
}

void STDCALL expglNormal3dv (const GLdouble *v)
{
    ldrpglNormal3dv (v);
}

void STDCALL expglNormal3f (GLfloat nx, GLfloat ny, GLfloat nz)
{
    ldrpglNormal3f (nx, ny, nz);
}

void STDCALL expglNormal3fv (const GLfloat *v)
{
    ldrpglNormal3fv (v);
}

void STDCALL expglNormal3i (GLint nx, GLint ny, GLint nz)
{
    ldrpglNormal3i (nx, ny, nz);
}

void STDCALL expglNormal3iv (const GLint *v)
{
    ldrpglNormal3iv (v);
}

void STDCALL expglNormal3s (GLshort nx, GLshort ny, GLshort nz)
{
    ldrpglNormal3s (nx, ny, nz);
}

void STDCALL expglNormal3sv (const GLshort *v)
{
    ldrpglNormal3sv (v);
}

void STDCALL expglOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    ldrpglOrtho (left, right, bottom, top, zNear, zFar);
}

void STDCALL expglPassThrough (GLfloat token)
{
    ldrpglPassThrough (token);
}

void STDCALL expglPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat *values)
{
    ldrpglPixelMapfv (map, mapsize, values);
}

void STDCALL expglPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values)
{
    ldrpglPixelMapuiv (map, mapsize, values);
}

void STDCALL expglPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values)
{
    ldrpglPixelMapusv (map, mapsize, values);
}

void STDCALL expglPixelStoref (GLenum pname, GLfloat param)
{
    ldrpglPixelStoref (pname, param);
}

void STDCALL expglPixelStorei (GLenum pname, GLint param)
{
    ldrpglPixelStorei (pname, param);
}

void STDCALL expglPixelTransferf (GLenum pname, GLfloat param)
{
    ldrpglPixelTransferf (pname, param);
}

void STDCALL expglPixelTransferi (GLenum pname, GLint param)
{
    ldrpglPixelTransferi (pname, param);
}

void STDCALL expglPixelZoom (GLfloat xfactor, GLfloat yfactor)
{
    ldrpglPixelZoom (xfactor, yfactor);
}

void STDCALL expglPointSize (GLfloat size)
{
    ldrpglPointSize (size);
}

void STDCALL expglPolygonMode (GLenum face, GLenum mode)
{
    ldrpglPolygonMode (face, mode);
}

void STDCALL expglPolygonOffset (GLfloat factor, GLfloat units)
{
    ldrpglPolygonOffset (factor, units);
}

void STDCALL expglPolygonStipple (const GLubyte *mask)
{
    ldrpglPolygonStipple (mask);
}

void STDCALL expglPopAttrib (void)
{
    ldrpglPopAttrib ();
}

void STDCALL expglPopClientAttrib (void)
{
    ldrpglPopClientAttrib ();
}

void STDCALL expglPopMatrix (void)
{
    ldrpglPopMatrix ();
}

void STDCALL expglPopName (void)
{
    ldrpglPopName ();
}

void STDCALL expglPushAttrib (GLbitfield mask)
{
    ldrpglPushAttrib (mask);
}

void STDCALL expglPushClientAttrib (GLbitfield mask)
{
    ldrpglPushClientAttrib (mask);
}

void STDCALL expglPushMatrix (void)
{
    ldrpglPushMatrix ();
}

void STDCALL expglPushName (GLuint name)
{
    ldrpglPushName (name);
}

void STDCALL expglRasterPos2d (GLdouble x, GLdouble y)
{
    ldrpglRasterPos2d (x, y);
}

void STDCALL expglRasterPos2dv (const GLdouble *v)
{
    ldrpglRasterPos2dv (v);
}

void STDCALL expglRasterPos2f (GLfloat x, GLfloat y)
{
    ldrpglRasterPos2f (x, y);
}

void STDCALL expglRasterPos2fv (const GLfloat *v)
{
    ldrpglRasterPos2fv (v);
}

void STDCALL expglRasterPos2i (GLint x, GLint y)
{
    ldrpglRasterPos2i (x, y);
}

void STDCALL expglRasterPos2iv (const GLint *v)
{
    ldrpglRasterPos2iv (v);
}

void STDCALL expglRasterPos2s (GLshort x, GLshort y)
{
    ldrpglRasterPos2s (x, y);
}

void STDCALL expglRasterPos2sv (const GLshort *v)
{
    ldrpglRasterPos2sv (v);
}

void STDCALL expglRasterPos3d (GLdouble x, GLdouble y, GLdouble z)
{
    ldrpglRasterPos3d (x, y, z);
}

void STDCALL expglRasterPos3dv (const GLdouble *v)
{
    ldrpglRasterPos3dv (v);
}

void STDCALL expglRasterPos3f (GLfloat x, GLfloat y, GLfloat z)
{
    ldrpglRasterPos3f (x, y, z);
}

void STDCALL expglRasterPos3fv (const GLfloat *v)
{
    ldrpglRasterPos3fv (v);
}

void STDCALL expglRasterPos3i (GLint x, GLint y, GLint z)
{
    ldrpglRasterPos3i (x, y, z);
}

void STDCALL expglRasterPos3iv (const GLint *v)
{
    ldrpglRasterPos3iv (v);
}

void STDCALL expglRasterPos3s (GLshort x, GLshort y, GLshort z)
{
    ldrpglRasterPos3s (x, y, z);
}

void STDCALL expglRasterPos3sv (const GLshort *v)
{
    ldrpglRasterPos3sv (v);
}

void STDCALL expglRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    ldrpglRasterPos4d (x, y, z, w);
}

void STDCALL expglRasterPos4dv (const GLdouble *v)
{
    ldrpglRasterPos4dv (v);
}

void STDCALL expglRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    ldrpglRasterPos4f (x, y, z, w);
}

void STDCALL expglRasterPos4fv (const GLfloat *v)
{
    ldrpglRasterPos4fv (v);
}

void STDCALL expglRasterPos4i (GLint x, GLint y, GLint z, GLint w)
{
    ldrpglRasterPos4i (x, y, z, w);
}

void STDCALL expglRasterPos4iv (const GLint *v)
{
    ldrpglRasterPos4iv (v);
}

void STDCALL expglRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w)
{
    ldrpglRasterPos4s (x, y, z, w);
}

void STDCALL expglRasterPos4sv (const GLshort *v)
{
    ldrpglRasterPos4sv (v);
}

void STDCALL expglReadBuffer (GLenum mode)
{
    ldrpglReadBuffer (mode);
}

void STDCALL expglReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
    ldrpglReadPixels (x, y, width, height, format, type, pixels);
}

void STDCALL expglRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    ldrpglRectd (x1, y1, x2, y2);
}

void STDCALL expglRectdv (const GLdouble *v1, const GLdouble *v2)
{
    ldrpglRectdv (v1, v2);
}

void STDCALL expglRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    ldrpglRectf (x1, y1, x2, y2);
}

void STDCALL expglRectfv (const GLfloat *v1, const GLfloat *v2)
{
    ldrpglRectfv (v1, v2);
}

void STDCALL expglRecti (GLint x1, GLint y1, GLint x2, GLint y2)
{
    ldrpglRecti (x1, y1, x2, y2);
}

void STDCALL expglRectiv (const GLint *v1, const GLint *v2)
{
    ldrpglRectiv (v1, v2);
}

void STDCALL expglRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    ldrpglRects (x1, y1, x2, y2);
}

void STDCALL expglRectsv (const GLshort *v1, const GLshort *v2)
{
    ldrpglRectsv (v1, v2);
}

void STDCALL expglRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    ldrpglRotated (angle, x, y, z);
}

void STDCALL expglRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    ldrpglRotatef (angle, x, y, z);
}

void STDCALL expglScaled (GLdouble x, GLdouble y, GLdouble z)
{
    ldrpglScaled (x, y, z);
}

void STDCALL expglScalef (GLfloat x, GLfloat y, GLfloat z)
{
    ldrpglScalef (x, y, z);
}

void STDCALL expglScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
    ldrpglScissor (x, y, width, height);
}

void STDCALL expglSelectBuffer (GLsizei size, GLuint *buffer)
{
    ldrpglSelectBuffer (size, buffer);
}

void STDCALL expglShadeModel (GLenum mode)
{
    ldrpglShadeModel (mode);
}

void STDCALL expglStencilFunc (GLenum func, GLint ref, GLuint mask)
{
    ldrpglStencilFunc (func, ref, mask);
}

void STDCALL expglStencilMask (GLuint mask)
{
    ldrpglStencilMask (mask);
}

void STDCALL expglStencilOp (GLenum fail, GLenum zfail, GLenum zpass)
{
    ldrpglStencilOp (fail, zfail, zpass);
}

void STDCALL expglTexCoord1d (GLdouble s)
{
    ldrpglTexCoord1d (s);
}

void STDCALL expglTexCoord1dv (const GLdouble *v)
{
    ldrpglTexCoord1dv (v);
}

void STDCALL expglTexCoord1f (GLfloat s)
{
    ldrpglTexCoord1f (s);
}

void STDCALL expglTexCoord1fv (const GLfloat *v)
{
    ldrpglTexCoord1fv (v);
}

void STDCALL expglTexCoord1i (GLint s)
{
    ldrpglTexCoord1i (s);
}

void STDCALL expglTexCoord1iv (const GLint *v)
{
    ldrpglTexCoord1iv (v);
}

void STDCALL expglTexCoord1s (GLshort s)
{
    ldrpglTexCoord1s (s);
}

void STDCALL expglTexCoord1sv (const GLshort *v)
{
    ldrpglTexCoord1sv (v);
}

void STDCALL expglTexCoord2d (GLdouble s, GLdouble t)
{
    ldrpglTexCoord2d (s, t);
}

void STDCALL expglTexCoord2dv (const GLdouble *v)
{
    ldrpglTexCoord2dv (v);
}

void STDCALL expglTexCoord2f (GLfloat s, GLfloat t)
{
    ldrpglTexCoord2f (s, t);
}

void STDCALL expglTexCoord2fv (const GLfloat *v)
{
    ldrpglTexCoord2fv (v);
}

void STDCALL expglTexCoord2i (GLint s, GLint t)
{
    ldrpglTexCoord2i (s, t);
}

void STDCALL expglTexCoord2iv (const GLint *v)
{
    ldrpglTexCoord2iv (v);
}

void STDCALL expglTexCoord2s (GLshort s, GLshort t)
{
    ldrpglTexCoord2s (s, t);
}

void STDCALL expglTexCoord2sv (const GLshort *v)
{
    ldrpglTexCoord2sv (v);
}

void STDCALL expglTexCoord3d (GLdouble s, GLdouble t, GLdouble r)
{
    ldrpglTexCoord3d (s, t, r);
}

void STDCALL expglTexCoord3dv (const GLdouble *v)
{
    ldrpglTexCoord3dv (v);
}

void STDCALL expglTexCoord3f (GLfloat s, GLfloat t, GLfloat r)
{
    ldrpglTexCoord3f (s, t, r);
}

void STDCALL expglTexCoord3fv (const GLfloat *v)
{
    ldrpglTexCoord3fv (v);
}

void STDCALL expglTexCoord3i (GLint s, GLint t, GLint r)
{
    ldrpglTexCoord3i (s, t, r);
}

void STDCALL expglTexCoord3iv (const GLint *v)
{
    ldrpglTexCoord3iv (v);
}

void STDCALL expglTexCoord3s (GLshort s, GLshort t, GLshort r)
{
    ldrpglTexCoord3s (s, t, r);
}

void STDCALL expglTexCoord3sv (const GLshort *v)
{
    ldrpglTexCoord3sv (v);
}

void STDCALL expglTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    ldrpglTexCoord4d (s, t, r, q);
}

void STDCALL expglTexCoord4dv (const GLdouble *v)
{
    ldrpglTexCoord4dv (v);
}

void STDCALL expglTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    ldrpglTexCoord4f (s, t, r, q);
}

void STDCALL expglTexCoord4fv (const GLfloat *v)
{
    ldrpglTexCoord4fv (v);
}

void STDCALL expglTexCoord4i (GLint s, GLint t, GLint r, GLint q)
{
    ldrpglTexCoord4i (s, t, r, q);
}

void STDCALL expglTexCoord4iv (const GLint *v)
{
    ldrpglTexCoord4iv (v);
}

void STDCALL expglTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q)
{
    ldrpglTexCoord4s (s, t, r, q);
}

void STDCALL expglTexCoord4sv (const GLshort *v)
{
    ldrpglTexCoord4sv (v);
}

void STDCALL expglTexEnvf (GLenum target, GLenum pname, GLfloat param)
{
    ldrpglTexEnvf (target, pname, param);
}

void STDCALL expglTexEnvfv (GLenum target, GLenum pname, const GLfloat *params)
{
    ldrpglTexEnvfv (target, pname, params);
}

void STDCALL expglTexEnvi (GLenum target, GLenum pname, GLint param)
{
    ldrpglTexEnvi (target, pname, param);
}

void STDCALL expglTexEnviv (GLenum target, GLenum pname, const GLint *params)
{
    ldrpglTexEnviv (target, pname, params);
}

void STDCALL expglTexGend (GLenum coord, GLenum pname, GLdouble param)
{
    ldrpglTexGend (coord, pname, param);
}

void STDCALL expglTexGendv (GLenum coord, GLenum pname, const GLdouble *params)
{
    ldrpglTexGendv (coord, pname, params);
}

void STDCALL expglTexGenf (GLenum coord, GLenum pname, GLfloat param)
{
    ldrpglTexGenf (coord, pname, param);
}

void STDCALL expglTexGenfv (GLenum coord, GLenum pname, const GLfloat *params)
{
    ldrpglTexGenfv (coord, pname, params);
}

void STDCALL expglTexGeni (GLenum coord, GLenum pname, GLint param)
{
    ldrpglTexGeni (coord, pname, param);
}

void STDCALL expglTexGeniv (GLenum coord, GLenum pname, const GLint *params)
{
    ldrpglTexGeniv (coord, pname, params);
}

void STDCALL expglTexImage1D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    ldrpglTexImage1D (target, level, internalformat, width, border, format, type, pixels);
}

void STDCALL expglTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    ldrpglTexImage2D (target, level, internalformat, width, height, border, format, type, pixels);
}

void STDCALL expglTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
    ldrpglTexParameterf (target, pname, param);
}

void STDCALL expglTexParameterfv (GLenum target, GLenum pname, const GLfloat *params)
{
    ldrpglTexParameterfv (target, pname, params);
}

void STDCALL expglTexParameteri (GLenum target, GLenum pname, GLint param)
{
    ldrpglTexParameteri (target, pname, param);
}

void STDCALL expglTexParameteriv (GLenum target, GLenum pname, const GLint *params)
{
    ldrpglTexParameteriv (target, pname, params);
}

void STDCALL expglTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
    ldrpglTexSubImage1D (target, level, xoffset, width, format, type, pixels);
}

void STDCALL expglTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
    ldrpglTexSubImage2D (target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void STDCALL expglTranslated (GLdouble x, GLdouble y, GLdouble z)
{
    ldrpglTranslated (x, y, z);
}

void STDCALL expglTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
    ldrpglTranslatef (x, y, z);
}

void STDCALL expglVertex2d (GLdouble x, GLdouble y)
{
    ldrpglVertex2d (x, y);
}

void STDCALL expglVertex2dv (const GLdouble *v)
{
    ldrpglVertex2dv (v);
}

void STDCALL expglVertex2f (GLfloat x, GLfloat y)
{
    ldrpglVertex2f (x, y);
}

void STDCALL expglVertex2fv (const GLfloat *v)
{
    ldrpglVertex2fv (v);
}

void STDCALL expglVertex2i (GLint x, GLint y)
{
    ldrpglVertex2i (x, y);
}

void STDCALL expglVertex2iv (const GLint *v)
{
    ldrpglVertex2iv (v);
}

void STDCALL expglVertex2s (GLshort x, GLshort y)
{
    ldrpglVertex2s (x, y);
}

void STDCALL expglVertex2sv (const GLshort *v)
{
    ldrpglVertex2sv (v);
}

void STDCALL expglVertex3d (GLdouble x, GLdouble y, GLdouble z)
{
    ldrpglVertex3d (x, y, z);
}

void STDCALL expglVertex3dv (const GLdouble *v)
{
    ldrpglVertex3dv (v);
}

void STDCALL expglVertex3f (GLfloat x, GLfloat y, GLfloat z)
{
    ldrpglVertex3f (x, y, z);
}

void STDCALL expglVertex3fv (const GLfloat *v)
{
    ldrpglVertex3fv (v);
}

void STDCALL expglVertex3i (GLint x, GLint y, GLint z)
{
    ldrpglVertex3i (x, y, z);
}

void STDCALL expglVertex3iv (const GLint *v)
{
    ldrpglVertex3iv (v);
}

void STDCALL expglVertex3s (GLshort x, GLshort y, GLshort z)
{
    ldrpglVertex3s (x, y, z);
}

void STDCALL expglVertex3sv (const GLshort *v)
{
    ldrpglVertex3sv (v);
}

void STDCALL expglVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    ldrpglVertex4d (x, y, z, w);
}

void STDCALL expglVertex4dv (const GLdouble *v)
{
    ldrpglVertex4dv (v);
}

void STDCALL expglVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    ldrpglVertex4f (x, y, z, w);
}

void STDCALL expglVertex4fv (const GLfloat *v)
{
    ldrpglVertex4fv (v);
}

void STDCALL expglVertex4i (GLint x, GLint y, GLint z, GLint w)
{
    ldrpglVertex4i (x, y, z, w);
}

void STDCALL expglVertex4iv (const GLint *v)
{
    ldrpglVertex4iv (v);
}

void STDCALL expglVertex4s (GLshort x, GLshort y, GLshort z, GLshort w)
{
    ldrpglVertex4s (x, y, z, w);
}

void STDCALL expglVertex4sv (const GLshort *v)
{
    ldrpglVertex4sv (v);
}

void STDCALL expglViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
    ldrpglViewport (x, y, width, height);
}

void STDCALL expglPointParameterfEXT( GLenum param, GLfloat value)
{
    ldrpglPointParameterfEXT (param, value);
}

void STDCALL expglPointParameterfvEXT( GLenum param, const GLfloat *value)
{
    ldrpglPointParameterfvEXT (param, value);
}

void STDCALL expglLockArraysEXT (int a, int b)
{
    ldrpglLockArraysEXT (a, b);
}

void STDCALL expglUnlockArraysEXT (void)
{
    ldrpglUnlockArraysEXT ();
}

void STDCALL expglActiveTextureARB( GLenum a)
{
    ldrpglActiveTextureARB (a);
}

void STDCALL expglClientActiveTextureARB( GLenum a)
{
    ldrpglClientActiveTextureARB (a);
}

void STDCALL expglGetCompressedTexImage( GLenum target, GLint lod, const void* data)
{
    ldrpglGetCompressedTexImage (target, lod, data);
}

void STDCALL expglDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices)
{
    ldrpglDrawRangeElements (mode, start, end, count, type, indices);
}

void STDCALL expglDrawRangeElementsEXT( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices)
{
    ldrpglDrawRangeElementsEXT (mode, start, end, count, type, indices);
}

void STDCALL expglDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
    ldrpglDrawElements (mode, count, type, indices);
}

void STDCALL expglVertexPointer(GLint size, GLenum type, GLsizei stride, const void *ptr)
{
    ldrpglVertexPointer (size, type, stride, ptr);
}

void STDCALL expglNormalPointer(GLenum type, GLsizei stride, const void *ptr)
{
    ldrpglNormalPointer (type, stride, ptr);
}

void STDCALL expglColorPointer(GLint size, GLenum type, GLsizei stride, const void *ptr)
{
    ldrpglColorPointer (size, type, stride, ptr);
}

void STDCALL expglTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *ptr)
{
    ldrpglTexCoordPointer (size, type, stride, ptr);
}

void STDCALL expglArrayElement(GLint i)
{
    ldrpglArrayElement (i);
}

void STDCALL expglMultiTexCoord1f(GLenum a, GLfloat b)
{
    ldrpglMultiTexCoord1f (a, b);
}

void STDCALL expglMultiTexCoord2f(GLenum a, GLfloat b, GLfloat c)
{
    ldrpglMultiTexCoord2f (a, b, c);
}

void STDCALL expglMultiTexCoord3f(GLenum a, GLfloat b, GLfloat c, GLfloat d)
{
    ldrpglMultiTexCoord3f (a, b, c, d);
}

void STDCALL expglMultiTexCoord4f(GLenum a, GLfloat b, GLfloat c, GLfloat d, GLfloat e)
{
    ldrpglMultiTexCoord4f (a, b, c, d, e);
}

void STDCALL expglActiveTexture(GLenum a)
{
    ldrpglActiveTexture (a);
}

void STDCALL expglClientActiveTexture(GLenum a)
{
    ldrpglClientActiveTexture (a);
}

void STDCALL expglCompressedTexImage3DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data)
{
    ldrpglCompressedTexImage3DARB (target, level, internalformat, width, height, depth, border, imageSize, data);
}

void STDCALL expglCompressedTexImage2DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border,  GLsizei imageSize, const void *data)
{
    ldrpglCompressedTexImage2DARB (target, level, internalformat, width, height, border, imageSize, data);
}

void STDCALL expglCompressedTexImage1DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data)
{
    ldrpglCompressedTexImage1DARB (target, level, internalformat, width, border, imageSize, data);
}

void STDCALL expglCompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data)
{
    ldrpglCompressedTexSubImage3DARB (target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}

void STDCALL expglCompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data)
{
    ldrpglCompressedTexSubImage2DARB (target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

void STDCALL expglCompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data)
{
    ldrpglCompressedTexSubImage1DARB (target, level, xoffset, width, format, imageSize, data);
}

void STDCALL expglDeleteObjectARB(GLhandleARB obj)
{
    ldrpglDeleteObjectARB (obj);
}

GLhandleARB STDCALL expglGetHandleARB(GLenum pname)
{
    return ldrpglGetHandleARB (pname);
}

void STDCALL expglDetachObjectARB(GLhandleARB containerObj, GLhandleARB attachedObj)
{
    ldrpglDetachObjectARB (containerObj, attachedObj);
}

GLhandleARB STDCALL expglCreateShaderObjectARB(GLenum shaderType)
{
    return ldrpglCreateShaderObjectARB (shaderType);
}

void STDCALL expglShaderSourceARB(GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length)
{
    ldrpglShaderSourceARB (shaderObj, count, string, length);
}

void STDCALL expglCompileShaderARB(GLhandleARB shaderObj)
{
    ldrpglCompileShaderARB (shaderObj);
}

GLhandleARB STDCALL expglCreateProgramObjectARB(void)
{
    return ldrpglCreateProgramObjectARB ();
}

void STDCALL expglAttachObjectARB(GLhandleARB containerObj, GLhandleARB obj)
{
    ldrpglAttachObjectARB (containerObj, obj);
}

void STDCALL expglLinkProgramARB(GLhandleARB programObj)
{
    ldrpglLinkProgramARB (programObj);
}

void STDCALL expglUseProgramObjectARB(GLhandleARB programObj)
{
    ldrpglUseProgramObjectARB (programObj);
}

void STDCALL expglValidateProgramARB(GLhandleARB programObj)
{
    ldrpglValidateProgramARB (programObj);
}

void STDCALL expglBindProgramARB(GLenum target, GLuint program)
{
    ldrpglBindProgramARB (target, program);
}

void STDCALL expglDeleteProgramsARB(GLsizei n, const GLuint *programs)
{
    ldrpglDeleteProgramsARB (n, programs);
}

void STDCALL expglGenProgramsARB(GLsizei n, GLuint *programs)
{
    ldrpglGenProgramsARB (n, programs);
}

void STDCALL expglProgramStringARB(GLenum target, GLenum format, GLsizei len, const void *string)
{
    ldrpglProgramStringARB (target, format, len, string);
}

void STDCALL expglProgramEnvParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    ldrpglProgramEnvParameter4fARB (target, index, x, y, z, w);
}

void STDCALL expglProgramLocalParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    ldrpglProgramLocalParameter4fARB (target, index, x, y, z, w);
}

void STDCALL expglGetProgramivARB( GLenum target, GLenum pname, GLint *params)
{
    ldrpglGetProgramivARB (target, pname, params);
}

void STDCALL expglUniform1fARB(GLint location, GLfloat v0)
{
    ldrpglUniform1fARB (location, v0);
}

void STDCALL expglUniform2fARB(GLint location, GLfloat v0, GLfloat v1)
{
    ldrpglUniform2fARB (location, v0, v1);
}

void STDCALL expglUniform3fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
    ldrpglUniform3fARB (location, v0, v1, v2);
}

void STDCALL expglUniform4fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    ldrpglUniform4fARB (location, v0, v1, v2, v3);
}

void STDCALL expglUniform1iARB(GLint location, GLint v0)
{
    ldrpglUniform1iARB (location, v0);
}

void STDCALL expglUniform2iARB(GLint location, GLint v0, GLint v1)
{
    ldrpglUniform2iARB (location, v0, v1);
}

void STDCALL expglUniform3iARB(GLint location, GLint v0, GLint v1, GLint v2)
{
    ldrpglUniform3iARB (location, v0, v1, v2);
}

void STDCALL expglUniform4iARB(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
    ldrpglUniform4iARB (location, v0, v1, v2, v3);
}

void STDCALL expglUniform1fvARB(GLint location, GLsizei count, const GLfloat *value)
{
    ldrpglUniform1fvARB (location, count, value);
}

void STDCALL expglUniform2fvARB(GLint location, GLsizei count, const GLfloat *value)
{
    ldrpglUniform2fvARB (location, count, value);
}

void STDCALL expglUniform3fvARB(GLint location, GLsizei count, const GLfloat *value)
{
    ldrpglUniform3fvARB (location, count, value);
}

void STDCALL expglUniform4fvARB(GLint location, GLsizei count, const GLfloat *value)
{
    ldrpglUniform4fvARB (location, count, value);
}

void STDCALL expglUniform1ivARB(GLint location, GLsizei count, const GLint *value)
{
    ldrpglUniform1ivARB (location, count, value);
}

void STDCALL expglUniform2ivARB(GLint location, GLsizei count, const GLint *value)
{
    ldrpglUniform2ivARB (location, count, value);
}

void STDCALL expglUniform3ivARB(GLint location, GLsizei count, const GLint *value)
{
    ldrpglUniform3ivARB (location, count, value);
}

void STDCALL expglUniform4ivARB(GLint location, GLsizei count, const GLint *value)
{
    ldrpglUniform4ivARB (location, count, value);
}

void STDCALL expglUniformMatrix2fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    ldrpglUniformMatrix2fvARB (location, count, transpose, value);
}

void STDCALL expglUniformMatrix3fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    ldrpglUniformMatrix3fvARB (location, count, transpose, value);
}

void STDCALL expglUniformMatrix4fvARB(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    ldrpglUniformMatrix4fvARB (location, count, transpose, value);
}

void STDCALL expglGetObjectParameterfvARB(GLhandleARB obj, GLenum pname, GLfloat *params)
{
    ldrpglGetObjectParameterfvARB (obj, pname, params);
}

void STDCALL expglGetObjectParameterivARB(GLhandleARB obj, GLenum pname, GLint *params)
{
    ldrpglGetObjectParameterivARB (obj, pname, params);
}

void STDCALL expglGetInfoLogARB(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog)
{
    ldrpglGetInfoLogARB (obj, maxLength, length, infoLog);
}

void STDCALL expglGetAttachedObjectsARB(GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj)
{
    ldrpglGetAttachedObjectsARB (containerObj, maxCount, count, obj);
}

GLint STDCALL expglGetUniformLocationARB(GLhandleARB programObj, const GLcharARB *name)
{
    return ldrpglGetUniformLocationARB (programObj, name);
}

void STDCALL expglGetActiveUniformARB(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name)
{
    ldrpglGetActiveUniformARB (programObj, index, maxLength, length, size, type, name);
}

void STDCALL expglGetUniformfvARB(GLhandleARB programObj, GLint location, GLfloat *params)
{
    ldrpglGetUniformfvARB (programObj, location, params);
}

void STDCALL expglGetUniformivARB(GLhandleARB programObj, GLint location, GLint *params)
{
    ldrpglGetUniformivARB (programObj, location, params);
}

void STDCALL expglGetShaderSourceARB(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source)
{
    ldrpglGetShaderSourceARB (obj, maxLength, length, source);
}

void STDCALL expglTexImage3D( GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels)
{
    ldrpglTexImage3D (target, level, internalFormat, width, height, depth, border, format, type, pixels);
}

void STDCALL expglTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels)
{
    ldrpglTexSubImage3D (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

void STDCALL expglCopyTexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    ldrpglCopyTexSubImage3D (target, level, xoffset, yoffset, zoffset, x, y, width, height);
}

void STDCALL expglBlendEquationEXT(GLenum a)
{
    ldrpglBlendEquationEXT (a);
}

void STDCALL expglStencilOpSeparate(GLenum a, GLenum b, GLenum c, GLenum d)
{
    ldrpglStencilOpSeparate (a, b, c, d);
}

void STDCALL expglStencilFuncSeparate(GLenum a, GLenum b, GLint c, GLuint d)
{
    ldrpglStencilFuncSeparate (a, b, c, d);
}

void STDCALL expglActiveStencilFaceEXT(GLenum a)
{
    ldrpglActiveStencilFaceEXT (a);
}

void STDCALL expglVertexAttribPointerARB(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer)
{
    ldrpglVertexAttribPointerARB (index, size, type, normalized, stride, pointer);
}

void STDCALL expglEnableVertexAttribArrayARB(GLuint index)
{
    ldrpglEnableVertexAttribArrayARB (index);
}

void STDCALL expglDisableVertexAttribArrayARB(GLuint index)
{
    ldrpglDisableVertexAttribArrayARB (index);
}

void STDCALL expglBindAttribLocationARB(GLhandleARB programObj, GLuint index, const GLcharARB *name)
{
    ldrpglBindAttribLocationARB (programObj, index, name);
}

void STDCALL expglGetActiveAttribARB(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name)
{
    ldrpglGetActiveAttribARB (programObj, index, maxLength, length, size, type, name);
}

GLint STDCALL expglGetAttribLocationARB(GLhandleARB programObj, const GLcharARB *name)
{
    return ldrpglGetAttribLocationARB (programObj, name);
}

void STDCALL expglBindBufferARB(GLenum target, GLuint buffer)
{
    ldrpglBindBufferARB (target, buffer);
}

void STDCALL expglDeleteBuffersARB(GLsizei n, const GLuint *buffers)
{
    ldrpglDeleteBuffersARB (n, buffers);
}

void STDCALL expglGenBuffersARB(GLsizei n, GLuint *buffers)
{
    ldrpglGenBuffersARB (n, buffers);
}

GLboolean STDCALL expglIsBufferARB(GLuint buffer)
{
    return ldrpglIsBufferARB (buffer);
}

void* STDCALL expglMapBufferARB(GLenum target, GLenum access)
{
    return ldrpglMapBufferARB (target, access);
}

GLboolean STDCALL expglUnmapBufferARB(GLenum target)
{
    return ldrpglUnmapBufferARB (target);
}

void STDCALL expglBufferDataARB(GLenum target, GLsizeiptrARB size, const void *data, GLenum usage)
{
    ldrpglBufferDataARB (target, size, data, usage);
}

void STDCALL expglBufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void *data)
{
    ldrpglBufferSubDataARB (target, offset, size, data);
}

void STDCALL expglGenQueriesARB(GLsizei n, GLuint *ids)
{
    ldrpglGenQueriesARB (n, ids);
}

void STDCALL expglDeleteQueriesARB(GLsizei n, const GLuint *ids)
{
    ldrpglDeleteQueriesARB (n, ids);
}

GLboolean STDCALL expglIsQueryARB(GLuint id)
{
    return ldrpglIsQueryARB (id);
}

void STDCALL expglBeginQueryARB(GLenum target, GLuint id)
{
    ldrpglBeginQueryARB (target, id);
}

void STDCALL expglEndQueryARB(GLenum target)
{
    ldrpglEndQueryARB (target);
}

void STDCALL expglGetQueryivARB(GLenum target, GLenum pname, GLint *params)
{
    ldrpglGetQueryivARB (target, pname, params);
}

void STDCALL expglGetQueryObjectivARB(GLuint id, GLenum pname, GLint *params)
{
    ldrpglGetQueryObjectivARB (id, pname, params);
}

void STDCALL expglGetQueryObjectuivARB(GLuint id, GLenum pname, GLuint *params)
{
    ldrpglGetQueryObjectuivARB (id, pname, params);
}

void STDCALL  expglSelectTextureSGIS ( GLenum a)
{
    ldrpglSelectTextureSGIS (a);
}

void STDCALL  expglMTexCoord2fSGIS ( GLenum a, GLfloat b, GLfloat c)
{
    ldrpglMTexCoord2fSGIS (a, b, c);
}

void STDCALL  expglSwapInterval ( int interval)
{
    ldrpglSwapInterval (interval);
}

GLboolean STDCALL expglIsRenderbuffer (GLuint renderbuffer)
{
    return ldrpglIsRenderbuffer (renderbuffer);
}

void STDCALL expglBindRenderbuffer (GLenum target, GLuint renderbuffer)
{
    ldrpglBindRenderbuffer (target, renderbuffer);
}

void STDCALL expglDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers)
{
    ldrpglDeleteRenderbuffers (n, renderbuffers);
}

void STDCALL expglGenRenderbuffers (GLsizei n, GLuint *renderbuffers)
{
    ldrpglGenRenderbuffers (n, renderbuffers);
}

void STDCALL expglRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    ldrpglRenderbufferStorage (target, internalformat, width, height);
}

void STDCALL expglRenderbufferStorageMultisample (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    ldrpglRenderbufferStorageMultisample (target, samples, internalformat, width, height);
}

void STDCALL expglGetRenderbufferParameteriv (GLenum target, GLenum pname, GLint *params)
{
    ldrpglGetRenderbufferParameteriv (target, pname, params);
}

GLboolean expglIsFramebuffer (GLuint framebuffer)
{
    return ldrpglIsFramebuffer (framebuffer);
}

void STDCALL expglBindFramebuffer (GLenum target, GLuint framebuffer)
{
    ldrpglBindFramebuffer (target, framebuffer);
}

void STDCALL expglDeleteFramebuffers (GLsizei n, const GLuint *framebuffers)
{
    ldrpglDeleteFramebuffers (n, framebuffers);
}

void STDCALL expglGenFramebuffers (GLsizei n, GLuint *framebuffers)
{
    ldrpglGenFramebuffers (n, framebuffers);
}

GLenum STDCALL expglCheckFramebufferStatus (GLenum target)
{
    return ldrpglCheckFramebufferStatus (target);
}

void STDCALL expglFramebufferTexture1D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    ldrpglFramebufferTexture1D (target, attachment, textarget, texture, level);
}

void STDCALL expglFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    ldrpglFramebufferTexture2D (target, attachment, textarget, texture, level);
}

void STDCALL expglFramebufferTexture3D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer)
{
    ldrpglFramebufferTexture3D (target, attachment, textarget, texture, level, layer);
}

void STDCALL expglFramebufferTextureLayer (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    ldrpglFramebufferTextureLayer (target, attachment, texture, level, layer);
}

void STDCALL expglFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    ldrpglFramebufferRenderbuffer (target, attachment, renderbuffertarget, renderbuffer);
}

void STDCALL expglGetFramebufferAttachmentParameteriv (GLenum target, GLenum attachment, GLenum pname, GLint *params)
{
    ldrpglGetFramebufferAttachmentParameteriv (target, attachment, pname, params);
}

void STDCALL expglBlitFramebuffer (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    ldrpglBlitFramebuffer (srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void STDCALL expglGenerateMipmap (GLenum target)
{
    ldrpglGenerateMipmap (target);
}

void STDCALL expglGetCompressedTexImageARB (GLenum target, GLint level, GLvoid *img)
{
    ldrpglGetCompressedTexImageARB (target, level, img);
}

void STDCALL expglCopyTexSubImage3DEXT (GLenum a, GLint b, GLint c, GLint d, GLint e, GLint f, GLint g, GLsizei h, GLsizei i)
{
    ldrpglCopyTexSubImage3DEXT (a, b, c, d, e, f, g, h, i);
}

void STDCALL expglTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
    ldrpglTexSubImage3DEXT (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

void STDCALL expglTexImage3DEXT(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    ldrpglTexImage3DEXT (target, level, internalformat, width, height, depth, border, format, type, pixels);
}

void STDCALL expglMultiTexCoord1fARB(GLenum target, GLfloat s)
{
    ldrpglMultiTexCoord1fARB (target, s);
}

void STDCALL expglMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t)
{
    ldrpglMultiTexCoord2fARB (target, s, t);
}

void STDCALL expglMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
    ldrpglMultiTexCoord3fARB (target, s, t, r);
}

void STDCALL expglMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    ldrpglMultiTexCoord4fARB (target, s, t, r, q);
}
void STDCALL expglBindVertexArray(GLuint array)
{
    ldrpglBindVertexArray(array);
}
void STDCALL expglDeleteVertexArrays(GLsizei n, const GLuint *arrays)
{
    ldrpglDeleteVertexArrays(n, arrays);
}
void STDCALL expglGenVertexArrays(GLsizei n, GLuint *arrays)
{
    ldrpglGenVertexArrays(n, arrays);
}
GLboolean STDCALL expglIsVertexArray(GLuint array)
{
    return ldrpglIsVertexArray(array);
}
void STDCALL expglDebugMessageControlARB(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled)
{
    ldrpglDebugMessageControlARB(source, type, severity, count, ids, enabled);
}
void STDCALL expglDebugMessageInsertARB(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* buf)
{
    ldrpglDebugMessageInsertARB(source, type, id, severity, length, buf);
}
void STDCALL expglDebugMessageCallbackARB(GLvoid *callback, GLvoid *userParam)
{
    ldrpglDebugMessageCallbackARB(callback, userParam);
}
GLuint STDCALL expglGetDebugMessageLogARB(GLuint count, GLsizei bufsize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog)
{
    return ldrpglGetDebugMessageLogARB(count, bufsize, sources, types, ids, severities, lengths, messageLog);
}
void STDCALL expglVertexAttrib2f(GLuint index, GLfloat v0, GLfloat v1)
{
    ldrpglVertexAttrib2f(index, v0, v1);
}
void STDCALL expglVertexAttrib2fv(GLuint index, const GLfloat *v)
{
    ldrpglVertexAttrib2fv(index, v);
}
void STDCALL expglVertexAttrib3fv(GLuint index, const GLfloat *v)
{
    ldrpglVertexAttrib3fv(index, v);
}
