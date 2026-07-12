
#include "TableData.Gen/PlayerDataTable.h"
#include "Help/ShmHelp.h"
#include "GammaCommon/GammaHelp.h"
#include "Interface/IGameApp.h"

extern IGameAppPtr g_pApp;

CSevenDayLogin::CSevenDayLogin(bool bSaveDB) : SDBBlockData(ePP_SevenDayLogin, sizeof(*this), bSaveDB) {
    memset((void *)((SDBBlockData *)(this) + 1), 0, sizeof(*this) - sizeof(SDBBlockData));
}

void CSevenDayLogin::CopyFrom(const CSevenDayLogin *pOther, bool bSimple) {
    SetUUID(pOther->GetUUID(), bSimple);
    SetLoginDay(pOther->GetLoginDay(), bSimple);
    SetHasGetDay(pOther->GetHasGetDay(), bSimple);
}

void CSevenDayLogin::CheckOrSetUUID() {}
void CSevenDayLogin::SynToOther() {
    SynValueData(m_pDataContainter, m_UUID, m_nDataTableID, 1, m_LoginDay, sizeof(m_LoginDay));
    SynValueData(m_pDataContainter, m_UUID, m_nDataTableID, 2, m_HasGetDay, sizeof(m_HasGetDay));
}

std::ostream &CSevenDayLogin::Log(std::ostream &ostr) const {
    ostr << ":UUID=" << m_UUID << ",LoginDay=" << (int32_t)m_LoginDay << ",HasGetDay=" << (int32_t)m_HasGetDay;
    return ostr;
}

int32_t CSevenDayLogin::Pack(CBufFileEx &buffer) const {
    uint32_t nOldPos = buffer.GetPos();
    buffer << m_UUID;
    buffer << m_LoginDay;
    buffer << m_HasGetDay;

    return buffer.GetPos() - nOldPos;
}

bool CSevenDayLogin::UnPackOneField(CBufFileEx &buffer, uint16_t fieldID) {
    if (fieldID == 0xffff) {
        UnPack(buffer);
    }
    switch (fieldID) {
    case 0:
        buffer >> m_UUID;
        return true;
    case 1:
        buffer >> m_LoginDay;
        return true;
    case 2:
        buffer >> m_HasGetDay;
        return true;
    default:
        return false;
    }
}
void CSevenDayLogin::UnPack(CBufFileEx &buffer) {
    buffer >> m_UUID;
    buffer >> m_LoginDay;
    buffer >> m_HasGetDay;
}

bool CSevenDayLogin::SynField(uint8_t synType, uint16_t fieldID, const void *pData, uint32_t nSize, uint32_t nPos) {
    if (synType != eCTOp_Update)
        return false;
    CBufFileEx buffer((tbyte *)pData, nSize);
    buffer.Seek(nPos);
    return UnPackOneField(buffer, fieldID);
}

int32_t CSevenDayLogin::SetLoginDay(uint8_t value, bool bSimple) {
    if (m_LoginDay == value)
        return -1;
    m_LoginDay = value;
    if (bSimple || !m_pDataContainter)
        return 1;
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Update, m_nDataTableID, m_UUID, 1);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 1, eCTOp_Update, &m_LoginDay, (uint32_t)sizeof(m_LoginDay));
    SynValueData(m_pDataContainter, m_UUID, m_nDataTableID, 1, m_LoginDay, sizeof(m_LoginDay));

    return 1;
}

int32_t CSevenDayLogin::SetHasGetDay(uint8_t value, bool bSimple) {
    if (m_HasGetDay == value)
        return -1;
    m_HasGetDay = value;
    if (bSimple || !m_pDataContainter)
        return 2;
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Update, m_nDataTableID, m_UUID, 2);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 2, eCTOp_Update, &m_HasGetDay,
                (uint32_t)sizeof(m_HasGetDay));
    SynValueData(m_pDataContainter, m_UUID, m_nDataTableID, 2, m_HasGetDay, sizeof(m_HasGetDay));

    return 2;
}

CCharBase::CCharBase(bool bSaveDB) : SDBBlockData(ePP_CharBase, sizeof(*this), bSaveDB) {
    memset((void *)((SDBBlockData *)(this) + 1), 0, sizeof(*this) - sizeof(SDBBlockData));
}

