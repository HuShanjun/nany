#pragma once

#include "Help/ShmHelp.h"
#include "GammaCommon/CPkgFile.h"

enum EDataDefineType {
    eDDT_Player = 0,
    eDDT_Count,
};

enum EPlayerProperty {
    ePP_SevenDayLogin,
    ePP_CharBase,
    ePP_BagItem,
    ePP_Count,
};

enum ECharLength {
    eCL_AttachAttrs = 10,
    eCL_TalentSlot = 4,
    eCL_UnlockPortraitArray = 4,
};


#pragma pack(push, 1)
struct SBagItem : public SElemBase {
    int32 m_ItemID;
    uint64 m_ItemUUID;
    int32 m_ItemCount;
    uint32 m_AttachAttrs[eCL_AttachAttrs]; // 装备、防具的附加属性[attr1,val1,qua1,attr2,val2,qua2...]
    uint64 m_CDEndTime;                    // CD结束时间
    uint64 m_ExpireTime;                   // 限时物品过期时间

    SBagItem() : SElemBase() { SetZeroValue(); }

    inline size_t GetItemIDDataSize() const { return sizeof(m_ItemID); }
    inline int32 GetItemID() const { return m_ItemID; }
    inline int32 SetItemID(int32 value, bool bSimple = false) {
        if (m_ItemID == value)
            return -1;
        m_ItemID = value;
        return 2;
    }

    inline size_t GetItemUUIDDataSize() const { return sizeof(m_ItemUUID); }
    inline uint64 GetItemUUID() const { return m_ItemUUID; }
    inline int32 SetItemUUID(uint64 value, bool bSimple = false) {
        if (m_ItemUUID == value)
            return -1;
        m_ItemUUID = value;
        return 3;
    }

    inline size_t GetItemCountDataSize() const { return sizeof(m_ItemCount); }
    inline int32 GetItemCount() const { return m_ItemCount; }
    inline int32 SetItemCount(int32 value, bool bSimple = false) {
        if (m_ItemCount == value)
            return -1;
        m_ItemCount = value;
        return 4;
    }

    inline size_t GetAttachAttrsDataSize() const { return sizeof(m_AttachAttrs); }
    uint32 GetAttachAttrsByIndex(uint32 nIndex) const {
        if (nIndex >= ELEM_COUNT(m_AttachAttrs))
            return 0;
        return m_AttachAttrs[nIndex];
    }

    int32 SetAttachAttrsByIndex(uint32 value, uint32 nIndex) {
        if (nIndex >= ELEM_COUNT(m_AttachAttrs))
            return -1;
        m_AttachAttrs[nIndex] = value;
        return 5;
    }

    inline const uint32 *GetAttachAttrs() const { return m_AttachAttrs; }
    inline int32 SetAttachAttrs(const uint32 *pValue, bool bSimple = false) {
        if (!memcmp(&m_AttachAttrs[0], pValue, sizeof(m_AttachAttrs)))
            return -1;
        memcpy(&m_AttachAttrs[0], pValue, sizeof(m_AttachAttrs));
        return 5;
    }

    inline size_t GetCDEndTimeDataSize() const { return sizeof(m_CDEndTime); }
    inline uint64 GetCDEndTime() const { return m_CDEndTime; }
    inline int32 SetCDEndTime(uint64 value, bool bSimple = false) {
        if (m_CDEndTime == value)
            return -1;
        m_CDEndTime = value;
        return 6;
    }

    inline size_t GetExpireTimeDataSize() const { return sizeof(m_ExpireTime); }
    inline uint64 GetExpireTime() const { return m_ExpireTime; }
    inline int32 SetExpireTime(uint64 value, bool bSimple = false) {
        if (m_ExpireTime == value)
            return -1;
        m_ExpireTime = value;
        return 7;
    }

    void SetZeroValue() {
        memset((tbyte *)this + sizeof(SElemBase), 0, sizeof(SBagItem) - sizeof(SElemBase));
        SetMaskid(0);
    }

    void CopyFrom(const SBagItem *pOther, bool bSimple = false) {
        if (NULL == pOther)
            throw std::runtime_error("pOther is null");
        SetUUID(pOther->GetUUID(), bSimple);
        SetMaskid(pOther->GetMaskid(), bSimple);
        SetItemID(pOther->GetItemID(), bSimple);
        SetItemUUID(pOther->GetItemUUID(), bSimple);
        SetItemCount(pOther->GetItemCount(), bSimple);
        SetAttachAttrs(pOther->GetAttachAttrs(), bSimple);
        SetCDEndTime(pOther->GetCDEndTime(), bSimple);
        SetExpireTime(pOther->GetExpireTime(), bSimple);
    }

    std::ostream &Log(std::ostream &ostr) const {
        ostr << ":UUID=" << m_UUID << ",MaskID=" << m_MaskID << ",ItemID=" << m_ItemID << ",ItemUUID=" << m_ItemUUID
             << ",ItemCount=" << m_ItemCount << ",AttachAttrs=" << &m_AttachAttrs << ",CDEndTime=" << m_CDEndTime
             << ",ExpireTime=" << m_ExpireTime;
        return ostr;
    }
    int32 Pack(CBufFileEx &buffer) const {
        uint32 nOldPos = buffer.GetPos();
        buffer << m_UUID;
        buffer << m_MaskID;
        buffer << m_ItemID;
        buffer << m_ItemUUID;
        buffer << m_ItemCount;
        buffer << m_AttachAttrs;
        buffer << m_CDEndTime;
        buffer << m_ExpireTime;

        return buffer.GetPos() - nOldPos;
    }
    void UnPack(CBufFileEx &buffer) {
        buffer >> m_UUID;
        buffer >> m_MaskID;
        buffer >> m_ItemID;
        buffer >> m_ItemUUID;
        buffer >> m_ItemCount;
        buffer >> m_AttachAttrs;
        buffer >> m_CDEndTime;
        buffer >> m_ExpireTime;
    }

    void CheckOrSetUUID() {
        if (0 == m_ItemUUID)
            m_ItemUUID = GenDBUUID();
    }
    static constexpr uint32 GetMaxPackSize() { return (uint32)(sizeof(SBagItem)); }
};

#pragma pack(pop)
