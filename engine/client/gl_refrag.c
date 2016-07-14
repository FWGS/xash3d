/*
gl_refrag.c - store entity fragments
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
#include "entity_types.h"
#include "studio.h"

/*
===============================================================================

			ENTITY FRAGMENT FUNCTIONS

===============================================================================
*/

static efrag_t	**lastlink;
static mnode_t	*r_pefragtopnode;
static vec3_t	r_emins, r_emaxs;
static cl_entity_t	*r_addent;

/*
================
R_RemoveEfrags

Call when removing an object from the world or moving it to another position
================
*/
void R_RemoveEfrags( cl_entity_t *ent )
{
	efrag_t	*ef, *old, *walk, **prev;
	
	ef = ent->efrag;
	
	while( ef )
	{
		prev = &ef->leaf->efrags;
		while( 1 )
		{
			walk = *prev;
			if( !walk ) break;

			if( walk == ef )
			{	
				// remove this fragment
				*prev = ef->leafnext;
				break;
			}
			else prev = &walk->leafnext;
		}
				
		old = ef;
		ef = ef->entnext;
		
		// put it on the free list
		old->entnext = clgame.free_efrags;
		clgame.free_efrags = old;
	}
	ent->efrag = NULL; 
}

/*
===================
R_SplitEntityOnNode
===================
*/
static void R_SplitEntityOnNode( mnode_t *node )
{
	efrag_t	*ef;
	mplane_t	*splitplane;
	mleaf_t	*leaf;
	int	sides;
	
	if( node->contents == CONTENTS_SOLID )
		return;
	
	// add an efrag if the node is a leaf
	if( node->contents < 0 )
	{
		if( !r_pefragtopnode )
			r_pefragtopnode = node;

		leaf = (mleaf_t *)node;

		// grab an efrag off the free list
		ef = clgame.free_efrags;
		if( !ef )
		{
			MsgDev( D_ERROR, "too many efrags!\n" );
			return; // no free fragments...
		}

		clgame.free_efrags = clgame.free_efrags->entnext;
		ef->entity = r_addent;
		
		// add the entity link	
		*lastlink = ef;
		lastlink = &ef->entnext;
		ef->entnext = NULL;
		
		// set the leaf links
		ef->leaf = leaf;
		ef->leafnext = leaf->efrags;
		leaf->efrags = ef;
		return;
	}
	
	// NODE_MIXED
	splitplane = node->plane;
	sides = BOX_ON_PLANE_SIDE( r_emins, r_emaxs, splitplane );
	
	if( sides == 3 )
	{
		// split on this plane
		// if this is the first splitter of this bmodel, remember it
		if( !r_pefragtopnode ) r_pefragtopnode = node;
	}
	
	// recurse down the contacted sides
	if( sides & 1 ) R_SplitEntityOnNode( node->children[0] );
	if( sides & 2 ) R_SplitEntityOnNode( node->children[1] );
}

/*
===========
R_AddEfrags
===========
*/
void R_AddEfrags( cl_entity_t *ent )
{
	int	i;
		
	if( !ent->model )
		return;

	r_addent = ent;
	lastlink = &ent->efrag;
	r_pefragtopnode = NULL;

	// NOTE: can't copy these bounds directly into model->mins\model->maxs
	// because all other code don't expected this
	if( ent->model->type == mod_studio )
	{
		studiohdr_t *phdr = (studiohdr_t *)Mod_Extradata( ent->model );
		mstudioseqdesc_t *pseqdesc;

		if( !phdr ) return;
		pseqdesc = (mstudioseqdesc_t *)((byte *)phdr + phdr->seqindex);

		for( i = 0; i < 3; i++ )
		{
			r_emins[i] = ent->origin[i] + pseqdesc[0].bbmin[i];
			r_emaxs[i] = ent->origin[i] + pseqdesc[0].bbmax[i];
		}
	}
	else
	{
		for( i = 0; i < 3; i++ )
		{
			r_emins[i] = ent->origin[i] + ent->model->mins[i];
			r_emaxs[i] = ent->origin[i] + ent->model->maxs[i];
		}
	}

	R_SplitEntityOnNode( cl.worldmodel->nodes );
	ent->topnode = r_pefragtopnode;
}

/*
================
R_StoreEfrags

================
*/
void R_StoreEfrags( efrag_t **ppefrag, int framecount )
{
	cl_entity_t	*pent;
	model_t		*clmodel;
	efrag_t		*pefrag;

	while(( pefrag = *ppefrag ) != NULL )
	{
		pent = pefrag->entity;
		clmodel = pent->model;

		switch( clmodel->type )
		{
		case mod_alias:
		case mod_brush:
		case mod_studio:
		case mod_sprite:
			pent = pefrag->entity;

			if( pent->visframe != framecount )
			{
				if( CL_AddVisibleEntity( pent, ET_FRAGMENTED ))
				{
					// mark that we've recorded this entity for this frame
					pent->visframe = framecount;
				}
			}

			ppefrag = &pefrag->leafnext;
			break;
		default:	
			Host_Error( "R_StoreEfrags: bad entity type %d\n", clmodel->type );
			break;
		}
	}
}
#endif // XASH_DEDICATED
