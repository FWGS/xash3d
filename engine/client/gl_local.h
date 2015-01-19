/*
gl_local.h - renderer local declarations
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

#ifndef GL_LOCAL_H
#define GL_LOCAL_H

#include "gl_export.h"
#include "com_model.h"
#include "cl_entity.h"
#include "ref_params.h"
#include "render_api.h"
#include "protocol.h"
#include "dlight.h"

extern byte	*r_temppool;

#define BLOCK_SIZE		world.block_size	// lightmap blocksize
#define BLOCK_SIZE_DEFAULT	128		// for keep backward compatibility
#define BLOCK_SIZE_MAX	1024

#define MAX_TEXTURES	4096
#define MAX_DETAIL_TEXTURES	256
#define MAX_LIGHTMAPS	256
#define SUBDIVIDE_SIZE	64
#define MAX_MIRRORS		32	// per one frame!

#define NUMVERTEXNORMALS	162
#define SHADEDOT_QUANT 	16	// precalculated dot products for quantized angles

// refparams
#define RP_NONE		0
#define RP_MIRRORVIEW	BIT( 0 )	// lock pvs at vieworg
#define RP_ENVVIEW		BIT( 1 )	// used for cubemapshot
#define RP_OLDVIEWLEAF	BIT( 2 )
#define RP_CLIPPLANE	BIT( 3 )	// mirrors used
#define RP_FLIPFRONTFACE	BIT( 4 )	// e.g. for mirrors drawing

#define RP_NONVIEWERREF	(RP_MIRRORVIEW|RP_ENVVIEW)
#define R_StudioOpaque( e )	( e->curstate.rendermode == kRenderNormal || e->curstate.rendermode == kRenderTransAlpha )
#define RP_LOCALCLIENT( e )	(CL_GetLocalPlayer() && ((e)->index == CL_GetLocalPlayer()->index && e->curstate.entityType == ET_PLAYER ))
#define RP_NORMALPASS()	((RI.params & RP_NONVIEWERREF) == 0 )

#define TF_SKY		(TF_SKYSIDE|TF_UNCOMPRESSED|TF_NOMIPMAP|TF_NOPICMIP)
#define TF_FONT		(TF_UNCOMPRESSED|TF_NOPICMIP|TF_NOMIPMAP|TF_CLAMP)
#define TF_IMAGE		(TF_UNCOMPRESSED|TF_NOPICMIP|TF_NOMIPMAP|TF_CLAMP)
#define TF_DECAL		(TF_CLAMP|TF_UNCOMPRESSED)

typedef struct gltexture_s
{
	char		name[256];	// game path, including extension (can be store image programs)
	word		srcWidth;		// keep unscaled sizes
	word		srcHeight;
	word		width;		// upload width\height
	word		height;

	uint		cacheframe;	// worldmodel->load_sequence

	GLuint		target;		// glTarget
	GLuint		texnum;		// gl texture binding
	GLint		format;		// uploaded format
	texFlags_t	flags;

	rgba_t		fogParams;	// some water textures
					// contain info about underwater fog
	rgbdata_t		*original;	// keep original image

	// debug info
	byte		texType;		// used for gl_showtextures
	size_t		size;		// upload size for debug targets

	// detail textures stuff
	float		xscale;
	float		yscale;

	struct gltexture_s	*nextHash;
} gltexture_t;

// mirror entity
typedef struct gl_entity_s
{
	cl_entity_t	*ent;
	mextrasurf_t	*chain;
} gl_entity_t;

typedef struct
{
	int		params;		// rendering parameters

	qboolean		drawWorld;	// ignore world for drawing PlayerModel
	qboolean		thirdPerson;	// thirdperson camera is enabled
	qboolean		isSkyVisible;	// sky is visible
	qboolean		drawOrtho;	// draw world as orthogonal projection	

	ref_params_t	refdef;		// actual refdef

	cl_entity_t	*currententity;
	model_t		*currentmodel;
	cl_entity_t	*currentbeam;	// same as above but for beams

	int		viewport[4];
	mplane_t		frustum[6];

	vec3_t		pvsorigin;
	vec3_t		vieworg;		// locked vieworigin
	vec3_t		vforward;
	vec3_t		vright;
	vec3_t		vup;

	vec3_t		cullorigin;
	vec3_t		cull_vforward;
	vec3_t		cull_vright;
	vec3_t		cull_vup;

	float		farClip;
	uint		clipFlags;

	qboolean		fogCustom;
	qboolean		fogEnabled;
	vec3_t		fogColor;
	float		fogDensity;
	float		fogStart;
	float		fogEnd;
	int		cached_contents;	// in water

	float		waveHeight;	// global waveHeight
	float		currentWaveHeight;	// current entity waveHeight

	float		skyMins[2][6];
	float		skyMaxs[2][6];

	matrix4x4		objectMatrix;		// currententity matrix
	matrix4x4		worldviewMatrix;		// modelview for world
	matrix4x4		modelviewMatrix;		// worldviewMatrix * objectMatrix

	matrix4x4		projectionMatrix;
	matrix4x4		worldviewProjectionMatrix;	// worldviewMatrix * projectionMatrix
	int		lightstylevalue[MAX_LIGHTSTYLES];	// value 0 - 65536
	float		lightcache[MAX_LIGHTSTYLES];

	float		viewplanedist;
	mplane_t		clipPlane;
} ref_instance_t;

typedef struct
{
	int		cinTexture;      	// cinematic texture
	int		skyTexture;	// default sky texture
	int		whiteTexture;
	int		grayTexture;
	int		blackTexture;
	int		acontTexture;
	int		defaultTexture;   	// use for bad textures
	int		particleTexture;	// particle texture
	int		particleTexture2;	// unsmoothed particle texture
	int		solidskyTexture;	// quake1 solid-sky layer
	int		alphaskyTexture;	// quake1 alpha-sky layer
	int		lightmapTextures[MAX_LIGHTMAPS];
	int		dlightTexture;	// custom dlight texture
	int		dlightTexture2;	// big dlight texture (for big lightmaps)
	int		attenuationTexture;	// normal attenuation
	int		attenuationTexture2;// dark attenuation
	int		attenuationTexture3;// bright attenuation
	int		attenuationTexture3D;// 3D attenuation
	int		attenuationStubTexture;
	int		blankbumpTexture;
	int		blankdeluxeTexture;
	int		normalizeTexture;
	int		dlightCubeTexture;	// dynamic cubemap
	int		grayCubeTexture;
	int		whiteCubeTexture;
	int		skyboxTextures[6];	// skybox sides
	int		mirrorTextures[MAX_MIRRORS];
	int		num_mirrors_used;	// used mirror textures

	int		skytexturenum;	// this not a gl_texturenum!

	int		skyboxbasenum;	// start with 5800

	// entity lists
	cl_entity_t	*static_entities[MAX_VISIBLE_PACKET];	// opaque non-moved brushes
	gl_entity_t	mirror_entities[MAX_VISIBLE_PACKET];	// an entities that has mirror
	cl_entity_t	*solid_entities[MAX_VISIBLE_PACKET];	// opaque moving or alpha brushes
	cl_entity_t	*trans_entities[MAX_VISIBLE_PACKET];	// translucent brushes
	cl_entity_t	*child_entities[MAX_VISIBLE_PACKET];	// entities with MOVETYPE_FOLLOW
	uint		num_static_entities;
	uint		num_mirror_entities;
	uint		num_solid_entities;
	uint		num_trans_entities;
	uint		num_child_entities;
         
	// OpenGL matrix states
	qboolean		modelviewIdentity;
	qboolean		fResetVis;
	
	int		visframecount;	// PVS frame
	int		dlightframecount;	// dynamic light frame
	int		realframecount;	// not including passes
	int		framecount;

	// cull info
	vec3_t		modelorg;		// relative to viewpoint
} ref_globals_t;

typedef struct
{
	uint		c_world_polys;
	uint		c_brush_polys;
	uint		c_studio_polys;
	uint		c_sprite_polys;
	uint		c_world_leafs;

	uint		c_view_beams_count;
	uint		c_active_tents_count;
	uint		c_studio_models_drawn;
	uint		c_sprite_models_drawn;
	uint		c_particle_count;

	uint		c_mirror_passes;

	uint		c_client_ents;	// entities that moved to client
} ref_speeds_t;

extern ref_speeds_t		r_stats;
extern ref_params_t		r_lastRefdef;
extern ref_instance_t	RI;
extern ref_globals_t	tr;

extern float		gldepthmin, gldepthmax;
extern mleaf_t		*r_viewleaf, *r_oldviewleaf;
extern mleaf_t		*r_viewleaf2, *r_oldviewleaf2;
extern dlight_t		cl_dlights[MAX_DLIGHTS];
extern dlight_t		cl_elights[MAX_ELIGHTS];
#define r_numEntities	(tr.num_solid_entities + tr.num_trans_entities + tr.num_child_entities + tr.num_static_entities)
#define r_numStatics	(r_stats.c_client_ents)

extern struct beam_s	*cl_active_beams;
extern struct beam_s	*cl_free_beams;
extern struct particle_s	*cl_free_trails;

//
// gl_backend.c
//
void GL_BackendStartFrame( void );
void GL_BackendEndFrame( void );
void GL_CleanUpTextureUnits( int last );
void GL_Bind( GLint tmu, GLenum texnum );
void GL_MultiTexCoord2f( GLenum texture, GLfloat s, GLfloat t );
void GL_SetTexCoordArrayMode( GLenum mode );
void GL_LoadTexMatrix( const matrix4x4 m );
void GL_LoadTexMatrixExt( const float *glmatrix );
void GL_LoadMatrix( const matrix4x4 source );
void GL_TexGen( GLenum coord, GLenum mode );
void GL_SelectTexture( GLint texture );
void GL_LoadIdentityTexMatrix( void );
void GL_DisableAllTexGens( void );
void GL_SetRenderMode( int mode );
void GL_FrontFace( GLenum front );
void GL_TextureTarget( uint target );
void GL_Cull( GLenum cull );
void R_ShowTextures( void );

//
// gl_cull.c
//
int R_CullModel( cl_entity_t *e, vec3_t origin, vec3_t mins, vec3_t maxs, float radius );
qboolean R_CullBox( const vec3_t mins, const vec3_t maxs, uint clipflags );
qboolean R_CullSphere( const vec3_t centre, const float radius, const uint clipflags );
qboolean R_CullSurface( msurface_t *surf, uint clipflags );

//
// gl_decals.c
//
void DrawSurfaceDecals( msurface_t *fa );
float *R_DecalSetupVerts( decal_t *pDecal, msurface_t *surf, int texture, int *outCount );
void DrawSingleDecal( decal_t *pDecal, msurface_t *fa );
void R_EntityRemoveDecals( model_t *mod );
void R_ClearDecals( void );

//
// gl_draw.c
//
void R_Set2DMode( qboolean enable );
void R_DrawTileClear( int x, int y, int w, int h );
void R_UploadStretchRaw( int texture, int cols, int rows, int width, int height, const byte *data );

//
// gl_image.c
//
void R_SetTextureParameters( void );
gltexture_t *R_GetTexture( GLenum texnum );
void GL_SetTextureType( GLenum texnum, GLenum type );
int GL_LoadTexture( const char *name, const byte *buf, size_t size, int flags, imgfilter_t *filter );
int GL_LoadTextureInternal( const char *name, rgbdata_t *pic, texFlags_t flags, qboolean update );
byte *GL_ResampleTexture( const byte *source, int in_w, int in_h, int out_w, int out_h, qboolean isNormalMap );
int GL_CreateTexture( const char *name, int width, int height, const void *buffer, texFlags_t flags );
void GL_ProcessTexture( int texnum, float gamma, int topColor, int bottomColor );
void GL_TexFilter( gltexture_t *tex, qboolean update );
void R_FreeImage( gltexture_t *image );
int GL_FindTexture( const char *name );
void GL_FreeTexture( GLenum texnum );
void GL_FreeImage( const char *name );
const char *GL_Target( GLenum target );
void R_TextureList_f( void );
void R_InitImages( void );
void R_ShutdownImages( void );

//
// gl_mirror.c
//
void R_BeginDrawMirror( msurface_t *fa );
void R_EndDrawMirror( void );
void R_DrawMirrors( void );
void R_FindMirrors( const ref_params_t *fd );

//
// gl_refrag.c
//
void R_StoreEfrags( efrag_t **ppefrag, int framecount );

//
// gl_rlight.c
//
void R_PushDlights( void );
void R_AnimateLight( void );
void R_GetLightSpot( vec3_t lightspot );
void R_MarkLights( dlight_t *light, int bit, mnode_t *node );
void R_LightDir( const vec3_t origin, vec3_t lightDir, float radius );
void R_LightForPoint( const vec3_t point, color24 *ambientLight, qboolean invLight, qboolean useAmbient, float radius );
int R_CountSurfaceDlights( msurface_t *surf );
int R_CountDlights( void );

//
// gl_rmain.c
//
void R_ClearScene( void );
void R_LoadIdentity( void );
void R_RenderScene( const ref_params_t *fd );
void R_DrawCubemapView( const vec3_t origin, const vec3_t angles, int size );
void R_TranslateForEntity( cl_entity_t *e );
void R_RotateForEntity( cl_entity_t *e );
int R_ComputeFxBlend( cl_entity_t *e );
qboolean R_InitRenderAPI( void );
void R_SetupFrustum( void );
void R_FindViewLeaf( void );
void R_DrawFog( void );

//
// gl_rmath.c
//
float V_CalcFov( float *fov_x, float width, float height );
void V_AdjustFov( float *fov_x, float *fov_y, float width, float height, qboolean lock_x );
void Matrix4x4_ToArrayFloatGL( const matrix4x4 in, float out[16] );
void Matrix4x4_FromArrayFloatGL( matrix4x4 out, const float in[16] );
void Matrix4x4_Concat( matrix4x4 out, const matrix4x4 in1, const matrix4x4 in2 );
void Matrix4x4_ConcatTranslate( matrix4x4 out, float x, float y, float z );
void Matrix4x4_ConcatRotate( matrix4x4 out, float angle, float x, float y, float z );
void Matrix4x4_ConcatScale( matrix4x4 out, float x );
void Matrix4x4_ConcatScale3( matrix4x4 out, float x, float y, float z );
void Matrix4x4_CreateTranslate( matrix4x4 out, float x, float y, float z );
void Matrix4x4_CreateRotate( matrix4x4 out, float angle, float x, float y, float z );
void Matrix4x4_CreateScale( matrix4x4 out, float x );
void Matrix4x4_CreateScale3( matrix4x4 out, float x, float y, float z );
void Matrix4x4_CreateProjection(matrix4x4 out, float xMax, float xMin, float yMax, float yMin, float zNear, float zFar);
void Matrix4x4_CreateOrtho(matrix4x4 m, float xLeft, float xRight, float yBottom, float yTop, float zNear, float zFar);
void Matrix4x4_CreateModelview( matrix4x4 out );

//
// gl_rmisc.
//
void R_ParseTexFilters( const char *filename );
imgfilter_t *R_FindTexFilter( const char *texname );

//
// gl_rsurf.c
//
void R_MarkLeaves( void );
void R_DrawWorld( void );
void R_DrawMirrors( void );
void R_DrawWaterSurfaces( void );
void R_DrawBrushModel( cl_entity_t *e );
void GL_SubdivideSurface( msurface_t *fa );
void GL_BuildPolygonFromSurface( model_t *mod, msurface_t *fa );
void GL_SetupFogColorForSurfaces( void );
void GL_RebuildLightmaps( void );
void GL_BuildLightmaps( void );
void GL_ResetFogColor( void );

//
// gl_sprite.c
//
void R_SpriteInit( void );
void Mod_LoadSpriteModel( model_t *mod, const void *buffer, qboolean *loaded, uint texFlags );
mspriteframe_t *R_GetSpriteFrame( const model_t *pModel, int frame, float yaw );
void R_DrawSpriteModel( cl_entity_t *e );

//
// gl_studio.c
//
void R_StudioInit( void );
void Mod_LoadStudioModel( model_t *mod, const void *buffer, qboolean *loaded );
struct mstudiotex_s *R_StudioGetTexture( cl_entity_t *e );
void R_DrawStudioModel( cl_entity_t *e );

//
// gl_warp.c
//
void R_InitSky( struct mip_s *mt, struct texture_s *tx );
void R_AddSkyBoxSurface( msurface_t *fa );
void R_ClearSkyBox( void );
void R_DrawSkyBox( void );
void EmitSkyLayers( msurface_t *fa );
void EmitSkyPolys( msurface_t *fa );
void EmitWaterPolys( glpoly_t *polys, qboolean noCull );
void R_DrawSkyChain( msurface_t *s );

//
// gl_vidnt.c
//
#define GL_CheckForErrors() GL_CheckForErrors_( __FILE__, __LINE__ )
void GL_CheckForErrors_( const char *filename, const int fileline );
void GL_UpdateSwapInterval( void );
void GL_UpdateGammaRamp( void );
qboolean GL_DeleteContext( void );
qboolean GL_Support( int r_ext );
void VID_CheckChanges( void );
int GL_MaxTextureUnits( void );
qboolean R_Init( void );
void R_Shutdown( void );

//
// renderer exports
//
qboolean R_Init( void );
void R_Shutdown( void );
void VID_CheckChanges( void );
int GL_LoadTexture( const char *name, const byte *buf, size_t size, int flags, imgfilter_t *filter );
void GL_FreeImage( const char *name );
qboolean VID_ScreenShot( const char *filename, int shot_type );
qboolean VID_CubemapShot( const char *base, uint size, const float *vieworg, qboolean skyshot );
void VID_RestoreGamma( void );
void R_BeginFrame( qboolean clearScene );
void R_RenderFrame( const ref_params_t *fd, qboolean drawWorld );
void R_EndFrame( void );
void R_ClearScene( void );
void R_GetTextureParms( int *w, int *h, int texnum );
void R_GetSpriteParms( int *frameWidth, int *frameHeight, int *numFrames, int curFrame, const struct model_s *pSprite );
void R_DrawStretchRaw( float x, float y, float w, float h, int cols, int rows, const byte *data, qboolean dirty );
void R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, int texnum );
qboolean R_SpeedsMessage( char *out, size_t size );
void R_SetupSky( const char *skyboxname );
qboolean R_CullBox( const vec3_t mins, const vec3_t maxs, uint clipflags );
qboolean R_WorldToScreen( const vec3_t point, vec3_t screen );
void R_ScreenToWorld( const vec3_t screen, vec3_t point );
qboolean R_AddEntity( struct cl_entity_s *pRefEntity, int entityType );
void Mod_LoadMapSprite( struct model_s *mod, const void *buffer, size_t size, qboolean *loaded );
void Mod_UnloadSpriteModel( struct model_s *mod );
void Mod_UnloadStudioModel( struct model_s *mod );
void Mod_UnloadBrushModel( struct model_s *mod );
void GL_SetRenderMode( int mode );
void R_RunViewmodelEvents( void );
void R_DrawViewModel( void );
int R_GetSpriteTexture( const struct model_s *m_pSpriteModel, int frame );
void R_DecalShoot( int textureIndex, int entityIndex, int modelIndex, vec3_t pos, int flags, vec3_t saxis, float scale );
void R_RemoveEfrags( struct cl_entity_s *ent );
void R_AddEfrags( struct cl_entity_s *ent );
void R_DecalRemoveAll( int texture );
byte *Mod_GetCurrentVis( void );
void Mod_SetOrthoBounds( float *mins, float *maxs );
void R_NewMap( void );

/*
=======================================================================

 GL STATE MACHINE

=======================================================================
*/
enum
{
	GL_OPENGL_110 = 0,		// base
	GL_WGL_SWAPCONTROL,		
	GL_WGL_PROCADDRESS,
	GL_HARDWARE_GAMMA_CONTROL,
	GL_ARB_VERTEX_BUFFER_OBJECT_EXT,
	GL_ENV_COMBINE_EXT,
	GL_ARB_MULTITEXTURE,
	GL_TEXTURECUBEMAP_EXT,
	GL_DOT3_ARB_EXT,
	GL_ANISOTROPY_EXT,
	GL_TEXTURE_LODBIAS,
	GL_OCCLUSION_QUERIES_EXT,
	GL_TEXTURE_COMPRESSION_EXT,
	GL_SHADER_GLSL100_EXT,
	GL_SGIS_MIPMAPS_EXT,
	GL_DRAW_RANGEELEMENTS_EXT,
	GL_LOCKARRAYS_EXT,
	GL_TEXTURE_3D_EXT,
	GL_CLAMPTOEDGE_EXT,
	GL_BLEND_MINMAX_EXT,
	GL_STENCILTWOSIDE_EXT,
	GL_BLEND_SUBTRACT_EXT,
	GL_SHADER_OBJECTS_EXT,
	GL_VERTEX_SHADER_EXT,	// glsl vertex program
	GL_FRAGMENT_SHADER_EXT,	// glsl fragment program	
	GL_EXT_POINTPARAMETERS,
	GL_SEPARATESTENCIL_EXT,
	GL_ARB_TEXTURE_NPOT_EXT,
	GL_CUSTOM_VERTEX_ARRAY_EXT,
	GL_TEXTURE_ENV_ADD_EXT,
	GL_CLAMP_TEXBORDER_EXT,
	GL_ARB_TEXTURE_FLOAT_EXT,
	GL_DEPTH_TEXTURE,
	GL_SHADOW_EXT,
	GL_EXTCOUNT,		// must be last
};

