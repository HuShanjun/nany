#pragma once
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/TTinyList.h"
#include "CGammaFileMgr.h"
#include "CPackage.h"
#include <string>

namespace Gamma
{
	class CFileReader;
	class CQueueBelongedNode : public TGammaList<CQueueBelongedNode>::CGammaListNode{};
	class CPackageBelongedNode : public TGammaList<CPackageBelongedNode>::CGammaListNode{};

	class CResObject 
		: public CQueueBelongedNode
		, public CPackageBelongedNode
	{
		CMapListener::iterator	m_itListener;
		CPackage*				m_pFilePackage;
		CRefStringPtr			m_strPathName;
		uint32					m_nSerialID;

	public:
		CResObject( uint32 nSerialID, const std::string& strPathName, CPackage* pPackage );
		~CResObject();

		void					SetListener( CMapListener::iterator	itListener );
		CMapListener::iterator	GetListener() const;
		uint32					GetSerialID() const;
		const CRefStringPtr&	GetPathName() const;
		ELoadState				GetLoadState() const;
		SFileBuffer				GetBuffer() const;
		CPackage*				GetFilePackage() const;
	};
}
