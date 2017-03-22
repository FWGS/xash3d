/*
net_huff.c - Huffman compression routines for network bitstream
Copyright (C) 2007 Uncle Mike

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

#define VALUE(a)			((int   )(size_t)(a))
#define NODE(a)			((void *)(a))

#define NODE_START			NODE(  1)
#define NODE_NONE			NODE(256)
#define NODE_NEXT			NODE(257)

#define NOT_REFERENCED		256

#define HUFF_TREE_SIZE		7175
typedef byte			*tree_t[HUFF_TREE_SIZE];

//
// pre-defined frequency counts for all bytes [0..255]
//
static const int huff_tree[256] =
{
0x3D1CB, 0x0A0E9, 0x01894, 0x01BC2, 0x00E92, 0x00EA6, 0x017DE, 0x05AF3,
0x08225, 0x01B26, 0x01E9E, 0x025F2, 0x02429, 0x0436B, 0x00F6D, 0x006F2,
0x02060, 0x00644, 0x00636, 0x0067F, 0x0044C, 0x004BD, 0x004D6, 0x0046E,
0x006D5, 0x00423, 0x004DE, 0x0047D, 0x004F9, 0x01186, 0x00AF5, 0x00D90,
0x0553B, 0x00487, 0x00686, 0x0042A, 0x00413, 0x003F4, 0x0041D, 0x0042E,
0x006BE, 0x00378, 0x0049C, 0x00352, 0x003C0, 0x0030C, 0x006D8, 0x00CE0,
0x02986, 0x011A2, 0x016F9, 0x00A7D, 0x0122A, 0x00EFD, 0x0082D, 0x0074B,
0x00A18, 0x0079D, 0x007B4, 0x003AC, 0x0046E, 0x006FC, 0x00686, 0x004B6,
0x01657, 0x017F0, 0x01C36, 0x019FE, 0x00E7E, 0x00ED3, 0x005D4, 0x005F4,
0x008A7, 0x00474, 0x0054B, 0x003CB, 0x00884, 0x004E0, 0x00530, 0x004AB,
0x006EA, 0x00436, 0x004F0, 0x004F2, 0x00490, 0x003C5, 0x00483, 0x004A2,
0x00543, 0x004CC, 0x005F9, 0x00640, 0x00A39, 0x00800, 0x009F2, 0x00CCB,
0x0096A, 0x00E01, 0x009C8, 0x00AF0, 0x00A73, 0x01802, 0x00E4F, 0x00B18,
0x037AD, 0x00C5C, 0x008AD, 0x00697, 0x00C88, 0x00AB3, 0x00DB8, 0x012BC,
0x00FFB, 0x00DBB, 0x014A8, 0x00FB0, 0x01F01, 0x0178F, 0x014F0, 0x00F54,
0x0131C, 0x00E9F, 0x011D6, 0x012C7, 0x016DC, 0x01900, 0x01851, 0x02063,
0x05ACB, 0x01E9E, 0x01BA1, 0x022E7, 0x0153D, 0x01183, 0x00E39, 0x01488,
0x014C0, 0x014D0, 0x014FA, 0x00DA4, 0x0099A, 0x0069E, 0x0071D, 0x00849,
0x0077C, 0x0047D, 0x005EC, 0x00557, 0x004D4, 0x00405, 0x004EA, 0x00450,
0x004DD, 0x003EE, 0x0047D, 0x00401, 0x004D9, 0x003B8, 0x00507, 0x003E5,
0x006B1, 0x003F1, 0x004A3, 0x0036F, 0x0044B, 0x003A1, 0x00436, 0x003B7,
0x00678, 0x003A2, 0x00481, 0x00406, 0x004EE, 0x00426, 0x004BE, 0x00424,
0x00655, 0x003A2, 0x00452, 0x00390, 0x0040A, 0x0037C, 0x00486, 0x003DE,
0x00497, 0x00352, 0x00461, 0x00387, 0x0043F, 0x00398, 0x00478, 0x00420,
0x00D86, 0x008C0, 0x0112D, 0x02F68, 0x01E4E, 0x00541, 0x0051B, 0x00CCE,
0x0079E, 0x00376, 0x003FF, 0x00458, 0x00435, 0x00412, 0x00425, 0x0042F,
0x005CC, 0x003E9, 0x00448, 0x00393, 0x0041C, 0x003E3, 0x0042E, 0x0036C,
0x00457, 0x00353, 0x00423, 0x00325, 0x00458, 0x0039B, 0x0044F, 0x00331,
0x0076B, 0x00750, 0x003D0, 0x00349, 0x00467, 0x003BC, 0x00487, 0x003B6,
0x01E6F, 0x003BA, 0x00509, 0x003A5, 0x00467, 0x00C87, 0x003FC, 0x0039F,
0x0054B, 0x00300, 0x00410, 0x002E9, 0x003B8, 0x00325, 0x00431, 0x002E4,
0x003F5, 0x00325, 0x003F0, 0x0031C, 0x003E4, 0x00421, 0x02CC1, 0x034C0
};

// static Huffman tree
static tree_t	huffTree;

// received from MSG_* code
static int	huffBitPos;
static qboolean	huffInit = false;

/*
=======================================================================================

  HUFFMAN TREE CONSTRUCTION

=======================================================================================
*/
/*
============
Huff_PrepareTree
============
*/
_inline void Huff_PrepareTree( tree_t tree )
{
	byte	**node;
	
	Q_memset( tree, 0, sizeof( tree_t ));
	
	// create first node
	node = &tree[263];
	VALUE( tree[0]++ );

	node[7] = NODE_NONE;
	tree[2] = (byte*)node;
	tree[3] = (byte*)node;
	tree[4] = (byte*)node;
	tree[261] = (byte*)node;
}

