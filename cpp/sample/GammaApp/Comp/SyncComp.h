#pragma once

#include "Interface/IComp.h"
#include "CompHelp.h"
#include "Help/ShmHelp.h"

class SyncComp : public IComp
{
public:
    DEFINE_COMP_ID(SyncComp, EAppCompID::eACID_SyncComp);

    SyncComp();
    ~SyncComp();

    void OnStarted() override {}
    void OnQuit() override {}

    void UpdateShmData(const CDataContainer *pContainter, void *pData, uint32 nOffset);
    void SynSelfData(CDataContainer *pDataObserver, int64 _DataUUID, uint16 _nTableID,
                     uint16 _nFieldIndex, uint8 _eCTType, const void *_pValue, uint32 nSize);
    void SynValueData(const CDataContainer *pContainter, int64 _uuid, uint16 _nTableID,
                      int16 _nFieldIndex, uint64 nValue, uint32 nDataSize);
    void SynVarLenData(const CDataContainer *pContainter, int64 _uuid, uint16 _nTableID,
                       int16 _nFieldIndex, const void *_pValue, uint32 nSize);
    void SynFixData(const CDataContainer *pContainter, int64 _uuid, uint16 _nTableID,
                    int16 _nFieldIndex, const void *_pValue, uint32 nSize);
};