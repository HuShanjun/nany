
#include "GammaCommon/SHA1.h"
#include "GammaCommon/GammaMath.h"
#include "GammaCommon/GammaHttp.h"
#include "GammaCommon/CGammaRand.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/TGammaStrStream.h"

namespace Gamma
{
	#define INVALID_DATA_SIZE ( (uint32_t)( INVALID_32BITID - 1 ) )

	//==============================================================================
	// 读取整数值
	//==============================================================================
	uint32_t GetUnsignedInt( const char* szBuffer, size_t& nLinePos, size_t nSize )
	{
		//跳过非数据
		while( nLinePos < nSize && 
			!IsNumber( szBuffer[nLinePos] ) && 
			!( szBuffer[nLinePos] >= 'A' && szBuffer[nLinePos] <= 'F' ) && 
			!( szBuffer[nLinePos] >= 'a' && szBuffer[nLinePos] <= 'f' ) )
		{
			if( szBuffer[nLinePos] == '\n' )
				return 0;
			nLinePos++;
		}

		uint32_t nValue = 0;
		while( nLinePos < nSize )
		{
			if( !IsHexNumber( szBuffer[nLinePos] ) )
				break;
			nValue = nValue*16 + ValueFromHexNumber( szBuffer[nLinePos++] );
		}

		return nValue;
	}

	//==============================================================================
	// CHttpRecvState::CHttpRecvState
	//==============================================================================
	CHttpRecvState::CHttpRecvState(void)
		: m_nHttpLength( INVALID_32BITID )
	{
	}

	CHttpRecvState::~CHttpRecvState(void)
	{
	}

