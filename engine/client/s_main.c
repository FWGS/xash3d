/*
s_main.c - sound engine
Copyright (C) 2009 Uncle Mike

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
#include "sound.h"
#include "client.h"
#include "con_nprint.h"
#include "ref_params.h"
#include "pm_local.h"
#ifdef XASH_SDL
#include <SDL.h>
#endif
#define MAX_DUPLICATED_CHANNELS	4		// threshold for identical static channels (probably error)
#define SND_CLIP_DISTANCE		(float)(GI->soundclip_dist)

dma_t		dma;
byte		*sndpool;
static soundfade_t	soundfade;
channel_t   	channels[MAX_CHANNELS];
sound_t		ambient_sfx[NUM_AMBIENTS];
qboolean		snd_ambient = false;
listener_t	s_listener;
int		total_channels;
int		soundtime;	// sample PAIRS
int   		paintedtime; 	// sample PAIRS
static int	trace_count = 0;
static int	last_trace_chan = 0;

convar_t		*s_volume;
convar_t		*s_musicvolume;
convar_t		*s_show;
convar_t		*s_mixahead;
convar_t		*s_primary;
convar_t		*s_lerping;
convar_t		*s_ambient_level;
convar_t		*s_ambient_fade;
convar_t		*s_combine_sounds;
convar_t		*snd_foliage_db_loss; 
convar_t		*snd_gain;
convar_t		*snd_gain_max;
convar_t		*snd_gain_min;
convar_t		*snd_mute_losefocus;
convar_t		*s_refdist;
convar_t		*s_refdb;
convar_t		*s_cull;		// cull sounds by geometry
convar_t		*s_test;		// cvar for testing new effects
convar_t		*s_phs;
convar_t		*s_reverse_channels;

/*
=============================================================================

		SOUND COMMON UTILITES

=============================================================================
*/
// dB = 20 log (amplitude/32768)		0 to -90.3dB
// amplitude = 32768 * 10 ^ (dB/20)		0 to +/- 32768
// gain = amplitude/32768			0 to 1.0
_inline float Gain_To_dB( float gain ) { return 20 * log( gain ); }
_inline float dB_To_Gain ( float dB ) { return pow( 10, dB / 20.0f ); }
_inline float Gain_To_Amplitude( float gain ) { return gain * 32768; }
_inline float Amplitude_To_Gain( float amplitude ) { return amplitude / 32768; }

// convert sound db level to approximate sound source radius,
// used only for determining how much of sound is obscured by world
_inline float dB_To_Radius( float db )
{
	return (SND_RADIUS_MIN + (SND_RADIUS_MAX - SND_RADIUS_MIN) * (db - SND_DB_MIN) / (SND_DB_MAX - SND_DB_MIN));
}

/*
=============================================================================

		SOUNDS PROCESSING

=============================================================================
*/
/*
=================
S_GetMasterVolume
=================
*/
float S_GetMasterVolume( void )
{
	float	scale = 1.0f;

	if( !s_listener.inmenu && soundfade.percent != 0 )
	{
		scale = bound( 0.0f, soundfade.percent / 100.0f, 1.0f );
		scale = 1.0f - scale;
	}
	return s_volume->value * scale;
}

/*
=================
S_FadeClientVolume
=================
*/
void S_FadeClientVolume( float fadePercent, float fadeOutSeconds, float holdTime, float fadeInSeconds )
{
	soundfade.starttime	= cl.mtime[0];
	soundfade.initial_percent = fadePercent;       
	soundfade.fadeouttime = fadeOutSeconds;    
	soundfade.holdtime = holdTime;   
	soundfade.fadeintime = fadeInSeconds;
}

/*
=================
S_IsClient
=================
*/
qboolean S_IsClient( int entnum )
{
	return ( entnum == s_listener.entnum );
}


// free channel so that it may be allocated by the
// next request to play a sound.  If sound is a 
// word in a sentence, release the sentence.
// Works for static, dynamic, sentence and stream sounds
/*
=================
S_FreeChannel
=================
*/
void S_FreeChannel( channel_t *ch )
{
	ch->sfx = NULL;
	ch->name[0] = '\0';
	ch->use_loop = false;
	ch->isSentence = false;

	// clear mixer
	Q_memset( &ch->pMixer, 0, sizeof( ch->pMixer ));

	SND_CloseMouth( ch );
}

/*
=================
S_UpdateSoundFade
=================
*/
void S_UpdateSoundFade( void )
{
	float	f, totaltime, elapsed;

	// determine current fade value.
	// assume no fading remains
	soundfade.percent = 0;  

	totaltime = soundfade.fadeouttime + soundfade.fadeintime + soundfade.holdtime;

	elapsed = cl.mtime[0] - soundfade.starttime;

	// clock wrapped or reset (BUG) or we've gone far enough
	if( elapsed < 0.0f || elapsed >= totaltime || totaltime <= 0.0f )
		return;

	// We are in the fade time, so determine amount of fade.
	if( soundfade.fadeouttime > 0.0f && ( elapsed < soundfade.fadeouttime ))
	{
		// ramp up
		f = elapsed / soundfade.fadeouttime;
	}
	else if( elapsed <= ( soundfade.fadeouttime + soundfade.holdtime ))	// Inside the hold time
	{
		// stay
		f = 1.0f;
	}
	else
	{
		// ramp down
		f = ( elapsed - ( soundfade.fadeouttime + soundfade.holdtime ) ) / soundfade.fadeintime;
		f = 1.0f - f; // backward interpolated...
	}

	// spline it.
	f = S_SimpleSpline( f );
	f = bound( 0.0f, f, 1.0f );

	soundfade.percent = soundfade.initial_percent * f;
}

/*
=================
SND_ChannelOkToTrace

All new sounds must traceline once,
but cap the max number of tracelines performed per frame
for longer or looping sounds to SND_TRACE_UPDATE_MAX.
=================
*/
qboolean SND_ChannelOkToTrace( channel_t *ch )
{
	int 	i, j;

	// always trace first time sound is spatialized
	if( ch->bfirstpass ) return true;

	// if already traced max channels, return
	if( trace_count >= SND_TRACE_UPDATE_MAX )
		return false;

	// search through all channels starting at g_snd_last_trace_chan index
	j = last_trace_chan;

 	for( i = 0; i < total_channels; i++ )
	{
		if( &( channels[j] ) == ch )
		{
			ch->bTraced = true;
			trace_count++;
			return true;
		}

		// wrap channel index
		if( ++j >= total_channels )
			j = 0;
	}
	
	// why didn't we find this channel?
	return false;			
}

/*
=================
SND_ChannelTraceReset

reset counters for traceline limiting per audio update
=================
*/
void SND_ChannelTraceReset( void )
{
	int	i;

	// reset search point - make sure we start counting from a new spot 
	// in channel list each time
	last_trace_chan += SND_TRACE_UPDATE_MAX;
	
	// wrap at total_channels
	if( last_trace_chan >= total_channels )
		last_trace_chan = last_trace_chan - total_channels;

	// reset traceline counter
	trace_count = 0;

	// reset channel traceline flag
	for( i = 0; i < total_channels; i++ )
		channels[i].bTraced = false; 
}

