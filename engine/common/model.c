/*
model.c - modelloader
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "mod_local.h"
#include "sprite.h"
#include "mathlib.h"
#include "studio.h"
#include "wadfile.h"
#include "world.h"
#include "gl_local.h"
#include "features.h"
#include "client.h"

#define MAX_SIDE_VERTS		512	// per one polygon

world_static_t	world;

byte		*mod_base;
byte		*com_studiocache;		// cache for submodels
static model_t	*com_models[MAX_MODELS];	// shared replacement modeltable
static model_t	cm_models[MAX_MODELS];
static int	cm_nummodels = 0;
static byte	visdata[MAX_MAP_LEAFS/8];	// intermediate buffer
int		bmodel_version;		// global stuff to detect bsp version
char		modelname[64];		// short model name (without path and ext)
convar_t		*mod_studiocache;
convar_t		*mod_allow_materials;
static wadlist_t	wadlist;
		
model_t		*loadmodel;
model_t		*worldmodel;

// cvars


// default hullmins
static vec3_t cm_hullmins[MAX_MAP_HULLS] =
{
{ -16, -16, -36 },
{ -16, -16, -18 },
{   0,   0,   0 },
{ -32, -32, -32 },
};

// defualt hullmaxs
static vec3_t cm_hullmaxs[MAX_MAP_HULLS] =
{
{  16,  16,  36 },
{  16,  16,  18 },
{   0,   0,   0 },
{  32,  32,  32 },
};

/*
===============================================================================

			MOD COMMON UTILS

===============================================================================
*/
/*
================
Mod_SetupHulls
================
*/
int Mod_ArrayUsage( const char *szItem, int items, int maxitems, int itemsize )
{
	float	percentage = maxitems ? (items * 100.0f / maxitems) : 0.0f;

	Msg( "%-12s  %7i/%-7i  %7i/%-7i  (%4.1f%%)", szItem, items, maxitems, items * itemsize, maxitems * itemsize, percentage );

	if( percentage > 99.9f )
		Msg( "^1SIZE OVERFLOW!!!^7\n" );
	else if( percentage > 95.0f )
		Msg( "^3SIZE DANGER!^7\n" );
	else if( percentage > 80.0f )
		Msg( "^2VERY FULL!^7\n" );
	else Msg( "\n" );

	return items * itemsize;
}

/*
================
Mod_SetupHulls
================
*/
int Mod_GlobUsage( const char *szItem, int itemstorage, int maxstorage )
{
	float	percentage = maxstorage ? (itemstorage * 100.0f / maxstorage) : 0.0f;

	Msg( "%-12s     [variable]    %7i/%-7i  (%4.1f%%)", szItem, itemstorage, maxstorage, percentage );

	if( percentage > 99.9f )
		Msg( "^1SIZE OVERFLOW!!!^7\n" );
	else if( percentage > 95.0f )
		Msg( "^3SIZE DANGER!^7\n" );
	else if( percentage > 80.0f )
		Msg( "^2VERY FULL!^7\n" );
	else Msg( "\n" );

	return itemstorage;
}

/*
=============
Mod_PrintBSPFileSizes

Dumps info about current file
=============
*/
void Mod_PrintBSPFileSizes_f( void )
{
	int	totalmemory = 0;
	model_t	*w = worldmodel;

	if( !w || !w->numsubmodels )
	{
		Msg( "No map loaded\n" );
		return;
	}

	Msg( "\n" );
	Msg( "Object names  Objects/Maxobjs  Memory / Maxmem  Fullness\n" );
	Msg( "------------  ---------------  ---------------  --------\n" );

	totalmemory += Mod_ArrayUsage( "models",	w->numsubmodels,	MAX_MAP_MODELS,		sizeof( dmodel_t ));
	totalmemory += Mod_ArrayUsage( "planes",	w->numplanes,	MAX_MAP_PLANES,		sizeof( dplane_t ));
	totalmemory += Mod_ArrayUsage( "vertexes",	w->numvertexes,	MAX_MAP_VERTS,		sizeof( dvertex_t ));
	totalmemory += Mod_ArrayUsage( "nodes",		w->numnodes,	MAX_MAP_NODES,		sizeof( dnode_t ));
	totalmemory += Mod_ArrayUsage( "texinfos",	w->numtexinfo,	MAX_MAP_TEXINFO,		sizeof( dtexinfo_t ));
	totalmemory += Mod_ArrayUsage( "faces",		w->numsurfaces,	MAX_MAP_FACES,		sizeof( dface_t ));
	if( world.version == XTBSP_VERSION )
		totalmemory += Mod_ArrayUsage( "clipnodes", w->numclipnodes, MAX_MAP_CLIPNODES * 3,	sizeof( dclipnode_t ));
	else
		totalmemory += Mod_ArrayUsage( "clipnodes", w->numclipnodes, MAX_MAP_CLIPNODES,		sizeof( dclipnode_t ));
	totalmemory += Mod_ArrayUsage( "leaves",	w->numleafs,	MAX_MAP_LEAFS,		sizeof( dleaf_t ));
	totalmemory += Mod_ArrayUsage( "marksurfaces",	w->nummarksurfaces,	MAX_MAP_MARKSURFACES,	sizeof( dmarkface_t ));
	totalmemory += Mod_ArrayUsage( "surfedges",	w->numsurfedges,	MAX_MAP_SURFEDGES,		sizeof( dsurfedge_t ));
	totalmemory += Mod_ArrayUsage( "edges",		w->numedges,	MAX_MAP_EDGES,		sizeof( dedge_t ));

	totalmemory += Mod_GlobUsage( "texdata",	world.texdatasize,	MAX_MAP_MIPTEX );
	totalmemory += Mod_GlobUsage( "lightdata",	world.litdatasize,	MAX_MAP_LIGHTING );
	totalmemory += Mod_GlobUsage( "deluxemap",	world.vecdatasize,	MAX_MAP_LIGHTING );
	totalmemory += Mod_GlobUsage( "visdata",	world.visdatasize,	MAX_MAP_VISIBILITY );
	totalmemory += Mod_GlobUsage( "entdata",	world.entdatasize,	MAX_MAP_ENTSTRING );

	Msg( "=== Total BSP file data space used: %s ===\n", Q_memprint( totalmemory ));
	Msg( "World size ( %g %g %g ) units\n", world.size[0], world.size[1], world.size[2] );
	Msg( "original name: ^1%s\n", worldmodel->name );
	Msg( "internal name: %s\n", (world.message[0]) ? va( "^2%s", world.message ) : "none" );
}

/*
================
Mod_Modellist_f
================
*/
void Mod_Modellist_f( void )
{
	int	i, nummodels;
	model_t	*mod;

	Msg( "\n" );
	Msg( "-----------------------------------\n" );

	for( i = nummodels = 0, mod = cm_models; i < cm_nummodels; i++, mod++ )
	{
		if( !mod->name[0] )
			continue; // free slot

		Msg( "%s%s\n", mod->name, (mod->type == mod_bad) ? " (DEFAULTED)" : "" );
		nummodels++;
	}

	Msg( "-----------------------------------\n" );
	Msg( "%i total models\n", nummodels );
	Msg( "\n" );
}

/*
================
Mod_SetupHulls
================
*/
void Mod_SetupHulls( vec3_t mins[MAX_MAP_HULLS], vec3_t maxs[MAX_MAP_HULLS] )
{
	Q_memcpy( mins, cm_hullmins, sizeof( cm_hullmins ));
	Q_memcpy( maxs, cm_hullmaxs, sizeof( cm_hullmaxs ));
}

/*
===================
Mod_CompressVis
===================
*/
byte *Mod_CompressVis( const byte *in, size_t *size )
{
	int	j, rep;
	int	visrow;
	byte	*dest_p;

	if( !worldmodel )
	{
		Host_Error( "Mod_CompressVis: no worldmodel\n" );
		return NULL;
	}
	
	dest_p = visdata;
	visrow = (worldmodel->numleafs + 7) >> 3;
	
	for( j = 0; j < visrow; j++ )
	{
		*dest_p++ = in[j];
		if( in[j] ) continue;

		rep = 1;
		for( j++; j < visrow; j++ )
		{
			if( in[j] || rep == 255 )
				break;
			else rep++;
		}
		*dest_p++ = rep;
		j--;
	}

	if( size ) *size = dest_p - visdata;

	return visdata;
}

/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis( const byte *in )
{
	int	c, row;
	byte	*out;

	if( !worldmodel )
	{
		Host_Error( "Mod_DecompressVis: no worldmodel\n" );
		return NULL;
	}

	row = (worldmodel->numleafs + 7) >> 3;	
	out = visdata;

	if( !in )
	{	
		// no vis info, so make all visible
		while( row )
		{
			*out++ = 0xff;
			row--;
		}
		return visdata;
	}

	do
	{
		if( *in )
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;

		while( c )
		{
			*out++ = 0;
			c--;
		}
	} while( out - visdata < row );

	return visdata;
}

/*
==================
Mod_PointInLeaf

==================
*/
mleaf_t *Mod_PointInLeaf( const vec3_t p, mnode_t *node )
{
	ASSERT( node != NULL );

	while( 1 )
	{
		if( node->contents < 0 )
			return (mleaf_t *)node;
		node = node->children[PlaneDiff( p, node->plane ) < 0];
	}

	// never reached
	return NULL;
}

/*
==================
Mod_LeafPVS

==================
*/
byte *Mod_LeafPVS( mleaf_t *leaf, model_t *model )
{
	if( !model || !leaf || leaf == model->leafs || !model->visdata )
		return Mod_DecompressVis( NULL );
	return Mod_DecompressVis( leaf->compressed_vis );
}

/*
==================
Mod_LeafPHS

==================
*/
byte *Mod_LeafPHS( mleaf_t *leaf, model_t *model )
{
	if( !model || !leaf || leaf == model->leafs || !model->visdata )
		return Mod_DecompressVis( NULL );
	return Mod_DecompressVis( leaf->compressed_pas );
}

/*
==================
Mod_PointLeafnum

==================
*/
int Mod_PointLeafnum( const vec3_t p )
{
	// map not loaded
	if ( !worldmodel ) return 0;
	return Mod_PointInLeaf( p, worldmodel->nodes ) - worldmodel->leafs;
}

/*
======================================================================

LEAF LISTING

======================================================================
*/
static void Mod_BoxLeafnums_r( leaflist_t *ll, mnode_t *node )
{
	mplane_t	*plane;
	int	s;

	while( 1 )
	{
		if( node->contents == CONTENTS_SOLID )
			return;

		if( node->contents < 0 )
		{
			mleaf_t	*leaf = (mleaf_t *)node;

			// it's a leaf!
			if( ll->count >= ll->maxcount )
			{
				ll->overflowed = true;
				return;
			}

			ll->list[ll->count++] = leaf - worldmodel->leafs - 1;
			return;
		}
	
		plane = node->plane;
		s = BOX_ON_PLANE_SIDE( ll->mins, ll->maxs, plane );

		if( s == 1 )
		{
			node = node->children[0];
		}
		else if( s == 2 )
		{
			node = node->children[1];
		}
		else
		{
			// go down both
			if( ll->topnode == -1 )
				ll->topnode = node - worldmodel->nodes;
			Mod_BoxLeafnums_r( ll, node->children[0] );
			node = node->children[1];
		}
	}
}

/*
==================
Mod_BoxLeafnums
==================
*/
int Mod_BoxLeafnums( const vec3_t mins, const vec3_t maxs, short *list, int listsize, int *topnode )
{
	leaflist_t	ll;

	if( !worldmodel ) return 0;

	VectorCopy( mins, ll.mins );
	VectorCopy( maxs, ll.maxs );
	ll.count = 0;
	ll.maxcount = listsize;
	ll.list = list;
	ll.topnode = -1;
	ll.overflowed = false;

	Mod_BoxLeafnums_r( &ll, worldmodel->nodes );

	if( topnode ) *topnode = ll.topnode;
	return ll.count;
}

/*
=============
Mod_BoxVisible

Returns true if any leaf in boxspace
is potentially visible
=============
*/
qboolean Mod_BoxVisible( const vec3_t mins, const vec3_t maxs, const byte *visbits )
{
	short	leafList[MAX_BOX_LEAFS];
	int	i, count;

	if( !visbits || !mins || !maxs )
		return true;

	count = Mod_BoxLeafnums( mins, maxs, leafList, MAX_BOX_LEAFS, NULL );

	for( i = 0; i < count; i++ )
	{
		int	leafnum = leafList[i];

		if( leafnum != -1 && visbits[leafnum>>3] & (1<<( leafnum & 7 )))
			return true;
	}
	return false;
}