void CCharBase::CopyFrom(const CCharBase *pOther, bool bSimple) {
    SetUUID(pOther->GetUUID(), bSimple);
    SetDataVersion(pOther->GetDataVersion(), bSimple);
    SetGasID(pOther->GetGasID(), bSimple);
    SetCurSceneUUID(pOther->GetCurSceneUUID(), bSimple);
    SetLastPosX(pOther->GetLastPosX(), bSimple);
    SetLastPosY(pOther->GetLastPosY(), bSimple);
    SetCurClassSlot(pOther->GetCurClassSlot(), bSimple);
    SetTalentSlot(pOther->GetTalentSlot(), bSimple);
    SetUnlockPortraitArray(pOther->GetUnlockPortraitArray(), bSimple);
}

void CCharBase::CheckOrSetUUID() {}

void CCharBase::SynToOther() {
    SynValueData(m_pDataContainter, m_UUID, m_nDataTableID, 1, m_DataVersion, sizeof(m_DataVersion));
    SynValueData(m_pDataContainter, m_UUID, m_nDataTableID, 6, m_CurClassSlot, sizeof(m_CurClassSlot));
}

std::ostream &CCharBase::Log(std::ostream &ostr) const {
    ostr << ":UUID=" << m_UUID << ",DataVersion=" << m_DataVersion << ",GasID=" << m_GasID
         << ",CurSceneUUID=" << m_CurSceneUUID << ",LastPosX=" << m_LastPosX << ",LastPosY=" << m_LastPosY
         << ",CurClassSlot=" << (int32_t)m_CurClassSlot << ",TalentSlot=" << &m_TalentSlot
         << ",UnlockPortraitArray=" << &m_UnlockPortraitArray;
    return ostr;
}

int32_t CCharBase::Pack(CBufFileEx &buffer) const {
    uint32_t nOldPos = buffer.GetPos();
    buffer << m_UUID;
    buffer << m_DataVersion;
    buffer << m_GasID;
    buffer << m_CurSceneUUID;
    buffer << m_LastPosX;
    buffer << m_LastPosY;
    buffer << m_CurClassSlot;
    buffer << m_TalentSlot;
    buffer << m_UnlockPortraitArray;
    return buffer.GetPos() - nOldPos;
}

bool CCharBase::UnPackOneField(CBufFileEx &buffer, uint16_t fieldID) {
    if (fieldID == 0xffff) {
        UnPack(buffer);
    }
    switch (fieldID) {
    case 0: buffer >> m_UUID; return true;
    case 1: buffer >> m_DataVersion; return true;
    case 2: buffer >> m_GasID; return true;
    case 3: buffer >> m_CurSceneUUID; return true;
    case 4: buffer >> m_LastPosX; return true;
    case 5: buffer >> m_LastPosY; return true;
    case 6: buffer >> m_CurClassSlot; return true;
    case 7: buffer >> m_TalentSlot; return true;
    case 8: buffer >> m_UnlockPortraitArray; return true;
    default: return false;
    }
}

void CCharBase::UnPack(CBufFileEx &buffer) {
    buffer >> m_UUID;
    buffer >> m_DataVersion;
    buffer >> m_GasID;
    buffer >> m_CurSceneUUID;
    buffer >> m_LastPosX;
    buffer >> m_LastPosY;
    buffer >> m_CurClassSlot;
    buffer >> m_TalentSlot;
    buffer >> m_UnlockPortraitArray;
}

bool CCharBase::SynField(uint8_t synType, uint16_t fieldID, const void *pData, uint32_t nSize, uint32_t nPos) {
    if (synType != eCTOp_Update)
        return false;
    CBufFileEx buffer((tbyte *)pData, nSize);
    buffer.Seek(nPos);
    return UnPackOneField(buffer, fieldID);
}

int32_t CCharBase::SetDataVersion(uint16_t value, bool bSimple) {
    if (m_DataVersion == value)
        return -1;
    m_DataVersion = value;
    if (bSimple || !m_pDataContainter)
        return 1;
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Update, m_nDataTableID, m_UUID, 1);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 1, eCTOp_Update, &m_DataVersion, (uint32_t)sizeof(m_DataVersion));
    SynValueData(m_pDataContainter, m_UUID, m_nDataTableID, 1, m_DataVersion, sizeof(m_DataVersion));
    return 1;
}

