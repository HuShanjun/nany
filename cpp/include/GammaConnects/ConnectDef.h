#pragma once
#include "GammaCommon/CDynamicObject.h"

namespace Gamma
{
	#define MAX_RAW_UPD_SIZE 1024

	enum EConnType
	{
		eConnType_UDP_Raw,
		eConnType_UDP_Prt,
		eConnType_TCP_Raw,
		eConnType_TCP_Prt,
		eConnType_TCP_Web,
		eConnType_TCP_Prts,
		eConnType_TCP_Webs,
	};

	struct SKcpConfig
	{
		uint32_t	KCPCONFIG_CONV;
		uint32_t	KCPCONFIG_UPDATE; //update间隔
		int32_t	KCPCONFIG_TICK; //kcp tick
		int32_t	KCPCONFIG_RESEND; //跨越次数重传,0为不重传
		int32_t	KCPCONFIG_SENDWND; //发送窗口
		int32_t	KCPCONFIG_RECVWND; //接收窗口
		int32_t	KCPCONFIG_RTO; //延时重传
	};
}