
#pragma once
#include "Help/ShmHelp.h"
#include "GammaCommon/CPkgFile.h"
#include "GammaShm/CBaseShareMemory.h"
#include "TableCommonDefine.h"
#pragma pack(push, 1)

class CSevenDayLogin : public SDBBlockData {
public:
    uint8_t m_LoginDay;  // 已经累积登录天数
    uint8_t m_HasGetDay; // 位值 初始0000000 领完1111111

public:
    CSevenDayLogin(bool bSaveDB = true);
    static bool HasDml() { return true; }
    static bool IsShmTable() { return true; }
    const char *GetTableName() { return "SevenDayLogin"; }
    static uint32_t GetSevenDayLoginDataSize() { return (uint32_t)(sizeof(CSevenDayLogin) - sizeof(SDBBlockData)); }
    void CopyFrom(const CSevenDayLogin *pOther, bool bSimple = false);
    void CheckOrSetUUID();
    void SynToOther();
    std::ostream &Log(std::ostream &ostr) const;
    int32_t Pack(CBufFileEx &buffer) const;
    bool UnPackOneField(CBufFileEx &buffer, uint16_t fieldID);
    void UnPack(CBufFileEx &buffer);
    bool SynField(uint8_t synType, uint16_t fieldID, const void *pData, uint32_t nSize, uint32_t nPos);
    inline void SetZeroValue() {
        memset(((tbyte *)this + sizeof(SDBBlockData)), 0, sizeof(*this) - sizeof(SDBBlockData));
    }
    static constexpr uint32_t GetMaxPackSize() { return (uint32_t)(sizeof(CSevenDayLogin) - sizeof(SDBBlockData)); };

    inline size_t GetLoginDayDataSize() const { return sizeof(m_LoginDay); }
    inline uint8_t GetLoginDay() const { return m_LoginDay; }
    int32_t SetLoginDay(uint8_t value, bool bSimple = false);

    inline size_t GetHasGetDayDataSize() const { return sizeof(m_HasGetDay); }
    inline uint8_t GetHasGetDay() const { return m_HasGetDay; }
    int32_t SetHasGetDay(uint8_t value, bool bSimple = false);
};

class CCharBase : public SDBBlockData {
public:
    uint16_t m_DataVersion;
    uint16_t m_GasID;
    int64_t m_CurSceneUUID;
    int32_t m_LastPosX;
    int32_t m_LastPosY;
    uint8_t m_CurClassSlot;
    uint16_t m_TalentSlot[eCL_TalentSlot];
    uint8_t m_UnlockPortraitArray[eCL_UnlockPortraitArray];

public:
    CCharBase(bool bSaveDB = true);
    static bool HasDml() { return true; }
    static bool IsShmTable() { return true; }
    const char *GetTableName() { return "CharBase"; }
    static uint32_t GetCharBaseDataSize() { return (uint32_t)(sizeof(CCharBase) - sizeof(SDBBlockData)); }
    void CopyFrom(const CCharBase *pOther, bool bSimple = false);
    void CheckOrSetUUID();
    void SynToOther();
    std::ostream &Log(std::ostream &ostr) const;
    int32_t Pack(CBufFileEx &buffer) const;
    bool UnPackOneField(CBufFileEx &buffer, uint16_t fieldID);
    void UnPack(CBufFileEx &buffer);
    bool SynField(uint8_t synType, uint16_t fieldID, const void *pData, uint32_t nSize, uint32_t nPos);
    inline void SetZeroValue() {
        memset(((tbyte *)this + sizeof(SDBBlockData)), 0, sizeof(*this) - sizeof(SDBBlockData));
    }
    static constexpr uint32_t GetMaxPackSize() { return (uint32_t)(sizeof(CCharBase) - sizeof(SDBBlockData)); };

    inline size_t GetDataVersionDataSize() const { return sizeof(m_DataVersion); }
    inline uint16_t GetDataVersion() const { return m_DataVersion; }
    int32_t SetDataVersion(uint16_t value, bool bSimple = false);

    inline size_t GetGasIDDataSize() const { return sizeof(m_GasID); }
    inline uint16_t GetGasID() const { return m_GasID; }
    int32_t SetGasID(uint16_t value, bool bSimple = false);

    inline size_t GetCurSceneUUIDDataSize() const { return sizeof(m_CurSceneUUID); }
    inline int64_t GetCurSceneUUID() const { return m_CurSceneUUID; }
    int32_t SetCurSceneUUID(int64_t value, bool bSimple = false);

