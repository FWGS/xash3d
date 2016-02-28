#include "common.h"
#include "client.h"
#include "gl_local.h"
#include "mod_local.h"
#include "input.h"
#include "gl_vidnt.h"


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

vidmode_t vidmode[] =
{
{ "Mode  0: 4x3",	640,	480,	false	},
{ "Mode  1: 4x3",	800,	600,	false	},
{ "Mode  2: 4x3",	960,	720,	false	},
{ "Mode  3: 4x3",	1024,	768,	false	},
{ "Mode  4: 4x3",	1152,	864,	false	},
{ "Mode  5: 4x3",	1280,	800,	false	},
{ "Mode  6: 4x3",	1280,	960,	false	},
{ "Mode  7: 5x4",	1280,	1024,	false	},
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
{ "Mode 18: 16x10",	1440,	900,	true	},
{ "Mode 19: 16x10",	1680,	1050,	true	},
{ "Mode 20: 16x9",	1920,	1080,	true	},
{ "Mode 21: 16x10",	1920,	1200,	true	},
{ "Mode 22: 16x10",	2560,	1600,	true	},
{ "Mode 23: 16x9",	1600,	900,	true	},
};

int num_vidmodes = ( sizeof( vidmode ) / sizeof( vidmode[0] ));

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
