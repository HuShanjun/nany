
#include "CAniCursorFile.h"
#include <fstream>
#include <sstream>
#include "GammaCommon/CPkgFile.h"
#include "GammaCommon/TBitSet.h"
#include "GammaCommon/TGammaStrStream.h"
#include "GammaCommon/GammaTime.h"

using namespace std;

namespace Gamma
{
	struct SBitmapInfoHead
	{
		uint32_t	nSize;
		uint32_t 	nWidth;
		uint32_t 	nHeight;
		uint16_t 	nPlanes;
		uint16_t 	nBitCount;
		uint32_t	nCompression;
		uint32_t	nSizeImage;
		uint32_t 	nXPelsPerMeter;
		uint32_t 	nYPelsPerMeter;
		uint32_t	nClrUsed;
		uint32_t	nClrImportant;
	};

	//=============================================
	// CAniCursorFile
	//=============================================
	CAniCursorFile::CAniCursorFile( const char* szFileName )
		: m_nTotalTime( 0 )
		, m_strFileName( szFileName )
	{
	}

	CAniCursorFile::~CAniCursorFile()
	{
		ClearData();
	}

	void CAniCursorFile::ClearData()
	{
		for( size_t i = 0; i < m_vecFrameDatas.size(); i++ )
		{
#ifdef _WIN32
			DestroyIcon( m_vecFrameDatas[i].m_hCursor );
#endif
		}

		memset( &m_aniHeader, 0, sizeof(m_aniHeader) );
		m_vecSequence.clear();
		m_vecFrameDurations.clear();
		m_vecFrameDatas.clear();
	}

	template<>
	void CAniCursorFile::ReadChunk<CAniCursorFile::eHeaderID_Unknown>( CBufFile& BufFile, const SChunckHeader& chunkHeader )
	{
		BufFile.SeekFromCur( chunkHeader.m_nSize );
	}

	CAniCursorFile* CAniCursorFile::GetCursor( const char* szCursorName )
	{	
		const char* pCursorKey = szCursorName;
		char szCursorKey[32];
		if ( ( (ptrdiff_t)szCursorName ) <= INVALID_16BITID )
		{	
			gammasstream( szCursorKey, ELEM_COUNT( szCursorKey ) ) << (uint32_t)((ptrdiff_t)(szCursorName));
			pCursorKey = &szCursorKey[0];
		}

		gammacstring strKey( pCursorKey, true );
		CAniCursorFile* pCursorFile = s_mapAllCursor.Find( strKey );
		if( !pCursorFile )
		{
			pCursorFile = new CAniCursorFile( pCursorKey );
			s_mapAllCursor.Insert( *pCursorFile );
			pCursorFile->Load( szCursorName );
		}

		return pCursorFile;
	}

	bool CAniCursorFile::Load( const char* szCursorFile )
	{
		if ( !szCursorFile )
			return false;	

		if ( ( (ptrdiff_t)szCursorFile ) <= INVALID_16BITID )
		{	
			ClearData();
#ifdef _WIN32
			SCursorData CursorData; 
			// MAKEINTRESOURCE  
			CursorData.m_hCursor = ::LoadCursor( NULL, szCursorFile );
			m_vecFrameDatas.push_back( CursorData );
			m_vecSequence.resize( 1, 0 );
			return true;
#else
			return false;
#endif
		}

		return GetGammaFileMgr().Load( szCursorFile, false, true, this ) != 0;
	}

	void CAniCursorFile::OnLoadedEnd( const char* szFileName, const tbyte* pBuffer, uint32_t nSize )
	{
		if( pBuffer == NULL || nSize == 0 )
			return;

		ClearData();
		CBufFile BufFile( pBuffer, nSize );

		// 非动画光标兼容
		if( strcmp( ".ani", m_strFileName.c_str() + m_strFileName.size() - 4 ) )
		{
			SCursorData CursorData;
			ReadStaticCursor( BufFile, CursorData, 0, nSize );
			m_vecFrameDatas.push_back( CursorData );
			return;
		}

		SFileInfo fileInfo;
		BufFile.Read( (char*)&fileInfo, sizeof(fileInfo) );
		if ( fileInfo.m_nRiff.m_nID != eHeaderID_RIFF
			|| fileInfo.m_nACON.m_nID != eHeaderID_ACON )
			return;

		// 不完整
		if ( fileInfo.m_nFileLen != (int32_t)nSize )
			return;

		while( !BufFile.IsEOF() )
			TryReadChunk( BufFile );
	}

