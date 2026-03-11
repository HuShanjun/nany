#pragma once

#include "GammaCommon/GammaCommonType.h"

/************************************************************************/
/*		消息简称意义													*/
/* Game Server: S														*/
/* Dispatch Server: D													*/
/* Logion Server: L														*/
/* Gcc Server: U											        	*/
/* Client: C															*/
/* Share Memory: M                                                      */
/* 请求消息： Query														*/
/* 应答消息： Answer													*/
/* 通知消息： Notify													*/
/************************************************************************/

namespace Gamma
{

#pragma pack(push,1)
	template<typename IdType> 
	class TBasePrtlMsg
	{
		IdType	m_nId;
	public:
		TBasePrtlMsg( IdType nTypeID ) 
			: m_nId( nTypeID ){}

	public:
		typedef IdType IdType_t;

		IdType_t	GetId() const						{ return m_nId; }
		size_t		GetExtraSize( size_t nSize ) const	{ return 0; }
	};
#pragma pack(pop)
}
