/*
gl_vidnt.c - NT GL vid component
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

#include "common.h"
#include "client.h"
#include "gl_local.h"
#include "mod_local.h"
#include "input.h"

#if (defined(_WIN32) && defined(XASH_SDL))
#include <SDL_syswm.h>
#endif

#ifdef __ANDROID__
#include <GL/nanogl.h>
#endif

#ifdef XASH_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif


#define VID_NOMODE -2.0f
#define VID_AUTOMODE	"-1"
#define VID_DEFAULTMODE	2.0f
#define DISP_CHANGE_BADDUALVIEW	-6 // MSVC 6.0 doesn't
#define num_vidmodes	( sizeof( vidmode ) / sizeof( vidmode[0] ))
#define WINDOW_NAME			"Xash Window" // Half-Life

convar_t	*renderinfo;
convar_t	*gl_allow_software;
convar_t	*gl_extensions;
convar_t	*gl_alphabits;
convar_t	*gl_stencilbits;
convar_t	*gl_ignorehwgamma;
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
convar_t	*gl_round_down;
convar_t	*gl_overview;
convar_t	*gl_max_size;
convar_t	*gl_picmip;
convar_t	*gl_skymip;
convar_t	*gl_finish;
convar_t	*gl_nosort;
convar_t	*gl_clear;
convar_t	*gl_test;

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

convar_t	*vid_displayfrequency;
convar_t	*vid_fullscreen;
convar_t	*vid_gamma;
convar_t	*vid_texgamma;
convar_t	*vid_mode;

byte		*r_temppool;

ref_globals_t	tr;
glconfig_t	glConfig;
glstate_t		glState;
glwstate_t	glw_state;
uint		num_instances;


typedef enum
{
	rserr_ok,
	rserr_invalid_fullscreen,
	rserr_invalid_mode,
	rserr_unknown
} rserr_t;

typedef struct vidmode_s
{
	const char	*desc;
	int		width; 
	int		height;
	qboolean		wideScreen;
} vidmode_t;

vidmode_t vidmode[] =
{
{ "Mode  0: 4x3",	640,	480,	false	},
{ "Mode  1: 4x3",	800,	600,	false	},
{ "Mode  2: 4x3",	960,	720,	false	},
{ "Mode  3: 4x3",	1024,	768,	false	},
{ "Mode  4: 4x3",	1152,	864,	false	},
{ "Mode  5: 4x3",	1280,	800,	false	},
{ "Mode  6: 4x3",	1280,	960,	false	},
{ "Mode  7: 4x3",	1280,	1024,	false	},
{ "Mode  8: 4x3",	1600,	1200,	false	},
{ "Mode  9: 4x3",	2048,	1536,	false	},
{ "Mode 10: 16x9",	800,	480,	true	},
{ "Mode 11: 16x9",	856,	480,	true	},
{ "Mode 12: 16x9",	960,	540,	true	},
{ "Mode 13: 16x9",	1024,	576,	true	},
{ "Mode 14: 16x9",	1024,	600,	true	},
{ "Mode 15: 16x9",	1280,	720,	true	},
{ "Mode 16: 16x9",	1360,	768,	true	},
{ "Mode 17: 16x9",	1366,	768,	true	},
{ "Mode 18: 16x9",	1440,	900,	true	},
{ "Mode 19: 16x9",	1680,	1050,	true	},
{ "Mode 20: 16x9",	1920,	1080,	true	},
{ "Mode 21: 16x9",	1920,	1200,	true	},
{ "Mode 22: 16x9",	2560,	1600,	true	},
{ "Mode 23: 16x9",	1600,	900,	true	},
};

#ifdef __ANDROID__

static char* opengl_110funcs[] =
{
 "glClearColor"         ,
 "glClear"              ,
 "glAlphaFunc"          ,
 "glBlendFunc"          ,
 "glCullFace"           ,
 "glDrawBuffer"         ,
 "glReadBuffer"         ,
 "glEnable"             ,
 "glDisable"            ,
 "glEnableClientState"  ,
 "glDisableClientState" ,
 "glGetBooleanv"        ,
 "glGetDoublev"         ,
 "glGetFloatv"          ,
 "glGetIntegerv"        ,
 "glGetError"           ,
 "glGetString"          ,
 "glFinish"             ,
 "glFlush"              ,
 "glClearDepth"         ,
 "glDepthFunc"          ,
 "glDepthMask"          ,
 "glDepthRange"         ,
 "glFrontFace"          ,
 "glDrawElements"       ,
 "glColorMask"          ,
 "glIndexPointer"       ,
 "glVertexPointer"      ,
 "glNormalPointer"      ,
 "glColorPointer"       ,
 "glTexCoordPointer"    ,
 "glArrayElement"       ,
 "glColor3f"            ,
 "glColor3fv"           ,
 "glColor4f"            ,
 "glColor4fv"           ,
 "glColor3ub"           ,
 "glColor4ub"           ,
 "glColor4ubv"          ,
 "glTexCoord1f"         ,
 "glTexCoord2f"         ,
 "glTexCoord3f"         ,
 "glTexCoord4f"         ,
 "glTexGenf"            ,
 "glTexGenfv"           ,
 "glTexGeni"            ,
 "glVertex2f"           ,
 "glVertex3f"           ,
 "glVertex3fv"          ,
 "glNormal3f"           ,
 "glNormal3fv"          ,
 "glBegin"              ,
 "glEnd"                ,
 "glLineWidth"          ,
 "glPointSize"          ,
 "glMatrixMode"         ,
 "glOrtho"              ,
 "glRasterPos2f"        ,
 "glFrustum"            ,
 "glViewport"           ,
 "glPushMatrix"         ,
 "glPopMatrix"          ,
 "glPushAttrib"         ,
 "glPopAttrib"          ,
 "glLoadIdentity"       ,
 "glLoadMatrixd"        ,
 "glLoadMatrixf"        ,
 "glMultMatrixd"        ,
 "glMultMatrixf"        ,
 "glRotated"            ,
 "glRotatef"            ,
 "glScaled"             ,
 "glScalef"             ,
 "glTranslated"         ,
 "glTranslatef"         ,
 "glReadPixels"         ,
 "glDrawPixels"         ,
 "glStencilFunc"        ,
 "glStencilMask"        ,
 "glStencilOp"          ,
 "glClearStencil"       ,
 "glIsEnabled"          ,
 "glIsList"             ,
 "glIsTexture"          ,
 "glTexEnvf"            ,
 "glTexEnvfv"           ,
 "glTexEnvi"            ,
 "glTexParameterf"      ,
 "glTexParameterfv"     ,
 "glTexParameteri"      ,
 "glHint"               ,
 "glPixelStoref"        ,
 "glPixelStorei"        ,
 "glGenTextures"        ,
 "glDeleteTextures"     ,
 "glBindTexture"        ,
 "glTexImage1D"         ,
 "glTexImage2D"         ,
 "glTexSubImage1D"      ,
 "glTexSubImage2D"      ,
 "glCopyTexImage1D"     ,
 "glCopyTexImage2D"     ,
 "glCopyTexSubImage1D"  ,
 "glCopyTexSubImage2D"  ,
 "glScissor"            ,
 "glGetTexEnviv"        ,
 "glPolygonOffset"      ,
 "glPolygonMode"        ,
 "glPolygonStipple"     ,
 "glClipPlane"          ,
 "glGetClipPlane"       ,
 "glShadeModel"         ,
 "glFogfv"              ,
 "glFogf"               ,
 "glFogi"               ,
 NULL
};

static char* pointparametersfunc[] =
{
"glPointParameterfEXT"  ,
"glPointParameterfvEXT" ,
NULL
};

static char* drawrangeelementsfuncs[] =
{
 "glDrawRangeElements" ,
 NULL
};

static char* drawrangeelementsextfuncs[] =
{
 "glDrawRangeElementsEXT" ,
 NULL
};

static char* sgis_multitexturefuncs[] =
{
 "glSelectTextureSGIS" ,
 "glMTexCoord2fSGIS"   ,
 NULL
};

static char* multitexturefuncs[] =
{
 "glMultiTexCoord1fARB"     ,
 "glMultiTexCoord2fARB"     ,
 "glMultiTexCoord3fARB"     ,
 "glMultiTexCoord4fARB"     ,
 "glActiveTextureARB"       ,
 "glClientActiveTextureARB" ,
 "glClientActiveTextureARB" ,
 NULL
};

static char* compiledvertexarrayfuncs[] =
{
 "glLockArraysEXT"   ,
 "glUnlockArraysEXT" ,
 "glDrawArrays"      ,
 NULL
};

static char* texture3dextfuncs[] =
{
 "glTexImage3DEXT"        ,
 "glTexSubImage3DEXT"     ,
 "glCopyTexSubImage3DEXT" ,
 NULL
};

static char* atiseparatestencilfuncs[] =
{
 "glStencilOpSeparateATI"   ,
 "glStencilFuncSeparateATI" ,
 NULL
};

static char* gl2separatestencilfuncs[] =
{
 "glStencilOpSeparate"   ,
 "glStencilFuncSeparate" ,
 NULL
};

static char* stenciltwosidefuncs[] =
{
 "glActiveStencilFaceEXT" ,
 NULL
};

static char* blendequationfuncs[] =
{
 "glBlendEquationEXT" ,
 NULL
};

static char* shaderobjectsfuncs[] =
{
 "glDeleteObjectARB"             ,
 "glGetHandleARB"                ,
 "glDetachObjectARB"             ,
 "glCreateShaderObjectARB"       ,
 "glShaderSourceARB"             ,
 "glCompileShaderARB"            ,
 "glCreateProgramObjectARB"      ,
 "glAttachObjectARB"             ,
 "glLinkProgramARB"              ,
 "glUseProgramObjectARB"         ,
 "glValidateProgramARB"          ,
 "glUniform1fARB"                ,
 "glUniform2fARB"                ,
 "glUniform3fARB"                ,
 "glUniform4fARB"                ,
 "glUniform1iARB"                ,
 "glUniform2iARB"                ,
 "glUniform3iARB"                ,
 "glUniform4iARB"                ,
 "glUniform1fvARB"               ,
 "glUniform2fvARB"               ,
 "glUniform3fvARB"               ,
 "glUniform4fvARB"               ,
 "glUniform1ivARB"               ,
 "glUniform2ivARB"               ,
 "glUniform3ivARB"               ,
 "glUniform4ivARB"               ,
 "glUniformMatrix2fvARB"         ,
 "glUniformMatrix3fvARB"         ,
 "glUniformMatrix4fvARB"         ,
 "glGetObjectParameterfvARB"     ,
 "glGetObjectParameterivARB"     ,
 "glGetInfoLogARB"               ,
 "glGetAttachedObjectsARB"       ,
 "glGetUniformLocationARB"       ,
 "glGetActiveUniformARB"         ,
 "glGetUniformfvARB"             ,
 "glGetUniformivARB"             ,
 "glGetShaderSourceARB"          ,
 "glVertexAttribPointerARB"      ,
 "glEnableVertexAttribArrayARB"  ,
 "glDisableVertexAttribArrayARB" ,
 "glBindAttribLocationARB"       ,
 "glGetActiveAttribARB"          ,
 "glGetAttribLocationARB"        ,
 NULL
};

static char* vertexshaderfuncs[] =
{
 "glVertexAttribPointerARB"      ,
 "glEnableVertexAttribArrayARB"  ,
 "glDisableVertexAttribArrayARB" ,
 "glBindAttribLocationARB"       ,
 "glGetActiveAttribARB"          ,
 "glGetAttribLocationARB"        ,
 NULL
};

static char* vbofuncs[] =
{
 "glBindBufferARB"    ,
 "glDeleteBuffersARB" ,
 "glGenBuffersARB"    ,
 "glIsBufferARB"      ,
 "glMapBufferARB"     ,
 "glUnmapBufferARB"   ,
 "glBufferDataARB"    ,
 "glBufferSubDataARB" ,
 NULL
};

static char* occlusionfunc[] =
{
 "glGenQueriesARB"        ,
 "glDeleteQueriesARB"     ,
 "glIsQueryARB"           ,
 "glBeginQueryARB"        ,
 "glEndQueryARB"          ,
 "glGetQueryivARB"        ,
 "glGetQueryObjectivARB"  ,
 "glGetQueryObjectuivARB" ,
 NULL
};

static char* texturecompressionfuncs[] =
{
 "glCompressedTexImage3DARB"    ,
 "glCompressedTexImage2DARB"    ,
 "glCompressedTexImage1DARB"    ,
 "glCompressedTexSubImage3DARB" ,
 "glCompressedTexSubImage2DARB" ,
 "glCompressedTexSubImage1DARB" ,
 "glGetCompressedTexImageARB"   ,
 NULL
};
#else

static dllfunc_t opengl_110funcs[] =
{
{ "glClearColor"         , (void **)&pglClearColor },
{ "glClear"              , (void **)&pglClear },
{ "glAlphaFunc"          , (void **)&pglAlphaFunc },
{ "glBlendFunc"          , (void **)&pglBlendFunc },
{ "glCullFace"           , (void **)&pglCullFace },
{ "glDrawBuffer"         , (void **)&pglDrawBuffer },
{ "glReadBuffer"         , (void **)&pglReadBuffer },
{ "glEnable"             , (void **)&pglEnable },
{ "glDisable"            , (void **)&pglDisable },
{ "glEnableClientState"  , (void **)&pglEnableClientState },
{ "glDisableClientState" , (void **)&pglDisableClientState },
{ "glGetBooleanv"        , (void **)&pglGetBooleanv },
{ "glGetDoublev"         , (void **)&pglGetDoublev },
{ "glGetFloatv"          , (void **)&pglGetFloatv },
{ "glGetIntegerv"        , (void **)&pglGetIntegerv },
{ "glGetError"           , (void **)&pglGetError },
{ "glGetString"          , (void **)&pglGetString },
{ "glFinish"             , (void **)&pglFinish },
{ "glFlush"              , (void **)&pglFlush },
{ "glClearDepth"         , (void **)&pglClearDepth },
{ "glDepthFunc"          , (void **)&pglDepthFunc },
{ "glDepthMask"          , (void **)&pglDepthMask },
{ "glDepthRange"         , (void **)&pglDepthRange },
{ "glFrontFace"          , (void **)&pglFrontFace },
{ "glDrawElements"       , (void **)&pglDrawElements },
{ "glColorMask"          , (void **)&pglColorMask },
{ "glIndexPointer"       , (void **)&pglIndexPointer },
{ "glVertexPointer"      , (void **)&pglVertexPointer },
{ "glNormalPointer"      , (void **)&pglNormalPointer },
{ "glColorPointer"       , (void **)&pglColorPointer },
{ "glTexCoordPointer"    , (void **)&pglTexCoordPointer },
{ "glArrayElement"       , (void **)&pglArrayElement },
{ "glColor3f"            , (void **)&pglColor3f },
{ "glColor3fv"           , (void **)&pglColor3fv },
{ "glColor4f"            , (void **)&pglColor4f },
{ "glColor4fv"           , (void **)&pglColor4fv },
{ "glColor3ub"           , (void **)&pglColor3ub },
{ "glColor4ub"           , (void **)&pglColor4ub },
{ "glColor4ubv"          , (void **)&pglColor4ubv },
{ "glTexCoord1f"         , (void **)&pglTexCoord1f },
{ "glTexCoord2f"         , (void **)&pglTexCoord2f },
{ "glTexCoord3f"         , (void **)&pglTexCoord3f },
{ "glTexCoord4f"         , (void **)&pglTexCoord4f },
{ "glTexGenf"            , (void **)&pglTexGenf },
{ "glTexGenfv"           , (void **)&pglTexGenfv },
{ "glTexGeni"            , (void **)&pglTexGeni },
{ "glVertex2f"           , (void **)&pglVertex2f },
{ "glVertex3f"           , (void **)&pglVertex3f },
{ "glVertex3fv"          , (void **)&pglVertex3fv },
{ "glNormal3f"           , (void **)&pglNormal3f },
{ "glNormal3fv"          , (void **)&pglNormal3fv },
{ "glBegin"              , (void **)&pglBegin },
{ "glEnd"                , (void **)&pglEnd },
{ "glLineWidth"          , (void**)&pglLineWidth },
{ "glPointSize"          , (void**)&pglPointSize },
{ "glMatrixMode"         , (void **)&pglMatrixMode },
{ "glOrtho"              , (void **)&pglOrtho },
{ "glRasterPos2f"        , (void **)&pglRasterPos2f },
{ "glFrustum"            , (void **)&pglFrustum },
{ "glViewport"           , (void **)&pglViewport },
{ "glPushMatrix"         , (void **)&pglPushMatrix },
{ "glPopMatrix"          , (void **)&pglPopMatrix },
{ "glPushAttrib"         , (void **)&pglPushAttrib },
{ "glPopAttrib"          , (void **)&pglPopAttrib },
{ "glLoadIdentity"       , (void **)&pglLoadIdentity },
{ "glLoadMatrixd"        , (void **)&pglLoadMatrixd },
{ "glLoadMatrixf"        , (void **)&pglLoadMatrixf },
{ "glMultMatrixd"        , (void **)&pglMultMatrixd },
{ "glMultMatrixf"        , (void **)&pglMultMatrixf },
{ "glRotated"            , (void **)&pglRotated },
{ "glRotatef"            , (void **)&pglRotatef },
{ "glScaled"             , (void **)&pglScaled },
{ "glScalef"             , (void **)&pglScalef },
{ "glTranslated"         , (void **)&pglTranslated },
{ "glTranslatef"         , (void **)&pglTranslatef },
{ "glReadPixels"         , (void **)&pglReadPixels },
{ "glDrawPixels"         , (void **)&pglDrawPixels },
{ "glStencilFunc"        , (void **)&pglStencilFunc },
{ "glStencilMask"        , (void **)&pglStencilMask },
{ "glStencilOp"          , (void **)&pglStencilOp },
{ "glClearStencil"       , (void **)&pglClearStencil },
{ "glIsEnabled"          , (void **)&pglIsEnabled },
{ "glIsList"             , (void **)&pglIsList },
{ "glIsTexture"          , (void **)&pglIsTexture },
{ "glTexEnvf"            , (void **)&pglTexEnvf },
{ "glTexEnvfv"           , (void **)&pglTexEnvfv },
{ "glTexEnvi"            , (void **)&pglTexEnvi },
{ "glTexParameterf"      , (void **)&pglTexParameterf },
{ "glTexParameterfv"     , (void **)&pglTexParameterfv },
{ "glTexParameteri"      , (void **)&pglTexParameteri },
{ "glHint"               , (void **)&pglHint },
{ "glPixelStoref"        , (void **)&pglPixelStoref },
{ "glPixelStorei"        , (void **)&pglPixelStorei },
{ "glGenTextures"        , (void **)&pglGenTextures },
{ "glDeleteTextures"     , (void **)&pglDeleteTextures },
{ "glBindTexture"        , (void **)&pglBindTexture },
{ "glTexImage1D"         , (void **)&pglTexImage1D },
{ "glTexImage2D"         , (void **)&pglTexImage2D },
{ "glTexSubImage1D"      , (void **)&pglTexSubImage1D },
{ "glTexSubImage2D"      , (void **)&pglTexSubImage2D },
{ "glCopyTexImage1D"     , (void **)&pglCopyTexImage1D },
{ "glCopyTexImage2D"     , (void **)&pglCopyTexImage2D },
{ "glCopyTexSubImage1D"  , (void **)&pglCopyTexSubImage1D },
{ "glCopyTexSubImage2D"  , (void **)&pglCopyTexSubImage2D },
{ "glScissor"            , (void **)&pglScissor },
{ "glGetTexEnviv"        , (void **)&pglGetTexEnviv },
{ "glPolygonOffset"      , (void **)&pglPolygonOffset },
{ "glPolygonMode"        , (void **)&pglPolygonMode },
{ "glPolygonStipple"     , (void **)&pglPolygonStipple },
{ "glClipPlane"          , (void **)&pglClipPlane },
{ "glGetClipPlane"       , (void **)&pglGetClipPlane },
{ "glShadeModel"         , (void **)&pglShadeModel },
{ "glFogfv"              , (void **)&pglFogfv },
{ "glFogf"               , (void **)&pglFogf },
{ "glFogi"               , (void **)&pglFogi },
{ NULL, NULL }
};

static dllfunc_t pointparametersfunc[] =
{
{ "glPointParameterfEXT"  , (void **)&pglPointParameterfEXT },
{ "glPointParameterfvEXT" , (void **)&pglPointParameterfvEXT },
{ NULL, NULL }
};

static dllfunc_t drawrangeelementsfuncs[] =
{
{ "glDrawRangeElements" , (void **)&pglDrawRangeElements },
{ NULL, NULL }
};

static dllfunc_t drawrangeelementsextfuncs[] =
{
{ "glDrawRangeElementsEXT" , (void **)&pglDrawRangeElementsEXT },
{ NULL, NULL }
};

static dllfunc_t sgis_multitexturefuncs[] =
{
{ "glSelectTextureSGIS" , (void **)&pglSelectTextureSGIS },
{ "glMTexCoord2fSGIS"   , (void **)&pglMTexCoord2fSGIS },
{ NULL, NULL }
};

static dllfunc_t multitexturefuncs[] =
{
{ "glMultiTexCoord1fARB"     , (void **)&pglMultiTexCoord1f },
{ "glMultiTexCoord2fARB"     , (void **)&pglMultiTexCoord2f },
{ "glMultiTexCoord3fARB"     , (void **)&pglMultiTexCoord3f },
{ "glMultiTexCoord4fARB"     , (void **)&pglMultiTexCoord4f },
{ "glActiveTextureARB"       , (void **)&pglActiveTextureARB },
{ "glClientActiveTextureARB" , (void **)&pglClientActiveTexture },
{ "glClientActiveTextureARB" , (void **)&pglClientActiveTextureARB },
{ NULL, NULL }
};

static dllfunc_t compiledvertexarrayfuncs[] =
{
{ "glLockArraysEXT"   , (void **)&pglLockArraysEXT },
{ "glUnlockArraysEXT" , (void **)&pglUnlockArraysEXT },
{ "glDrawArrays"      , (void **)&pglDrawArrays },
{ NULL, NULL }
};

static dllfunc_t texture3dextfuncs[] =
{
{ "glTexImage3DEXT"        , (void **)&pglTexImage3D },
{ "glTexSubImage3DEXT"     , (void **)&pglTexSubImage3D },
{ "glCopyTexSubImage3DEXT" , (void **)&pglCopyTexSubImage3D },
{ NULL, NULL }
};

static dllfunc_t atiseparatestencilfuncs[] =
{
{ "glStencilOpSeparateATI"   , (void **)&pglStencilOpSeparate },
{ "glStencilFuncSeparateATI" , (void **)&pglStencilFuncSeparate },
{ NULL, NULL }
};

static dllfunc_t gl2separatestencilfuncs[] =
{
{ "glStencilOpSeparate"   , (void **)&pglStencilOpSeparate },
{ "glStencilFuncSeparate" , (void **)&pglStencilFuncSeparate },
{ NULL, NULL }
};

static dllfunc_t stenciltwosidefuncs[] =
{
{ "glActiveStencilFaceEXT" , (void **)&pglActiveStencilFaceEXT },
{ NULL, NULL }
};

static dllfunc_t blendequationfuncs[] =
{
{ "glBlendEquationEXT" , (void **)&pglBlendEquationEXT },
{ NULL, NULL }
};

static dllfunc_t shaderobjectsfuncs[] =
{
{ "glDeleteObjectARB"             , (void **)&pglDeleteObjectARB },
{ "glGetHandleARB"                , (void **)&pglGetHandleARB },
{ "glDetachObjectARB"             , (void **)&pglDetachObjectARB },
{ "glCreateShaderObjectARB"       , (void **)&pglCreateShaderObjectARB },
{ "glShaderSourceARB"             , (void **)&pglShaderSourceARB },
{ "glCompileShaderARB"            , (void **)&pglCompileShaderARB },
{ "glCreateProgramObjectARB"      , (void **)&pglCreateProgramObjectARB },
{ "glAttachObjectARB"             , (void **)&pglAttachObjectARB },
{ "glLinkProgramARB"              , (void **)&pglLinkProgramARB },
{ "glUseProgramObjectARB"         , (void **)&pglUseProgramObjectARB },
{ "glValidateProgramARB"          , (void **)&pglValidateProgramARB },
{ "glUniform1fARB"                , (void **)&pglUniform1fARB },
{ "glUniform2fARB"                , (void **)&pglUniform2fARB },
{ "glUniform3fARB"                , (void **)&pglUniform3fARB },
{ "glUniform4fARB"                , (void **)&pglUniform4fARB },
{ "glUniform1iARB"                , (void **)&pglUniform1iARB },
{ "glUniform2iARB"                , (void **)&pglUniform2iARB },
{ "glUniform3iARB"                , (void **)&pglUniform3iARB },
{ "glUniform4iARB"                , (void **)&pglUniform4iARB },
{ "glUniform1fvARB"               , (void **)&pglUniform1fvARB },
{ "glUniform2fvARB"               , (void **)&pglUniform2fvARB },
{ "glUniform3fvARB"               , (void **)&pglUniform3fvARB },
{ "glUniform4fvARB"               , (void **)&pglUniform4fvARB },
{ "glUniform1ivARB"               , (void **)&pglUniform1ivARB },
{ "glUniform2ivARB"               , (void **)&pglUniform2ivARB },
{ "glUniform3ivARB"               , (void **)&pglUniform3ivARB },
{ "glUniform4ivARB"               , (void **)&pglUniform4ivARB },
{ "glUniformMatrix2fvARB"         , (void **)&pglUniformMatrix2fvARB },
{ "glUniformMatrix3fvARB"         , (void **)&pglUniformMatrix3fvARB },
{ "glUniformMatrix4fvARB"         , (void **)&pglUniformMatrix4fvARB },
{ "glGetObjectParameterfvARB"     , (void **)&pglGetObjectParameterfvARB },
{ "glGetObjectParameterivARB"     , (void **)&pglGetObjectParameterivARB },
{ "glGetInfoLogARB"               , (void **)&pglGetInfoLogARB },
{ "glGetAttachedObjectsARB"       , (void **)&pglGetAttachedObjectsARB },
{ "glGetUniformLocationARB"       , (void **)&pglGetUniformLocationARB },
{ "glGetActiveUniformARB"         , (void **)&pglGetActiveUniformARB },
{ "glGetUniformfvARB"             , (void **)&pglGetUniformfvARB },
{ "glGetUniformivARB"             , (void **)&pglGetUniformivARB },
{ "glGetShaderSourceARB"          , (void **)&pglGetShaderSourceARB },
{ "glVertexAttribPointerARB"      , (void **)&pglVertexAttribPointerARB },
{ "glEnableVertexAttribArrayARB"  , (void **)&pglEnableVertexAttribArrayARB },
{ "glDisableVertexAttribArrayARB" , (void **)&pglDisableVertexAttribArrayARB },
{ "glBindAttribLocationARB"       , (void **)&pglBindAttribLocationARB },
{ "glGetActiveAttribARB"          , (void **)&pglGetActiveAttribARB },
{ "glGetAttribLocationARB"        , (void **)&pglGetAttribLocationARB },
{ NULL, NULL }
};

static dllfunc_t vertexshaderfuncs[] =
{
{ "glVertexAttribPointerARB"      , (void **)&pglVertexAttribPointerARB },
{ "glEnableVertexAttribArrayARB"  , (void **)&pglEnableVertexAttribArrayARB },
{ "glDisableVertexAttribArrayARB" , (void **)&pglDisableVertexAttribArrayARB },
{ "glBindAttribLocationARB"       , (void **)&pglBindAttribLocationARB },
{ "glGetActiveAttribARB"          , (void **)&pglGetActiveAttribARB },
{ "glGetAttribLocationARB"        , (void **)&pglGetAttribLocationARB },
{ NULL, NULL }
};

static dllfunc_t vbofuncs[] =
{
{ "glBindBufferARB"    , (void **)&pglBindBufferARB },
{ "glDeleteBuffersARB" , (void **)&pglDeleteBuffersARB },
{ "glGenBuffersARB"    , (void **)&pglGenBuffersARB },
{ "glIsBufferARB"      , (void **)&pglIsBufferARB },
{ "glMapBufferARB"     , (void **)&pglMapBufferARB },
{ "glUnmapBufferARB"   , (void **)&pglUnmapBufferARB },
{ "glBufferDataARB"    , (void **)&pglBufferDataARB },
{ "glBufferSubDataARB" , (void **)&pglBufferSubDataARB },
{ NULL, NULL}
};

static dllfunc_t occlusionfunc[] =
{
{ "glGenQueriesARB"        , (void **)&pglGenQueriesARB },
{ "glDeleteQueriesARB"     , (void **)&pglDeleteQueriesARB },
{ "glIsQueryARB"           , (void **)&pglIsQueryARB },
{ "glBeginQueryARB"        , (void **)&pglBeginQueryARB },
{ "glEndQueryARB"          , (void **)&pglEndQueryARB },
{ "glGetQueryivARB"        , (void **)&pglGetQueryivARB },
{ "glGetQueryObjectivARB"  , (void **)&pglGetQueryObjectivARB },
{ "glGetQueryObjectuivARB" , (void **)&pglGetQueryObjectuivARB },
{ NULL, NULL }
};

static dllfunc_t texturecompressionfuncs[] =
{
{ "glCompressedTexImage3DARB"    , (void **)&pglCompressedTexImage3DARB },
{ "glCompressedTexImage2DARB"    , (void **)&pglCompressedTexImage2DARB },
{ "glCompressedTexImage1DARB"    , (void **)&pglCompressedTexImage1DARB },
{ "glCompressedTexSubImage3DARB" , (void **)&pglCompressedTexSubImage3DARB },
{ "glCompressedTexSubImage2DARB" , (void **)&pglCompressedTexSubImage2DARB },
{ "glCompressedTexSubImage1DARB" , (void **)&pglCompressedTexSubImage1DARB },
{ "glGetCompressedTexImageARB"   , (void **)&pglGetCompressedTexImage },
{ NULL, NULL }
};
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
=================
GL_MaxTextureUnits
=================
*/
int GL_MaxTextureUnits( void )
{
	if( GL_Support( GL_SHADER_GLSL100_EXT ))
		return min( max( glConfig.max_texture_coords, glConfig.max_teximage_units ), MAX_TEXTURE_UNITS );
	return glConfig.max_texture_units;
}

