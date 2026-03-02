//=========================================================================
// INetHandler.h 
// 定义网络接收Buffer类
// 柯达昭
// 2007-12-05
//=========================================================================
#ifndef __GAMMA_INETHANDLER_H__
#define __GAMMA_INETHANDLER_H__

namespace Gamma
{
    enum ECloseType
    {
        eCE_Null,
        eCE_NoBuffer,
        eCE_AddressReuse,
        eCE_ConnectRefuse,
        eCE_TimeOut,
        eCE_Unreach,
        eCE_InvalidAddress,
        eCE_Reset,						///< 以上为非正常错误关闭
		eCE_ShutdownImmediatly,			///< 立刻关闭无视一切，底层设置
		eCE_ShutdownOnCheck,			///< 延后关闭到Check，底层设置
		eCE_NormalClose,				///< 对方的正常关闭，底层设置
		eCE_GraceClose,					///< 应用层的命令关闭，应用层设置，优雅关闭
		eCE_ForceClose,					///< 应用层的命令关闭，应用层设置，强制关闭
        eCE_Unknown = -1
    };

    class IConnecter;

    class IListenHandler
    {
    public:
        virtual void OnAccept( IConnecter& Connect ) = 0;
    };

    class IConnectHandler
    {
    public:
        virtual void    OnConnected() = 0;
        virtual void    OnDisConnect( ECloseType eCloseType ) = 0;
        virtual size_t  OnRecv( const char* pBuf, size_t nSize ) = 0;
    };
}

#endif
