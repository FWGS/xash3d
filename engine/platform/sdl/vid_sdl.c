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
#if XASH_VIDEO == VIDEO_SDL
#include "client.h"
#include "gl_local.h"
#include "mod_local.h"
#include "input.h"
#include "gl_vidnt.h"



#if (defined(_WIN32) && defined(XASH_SDL))
#include <SDL_syswm.h>
#endif

#ifdef XASH_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

#ifdef WIN32
// Enable NVIDIA High Performance Graphics while using Integrated Graphics.
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
#endif


typedef enum
{
	rserr_ok,
	rserr_invalid_fullscreen,
	rserr_invalid_mode,
	rserr_unknown
} rserr_t;

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
		MsgDev(D_ERROR, "Error: GL_GetProcAddress failed for %s\n", name);
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
	for( func = funcs; func && func->name; func++ )
		*func->func = NULL;


	GL_SetExtension( r_ext, true ); // predict extension state
	for( func = funcs; func && func->name != NULL; func++ )
	{
		// functions are cleared before all the extensions are evaluated
		if(!(*func->func = (void *)GL_GetProcAddress( func->name )))
			GL_SetExtension( r_ext, false ); // one or more functions are invalid, extension will be disabled
	}

	if( GL_Support( r_ext ))
		MsgDev( D_NOTE, "- ^2enabled\n" );
	else MsgDev( D_NOTE, "- ^1failed\n" );
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
	if( gl_swapInterval->modified )
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
	int samples;

#ifdef XASH_X11
	SDL_SetHint("SDL_VIDEO_X11_XRANDR", "1");
	SDL_SetHint("SDL_VIDEO_X11_XVIDMODE", "1");
#endif
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	switch( gl_msaa->integer )
	{
	case 2:
	case 4:
	case 8:
	case 16:
		samples = gl_msaa->integer;
		break;
	default:
		samples = 0; // don't use, because invalid parameter is passed
	}

	if( samples )
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}
#endif // XASH_SDL
}

