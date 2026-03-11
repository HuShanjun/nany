
namespace Gamma
{	
	template<typename DispatchClass, typename MsgClass>
	size_t CheckMsg( DispatchClass* pClass, const void* pData, size_t nSize )
	{
		typedef DispatchClass CDispatchClass;
		if( nSize < MsgClass::GetHeaderSize() )
			return (size_t)eCR_Again;

		const MsgClass* pMsg = static_cast<const MsgClass*>(pData);
		size_t nExtraSize = pMsg->GetExtraSize( nSize );
		if( nExtraSize >= CDispatchClass::eMaxRecivesize )
			return (size_t)eCR_Error;

		size_t nCmdSize =  nExtraSize + MsgClass::GetHeaderSize();//--?2014年9月25日19:40:54
		if( nCmdSize > nSize )
			return (size_t)eCR_Again;
		return nCmdSize;
	}

	template<typename DispatchClass, typename IdType_t, typename SubClass, typename MsgBaseType >
	size_t TDispatch<DispatchClass, IdType_t, SubClass, MsgBaseType>::Dispatch( const void* pBuf, size_t nSize )
	{
		typedef DispatchClass CDispatchClass;
		size_t nUseSize = 0;

		CDispatchClass* pDispatchClass = static_cast<CDispatchClass*>(this);

		// 这里不使用GammaThrow的原因是避免将信息输出到Err的log文件
		// 消息解释错误只可能是玩家外挂行为，不需要理会                     
		while( nSize >= sizeof( TBasePrtlMsg<IdType_t> ) )
		{
			const MsgBaseType* pMsg = static_cast<const MsgBaseType*>( pBuf );

			if( pDispatchClass->GetMsgFunctionCount() <= pMsg->GetId() )
			{
				GammaLog << "Dispatch error:Unknow the message id," << (uint32_t)pMsg->GetId() << std::endl;
				GammaThrow( "Unknow the message id!" );
			}

			MsgCheckFun_t pCheckFun = pDispatchClass->GetCheckFun( pMsg->GetId() );
			if( pCheckFun == NULL )
			{
				GammaLog << "Dispatch error:Not find CheckFunction:" << (uint32_t)pMsg->GetId() << std::endl;
				GammaThrow ( "Not find CheckFunction!" );
			}

			size_t nCheckSize = ( *pCheckFun )( pDispatchClass, pBuf, nSize );
			if( nCheckSize == eCR_Again )
				return nUseSize;

			if( nCheckSize == eCR_Error )
			{
				GammaLog << "Dispatch error:The net package error:" << (uint32_t)pMsg->GetId() << std::endl;
				GammaThrow ( "The net package error!" );
			}

			if( (uint32_t)nCheckSize > m_nMaxLen )
			{
				GammaLog << "Dispatch error:The msg require len too long:" << (uint32_t)pMsg->GetId() << "," << nCheckSize << std::endl;
				GammaThrow ( "The msg require len too long !" );
			}

			MsgProcessFun_t pProcessFun = pDispatchClass->GetProcessFun( pMsg->GetId() );
			( pDispatchClass->*pProcessFun )( pMsg, nCheckSize );//基本上调用的都是OnCommand
			
			nUseSize	= nUseSize + nCheckSize;
			nSize		= nSize - nCheckSize;
			pBuf		= ( (const char*)pBuf ) + nCheckSize;
		}

		return nUseSize;
	}

	template<typename DispatchClass, typename IdType_t, typename SubClass, typename MsgBaseType>
	template<typename ClassType, typename MsgType>
	void TDispatch<DispatchClass, IdType_t, SubClass, MsgBaseType>::RegistProcessFun
		( void (ClassType::*pMsgProcessFun)( const MsgType*, size_t ) )
	{
		typedef DispatchClass CDispatchClass;
		if( MsgType::GetIdByType() >= GetMsgFunction().size() )
			GetMsgFunction().resize( MsgType::GetIdByType() + 1 );

		GetMsgFunction()[MsgType::GetIdByType()].first = &CheckMsg<CDispatchClass, MsgType>;
		GetMsgFunction()[MsgType::GetIdByType()].second = (MsgProcessFun_t)pMsgProcessFun;
		GetMsgFunction()[MsgType::GetIdByType()].szName = MsgType::GetName();
		GetMsgFunction()[MsgType::GetIdByType()].nHeadSize = sizeof( MsgType );
	}

	template<typename DispatchClass, typename IdType_t, typename SubClass, typename MsgBaseType>
	Gamma::TDispatch<DispatchClass, IdType_t, SubClass, MsgBaseType>::TDispatch( uint32_t nMaxLen /*= INVALID_32BITID */ )
	{
		m_nMaxLen = nMaxLen;
	}
}