/*
==================
Mod_AmbientLevels

grab the ambient sound levels for current point
==================
*/
void Mod_AmbientLevels( const vec3_t p, byte *pvolumes )
{
	mleaf_t	*leaf;

	if( !worldmodel || !p || !pvolumes )
		return;	

	leaf = Mod_PointInLeaf( p, worldmodel->nodes );
	*(int *)pvolumes = *(int *)leaf->ambient_sound_level;
}

/*
================
Mod_FreeUserData
================
*/
static void Mod_FreeUserData( model_t *mod )
{
	// already freed?
	if( !mod || !mod->name[0] )
		return;

	if( clgame.drawFuncs.Mod_ProcessUserData != NULL )
	{
		// let the client.dll free custom data
		clgame.drawFuncs.Mod_ProcessUserData( mod, false, NULL );
	}
}

/*
================
Mod_FreeModel
================
*/
static void Mod_FreeModel( model_t *mod )
{
	// already freed?
	if( !mod || !mod->name[0] )
		return;

	Mod_FreeUserData( mod );

	// select the properly unloader
	switch( mod->type )
	{
	case mod_sprite:
		Mod_UnloadSpriteModel( mod );
		break;
	case mod_studio:
		Mod_UnloadStudioModel( mod );
		break;
	case mod_brush:
		Mod_UnloadBrushModel( mod );
		break;
	}
}

/*
===============================================================================

			MODEL INITALIZE\SHUTDOWN

===============================================================================
*/

void Mod_Init( void )
{
	com_studiocache = Mem_AllocPool( "Studio Cache" );
	mod_studiocache = Cvar_Get( "r_studiocache", "1", CVAR_ARCHIVE, "enables studio cache for speedup tracing hitboxes" );

	if( host.type == HOST_NORMAL )
		mod_allow_materials = Cvar_Get( "host_allow_materials", "0", CVAR_LATCH|CVAR_ARCHIVE, "allow HD textures" );
	else mod_allow_materials = NULL; // no reason to load HD-textures for dedicated server

	Cmd_AddCommand( "mapstats", Mod_PrintBSPFileSizes_f, "show stats for currently loaded map" );
	Cmd_AddCommand( "modellist", Mod_Modellist_f, "display loaded models list" );

	Mod_ResetStudioAPI ();
	Mod_InitStudioHull ();
}

void Mod_ClearAll( qboolean keep_playermodel )
{
	model_t	*plr = com_models[MAX_MODELS-1];
	model_t	*mod;
	int	i;

	for( i = 0, mod = cm_models; i < cm_nummodels; i++, mod++ )
	{
		if( keep_playermodel && mod == plr )
			continue;

		Mod_FreeModel( mod );
		memset( mod, 0, sizeof( *mod ));
	}

	// g-cont. may be just leave unchanged?
	if( !keep_playermodel ) cm_nummodels = 0;
}

void Mod_ClearUserData( void )
{
	int	i;

	for( i = 0; i < cm_nummodels; i++ )
		Mod_FreeUserData( &cm_models[i] );
}

void Mod_Shutdown( void )
{
	Mod_ClearAll( false );
	Mem_FreePool( &com_studiocache );
}

/*
===============================================================================

			MAP LOADING

===============================================================================
*/
/*
=================
Mod_LoadSubmodels
=================
*/
static void Mod_LoadSubmodels( const dlump_t *l )
{
	dmodel_t	*in;
	dmodel_t	*out;
	int	i, j, count;
	
	in = (void *)(mod_base + l->fileofs);
	if( l->filelen % sizeof( *in )) Host_Error( "Mod_LoadBModel: funny lump size\n" );
	count = l->filelen / sizeof( *in );

	if( count < 1 ) Host_Error( "Map %s without models\n", loadmodel->name );
	if( count > MAX_MAP_MODELS ) Host_Error( "Map %s has too many models\n", loadmodel->name );

	// allocate extradata
	out = Mem_Alloc( loadmodel->mempool, count * sizeof( *out ));
	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;
	if( world.loading ) world.max_surfaces = 0;

	for( i = 0; i < count; i++, in++, out++ )
	{
		for( j = 0; j < 3; j++ )
		{
			// spread the mins / maxs by a pixel
			out->mins[j] = in->mins[j] - 1.0f;
			out->maxs[j] = in->maxs[j] + 1.0f;
			out->origin[j] = in->origin[j];
		}

		for( j = 0; j < MAX_MAP_HULLS; j++ )
			out->headnode[j] = in->headnode[j];

		out->visleafs = in->visleafs;
		out->firstface = in->firstface;
		out->numfaces = in->numfaces;

		if( i == 0 || !world.loading )
			continue; // skip the world

		if( VectorIsNull( out->origin ))
		{
			// NOTE: zero origin after recalculating is indicated included origin brush
			VectorAverage( out->mins, out->maxs, out->origin );
		}

		world.max_surfaces = max( world.max_surfaces, out->numfaces ); 
	}

	if( world.loading )
		world.draw_surfaces = Mem_Alloc( loadmodel->mempool, world.max_surfaces * sizeof( msurface_t* ));
}

/*
=================
Mod_LoadTextures
=================
*/
static void Mod_LoadTextures( const dlump_t *l )
{
	dmiptexlump_t	*in;
	texture_t		*tx, *tx2;
	texture_t		*anims[10];
	texture_t		*altanims[10];
	int		num, max, altmax;
	char		texname[64];
	imgfilter_t	*filter;
	mip_t		*mt;
	int 		i, j; 

	if( world.loading )
	{
		// release old sky layers first
		GL_FreeTexture( tr.solidskyTexture );
		GL_FreeTexture( tr.alphaskyTexture );
		tr.solidskyTexture = tr.alphaskyTexture = 0;
		world.texdatasize = l->filelen;
		world.has_mirrors = false;
		world.sky_sphere = false;
	}

	if( !l->filelen )
	{
		// no textures
		loadmodel->textures = NULL;
		return;
	}

	in = (void *)(mod_base + l->fileofs);

	loadmodel->numtextures = in->nummiptex;
	loadmodel->textures = (texture_t **)Mem_Alloc( loadmodel->mempool, loadmodel->numtextures * sizeof( texture_t* ));

	for( i = 0; i < loadmodel->numtextures; i++ )
	{
		qboolean	load_external = false;
		qboolean	load_external_luma = false;

		if( in->dataofs[i] == -1 )
		{
			// create default texture (some mods requires this)
			tx = Mem_Alloc( loadmodel->mempool, sizeof( *tx ));
			loadmodel->textures[i] = tx;
		
			Q_strncpy( tx->name, "*default", sizeof( tx->name ));
			tx->gl_texturenum = tr.defaultTexture;
			tx->width = tx->height = 16;
			continue; // missed
		}

		mt = (mip_t *)((byte *)in + in->dataofs[i] );

		if( !mt->name[0] )
		{
			MsgDev( D_WARN, "unnamed texture in %s\n", loadmodel->name );
			Q_snprintf( mt->name, sizeof( mt->name ), "miptex_%i", i );
		}

		tx = Mem_Alloc( loadmodel->mempool, sizeof( *tx ));
		loadmodel->textures[i] = tx;

		// convert to lowercase
		Q_strnlwr( mt->name, mt->name, sizeof( mt->name ));
		Q_strncpy( tx->name, mt->name, sizeof( tx->name ));
		filter = R_FindTexFilter( tx->name ); // grab texture filter

		tx->width = mt->width;
		tx->height = mt->height;

		// check for multi-layered sky texture
		if( world.loading && !Q_strncmp( mt->name, "sky", 3 ) && mt->width == 256 && mt->height == 128 )
		{	
			if( Mod_AllowMaterials( ))
			{
				// build standard path: "materials/mapname/texname_solid.tga"
				Q_snprintf( texname, sizeof( texname ), "materials/%s/%s_solid.tga", modelname, mt->name );

				if( !FS_FileExists( texname, false ))
				{
					// build common path: "materials/mapname/texname_solid.tga"
					Q_snprintf( texname, sizeof( texname ), "materials/common/%s_solid.tga", mt->name );

					if( FS_FileExists( texname, false ))
						load_external = true;
				}
				else load_external = true;

				if( load_external )
				{
					tr.solidskyTexture = GL_LoadTexture( texname, NULL, 0, TF_UNCOMPRESSED|TF_NOMIPMAP, NULL );
					GL_SetTextureType( tr.solidskyTexture, TEX_BRUSH );
					load_external = false;
				}

				if( tr.solidskyTexture )
				{
					// build standard path: "materials/mapname/texname_alpha.tga"
					Q_snprintf( texname, sizeof( texname ), "materials/%s/%s_alpha.tga", modelname, mt->name );

					if( !FS_FileExists( texname, false ))
					{
						// build common path: "materials/mapname/texname_alpha.tga"
						Q_snprintf( texname, sizeof( texname ), "materials/common/%s_alpha.tga", mt->name );

						if( FS_FileExists( texname, false ))
							load_external = true;
					}
					else load_external = true;

					if( load_external )
					{
						tr.alphaskyTexture = GL_LoadTexture( texname, NULL, 0, TF_UNCOMPRESSED|TF_NOMIPMAP, NULL );
						GL_SetTextureType( tr.alphaskyTexture, TEX_BRUSH );
						load_external = false;
					}
				}

				if( !tr.solidskyTexture || !tr.alphaskyTexture )
				{
					// couldn't find one of layer
					GL_FreeTexture( tr.solidskyTexture );
					GL_FreeTexture( tr.alphaskyTexture );
					tr.solidskyTexture = tr.alphaskyTexture = 0;
				}
			}

			if( !tr.solidskyTexture && !tr.alphaskyTexture )
				R_InitSky( mt, tx ); // fallback to standard sky

			if( tr.solidskyTexture && tr.alphaskyTexture )
				world.sky_sphere = true;
		}
		else 
		{
			if( Mod_AllowMaterials( ))
			{
				if( mt->name[0] == '*' ) mt->name[0] = '!'; // replace unexpected symbol

				// build standard path: "materials/mapname/texname.tga"
				Q_snprintf( texname, sizeof( texname ), "materials/%s/%s.tga", modelname, mt->name );

				if( !FS_FileExists( texname, false ))
				{
					// build common path: "materials/mapname/texname.tga"
					Q_snprintf( texname, sizeof( texname ), "materials/common/%s.tga", mt->name );

					if( FS_FileExists( texname, false ))
						load_external = true;
				}
				else load_external = true;
			}
load_wad_textures:
			if( !load_external )
				Q_snprintf( texname, sizeof( texname ), "%s%s.mip", ( mt->offsets[0] > 0 ) ? "#" : "", mt->name );
			else MsgDev( D_NOTE, "loading HQ: %s\n", texname );

			if( mt->offsets[0] > 0 && !load_external )
			{
				// NOTE: imagelib detect miptex version by size
				// 770 additional bytes is indicated custom palette
				int size = (int)sizeof( mip_t ) + ((mt->width * mt->height * 85)>>6);
				if( bmodel_version >= HLBSP_VERSION ) size += sizeof( short ) + 768;

				tx->gl_texturenum = GL_LoadTexture( texname, (byte *)mt, size, 0, filter );
			}
			else
			{
				// okay, loading it from wad
				qboolean	fullpath_loaded = false;

				// check wads in reverse order
				for( j = wadlist.count - 1; j >= 0; j-- )
				{
					char	*texpath = va( "%s.wad/%s", wadlist.wadnames[j], texname );

					if( FS_FileExists( texpath, false ))
					{
						tx->gl_texturenum = GL_LoadTexture( texpath, NULL, 0, 0, filter );
						fullpath_loaded = true;
						break;
					}
				}

				if( !fullpath_loaded ) // probably this never happens
					tx->gl_texturenum = GL_LoadTexture( texname, NULL, 0, 0, filter );

				if( !tx->gl_texturenum && load_external )
				{
					// in case we failed to loading 32-bit texture
					MsgDev( D_ERROR, "Couldn't load %s\n", texname );
					load_external = false;
					goto load_wad_textures;
				}
			}
		}

		// set the emo-texture for missed
		if( !tx->gl_texturenum ) tx->gl_texturenum = tr.defaultTexture;

		if( load_external )
		{
			// build standard luma path: "materials/mapname/texname_luma.tga"
			Q_snprintf( texname, sizeof( texname ), "materials/%s/%s_luma.tga", modelname, mt->name );

			if( !FS_FileExists( texname, false ))
			{
				// build common path: "materials/mapname/texname_luma.tga"
				Q_snprintf( texname, sizeof( texname ), "materials/common/%s_luma.tga", mt->name );

				if( FS_FileExists( texname, false ))
					load_external_luma = true;
			}
			else load_external_luma = true;
		}

		// check for luma texture
		if( R_GetTexture( tx->gl_texturenum )->flags & TF_HAS_LUMA || load_external_luma )
		{
			if( !load_external_luma )
				Q_snprintf( texname, sizeof( texname ), "#%s_luma.mip", mt->name );
			else MsgDev( D_NOTE, "loading luma HQ: %s\n", texname );

			if( mt->offsets[0] > 0 && !load_external_luma )
			{
				// NOTE: imagelib detect miptex version by size
				// 770 additional bytes is indicated custom palette
				int size = (int)sizeof( mip_t ) + ((mt->width * mt->height * 85)>>6);
				if( bmodel_version >= HLBSP_VERSION ) size += sizeof( short ) + 768;

				tx->fb_texturenum = GL_LoadTexture( texname, (byte *)mt, size, TF_NOMIPMAP|TF_MAKELUMA, NULL );
			}
			else
			{
				size_t srcSize = 0;
				byte *src = NULL;

				// NOTE: we can't loading it from wad as normal because _luma texture doesn't exist
				// and not be loaded. But original texture is already loaded and can't be modified
				// So load original texture manually and convert it to luma
				if( !load_external_luma )
				{
					// check wads in reverse order
					for( j = wadlist.count - 1; j >= 0; j-- )
					{
						char	*texpath = va( "%s.wad/%s.mip", wadlist.wadnames[j], tx->name );

						if( FS_FileExists( texpath, false ))
						{
							src = FS_LoadFile( texpath, &srcSize, false );
							break;
						}
					}
				}

				// okay, loading it from wad or hi-res version
				tx->fb_texturenum = GL_LoadTexture( texname, src, srcSize, TF_NOMIPMAP|TF_MAKELUMA, NULL );
				if( src ) Mem_Free( src );

				if( !tx->fb_texturenum && load_external_luma )
				{
					// in case we failed to loading 32-bit luma texture
					MsgDev( D_ERROR, "Couldn't load %s\n", texname );
				} 
			}
		}

		if( tx->gl_texturenum != tr.defaultTexture )
		{
			// apply texture type (just for debug)
			GL_SetTextureType( tx->gl_texturenum, TEX_BRUSH );
			GL_SetTextureType( tx->fb_texturenum, TEX_BRUSH );
		}
	}

	// sequence the animations
	for( i = 0; i < loadmodel->numtextures; i++ )
	{
		tx = loadmodel->textures[i];

		if( !tx || tx->name[0] != '+' )
			continue;

		if( tx->anim_next )
			continue;	// allready sequenced

		// find the number of frames in the animation
		Q_memset( anims, 0, sizeof( anims ));
		Q_memset( altanims, 0, sizeof( altanims ));

		max = tx->name[1];
		altmax = 0;

		if( max >= '0' && max <= '9' )
		{
			max -= '0';
			altmax = 0;
			anims[max] = tx;
			max++;
		}
		else if( max >= 'a' && max <= 'j' )
		{
			altmax = max - 'a';
			max = 0;
			altanims[altmax] = tx;
			altmax++;
		}
		else Host_Error( "Mod_LoadTextures: bad animating texture %s\n", tx->name );

		for( j = i + 1; j < loadmodel->numtextures; j++ )
		{
			tx2 = loadmodel->textures[j];
			if( !tx2 || tx2->name[0] != '+' )
				continue;

			if( Q_strcmp( tx2->name + 2, tx->name + 2 ))
				continue;

			num = tx2->name[1];
			if( num >= '0' && num <= '9' )
			{
				num -= '0';
				anims[num] = tx2;
				if (num+1 > max)
					max = num + 1;
			}
			else if( num >= 'a' && num <= 'j' )
			{
				num = num - 'a';
				altanims[num] = tx2;
				if( num + 1 > altmax )
					altmax = num + 1;
			}
			else Host_Error( "Mod_LoadTextures: bad animating texture %s\n", tx->name );
		}

		// link them all together
		for( j = 0; j < max; j++ )
		{
			tx2 = anims[j];
			if( !tx2 ) Host_Error( "Mod_LoadTextures: missing frame %i of %s\n", j, tx->name );
			tx2->anim_total = max * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j + 1) * ANIM_CYCLE;
			tx2->anim_next = anims[(j + 1) % max];
			if( altmax ) tx2->alternate_anims = altanims[0];
		}

		for( j = 0; j < altmax; j++ )
		{
			tx2 = altanims[j];
			if( !tx2 ) Host_Error( "Mod_LoadTextures: missing frame %i of %s\n", j, tx->name );
			tx2->anim_total = altmax * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j+1) * ANIM_CYCLE;
			tx2->anim_next = altanims[(j + 1) % altmax];
			if( max ) tx2->alternate_anims = anims[0];
		}
	}

	// sequence the detail textures
	for( i = 0; i < loadmodel->numtextures; i++ )
	{
		tx = loadmodel->textures[i];

		if( !tx || tx->name[0] != '-' )
			continue;

		if( tx->anim_next )
			continue;	// allready sequenced

		// find the number of frames in the sequence
		Q_memset( anims, 0, sizeof( anims ));

		max = tx->name[1];

		if( max >= '0' && max <= '9' )
		{
			max -= '0';
			anims[max] = tx;
			max++;
		}
		else Host_Error( "Mod_LoadTextures: bad detail texture %s\n", tx->name );

		for( j = i + 1; j < loadmodel->numtextures; j++ )
		{
			tx2 = loadmodel->textures[j];
			if( !tx2 || tx2->name[0] != '-' )
				continue;

			if( Q_strcmp( tx2->name + 2, tx->name + 2 ))
				continue;

			num = tx2->name[1];
			if( num >= '0' && num <= '9' )
			{
				num -= '0';
				anims[num] = tx2;
				if( num+1 > max )
					max = num + 1;
			}
			else Host_Error( "Mod_LoadTextures: bad detail texture %s\n", tx->name );
		}

		// link them all together
		for( j = 0; j < max; j++ )
		{
			tx2 = anims[j];
			if( !tx2 ) Host_Error( "Mod_LoadTextures: missing frame %i of %s\n", j, tx->name );
			tx2->anim_total = -( max * ANIM_CYCLE ); // to differentiate from animations
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j + 1) * ANIM_CYCLE;
			tx2->anim_next = anims[(j + 1) % max];
		}	
	}
}