/*
=================
GL_GetProcAddress
=================
*/
void *GL_GetProcAddress( const char *name )
{
#ifdef XASH_SDL
	void *func = SDL_GL_GetProcAddress(name);
#elif defined (XASH_GLES)
	void *func = nanoGL_GetProcAddress(name);
#else //No opengl implementation
	void *func = NULL;
#endif
	if(!func)
	{
		MsgDev(D_ERROR, "Error: GL_GetProcAddress failed for %s", name);
	}
	return func;
}

/*
=================
GL_CheckExtension
=================
*/
void GL_CheckExtension( const char *name, const dllfunc_t *funcs, const char *cvarname, int r_ext )
{
	const dllfunc_t	*func;
	convar_t		*parm;

	MsgDev( D_NOTE, "GL_CheckExtension: %s ", name );

	if( cvarname )
	{
		// system config disable extensions
		parm = Cvar_Get( cvarname, "1", CVAR_GLCONFIG, va( "enable or disable %s", name ));

		if( parm->integer == 0 || ( gl_extensions->integer == 0 && r_ext != GL_OPENGL_110 ))
		{
			MsgDev( D_NOTE, "- disabled\n" );
			GL_SetExtension( r_ext, 0 );
			return; // nothing to process at
		}
		GL_SetExtension( r_ext, 1 );
	}

	if(( name[2] == '_' || name[3] == '_' ) && !Q_strstr( glConfig.extensions_string, name ))
	{
		GL_SetExtension( r_ext, false );	// update render info
		MsgDev( D_NOTE, "- ^1failed\n" );
		return;
	}

	// clear exports
#ifndef __ANDROID__
	for( func = funcs; func && func->name; func++ )
		*func->func = NULL;
#endif


	GL_SetExtension( r_ext, true ); // predict extension state
#ifndef __ANDROID__
	for( func = funcs; func && func->name != NULL; func++ )
	{
		// functions are cleared before all the extensions are evaluated
		if(!(*func->func = (void *)GL_GetProcAddress( func->name )))
			GL_SetExtension( r_ext, false ); // one or more functions are invalid, extension will be disabled
	}
#endif

	if( GL_Support( r_ext ))
		MsgDev( D_NOTE, "- ^2enabled\n" );
	else MsgDev( D_NOTE, "- ^1failed\n" );
}

