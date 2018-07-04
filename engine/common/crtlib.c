/*
crtlib.c - internal stdlib
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

#include <math.h>
#include "common.h"

void Q_strnupr( const char *in, char *out, size_t size_out )
{
	if( size_out == 0 ) return;

	while( *in && size_out > 1 )
	{
		if( *in >= 'a' && *in <= 'z' )
			*out++ = *in++ + 'A' - 'a';
		else *out++ = *in++;
		size_out--;
	}
	*out = '\0';
}

void Q_strnlwr( const char *in, char *out, size_t size_out )
{
	if( size_out == 0 ) return;

	while( *in && size_out > 1 )
	{
		if( *in >= 'A' && *in <= 'Z' )
			*out++ = *in++ + 'a' - 'A';
		else *out++ = *in++;
		size_out--;
	}
	*out = '\0';
}

qboolean Q_isdigit( const char *str )
{
	if( str && *str )
	{
		while( isdigit( *str )) str++;
		if( !*str ) return true;
	}
	return false;
}

#ifndef XASH_SKIPCRTLIB

char Q_toupper( const char in )
{
	char	out;

	if( in >= 'a' && in <= 'z' )
		out = in + 'A' - 'a';
	else out = in;

	return out;
}

char Q_tolower( const char in )
{
	char	out;

	if( in >= 'A' && in <= 'Z' )
		out = in + 'a' - 'A';
	else out = in;

	return out;
}
#endif

char *_copystring( byte *mempool, const char *s, const char *filename, int fileline )
{
	char	*b;

	if( !s ) return NULL;
	if( !mempool ) mempool = host.mempool;

	b = _Mem_Alloc( mempool, Q_strlen( s ) + 1, filename, fileline );
	Q_strcpy( b, s );

	return b;
}
#ifndef XASH_SKIPCRTLIB
int Q_atoi( const char *str )
{
	int       val = 0;
	int	c, sign;

	if( !str ) return 0;

	// check for empty charachters in string
	while( str && *str == ' ' )
		str++;

	if( !str ) return 0;
	
	if( *str == '-' )
	{
		sign = -1;
		str++;
	}
	else sign = 1;
		
	// check for hex
	if( str[0] == '0' && ( str[1] == 'x' || str[1] == 'X' ))
	{
		str += 2;
		while( 1 )
		{
			c = *str++;
			if( c >= '0' && c <= '9' ) val = (val<<4) + c - '0';
			else if( c >= 'a' && c <= 'f' ) val = (val<<4) + c - 'a' + 10;
			else if( c >= 'A' && c <= 'F' ) val = (val<<4) + c - 'A' + 10;
			else return val * sign;
		}
	}
	
	// check for character
	if( str[0] == '\'' )
		return sign * str[1];
	
	// assume decimal
	while( 1 )
	{
		c = *str++;
		if( c < '0' || c > '9' )
			return val * sign;
		val = val * 10 + c - '0';
	}
	return 0;
}

float Q_atof( const char *str )
{
	double	val = 0;
	int	c, sign, decimal, total;

	if( !str ) return 0.0f;

	// check for empty charachters in string
	while( str && *str == ' ' )
		str++;

	if( !str ) return 0.0f;
	
	if( *str == '-' )
	{
		sign = -1;
		str++;
	}
	else sign = 1;
		
	// check for hex
	if( str[0] == '0' && ( str[1] == 'x' || str[1] == 'X' ))
	{
		str += 2;
		while( 1 )
		{
			c = *str++;
			if( c >= '0' && c <= '9' ) val = (val * 16) + c - '0';
			else if( c >= 'a' && c <= 'f' ) val = (val * 16) + c - 'a' + 10;
			else if( c >= 'A' && c <= 'F' ) val = (val * 16) + c - 'A' + 10;
			else return val * sign;
		}
	}
	
	// check for character
	if( str[0] == '\'' ) return sign * str[1];
	
	// assume decimal
	decimal = -1;
	total = 0;

	while( 1 )
	{
		c = *str++;
		if( c == '.' )
		{
			decimal = total;
			continue;
		}

		if( c < '0' || c > '9' )
			break;
		val = val * 10 + c - '0';
		total++;
	}

	if( decimal == -1 )
		return val * sign;

	while( total > decimal )
	{
		val /= 10;
		total--;
	}
	
	return val * sign;
}
#endif
void Q_atov( float *vec, const char *str, size_t siz )
{
	string	buffer;
	char	*pstr, *pfront;
	int	j;

	Q_strncpy( buffer, str, sizeof( buffer ));
	Q_memset( vec, 0, sizeof( vec_t ) * siz );
	pstr = pfront = buffer;

	for( j = 0; j < siz; j++ )
	{
		vec[j] = Q_atof( pfront );

		// valid separator is space
		while( *pstr && *pstr != ' ' )
			pstr++;

		if( !*pstr ) break;
		pstr++;
		pfront = pstr;
	}
}

static qboolean Q_starcmp( const char *pattern, const char *text )
{
	char		c, c1;
	const char	*p = pattern, *t = text;

	while(( c = *p++ ) == '?' || c == '*' )
	{
		if( c == '?' && *t++ == '\0' )
			return false;
	}

	if( c == '\0' ) return true;

	for( c1 = (( c == '\\' ) ? *p : c ); ; )
	{
		if( Q_tolower( *t ) == c1 && Q_stricmpext( p - 1, t ))
			return true;
		if( *t++ == '\0' ) return false;
	}
}

qboolean Q_stricmpext( const char *pattern, const char *text )
{
	char	c;

	while(( c = *pattern++ ) != '\0' )
	{
		switch( c )
		{
		case '?':
			if( *text++ == '\0' )
				return false;
			break;
		case '\\':
			if( Q_tolower( *pattern++ ) != Q_tolower( *text++ ))
				return false;
			break;
		case '*':
			return Q_starcmp( pattern, text );
		default:
			if( Q_tolower( c ) != Q_tolower( *text++ ))
				return false;
		}
	}
	return true;
}

const char* Q_timestamp( int format )
{
	static string	timestamp;
	time_t		crt_time;
	const struct tm	*crt_tm;
	string		timestring;

	time( &crt_time );
	crt_tm = localtime( &crt_time );

	switch( format )
	{
	case TIME_FULL:
		// Build the full timestamp (ex: "Apr03 2007 [23:31.55]");
		strftime( timestring, sizeof( timestring ), "%b%d %Y [%H:%M.%S]", crt_tm );
		break;
	case TIME_DATE_ONLY:
		// Build the date stamp only (ex: "Apr03 2007");
		strftime( timestring, sizeof( timestring ), "%b%d %Y", crt_tm );
		break;
	case TIME_TIME_ONLY:
		// Build the time stamp only (ex: "23:31.55");
		strftime( timestring, sizeof( timestring ), "%H:%M.%S", crt_tm );
		break;
	case TIME_NO_SECONDS:
		// Build the time stamp exclude seconds (ex: "13:46");
		strftime( timestring, sizeof( timestring ), "%H:%M", crt_tm );
		break;
	case TIME_YEAR_ONLY:
		// Build the date stamp year only (ex: "2006");
		strftime( timestring, sizeof( timestring ), "%Y", crt_tm );
		break;
	case TIME_FILENAME:
		// Build a timestamp that can use for filename (ex: "Nov2006-26 (19.14.28)");
		strftime( timestring, sizeof( timestring ), "%b%Y-%d_%H.%M.%S", crt_tm );
		break;
	default: return NULL;
	}

	Q_strncpy( timestamp, timestring, sizeof( timestamp ));

	return timestamp;
}

#ifndef XASH_SKIPCRTLIB
char *Q_strstr( const char *string, const char *string2 )
{
	int	c, len;

	if( !string || !string2 ) return NULL;

	c = *string2;
	len = Q_strlen( string2 );

	while( string )
	{
		for( ; *string && *string != c; string++ );

		if( *string )
		{
			if( !Q_strncmp( string, string2, len ))
				break;
			string++;
		}
		else return NULL;
	}
	return (char *)string;
}

char *Q_stristr( const char *string, const char *string2 )
{
	int	c, len;

	if( !string || !string2 ) return NULL;

	c = Q_tolower( *string2 );
	len = Q_strlen( string2 );

	while( string )
	{
		for( ; *string && Q_tolower( *string ) != c; string++ );

		if( *string )
		{
			if( !Q_strnicmp( string, string2, len ))
				break;
			string++;
		}
		else return NULL;
	}
	return (char *)string;
}

#if XASH_USE_STB_SPRINTF
	#define STB_SPRINTF_IMPLEMENTATION
	#define STB_SPRINTF_DECORATE(name) Q_##name
	#undef Q_vsprintf
	#include "stb/stb_sprintf.h"
#else // XASH_USE_STB_SPRINTF
int Q_vsnprintf( char *buffer, size_t buffersize, const char *format, va_list args )
{
	int	result;

	result = vsnprintf( buffer, buffersize, format, args );

	if( result < 0 || result >= ( int )buffersize )
	{
		buffer[buffersize - 1] = '\0';
		return -1;
	}
	return result;
}

int Q_snprintf( char *buffer, size_t buffersize, const char *format, ... )
{
	va_list	args;
	int	result;

	va_start( args, format );
	result = Q_vsnprintf( buffer, buffersize, format, args );
	va_end( args );

	return result;
}

int Q_sprintf( char *buffer, const char *format, ... )
{
	va_list	args;
	int	result;

	va_start( args, format );
	result = Q_vsnprintf( buffer, 99999, format, args );
	va_end( args );

	return result;
}
#endif // XASH_USE_STB_SPRINTF
#endif // XASH_SKIPCRTLIB

char *Q_pretifymem( float value, int digitsafterdecimal )
{
	static char	output[8][32];
	static int	current;
	float		onekb = 1024.0f;
	float		onemb = onekb * onekb;
	char		suffix[8];
	char		*out = output[current];
	char		val[32], *i, *o, *dot;
	int		pos;

	current = ( current + 1 ) & ( 8 - 1 );

	// first figure out which bin to use
	if( value > onemb )
	{
		value /= onemb;
		Q_sprintf( suffix, " Mb" );
	}
	else if( value > onekb )
	{
		value /= onekb;
		Q_sprintf( suffix, " Kb" );
	}
	else Q_sprintf( suffix, " bytes" );

	// clamp to >= 0
	digitsafterdecimal = max( digitsafterdecimal, 0 );

	// if it's basically integral, don't do any decimals
	if( fabs( value - (int)value ) < 0.00001 )
	{
		Q_sprintf( val, "%i%s", (int)value, suffix );
	}
	else
	{
		char fmt[32];

		// otherwise, create a format string for the decimals
		Q_sprintf( fmt, "%%.%if%s", digitsafterdecimal, suffix );
		Q_sprintf( val, fmt, value );
	}

	// copy from in to out
	i = val;
	o = out;

	// search for decimal or if it was integral, find the space after the raw number
	dot = Q_strstr( i, "." );
	if( !dot ) dot = Q_strstr( i, " " );

	pos = dot - i;	// compute position of dot
	pos -= 3;		// don't put a comma if it's <= 3 long

	while( *i )
	{
		// if pos is still valid then insert a comma every third digit, except if we would be
		// putting one in the first spot
		if( pos >= 0 && !( pos % 3 ))
		{
			// never in first spot
			if( o != out ) *o++ = ',';
		}

		pos--;		// count down comma position
		*o++ = *i++;	// copy rest of data as normal
	}
	*o = 0; // terminate

	return out;
}

/*
============
va

does a varargs printf into a temp buffer,
so I don't need to have varargs versions
of all text functions.
============
*/
char *va( const char *format, ... )
{
	va_list		argptr;
	static char	string[256][1024], *s;
	static int	stringindex = 0;

	s = string[stringindex];
	stringindex = (stringindex + 1) & 255;
	va_start( argptr, format );
	Q_vsnprintf( s, sizeof( string[0] ), format, argptr );
	va_end( argptr );

	return s;
}

