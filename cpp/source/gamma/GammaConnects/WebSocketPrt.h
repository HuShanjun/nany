#pragma once
#include "GammaConnects/TBasePrtlMsg.h"
#include "GammaCommon/GammaHttp.h"
#include "GammaCommon/GammaHelp.h"
#include <stdarg.h>

namespace Gamma
{
#pragma pack(push,1)
	struct SWebSocketMsg : public SWebSocketProtocal
	{
		SWebSocketMsg( uint8_t nId )
		{
			m_nId = nId;
			m_nReserved = 0;
		}

		uint8_t GetId() const	
		{
			return m_nId; 
		}

		size_t GetExtraSize( size_t nSize ) const
		{
			size_t nCmdSize = GetWebSocketProtocolLen( this, nSize );
			return nCmdSize == INVALID_64BITID ? nSize : nCmdSize;
		}
	};

#undef Cmd_Begin
#undef Cmd_End

#define Cmd_Begin(CommandName,CommandId) \
	class CommandName : public SWebSocketMsg\
	{\
	public:\
	CommandName() : SWebSocketMsg( CommandId ){}\
	static uint8_t GetIdByType()		{ return (uint8_t)CommandId; }\
	static size_t GetHeaderSize()	{ return sizeof(CommandName); }\
	static const char* GetName()	{ return #CommandName; }

#define Cmd_End };

	Cmd_Begin( CWS_Empty, eWS_Empty )
	Cmd_End

	Cmd_Begin( CWS_Text, eWS_Text )
	Cmd_End

	Cmd_Begin( CWS_Binary, eWS_Binary )
	Cmd_End

	Cmd_Begin( CWS_Close, eWS_Close )
	Cmd_End

	Cmd_Begin( CWS_Ping, eWS_Ping )
	Cmd_End

	Cmd_Begin( CWS_Pong, eWS_Pong )
	Cmd_End
#pragma pack(pop)
}