/*
============
Huff_GetNode
============
*/
_inline void **Huff_GetNode( byte **tree )
{
	void	**node;
	int	value;

	node = (void**)tree[262];
	if( !node )
	{
		value = VALUE( tree[1]++ );
		node = (void**)&tree[value + 6407];
		return node;
	}

	tree[262] = node[0];
	return node;
}

/*
============
Huff_Swap
============
*/
_inline void Huff_Swap( void **tree1, void **tree2, void **tree3 )
{
	void **a, **b;

	a = tree2[2];
	if( a )
	{
		if( a[0] == tree2 )
			a[0] = tree3;
		else a[1] = tree3;
	}
	else tree1[2] = tree3;
	b = tree3[2];

	if( b )
	{
		if( b[0] == tree3 )
		{
			b[0] = tree2;
			tree2[2] = b;
			tree3[2] = a;
			return;
		}

		b[1] = tree2;
		tree2[2] = b;
		tree3[2] = a;
		return;
	}

	tree1[2] = tree2;
	tree2[2] = NULL;
	tree3[2] = a;
}

/*
============
Huff_SwapTrees
============
*/
_inline void Huff_SwapTrees( void **tree1, void **tree2 )
{
	void **temp;

	temp = tree1[3];
	tree1[3] = tree2[3];
	tree2[3] = temp;

	temp = tree1[4];
	tree1[4] = tree2[4];
	tree2[4] = temp;

	if( tree1[3] == tree1 )
		tree1[3] = tree2;

	if( tree2[3] == tree2 )
		tree2[3] = tree1;

	temp = tree1[3];
	if( temp ) temp[4] = tree1;

	temp = tree2[3];
	if( temp ) temp[4] = tree2;

	temp = tree1[4];
	if( temp ) temp[3] = tree1;

	temp = tree2[4];
	if( temp ) temp[3] = tree2;
}

/*
============
Huff_DeleteNode
============
*/
_inline void Huff_DeleteNode( void **tree1, void **tree2 )
{
	tree2[0] = tree1[262];
	tree1[262] = tree2;
}