	EHttpReadState CHttpRecvState::CheckHttpBuffer( 
		char* szBuffer, uint32_t& nBufferSize )
	{
		if( m_nHttpLength == INVALID_DATA_SIZE )
			return eHRS_NeedMore;

		uint32_t nSize = nBufferSize;
		if( m_nHttpLength == INVALID_32BITID && nSize < 4 )
			return eHRS_NeedMore;

		if( m_nHttpLength != INVALID_32BITID )
		{
			if( nSize < m_nHttpLength )
				return eHRS_NeedMore;
			nBufferSize = m_nHttpLength;
			return eHRS_Ok;
		}

		const char* szMsg = szBuffer;
		if( *(uint32_t*)szMsg != MAKE_DWORD( 'H', 'T', 'T', 'P' ) )
		{
			m_nHttpLength = INVALID_DATA_SIZE;
			return eHRS_NeedMore;
		}

		// 跳过版本号
		size_t nLinePos = 4;
		while( nLinePos < nSize && szMsg[nLinePos] != ' ' )
			nLinePos++;

		// 数据不够
		if( nLinePos == nSize )
			return eHRS_NeedMore;

		//跳过空格
		while( nLinePos < nSize && szMsg[nLinePos] == ' ')
			nLinePos++;

		// 数据不够
		if( nLinePos == nSize )
			return eHRS_NeedMore;

		// 状态代码
		if( szMsg[nLinePos] != '2' )
			return eHRS_Error;

		const char* szTransferType = NULL;
		uint32_t nContentLength = INVALID_32BITID;
		while( true )
		{
			// 数据不够
			if( nLinePos >= nSize )
				return eHRS_NeedMore;

			if( szMsg[nLinePos++] != '\n' )
				continue;

			if( szMsg[nLinePos] == '\n' || *(uint16_t*)( szMsg + nLinePos ) == MAKE_UINT16( '\r', '\n' ) )
			{
				nLinePos += szMsg[nLinePos] == '\n' ? 1 : 2;
				break;
			}

			static const char* szContentLengthTag = "Content-Length";
			static const size_t nContentLengthTagLen = strlen( szContentLengthTag );

			static const char* szTransferEncoding = "Transfer-Encoding";
			static const size_t nTransferEncodingLen = strlen( szTransferEncoding );

			if( nLinePos + nContentLengthTagLen <= nSize && 
				!memcmp( szMsg + nLinePos, szContentLengthTag, nContentLengthTagLen ) )
			{
				nLinePos += (uint32_t)nContentLengthTagLen;

				//跳过非数据
				while( nLinePos < nSize && !IsNumber( szMsg[nLinePos] ) )
					nLinePos++;

				nContentLength = 0;
				while( nLinePos < nSize && IsNumber( szMsg[nLinePos] ) )
					nContentLength = nContentLength*10 + szMsg[nLinePos++] - '0';

				// 数据不够
				if( nLinePos >= nSize )
					return eHRS_NeedMore;

				if( nContentLength == 0 )
				{
					m_nHttpLength = 0;
					nBufferSize = 0;
					return eHRS_Ok;
				}
			}
			else if( nLinePos + nTransferEncodingLen <= nSize && 
				!memcmp( szMsg + nLinePos, szTransferEncoding, nTransferEncodingLen ) )
			{
				nLinePos += nTransferEncodingLen;
				nContentLength = INVALID_DATA_SIZE;
				szTransferType = szMsg + nLinePos;
				while( *szTransferType != ':' )
					szTransferType++;
				szTransferType++;
				while( IsBlank( *szTransferType ) )
					szTransferType++;
			}
		}

		// 数据不够
		if( nLinePos >= nSize )
			return eHRS_NeedMore;

		if( nContentLength == INVALID_32BITID )
		{
			m_nHttpLength = INVALID_DATA_SIZE;
			return eHRS_NeedMore;
		}

		// Content-Length存在
		if( nContentLength < INVALID_DATA_SIZE )
		{
			m_nHttpLength = nContentLength;
			nBufferSize -= (uint32_t)nLinePos;
			memmove( &szBuffer[0], &szBuffer[nLinePos], nBufferSize );
			if( nBufferSize > m_nHttpLength )
				nBufferSize = m_nHttpLength;
			return nBufferSize == m_nHttpLength ? eHRS_Ok : eHRS_NeedMore;
		}

		static const char* szChunkedTypeTag = "chunked";
		static const size_t nChunkedTypeTagLen = strlen( szChunkedTypeTag );
		if( memcmp( szChunkedTypeTag, szTransferType, nChunkedTypeTagLen ) )
			return eHRS_Error;

		size_t nStart = nLinePos;

		// 检查数据是否完整
		while( true )
		{
			nContentLength = GetUnsignedInt( szMsg, nLinePos, nSize );
			// 数据不够
			if( szMsg[nLinePos] != '\r' )
				return eHRS_NeedMore;

			// 跳过回车
			while( nLinePos < nSize && szMsg[nLinePos] != '\n' )
				nLinePos++;
			nLinePos++;

			// 数据不够
			if( nLinePos + nContentLength >= nSize )
				return eHRS_NeedMore;

			if( nContentLength == 0 )
				break;

			nLinePos += nContentLength;

			// 跳过回车
			while( nLinePos < nSize && szMsg[nLinePos] != '\n' )
				nLinePos++;
			nLinePos++;
		}

		// 读取
		nLinePos = nStart;
		nBufferSize = 0;
		while( true )
		{
			nContentLength = GetUnsignedInt( szMsg, nLinePos, nSize );
			// 跳过回车
			while( nLinePos < nSize && szMsg[nLinePos] != '\n' )
				nLinePos++;
			nLinePos++;
			if( nContentLength == 0 )
				break;

			memmove( &szBuffer[nBufferSize], szMsg + nLinePos, nContentLength );
			nBufferSize += nContentLength;
			nLinePos += nContentLength;

			// 跳过回车
			while( nLinePos < nSize && szMsg[nLinePos] != '\n' )
				nLinePos++;
			nLinePos++;
		}

		return eHRS_Ok;
	}

	uint32_t CHttpRecvState::GetDataSize() const
	{
		if( m_nHttpLength >= INVALID_DATA_SIZE )
			return 0;
		return m_nHttpLength;
	}

	void CHttpRecvState::Reset()
	{
		m_nHttpLength = INVALID_32BITID;
	}

