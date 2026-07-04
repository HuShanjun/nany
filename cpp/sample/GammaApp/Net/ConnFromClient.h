#pragma once

#include "GammaConnects/CBaseConn.h"
#include <cstddef>

class CPlayerDataDefine;
class ConnFromClient : public Gamma::CBaseConn
{
    DECLARE_DYNAMIC_CLASS( ConnFromClient );
protected:
    virtual size_t OnShellMsg(const void* pCmd, size_t nSize, bool bUnreliable) override;
    virtual void	OnConnected() override;									/// 连接成功
    virtual void	OnDisConnect() override;

    void SendString(const std::string& msg);

    template<typename... Args>
    void SendFormat(std::format_string<Args...> fmt, Args&&... args){
        std::string response = std::format(fmt, std::forward<Args>(args)...);
        SendString(response);
    }
    
private:/// 开始断开
    std::map<uint32_t, CPlayerDataDefine*> m_mapPlayerDefine;
};