/*
===============
GL_BuildGammaTable
===============
*/
void GL_BuildGammaTable( void )
{
	int	i, v;
	double	invGamma, div;

	invGamma = 1.0 / bound( 0.5, vid_gamma->value, 2.3 );
	div = (double) 1.0 / 255.5;

	Q_memcpy( glState.gammaRamp, glState.stateRamp, sizeof( glState.gammaRamp ));

	for( i = 0; i < 256; i++ )
	{
		v = (int)(65535.0 * pow(((double)i + 0.5 ) * div, invGamma ) + 0.5 );

		glState.gammaRamp[i+0] = ((word)bound( 0, v, 65535 ));
		glState.gammaRamp[i+256] = ((word)bound( 0, v, 65535 ));
		glState.gammaRamp[i+512] = ((word)bound( 0, v, 65535 ));
	}
}

/*
===============
GL_UpdateGammaRamp
===============
*/
void GL_UpdateGammaRamp( void )
{
	if( !glConfig.deviceSupportsGamma ) return;

	GL_BuildGammaTable();
#ifdef XASH_SDL
	SDL_SetWindowGammaRamp( host.hWnd, &glState.gammaRamp[0], &glState.gammaRamp[256], &glState.gammaRamp[512] );
#endif
}

/*
===============
GL_UpdateSwapInterval
===============
*/
void GL_UpdateSwapInterval( void )
{
	if( cls.state < ca_active )
	{
#ifdef XASH_SDL
		SDL_GL_SetSwapInterval( 0 );
		gl_swapInterval->modified = true;
#endif
	}
	else if( gl_swapInterval->modified )
	{
		gl_swapInterval->modified = false;
#ifdef XASH_SDL
		if( SDL_GL_SetSwapInterval(gl_swapInterval->integer) )
			MsgDev(D_ERROR, "SDL_GL_SetSwapInterval: %s\n", SDL_GetError());
#endif
	}
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
}

