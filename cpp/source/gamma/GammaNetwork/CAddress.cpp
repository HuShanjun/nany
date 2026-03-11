
#include "GammaNetwork/CAddress.h"
#include "GammaNetworkHelp.h"


namespace Gamma
{
#ifdef WIN32
	const char* GetLocalHostIP()
	{
		//return "127.0.0.1";
		char buf[256] = "";
		struct hostent* ph = 0;
		WSADATA w;
		WSAStartup(0x0101, &w);//这一行必须在使用任何SOCKET函数前写！
		gethostname(buf, 256);
		ph = gethostbyname(buf);
		const char* ip = inet_ntoa(*((struct in_addr*)ph->h_addr_list[0]));//此处获得本机IP
		WSACleanup();
		return ip;
	}
#endif // WIN32


	uint32_t ConVertAddressToInt32( const char* szAddress )
	{
		return (uint32_t)inet_addr(szAddress);
	}

	const char* ConvertInt32ToAddress( uint32_t nIP )
	{
		in_addr ia;
		ia.s_addr = nIP;
		return inet_ntoa(ia);
	}

	bool IsIP(const char* szAddress)
	{
		if (szAddress == nullptr || strlen(szAddress) == 0)
		{
			return false;
		}
		if (inet_addr(szAddress) != INADDR_NONE)
		{
			return true;
		}
		struct in6_addr sin6_addr;
		char szip[100] = { 0 };
		if (1 == inet_pton(AF_INET6, GetStrItem(szip, szAddress, '%', 0), &sin6_addr))
		{
			return true;
		}
		return false;
	}

	bool IsPort(const char* szPort)
	{
		const char* tmp = szPort;

		if (*tmp == '-')
			tmp++;
		else if (*tmp == '+')
			tmp++;

		while ('\0' != *tmp)
		{
			if ((*tmp < '0') || (*tmp > '9'))
				return false;
			tmp++;
		}

		uint32_t nPort = atoi(szPort);
		if (nPort > 0xFFFF)
			return false;

		return true;
	}

	CAddress::CAddress()
		: m_nPort(0)
		, m_nAddress(0)
	{
		memset( m_sAddress, 0, sizeof(m_sAddress) );
	}

	CAddress::CAddress( const char* szAddress, uint16_t nPort ) 
		: m_nPort(nPort)
	{
		SetAddress( szAddress );
	}

	CAddress::CAddress( uint32_t nAddress, uint16_t nPort ) 
		: m_nPort( nPort )
	{
		SetAddress( nAddress );
	}

	CAddress::CAddress( uint64_t nPackAddress )
		: m_nPort( (uint16_t)nPackAddress )
	{
		SetAddress( (uint32_t)( nPackAddress>>32 ) );
	}

	bool CAddress::operator == ( const CAddress& ano )const
	{
		return m_nAddress == ano.m_nAddress && m_nPort == ano.m_nPort;
	}

	const CAddress& CAddress::operator= ( const CAddress& ano )
	{
		memcpy( m_sAddress, ano.m_sAddress, sizeof( m_sAddress ) );
		m_nAddress	= ano.m_nAddress;
		m_nPort		= ano.m_nPort;
		return *this;
	}

	bool CAddress::operator > ( const CAddress& ano ) const
	{
		return GetPackAddress() > ano.GetPackAddress();
	}

	void CAddress::SetAddress( const char* szAddress )
	{
		strcpy2array_safe( m_sAddress, szAddress );
		m_nAddress = ConVertAddressToInt32( szAddress );
	}

	void CAddress::SetAddress( uint32_t nAddress )
	{
		strcpy2array_safe( m_sAddress, ConvertInt32ToAddress( nAddress ) );
		m_nAddress = ConVertAddressToInt32( m_sAddress );
	}

	const char* CAddress::GetAddress()const
	{
		return m_sAddress;
	}

	void CAddress::SetPort( uint16_t nPort )
	{
		m_nPort = nPort;
	}

	uint16_t CAddress::GetPort()const
	{
		return m_nPort;
	}

	uint64_t CAddress::GetPackAddress() const
	{
		return ( ( (uint64_t)m_nAddress ) << 32 )|m_nPort;
	}

	uint32_t CAddress::GetIP() const
	{
		return m_nAddress;
	}

	uint8_t CAddress::GetAddressSize() const
	{
		return (uint8_t)strlen(m_sAddress);
	}
}