int32_t CCharBase::SetGasID(uint16_t value, bool bSimple) {
    if (m_GasID == value)
        return -1;
    m_GasID = value;
    if (bSimple || !m_pDataContainter)
        return 2;
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Update, m_nDataTableID, m_UUID, 2);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 2, eCTOp_Update, &m_GasID, (uint32_t)sizeof(m_GasID));
    return 2;
}

int32_t CCharBase::SetCurSceneUUID(int64_t value, bool bSimple) {
    if (m_CurSceneUUID == value)
        return -1;
    m_CurSceneUUID = value;
    if (bSimple || !m_pDataContainter)
        return 3;
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Update, m_nDataTableID, m_UUID, 3);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 3, eCTOp_Update, &m_CurSceneUUID, (uint32_t)sizeof(m_CurSceneUUID));
    return 3;
}

int32_t CCharBase::SetLastPosX(int32_t value, bool bSimple) {
    if (m_LastPosX == value)
        return -1;
    m_LastPosX = value;
    if (bSimple || !m_pDataContainter)
        return 4;
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Update, m_nDataTableID, m_UUID, 4);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 4, eCTOp_Update, &m_LastPosX, (uint32_t)sizeof(m_LastPosX));
    return 4;
}

int32_t CCharBase::SetLastPosY(int32_t value, bool bSimple) {
    if (m_LastPosY == value)
        return -1;
    m_LastPosY = value;
    if (bSimple || !m_pDataContainter)
        return 5;
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Update, m_nDataTableID, m_UUID, 5);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 5, eCTOp_Update, &m_LastPosY, (uint32_t)sizeof(m_LastPosY));
    return 5;
}

int32_t CCharBase::SetCurClassSlot(uint8_t value, bool bSimple) {
    if (m_CurClassSlot == value)
        return -1;
    m_CurClassSlot = value;
    if (bSimple || !m_pDataContainter)
        return 6;
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Update, m_nDataTableID, m_UUID, 6);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 6, eCTOp_Update, &m_CurClassSlot, (uint32_t)sizeof(m_CurClassSlot));
    SynValueData(m_pDataContainter, m_UUID, m_nDataTableID, 6, m_CurClassSlot, sizeof(m_CurClassSlot));
    return 6;
}

uint16_t CCharBase::GetTalentSlotByIndex(uint32_t nIndex) const {
    if (nIndex >= ELEM_COUNT(m_TalentSlot))
        return 0;
    return m_TalentSlot[nIndex];
}

int32_t CCharBase::SetTalentSlotByIndex(uint16_t value, uint32_t nIndex, bool bSimple) {
    if (nIndex >= ELEM_COUNT(m_TalentSlot))
        return -1;
    if (m_TalentSlot[nIndex] == value)
        return 7;
    m_TalentSlot[nIndex] = value;
    if (bSimple || !m_pDataContainter)
        return 7;
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Update, m_nDataTableID, m_UUID, 7);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 7, eCTOp_Update, &m_TalentSlot, (uint32_t)sizeof(m_TalentSlot));
    return 7;
}

int32_t CCharBase::SetTalentSlot(const uint16_t *pValue, bool bSimple) {
    if (!memcmp(&m_TalentSlot[0], pValue, sizeof(m_TalentSlot)))
        return -1;
    memcpy(&m_TalentSlot[0], pValue, sizeof(m_TalentSlot));
    if (bSimple || !m_pDataContainter)
        return 7;
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Update, m_nDataTableID, m_UUID, 7);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 7, eCTOp_Update, &m_TalentSlot, (uint32_t)sizeof(m_TalentSlot));
    return 7;
}

void CCharBase::SyncTalentSlot() {
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 7, eCTOp_Update, &m_TalentSlot, (uint32_t)sizeof(m_TalentSlot));
}

uint8_t CCharBase::GetUnlockPortraitArrayByIndex(uint32_t nIndex) const {
    if (nIndex >= ELEM_COUNT(m_UnlockPortraitArray))
        return 0;
    return m_UnlockPortraitArray[nIndex];
}