/*
===============
GL_ContextError
===============
*/
static void GL_ContextError( void )
{
#ifdef XASH_SDL
	MsgDev( D_ERROR, "GL_ContextError: %s\n", SDL_GetError() );
#endif
}

/*
==================
GL_SetupAttributes
==================
*/
void GL_SetupAttributes()
{
#ifdef XASH_SDL
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

#ifdef __ANDROID__
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
#endif // XASH_SDL
}

/*
=================
GL_CreateContext
=================
*/
qboolean GL_CreateContext( void )
{
#ifdef __ANDROID__
	nanoGL_Init();
#endif
	/*if( !Sys_CheckParm( "-gldebug" ) || host.developer < 1 ) // debug bit the kills perfomance
		return true;*/
#ifdef XASH_SDL
	if( ( glw_state.context = SDL_GL_CreateContext( host.hWnd ) ) == NULL)
	{
		MsgDev(D_ERROR, "GL_CreateContext: %s\n", SDL_GetError());
		return GL_DeleteContext();
	}
#endif
	return true;
}

/*
=================
GL_UpdateContext
=================
*/
qboolean GL_UpdateContext( void )
{
#ifdef XASH_SDL
	if(!( SDL_GL_MakeCurrent( host.hWnd, glw_state.context ) ) )
	{
		MsgDev(D_ERROR, "GL_UpdateContext: %s", SDL_GetError());
		return GL_DeleteContext();
	}
#endif
	return true;
}

