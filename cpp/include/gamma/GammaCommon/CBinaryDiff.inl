
namespace Gamma
{
	template< typename File, typename FunRead >
	inline void CBinaryDiff::Load( File& fileRead, FunRead funRead, uint32_t nChunk, uint32_t nMaxSize )
	{
		m_nMaxSize = nMaxSize;
		m_vecDiffChunk.resize( nChunk );
		m_vecDiffOffset.resize( nChunk );
		std::vector<tbyte> vecBuf;
		for( uint32_t i = 0; i < m_vecDiffChunk.size(); i++ )
		{
			m_vecDiffOffset[i] = (uint32_t)vecBuf.size();
			( fileRead.*funRead )( (char*)&m_vecDiffChunk[i], sizeof(SChunkInfo) );
			vecBuf.resize( vecBuf.size() + m_vecDiffChunk[i].m_nSize );
			( fileRead.*funRead )( (char*)&vecBuf[m_vecDiffOffset[i]], m_vecDiffChunk[i].m_nSize );

			GammaAst( i == 0 || m_vecDiffChunk[i].m_nOffset > m_vecDiffChunk[ i - 1 ].m_nOffset );
		}
		m_vecDiffData.assign( vecBuf.begin(), vecBuf.end() );
	}

	template< typename File, typename FunWrite >
	inline void CBinaryDiff::Save( File& fileRead, FunWrite funWrite )
	{
		for( uint32_t i = 0; i < m_vecDiffChunk.size(); i++ )
		{
			( fileRead.*funWrite )( (const char*)&m_vecDiffChunk[i], sizeof(SChunkInfo) );
			( fileRead.*funWrite )( (const char*)&m_vecDiffData[m_vecDiffOffset[i]], m_vecDiffChunk[i].m_nSize );
		}
	}

	template< typename File, typename FunRead, typename FunSize, typename FunSeek >
	inline void CBinaryDiff::BuildDiff( File& fileRead1, File& fileRead2, FunRead funRead, FunSize funSize, FunSize funSeek )
	{
		//=====================================================================================
		// 下面的循环对上面打开的两个数据包进行逐字节比较，找出新旧数据包中所有不相同的区域并把
		// 它们记录到一个队列。
		// 注意：下面的变量bDone用来防止当末尾数据不同时可能出现少添加一个信息的bug。
		//======================================================================================
		bool		bDone		= false;    // 是否完成文件比较
		bool		bEntry		= true;     // 状态变量，指出当前状态是否为入口，这里的入口指的是相异字节的起始点
		SChunkInfo	ChunkInfo	= { 0 };    // 记录相异块的信息
		uint32_t		nNewSize	= 0;
		int32_t		nOld		= 0;
		int32_t		nNew		= 0;
		uint32_t		nReadCount	= 0;

		m_vecDiffChunk.clear();

		do
		{
			char i, k;
			nOld = ( fileRead1.*funRead )( &i, sizeof(i) );
			nNew = ( fileRead2.*funRead )( &k, sizeof(k) );
			nReadCount++;

			// 测试是否完成比较
			if( nOld <= 0 || nNew <= 0 )
				bDone = true;
			else
				nNewSize++;

			if( !bDone && i != k )  // 字节相异
			{
				// 为入口状态
				if( bEntry )
				{
					ChunkInfo.m_nSize   = 1;
					ChunkInfo.m_nOffset = nReadCount - 1;
					bEntry            = false;
				}
				else
				{
					++ChunkInfo.m_nSize;
				}
			}
			else  // 字节相同
			{
				if( !bEntry )
				{
					m_vecDiffChunk.push_back( ChunkInfo );    // 将相异块信息添加到队列
					bEntry = true;
				}
			}

		}while( !bDone );

		//=====================================================================================
		//
		// 处理新数据包大于旧数据包的情况。在两个数据包相比较的时候，会有三种情况：
		//
		// 1：旧数据包较大。
		// 2：新数据包较大。
		// 3：两者等大。
		//
		// 这里只需要处理第2种情况既可，其它两种情况不必处理。
		//
		//=====================================================================================
		if( nOld <= 0 && nNew > 0 )
		{
			ChunkInfo.m_nOffset	= nNewSize;
			ChunkInfo.m_nSize	= (uint32_t)( ( fileRead2.*funSize )() - ( fileRead1.*funSize )() );    // 计算新数据包多出来的那一部份的大小
			m_vecDiffChunk.push_back( ChunkInfo );
		}

		// 没有相异的数据块
		if( !m_vecDiffChunk.size() )
			return;

		Optimize( fileRead2, funRead );
	}

	template< typename File, typename FunRead, typename FunSeek >
	inline void CBinaryDiff::Optimize( File& fileRead, FunRead funRead, FunSeek funSeek )
	{
		std::vector<SChunkInfo> vOldChunkInfo = m_vecDiffChunk;
		m_vecDiffChunk.clear();
		m_vecDiffOffset.clear();
		uint32_t nNewSize = 0;

		for( size_t i = 0; i < vOldChunkInfo.size(); )
		{
			SChunkInfo CurChunk = vOldChunkInfo[ i++ ];
			SChunkInfo NextChunk;
			for( ; i < vOldChunkInfo.size(); ++i )
			{
				NextChunk = vOldChunkInfo[i];
				if( ( NextChunk.m_nOffset - ( CurChunk.m_nOffset + CurChunk.m_nSize ) ) > sizeof( SChunkInfo ) )
					break;
				CurChunk.m_nSize = NextChunk.m_nOffset - CurChunk.m_nOffset + NextChunk.m_nSize;
			}

			m_vecDiffOffset.push_back( nNewSize );
			m_vecDiffChunk.push_back( CurChunk );
			nNewSize += CurChunk.m_nSize;
		}

		m_vecDiffData.resize( nNewSize );
		return ReadDiffDataFromSrc( fileRead, funRead, funSeek );
	}

