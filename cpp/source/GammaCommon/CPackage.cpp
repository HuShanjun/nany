
#include "CPackage.h"
#include "CResObject.h"
#include "CReadFileThread.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/GammaTime.h"
#include <list>

namespace Gamma
{
	CPackage::CPackage( CPackageMgr* pMgr,
		SPackagePart* pPkgNode )
		: m_eLoadState( eLoadState_NotLoad )
		, m_nRef( 1 )
		, m_pMgr( pMgr )
		, m_pPkgNode( pPkgNode )
	{
	}

	CPackage::~CPackage()
	{
		if( m_pPkgNode )
		{
			SPackagePart* pPkgNode = m_pPkgNode;
			while( pPkgNode->m_nPackageIndex )
				pPkgNode = pPkgNode->m_pPrePart;
			pPkgNode->m_pPackage = NULL;
		}
	}

	bool CPackage::IsHttpRes() const
	{
		const std::string& strName = m_pPkgNode->m_strFilePath;
		const std::string& strPath = 
			( CPathMgr::IsAbsolutePath( strName.c_str() ) ) ?
			strName : m_pMgr->GetBaseWebPathString();
		return strPath.size() >= 7 && !memcmp( strPath.c_str(), "http://", 7 );
	}

	ELoadState CPackage::GetLoadState() const
	{
		return m_eLoadState;
	}

	bool CPackage::HasLoadedSucceed() const
	{
		return m_pPkgNode && m_pPkgNode->m_bLoaded;
	}

	const char* CPackage::GetFileBuffer() const
	{
		if( !m_pMgr || m_eLoadState == eLoadState_Failed )
			return NULL;
		if( !m_strFileBuffer )
			return NULL;
		return m_strFileBuffer->c_str();
	}

	SFileBuffer CPackage::GetFileBuffer( const char* szFileName ) const
	{
		if( !m_pMgr || m_eLoadState == eLoadState_Failed )
			return SFileBuffer();
		const char* szShortPath = m_pMgr->RevertToShortPath( szFileName );
		const CFileContext* pFile = m_pMgr->GetFileContext( szShortPath ? szShortPath : szFileName );
		if( !pFile )
			return SFileBuffer( m_strFileBuffer, 0, (uint32)m_strFileBuffer->length() );
		if( pFile->GetPackage() != this )
			return SFileBuffer();
		if( m_pPkgNode->m_nOrgSize == 0 )
			return SFileBuffer( m_strFileBuffer, 0, (uint32)m_strFileBuffer->length() );
		return GetFileBuffer( pFile->m_nIndex );
	}

	SFileBuffer CPackage::GetFileBuffer( uint32 nIndex ) const
	{
		if( m_eLoadState == eLoadState_Failed )
			return SFileBuffer();
		if( m_vecBlockOffset.empty() || nIndex == INVALID_32BITID )
			return SFileBuffer( m_strFileBuffer, 0, (uint32)m_strFileBuffer->size() );
		if( nIndex >= m_vecBlockOffset.size() )
			return SFileBuffer();
		uint32 nSize = 0;
		uint32 nOffset = m_vecBlockOffset[nIndex];
		const char* szBuffer = m_strFileBuffer->c_str() + nOffset;	
		memcpy( &nSize, szBuffer, sizeof(uint32) );
		return SFileBuffer( m_strFileBuffer, nOffset + sizeof(uint32), nSize );
	}

	void CPackage::MarkNotLoad()
	{
		m_eLoadState = eLoadState_NotLoad;
		CReaderList list = m_listReader;
		m_listReader.clear();
		for( uint32 i = 0; i < list.size(); i++ )
			delete list[i];
	}

	void CPackage::MarkLoading()
	{
		m_eLoadState = eLoadState_Loading;
	}

	uint32 CPackage::GetRef()
	{
		return m_nRef;
	}

	void CPackage::AddRef()
	{
		++m_nRef;
	}

	void CPackage::Release()
	{
		if( --m_nRef )
			return;
		if( m_eLoadState == eLoadState_Succeeded && !m_pMgr->IsReleaseCacheEnable() )
			return;
		delete this;
	}

	void CPackage::AddResObject( CResObject& ResObject )
	{
		// 这里吧m_pMgr放在最前面是为了保证在第一时
		// 间让m_pMgr处理断线重连，不至于波及到其他监听器
		CMapListener::iterator it = ResObject.GetListener();
		if( it->first != m_pMgr )
			m_listResObject.PushBack( ResObject );
		else
			m_listResObject.PushFront( ResObject );
	}