/*
=================
GL_DeleteContext
=================
*/
qboolean GL_DeleteContext( void )
{
#ifdef XASH_SDL
	SDL_GL_DeleteContext(glw_state.context);
#endif
	glw_state.context = NULL;

	return false;
}
#ifdef XASH_SDL
/*
=================
VID_ChoosePFD
=================
*/
static int VID_ChoosePFD( SDL_PixelFormat *pfd, int colorBits, int alphaBits, int depthBits, int stencilBits )
{
	/*int	pixelFormat = 0;

	MsgDev( D_NOTE, "VID_ChoosePFD( color %i, alpha %i, depth %i, stencil %i )\n", colorBits, alphaBits, depthBits, stencilBits );

	// Fill out the PFD
	pfd->nSize = sizeof (PIXELFORMATDESCRIPTOR);
	pfd->nVersion = 1;
	pfd->dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
	pfd->iPixelType = PFD_TYPE_RGBA;

	pfd->cColorBits = colorBits;
	pfd->cRedBits = 0;
	pfd->cRedShift = 0;
	pfd->cGreenBits = 0;
	pfd->cGreenShift = 0;
	pfd->cBlueBits = 0;
	pfd->cBlueShift = 0;	// wow! Blue Shift %)

	pfd->cAlphaBits = alphaBits;
	pfd->cAlphaShift = 0;

	pfd->cAccumBits = 0;
	pfd->cAccumRedBits = 0;
	pfd->cAccumGreenBits = 0;
	pfd->cAccumBlueBits = 0;
	pfd->cAccumAlphaBits= 0;

	pfd->cDepthBits = depthBits;
	pfd->cStencilBits = stencilBits;

	pfd->cAuxBuffers = 0;
	pfd->iLayerType = PFD_MAIN_PLANE;
	pfd->bReserved = 0;

	pfd->dwLayerMask = 0;
	pfd->dwVisibleMask = 0;
	pfd->dwDamageMask = 0;

	// count PFDs
	pixelFormat = ChoosePixelFormat( glw_state.hDC, pfd );

	if( !pixelFormat )
	{
		MsgDev( D_ERROR, "VID_ChoosePFD failed\n" );
		return 0;
	}

	return pixelFormat;*/

	return 0;
}
#endif
#ifdef _WIN32
BOOL CALLBACK pfnEnumWnd( HWND hwnd, LPARAM lParam )
{
	string	wndname;

	if( GetClassName( hwnd, wndname, sizeof( wndname ) - 1 ))
	{
		if( !Q_strcmp( wndname, WINDOW_NAME ))
			num_instances++;
	}
	return true;
}
#else
#ifdef XASH_X11
Window* NetClientList(Display* display, unsigned long *len)
{
	Window* windowList;
	Atom type;
	int form, errno;
	unsigned long remain;

	errno = XGetWindowProperty(
		display,
		XDefaultRootWindow(display),
		XInternAtom(display, "_NET_CLIENT_LIST", False),
		0,
		65536,
		False,
		XA_WINDOW,
		&type,
		&form,
		len,
		&remain,
		(byte**)&windowList
	);

	if(errno != Success)
	{
		MsgDev(D_ERROR, "VID_EnumerateInstances: Xlib error: %s\n", strerror(errno));
		return NULL;
	}
	return windowList;
}

char* WindowClassName(Display* display, Window window)
{
	char* className;
	Atom type;
	unsigned long len, remain;
	int form, errno;
	errno = XGetWindowProperty(
		display,
		window,
		XInternAtom(display, "WM_CLASS", False),
		0,
		65536,
		False,
		XA_WINDOW,
		&type,
		&form,
		&len,
		&remain,
		(byte**)&className
	);

	if(errno != Success)
	{
		MsgDev(D_ERROR, "VID_EnumerateInstances: Xlib error: %s\n", strerror(errno));
		return NULL;
	}

	return className;
}
#endif
#endif

uint VID_EnumerateInstances( void )
{
	num_instances = 0;

#ifdef _WIN32
	if( EnumWindows( &pfnEnumWnd, 0 ))
		return num_instances;
#else
#ifdef XASH_X11
	Display* display = XOpenDisplay(NULL);
	Window* winlist;
	char* name;
	unsigned long len;
	int i;

	if(!display)
	{
		MsgDev(D_ERROR, "Lol, no displays? Returning 1 instance.\n");
		return 1;
	}

	if( !(winlist = NetClientList(display, &len)) ) return 1;

	for(i = 0; i < len; i++)
	{
		if( !(name = WindowClassName(display, winlist[i])) ) continue;
		if( !Q_strcmp( name, WINDOW_NAME ) )
			num_instances++;
		free(name);
	}

	XFree(winlist);
#endif
#endif
	return 1;
}

void VID_StartupGamma( void )
{
	// Device supports gamma anyway, but cannot do anything with it.
	fs_offset_t	gamma_size;
	byte	*savedGamma;
	size_t	gammaTypeSize = sizeof(glState.stateRamp);

	// init gamma ramp
	Q_memset( glState.stateRamp, 0, gammaTypeSize);

#if !defined (__ANDROID__) && defined(XASH_SDL)
	glConfig.deviceSupportsGamma = !SDL_GetWindowGammaRamp( host.hWnd, NULL, NULL, NULL);
#else
	// Android doesn't support hw gamma. (thanks, SDL!)
	glConfig.deviceSupportsGamma = 0;
#endif

	if( !glConfig.deviceSupportsGamma )
	{
		// force to set cvar
		Cvar_FullSet( "gl_ignorehwgamma", "1", CVAR_GLCONFIG );
		MsgDev( D_ERROR, "VID_StartupGamma: hardware gamma unsupported\n");
	}

	if( gl_ignorehwgamma->integer )
	{
		glConfig.deviceSupportsGamma = false;	// even if supported!
		BuildGammaTable( vid_gamma->value, vid_texgamma->value );
		MsgDev( D_NOTE, "VID_StartupGamma: software gamma initialized\n" );
		return;
	}

	// share this extension so engine can grab them
	GL_SetExtension( GL_HARDWARE_GAMMA_CONTROL, glConfig.deviceSupportsGamma );

	savedGamma = FS_LoadFile( "gamma.dat", &gamma_size, false );

	if( !savedGamma || gamma_size != (fs_offset_t)gammaTypeSize)
	{
		// saved gamma not found or corrupted file
		FS_WriteFile( "gamma.dat", glState.stateRamp, gammaTypeSize);
		MsgDev( D_NOTE, "VID_StartupGamma: gamma.dat initialized\n" );
		if( savedGamma ) Mem_Free( savedGamma );
	}
	else
	{
		GL_BuildGammaTable();

		// validate base gamma
		if( !Q_memcmp( savedGamma, glState.stateRamp, gammaTypeSize))
		{
			// all ok, previous gamma is valid
			MsgDev( D_NOTE, "VID_StartupGamma: validate screen gamma - ok\n" );
		}
		else if( !Q_memcmp( glState.gammaRamp, glState.stateRamp, gammaTypeSize))
		{
			// screen gamma is equal to render gamma (probably previous instance crashed)
			// run additional check to make sure for it
			if( Q_memcmp( savedGamma, glState.stateRamp, gammaTypeSize))
			{
				// yes, current gamma it's totally wrong, restore it from gamma.dat
				MsgDev( D_NOTE, "VID_StartupGamma: restore original gamma after crash\n" );
				Q_memcpy( glState.stateRamp, savedGamma, gammaTypeSize);
			}
			else
			{
				// oops, savedGamma == glState.stateRamp == glState.gammaRamp
				// probably r_gamma set as default
				MsgDev( D_NOTE, "VID_StartupGamma: validate screen gamma - disabled\n" ); 
			}
		}
		else if( !Q_memcmp( glState.gammaRamp, savedGamma, gammaTypeSize))
		{
			// saved gamma is equal render gamma, probably gamma.dat wroted after crash
			// run additional check to make sure it
			if( Q_memcmp( savedGamma, glState.stateRamp, gammaTypeSize))
			{
				// yes, saved gamma it's totally wrong, get origianl gamma from screen
				MsgDev( D_NOTE, "VID_StartupGamma: merge gamma.dat after crash\n" );
				FS_WriteFile( "gamma.dat", glState.stateRamp, gammaTypeSize);
			}
			else
			{
				// oops, savedGamma == glState.stateRamp == glState.gammaRamp
				// probably r_gamma set as default
				MsgDev( D_NOTE, "VID_StartupGamma: validate screen gamma - disabled\n" ); 
			}
		}
		else
		{
			// current gamma unset by other application, so we can restore it here
			MsgDev( D_NOTE, "VID_StartupGamma: restore original gamma after crash\n" );
			Q_memcpy( glState.stateRamp, savedGamma, gammaTypeSize);
		}

		Mem_Free( savedGamma );
	}

	vid_gamma->modified = true;
}

void VID_RestoreGamma( void )
{
	if( !glConfig.deviceSupportsGamma )
		return;

	// don't touch gamma if multiple instances was running
	if( VID_EnumerateInstances( ) > 1 ) return;

#if defined(XASH_SDL)
	SDL_SetWindowGammaRamp( host.hWnd, &glState.stateRamp[0],
			&glState.stateRamp[256], &glState.stateRamp[512] );
#endif
}