	void CAniCursorFile::TryReadChunk( CBufFile& BufFile )
	{
		SChunckHeader chunkHeader;
		BufFile.Read( (char*)&chunkHeader, sizeof(chunkHeader) );
		EChunkIDType eID = (EChunkIDType)chunkHeader.m_nID.m_nID;
		CChunckReadMap::iterator it = s_mapAllCursor.m_chuckReaders.find( eID );
		if( it != s_mapAllCursor.m_chuckReaders.end() )
			(this->*( it->second ) )( BufFile, chunkHeader );
		else
			ReadChunk<eHeaderID_Unknown>( BufFile, chunkHeader );
	}

	template<>
	void CAniCursorFile::ReadChunk<CAniCursorFile::eHeaderID_LIST>( CBufFile& BufFile, const SChunckHeader& chunkHeader )
	{
		// 什么都不做
	}

	template<>
	void CAniCursorFile::ReadChunk<CAniCursorFile::eHeaderID_RIFF>( CBufFile& BufFile, const SChunckHeader& chunkHeader )
	{
		// 调过ACON
		BufFile.SeekFromCur( 4 );
	}

	template<>
	void CAniCursorFile::ReadChunk<CAniCursorFile::eHeaderID_ACON>( CBufFile& BufFile, const SChunckHeader& chunkHeader )
	{
		BufFile.SeekFromCur( -4 );
	}

	template<>
	void CAniCursorFile::ReadChunk<CAniCursorFile::eHeaderID_INFO>( CBufFile& BufFile, const SChunckHeader& chunkHeader )
	{
		BufFile.SeekFromCur( -4 );
	}

	template<>
	void CAniCursorFile::ReadChunk<CAniCursorFile::eHeaderID_INAM>( CBufFile& BufFile, const SChunckHeader& chunkHeader )
	{
		BufFile.SeekFromCur( chunkHeader.m_nSize );
	}

	template<>
	void CAniCursorFile::ReadChunk<CAniCursorFile::eHeaderID_IART>( CBufFile& BufFile, const SChunckHeader& chunkHeader )
	{
		BufFile.SeekFromCur( chunkHeader.m_nSize );
	}

	template<>
	void CAniCursorFile::ReadChunk<CAniCursorFile::eHeaderID_FRAM>( CBufFile& BufFile, const SChunckHeader& chunkHeader )
	{
		BufFile.SeekFromCur( -4 );
	}

	template<>
	void CAniCursorFile::ReadChunk<CAniCursorFile::eHeaderID_ICON>( CBufFile& BufFile, const SChunckHeader& chunkHeader )
	{
		if ( !chunkHeader.m_nSize )
			return;

		uint32_t nCurPos = BufFile.GetPos();
		BufFile.SeekFromCur( 2 );

		uint16_t nImgType = 0xff;
		BufFile.Read( (char*)&nImgType, sizeof(nImgType) );

		if ( nImgType <= 2 )	// cursor or icon
		{
			uint16_t nCursorCount = 1;
			BufFile.Read( (char*)&nCursorCount, sizeof(nCursorCount) );
			// 不允许一帧有多张图
			if ( nCursorCount > 1 )
			{
				BufFile.Seek( nCurPos );
				BufFile.SeekFromCur( chunkHeader.m_nSize );
				return;
			}

			m_vecFrameDatas.push_back( SCursorData() );
			BufFile.Seek( nCurPos );
			ReadStaticCursor( BufFile, *m_vecFrameDatas.rbegin(), nCurPos, chunkHeader.m_nSize );
		}
		else
		{
			BufFile.Seek( nCurPos );
			BufFile.SeekFromCur( chunkHeader.m_nSize );
		}	
	}

	template<>
	void CAniCursorFile::ReadChunk<CAniCursorFile::eHeaderID_ANIH>( CBufFile& BufFile, const SChunckHeader& chunkHeader )
	{
		BufFile.Read( (char*)&m_aniHeader, sizeof(m_aniHeader) );
	}

	template<>
	void CAniCursorFile::ReadChunk<CAniCursorFile::eHeaderID_RATE>( CBufFile& BufFile, const SChunckHeader& chunkHeader )
	{
		m_vecFrameDurations.resize( chunkHeader.m_nSize / sizeof(uint32_t) );
		BufFile.Read( (char*)&m_vecFrameDurations[0], chunkHeader.m_nSize );
		for ( size_t i = 0; i < m_vecFrameDurations.size(); i++ )
			m_vecFrameDurations[i] = m_vecFrameDurations[i] * MillisecondsPerJIF;
	}