	//==============================================================================
	// CHttpRequestState::CHttpRequestState
	//==============================================================================
	CHttpRequestState::CHttpRequestState()
		: m_nKeepAlive( INVALID_32BITID )
		, m_szDataStart( NULL )
		, m_nDataLength( INVALID_32BITID )
		, m_bGetMethod( true )
		, m_szPageStart( NULL )
		, m_nPageLength( 0 )
		, m_szParamStart( NULL )
		, m_nParamLength( 0 )
	{
	}

	CHttpRequestState::~CHttpRequestState()
	{
	}

	Gamma::EHttpReadState CHttpRequestState::CheckHttpBuffer( 
		const char* szBuffer, uint32_t nBufferSize )
	{
		if( nBufferSize < 6 )
			return eHRS_NeedMore;
		if( !memcmp( szBuffer, "POST /", 6 ) )
			m_bGetMethod = false;
		else if( !memcmp( szBuffer, "GET /", 5 ) )
			m_bGetMethod = true;
		else
			return eHRS_Error;

		uint32_t nCurPos = m_bGetMethod ? 5 : 6;
		uint32_t nNameStart = nCurPos;

		// 得页面路径  
		while( szBuffer[nCurPos] != ' ' && nCurPos < nBufferSize )
		{
			if( szBuffer[nCurPos] == '?' )
				m_szParamStart = szBuffer + nCurPos + 1;
			nCurPos++;
		}
		if( nCurPos == nBufferSize )
			return eHRS_NeedMore;
		m_szPageStart = szBuffer + nNameStart;
		if( m_szParamStart == NULL )
		{
			m_nPageLength = nCurPos - nNameStart;
			m_szParamStart = "";
		}
		else
		{
			m_nPageLength = (uint32_t)( m_szParamStart - m_szPageStart - 1 );
			m_nParamLength = (uint32_t)( szBuffer + nCurPos - m_szParamStart );
		}

		m_nDataLength = 0;
		m_szDataStart = NULL;

		// 获取数据长度  
		while( true )
		{ 
			while( szBuffer[nCurPos] != '\n' )
			{
				if( ++nCurPos < nBufferSize )
					continue;
				return eHRS_NeedMore;
			}

			if( szBuffer[++nCurPos] == '\r' )
			{
				// 数据是否足够  
				if( nBufferSize < nCurPos + 2 + m_nDataLength )
					return eHRS_NeedMore;
				m_szDataStart = szBuffer + nCurPos + 2;
				return eHRS_Ok;
			}
			else if( nCurPos + 15 < nBufferSize &&
				!memcmp( "Content-Length:", szBuffer + nCurPos, 15 ) )
			{
				// 获取数据长度  
				nCurPos += 15;
				const char* szSizeStart = NULL;
				while( nCurPos < nBufferSize && szBuffer[nCurPos] != '\r' )
					if( IsNumber( szBuffer[nCurPos++] ) && szSizeStart == NULL )
						szSizeStart = szBuffer + nCurPos - 1;
				// 数据是否足够  
				if( nCurPos == nBufferSize )
					return eHRS_NeedMore;
				if( szBuffer[nCurPos] != '\r' )
					return eHRS_Error;
				m_nDataLength = atoi( szSizeStart );
			}
			else if( nCurPos + 11 < nBufferSize &&
				!memcmp( "Connection:", szBuffer + nCurPos, 11 ) )
			{
				// 获取类型
				nCurPos += 11;
				while( nCurPos < nBufferSize && szBuffer[nCurPos] != '\r' )
				{
					if( szBuffer[nCurPos] == 'C' || szBuffer[nCurPos] == 'c' )
						m_nKeepAlive = 0;
					nCurPos++;
				}
				// 数据是否足够  
				if( nCurPos == nBufferSize )
					return eHRS_NeedMore;
				if( szBuffer[nCurPos] != '\r' )
					return eHRS_Error;
			}
			else if( nCurPos + 11 < nBufferSize &&
				!memcmp( "Keep-Alive:", szBuffer + nCurPos, 11 ) )
			{
				// 获取数据长度  
				nCurPos += 11;
				const char* szTimeoutStart = NULL;
				while( nCurPos < nBufferSize && szBuffer[nCurPos] != '\r' )
					if( IsNumber( szBuffer[nCurPos++] ) && szTimeoutStart == NULL )
						szTimeoutStart = szBuffer + nCurPos - 1;
				// 数据是否足够  
				if( nCurPos == nBufferSize )
					return eHRS_NeedMore;
				if( szBuffer[nCurPos] != '\r' )
					return eHRS_Error;
				m_nKeepAlive = atoi( szTimeoutStart );
			}
		}

		return eHRS_Error;
	}