void GL_InitExtensions( void )
{
	// initialize gl extensions
	GL_CheckExtension( "OpenGL 1.1.0", opengl_110funcs, NULL, GL_OPENGL_110 );

	// get our various GL strings
	glConfig.vendor_string = pglGetString( GL_VENDOR );
	glConfig.renderer_string = pglGetString( GL_RENDERER );
	glConfig.version_string = pglGetString( GL_VERSION );
	glConfig.extensions_string = pglGetString( GL_EXTENSIONS );
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
	else if( Q_strstr( glConfig.extensions_string, "GL_EXT_texture_rectangle" ))
	{
		glConfig.texRectangle = GL_TEXTURE_RECTANGLE_EXT;
		pglGetIntegerv( GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT, &glConfig.max_2d_rectangle_size );
	}
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
=================
GL_CreateContext
=================
*/
qboolean GL_CreateContext( void )
{
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
	if( glw_state.context )
		SDL_GL_DeleteContext(glw_state.context);
#endif
	glw_state.context = NULL;

	return false;
}
uint num_instances;
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

#if defined(XASH_SDL)
	glConfig.deviceSupportsGamma = !SDL_GetWindowGammaRamp( host.hWnd, NULL, NULL, NULL);
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

void R_ChangeDisplaySettingsFast( int width, int height );

void *SDL_GetVideoDevice( void );

#if 0
#ifdef _WIN32
#define XASH_SDL_WINDOW_RECREATE
#elif defined XASH_X11
#define XASH_SDL_USE_FAKEWND
#endif
#endif

#ifdef XASH_SDL_USE_FAKEWND

SDL_Window *fakewnd;

qboolean VID_SetScreenResolution( int width, int height )
{
	SDL_DisplayMode want, got;

	want.w = width;
	want.h = height;
	want.driverdata = NULL;
	want.format = want.refresh_rate = 0; // don't care

	if( !SDL_GetClosestDisplayMode(0, &want, &got) )
		return false;

	MsgDev(D_NOTE, "Got closest display mode: %ix%i@%i\n", got.w, got.h, got.refresh_rate);

	if( fakewnd )
		SDL_DestroyWindow( fakewnd );

	fakewnd = SDL_CreateWindow("fakewnd", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, got.h, got.w, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_GRABBED | SDL_WINDOW_FULLSCREEN );

	if( !fakewnd )
		return false;

	if( SDL_SetWindowDisplayMode( fakewnd, &got) == -1 )
		return false;

	//SDL_ShowWindow( fakewnd );
	if( SDL_SetWindowFullscreen( fakewnd, SDL_WINDOW_FULLSCREEN) == -1 )
		return false;
	SDL_SetWindowBordered( host.hWnd, SDL_FALSE );
	SDL_SetWindowPosition( host.hWnd, 0, 0 );
	SDL_SetWindowGrab( host.hWnd, SDL_TRUE );
	SDL_HideWindow( host.hWnd );
	SDL_ShowWindow( host.hWnd );
	SDL_SetWindowSize( host.hWnd, got.w, got.h );

	SDL_GL_GetDrawableSize( host.hWnd, &got.w, &got.h );

	R_ChangeDisplaySettingsFast( got.w, got.h );
	SDL_HideWindow( fakewnd );
	return true;
}

void VID_RestoreScreenResolution( void )
{
	if( fakewnd )
	{

		SDL_ShowWindow( fakewnd );
		SDL_DestroyWindow( fakewnd );
	}
	fakewnd = NULL;
	if( !Cvar_VariableInteger("fullscreen") )
	{
		SDL_SetWindowBordered( host.hWnd, SDL_TRUE );
		//SDL_SetWindowPosition( host.hWnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED  );
		SDL_SetWindowGrab( host.hWnd, SDL_FALSE );
	}
	else
	{
		SDL_MinimizeWindow( host.hWnd );
	}
}
#else
static qboolean recreate = false;
qboolean VID_SetScreenResolution( int width, int height )
{
	SDL_DisplayMode want, got;
	Uint32 wndFlags = 0;
	static string wndname;

	if( vid_highdpi->integer ) wndFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
	Q_strncpy( wndname, GI->title, sizeof( wndname ));

	want.w = width;
	want.h = height;
	want.driverdata = NULL;
	want.format = want.refresh_rate = 0; // don't care

	if( !SDL_GetClosestDisplayMode(0, &want, &got) )
		return false;

	MsgDev(D_NOTE, "Got closest display mode: %ix%i@%i\n", got.w, got.h, got.refresh_rate);

#ifdef XASH_SDL_WINDOW_RECREATE
	if( recreate )
	{
	SDL_DestroyWindow( host.hWnd );
	host.hWnd = SDL_CreateWindow(wndname, 0, 0, width, height, wndFlags | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_GRABBED );
	SDL_GL_MakeCurrent( host.hWnd, glw_state.context );
	recreate = false;
	}
#endif

	if( SDL_SetWindowDisplayMode( host.hWnd, &got) == -1 )
		return false;

	if( SDL_SetWindowFullscreen( host.hWnd, SDL_WINDOW_FULLSCREEN) == -1 )
		return false;
	SDL_SetWindowBordered( host.hWnd, SDL_FALSE );
	SDL_SetWindowPosition( host.hWnd, 0, 0 );
	SDL_SetWindowGrab( host.hWnd, SDL_TRUE );
	SDL_SetWindowSize( host.hWnd, got.w, got.h );

	SDL_GL_GetDrawableSize( host.hWnd, &got.w, &got.h );

	R_ChangeDisplaySettingsFast( got.w, got.h );
	return true;
}

void VID_RestoreScreenResolution( void )
{
	if( !Cvar_VariableInteger("fullscreen") )
	{
		SDL_SetWindowBordered( host.hWnd, SDL_TRUE );
		SDL_SetWindowPosition( host.hWnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED  );
		SDL_SetWindowGrab( host.hWnd, SDL_FALSE );
	}
	else
	{
		SDL_MinimizeWindow( host.hWnd );
		SDL_SetWindowFullscreen( host.hWnd, 0 );
		recreate = true;
	}
}
#endif

qboolean VID_CreateWindow( int width, int height, qboolean fullscreen )
{
#ifdef XASH_SDL
	static string	wndname;
	Uint32 wndFlags = 0;
	rgbdata_t *icon = NULL;
	char iconpath[64];

	if( vid_highdpi->integer ) wndFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
	Q_strncpy( wndname, GI->title, sizeof( wndname ));

	if( !fullscreen )
	host.hWnd = SDL_CreateWindow(wndname, r_xpos->integer,
		r_ypos->integer, width, height, wndFlags | SDL_WINDOW_MOUSE_FOCUS | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE );
	else
	{
		host.hWnd = SDL_CreateWindow(wndname, 0, 0, width, height, wndFlags | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_GRABBED );
		SDL_SetWindowFullscreen( host.hWnd, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS );
	}


	if( !host.hWnd )
	{
		MsgDev( D_ERROR, "VID_CreateWindow: couldn't create '%s': %s\n", wndname, SDL_GetError());

		// remove MSAA, if it present, because
		// window creating may fail on GLX visual choose
		if( gl_msaa->integer )
		{
			Cvar_Set("gl_msaa", "0");
			GL_SetupAttributes(); // re-choose attributes

			// try again
			return VID_CreateWindow( width, height, fullscreen );
		}
		return false;
	}

	if( fullscreen )
	{
		if( !VID_SetScreenResolution( width, height ) )
			return false;

	}
	else
		VID_RestoreScreenResolution();

	host.window_center_x = width / 2;
	host.window_center_y = height / 2;

#if defined(_WIN32)
#if !defined(__amd64__)
	{
		HICON ico;
		SDL_SysWMinfo info;

		if( FS_FileExists( GI->iconpath, true ) )
		{
			char	localPath[MAX_PATH];

			Q_snprintf( localPath, sizeof( localPath ), "%s/%s", GI->gamedir, GI->iconpath );
			ico = LoadImage( NULL, localPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE|LR_DEFAULTSIZE );

			if( !ico )
			{
				MsgDev( D_INFO, "Extract %s from pak if you want to see it.\n", GI->iconpath );
				ico = LoadIcon( host.hInst, MAKEINTRESOURCE( 101 ) );
			}
		}
		else ico = LoadIcon( host.hInst, MAKEINTRESOURCE( 101 ) );

		if( SDL_GetWindowWMInfo( host.hWnd, &info ) )
		{
			// info.info.info.info.info... Holy shit, SDL?
			SetClassLong( info.info.win.window, GCL_HICON, (LONG)ico );
		}
	}
#endif // __amd64__
#else // _WIN32

	Q_strcpy( iconpath, GI->iconpath );
	FS_StripExtension( iconpath );
	FS_DefaultExtension( iconpath, ".tga") ;

	icon = FS_LoadImage( iconpath, NULL, 0 );

	if( icon )
	{
		SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(
			icon->buffer,
			icon->width,
			icon->height,
			32,
			4 * icon->width,
			0x000000ff,
			0x0000ff00,
			0x00ff0000,
			0xff000000 );

		if( surface )
		{
			SDL_SetWindowIcon( host.hWnd, surface );
			SDL_FreeSurface( surface );
		}

		FS_FreeImage( icon );
	}
#endif

	SDL_ShowWindow( host.hWnd );
#else
	host.hWnd = 1; //fake window
	host.window_center_x = width / 2;
	host.window_center_y = height / 2;
#endif
	if( !glw_state.initialized )
	{
		if( !GL_CreateContext( ))
		{
			return false;
		}

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

	VID_RestoreScreenResolution();
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

	if( glState.width != width || glState.height != height )
	{
		glState.width = width;
		glState.height = height;
		if( width * 3 != height * 4 && width * 4 != height * 5 )
			glState.wideScreen = true;
		else glState.wideScreen = false;

		// as we don't recreate window here, update center positions by hand
		host.window_center_x = width / 2;
		host.window_center_y = height / 2;

		SCR_VidInit();
	}
}

rserr_t R_ChangeDisplaySettings( int width, int height, qboolean fullscreen )
{
#ifdef XASH_SDL
	SDL_DisplayMode displayMode;

	SDL_GetCurrentDisplayMode(0, &displayMode);

	R_SaveVideoMode( width, height );

	// check our desktop attributes
	glw_state.desktopBitsPixel = SDL_BITSPERPIXEL(displayMode.format);
	glw_state.desktopWidth = displayMode.w;
	glw_state.desktopHeight = displayMode.h;

	glState.fullScreen = fullscreen;

	// check for 4:3 or 5:4
	if( width * 3 != height * 4 && width * 4 != height * 5 )
		glState.wideScreen = true;
	else glState.wideScreen = false;


	if(!host.hWnd)
	{
		if( !VID_CreateWindow( width, height, fullscreen ) )
			return rserr_invalid_mode;
	}
	else if( fullscreen )
	{
		if( !VID_SetScreenResolution( width, height ) )
			return rserr_invalid_fullscreen;
	}
	else
	{
		VID_RestoreScreenResolution();
		if( SDL_SetWindowFullscreen(host.hWnd, 0) )
			return rserr_invalid_fullscreen;
		SDL_SetWindowSize(host.hWnd, width, height);
		SDL_GL_GetDrawableSize( host.hWnd, &width, &height );
		R_ChangeDisplaySettingsFast( width, height );
	}
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
			Sys_Warn("fullscreen unavailable in this mode!");
			if(( err = R_ChangeDisplaySettings( iScreenWidth, iScreenHeight, false )) == rserr_ok )
				return true;
		}
		else if( err == rserr_invalid_mode )
		{
			Cvar_SetFloat( "vid_mode", glConfig.prev_mode );
			MsgDev( D_ERROR, "VID_SetMode: invalid mode\n" );
			Sys_Warn("invalid mode");
		}

		// try setting it back to something safe
		if(( err = R_ChangeDisplaySettings( glConfig.prev_width, glConfig.prev_height, false )) != rserr_ok )
		{
			MsgDev( D_ERROR, "VID_SetMode: could not revert to safe mode\n" );
			Sys_Warn("could not revert to safe mode!");
			return false;
		}
	}
#endif
	return true;
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
#endif //XASH_VIDEO