/*
=================
SND_PickDynamicChannel

Select a channel from the dynamic channel allocation area.  For the given entity, 
override any other sound playing on the same channel (see code comments below for
exceptions).
=================
*/
channel_t *SND_PickDynamicChannel( int entnum, int channel, sfx_t *sfx )
{
	int	ch_idx;
	int	first_to_die;
	int	life_left;
	int	timeleft;

	// check for replacement sound, or find the best one to replace
	first_to_die = -1;
	life_left = 0x7fffffff;

	for( ch_idx = NUM_AMBIENTS; ch_idx < MAX_DYNAMIC_CHANNELS; ch_idx++ )
	{
		channel_t	*ch = &channels[ch_idx];
		
		// Never override a streaming sound that is currently playing or
		// voice over IP data that is playing or any sound on CHAN_VOICE( acting )
		if( ch->sfx && ( ch->entchannel == CHAN_STREAM ))
			continue;

		if( channel != CHAN_AUTO && ch->entnum == entnum && ( ch->entchannel == channel || channel == -1 ))
		{
			// always override sound from same entity
			first_to_die = ch_idx;
			break;
		}

		// don't let monster sounds override player sounds
		if( ch->sfx && S_IsClient( ch->entnum ) && !S_IsClient( entnum ))
			continue;

		// try to pick the sound with the least amount of data left to play
		timeleft = 0;
		if( ch->sfx )
		{
			timeleft = 1; // ch->end - paintedtime
		}

		if( timeleft < life_left )
		{
			life_left = timeleft;
			first_to_die = ch_idx;
		}
	}

	if( first_to_die == -1 )
		return NULL;

	if( channels[first_to_die].sfx )
	{
		// don't restart looping sounds for the same entity
		wavdata_t	*sc = channels[first_to_die].sfx->cache;

		if( sc && sc->loopStart != -1 )
		{
			channel_t	*ch = &channels[first_to_die];

			if( ch->entnum == entnum && ch->entchannel == channel && ch->sfx == sfx )
			{
				// same looping sound, same ent, same channel, don't restart the sound
				return NULL;
			}
		}

		// be sure and release previous channel if sentence.
		S_FreeChannel( &( channels[first_to_die] ));
	}

	return &channels[first_to_die];
}       

/*
=====================
SND_PickStaticChannel

Pick an empty channel from the static sound area, or allocate a new
channel.  Only fails if we're at max_channels (128!!!) or if 
we're trying to allocate a channel for a stream sound that is 
already playing.
=====================
*/
channel_t *SND_PickStaticChannel( int entnum, sfx_t *sfx, const vec3_t pos )
{
	channel_t	*ch = NULL;
	int	i;

#if 1
	int dupe = 0;

	// TODO: remove this code when predicting is will be done
	// check for duplicate sounds
	for( i = 0; i < total_channels; i++ )
	{
		if( channels[i].sfx == sfx && VectorCompare( channels[i].origin, pos ))
			dupe++;
	}

	// check for duplicated static channels (same origin and same sfx)
	if( dupe > MAX_DUPLICATED_CHANNELS )
		return NULL;
#endif
	// check for replacement sound, or find the best one to replace
 	for( i = MAX_DYNAMIC_CHANNELS; i < total_channels; i++ )
 	{
		if( channels[i].sfx == NULL )
			break;
	}

	if( i < total_channels ) 
	{
		// reuse an empty static sound channel
		ch = &channels[i];
	}
	else
	{
		// no empty slots, alloc a new static sound channel
		if( total_channels == MAX_CHANNELS )
		{
			MsgDev( D_ERROR, "S_PickChannel: no free channels\n" );
			return NULL;
		}
		// get a channel for the static sound
		ch = &channels[total_channels];
		total_channels++;
	}
	return ch;
}

/*
=================
S_AlterChannel

search through all channels for a channel that matches this
soundsource, entchannel and sfx, and perform alteration on channel
as indicated by 'flags' parameter. If shut down request and
sfx contains a sentence name, shut off the sentence.
returns TRUE if sound was altered,
returns FALSE if sound was not found (sound is not playing)
=================
*/
int S_AlterChannel( int entnum, int channel, sfx_t *sfx, int vol, int pitch, int flags )
{
	channel_t	*ch;
	int	i;	

	if( S_TestSoundChar( sfx->name, '!' ))
	{
		// This is a sentence name.
		// For sentences: assume that the entity is only playing one sentence
		// at a time, so we can just shut off
		// any channel that has ch->isSentence >= 0 and matches the entnum.

		for( i = NUM_AMBIENTS, ch = channels + NUM_AMBIENTS; i < total_channels; i++, ch++ )
		{
			if( ch->entnum == entnum && ch->entchannel == channel && ch->sfx && ch->isSentence )
			{
				if( flags & SND_CHANGE_PITCH )
					ch->basePitch = pitch;
				
				if( flags & SND_CHANGE_VOL )
					ch->master_vol = vol;
				
				if( flags & SND_STOP )
					S_FreeChannel( ch );

				return true;
			}
		}
		// channel not found
		return false;

	}

	// regular sound or streaming sound
	for( i = NUM_AMBIENTS, ch = channels + NUM_AMBIENTS; i < total_channels; i++, ch++ )
	{
		if( ch->entnum == entnum && ch->entchannel == channel && ch->sfx == sfx )
		{
			if( flags & SND_CHANGE_PITCH )
				ch->basePitch = pitch;
				
			if( flags & SND_CHANGE_VOL )
				ch->master_vol = vol;

			if( flags & SND_STOP )
				S_FreeChannel( ch );

			return true;
		}
	}
	return false;
}

/*
=================
SND_FadeToNewGain

always ramp channel gain changes over time
returns ramped gain, given new target gain
=================
*/
float SND_FadeToNewGain( channel_t *ch, float gain_new )
{
	float	speed, frametime;

	if( gain_new == -1.0 )
	{
		// if -1 passed in, just keep fading to existing target
		gain_new = ch->ob_gain_target;
	}

	// if first time updating, store new gain into gain & target, return
	// if gain_new is close to existing gain, store new gain into gain & target, return
	if( ch->bfirstpass || ( fabs( gain_new - ch->ob_gain ) < 0.01f ))
	{
		ch->ob_gain = gain_new;
		ch->ob_gain_target = gain_new;
		ch->ob_gain_inc = 0.0f;
		return gain_new;
	}

	// set up new increment to new target
	frametime = s_listener.frametime;
	speed = ( frametime / SND_GAIN_FADE_TIME ) * ( gain_new - ch->ob_gain );

	ch->ob_gain_inc = fabs( speed );

	// ch->ob_gain_inc = fabs( gain_new - ch->ob_gain ) / 10.0f;
	ch->ob_gain_target = gain_new;

	// if not hit target, keep approaching
	if( fabs( ch->ob_gain - ch->ob_gain_target ) > 0.01f )
	{
		ch->ob_gain = ApproachVal( ch->ob_gain_target, ch->ob_gain, ch->ob_gain_inc );
	}
	else
	{
		// close enough, set gain = target
		ch->ob_gain = ch->ob_gain_target;
	}

	return ch->ob_gain;
}

