/*
gl_vid_common.c - Common video initialization functions
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/


#ifndef XASH_DEDICATED

#include "common.h"
#include "client.h"
#include "gl_local.h"
#include "mod_local.h"
#include "input.h"
#include "gl_vidnt.h"

extern convar_t *renderinfo;
convar_t	*gl_allow_software;
convar_t	*gl_extensions;
convar_t	*gl_alphabits;
convar_t	*gl_stencilbits;
convar_t	*gl_texture_anisotropy;
convar_t	*gl_compress_textures;
convar_t	*gl_luminance_textures;
convar_t	*gl_compensate_gamma_screenshots;
convar_t	*gl_keeptjunctions;
convar_t	*gl_texture_lodbias;
convar_t	*gl_showtextures;
convar_t	*gl_detailscale;
convar_t	*gl_swapInterval;
convar_t	*gl_check_errors;
convar_t	*gl_allow_static;
convar_t	*gl_allow_mirrors;
convar_t	*gl_texturemode;
convar_t	*gl_wireframe;
convar_t	*gl_overview;
convar_t	*gl_max_size;
convar_t	*gl_picmip;
convar_t	*gl_skymip;
convar_t	*gl_finish;
convar_t	*gl_nosort;
convar_t	*gl_clear;
convar_t	*gl_test;
convar_t	*gl_msaa;
convar_t	*gl_overbright;
convar_t	*gl_overbright_studio;

convar_t	*r_xpos;
convar_t	*r_ypos;
convar_t	*r_speeds;
convar_t	*r_fullbright;
convar_t	*r_norefresh;
convar_t	*r_lighting_extended;
convar_t	*r_lighting_modulate;
convar_t	*r_lighting_ambient;
convar_t	*r_detailtextures;
convar_t	*r_faceplanecull;
convar_t	*r_drawentities;
convar_t	*r_adjust_fov;
convar_t	*r_flaresize;
convar_t	*r_lefthand;
convar_t	*r_decals;
convar_t	*r_novis;
convar_t	*r_nocull;
convar_t	*r_lockpvs;
convar_t	*r_lockcull;
convar_t	*r_dynamic;
convar_t	*r_lightmap;
convar_t	*r_fastsky;
convar_t	*r_vbo;
convar_t 	*r_bump;
convar_t	*r_vbo_dlightmode;
convar_t	*r_underwater_distortion;
convar_t	*mp_decals;

convar_t	*vid_displayfrequency;
convar_t	*vid_fullscreen;
convar_t	*vid_gamma;
convar_t	*vid_texgamma;
convar_t	*vid_mode;
convar_t	*vid_highdpi;

byte		*r_temppool;

ref_globals_t	tr;
glconfig_t	glConfig;
glstate_t		glState;
glwstate_t	glw_state;

vidmode_t vidmode[] =
{
{ "640 x 480",			640,	480,	false	},
{ "800 x 600",			800,	600,	false	},
{ "960 x 720",			960,	720,	false	},
{ "1024 x 768",			1024,	768,	false	},
{ "1152 x 864",			1152,	864,	false	},
{ "1280 x 800",			1280,	800,	false	},
{ "1280 x 960",			1280,	960,	false	},
{ "1280 x 1024",		1280,	1024,	false	},
{ "1600 x 1200",		1600,	1200,	false	},
{ "2048 x 1536",		2048,	1536,	false	},
{ "800 x 480 (wide)",	800,	480,	true	},
{ "856 x 480 (wide)",	856,	480,	true	},
{ "960 x 540 (wide)",	960,	540,	true	},
{ "1024 x 576 (wide)",	1024,	576,	true	},
{ "1024 x 600 (wide)",	1024,	600,	true	},
{ "1280 x 720 (wide)",	1280,	720,	true	},
{ "1360 x 768 (wide)",	1360,	768,	true	},
{ "1366 x 768 (wide)",	1366,	768,	true	},
{ "1440 x 900 (wide)",	1440,	900,	true	},
{ "1680 x 1050 (wide)",	1680,	1050,	true	},
{ "1920 x 1080 (wide)",	1920,	1080,	true	},
{ "1920 x 1200 (wide)",	1920,	1200,	true	},
{ "2560 x 1440 (wide)",	2560,	1440,	true	},
{ "2560 x 1600 (wide)",	2560,	1600,	true	},
{ "1600 x 900 (wide)",	1600,	 900,	true	},
};

int num_vidmodes = ( sizeof( vidmode ) / sizeof( vidmode[0] ));

/*
=================
VID_GetModeString
=================
*/
const char *VID_GetModeString( int vid_mode )
{
	if( vid_mode >= 0 && vid_mode < num_vidmodes )
		return vidmode[vid_mode].desc;
	return NULL; // out of bounds
}

/*
==============================================

OpenGL funcs

==============================================
*/
#ifndef XASH_GL_STATIC
// helper opengl functions
GLenum ( APIENTRY *pglGetError )(void);
const GLubyte * ( APIENTRY *pglGetString )(GLenum name);