/*
============
Huff_IncrementFreq_r
============
*/
static void Huff_IncrementFreq_r( byte **tree1, byte **tree2 )
{
	void **a, **b;

	if( !tree2 ) return;

	a = (void**)tree2[3];
	if( a )
	{
		a = a[6];
		if( a == (void**)tree2[6] )
		{
			b = (void**)tree2[5];
			if( b[0] != tree2[2] )
				Huff_Swap( (void**)tree1, (void**)b[0], (void**)tree2 );
			Huff_SwapTrees( (void**)b[0], (void**)tree2 );
		}
	}

	a = (void**)tree2[4];
	if( a && a[6] == tree2[6] )
	{
		b = (void**)tree2[5];
		b[0] = a;
	}
	else
	{
		a = (void**)tree2[5];
		a[0] = 0;
		Huff_DeleteNode( (void**)tree1, (void**)tree2[5] );
	}

	
	VALUE( tree2[6]++ );
	a = (void**)tree2[3];
	if( a && a[6] == tree2[6] )
	{
		tree2[5] = a[5];
	}
	else
	{
		a = Huff_GetNode( tree1 );
		tree2[5] = (byte*)a;
		a[0] = tree2;
	}

	if( tree2[2] )
	{
		Huff_IncrementFreq_r( (byte**)tree1, (byte**)tree2[2] );
	
		if( tree2[4] == tree2[2] )
		{
			Huff_SwapTrees( (void**)tree2, (void**)tree2[2] );
			a = (void**)tree2[5];

			if( a[0] == tree2 ) a[0] = tree2[2];
		}
	}
}

/*
============
Huff_AddReference

Insert 'ch' into the tree or increment it's frequency
============
*/
static void Huff_AddReference( byte **tree, int ch )
{
	void **a, **b, **c, **d;
	int value;

	ch &= 255;
	if( tree[ch + 5] )
	{
		Huff_IncrementFreq_r( tree, (byte**)tree[ch + 5] );
		return; // already added
	}

	value = VALUE( tree[0]++ );
	b = (void**)&tree[value * 8 + 263];

	value = VALUE( tree[0]++ );
	a = (void**)&tree[value * 8 + 263];

	a[7] = NODE_NEXT;
	a[6] = NODE_START;
	d = (void**)tree[3];
	a[3] = d[3];
	if( a[3] )
	{
		d = a[3];
		d[4] = a;
		d = a[3];
		if( d[6] == NODE_START )
		{
			a[5] = d[5];
		}
		else
		{
			d = Huff_GetNode( tree );
			a[5] = d;
			d[0] = a;
		}
	}
	else
	{
		d = Huff_GetNode( tree );
		a[5] = d;
		d[0] = a;

	}
	
	d = (void**)tree[3];
	d[3] = a;
	a[4] = tree[3];
	b[7] = NODE( (size_t)ch );
	b[6] = NODE_START;
	d = (void**)tree[3];
	b[3] = d[3];
	if( b[3] )
	{
		d = b[3];
		d[4] = b;
		if( d[6] == NODE_START )
		{
			b[5] = d[5];
		}
		else
		{
			d = Huff_GetNode( tree );
			b[5] = d;
			d[0] = a;
		}
	}
	else
	{
		d = Huff_GetNode( tree );
		b[5] = d;
		d[0] = b;
	}

	d = (void**)tree[3];
	d[3] = b;
	b[4] = tree[3];
	b[1] = NULL;
	b[0] = NULL;
	d = (void**)tree[3];
	c = d[2];
	if( c )
	{
		if( c[0] == tree[3] )
			c[0] = a;
		else c[1] = a;
	}
	else tree[2] =(byte*) a;

	a[1] = b;
	d = (void**)tree[3];
	a[0] = d;
	a[2] = d[2];
	b[2] = a;
	d = (void**)tree[3];
	d[2] = a;
	tree[ch + 5] = (byte*)b;

	Huff_IncrementFreq_r( tree, a[2] );
}