/*
=================
Mod_LoadTexInfo
=================
*/
static void Mod_LoadTexInfo( const dlump_t *l )
{
	dtexinfo_t	*in;
	mtexinfo_t	*out;
	int		miptex;
	int		i, j, count;
	float		len1, len2;
	
	in = (void *)(mod_base + l->fileofs);
	if( l->filelen % sizeof( *in ))
		Host_Error( "Mod_LoadTexInfo: funny lump size in %s\n", loadmodel->name );

	count = l->filelen / sizeof( *in );
          out = Mem_Alloc( loadmodel->mempool, count * sizeof( *out ));
	
	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		for( j = 0; j < 8; j++ )
			out->vecs[0][j] = in->vecs[0][j];

		len1 = VectorLength( out->vecs[0] );
		len2 = VectorLength( out->vecs[1] );
		len1 = ( len1 + len2 ) / 2;

		// g-cont: can use this info for GL_TEXTURE_LOAD_BIAS_EXT ?
		if( len1 < 0.32f ) out->mipadjust = 4;
		else if( len1 < 0.49f ) out->mipadjust = 3;
		else if( len1 < 0.99f ) out->mipadjust = 2;
		else out->mipadjust = 1;

		miptex = in->miptex;
		if( miptex < 0 || miptex > loadmodel->numtextures )
			Host_Error( "Mod_LoadTexInfo: bad miptex number in '%s'\n", loadmodel->name );

		out->texture = loadmodel->textures[miptex];
		out->flags = in->flags;
	}
}

/*
=================
Mod_LoadDeluxemap
=================
*/
static void Mod_LoadDeluxemap( void )
{
	char	path[64];
	int	iCompare;
	byte	*in;

	if( !world.loading ) return;	// only world can have deluxedata

	if( !( host.features & ENGINE_LOAD_DELUXEDATA ))
	{
		world.deluxedata = NULL;
		world.vecdatasize = 0;
		return;
	}

	Q_snprintf( path, sizeof( path ), "maps/%s.dlit", modelname );

	// make sure what deluxemap is actual
	if( !COM_CompareFileTime( path, loadmodel->name, &iCompare ))
	{
		world.deluxedata = NULL;
		world.vecdatasize = 0;
		return;
          }

	if( iCompare < 0 ) // this may happens if level-designer used -onlyents key for hlcsg
		MsgDev( D_WARN, "Mod_LoadDeluxemap: %s probably is out of date\n", path );

	in = FS_LoadFile( path, &world.vecdatasize, true );

	ASSERT( in != NULL );

	if( *(uint *)in != IDDELUXEMAPHEADER || *((uint *)in + 1) != DELUXEMAP_VERSION )
	{
		MsgDev( D_ERROR, "Mod_LoadDeluxemap: %s is not a deluxemap file\n", path );
		world.deluxedata = NULL;
		world.vecdatasize = 0;
		Mem_Free( in );
		return;
	}

	// skip header bytes
	world.vecdatasize -= 8;

	if( world.vecdatasize != world.litdatasize )
	{
		MsgDev( D_ERROR, "Mod_LoadDeluxemap: %s has mismatched size (%i should be %i)\n", path, world.vecdatasize, world.litdatasize );
		world.deluxedata = NULL;
		world.vecdatasize = 0;
		Mem_Free( in );
		return;
	}

	MsgDev( D_INFO, "Mod_LoadDeluxemap: %s loaded\n", path );
	world.deluxedata = Mem_Alloc( loadmodel->mempool, world.vecdatasize );
	Q_memcpy( world.deluxedata, in + 8, world.vecdatasize );
	Mem_Free( in );
}

/*
=================
Mod_LoadLighting
=================
*/
static void Mod_LoadLighting( const dlump_t *l )
{
	byte	d, *in;
	color24	*out;
	int	i;

	if( !l->filelen )
	{
		if( world.loading )
		{
			MsgDev( D_WARN, "map ^2%s^7 has no lighting\n", loadmodel->name );
			loadmodel->lightdata = NULL;
			world.deluxedata = NULL;
			world.vecdatasize = 0;
			world.litdatasize = 0;
		}
		return;
	}
	in = (void *)(mod_base + l->fileofs);
	if( world.loading ) world.litdatasize = l->filelen;

	switch( bmodel_version )
	{
	case Q1BSP_VERSION:
		// expand the white lighting data
		loadmodel->lightdata = (color24 *)Mem_Alloc( loadmodel->mempool, l->filelen * sizeof( color24 ));
		out = loadmodel->lightdata;

		for( i = 0; i < l->filelen; i++, out++ )
		{
			d = *in++;
			out->r = d;
			out->g = d;
			out->b = d;
		}
		break;
	case HLBSP_VERSION:
	case XTBSP_VERSION:
		// load colored lighting
		loadmodel->lightdata = Mem_Alloc( loadmodel->mempool, l->filelen );
		Q_memcpy( loadmodel->lightdata, in, l->filelen );
		break;
	}

	// try to loading deluxemap too
	Mod_LoadDeluxemap ();
}