	void CHttpRequestState::Reset()
	{
		m_bGetMethod = true;
		m_szDataStart = NULL;
		m_nDataLength = 0;
		m_szPageStart = NULL;
		m_nPageLength = 0;
		m_szParamStart = NULL;
		m_nPageLength = 0;
	}

	//==============================================================================
	// HTTP帮助函数
	//==============================================================================
	SUrlInfo GetHostAndPortFromUrl( const char* szUrl )
	{
		SUrlInfo Info = { 0, 0, 0 };
		uint32_t nHeadLen = 0;
		if( memcmp( szUrl, "http://", 7 ) == 0 )
			nHeadLen = 7;
		else if( memcmp( szUrl, "https://", 8 ) == 0 )
			nHeadLen = 8;

		if( nHeadLen == 0 )
			return Info;

		Info.bHttps = nHeadLen == 8;
		Info.nPort = Info.bHttps ? 443 : 80;
		Info.szHost = szUrl + nHeadLen;
		while( Info.szHost[Info.nHostLen] != '/' && 
			Info.szHost[Info.nHostLen] != ':' )
			Info.nHostLen++;
		if( Info.szHost[Info.nHostLen] == ':' )
			Info.nPort = (uint16_t)GammaA2I( Info.szHost + Info.nHostLen + 1 );
		return Info;
	}
	
	uint32_t MakeHttpRequest( char* szBuffer, uint32_t nBufferSize, 
		bool bPost, const char* szUrl, const void* pData, uint32_t nDataSize )
	{
		uint32_t nUrlLen = (uint32_t)strlen( szUrl );
		if( nBufferSize <= nUrlLen + nDataSize + HTTP_REQUEST_HEAD_SIZE )
			return 0;
		SUrlInfo Info = GetHostAndPortFromUrl( szUrl );
		const char* szMeth = bPost ? "POST " : "GET ";
		uint32_t nLen =
			(uint32_t)( gammasstream( szBuffer, nBufferSize ) 
			<< szMeth << ( Info.szHost + Info.nHostLen ) << " HTTP/1.1\r\n"
			<< "Host: " << gammacstring( Info.szHost, Info.nHostLen, true ) << "\r\n"
			<< "Accept: */*\r\n" 
			<< "Content-Length: " << nDataSize << "\r\n"
			<< "Content-Type: application/x-www-form-urlencoded\r\n\r\n"
			).GetCurPos();
		if( nDataSize )
			memcpy( szBuffer + nLen, pData, nDataSize );
		return nLen + nDataSize;
	}

	uint32_t MakeWebSocketShakeHand( char* szBuffer, uint32_t nBufferSize, 
		uint8_t(&aryBinKey)[16], const char* szUrl )
	{
		uint32_t nUrlLen = (uint32_t)strlen( szUrl );
		if( nBufferSize <= nUrlLen + HTTP_REQUEST_HEAD_SIZE )
			return 0;
		SUrlInfo Info = GetHostAndPortFromUrl( szUrl );

		char szUUID[64];
		Base64Encode(szUUID, ELEM_COUNT(szUUID), aryBinKey, sizeof(aryBinKey));

		gammasstream ssShakeHand( szBuffer, nBufferSize );
		ssShakeHand <<
			"GET " << ( Info.szHost + Info.nHostLen ) << " HTTP/1.1\r\n"
			"Host: " << gammacstring( Info.szHost, Info.nHostLen, true ) << " \r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Key: " << szUUID << "\r\n"
			"Origin: null\r\n"
			"Sec-WebSocket-Version: 13\r\n\r\n";
		return (uint32_t)ssShakeHand.GetCurPos();
	}

