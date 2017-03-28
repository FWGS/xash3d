/*
identification.c - unique id generation
Copyright (C) 2017 mittorn

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
#include "fcntl.h"
/*
==========================================================

simple 64-bit one-hash-func bloom filter
should be enough to determine if device exist in identifier

==========================================================
*/
typedef integer64 bloomfilter_t;

#define bf64_mask ((1U<<6)-1)

bloomfilter_t BloomFilter_Process( const char *buffer, int size )
{
	dword crc32;
	bloomfilter_t value = 0;

	CRC32_Init( &crc32 );
	CRC32_ProcessBuffer( &crc32, buffer, size );

	while( crc32 )
	{
		value |= ((integer64)1) << ( crc32 & bf64_mask );
		crc32 = crc32 >> 6;
	}

	return value;
}

bloomfilter_t BloomFilter_ProcessStr( const char *buffer )
{
	return BloomFilter_Process( buffer, Q_strlen( buffer ) );
}

uint BloomFilter_Weight( bloomfilter_t value )
{
	int weight = 0;

	while( value )
	{
		if( value & 1 )
			weight++;
		value = value >> 1;
	}

	return weight;
}

qboolean BloomFilter_ContainsString( bloomfilter_t filter, const char *str )
{
	bloomfilter_t value = BloomFilter_ProcessStr( str );

	return (filter & value) == value;
}

void ID_BloomFilter_f( void )
{
	bloomfilter_t value = 0;
	int i;

	for( i = 1; i < Cmd_Argc(); i++ )
		value |= BloomFilter_ProcessStr( Cmd_Argv( i ) );

	Msg( "%d %016llX\n", BloomFilter_Weight( value ), value );

	// test
	// for( i = 1; i < Cmd_Argc(); i++ )
	//	Msg( "%s: %d\n", Cmd_Argv( i ), BloomFilter_ContainsString( value, Cmd_Argv( i ) ) );
}

qboolean ID_VerifyHEX( const char *hex )
{
	uint chars = 0;
	char prev = 0;
	qboolean monotonic = true; // detect 11:22...
	int weight = 0;

	while( *hex++ )
	{
		char ch = Q_tolower( *hex );

		if( ch >= 'a' && ch <= 'f' || ch >= '0' && ch <= '9' )
		{
			if( prev && ( ch - prev < -1 || ch - prev > 1 ) )
				monotonic = false;

			if( ch >= 'a' )
				chars |= 1 << (ch - 'a' + 10);
			else
				chars |= 1 << (ch - '0');

			prev = ch;
		}
	}

	if( monotonic )
		return false;

	while( chars )
	{
		if( chars & 1 )
			weight++;

		chars = chars >> 1;

		if( weight > 2 )
			return true;
	}

	return false;
}

void ID_VerifyHEX_f( void )
{
	if( ID_VerifyHEX( Cmd_Argv( 1 ) ) )
		Msg( "Good\n" );
	else
		Msg( "Bad\n" );
}

qboolean ID_ProcessCPUInfo( bloomfilter_t *value )
{
	int cpuinfofd = open( "/proc/cpuinfo", O_RDONLY );
	char buffer[1024], *pbuf, *pbuf2;
	int ret;

	if( cpuinfofd < 0 )
		return false;

	if( (ret = read( cpuinfofd, buffer, 1024 ) ) <= 0 )
		return false;

	buffer[ret] = 0;

	pbuf = Q_strstr( buffer, "Serial" );
	if( !pbuf )
		return false;
	pbuf += 6;

	if( pbuf2 = Q_strchr( pbuf, '\n' ) )
	    *pbuf2 = 0;

	if( !ID_VerifyHEX( pbuf ) )
		return false;

	*value |= BloomFilter_Process( pbuf, pbuf2 - pbuf );
	return true;
}

void ID_TestCPUInfo_f( void )
{
	bloomfilter_t value = 0;

	if( ID_ProcessCPUInfo( &value ) )
		Msg( "Got %016llX\n", value );
	else
		Msg( "Could not get serial" );
}
void ID_Init( void )
{
	Cmd_AddCommand( "bloomfilter", ID_BloomFilter_f, "print bloomfilter raw value of arguments set");
	Cmd_AddCommand( "verifyhex", ID_VerifyHEX_f, "check if id source seems to be fake" );
	Cmd_AddCommand( "testcpuinfo", ID_TestCPUInfo_f, "try read cpu serial" );
}