/*
=======================================================================================

  BITSTREAM I/O

=======================================================================================
*/
/*
============
Huff_EmitBit

Put one bit into buffer
============
*/
_inline void Huff_EmitBit( int bit, byte *buffer )
{
	if(!(huffBitPos & 7)) buffer[huffBitPos >> 3] = 0;
	buffer[huffBitPos >> 3] |= bit << (huffBitPos & 7);
	huffBitPos++;
}

/*
============
Huff_GetBit

Read one bit from buffer
============
*/
_inline int Huff_GetBit( byte *buffer )
{
	int bit = buffer[huffBitPos >> 3] >> (huffBitPos & 7);
	huffBitPos++;
	return (bit & 1);
}

/*
============
Huff_EmitPathToByte
============
*/
_inline void Huff_EmitPathToByte( void **tree, void **subtree, byte *buffer )
{
	if( tree[2] ) Huff_EmitPathToByte( tree[2], tree, buffer );
	if( !subtree ) return;

	// emit tree walking control bits
	if( tree[1] == subtree )
		Huff_EmitBit( 1, buffer );
	else Huff_EmitBit( 0, buffer );
}

/*
============
Huff_GetByteFromTree

Get one byte using dynamic or static tree
============
*/
_inline int Huff_GetByteFromTree( void **tree, byte *buffer )
{
	if( !tree ) return 0;

	// walk through the tree until we get a value
	while( tree[7] == NODE_NEXT )
	{
		if( !Huff_GetBit( buffer ))
			tree = tree[0];
		else tree = tree[1];
		if( !tree ) return 0;
	}
	return VALUE( tree[7] );
}

/*
============
Huff_EmitByteDynamic

Emit one byte using dynamic tree
============
*/
static void Huff_EmitByteDynamic( void **tree, int value, byte *buffer )
{
	void	**subtree;
	int	i;

	// if byte was already referenced, emit path to it
	subtree = tree[value + 5];
	if( subtree )
	{
		if( subtree[2] )
			Huff_EmitPathToByte( subtree[2], subtree, buffer );
		return;
	}

	// byte was not referenced, just emit 8 bits
	Huff_EmitByteDynamic( tree, NOT_REFERENCED, buffer );

	for( i = 7; i >= 0; i-- )
	{
		Huff_EmitBit( (value >> i) & 1, buffer );
	}

}

/*
=======================================================================================

  PUBLIC INTERFACE

=======================================================================================
*/
/*
============
Huff_CompressPacket

Compress message using dynamic Huffman tree,
beginning from specified offset
============
*/
void Huff_CompressPacket( sizebuf_t *msg, int offset )
{
	tree_t	tree;
	byte	buffer[NET_MAX_PAYLOAD];
	byte	*data;
	int	outLen;
	int	i, inLen;

	data = BF_GetData( msg ) + offset;
	inLen = BF_GetNumBytesWritten( msg ) - offset;	
	if( inLen <= 0 || inLen >= NET_MAX_PAYLOAD )
		return;

	Huff_PrepareTree( tree );

	buffer[0] = inLen >> 8;
	buffer[1] = inLen & 0xFF;
	huffBitPos = 16;

	for( i = 0; i < inLen; i++ )
	{
		Huff_EmitByteDynamic( (void**)tree, data[i], buffer );
		Huff_AddReference( tree, data[i] );
	}
	
	outLen = (huffBitPos >> 3) + 1;
	msg->iCurBit = (offset + outLen) << 3;
	Q_memcpy( data, buffer, outLen );
}
/*
============
Huff_CompressData

Compress generic data buffer using dynamic Huffman tree
============
*/
void Huff_CompressData( byte *data, size_t *length )
{
	tree_t	tree;
	byte	buffer[NET_MAX_PAYLOAD];
	int	outLen;
	int	i, inLen;

	inLen = *length;
	if( inLen <= 0 || inLen >= NET_MAX_PAYLOAD )
	{
		MsgDev( D_ERROR, "Huff_CompressData: overflow\n");
		return;
	}

	Huff_PrepareTree( tree );

	buffer[0] = inLen >> 8;
	buffer[1] = inLen & 0xFF;
	huffBitPos = 16;

	for( i = 0; i < inLen; i++ )
	{
		Huff_EmitByteDynamic( (void**)tree, data[i], buffer );
		Huff_AddReference( tree, data[i] );
	}

	outLen = (huffBitPos >> 3) + 1;
	*length = outLen;
	Q_memcpy( data, buffer, outLen );
}