	template<>
	void CAniCursorFile::ReadChunk<CAniCursorFile::eHeaderID_SEQ>( CBufFile& BufFile, const SChunckHeader& chunkHeader )
	{
		m_vecSequence.resize( chunkHeader.m_nSize / sizeof(uint32_t) );
		BufFile.Read( (char*)&m_vecSequence[0], chunkHeader.m_nSize );
	}

	void CAniCursorFile::ReadStaticCursor( CBufFile& BufFile, SCursorData& cursor, size_t nReadStartPos, size_t nDataSize )
	{
		uint8_t nWidth;
		uint8_t nHeight;
		int16_t nHotSpotX;
		int16_t nHotSpotY;
		uint16_t nReserved = 0;
		uint16_t nImgType = 0xff;
		uint16_t nCursorCount = 1;

		BufFile.Read( (char*)&nReserved, sizeof(nReserved) );
		BufFile.Read( (char*)&nImgType, sizeof(nImgType) );
		BufFile.Read( (char*)&nCursorCount, sizeof(nCursorCount) );
		BufFile.Read( (char*)&nWidth, sizeof(nWidth) );
		BufFile.Read( (char*)&nHeight, sizeof(nHeight) );
		BufFile.SeekFromCur( 2 );

		if ( nImgType == 2 )
		{
			// 光标才读取热点
			BufFile.Read( (char*)&nHotSpotX, sizeof(nHotSpotX) );
			BufFile.Read( (char*)&nHotSpotY, sizeof(nHotSpotY) );
		}
		else
		{
			// 图标热点永远在正中心
			nHotSpotX = nWidth / 2;
			nHotSpotY = nHeight / 2;
			BufFile.SeekFromCur( 4 );
		}

		BufFile.SeekFromCur( 8 );

		const SBitmapInfoHead* pInfo = (const SBitmapInfoHead*)BufFile.GetBufByCur();
		BufFile.SeekFromCur( sizeof(SBitmapInfoHead) );

		uint32_t nPixelCount = nHeight * nWidth;
		vector<uint32_t> vecColor( nPixelCount );
		uint32_t* pColor = &vecColor[0];

		if( pInfo->nBitCount == 32 )
		{
			const uint32_t* pPixel = (const uint32_t*)( pInfo + 1 );
			for( uint32_t y = 0; y < nHeight; ++y )
			{
				uint32_t* pCurLine = pColor + ( nHeight - y - 1 )*nWidth;
				for( uint32_t x = 0; x < nWidth; ++x, ++pPixel )
					pCurLine[x] = *pPixel;
			}
		}
		else if( pInfo->nBitCount == 24 )
		{
			const tbyte* pPixel = (const tbyte*)( pInfo + 1 );
			for( uint32_t y = 0; y < nHeight; ++y )
			{
				uint32_t* pCurLine = pColor + ( nHeight - y - 1 )*nWidth;
				for( uint32_t x = 0; x < nWidth; ++x, pPixel += 3 )
					pCurLine[x] = ( *(uint32_t*)pPixel )|0xff000000;
			}
		}
		else
		{
			const uint32_t* pPalette = (const uint32_t*)( pInfo + 1 );
			const uint32_t* pBitSetBuf = pPalette + ( 1LL << pInfo->nBitCount );
			const TBitSet<0x7fffffff>* pBitSet = (const TBitSet<0x7fffffff>*)( pBitSetBuf );
			for( uint32_t y = 0, n = 0; y < nHeight; ++y )
			{
				uint32_t* pCurLine = pColor + ( nHeight - y - 1 )*nWidth;
				for( uint32_t x = 0; x < nWidth; ++x, n += pInfo->nBitCount )
					pCurLine[x] = pPalette[pBitSet->GetBit( n, pInfo->nBitCount )]|0xff000000;
			}
		}

		BufFile.SeekFromCur( nPixelCount*pInfo->nBitCount/8 );
		BufFile.SeekFromCur( nPixelCount/8 );

#ifdef _WIN32
		cursor.m_hCursor = NULL;
		HDC hDC = ::GetDC( NULL );
		ICONINFO info; 
		BITMAPINFO bmi;
		uint32_t* pBit = NULL;
		ZeroMemory( &bmi.bmiHeader, sizeof(BITMAPINFOHEADER) );
		bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth		= nWidth;
		bmi.bmiHeader.biHeight		= -nHeight;
		bmi.bmiHeader.biPlanes		= 1;
		bmi.bmiHeader.biBitCount    = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		info.hbmColor = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS, (void **)&pBit, NULL, 0 );
		ReleaseDC( NULL, hDC );

