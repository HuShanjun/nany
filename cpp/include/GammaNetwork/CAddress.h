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
		uint32		m_nAddress;
		uint16		m_nPort;

	public:

		CAddress();
		CAddress( const char* szAddress, uint16 nPort );
		CAddress( uint32 nAddress, uint16 nPort );
		CAddress( uint64 nPackAddress );

		bool operator  == (const CAddress& ano) const;
		const CAddress& operator = ( const CAddress& ano );
		bool operator >( const CAddress& ano ) const;

		void		SetAddress( const char* szAddress );
		void		SetAddress( uint32 nAddress );
		void		SetPort( uint16 nPort );
		const char* GetAddress()const;
		uint8		GetAddressSize() const;
		uint32		GetIP() const;
		uint16		GetPort()const;
		uint64		GetPackAddress() const;
	};

	GAMMA_NETWORK_API uint32		ConVertAddressToInt32( const char* szAddress );
	GAMMA_NETWORK_API const char*	ConvertInt32ToAddress( uint32 nIP );
	GAMMA_NETWORK_API bool			IsIP(const char* szAddress);
	GAMMA_NETWORK_API bool			IsPort(const char* szPort);
#ifdef WIN32
	const char*						GetLocalHostIP();
#endif
}
