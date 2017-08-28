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
char *GAME_EXPORT Info_ValueForKey( const char *s, const char *key )
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

qboolean GAME_EXPORT Info_RemoveKey( char *s, const char *key )
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

/*
==================
Info_IsImportantKey

Check is key is important, like "model", "name", starkey and so on
==================
*/
static int Info_KeyImportance( const char *key )
{
	// these keys are very important
	// because clients uses these keys to identify or show playermodel
	// it will ruin game, if they will be removed
	if( !Q_strcmp( key, "model" ))
		return 2;
	if( !Q_strcmp( key, "name" ))
		return 2;

	// these keys are used mainly by server code
	if( key[0] == '*' )
		return 1;
	if( !Q_strcmp( key, "rate" ))
		return 1;
	if( !Q_strcmp( key, "cl_lw" ))
		return 1;
	if( !Q_strcmp( key, "cl_lc" ))
		return 1;
	if( !Q_strcmp( key, "cl_updaterate" ))
		return 1;
	if( !Q_strcmp( key, "hltv" ))
		return 1;

	return 0; // not important key, can be omitted or removed
}


/*
==================
Info_FindLargestKey

Find largest key, excluding keys which is important
==================
*/
static char *Info_FindLargestKey( char *s, const int importance )
{
	static char largestKey[MAX_INFO_KEY];
	char key[MAX_INFO_KEY];
	int largeKeySize = 0;

	if( *s == '\\' ) s++;

	largestKey[0] = 0;

	while( *s )
	{
		int keySize = 0, valueSize = 0;

		while( keySize < MAX_INFO_KEY && *s && *s != '\\' )
		{
			key[keySize] = *s;
			keySize++;
			s++;
		}
		key[keySize] = 0;

		if( !*s )
			break;
		if( *s == '\\' ) s++;

		while( *s && *s != '\\' )
		{
			valueSize++;
			s++;
		}

		// exclude important keys
		if( largeKeySize < keySize + valueSize && Info_KeyImportance( key ) < importance )
		{
			largeKeySize = keySize + valueSize;
			Q_strncpy( largestKey, key, sizeof( largestKey ) );
		}

		if( *s )
			s++;
	}

	return largestKey;
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

	if( Q_strlen( key ) > MAX_INFO_KEY - 1 || Q_strlen( value ) > MAX_INFO_VALUE - 1 )
	{
		MsgDev( D_ERROR, "SetValueForKey: keys and values must be < %i characters.\n", MAX_INFO_KEY );
		return false;
	}

	Info_RemoveKey( s, key );

	if( !value || !Q_strlen( value ))
		return true; // just clear variable

	Q_snprintf( new, sizeof( new ) - 1, "\\%s\\%s", key, value );

	if( Q_strlen( new ) + Q_strlen( s ) > maxsize )
	{
		// If it is important key, force add it
		int importance = Info_KeyImportance( key );

		if( importance )
		{
			char *largeKey;

			// remove largest key, check size. if there is still no room, do it again
			do
			{
				largeKey = Info_FindLargestKey( s, importance );
				Info_RemoveKey( s, largeKey );
			}
			while( Q_strlen( new ) + Q_strlen( s ) > maxsize && *largeKey != 0 );

			if( !*largeKey )
			{
				MsgDev( D_ERROR, "SetValueForKey: info string length exceeded\n" );
				return true; // info changed, new value can't saved
			}
		}
		else
		{
			MsgDev( D_ERROR, "SetValueForKey: info string length exceeded\n" );
			return true; // info changed, new value can't saved
		}
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

qboolean GAME_EXPORT Info_SetValueForKey( char *s, const char *key, const char *value, size_t maxsize )
{
	if( key[0] == '*' )
	{
		MsgDev( D_ERROR, "Can't set *keys\n" );
		return false;
	}

	return Info_SetValueForStarKey( s, key, value, maxsize );
}

static void Cvar_LookupBitInfo( const char *name, const char *string, void *info, void *unused )
{
	Info_SetValueForKey( (char *)info, name, string, MAX_INFO_STRING );
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
