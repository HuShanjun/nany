#include "ConnFromClient.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/pystring.h"
#include "TableData.gen/DataDefine.h"
#include "ShmApp.h"
#include <cstdint>
#include <format>

using namespace Gamma;

DEFINE_DYNAMIC_CLASS(ConnFromClient, 2048, GET_CLASS_ID(ConnFromClient))

size_t ConnFromClient::OnShellMsg(const void *pCmd, size_t nSize, bool bUnreliable) {
    GammaLog << "ConnFromClient::OnShellMsg " << GetRemoteAddress() << " | " << GetLocalAddress()
             << " " << nSize << std::endl;
    std::string data((const char *)pCmd, nSize);
    data = pystring::strip(data);
    // 去掉首尾空白符
    GammaLog << "data : " << data << std::endl;
    if (data.starts_with("ping")) {
        SendShellMsg("pong", 4, false);
        return nSize;
    } else if (data == "alloc player") {
        static uint32_t player_id = 0;
        ++player_id;
        CPlayerDataDefineShm player_data;
        player_data.m_nChunckID = player_id;
        auto* char_base = player_data.GetCharBase();
        char_base->SetDataVersion(1, true);
        char_base->SetGasID(1, true);
        char_base->SetCurSceneUUID(1, true);
        char_base->SetLastPosX(1, true);
        char_base->SetLastPosY(1, true);
        char_base->SetCurClassSlot(1, true);
        uint16_t talent_slot[eCL_TalentSlot] = {1, 2, 3, 4};
        char_base->SetTalentSlot(talent_slot, true);
        uint8_t unlock_portrait_array[eCL_UnlockPortraitArray] = {1, 2, 3, 4};
        char_base->SetUnlockPortraitArray(unlock_portrait_array, true);
        player_data.GetBagItem()->SetBagItemCurNum(100);

        auto app = GetGameApp<CShmApp>();
        auto char_data_mgr = app->GetCharDataMgr();
        auto shm_data = char_data_mgr->AllocalPlayerData(player_id, &player_data);
        CPlayerDataDefine* pDefine = new CPlayerDataDefine(player_id, shm_data);
        m_mapPlayerDefine.emplace(player_id, pDefine);
        SendFormat("success: alloc player {} \r\n", player_id);
        return nSize;
    } else if (data.starts_with("set charbase")) {
        std::vector<std::string> args;
        pystring::split(data, args);
        if (args.size() < 3) {
            SendFormat("error: invalid command \r\n");
            return nSize;
        }
        auto player_id = std::stoi(args[2]);
        auto player_define = m_mapPlayerDefine.find(player_id);
        if (player_define != m_mapPlayerDefine.end()) {
            player_define->second->GetCharBase()->SetDataVersion(15);
        }
    }

    return nSize;
}

void ConnFromClient::OnConnected() {
    const CAddress &remote_address = GetRemoteAddress();
    const CAddress &local_address = GetLocalAddress();
    GammaLog << "ConnFromClient::OnConnected " << remote_address << " | " << local_address
             << std::endl;
    return;
}

void ConnFromClient::OnDisConnect() {
    GammaLog << "ConnFromClient::OnDisConnect " << GetRemoteAddress() << " | " << GetLocalAddress()
             << std::endl;
    return;
}

void ConnFromClient::SendString(const std::string& msg) {
    SendShellMsg(msg.c_str(), msg.size(), false);
}