/*
=================
Mod_CalcSurfaceExtents

Fills in surf->texturemins[] and surf->extents[]
=================
*/
static void Mod_CalcSurfaceExtents( msurface_t *surf )
{
	float		mins[2], maxs[2], val;
	int		bmins[2], bmaxs[2];
	int		i, j, e;
	mtexinfo_t	*tex;
	mvertex_t		*v;

	tex = surf->texinfo;

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -999999;

	for( i = 0; i < surf->numedges; i++ )
	{
		e = loadmodel->surfedges[surf->firstedge + i];

		if( e >= loadmodel->numedges || e <= -loadmodel->numedges )
			Host_Error( "Mod_CalcSurfaceBounds: bad edge\n" );

		if( e >= 0 ) v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for( j = 0; j < 2; j++ )
		{
			val = DotProduct( v->position, surf->texinfo->vecs[j] ) + surf->texinfo->vecs[j][3];
			if( val < mins[j] ) mins[j] = val;
			if( val > maxs[j] ) maxs[j] = val;
		}
	}

	for( i = 0; i < 2; i++ )
	{
		bmins[i] = floor( mins[i] / LM_SAMPLE_SIZE );
		bmaxs[i] = ceil( maxs[i] / LM_SAMPLE_SIZE );

		surf->texturemins[i] = bmins[i] * LM_SAMPLE_SIZE;
		surf->extents[i] = (bmaxs[i] - bmins[i]) * LM_SAMPLE_SIZE;

		if(!( tex->flags & TEX_SPECIAL ) && surf->extents[i] > 4096 )
			MsgDev( D_ERROR, "Bad surface extents %i\n", surf->extents[i] );
	}
}

/*
=================
Mod_CalcSurfaceBounds

fills in surf->mins and surf->maxs
=================
*/
static void Mod_CalcSurfaceBounds( msurface_t *surf, mextrasurf_t *info )
{
	int	i, e;
	mvertex_t	*v;

	ClearBounds( info->mins, info->maxs );

	for( i = 0; i < surf->numedges; i++ )
	{
		e = loadmodel->surfedges[surf->firstedge + i];

		if( e >= loadmodel->numedges || e <= -loadmodel->numedges )
			Host_Error( "Mod_CalcSurfaceBounds: bad edge\n" );

		if( e >= 0 ) v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];
		AddPointToBounds( v->position, info->mins, info->maxs );
	}

	VectorAverage( info->mins, info->maxs, info->origin );
}

/*
=================
Mod_BuildPolygon
=================
*/
static void Mod_BuildPolygon( mextrasurf_t *info, msurface_t *surf, int numVerts, const float *verts )
{
	float		s, t;
	uint		bufSize;
	vec3_t		normal, tangent, binormal;
	mtexinfo_t	*texinfo = surf->texinfo;
	int		i, numElems;
	byte		*buffer;
	msurfmesh_t	*mesh;

	// allocate mesh
	numElems = (numVerts - 2) * 3;

	bufSize = sizeof( msurfmesh_t ) + numVerts * sizeof( glvert_t ) + numElems * sizeof( word );
	buffer = Mem_Alloc( loadmodel->mempool, bufSize );

	mesh = (msurfmesh_t *)buffer;
	buffer += sizeof( msurfmesh_t );
	mesh->numVerts = numVerts;
	mesh->numElems = numElems;

	// calc tangent space
	if( surf->flags & SURF_PLANEBACK )
		VectorNegate( surf->plane->normal, normal );
	else VectorCopy( surf->plane->normal, normal );
	VectorCopy( surf->texinfo->vecs[0], tangent );
	VectorNegate( surf->texinfo->vecs[1], binormal );

	VectorNormalize( normal ); // g-cont. this is even needed?
	VectorNormalize( tangent );
	VectorNormalize( binormal );

	// setup pointers
	mesh->verts = (glvert_t *)buffer;
	buffer += numVerts * sizeof( glvert_t );
	mesh->elems = (word *)buffer;
	buffer += numElems * sizeof( word );

	mesh->next = info->mesh;
	mesh->surf = surf;	// NOTE: meshchains can be linked with one surface
	info->mesh = mesh;

	// create indices
	for( i = 0; i < mesh->numVerts - 2; i++ )
	{
		mesh->elems[i*3+0] = 0;
		mesh->elems[i*3+1] = i + 1;
		mesh->elems[i*3+2] = i + 2;
	}

	for( i = 0; i < numVerts; i++, verts += 3 )
	{
		glvert_t	*out = &mesh->verts[i];

		// vertex
		VectorCopy( verts, out->vertex );
		VectorCopy( tangent, out->tangent );
		VectorCopy( binormal, out->binormal );
		VectorCopy( normal, out->normal );

		// texture coordinates
		s = DotProduct( verts, texinfo->vecs[0] ) + texinfo->vecs[0][3];
		s /= texinfo->texture->width;

		t = DotProduct( verts, texinfo->vecs[1] ) + texinfo->vecs[1][3];
		t /= texinfo->texture->height;

		out->stcoord[0] = s;
		out->stcoord[1] = t;

		// lightmap texture coordinates
		s = DotProduct( verts, texinfo->vecs[0] ) + texinfo->vecs[0][3] - surf->texturemins[0];
		s += surf->light_s * LM_SAMPLE_SIZE;
		s += LM_SAMPLE_SIZE >> 1;
		s /= BLOCK_SIZE * LM_SAMPLE_SIZE;

		t = DotProduct( verts, texinfo->vecs[1] ) + texinfo->vecs[1][3] - surf->texturemins[1];
		t += surf->light_t * LM_SAMPLE_SIZE;
		t += LM_SAMPLE_SIZE >> 1;
		t /= BLOCK_SIZE * LM_SAMPLE_SIZE;

		out->lmcoord[0] = s;
		out->lmcoord[1] = t;

		// clear colors (it can be used for vertex lighting)
		Q_memset( out->color, 0xFF, sizeof( out->color ));
	}
}

/*
=================
Mod_SubdividePolygon
=================
*/
static void Mod_SubdividePolygon( mextrasurf_t *info, msurface_t *surf, int numVerts, float *verts, float tessSize )
{
	vec3_t		vTotal, nTotal, tTotal, bTotal;
	vec3_t		front[MAX_SIDE_VERTS], back[MAX_SIDE_VERTS];
	float		*v, m, oneDivVerts, dist, dists[MAX_SIDE_VERTS];
	qboolean		lightmap = (surf->flags & SURF_DRAWTILED) ? false : true;
	vec3_t		normal, tangent, binormal, mins, maxs;
	mtexinfo_t	*texinfo = surf->texinfo;
	vec2_t		totalST, totalLM;
	float		s, t, scale;
	int		i, j, f, b;
	uint		bufSize;
	byte		*buffer;
	msurfmesh_t	*mesh;

	ClearBounds( mins, maxs );

	for( i = 0, v = verts; i < numVerts; i++, v += 3 )
		AddPointToBounds( v, mins, maxs );

	for( i = 0; i < 3; i++ )
	{
		m = tessSize * (float)floor((( mins[i] + maxs[i] ) * 0.5f ) / tessSize + 0.5f );

		if( maxs[i] - m < 8.0f ) continue;
		if( m - mins[i] < 8.0f ) continue;

		// cut it
		for( j = 0, v = verts + i; j < numVerts; j++, v += 3 )
			dists[j] = *v - m;

		// wrap cases
		dists[j] = dists[0];
		v -= i;

		VectorCopy( verts, v );

		for( f = j = b = 0, v = verts; j < numVerts; j++, v += 3 )
		{
			if( dists[j] >= 0.0f )
			{
				VectorCopy( v, front[f] );
				f++;
			}

			if( dists[j] <= 0.0f )
			{
				VectorCopy( v, back[b] );
				b++;
			}
			
			if( dists[j] == 0.0f || dists[j+1] == 0.0f )
				continue;
			
			if(( dists[j] > 0.0f ) != ( dists[j+1] > 0.0f ))
			{
				// clip point
				dist = dists[j] / (dists[j] - dists[j+1]);
				front[f][0] = back[b][0] = v[0] + (v[3] - v[0]) * dist;
				front[f][1] = back[b][1] = v[1] + (v[4] - v[1]) * dist;
				front[f][2] = back[b][2] = v[2] + (v[5] - v[2]) * dist;
				f++, b++;
			}
		}

		Mod_SubdividePolygon( info, surf, f, front[0], tessSize );
		Mod_SubdividePolygon( info, surf, b, back[0], tessSize );
		return;
	}

	bufSize = sizeof( msurfmesh_t ) + ( numVerts + 2 ) * sizeof( glvert_t ); // temp buffer has no indices
	buffer = Mem_Alloc( loadmodel->mempool, bufSize );

	mesh = (msurfmesh_t *)buffer;
	buffer += sizeof( msurfmesh_t );

	// create vertices
	mesh->numVerts = numVerts + 2;
	mesh->numElems = numVerts * 3;

	// calc tangent space
	if( surf->flags & SURF_PLANEBACK )
		VectorNegate( surf->plane->normal, normal );
	else VectorCopy( surf->plane->normal, normal );
	VectorCopy( surf->texinfo->vecs[0], tangent );
	VectorNegate( surf->texinfo->vecs[1], binormal );

	VectorNormalize( normal ); // g-cont. this is even needed?
	VectorNormalize( tangent );
	VectorNormalize( binormal );

	// setup pointers
	mesh->verts = (glvert_t *)buffer;
	buffer += numVerts * sizeof( glvert_t );

	VectorClear( vTotal );
	VectorClear( nTotal );
	VectorClear( bTotal );
	VectorClear( tTotal );

	totalST[0] = totalST[1] = 0;
	totalLM[0] = totalLM[1] = 0;

	scale = ( 1.0f / tessSize );

	for( i = 0; i < numVerts; i++, verts += 3 )
	{
		glvert_t	*out = &mesh->verts[i+1];

		// vertex
		VectorCopy( verts, out->vertex );
		VectorCopy( normal, out->normal );
		VectorCopy( tangent, out->tangent );
		VectorCopy( binormal, out->binormal );

		VectorAdd( vTotal, verts, vTotal );
 		VectorAdd( nTotal, normal, nTotal );
 		VectorAdd( tTotal, tangent, tTotal );
 		VectorAdd( bTotal, binormal, bTotal );

		if( lightmap )
		{
			// texture coordinates
			s = DotProduct( verts, texinfo->vecs[0] ) + texinfo->vecs[0][3];
			s /= texinfo->texture->width;

			t = DotProduct( verts, texinfo->vecs[1] ) + texinfo->vecs[1][3];
			t /= texinfo->texture->height;
		}
		else
		{
			// texture coordinates
			s = DotProduct( verts, texinfo->vecs[0] ) * scale;
			t = DotProduct( verts, texinfo->vecs[1] ) * scale;
		}

		out->stcoord[0] = s;
		out->stcoord[1] = t;

		totalST[0] += s;
		totalST[1] += t;

		if( lightmap )
		{
			// lightmap texture coordinates
			s = DotProduct( verts, texinfo->vecs[0] ) + texinfo->vecs[0][3] - surf->texturemins[0];
			s += surf->light_s * LM_SAMPLE_SIZE;
			s += LM_SAMPLE_SIZE >> 1;
			s /= BLOCK_SIZE * LM_SAMPLE_SIZE;

			t = DotProduct( verts, texinfo->vecs[1] ) + texinfo->vecs[1][3] - surf->texturemins[1];
			t += surf->light_t * LM_SAMPLE_SIZE;
			t += LM_SAMPLE_SIZE >> 1;
			t /= BLOCK_SIZE * LM_SAMPLE_SIZE;
		}
		else
		{
			s = t = 0.0f;
		}

		out->lmcoord[0] = s;
		out->lmcoord[1] = t;

		totalLM[0] += s;
		totalLM[1] += t;

		// clear colors (it can be used for vertex lighting)
		Q_memset( out->color, 0xFF, sizeof( out->color ));
	}

	// vertex
	oneDivVerts = ( 1.0f / (float)numVerts );

	VectorScale( vTotal, oneDivVerts, mesh->verts[0].vertex );
	VectorScale( nTotal, oneDivVerts, mesh->verts[0].normal );
	VectorScale( tTotal, oneDivVerts, mesh->verts[0].tangent );
	VectorScale( bTotal, oneDivVerts, mesh->verts[0].binormal );

	VectorNormalize( mesh->verts[0].normal );
	VectorNormalize( mesh->verts[0].tangent );
	VectorNormalize( mesh->verts[0].binormal );

	// texture coordinates
	mesh->verts[0].stcoord[0] = totalST[0] * oneDivVerts;
	mesh->verts[0].stcoord[1] = totalST[1] * oneDivVerts;

	// lightmap texture coordinates
	mesh->verts[0].lmcoord[0] = totalLM[0] * oneDivVerts;
	mesh->verts[0].lmcoord[1] = totalLM[1] * oneDivVerts;

	// copy first vertex to last
	Q_memcpy( &mesh->verts[i+1], &mesh->verts[1], sizeof( glvert_t ));

	mesh->next = info->mesh;
	mesh->surf = surf;	// NOTE: meshchains can be linked with one surface
	info->mesh = mesh;
}

