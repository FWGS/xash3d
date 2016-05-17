#ifndef XASH_SKIPCRTLIB
#ifdef XASH_FASTSTR
xash_force_inline int Q_strlen( const char *string )
{
	register const char	*pchr;
	register const unsigned int *plongword;

	const unsigned int himagic = 0x80808080, lomagic = 0x01010101;

	if( !string ) return 0;

	// first, count all unaligned bytes
	for( pchr = string; ( ( unsigned long int )pchr & ( sizeof( int ) - 1) ) != 0; pchr++ )
		if( *pchr == '\0' )
			return pchr - string;

	plongword = ( unsigned long int * ) pchr;

	// plongword is aligned now, read by 4 bytes
	while( true )
	{
		register unsigned int longword = *plongword++;

		// if magic check failed,
		if( ( ( longword - lomagic ) & himagic ) != 0 )
		 {
			 const char *pchar = ( const char * )( plongword - 1 );

			 if( pchar[0] == 0 )
				 return pchar - string;
			 if( pchar[1] == 0 )
				 return pchar - string + 1;
			 if( pchar[2] == 0 )
				 return pchar - string + 2;
			 if( pchar[3] == 0 )
				 return pchar - string + 3;
		 }
	}
	return 0;
#warning "fastcrtlib"
}
#else
xash_force_inline int Q_strlen( const char *string )
{
	int	len;
	const char	*p;

	if( !string ) return 0;

	len = 0;
	p = string;

	while( *p )
	{
		p++;
		len++;
	}

	return len;
}

#endif
#endif

xash_force_inline size_t Q_strncat( char *dst, const char *src, size_t size )
{
	register char	*d = dst;
	register const char	*s = src;
	register size_t	n = size;
	size_t		dlen;

	if( !dst || !src || !size )
		return 0;

	// find the end of dst and adjust bytes left but don't go past end
	while( n-- != 0 && *d != '\0' ) d++;
	dlen = d - dst;
	n = size - dlen;

	if( n == 0 ) return( dlen + Q_strlen( s ));

	while( *s != '\0' )
	{
		if( n != 1 )
		{
			*d++ = *s;
			n--;
		}
		s++;
	}

	*d = '\0';
	return( dlen + ( s - src )); // count does not include NULL
}

xash_force_inline size_t Q_strncpy( char *dst, const char *src, size_t size )
{
	register char	*d = dst;
	register const char	*s = src;
	register size_t	n = size;

	if( !dst || !src || !size )
		return 0;

	// copy as many bytes as will fit
	if( n != 0 && --n != 0 )
	{
		do
		{
			if(( *d++ = *s++ ) == 0 )
				break;
		} while( --n != 0 );
	}

	// not enough room in dst, add NULL and traverse rest of src
	if( n == 0 )
	{
		if( size != 0 )
			*d = '\0'; // NULL-terminate dst
		while( *s++ );
	}
	return ( s - src - 1 ); // count does not include NULL
}

#ifndef XASH_SKIPCRTLIB
xash_force_inline char *Q_strchr( const char *s, char c )
{
	int	len = Q_strlen( s );

	while( len-- )
	{
		if( *++s == c )
			return (char *)s;
	}
	return NULL;
}

xash_force_inline char *Q_strrchr( const char *s, char c )
{
	int	len = Q_strlen( s );

	s += len;

	while( len-- )
	{
		if( *--s == c )
			return (char *)s;
	}
	return NULL;
}

xash_force_inline int Q_strnicmp( const char *s1, const char *s2, int n )
{
	int	c1, c2;

	if( s1 == NULL )
	{
		if( s2 == NULL )
			return 0;
		else return -1;
	}
	else if( s2 == NULL )
	{
		return 1;
		  }

	do {
		c1 = *s1++;
		c2 = *s2++;

		if( !n-- ) return 0; // strings are equal until end point

		if( c1 != c2 )
		{
			if( c1 >= 'a' && c1 <= 'z' ) c1 -= ('a' - 'A');
			if( c2 >= 'a' && c2 <= 'z' ) c2 -= ('a' - 'A');
			if( c1 != c2 ) return c1 < c2 ? -1 : 1;
		}
	} while( c1 );

	// strings are equal
	return 0;
}

xash_force_inline int Q_strncmp( const char *s1, const char *s2, int n )
{
	int	c1, c2;

	if( s1 == NULL )
	{
		if( s2 == NULL )
			return 0;
		else return -1;
	}
	else if( s2 == NULL )
	{
		return 1;
	}

	do {
		c1 = *s1++;
		c2 = *s2++;

		// strings are equal until end point
		if( !n-- ) return 0;
		if( c1 != c2 ) return c1 < c2 ? -1 : 1;

	} while( c1 );

	// strings are equal
	return 0;
}
#endif