int32_t CCharBase::SetUnlockPortraitArrayByIndex(uint8_t value, uint32_t nIndex, bool bSimple) {
    if (nIndex >= ELEM_COUNT(m_UnlockPortraitArray))
        return -1;
    if (m_UnlockPortraitArray[nIndex] == value)
        return 8;
    m_UnlockPortraitArray[nIndex] = value;
    if (bSimple || !m_pDataContainter)
        return 8;
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Update, m_nDataTableID, m_UUID, 8);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 8, eCTOp_Update, &m_UnlockPortraitArray,
                (uint32_t)sizeof(m_UnlockPortraitArray));
    return 8;
}

int32_t CCharBase::SetUnlockPortraitArray(const uint8_t *pValue, bool bSimple) {
    if (!memcmp(&m_UnlockPortraitArray[0], pValue, sizeof(m_UnlockPortraitArray)))
        return -1;
    memcpy(&m_UnlockPortraitArray[0], pValue, sizeof(m_UnlockPortraitArray));
    if (bSimple || !m_pDataContainter)
        return 8;
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Update, m_nDataTableID, m_UUID, 8);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 8, eCTOp_Update, &m_UnlockPortraitArray,
                (uint32_t)sizeof(m_UnlockPortraitArray));
    return 8;
}

void CCharBase::SyncUnlockPortraitArray() {
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 8, eCTOp_Update, &m_UnlockPortraitArray,
                (uint32_t)sizeof(m_UnlockPortraitArray));
}

CBagItemArray::CBagItemArray(bool bSaveDB) : SDBBlockData(ePP_BagItem, sizeof(*this), bSaveDB) {
    for (uint32_t i = 0; i < MAX_COUNT_BAGITEM; ++i)
        m_BagItem[i].SetZeroValue();
}
void CBagItemArray::ResetMaskID(uint32_t nBeginIndex) {
    uint64_t nBeginId = (m_UUID << 16);
    for (uint32_t i = nBeginIndex; i < m_nBagItemCount.m_nSize; ++i) {
        m_BagItem[i].m_UUID = nBeginId + i;
        m_BagItem[i].m_MaskID = m_UUID;
    }
}
void CBagItemArray::CheckData() {
    if (!IsShmTable())
        return;
    for (uint32_t i = 0; i < GetBagItemMaxCount(); ++i) {
        SBagItem &data = m_BagItem[i];
        if (data.m_UUID != 0 && (data.m_UUID >> 16) != GetUUID())
            GammaThrow("Error");
    }
}

// bReset 数量也重置，一般逻辑中=false
void CBagItemArray::SetZeroValue(bool bReset) {
    if (bReset) {
        m_nBagItemCount.m_nSize = 0;
        memset(m_BagItem, 0, sizeof(SBagItem) * MAX_COUNT_BAGITEM);
    } else {
        for (uint32_t i = 0; i < m_nBagItemCount.m_nSize; ++i)
            m_BagItem[i].SetZeroValue();
    }
    CheckData();
}
bool CBagItemArray::SetBagItemCurNum(uint32_t nNum) {
    if (nNum > (uint32_t)MAX_COUNT_BAGITEM)
        return false;
    m_nBagItemCount.m_nSize = nNum;
    if (m_pDataContainter)
        SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, 0, eCTOp_Assign, &m_nBagItemCount.m_nSize,
                    (uint32_t)sizeof(m_nBagItemCount.m_nSize));
    return true;
}
SBagItem *CBagItemArray::GetBagItem(uint32_t nIndex) {
    if (nIndex >= m_nBagItemCount.m_nSize)
        return nullptr;
    return &m_BagItem[nIndex];
}

