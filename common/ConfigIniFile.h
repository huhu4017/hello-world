/* ----------------------------------------------------------------------------------------------------------
 
	Popcap Shanghai studio

	Created by Zhang Hui in 2009-1-20
 ----------------------------------------------------------------------------------------------------------*/

#ifndef __ConfigIniFile_h__
#define __ConfigIniFile_h__

#include <cstddef>

class ConfigIniFile
{
public:
	ConfigIniFile();
	~ConfigIniFile();

	bool			IsOpen();

	unsigned int	GetStringValue( const char* szSection, const char* szKey, char* szReturnStr, unsigned int nSize );
	unsigned int	GetStringValue( const char* szSection, const char* szKey, char* szReturnStr, unsigned int nSize, const char* szDefaultValue );
	unsigned int	SetStringValue( const char* szSection, const char* szKey, const char* szKeyValue );

	unsigned int	GetIntValue( const char* szSection, const char* szKey, int& iReturnValue );
	unsigned int	GetIntValue( const char* szSection, const char* szKey, int& iReturnValue, int iDefaultValue );
	unsigned int	SetIntValue( const char* szSection, const char* szKey, int iValue );

	int				OpenFile( const char* szFileName );
	void			CloseFile();

private:
	
	unsigned int	FindSection( const char* szSection, char*& szSectionBegin, char*& szSectionEnd );
	unsigned int	FindKeyRange( const char* szKey, const char* szSectionBegin, const char* szSectionEnd, char*& szValueBegin, char*& szValueEnd );
	unsigned int	FindKeyValue( const char* szKey, const char* szSectionBegin, const char* szSectionEnd, char*& szValueBegin, char*& szValueEnd );
	char*			FindStr( const char* szCharSet, const char* szBegin, const char* szEnd );
	char*			SearchMarchStr( const char* szBegin, const char* szCharSet );

	char*			MapToContent( const char* p );
	char*			MapToShadow( const char* p );

	void			ToLower( char* szSrc, size_t len );

	char*			m_szContent;		
	char*			m_szShadow;			
	size_t			m_nSize;
	bool			m_bOpen;
	char*			m_szFileName;
	
};



#endif
