#include "GammaCommon/CDynamicObject.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaConnects/IConnectionMgr.h"
#include "GammaConnects/CBaseConn.h"
#include "SampleProtocol.h"
#include <iostream>
#include <cstring>

using namespace Gamma;

class CServerConn : public CBaseConn
{
	DECLARE_DYNAMIC_CLASS(CServerConn);

public:
	void OnConnected() override
	{
		std::cout << "[Server] Client connected from "
			<< GetRemoteAddress().GetAddress()
			<< ":" << GetRemoteAddress().GetPort() << std::endl;
	}

	void OnDisConnect() override
	{
		std::cout << "[Server] Client disconnected." << std::endl;
	}

	size_t OnShellMsg(const void* pCmd, size_t nSize, bool bUnreliable) override
	{
		if (nSize < sizeof(SMsgHeader))
			return 0;

		auto* pHeader = (const SMsgHeader*)pCmd;
		switch (pHeader->eMsgId)
		{
		case eSM_Echo:
		{
			if (nSize < sizeof(SMsgEcho))
				return 0;
			auto* pEcho = (const SMsgEcho*)pCmd;
			std::cout << "[Server] Recv Echo: \"" << pEcho->szText << "\"" << std::endl;

			SMsgEchoReply reply = {};
			reply.eMsgId = eSM_EchoReply;
			strncpy(reply.szText, pEcho->szText, sizeof(reply.szText) - 1);
			SendShellMsg(&reply, sizeof(reply));

			return sizeof(SMsgEcho);
		}
		default:
			std::cerr << "[Server] Unknown msg id=" << pHeader->eMsgId << std::endl;
			return nSize;
		}
	}
};

DEFINE_DYNAMIC_CLASS(CServerConn, 16, GET_CLASS_ID(CServerConn));

int main(int argc, char* argv[])
{
	const char* szHost = "0.0.0.0";
	uint16_t nPort = 19860;
	if (argc > 1) nPort = (uint16_t)atoi(argv[1]);

	IConnectionMgr* pMgr = CreateConnMgr(30);
	pMgr->StartService(szHost, nPort, CServerConn::GetID(), eConnType_TCP_Prt);
	std::cout << "[Server] Listening on " << szHost << ":" << nPort << std::endl;
	std::cout << "[Server] Press Ctrl+C to stop." << std::endl;

	while (true)
		pMgr->Check(100);

	pMgr->StopAllService();
	pMgr->Release();
	return 0;
}