// base gl functions
void ( APIENTRY *pglAccum )(GLenum op, GLfloat value);
void ( APIENTRY *pglAlphaFunc )(GLenum func, GLclampf ref);
void ( APIENTRY *pglArrayElement )(GLint i);
void ( APIENTRY *pglBegin )(GLenum mode);
void ( APIENTRY *pglBindTexture )(GLenum target, GLuint texture);
void ( APIENTRY *pglBitmap )(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
void ( APIENTRY *pglBlendFunc )(GLenum sfactor, GLenum dfactor);
void ( APIENTRY *pglCallList )(GLuint list);
void ( APIENTRY *pglCallLists )(GLsizei n, GLenum type, const GLvoid *lists);
void ( APIENTRY *pglClear )(GLbitfield mask);
void ( APIENTRY *pglClearAccum )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void ( APIENTRY *pglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void ( APIENTRY *pglClearDepth )(GLclampd depth);
void ( APIENTRY *pglClearIndex )(GLfloat c);
void ( APIENTRY *pglClearStencil )(GLint s);
GLboolean ( APIENTRY *pglIsEnabled )( GLenum cap );
GLboolean ( APIENTRY *pglIsList )( GLuint list );
GLboolean ( APIENTRY *pglIsTexture )( GLuint texture );
void ( APIENTRY *pglClipPlane )(GLenum plane, const GLdouble *equation);
void ( APIENTRY *pglColor3b )(GLbyte red, GLbyte green, GLbyte blue);
void ( APIENTRY *pglColor3bv )(const GLbyte *v);
void ( APIENTRY *pglColor3d )(GLdouble red, GLdouble green, GLdouble blue);
void ( APIENTRY *pglColor3dv )(const GLdouble *v);
void ( APIENTRY *pglColor3f )(GLfloat red, GLfloat green, GLfloat blue);
void ( APIENTRY *pglColor3fv )(const GLfloat *v);
void ( APIENTRY *pglColor3i )(GLint red, GLint green, GLint blue);
void ( APIENTRY *pglColor3iv )(const GLint *v);
void ( APIENTRY *pglColor3s )(GLshort red, GLshort green, GLshort blue);
void ( APIENTRY *pglColor3sv )(const GLshort *v);
void ( APIENTRY *pglColor3ub )(GLubyte red, GLubyte green, GLubyte blue);
void ( APIENTRY *pglColor3ubv )(const GLubyte *v);
void ( APIENTRY *pglColor3ui )(GLuint red, GLuint green, GLuint blue);
void ( APIENTRY *pglColor3uiv )(const GLuint *v);
void ( APIENTRY *pglColor3us )(GLushort red, GLushort green, GLushort blue);
void ( APIENTRY *pglColor3usv )(const GLushort *v);
void ( APIENTRY *pglColor4b )(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
void ( APIENTRY *pglColor4bv )(const GLbyte *v);
void ( APIENTRY *pglColor4d )(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
void ( APIENTRY *pglColor4dv )(const GLdouble *v);
void ( APIENTRY *pglColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void ( APIENTRY *pglColor4fv )(const GLfloat *v);
void ( APIENTRY *pglColor4i )(GLint red, GLint green, GLint blue, GLint alpha);
void ( APIENTRY *pglColor4iv )(const GLint *v);
void ( APIENTRY *pglColor4s )(GLshort red, GLshort green, GLshort blue, GLshort alpha);
void ( APIENTRY *pglColor4sv )(const GLshort *v);
void ( APIENTRY *pglColor4ub )(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void ( APIENTRY *pglColor4ubv )(const GLubyte *v);
void ( APIENTRY *pglColor4ui )(GLuint red, GLuint green, GLuint blue, GLuint alpha);
void ( APIENTRY *pglColor4uiv )(const GLuint *v);
void ( APIENTRY *pglColor4us )(GLushort red, GLushort green, GLushort blue, GLushort alpha);
void ( APIENTRY *pglColor4usv )(const GLushort *v);
void ( APIENTRY *pglColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void ( APIENTRY *pglColorMaterial )(GLenum face, GLenum mode);
void ( APIENTRY *pglColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY *pglCopyPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
void ( APIENTRY *pglCopyTexImage1D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
void ( APIENTRY *pglCopyTexImage2D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void ( APIENTRY *pglCopyTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
void ( APIENTRY *pglCopyTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY *pglCullFace )(GLenum mode);
void ( APIENTRY *pglDeleteLists )(GLuint list, GLsizei range);
void ( APIENTRY *pglDeleteTextures )(GLsizei n, const GLuint *textures);
void ( APIENTRY *pglDepthFunc )(GLenum func);
void ( APIENTRY *pglDepthMask )(GLboolean flag);
void ( APIENTRY *pglDepthRange )(GLclampd zNear, GLclampd zFar);
void ( APIENTRY *pglDisable )(GLenum cap);
void ( APIENTRY *pglDisableClientState )(GLenum array);
void ( APIENTRY *pglDrawArrays )(GLenum mode, GLint first, GLsizei count);
void ( APIENTRY *pglDrawBuffer )(GLenum mode);
void ( APIENTRY *pglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void ( APIENTRY *pglDrawPixels )(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY *pglEdgeFlag )(GLboolean flag);
void ( APIENTRY *pglEdgeFlagPointer )(GLsizei stride, const GLvoid *pointer);
void ( APIENTRY *pglEdgeFlagv )(const GLboolean *flag);
void ( APIENTRY *pglEnable )(GLenum cap);
void ( APIENTRY *pglEnableClientState )(GLenum array);
void ( APIENTRY *pglEnd )(void);
void ( APIENTRY *pglEndList )(void);
void ( APIENTRY *pglEvalCoord1d )(GLdouble u);
void ( APIENTRY *pglEvalCoord1dv )(const GLdouble *u);
void ( APIENTRY *pglEvalCoord1f )(GLfloat u);
void ( APIENTRY *pglEvalCoord1fv )(const GLfloat *u);
void ( APIENTRY *pglEvalCoord2d )(GLdouble u, GLdouble v);
void ( APIENTRY *pglEvalCoord2dv )(const GLdouble *u);
void ( APIENTRY *pglEvalCoord2f )(GLfloat u, GLfloat v);
void ( APIENTRY *pglEvalCoord2fv )(const GLfloat *u);
void ( APIENTRY *pglEvalMesh1 )(GLenum mode, GLint i1, GLint i2);
void ( APIENTRY *pglEvalMesh2 )(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
void ( APIENTRY *pglEvalPoint1 )(GLint i);
void ( APIENTRY *pglEvalPoint2 )(GLint i, GLint j);
void ( APIENTRY *pglFeedbackBuffer )(GLsizei size, GLenum type, GLfloat *buffer);
void ( APIENTRY *pglFinish )(void);
void ( APIENTRY *pglFlush )(void);
void ( APIENTRY *pglFogf )(GLenum pname, GLfloat param);
void ( APIENTRY *pglFogfv )(GLenum pname, const GLfloat *params);
void ( APIENTRY *pglFogi )(GLenum pname, GLint param);
void ( APIENTRY *pglFogiv )(GLenum pname, const GLint *params);
void ( APIENTRY *pglFrontFace )(GLenum mode);
void ( APIENTRY *pglFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void ( APIENTRY *pglGenTextures )(GLsizei n, GLuint *textures);
void ( APIENTRY *pglGetBooleanv )(GLenum pname, GLboolean *params);
void ( APIENTRY *pglGetClipPlane )(GLenum plane, GLdouble *equation);
void ( APIENTRY *pglGetDoublev )(GLenum pname, GLdouble *params);
void ( APIENTRY *pglGetFloatv )(GLenum pname, GLfloat *params);
void ( APIENTRY *pglGetIntegerv )(GLenum pname, GLint *params);
void ( APIENTRY *pglGetLightfv )(GLenum light, GLenum pname, GLfloat *params);
void ( APIENTRY *pglGetLightiv )(GLenum light, GLenum pname, GLint *params);
void ( APIENTRY *pglGetMapdv )(GLenum target, GLenum query, GLdouble *v);
void ( APIENTRY *pglGetMapfv )(GLenum target, GLenum query, GLfloat *v);
void ( APIENTRY *pglGetMapiv )(GLenum target, GLenum query, GLint *v);
void ( APIENTRY *pglGetMaterialfv )(GLenum face, GLenum pname, GLfloat *params);
void ( APIENTRY *pglGetMaterialiv )(GLenum face, GLenum pname, GLint *params);
void ( APIENTRY *pglGetPixelMapfv )(GLenum map, GLfloat *values);
void ( APIENTRY *pglGetPixelMapuiv )(GLenum map, GLuint *values);
void ( APIENTRY *pglGetPixelMapusv )(GLenum map, GLushort *values);
void ( APIENTRY *pglGetPointerv )(GLenum pname, GLvoid* *params);
void ( APIENTRY *pglGetPolygonStipple )(GLubyte *mask);
void ( APIENTRY *pglGetTexEnvfv )(GLenum target, GLenum pname, GLfloat *params);
void ( APIENTRY *pglGetTexEnviv )(GLenum target, GLenum pname, GLint *params);
void ( APIENTRY *pglGetTexGendv )(GLenum coord, GLenum pname, GLdouble *params);
void ( APIENTRY *pglGetTexGenfv )(GLenum coord, GLenum pname, GLfloat *params);
void ( APIENTRY *pglGetTexGeniv )(GLenum coord, GLenum pname, GLint *params);
void ( APIENTRY *pglGetTexImage )(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
void ( APIENTRY *pglGetTexLevelParameterfv )(GLenum target, GLint level, GLenum pname, GLfloat *params);
void ( APIENTRY *pglGetTexLevelParameteriv )(GLenum target, GLint level, GLenum pname, GLint *params);
void ( APIENTRY *pglGetTexParameterfv )(GLenum target, GLenum pname, GLfloat *params);
void ( APIENTRY *pglGetTexParameteriv )(GLenum target, GLenum pname, GLint *params);
void ( APIENTRY *pglHint )(GLenum target, GLenum mode);
void ( APIENTRY *pglIndexMask )(GLuint mask);
void ( APIENTRY *pglIndexPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY *pglIndexd )(GLdouble c);
void ( APIENTRY *pglIndexdv )(const GLdouble *c);
void ( APIENTRY *pglIndexf )(GLfloat c);
void ( APIENTRY *pglIndexfv )(const GLfloat *c);
void ( APIENTRY *pglIndexi )(GLint c);
void ( APIENTRY *pglIndexiv )(const GLint *c);
void ( APIENTRY *pglIndexs )(GLshort c);
void ( APIENTRY *pglIndexsv )(const GLshort *c);
void ( APIENTRY *pglIndexub )(GLubyte c);
void ( APIENTRY *pglIndexubv )(const GLubyte *c);
void ( APIENTRY *pglInitNames )(void);
void ( APIENTRY *pglInterleavedArrays )(GLenum format, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY *pglLightModelf )(GLenum pname, GLfloat param);
void ( APIENTRY *pglLightModelfv )(GLenum pname, const GLfloat *params);
void ( APIENTRY *pglLightModeli )(GLenum pname, GLint param);
void ( APIENTRY *pglLightModeliv )(GLenum pname, const GLint *params);
void ( APIENTRY *pglLightf )(GLenum light, GLenum pname, GLfloat param);
void ( APIENTRY *pglLightfv )(GLenum light, GLenum pname, const GLfloat *params);
void ( APIENTRY *pglLighti )(GLenum light, GLenum pname, GLint param);
void ( APIENTRY *pglLightiv )(GLenum light, GLenum pname, const GLint *params);
void ( APIENTRY *pglLineStipple )(GLint factor, GLushort pattern);
void ( APIENTRY *pglLineWidth )(GLfloat width);
void ( APIENTRY *pglListBase )(GLuint base);
void ( APIENTRY *pglLoadIdentity )(void);
void ( APIENTRY *pglLoadMatrixd )(const GLdouble *m);
void ( APIENTRY *pglLoadMatrixf )(const GLfloat *m);
void ( APIENTRY *pglLoadName )(GLuint name);
void ( APIENTRY *pglLogicOp )(GLenum opcode);
void ( APIENTRY *pglMap1d )(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
void ( APIENTRY *pglMap1f )(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
void ( APIENTRY *pglMap2d )(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
void ( APIENTRY *pglMap2f )(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
void ( APIENTRY *pglMapGrid1d )(GLint un, GLdouble u1, GLdouble u2);
void ( APIENTRY *pglMapGrid1f )(GLint un, GLfloat u1, GLfloat u2);
void ( APIENTRY *pglMapGrid2d )(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
void ( APIENTRY *pglMapGrid2f )(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
void ( APIENTRY *pglMaterialf )(GLenum face, GLenum pname, GLfloat param);
void ( APIENTRY *pglMaterialfv )(GLenum face, GLenum pname, const GLfloat *params);
void ( APIENTRY *pglMateriali )(GLenum face, GLenum pname, GLint param);
void ( APIENTRY *pglMaterialiv )(GLenum face, GLenum pname, const GLint *params);
void ( APIENTRY *pglMatrixMode )(GLenum mode);
void ( APIENTRY *pglMultMatrixd )(const GLdouble *m);
void ( APIENTRY *pglMultMatrixf )(const GLfloat *m);
void ( APIENTRY *pglNewList )(GLuint list, GLenum mode);
void ( APIENTRY *pglNormal3b )(GLbyte nx, GLbyte ny, GLbyte nz);
void ( APIENTRY *pglNormal3bv )(const GLbyte *v);
void ( APIENTRY *pglNormal3d )(GLdouble nx, GLdouble ny, GLdouble nz);
void ( APIENTRY *pglNormal3dv )(const GLdouble *v);
void ( APIENTRY *pglNormal3f )(GLfloat nx, GLfloat ny, GLfloat nz);
void ( APIENTRY *pglNormal3fv )(const GLfloat *v);
void ( APIENTRY *pglNormal3i )(GLint nx, GLint ny, GLint nz);
void ( APIENTRY *pglNormal3iv )(const GLint *v);
void ( APIENTRY *pglNormal3s )(GLshort nx, GLshort ny, GLshort nz);
void ( APIENTRY *pglNormal3sv )(const GLshort *v);
void ( APIENTRY *pglNormalPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY *pglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void ( APIENTRY *pglPassThrough )(GLfloat token);
void ( APIENTRY *pglPixelMapfv )(GLenum map, GLsizei mapsize, const GLfloat *values);
void ( APIENTRY *pglPixelMapuiv )(GLenum map, GLsizei mapsize, const GLuint *values);
void ( APIENTRY *pglPixelMapusv )(GLenum map, GLsizei mapsize, const GLushort *values);
void ( APIENTRY *pglPixelStoref )(GLenum pname, GLfloat param);
void ( APIENTRY *pglPixelStorei )(GLenum pname, GLint param);
void ( APIENTRY *pglPixelTransferf )(GLenum pname, GLfloat param);
void ( APIENTRY *pglPixelTransferi )(GLenum pname, GLint param);
void ( APIENTRY *pglPixelZoom )(GLfloat xfactor, GLfloat yfactor);
void ( APIENTRY *pglPointSize )(GLfloat size);
void ( APIENTRY *pglPolygonMode )(GLenum face, GLenum mode);
void ( APIENTRY *pglPolygonOffset )(GLfloat factor, GLfloat units);
void ( APIENTRY *pglPolygonStipple )(const GLubyte *mask);
void ( APIENTRY *pglPopAttrib )(void);
void ( APIENTRY *pglPopClientAttrib )(void);
void ( APIENTRY *pglPopMatrix )(void);
void ( APIENTRY *pglPopName )(void);
void ( APIENTRY *pglPushAttrib )(GLbitfield mask);
void ( APIENTRY *pglPushClientAttrib )(GLbitfield mask);
void ( APIENTRY *pglPushMatrix )(void);
void ( APIENTRY *pglPushName )(GLuint name);
void ( APIENTRY *pglRasterPos2d )(GLdouble x, GLdouble y);
void ( APIENTRY *pglRasterPos2dv )(const GLdouble *v);
void ( APIENTRY *pglRasterPos2f )(GLfloat x, GLfloat y);
void ( APIENTRY *pglRasterPos2fv )(const GLfloat *v);
void ( APIENTRY *pglRasterPos2i )(GLint x, GLint y);
void ( APIENTRY *pglRasterPos2iv )(const GLint *v);
void ( APIENTRY *pglRasterPos2s )(GLshort x, GLshort y);
void ( APIENTRY *pglRasterPos2sv )(const GLshort *v);
void ( APIENTRY *pglRasterPos3d )(GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY *pglRasterPos3dv )(const GLdouble *v);
void ( APIENTRY *pglRasterPos3f )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY *pglRasterPos3fv )(const GLfloat *v);
void ( APIENTRY *pglRasterPos3i )(GLint x, GLint y, GLint z);
void ( APIENTRY *pglRasterPos3iv )(const GLint *v);
void ( APIENTRY *pglRasterPos3s )(GLshort x, GLshort y, GLshort z);
void ( APIENTRY *pglRasterPos3sv )(const GLshort *v);
void ( APIENTRY *pglRasterPos4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void ( APIENTRY *pglRasterPos4dv )(const GLdouble *v);
void ( APIENTRY *pglRasterPos4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void ( APIENTRY *pglRasterPos4fv )(const GLfloat *v);
void ( APIENTRY *pglRasterPos4i )(GLint x, GLint y, GLint z, GLint w);
void ( APIENTRY *pglRasterPos4iv )(const GLint *v);
void ( APIENTRY *pglRasterPos4s )(GLshort x, GLshort y, GLshort z, GLshort w);
void ( APIENTRY *pglRasterPos4sv )(const GLshort *v);
void ( APIENTRY *pglReadBuffer )(GLenum mode);
void ( APIENTRY *pglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void ( APIENTRY *pglRectd )(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
void ( APIENTRY *pglRectdv )(const GLdouble *v1, const GLdouble *v2);
void ( APIENTRY *pglRectf )(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void ( APIENTRY *pglRectfv )(const GLfloat *v1, const GLfloat *v2);
void ( APIENTRY *pglRecti )(GLint x1, GLint y1, GLint x2, GLint y2);
void ( APIENTRY *pglRectiv )(const GLint *v1, const GLint *v2);
void ( APIENTRY *pglRects )(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
void ( APIENTRY *pglRectsv )(const GLshort *v1, const GLshort *v2);
void ( APIENTRY *pglRotated )(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY *pglRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY *pglScaled )(GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY *pglScalef )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY *pglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY *pglSelectBuffer )(GLsizei size, GLuint *buffer);
void ( APIENTRY *pglShadeModel )(GLenum mode);
void ( APIENTRY *pglStencilFunc )(GLenum func, GLint ref, GLuint mask);
void ( APIENTRY *pglStencilMask )(GLuint mask);
void ( APIENTRY *pglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
void ( APIENTRY *pglTexCoord1d )(GLdouble s);
void ( APIENTRY *pglTexCoord1dv )(const GLdouble *v);
void ( APIENTRY *pglTexCoord1f )(GLfloat s);
void ( APIENTRY *pglTexCoord1fv )(const GLfloat *v);
void ( APIENTRY *pglTexCoord1i )(GLint s);
void ( APIENTRY *pglTexCoord1iv )(const GLint *v);
void ( APIENTRY *pglTexCoord1s )(GLshort s);
void ( APIENTRY *pglTexCoord1sv )(const GLshort *v);
void ( APIENTRY *pglTexCoord2d )(GLdouble s, GLdouble t);
void ( APIENTRY *pglTexCoord2dv )(const GLdouble *v);
void ( APIENTRY *pglTexCoord2f )(GLfloat s, GLfloat t);
void ( APIENTRY *pglTexCoord2fv )(const GLfloat *v);
void ( APIENTRY *pglTexCoord2i )(GLint s, GLint t);
void ( APIENTRY *pglTexCoord2iv )(const GLint *v);
void ( APIENTRY *pglTexCoord2s )(GLshort s, GLshort t);
void ( APIENTRY *pglTexCoord2sv )(const GLshort *v);
void ( APIENTRY *pglTexCoord3d )(GLdouble s, GLdouble t, GLdouble r);
void ( APIENTRY *pglTexCoord3dv )(const GLdouble *v);
void ( APIENTRY *pglTexCoord3f )(GLfloat s, GLfloat t, GLfloat r);
void ( APIENTRY *pglTexCoord3fv )(const GLfloat *v);
void ( APIENTRY *pglTexCoord3i )(GLint s, GLint t, GLint r);
void ( APIENTRY *pglTexCoord3iv )(const GLint *v);
void ( APIENTRY *pglTexCoord3s )(GLshort s, GLshort t, GLshort r);
void ( APIENTRY *pglTexCoord3sv )(const GLshort *v);
void ( APIENTRY *pglTexCoord4d )(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void ( APIENTRY *pglTexCoord4dv )(const GLdouble *v);
void ( APIENTRY *pglTexCoord4f )(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void ( APIENTRY *pglTexCoord4fv )(const GLfloat *v);
void ( APIENTRY *pglTexCoord4i )(GLint s, GLint t, GLint r, GLint q);
void ( APIENTRY *pglTexCoord4iv )(const GLint *v);
void ( APIENTRY *pglTexCoord4s )(GLshort s, GLshort t, GLshort r, GLshort q);
void ( APIENTRY *pglTexCoord4sv )(const GLshort *v);
void ( APIENTRY *pglTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY *pglTexEnvf )(GLenum target, GLenum pname, GLfloat param);
void ( APIENTRY *pglTexEnvfv )(GLenum target, GLenum pname, const GLfloat *params);
void ( APIENTRY *pglTexEnvi )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY *pglTexEnviv )(GLenum target, GLenum pname, const GLint *params);
void ( APIENTRY *pglTexGend )(GLenum coord, GLenum pname, GLdouble param);
void ( APIENTRY *pglTexGendv )(GLenum coord, GLenum pname, const GLdouble *params);
void ( APIENTRY *pglTexGenf )(GLenum coord, GLenum pname, GLfloat param);
void ( APIENTRY *pglTexGenfv )(GLenum coord, GLenum pname, const GLfloat *params);
void ( APIENTRY *pglTexGeni )(GLenum coord, GLenum pname, GLint param);
void ( APIENTRY *pglTexGeniv )(GLenum coord, GLenum pname, const GLint *params);
void ( APIENTRY *pglTexImage1D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY *pglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY *pglTexParameterf )(GLenum target, GLenum pname, GLfloat param);
void ( APIENTRY *pglTexParameterfv )(GLenum target, GLenum pname, const GLfloat *params);
void ( APIENTRY *pglTexParameteri )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY *pglTexParameteriv )(GLenum target, GLenum pname, const GLint *params);
void ( APIENTRY *pglTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY *pglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY *pglTranslated )(GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY *pglTranslatef )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY *pglVertex2d )(GLdouble x, GLdouble y);
void ( APIENTRY *pglVertex2dv )(const GLdouble *v);
void ( APIENTRY *pglVertex2f )(GLfloat x, GLfloat y);
void ( APIENTRY *pglVertex2fv )(const GLfloat *v);
void ( APIENTRY *pglVertex2i )(GLint x, GLint y);
void ( APIENTRY *pglVertex2iv )(const GLint *v);
void ( APIENTRY *pglVertex2s )(GLshort x, GLshort y);
void ( APIENTRY *pglVertex2sv )(const GLshort *v);
void ( APIENTRY *pglVertex3d )(GLdouble x, GLdouble y, GLdouble z);
void ( APIENTRY *pglVertex3dv )(const GLdouble *v);
void ( APIENTRY *pglVertex3f )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY *pglVertex3fv )(const GLfloat *v);
void ( APIENTRY *pglVertex3i )(GLint x, GLint y, GLint z);
void ( APIENTRY *pglVertex3iv )(const GLint *v);
void ( APIENTRY *pglVertex3s )(GLshort x, GLshort y, GLshort z);
void ( APIENTRY *pglVertex3sv )(const GLshort *v);
void ( APIENTRY *pglVertex4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void ( APIENTRY *pglVertex4dv )(const GLdouble *v);
void ( APIENTRY *pglVertex4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void ( APIENTRY *pglVertex4fv )(const GLfloat *v);
void ( APIENTRY *pglVertex4i )(GLint x, GLint y, GLint z, GLint w);
void ( APIENTRY *pglVertex4iv )(const GLint *v);
void ( APIENTRY *pglVertex4s )(GLshort x, GLshort y, GLshort z, GLshort w);
void ( APIENTRY *pglVertex4sv )(const GLshort *v);
void ( APIENTRY *pglVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY *pglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY *pglPointParameterfEXT)( GLenum param, GLfloat value );
void ( APIENTRY *pglPointParameterfvEXT)( GLenum param, const GLfloat *value );
void ( APIENTRY *pglLockArraysEXT) (int , int);
void ( APIENTRY *pglUnlockArraysEXT) (void);
void ( APIENTRY *pglActiveTextureARB)( GLenum );
void ( APIENTRY *pglClientActiveTextureARB)( GLenum );
void ( APIENTRY *pglGetCompressedTexImage)( GLenum target, GLint lod, const GLvoid* data );
void ( APIENTRY *pglDrawRangeElements)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices );
void ( APIENTRY *pglDrawRangeElementsEXT)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices );
void ( APIENTRY *pglMultiTexCoord1f) (GLenum, GLfloat);
void ( APIENTRY *pglMultiTexCoord2f) (GLenum, GLfloat, GLfloat);
void ( APIENTRY *pglMultiTexCoord3f) (GLenum, GLfloat, GLfloat, GLfloat);
void ( APIENTRY *pglMultiTexCoord4f) (GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
void ( APIENTRY *pglActiveTexture) (GLenum);
void ( APIENTRY *pglClientActiveTexture) (GLenum);
void ( APIENTRY *pglCompressedTexImage3DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
void ( APIENTRY *pglCompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border,  GLsizei imageSize, const void *data);
void ( APIENTRY *pglCompressedTexImage1DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data);
void ( APIENTRY *pglCompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
void ( APIENTRY *pglCompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
void ( APIENTRY *pglCompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data);
void ( APIENTRY *pglDeleteObjectARB)(GLhandleARB obj);
GLhandleARB ( APIENTRY *pglGetHandleARB)(GLenum pname);
void ( APIENTRY *pglDetachObjectARB)(GLhandleARB containerObj, GLhandleARB attachedObj);
GLhandleARB ( APIENTRY *pglCreateShaderObjectARB)(GLenum shaderType);
void ( APIENTRY *pglShaderSourceARB)(GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length);
void ( APIENTRY *pglCompileShaderARB)(GLhandleARB shaderObj);
GLhandleARB ( APIENTRY *pglCreateProgramObjectARB)(void);
void ( APIENTRY *pglAttachObjectARB)(GLhandleARB containerObj, GLhandleARB obj);
void ( APIENTRY *pglLinkProgramARB)(GLhandleARB programObj);
void ( APIENTRY *pglUseProgramObjectARB)(GLhandleARB programObj);
void ( APIENTRY *pglValidateProgramARB)(GLhandleARB programObj);
void ( APIENTRY *pglBindProgramARB)(GLenum target, GLuint program);
void ( APIENTRY *pglDeleteProgramsARB)(GLsizei n, const GLuint *programs);
void ( APIENTRY *pglGenProgramsARB)(GLsizei n, GLuint *programs);
void ( APIENTRY *pglProgramStringARB)(GLenum target, GLenum format, GLsizei len, const GLvoid *string);
void ( APIENTRY *pglProgramEnvParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void ( APIENTRY *pglProgramLocalParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void ( APIENTRY *pglUniform1fARB)(GLint location, GLfloat v0);
void ( APIENTRY *pglUniform2fARB)(GLint location, GLfloat v0, GLfloat v1);
void ( APIENTRY *pglUniform3fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void ( APIENTRY *pglUniform4fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void ( APIENTRY *pglUniform1iARB)(GLint location, GLint v0);
void ( APIENTRY *pglUniform2iARB)(GLint location, GLint v0, GLint v1);
void ( APIENTRY *pglUniform3iARB)(GLint location, GLint v0, GLint v1, GLint v2);
void ( APIENTRY *pglUniform4iARB)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
void ( APIENTRY *pglUniform1fvARB)(GLint location, GLsizei count, const GLfloat *value);
void ( APIENTRY *pglUniform2fvARB)(GLint location, GLsizei count, const GLfloat *value);
void ( APIENTRY *pglUniform3fvARB)(GLint location, GLsizei count, const GLfloat *value);
void ( APIENTRY *pglUniform4fvARB)(GLint location, GLsizei count, const GLfloat *value);
void ( APIENTRY *pglUniform1ivARB)(GLint location, GLsizei count, const GLint *value);
void ( APIENTRY *pglUniform2ivARB)(GLint location, GLsizei count, const GLint *value);
void ( APIENTRY *pglUniform3ivARB)(GLint location, GLsizei count, const GLint *value);
void ( APIENTRY *pglUniform4ivARB)(GLint location, GLsizei count, const GLint *value);
void ( APIENTRY *pglUniformMatrix2fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void ( APIENTRY *pglUniformMatrix3fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void ( APIENTRY *pglUniformMatrix4fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void ( APIENTRY *pglGetObjectParameterfvARB)(GLhandleARB obj, GLenum pname, GLfloat *params);
void ( APIENTRY *pglGetObjectParameterivARB)(GLhandleARB obj, GLenum pname, GLint *params);
void ( APIENTRY *pglGetInfoLogARB)(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
void ( APIENTRY *pglGetAttachedObjectsARB)(GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
GLint ( APIENTRY *pglGetUniformLocationARB)(GLhandleARB programObj, const GLcharARB *name);
void ( APIENTRY *pglGetActiveUniformARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
void ( APIENTRY *pglGetUniformfvARB)(GLhandleARB programObj, GLint location, GLfloat *params);
void ( APIENTRY *pglGetUniformivARB)(GLhandleARB programObj, GLint location, GLint *params);
void ( APIENTRY *pglGetShaderSourceARB)(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);
void ( APIENTRY *pglTexImage3D)( GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
void ( APIENTRY *pglTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels );
void ( APIENTRY *pglCopyTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
void ( APIENTRY *pglBlendEquationEXT)(GLenum);
void ( APIENTRY *pglStencilOpSeparate)(GLenum, GLenum, GLenum, GLenum);
void ( APIENTRY *pglStencilFuncSeparate)(GLenum, GLenum, GLint, GLuint);
void ( APIENTRY *pglActiveStencilFaceEXT)(GLenum);
void ( APIENTRY *pglVertexAttribPointerARB)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY *pglEnableVertexAttribArrayARB)(GLuint index);
void ( APIENTRY *pglDisableVertexAttribArrayARB)(GLuint index);
void ( APIENTRY *pglBindAttribLocationARB)(GLhandleARB programObj, GLuint index, const GLcharARB *name);
void ( APIENTRY *pglGetActiveAttribARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
GLint ( APIENTRY *pglGetAttribLocationARB)(GLhandleARB programObj, const GLcharARB *name);
void ( APIENTRY *pglBindBufferARB) (GLenum target, GLuint buffer);
void ( APIENTRY *pglDeleteBuffersARB) (GLsizei n, const GLuint *buffers);
void ( APIENTRY *pglGenBuffersARB) (GLsizei n, GLuint *buffers);
GLboolean ( APIENTRY *pglIsBufferARB) (GLuint buffer);
GLvoid* ( APIENTRY *pglMapBufferARB) (GLenum target, GLenum access);
GLboolean ( APIENTRY *pglUnmapBufferARB) (GLenum target);
void ( APIENTRY *pglBufferDataARB) (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
void ( APIENTRY *pglBufferSubDataARB) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
void ( APIENTRY *pglGenQueriesARB) (GLsizei n, GLuint *ids);
void ( APIENTRY *pglDeleteQueriesARB) (GLsizei n, const GLuint *ids);
GLboolean ( APIENTRY *pglIsQueryARB) (GLuint id);
void ( APIENTRY *pglBeginQueryARB) (GLenum target, GLuint id);
void ( APIENTRY *pglEndQueryARB) (GLenum target);
void ( APIENTRY *pglGetQueryivARB) (GLenum target, GLenum pname, GLint *params);
void ( APIENTRY *pglGetQueryObjectivARB) (GLuint id, GLenum pname, GLint *params);
void ( APIENTRY *pglGetQueryObjectuivARB) (GLuint id, GLenum pname, GLuint *params);
void ( APIENTRY * pglSelectTextureSGIS) ( GLenum );
void ( APIENTRY * pglMTexCoord2fSGIS) ( GLenum, GLfloat, GLfloat );
void ( APIENTRY * pglSwapInterval) ( int interval );
void ( APIENTRY *pglDebugMessageControlARB)( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled );
void ( APIENTRY *pglDebugMessageInsertARB)( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* buf );
void ( APIENTRY *pglDebugMessageCallbackARB)( pglDebugProcARB callback, void* userParam );
GLuint ( APIENTRY *pglGetDebugMessageLogARB)( GLuint count, GLsizei bufsize, GLenum* sources, GLenum* types, GLuint* ids, GLuint* severities, GLsizei* lengths, char* messageLog );
#endif

/*
=================
GL_SetExtension
=================
*/
void GL_SetExtension( int r_ext, int enable )
{
	if( r_ext >= 0 && r_ext < GL_EXTCOUNT )
		glConfig.extension[r_ext] = enable ? GL_TRUE : GL_FALSE;
	else MsgDev( D_ERROR, "GL_SetExtension: invalid extension %d\n", r_ext );
}

/*
=================
GL_Support
=================
*/
qboolean GL_Support( int r_ext )
{
	if( r_ext >= 0 && r_ext < GL_EXTCOUNT )
		return glConfig.extension[r_ext] ? true : false;
	MsgDev( D_ERROR, "GL_Support: invalid extension %d\n", r_ext );

	return false;
}

/*
===============
GL_SetDefaultTexState
===============
*/
static void GL_SetDefaultTexState( void )
{
	int	i;

	Q_memset( glState.currentTextures, -1, MAX_TEXTURE_UNITS * sizeof( *glState.currentTextures ));
	Q_memset( glState.texCoordArrayMode, 0, MAX_TEXTURE_UNITS * sizeof( *glState.texCoordArrayMode ));
	Q_memset( glState.genSTEnabled, 0, MAX_TEXTURE_UNITS * sizeof( *glState.genSTEnabled ));

	for( i = 0; i < MAX_TEXTURE_UNITS; i++ )
	{
		glState.currentTextureTargets[i] = GL_NONE;
		glState.texIdentityMatrix[i] = true;
	}
}

/*
===============
GL_SetDefaultState
===============
*/
static void GL_SetDefaultState( void )
{
	Q_memset( &glState, 0, sizeof( glState ));
	GL_SetDefaultTexState ();
	glConfig.max_texture_units_cached = -1;
}


void R_SaveVideoMode( int w, int h )
{
	glState.width = w;
	glState.height = h;

	Cvar_FullSet( "width", va( "%i", w ), CVAR_READ_ONLY | CVAR_RENDERINFO );
	Cvar_FullSet( "height", va( "%i", h ), CVAR_READ_ONLY | CVAR_RENDERINFO );

	if( vid_mode->integer >= 0 && vid_mode->integer <= num_vidmodes )
		glState.wideScreen = vidmode[vid_mode->integer].wideScreen;

	MsgDev( D_NOTE, "Set: [%dx%d]\n", w, h );
}


qboolean R_DescribeVIDMode( int width, int height )
{
	int	i;

	for( i = 0; i < sizeof( vidmode ) / sizeof( vidmode[0] ); i++ )
	{
		if( vidmode[i].width == width && vidmode[i].height == height )
		{
			// found specified mode
			Cvar_SetFloat( "vid_mode", i );
			return true;
		}
	}
	Cvar_SetFloat("vid_mode", VID_NOMODE);
	return false;
}




/*
==================
VID_CheckChanges

check vid modes and fullscreen
==================
*/
void VID_CheckChanges( void )
{
	// check if screen cvars registered

	SCR_Init();

	if( cl_allow_levelshots->modified )
	{
		GL_FreeTexture( cls.loadingBar );
		SCR_RegisterTextures(); // reload 'lambda' image
		cl_allow_levelshots->modified = false;
	}

	if( renderinfo->modified )
	{
		if( !VID_SetMode())
		{
			// can't initialize video subsystem
			Sys_Warn( "Failed to initialise graphics.\nStarting dedicated server.\nAdd \"-dedicated\" to disable this message." );
			Host_NewInstance( va("#%s", GI->gamefolder ), "fallback to dedicated mode\n" );
		}
		else
		{
			renderinfo->modified = false;
			SCR_VidInit(); // tell the client.dll what vid_mode has changed
		}
	}
}



/*
===============
GL_SetDefaults
===============
*/
static void GL_SetDefaults( void )
{
	int	i;

	pglFinish();

	pglClearColor( 0.5f, 0.5f, 0.5f, 1.0f );

#ifdef GL_MULTISAMPLE
	if( gl_msaa->integer )
		pglEnable( GL_MULTISAMPLE );
#endif

	pglDisable( GL_DEPTH_TEST );
	pglDisable( GL_CULL_FACE );
	pglDisable( GL_SCISSOR_TEST );
	pglDepthFunc( GL_LEQUAL );
	pglDepthMask( GL_FALSE );

	pglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	if( glState.stencilEnabled )
	{
		pglDisable( GL_STENCIL_TEST );
		pglStencilMask( ( GLuint ) ~0 );
		pglStencilFunc( GL_EQUAL, 0, ~0 );
		pglStencilOp( GL_KEEP, GL_INCR, GL_INCR );
	}

	pglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	pglPolygonOffset( -1.0f, -2.0f );

	// properly disable multitexturing at startup
	for( i = (MAX_TEXTURE_UNITS - 1); i > 0; i-- )
	{
		if( i >= GL_MaxTextureUnits( ))
			continue;

		GL_SelectTexture( i );
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		pglDisable( GL_BLEND );
		pglDisable( GL_TEXTURE_2D );
	}

	GL_SelectTexture( 0 );
	pglDisable( GL_BLEND );
	pglDisable( GL_ALPHA_TEST );
	pglDisable( GL_POLYGON_OFFSET_FILL );
	pglAlphaFunc( GL_GREATER, 0.0f );
	pglEnable( GL_TEXTURE_2D );
	pglShadeModel( GL_FLAT );

	pglPointSize( 1.2f );
	pglLineWidth( 1.2f );

	GL_Cull( 0 );
	GL_FrontFace( 0 );

	R_SetTextureParameters();

	pglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	pglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	pglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	pglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
}

/*
=================
R_RenderInfo_f
=================
*/
void R_RenderInfo_f( void )
{
	Msg( "\n" );
	Msg( "GL_VENDOR: %s\n", glConfig.vendor_string );
	Msg( "GL_RENDERER: %s\n", glConfig.renderer_string );
	Msg( "GL_VERSION: %s\n", glConfig.version_string );

	// don't spam about extensions
	if( host.developer >= 4 )
		Msg( "GL_EXTENSIONS: %s\n", glConfig.extensions_string );

	Msg( "GL_MAX_TEXTURE_SIZE: %i\n", glConfig.max_2d_texture_size );

	if( GL_Support( GL_ARB_MULTITEXTURE ))
		Msg( "GL_MAX_TEXTURE_UNITS_ARB: %i\n", glConfig.max_texture_units );
	if( GL_Support( GL_TEXTURECUBEMAP_EXT ))
		Msg( "GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB: %i\n", glConfig.max_cubemap_size );
	if( GL_Support( GL_ANISOTROPY_EXT ))
		Msg( "GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT: %.1f\n", glConfig.max_texture_anisotropy );
	if( GL_Support( GL_TEXTURE_LODBIAS ))
		Msg( "GL_MAX_TEXTURE_LODBIAS: %f\n", glConfig.max_texture_lodbias );
	if( glConfig.texRectangle )
		Msg( "GL_MAX_RECTANGLE_TEXTURE_SIZE_NV: %i\n", glConfig.max_2d_rectangle_size );
	if( GL_Support( GL_SHADER_GLSL100_EXT ))
	{
		Msg( "GL_MAX_TEXTURE_COORDS_ARB: %i\n", glConfig.max_texture_coords );
		Msg( "GL_MAX_TEXTURE_IMAGE_UNITS_ARB: %i\n", glConfig.max_teximage_units );
		Msg( "GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB: %i\n", glConfig.max_vertex_uniforms );
		Msg( "GL_MAX_VERTEX_ATTRIBS_ARB: %i\n", glConfig.max_vertex_attribs );
	}

	Msg( "\n" );
	Msg( "MODE: %i x %i\n", scr_width->integer, scr_height->integer );
	Msg( "GAMMA: %s\n", "software" );
	Msg( "\n" );
	Msg( "PICMIP: %i\n", gl_picmip->integer );
	Msg( "SKYMIP: %i\n", gl_skymip->integer );
	Msg( "TEXTUREMODE: %s\n", gl_texturemode->string );
	Msg( "VERTICAL SYNC: %s\n", gl_swapInterval->integer ? "enabled" : "disabled" );
	Msg( "Color %d bits, Alpha %d bits, Depth %d bits, Stencil %d bits\n", glConfig.color_bits,
		glConfig.alpha_bits, glConfig.depth_bits, glConfig.stencil_bits );
	Msg( "MSAA samples: %d\n", glConfig.msaasamples );
}

//=======================================================================

void GL_InitCommands( void )
{
	Cbuf_AddText( "vidlatch\n" );
	Cbuf_Execute();

	// system screen width and height (don't suppose for change from console at all)
	renderinfo = Cvar_Get( "@renderinfo", "0", CVAR_READ_ONLY, "" ); // use ->modified value only
	r_speeds = Cvar_Get( "r_speeds", "0", CVAR_ARCHIVE, "shows renderer speeds" );
	r_fullbright = Cvar_Get( "r_fullbright", "0", CVAR_CHEAT, "disable lightmaps, get fullbright for entities" );
	r_norefresh = Cvar_Get( "r_norefresh", "0", 0, "disable 3D rendering (use with caution)" );
	r_lighting_extended = Cvar_Get( "r_lighting_extended", "1", CVAR_ARCHIVE, "allow to get lighting from world and bmodels" );
	r_lighting_modulate = Cvar_Get( "r_lighting_modulate", "0.6", CVAR_ARCHIVE, "lightstyles modulate scale" );
	r_lighting_ambient = Cvar_Get( "r_lighting_ambient", "0.3", CVAR_ARCHIVE, "map ambient lighting scale" );
	r_adjust_fov = Cvar_Get( "r_adjust_fov", "1", CVAR_ARCHIVE, "making FOV adjustment for wide-screens" );
	r_novis = Cvar_Get( "r_novis", "0", 0, "ignore vis information (perfomance test)" );
	r_nocull = Cvar_Get( "r_nocull", "0", 0, "ignore frustrum culling (perfomance test)" );
	r_faceplanecull = Cvar_Get( "r_faceplanecull", "1", 0, "ignore face plane culling (perfomance test)" );
	r_detailtextures = Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE, "enable detail textures support, use \"2\" for auto-generate mapname_detail.txt" );
	r_lockpvs = Cvar_Get( "r_lockpvs", "0", CVAR_CHEAT, "lockpvs area at current point (pvs test)" );
	r_lockcull = Cvar_Get( "r_lockcull", "0", CVAR_CHEAT, "lock frustrum area at current point (cull test)" );
	r_dynamic = Cvar_Get( "r_dynamic", "1", CVAR_ARCHIVE, "allow dynamic lighting (dlights, lightstyles)" );
	r_lightmap = Cvar_Get( "r_lightmap", "0", CVAR_CHEAT, "lightmap debugging tool" );
	r_fastsky = Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE, "enable algorhytm fo fast sky rendering (for old machines)" );
	r_drawentities = Cvar_Get( "r_drawentities", "1", CVAR_CHEAT|CVAR_ARCHIVE, "render entities" );
	r_flaresize = Cvar_Get( "r_flaresize", "200", CVAR_ARCHIVE, "set flares size" );
	r_lefthand = Cvar_Get( "hand", "0", CVAR_ARCHIVE, "viewmodel handedness" );
	r_decals = Cvar_Get( "r_decals", "4096", 0, "sets the maximum number of decals" );
	r_xpos = Cvar_Get( "r_xpos", "130", CVAR_GLCONFIG, "window position by horizontal" );
	r_ypos = Cvar_Get( "r_ypos", "48", CVAR_GLCONFIG, "window position by vertical" );
	r_underwater_distortion = Cvar_Get( "r_underwater_distortion", "0.4", CVAR_ARCHIVE, "underwater distortion speed" );
	mp_decals = Cvar_Get( "mp_decals", "300", CVAR_ARCHIVE, "sets the maximum number of decals in multiplayer" );

	gl_picmip = Cvar_Get( "gl_picmip", "0", CVAR_GLCONFIG, "reduces resolution of textures by powers of 2" );
	gl_skymip = Cvar_Get( "gl_skymip", "0", CVAR_GLCONFIG, "reduces resolution of skybox textures by powers of 2" );
	gl_allow_software = Cvar_Get( "gl_allow_software", "0", CVAR_ARCHIVE, "allow OpenGL software emulation" );
	gl_alphabits = Cvar_Get( "gl_alphabits", "8", CVAR_GLCONFIG, "pixelformat alpha bits (0 - auto)" );
	gl_texturemode = Cvar_Get( "gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE, "texture filter" );
	gl_max_size = Cvar_Get( "gl_max_size", "512", CVAR_ARCHIVE, "no effect in Xash3D just a legacy" );
	gl_stencilbits = Cvar_Get( "gl_stencilbits", "8", CVAR_GLCONFIG, "pixelformat stencil bits (0 - auto)" );
	gl_check_errors = Cvar_Get( "gl_check_errors", "1", CVAR_ARCHIVE, "ignore video engine errors" );
	gl_swapInterval = Cvar_Get( "gl_swapInterval", "0", CVAR_ARCHIVE,  "time beetween frames (in msec)" );
	gl_extensions = Cvar_Get( "gl_extensions", "1", CVAR_GLCONFIG, "allow gl_extensions" );
	gl_detailscale = Cvar_Get( "gl_detailscale", "4.0", CVAR_ARCHIVE, "default scale applies while auto-generate list of detail textures" );
	gl_texture_anisotropy = Cvar_Get( "gl_anisotropy", "2.0", CVAR_ARCHIVE, "textures anisotropic filter" );
	gl_texture_lodbias =  Cvar_Get( "gl_texture_lodbias", "0.0", CVAR_ARCHIVE, "LOD bias for mipmapped textures" );
	gl_compress_textures = Cvar_Get( "gl_compress_textures", "0", CVAR_GLCONFIG, "compress textures to safe video memory" );
	gl_luminance_textures = Cvar_Get( "gl_luminance_textures", "0", CVAR_GLCONFIG, "force all textures to luminance" );
	gl_msaa = Cvar_Get( "gl_msaa", "0", CVAR_GLCONFIG, "MSAA samples. Use with caution, engine may fail with some values" );
	gl_compensate_gamma_screenshots = Cvar_Get( "gl_compensate_gamma_screenshots", "0", CVAR_ARCHIVE, "allow to apply gamma value for screenshots and snapshots" );
	gl_keeptjunctions = Cvar_Get( "gl_keeptjunctions", "1", CVAR_ARCHIVE, "disable to reduce vertexes count but removing tjuncs causes blinking pixels" );
	gl_allow_static = Cvar_Get( "gl_allow_static", "0", CVAR_ARCHIVE, "force to drawing non-moveable brushes as part of world (save FPS)" );
	gl_allow_mirrors = Cvar_Get( "gl_allow_mirrors", "1", CVAR_ARCHIVE, "allow to draw mirror surfaces(deprecated, may be removed from future builds)" );
	gl_showtextures = Cvar_Get( "r_showtextures", "0", CVAR_CHEAT, "show all uploaded textures (type values from 1 to 13)" );
	gl_finish = Cvar_Get( "gl_finish", "0", CVAR_ARCHIVE, "use glFinish instead of glFlush" );
	gl_nosort = Cvar_Get( "gl_nosort", "0", CVAR_ARCHIVE, "disable sorting of translucent surfaces" );
	gl_clear = Cvar_Get( "gl_clear", "0", CVAR_ARCHIVE, "clearing screen after each frame" );
	gl_test = Cvar_Get( "gl_test", "0", 0, "engine developer cvar for quick testing new features" );
	gl_wireframe = Cvar_Get( "gl_wireframe", "0", 0, "show wireframe overlay" );
	gl_overview = Cvar_Get( "dev_overview", "0", 0, "show level overview" );
	gl_overbright = Cvar_Get( "gl_overbright", "0", CVAR_ARCHIVE, "Overbright mode (0-2)");
	gl_overbright_studio = Cvar_Get( "gl_overbright_studio", "0", CVAR_ARCHIVE, "Overbright for studiomodels");

	// these cvar not used by engine but some mods requires this
	Cvar_Get( "gl_polyoffset", "-0.1", 0, "polygon offset for decals" );

	// make sure r_swapinterval is checked after vid_restart
	gl_swapInterval->modified = true;

	vid_gamma = Cvar_Get( "gamma", "1.0", CVAR_ARCHIVE, "gamma amount" );
	vid_texgamma = Cvar_Get( "texgamma", "2.2", 0, "texgamma amount (default Half-Life artwork gamma)" );
	vid_mode = Cvar_Get( "vid_mode", VID_AUTOMODE, CVAR_RENDERINFO, "display resolution mode" );
	vid_fullscreen = Cvar_Get( "fullscreen", "0", CVAR_RENDERINFO, "set in 1 to enable fullscreen mode" );
	vid_displayfrequency = Cvar_Get ( "vid_displayfrequency", "0", CVAR_RENDERINFO, "fullscreen refresh rate" );
	vid_highdpi = Cvar_Get( "vid_highdpi", "1", CVAR_RENDERINFO, "Enable High-DPI mode" );

	Cmd_AddCommand( "r_info", R_RenderInfo_f, "display renderer info" );
	Cmd_AddCommand( "texturelist", R_TextureList_f, "display loaded textures list" );
}

void GL_RemoveCommands( void )
{
	Cmd_RemoveCommand( "r_info");
	Cmd_RemoveCommand( "texturelist" );
}

#ifdef WIN32
typedef enum _XASH_DPI_AWARENESS
{
	XASH_DPI_UNAWARE = 0,
	XASH_SYSTEM_DPI_AWARE = 1,
	XASH_PER_MONITOR_DPI_AWARE = 2
} XASH_DPI_AWARENESS;

void Win_SetDPIAwareness( void )
{
	HMODULE hModule;
	HRESULT ( __stdcall *pSetProcessDpiAwareness )( XASH_DPI_AWARENESS );
	BOOL ( __stdcall *pSetProcessDPIAware )( void );
	BOOL bSuccess = FALSE;

	if( ( hModule = LoadLibrary( "shcore.dll" ) ) )
	{
		if( ( pSetProcessDpiAwareness = (void*)GetProcAddress( hModule, "SetProcessDpiAwareness" ) ) )
		{
			// I hope SDL don't handle WM_DPICHANGED message
			HRESULT hResult = pSetProcessDpiAwareness( XASH_SYSTEM_DPI_AWARE );

			if( hResult == S_OK )
			{
				MsgDev( D_NOTE, "SetDPIAwareness: Success\n" );
				bSuccess = TRUE;
			}
			else if( hResult == E_INVALIDARG ) MsgDev( D_NOTE, "SetDPIAwareness: Invalid argument\n" );
			else if( hResult == E_ACCESSDENIED ) MsgDev( D_NOTE, "SetDPIAwareness: Access Denied\n" );
		}
		else MsgDev( D_NOTE, "SetDPIAwareness: Can't get SetProcessDpiAwareness\n" );
		FreeLibrary( hModule );
	}
	else MsgDev( D_NOTE, "SetDPIAwareness: Can't load shcore.dll\n" );


	if( !bSuccess )
	{
		MsgDev( D_NOTE, "SetDPIAwareness: Trying SetProcessDPIAware...\n" );

		if( ( hModule = LoadLibrary( "user32.dll" ) ) )
		{
			if( ( pSetProcessDPIAware = ( void* )GetProcAddress( hModule, "SetProcessDPIAware" ) ) )
			{
				// I hope SDL don't handle WM_DPICHANGED message
				BOOL hResult = pSetProcessDPIAware();

				if( hResult )
				{
					MsgDev( D_NOTE, "SetDPIAwareness: Success\n" );
					bSuccess = TRUE;
				}
				else MsgDev( D_NOTE, "SetDPIAwareness: fail\n" );
			}
			else MsgDev( D_NOTE, "SetDPIAwareness: Can't get SetProcessDPIAware\n" );
			FreeLibrary( hModule );
		}
		else MsgDev( D_NOTE, "SetDPIAwareness: Can't load user32.dll\n" );
	}
}
#endif

/*
===============
R_CheckVBO

register VBO cvars and get default value
===============
*/
static void R_CheckVBO( void )
{
	const char *def = "1";
	const char *dlightmode = "1";
	int flags = CVAR_ARCHIVE;
	qboolean disable = false;

	// some bad GLES1 implementations breaks dlights completely
	if( glConfig.max_texture_units < 3 )
		disable = true;

#ifdef XASH_MOBILE_PLATFORM
	// VideoCore4 drivers have a problem with mixing VBO and client arrays
	// Disable it, as there is no suitable workaround here
	if( Q_stristr( glConfig.renderer_string, "VideoCore IV" ) || Q_stristr( glConfig.renderer_string, "vc4" ) )
		disable = true;

	// dlightmode 1 is not too much tested on android
	// so better to left it off
	dlightmode = "0";
#endif

	if( disable )
	{
		// do not keep in config unless dev > 3 and enabled
		flags = 0;
		def = "0";
	}

	r_vbo = Cvar_Get( "r_vbo", def, flags, "draw world using VBO" );
	r_bump = Cvar_Get( "r_bump", def, CVAR_ARCHIVE, "enable bump-mapping (r_vbo required)" );
	r_vbo_dlightmode = Cvar_Get( "r_vbo_dlightmode", dlightmode, CVAR_ARCHIVE, "vbo dlight rendering mode(0-1)" );

	// check if enabled manually
	if( r_vbo->integer && host.developer > 3 )
		r_vbo->flags |= CVAR_ARCHIVE;
}

static int GetCommandLineIntegerValue(const char* argName)
{
	int argIndex = Sys_CheckParm(argName);

	if ( argIndex < 1 || argIndex + 1 >= host.argc || !host.argv[argIndex + 1] )
	{
		return 0;
	}

	return Q_atoi(host.argv[argIndex + 1]);
}

static void SetWidthAndHeightFromCommandLine()
{
	int width = GetCommandLineIntegerValue("-width");
	int height = GetCommandLineIntegerValue("-height");

	if ( width < 1 || height < 1 )
	{
		// Not specified or invalid, so don't bother.
		return;
	}

	Cvar_SetFloat("vid_mode", VID_NOMODE);
	Cvar_SetFloat("width", width);
	Cvar_SetFloat("height", height);
}

static void SetFullscreenModeFromCommandLine()
{
#ifndef XASH_MOBILE_PLATFORM
	if ( Sys_CheckParm("-fullscreen") )
	{
		Cvar_Set2("fullscreen", "1", true);
	}
	else if ( Sys_CheckParm("-windowed") )
	{
		Cvar_Set2("fullscreen", "0", true);
	}
#endif
}

/*
===============
R_Init
===============
*/
qboolean R_Init( void )
{
	if( glw_state.initialized )
		return true;

	// give initial OpenGL configuration
	Cbuf_AddText( "exec opengl.cfg\n" );

	GL_InitCommands();
	
	// Set screen resolution and fullscreen mode if passed in on command line.
	// This is done after executing opengl.cfg, as the command line values should take priority.
	SetWidthAndHeightFromCommandLine();
	SetFullscreenModeFromCommandLine();

	GL_SetDefaultState();

#ifdef WIN32
	Win_SetDPIAwareness( );
#endif

	// create the window and set up the context
	if( !R_Init_OpenGL( ))
	{
		GL_RemoveCommands();
		R_Free_OpenGL();

		// can't initialize video subsystem
		Host_NewInstance( va("#%s", GI->gamefolder ), "fallback to dedicated mode\n" );
		return false;
	}

	renderinfo->modified = false;
	r_temppool = Mem_AllocPool( "Render Zone" );

	GL_InitExtensions();
	GL_SetDefaults();
	R_CheckVBO();
	R_InitImages();
	R_SpriteInit();
	R_StudioInit();

	R_Strobe_Init();

	R_ClearDecals();
	R_ClearScene();

	// initialize screen
	SCR_Init();

	return true;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown( void )
{
	int	i;

	if( !glw_state.initialized )
		return;

	// release SpriteTextures
	for( i = 1; i < MAX_IMAGES; i++ )
	{
		if( !clgame.sprites[i].name[0] ) continue;
		Mod_UnloadSpriteModel( &clgame.sprites[i] );
	}
	Q_memset( clgame.sprites, 0, sizeof( clgame.sprites ));

	GL_RemoveCommands();
	R_ShutdownImages();

	Mem_FreePool( &r_temppool );

	// shut down OS specific OpenGL stuff like contexts, etc.
	R_Free_OpenGL();
}

/*
=================
GL_CheckForErrors
=================
*/
void GL_CheckForErrors_( const char *filename, const int fileline )
{
	int	err;
	char	*str;

	if( !gl_check_errors->integer )
		return;

	if(( err = pglGetError( )) == GL_NO_ERROR )
		return;

	switch( err )
	{
	case GL_STACK_OVERFLOW:
		str = "GL_STACK_OVERFLOW";
		break;
	case GL_STACK_UNDERFLOW:
		str = "GL_STACK_UNDERFLOW";
		break;
	case GL_INVALID_ENUM:
		str = "GL_INVALID_ENUM";
		break;
	case GL_INVALID_VALUE:
		str = "GL_INVALID_VALUE";
		break;
	case GL_INVALID_OPERATION:
		str = "GL_INVALID_OPERATION";
		break;
	case GL_OUT_OF_MEMORY:
		str = "GL_OUT_OF_MEMORY";
		break;
	default:
		str = "UNKNOWN ERROR";
		break;
	}

	Host_Error( "GL_CheckForErrors: %s (called at %s:%i)\n", str, filename, fileline );
}
#endif // XASH_DEDICATED
