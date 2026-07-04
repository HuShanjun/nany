#pragma once
#include "GammaCommon/GammaHelp.h"
#include "GammaShm/CBaseShareMemory.h"
// #include "Interface/uuid.h"
#include <cstdint>

#if (defined(_WIN32))
#if defined(SHELLCOMMON_DLL)
#define SHELLCOMMON_API __declspec(dllexport)
#else
#define SHELLCOMMON_API __declspec(dllimport)
#endif
#else
#define SHELLCOMMON_API
#endif

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using tbyte = uint8_t;

//===================================================
// 定义玩家属性类型
//===================================================
using namespace Gamma;

#define MAX_DBBUFF 1024 * 128
const int32_t MAX_PLAYER_COUNT = 1200;

enum ETrunkType {
    eTrunkType_Char,
    eTrunkType_Count,
};

enum EDBOperate {
    eDBOp_Query,
    eDBOp_Insert,
    eDBOp_Update,
    eDBOp_Delete,
    eDBOp_Count,
};

// SBlockData表操作方式
enum EColTableOperate {
    eCTOp_None,
    eCTOp_Assign,
    eCTOp_Alloc,
    eCTOp_Release,
    eCTOp_Update,
    eCTOp_Insert,
    eCTOp_Delete,
    eCTOp_Count,
};

#pragma pack(push, 1)
class SDBBlockData : public Gamma::SBlockData {
public:
    SDBBlockData(uint16_t nDataTableID = 0, uint32_t nDataSize = 0, bool bSaveDB = false) :
        Gamma::SBlockData(nDataTableID, nDataSize, bSaveDB) {
    }
    bool Next(SDBBlockData *_pOut);
    void *GetData() {
        return (void *)(this + 1);
    }
    static uint32_t GetBaseDataSize() {
        return sizeof(SDBBlockData);
    }
};

// 纵表操作类型,最多32种
enum EElemDataOperater {
    eEO_SaveDB,
    eEO_SynClient,
};

struct SElemBase {
    int32_t m_OpMask;
    int64_t m_UUID; // uuid << 16 || index
    int64_t m_MaskID;

    SElemBase() : m_OpMask(0), m_UUID(0), m_MaskID(0) {
    }

    int64_t GetUUID() const {
        return m_UUID;
    }

    int32_t SetUUID(int64_t uuid, bool bSimple = false) {
        m_UUID = uuid;
        return 0;
    }

    int64_t GetMaskid() const {
        return m_MaskID;
    }
    int32_t SetMaskid(int64_t maskid, bool bSimple = false) {
        m_MaskID = maskid;
        return 1;
    }
    uint32_t GetOpMask() const {
        return m_OpMask;
    }
    bool HasOp(EElemDataOperater eType) const {
        return (m_OpMask & (1 << eType)) == (1 << eType);
    }
    void ChangeOp(EElemDataOperater eType, bool bSet) {
        if (bSet)
            m_OpMask |= (1 << eType);
        else
            m_OpMask &= ~(1 << eType);
    }
    uint32_t GetIndex() const {
        return (uint32_t)m_UUID;
    }
    void SetSaveDB(bool bValue) {
        ChangeOp(eEO_SaveDB, bValue);
    }
    bool IsSaveDB() const {
        return HasOp(eEO_SaveDB);
    }
    void SetSynClient(bool bValue) {
        ChangeOp(eEO_SynClient, bValue);
    }
    bool IsSynClient() const {
        return HasOp(eEO_SynClient);
    }
};
#pragma pack(pop)

class CDBResultHandler {
    uint32_t m_nID;

public:
    CDBResultHandler();
    virtual ~CDBResultHandler();
    virtual void OnDbResult(int64_t nUUID, int32_t _nResult, uint8_t _eType, uint32_t nVersion) = 0;
    uint32_t GetID() {
        return m_nID;
    }
};

// SetBlockInfo(
// 				m_BlockInfo[ePP_ClassBagItem],
// 				m_ClassBagItem,
// 				ePP_ClassBagItem,
// 				(const tbyte*)&m_ClassBagItem.GetClassBagItemSize(),
// 				m_ClassBagItem.GetClassBagItemMaxCount(),
// 				m_ClassBagItem.GetClassBagItemElemSize()
// );

// pDiffArrayStart 差分数组的起始位置 m_nClassBagItemCount;
template <typename PropClass>
inline void SetBlockInfo(Gamma::SBlockInfo &BlockInfo, PropClass &BlockData, int32_t eType,
                         const tbyte *pDiffArrayStart, size_t nArrayElemCount,
                         size_t nArrayElemSize) {
    BlockInfo.m_nType = eType;
    BlockInfo.m_nDataOffset =
        (uint32_t)((ptrdiff_t)(tbyte *)&BlockData) - (uint32_t)((ptrdiff_t)(tbyte *)&BlockInfo);
    BlockInfo.m_nMaxBlockSize = sizeof(PropClass);
    BlockInfo.m_nArrayStartOff =
        nArrayElemSize ? (uint32_t)(pDiffArrayStart - (tbyte *)&BlockData) : 0;
    BlockInfo.m_nArrayElemSize = (uint32_t)nArrayElemSize;
    BlockInfo.m_pContextData = NULL;
    BlockData.m_nQueryDataVersion = BlockData.m_nAnswerDataVersion = 0;
    BlockData.m_nBlockInfoIndex = (uint32_t)eType;

    // 如果注册diff数组，那么此数组一定要在最后
    GammaAst(nArrayElemSize == false
             || BlockInfo.m_nArrayStartOff + nArrayElemCount * nArrayElemSize + sizeof(SSizeType)
                    == sizeof(PropClass));

    // 一个数据块不能太大
    GammaAst(BlockInfo.m_nMaxBlockSize < MAX_DBBUFF);
}

// 游戏进程中使用的uuid，如npc
//  int64_t	GenGameUUID(EUUIDCRTTYPE nType);

void DumpShm();

uint64 GenDBUUID();

void UpdateShmData(const CDataContainer *pContainter, void *pData, uint32 nOffset);
void SynSelfData(CDataContainer *pDataObserver, int64 _DataUUID, uint16 _nTableID,
                 uint16 _nFieldIndex, uint8 _eCTType, const void *_pValue, uint32 nSize);
void SynValueData(const CDataContainer *pContainter, int64 _uuid, uint16 _nTableID,
                  int16 _nFieldIndex, uint64 nValue, uint32 nDataSize);
void SynVarLenData(const CDataContainer *pContainter, int64 _uuid, uint16 _nTableID,
                   int16 _nFieldIndex, const void *_pValue, uint32 nSize);
void SynFixData(const CDataContainer *pContainter, int64 _uuid, uint16 _nTableID,
                int16 _nFieldIndex, const void *_pValue, uint32 nSize);