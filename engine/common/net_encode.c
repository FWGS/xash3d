/*
net_encode.c - encode network messages
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
#include "netchan.h"
#include "mathlib.h"
#include "net_encode.h"
#include "event_api.h"
#include "usercmd.h"
#include "pm_movevars.h"
#include "protocol.h"
#include "client.h"

#define DELTA_PATH		"delta.lst"

static qboolean		delta_init = false;
 
// list of all the struct names
static const delta_field_t cmd_fields[] =
{
{ UCMD_DEF( lerp_msec )		},
{ UCMD_DEF( msec )			},
{ UCMD_DEF( viewangles[0] )		},
{ UCMD_DEF( viewangles[1] )		},
{ UCMD_DEF( viewangles[2] )		},
{ UCMD_DEF( forwardmove )		},
{ UCMD_DEF( sidemove )		},
{ UCMD_DEF( upmove )		},
{ UCMD_DEF( lightlevel )		},
{ UCMD_DEF( buttons )		},
{ UCMD_DEF( impulse )		},
{ UCMD_DEF( weaponselect )		},
{ UCMD_DEF( impact_index )		},
{ UCMD_DEF( impact_position[0] )	},
{ UCMD_DEF( impact_position[1] )	},
{ UCMD_DEF( impact_position[2] )	},
{ NULL },
};

static const delta_field_t pm_fields[] =
{
{ PHYS_DEF( gravity )		},
{ PHYS_DEF( stopspeed )		},
{ PHYS_DEF( maxspeed )		},
{ PHYS_DEF( spectatormaxspeed )	},
{ PHYS_DEF( accelerate )		},
{ PHYS_DEF( airaccelerate )		},
{ PHYS_DEF( wateraccelerate )		},
{ PHYS_DEF( friction )		},
{ PHYS_DEF( edgefriction )		},
{ PHYS_DEF( waterfriction )		},
{ PHYS_DEF( bounce )		},
{ PHYS_DEF( stepsize )		},
{ PHYS_DEF( maxvelocity )		},
{ PHYS_DEF( zmax )			},
{ PHYS_DEF( waveHeight )		},
{ PHYS_DEF( footsteps )		},
{ PHYS_DEF( skyName )		},
{ PHYS_DEF( rollangle )		},
{ PHYS_DEF( rollspeed )		},
{ PHYS_DEF( skycolor_r )		},
{ PHYS_DEF( skycolor_g )		},
{ PHYS_DEF( skycolor_b )		},
{ PHYS_DEF( skyvec_x )		},
{ PHYS_DEF( skyvec_y )		},
{ PHYS_DEF( skyvec_z )		},
{ PHYS_DEF( fog_settings )		},
{ PHYS_DEF( wateralpha )		},
{ PHYS_DEF( skydir_x )		},
{ PHYS_DEF( skydir_y )		},
{ PHYS_DEF( skydir_z )		},
{ PHYS_DEF( skyangle )		},
{ NULL },
};

static const delta_field_t ev_fields[] =
{
{ EVNT_DEF( flags )		},
{ EVNT_DEF( entindex )	},
{ EVNT_DEF( origin[0] )	},
{ EVNT_DEF( origin[1] )	},
{ EVNT_DEF( origin[2] )	},
{ EVNT_DEF( angles[0] )	},
{ EVNT_DEF( angles[1] )	},
{ EVNT_DEF( angles[2] )	},
{ EVNT_DEF( velocity[0] )	},
{ EVNT_DEF( velocity[1] )	},
{ EVNT_DEF( velocity[2] )	},
{ EVNT_DEF( ducking )	},
{ EVNT_DEF( fparam1 )	},
{ EVNT_DEF( fparam2 )	},
{ EVNT_DEF( iparam1 )	},
{ EVNT_DEF( iparam2 )	},
{ EVNT_DEF( bparam1 )	},
{ EVNT_DEF( bparam2 )	},
{ NULL },
};

static const delta_field_t wd_fields[] =
{
{ WPDT_DEF( m_iId )			},
{ WPDT_DEF( m_iClip )		},
{ WPDT_DEF( m_flNextPrimaryAttack )	},
{ WPDT_DEF( m_flNextSecondaryAttack )	},
{ WPDT_DEF( m_flTimeWeaponIdle )	},
{ WPDT_DEF( m_fInReload )		},
{ WPDT_DEF( m_fInSpecialReload )	},
{ WPDT_DEF( m_flNextReload )		},
{ WPDT_DEF( m_flPumpTime )		},
{ WPDT_DEF( m_fReloadTime )		},
{ WPDT_DEF( m_fAimedDamage )		},
{ WPDT_DEF( m_fNextAimBonus )		},
{ WPDT_DEF( m_fInZoom )		},
{ WPDT_DEF( m_iWeaponState )		},
{ WPDT_DEF( iuser1 )		},
{ WPDT_DEF( iuser2 )		},
{ WPDT_DEF( iuser3 )		},
{ WPDT_DEF( iuser4 )		},
{ WPDT_DEF( fuser1 )		},
{ WPDT_DEF( fuser2 )		},
{ WPDT_DEF( fuser3 )		},
{ WPDT_DEF( fuser4 )		},
{ NULL },
};

static const delta_field_t cd_fields[] =
{
{ CLDT_DEF( origin[0] )	},
{ CLDT_DEF( origin[1] )	},
{ CLDT_DEF( origin[2] )	},
{ CLDT_DEF( velocity[0] )	},
{ CLDT_DEF( velocity[1] )	},
{ CLDT_DEF( velocity[2] )	},
{ CLDT_DEF( viewmodel )	},
{ CLDT_DEF( punchangle[0] )	},
{ CLDT_DEF( punchangle[1] )	},
{ CLDT_DEF( punchangle[2] )	},
{ CLDT_DEF( flags )		},
{ CLDT_DEF( waterlevel )	},
{ CLDT_DEF( watertype )	},
{ CLDT_DEF( view_ofs[0] )	},
{ CLDT_DEF( view_ofs[1] )	},
{ CLDT_DEF( view_ofs[2] )	},
{ CLDT_DEF( health )	},
{ CLDT_DEF( bInDuck )	},
{ CLDT_DEF( weapons )	},
{ CLDT_DEF( flTimeStepSound )	},
{ CLDT_DEF( flDuckTime )	},
{ CLDT_DEF( flSwimTime )	},
{ CLDT_DEF( waterjumptime )	},
{ CLDT_DEF( maxspeed )	},
{ CLDT_DEF( fov )		},
{ CLDT_DEF( weaponanim )	},
{ CLDT_DEF( m_iId )		},
{ CLDT_DEF( ammo_shells )	},
{ CLDT_DEF( ammo_nails )	},
{ CLDT_DEF( ammo_cells )	},
{ CLDT_DEF( ammo_rockets )	},
{ CLDT_DEF( m_flNextAttack )	},
{ CLDT_DEF( tfstate )	},
{ CLDT_DEF( pushmsec )	},
{ CLDT_DEF( deadflag )	},
{ CLDT_DEF( physinfo )	},
{ CLDT_DEF( iuser1 )	},
{ CLDT_DEF( iuser2 )	},
{ CLDT_DEF( iuser3 )	},
{ CLDT_DEF( iuser4 )	},
{ CLDT_DEF( fuser1 )	},
{ CLDT_DEF( fuser2 )	},
{ CLDT_DEF( fuser3 )	},
{ CLDT_DEF( fuser4 )	},
{ CLDT_DEF( vuser1[0] )	},
{ CLDT_DEF( vuser1[1] )	},
{ CLDT_DEF( vuser1[2] )	},
{ CLDT_DEF( vuser2[0] )	},
{ CLDT_DEF( vuser2[1] )	},
{ CLDT_DEF( vuser2[2] )	},
{ CLDT_DEF( vuser3[0] )	},
{ CLDT_DEF( vuser3[1] )	},
{ CLDT_DEF( vuser3[2] )	},
{ CLDT_DEF( vuser4[0] )	},
{ CLDT_DEF( vuser4[1] )	},
{ CLDT_DEF( vuser4[2] )	},
{ NULL },
};

static const delta_field_t ent_fields[] =
{
{ ENTS_DEF( entityType )	},
{ ENTS_DEF( origin[0] )	},
{ ENTS_DEF( origin[1] )	},
{ ENTS_DEF( origin[2] )	},
{ ENTS_DEF( angles[0] )	},
{ ENTS_DEF( angles[1] )	},
{ ENTS_DEF( angles[2] )	},
{ ENTS_DEF( modelindex )	},
{ ENTS_DEF( sequence )	},
{ ENTS_DEF( frame )		},
{ ENTS_DEF( colormap )	},
{ ENTS_DEF( skin )		},
{ ENTS_DEF( solid )		},
{ ENTS_DEF( effects )	},
{ ENTS_DEF( scale )		},
{ ENTS_DEF( eflags )	},
{ ENTS_DEF( rendermode )	},
{ ENTS_DEF( renderamt )	},
{ ENTS_DEF( rendercolor.r )	},
{ ENTS_DEF( rendercolor.g )	},
{ ENTS_DEF( rendercolor.b )	},
{ ENTS_DEF( renderfx )	},
{ ENTS_DEF( movetype )	},
{ ENTS_DEF( animtime )	},
{ ENTS_DEF( framerate )	},
{ ENTS_DEF( body )		},
{ ENTS_DEF( controller[0] )	},
{ ENTS_DEF( controller[1] )	},
{ ENTS_DEF( controller[2] )	},
{ ENTS_DEF( controller[3] )	},
{ ENTS_DEF( blending[0] )	},
{ ENTS_DEF( blending[1] )	},
{ ENTS_DEF( blending[2] )	},
{ ENTS_DEF( blending[3] )	},
{ ENTS_DEF( velocity[0] )	},
{ ENTS_DEF( velocity[1] )	},
{ ENTS_DEF( velocity[2] )	},
{ ENTS_DEF( mins[0] )	},
{ ENTS_DEF( mins[1] )	},
{ ENTS_DEF( mins[2] )	},
{ ENTS_DEF( maxs[0] )	},
{ ENTS_DEF( maxs[1] )	},
{ ENTS_DEF( maxs[2] )	},
{ ENTS_DEF( aiment )	},
{ ENTS_DEF( owner )		},
{ ENTS_DEF( friction )	},
{ ENTS_DEF( gravity )	},
{ ENTS_DEF( team )		},
{ ENTS_DEF( playerclass )	},
{ ENTS_DEF( health )	},
{ ENTS_DEF( spectator )	},
{ ENTS_DEF( weaponmodel )	},
{ ENTS_DEF( gaitsequence )	},
{ ENTS_DEF( basevelocity[0] )	},
{ ENTS_DEF( basevelocity[1] )	},
{ ENTS_DEF( basevelocity[2] )	},
{ ENTS_DEF( usehull )	},
{ ENTS_DEF( oldbuttons )	},	// probably never transmitted
{ ENTS_DEF( onground )	},
{ ENTS_DEF( iStepLeft )	},
{ ENTS_DEF( flFallVelocity )	},
{ ENTS_DEF( fov )		},
{ ENTS_DEF( weaponanim )	},
{ ENTS_DEF( startpos[0] )	},
{ ENTS_DEF( startpos[1] )	},
{ ENTS_DEF( startpos[2] )	},
{ ENTS_DEF( endpos[0] )	},
{ ENTS_DEF( endpos[1] )	},
{ ENTS_DEF( endpos[2] )	},
{ ENTS_DEF( impacttime )	},
{ ENTS_DEF( starttime )	},
{ ENTS_DEF( iuser1 )	},
{ ENTS_DEF( iuser2 )	},
{ ENTS_DEF( iuser3 )	},
{ ENTS_DEF( iuser4 )	},
{ ENTS_DEF( fuser1 )	},
{ ENTS_DEF( fuser2 )	},
{ ENTS_DEF( fuser3 )	},
{ ENTS_DEF( fuser4 )	},
{ ENTS_DEF( vuser1[0] )	},
{ ENTS_DEF( vuser1[1] )	},
{ ENTS_DEF( vuser1[2] )	},
{ ENTS_DEF( vuser2[0] )	},
{ ENTS_DEF( vuser2[1] )	},
{ ENTS_DEF( vuser2[2] )	},
{ ENTS_DEF( vuser3[0] )	},
{ ENTS_DEF( vuser3[1] )	},
{ ENTS_DEF( vuser3[2] )	},
{ ENTS_DEF( vuser4[0] )	},
{ ENTS_DEF( vuser4[1] )	},
{ ENTS_DEF( vuser4[2] )	},
{ NULL },
};

static delta_info_t dt_info[] =
{
{ "event_t", ev_fields, NUM_FIELDS( ev_fields ) },
{ "movevars_t", pm_fields, NUM_FIELDS( pm_fields ) },
{ "usercmd_t", cmd_fields, NUM_FIELDS( cmd_fields ) },
{ "clientdata_t", cd_fields, NUM_FIELDS( cd_fields ) },
{ "weapon_data_t", wd_fields, NUM_FIELDS( wd_fields ) },
{ "entity_state_t", ent_fields, NUM_FIELDS( ent_fields ) },
{ "entity_state_player_t", ent_fields, NUM_FIELDS( ent_fields ) },
{ "custom_entity_state_t", ent_fields, NUM_FIELDS( ent_fields ) },
{ NULL },
};

delta_info_t *Delta_FindStruct( const char *name )
{
	int	i;

	if( !name || !name[0] )
		return NULL;

	for( i = 0; i < NUM_FIELDS( dt_info ); i++ )
	{
		if( !Q_stricmp( dt_info[i].pName, name ))
			return &dt_info[i];
	}

	MsgDev( D_WARN, "Struct %s not found in delta_info\n", name );

	// found nothing
	return NULL;
}

int Delta_NumTables( void )
{
	return NUM_FIELDS( dt_info );
}

delta_info_t *Delta_FindStructByIndex( int index )
{
	if( index < 0 || index >= NUM_FIELDS( dt_info ))
		return NULL;

	return &dt_info[index];
}

delta_info_t *Delta_FindStructByEncoder( const char *encoderName )
{
	int	i;

	if( !encoderName || !encoderName[0] )
		return NULL;

	for( i = 0; i < NUM_FIELDS( dt_info ); i++ )
	{
		if( !Q_stricmp( dt_info[i].funcName, encoderName ))
			return &dt_info[i];
	}
	// found nothing
	return NULL;
}

delta_info_t *Delta_FindStructByDelta( const delta_t *pFields )
{
	int	i;

	if( !pFields ) return NULL;

	for( i = 0; i < NUM_FIELDS( dt_info ); i++ )
	{
		if( dt_info[i].pFields == pFields )
			return &dt_info[i];
	}
	// found nothing
	return NULL;
}

void Delta_CustomEncode( delta_info_t *dt, const void *from, const void *to )
{
	int	i;

	ASSERT( dt != NULL );

	// set all fields is active by default
	for( i = 0; i < dt->numFields; i++ )
		dt->pFields[i].bInactive = false;

	if( dt->userCallback )
	{
		dt->userCallback( dt->pFields, from, to );
	}
}

delta_field_t *Delta_FindFieldInfo( const delta_field_t *pInfo, const char *fieldName )
{
	if( !fieldName || !*fieldName )
		return NULL;	

	for( ; pInfo->name; pInfo++ )
	{
		if( !Q_strcmp( pInfo->name, fieldName ))
			return (delta_field_t *)pInfo;
	}
	return NULL;
}

int Delta_IndexForFieldInfo( const delta_field_t *pInfo, const char *fieldName )
{
	int	i;

	if( !fieldName || !*fieldName )
		return -1;	

	for( i = 0; pInfo->name; i++, pInfo++ )
	{
		if( !Q_strcmp( pInfo->name, fieldName ))
			return i;
	}
	return -1;
}

qboolean Delta_AddField( const char *pStructName, const char *pName, int flags, int bits, float mul, float post_mul )
{
	delta_info_t	*dt;
	delta_field_t	*pFieldInfo;
	delta_t		*pField;
	int		i;

	// get the delta struct
	dt = Delta_FindStruct( pStructName );
	ASSERT( dt != NULL );

	// check for coexisting field
	for( i = 0, pField = dt->pFields; i < dt->numFields; i++, pField++ )
	{
		if( !Q_strcmp( pField->name, pName ))
		{
			MsgDev( D_NOTE, "Delta_Add: %s->%s already existing\n", pStructName, pName );
			return false; // field already exist		
		}
	}

	// find field description
	pFieldInfo = Delta_FindFieldInfo( dt->pInfo, pName );
	if( !pFieldInfo )
	{
		MsgDev( D_ERROR, "Delta_Add: couldn't find description for %s->%s\n", pStructName, pName );
		return false;
	}

	if( dt->numFields + 1 > dt->maxFields )
	{
		MsgDev( D_WARN, "Delta_Add: can't add %s->%s encoder list is full\n", pStructName, pName );
		return false; // too many fields specified (duplicated ?)
	}

	// allocate a new one
	dt->pFields = Z_Realloc( dt->pFields, (dt->numFields + 1) * sizeof( delta_t ));	
	for( i = 0, pField = dt->pFields; i < dt->numFields; i++, pField++ );

	// copy info to new field
	pField->name = pFieldInfo->name;
	pField->offset = pFieldInfo->offset;
	pField->size = pFieldInfo->size;
	pField->flags = flags;
	pField->bits = bits;
	pField->multiplier = mul;
	pField->post_multiplier = post_mul;
	dt->numFields++;

	return true;
}

void Delta_WriteTableField( sizebuf_t *msg, int tableIndex, const delta_t *pField )
{
	int		nameIndex;
	delta_info_t	*dt;
	
	ASSERT( pField );

	if( !pField->name || !*pField->name )
		return;	// not initialized ?

	dt = Delta_FindStructByIndex( tableIndex );
	ASSERT( dt && dt->bInitialized );

	nameIndex = Delta_IndexForFieldInfo( dt->pInfo, pField->name );
	ASSERT( nameIndex >= 0 && nameIndex < dt->maxFields );

	BF_WriteByte( msg, svc_deltatable );
	BF_WriteUBitLong( msg, tableIndex, 4 );		// assume we support 16 network tables
	BF_WriteUBitLong( msg, nameIndex, 8 );		// 255 fields by struct should be enough
	BF_WriteUBitLong( msg, pField->flags, 10 );	// flags are indicated various input types
	BF_WriteUBitLong( msg, pField->bits - 1, 5 );	// max received value is 32 (32 bit)

	// multipliers is null-compressed
	if( pField->multiplier != 1.0f )
	{
		BF_WriteOneBit( msg, 1 );
		BF_WriteFloat( msg, pField->multiplier );
	}
	else BF_WriteOneBit( msg, 0 );

	if( pField->post_multiplier != 1.0f )
	{
		BF_WriteOneBit( msg, 1 );
		BF_WriteFloat( msg, pField->post_multiplier );
	}
	else BF_WriteOneBit( msg, 0 );
}

void Delta_ParseTableField( sizebuf_t *msg )
{
	int		tableIndex, nameIndex;
	float		mul = 1.0f, post_mul = 1.0f;
	int		flags, bits;
	const char	*pName;
	delta_info_t	*dt;

	tableIndex = BF_ReadUBitLong( msg, 4 );
	dt = Delta_FindStructByIndex( tableIndex );

	if( !dt )
		Host_Error( "Delta_ParseTableField: not initialized" );
	nameIndex = BF_ReadUBitLong( msg, 8 );	// read field name index		
	if( !( nameIndex >= 0 && nameIndex < dt->maxFields ) )
		Host_Error( "Delta_ParseTableField: wrong nameIndex" );
	pName = dt->pInfo[nameIndex].name;
	flags = BF_ReadUBitLong( msg, 10 );
	bits = BF_ReadUBitLong( msg, 5 ) + 1;

	// read the multipliers
	if( BF_ReadOneBit( msg ))
		mul = BF_ReadFloat( msg );

	if( BF_ReadOneBit( msg ))
		post_mul = BF_ReadFloat( msg );

	// delta encoders it's already initialized on this machine (local game)
	if( delta_init ) return;

	// add field to table
	Delta_AddField( dt->pName, pName, flags, bits, mul, post_mul );
}

qboolean Delta_ParseField( char **delta_script, const delta_field_t *pInfo, delta_t *pField, qboolean bPost )
{
	string		token;
	delta_field_t	*pFieldInfo;
	char		*oldpos;

	*delta_script = COM_ParseFile( *delta_script, token );
	if( Q_strcmp( token, "(" ))
	{
		MsgDev( D_ERROR, "Delta_ParseField: expected '(', found '%s' instead\n", token );
		return false;
	}

	// read the variable name
	if(( *delta_script = COM_ParseFile( *delta_script, token )) == NULL )
	{
		MsgDev( D_ERROR, "Delta_ParseField: missing field name\n" );
		return false;
	}

	pFieldInfo = Delta_FindFieldInfo( pInfo, token );
	if( !pFieldInfo )
	{
		MsgDev( D_ERROR, "Delta_ParseField: unable to find field %s\n", token );
		return false;
	}

	*delta_script = COM_ParseFile( *delta_script, token );
	if( Q_strcmp( token, "," ))
	{
		MsgDev( D_ERROR, "Delta_ParseField: expected ',', found '%s' instead\n", token );
		return false;
	}

	// copy base info to new field
	pField->name = pFieldInfo->name;
	pField->offset = pFieldInfo->offset;
	pField->size = pFieldInfo->size;
	pField->flags = 0;

	// read delta-flags
	while(( *delta_script = COM_ParseFile( *delta_script, token )) != NULL )
	{
		if( !Q_strcmp( token, "," ))
			break;	// end of flags argument

		if( !Q_strcmp( token, "|" ))
			continue;

		if( !Q_strcmp( token, "DT_BYTE" ))
			pField->flags |= DT_BYTE;
		else if( !Q_strcmp( token, "DT_SHORT" ))
			pField->flags |= DT_SHORT;
		else if( !Q_strcmp( token, "DT_FLOAT" ))
			pField->flags |= DT_FLOAT;
		else if( !Q_strcmp( token, "DT_INTEGER" ))
			pField->flags |= DT_INTEGER;
		else if( !Q_strcmp( token, "DT_ANGLE" ))
			pField->flags |= DT_ANGLE;
		else if( !Q_strcmp( token, "DT_TIMEWINDOW_8" ))
			pField->flags |= DT_TIMEWINDOW_8;
		else if( !Q_strcmp( token, "DT_TIMEWINDOW_BIG" ))
			pField->flags |= DT_TIMEWINDOW_BIG;
		else if( !Q_strcmp( token, "DT_STRING" ))
			pField->flags |= DT_STRING;
		else if( !Q_strcmp( token, "DT_SIGNED" ))
			pField->flags |= DT_SIGNED;
	}

	if( Q_strcmp( token, "," ))
	{
		MsgDev( D_ERROR, "Delta_ParseField: expected ',', found '%s' instead\n", token );
		return false;
	}

	// read delta-bits

	if(( *delta_script = COM_ParseFile( *delta_script, token )) == NULL )
	{
		MsgDev( D_ERROR, "Delta_ReadField: %s field bits argument is missing\n", pField->name );
		return false;
	}

	pField->bits = Q_atoi( token );

	*delta_script = COM_ParseFile( *delta_script, token ); 
	if( Q_strcmp( token, "," ))
	{
		MsgDev( D_ERROR, "Delta_ReadField: expected ',', found '%s' instead\n", token );
		return false;
	}

	// read delta-multiplier
	if(( *delta_script = COM_ParseFile( *delta_script, token )) == NULL )
	{
		MsgDev( D_ERROR, "Delta_ReadField: %s missing 'multiplier' argument\n", pField->name );
		return false;
	}

	pField->multiplier = Q_atof( token );

	if( bPost )
	{
		*delta_script = COM_ParseFile( *delta_script, token );
		if( Q_strcmp( token, "," ))
		{
			MsgDev( D_ERROR, "Delta_ReadField: expected ',', found '%s' instead\n", token );
			return false;
		}

		// read delta-postmultiplier
		if(( *delta_script = COM_ParseFile( *delta_script, token )) == NULL )
		{
			MsgDev( D_ERROR, "Delta_ReadField: %s missing 'post_multiply' argument\n", pField->name );
			return false;
		}

		pField->post_multiplier = Q_atof( token );
	}
	else
	{
		// to avoid division by zero
		pField->post_multiplier = 1.0f;
	}

	// closing brace...
	*delta_script = COM_ParseFile( *delta_script, token );
	if( Q_strcmp( token, ")" ))
	{
		MsgDev( D_ERROR, "Delta_ParseField: expected ')', found '%s' instead\n", token );
		return false;
	}

	// ... and trying to parse optional ',' post-symbol
	oldpos = *delta_script;
	*delta_script = COM_ParseFile( *delta_script, token );
	if( token[0] != ',' ) *delta_script = oldpos; // not a ','

	return true;
}

void Delta_ParseTable( char **delta_script, delta_info_t *dt, const char *encodeDll, const char *encodeFunc )
{
	string		token;
	delta_t		*pField;
	const delta_field_t	*pInfo;

	// allocate the delta-structures
	if( !dt->pFields ) dt->pFields = (delta_t *)Z_Malloc( dt->maxFields * sizeof( delta_t ));

	pField = dt->pFields;
	pInfo = dt->pInfo;
	dt->numFields = 0;

	// assume we have handled '{'
	while(( *delta_script = COM_ParseFile( *delta_script, token )) != NULL )
	{
		ASSERT( dt->numFields <= dt->maxFields );

		if( !Q_strcmp( token, "DEFINE_DELTA" ))
		{
			if( Delta_ParseField( delta_script, pInfo, &pField[dt->numFields], false ))
				dt->numFields++;
		}
		else if( !Q_strcmp( token, "DEFINE_DELTA_POST" ))
		{
			if( Delta_ParseField( delta_script, pInfo, &pField[dt->numFields], true ))
				dt->numFields++;
		}
		else if( token[0] == '}' )
		{
			// end of the section
			break;
		}
	}

	// copy function name
	Q_strncpy( dt->funcName, encodeFunc, sizeof( dt->funcName ));

	if( !Q_stricmp( encodeDll, "none" ))
		dt->customEncode = CUSTOM_NONE;
	else if( !Q_stricmp( encodeDll, "gamedll" ))
		dt->customEncode = CUSTOM_SERVER_ENCODE;
	else if( !Q_stricmp( encodeDll, "clientdll" ))
		dt->customEncode = CUSTOM_CLIENT_ENCODE;

	// adjust to fit memory size
	if( dt->numFields < dt->maxFields )
	{
		dt->pFields = Z_Realloc( dt->pFields, dt->numFields * sizeof( delta_t ));
	}

	dt->bInitialized = true; // table is ok
}

void Delta_InitFields( void )
{
	char		*afile, *pfile;
	string		encodeDll, encodeFunc, token;	
	delta_info_t	*dt;

	afile = (char *)FS_LoadFile( DELTA_PATH, NULL, false );
	if( !afile ) Sys_Error( "DELTA_Load: couldn't load file %s\n", DELTA_PATH );

	pfile = afile;

	while(( pfile = COM_ParseFile( pfile, token )) != NULL )
	{
		dt = Delta_FindStruct( token );
		if( dt == NULL )
		{
			Sys_Error( "delta.lst: unknown struct %s\n", token );
		}

		pfile = COM_ParseFile( pfile, encodeDll );

		if( !Q_stricmp( encodeDll, "none" ))
			Q_strcpy( encodeFunc, "null" );
		else pfile = COM_ParseFile( pfile, encodeFunc );

		// jump to '{'
		pfile = COM_ParseFile( pfile, token );
		if( token[0] != '{' )
		{
			Sys_Error( "delta.lst: missing '{' in section %s\n", dt->pName );
		}			

		Delta_ParseTable( &pfile, dt, encodeDll, encodeFunc );
	}
	Mem_Free( afile );

	// adding some required fields that user may forget or don't know how to specified
	Delta_AddField( "event_t", "velocity[0]", DT_SIGNED | DT_FLOAT, 16, 8.0f, 1.0f );
	Delta_AddField( "event_t", "velocity[1]", DT_SIGNED | DT_FLOAT, 16, 8.0f, 1.0f );
	Delta_AddField( "event_t", "velocity[2]", DT_SIGNED | DT_FLOAT, 16, 8.0f, 1.0f );	
}

void Delta_Init( void )
{
	delta_info_t	*dt;

	// shutdown it first
	if( delta_init ) Delta_Shutdown ();

	Delta_InitFields ();	// initialize fields
	delta_init = true;

	dt = Delta_FindStruct( "movevars_t" );

	ASSERT( dt != NULL );
	if( dt->bInitialized ) return;	// "movevars_t" already specified by user

	// create movevars_t delta internal
	Delta_AddField( "movevars_t", "gravity", DT_FLOAT|DT_SIGNED, 16, 8.0f, 1.0f );
	Delta_AddField( "movevars_t", "stopspeed", DT_FLOAT|DT_SIGNED, 16, 8.0f, 1.0f );
	Delta_AddField( "movevars_t", "maxspeed", DT_FLOAT|DT_SIGNED, 16, 8.0f, 1.0f );
	Delta_AddField( "movevars_t", "spectatormaxspeed", DT_FLOAT|DT_SIGNED, 16, 8.0f, 1.0f );
	Delta_AddField( "movevars_t", "accelerate", DT_FLOAT|DT_SIGNED, 16, 8.0f, 1.0f );
	Delta_AddField( "movevars_t", "airaccelerate", DT_FLOAT|DT_SIGNED, 16, 8.0f, 1.0f );
	Delta_AddField( "movevars_t", "wateraccelerate", DT_FLOAT|DT_SIGNED, 16, 8.0f, 1.0f );
	Delta_AddField( "movevars_t", "friction", DT_FLOAT|DT_SIGNED, 16, 8.0f, 1.0f );
	Delta_AddField( "movevars_t", "edgefriction", DT_FLOAT|DT_SIGNED, 16, 8.0f, 1.0f );
	Delta_AddField( "movevars_t", "waterfriction", DT_FLOAT|DT_SIGNED, 16, 8.0f, 1.0f );
	Delta_AddField( "movevars_t", "bounce", DT_FLOAT|DT_SIGNED, 16, 8.0f, 1.0f );
	Delta_AddField( "movevars_t", "stepsize", DT_FLOAT|DT_SIGNED, 16, 16.0f, 1.0f );
	Delta_AddField( "movevars_t", "maxvelocity", DT_FLOAT|DT_SIGNED, 16, 8.0f, 1.0f );
	if( host.features & ENGINE_WRITE_LARGE_COORD )
		Delta_AddField( "movevars_t", "zmax", DT_FLOAT|DT_SIGNED, 18, 1.0f, 1.0f );	// no fractional part
	else
		Delta_AddField( "movevars_t", "zmax", DT_FLOAT|DT_SIGNED, 16, 1.0f, 1.0f );	// no fractional part
	Delta_AddField( "movevars_t", "waveHeight", DT_FLOAT|DT_SIGNED, 16, 16.0f, 8.0f );
	Delta_AddField( "movevars_t", "skyName", DT_STRING, 1, 1.0f, 1.0f ); 
	Delta_AddField( "movevars_t", "footsteps", DT_INTEGER, 1, 1.0f, 1.0f );
	Delta_AddField( "movevars_t", "rollangle", DT_FLOAT|DT_SIGNED, 16, 32.0f, 1.0f );
	Delta_AddField( "movevars_t", "rollspeed", DT_FLOAT|DT_SIGNED, 16, 8.0f, 1.0f );
	Delta_AddField( "movevars_t", "skycolor_r", DT_FLOAT|DT_SIGNED, 12, 1.0f, 1.0f ); // 0 - 264
	Delta_AddField( "movevars_t", "skycolor_g", DT_FLOAT|DT_SIGNED, 12, 1.0f, 1.0f );
	Delta_AddField( "movevars_t", "skycolor_b", DT_FLOAT|DT_SIGNED, 12, 1.0f, 1.0f );
	Delta_AddField( "movevars_t", "skyvec_x", DT_FLOAT|DT_SIGNED, 16, 32.0f, 1.0f ); // 0 - 1
	Delta_AddField( "movevars_t", "skyvec_y", DT_FLOAT|DT_SIGNED, 16, 32.0f, 1.0f );
	Delta_AddField( "movevars_t", "skyvec_z", DT_FLOAT|DT_SIGNED, 16, 32.0f, 1.0f );
	Delta_AddField( "movevars_t", "skydir_x", DT_FLOAT|DT_SIGNED, 16, 32.0f, 1.0f ); // 0 - 1
	Delta_AddField( "movevars_t", "skydir_y", DT_FLOAT|DT_SIGNED, 16, 32.0f, 1.0f );
	Delta_AddField( "movevars_t", "skydir_z", DT_FLOAT|DT_SIGNED, 16, 32.0f, 1.0f );
	Delta_AddField( "movevars_t", "skyangle", DT_ANGLE, 16, 1.0f, 1.0f ); // 0 - 360
	Delta_AddField( "movevars_t", "wateralpha", DT_FLOAT|DT_SIGNED, 16, 32.0f, 1.0f );
	Delta_AddField( "movevars_t", "fog_settings", DT_INTEGER, 32, 1.0f, 1.0f );
	// now done
	dt->bInitialized = true;
}

void Delta_InitClient( void )
{
	int	i, numActive = 0;

	// already initalized
	if( delta_init ) return;

	for( i = 0; i < NUM_FIELDS( dt_info ); i++ )
	{
		if( dt_info[i].numFields > 0 )
		{
			dt_info[i].bInitialized = true;
			numActive++;
		}
	}

	if( numActive ) delta_init = true;
}

void Delta_Shutdown( void )
{
	int	i;

	if( !delta_init ) return;

	for( i = 0; i < NUM_FIELDS( dt_info ); i++ )
	{
		dt_info[i].numFields = 0;
		dt_info[i].customEncode = CUSTOM_NONE;
		dt_info[i].userCallback = NULL;
		dt_info[i].funcName[0] = '\0';

		if( dt_info[i].pFields )
		{
			Mem_Free( dt_info[i].pFields );
			dt_info[i].pFields = NULL;
		}

		dt_info[i].bInitialized = false;
	}

	delta_init = false;
}

/*
=====================
Delta_ClampIntegerField

prevent data to out of range
=====================
*/
int Delta_ClampIntegerField( int iValue, qboolean bSigned, int bits )
{
	switch( bits )
	{
	case 1:
		iValue = bound( 0, (byte)iValue, 1 );
		break;
	case 2:
		if( bSigned ) iValue = bound( -2, (short)iValue, 1 );
		else iValue = boundmax( (word)iValue, 3 );
		break;
	case 3:
		if( bSigned ) iValue = bound( -4, (short)iValue, 3 );
		else iValue = boundmax( (word)iValue, 7 );
		break;
	case 4:
		if( bSigned ) iValue = bound( -8, (short)iValue, 7 );
		else iValue = boundmax( (word)iValue, 15 );
		break;
	case 5:
		if( bSigned ) iValue = bound( -16, (short)iValue, 15 );
		else iValue = boundmax( (word)iValue, 31 );
		break;
	case 6:
		if( bSigned ) iValue = bound( -32, (short)iValue, 31 );
		else iValue = boundmax( (word)iValue, 63 );
		break;
	case 7:
		if( bSigned ) iValue = bound( -64, (short)iValue, 63 );
		else iValue = boundmax( (word)iValue, 127 );
		break;
	case 8:
		if( bSigned ) iValue = bound( -128, (short)iValue, 127 );
		else iValue = boundmax( (word)iValue, 255 );
		break;
	case 9:
		if( bSigned ) iValue = bound( -256, (short)iValue, 255 );
		else iValue = boundmax( (word)iValue, 511 );
		break;
	case 10:
		if( bSigned ) iValue = bound( -512, (short)iValue, 511 );
		else iValue = boundmax( (word)iValue, 1023 );
		break;
	case 11:
		if( bSigned ) iValue = bound( -1024, (short)iValue, 1023 );
		else iValue = boundmax( (word)iValue, 2047 );
		break;
	case 12:
		if( bSigned ) iValue = bound( -2048, (short)iValue, 2047 );
		else iValue = boundmax( (word)iValue, 4095 );
		break;
	case 13:
		if( bSigned ) iValue = bound( -4096, (short)iValue, 4095 );
		else iValue = boundmax( (word)iValue, 8191 );
		break;
	case 14:
		if( bSigned ) iValue = bound( -8192, (short)iValue, 8191 );
		else iValue = boundmax( (word)iValue, 16383 );
		break;
	case 15:
		if( bSigned ) iValue = bound( -16384, (short)iValue, 16383 );
		else iValue = boundmax( (word)iValue, 32767 );
		break;
	case 16:
		if( bSigned ) iValue = bound( -32768, (short)iValue, 32767 );
		else iValue = boundmax( (word)iValue, 65535 );
		break;
	}

	return iValue; // clamped;
} 

