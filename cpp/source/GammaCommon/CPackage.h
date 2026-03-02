#pragma once
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/TRefString.h"
#include "CPackageMgr.h"

namespace Gamma
{
	struct SFileBuffer
	{
		SFileBuffer() : m_nOffset(0), m_nBufferSize(0){}
		SFileBuffer( const CRefStringPtr& Buffer, uint32 nOffset, uint32 nBufferSize )
			: m_BufferPtr(Buffer), m_nOffset(nOffset), m_nBufferSize(nBufferSize){}
		CRefStringPtr	m_BufferPtr;
		uint32			m_nOffset;
		uint32			m_nBufferSize;
	};

	class CAddressHttp;
	class CResObject;
	class CFileReader;
	class CPackageBelongedNode;
	typedef std::vector<CFileReader*> CReaderList;

	class CPackage
	{
		typedef TGammaList<CPackageBelongedNode> CResObjectList;
	public:
		CPackage( CPackageMgr* pMgr, 
			SPackagePart* pPkgNode );
		~CPackage();

		uint32				GetRef();
		void				AddRef();
		void				Release();

		void				MarkNotLoad();
		void				MarkLoading();
		SFileBuffer			GetFileBuffer( const char* szFileName ) const;
		SFileBuffer			GetFileBuffer( uint32 nIndex ) const;
		const char*			GetFileBuffer() const;
		bool				IsHttpRes() const;
		ELoadState			GetLoadState() const;		
		bool				HasLoadedSucceed() const;
		SPackagePart*		GetLastPkgNode() { return m_pPkgNode; }
		CReaderList&		GetReaderList() { return m_listReader; }
		CReaderList&		CreateReaders( bool bCache );
		void				AddResObject( CResObject& ResObject );
		bool				OnLoaded( uint32 nIdleTime );

	private:
		CPackageMgr*		m_pMgr;
		SPackagePart*		m_pPkgNode;

		uint32				m_nRef;
		ELoadState			m_eLoadState;

		CRefStringPtr		m_strFileBuffer;
		std::vector<uint32>	m_vecBlockOffset; 
		CResObjectList		m_listResObject;
		CReaderList			m_listReader;
	};
}
