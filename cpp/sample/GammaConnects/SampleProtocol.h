#pragma once
#include "GammaCommon/GammaCommonType.h"

#pragma pack(push, 1)

enum ESampleMsg : uint16_t
{
	eSM_Echo = 1,
	eSM_EchoReply = 2,
};

struct SMsgHeader
{
	ESampleMsg eMsgId;
};

struct SMsgEcho : SMsgHeader
{
	char szText[256];
};

struct SMsgEchoReply : SMsgHeader
{
	char szText[256];
};

#pragma pack(pop)