/*
=====================
Delta_CompareField

compare fields by offsets
assume from and to is valid
TESTTEST: clamp all fields and multiply by specified value before comparing
=====================
*/
qboolean Delta_CompareField( delta_t *pField, void *from, void *to, float timebase )
{
	qboolean	bSigned = ( pField->flags & DT_SIGNED ) ? true : false;
	float	val_a, val_b;
	int	fromF, toF;

	ASSERT( pField );
	ASSERT( from );
	ASSERT( to );

	if( pField->bInactive )
		return true;

	fromF = toF = 0;

	if( pField->flags & DT_BYTE )
	{
		if( pField->flags & DT_SIGNED )
		{
			fromF = *(signed char *)((byte *)from + pField->offset );
			toF = *(signed char *)((byte *)to + pField->offset );
		}
		else
		{
			fromF = *(byte *)((byte *)from + pField->offset );
			toF = *(byte *)((byte *)to + pField->offset );
		}

		fromF = Delta_ClampIntegerField( fromF, bSigned, pField->bits );
		toF = Delta_ClampIntegerField( toF, bSigned, pField->bits );
		if( pField->multiplier != 1.0f )
		{
			fromF *= pField->multiplier;
			toF *= pField->multiplier;
		}
	}
	else if( pField->flags & DT_SHORT )
	{
		if( pField->flags & DT_SIGNED )
		{
			fromF = *(short *)((byte *)from + pField->offset );
			toF = *(short *)((byte *)to + pField->offset );
		}
		else
		{
			fromF = *(word *)((byte *)from + pField->offset );
			toF = *(word *)((byte *)to + pField->offset );
		}

		fromF = Delta_ClampIntegerField( fromF, bSigned, pField->bits );
		toF = Delta_ClampIntegerField( toF, bSigned, pField->bits );
		if( pField->multiplier != 1.0f )
		{
			fromF *= pField->multiplier;
			toF *= pField->multiplier;
		}
	}
	else if( pField->flags & DT_INTEGER )
	{
		if( pField->flags & DT_SIGNED )
		{
			fromF = *(int *)((byte *)from + pField->offset );
			toF = *(int *)((byte *)to + pField->offset );
		}
		else
		{
			fromF = *(uint *)((byte *)from + pField->offset );
			toF = *(uint *)((byte *)to + pField->offset );
		}

		fromF = Delta_ClampIntegerField( fromF, bSigned, pField->bits );
		toF = Delta_ClampIntegerField( toF, bSigned, pField->bits );
		if( pField->multiplier != 1.0f )
		{
			fromF *= pField->multiplier;
			toF *= pField->multiplier;
		}
	}
	else if( pField->flags & ( DT_FLOAT|DT_ANGLE ))
	{
		// don't convert floats to integers
		fromF = *((int *)((byte *)from + pField->offset ));
		toF = *((int *)((byte *)to + pField->offset ));
	}
	else if( pField->flags & DT_TIMEWINDOW_8 )
	{
		val_a = Q_rint((*(float *)((byte *)from + pField->offset )) * 100.0f );
		val_b = Q_rint((*(float *)((byte *)to + pField->offset )) * 100.0f );
		val_a -= Q_rint(timebase * 100.0f);
		val_b -= Q_rint(timebase * 100.0f);
		fromF = *((int *)&val_a);
		toF = *((int *)&val_b);
	}
	else if( pField->flags & DT_TIMEWINDOW_BIG )
	{
		val_a = (*(float *)((byte *)from + pField->offset ));
		val_b = (*(float *)((byte *)to + pField->offset ));
		if( pField->multiplier != 1.0f )
		{
			val_a *= pField->multiplier;
			val_b *= pField->multiplier;
			val_a = (timebase * pField->multiplier) - val_a;
			val_b = (timebase * pField->multiplier) - val_b;
		}
		else
		{
			val_a = timebase - val_a;
			val_b = timebase - val_b;
		}
		fromF = *((int *)&val_a);
		toF = *((int *)&val_b);
	}
	else if( pField->flags & DT_STRING )
	{
		// compare strings
		char	*s1 = (char *)((byte *)from + pField->offset );
		char	*s2 = (char *)((byte *)to + pField->offset );

		// 0 is equal, otherwise not equal
		toF = Q_strcmp( s1, s2 );
	}

	return ( fromF == toF ) ? true : false;
}