		if( !info.hbmColor || !pBit )
			return;

		memcpy( pBit, pColor, nWidth*nHeight*sizeof(uint32_t) );
		const void* pMask = BufFile.GetBuf() + BufFile.GetPos() - nPixelCount/8;
		info.hbmMask = CreateBitmap( nWidth, nHeight, 1, 1, pMask );  
		info.fIcon = FALSE;    
		info.xHotspot = nHotSpotX;  
		info.yHotspot = nHotSpotY;  
		cursor.m_hCursor = CreateIconIndirect( &info );  
		DeleteObject( info.hbmColor );
		DeleteObject( info.hbmMask );
#endif
	}

	uint32_t CAniCursorFile::GetFrameCount() const
	{
		return (uint32_t)m_vecSequence.size();
	}

	uint32_t CAniCursorFile::GetFrameDuration( uint32_t nFrame ) const
	{
		if( nFrame >= m_vecSequence.size() )
			return 0;
		if( m_vecFrameDurations.empty() )
			return m_aniHeader.m_nJIFRate * MillisecondsPerJIF;
		return m_vecFrameDurations[m_vecSequence[nFrame]];
	}

	uint32_t CAniCursorFile::GetTotalTime() const
	{
		if( m_nTotalTime )
			return m_nTotalTime;
		if( m_vecSequence.empty() )
			return 0;
		if( m_vecFrameDurations.empty() )
			return 0;
		for( uint32_t i = 0; i < m_vecSequence.size(); i++ )
			m_nTotalTime += m_vecFrameDurations[m_vecSequence[i]];
		return m_nTotalTime;
	}

	void CAniCursorFile::Update()
	{
		uint32_t nTotalTime = GetTotalTime();
		if( nTotalTime == 0 )
			return SetCursor( NULL );

		uint32_t nCurTime = (uint32_t)( GetGammaTime()%nTotalTime );
		uint32_t nFrame = 0;
		while( nFrame < m_vecSequence.size() &&
			nCurTime >= m_vecFrameDurations[m_vecSequence[nFrame]] )
			nCurTime -= m_vecFrameDurations[m_vecSequence[nFrame++]];
		nFrame = nFrame%m_vecSequence.size();
		uint32_t nIndex = m_vecSequence[nFrame];
		if( nIndex >= m_vecFrameDatas.size() )
			return SetCursor( NULL );
		SetCursor( &m_vecFrameDatas[nIndex] );
	}

	void CAniCursorFile::SetCursor( const SCursorData* pCursorData )
	{
#ifdef _WIN32
		::SetCursor( pCursorData ? pCursorData->m_hCursor : NULL );
#endif
	}
	
	//=============================================
	// CAniCursorFile::CAllCursorMap
	//=============================================
	CAniCursorFile::CAllCursorMap CAniCursorFile::s_mapAllCursor;

	CAniCursorFile::CAllCursorMap::CAllCursorMap()
	{
		#define REGIST_READ_FUNC( chunkID ) \
			m_chuckReaders[ chunkID ] = (ReadFunc)&CAniCursorFile::ReadChunk<chunkID>;
		REGIST_READ_FUNC( eHeaderID_LIST );
		REGIST_READ_FUNC( eHeaderID_RIFF );
		REGIST_READ_FUNC( eHeaderID_LIST );
		REGIST_READ_FUNC( eHeaderID_ACON );
		REGIST_READ_FUNC( eHeaderID_INFO );
		REGIST_READ_FUNC( eHeaderID_FRAM );
		REGIST_READ_FUNC( eHeaderID_ICON );
		REGIST_READ_FUNC( eHeaderID_ANIH );
		REGIST_READ_FUNC( eHeaderID_RATE );
		REGIST_READ_FUNC( eHeaderID_SEQ );
		REGIST_READ_FUNC( eHeaderID_INAM );
		REGIST_READ_FUNC( eHeaderID_IART );
	}

	CAniCursorFile::CAllCursorMap::~CAllCursorMap()
	{
		while( GetFirst() )
			delete GetFirst();
		m_chuckReaders.clear();
	}

}
