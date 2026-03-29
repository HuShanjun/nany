#include "GammaNetwork/INetworkInterface.h"
#include <iostream>
#include <cstring>
#include <vector>

using namespace Gamma;

class CEchoConnHandler : public IConnectHandler
{
	IConnecter* m_pConnecter;

public:
	CEchoConnHandler() : m_pConnecter(nullptr) {}

	void BindConnecter(IConnecter* pConn)
	{
		m_pConnecter = pConn;
		m_pConnecter->SetHandler(this);
	}

	void OnConnected() override
	{
		std::cout << "[Server] Client connected from "
			<< m_pConnecter->GetRemoteAddress().GetAddress()
			<< ":" << m_pConnecter->GetRemoteAddress().GetPort() << std::endl;
	}

	void OnDisConnect(ECloseType eCloseType) override
	{
		std::cout << "[Server] Client disconnected, CloseType=" << (int)eCloseType << std::endl;
	}

	size_t OnRecv(const char* pBuf, size_t nSize) override
	{
		std::string msg(pBuf, nSize);
		std::cout << "[Server] Recv (" << nSize << " bytes): " << msg << std::endl;
		m_pConnecter->Send(pBuf, nSize);
		return nSize;
	}
};

class CEchoListenHandler : public IListenHandler
{
	std::vector<CEchoConnHandler*> m_vecHandlers;

public:
	~CEchoListenHandler()
	{
		for (auto* p : m_vecHandlers)
			delete p;
	}

	void OnAccept(IConnecter& Connect) override
	{
		std::cout << "[Server] OnAccept: new connection." << std::endl;
		auto* pHandler = new CEchoConnHandler();
		pHandler->BindConnecter(&Connect);
		m_vecHandlers.push_back(pHandler);
	}
};

int main(int argc, char* argv[])
{
	const char* szHost = "0.0.0.0";
	uint16_t nPort = 19850;
	if (argc > 1) nPort = (uint16_t)atoi(argv[1]);

	INetwork* pNetwork = CreateNetWork();
	pNetwork->EnableLog(true);

	CEchoListenHandler listenHandler;
	IListener* pListener = pNetwork->StartListener(szHost, nPort);
	pListener->SetHandler(&listenHandler);
	std::cout << "[Server] Listening on " << szHost << ":" << nPort << std::endl;
	std::cout << "[Server] Press Ctrl+C to stop." << std::endl;

	while (true)
		pNetwork->Check(100);

	pListener->Release();
	pNetwork->Release();
	return 0;
}