/*
=================
SND_GetGainObscured

drop gain on channel if sound emitter obscured by
world, unbroken windows, closed doors, large solid entities etc.
=================
*/
float SND_GetGainObscured( channel_t *ch, qboolean fplayersound, qboolean flooping )
{
	float	gain = 1.0f;
	vec3_t	endpoint;
	int	count = 1;
	pmtrace_t	tr;

	if( fplayersound ) return gain; // unchanged

	// during signon just apply regular state machine since world hasn't been
	// created or settled yet...
	if( !CL_Active( ))
	{
		gain = SND_FadeToNewGain( ch, -1.0f );
		return gain;
	}

	// don't do gain obscuring more than once on short one-shot sounds
	if( !ch->bfirstpass && !ch->isSentence && !flooping && ( ch->entchannel != CHAN_STREAM ))
	{
		gain = SND_FadeToNewGain( ch, -1.0f );
		return gain;
	}

	// if long or looping sound, process N channels per frame - set 'processed' flag, clear by
	// cycling through all channels - this maintains a cap on traces per frame
	if( !SND_ChannelOkToTrace( ch ))
	{
		// just keep updating fade to existing target gain - no new trace checking
		gain = SND_FadeToNewGain( ch, -1.0 );
		return gain;
	}

	// set up traceline from player eyes to sound emitting entity origin
	VectorCopy( ch->origin, endpoint );

	tr = CL_TraceLine( s_listener.origin, endpoint, PM_STUDIO_IGNORE );

	if(( tr.fraction < 1.0f || tr.allsolid || tr.startsolid ) && tr.fraction < 0.99f )
	{
		// can't see center of sound source:
		// build extents based on dB sndlvl of source,
		// test to see how many extents are visible,
		// drop gain by g_snd_obscured_loss_db per extent hidden
		vec3_t	endpoints[4];
		int	i, sndlvl = DIST_MULT_TO_SNDLVL( ch->dist_mult );
		vec3_t	vecl, vecr, vecl2, vecr2;
		vec3_t	vsrc_forward;
		vec3_t	vsrc_right;
		vec3_t	vsrc_up;
		float	radius;

		// get radius
		if( ch->radius > 0 ) radius = ch->radius;
		else radius = dB_To_Radius( sndlvl ); // approximate radius from soundlevel
		
		// set up extent endpoints - on upward or downward diagonals, facing player
		for( i = 0; i < 4; i++ ) VectorCopy( endpoint, endpoints[i] );

		// vsrc_forward is normalized vector from sound source to listener
		VectorSubtract( s_listener.origin, endpoint, vsrc_forward );
		VectorNormalize( vsrc_forward );
		VectorVectors( vsrc_forward, vsrc_right, vsrc_up );

		VectorAdd( vsrc_up, vsrc_right, vecl );
		
		// if src above listener, force 'up' vector to point down - create diagonals up & down
		if( endpoint[2] > s_listener.origin[2] + ( 10 * 12 ))
			vsrc_up[2] = -vsrc_up[2];

		VectorSubtract( vsrc_up, vsrc_right, vecr );
		VectorNormalize( vecl );
		VectorNormalize( vecr );

		// get diagonal vectors from sound source 
		VectorScale( vecl, radius, vecl2 );
		VectorScale( vecr, radius, vecr2 );
		VectorScale( vecl, (radius / 2.0f), vecl );
		VectorScale( vecr, (radius / 2.0f), vecr );

		// endpoints from diagonal vectors
		VectorAdd( endpoints[0], vecl, endpoints[0] );
		VectorAdd( endpoints[1], vecr, endpoints[1] );
		VectorAdd( endpoints[2], vecl2, endpoints[2] );
		VectorAdd( endpoints[3], vecr2, endpoints[3] );

		// drop gain for each point on radius diagonal that is obscured
		for( count = 0, i = 0; i < 4; i++ )
		{
			// UNDONE: some endpoints are in walls - in this case, trace from the wall hit location
			tr = CL_TraceLine( s_listener.origin, endpoints[i], PM_STUDIO_IGNORE );

			if(( tr.fraction < 1.0f || tr.allsolid || tr.startsolid ) && tr.fraction < 0.99f && !tr.startsolid )
			{
				// skip first obscured point: at least 2 points + center should be obscured to hear db loss
				if( ++count > 1 ) gain = gain * dB_To_Gain( SND_OBSCURED_LOSS_DB );
			}
		}
	}

	// crossfade to new gain
	gain = SND_FadeToNewGain( ch, gain );

	return gain;
}

/*
=================
SND_GetGain

The complete gain calculation, with SNDLVL given in dB is:
GAIN = 1/dist * snd_refdist * 10 ^ (( SNDLVL - snd_refdb - (dist * snd_foliage_db_loss / 1200)) / 20 )
for gain > SND_GAIN_THRESH, start curve smoothing with
GAIN = 1 - 1 / (Y * GAIN ^ SND_GAIN_POWER)
where Y = -1 / ( (SND_GAIN_THRESH ^ SND_GAIN_POWER) * ( SND_GAIN_THRESH - 1 ))
gain curve construction
=================
*/
float SND_GetGain( channel_t *ch, qboolean fplayersound, qboolean flooping, float dist )
{
	float	gain = snd_gain->value;

	if( ch->dist_mult )
	{
		// test additional attenuation
		// at 30c, 14.7psi, 60% humidity, 1000Hz == 0.22dB / 100ft.
		// dense foliage is roughly 2dB / 100ft
		float additional_dB_loss = snd_foliage_db_loss->value * (dist / 1200);
		float additional_dist_mult = pow( 10, additional_dB_loss / 20 );
		float relative_dist = dist * ch->dist_mult * additional_dist_mult;

		// hard code clamp gain to 10x normal (assumes volume and external clipping)
		if( relative_dist > 0.1f )
			gain *= ( 1.0f / relative_dist );
		else gain *= 10.0f;

		// if gain passess threshold, compress gain curve such that gain smoothly approaches 1.0
		if( gain > SND_GAIN_COMP_THRESH )
		{
			float	snd_gain_comp_power = SND_GAIN_COMP_EXP_MAX;
			int	sndlvl = DIST_MULT_TO_SNDLVL( ch->dist_mult );
			float	Y;
			
			// decrease compression curve fit for higher sndlvl values
			if( sndlvl > SND_DB_MED )
			{
				// snd_gain_power varies from max to min as sndlvl varies from 90 to 140
				snd_gain_comp_power = RemapVal((float)sndlvl, SND_DB_MED, SND_DB_MAX, SND_GAIN_COMP_EXP_MAX, SND_GAIN_COMP_EXP_MIN );
			}

			// calculate crossover point
			Y = -1.0f / ( pow( SND_GAIN_COMP_THRESH, snd_gain_comp_power ) * ( SND_GAIN_COMP_THRESH - 1 ));
			
			// calculate compressed gain
			gain = 1.0f - 1.0f / (Y * pow( gain, snd_gain_comp_power ));
			gain = gain * snd_gain_max->value;
		}

		if( gain < snd_gain_min->value )
		{
			// sounds less than snd_gain_min fall off to 0 in distance it took them to fall to snd_gain_min
			gain = snd_gain_min->value * ( 2.0f - relative_dist * snd_gain_min->value );
			if( gain <= 0.0f ) gain = 0.001f; // don't propagate 0 gain
		}
	}

	if( fplayersound )
	{
		// player weapon sounds get extra gain - this compensates
		// for npc distance effect weapons which mix louder as L+R into L, R
		// Hack.
		if( ch->entchannel == CHAN_WEAPON )
			gain = gain * dB_To_Gain( SND_GAIN_PLAYER_WEAPON_DB );
	}

	// modify gain if sound source not visible to player
	gain = gain * SND_GetGainObscured( ch, fplayersound, flooping );

	return gain; 
}

qboolean SND_CheckPHS( channel_t *ch )
{
	mleaf_t	*leaf;
	int	leafnum;
	byte	*mask = NULL;

	// cull sounds by PHS
	if( !s_phs->integer )
		return true;

	leaf = Mod_PointInLeaf( ch->origin, cl.worldmodel->nodes );
	mask = Mod_LeafPHS( leaf, cl.worldmodel );

	if( mask )
	{
		leafnum = Mod_PointLeafnum( s_listener.origin ) - 1;

		if( leafnum != -1 && (!(mask[leafnum>>3] & (1U << ( leafnum & 7 )))))
			return false;
	}
	return true;
}