/*
=====================
Delta_WriteField

write fields by offsets
assume from and to is valid
=====================
*/
qboolean Delta_WriteField( sizebuf_t *msg, delta_t *pField, void *from, void *to, float timebase )
{
	qboolean		bSigned = ( pField->flags & DT_SIGNED ) ? true : false;
	float		flValue, flAngle, flTime;
	uint		iValue;
	const char	*pStr;

	if( Delta_CompareField( pField, from, to, timebase ))
	{
		BF_WriteOneBit( msg, 0 );	// unchanged
		return false;
	}

	BF_WriteOneBit( msg, 1 );	// changed

	if( pField->flags & DT_BYTE )
	{
		iValue = *(byte *)((byte *)to + pField->offset );
		iValue = Delta_ClampIntegerField( iValue, bSigned, pField->bits );
		if ( pField->multiplier != 1.0f ) iValue *= pField->multiplier;
		BF_WriteBitLong( msg, iValue, pField->bits, bSigned );
	}
	else if( pField->flags & DT_SHORT )
	{
		iValue = *(word *)((byte *)to + pField->offset );
		iValue = Delta_ClampIntegerField( iValue, bSigned, pField->bits );
		if ( pField->multiplier != 1.0f ) iValue *= pField->multiplier;
		BF_WriteBitLong( msg, iValue, pField->bits, bSigned );
	}
	else if( pField->flags & DT_INTEGER )
	{
		iValue = *(uint *)((byte *)to + pField->offset );
		iValue = Delta_ClampIntegerField( iValue, bSigned, pField->bits );
		if ( pField->multiplier != 1.0f ) iValue *= pField->multiplier;
		BF_WriteBitLong( msg, iValue, pField->bits, bSigned );
	}
	else if( pField->flags & DT_FLOAT )
	{
		#ifdef __arm__
		memcpy(&flValue,(byte *)to + pField->offset, sizeof(float) );
		#else
		flValue = *(float *)((byte *)to + pField->offset );
		#endif
		iValue = (int)(flValue * pField->multiplier);
		BF_WriteBitLong( msg, iValue, pField->bits, bSigned );
	}
	else if( pField->flags & DT_ANGLE )
	{
		#ifdef __arm__
		memcpy(&flAngle,(byte *)to + pField->offset, sizeof(float) );
		#else
		flAngle = *(float *)((byte *)to + pField->offset );
		#endif

		// NOTE: never applies multipliers to angle because
		// result may be wrong on client-side
		BF_WriteBitAngle( msg, flAngle, pField->bits );
	}
	else if( pField->flags & DT_TIMEWINDOW_8 )
	{
		#ifdef __arm__
		memcpy(&flValue,(byte *)to + pField->offset, sizeof(float) );
		#else
		flValue = *(float *)((byte *)to + pField->offset );
		#endif
		flTime = Q_rint( timebase * 100.0f ) - Q_rint(flValue * 100.0f);
		#if 1
		iValue = (uint)fabs( flTime );
		#else
		iValue = (uint)fabs( flTime );
		if (flTime<0.0f) {
			iValue |= 0x80000000;
		}
		#endif

		BF_WriteBitLong( msg, iValue, pField->bits, bSigned );
	}
	else if( pField->flags & DT_TIMEWINDOW_BIG )
	{
		#ifdef __arm__
		memcpy(&flValue,(byte *)to + pField->offset, sizeof(float) );
		#else
		flValue = *(float *)((byte *)to + pField->offset );
		#endif
		flTime = (timebase * pField->multiplier) - (flValue * pField->multiplier);
		#if 1
		iValue = (uint)fabs( flTime );
		#else
		iValue = (uint)fabs( flTime );
		if (flTime<0.0f) {
			iValue |= 0x80000000;
		}
		#endif

		BF_WriteBitLong( msg, iValue, pField->bits, bSigned );
	}
	else if( pField->flags & DT_STRING )
	{
		pStr = (char *)((byte *)to + pField->offset );
		BF_WriteString( msg, pStr );
	}
	return true;
}

