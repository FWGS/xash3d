/*
gl_vid_android.c - nanogl video backend
Copyright (C) 2016 mittorn

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
#if XASH_VIDEO == VIDEO_SDL && defined XASH_GL_STATIC
#include "client.h"
#include "gl_local.h"
#include "mod_local.h"
#include "input.h"
#include "gl_vidnt.h"
#ifdef XASH_NANOGL
#include <GL/nanogl.h>
#endif
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


/*
=================
GL_GetProcAddress
=================
*/
void *GL_GetProcAddress( const char *name )
{
#if defined( XASH_GLES )
	void *func = nanoGL_GetProcAddress( name );
#else
	void *func = SDL_GL_GetProcAddress( name );
#endif

	if( !func )
	{
		MsgDev( D_ERROR, "Error: GL_GetProcAddress failed for %s", name );
	}
	return func;
}


void GL_InitExtensions( void )
{
	// initialize gl extensions
	GL_CheckExtension( "OpenGL 1.1.0", (void*)opengl_110funcs, NULL, GL_OPENGL_110 );

	// get our various GL strings
	glConfig.vendor_string = pglGetString( GL_VENDOR );
	glConfig.renderer_string = pglGetString( GL_RENDERER );
	glConfig.version_string = pglGetString( GL_VERSION );
	glConfig.extensions_string = pglGetString( GL_EXTENSIONS );
	MsgDev( D_INFO, "Video: %s\n", glConfig.renderer_string );

	// initalize until base opengl functions loaded

	GL_SetExtension( GL_DRAW_RANGEELEMENTS_EXT, true );
	GL_SetExtension( GL_ARB_MULTITEXTURE, true );
	pglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glConfig.max_texture_units );
	glConfig.max_texture_coords = glConfig.max_texture_units = 4;

	GL_SetExtension( GL_ENV_COMBINE_EXT, true );
	GL_SetExtension( GL_DOT3_ARB_EXT, true );
	GL_SetExtension( GL_TEXTURE_3D_EXT, false );
	GL_SetExtension( GL_SGIS_MIPMAPS_EXT, true ); // gles specs
	GL_SetExtension( GL_ARB_VERTEX_BUFFER_OBJECT_EXT, true ); // gles specs

	// hardware cubemaps
	GL_CheckExtension( "GL_OES_texture_cube_map", NULL, "gl_texture_cubemap", GL_TEXTURECUBEMAP_EXT );

	if( GL_Support( GL_TEXTURECUBEMAP_EXT ))
	{
		pglGetIntegerv( GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &glConfig.max_cubemap_size );
	}
	GL_SetExtension( GL_ARB_SEAMLESS_CUBEMAP, false );

	GL_SetExtension( GL_EXT_POINTPARAMETERS, false );
	GL_CheckExtension( "GL_OES_texture_npot", NULL, "gl_texture_npot", GL_ARB_TEXTURE_NPOT_EXT );

	GL_SetExtension( GL_TEXTURE_COMPRESSION_EXT, false );
	GL_SetExtension( GL_CUSTOM_VERTEX_ARRAY_EXT, false );
	GL_SetExtension( GL_CLAMPTOEDGE_EXT, true ); // by gles1 specs
	GL_SetExtension( GL_ANISOTROPY_EXT, false );
	GL_SetExtension( GL_TEXTURE_LODBIAS, false );
	GL_SetExtension( GL_CLAMP_TEXBORDER_EXT, false );
	GL_SetExtension( GL_BLEND_MINMAX_EXT, false );
	GL_SetExtension( GL_BLEND_SUBTRACT_EXT, false );
	GL_SetExtension( GL_SEPARATESTENCIL_EXT, false );
	GL_SetExtension( GL_STENCILTWOSIDE_EXT, false );
	GL_SetExtension( GL_TEXTURE_ENV_ADD_EXT,false  );
	GL_SetExtension( GL_SHADER_OBJECTS_EXT, false );
	GL_SetExtension( GL_SHADER_GLSL100_EXT, false );
	GL_SetExtension( GL_VERTEX_SHADER_EXT,false );
	GL_SetExtension( GL_FRAGMENT_SHADER_EXT, false );
	GL_SetExtension( GL_SHADOW_EXT, false );
	GL_SetExtension( GL_ARB_DEPTH_FLOAT_EXT, false );
	GL_SetExtension( GL_OCCLUSION_QUERIES_EXT,false );
	GL_CheckExtension( "GL_OES_depth_texture", NULL, "gl_depthtexture", GL_DEPTH_TEXTURE );

	glConfig.texRectangle = glConfig.max_2d_rectangle_size = 0; // no rectangle

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


	GL_SetExtension( r_ext, true ); // predict extension state

	if( GL_Support( r_ext ))
		MsgDev( D_NOTE, "- ^2enabled\n" );
	else MsgDev( D_NOTE, "- ^1failed\n" );
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
		if( SDL_GL_SetSwapInterval(gl_swapInterval->integer) )
			MsgDev(D_ERROR, "SDL_GL_SetSwapInterval: %s\n", SDL_GetError());
	}
}

