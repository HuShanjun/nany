#pragma once

#include <cstdint>
#include <map>
#include <string>

struct SLoginTicket {
    uint32_t nTicketID = 0;
    std::string strAccount;
    uint8_t nState = 0; // 0=pending, 1=ok, 2=fail
};

class CLoginAuthStore {
public:
    uint32_t SubmitAuth(const std::string &account);
    bool CompleteAuth(uint32_t ticket, bool ok);
    bool HasTicket(uint32_t ticket) const;

private:
    uint32_t m_nNextTicket = 1;
    std::map<uint32_t, SLoginTicket> m_mapTickets;
};
