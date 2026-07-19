#include "CLoginAuthStore.h"

uint32_t CLoginAuthStore::SubmitAuth(const std::string &account) {
    if (account.empty()) {
        return 0;
    }
    uint32_t id = m_nNextTicket++;
    SLoginTicket ticket;
    ticket.nTicketID = id;
    ticket.strAccount = account;
    ticket.nState = 0;
    m_mapTickets[id] = ticket;
    return id;
}

bool CLoginAuthStore::CompleteAuth(uint32_t ticket, bool ok) {
    auto it = m_mapTickets.find(ticket);
    if (it == m_mapTickets.end()) {
        return false;
    }
    it->second.nState = ok ? 1 : 2;
    return true;
}

bool CLoginAuthStore::HasTicket(uint32_t ticket) const {
    return m_mapTickets.find(ticket) != m_mapTickets.end();
}
