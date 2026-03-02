#pragma once
#include "GammaConnects/TBasePrtlMsg.h"
#include "GammaCommon/GammaHelp.h"
#include <stdarg.h>

namespace Gamma
{
#pragma pack(push,1)

	typedef TBasePrtlMsg<uint8> CPrtGammaConnects;

#undef Cmd_Begin
#undef Cmd_End

#define Cmd_Begin(CommandName,CommandId) \
	class CommandName : public CPrtGammaConnects\
	{\
	public:\
	CommandName() : CPrtGammaConnects( CommandId ){}\
	static uint8 GetIdByType()		{ return (uint8)CommandId; }\
	static size_t GetHeaderSize()	{ return sizeof(CommandName); }\
	static const char* GetName()	{ return #CommandName; }


#define Cmd_End };

	//============================================
	// 命令ID
	//============================================
	enum EGammaConnectsID
	{
		eGC_ShellMsg8			= 0,
		eGC_ShellMsg32			= 253,
		eGC_HeartbeatMsg 		= 254,
		eGC_HeartbeatReply		= 255,
		eGC_CommonEnd			,
	};

	Cmd_Begin( CGC_HeartbeatMsg, eGC_HeartbeatMsg )
	Cmd_End

	Cmd_Begin( CGC_HeartbeatReply, eGC_HeartbeatReply )
	Cmd_End

	Cmd_Begin( CGC_ShellMsg8, eGC_ShellMsg8 )
		CGC_ShellMsg8( uint8 nId ) : CPrtGammaConnects( nId ){}
		uint8 nMsgLen;
		size_t GetExtraSize( size_t nSize ) const { return GetId()*256 + nMsgLen; }
	Cmd_End

	Cmd_Begin( CGC_ShellMsg32, eGC_ShellMsg32 )
		uint32 nMsgLen;
		size_t GetExtraSize( size_t nSize ) const { return nMsgLen; }
	Cmd_End

#pragma pack(pop)
}
