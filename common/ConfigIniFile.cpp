/* ----------------------------------------------------------------------------------------------------------
 
	Popcap Shanghai studio

	Created by Zhang Hui in 2009-1-20
 ----------------------------------------------------------------------------------------------------------*/

#include "ConfigIniFile.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

/* ----------------------------------------------------------------------------------------------------------*/
ConfigIniFile::ConfigIniFile()
{
	m_szFileName	= NULL;
	m_szContent		= NULL;
	m_szShadow		= NULL;
	m_nSize			= 0;
	m_bOpen			= false;
}

/* ----------------------------------------------------------------------------------------------------------*/
ConfigIniFile::~ConfigIniFile()
{
	CloseFile();
}

/* ----------------------------------------------------------------------------------------------------------*/
int ConfigIniFile::OpenFile( const char* szFileName )
{
	FILE	*fp;
	size_t	nLen;
	int		nRet;

	CloseFile();

	if ( 0 == szFileName )
	{
		return -1;
	}

#if defined(_WIN32)
	m_szFileName = _strdup( szFileName );
#else
	m_szFileName = strdup( szFileName );
#endif

	fp = fopen( m_szFileName, "rb" );

	if ( 0 == fp )
	{
		return -1;
	}

	nRet = fseek( fp, 0L, SEEK_END );

	if (nRet != 0)
	{
		fclose( fp );
		return -1;
	}

	nLen = (size_t) ftell( fp );
	m_szContent = (char* ) new char[ nLen + 1 ];
	m_szShadow  = (char* ) new char[ nLen + 1 ];

	if ( m_szShadow == 0  || m_szContent == 0 )
	{
		fclose( fp );
		return -1;
	}

	nRet = fseek( fp, 0L, SEEK_SET );
	if ( 0 != nRet )
	{
		fclose( fp );
		return -1;
	}

	m_nSize = fread( m_szContent, 1, nLen, fp );
	m_szContent[m_nSize] = '\0';

	memcpy( m_szShadow, m_szContent, m_nSize + 1 );
	ToLower( m_szShadow, m_nSize + 1 );

	fclose( fp );
	m_bOpen = true;

	return 0;
}

/* ----------------------------------------------------------------------------------------------------------*/
void ConfigIniFile::CloseFile()
{
	if ( m_szFileName )
	{
		free( m_szFileName );
		m_szFileName = NULL;
	}

	if ( m_szContent )
	{
		delete [] m_szContent;
		m_szContent = NULL;
	}

	if ( m_szShadow )
	{
		delete [] m_szShadow;
		m_szShadow = NULL;
	}

	m_bOpen = false;
	m_nSize = 0;
}

/* ----------------------------------------------------------------------------------------------------------*/
bool ConfigIniFile::IsOpen()
{
	return m_bOpen;
}

/* ----------------------------------------------------------------------------------------------------------*/
unsigned int ConfigIniFile::GetIntValue( const char* szSection, const char* szKey, int& iReturnValue, int iDefaultValue )
{
	if ( 0 == GetIntValue( szSection, szKey, iReturnValue ) )
	{
		iReturnValue = iDefaultValue;
		return 0;
	}
	
	return 1;
}

/* ----------------------------------------------------------------------------------------------------------*/
unsigned int ConfigIniFile::GetIntValue( const char* szSection, const char* szKey, int& iReturnValue )
{
	char szBuf[100];

	if ( 0 == GetStringValue( szSection, szKey, szBuf, 100 ) )
	{
		return 0;
	}

	iReturnValue = atol( szBuf );

	return 1;
}

/* ----------------------------------------------------------------------------------------------------------*/
unsigned int ConfigIniFile::GetStringValue( const char* szSection, const char* szKey, char* szReturnStr, unsigned int nSize, const char* szDefaultValue )
{
	unsigned int len;

	if ( nSize == 0 )
	{
		return 0;
	}

	len = GetStringValue( szSection, szKey, szReturnStr, nSize );

	if ( 0 == len )
	{
		strncpy( szReturnStr, szDefaultValue, nSize - 1 );
		szReturnStr[ nSize - 1 ] = '\0';
		return strlen( szReturnStr );
	}

	return len;
}