/*
==================
GL_SetupAttributes
==================
*/
void GL_SetupAttributes()
{
	int samples;

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, gl_stencilbits->integer );

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

#ifdef XASH_NANOGL
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

#if defined XASH_WES || defined XASH_REGAL
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

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
}

/*
=================
GL_CreateContext
=================
*/
qboolean GL_CreateContext( void )
{
	int colorBits[3];
#ifdef XASH_NANOGL
	nanoGL_Init();
#endif

	if( ( glw_state.context = SDL_GL_CreateContext( host.hWnd ) ) == NULL)
	{
		MsgDev(D_ERROR, "GL_CreateContext: %s\n", SDL_GetError());
		return GL_DeleteContext();
	}

	SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &colorBits[0] );
	SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &colorBits[1] );
	SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, &colorBits[2] );
	glConfig.color_bits = colorBits[0] + colorBits[1] + colorBits[2];

	SDL_GL_GetAttribute( SDL_GL_ALPHA_SIZE, &glConfig.alpha_bits );
	SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &glConfig.depth_bits );
	SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &glConfig.stencil_bits );
	glState.stencilEnabled = glConfig.stencil_bits ? true : false;

	SDL_GL_GetAttribute( SDL_GL_MULTISAMPLESAMPLES, &glConfig.msaasamples );

#ifdef XASH_WES
	void wes_init();
	wes_init();
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
	if(!( SDL_GL_MakeCurrent( host.hWnd, glw_state.context ) ) )
	{
		MsgDev(D_ERROR, "GL_UpdateContext: %s", SDL_GetError());
		return GL_DeleteContext();
	}
	return true;
}

#ifndef EGL_LIB
#define EGL_LIB NULL
#endif


/*
=================
GL_DeleteContext
=================
*/
qboolean GL_DeleteContext( void )
{
	if( glw_state.context )
		SDL_GL_DeleteContext(glw_state.context);
	glw_state.context = NULL;

	return false;
}


/*
==================
R_Init_OpenGL
==================
*/
qboolean R_Init_OpenGL( void )
{
	GL_SetupAttributes();

	if( SDL_GL_LoadLibrary( EGL_LIB ) )
	{
		MsgDev( D_ERROR, "Couldn't initialize OpenGL: %s\n", SDL_GetError());
		return false;
	}

	return VID_SetMode();
}


/*
==================
R_Free_OpenGL
==================
*/
void R_Free_OpenGL( void )
{
	GL_DeleteContext ();

	VID_DestroyWindow ();

	SDL_GL_UnloadLibrary ();

	// now all extensions are disabled
	Q_memset( glConfig.extension, 0, sizeof( glConfig.extension[0] ) * GL_EXTCOUNT );
	glw_state.initialized = false;
}
#endif // XASH_VIDEO