/*
=================
S_SpatializeChannel
=================
*/
void S_SpatializeChannel( int *left_vol, int *right_vol, int master_vol, float gain, float dot, float dist )
{
	float	lscale, rscale, scale;

	rscale = 1.0f + dot;
	lscale = 1.0f - dot;

	// add in distance effect
	if( s_cull->integer ) scale = gain * rscale / 2;
	else scale = ( 1.0f - dist ) * rscale;
	*right_vol = (int)( master_vol * scale );

	if( s_cull->integer ) scale = gain * lscale / 2;
	else scale = ( 1.0f - dist ) * lscale;
	*left_vol = (int)( master_vol * scale );

	*right_vol = bound( 0, *right_vol, 255 );
	*left_vol = bound( 0, *left_vol, 255 );
}

/*
=================
SND_Spatialize
=================
*/
void SND_Spatialize( channel_t *ch )
{
	vec3_t	source_vec;
	float	dist, dot, gain = 1.0f;
	qboolean	fplayersound = false;
	qboolean	looping = false;
	wavdata_t	*pSource;

	// anything coming from the view entity will allways be full volume

	if( S_IsClient( ch->entnum ))
	{
		if( !s_cull->integer )
		{
			ch->leftvol = ch->rightvol = ch->master_vol;
			VOX_SetChanVol( ch );
			return;
		}
		// sounds coming from listener actually come from a short distance directly in front of listener
		fplayersound = true;
	}

	if( s_cull->integer )
	{
		pSource = ch->sfx->cache;

		if( ch->use_loop && pSource && pSource->loopStart != -1 )
			looping = true;
	}


	if( !ch->staticsound )
	{
		if( !CL_GetEntitySpatialization( ch->entnum, ch->origin, &ch->radius ) || !SND_CheckPHS( ch ))
		{
			// origin is null and entity not exist on client
			ch->leftvol = ch->rightvol = 0;
			ch->bfirstpass = false;
			return;
		}
	}

	// source_vec is vector from listener to sound source
	// player sounds come from 1' in front of player
	if( fplayersound ) VectorScale( s_listener.forward, 12.0f, source_vec );
	else VectorSubtract( ch->origin, s_listener.origin, source_vec );

	// normalize source_vec and get distance from listener to source
	dist = VectorNormalizeLength( source_vec );
	dot = DotProduct( s_listener.right, source_vec );

	// for sounds with a radius, spatialize left/right evenly within the radius

	// Commented because this code cause too LOUD sounds in multiplayers.
	// when blend is equal 0
	// TODO: Test sound behaviour without radius code, maybe it must be reviewed or deleted

	/*if( ch->radius > 0 && dist < ch->radius )
	{
		float	interval = ch->radius * 0.5f;
		float	blend = dist - interval;

		if( blend < 0 )
			blend = 0;

		blend /= interval;	

		// blend is 0.0 - 1.0, from 50% radius -> 100% radius
		// at radius * 0.5, dot is 0 (ie: sound centered left/right)
		// at radius dot == dot
		dot *= blend;
	}*/

	if( s_cull->integer )
	{
		// calculate gain based on distance, atmospheric attenuation, interposed objects
		// perform compression as gain approaches 1.0
		gain = SND_GetGain( ch, fplayersound, looping, dist );
	}

	// don't pan sounds with no attenuation
	if( ch->dist_mult <= 0.0f ) dot = 0.0f;

	// fill out channel volumes for single location
	S_SpatializeChannel( &ch->leftvol, &ch->rightvol, ch->master_vol, gain, dot, dist * ch->dist_mult );

	// if playing a word, set volume
	VOX_SetChanVol( ch );

	// end of first time spatializing sound
	if( CL_Active( )) ch->bfirstpass = false;
}

/*
====================
S_StartSound

Start a sound effect for the given entity on the given channel (ie; voice, weapon etc).  
Try to grab a channel out of the 8 dynamic spots available.
Currently used for looping sounds, streaming sounds, sentences, and regular entity sounds.
NOTE: volume is 0.0 - 1.0 and attenuation is 0.0 - 1.0 when passed in.
Pitch changes playback pitch of wave by % above or below 100.  Ignored if pitch == 100

NOTE: it's not a good idea to play looping sounds through StartDynamicSound, because
if the looping sound starts out of range, or is bumped from the buffer by another sound
it will never be restarted.  Use StartStaticSound (pass CHAN_STATIC to EMIT_SOUND or
SV_StartSound.
====================
*/
void S_StartSound( const vec3_t pos, int ent, int chan, sound_t handle, float fvol, float attn, int pitch, int flags )
{
	wavdata_t	*pSource;
	sfx_t	*sfx = NULL;
	channel_t	*target_chan, *check;
	int	vol, ch_idx;

	if( !dma.initialized ) return;
	sfx = S_GetSfxByHandle( handle );
	if( !sfx ) return;

	vol = bound( 0, fvol * 255, 255 );
	if( pitch <= 1 ) pitch = PITCH_NORM; // Invasion issues

	if( flags & ( SND_STOP|SND_CHANGE_VOL|SND_CHANGE_PITCH ))
	{
		if( S_AlterChannel( ent, chan, sfx, vol, pitch, flags ))
			return;

		if( flags & SND_STOP ) return;
		// fall through - if we're not trying to stop the sound, 
		// and we didn't find it (it's not playing), go ahead and start it up
	}

	if( pitch == 0 )
	{
		MsgDev( D_WARN, "S_StartSound: ( %s ) ignored, called with pitch 0\n", sfx->name );
		return;
	}

	if( !pos ) pos = cl.refdef.vieworg;

	// pick a channel to play on
	if( chan == CHAN_STATIC ) target_chan = SND_PickStaticChannel( ent, sfx, pos );
	else target_chan = SND_PickDynamicChannel( ent, chan, sfx );

	if( !target_chan )
	{
		MsgDev( D_NOTE, "^1Error: ^7dropped sound \"sound/%s\"\n", sfx->name );
		return;
	}

	// spatialize
	Q_memset( target_chan, 0, sizeof( *target_chan ));

	VectorCopy( pos, target_chan->origin );
	target_chan->staticsound = ( ent == 0 ) ? true : false;
	target_chan->use_loop = (flags & SND_STOP_LOOPING) ? false : true;
	target_chan->localsound = (flags & SND_LOCALSOUND) ? true : false;
	target_chan->dist_mult = (attn / SND_CLIP_DISTANCE);
	target_chan->master_vol = vol;
	target_chan->entnum = ent;
	target_chan->entchannel = chan;
	target_chan->basePitch = pitch;
	target_chan->isSentence = false;
	target_chan->radius = 0.0f;
	target_chan->sfx = sfx;

	// initialize gain due to obscured sound source
	target_chan->bfirstpass = true;
	target_chan->ob_gain = 0.0f;
	target_chan->ob_gain_inc = 0.0f;
	target_chan->ob_gain_target = 0.0f;
	target_chan->bTraced = false;

	pSource = NULL;

	if( S_TestSoundChar( sfx->name, '!' ))
	{
		// this is a sentence
		// link all words and load the first word
		// NOTE: sentence names stored in the cache lookup are
		// prepended with a '!'.  Sentence names stored in the
		// sentence file do not have a leading '!'. 
		VOX_LoadSound( target_chan, S_SkipSoundChar( sfx->name ));
		Q_strncpy( target_chan->name, sfx->name, sizeof( target_chan->name ));
		sfx = target_chan->sfx;
		pSource = sfx->cache;
	}
	else
	{
		// regular or streamed sound fx
		pSource = S_LoadSound( sfx );
		target_chan->name[0] = '\0';
	}

	if( !pSource )
	{
		S_FreeChannel( target_chan );
		return;
	}

	SND_Spatialize( target_chan );

	// If a client can't hear a sound when they FIRST receive the StartSound message,
	// the client will never be able to hear that sound. This is so that out of 
	// range sounds don't fill the playback buffer. For streaming sounds, we bypass this optimization.
	if( !target_chan->leftvol && !target_chan->rightvol )
	{
		// looping sounds don't use this optimization because they should stick around until they're killed.
		if( !sfx->cache || sfx->cache->loopStart == -1 )
		{
			// if this is a streaming sound, play the whole thing.
			if( chan != CHAN_STREAM )
			{
				S_FreeChannel( target_chan );
				return; // not audible at all
			}
		}
	}

	// Init client entity mouth movement vars
	SND_InitMouth( ent, chan );

	for( ch_idx = NUM_AMBIENTS, check = channels + NUM_AMBIENTS; ch_idx < MAX_DYNAMIC_CHANNELS; ch_idx++, check++)
	{
		if( check == target_chan ) continue;

		if( check->sfx == sfx && !check->pMixer.sample )
		{
			// skip up to 0.1 seconds of audio
			int skip = Com_RandomLong( 0, (int)( 0.1f * check->sfx->cache->rate ));
                              
			S_SetSampleStart( check, sfx->cache, skip );
			break;
		}
	}
}