/* ----------------------------------------------------------------------------------------------------------*/
unsigned int ConfigIniFile::GetStringValue( const char* szSection, const char* szKey, char* szReturnStr, unsigned int nSize )
{
	char* szSectionBegin;char* szSectionEnd;
	char* szValueBegin;char* szValueEnd;
	unsigned int dwCount;

	if ( false == IsOpen() )
	{
		return 0;
	}

	if ( 0 == szSection || 0 == szKey || 0 == szReturnStr )
	{
		return 0;
	}
	
	if ( 0 == nSize )
	{
		return 0;
	}
	
	if ( false == FindSection( szSection, szSectionBegin, szSectionEnd ) )
	{
		return 0;
	}

	/* Not find */
	if ( 0 == FindKeyValue( szKey, szSectionBegin, szSectionEnd, szValueBegin, szValueEnd ) )
	{
		return 0;
	}
	
	dwCount = 0;

	for( ; szValueBegin < szValueEnd && dwCount < (nSize - 1); ++szValueBegin, ++dwCount )
	{
		szReturnStr[ dwCount ] = *szValueBegin;
	}

	szReturnStr[ dwCount ] = '\0';

	/* If end char is multi-char */
	if ( ( dwCount == nSize-1 ) && (( unsigned char )(szReturnStr[ dwCount-1 ]) > 0x7f))
	{
		szReturnStr[dwCount-1] = '\0';
		dwCount --;
	}

	return dwCount;
}

/* ----------------------------------------------------------------------------------------------------------*/
unsigned int ConfigIniFile::SetIntValue( const char* szSection, const char* szKey, int iValue )
{
	char szBuf[100];

	sprintf( szBuf, "%d", iValue );

	return SetStringValue( szSection, szKey, szBuf );
}

/* ----------------------------------------------------------------------------------------------------------*/
unsigned int ConfigIniFile::SetStringValue( const char* szSection, const char* szKey, const char* szKeyValue )
{
	char* szSectionBegin = NULL;
	char* szSectionEnd = NULL;
	char* szKeyBegin = NULL;
	char* szKeyEnd = NULL;
	char* szContent = NULL;
	char* szShadow = NULL;
	char* szBuf = NULL;
	size_t len = 0;

	if ( false == IsOpen() )
	{
		return 0;
	}

	if ( 0 == szSection || 0 == szKey || 0 == szKeyValue )
	{
		return 0;
	}

	if ( false == FindKeyRange( szKey, szSectionBegin, szSectionEnd, szKeyBegin, szKeyEnd ) )
	{
		szKeyBegin = szSectionBegin;
		szKeyEnd = szSectionEnd;
	}

	len = strlen( szKey ) + strlen( szKeyValue ) + 4;
	szBuf = (char* )new char [ len + 1 ];
	szContent = (char* )new char [ m_nSize - (szKeyEnd - szKeyBegin) + len + 1 ];
	szShadow = (char* )new char [ m_nSize - (szKeyEnd - szKeyBegin) + len + 1 ];

	if ( 0 == szBuf || 0 == szContent || 0 == szShadow )
	{
		if ( szBuf )
		{
			delete [] szBuf;
		}

		if ( szContent )
		{
			delete [] szContent;
		}

		if ( szShadow )
		{
			delete [] szShadow;
		}
	}

	memset( szBuf, 0, len + 1 );
	memset( szContent, 0, len + 1 );
	memset( szShadow, 0, len + 1 );

	memcpy( (void* )szContent, (void* )m_szContent, (size_t)( szKeyBegin - m_szContent ) );
	memcpy( (void* )( szContent + (szKeyBegin - m_szContent) ), (void* )szBuf, len );
	memcpy( (void* )( szContent + (szKeyBegin - m_szContent) + len ), (void* )szKeyEnd, m_nSize - ( szKeyEnd - m_szContent ) + 1 );

	delete [] szBuf;
	delete [] m_szContent;
	delete [] m_szShadow;

	m_nSize = m_nSize - ( szKeyEnd - szKeyBegin ) + len;

	m_szContent = szContent;

	m_szShadow = szShadow;
	memcpy( m_szShadow, m_szContent, m_nSize + 1 );
	ToLower( m_szShadow, m_nSize + 1 );

	/* Update to file */
	FILE* fp;

	if ( 0 == m_szFileName )
	{
		return 0;
	}

	fp = fopen( m_szFileName, "wb" );

	if ( 0 == fp )
	{
		return 0;
	}

	len = fwrite( m_szContent, 1, m_nSize, fp );

	if ( (size_t)len != m_nSize )
	{
		/* write faised */
		fclose( fp );
		return 0;
	}

	fclose( fp );

	return 1;
}

