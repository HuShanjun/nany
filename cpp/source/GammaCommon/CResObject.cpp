
#include "CResObject.h"

namespace Gamma
{
	CResObject::CResObject( uint32_t nSerialID, 
		const std::string& strPathName, CPackage* pPackage )
		: m_pFilePackage( pPackage )
		, m_nSerialID( nSerialID )
		, m_strPathName( new CRefString( strPathName ) )
	{
		GammaAst( m_pFilePackage );
		m_pFilePackage->AddRef();
	}

	CResObject::~CResObject()
	{
		CQueueBelongedNode::Remove();
		CPackageBelongedNode::Remove();
		SAFE_RELEASE( m_pFilePackage );
	}

	SFileBuffer CResObject::GetBuffer() const
	{
		GammaAst( m_pFilePackage );
		return m_pFilePackage->GetFileBuffer( m_strPathName->c_str() );
	}

	ELoadState CResObject::GetLoadState() const
	{
		GammaAst( m_pFilePackage );
		return m_pFilePackage->GetLoadState();
	}

	const CRefStringPtr& CResObject::GetPathName() const
	{
		return m_strPathName;
	}

	uint32_t CResObject::GetSerialID() const
	{
		return m_nSerialID;
	}

	CMapListener::iterator CResObject::GetListener() const
	{
		return m_itListener;
	}

	void CResObject::SetListener( CMapListener::iterator itListener )
	{
		m_itListener = itListener;
	}

	CPackage* CResObject::GetFilePackage() const
	{
		return m_pFilePackage;
	}
}