SBagItem *CBagItemArray::SetBagItem(const SBagItem *_pProp, int16_t nIndex) {
    if (!_pProp || nIndex < 0 || (uint32_t)nIndex >= m_nBagItemCount.m_nSize)
        return nullptr;

    SBagItem &data = m_BagItem[nIndex];
    uint32_t nLen = sizeof(SBagItem) - sizeof(SElemBase);
    if (!memcmp((const tbyte *)&data + sizeof(SElemBase), (const tbyte *)_pProp + sizeof(SElemBase), nLen))
        return &data;
    memcpy((tbyte *)&data + sizeof(SElemBase), (const tbyte *)_pProp + sizeof(SElemBase), nLen);
    data.m_UUID = (m_UUID << 16) + (uint64_t)nIndex;
    data.m_MaskID = m_UUID;
    data.CheckOrSetUUID();
    if (!m_pDataContainter)
        return &data;
    CheckData();
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Update, m_nDataTableID, m_UUID, nIndex);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, nIndex, eCTOp_Update, &data, (uint32_t)sizeof(data));
    if (data.IsSynClient())
        SynFixData(m_pDataContainter, m_UUID, m_nDataTableID, nIndex, &data, (uint32_t)sizeof(data));
    return &data;
}

SBagItem *CBagItemArray::AddBagItem(const SBagItem *_pProp, int16_t nIndex) {
    int16_t nCurNum = (int16_t)m_nBagItemCount.m_nSize;
    if (nIndex < 0 || nIndex > nCurNum)
        nIndex = nCurNum;
    if (!_pProp || nIndex >= GetBagItemMaxCount())
        return nullptr;
    if (nIndex < nCurNum) {
        size_t nSize = sizeof(SBagItem) * (m_nBagItemCount.m_nSize - nIndex);
        memmove(m_BagItem + nIndex + 1, m_BagItem + nIndex, nSize);
    }
    SBagItem &data = m_BagItem[nIndex];
    data = *_pProp;
    m_nBagItemCount.m_nSize++;
    ResetMaskID(nIndex);
    data.CheckOrSetUUID();
    if (!m_pDataContainter)
        return &data;
    CheckData();
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Insert, m_nDataTableID, m_UUID, nIndex);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, nIndex, eCTOp_Insert, &data, (uint32_t)sizeof(data));
    if (m_BagItem[nIndex].IsSynClient())
        SynFixData(m_pDataContainter, m_UUID, m_nDataTableID, nIndex, &data, (uint32_t)sizeof(data));
    return &data;
}

bool CBagItemArray::DelBagItem(int16_t nIndex) {
    if (nIndex < 0 || (uint32_t)nIndex >= m_nBagItemCount.m_nSize)
        return false;
    m_nBagItemCount.m_nSize--;
    if (nIndex != m_nBagItemCount.m_nSize) {
        uint64_t nUUID = m_BagItem[nIndex].m_UUID;
        memcpy(m_BagItem + nIndex, m_BagItem + m_nBagItemCount.m_nSize, sizeof(SBagItem));
        m_BagItem[nIndex].m_UUID = nUUID;
    }
    auto &data = m_BagItem[m_nBagItemCount.m_nSize];
    data.SetZeroValue();
    if (!m_pDataContainter)
        return true;
    CheckData();
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Delete, m_nDataTableID, m_UUID, nIndex);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, nIndex, eCTOp_Delete, this, sizeof(*this));
    if (m_BagItem[nIndex].IsSynClient())
        SynFixData(m_pDataContainter, m_UUID, m_nDataTableID, -nIndex, &m_BagItem[nIndex],
                   (uint32_t)sizeof(m_BagItem[nIndex]));
    return true;
}

void CBagItemArray::CopyFrom(const CBagItemArray *pOther, bool bSimple) {
    m_nBagItemCount = pOther->m_nBagItemCount;
    for (uint32_t nIndex = 0; nIndex < m_nBagItemCount.m_nSize; nIndex++)
        m_BagItem[nIndex].CopyFrom(&pOther->m_BagItem[nIndex], bSimple);
    CheckData();
}

bool CBagItemArray::Submit(int16_t nIndex) {
    if (nIndex < 0 || (uint32_t)nIndex >= m_nBagItemCount.m_nSize)
        return false;
    SBagItem &data = m_BagItem[nIndex];
    data.m_UUID = (m_UUID << 16) + (uint64_t)nIndex;
    data.m_MaskID = m_UUID;
    if (!m_pDataContainter)
        return true;
    CheckData();
    if (IsSaveDB()) {
        if (IsShmTable())
            UpdateShmData(m_pDataContainter, this, 0);
        else
            m_pDataContainter->UpdateDB(eCTOp_Update, m_nDataTableID, m_UUID, nIndex);
    }
    SynSelfData(m_pDataContainter, m_UUID, m_nDataTableID, nIndex, eCTOp_Update, &data, (uint32_t)sizeof(data));
    if (data.IsSynClient())
        SynFixData(m_pDataContainter, m_UUID, m_nDataTableID, nIndex, &data, (uint32_t)sizeof(data));
    return true;
}

