// align checks
#ifdef XASH_FASTSTR

#define ALIGNOF(x) ( ( unsigned long int )(x) & ( sizeof( int ) - 1) )
#define IS_UNALIGNED(x) (ALIGNOF(x) != 0)
#define HAS_NULL(x) ( ( ( x - lomagic ) & himagic ) != 0 )

// disable address sanitizer
#if defined(__has_feature)
 #if __has_feature(address_sanitizer)
  #define NOASAN __attribute__((no_sanitize("address")))
 #endif
#endif
#ifndef NOASAN
#ifdef __GNUC__
#define NOASAN __attribute__((no_sanitize_address)) __attribute__((no_address_safety_analysis))
#else
#define NOASAN
#endif
#endif
#else
#define NOASAN
#endif
#ifndef XASH_SKIPCRTLIB
#ifdef XASH_FASTSTR

// align checks
#define ALIGNOF(x) ( ( unsigned long int )(x) & ( sizeof( int ) - 1) )
#define IS_UNALIGNED(x) (ALIGNOF(x) != 0)
#define HAS_NULL(x) ( ( ( x - lomagic ) & himagic ) != 0 )
xash_force_inline NOASAN int Q_strlen( const char *string )
{
	register const char	*pchr = string;
	register const unsigned int *plongword;

	const unsigned int himagic = 0x80808080, lomagic = 0x01010101;

	if( !string ) return 0;

#ifndef HAVE_EFFICIENT_UNALIGNED_ACCESS
	// first, count all unaligned bytes
	for( ; IS_UNALIGNED(pchr); pchr++ )
		if( *pchr == '\0' )
			return pchr - string;
#endif

	plongword = ( const unsigned int * ) pchr;

	// plongword is aligned now, read by 4 bytes
	while( true )
	{
		register unsigned int longword = *plongword++;

		// if magic check failed,
		if( HAS_NULL(longword) )
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
}
#else // XASH_SKIPCRTLIB
xash_force_inline NOASAN int Q_strlen( const char *string )
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
#ifdef XASH_FASTSTR
xash_force_inline size_t Q_strncpy_unaligned( char *dst, const char *src, size_t size )
{
	register char	*d = dst;
	register const char	*s = src;
	register size_t	n = size;

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

xash_force_inline size_t Q_strcpy_unaligned( char *dst, const char *src )
{
	register char	*d = dst;
	register const char	*s = src;

	do
	{
		if(( *d++ = *s++ ) == 0 )
			break;
	} while( true );

	return ( s - src - 1 ); // count does not include NULL
}

xash_force_inline NOASAN size_t Q_strncpy( char *dst, const char *src, size_t size )
{
	register const char	*pchr = src;
	register const unsigned int *plongword;
	register unsigned int *pdst;
	register unsigned int len;

	const unsigned int himagic = 0x80808080, lomagic = 0x01010101;

	if( !dst || !src || !size )
		return 0;

	len = 0;
#ifndef HAVE_EFFICIENT_UNALIGNED_ACCESS
	if( ALIGNOF(src) != ALIGNOF(dst) )
		return Q_strncpy_unaligned( dst, src, size - len );

	// first, copy all unaligned bytes
	for( ; IS_UNALIGNED(pchr); pchr++, len++, dst++ )
	{
		*dst = *pchr;
		if( len >= size - 1 )
		{
			*dst = '\0';
			return len + Q_strlen(pchr);
		}
		
		if( *pchr == '\0' )
			return len;
	}
#endif
	plongword = ( const unsigned int * ) pchr;
	pdst = ( unsigned int * ) dst;

	// plongword is aligned now, copy by 4 bytes

	while( true )
	{
		register unsigned int longword = *plongword++;

		// if magic check failed
		if( HAS_NULL(longword) || (size - len <= 4) )
		{
			const char *pchar = ( const char * )( plongword - 1 );
			char *pchdst = ( char * )( pdst );
			int i;


			for( i = 0; i < 4; i++ )
			{
				pchdst[i] = pchar[i];

				if( len + i >= size - 1 )
				{
					pchdst[i] = '\0';
					return len + i + Q_strlen(pchar + i);
				}
				if( pchar[i] == 0 )
					return len + i;
			}
		}

		len += sizeof( longword );
		*pdst++ = longword;
	}
	return 0;
}

xash_force_inline NOASAN size_t Q_strncat( char *dst, const char *src, size_t size )
{
	register const char	*pchr = src;
	register const unsigned int *plongword;
	register unsigned int *pdst;
	register unsigned int len;

	const unsigned int himagic = 0x80808080, lomagic = 0x01010101;

	if( !dst || !src || !size )
		return 0;

	len = Q_strlen( dst );
	dst += len;
		
#ifndef HAVE_EFFICIENT_UNALIGNED_ACCESS
	if( ALIGNOF(src) != ALIGNOF(dst) )
		return len + Q_strncpy_unaligned( dst, src, size - len );

	// first, copy all unaligned bytes
	for( pchr = src; IS_UNALIGNED(pchr); pchr++, len++, dst++ )
	{
		*dst = *pchr;
		if( len >= size )
		{
			*dst = '\0';
			return len;
		}
		
		if( *pchr == '\0' )
			return len;
	}
#endif

	plongword = ( const unsigned int * ) pchr;
	pdst = ( unsigned int * ) dst;
	// plongword is aligned now, copy by 4 bytes

	while( true )
	{
		register unsigned int longword = *plongword++;

		// if magic check failed
		if( HAS_NULL(longword) || (size - len < 4) )
		{
			const char *pchar = ( const char * )( plongword - 1 );
			char *pchdst = ( char * )( pdst );
			int i;


			for( i = 0; i < 4; i++ )
			{
				pchdst[i] = pchar[i];

				if( len + i >= size )
				{
					pchdst[i] = '\0';
					return len + i;
				}
				if( pchar[i] == 0 )
					return len + i;
			}
		}

		len += sizeof( longword );
		*pdst++ = longword;
	}
	return 0;
}

/*
 * sanity checker
xash_force_inline size_t Q_strncat( char *dst, const char *src, size_t size )
{
	char buf1[100000], buf2[100000], r1, r2;
	if( !dst || !src || !size )
		return 0;
	if( size > 99999 )
		size = 99999;
	strncpy( buf1, dst, size );
	strncpy( buf2, dst, size );
	buf1[99999] = buf2[99999] = 0;

	r1 = Q_strncat_( buf1, src, size );
	r2 = Q_strncat2( buf2, src, size );
	if( r1 != r2 )
		printf("DIFFERENT RESULT %d %d %d %d %s\n%s\n", r1, r2, aligned, counter, buf1, src);
	if( strcmp( buf1, buf2 ) )
	{
		printf("DIFFERENT DATA %s %d %d %d %4s\n", src, size, aligned, counter, last);
		printf("1: %s\n", buf1);
		printf("2: %s\n", buf2);
		
	}
	//strncpy( dst, buf1, size );
	return Q_strncat2(dst, src, size);
	
}
*/

xash_force_inline NOASAN size_t Q_strcpy( char *dst, const char *src )
{
	register const char	*pchr = src;
	register const unsigned int *plongword;
	register unsigned int *pdst;
	register unsigned int len;

	const unsigned int himagic = 0x80808080, lomagic = 0x01010101;

	if( !dst || !src )
		return 0;

	len = 0;
#ifndef HAVE_EFFICIENT_UNALIGNED_ACCESS
	if( ALIGNOF(src) != ALIGNOF(dst) )
		return Q_strcpy_unaligned( dst, src );

	// first, copy all unaligned bytes
	for( ; IS_UNALIGNED(pchr); pchr++, len++, dst++ )
	{
		*dst = *pchr;

		if( *pchr == '\0' )
			return len;
	}
#endif
	plongword = ( const unsigned int * ) pchr;
	pdst = ( unsigned int * ) dst;

	// plongword is aligned now, copy by 4 bytes

	while( true )
	{
		register unsigned int longword = *plongword++;

		// if magic check failed
		if( HAS_NULL(longword) )
		{
			const char *pchar = ( const char * )( plongword - 1 );
			char *pchdst = ( char * )( pdst );
			int i;


			for( i = 0; i < 4; i++ )
			{
				pchdst[i] = pchar[i];

				if( pchar[i] == 0 )
					return len + i;
			}
		}

		len += sizeof( longword );
		*pdst++ = longword;
	}

	return 0;
}

xash_force_inline NOASAN size_t Q_strcat( char *dst, const char *src )
{
	register const char	*pchr = src;
	register const unsigned int *plongword;
	register unsigned int *pdst;
	register unsigned int len;

	const unsigned int himagic = 0x80808080, lomagic = 0x01010101;

	if( !dst || !src )
		return 0;

	len = Q_strlen( dst );
	dst += len;

#ifndef HAVE_EFFICIENT_UNALIGNED_ACCESS
	if( ALIGNOF(src) != ALIGNOF(dst) )
		return len + Q_strcpy_unaligned( dst, src );

	// first, copy all unaligned bytes
	for( pchr = src; IS_UNALIGNED(pchr); pchr++, len++, dst++ )
	{
		*dst = *pchr;

		if( *pchr == '\0' )
			return len;
	}
#endif

	plongword = ( const unsigned int * ) pchr;
	pdst = ( unsigned int * ) dst;
	// plongword is aligned now, copy by 4 bytes

	while( true )
	{
		register unsigned int longword = *plongword++;

		// if magic check failed
		if( HAS_NULL(longword) )
		{
			const char *pchar = ( const char * )( plongword - 1 );
			char *pchdst = ( char * )( pdst );
			int i;


			for( i = 0; i < 4; i++ )
			{
				pchdst[i] = pchar[i];

				if( pchar[i] == 0 )
					return len + i;
			}
		}

		len += sizeof( longword );
		*pdst++ = longword;
	}
	return 0;
}

#else
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

xash_force_inline size_t Q_strcat( char *dst, const char *src )
{
	register char	*d = dst;
	register const char	*s = src;
	size_t		dlen;

	if( !dst || !src )
		return 0;

	// find the end of dst and adjust bytes left but don't go past end
	while( *d != '\0' ) d++;
	dlen = d - dst;

	while( *s != '\0' )
	{
		*d++ = *s++;
	}

	*d = '\0';
	return( dlen + ( s - src )); // count does not include NULL
}

xash_force_inline size_t Q_strcpy( char *dst, const char *src )
{
	register char	*d = dst;
	register const char	*s = src;

	if( !dst || !src )
		return 0;

	do
	{
		if(( *d++ = *s++ ) == 0 )
			break;
	} while( true );

	return ( s - src - 1 ); // count does not include NULL
}

#endif
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


#ifdef XASH_FASTSTR
//char *last1, *last2;
xash_force_inline int Q_strncmp_unaligned( const char *s1, const char *s2, int n )
{
	register unsigned int c1, c2;

	do {
		c1 = (unsigned int)(byte)*s1++;
		c2 = (unsigned int)(byte)*s2++;

		// strings are equal until end point
		if( !n-- ) return 0;
		if( c1 != c2 ) return c1 < c2 ? -1 : 1;


	} while( c1 );

	// strings are equal
	return 0;
}

xash_force_inline int Q_strnicmp_unaligned( const char *s1, const char *s2, int n )
{
	register unsigned int c1, c2;

	do {
		c1 = (unsigned int)(byte)*s1++;
		c2 = (unsigned int)(byte)*s2++;

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

xash_force_inline int Q_strcmp_unaligned( const char *s1, const char *s2 )
{
	register unsigned int c1, c2;

	do {
		c1 = (unsigned int)(byte)*s1++;
		c2 = (unsigned int)(byte)*s2++;

		if( c1 != c2 ) return c1 < c2 ? -1 : 1;


	} while( c1 );

	// strings are equal
	return 0;
}

xash_force_inline int Q_stricmp_unaligned( const char *s1, const char *s2 )
{
	register unsigned int c1, c2;

	do {
		c1 = (unsigned int)(byte)*s1++;
		c2 = (unsigned int)(byte)*s2++;

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

xash_force_inline NOASAN int Q_strnicmp( const char *s1, const char *s2, int n )
{
	register const unsigned int *p1, *p2;
	register int len;

	const unsigned int himagic = 0x80808080, lomagic = 0x01010101;

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

	len = 0;

#ifndef HAVE_EFFICIENT_UNALIGNED_ACCESS
	if( ALIGNOF(s1) != ALIGNOF(s2) )
		return Q_strnicmp_unaligned( s1, s2, n );

	// first, compare unaligned bytes
	{
		register unsigned int c1, c2;

		while( IS_UNALIGNED(s1) )
		{
			c1 = (unsigned int)(byte)*s1++;
			c2 = (unsigned int)(byte)*s2++;

			// strings are equal until end point
			if( len++ >= n ) return 0;
			if( c1 != c2 )
			{
				if( c1 >= 'a' && c1 <= 'z' ) c1 -= ('a' - 'A');
				if( c2 >= 'a' && c2 <= 'z' ) c2 -= ('a' - 'A');
				if( c1 != c2 ) return c1 < c2 ? -1 : 1;
			}
			// strings are equal until null terminator
			if( !c1 ) return 0;
		}
	}
#endif

	p1 = ( const unsigned int * ) s1;
	p2 = ( const unsigned int * ) s2;
	// pointers is aligned now, compare by 4 bytes

	while( true )
	{
		register unsigned int c1 = *p1, c2 = *p2;

		// if magic check failed
		if( HAS_NULL(c1) || HAS_NULL(c2) || ( n - len < 4 ) || ( c1 != c2 ) )
		{
			return Q_strnicmp_unaligned( (const char *)p1, (const char *)p2, n - len );
		}

		len += 4;
		p1++;
		p2++;
	}

	return 0;
}

xash_force_inline NOASAN int Q_strncmp( const char *s1, const char *s2, int n )
{
	register const unsigned int *p1, *p2;
	register int len;

	const unsigned int himagic = 0x80808080, lomagic = 0x01010101;

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
	
	len = 0;
#ifndef HAVE_EFFICIENT_UNALIGNED_ACCESS
	if( ALIGNOF(s1) != ALIGNOF(s2) )
		return Q_strncmp_unaligned( s1, s2, n );

	// first, compare unaligned bytes
	{
		register unsigned int c1, c2;

		while( IS_UNALIGNED(s1) )
		{
			c1 = (unsigned int)(byte)*s1++;
			c2 = (unsigned int)(byte)*s2++;
			
			// strings are equal until end point
			if( len++ >= n ) return 0;
			if( c1 != c2 ) return c1 < c2 ? -1 : 1;
			// strings are equal until null terminator
			if( !c1 ) return 0;
		}
	}
#endif
	p1 = ( const unsigned int * ) s1;
	p2 = ( const unsigned int * ) s2;
	// pointers is aligned now, compare by 4 bytes

	while( true )
	{
		register unsigned int c1 = *p1, c2 = *p2;

		// if magic check failed
		if( HAS_NULL(c1) || HAS_NULL(c2) || ( n - len < 4 ) || ( c1 != c2 ) )
		{
			return Q_strncmp_unaligned( (const char *)p1, (const char *)p2, n - len );
		}

		len += 4;
		p1++;
		p2++;
	}

	return 0;
}

xash_force_inline NOASAN int Q_stricmp( const char *s1, const char *s2 )
{
	register const unsigned int *p1, *p2;

	const unsigned int himagic = 0x80808080, lomagic = 0x01010101;

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

#ifndef HAVE_EFFICIENT_UNALIGNED_ACCESS
	if( ALIGNOF(s1) != ALIGNOF(s2) )
		return Q_stricmp_unaligned( s1, s2 );

	// first, compare unaligned bytes
	{
		register unsigned int c1, c2;

		while( IS_UNALIGNED(s1) )
		{
			c1 = (unsigned int)(byte)*s1++;
			c2 = (unsigned int)(byte)*s2++;

			if( c1 != c2 )
			{
				if( c1 >= 'a' && c1 <= 'z' ) c1 -= ('a' - 'A');
				if( c2 >= 'a' && c2 <= 'z' ) c2 -= ('a' - 'A');
				if( c1 != c2 ) return c1 < c2 ? -1 : 1;
			}
			// strings are equal until null terminator
			if( !c1 ) return 0;
		}
	}
#endif

	p1 = ( const unsigned int * ) s1;
	p2 = ( const unsigned int * ) s2;
	// pointers is aligned now, compare by 4 bytes

	while( true )
	{
		register unsigned int c1 = *p1, c2 = *p2;

		// if magic check failed
		if( HAS_NULL(c1) || HAS_NULL(c2) || ( c1 != c2 ) )
		{
			return Q_stricmp_unaligned( (const char *)p1, (const char *)p2 );
		}

		p1++;
		p2++;
	}

	return 0;
}

xash_force_inline NOASAN int Q_strcmp( const char *s1, const char *s2 )
{
	register const unsigned int *p1, *p2;

	const unsigned int himagic = 0x80808080, lomagic = 0x01010101;

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

#ifndef HAVE_EFFICIENT_UNALIGNED_ACCESS
	if( ALIGNOF(s1) != ALIGNOF(s2) )
		return Q_strcmp_unaligned( s1, s2 );

	// first, compare unaligned bytes
	{
		register unsigned int c1, c2;

		while( IS_UNALIGNED(s1) )
		{
			c1 = (unsigned int)(byte)*s1++;
			c2 = (unsigned int)(byte)*s2++;

			if( c1 != c2 ) return c1 < c2 ? -1 : 1;
			// strings are equal until null terminator
			if( !c1 ) return 0;
		}
	}
#endif
	p1 = ( const unsigned int * ) s1;
	p2 = ( const unsigned int * ) s2;
	// pointers is aligned now, compare by 4 bytes

	while( true )
	{
		register unsigned int c1 = *p1, c2 = *p2;

		// if magic check failed
		if( HAS_NULL(c1) || HAS_NULL(c2) || ( c1 != c2 ) )
		{
			return Q_strcmp_unaligned( (const char *)p1, (const char *)p2 );
		}

		p1++;
		p2++;
	}

	return 0;
}

/*
xash_force_inline int Q_strncmp( const char *s1, const char *s2, int n )
{
	int r1, r2;
	char *l1, *l2;

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

	r1 = Q_strncmp_( s1, s2, n );
	l1 = last1, l2 = last2;
	r2 = Q_strncmp_unaligned( s1, s2, n );
	if( r1 != r2 || last1 != l1 || last2 != l2 )
	{
		Msg("ERROR \"%s\" \"%s\" %p %p %d %d %d %p %p %p %p %08X %08X\n", s1, s2, s1, s2, n, r1, r2, l1, last1, l2, last2, *s1, *s2 );
		exit(-6);
	}
	return r1;
}

xash_force_inline int Q_strnicmp( const char *s1, const char *s2, int n )
{
	int r1, r2;
	char *l1, *l2;

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

	r1 = Q_strnicmp_( s1, s2, n );
	l1 = last1, l2 = last2;
	r2 = Q_strnicmp_unaligned( s1, s2, n );
	if( r1 != r2 || last1 != l1 || last2 != l2 )
	{
		Msg("ERROR \"%s\" \"%s\" %p %p %d %d %d %p %p %p %p %08X %08X\n", s1, s2, s1, s2, n, r1, r2, l1, last1, l2, last2, *s1, *s2 );
		exit(-6);
	}
	return r1;
}*/

#else
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

xash_force_inline int Q_strcmp( const char *s1, const char *s2 )
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

		if( c1 != c2 ) return c1 < c2 ? -1 : 1;

	} while( c1 );

	// strings are equal
	return 0;
}

xash_force_inline int Q_stricmp( const char *s1, const char *s2 )
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
#endif
#endif