/*
=================
GL_SetPixelformat
=================
*/
qboolean GL_SetPixelformat( void )
{
	/*PIXELFORMATDESCRIPTOR	PFD;
	int			alphaBits;
	int			stencilBits;
	int			pixelFormat;

	if(( glw_state.hDC = GetDC( host.hWnd )) == NULL )
		return false;

	// set alpha/stencil
	alphaBits = bound( 0, gl_alphabits->integer, 8 );
	stencilBits = bound( 0, gl_stencilbits->integer, 8 );

	if( glw_state.desktopBitsPixel < 32 )
	{
		// clear alphabits in case we in 16-bit mode
		alphaBits = 0;
	}

	// choose a pixel format
	pixelFormat = VID_ChoosePFD( &PFD, 24, alphaBits, 32, stencilBits );

	if( !pixelFormat )
	{
		// try again with default color/depth/stencil
		pixelFormat = VID_ChoosePFD( &PFD, 24, 0, 32, 0 );

		if( !pixelFormat )
		{
			MsgDev( D_ERROR, "GL_SetPixelformat: failed to find an appropriate PIXELFORMAT\n" );
			return false;
		}
	}

	// set the pixel format
	if( !SetPixelFormat( glw_state.hDC, pixelFormat, &PFD ))
	{
		MsgDev( D_ERROR, "GL_SetPixelformat: failed\n" );
		return false;
	}

	DescribePixelFormat( glw_state.hDC, pixelFormat, sizeof( PIXELFORMATDESCRIPTOR ), &PFD );

	if( PFD.dwFlags & PFD_GENERIC_FORMAT )
	{
		if( PFD.dwFlags & PFD_GENERIC_ACCELERATED )
		{
			MsgDev( D_NOTE, "VID_ChoosePFD: using Generic MCD acceleration\n" );
			glw_state.software = false;
		}
		else if( gl_allow_software->integer )
		{
			MsgDev( D_NOTE, "VID_ChoosePFD: using software emulation\n" );
			glw_state.software = true;
		}
		else
		{
			MsgDev( D_ERROR, "GL_SetPixelformat: no hardware acceleration found\n" );
			return false;
		}
	}
	else
	{
		MsgDev( D_NOTE, "VID_ChoosePFD: using hardware acceleration\n");
		glw_state.software = false;
	}

	glConfig.color_bits = PFD.cColorBits;
	glConfig.alpha_bits = PFD.cAlphaBits;
	glConfig.depth_bits = PFD.cDepthBits;
	glConfig.stencil_bits = PFD.cStencilBits;

	if( PFD.cStencilBits != 0 )
		glState.stencilEnabled = true;
	else glState.stencilEnabled = false;

	// print out PFD specifics 
	MsgDev( D_NOTE, "GL PFD: color( %d-bits ) alpha( %d-bits ) Z( %d-bit )\n", PFD.cColorBits, PFD.cAlphaBits, PFD.cDepthBits );*/

	return true;
}