/*
====================
S_RestoreSound

Restore a sound effect for the given entity on the given channel
====================
*/
void S_RestoreSound( const vec3_t pos, int ent, int chan, sound_t handle, float fvol, float attn, int pitch, int flags, double sample, double end, int wordIndex )
{
	wavdata_t	*pSource;
	sfx_t	*sfx = NULL;
	channel_t	*target_chan;
	int	vol;

	if( !dma.initialized ) return;
	sfx = S_GetSfxByHandle( handle );
	if( !sfx ) return;

	vol = bound( 0, fvol * 255, 255 );
	if( pitch <= 1 ) pitch = PITCH_NORM; // Invasion issues

	if( pitch == 0 )
	{
		MsgDev( D_WARN, "S_RestoreSound: ( %s ) ignored, called with pitch 0\n", sfx->name );
		return;
	}

	// pick a channel to play on
	if( chan == CHAN_STATIC )
		target_chan = SND_PickStaticChannel( ent, sfx, pos );
	else target_chan = SND_PickDynamicChannel( ent, chan, sfx );

	if( !target_chan )
	{
		MsgDev( D_ERROR, "S_RestoreSound: dropped sound \"sound/%s\"\n", sfx->name );
		return;
	}

	// spatialize
	Q_memset( target_chan, 0, sizeof( *target_chan ));

	VectorCopy( pos, target_chan->origin );
	target_chan->staticsound = ( ent == 0 ) ? true : false;
	target_chan->use_loop = (flags & SND_STOP_LOOPING) ? false : true;
	target_chan->localsound = (flags & SND_LOCALSOUND) ? true : false;
	target_chan->dist_mult = (attn / SND_CLIP_DISTANCE);
	target_chan->master_vol = vol;
	target_chan->entnum = ent;
	target_chan->entchannel = chan;
	target_chan->basePitch = pitch;
	target_chan->isSentence = false;
	target_chan->radius = 0.0f;
	target_chan->sfx = sfx;

	// initialize gain due to obscured sound source
	target_chan->bfirstpass = true;
	target_chan->ob_gain = 0.0f;
	target_chan->ob_gain_inc = 0.0f;
	target_chan->ob_gain_target = 0.0f;
	target_chan->bTraced = false;

	pSource = NULL;

	if( S_TestSoundChar( sfx->name, '!' ))
	{
		// this is a sentence
		// link all words and load the first word
		// NOTE: sentence names stored in the cache lookup are
		// prepended with a '!'.  Sentence names stored in the
		// sentence file do not have a leading '!'. 
		VOX_LoadSound( target_chan, S_SkipSoundChar( sfx->name ));

		// save the sentencename for future save\restores
		Q_strncpy( target_chan->name, sfx->name, sizeof( target_chan->name ));

		// not a first word in sentence!
		if( wordIndex != 0 )
		{
			VOX_FreeWord( target_chan );		// release first loaded word
			target_chan->wordIndex = wordIndex;	// restore current word
			VOX_LoadWord( target_chan );

			if( target_chan->currentWord )
			{
				target_chan->sfx = target_chan->words[target_chan->wordIndex].sfx;
				sfx = target_chan->sfx;
				pSource = sfx->cache;
			}
		}
		else
		{
			sfx = target_chan->sfx;
			pSource = sfx->cache;
		}
	}
	else
	{
		// regular or streamed sound fx
		pSource = S_LoadSound( sfx );
		target_chan->name[0] = '\0';
	}

	if( !pSource )
	{
		S_FreeChannel( target_chan );
		return;
	}

	SND_Spatialize( target_chan );

	// NOTE: first spatialization may be failed because listener position is invalid at this time
	// so we should keep all sounds an actual and waiting for player spawn.

	// apply the sample offests
	target_chan->pMixer.sample = sample;
	target_chan->pMixer.forcedEndSample = end;	

	// Init client entity mouth movement vars
	SND_InitMouth( ent, chan );
}

/*
=================
S_AmbientSound

Start playback of a sound, loaded into the static portion of the channel array.
Currently, this should be used for looping ambient sounds, looping sounds
that should not be interrupted until complete, non-creature sentences,
and one-shot ambient streaming sounds.  Can also play 'regular' sounds one-shot,
in case designers want to trigger regular game sounds.
Pitch changes playback pitch of wave by % above or below 100.  Ignored if pitch == 100

NOTE: volume is 0.0 - 1.0 and attenuation is 0.0 - 1.0 when passed in.
=================
*/
void S_AmbientSound( const vec3_t pos, int ent, sound_t handle, float fvol, float attn, int pitch, int flags )
{
	channel_t	*ch;
	wavdata_t	*pSource = NULL;
	sfx_t	*sfx = NULL;
	int	vol, fvox = 0;
	float	radius = SND_RADIUS_MAX;
	vec3_t	origin;		

	if( !dma.initialized ) return;
	sfx = S_GetSfxByHandle( handle );
	if( !sfx ) return;

	vol = bound( 0, fvol * 255, 255 );
	if( pitch <= 1 ) pitch = PITCH_NORM; // Invasion issues

	if( flags & (SND_STOP|SND_CHANGE_VOL|SND_CHANGE_PITCH))
	{
		if( S_AlterChannel( ent, CHAN_STATIC, sfx, vol, pitch, flags ))
			return;
		if( flags & SND_STOP ) return;
	}
	
	if( pitch == 0 )
	{
		MsgDev( D_WARN, "S_AmbientSound: ( %s ) ignored, called with pitch 0\n", sfx->name );
		return;
	}

	if( pos ) VectorCopy( pos, origin );
	else VectorClear( origin );

	CL_GetEntitySpatialization( ent, origin, &radius );

	// pick a channel to play on from the static area
	ch = SND_PickStaticChannel( ent, sfx, origin );
	if( !ch ) return;

	if( S_TestSoundChar( sfx->name, '!' ))
	{
		// this is a sentence. link words to play in sequence.
		// NOTE: sentence names stored in the cache lookup are
		// prepended with a '!'.  Sentence names stored in the
		// sentence file do not have a leading '!'. 

		// link all words and load the first word
		VOX_LoadSound( ch, S_SkipSoundChar( sfx->name ));
		Q_strncpy( ch->name, sfx->name, sizeof( ch->name ));
		sfx = ch->sfx;
		pSource = sfx->cache;
		fvox = 1;
	}
	else
	{
		// load regular or stream sound
		pSource = S_LoadSound( sfx );
		ch->sfx = sfx;
		ch->isSentence = false;
		ch->name[0] = '\0';
	}

	if( !pSource )
	{
		S_FreeChannel( ch );
		return;
	}

	VectorCopy( origin, ch->origin );

	// never update positions if source entity is 0
	ch->staticsound = ( ent == 0 ) ? true : false;
	ch->use_loop = (flags & SND_STOP_LOOPING) ? false : true;
	ch->localsound = (flags & SND_LOCALSOUND) ? true : false;
	ch->master_vol = vol;
	ch->dist_mult = (attn / SND_CLIP_DISTANCE);
	ch->entchannel = CHAN_STATIC;
	ch->basePitch = pitch;
	ch->entnum = ent;
	ch->radius = radius;

	// initialize gain due to obscured sound source
	ch->bfirstpass = true;
	ch->ob_gain = 0.0;
	ch->ob_gain_inc = 0.0;
	ch->ob_gain_target = 0.0;
	ch->bTraced = false;

	SND_Spatialize( ch );
}