/*
=====================
Delta_ReadField

read fields by offsets
assume 'from' and 'to' is valid
=====================
*/
qboolean Delta_ReadField( sizebuf_t *msg, delta_t *pField, void *from, void *to, float timebase )
{
	qboolean		bSigned = ( pField->flags & DT_SIGNED ) ? true : false;
	float		flValue, flAngle, flTime;
	qboolean		bChanged;
	uint		iValue;
	const char	*pStr;
	char		*pOut;
	
	bChanged = BF_ReadOneBit( msg );

	ASSERT( pField->multiplier != 0.0f );

	if( pField->flags & DT_BYTE )
	{
		if( bChanged )
		{
			iValue = BF_ReadBitLong( msg, pField->bits, bSigned );
			if ( pField->multiplier != 1.0f ) iValue /= pField->multiplier;
		}
		else
		{
			iValue = *(byte *)((byte *)from + pField->offset );
		}
		*(byte *)((byte *)to + pField->offset ) = iValue;
	}
	else if( pField->flags & DT_SHORT )
	{
		if( bChanged )
		{
			iValue = BF_ReadBitLong( msg, pField->bits, bSigned );
			if ( pField->multiplier != 1.0f ) iValue /= pField->multiplier;
		}
		else
		{
			iValue = *(word *)((byte *)from + pField->offset );
		}
		*(word *)((byte *)to + pField->offset ) = iValue;
	}
	else if( pField->flags & DT_INTEGER )
	{
		if( bChanged )
		{
			iValue = BF_ReadBitLong( msg, pField->bits, bSigned );
			if ( pField->multiplier != 1.0f ) iValue /= pField->multiplier;
		}
		else
		{
			iValue = *(uint *)((byte *)from + pField->offset );
		}
		*(uint *)((byte *)to + pField->offset ) = iValue;
	}
	else if( pField->flags & DT_FLOAT )
	{
		if( bChanged )
		{
			iValue = BF_ReadBitLong( msg, pField->bits, bSigned );
			flValue = (int)iValue * ( 1.0f / pField->multiplier );
			flValue = flValue * pField->post_multiplier;
		}
		else
		{
			#ifdef __arm__
			memcpy(&flValue, (byte *)from + pField->offset, sizeof(float) );
			#else
			flValue = *(float *)((byte *)from + pField->offset );
			#endif
		}
		#ifdef __arm__
		memcpy((byte *)to + pField->offset, &flValue, sizeof(float));
		#else
		*(float *)((byte *)to + pField->offset ) = flValue;
		#endif
	}
	else if( pField->flags & DT_ANGLE )
	{
		if( bChanged )
		{
			flAngle = BF_ReadBitAngle( msg, pField->bits );
		}
		else
		{
			#ifdef __arm__
			memcpy(&flAngle, (byte *)from + pField->offset, sizeof(float) );
			#else
			flAngle = *(float *)((byte *)from + pField->offset );
			#endif
		}
		#ifdef __arm__
		memcpy((byte *)to + pField->offset, &flAngle, sizeof(float));
		#else
		*(float *)((byte *)to + pField->offset ) = flAngle;
		#endif
	}
	else if( pField->flags & DT_TIMEWINDOW_8 )
	{
		if( bChanged )
		{
			iValue = BF_ReadBitLong( msg, pField->bits, bSigned );
			flValue = (float)((int)(iValue * 0.01f ));
			flTime = timebase + flValue;
		}
		else
		{
			#ifdef __arm__
			memcpy(&flTime, (byte *)from + pField->offset, sizeof(float) );
			#else
			flTime = *(float *)((byte *)from + pField->offset );
			#endif
		}
		#ifdef __arm__
		memcpy((byte *)to + pField->offset, &flTime, sizeof(float));
		#else
		*(float *)((byte *)to + pField->offset ) = flTime;
		#endif
	}
	else if( pField->flags & DT_TIMEWINDOW_BIG )
	{
		if( bChanged )
		{
			iValue = BF_ReadBitLong( msg, pField->bits, bSigned );
			flValue = (float)((int)iValue) * ( 1.0f / pField->multiplier );
			flTime = timebase + flValue;
		}
		else
		{
			#ifdef __arm__
			memcpy(&flTime, (byte *)from + pField->offset, sizeof(float) );
			#else
			flTime = *(float *)((byte *)from + pField->offset );
			#endif
		}
		#ifdef __arm__
		memcpy((byte *)to + pField->offset, &flTime, sizeof(float));
		#else
		*(float *)((byte *)to + pField->offset ) = flTime;
		#endif
	}
	else if( pField->flags & DT_STRING )
	{
		if( bChanged )
		{
			pStr = BF_ReadString( msg );
		}
		else
		{
			pStr = (char *)((byte *)from + pField->offset );
		}

		pOut = (char *)((byte *)to + pField->offset );
		Q_strncpy( pOut, pStr, pField->size );
	}
	return bChanged;
}

