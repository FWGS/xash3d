/*
cl_events.c - client-side event system implementation
Copyright (C) 2011 Uncle Mike

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
#include "event_flags.h"
#include "net_encode.h"

/*
===============
CL_ResetEvent

===============
*/
void CL_ResetEvent( event_info_t *ei )
{
	Q_memset( ei, 0, sizeof( *ei ));
}

/*
=============
CL_SetEventIndex

=============
*/
void CL_SetEventIndex( const char *szEvName, int ev_index )
{
	cl_user_event_t	*ev;
	int		i;

	if( !szEvName || !*szEvName )
		return; // ignore blank names

	// search event by name to link with
	for( i = 0; i < MAX_EVENTS; i++ )
	{
		ev = clgame.events[i];
		if( !ev ) break;

		if( !Q_stricmp( ev->name, szEvName ))
		{
			ev->index = ev_index;
			return;
		}
	}
}

/*
=============
CL_EventIndex

=============
*/
word CL_EventIndex( const char *name )
{
	int	i;
	
	if( !name || !name[0] )
		return 0;

	for( i = 1; i < MAX_EVENTS && cl.event_precache[i][0]; i++ )
	{
		if( !Q_stricmp( cl.event_precache[i], name ))
			return i;
	}
	return 0;
}

/*
=============
CL_EventIndex

=============
*/
const char *CL_IndexEvent( word index )
{
	if( index < 0 || index >= MAX_EVENTS )
		return 0;

	return cl.event_precache[index];
}

/*
=============
CL_RegisterEvent

=============
*/
void CL_RegisterEvent( int lastnum, const char *szEvName, pfnEventHook func )
{
	cl_user_event_t	*ev;

	if( lastnum == MAX_EVENTS )
	{
		MsgDev( D_ERROR, "CL_RegisterEvent: MAX_EVENTS hit!\n" );
		return;
	}

	// clear existing or allocate new one
	if( !clgame.events[lastnum] )
		clgame.events[lastnum] = Mem_Alloc( cls.mempool, sizeof( cl_user_event_t ));
	else Q_memset( clgame.events[lastnum], 0, sizeof( cl_user_event_t ));

	ev = clgame.events[lastnum];

	// NOTE: ev->index will be set later
	Q_strncpy( ev->name, szEvName, CS_SIZE );
	ev->func = func;
}

/*
=============
CL_FireEvent

=============
*/
qboolean CL_FireEvent( event_info_t *ei )
{
	cl_user_event_t	*ev;
	const char	*name;
	int		i, idx;

	if( !ei || !ei->index )
		return false;

	// get the func pointer
	for( i = 0; i < MAX_EVENTS; i++ )
	{
		ev = clgame.events[i];		

		if( !ev )
		{
			idx = bound( 1, ei->index, MAX_EVENTS );
			MsgDev( D_ERROR, "CL_FireEvent: %s not precached\n", cl.event_precache[idx] );
			break;
		}

		if( ev->index == ei->index )
		{
			if( ev->func )
			{
				ev->func( &ei->args );
				return true;
			}

			name = cl.event_precache[ei->index];
			MsgDev( D_ERROR, "CL_FireEvent: %s not hooked\n", name );
			break;			
		}
	}

	return false;
}

/*
=============
CL_FireEvents

called right before draw frame
=============
*/
void CL_FireEvents( void )
{
	int		i;
	event_state_t	*es;
	event_info_t	*ei;
	qboolean		success;

	es = &cl.events;

	for( i = 0; i < MAX_EVENT_QUEUE; i++ )
	{
		ei = &es->ei[i];

		if( ei->index == 0 )
			continue;

		// delayed event!
		if( ei->fire_time && ( ei->fire_time > cl.time ))
			continue;

		success = CL_FireEvent( ei );

		// zero out the remaining fields
		CL_ResetEvent( ei );
	}
}