	template< typename File, typename FunRead, typename FunSeek >
	inline void CBinaryDiff::Merge( const CBinaryDiff& SrcBinaryDiff, File& fileRead, FunRead funRead, FunSeek funSeek )
	{
		if( SrcBinaryDiff.m_vecDiffChunk.empty() )
			return ReadDiffDataFromSrc( fileRead, funRead, funSeek );

		std::vector<SChunkInfo> vOldChunkInfo = m_vecDiffChunk;
		m_vecDiffChunk.clear();
		uint32_t nNewSize = 0;
		uint32_t nSrc = 0;
		uint32_t nDes = 0;
		while( nSrc < SrcBinaryDiff.m_vecDiffChunk.size() && nDes < vOldChunkInfo.size() )
		{
			SChunkInfo CurChunk;
			if( SrcBinaryDiff.m_vecDiffChunk[nSrc].m_nOffset < vOldChunkInfo[nDes].m_nOffset )
				CurChunk = SrcBinaryDiff.m_vecDiffChunk[nSrc];
			else
				CurChunk = vOldChunkInfo[nDes];

			uint32_t nChunkEnd = CurChunk.m_nOffset + CurChunk.m_nSize;
			while( 
				FindNextMerChunk( vOldChunkInfo, nChunkEnd, nDes ) || 
				FindNextMerChunk( SrcBinaryDiff.m_vecDiffChunk, nChunkEnd, nSrc ) );
			CurChunk.m_nSize = nChunkEnd - CurChunk.m_nOffset;
			m_vecDiffChunk.push_back( CurChunk );
		}

		while( nSrc < SrcBinaryDiff.m_vecDiffChunk.size() )
		{
			nNewSize += SrcBinaryDiff.m_vecDiffChunk[nSrc].m_nSize;
			m_vecDiffChunk.push_back( SrcBinaryDiff.m_vecDiffChunk[nSrc++] );
		}

		while( nDes < vOldChunkInfo.size() )
		{
			nNewSize += vOldChunkInfo[nDes].m_nSize;
			m_vecDiffChunk.push_back( vOldChunkInfo[nDes++] );
		}

		return Optimize( fileRead, funRead, funSeek );
	}

	template< typename File, typename FunRead, typename FunSeek >
	inline void CBinaryDiff::ReadDiffDataFromSrc( File& fileRead, FunRead funRead, FunSeek funSeek )
	{
		for( size_t i = 0, j = 0; i < m_vecDiffChunk.size(); ++i )
		{
			( fileRead.*funSeek )( m_vecDiffChunk[i].m_nOffset );
			( fileRead.*funRead )( (char*)&m_vecDiffData[j], m_vecDiffChunk[i].m_nSize );
			j += m_vecDiffChunk[i].m_nSize;
		}
	}

	inline bool CBinaryDiff::IsNeedPatch( uint32_t nOffset, uint32_t nSize ) const
	{
		uint32_t nBegin, nEnd;
		GetBound( m_vecDiffChunk, m_vecDiffChunk.size(), nOffset, nBegin, nEnd );
		if( nBegin < m_vecDiffChunk.size() && m_vecDiffChunk[nBegin].m_nOffset + m_vecDiffChunk[nBegin].m_nSize > nOffset )
			return true;
		if( nEnd < m_vecDiffChunk.size() && nOffset + nSize > m_vecDiffChunk[nEnd].m_nOffset )
			return true;
		return false;
	}

	inline void CBinaryDiff::Patch( void* pBuf, uint32_t nOffset, uint32_t nSize )
	{
		uint32_t nBegin, nEnd;
		GetBound( m_vecDiffChunk, m_vecDiffChunk.size(), nOffset, nBegin, nEnd );

		uint32_t nDataEnd = nOffset + nSize;
		while( nBegin < m_vecDiffChunk.size() && m_vecDiffChunk[nBegin].m_nOffset < nDataEnd )
		{
			tbyte* pChunkData	= &m_vecDiffData[ m_vecDiffOffset[nBegin] ];
			uint32_t nChunkEnd	= m_vecDiffChunk[nBegin].m_nOffset + m_vecDiffChunk[nBegin].m_nSize;
			uint32_t nMin			= (uint32_t)Max<uint32_t>( nOffset, m_vecDiffChunk[nBegin].m_nOffset );
			uint32_t nMax			= (uint32_t)Min<uint32_t>( nDataEnd, nChunkEnd );
			if( nMax > nMin )
			{
				tbyte* pDes		= ( (tbyte*)pBuf ) + nMin - nOffset;
				tbyte* pSrc		= ( (tbyte*)pChunkData ) + nMin - m_vecDiffChunk[nBegin].m_nOffset;
				memcpy( pDes, pSrc, nMax - nMin );
			}
			nBegin++;
		}
	}

	inline uint32_t CBinaryDiff::Init( uint32_t nMaxSize, uint32_t nChunkCount, const tbyte* pDiffChunk )
	{
		CBufFile File( (const tbyte*)pDiffChunk, 0xffffffff );
		Load( File, &CBufFile::Read, nChunkCount, nMaxSize );
		return (uint32_t)File.GetPos();
	}
}