/*
=============================================================================

usercmd_t communication
  
=============================================================================
*/
/*
=====================
MSG_WriteDeltaUsercmd
=====================
*/
void MSG_WriteDeltaUsercmd( sizebuf_t *msg, usercmd_t *from, usercmd_t *to )
{
	delta_t		*pField;
	delta_info_t	*dt;
	int		i;

	dt = Delta_FindStruct( "usercmd_t" );
	if( !dt || !dt->bInitialized )
	{
		Host_Error( "MSG_WriteDeltaUsercmd: delta not initialized!\n" );
	}

	pField = dt->pFields;
	ASSERT( pField );

	// activate fields and call custom encode func
	Delta_CustomEncode( dt, from, to );

	// process fields
	for( i = 0; i < dt->numFields; i++, pField++ )
	{
		Delta_WriteField( msg, pField, from, to, 0.0f );
	}
}

/*
=====================
MSG_ReadDeltaUsercmd
=====================
*/
void MSG_ReadDeltaUsercmd( sizebuf_t *msg, usercmd_t *from, usercmd_t *to )
{
	delta_t		*pField;
	delta_info_t	*dt;
	int		i;

	dt = Delta_FindStruct( "usercmd_t" );
	if( !dt || !dt->bInitialized )
	{
		Host_Error( "MSG_ReadDeltaUsercmd: delta not initialized!\n" );
	}

	pField = dt->pFields;
	ASSERT( pField );

	*to = *from;

	// process fields
	for( i = 0; i < dt->numFields; i++, pField++ )
	{
		Delta_ReadField( msg, pField, from, to, 0.0f );
	}
}