/*
============
Huff_DecompressPacket

Decompress message using dynamic Huffman tree,
beginning from specified offset
============
*/
void Huff_DecompressPacket( sizebuf_t *msg, int offset )
{
	tree_t	tree;
	byte	buffer[NET_MAX_PAYLOAD];
	byte	*data;
	int	outLen;
	int	inLen;
	int	ch, i, j;

	data = BF_GetData( msg ) + offset;
	inLen = BF_GetMaxBytes( msg ) - offset;
	if( inLen <= 0 ) return;

	Huff_PrepareTree( tree );

	outLen = ( data[0] << 8 ) + data[1];
	huffBitPos = 16;

	if( outLen > NET_MAX_PAYLOAD - offset )
	{
		outLen = NET_MAX_PAYLOAD - offset;
		MsgDev( D_ERROR, "Huff_DecompressData: overflow\n");
	}

	for( i = 0; i < outLen; i++ )
	{
		if(( huffBitPos >> 3 ) > inLen )
		{
			buffer[i] = 0;
			break;
		}

		ch = Huff_GetByteFromTree( (void**)tree[2], data );

		if( ch == NOT_REFERENCED )
		{
			ch = 0; // just read 8 bits
			for( j = 0; j < 8; j++ )
			{
				ch <<= 1;
				ch |= Huff_GetBit( data );
			}
		}
		buffer[i] = ch;
		Huff_AddReference( tree, ch );
	}

	msg->nDataBits = ( offset + outLen ) << 3;
	Q_memcpy( data, buffer, outLen );
}

/*
============
Huff_DecompressData

Decompress generic data buffer using dynamic Huffman tree
============
*/
void Huff_DecompressData( byte *data, size_t *length )
{
	tree_t	tree;
	byte	buffer[NET_MAX_PAYLOAD];
	int	outLen;
	int	inLen = *length;
	int	ch, i, j;

	if( inLen <= 0 ) return;

	Huff_PrepareTree( tree );

	outLen = ( data[0] << 8 ) + data[1];
	huffBitPos = 16;

	if( outLen > NET_MAX_PAYLOAD )
		outLen = NET_MAX_PAYLOAD;

	for( i = 0; i < outLen; i++ )
	{
		if(( huffBitPos >> 3 ) > inLen )
		{
			buffer[i] = 0;
			break;
		}

		ch = Huff_GetByteFromTree( (void**)tree[2], data );

		if( ch == NOT_REFERENCED )
		{
			ch = 0; // just read 8 bits
			for( j = 0; j < 8; j++ )
			{
				ch <<= 1;
				ch |= Huff_GetBit( data );
			}
		}
		buffer[i] = ch;
		Huff_AddReference( tree, ch );
	}

	*length = outLen;
	Q_memcpy( data, buffer, outLen );
}

/*
============
Huff_Init
============
*/
void Huff_Init( void )
{
	int	i, j;

	if( huffInit ) return;

	// build empty tree
	Huff_PrepareTree( huffTree );

	// add all pre-defined byte references
	for( i = 0; i < 256; i++ )
		for( j = 0; j < huff_tree[i]; j++ )
			Huff_AddReference( huffTree, i );
	huffInit = true;
}