/*
================
Mod_ConvertSurface

turn the polychain into one subdivided surface
================
*/
static void Mod_ConvertSurface( mextrasurf_t *info, msurface_t *surf )
{
	msurfmesh_t	*poly, *next, *mesh;
	int		numElems, numVerts;
	glvert_t		*outVerts;
	word		*outElems;
	int		i, bufSize;
	byte		*buffer;

	// find the total vertex count and index count
	numElems = numVerts = 0;

	// determine count of indices and vertices
	for( poly = info->mesh; poly; poly = poly->next )
	{
		numElems += ( poly->numVerts - 2 ) * 3;
		numVerts += poly->numVerts;
	}

	// unsigned short limit
	if( numVerts >= 65536 ) Host_Error( "Mod_ConvertSurface: vertex count %i exceeds 65535\n", numVerts );
	if( numElems >= 65536 ) Host_Error( "Mod_ConvertSurface: index count %i exceeds 65535\n", numElems );

	bufSize = sizeof( msurfmesh_t ) + numVerts * sizeof( glvert_t ) + numElems * sizeof( word );
	buffer = Mem_Alloc( loadmodel->mempool, bufSize );

	mesh = (msurfmesh_t *)buffer;
	buffer += sizeof( msurfmesh_t );

	mesh->numVerts = numVerts;
	mesh->numElems = numElems;

	// setup pointers
	mesh->verts = (glvert_t *)buffer;
	buffer += numVerts * sizeof( glvert_t );
	mesh->elems = (word *)buffer;
	buffer += numElems * sizeof( word );

	// setup moving pointers
	outVerts = (glvert_t *)mesh->verts;
	outElems = (word *)mesh->elems;

	// store vertex data
	numElems = numVerts = 0;

	for( poly = info->mesh; poly; poly = poly->next )
	{
		// indexes
		outElems = mesh->elems + numElems;
		outVerts = mesh->verts + numVerts;

		for( i = 0; i < poly->numVerts - 2; i++ )
		{
			outElems[i*3+0] = numVerts;
			outElems[i*3+1] = numVerts + i + 1;
			outElems[i*3+2] = numVerts + i + 2;
		}

		Q_memcpy( outVerts, poly->verts, sizeof( glvert_t ) * poly->numVerts );

		numElems += (poly->numVerts - 2) * 3;
		numVerts += poly->numVerts;
	}

	// release the old polys crap
	for( poly = info->mesh; poly; poly = next )
	{
		next = poly->next;
		Mem_Free( poly );
	}

	ASSERT( mesh->numVerts == numVerts );
	ASSERT( mesh->numElems == numElems );

	mesh->next = info->mesh;
	mesh->surf = surf;	// NOTE: meshchains can be linked with one surface
	info->mesh = mesh;
}

/*
=================
Mod_BuildSurfacePolygons
=================
*/
void Mod_BuildSurfacePolygons( msurface_t *surf, mextrasurf_t *info )
{
	vec3_t	verts[MAX_SIDE_VERTS];
	char	*texname;
	int	i, e;
	mvertex_t	*v;

	if( info->mesh ) return; // already exist

	// convert edges back to a normal polygon
	for( i = 0; i < surf->numedges; i++ )
	{
		if( i == MAX_SIDE_VERTS )
		{
			MsgDev( D_ERROR, "BuildSurfMesh: poly %i exceeded %i vertexes!\n", surf - loadmodel->surfaces, MAX_SIDE_VERTS );
			break; // too big polygon ?
		}

		e = loadmodel->surfedges[surf->firstedge + i];
		if( e > 0 ) v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];
		VectorCopy( v->position, verts[i] );
	}

	// subdivide water or sky sphere for Quake1 maps
	if(( surf->flags & SURF_DRAWTURB && !( surf->flags & SURF_REFLECT )) || ( surf->flags & SURF_DRAWSKY && world.loading && world.sky_sphere ))
	{
		Mod_SubdividePolygon( info, surf, surf->numedges, verts[0], 64.0f );
		Mod_ConvertSurface( info, surf );
	}
	else
	{
		Mod_BuildPolygon( info, surf, surf->numedges, verts[0] );
	}

	if( info->mesh ) return;	// all done

	if( surf->texinfo && surf->texinfo->texture )
		texname = surf->texinfo->texture->name;
	else texname = "notexture";

	MsgDev( D_ERROR, "BuildSurfMesh: surface %i (%s) failed to build surfmesh\n", surf - loadmodel->surfaces, texname );
}

/*
=================
Mod_TesselatePolygon

tesselate specified polygon
by user request
=================
*/
void Mod_TesselatePolygon( msurface_t *surf, model_t *mod, float tessSize )
{
	mextrasurf_t	*info;
	model_t		*old = loadmodel;
	vec3_t		verts[MAX_SIDE_VERTS];
	char		*texname;
	int		i, e;
	mvertex_t		*v;

	if( !surf || !mod ) return; // bad arguments?

	tessSize = bound( 8.0f, tessSize, 256.0f );
	info = SURF_INFO( surf, mod );
	loadmodel = mod;

	// release old mesh
	if( info->mesh )
	{
		Mem_Free( info->mesh );
		info->mesh = NULL;
	}

	// convert edges back to a normal polygon
	for( i = 0; i < surf->numedges; i++ )
	{
		if( i == MAX_SIDE_VERTS )
		{
			MsgDev( D_ERROR, "BuildSurfMesh: poly %i exceeded %i vertexes!\n", surf - loadmodel->surfaces, MAX_SIDE_VERTS );
			break; // too big polygon ?
		}

		e = loadmodel->surfedges[surf->firstedge + i];
		if( e > 0 ) v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];
		VectorCopy( v->position, verts[i] );
	}

	Mod_SubdividePolygon( info, surf, surf->numedges, verts[0], tessSize );
	Mod_ConvertSurface( info, surf );

	// restore loadmodel value
	loadmodel = old;

	if( info->mesh ) return;	// all done

	if( surf->texinfo && surf->texinfo->texture )
		texname = surf->texinfo->texture->name;
	else texname = "notexture";

	MsgDev( D_ERROR, "BuildSurfMesh: surface %i (%s) failed to build surfmesh\n", surf - loadmodel->surfaces, texname );
}

/*
=================
Mod_LoadSurfaces
=================
*/
static void Mod_LoadSurfaces( const dlump_t *l )
{
	dface_t		*in;
	msurface_t	*out;
	mextrasurf_t	*info;
	int		i, j;
	int		count;

	in = (void *)(mod_base + l->fileofs);
	if( l->filelen % sizeof( *in ))
		Host_Error( "Mod_LoadSurfaces: funny lump size in '%s'\n", loadmodel->name );
	count = l->filelen / sizeof( *in );

	loadmodel->numsurfaces = count;
	loadmodel->surfaces = Mem_Alloc( loadmodel->mempool, count * sizeof( msurface_t ));
	loadmodel->cache.data =  Mem_Alloc( loadmodel->mempool, count * sizeof( mextrasurf_t ));
	out = loadmodel->surfaces;
	info = loadmodel->cache.data;

	for( i = 0; i < count; i++, in++, out++, info++ )
	{
		texture_t	*tex;

		if(( in->firstedge + in->numedges ) > loadmodel->numsurfedges )
		{
			MsgDev( D_ERROR, "Bad surface %i from %i\n", i, count );
			continue;
		} 

		out->firstedge = in->firstedge;
		out->numedges = in->numedges;
		out->flags = 0;

		if( in->side ) out->flags |= SURF_PLANEBACK;
		out->plane = loadmodel->planes + in->planenum;
		out->texinfo = loadmodel->texinfo + in->texinfo;

		tex = out->texinfo->texture;

		if( !Q_strncmp( tex->name, "sky", 3 ))
			out->flags |= (SURF_DRAWTILED|SURF_DRAWSKY);

		if(( tex->name[0] == '*' && Q_stricmp( tex->name, "*default" )) || tex->name[0] == '!' )
		{
			out->flags |= (SURF_DRAWTURB|SURF_DRAWTILED);

			if( !( host.features & ENGINE_BUILD_SURFMESHES ))
				out->flags |= SURF_NOCULL;
		}

		if( !Q_strncmp( tex->name, "water", 5 ) || !Q_strnicmp( tex->name, "laser", 5 ))
			out->flags |= (SURF_DRAWTURB|SURF_DRAWTILED|SURF_NOCULL);

		if( !Q_strncmp( tex->name, "scroll", 6 ))
			out->flags |= SURF_CONVEYOR;

		// g-cont. added a combined conveyor-transparent
		if( !Q_strncmp( tex->name, "{scroll", 7 ))
			out->flags |= SURF_CONVEYOR|SURF_TRANSPARENT;

		// g-cont this texture from decals.wad he-he
		// support !reflect for reflected water
		if( !Q_strncmp( tex->name, "reflect", 7 ) || !Q_strncmp( tex->name, "!reflect", 8 ))
		{
			out->flags |= SURF_REFLECT;
			world.has_mirrors = true;
		}

		if( tex->name[0] == '{' )
			out->flags |= SURF_TRANSPARENT;

		if( out->texinfo->flags & TEX_SPECIAL )
			out->flags |= SURF_DRAWTILED;

		Mod_CalcSurfaceBounds( out, info );
		Mod_CalcSurfaceExtents( out );

		if( loadmodel->lightdata && in->lightofs != -1 )
		{
			if( bmodel_version >= HLBSP_VERSION )
				out->samples = loadmodel->lightdata + (in->lightofs / 3);
			else out->samples = loadmodel->lightdata + in->lightofs;

			// if deluxemap is present setup it too
			if( world.deluxedata )
				info->deluxemap = world.deluxedata + (in->lightofs / 3);
		}

		for( j = 0; j < MAXLIGHTMAPS; j++ )
			out->styles[j] = in->styles[j];

		// build polygons for non-lightmapped surfaces
		if( host.features & ENGINE_BUILD_SURFMESHES && (( out->flags & SURF_DRAWTILED ) || !out->samples ))
			Mod_BuildSurfacePolygons( out, info );

		if( out->flags & SURF_DRAWSKY && world.loading && world.sky_sphere )
			GL_SubdivideSurface( out ); // cut up polygon for warps

		if( out->flags & SURF_DRAWTURB )
			GL_SubdivideSurface( out ); // cut up polygon for warps
	}
}

/*
=================
Mod_LoadVertexes
=================
*/
static void Mod_LoadVertexes( const dlump_t *l )
{
	dvertex_t	*in;
	mvertex_t	*out;
	int	i, count;

	in = (void *)( mod_base + l->fileofs );
	if( l->filelen % sizeof( *in ))
		Host_Error( "Mod_LoadVertexes: funny lump size in %s\n", loadmodel->name );
	count = l->filelen / sizeof( *in );

	loadmodel->numvertexes = count;
	out = loadmodel->vertexes = Mem_Alloc( loadmodel->mempool, count * sizeof( mvertex_t ));

	if( world.loading ) ClearBounds( world.mins, world.maxs );

	for( i = 0; i < count; i++, in++, out++ )
	{
		VectorCopy( in->point, out->position );
		if( world.loading ) AddPointToBounds( in->point, world.mins, world.maxs );
	}

	if( !world.loading ) return;

	VectorSubtract( world.maxs, world.mins, world.size );

	for( i = 0; i < 3; i++ )
	{
		// spread the mins / maxs by a pixel
		world.mins[i] -= 1.0f;
		world.maxs[i] += 1.0f;
	}
}

/*
=================
Mod_LoadEdges
=================
*/
static void Mod_LoadEdges( const dlump_t *l )
{
	dedge_t	*in;
	medge_t	*out;
	int	i, count;

	in = (void *)( mod_base + l->fileofs );	
	if( l->filelen % sizeof( *in ))
		Host_Error( "Mod_LoadEdges: funny lump size in %s\n", loadmodel->name );

	count = l->filelen / sizeof( *in );
	loadmodel->edges = out = Mem_Alloc( loadmodel->mempool, count * sizeof( medge_t ));
	loadmodel->numedges = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		out->v[0] = (word)in->v[0];
		out->v[1] = (word)in->v[1];
	}
}