/*
============================================================================

event_args_t communication

============================================================================
*/
/*
=====================
MSG_WriteDeltaEvent
=====================
*/
void MSG_WriteDeltaEvent( sizebuf_t *msg, event_args_t *from, event_args_t *to )
{
	delta_t		*pField;
	delta_info_t	*dt;
	int		i;

	dt = Delta_FindStruct( "event_t" );
	if( !dt || !dt->bInitialized )
	{
		Host_Error( "MSG_WriteDeltaEvent: delta not initialized!\n" );
	}

	pField = dt->pFields;
	ASSERT( pField );

	// activate fields and call custom encode func
	Delta_CustomEncode( dt, from, to );

	// process fields
	for( i = 0; i < dt->numFields; i++, pField++ )
	{
		Delta_WriteField( msg, pField, from, to, 0.0f );
	}
}

/*
=====================
MSG_ReadDeltaEvent
=====================
*/
void MSG_ReadDeltaEvent( sizebuf_t *msg, event_args_t *from, event_args_t *to )
{
	delta_t		*pField;
	delta_info_t	*dt;
	int		i;

	dt = Delta_FindStruct( "event_t" );
	if( !dt || !dt->bInitialized )
	{
		Host_Error( "MSG_ReadDeltaEvent: delta not initialized!\n" );
	}

	pField = dt->pFields;
	ASSERT( pField );

	*to = *from;

	// process fields
	for( i = 0; i < dt->numFields; i++, pField++ )
	{
		Delta_ReadField( msg, pField, from, to, 0.0f );
	}
}