/*
==================
S_StartLocalSound
==================
*/
void S_StartLocalSound(  const char *name, float volume, qboolean reliable )
{
	sound_t	sfxHandle;
	int	flags = (SND_LOCALSOUND|SND_STOP_LOOPING);
	int	channel = CHAN_AUTO;

	if( reliable ) channel = CHAN_STATIC;

	if( !dma.initialized ) return;	
	sfxHandle = S_RegisterSound( name );
	S_StartSound( NULL, s_listener.entnum, channel, sfxHandle, volume, ATTN_NONE, PITCH_NORM, flags );
}

/*
==================
S_GetCurrentStaticSounds

grab all static sounds playing at current channel
==================
*/
int S_GetCurrentStaticSounds( soundlist_t *pout, int size )
{
	int	sounds_left = size;
	int	i;

	if( !dma.initialized )
		return 0;

	for( i = MAX_DYNAMIC_CHANNELS; i < total_channels && sounds_left; i++ )
	{
		if( channels[i].entchannel == CHAN_STATIC && channels[i].sfx && channels[i].sfx->name[0] )
		{
			if( channels[i].isSentence && channels[i].name[0] )
				Q_strncpy( pout->name, channels[i].name, sizeof( pout->name ));
			else Q_strncpy( pout->name, channels[i].sfx->name, sizeof( pout->name ));
			pout->entnum = channels[i].entnum;
			VectorCopy( channels[i].origin, pout->origin );
			pout->volume = (float)channels[i].master_vol / 255.0f;
			pout->attenuation = channels[i].dist_mult * SND_CLIP_DISTANCE;
			pout->looping = ( channels[i].use_loop && channels[i].sfx->cache->loopStart != -1 );
			pout->pitch = channels[i].basePitch;
			pout->channel = channels[i].entchannel;
			pout->wordIndex = channels[i].wordIndex;
			pout->samplePos = channels[i].pMixer.sample;
			pout->forcedEnd = channels[i].pMixer.forcedEndSample;

			sounds_left--;
			pout++;
		}
	}

	return ( size - sounds_left );
}

/*
==================
S_GetCurrentStaticSounds

grab all static sounds playing at current channel
==================
*/
int S_GetCurrentDynamicSounds( soundlist_t *pout, int size )
{
	int	sounds_left = size;
	int	i, looped;

	if( !dma.initialized )
		return 0;

	for( i = 0; i < MAX_CHANNELS && sounds_left; i++ )
	{
		if( !channels[i].sfx || !channels[i].sfx->name[0] || !Q_stricmp( channels[i].sfx->name, "*default" ))
			continue;	// don't serialize default sounds

		looped = ( channels[i].use_loop && channels[i].sfx->cache->loopStart != -1 );

		if( channels[i].entchannel == CHAN_STATIC && looped )
			continue;	// never serialize static looped sounds. It will be restoring in game code 

		if( channels[i].isSentence && channels[i].name[0] )
			Q_strncpy( pout->name, channels[i].name, sizeof( pout->name ));
		else Q_strncpy( pout->name, channels[i].sfx->name, sizeof( pout->name ));
		pout->entnum = (channels[i].entnum < 0) ? 0 : channels[i].entnum;
		VectorCopy( channels[i].origin, pout->origin );
		pout->volume = (float)channels[i].master_vol / 255.0f;
		pout->attenuation = channels[i].dist_mult * SND_CLIP_DISTANCE;
		pout->pitch = channels[i].basePitch;
		pout->channel = channels[i].entchannel;
		pout->wordIndex = channels[i].wordIndex;
		pout->samplePos = channels[i].pMixer.sample;
		pout->forcedEnd = channels[i].pMixer.forcedEndSample;
		pout->looping = looped;

		sounds_left--;
		pout++;
	}

	return ( size - sounds_left );
}

/*
===================
S_InitAmbientChannels
===================
*/
void S_InitAmbientChannels( void )
{
	int	ambient_channel;
	channel_t	*chan;

	for( ambient_channel = 0; ambient_channel < NUM_AMBIENTS; ambient_channel++ )
	{
		chan = &channels[ambient_channel];	

		chan->staticsound = true;
		chan->use_loop = true;
		chan->entchannel = CHAN_STATIC;
		chan->dist_mult = (ATTN_NONE / SND_CLIP_DISTANCE);
		chan->basePitch = PITCH_NORM;
	}
}

/*
===================
S_UpdateAmbientSounds
===================
*/
void S_UpdateAmbientSounds( void )
{
	mleaf_t	*leaf;
	float	vol;
	int	ambient_channel;
	channel_t	*chan;

	if( !snd_ambient ) return;

	// calc ambient sound levels
	if( !cl.worldmodel ) return;

	leaf = Mod_PointInLeaf( s_listener.origin, cl.worldmodel->nodes );

	if( !leaf || !s_ambient_level->value )
	{
		for( ambient_channel = 0; ambient_channel < NUM_AMBIENTS; ambient_channel++ )
			channels[ambient_channel].sfx = NULL;
		return;
	}

	for( ambient_channel = 0; ambient_channel < NUM_AMBIENTS; ambient_channel++ )
	{
		chan = &channels[ambient_channel];	
		chan->sfx = S_GetSfxByHandle( ambient_sfx[ambient_channel] );

		if( !chan->sfx ) continue;

		vol = s_ambient_level->value * leaf->ambient_sound_level[ambient_channel];
		if( vol < 8 ) vol = 0;

		// don't adjust volume too fast
		if( chan->master_vol < vol )
		{
			chan->master_vol += s_listener.frametime * s_ambient_fade->value;
			if( chan->master_vol > vol ) chan->master_vol = vol;
		}
		else if( chan->master_vol > vol )
		{
			chan->master_vol -= s_listener.frametime * s_ambient_fade->value;
			if( chan->master_vol < vol ) chan->master_vol = vol;
		}
		
		chan->leftvol = chan->rightvol = chan->master_vol;
	}
}

/*
==================
S_ClearBuffer
==================
*/
void S_ClearBuffer( void )
{
	s_rawend = 0;

	SNDDMA_BeginPainting ();
	if( dma.buffer ) Q_memset( dma.buffer, 0, dma.samples * 2 );
	SNDDMA_Submit ();

	MIX_ClearAllPaintBuffers( PAINTBUFFER_SIZE, true );
}

