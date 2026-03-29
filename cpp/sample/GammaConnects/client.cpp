#include "GammaCommon/CDynamicObject.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaConnects/IConnectionMgr.h"
#include "GammaConnects/CBaseConn.h"
#include "SampleProtocol.h"
#include <iostream>
#include <cstring>
#include <string>
#include <thread>
#include <chrono>

using namespace Gamma;

class CClientConn : public CBaseConn
{
	DECLARE_DYNAMIC_CLASS(CClientConn);

	bool m_bGotReply;

public:
	CClientConn() : m_bGotReply(false) {}
	bool HasReply() const { return m_bGotReply; }
	void ResetReply() { m_bGotReply = false; }

	void OnConnected() override
	{
		std::cout << "[Client] Connected to server." << std::endl;
	}

	void OnDisConnect() override
	{
		std::cout << "[Client] Disconnected." << std::endl;
	}

	size_t OnShellMsg(const void* pCmd, size_t nSize, bool bUnreliable) override
	{
		if (nSize < sizeof(SMsgHeader))
			return 0;

		auto* pHeader = (const SMsgHeader*)pCmd;
		switch (pHeader->eMsgId)
		{
		case eSM_EchoReply:
		{
			if (nSize < sizeof(SMsgEchoReply))
				return 0;
			auto* pReply = (const SMsgEchoReply*)pCmd;
			std::cout << "[Client] Recv EchoReply: \"" << pReply->szText << "\"" << std::endl;
			m_bGotReply = true;
			return sizeof(SMsgEchoReply);
		}
		default:
			std::cerr << "[Client] Unknown msg id=" << pHeader->eMsgId << std::endl;
			return nSize;
		}
	}

	void SendEcho(const char* szText)
	{
		SMsgEcho msg = {};
		msg.eMsgId = eSM_Echo;
		strncpy(msg.szText, szText, sizeof(msg.szText) - 1);
		SendShellMsg(&msg, sizeof(msg));
	}
};

DEFINE_DYNAMIC_CLASS(CClientConn, 16, GET_CLASS_ID(CClientConn));

int main(int argc, char* argv[])
{
	const char* szHost = "127.0.0.1";
	uint16_t nPort = 19860;
	if (argc > 1) szHost = argv[1];
	if (argc > 2) nPort = (uint16_t)atoi(argv[2]);

	IConnectionMgr* pMgr = CreateConnMgr(30);

	CClientConn* pClient = static_cast<CClientConn*>(
		pMgr->Connect(szHost, nPort, CClientConn::GetID(), eConnType_TCP_Prt));
	std::cout << "[Client] Connecting to " << szHost << ":" << nPort << " ..." << std::endl;

	for (int i = 0; i < 100; ++i)
	{
		pMgr->Check(10);
		if (pClient->IsConnected())
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	if (!pClient->IsConnected())
	{
		std::cerr << "[Client] Failed to connect." << std::endl;
		pMgr->Release();
		return -1;
	}

	std::cout << "[Client] Remote=" << pClient->GetRemoteAddress().GetAddress()
		<< ":" << pClient->GetRemoteAddress().GetPort() << std::endl;
	std::cout << "[Client] Type a message and press Enter (empty line to quit):" << std::endl;

	std::string line;
	while (pClient->IsConnected())
	{
		std::cout << "> ";
		if (!std::getline(std::cin, line) || line.empty())
			break;

		pClient->ResetReply();
		pClient->SendEcho(line.c_str());

		for (int i = 0; i < 40; ++i)
		{
			pMgr->Check(10);
			if (pClient->HasReply())
				break;
			std::this_thread::sleep_for(std::chrono::milliseconds(25));
		}
	}

	std::cout << "[Client] PingDelay=" << pClient->GetPingDelay()
		<< " TotalSend=" << pClient->GetTotalSendSize()
		<< " TotalRecv=" << pClient->GetTotalRecvSize() << std::endl;

	pClient->ShellCmdClose("client quit");
	for (int i = 0; i < 10; ++i)
		pMgr->Check(10);

	pMgr->StopAllConnect();
	pMgr->Release();
	std::cout << "[Client] Done." << std::endl;
	return 0;
}