/*
=============================================================================

movevars_t communication
  
=============================================================================
*/
qboolean MSG_WriteDeltaMovevars( sizebuf_t *msg, movevars_t *from, movevars_t *to )
{
	delta_t		*pField;
	delta_info_t	*dt;
	int		i, startBit;
	int		numChanges = 0;

	dt = Delta_FindStruct( "movevars_t" );
	if( !dt || !dt->bInitialized )
	{
		Host_Error( "MSG_WriteDeltaMovevars: delta not initialized!\n" );
	}

	pField = dt->pFields;
	ASSERT( pField );

	startBit = msg->iCurBit;

	// activate fields and call custom encode func
	Delta_CustomEncode( dt, from, to );

	BF_WriteByte( msg, svc_deltamovevars );

	// process fields
	for( i = 0; i < dt->numFields; i++, pField++ )
	{
		if( Delta_WriteField( msg, pField, from, to, 0.0f ))
			numChanges++;
	}

	// if we have no changes - kill the message
	if( !numChanges )
	{
		BF_SeekToBit( msg, startBit );
		return false;
	}
	return true;
}

void MSG_ReadDeltaMovevars( sizebuf_t *msg, movevars_t *from, movevars_t *to )
{
	delta_t		*pField;
	delta_info_t	*dt;
	int		i;

	dt = Delta_FindStruct( "movevars_t" );
	if( !dt || !dt->bInitialized )
	{
		Host_Error( "MSG_ReadDeltaMovevars: delta not initialized!\n" );
	}

	pField = dt->pFields;
	ASSERT( pField );

	*to = *from;

	// process fields
	for( i = 0; i < dt->numFields; i++, pField++ )
	{
		Delta_ReadField( msg, pField, from, to, 0.0f );
	}
}

/*
=============================================================================

clientdata_t communication

=============================================================================
*/
/*
==================
MSG_WriteClientData

Writes current client data only for local client
Other clients can grab the client state from entity_state_t
==================
*/
void MSG_WriteClientData( sizebuf_t *msg, clientdata_t *from, clientdata_t *to, float timebase )
{
	delta_t		*pField;
	delta_info_t	*dt;
	int		i;

	dt = Delta_FindStruct( "clientdata_t" );
	if( !dt || !dt->bInitialized )
	{
		Host_Error( "MSG_WriteClientData: delta not initialized!\n" );
	}

	pField = dt->pFields;
	ASSERT( pField );
	// activate fields and call custom encode func
	Delta_CustomEncode( dt, from, to );

	// process fields
	for( i = 0; i < dt->numFields; i++, pField++ )
	{
		 Delta_WriteField( msg, pField, from, to, timebase );
	}
}

/*
==================
MSG_ReadClientData

Read the clientdata
==================
*/
void MSG_ReadClientData( sizebuf_t *msg, clientdata_t *from, clientdata_t *to, float timebase )
{
	delta_t		*pField;
	delta_info_t	*dt;
	int		i;

	dt = Delta_FindStruct( "clientdata_t" );
	if( !dt || !dt->bInitialized )
	{
		Host_Error( "MSG_ReadClientData: delta not initialized!\n" );
	}

	pField = dt->pFields;
	ASSERT( pField );

	*to = *from;

	// process fields
	for( i = 0; i < dt->numFields; i++, pField++ )
	{
		Delta_ReadField( msg, pField, from, to, timebase );
	}
}

/*
=============================================================================

weapon_data_t communication

=============================================================================
*/
/*
==================
MSG_WriteWeaponData

Writes current client data only for local client
Other clients can grab the client state from entity_state_t
==================
*/
void MSG_WriteWeaponData( sizebuf_t *msg, weapon_data_t *from, weapon_data_t *to, float timebase, int index )
{
	delta_t		*pField;
	delta_info_t	*dt;
	int		i, startBit;
	int		numChanges = 0;

	dt = Delta_FindStruct( "weapon_data_t" );
	if( !dt || !dt->bInitialized )
	{
		Host_Error( "MSG_WriteWeaponData: delta not initialized!\n" );
	}

	pField = dt->pFields;
	ASSERT( pField );

	// activate fields and call custom encode func
	Delta_CustomEncode( dt, from, to );

	startBit = msg->iCurBit;

	BF_WriteOneBit( msg, 1 );
	BF_WriteUBitLong( msg, index, MAX_WEAPON_BITS );
               
	// process fields
	for( i = 0; i < dt->numFields; i++, pField++ )
	{
		if( Delta_WriteField( msg, pField, from, to, timebase ))
			numChanges++;
	}

	// if we have no changes - kill the message
	if( !numChanges ) BF_SeekToBit( msg, startBit );
}

/*
==================
MSG_ReadWeaponData

Read the clientdata
==================
*/
void MSG_ReadWeaponData( sizebuf_t *msg, weapon_data_t *from, weapon_data_t *to, float timebase )
{
	delta_t		*pField;
	delta_info_t	*dt;
	int		i;

	dt = Delta_FindStruct( "weapon_data_t" );
	if( !dt || !dt->bInitialized )
	{
		Host_Error( "MSG_ReadWeaponData: delta not initialized!\n" );
	}

	pField = dt->pFields;
	ASSERT( pField );

	*to = *from;

	// process fields
	for( i = 0; i < dt->numFields; i++, pField++ )
	{
		Delta_ReadField( msg, pField, from, to, timebase );
	}
}