/*
==================
S_StopSound

stop all sounds for entity on a channel.
==================
*/
void GAME_EXPORT S_StopSound( int entnum, int channel, const char *soundname )
{
	sfx_t	*sfx;

	if( !dma.initialized ) return;
	sfx = S_FindName( soundname, NULL );
	S_AlterChannel( entnum, channel, sfx, 0, 0, SND_STOP );
}

/*
==================
S_StopAllSounds
==================
*/
void S_StopAllSounds( void )
{
	int	i;

	if( !dma.initialized ) return;
	total_channels = MAX_DYNAMIC_CHANNELS;	// no statics

	for( i = 0; i < MAX_CHANNELS; i++ ) 
	{
		if( !channels[i].sfx ) continue;
		S_FreeChannel( &channels[i] );
	}

	DSP_ClearState();

	// clear all the channels
	Q_memset( channels, 0, sizeof( channels ));

	// restart the ambient sounds
	S_InitAmbientChannels ();

	S_ClearBuffer ();

	// clear any remaining soundfade
	Q_memset( &soundfade, 0, sizeof( soundfade ));
}

//=============================================================================
void S_UpdateChannels( void )
{
	uint	endtime;
	int	samps;

	SNDDMA_BeginPainting();

	if( !dma.buffer ) return;

	// updates DMA time
	soundtime = SNDDMA_GetSoundtime();

	// soundtime - total samples that have been played out to hardware at dmaspeed
	// paintedtime - total samples that have been mixed at speed
	// endtime - target for samples in mixahead buffer at speed
	endtime = soundtime + s_mixahead->value * SOUND_DMA_SPEED;
	samps = dma.samples >> 1;

	if((int)(endtime - soundtime) > samps )
		endtime = soundtime + samps;
	
	if(( endtime - paintedtime ) & 0x3 )
	{
		// the difference between endtime and painted time should align on 
		// boundaries of 4 samples. this is important when upsampling from 11khz -> 44khz.
		endtime -= ( endtime - paintedtime ) & 0x3;
	}

	MIX_PaintChannels( endtime );

	SNDDMA_Submit();
}

/*
=================
S_ExtraUpdate

Don't let sound skip if going slow
=================
*/
void S_ExtraUpdate( void )
{
	if( !dma.initialized ) return;
	S_UpdateChannels ();
}

/*
============
S_RenderFrame

Called once each time through the main loop
============
*/
void S_RenderFrame( ref_params_t *fd )
{
	int		i, j, total;
	channel_t		*ch, *combine;
	con_nprint_t	info;

	if( !dma.initialized ) return;
	if( !fd ) return; // too early

	// if the loading plaque is up, clear everything
	// out to make sure we aren't looping a dirty
	// dma buffer while loading
	// update any client side sound fade
	S_UpdateSoundFade();

	s_listener.entnum = fd->viewentity;	// can be camera entity too
	s_listener.frametime = fd->frametime;
	s_listener.waterlevel = fd->waterlevel;
	s_listener.active = CL_IsInGame();
	s_listener.inmenu = CL_IsInMenu();
	s_listener.paused = fd->paused;

	VectorCopy( fd->vieworg, s_listener.origin );
	VectorCopy( fd->simvel, s_listener.velocity );
	AngleVectors( fd->viewangles, s_listener.forward, s_listener.right, s_listener.up );

	// update general area ambient sound sources
	S_UpdateAmbientSounds();

	combine = NULL;

	// update spatialization for static and dynamic sounds	
	for( i = NUM_AMBIENTS, ch = channels + NUM_AMBIENTS; i < total_channels; i++, ch++ )
	{
		if( !ch->sfx ) continue;
		SND_Spatialize( ch ); // respatialize channel

		if( !ch->leftvol && !ch->rightvol )
			continue;

		// try to combine static sounds with a previous channel of the same
		// sound effect so we don't mix five torches every frame
		// g-cont: perfomance option, probably kill stereo effect in most cases
		if( i >= MAX_DYNAMIC_CHANNELS && s_combine_sounds->integer )
		{
			// see if it can just use the last one
			if( combine && combine->sfx == ch->sfx )
			{
				combine->leftvol += ch->leftvol;
				combine->rightvol += ch->rightvol;
				ch->leftvol = ch->rightvol = 0;
				continue;
			}

			// search for one
			combine = channels + MAX_DYNAMIC_CHANNELS;

			for( j = MAX_DYNAMIC_CHANNELS; j < i; j++, combine++ )
			{
				if( combine->sfx == ch->sfx )
					break;
			}

			if( j == total_channels )
			{
				combine = NULL;
			}
			else
			{
				if( combine != ch )
				{
					combine->leftvol += ch->leftvol;
					combine->rightvol += ch->rightvol;
					ch->leftvol = ch->rightvol = 0;
				}
				continue;
			}
		}
	}

	// debugging output
	if( s_show->integer )
	{
		info.color[0] = 1.0f;
		info.color[1] = 0.6f;
		info.color[2] = 0.0f;
		info.time_to_live = 0.5f;

		for( i = 0, total = 1, ch = channels; i < MAX_CHANNELS; i++, ch++ )
		{
			if( ch->sfx && ( ch->leftvol || ch->rightvol ))
			{
				info.index = total;
				Con_NXPrintf( &info, "chan %i, pos (%.f %.f %.f) %f ent %i, lv%3i rv%3i %s\n",
				i, ch->origin[0], ch->origin[1], ch->origin[2], ch->radius, ch->entnum, ch->leftvol, ch->rightvol, ch->sfx->name );
				total++;
			}
		}

		// to differentiate modes
		if( s_cull->integer ) VectorSet( info.color, 0.0f, 1.0f, 0.0f );
		else VectorSet( info.color, 1.0f, 1.0f, 1.0f );
		info.index = 0;

		Con_NXPrintf( &info, "room_type: %i ----(%i)---- painted: %i\n", idsp_room, total - 1, paintedtime );
	}

	S_StreamBackgroundTrack ();
	S_StreamSoundTrack ();

	// mix some sound
	S_UpdateChannels ();
}

/*
===============================================================================

console functions

===============================================================================
*/
void S_Play_f( void )
{
	if( Cmd_Argc() == 1 )
	{
		Msg( "Usage: play <soundfile>\n" );
		return;
	}

	S_StartLocalSound( Cmd_Argv( 1 ), VOL_NORM, false );
}

void S_PlayVol_f( void )
{
	if( Cmd_Argc() == 1 )
	{
		Msg( "Usage: playvol <soundfile volume>\n" );
		return;
	}

	S_StartLocalSound( Cmd_Argv( 1 ), Q_atof( Cmd_Argv( 2 )), false );
}

void S_Say_f( void )
{
	if( Cmd_Argc() == 1 )
	{
		Msg( "Usage: speak <soundfile>\n" );
		return;
	}

	S_StartLocalSound( Cmd_Argv( 1 ), 1.0f, false );
}

void S_SayReliable_f( void )
{
	if( Cmd_Argc() == 1 )
	{
		Msg( "Usage: spk <soundfile>\n" );
		return;
	}

	S_StartLocalSound( Cmd_Argv( 1 ), 1.0f, true );
}