/* ----------------------------------------------------------------------------------------------------------*/
unsigned int ConfigIniFile::FindSection( const char* szSection, char*& szSectionBegin, char*& szSectionEnd )
{
	char*			szLowerSection;
	char*			szSectionBeginOnShadow;
	unsigned int	bIsFirstValidCharOnLine;
	char*			pR;

	if ( 0 == szSection )
	{
		return 0;
	}

	if ( 0 == m_szContent || 0 == m_szShadow )
	{
		return 0;
	}

	szLowerSection = new char [ strlen( szSection ) + 2 + 1 ];

	sprintf( szLowerSection, "[%s]", szSection );
	ToLower( szLowerSection, strlen( szLowerSection ) );

	szSectionBeginOnShadow = FindStr( szLowerSection, m_szShadow, m_szShadow + m_nSize );

	if ( 0 == szSectionBeginOnShadow )
	{
		delete [] szLowerSection;
		return 0;
	}
	
	szSectionBegin = MapToContent( szSectionBeginOnShadow ) + strlen( szLowerSection );
	
	/* Move SectionBegin to next line
	   and filter end char */
	for ( ; szSectionBegin < (m_szContent + m_nSize); ++szSectionBegin )
	{
		if ( *szSectionBegin == '\r' || *szSectionBegin == '\n' )
			break;
	}

	for( ; szSectionBegin < (m_szContent + m_nSize); ++szSectionBegin )
	{
		if ( *szSectionBegin != '\r' && *szSectionBegin != '\n' )
		{
			break;
		}
	}

	delete [] szLowerSection;
	
	bIsFirstValidCharOnLine = true;
	pR = szSectionBegin;
	for( ; pR < (m_szContent + m_nSize + 1); ++pR )
	{
		if ( bIsFirstValidCharOnLine && *pR == '[' )
		{
			break;
		}

		if ( *pR == '\0' )
		{
			break;
		}

		if ( *pR == '\r' || *pR == '\n' )
		{
			bIsFirstValidCharOnLine = true;
		}
		else if ( *pR != ' ' && *pR != '\t' )
		{
			bIsFirstValidCharOnLine = false;
		}
	}

	szSectionEnd = pR;

	return 1;
}