/*
=============================================================================

entity_state_t communication

=============================================================================
*/
/*
==================
MSG_WriteDeltaEntity

Writes part of a packetentities message, including the entity number.
Can delta from either a baseline or a previous packet_entity
If to is NULL, a remove entity update will be sent
If force is not set, then nothing at all will be generated if the entity is
identical, under the assumption that the in-order delta code will catch it.
==================
*/
void MSG_WriteDeltaEntity( entity_state_t *from, entity_state_t *to, sizebuf_t *msg, qboolean force, qboolean player, float timebase ) 
{
	delta_info_t	*dt = NULL;
	delta_t		*pField;
	int		i, startBit;
	int		numChanges = 0;

	if( to == NULL )
	{
		int	fRemoveType;

		if( from == NULL ) return;

		// a NULL to is a delta remove message
		BF_WriteWord( msg, from->number );

		// fRemoveType:
		// 0 - keep alive, has delta-update
		// 1 - remove from delta message (but keep states)
		// 2 - completely remove from server
		if( force ) fRemoveType = 2;
		else fRemoveType = 1;

		BF_WriteUBitLong( msg, fRemoveType, 2 );
		return;
	}

	startBit = msg->iCurBit;

	if( to->number < 0 || to->number >= GI->max_edicts )
	{
		MsgDev( D_ERROR, "MSG_WriteDeltaEntity: Bad entity number: %i\n", to->number );
		return;
	}

	BF_WriteWord( msg, to->number );
	BF_WriteUBitLong( msg, 0, 2 ); // alive

	if( to->entityType != from->entityType )
	{
		BF_WriteOneBit( msg, 1 );
		BF_WriteUBitLong( msg, to->entityType, 2 );
	}
	else BF_WriteOneBit( msg, 0 ); 

	if( to->entityType == ENTITY_NORMAL )
	{
		if( player )
		{
			dt = Delta_FindStruct( "entity_state_player_t" );
		}
		else
		{
			dt = Delta_FindStruct( "entity_state_t" );
		}
	}
	else if( to->entityType == ENTITY_BEAM )
	{
		dt = Delta_FindStruct( "custom_entity_state_t" );
	}

	ASSERT( dt && dt->bInitialized );
		
	pField = dt->pFields;
	ASSERT( pField );

	// activate fields and call custom encode func
	Delta_CustomEncode( dt, from, to );

	// process fields
	for( i = 0; i < dt->numFields; i++, pField++ )
	{
		if( Delta_WriteField( msg, pField, from, to, timebase ))
			numChanges++;
	}

	// if we have no changes - kill the message
	if( !numChanges && !force ) BF_SeekToBit( msg, startBit );
}

/*
==================
MSG_ReadDeltaEntity

The entity number has already been read from the message, which
is how the from state is identified.
                             
If the delta removes the entity, entity_state_t->number will be set to MAX_EDICTS
Can go from either a baseline or a previous packet_entity
==================
*/
qboolean MSG_ReadDeltaEntity( sizebuf_t *msg, entity_state_t *from, entity_state_t *to, int number, qboolean player, float timebase )
{
	delta_info_t	*dt = NULL;
	delta_t		*pField;
	int		i, fRemoveType;
#ifndef XASH_DEDICATED
	if( number < 0 || number >= clgame.maxEntities )
	{
		// broken packet, try to skip it
		MsgDev( D_ERROR, "MSG_ReadDeltaEntity: bad delta entity number: %i\n", number );
		return false;
	}

	*to = *from;
	to->number = number;
	fRemoveType = BF_ReadUBitLong( msg, 2 );

	if( fRemoveType )
	{
		// check for a remove
		Q_memset( to, 0, sizeof( *to ));

		if( fRemoveType & 1 )
		{
			// removed from delta-message
			return false;
                    }

		if( fRemoveType & 2 )
		{	
			// entity was removed from server
			to->number = -1;
			return false;
		}

		MsgDev( D_ERROR, "MSG_ReadDeltaEntity: unknown update type %i\n", fRemoveType );
		return false;
	}

	if( BF_ReadOneBit( msg ))
		to->entityType = BF_ReadUBitLong( msg, 2 );


	if( to->entityType == ENTITY_BEAM )
	{
		dt = Delta_FindStruct( "custom_entity_state_t" );
	}
	else //  ENTITY_NORMAL or other (try predict type)
	{
		/* Omit connection drop on wrong data from server.
		 * I know that it is very dirty,
		 * but i don't know how to do it better.*/
		if( to->entityType != ENTITY_NORMAL )
			MsgDev( D_NOTE, "MSG_ReadDeltaEntity: broken delta: entityType = %d\n", to->entityType );
		if( player )
		{
			dt = Delta_FindStruct( "entity_state_player_t" );
		}
		else
		{
			dt = Delta_FindStruct( "entity_state_t" );
		}
	}

	if( !(dt && dt->bInitialized) ) // Broken  delta?
	{
		MsgDev( D_ERROR, "MSG_ReadDeltaEntity: broken delta\n");
		return true;
	}
	pField = dt->pFields;
	ASSERT( pField );

	// process fields
	for( i = 0; i < dt->numFields; i++, pField++ )
	{
		Delta_ReadField( msg, pField, from, to, timebase );
	}
#endif
	// message parsed
	return true;
}

/*
=============================================================================

	game.dll interface
  
=============================================================================
*/
void GAME_EXPORT Delta_AddEncoder( char *name, pfnDeltaEncode encodeFunc )
{
	delta_info_t	*dt;

	dt = Delta_FindStructByEncoder( name );

	if( !dt || !dt->bInitialized )
	{
		MsgDev( D_ERROR, "Delta_AddEncoder: couldn't find delta with specified custom encode %s\n", name );
		return;
	}

	if( dt->customEncode == CUSTOM_NONE )
	{
		MsgDev( D_ERROR, "Delta_AddEncoder: %s not supposed for custom encoding\n", dt->pName );
		return;
	}

	// register new encode func
	dt->userCallback = encodeFunc;	
}

int Delta_FindField( delta_t *pFields, const char *fieldname )
{
	delta_info_t	*dt;
	delta_t		*pField;
	int		i;

	dt = Delta_FindStructByDelta( pFields );
	if( dt == NULL || !fieldname || !fieldname[0] )
		return -1;

	for( i = 0, pField = dt->pFields; i < dt->numFields; i++, pField++ )
	{
		if( !Q_strcmp( pField->name, fieldname ))
			return i;
	}
	return -1;
}

void GAME_EXPORT Delta_SetField( delta_t *pFields, const char *fieldname )
{
	delta_info_t	*dt;
	delta_t		*pField;
	int		i;

	dt = Delta_FindStructByDelta( pFields );
	if( dt == NULL || !fieldname || !fieldname[0] )
		return;

	for( i = 0, pField = dt->pFields; i < dt->numFields; i++, pField++ )
	{
		if( !Q_strcmp( pField->name, fieldname ))
		{
			pField->bInactive = false;
			return;
		}
	}
}

void GAME_EXPORT Delta_UnsetField( delta_t *pFields, const char *fieldname )
{
	delta_info_t	*dt;
	delta_t		*pField;
	int		i;

	dt = Delta_FindStructByDelta( pFields );
	if( dt == NULL || !fieldname || !fieldname[0] )
		return;

	for( i = 0, pField = dt->pFields; i < dt->numFields; i++, pField++ )
	{
		if( !Q_strcmp( pField->name, fieldname ))
		{
			pField->bInactive = true;
			return;
		}
	}
}

void GAME_EXPORT Delta_SetFieldByIndex( delta_t *pFields, int fieldNumber )
{
	delta_info_t	*dt;

	dt = Delta_FindStructByDelta( pFields );
	if( dt == NULL || fieldNumber < 0 || fieldNumber >= dt->numFields )
		return;

	dt->pFields[fieldNumber].bInactive = false;
}

void GAME_EXPORT Delta_UnsetFieldByIndex( delta_t *pFields, int fieldNumber )
{
	delta_info_t	*dt;

	dt = Delta_FindStructByDelta( pFields );
	if( dt == NULL || fieldNumber < 0 || fieldNumber >= dt->numFields )
		return;

	dt->pFields[fieldNumber].bInactive = true;
}
