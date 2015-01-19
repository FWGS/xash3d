/*
infostring.c - network info strings
Copyright (C) 2008 Uncle Mike

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

#define MAX_INFO_KEY	64
#define MAX_INFO_VALUE	64

static char		infostring[MAX_INFO_STRING*4];

/*
=======================================================================

			INFOSTRING STUFF
=======================================================================
*/
/*
===============
Info_Print

printing current key-value pair
===============
*/
void Info_Print( const char *s )
{
	char	key[MAX_INFO_STRING];
	char	value[MAX_INFO_STRING];
	char	*o;
	int	l;

	if( *s == '\\' ) s++;

	while( *s )
	{
		o = key;
		while( *s && *s != '\\' )
			*o++ = *s++;

		l = o - key;
		if( l < 20 )
		{
			Q_memset( o, ' ', 20 - l );
			key[20] = 0;
		}
		else *o = 0;
		Msg( "%s", key );

		if( !*s )
		{
			Msg( "(null)\n" );
			return;
		}

		o = value;
		s++;
		while( *s && *s != '\\' )
			*o++ = *s++;
		*o = 0;

		if( *s ) s++;
		Msg( "%s\n", value );
	}
}

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
char *Info_ValueForKey( const char *s, const char *key )
{
	char	pkey[MAX_INFO_STRING];
	static	char value[2][MAX_INFO_STRING]; // use two buffers so compares work without stomping on each other
	static	int valueindex;
	char	*o;
	
	valueindex ^= 1;
	if( *s == '\\' ) s++;

	while( 1 )
	{
		o = pkey;
		while( *s != '\\' && *s != '\n' )
		{
			if( !*s ) return "";
			*o++ = *s++;
		}

		*o = 0;
		s++;

		o = value[valueindex];

		while( *s != '\\' && *s != '\n' && *s )
		{
			if( !*s ) return "";
			*o++ = *s++;
		}
		*o = 0;

		if( !Q_strcmp( key, pkey ))
			return value[valueindex];
		if( !*s ) return "";
		s++;
	}
}

qboolean Info_RemoveKey( char *s, const char *key )
{
	char	*start;
	char	pkey[MAX_INFO_STRING];
	char	value[MAX_INFO_STRING];
	char	*o;

	if( Q_strstr( key, "\\" ))
		return false;

	while( 1 )
	{
		start = s;
		if( *s == '\\' ) s++;
		o = pkey;

		while( *s != '\\' )
		{
			if( !*s ) return false;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while( *s != '\\' && *s )
		{
			if( !*s ) return false;
			*o++ = *s++;
		}
		*o = 0;

		if( !Q_strcmp( key, pkey ))
		{
			Q_strcpy( start, s ); // remove this part
			return true;
		}
		if( !*s ) return false;
	}
}

void Info_RemovePrefixedKeys( char *start, char prefix )
{
	char	*s, *o;
	char	pkey[MAX_INFO_STRING];
	char	value[MAX_INFO_STRING];

	s = start;

	while( 1 )
	{
		if( *s == '\\' )
			s++;
		o = pkey;
		while( *s != '\\' )
		{
			if( !*s ) return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while( *s != '\\' && *s )
		{
			if( !*s ) return;
			*o++ = *s++;
		}
		*o = 0;

		if( pkey[0] == prefix )
		{
			Info_RemoveKey( start, pkey );
			s = start;
		}

		if( !*s ) return;
	}
}

/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate( const char *s )
{
	if( Q_strstr( s, "\"" )) return false;
	if( Q_strstr( s, ";" )) return false;
	return true;
}

qboolean Info_SetValueForStarKey( char *s, const char *key, const char *value, int maxsize )
{
	char	new[1024], *v;
	int	c;

	if( Q_strstr( key, "\\" ) || Q_strstr( value, "\\" ))
	{
		MsgDev( D_ERROR, "SetValueForKey: can't use keys or values with a \\\n" );
		return false;
	}

	if( Q_strstr( key, ";" ))
	{
		MsgDev( D_ERROR, "SetValueForKey: can't use keys or values with a semicolon\n" );
		return false;
	}

	if( Q_strstr( key, "\"" ) || Q_strstr( value, "\"" ))
	{
		MsgDev( D_ERROR, "SetValueForKey: can't use keys or values with a \"\n" );
		return false;
	}

	if( Q_strlen( key ) > MAX_INFO_KEY - 1 || Q_strlen( value ) > MAX_INFO_KEY - 1 )
	{
		MsgDev( D_ERROR, "SetValueForKey: keys and values must be < %i characters.\n", MAX_INFO_KEY );
		return false;
	}

	// this next line is kinda trippy
	if( *(v = Info_ValueForKey( s, key )))
	{
		// key exists, make sure we have enough room for new value, if we don't, don't change it!
		if( Q_strlen( value ) - Q_strlen( v ) + Q_strlen( s ) > maxsize )
		{
			MsgDev( D_ERROR, "SetValueForKey: info string length exceeded\n" );
			return false;
		}
	}

	Info_RemoveKey( s, key );

	if( !value || !Q_strlen( value ))
		return true; // just clear variable

	Q_snprintf( new, sizeof( new ) - 1, "\\%s\\%s", key, value );
	if( Q_strlen( new ) + Q_strlen( s ) > maxsize )
	{
		MsgDev( D_ERROR, "SetValueForKey: info string length exceeded\n" );
		return true; // info changed, new value can't saved
	}

	// only copy ascii values
	s += Q_strlen( s );
	v = new;

	while( *v )
	{
		c = *v++;
		c &= 255;	// strip high bits
		if( c >= 32 && c <= 255 )
			*s++ = c;
	}
	*s = 0;

	// all done
	return true;
}

qboolean Info_SetValueForKey( char *s, const char *key, const char *value )
{
	if( key[0] == '*' )
	{
		MsgDev( D_ERROR, "Can't set *keys\n" );
		return false;
	}

	return Info_SetValueForStarKey( s, key, value, MAX_INFO_STRING );
}

static void Cvar_LookupBitInfo( const char *name, const char *string, const char *info, void *unused )
{
	Info_SetValueForKey( (char *)info, name, string );
}

char *Cvar_Userinfo( void )
{
	infostring[0] = 0; // clear previous calls
	Cvar_LookupVars( CVAR_USERINFO, infostring, NULL, Cvar_LookupBitInfo ); 
	return infostring;
}

char *Cvar_Serverinfo( void )
{
	infostring[0] = 0; // clear previous calls
	Cvar_LookupVars( CVAR_SERVERINFO, infostring, NULL, Cvar_LookupBitInfo ); 
	return infostring;
}

char *Cvar_Physicinfo( void )
{
	infostring[0] = 0; // clear previous calls
	Cvar_LookupVars( CVAR_PHYSICINFO, infostring, NULL, Cvar_LookupBitInfo ); 
	return infostring;
}