/*
=================
Mod_LoadSurfEdges
=================
*/
static void Mod_LoadSurfEdges( const dlump_t *l )
{
	dsurfedge_t	*in;
	int		count;

	in = (void *)( mod_base + l->fileofs );	
	if( l->filelen % sizeof( *in ))
		Host_Error( "Mod_LoadSurfEdges: funny lump size in %s\n", loadmodel->name );

	count = l->filelen / sizeof( dsurfedge_t );
	loadmodel->surfedges = Mem_Alloc( loadmodel->mempool, count * sizeof( dsurfedge_t ));
	loadmodel->numsurfedges = count;

	Q_memcpy( loadmodel->surfedges, in, count * sizeof( dsurfedge_t ));
}

/*
=================
Mod_LoadMarkSurfaces
=================
*/
static void Mod_LoadMarkSurfaces( const dlump_t *l )
{
	dmarkface_t	*in;
	int		i, j, count;
	msurface_t	**out;
	
	in = (void *)( mod_base + l->fileofs );	
	if( l->filelen % sizeof( *in ))
		Host_Error( "Mod_LoadMarkFaces: funny lump size in %s\n", loadmodel->name );

	count = l->filelen / sizeof( *in );
	loadmodel->marksurfaces = out = Mem_Alloc( loadmodel->mempool, count * sizeof( *out ));
	loadmodel->nummarksurfaces = count;

	for( i = 0; i < count; i++ )
	{
		j = in[i];
		if( j < 0 ||  j >= loadmodel->numsurfaces )
			Host_Error( "Mod_LoadMarkFaces: bad surface number in '%s'\n", loadmodel->name );
		out[i] = loadmodel->surfaces + j;
	}
}

/*
=================
Mod_SetParent
=================
*/
static void Mod_SetParent( mnode_t *node, mnode_t *parent )
{
	node->parent = parent;

	if( node->contents < 0 ) return; // it's node
	Mod_SetParent( node->children[0], node );
	Mod_SetParent( node->children[1], node );
}

/*
=================
Mod_LoadNodes
=================
*/
static void Mod_LoadNodes( const dlump_t *l )
{
	dnode_t	*in;
	mnode_t	*out;
	int	i, j, p;
	
	in = (void *)(mod_base + l->fileofs);
	if( l->filelen % sizeof( *in )) Host_Error( "Mod_LoadNodes: funny lump size\n" );
	loadmodel->numnodes = l->filelen / sizeof( *in );

	if( loadmodel->numnodes < 1 ) Host_Error( "Map %s has no nodes\n", loadmodel->name );
	out = loadmodel->nodes = (mnode_t *)Mem_Alloc( loadmodel->mempool, loadmodel->numnodes * sizeof( *out ));

	for( i = 0; i < loadmodel->numnodes; i++, out++, in++ )
	{
		for( j = 0; j < 3; j++ )
		{
			out->minmaxs[j] = in->mins[j];
			out->minmaxs[3+j] = in->maxs[j];
		}

		p = in->planenum;
		out->plane = loadmodel->planes + p;
		out->firstsurface = in->firstface;
		out->numsurfaces = in->numfaces;

		for( j = 0; j < 2; j++ )
		{
			p = in->children[j];
			if( p >= 0 ) out->children[j] = loadmodel->nodes + p;
			else out->children[j] = (mnode_t *)(loadmodel->leafs + ( -1 - p ));
		}
	}

	// sets nodes and leafs
	Mod_SetParent( loadmodel->nodes, NULL );
}

/*
=================
Mod_LoadLeafs
=================
*/
static void Mod_LoadLeafs( const dlump_t *l )
{
	dleaf_t 	*in;
	mleaf_t	*out;
	int	i, j, p, count;
		
	in = (void *)(mod_base + l->fileofs);
	if( l->filelen % sizeof( *in )) Host_Error( "Mod_LoadLeafs: funny lump size\n" );

	count = l->filelen / sizeof( *in );
	if( count < 1 ) Host_Error( "Map %s has no leafs\n", loadmodel->name );
	out = (mleaf_t *)Mem_Alloc( loadmodel->mempool, count * sizeof( *out ));

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		for( j = 0; j < 3; j++ )
		{
			out->minmaxs[j] = in->mins[j];
			out->minmaxs[3+j] = in->maxs[j];
		}

		out->contents = in->contents;
	
		p = in->visofs;

		if( p == -1 ) out->compressed_vis = NULL;
		else out->compressed_vis = loadmodel->visdata + p;

		for( j = 0; j < 4; j++ )
			out->ambient_sound_level[j] = in->ambient_level[j];

		out->firstmarksurface = loadmodel->marksurfaces + in->firstmarksurface;
		out->nummarksurfaces = in->nummarksurfaces;

		// gl underwater warp
		if( out->contents != CONTENTS_EMPTY )
		{
			for( j = 0; j < out->nummarksurfaces; j++ )
			{
				// underwater surfaces can't have reflection (perfomance)
				out->firstmarksurface[j]->flags |= SURF_UNDERWATER;
				out->firstmarksurface[j]->flags &= ~SURF_REFLECT;
			}
		}
	}

	if( loadmodel->leafs[0].contents != CONTENTS_SOLID )
		Host_Error( "Mod_LoadLeafs: Map %s has leaf 0 is not CONTENTS_SOLID\n", loadmodel->name );
}

/*
=================
Mod_LoadPlanes
=================
*/
static void Mod_LoadPlanes( const dlump_t *l )
{
	dplane_t	*in;
	mplane_t	*out;
	int	i, j, count;
	
	in = (void *)(mod_base + l->fileofs);
	if( l->filelen % sizeof( *in )) Host_Error( "Mod_LoadPlanes: funny lump size\n" );
	count = l->filelen / sizeof( *in );

	if( count < 1 ) Host_Error( "Map %s has no planes\n", loadmodel->name );
	out = (mplane_t *)Mem_Alloc( loadmodel->mempool, count * sizeof( *out ));

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for( i = 0; i < count; i++, in++, out++ )
	{
		for( j = 0; j < 3; j++ )
		{
			out->normal[j] = in->normal[j];
			if( out->normal[j] < 0.0f )
				out->signbits |= 1<<j;
		}

		out->dist = in->dist;
		out->type = in->type;
	}
}

/*
=================
Mod_LoadVisibility
=================
*/
static void Mod_LoadVisibility( const dlump_t *l )
{
	// bmodels has no visibility
	if( !world.loading ) return;

	if( !l->filelen )
	{
		MsgDev( D_WARN, "map ^2%s^7 has no visibility\n", loadmodel->name );
		loadmodel->visdata = NULL;
		world.visdatasize = 0;
		return;
	}

	loadmodel->visdata = Mem_Alloc( loadmodel->mempool, l->filelen );
	Q_memcpy( loadmodel->visdata, (void *)(mod_base + l->fileofs), l->filelen );
	world.visdatasize = l->filelen; // save it for PHS allocation
}

/*
=================
Mod_LoadEntities
=================
*/
static void Mod_LoadEntities( const dlump_t *l )
{
	char	*pfile;
	string	keyname;
	char	token[2048];

	// make sure what we really has terminator
	loadmodel->entities = Mem_Alloc( loadmodel->mempool, l->filelen + 1 );
	Q_memcpy( loadmodel->entities, mod_base + l->fileofs, l->filelen );
	if( !world.loading ) return;

	world.entdatasize = l->filelen;
	pfile = (char *)loadmodel->entities;
	world.message[0] = '\0';
	wadlist.count = 0;

	// parse all the wads for loading textures in right ordering
	while(( pfile = COM_ParseFile( pfile, token )) != NULL )
	{
		if( token[0] != '{' )
			Host_Error( "Mod_LoadEntities: found %s when expecting {\n", token );

		while( 1 )
		{
			// parse key
			if(( pfile = COM_ParseFile( pfile, token )) == NULL )
				Host_Error( "Mod_LoadEntities: EOF without closing brace\n" );
			if( token[0] == '}' ) break; // end of desc

			Q_strncpy( keyname, token, sizeof( keyname ));

			// parse value	
			if(( pfile = COM_ParseFile( pfile, token )) == NULL ) 
				Host_Error( "Mod_LoadEntities: EOF without closing brace\n" );

			if( token[0] == '}' )
				Host_Error( "Mod_LoadEntities: closing brace without data\n" );

			if( !Q_stricmp( keyname, "wad" ))
			{
				char	*path = token;
				string	wadpath;

				// parse wad pathes
				while( path )
				{
					char *end = Q_strchr( path, ';' );
					if( !end ) break;
					Q_strncpy( wadpath, path, (end - path) + 1 );
					FS_FileBase( wadpath, wadlist.wadnames[wadlist.count++] );
					path += (end - path) + 1; // move pointer
					if( wadlist.count >= 256 ) break; // too many wads...
				}
			}
			else if( !Q_stricmp( keyname, "mapversion" ))
				world.mapversion = Q_atoi( token );
			else if( !Q_stricmp( keyname, "message" ))
				Q_strncpy( world.message, token, sizeof( world.message ));
		}
		return;	// all done
	}
}

/*
=================
Mod_LoadClipnodes
=================
*/
static void Mod_LoadClipnodes( const dlump_t *l )
{
	dclipnode_t	*in, *out;
	int		i, count;
	hull_t		*hull;

	in = (void *)(mod_base + l->fileofs);
	if( l->filelen % sizeof( *in )) Host_Error( "Mod_LoadClipnodes: funny lump size\n" );
	count = l->filelen / sizeof( *in );
	out = Mem_Alloc( loadmodel->mempool, count * sizeof( *out ));	

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count;

	// hulls[0] is a point hull, always zeroed

	hull = &loadmodel->hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	VectorCopy( GI->client_mins[1], hull->clip_mins ); // copy human hull
	VectorCopy( GI->client_maxs[1], hull->clip_maxs );
	VectorSubtract( hull->clip_maxs, hull->clip_mins, world.hull_sizes[1] );

	hull = &loadmodel->hulls[2];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	VectorCopy( GI->client_mins[2], hull->clip_mins ); // copy large hull
	VectorCopy( GI->client_maxs[2], hull->clip_maxs );
	VectorSubtract( hull->clip_maxs, hull->clip_mins, world.hull_sizes[2] );

	hull = &loadmodel->hulls[3];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	VectorCopy( GI->client_mins[3], hull->clip_mins ); // copy head hull
	VectorCopy( GI->client_maxs[3], hull->clip_maxs );
	VectorSubtract( hull->clip_maxs, hull->clip_mins, world.hull_sizes[3] );

	for( i = 0; i < count; i++, out++, in++ )
	{
		out->planenum = in->planenum;
		out->children[0] = in->children[0];
		out->children[1] = in->children[1];
	}
}

/*
=================
Mod_LoadClipnodes31
=================
*/
static void Mod_LoadClipnodes31( const dlump_t *l, const dlump_t *l2, const dlump_t *l3 )
{
	dclipnode_t	*in, *in2, *in3, *out, *out2, *out3;
	int		i, count, count2, count3;
	hull_t		*hull;

	in = (void *)(mod_base + l->fileofs);
	if( l->filelen % sizeof( *in )) Host_Error( "Mod_LoadClipnodes: funny lump size\n" );
	count = l->filelen / sizeof( *in );
	out = Mem_Alloc( loadmodel->mempool, count * sizeof( *out ));	

	in2 = (void *)(mod_base + l2->fileofs);
	if( l2->filelen % sizeof( *in2 )) Host_Error( "Mod_LoadClipnodes2: funny lump size\n" );
	count2 = l2->filelen / sizeof( *in2 );
	out2 = Mem_Alloc( loadmodel->mempool, count2 * sizeof( *out2 ));

	in3 = (void *)(mod_base + l3->fileofs);
	if( l3->filelen % sizeof( *in3 )) Host_Error( "Mod_LoadClipnodes3: funny lump size\n" );
	count3 = l3->filelen / sizeof( *in3 );
	out3 = Mem_Alloc( loadmodel->mempool, count3 * sizeof( *out3 ));

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count + count2 + count3;

	// hulls[0] is a point hull, always zeroed

	hull = &loadmodel->hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;
	VectorCopy( GI->client_mins[1], hull->clip_mins ); // copy human hull
	VectorCopy( GI->client_maxs[1], hull->clip_maxs );
	VectorSubtract( hull->clip_maxs, hull->clip_mins, world.hull_sizes[1] );

	hull = &loadmodel->hulls[2];
	hull->clipnodes = out2;
	hull->firstclipnode = 0;
	hull->lastclipnode = count2 - 1;
	hull->planes = loadmodel->planes;
	VectorCopy( GI->client_mins[2], hull->clip_mins ); // copy large hull
	VectorCopy( GI->client_maxs[2], hull->clip_maxs );
	VectorSubtract( hull->clip_maxs, hull->clip_mins, world.hull_sizes[2] );

	hull = &loadmodel->hulls[3];
	hull->clipnodes = out3;
	hull->firstclipnode = 0;
	hull->lastclipnode = count3 - 1;
	hull->planes = loadmodel->planes;
	VectorCopy( GI->client_mins[3], hull->clip_mins ); // copy head hull
	VectorCopy( GI->client_maxs[3], hull->clip_maxs );
	VectorSubtract( hull->clip_maxs, hull->clip_mins, world.hull_sizes[3] );

	for( i = 0; i < count; i++, out++, in++ )
	{
		out->planenum = in->planenum;
		out->children[0] = in->children[0];
		out->children[1] = in->children[1];
	}

	for( i = 0; i < count2; i++, out2++, in2++ )
	{
		out2->planenum = in2->planenum;
		out2->children[0] = in2->children[0];
		out2->children[1] = in2->children[1];
	}

	for( i = 0; i < count3; i++, out3++, in3++ )
	{
		out3->planenum = in3->planenum;
		out3->children[0] = in3->children[0];
		out3->children[1] = in3->children[1];
	}
}

