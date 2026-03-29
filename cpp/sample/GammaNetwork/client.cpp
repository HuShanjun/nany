#include "GammaNetwork/INetworkInterface.h"
#include <iostream>
#include <cstring>
#include <string>
#include <thread>
#include <chrono>

using namespace Gamma;

class CClientHandler : public IConnectHandler
{
	bool m_bConnected;
	bool m_bDisconnected;

public:
	CClientHandler() : m_bConnected(false), m_bDisconnected(false) {}
	bool IsConnected() const { return m_bConnected; }
	bool IsDisconnected() const { return m_bDisconnected; }

	void OnConnected() override
	{
		m_bConnected = true;
		std::cout << "[Client] Connected to server." << std::endl;
	}

	void OnDisConnect(ECloseType eCloseType) override
	{
		m_bDisconnected = true;
		std::cout << "[Client] Disconnected, CloseType=" << (int)eCloseType << std::endl;
	}

	size_t OnRecv(const char* pBuf, size_t nSize) override
	{
		std::cout << "[Client] Recv echo (" << nSize << " bytes): "
			<< std::string(pBuf, nSize) << std::endl;
		return nSize;
	}
};

int main(int argc, char* argv[])
{
	const char* szHost = "127.0.0.1";
	uint16_t nPort = 19850;
	if (argc > 1) szHost = argv[1];
	if (argc > 2) nPort = (uint16_t)atoi(argv[2]);

	INetwork* pNetwork = CreateNetWork();
	pNetwork->EnableLog(true);

	CClientHandler clientHandler;
	IConnecter* pClient = pNetwork->Connect(szHost, nPort);
	pClient->SetHandler(&clientHandler);
	std::cout << "[Client] Connecting to " << szHost << ":" << nPort << " ..." << std::endl;

	for (int i = 0; i < 100; ++i)
	{
		pNetwork->Check(10);
		if (clientHandler.IsConnected())
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	if (!clientHandler.IsConnected())
	{
		std::cerr << "[Client] Failed to connect." << std::endl;
		pNetwork->Release();
		return -1;
	}

	std::cout << "[Client] Remote=" << pClient->GetRemoteAddress().GetAddress()
		<< ":" << pClient->GetRemoteAddress().GetPort() << std::endl;
	std::cout << "[Client] Type a message and press Enter (empty line to quit):" << std::endl;

	std::string line;
	while (!clientHandler.IsDisconnected())
	{
		std::cout << "> ";
		if (!std::getline(std::cin, line) || line.empty())
			break;

		pClient->Send(line.c_str(), line.size());

		for (int i = 0; i < 20; ++i)
		{
			pNetwork->Check(10);
			std::this_thread::sleep_for(std::chrono::milliseconds(25));
		}
	}

	pClient->CmdClose(true);
	for (int i = 0; i < 10; ++i)
		pNetwork->Check(10);

	std::cout << "[Client] TotalSend=" << pClient->GetTotalSendSize()
		<< " TotalRecv=" << pClient->GetTotalRecvSize() << std::endl;

	pNetwork->Release();
	std::cout << "[Client] Done." << std::endl;
	return 0;
}