/*
=============
CL_FindEvent

find first empty event
=============
*/
event_info_t *CL_FindEmptyEvent( void )
{
	int		i;
	event_state_t	*es;
	event_info_t	*ei;

	es = &cl.events;

	// look for first slot where index is != 0
	for( i = 0; i < MAX_EVENT_QUEUE; i++ )
	{
		ei = &es->ei[i];
		if( ei->index != 0 )
			continue;
		return ei;
	}

	// no slots available
	return NULL;
}

/*
=============
CL_FindEvent

replace only unreliable events
=============
*/
event_info_t *CL_FindUnreliableEvent( void )
{
	event_state_t	*es;
	event_info_t	*ei;
	int		i;

	es = &cl.events;

	for ( i = 0; i < MAX_EVENT_QUEUE; i++ )
	{
		ei = &es->ei[i];
		if( ei->index != 0 )
		{
			// it's reliable, so skip it
			if( ei->flags & FEV_RELIABLE )
				continue;
		}
		return ei;
	}

	// this should never happen
	return NULL;
}

/*
=============
CL_QueueEvent

=============
*/
void CL_QueueEvent( int flags, int index, float delay, event_args_t *args )
{
	qboolean		unreliable = (flags & FEV_RELIABLE) ? false : true;
	event_info_t	*ei;

	// find a normal slot
	ei = CL_FindEmptyEvent();

	if( !ei && unreliable )
		return;

	// okay, so find any old unreliable slot
	if( !ei )
	{
		ei = CL_FindUnreliableEvent();
		if( !ei ) return;
	}

	ei->index	= index;
	ei->fire_time = delay ? (cl.time + delay) : 0.0f;
	ei->flags	= flags;
	ei->args = *args;
}

/*
=============
CL_ParseReliableEvent

=============
*/
void CL_ParseReliableEvent( sizebuf_t *msg )
{
	int		event_index;
	event_args_t	nullargs, args;
	float		delay = 0.0f;
	cl_entity_t	*pEnt;

	Q_memset( &nullargs, 0, sizeof( nullargs ));

	event_index = BF_ReadUBitLong( msg, MAX_EVENT_BITS );

	if( BF_ReadOneBit( msg ))
		delay = (float)BF_ReadWord( msg ) * (1.0f / 100.0f);

	// reliable events not use delta-compression just null-compression
	MSG_ReadDeltaEvent( msg, &nullargs, &args );

	if(( pEnt = CL_GetEntityByIndex( args.entindex )) != NULL )
	{
		if( VectorIsNull( args.origin ))
			VectorCopy( pEnt->curstate.origin, args.origin );
		if( VectorIsNull( args.angles ))
			VectorCopy( pEnt->curstate.angles, args.angles );
		if( VectorIsNull( args.velocity ))
			VectorCopy( pEnt->curstate.velocity, args.velocity );
	}

	CL_QueueEvent( FEV_RELIABLE|FEV_SERVER, event_index, delay, &args );
}