enum
{
	GL_KEEP_UNIT = -1,
	GL_TEXTURE0 = 0,
	GL_TEXTURE1,
	GL_TEXTURE2,
	GL_TEXTURE3,		// g-cont. 4 units should be enough
	MAX_TEXTURE_UNITS = 32	// can't acess to all over units without GLSL or cg
};

typedef struct
{
	const char	*renderer_string;		// ptrs to OpenGL32.dll, use with caution
	const char	*vendor_string;
	const char	*version_string;

	// list of supported extensions
	const char	*extensions_string;
	byte		extension[GL_EXTCOUNT];

	int		max_texture_units;
	int		max_texture_coords;
	int		max_teximage_units;
	GLint		max_2d_texture_size;
	GLint		max_2d_rectangle_size;
	GLint		max_3d_texture_size;
	GLint		max_cubemap_size;
	GLint		texRectangle;

	GLfloat		max_texture_anisotropy;
	GLfloat		max_texture_lodbias;

	int		color_bits;
	int		alpha_bits;
	int		depth_bits;
	int		stencil_bits;

	qboolean		deviceSupportsGamma;
	int		prev_mode;
} glconfig_t;

typedef struct
{
	word		gammaRamp[768];		// current gamma ramp
	word		stateRamp[768];		// original gamma ramp

	int		width, height;
	qboolean		fullScreen;
	qboolean		wideScreen;

	int		activeTMU;
	GLint		currentTextures[MAX_TEXTURE_UNITS];
	GLuint		currentTextureTargets[MAX_TEXTURE_UNITS];
	GLboolean		texIdentityMatrix[MAX_TEXTURE_UNITS];
	GLint		genSTEnabled[MAX_TEXTURE_UNITS];	// 0 - disabled, OR 1 - S, OR 2 - T, OR 4 - R
	GLint		texCoordArrayMode[MAX_TEXTURE_UNITS];	// 0 - disabled, 1 - enabled, 2 - cubemap

	int		faceCull;
	int		frontFace;

	qboolean		drawTrans;
	qboolean		stencilEnabled;
	qboolean		in2DMode;
} glstate_t;