bool CBagItemArray::SynClient(int16_t nIndex, bool bSyn) {
    if (nIndex < 0 || (uint32_t)nIndex >= m_nBagItemCount.m_nSize)
        return false;
    SBagItem &data = m_BagItem[nIndex];
    data.SetSynClient(bSyn);
    if (m_pDataContainter)
        SynFixData(m_pDataContainter, m_UUID, m_nDataTableID, nIndex, &data, (uint32_t)sizeof(data));
    return true;
}

void CBagItemArray::SynToOther() {
    if (!m_pDataContainter)
        return;
    for (uint32_t nIndex = 0; nIndex < m_nBagItemCount.m_nSize; nIndex++) {
        if (m_BagItem[nIndex].IsSynClient())
            SynFixData(m_pDataContainter, m_UUID, m_nDataTableID, nIndex, &m_BagItem[nIndex],
                       (uint32_t)sizeof(m_BagItem[nIndex]));
    }
}
std::ostream &CBagItemArray::Log(std::ostream &ostr) const {
    ostr << "EleNum=" << m_nBagItemCount.m_nSize << std::endl;
    for (uint32_t i = 0; i < m_nBagItemCount.m_nSize; i++) {
        ostr << "[" << i << "]";
        m_BagItem[i].Log(ostr) << std::endl;
    }
    return ostr;
}

int32_t CBagItemArray::Pack(CBufFileEx &buffer) const {
    uint32_t nOldPos = buffer.GetPos();
    buffer << m_UUID;
    buffer << m_nBagItemCount.m_nSize;
    for (uint32_t i = 0; i < m_nBagItemCount.m_nSize; i++) {
        m_BagItem[i].Pack(buffer);
    }
    return buffer.GetPos() - nOldPos;
}

void CBagItemArray::UnPack(CBufFileEx &buffer) {
    buffer >> m_UUID;
    buffer >> m_nBagItemCount.m_nSize;
    for (uint32_t i = 0; i < m_nBagItemCount.m_nSize; i++) {
        m_BagItem[i].UnPack(buffer);
    }
}

bool CBagItemArray::SynField(uint8_t synType, uint16_t fieldID, const void *pData, uint32_t nSize, uint32_t nPos) {
    SBagItem newData;
    if (nSize > 0) {
        CBufFileEx buffer((tbyte *)pData, nSize);
        buffer.Seek(nPos);
        if (synType == eCTOp_Assign) {
            uint16_t nArraySize = 0;
            buffer >> nArraySize;
            SetBagItemCurNum(nArraySize);
            return true;
        } else {
            int32_t m_OpMask;
            buffer >> m_OpMask; // todo:temp
            newData.UnPack(buffer);
        }
    }
    switch (synType) {
    case eCTOp_Update:
        return SetBagItem(&newData, fieldID);
    case eCTOp_Insert:
        return AddBagItem(&newData, fieldID);
    case eCTOp_Delete:
        return DelBagItem(fieldID);
    default:
        return false;
    }
}

SBagItem *CBagItemArray::QueryByMaskID(int64_t MaskID) {
    int nCnt = GetBagItemCurNum();
    for (int i = 0; i < nCnt; i++) {
        SBagItem &curElem = m_BagItem[i];
        if (curElem.m_MaskID == MaskID) {
            return &curElem;
        }
    }
    return nullptr;
}

SBagItem *CBagItemArray::QueryByItemUUID(uint64_t ItemUUID) {
    int nCnt = GetBagItemCurNum();
    for (int i = 0; i < nCnt; i++) {
        SBagItem &curElem = m_BagItem[i];
        if (curElem.m_MaskID > 0 && curElem.m_ItemUUID == ItemUUID) {
            return &curElem;
        }
    }
    return nullptr;
}

void *CBagItemArray::QueryAllDataByItemID(int32_t ItemID) {
    return nullptr;
}