/*
=============
CL_ParseEvent

=============
*/
void CL_ParseEvent( sizebuf_t *msg )
{
	int		event_index;
	int		i, num_events;
	int		packet_ent;
	event_args_t	nullargs, args;
	qboolean		has_update;
	entity_state_t	*state;
	cl_entity_t	*pEnt;
	float		delay;

	Q_memset( &nullargs, 0, sizeof( nullargs ));

	num_events = BF_ReadUBitLong( msg, 5 );

	// parse events queue
	for( i = 0 ; i < num_events; i++ )
	{
		event_index = BF_ReadUBitLong( msg, MAX_EVENT_BITS );
		Q_memset( &args, 0, sizeof( args ));
		has_update = false;

		if( BF_ReadOneBit( msg ))
		{
			packet_ent = BF_ReadUBitLong( msg, MAX_ENTITY_BITS );

			if( BF_ReadOneBit( msg ))
			{
				MSG_ReadDeltaEvent( msg, &nullargs, &args );
				has_update = true;
			}
		}
		else packet_ent = -1;

		if( packet_ent != -1 )
			state = &cls.packet_entities[(cl.frame.first_entity+packet_ent)%cls.num_client_entities];
		else state = NULL;

		// it's a client. Override some params
		if( args.entindex >= 1 && args.entindex <= cl.maxclients )
		{
			if(( args.entindex - 1 ) == cl.playernum )
			{
				if( state && !CL_IsPredicted( ))
				{
					// restore viewangles from angles
					args.angles[PITCH] = -state->angles[PITCH] * 3;
					args.angles[YAW] = state->angles[YAW];
					args.angles[ROLL] = 0; // no roll
				}
				else
				{
					// get the predicted angles
					VectorCopy( cl.refdef.cl_viewangles, args.angles );
				}

				VectorCopy( cl.frame.local.client.origin, args.origin );
				VectorCopy( cl.frame.local.client.velocity, args.velocity );
			}
			else if( state )
			{
				// restore viewangles from angles
				args.angles[PITCH] = -state->angles[PITCH] * 3;
				args.angles[YAW] = state->angles[YAW];
				args.angles[ROLL] = 0; // no roll
			}
		}
		else if( state )
		{
			if( VectorIsNull( args.origin ))
				VectorCopy( state->origin, args.origin );
			if( VectorIsNull( args.angles ))
				VectorCopy( state->angles, args.angles );
			if( VectorIsNull( args.velocity ))
				VectorCopy( state->velocity, args.velocity );
		}
		else if(( pEnt = CL_GetEntityByIndex( args.entindex )) != NULL )
		{
			if( VectorIsNull( args.origin ))
				VectorCopy( pEnt->curstate.origin, args.origin );
			if( VectorIsNull( args.angles ))
				VectorCopy( pEnt->curstate.angles, args.angles );
			if( VectorIsNull( args.velocity ))
				VectorCopy( pEnt->curstate.velocity, args.velocity );
		}

		if( BF_ReadOneBit( msg ))
			delay = (float)BF_ReadWord( msg ) * (1.0f / 100.0f);
		else delay = 0.0f;

		// g-cont. should we need find the event with same index?
		CL_QueueEvent( 0, event_index, delay, &args );
	}
}

/*
=============
CL_PlaybackEvent

=============
*/
void CL_PlaybackEvent( int flags, const edict_t *pInvoker, word eventindex, float delay, float *origin,
	float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 )
{
	event_args_t	args;
	int		invokerIndex = 0;

	// first check event for out of bounds
	if( eventindex < 1 || eventindex > MAX_EVENTS )
	{
		MsgDev( D_ERROR, "CL_PlaybackEvent: invalid eventindex %i\n", eventindex );
		return;
	}
	// check event for precached
	if( !CL_EventIndex( cl.event_precache[eventindex] ))
	{
		MsgDev( D_ERROR, "CL_PlaybackEvent: event %i was not precached\n", eventindex );
		return;		
	}

	flags |= FEV_CLIENT; // it's a client event
	flags &= ~(FEV_NOTHOST|FEV_HOSTONLY|FEV_GLOBAL);

	if( delay < 0.0f ) delay = 0.0f; // fixup negative delays
	invokerIndex = cl.playernum + 1; // only local client can issue client events

	args.flags = 0;
	args.entindex = invokerIndex;

// TODO: restore checks when predicting will be done
//	if( !angles || VectorIsNull( angles ))
		VectorCopy( cl.refdef.cl_viewangles, args.angles );

//	if( !origin || VectorIsNull( origin ))
		VectorCopy( cl.frame.local.client.origin, args.origin );

	VectorCopy( cl.frame.local.client.velocity, args.velocity );
	args.ducking = cl.frame.local.playerstate.usehull;

	args.fparam1 = fparam1;
	args.fparam2 = fparam2;
	args.iparam1 = iparam1;
	args.iparam2 = iparam2;
	args.bparam1 = bparam1;
	args.bparam2 = bparam2;

	CL_QueueEvent( flags, eventindex, delay, &args );
}