void _Q_memcpy( void *dest, const void *src, size_t count, const char *filename, int fileline )
{
	if( src == NULL || count <= 0 ) return; // nothing to copy
	if( dest == NULL ) Sys_Error( "memcpy: dest == NULL (called at %s:%i)\n", filename, fileline );
	memcpy( dest, src, count );
}

void _Q_memset( void *dest, int set, size_t count, const char *filename, int fileline )
{
	if( dest == NULL ) Sys_Error( "memset: dest == NULL (called at %s:%i)\n", filename, fileline );
	memset( dest, set, count );
}

int _Q_memcmp( const void *src0, const void *src1, size_t count, const char *filename, int fileline )
{
	if( src0 == NULL ) Sys_Error( "memcmp: src1 == NULL (called at %s:%i)\n", filename, fileline );
	if( src1 == NULL ) Sys_Error( "memcmp: src2 == NULL (called at %s:%i)\n", filename, fileline );
	return memcmp( src0, src1, count );
}

void _Q_memmove( void *dest, const void *src, size_t count, const char *filename, int fileline )
{
	if( src == NULL || count <= 0 ) return; // nothing to move
	if( dest == NULL ) Sys_Error( "memmove: dest == NULL (called at %s:%i)\n", filename, fileline );
	memmove( dest, src, count );
}
#ifndef XASH_FORCEINLINE
#include "crtlib_inline.h"
#endif
