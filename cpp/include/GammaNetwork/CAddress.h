#pragma once
#include "GammaCommon/GammaCommonType.h"
#include "GammaNetwork/GammaNetworkBase.h"
#include "GammaConnects/ConnectDef.h"
#include <string>

#ifdef _WIN32
//取消极其恶心的windows宏
#undef SetPort
#endif

namespace Gamma
{
	struct GAMMA_NETWORK_API CAddress
	{
	private:
		char		m_sAddress[64];
		uint32_t		m_nAddress;
		uint16_t		m_nPort;

	public:

		CAddress();
		CAddress( const char* szAddress, uint16_t nPort );
		CAddress( uint32_t nAddress, uint16_t nPort );
		CAddress( uint64_t nPackAddress );

		bool operator  == (const CAddress& ano) const;
		const CAddress& operator = ( const CAddress& ano );
		bool operator >( const CAddress& ano ) const;

		void		SetAddress( const char* szAddress );
		void		SetAddress( uint32_t nAddress );
		void		SetPort( uint16_t nPort );
		const char* GetAddress()const;
		uint8_t		GetAddressSize() const;
		uint32_t		GetIP() const;
		uint16_t		GetPort()const;
		uint64_t		GetPackAddress() const;
	};

	GAMMA_NETWORK_API uint32_t		ConVertAddressToInt32( const char* szAddress );
	GAMMA_NETWORK_API const char*	ConvertInt32ToAddress( uint32_t nIP );
	GAMMA_NETWORK_API bool			IsIP(const char* szAddress);
	GAMMA_NETWORK_API bool			IsPort(const char* szPort);
#ifdef WIN32
	const char*						GetLocalHostIP();
#endif
}