	CReaderList& CPackage::CreateReaders( bool bCache )
	{
		GammaAst( m_listReader.empty() );
		uint8 aryMd5Buffer[16] = { 0 };
		std::string strUrl;

		SPackagePart* pPkgNode = m_pPkgNode;
		m_listReader.resize( m_pPkgNode->m_nPackageIndex + 1 );
		for( int32 i = m_pPkgNode->m_nPackageIndex; i >= 0; --i )
		{
			uint8* aryMd5  = NULL;
			uint32 nOrgSize = pPkgNode->m_nOrgSize;
			if( nOrgSize && nOrgSize != INVALID_32BITID )
			{
				const std::string& strPath = pPkgNode->m_strFilePath;
				std::string::size_type nPos = strPath.find_last_of( '/' );
				if( nPos == std::string::npos )
				{
					aryMd5 = aryMd5Buffer;
					const char* szMd5 = strPath.c_str() + nPos + 1;
					for( size_t i = 0; i < 16; i++ )
					{
						aryMd5[i] = (uint8)( ValueFromHexNumber( *szMd5++ ) << 4 );
						aryMd5[i] |= (uint8)( ValueFromHexNumber( *szMd5++ ) );
					}
				}
			}

			const std::string& strName = pPkgNode->m_strFilePath;
			const char* szUrl = strName.c_str();
			if( !CPathMgr::IsAbsolutePath( szUrl ) )
			{
				strUrl = m_pMgr->GetBaseWebPathString();
				strUrl.append( strName );
				szUrl = strUrl.c_str();
			}

			CFileReader* pReader = new CFileReader( this, szUrl, nOrgSize, aryMd5, bCache );
			m_listReader[pPkgNode->m_nPackageIndex] = pReader;
			pPkgNode = pPkgNode->m_pPrePart;
		}

		return m_listReader;
	}

	bool CPackage::OnLoaded( uint32 nIdleTime )
	{
		GammaAst( !m_listReader.empty() );
		bool bSucceeded = true;
		for( uint32 i = 0; i < m_listReader.size(); i++ )
		{
			CFileReader* pReader = m_listReader[i];
			if( pReader->GetLoadState() < eLoadState_Failed )
				return true;
			const CRefStringPtr& ptrBuffer = pReader->GetFileBuffer();
			if( ptrBuffer && !ptrBuffer->empty() )
				continue;
			bSucceeded = false;
		}

		CGammaFileMgr* pFileMgr = m_pMgr->GetFileMgr();
		CTimer timer;

		if( m_eLoadState != eLoadState_Failed && m_eLoadState != eLoadState_Succeeded )
		{
			m_eLoadState = eLoadState_Failed;

			if( bSucceeded )
			{
				m_eLoadState = eLoadState_Succeeded;

				if( m_listReader.size() == 1 )
					m_strFileBuffer = m_listReader[0]->GetFileBuffer();
				else
				{
					size_t nTotalSize = 0;
					for( uint32 i = 0; i < m_listReader.size(); i++ )
						nTotalSize += m_listReader[i]->GetFileBuffer()->size();

					m_strFileBuffer = new CRefString;
					m_strFileBuffer->resize( nTotalSize );
					char* szDest = &( (*m_strFileBuffer)[0]);

					uint32 nCurPos = 0;
					for( uint32 i = 0; i < m_listReader.size(); i++ )
					{
						const CRefStringPtr& ptrBuffer = m_listReader[i]->GetFileBuffer();
						memcpy( szDest + nCurPos, &( (*ptrBuffer)[0] ), ptrBuffer->size() ); 
						nCurPos += (uint32)ptrBuffer->size();
					}
				}

				if( m_pPkgNode && m_pPkgNode->m_nOrgSize != 0 )
				{
					uint32 nFileCount = 0;
					for( uint32 nOffset = 0; nOffset < m_strFileBuffer->size(); nFileCount++ )
						nOffset += sizeof(uint32) + *(uint32*)( m_strFileBuffer->c_str() + nOffset );

					m_vecBlockOffset.resize( nFileCount );
					for( uint32 nOffset = 0, i = 0; nOffset < m_strFileBuffer->size(); i++ )
					{
						m_vecBlockOffset[i] = nOffset; 
						nOffset += sizeof(uint32) + *(uint32*)( m_strFileBuffer->c_str() + nOffset );
					}
				}

				// 文件曾经加载成功，这些文件会在并行加载中被优先加载
				SPackagePart* pPkgNode = m_pPkgNode;
				for( int32 i = m_pPkgNode->m_nPackageIndex; i >= 0; --i )
				{
					pPkgNode->m_bLoaded = true;
					pPkgNode = pPkgNode->m_pPrePart;
				}
			}
		}

		CPackageBelongedNode* pResNode = m_listResObject.GetFirst();
		while( pResNode && timer.GetElapse() < nIdleTime )
		{
			pResNode->Remove();
			pFileMgr->OnResObjectLoadedEnd( static_cast<CResObject*>( pResNode ) );
			// 因为CPackageMgr在m_eLoadState == eLoadState_Failed的时候
			// 会重新加载，因此这里需要判断m_eLoadState状态
			if( m_eLoadState <= eLoadState_Loading )
				return true;
			pResNode = m_listResObject.GetFirst();
		}

		if( m_listResObject.GetFirst() )
			return false;

		CReaderList list = m_listReader;
		m_listReader.clear();
		for( uint32 i = 0; i < list.size(); i++ )
			delete list[i];
		return true;
	}

}
