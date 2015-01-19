/*
bspfile.h - BSP format included q1, hl1 support
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

#ifndef BSPFILE_H
#define BSPFILE_H

/*
==============================================================================

BRUSH MODELS

.bsp contain level static geometry with including PVS and lightning info
==============================================================================
*/

// header
#define Q1BSP_VERSION	29	// quake1 regular version (beta is 28)
#define HLBSP_VERSION	30	// half-life regular version
#define XTBSP_VERSION	31	// extended lightmaps and expanded clipnodes limit

#define DELUXEMAP_VERSION	1
#define IDDELUXEMAPHEADER	(('T'<<24)+('I'<<16)+('L'<<8)+'Q') // little-endian "QLIT"

// worldcraft predefined angles
#define ANGLE_UP			-1
#define ANGLE_DOWN			-2

// bmodel limits
#define MAX_MAP_HULLS		4		// MAX_HULLS

#define SURF_NOCULL			BIT( 0 )		// two-sided polygon (e.g. 'water4b')
#define SURF_PLANEBACK		BIT( 1 )		// plane should be negated
#define SURF_DRAWSKY		BIT( 2 )		// sky surface
#define SURF_WATERCSG		BIT( 3 )		// culled by csg (was SURF_DRAWSPRITE)
#define SURF_DRAWTURB		BIT( 4 )		// warp surface
#define SURF_DRAWTILED		BIT( 5 )		// face without lighmap
#define SURF_CONVEYOR		BIT( 6 )		// scrolled texture (was SURF_DRAWBACKGROUND)
#define SURF_UNDERWATER		BIT( 7 )		// caustics
#define SURF_TRANSPARENT		BIT( 8 )		// it's a transparent texture (was SURF_DONTWARP)

#define SURF_REFLECT		BIT( 31 )		// reflect surface (mirror)

// lightstyle management
#define LM_STYLES			4		// MAXLIGHTMAPS
#define LS_NORMAL			0x00
#define LS_UNUSED			0xFE
#define LS_NONE			0xFF

#define MAX_MAP_MODELS		1024		// can be increased up to 2048 if needed
#define MAX_MAP_BRUSHES		32768		// unsigned short limit
#define MAX_MAP_ENTITIES		8192		// can be increased up to 32768 if needed
#define MAX_MAP_ENTSTRING		0x80000		// 512 kB should be enough
#define MAX_MAP_PLANES		65536		// can be increased without problems
#define MAX_MAP_NODES		32767		// because negative shorts are leafs
#define MAX_MAP_CLIPNODES		32767		// because negative shorts are contents
#define MAX_MAP_LEAFS		32767		// signed short limit
#define MAX_MAP_VERTS		65535		// unsigned short limit
#define MAX_MAP_FACES		65535		// unsigned short limit
#define MAX_MAP_MARKSURFACES		65535		// unsigned short limit
#define MAX_MAP_TEXINFO		MAX_MAP_FACES	// in theory each face may have personal texinfo
#define MAX_MAP_EDGES		0x100000		// can be increased but not needed
#define MAX_MAP_SURFEDGES		0x200000		// can be increased but not needed
#define MAX_MAP_TEXTURES		2048		// can be increased but not needed
#define MAX_MAP_MIPTEX		0x2000000		// 32 Mb internal textures data
#define MAX_MAP_LIGHTING		0x2000000		// 32 Mb lightmap raw data (can contain deluxemaps)
#define MAX_MAP_VISIBILITY		0x800000		// 8 Mb visdata

// quake lump ordering
#define LUMP_ENTITIES		0
#define LUMP_PLANES			1
#define LUMP_TEXTURES		2		// internal textures
#define LUMP_VERTEXES		3
#define LUMP_VISIBILITY		4
#define LUMP_NODES			5
#define LUMP_TEXINFO		6
#define LUMP_FACES			7
#define LUMP_LIGHTING		8
#define LUMP_CLIPNODES		9
#define LUMP_LEAFS			10
#define LUMP_MARKSURFACES		11
#define LUMP_EDGES			12
#define LUMP_SURFEDGES		13
#define LUMP_MODELS			14		// internal submodels
#define HEADER_LUMPS		15

// version 31
#define LUMP_CLIPNODES2		15		// hull0 goes into LUMP_NODES, hull1 goes into LUMP_CLIPNODES,
#define LUMP_CLIPNODES3		16		// hull2 goes into LUMP_CLIPNODES2, hull3 goes into LUMP_CLIPNODES3

// texture flags
#define TEX_SPECIAL			BIT( 0 )		// sky or slime, no lightmap or 256 subdivision

// ambient sound types
enum
{
	AMBIENT_WATER = 0,		// waterfall
	AMBIENT_SKY,		// wind
	AMBIENT_SLIME,		// never used in quake
	AMBIENT_LAVA,		// never used in quake
	NUM_AMBIENTS,		// automatic ambient sounds
};

//
// BSP File Structures
//

typedef struct
{
	int	fileofs;
	int	filelen;
} dlump_t;

typedef struct
{
	int	version;
	dlump_t	lumps[HEADER_LUMPS];
} dheader_t;

typedef struct
{
	vec3_t	mins;
	vec3_t	maxs;
	vec3_t	origin;			// for sounds or lights
	int	headnode[MAX_MAP_HULLS];
	int	visleafs;			// not including the solid leaf 0
	int	firstface;
	int	numfaces;
} dmodel_t;

typedef struct
{
	int	nummiptex;
	int	dataofs[4];		// [nummiptex]
} dmiptexlump_t;

typedef struct
{
	vec3_t	point;
} dvertex_t;

typedef struct
{
	vec3_t	normal;
	float	dist;
	int	type;			// PLANE_X - PLANE_ANYZ ?
} dplane_t;

typedef struct
{
	int	planenum;
	short	children[2];		// negative numbers are -(leafs + 1), not nodes
	short	mins[3];			// for sphere culling
	short	maxs[3];
	word	firstface;
	word	numfaces;			// counting both sides
} dnode_t;

// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct
{
	int	contents;
	int	visofs;			// -1 = no visibility info

	short	mins[3];			// for frustum culling
	short	maxs[3];
	word	firstmarksurface;
	word	nummarksurfaces;

	// automatic ambient sounds
	byte	ambient_level[NUM_AMBIENTS];	// ambient sound level (0 - 255)
} dleaf_t;

typedef struct
{
	int	planenum;
	short	children[2];		// negative numbers are contents
} dclipnode_t;

typedef struct
{
	float	vecs[2][4];		// texmatrix [s/t][xyz offset]
	int	miptex;
	int	flags;
} dtexinfo_t;

typedef word	dmarkface_t;		// leaf marksurfaces indexes
typedef int	dsurfedge_t;		// map surfedges

// NOTE: that edge 0 is never used, because negative edge nums
// are used for counterclockwise use of the edge in a face
typedef struct
{
	word	v[2];			// vertex numbers
} dedge_t;

typedef struct
{
	word	planenum;
	short	side;

	int	firstedge;		// we must support > 64k edges
	short	numedges;
	short	texinfo;

	// lighting info
	byte	styles[LM_STYLES];
	int	lightofs;			// start of [numstyles*surfsize] samples
} dface_t;

#endif//BSPFILE_H