#pragma once
#include "GammaCommon/GammaHelp.h"
#include "GammaConnects/TBasePrtlMsg.h"
#include <vector>
#include <string>

namespace Gamma
{
	enum class ECheckResult {
		 eCR_Again = 0,  // 消息不完整，需要再次监察
		 eCR_Error = -1  // 消息错误，需要丢弃
	};

	template<typename DispatchClass, 
		typename IdType_t, 
		typename SubClass = DispatchClass,
		typename MsgBaseType = TBasePrtlMsg<IdType_t> >
	class TDispatch
	{
		uint32_t	m_nMaxLen;

	protected:
		typedef size_t (*MsgCheckFun_t)( DispatchClass* pClass, const void*, size_t );
		typedef void   (DispatchClass::*MsgProcessFun_t)		( const MsgBaseType*, size_t );

		struct CMsgFunction
		{ 
			MsgCheckFun_t pCheckFun;
			MsgProcessFun_t pProcessFun;
			size_t		nHeadSize;
			const char* szName;
		};

		static std::vector<CMsgFunction>& GetMsgFunction()
		{
			static std::vector<CMsgFunction> _instance;
			return _instance;
		}

		size_t GetMsgFunctionCount()
		{
			return GetMsgFunction().size();
		}

		MsgCheckFun_t GetCheckFun( size_t nIndex )
		{
			return GetMsgFunction()[nIndex].pCheckFun;
		}

		MsgProcessFun_t GetProcessFun( size_t nIndex )
		{
			return GetMsgFunction()[nIndex].pProcessFun;
		}

	public:
		enum { eMaxRecivesize = 4 * 4096 * 4096 };

		TDispatch( uint32_t nMaxLen = INVALID_32BITID );

		template<typename ClassType, typename MsgType>
		static void	RegistProcessFun( void (ClassType::*pMsgProcessFun)( const MsgType*, size_t ) );
		
		size_t		Dispatch( const void* pData, size_t nSize );
	};

}

#include "GammaConnects/TDispatch.inl"