    inline size_t GetLastPosXDataSize() const { return sizeof(m_LastPosX); }
    inline int32_t GetLastPosX() const { return m_LastPosX; }
    int32_t SetLastPosX(int32_t value, bool bSimple = false);

    inline size_t GetLastPosYDataSize() const { return sizeof(m_LastPosY); }
    inline int32_t GetLastPosY() const { return m_LastPosY; }
    int32_t SetLastPosY(int32_t value, bool bSimple = false);

    inline size_t GetCurClassSlotDataSize() const { return sizeof(m_CurClassSlot); }
    inline uint8_t GetCurClassSlot() const { return m_CurClassSlot; }
    int32_t SetCurClassSlot(uint8_t value, bool bSimple = false);

    inline size_t GetTalentSlotDataSize() const { return sizeof(m_TalentSlot); }
    uint16_t GetTalentSlotByIndex(uint32_t nIndex) const;
    int32_t SetTalentSlotByIndex(uint16_t nValue, uint32_t nIndex, bool bSimple = false);
    inline const uint16_t *GetTalentSlot() const { return &m_TalentSlot[0]; }
    int32_t SetTalentSlot(const uint16_t *pValue, bool bSimple = false);
    void SyncTalentSlot();

    inline size_t GetUnlockPortraitArrayDataSize() const { return sizeof(m_UnlockPortraitArray); }
    uint8_t GetUnlockPortraitArrayByIndex(uint32_t nIndex) const;
    int32_t SetUnlockPortraitArrayByIndex(uint8_t nValue, uint32_t nIndex, bool bSimple = false);
    inline const uint8_t *GetUnlockPortraitArray() const { return &m_UnlockPortraitArray[0]; }
    int32_t SetUnlockPortraitArray(const uint8_t *pValue, bool bSimple = false);
    void SyncUnlockPortraitArray();
};

class CBagItemArray : public SDBBlockData {
    enum {MAX_COUNT_BAGITEM = 100};
private:
    Gamma::SSizeType m_nBagItemCount;
    SBagItem m_BagItem[MAX_COUNT_BAGITEM];
    void CheckData();

public:
    CBagItemArray(bool bSaveDB = true);
    static bool HasDml() { return true; }
    static bool IsShmTable() { return true; }
    const char *GetTableName() { return "BagItem"; }
    static uint32_t GetBagItemElemSize() { return (uint32_t)sizeof(SBagItem); }
    static uint32_t GetBagItemMaxCount() { return (uint32_t)MAX_COUNT_BAGITEM; }
    static uint32_t GetBagItemDataSize() { return (uint32_t)(sizeof(SBagItem) * MAX_COUNT_BAGITEM); }
    void ResetMaskID(uint32_t nBeginIndex);
    void SetZeroValue(bool bReset = false);
    const Gamma::SSizeType &GetBagItemSize() const { return m_nBagItemCount; }
    uint32_t GetBagItemCurNum() const { return m_nBagItemCount.m_nSize; }
    bool SetBagItemCurNum(uint32_t nNum);
    uint32_t GetBagItemMaxNum() const { return (uint32_t)MAX_COUNT_BAGITEM; }
    SBagItem *GetBagItem(uint32_t nIndex);
    SBagItem *SetBagItem(const SBagItem *_pProp, int16_t nIndex);
    SBagItem *AddBagItem(const SBagItem *_pProp, int16_t nIndex);
    bool DelBagItem(int16_t nIndex);
    void CopyFrom(const CBagItemArray *pOther, bool bSimple = false);

    bool Submit(int16_t nIndex);
    bool SynClient(int16_t nIndex, bool bSyn);
    void SynToOther();
    std::ostream &Log(std::ostream &ostr) const;
    int32_t Pack(CBufFileEx &buffer) const;
    void UnPack(CBufFileEx &buffer);
    bool SynField(uint8_t synType, uint16_t fieldID, const void *pData, uint32_t nSize, uint32_t nPos);
    static constexpr uint32_t GetMaxPackSize() { return (uint32_t)(MAX_COUNT_BAGITEM * sizeof(SBagItem)); };

    SBagItem *QueryByMaskID(int64_t MaskID);
    SBagItem *QueryByItemUUID(uint64_t ItemUUID);
    void *QueryAllDataByItemID(int32_t ItemID);
};

#pragma pack(pop)