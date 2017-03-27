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

/*
==========================================================

simple 64-bit one-hash-func bloom filter
should be enough to determine if device exist in identifier

==========================================================
*/
typedef integer64 bloomfilter_t;

#define bf64_mask ((1U<<6)-1)

bloomfilter_t BloomFiler_Process( const char *buffer, int size )
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

bloomfilter_t BloomFiler_ProcessStr( const char *buffer )
{
	return BloomFiler_Process( buffer, Q_strlen( buffer ) );
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
	bloomfilter_t value = BloomFiler_ProcessStr( str );

	return (filter & value) == value;
}

void BloomFilter_BloomFilter_f( void )
{
	bloomfilter_t value = 0;
	int i;

	for( i = 1; i < Cmd_Argc(); i++ )
		value |= BloomFiler_ProcessStr( Cmd_Argv( i ) );

	Msg( "%d %llX\n", BloomFilter_Weight( value ), value );

	// test
	// for( i = 1; i < Cmd_Argc(); i++ )
	//	Msg( "%s: %d\n", Cmd_Argv( i ), BloomFilter_ContainsString( value, Cmd_Argv( i ) ) );
}

void ID_Init( void )
{
	Cmd_AddCommand( "bloomfilter", BloomFilter_BloomFilter_f, "print bloomfilter raw value of arguments set");
}
