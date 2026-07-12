#pragma once
#include "GammaConnects/CBaseConn.h"
#include "GammaConnects/TDispatch.h"
#include "GammaCommon/TGammaRBTree.h"
#include "interface/CCurrent.h"

using namespace Gamma;
class CGameConnServer;
typedef TGammaRBTree<CGameConnServer> CWorldConnectMap;

class CGameConnServer
	: public CCurrent
	, public CWorldConnectMap::CGammaRBTreeNode
{
	DECLARE_DYNAMIC_CLASS( CGameConnServer );	
protected:
	uint32					m_nServerID;
	int32					m_nServerType;

	virtual size_t			OnShellMsg( const void* pData, size_t nSize, bool bUnreliable );
	virtual void			OnConnected();
	virtual void			OnDisConnect();

public:
	CGameConnServer(void);
	~CGameConnServer(void);

	operator uint32()		{	return m_nServerID; }
	virtual void			SetServerID(uint32 nID);
	virtual uint32			GetConnectID() { return m_nServerID; }

	virtual void			SendShellMsg( const SSendBuf aryBuffer[], uint32 nBufferCount );
	virtual void			SendShellMsg( const void* pData1, uint32 nSize1 );

	virtual void			SetContextID(int64 nValue) {}
	virtual int64			GetContextID() { return 0; }
};