typedef struct
{
	HDC		hDC;		// handle to device context
	HGLRC		hGLRC;		// handle to GL rendering context

	int		desktopBitsPixel;
	int		desktopWidth;
	int		desktopHeight;

	qboolean		software;		// OpenGL software emulation
	qboolean		initialized;	// OpenGL subsystem started
} glwstate_t;

extern glconfig_t		glConfig;
extern glstate_t		glState;
extern glwstate_t		glw_state;

//
// renderer cvars
//
extern convar_t	*gl_allow_software;
extern convar_t	*gl_texture_anisotropy;
extern convar_t	*gl_extensions;
extern convar_t	*gl_stencilbits;
extern convar_t	*gl_ignorehwgamma;
extern convar_t	*gl_swapInterval;
extern convar_t	*gl_check_errors;
extern convar_t	*gl_round_down;
extern convar_t	*gl_texturemode;
extern convar_t	*gl_texture_lodbias;
extern convar_t	*gl_showtextures;
extern convar_t	*gl_compress_textures;
extern convar_t	*gl_luminance_textures;
extern convar_t	*gl_compensate_gamma_screenshots;
extern convar_t	*gl_keeptjunctions;
extern convar_t	*gl_detailscale;
extern convar_t	*gl_overview;	// draw map in overview mode
extern convar_t	*gl_wireframe;
extern convar_t	*gl_allow_static;
extern convar_t	*gl_allow_mirrors;
extern convar_t	*gl_picmip;
extern convar_t	*gl_skymip;
extern convar_t	*gl_finish;
extern convar_t	*gl_nosort;
extern convar_t	*gl_clear;
extern convar_t	*gl_test;		// cvar to testify new effects

extern convar_t	*r_width;
extern convar_t	*r_height;
extern convar_t	*r_speeds;
extern convar_t	*r_fullbright;
extern convar_t	*r_norefresh;
extern convar_t	*r_lighting_extended;
extern convar_t	*r_lighting_modulate;
extern convar_t	*r_lighting_ambient;
extern convar_t	*r_detailtextures;
extern convar_t	*r_faceplanecull;
extern convar_t	*r_drawentities;
extern convar_t	*r_adjust_fov;
extern convar_t	*r_lefthand;
extern convar_t	*r_flaresize;
extern convar_t	*r_decals;
extern convar_t	*r_novis;
extern convar_t	*r_nocull;
extern convar_t	*r_lockpvs;
extern convar_t	*r_lockcull;
extern convar_t	*r_dynamic;
extern convar_t	*r_lightmap;
extern convar_t	*r_fastsky;

extern convar_t	*vid_displayfrequency;
extern convar_t	*vid_fullscreen;
extern convar_t	*vid_gamma;
extern convar_t	*vid_texgamma;
extern convar_t	*vid_mode;

#endif//GL_LOCAL_H