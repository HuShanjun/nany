#include "SyncComp.h"
#include "GammaCommon/ILog.h"
#include "ShmApp.h"

void SyncComp::UpdateShmData(const CDataContainer *pContainter, void *pData, uint32 nOffset) {
    auto pShmApp = GetGameApp<CShmApp>();
    if (!pShmApp) {
        LogWarn("[SyncComp::UpdateShmData] pShmApp is null")    ;
        return;
    }
    auto pCharDataMgr = pShmApp->GetCharDataMgr();
    if (!pCharDataMgr) {
        LogWarn("[SyncComp::UpdateShmData] pCharDataMgr is null");
        return;
    }
    pCharDataMgr->UpdateShmData(pContainter, pData, nOffset);
}

void SyncComp::SynSelfData(CDataContainer *pDataObserver, int64 _DataUUID, uint16 _nTableID,
                           uint16 _nFieldIndex, uint8 _eCTType, const void *_pValue, uint32 nSize) {
}

void SyncComp::SynValueData(const CDataContainer *pContainter, int64 _uuid, uint16 _nTableID,
                            int16 _nFieldIndex, uint64 nValue, uint32 nDataSize) {
}

void SyncComp::SynVarLenData(const CDataContainer *pContainter, int64 _uuid, uint16 _nTableID,
                             int16 _nFieldIndex, const void *_pValue, uint32 nSize) {
}

void SyncComp::SynFixData(const CDataContainer *pContainter, int64 _uuid, uint16 _nTableID,
                          int16 _nFieldIndex, const void *_pValue, uint32 nSize) {
}