/*
=================
Mod_FindModelOrigin

routine to detect bmodels with origin-brush
=================
*/
static void Mod_FindModelOrigin( const char *entities, const char *modelname, vec3_t origin )
{
	char	*pfile;
	string	keyname;
	char	token[2048];
	qboolean	model_found;
	qboolean	origin_found;

	if( !entities || !modelname || !*modelname || !origin )
		return;

	pfile = (char *)entities;

	while(( pfile = COM_ParseFile( pfile, token )) != NULL )
	{
		if( token[0] != '{' )
			Host_Error( "Mod_FindModelOrigin: found %s when expecting {\n", token );

		model_found = origin_found = false;
		VectorClear( origin );

		while( 1 )
		{
			// parse key
			if(( pfile = COM_ParseFile( pfile, token )) == NULL )
				Host_Error( "Mod_FindModelOrigin: EOF without closing brace\n" );
			if( token[0] == '}' ) break; // end of desc

			Q_strncpy( keyname, token, sizeof( keyname ));

			// parse value	
			if(( pfile = COM_ParseFile( pfile, token )) == NULL ) 
				Host_Error( "Mod_FindModelOrigin: EOF without closing brace\n" );

			if( token[0] == '}' )
				Host_Error( "Mod_FindModelOrigin: closing brace without data\n" );

			if( !Q_stricmp( keyname, "model" ) && !Q_stricmp( modelname, token ))
				model_found = true;

			if( !Q_stricmp( keyname, "origin" ))
			{
				Q_atov( origin, token, 3 );
				origin_found = true;
			}
		}

		if( model_found ) break;
	}	
}

/*
=================
Mod_MakeHull0

Duplicate the drawing hull structure as a clipping hull
=================
*/
static void Mod_MakeHull0( void )
{
	mnode_t		*in, *child;
	dclipnode_t	*out;
	hull_t		*hull;
	int		i, j, count;
	
	hull = &loadmodel->hulls[0];	
	
	in = loadmodel->nodes;
	count = loadmodel->numnodes;
	out = Mem_Alloc( loadmodel->mempool, count * sizeof( *out ));	

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count - 1;
	hull->planes = loadmodel->planes;

	for( i = 0; i < count; i++, out++, in++ )
	{
		out->planenum = in->plane - loadmodel->planes;

		for( j = 0; j < 2; j++ )
		{
			child = in->children[j];

			if( child->contents < 0 )
				out->children[j] = child->contents;
			else out->children[j] = child - loadmodel->nodes;
		}
	}
}

/*
=================
Mod_CalcPHS
=================
*/
void Mod_CalcPHS( void )
{
	int	hcount, vcount;
	int	i, j, k, l, index, num;
	int	rowbytes, rowwords;
	int	bitbyte, rowsize;
	int	*visofs, total_size = 0;
	byte	*vismap, *vismap_p;
	byte	*uncompressed_vis;
	byte	*uncompressed_pas;
	byte	*compressed_pas;
	byte	*scan, *comp;
	uint	*dest, *src;
	double	timestart;
	size_t	phsdatasize;

	// no worldmodel or no visdata
	if( !world.loading || !worldmodel || !worldmodel->visdata )
		return;

	MsgDev( D_NOTE, "Building PAS...\n" );
	timestart = Sys_DoubleTime();

	// NOTE: first leaf is skipped becuase is a outside leaf. Now all leafs have shift up by 1.
	// and last leaf (which equal worldmodel->numleafs) has no visdata! Add one extra leaf
	// to avoid this situation.
	num = worldmodel->numleafs;
	rowwords = (num + 31) >> 5;
	rowbytes = rowwords * 4;

	// typically PHS reqiured more room because RLE fails on multiple 1 not 0
	phsdatasize = world.visdatasize * 32; // empirically determined

	// allocate pvs and phs data single array
	visofs = Mem_Alloc( worldmodel->mempool, num * sizeof( int ));
	uncompressed_vis = Mem_Alloc( worldmodel->mempool, rowbytes * num * 2 );
	uncompressed_pas = uncompressed_vis + rowbytes * num;
	compressed_pas = Mem_Alloc( worldmodel->mempool, phsdatasize );
	vismap = vismap_p = compressed_pas; // compressed PHS buffer
	scan = uncompressed_vis;
	vcount = 0;

	// uncompress pvs first
	for( i = 0; i < num; i++, scan += rowbytes )
	{
		Q_memcpy( scan, Mod_LeafPVS( worldmodel->leafs + i, worldmodel ), rowbytes );
		if( i == 0 ) continue;

		for( j = 0; j < num; j++ )
		{
			if( scan[j>>3] & (1<<( j & 7 )))
				vcount++;
		}
	}

	scan = uncompressed_vis;
	hcount = 0;

	dest = (uint *)uncompressed_pas;

	for( i = 0; i < num; i++, dest += rowwords, scan += rowbytes )
	{
		Q_memcpy( dest, scan, rowbytes );

		for( j = 0; j < rowbytes; j++ )
		{
			bitbyte = scan[j];
			if( !bitbyte ) continue;

			for( k = 0; k < 8; k++ )
			{
				if(!( bitbyte & ( 1<<k )))
					continue;
				// or this pvs row into the phs
				// +1 because pvs is 1 based
				index = ((j<<3) + k + 1);
				if( index >= num ) continue;

				src = (uint *)uncompressed_vis + index * rowwords;
				for( l = 0; l < rowwords; l++ )
					dest[l] |= src[l];
			}
		}

		// compress PHS data back
		comp = Mod_CompressVis( (byte *)dest, &rowsize );
		visofs[i] = vismap_p - vismap; // leaf 0 is a common solid 
		total_size += rowsize;

		if( total_size > phsdatasize )
		{
			Host_Error( "CalcPHS: vismap expansion overflow %s > %s\n", Q_memprint( total_size ), Q_memprint( phsdatasize ));
		}

		Q_memcpy( vismap_p, comp, rowsize );
		vismap_p += rowsize; // move pointer

		if( i == 0 ) continue;

		for( j = 0; j < num; j++ )
		{
			if(((byte *)dest)[j>>3] & (1<<( j & 7 )))
				hcount++;
		}
	}

	// adjust compressed pas data to fit the size
	compressed_pas = Mem_Realloc( worldmodel->mempool, compressed_pas, total_size );

	// apply leaf pointers
	for( i = 0; i < worldmodel->numleafs; i++ )
		worldmodel->leafs[i].compressed_pas = compressed_pas + visofs[i];

	// release uncompressed data
	Mem_Free( uncompressed_vis );
	Mem_Free( visofs );	// release vis offsets

	// NOTE: we don't need to store off pointer to compressed pas-data
	// because this is will be automatiaclly frees by mempool internal pointer
	// and we never use this pointer after this point
	MsgDev( D_NOTE, "Average leaves visible / audible / total: %i / %i / %i\n", vcount / num, hcount / num, num );
	MsgDev( D_NOTE, "PAS building time: %g secs\n", Sys_DoubleTime() - timestart );
}

/*
=================
Mod_UnloadBrushModel

Release all uploaded textures
=================
*/
void Mod_UnloadBrushModel( model_t *mod )
{
	texture_t	*tx;
	int	i;

	ASSERT( mod != NULL );

	if( mod->type != mod_brush )
		return; // not a bmodel

	if( mod->name[0] != '*' )
	{
		for( i = 0; i < mod->numtextures; i++ )
		{
			tx = mod->textures[i];
			if( !tx || tx->gl_texturenum == tr.defaultTexture )
				continue;	// free slot

			GL_FreeTexture( tx->gl_texturenum );	// main texture
			GL_FreeTexture( tx->fb_texturenum );	// luma texture
		}

		Mem_FreePool( &mod->mempool );
	}

	Q_memset( mod, 0, sizeof( *mod ));
}

/*
=================
Mod_LoadBrushModel
=================
*/
static void Mod_LoadBrushModel( model_t *mod, const void *buffer, qboolean *loaded )
{
	int	i, j;
	int	sample_size;
	char	*ents;
	dheader_t	*header;
	dmodel_t 	*bm;

	if( loaded ) *loaded = false;	
	header = (dheader_t *)buffer;
	loadmodel->type = mod_brush;
	i = header->version;

	switch( i )
	{
	case 28:	// get support for quake1 beta
		i = Q1BSP_VERSION;
		sample_size = 16;
		break;
	case Q1BSP_VERSION:
	case HLBSP_VERSION:
		sample_size = 16;
		break;
	case XTBSP_VERSION:
		sample_size = 8;
		break;
	default:
		MsgDev( D_ERROR, "%s has wrong version number (%i should be %i)", loadmodel->name, i, HLBSP_VERSION );
		return;
	}

	// will be merged later
	if( world.loading )
	{
		world.lm_sample_size = sample_size;
		world.version = i;
	}
	else if( world.lm_sample_size != sample_size )
	{
		// can't mixing world and bmodels with different sample sizes!
		MsgDev( D_ERROR, "%s has wrong version number (%i should be %i)", loadmodel->name, i, world.version );
		return;		
	}
	bmodel_version = i;	// share it

	// swap all the lumps
	mod_base = (byte *)header;
	loadmodel->mempool = Mem_AllocPool( va( "^2%s^7", loadmodel->name ));

	// load into heap
	if( header->lumps[LUMP_ENTITIES].fileofs <= 1024 && (header->lumps[LUMP_ENTITIES].filelen % sizeof( dplane_t )) == 0 )
	{
		// blue-shift swapped lumps
		Mod_LoadEntities( &header->lumps[LUMP_PLANES] );
		Mod_LoadPlanes( &header->lumps[LUMP_ENTITIES] );
	}
	else
	{
		// normal half-life lumps
		Mod_LoadEntities( &header->lumps[LUMP_ENTITIES] );
		Mod_LoadPlanes( &header->lumps[LUMP_PLANES] );
	}

	// Half-Life: alpha version has BSP version 29 and map version 220 (and lightdata is RGB)
	if( world.version <= 29 && world.mapversion == 220 && (header->lumps[LUMP_LIGHTING].filelen % 3) == 0 )
		world.version = bmodel_version = HLBSP_VERSION;

	Mod_LoadVertexes( &header->lumps[LUMP_VERTEXES] );
	Mod_LoadEdges( &header->lumps[LUMP_EDGES] );
	Mod_LoadSurfEdges( &header->lumps[LUMP_SURFEDGES] );
	Mod_LoadTextures( &header->lumps[LUMP_TEXTURES] );
	Mod_LoadLighting( &header->lumps[LUMP_LIGHTING] );
	Mod_LoadVisibility( &header->lumps[LUMP_VISIBILITY] );
	Mod_LoadTexInfo( &header->lumps[LUMP_TEXINFO] );
	Mod_LoadSurfaces( &header->lumps[LUMP_FACES] );
	Mod_LoadMarkSurfaces( &header->lumps[LUMP_MARKSURFACES] );
	Mod_LoadLeafs( &header->lumps[LUMP_LEAFS] );
	Mod_LoadNodes( &header->lumps[LUMP_NODES] );

	if( bmodel_version == XTBSP_VERSION )
		Mod_LoadClipnodes31( &header->lumps[LUMP_CLIPNODES], &header->lumps[LUMP_CLIPNODES2], &header->lumps[LUMP_CLIPNODES3] );
	else Mod_LoadClipnodes( &header->lumps[LUMP_CLIPNODES] );
	Mod_LoadSubmodels( &header->lumps[LUMP_MODELS] );

	Mod_MakeHull0 ();
	
	loadmodel->numframes = 2;	// regular and alternate animation
	ents = loadmodel->entities;
	
	// set up the submodels
	for( i = 0; i < mod->numsubmodels; i++ )
	{
		bm = &mod->submodels[i];

		mod->hulls[0].firstclipnode = bm->headnode[0];
		for( j = 1; j < MAX_MAP_HULLS; j++ )
		{
			mod->hulls[j].firstclipnode = bm->headnode[j];
//			mod->hulls[j].lastclipnode = mod->numclipnodes - 1;
		}
		
		mod->firstmodelsurface = bm->firstface;
		mod->nummodelsurfaces = bm->numfaces;

		VectorCopy( bm->mins, mod->mins );		
		VectorCopy( bm->maxs, mod->maxs );

		mod->radius = RadiusFromBounds( mod->mins, mod->maxs );
		mod->numleafs = bm->visleafs;
		mod->flags = 0;

		if( i != 0 )
		{
			// HACKHACK: c2a1 issues
			if( !bm->origin[0] && !bm->origin[1] ) mod->flags |= MODEL_HAS_ORIGIN;

			Mod_FindModelOrigin( ents, va( "*%i", i ), bm->origin );

			// flag 2 is indicated model with origin brush!
			if( !VectorIsNull( bm->origin )) mod->flags |= MODEL_HAS_ORIGIN;
		}

		for( j = 0; i != 0 && j < mod->nummodelsurfaces; j++ )
		{
			msurface_t	*surf = mod->surfaces + mod->firstmodelsurface + j;
			mextrasurf_t	*info = SURF_INFO( surf, mod );

			if( surf->flags & SURF_CONVEYOR )
				mod->flags |= MODEL_CONVEYOR;

			// kill water backplanes for submodels (half-life rules)
			if( surf->flags & SURF_DRAWTURB )
			{
				mod->flags |= MODEL_LIQUID;

				if( surf->plane->type == PLANE_Z )
				{
					// kill bottom plane too
					if( info->mins[2] == bm->mins[2] + 1 )
						surf->flags |= SURF_WATERCSG;
				}
				else
				{
					// kill side planes
					surf->flags |= SURF_WATERCSG;
				}
			}
		}

		if( i < mod->numsubmodels - 1 )
		{
			char	name[8];

			// duplicate the basic information
			Q_snprintf( name, sizeof( name ), "*%i", i + 1 );
			loadmodel = Mod_FindName( name, true );
			*loadmodel = *mod;
			Q_strncpy( loadmodel->name, name, sizeof( loadmodel->name ));
			loadmodel->mempool = NULL;
			mod = loadmodel;
		}
	}

	if( loaded ) *loaded = true;	// all done
}

