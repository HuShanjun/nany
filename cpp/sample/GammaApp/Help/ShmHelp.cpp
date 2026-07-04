#include "ShmHelp.h"
#include <ShmApp.h>

void UpdateShmData(const CDataContainer* pContainter, void* pData, uint32 nOffset) {
    // TODO: implement
    auto shmApp = GetGameApp<CShmApp>();
    auto char_data_mgr = shmApp->GetCharDataMgr();
    char_data_mgr->UpdateShmData(pContainter, pData, nOffset);
}

void SynSelfData( CDataContainer* pDataObserver, int64 _DataUUID, uint16 _nTableID, uint16 _nFieldIndex, uint8 _eCTType, const void* _pValue, uint32 nSize ) {
    // TODO: implement
}

void SynValueData( const CDataContainer* pContainter, int64 _uuid, uint16 _nTableID, int16 _nFieldIndex, uint64 nValue, uint32 nDataSize ){
    // TODO: implement
}

void SynVarLenData(const CDataContainer* pContainter, int64_t uuid, uint16_t nTableID, uint16_t nFieldIndex, const void* _pValue, uint32 nSize) {
    // TODO: implement
}

void SynFixData( const CDataContainer* pContainter, int64 _uuid, uint16 _nTableID, int16 _nFieldIndex, const void* _pValue, uint32 nSize ){
    // TODO: implement
}

uint64 GenDBUUID(){
    // TODO: implement
    static int64 s_nUUID = 0;
    s_nUUID++;
    return s_nUUID;
}