/*
=================
S_Music_f
=================
*/
void S_Music_f( void )
{
	int	c = Cmd_Argc();

	// run background track
	if( c == 1 )
	{
		// blank name stopped last track
		S_StopBackgroundTrack();
	}
	else if( c == 2 )
	{
		string	intro, main, track;
		char	*ext[] = { "mp3", "wav" };
		int	i;

		Q_strncpy( track, Cmd_Argv( 1 ), sizeof( track ));
		Q_snprintf( intro, sizeof( intro ), "%s_intro", Cmd_Argv( 1 ));
		Q_snprintf( main, sizeof( main ), "%s_main", Cmd_Argv( 1 ));

		for( i = 0; i < 2; i++ )
		{
			const char *intro_path = va( "media/%s.%s", intro, ext[i] );
			const char *main_path = va( "media/%s.%s", main, ext[i] );

			if( FS_FileExists( intro_path, false ) && FS_FileExists( main_path, false ))
			{
				// combined track with introduction and main loop theme
				S_StartBackgroundTrack( intro, main, 0 );
				break;
			}
			else if( FS_FileExists( va( "media/%s.%s", track, ext[i] ), false ))
			{
				// single non-looped theme
				S_StartBackgroundTrack( track, NULL, 0 );
				break;
			}
		}

	}
	else if( c == 3 )
	{
		S_StartBackgroundTrack( Cmd_Argv( 1 ), Cmd_Argv( 2 ), 0 );
	}
	else if( c == 4 && Q_atoi( Cmd_Argv( 3 )) != 0 )
	{
		// restore command for singleplayer: all arguments are valid
		S_StartBackgroundTrack( Cmd_Argv( 1 ), Cmd_Argv( 2 ), Q_atoi( Cmd_Argv( 3 )));
	}
	else Msg( "Usage: music <musicfile> [loopfile]\n" );
}

/*
=================
S_StopSound_f
=================
*/
void S_StopSound_f( void )
{
	S_StopAllSounds();
}

/*
=================
S_SoundInfo_f
=================
*/
void S_SoundInfo_f( void )
{
	S_PrintDeviceName();

	Msg( "%5d channel(s)\n", 2 );
	Msg( "%5d samples\n", dma.samples );
	Msg( "%5d bits/sample\n", 16 );
	Msg( "%5d bytes/sec\n", SOUND_DMA_SPEED );
	Msg( "%5d total_channels\n", total_channels );

	S_PrintBackgroundTrackState ();
}

/*
================
S_Init
================
*/
qboolean S_Init( void )
{

	s_volume = Cvar_Get( "volume", "0.7", CVAR_ARCHIVE, "sound volume" );
	s_musicvolume = Cvar_Get( "musicvolume", "1.0", CVAR_ARCHIVE, "background music volume" );
	s_mixahead = Cvar_Get( "_snd_mixahead", "0.12", 0, "how much sound to mix ahead of time" );
	s_show = Cvar_Get( "s_show", "0", CVAR_ARCHIVE, "show playing sounds" );
	s_lerping = Cvar_Get( "s_lerping", "0", CVAR_ARCHIVE, "apply interpolation to sound output" );
	s_ambient_level = Cvar_Get( "ambient_level", "0.3", 0, "volume of environment noises (water and wind)" );
	s_ambient_fade = Cvar_Get( "ambient_fade", "100", 0, "rate of volume fading when client is moving" );
	s_combine_sounds = Cvar_Get( "s_combine_channels", "1", CVAR_ARCHIVE, "combine channels with same sounds" ); 
	snd_foliage_db_loss = Cvar_Get( "snd_foliage_db_loss", "4", 0, "foliage loss factor" ); 
	snd_gain_max = Cvar_Get( "snd_gain_max", "1", 0, "gain maximal threshold" );
	snd_gain_min = Cvar_Get( "snd_gain_min", "0.01", 0, "gain minimal threshold" );
	snd_mute_losefocus = Cvar_Get( "snd_mute_losefocus", "1", CVAR_ARCHIVE, "silence the audio when game window loses focus" );
	s_refdist = Cvar_Get( "s_refdist", "36", 0, "soundlevel reference distance" );
	s_refdb = Cvar_Get( "s_refdb", "60", 0, "soundlevel refernce dB" );
	snd_gain = Cvar_Get( "snd_gain", "1", 0, "sound default gain" );
	s_cull = Cvar_Get( "s_cull", "0", CVAR_ARCHIVE, "cull sounds by geometry" );
	s_test = Cvar_Get( "s_test", "0", 0, "engine developer cvar for quick testing of new features" );
	s_phs = Cvar_Get( "s_phs", "0", CVAR_ARCHIVE, "cull sounds by PHS" );
	s_reverse_channels = Cvar_Get( "s_reverse_channels", "0", CVAR_ARCHIVE, "reverse left and right channels" );

#if XASH_SOUND != SOUND_NULL
	if( Sys_CheckParm( "-nosound" ))
	{
		MsgDev( D_INFO, "Audio: Disabled\n" );
		return false;
	}
#ifdef XASH_SDL
	if( SDL_Init( SDL_INIT_AUDIO ) )
	{
		MsgDev( D_ERROR, "Audio: SDL: %s \n", SDL_GetError() );
		return false;
	}
#elif !defined(XASH_OPENSL)
	return false;
#endif

	Cmd_AddCommand( "play", S_Play_f, "play a specified sound file" );
	Cmd_AddCommand( "playvol", S_PlayVol_f, "play a specified sound file with specified volume" );
	Cmd_AddCommand( "stopsound", S_StopSound_f, "stop all sounds" );
	Cmd_AddCommand( "music", S_Music_f, "start a background track" );
	Cmd_AddCommand( "soundlist", S_SoundList_f, "display loaded sounds" );
	Cmd_AddCommand( "s_info", S_SoundInfo_f, "print sound system information" );
	Cmd_AddCommand( "+voicerecord", Cmd_Null_f, "start voice recording (non-implemented)" );
	Cmd_AddCommand( "-voicerecord", Cmd_Null_f, "stop voice recording (non-implemented)" );
	Cmd_AddCommand( "spk", S_SayReliable_f, "reliable play of a specified sentence" );
	Cmd_AddCommand( "speak", S_Say_f, "play a specified sentence" );

	if( !SNDDMA_Init( host.hWnd ))
	{
		MsgDev( D_INFO, "S_Init: sound system can't be initialized\n" );
		return false;
	}

	sndpool = Mem_AllocPool( "Sound Zone" );
	soundtime = 0;
	paintedtime = 0;

	// clear ambient sounds
	Q_memset( ambient_sfx, 0, sizeof( ambient_sfx ));

	MIX_InitAllPaintbuffers ();

	S_InitScaletable ();
	S_StopAllSounds ();
	VOX_Init ();
	AllocDsps ();

	return true;
#else
	MsgDev( D_INFO, "Audio: Not supported in this build\n" );
	return false;
#endif
}

// =======================================================================
// Shutdown sound engine
// =======================================================================
void S_Shutdown( void )
{
	if( !dma.initialized ) return;

	Cmd_RemoveCommand( "play" );
	Cmd_RemoveCommand( "playvol" );
	Cmd_RemoveCommand( "stopsound" );
	Cmd_RemoveCommand( "music" );
	Cmd_RemoveCommand( "soundlist" );
	Cmd_RemoveCommand( "s_info" );
	Cmd_RemoveCommand( "+voicerecord" );
	Cmd_RemoveCommand( "-voicerecord" );
	Cmd_RemoveCommand( "spk" );
	Cmd_RemoveCommand( "speak" );

	S_StopAllSounds ();
	S_FreeSounds ();
	VOX_Shutdown ();
	FreeDsps ();

	SNDDMA_Shutdown ();
	MIX_FreeAllPaintbuffers ();
	Mem_FreePool( &sndpool );
	dsp_room = NULL;
}
#endif // XASH_DEDICATED
