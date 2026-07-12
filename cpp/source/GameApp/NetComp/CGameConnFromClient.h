#pragma once
#include "GammaCommon/TGammaRBTree.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/CVersion.h"
#include "GammaCommon/rc4.h"
#include "GammaConnects/CBaseConn.h"
#include "GammaConnects/TDispatch.h"
#include "Interface/CCurrent.h"

using namespace Gamma;
typedef TGammaRBSetNode<uint32> CClientConnectNode;

class CGameConnFromClient
	: public CCurrent
	, public CClientConnectNode
{
	DECLARE_DYNAMIC_CLASS( CGameConnFromClient );
	
protected:
	int64					m_nPlayerID;

	virtual size_t			OnShellMsg( const void* pData, size_t nSize, bool bUnreliable );
	virtual void			OnConnected();
	virtual void			OnDisConnect();

public:
	CGameConnFromClient(void);
	~CGameConnFromClient(void);

	virtual uint32			GetConnectID() { return CClientConnectNode::m_Data; }
	virtual void			SendShellMsg( const SSendBuf aryBuffer[], uint32 nBufferCount, bool bUnreliable = false) override;
	virtual void			SendShellMsg( const void* pData1, uint32 nSize1, bool bUnreliable = false) override;

	virtual void SetContextID(int64 nValue ) { m_nPlayerID = nValue;  }
	virtual int64 GetContextID()	{	return m_nPlayerID;	}

};