	GAMMA_COMMON_API uint32_t WebSocketShakeHandCheck( const char* pBuffer, size_t nSize, 
		bool bServer, const char*& szWebSocketKey, uint32_t& nWebSocketKeyLen )
	{
		uint32_t nReadCount = 0;
		uint32_t nPreLineStart = 0;
		uint32_t nKeyCount = 0;
		const char* szKeyStart = NULL;
		bool bFinished = false;
		while( nReadCount < nSize )
		{
			if( pBuffer[nReadCount] == '\r' )
			{
				if( ++nReadCount >= nSize )
					return 0;

				// 不是"\r\n"协议不正确
				if (pBuffer[nReadCount] != '\n')
				{
					szWebSocketKey = "shakehand error( miss 0x0a )";
					return INVALID_32BITID;
				}

				// 检查Key是否存在
				if( bServer )
				{
					static const char* szKey = "Sec-WebSocket-Key";
					static const uint32_t nKey = (uint32_t)strlen( szKey );
					if( !memcmp( szKey, pBuffer + nPreLineStart, nKey ) )
					{
						szKeyStart = pBuffer + nPreLineStart + nKey;
						nKeyCount = nReadCount - 1 - nPreLineStart - nKey;
					}
				}
				else
				{
					static const char* szKey = "Sec-WebSocket-Accept";
					static const uint32_t nKey = (uint32_t)strlen( szKey );
					if( !memcmp( szKey, pBuffer + nPreLineStart, nKey ) )
					{
						szKeyStart = pBuffer + nPreLineStart + nKey;
						nKeyCount = nReadCount - 1 - nPreLineStart - nKey;
					}				
				}

				// 空行结尾
				if( nPreLineStart + 1 == nReadCount )
				{
					bFinished = true;
					nReadCount++;
					break;
				}
				nPreLineStart = nReadCount + 1;
			}
			++nReadCount;
		}	

		if( !bFinished )
			return 0;

		if( !szKeyStart )
		{
			szWebSocketKey = "shakehand error( no key )";
			return INVALID_32BITID;
		}

		uint32_t nKeyIndex = 0;
		while( nKeyIndex < nKeyCount && 
			( szKeyStart[nKeyIndex] == ':' || IsBlank( szKeyStart[nKeyIndex] ) ) )
			nKeyIndex++;

		if( nKeyIndex >= nKeyCount )
		{
			szWebSocketKey = "shakehand error( key length = 0 )";
			return INVALID_32BITID;
		}

		if( nKeyCount - nKeyIndex > 64 )
		{
			szWebSocketKey = "shakehand error( key length >= 64 )";
			return INVALID_32BITID;
		}

		szWebSocketKey = szKeyStart + nKeyIndex;
		nWebSocketKeyLen = nKeyCount - nKeyIndex;
		return nReadCount;
	}