/* ----------------------------------------------------------------------------------------------------------*/
unsigned int ConfigIniFile::FindKeyRange( const char* szKey, const char* szSectionBegin, const char* szSectionEnd, char*& szKeyBegin, char*& szKeyEnd )
{
	char* szLowerKey;

	if ( 0 == szKey || 0 == szSectionBegin || 0 == szSectionEnd )
	{
		return 0;
	}

	if ( szSectionBegin >= szSectionEnd )
	{
		return 0;
	}
	
	if ( 0 == m_szContent || 0 == m_szShadow )
	{
		return 0;
	}

#if defined(_WIN32)
	szLowerKey = _strdup( szKey );
#else
	szLowerKey = strdup( szKey );
#endif
	ToLower( szLowerKey, strlen( szLowerKey ) );

	char* szKeyBeginOnShadow;
	szKeyBeginOnShadow = FindStr( szLowerKey, MapToShadow( szSectionBegin ), MapToShadow( szSectionEnd ) );

	if ( 0 == szKeyBeginOnShadow )
	{
		free( szLowerKey );
		return 0;
	}

	free( szLowerKey );

	szKeyBegin = MapToContent( szKeyBeginOnShadow );
	szKeyEnd = szKeyBegin + strlen( szKey );

	for ( ; szKeyEnd < szSectionEnd; ++szKeyEnd )
	{
		if ( *szKeyEnd != ' ' && *szKeyEnd != '\t' )
		{
			break;
		}
	}
	

	if ( *szKeyEnd != '=' )
	{
		char* szSearchBegin;
		szSearchBegin = szKeyEnd;

		return FindKeyRange( szKey, szSearchBegin, szSectionEnd, szKeyBegin, szKeyEnd );
	}

	for ( ++szKeyEnd; szKeyEnd < szSectionEnd; ++szKeyEnd )
	{
		if ( *szKeyEnd == '\r' || *szKeyEnd == '\n' )
		{
			break;
		}
	}

	for ( ; szKeyEnd < szSectionEnd; ++szKeyEnd )
	{
		if ( *szKeyEnd != '\r' && *szKeyEnd != '\n' )
		{
			break;
		}
	}

	if ( szKeyEnd > szKeyBegin )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/* ----------------------------------------------------------------------------------------------------------*/
unsigned int ConfigIniFile::FindKeyValue(const char* szKey, const char* szSectionBegin, const char* szSectionEnd, char*& szValueBegin, char*& szValueEnd )
{
	char* szLowerKey;

	if ( 0 == szKey || 0 == szSectionBegin || 0 == szSectionEnd )
	{
		return 0;
	}

	if ( szSectionBegin >= szSectionEnd )
	{
		return 0;
	}

	if ( 0 == m_szContent || 0 == m_szShadow )
	{
		return 0;
	}

#if defined(_WIN32)
	szLowerKey = _strdup( szKey );
#else
	szLowerKey = strdup( szKey );
#endif
	ToLower( szLowerKey, strlen( szLowerKey ) );

	char* szKeyBeginOnShadow;

	szKeyBeginOnShadow = FindStr( szLowerKey, MapToShadow( szSectionBegin ), MapToShadow( szSectionEnd ) );
	if ( 0 == szKeyBeginOnShadow )
	{
		free( szLowerKey );
		return 0;
	}

	free( szLowerKey );

	szValueBegin = MapToContent( szKeyBeginOnShadow ) + strlen( szKey );

	for( ; szValueBegin < szSectionEnd; ++szValueBegin )
	{
		if ( *szValueBegin != ' ' && *szValueBegin != '\t' )
		{
			break;
		}
	}

	if ( *szValueBegin != '=' )
	{
		char* szSearchBegin;

		szSearchBegin = szValueBegin;

		return FindKeyValue( szKey, szSearchBegin, szSectionEnd, szValueBegin, szValueEnd );
	}

	for( szValueBegin++ ; szValueBegin < szSectionEnd; ++szValueBegin )
	{
		if ((*szValueBegin == '\r') || (*szValueBegin == '\n')
			|| ((*szValueBegin != '\t') && (*szValueBegin != ' ')))
		{
			break;
		}
	}
	
	szValueEnd = szValueBegin;
	for( ; szValueEnd < szSectionEnd; ++szValueEnd )
	{
		if ( *szValueEnd == '\t' || *szValueEnd == '\r' || *szValueEnd == '\n' )
		{
			break;
		}
	}

	if ( szValueEnd > szValueBegin )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/* ----------------------------------------------------------------------------------------------------------*/
char* ConfigIniFile::FindStr( const char* szCharSet, const char* szBegin, const char* szEnd )
{
	char*	pFind;

	if ( 0 == szCharSet || 0 == szBegin || 0 == szEnd )
	{
		return 0;
	}

	if ( szBegin >= szEnd )
	{
		return 0;
	}

	pFind = SearchMarchStr( szBegin, szCharSet );

	if ( 0 == pFind || ( ( pFind + strlen(szCharSet) ) > szEnd ) )
	{
		return 0;
	}
	else
	{
		return pFind;
	}
}

/* ----------------------------------------------------------------------------------------------------------*/
char* ConfigIniFile::SearchMarchStr( const char* szBegin, const char* szCharSet )
{
	const char *pFind = NULL;
	const char *pFind1;
	const char *pTempBegin = szBegin;

	for(;;)
	{
		pFind = strstr( pTempBegin, szCharSet );
		if ( NULL == pFind )
		{
			return NULL;
		}

		if ( pTempBegin < pFind )
		{
			pFind1 = pFind - 1;

			if ( ' ' != *pFind1 && '\t' != *pFind1 && '\r' != *pFind1 && '\n' != *pFind1 )
			{
				pTempBegin = pFind + strlen( szCharSet );
				continue;
			}
		}

		pFind1 = pFind + strlen( szCharSet );

		if ( ' ' == *pFind1 || '=' == *pFind1 || '\t' == *pFind1 || '\r' == *pFind1|| '\n' == *pFind1 )
		{
			return const_cast<char*>(pFind);
		}
		pTempBegin = pFind + strlen( szCharSet );
	}

	return NULL;
}

/* ----------------------------------------------------------------------------------------------------------*/
char* ConfigIniFile::MapToContent( const char* p )
{
	return ( m_szContent + ( p - m_szShadow ) );
}

/* ----------------------------------------------------------------------------------------------------------*/
char* ConfigIniFile::MapToShadow( const char* p )
{
	return ( m_szShadow + ( p - m_szContent ) );
}

/* ----------------------------------------------------------------------------------------------------------*/
void ConfigIniFile::ToLower( char* szSrc, size_t len )
{
	char			cb;
	size_t			i;

	if ( NULL == szSrc )
	{
		return;
	}
	
	for( i = 0; i < len; ++i )
	{
		cb = *( szSrc + i );
		if ( cb >= 'A' && cb <= 'Z' )
		{
			*( szSrc + i ) = ( cb + 32 );
		}
	}
}


/* ----------------------------------------------------------------------------------------------------------
   End of file */
