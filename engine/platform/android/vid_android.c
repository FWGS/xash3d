/*
gl_vid_android.c - Android video backend
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
#if XASH_VIDEO == VIDEO_ANDROID
#include "client.h"
#include "gl_local.h"
#include "mod_local.h"
#include "input.h"
#include <GL/nanogl.h>
#include "gl_vidnt.h"
#include "filesystem.h"



typedef enum
{
	rserr_ok,
	rserr_invalid_fullscreen,
	rserr_invalid_mode,
	rserr_unknown
} rserr_t;

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
	void *func = nanoGL_GetProcAddress(name);
	if(!func)
	{
		MsgDev(D_ERROR, "Error: GL_GetProcAddress failed for %s", name);
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

	GL_SetExtension( GL_DRAW_RANGEELEMENTS_EXT, false );
	GL_SetExtension( GL_ARB_MULTITEXTURE, false );
	GL_SetExtension( GL_ENV_COMBINE_EXT, false );
	GL_SetExtension( GL_DOT3_ARB_EXT, false );
	GL_SetExtension( GL_TEXTURE_3D_EXT, false );
	GL_SetExtension( GL_SGIS_MIPMAPS_EXT, true ); // gles specs

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
	GL_SetExtension( GL_ARB_VERTEX_BUFFER_OBJECT_EXT, false );
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
GL_UpdateGammaRamp
===============
*/
void GL_UpdateGammaRamp( void )
{
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
		Android_SwapInterval( gl_swapInterval->integer );
	}
}

/*
===============
GL_ContextError
===============
*/
static void GL_ContextError( void )
{

}

/*
==================
GL_SetupAttributes
==================
*/
void GL_SetupAttributes()
{

}

/*
=================
GL_CreateContext
=================
*/
qboolean GL_CreateContext( void )
{
	nanoGL_Init();
	return true;
}

/*
=================
GL_UpdateContext
=================
*/
qboolean GL_UpdateContext( void )
{
	return true;
}

/*
=================
GL_DeleteContext
=================
*/
qboolean GL_DeleteContext( void )
{
#if 0 // unsure, need testing
	MsgDev( D_NOTE, "nanoGL_Destroy( );");
	nanoGL_Destroy();

	MsgDev( D_NOTE, "Android_ShutdownGL( );");
	Android_ShutdownGL();
#endif

	return false;
}

uint VID_EnumerateInstances( void )
{
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

	// force to set cvar
	Cvar_FullSet( "gl_ignorehwgamma", "1", CVAR_GLCONFIG );

	glConfig.deviceSupportsGamma = false;	// even if supported!
	BuildGammaTable( vid_gamma->value, vid_texgamma->value );
	MsgDev( D_NOTE, "VID_StartupGamma: software gamma initialized\n" );
}

void VID_RestoreGamma( void )
{
}

qboolean VID_CreateWindow( int width, int height, qboolean fullscreen )
{
	return true;
}

void VID_DestroyWindow( void )
{
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
	MsgDev( D_NOTE, "R_ChangeDisplaySettingsFast(%d, %d)\n", width, height);

	glState.width = width;
	glState.height = height;
	host.window_center_x = width / 2;
	host.window_center_y = height / 2;

	glState.wideScreen = true; // V_AdjustFov will check for widescreen

	SCR_VidInit();
}


rserr_t R_ChangeDisplaySettings( int width, int height, qboolean fullscreen )
{
	Android_GetScreenRes(&width, &height);
	R_SaveVideoMode( width, height );

	host.window_center_x = width / 2;
	host.window_center_y = height / 2;
	
	glState.wideScreen = true; // V_AdjustFov will check for widescreen

	return rserr_ok;
}


qboolean VID_SetScreenResolution( int width, int height )
{
	return false;
}

void VID_RestoreScreenResolution( void )
{

}


/*
==================
VID_SetMode

Set the described video mode
==================
*/
qboolean VID_SetMode( void )
{
	int width, height;
	Android_GetScreenRes( &width, &height );
	MsgDev( D_NOTE, "VID_SetMode(%d, %d)\n", width, height);
	R_ChangeDisplaySettings( width, height, false );
	return true;
}

/*
==================
R_Init_OpenGL
==================
*/
qboolean R_Init_OpenGL( void )
{
	searchpath_t	*search = FS_FindFile( GI->iconpath, NULL, true );

	if( search )
		Android_SetIcon( va( "%s/%s%s", host.rootdir, search->filename, GI->iconpath ) );

	Android_SetTitle( GI->title );
	VID_StartupGamma();
	MsgDev( D_NOTE, "R_Init_OpenGL()\n");
	Android_InitGL();

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

	// now all extensions are disabled
	Q_memset( glConfig.extension, 0, sizeof( glConfig.extension[0] ) * GL_EXTCOUNT );
	glw_state.initialized = false;
}
#endif // XASH_VIDEO