void R_SaveVideoMode( int w, int h )
{
	glState.width = w;
	glState.height = h;

	Cvar_FullSet( "width", va( "%i", w ), CVAR_READ_ONLY | CVAR_RENDERINFO );
	Cvar_FullSet( "height", va( "%i", h ), CVAR_READ_ONLY | CVAR_RENDERINFO );

	if( vid_mode->integer >= 0 && vid_mode->integer <= num_vidmodes )
		glState.wideScreen = vidmode[vid_mode->integer].wideScreen;

	MsgDev( D_INFO, "Set: [%dx%d]\n", w, h );
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

qboolean VID_CreateWindow( int width, int height, qboolean fullscreen )
{
#ifdef XASH_SDL
	static string	wndname;
	Uint32 wndFlags = SDL_WINDOW_MOUSE_FOCUS | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

	Q_strncpy( wndname, GI->title, sizeof( wndname ));

	host.hWnd = SDL_CreateWindow(wndname, r_xpos->integer,
		r_ypos->integer, width, height, wndFlags);

	if( !host.hWnd )
	{
		MsgDev( D_ERROR, "VID_CreateWindow: couldn't create '%s': %s\n", wndname, SDL_GetError());
		return false;
	}

	if( fullscreen )
	{
		SDL_DisplayMode want, got;

		want.w = width;
		want.h = height;
		want.driverdata = want.format = want.refresh_rate = 0; // don't care

		if( !SDL_GetClosestDisplayMode(0, &want, &got) )
			return false;

		MsgDev(D_NOTE, "Got closest display mode: %ix%i@%i\n", got.w, got.h, got.refresh_rate);

		if( SDL_SetWindowDisplayMode(host.hWnd, &got) == -1 )
			return false;

		if( SDL_SetWindowFullscreen(host.hWnd, SDL_WINDOW_FULLSCREEN) == -1 )
			return false;

	}

	host.window_center_x = width / 2;
	host.window_center_y = height / 2;

#if defined(_WIN32)
	HICON ico;

	if( FS_FileExists( GI->iconpath, true ))
	{
		char	localPath[MAX_PATH];

		Q_snprintf( localPath, sizeof( localPath ), "%s/%s", GI->gamedir, GI->iconpath );
		ico = LoadImage( NULL, localPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE|LR_DEFAULTSIZE );

		if( !ico )
		{
			MsgDev( D_INFO, "Extract %s from pak if you want to see it.\n", GI->iconpath );
			ico = LoadIcon( host.hInst, MAKEINTRESOURCE( 101 ));
		}
	}
	else ico = LoadIcon( host.hInst, MAKEINTRESOURCE( 101 ));

	SDL_SysWMinfo info;
	if(SDL_GetWindowWMInfo(host.hWnd, &info))
	{
		// info.info.info.info.info... Holy shit, SDL?
		SetClassLong(info.info.win.window, GCL_HICON, ico);
	}
#endif

	SDL_ShowWindow( host.hWnd );

	// init all the gl stuff for the window
	if( !GL_SetPixelformat( ))
	{
		SDL_HideWindow( host.hWnd );
		SDL_DestroyWindow( host.hWnd );
		host.hWnd = NULL;

		MsgDev( D_ERROR, "OpenGL driver not installed\n" );

		return false;
	}
#else
	host.hWnd = 1; //fake window
	GL_SetPixelformat( );
	host.window_center_x = width / 2;
	host.window_center_y = height / 2;
#endif
	if( !glw_state.initialized )
	{
		if( !GL_CreateContext( ))
			return false;

		VID_StartupGamma();
	}
	else
	{
		if( !GL_UpdateContext( ))
			return false;		
	}
	return true;
}

void VID_DestroyWindow( void )
{
#ifdef XASH_SDL
	if( glw_state.context )
	{
		SDL_GL_DeleteContext( glw_state.context );
		glw_state.context = NULL;
	}

	if( host.hWnd )
	{
		SDL_DestroyWindow ( host.hWnd );
		host.hWnd = NULL;
	}
#endif
	if( glState.fullScreen )
	{
		glState.fullScreen = false;
	}
}

/*
==================
R_ChangeDisplaySettingsFast

Change window size fastly to custom values, without setting vid mode
==================
*/
void R_ChangeDisplaySettingsFast( int width, int height )
{
	//Cvar_SetFloat("vid_mode", VID_NOMODE);
	Cvar_SetFloat("width", width);
	Cvar_SetFloat("height", height);

	glState.width = width;
	glState.height = height;

	SCR_VidInit();
}


rserr_t R_ChangeDisplaySettings( int width, int height, qboolean fullscreen )
{
#ifdef XASH_SDL
	SDL_DisplayMode displayMode;

	SDL_GetCurrentDisplayMode(0, &displayMode);

#ifdef __ANDROID__
	width = displayMode.w;
	height = displayMode.h;
	fullscreen = false;
#endif

	R_SaveVideoMode( width, height );

	// check our desktop attributes
	glw_state.desktopBitsPixel = SDL_BITSPERPIXEL(displayMode.format);
	glw_state.desktopWidth = displayMode.w;
	glw_state.desktopHeight = displayMode.h;

	glState.fullScreen = fullscreen;

	if(!host.hWnd)
	{
		if( !VID_CreateWindow( width, height, fullscreen ) )
			return rserr_invalid_mode;
	}
#ifndef __ANDROID__ // don't change resolution on Android platform
	else if( fullscreen )
	{
		SDL_DisplayMode want, got;

		want.w = width;
		want.h = height;
		want.driverdata = want.format = want.refresh_rate = 0; // don't care

		if( !SDL_GetClosestDisplayMode(0, &want, &got) )
			return rserr_invalid_mode;

		MsgDev(D_NOTE, "Got closest display mode: %ix%i@%i\n", got.w, got.h, got.refresh_rate);

		if( ( SDL_GetWindowFlags(host.hWnd) & SDL_WINDOW_FULLSCREEN ) == SDL_WINDOW_FULLSCREEN)
			if( SDL_SetWindowFullscreen(host.hWnd, 0) == -1 )
				return rserr_invalid_fullscreen;

		if( SDL_SetWindowDisplayMode(host.hWnd, &got) )
			return rserr_invalid_mode;

		if( SDL_SetWindowFullscreen(host.hWnd, SDL_WINDOW_FULLSCREEN) == -1 )
			return rserr_invalid_fullscreen;

		R_ChangeDisplaySettingsFast( got.w, got.h );
	}
	else
	{
		if( SDL_SetWindowFullscreen(host.hWnd, 0) )
			return rserr_invalid_fullscreen;
		SDL_SetWindowSize(host.hWnd, width, height);
		R_ChangeDisplaySettingsFast( width, height );
	}
#endif // __ANDROID__
#endif // XASH_SDL
	return rserr_ok;
}
/*
==================
VID_SetMode

Set the described video mode
==================
*/
qboolean VID_SetMode( void )
{
#ifdef XASH_SDL
	qboolean	fullscreen = false;
	int iScreenWidth, iScreenHeight;
	rserr_t	err;

	if( vid_mode->integer == -1 )	// trying to get resolution automatically by default
	{
		SDL_DisplayMode mode;

		SDL_GetDesktopDisplayMode(0, &mode);

		iScreenWidth = mode.w;
		iScreenHeight = mode.h;

		Cvar_SetFloat( "fullscreen", 1 );
	}
	else if( vid_mode->modified && vid_mode->integer >= 0 && vid_mode->integer <= num_vidmodes )
	{
		iScreenWidth = vidmode[vid_mode->integer].width;
		iScreenHeight = vidmode[vid_mode->integer].height;
	}
	else
	{
		iScreenHeight = scr_height->integer;
		iScreenWidth = scr_width->integer;
	}

	gl_swapInterval->modified = true;
	fullscreen = Cvar_VariableInteger("fullscreen") != 0;

	if(( err = R_ChangeDisplaySettings( iScreenWidth, iScreenHeight, fullscreen )) == rserr_ok )
	{
		glConfig.prev_width = iScreenWidth;
		glConfig.prev_height = iScreenHeight;
	}
	else
	{
		if( err == rserr_invalid_fullscreen )
		{
			Cvar_SetFloat( "fullscreen", 0 );
			MsgDev( D_ERROR, "VID_SetMode: fullscreen unavailable in this mode\n" );
			if(( err = R_ChangeDisplaySettings( iScreenWidth, iScreenHeight, false )) == rserr_ok )
				return true;
		}
		else if( err == rserr_invalid_mode )
		{
			Cvar_SetFloat( "vid_mode", glConfig.prev_mode );
			MsgDev( D_ERROR, "VID_SetMode: invalid mode\n" );
		}

		// try setting it back to something safe
		if(( err = R_ChangeDisplaySettings( glConfig.prev_width, glConfig.prev_height, false )) != rserr_ok )
		{
			MsgDev( D_ERROR, "VID_SetMode: could not revert to safe mode\n" );
			return false;
		}
	}
#endif
	return true;
}

/*
==================
VID_CheckChanges

check vid modes and fullscreen
==================
*/
void VID_CheckChanges( void )
{
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
==================
R_Init_OpenGL
==================
*/
qboolean R_Init_OpenGL( void )
{
	GL_SetupAttributes();
#ifdef XASH_SDL
	if( SDL_GL_LoadLibrary( NULL ) )
	{
		MsgDev( D_ERROR, "Couldn't initialize OpenGL: %s\n", SDL_GetError());
		return false;
	}
#endif
	return VID_SetMode();
}

/*
==================
R_Free_OpenGL
==================
*/
void R_Free_OpenGL( void )
{
	VID_RestoreGamma ();

	GL_DeleteContext ();

	VID_DestroyWindow ();
#ifdef XASH_SDL
	SDL_GL_UnloadLibrary ();
#endif
	// now all extensions are disabled
	Q_memset( glConfig.extension, 0, sizeof( glConfig.extension[0] ) * GL_EXTCOUNT );
	glw_state.initialized = false;
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
	Msg( "MODE: %i x %i %s\n", scr_width->integer, scr_height->integer );
	Msg( "GAMMA: %s\n", (glConfig.deviceSupportsGamma) ? "hardware" : "software" );
	Msg( "\n" );
	Msg( "PICMIP: %i\n", gl_picmip->integer );
	Msg( "SKYMIP: %i\n", gl_skymip->integer );
	Msg( "TEXTUREMODE: %s\n", gl_texturemode->string );
	Msg( "VERTICAL SYNC: %s\n", gl_swapInterval->integer ? "enabled" : "disabled" );
	Msg( "Color %d bits, Alpha %d bits, Depth %d bits, Stencil %d bits\n", glConfig.color_bits,
		glConfig.alpha_bits, glConfig.depth_bits, glConfig.stencil_bits );
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
	r_decals = Cvar_Get( "r_decals", "4096", CVAR_ARCHIVE, "sets the maximum number of decals" );
	r_xpos = Cvar_Get( "r_xpos", "130", CVAR_GLCONFIG, "window position by horizontal" );
	r_ypos = Cvar_Get( "r_ypos", "48", CVAR_GLCONFIG, "window position by vertical" );
			
	gl_picmip = Cvar_Get( "gl_picmip", "0", CVAR_GLCONFIG, "reduces resolution of textures by powers of 2" );
	gl_skymip = Cvar_Get( "gl_skymip", "0", CVAR_GLCONFIG, "reduces resolution of skybox textures by powers of 2" );
	gl_ignorehwgamma = Cvar_Get( "gl_ignorehwgamma", "0", CVAR_GLCONFIG, "ignore hardware gamma" );
	gl_allow_software = Cvar_Get( "gl_allow_software", "0", CVAR_ARCHIVE, "allow OpenGL software emulation" );
	gl_alphabits = Cvar_Get( "gl_alphabits", "8", CVAR_GLCONFIG, "pixelformat alpha bits (0 - auto)" );
	gl_texturemode = Cvar_Get( "gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE, "texture filter" );
	gl_round_down = Cvar_Get( "gl_round_down", "0", CVAR_GLCONFIG, "down size non-power of two textures" );
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
	gl_compensate_gamma_screenshots = Cvar_Get( "gl_compensate_gamma_screenshots", "0", CVAR_ARCHIVE, "allow to apply gamma value for screenshots and snapshots" ); 
	gl_keeptjunctions = Cvar_Get( "gl_keeptjunctions", "1", CVAR_ARCHIVE, "disable to reduce vertexes count but removing tjuncs causes blinking pixels" ); 
	gl_allow_static = Cvar_Get( "gl_allow_static", "0", CVAR_ARCHIVE, "force to drawing non-moveable brushes as part of world (save FPS)" );
	gl_allow_mirrors = Cvar_Get( "gl_allow_mirrors", "1", CVAR_ARCHIVE, "allow to draw mirror surfaces" );
	gl_showtextures = Cvar_Get( "r_showtextures", "0", CVAR_CHEAT, "show all uploaded textures (type values from 1 to 13)" );
	gl_finish = Cvar_Get( "gl_finish", "0", CVAR_ARCHIVE, "use glFinish instead of glFlush" );
	gl_nosort = Cvar_Get( "gl_nosort", "0", CVAR_ARCHIVE, "disable sorting of translucent surfaces" );
	gl_clear = Cvar_Get( "gl_clear", "0", CVAR_ARCHIVE, "clearing screen after each frame" );
	gl_test = Cvar_Get( "gl_test", "0", 0, "engine developer cvar for quick testing new features" );
	gl_wireframe = Cvar_Get( "gl_wireframe", "0", 0, "show wireframe overlay" );
	gl_overview = Cvar_Get( "dev_overview", "0", 0, "show level overview" );

	// these cvar not used by engine but some mods requires this
	Cvar_Get( "gl_polyoffset", "-0.1", 0, "polygon offset for decals" );
 
	// make sure r_swapinterval is checked after vid_restart
	gl_swapInterval->modified = true;

	vid_gamma = Cvar_Get( "gamma", "1.0", CVAR_ARCHIVE, "gamma amount" );
	vid_texgamma = Cvar_Get( "texgamma", "2.2", CVAR_GLCONFIG, "texgamma amount (default Half-Life artwork gamma)" );
	vid_mode = Cvar_Get( "vid_mode", VID_AUTOMODE, CVAR_RENDERINFO, "display resolution mode" );
	vid_fullscreen = Cvar_Get( "fullscreen", "0", CVAR_RENDERINFO, "set in 1 to enable fullscreen mode" );
	vid_displayfrequency = Cvar_Get ( "vid_displayfrequency", "0", CVAR_RENDERINFO, "fullscreen refresh rate" );

	Cmd_AddCommand( "r_info", R_RenderInfo_f, "display renderer info" );
	Cmd_AddCommand( "texturelist", R_TextureList_f, "display loaded textures list" );
}

void GL_RemoveCommands( void )
{
	Cmd_RemoveCommand( "r_info");
	Cmd_RemoveCommand( "texturelist" );
}

void GL_InitExtensions( void )
{
	// initialize gl extensions
	GL_CheckExtension( "OpenGL 1.1.0", opengl_110funcs, NULL, GL_OPENGL_110 );

	// get our various GL strings
	glConfig.vendor_string = (char *)pglGetString( GL_VENDOR );
	glConfig.renderer_string = (char *)pglGetString( GL_RENDERER );
	glConfig.version_string = (char *)pglGetString( GL_VERSION );
	glConfig.extensions_string = (char *)pglGetString( GL_EXTENSIONS );
	MsgDev( D_INFO, "Video: %s\n", glConfig.renderer_string );

	// initalize until base opengl functions loaded

	GL_CheckExtension( "glDrawRangeElements", drawrangeelementsfuncs, "gl_drawrangeelments", GL_DRAW_RANGEELEMENTS_EXT );

	if( !GL_Support( GL_DRAW_RANGEELEMENTS_EXT ))
		GL_CheckExtension( "GL_EXT_draw_range_elements", drawrangeelementsextfuncs, "gl_drawrangeelments", GL_DRAW_RANGEELEMENTS_EXT );

	// multitexture
	glConfig.max_texture_units = glConfig.max_texture_coords = glConfig.max_teximage_units = 1;
	GL_CheckExtension( "GL_ARB_multitexture", multitexturefuncs, "gl_arb_multitexture", GL_ARB_MULTITEXTURE );

	if( GL_Support( GL_ARB_MULTITEXTURE ))
	{
		pglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glConfig.max_texture_units );
		GL_CheckExtension( "GL_ARB_texture_env_combine", NULL, "gl_texture_env_combine", GL_ENV_COMBINE_EXT );

		if( !GL_Support( GL_ENV_COMBINE_EXT ))
			GL_CheckExtension( "GL_EXT_texture_env_combine", NULL, "gl_texture_env_combine", GL_ENV_COMBINE_EXT );

		if( GL_Support( GL_ENV_COMBINE_EXT ))
			GL_CheckExtension( "GL_ARB_texture_env_dot3", NULL, "gl_texture_env_dot3", GL_DOT3_ARB_EXT );
	}
	else
	{
		GL_CheckExtension( "GL_SGIS_multitexture", sgis_multitexturefuncs, "gl_sgis_multitexture", GL_ARB_MULTITEXTURE );
		if( GL_Support( GL_ARB_MULTITEXTURE )) glConfig.max_texture_units = 2;
	}

	if( glConfig.max_texture_units == 1 )
		GL_SetExtension( GL_ARB_MULTITEXTURE, false );

	// 3d texture support
	GL_CheckExtension( "GL_EXT_texture3D", texture3dextfuncs, "gl_texture_3d", GL_TEXTURE_3D_EXT );

	if( GL_Support( GL_TEXTURE_3D_EXT ))
	{
		pglGetIntegerv( GL_MAX_3D_TEXTURE_SIZE, &glConfig.max_3d_texture_size );

		if( glConfig.max_3d_texture_size < 32 )
		{
			GL_SetExtension( GL_TEXTURE_3D_EXT, false );
			MsgDev( D_ERROR, "GL_EXT_texture3D reported bogus GL_MAX_3D_TEXTURE_SIZE, disabled\n" );
		}
	}

	GL_CheckExtension( "GL_SGIS_generate_mipmap", NULL, "gl_sgis_generate_mipmaps", GL_SGIS_MIPMAPS_EXT );

	// hardware cubemaps
	GL_CheckExtension( "GL_ARB_texture_cube_map", NULL, "gl_texture_cubemap", GL_TEXTURECUBEMAP_EXT );

	if( GL_Support( GL_TEXTURECUBEMAP_EXT ))
	{
		pglGetIntegerv( GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig.max_cubemap_size );

		// check for seamless cubemaps too
		GL_CheckExtension( "GL_ARB_seamless_cube_map", NULL, "gl_seamless_cubemap", GL_ARB_SEAMLESS_CUBEMAP );
	}

	// point particles extension
	GL_CheckExtension( "GL_EXT_point_parameters", pointparametersfunc, NULL, GL_EXT_POINTPARAMETERS );

	GL_CheckExtension( "GL_ARB_texture_non_power_of_two", NULL, "gl_texture_npot", GL_ARB_TEXTURE_NPOT_EXT );
	GL_CheckExtension( "GL_ARB_texture_compression", texturecompressionfuncs, "gl_dds_hardware_support", GL_TEXTURE_COMPRESSION_EXT );
	GL_CheckExtension( "GL_EXT_compiled_vertex_array", compiledvertexarrayfuncs, "gl_cva_support", GL_CUSTOM_VERTEX_ARRAY_EXT );

	if( !GL_Support( GL_CUSTOM_VERTEX_ARRAY_EXT ))
		GL_CheckExtension( "GL_SGI_compiled_vertex_array", compiledvertexarrayfuncs, "gl_cva_support", GL_CUSTOM_VERTEX_ARRAY_EXT );

	GL_CheckExtension( "GL_EXT_texture_edge_clamp", NULL, "gl_clamp_to_edge", GL_CLAMPTOEDGE_EXT );

	if( !GL_Support( GL_CLAMPTOEDGE_EXT ))
		GL_CheckExtension("GL_SGIS_texture_edge_clamp", NULL, "gl_clamp_to_edge", GL_CLAMPTOEDGE_EXT );

	glConfig.max_texture_anisotropy = 0.0f;
	GL_CheckExtension( "GL_EXT_texture_filter_anisotropic", NULL, "gl_ext_anisotropic_filter", GL_ANISOTROPY_EXT );

	if( GL_Support( GL_ANISOTROPY_EXT ))
		pglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.max_texture_anisotropy );

	GL_CheckExtension( "GL_EXT_texture_lod_bias", NULL, "gl_ext_texture_lodbias", GL_TEXTURE_LODBIAS );
	if( GL_Support( GL_TEXTURE_LODBIAS ))
		pglGetFloatv( GL_MAX_TEXTURE_LOD_BIAS_EXT, &glConfig.max_texture_lodbias );

	GL_CheckExtension( "GL_ARB_texture_border_clamp", NULL, "gl_ext_texborder_clamp", GL_CLAMP_TEXBORDER_EXT );

	GL_CheckExtension( "GL_EXT_blend_minmax", blendequationfuncs, "gl_ext_customblend", GL_BLEND_MINMAX_EXT );
	GL_CheckExtension( "GL_EXT_blend_subtract", blendequationfuncs, "gl_ext_customblend", GL_BLEND_SUBTRACT_EXT );

	GL_CheckExtension( "glStencilOpSeparate", gl2separatestencilfuncs, "gl_separate_stencil", GL_SEPARATESTENCIL_EXT );

	if( !GL_Support( GL_SEPARATESTENCIL_EXT ))
		GL_CheckExtension("GL_ATI_separate_stencil", atiseparatestencilfuncs, "gl_separate_stencil", GL_SEPARATESTENCIL_EXT );

	GL_CheckExtension( "GL_EXT_stencil_two_side", stenciltwosidefuncs, "gl_stenciltwoside", GL_STENCILTWOSIDE_EXT );
	GL_CheckExtension( "GL_ARB_vertex_buffer_object", vbofuncs, "gl_vertex_buffer_object", GL_ARB_VERTEX_BUFFER_OBJECT_EXT );

	// we don't care if it's an extension or not, they are identical functions, so keep it simple in the rendering code
#ifndef __ANDROID__
	if( pglDrawRangeElementsEXT == NULL ) pglDrawRangeElementsEXT = pglDrawRangeElements;
#endif
	GL_CheckExtension( "GL_ARB_texture_env_add", NULL, "gl_texture_env_add", GL_TEXTURE_ENV_ADD_EXT );

	// vp and fp shaders
	GL_CheckExtension( "GL_ARB_shader_objects", shaderobjectsfuncs, "gl_shaderobjects", GL_SHADER_OBJECTS_EXT );
	GL_CheckExtension( "GL_ARB_shading_language_100", NULL, "gl_glslprogram", GL_SHADER_GLSL100_EXT );
	GL_CheckExtension( "GL_ARB_vertex_shader", vertexshaderfuncs, "gl_vertexshader", GL_VERTEX_SHADER_EXT );
	GL_CheckExtension( "GL_ARB_fragment_shader", NULL, "gl_pixelshader", GL_FRAGMENT_SHADER_EXT );

	GL_CheckExtension( "GL_ARB_depth_texture", NULL, "gl_depthtexture", GL_DEPTH_TEXTURE );
	GL_CheckExtension( "GL_ARB_shadow", NULL, "gl_arb_shadow", GL_SHADOW_EXT );

	GL_CheckExtension( "GL_ARB_texture_float", NULL, "gl_arb_texture_float", GL_ARB_TEXTURE_FLOAT_EXT );
	GL_CheckExtension( "GL_ARB_depth_buffer_float", NULL, "gl_arb_depth_float", GL_ARB_DEPTH_FLOAT_EXT );

	// occlusion queries
	GL_CheckExtension( "GL_ARB_occlusion_query", occlusionfunc, "gl_occlusion_queries", GL_OCCLUSION_QUERIES_EXT );

	if( GL_Support( GL_SHADER_GLSL100_EXT ))
	{
		pglGetIntegerv( GL_MAX_TEXTURE_COORDS_ARB, &glConfig.max_texture_coords );
		pglGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &glConfig.max_teximage_units );
	}
	else
	{
		// just get from multitexturing
		glConfig.max_texture_coords = glConfig.max_teximage_units = glConfig.max_texture_units;
	}

	// rectangle textures support
	if( Q_strstr( glConfig.extensions_string, "GL_NV_texture_rectangle" ))
	{
		glConfig.texRectangle = GL_TEXTURE_RECTANGLE_NV;
		pglGetIntegerv( GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, &glConfig.max_2d_rectangle_size );
	}
#ifndef __ANDROID__
	else if( Q_strstr( glConfig.extensions_string, "GL_EXT_texture_rectangle" ))
	{
		glConfig.texRectangle = GL_TEXTURE_RECTANGLE_EXT;
		pglGetIntegerv( GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT, &glConfig.max_2d_rectangle_size );
	}
#endif
	else glConfig.texRectangle = glConfig.max_2d_rectangle_size = 0; // no rectangle

	glConfig.max_2d_texture_size = 0;
	pglGetIntegerv( GL_MAX_TEXTURE_SIZE, &glConfig.max_2d_texture_size );
	if( glConfig.max_2d_texture_size <= 0 ) glConfig.max_2d_texture_size = 256;

	Cvar_Get( "gl_max_texture_size", "0", CVAR_INIT, "opengl texture max dims" );
	Cvar_Set( "gl_max_texture_size", va( "%i", glConfig.max_2d_texture_size ));

	pglGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &glConfig.max_vertex_uniforms );
	pglGetIntegerv( GL_MAX_VERTEX_ATTRIBS_ARB, &glConfig.max_vertex_attribs );

	// MCD has buffering issues
	if(Q_strstr( glConfig.renderer_string, "gdi" ))
		Cvar_SetFloat( "gl_finish", 1 );

	Cvar_Set( "gl_anisotropy", va( "%f", bound( 0, gl_texture_anisotropy->value, glConfig.max_texture_anisotropy )));

	// software mipmap generator does wrong result with NPOT textures ...
	if( !GL_Support( GL_SGIS_MIPMAPS_EXT ))
		GL_SetExtension( GL_ARB_TEXTURE_NPOT_EXT, false );

	if( GL_Support( GL_TEXTURE_COMPRESSION_EXT ))
		Image_AddCmdFlags( IL_DDS_HARDWARE );

	glw_state.initialized = true;

	tr.framecount = tr.visframecount = 1;
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
	GL_SetDefaultState();

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
	R_InitImages();
	R_SpriteInit();
	R_StudioInit();
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