	GAMMA_COMMON_API uint32_t MakeWebSocketServerShakeHandResponese( char* szBuffer, 
		uint32_t nBufferSize, const char* szWebSocketKey, uint32_t nWebSocketKeyLen )
	{
		// 填充http响应头信息  
		char szClientKey[64 + 40];
		GammaAst( nWebSocketKeyLen + 36 < ELEM_COUNT(szClientKey) );
		memcpy(szClientKey, szWebSocketKey, nWebSocketKeyLen);
		memcpy(szClientKey + nWebSocketKeyLen, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
		szClientKey[nWebSocketKeyLen + 36] = 0;

		tbyte shaHash[20];
		sha1((const tbyte*)szClientKey, nWebSocketKeyLen + 36, shaHash);
		char szBase64[256];
		Base64Encode(szBase64, ELEM_COUNT(szBase64), shaHash, sizeof(shaHash));

		gammasstream ssBuffer(szBuffer, nBufferSize);
		ssBuffer <<	"HTTP/1.1 101 Switching Protocols\r\n"
			"Upgrade: websocket\r\n"
			"Connection: upgrade\r\n"
			"Sec-WebSocket-Accept: " << szBase64 << "\r\n\r\n";
		return (uint32_t)ssBuffer.GetCurPos();
	}

	GAMMA_COMMON_API uint64_t GetWebSocketProtocolLen( 
		const SWebSocketProtocal* pProtocol, uint64_t nSize )
	{
		union{ uint64_t u64; uint8_t u8[sizeof(uint64_t)]; } Size;
		Size.u64 = pProtocol->m_nLen;
		const char* pStart = (const char*)(pProtocol + 1);
		const char* pAppend = pStart;
		if( Size.u64 == 126 )
		{
			if( nSize < sizeof(SWebSocketProtocal) + 2 )
				return INVALID_64BITID;
			Size.u8[1] = *pAppend++;
			Size.u8[0] = *pAppend++;
		}
		else if( Size.u64 == 127 )
		{
			if( nSize < sizeof(SWebSocketProtocal) + 8 )
				return INVALID_64BITID;
			Size.u8[7] = *pAppend++;
			Size.u8[6] = *pAppend++;
			Size.u8[5] = *pAppend++;
			Size.u8[4] = *pAppend++;
			Size.u8[3] = *pAppend++;
			Size.u8[2] = *pAppend++;
			Size.u8[1] = *pAppend++;
			Size.u8[0] = *pAppend++;
		}

		return (pAppend - pStart) + Size.u64 + (pProtocol->m_bMask ? 4 : 0);
	}

	GAMMA_COMMON_API uint64_t DecodeWebSocketProtocol( 
		const SWebSocketProtocal* pProtocol, char*& pExtraBuffer, uint64_t& nSize )
	{
		uint64_t nLen = pProtocol->m_nLen;
		const char* pStart = pExtraBuffer;
		char* pAppend = (char*)pStart;
		uint32_t nAppendSize = 0;
		if( nLen >= 126 )
		{
			nAppendSize = nLen == 126 ? sizeof(uint16_t) : sizeof(uint64_t);
			if( nSize < nAppendSize )
				return INVALID_64BITID;
			char* pDest = (char*)&nLen;
			for( uint32_t i = 0; i < nAppendSize; i++ )
				pDest[nAppendSize - 1 - i] = pAppend[i];
			pAppend += nAppendSize;
		}

		if( nSize < nAppendSize + nLen )
			return INVALID_64BITID;

		if( pProtocol->m_bMask )
		{
			const char* szMask = pAppend;
			pAppend += 4;
			for( uint64_t i = 0; i < nLen; i++ )
				pAppend[i] ^= szMask[i%4];
		}

		pExtraBuffer = pAppend;
		nSize = nLen;
		return ( pAppend - pStart ) + nLen; 
	}

	GAMMA_COMMON_API uint32_t EncodeWebSocketProtocol(
		SWebSocketProtocal& Protocol, char* pExtraBuffer, uint64_t nSize )
	{
		union { uint64_t u64; uint8_t u8[sizeof(uint64_t)]; } Size;
		Size.u64 = nSize;
		Protocol.m_bFinished = 1;
		if (Size.u64 < 126)
			Protocol.m_nLen = Size.u8[0];
		else if (Size.u64 < 65536)
			Protocol.m_nLen = 126;
		else
			Protocol.m_nLen = 127;

		if (!Protocol.m_bMask)
			return 0;

		char szMask[] =
		{
			(char)CGammaRand::Rand(0, 256),
			(char)CGammaRand::Rand(0, 256),
			(char)CGammaRand::Rand(0, 256),
			(char)CGammaRand::Rand(0, 256)
		};

		for( uint32_t i = 0; i < nSize; i++ )
			pExtraBuffer[i] ^= szMask[i % 4];
		return *(uint32_t*)szMask;
	}

}