/*
==================
Mod_FindName

==================
*/
model_t *Mod_FindName( const char *filename, qboolean create )
{
	model_t	*mod;
	char	name[64];
	int	i;
	
	if( !filename || !filename[0] )
		return NULL;

	if( *filename == '!' ) filename++;
	Q_strncpy( name, filename, sizeof( name ));
	COM_FixSlashes( name );
		
	// search the currently loaded models
	for( i = 0, mod = cm_models; i < cm_nummodels; i++, mod++ )
	{
		if( !mod->name[0] ) continue;
		if( !Q_stricmp( mod->name, name ))
		{
			// prolonge registration
			mod->needload = world.load_sequence;
			return mod;
		}
	}

	if( !create ) return NULL;			

	// find a free model slot spot
	for( i = 0, mod = cm_models; i < cm_nummodels; i++, mod++ )
		if( !mod->name[0] ) break; // this is a valid spot

	if( i == cm_nummodels )
	{
		if( cm_nummodels == MAX_MODELS )
			Host_Error( "Mod_ForName: MAX_MODELS limit exceeded\n" );
		cm_nummodels++;
	}

	// copy name, so model loader can find model file
	Q_strncpy( mod->name, name, sizeof( mod->name ));

	return mod;
}

/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
model_t *Mod_LoadModel( model_t *mod, qboolean crash )
{
	byte	*buf;
	char	tempname[64];
	qboolean	loaded;

	if( !mod )
	{
		if( crash ) Host_Error( "Mod_ForName: NULL model\n" );
		else MsgDev( D_ERROR, "Mod_ForName: NULL model\n" );
		return NULL;		
	}

	// check if already loaded (or inline bmodel)
	if( mod->mempool || mod->name[0] == '*' )
		return mod;

	// store modelname to show error
	Q_strncpy( tempname, mod->name, sizeof( tempname ));
	COM_FixSlashes( tempname );

	buf = FS_LoadFile( tempname, NULL, false );

	if( !buf )
	{
		Q_memset( mod, 0, sizeof( model_t ));

		if( crash ) Host_Error( "Mod_ForName: %s couldn't load\n", tempname );
		else MsgDev( D_ERROR, "Mod_ForName: %s couldn't load\n", tempname );

		return NULL;
	}

	FS_FileBase( mod->name, modelname );

	MsgDev( D_NOTE, "Mod_LoadModel: %s\n", mod->name );
	mod->needload = world.load_sequence; // register mod
	mod->type = mod_bad;
	loadmodel = mod;

	// call the apropriate loader
	switch( *(uint *)buf )
	{
	case IDSTUDIOHEADER:
		Mod_LoadStudioModel( mod, buf, &loaded );
		break;
	case IDSPRITEHEADER:
		Mod_LoadSpriteModel( mod, buf, &loaded, 0 );
		break;
	case Q1BSP_VERSION:
	case HLBSP_VERSION:
	case XTBSP_VERSION:
		Mod_LoadBrushModel( mod, buf, &loaded );
		break;
	default:
		Mem_Free( buf );
		if( crash ) Host_Error( "Mod_ForName: %s unknown format\n", tempname );
		else MsgDev( D_ERROR, "Mod_ForName: %s unknown format\n", tempname );
		return NULL;
	}

	if( !loaded )
	{
		Mod_FreeModel( mod );
		Mem_Free( buf );

		if( crash ) Host_Error( "Mod_ForName: %s couldn't load\n", tempname );
		else MsgDev( D_ERROR, "Mod_ForName: %s couldn't load\n", tempname );

		return NULL;
	}
	else if( clgame.drawFuncs.Mod_ProcessUserData != NULL )
	{
		// let the client.dll load custom data
		clgame.drawFuncs.Mod_ProcessUserData( mod, true, buf );
	}

	Mem_Free( buf );

	return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName( const char *name, qboolean crash )
{
	model_t	*mod;
	
	mod = Mod_FindName( name, true );
	return Mod_LoadModel( mod, crash );
}

/*
==================
Mod_LoadWorld

Loads in the map and all submodels
==================
*/
void Mod_LoadWorld( const char *name, uint *checksum, qboolean force )
{
	int	i;

	// now replacement table is invalidate
	Q_memset( com_models, 0, sizeof( com_models ));

	com_models[1] = cm_models; // make link to world

	// update the lightmap blocksize
	if( host.features & ENGINE_LARGE_LIGHTMAPS )
		world.block_size = BLOCK_SIZE_MAX;
	else world.block_size = BLOCK_SIZE_DEFAULT;

	if( !Q_stricmp( cm_models[0].name, name ) && !force )
	{
		// singleplayer mode: server already loaded map
		if( checksum ) *checksum = world.checksum;

		// still have the right version
		return;
	}

	// clear all studio submodels on restart
	// HACKHACK: throw all external BSP-models to refresh their lightmaps properly
	for( i = 1; i < cm_nummodels; i++ )
	{
		if( cm_models[i].type == mod_studio )
			cm_models[i].submodels = NULL;
		else if( cm_models[i].type == mod_brush )
			Mod_FreeModel( cm_models + i );
	}

	// purge all submodels
	Mod_FreeModel( &cm_models[0] );
	Mem_EmptyPool( com_studiocache );
	world.load_sequence++;	// now all models are invalid

	// load the newmap
	world.loading = true;
	worldmodel = Mod_ForName( name, true );
	CRC32_MapFile( &world.checksum, worldmodel->name );
	world.loading = false;

	if( checksum ) *checksum = world.checksum;
		
	// calc Potentially Hearable Set and compress it
	Mod_CalcPHS();
}

/*
==================
Mod_FreeUnused

Purge all unused models
==================
*/
void Mod_FreeUnused( void )
{
	model_t	*mod;
	int	i;

	for( i = 0, mod = cm_models; i < cm_nummodels; i++, mod++ )
	{
		if( !mod->name[0] ) continue;
		if( mod->needload != world.load_sequence )
			Mod_FreeModel( mod );
	}
}

/*
===================
Mod_GetType
===================
*/
modtype_t Mod_GetType( int handle )
{
	model_t	*mod = Mod_Handle( handle );

	if( !mod ) return mod_bad;
	return mod->type;
}

/*
===================
Mod_GetFrames
===================
*/
void Mod_GetFrames( int handle, int *numFrames )
{
	model_t	*mod = Mod_Handle( handle );

	if( !numFrames ) return;
	if( !mod )
	{
		*numFrames = 1;
		return;
	}

	*numFrames = mod->numframes;
	if( *numFrames < 1 ) *numFrames = 1;
}

/*
===================
Mod_GetBounds
===================
*/
void Mod_GetBounds( int handle, vec3_t mins, vec3_t maxs )
{
	model_t	*cmod;

	if( handle <= 0 ) return;
	cmod = Mod_Handle( handle );

	if( cmod )
	{
		if( mins ) VectorCopy( cmod->mins, mins );
		if( maxs ) VectorCopy( cmod->maxs, maxs );
	}
	else
	{
		MsgDev( D_ERROR, "Mod_GetBounds: NULL model %i\n", handle );
		if( mins ) VectorClear( mins );
		if( maxs ) VectorClear( maxs );
	}
}

/*
===============
Mod_Calloc

===============
*/
void *Mod_Calloc( int number, size_t size )
{
	cache_user_t	*cu;

	if( number <= 0 || size <= 0 ) return NULL;
	cu = (cache_user_t *)Mem_Alloc( com_studiocache, sizeof( cache_user_t ) + number * size );
	cu->data = (void *)cu; // make sure what cu->data is not NULL

	return cu;
}

/*
===============
Mod_CacheCheck

===============
*/
void *Mod_CacheCheck( cache_user_t *c )
{
	return Cache_Check( com_studiocache, c );
}

/*
===============
Mod_LoadCacheFile

===============
*/
void Mod_LoadCacheFile( const char *filename, cache_user_t *cu )
{
	byte	*buf;
	string	name;
	size_t	i, j, size;

	ASSERT( cu != NULL );

	if( !filename || !filename[0] ) return;

	// eliminate '!' symbol (i'm doesn't know what this doing)
	for( i = j = 0; i < Q_strlen( filename ); i++ )
	{
		if( filename[i] == '!' ) continue;
		else if( filename[i] == '\\' ) name[j] = '/';
		else name[j] = Q_tolower( filename[i] );
		j++;
	}
	name[j] = '\0';

	buf = FS_LoadFile( name, &size, false );
	if( !buf || !size ) Host_Error( "LoadCacheFile: ^1can't load %s^7\n", filename );
	cu->data = Mem_Alloc( com_studiocache, size );
	Q_memcpy( cu->data, buf, size );
	Mem_Free( buf );
}

/*
===================
Mod_RegisterModel

register model with shared index
===================
*/
qboolean Mod_RegisterModel( const char *name, int index )
{
	model_t	*mod;

	if( index < 0 || index > MAX_MODELS )
		return false;

	// this array used for acess to servermodels
	mod = Mod_ForName( name, false );
	com_models[index] = mod;

	return ( mod != NULL );
}

/*
===============
Mod_Extradata

===============
*/
void *Mod_Extradata( model_t *mod )
{
	if( mod && mod->type == mod_studio )
		return mod->cache.data;
	return NULL;
}

/*
==================
Mod_Handle

==================
*/
model_t *Mod_Handle( int handle )
{
	if( handle < 0 || handle >= MAX_MODELS )
	{
		MsgDev( D_NOTE, "Mod_Handle: bad handle #%i\n", handle );
		return NULL;
	}
	return com_models[handle];
}

wadlist_t *Mod_WadList( void )
{
	return &wadlist;
}