#include "CCharDataMgr.h"

#include <filesystem>
#include <format>

#include "GammaApp/IGameApp.h"
#include "GammaCommon/CPathMgr.h"
#include "GammaCommon/CPkgFile.h"
#include "GammaCommon/CThread.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/TGammaStrStream.h"
#include "Help/ShellTickEnum.h"
#include "PrtGameDbMsg.h"

namespace fs = std::filesystem;

extern IGameAppPtr g_pApp;

CCharDataMgr::CCharDataMgr() : m_pShmMgr(nullptr) {
    for (int32 i = 0; i < eTrunkType_Count; ++i) {
        SChunckTypeInfo &TrunkTypeInfo = m_GameShareFile.m_aryChunkTypeInfo[i];
        SCharDataUseInfo &DataInfo = m_CharDataInfo[i];

        DataInfo.m_aryCommitTrunk.resize(TrunkTypeInfo.m_nChunkCount);
        tbyte *pSrcInfoStart = (tbyte *)(&TrunkTypeInfo) + TrunkTypeInfo.m_nChunkOffset;
        SChunckHead *pTrunkHead = (SChunckHead *)(pSrcInfoStart);
        pTrunkHead->m_nDataState = eSCT_Invalid;
        pTrunkHead->m_nDataVersion = 0;
        for (uint32 j = 0; j < TrunkTypeInfo.m_nChunkCount; j++) {
            DataInfo.m_aryCommitTrunk[j].m_aryCommitBlockData.resize(pTrunkHead->m_nBlockCount);
            DataInfo.m_aryFreeTrunks.PushBack(DataInfo.m_aryCommitTrunk[j]);
        }
    }
}

CCharDataMgr::~CCharDataMgr() {
    g_pApp->UnRegister(this);
    SAFE_RELEASE(m_pShmMgr);
}

void CCharDataMgr::OnInit() {
}

void CCharDataMgr::Start() {
    int32 nServerID = 527;
    fs::path current_path = fs::current_path();
    fs::path shm_data_dir= fs::weakly_canonical(current_path / ".."/ "shm"/ "char");
    if (!fs::exists(shm_data_dir)) {
        fs::create_directories(shm_data_dir);
    }
    fs::path shm_data_file = fs::weakly_canonical(shm_data_dir / std::format("GameShareMemFile_{}", nServerID));

    bool bKeepShmFile = false; // 退出是否保持shm
    uint32 nCommitInterval = 30000;
    uint32 nFlushInterval = 10000;

    m_pShmMgr = CreateShareMemoryMgr(0, &m_GameShareFile.m_ShareFileHead, shm_data_file.string().c_str(), this,
                                     nCommitInterval, bKeepShmFile, nFlushInterval);
    m_pGameShareFile = static_cast<SGameShareFile *>((void *)m_pShmMgr->GetShareMemory());
    //	m_pShmMgr->SetCheckInterval(1000);
    m_pShmMgr->Start();
    g_pApp->Register(this, 33, eSST_CharDataCheck);
    g_pApp->OnReady();
}

void CCharDataMgr::DumpPlayerData() {
    GammaLog << "[CCharDataMgr] DumpPlayerData" << endl;
    SCharDataUseInfo &CharDataInfo = m_CharDataInfo[eTrunkType_Char];
    GammaLog << "[CCharDataMgr] DumpPlayerData CommitTrunkNum="
             << CharDataInfo.m_aryCommitTrunk.size() << endl;
    GammaLog << "[CCharDataMgr] DumpPlayerData UsingTrunkNum="
             << CharDataInfo.m_mapUsingTrunks.size() << endl;
    GammaLog << "[CCharDataMgr] DumpPlayerData FlushingTrunkNum="
             << CharDataInfo.m_mapFlushingTrunks.size() << endl;
}

bool CCharDataMgr::OnServerStop() {
    if (!m_pShmMgr)
        return true;
    OnTick();
    if (!IsDataReady())
        return false;
    //	m_pShmMgr->NotifyAppClose();
    SAFE_RELEASE(m_pShmMgr);
    return true;
}

bool CCharDataMgr::EnableAllocData(ETrunkType _eTrunkType, uint64 _nTrunkID) {
    int32 nFreeIndex = GetFreeTrunk(_eTrunkType, _nTrunkID);
    if (nFreeIndex == ePPM_Exist || nFreeIndex == ePPM_NoSpace) {
        GammaLog << "[CCharDataMgr] error:EnableAllocData," << (uint32)_eTrunkType << ","
                 << "," << _nTrunkID << "," << nFreeIndex << endl;
        return false;
    }
    return true;
}

bool CCharDataMgr::IsDataReady() {
    return m_pShmMgr && m_pShmMgr->IsReady();
}

void CCharDataMgr::OnReboot() {
    /*
    CGameRebort NetMsg;
    auto pDBConsumer = g_pApp->GetModuleConsummer<GameDBConsummer>();
    pDBConsumer->SendDBMsg(1, &NetMsg, sizeof(NetMsg));
    */
    GammaLog << "[CCharDataMgr] DB OnReboot" << endl;
    m_pShmMgr->OnRebootFinish(true);
}

// block数据已经提交到shm，主线程写入db
void CCharDataMgr::OnCommitBlocksInfo(const SCommitBlocksInfo *_pInfo, size_t _nSize) {
    CGameShmCommitBlocksInfo NetData;
    NetData.nTime = GetProcessTime();
    SSendBuf vecBuff[] = {
        SSendBuf(&NetData, (uint32)sizeof(NetData)),
        SSendBuf(_pInfo, _nSize),
    };
    // auto pDBConsumer = g_pApp->GetModuleConsummer<GameDBConsummer>();
    // pDBConsumer->SendDBMsg( (uint32)_pInfo->m_nTrunkID, vecBuff, 2 );
}

void CCharDataMgr::OnCommitFlag(const SCommitFlag *_pInfo, size_t _nSize) {
    // center 控制登录线的唯一性，不需要db处理
    /*
    CGameShmCommitFlag NetData;
    NetData.nTime = GetProcessTime();
    SSendBuf vecBuff[] =
    {
        SSendBuf(&NetData, (uint32)sizeof(NetData)),
        SSendBuf(_pInfo, _nSize),
    };
    auto pDBConsumer = g_pApp->GetModuleConsummer<GameDBConsummer>();
    pDBConsumer->SendDBMsg( (uint32)_pInfo->m_nTrunkID, vecBuff, 2 );
    */
    IShareMemoryMgr *pShareMemMgr = GetShmMgr();
    if (!pShareMemMgr)
        return;
    SCommitFlag Info = *_pInfo;
    Info.m_nMainThreadFinishTime = GetProcessTime();
    GetShmMgr()->OnCommitFlagFinish(&Info, false);
}

void CCharDataMgr::OnCommitAllBlocksOK(const SCommitFlag *_pInfo, size_t _nSize) {
    m_pShmMgr->OnCommitAllBlocksOKFinish(_pInfo, true);
    GammaLog << "[CCharDataMgr] DB CommitFlag OK," << _pInfo->m_nTrunkID << endl;

    SCharDataUseInfo &CharDataInfo = m_CharDataInfo[_pInfo->m_nTrunkType];
    CID2ArrayIndex::iterator it = CharDataInfo.m_mapUsingTrunks.find(_pInfo->m_nTrunkID);
    if (it == CharDataInfo.m_mapUsingTrunks.end()) {
        GammaLog << "[CCharDataMgr] DB CommitFlag slow," << _pInfo->m_nTrunkID << endl;
        return;
    }
    CheckFlushingTrunk((ETrunkType)_pInfo->m_nTrunkType, it->second);
}

std::pair<ETrunkType, uint32> CCharDataMgr::CalculateTrunkInfo(const tbyte *_pOffset,
                                                               SBlockData **_ppBlockData,
                                                               SChunckHead *&_pTrunckHead) {
    uint32 eType = 0;
    tbyte *pTrunckStart;
    uint32 nTotalSize;

    _pTrunckHead = NULL;

    // 找到trunk的起始和结束位置
    while (eType < eTrunkType_Count) {
        SChunckTypeInfo &TrunkTypeInfo = m_GameShareFile.m_aryChunkTypeInfo[eType];
        pTrunckStart = (tbyte *)(&TrunkTypeInfo) + TrunkTypeInfo.m_nChunkOffset;
        nTotalSize = TrunkTypeInfo.m_nChunkSize * TrunkTypeInfo.m_nChunkCount;
        if (_pOffset >= pTrunckStart && _pOffset < pTrunckStart + nTotalSize)
            break;
        ++eType;
    }

    SChunckTypeInfo &TrunkTypeInfo = m_GameShareFile.m_aryChunkTypeInfo[eType];
    // 计算m_aryChunkTypeInfo的索引
    uint32 nIndex = (uint32)((_pOffset - (tbyte *)pTrunckStart) / TrunkTypeInfo.m_nChunkSize);
    _pTrunckHead = (SChunckHead *)(pTrunckStart + nIndex * TrunkTypeInfo.m_nChunkSize);

    if (eType == eTrunkType_Count)
        return pair<ETrunkType, uint32>((ETrunkType)eType, nIndex);

    if (_ppBlockData) {
        *_ppBlockData = NULL;

        SBlockInfo *pBlockInfo =
            (SBlockInfo *)(((tbyte *)_pTrunckHead) + _pTrunckHead->m_nChunkHeadSize);
        for (int32 i = _pTrunckHead->m_nBlockCount - 1; i >= 0; --i) {
            SBlockInfo *pCurInfo = pBlockInfo + i;
            SBlockData *pBlockData = (SBlockData *)((tbyte *)pCurInfo + pCurInfo->m_nDataOffset);
            if ((tbyte *)pBlockData > _pOffset)
                continue;
            *_ppBlockData = pBlockData;
            break;
        }
    }
    return pair<ETrunkType, uint32>((ETrunkType)eType, nIndex);
}

bool CCharDataMgr::RegistCommitData(tbyte *_pData, bool _bCheckDataStart) {
    if (_pData < (tbyte *)((&m_GameShareFile) + 0) || _pData > (tbyte *)((&m_GameShareFile) + 1))
        return false;

    SBlockData *pBlockData = (SBlockData *)_pData;
    if (!_bCheckDataStart && pBlockData->m_nRegistForCommit != INVALID_32BITID)
        return true;

    SChunckHead *pTrunckHead = NULL;
    // 计算trunk类型，和所在trunk的索引
    pair<ETrunkType, uint32> TrunkInfo =
        CalculateTrunkInfo(_pData, _bCheckDataStart ? &pBlockData : NULL, pTrunckHead);
    if (TrunkInfo.first >= eTrunkType_Count || !pTrunckHead)
        return false;
    if (pTrunckHead->m_nDataState != eSCT_UpdateBlock
        || pTrunckHead->m_nChunckID == INVALID_64BITID)
        return false;

    pTrunckHead->m_nDataVersion++;
    pBlockData->m_nQueryDataVersion = pTrunckHead->m_nDataVersion;
    // m_nRegistForCommit为INVALID_32BITID表示不用提交，
    // 否则表示数据块m_aryCommitBlockData的索引，用于服务器创建对象不成功的时候从commit列表删除
    if (pBlockData->m_nRegistForCommit != INVALID_32BITID)
        return true;

    SCharDataUseInfo &CharDataInfo = m_CharDataInfo[TrunkInfo.first];
    CTrunkCommitInfo &CommitTrunk = CharDataInfo.m_aryCommitTrunk[TrunkInfo.second];
    pBlockData->m_nRegistForCommit = CommitTrunk.m_nCurCommitBlockCount++;
    CommitTrunk.m_aryCommitBlockData[pBlockData->m_nRegistForCommit] = pBlockData;
    if (!CommitTrunk.IsInList())
        CharDataInfo.m_aryPendingTrunks.PushBack(CommitTrunk);
    return true;
}

void CCharDataMgr::OnTick() {
    if (!m_pShmMgr)
        return;
    m_pShmMgr->Check();

    for (uint32 eType = 0; eType < eTrunkType_Count; eType++) {
        SCharDataUseInfo &CharDataInfo = m_CharDataInfo[eType];

        CommitData(CharDataInfo.m_aryPendingTrunks, eType);

        CIndex2ArrayID::iterator it = CharDataInfo.m_mapFlushingTrunks.begin();
        for (uint32 i = 0; i < 100 && it != CharDataInfo.m_mapFlushingTrunks.end(); i++)
            CheckFlushingTrunk((ETrunkType)eType, (it++)->first);
    }
}

// 提交数据  从Src拷贝到Des 维护CommitIDFirst/Last
void CCharDataMgr::CommitData(TGammaList<CTrunkCommitInfo> &_listTrunk, uint32 _eTrunkType) {
    SChunckTypeInfo &SrcTrunkTypeInfo = m_GameShareFile.m_aryChunkTypeInfo[_eTrunkType];
    SChunckTypeInfo &DesTrunkTypeInfo = m_pGameShareFile->m_aryChunkTypeInfo[_eTrunkType];

    tbyte *pSrcInfoStart = (tbyte *)(&SrcTrunkTypeInfo) + SrcTrunkTypeInfo.m_nChunkOffset;
    tbyte *pDesInfoStart = (tbyte *)(&DesTrunkTypeInfo) + DesTrunkTypeInfo.m_nChunkOffset;

    while (_listTrunk.GetFirst()) {
        CTrunkCommitInfo &CurCommitData = *_listTrunk.GetFirst();
        CurCommitData.Remove();

        SCharDataUseInfo &CharDataInfo = m_CharDataInfo[_eTrunkType];
        // 数据提交开始
        // CurCommitData就是m_aryCommitTrunk的元素，所以&CurCommitData -
        // &CharDataInfo.m_aryCommitTrunk[0]就是m_aryCommitTrunk的索引 约定：m_aryCommitTrunk[i] 与
        // SGameShareFile.m_PlayersData[i] 一一对应——下标 i 就是 Shm 槽位号。
        // 所以nTrunkIndex就是Shm槽位号
        uint32 nTrunkIndex = (uint32)(&CurCommitData - &CharDataInfo.m_aryCommitTrunk[0]);
        uint32 nOffsetFromStart = SrcTrunkTypeInfo.m_nChunkSize * nTrunkIndex;
        tbyte *pSrcTrunk = pSrcInfoStart + nOffsetFromStart;
        tbyte *pDesTrunk = pDesInfoStart + nOffsetFromStart;
        SChunckHead *pSrcTrunkHead = (SChunckHead *)pSrcTrunk;
        SChunckHead *pDesTrunkHead = (SChunckHead *)pDesTrunk;
        pDesTrunkHead->m_nDataVersion = pSrcTrunkHead->m_nDataVersion;

        pSrcTrunkHead->m_nCommitIDFirst++;
        pDesTrunkHead->m_nCommitIDFirst = pSrcTrunkHead->m_nCommitIDFirst;
        SBlockInfo *pBlockInfoSrc =
            (SBlockInfo *)(((tbyte *)pSrcTrunkHead) + pSrcTrunkHead->m_nChunkHeadSize);

        for (uint32 i = 0; i < CurCommitData.m_nCurCommitBlockCount; i++) {
            SBlockData *pBlockDataSrc = CurCommitData.m_aryCommitBlockData[i];
            SBlockData *pBlockDataDes =
                (SBlockData *)(pDesTrunk + (((tbyte *)pBlockDataSrc) - pSrcTrunk));

            if (pBlockDataSrc->m_nQueryDataVersion > pBlockDataSrc->m_nAnswerDataVersion) {
                // 数据提交开始
                pBlockDataDes->m_nCommitIDFirst = pSrcTrunkHead->m_nCommitIDFirst;

                int32 nDiffInfo[INVALID_16BITID];
                uint32 nMaxBlockSize =
                    pBlockInfoSrc[pBlockDataSrc->m_nBlockInfoIndex].m_nMaxBlockSize;
                uint16 nDiffCount =
                    CopyPropBlock(pBlockDataDes, pBlockDataSrc, nMaxBlockSize, nDiffInfo);

                pBlockDataDes->m_nQueryDataVersion = pBlockDataSrc->m_nQueryDataVersion;

                pBlockDataSrc->m_nRegistForCommit = INVALID_32BITID;
                pBlockDataSrc->m_nAnswerDataVersion = pSrcTrunkHead->m_nDataVersion;

                // 数据提交结束
                pBlockDataDes->m_nCommitIDLast = pSrcTrunkHead->m_nCommitIDFirst;

                // 最后回调
                if (!pBlockInfoSrc->m_pContextData)
                    continue;

                // IDataCommitListener* pListener =
                // static_cast<IDataCommitListener*>(pBlockInfoSrc->m_pContextData); try
                // {
                // 	tbyte szMaxBuffer[INVALID_16BITID];
                // 	char* pBufSrc = (char*)(pBlockDataSrc);
                // 	uint16 nOffset = sizeof(SBlockData);

                // 	uint16 nBlockIndex = (uint16)pBlockDataSrc->m_nBlockInfoIndex;
                // 	uint16 nCount = 0;
                // 	CBufFileEx File(szMaxBuffer, ELEM_COUNT(szMaxBuffer));
                // 	for (int32 i = 0; i < nDiffCount; i++)
                // 	{
                // 		if (nDiffInfo[i] > 0)
                // 		{
                // 			uint16 nSize = (uint16)nDiffInfo[i];
                // 			File.Write(&nOffset, sizeof(uint16));
                // 			File.Write(&nSize, sizeof(uint16));
                // 			File.Write(pBufSrc + nOffset, nSize);
                // 			nCount++;
                // 		}
                // 		nOffset = (uint16)(nOffset + Abs(nDiffInfo[i]));
                // 	}
                // 	pListener->OnDataCommit(nBlockIndex, nCount, szMaxBuffer, File.GetPos());
                // }
                // catch (...)
                // {
                // 	GammaErr << "OnDataCommit (" << pSrcTrunkHead->m_nChunckID
                // 		<< ":" << pBlockDataSrc->m_nBlockInfoIndex << ")Error!!!" << endl;
                // }
            }
        }

        CurCommitData.m_nCurCommitBlockCount = 0;
        // 数据提交结束
        pSrcTrunkHead->m_nCommitIDLast = pSrcTrunkHead->m_nCommitIDFirst;
        pDesTrunkHead->m_nCommitIDLast = pDesTrunkHead->m_nCommitIDFirst;

        if (pSrcTrunkHead->m_nDataState == eSCT_UpdateTrunk) {
            pDesTrunkHead->m_nDataState = eSCT_UpdateTrunk;
            pSrcTrunkHead->m_nChunckID = INVALID_64BITID;
        }
    }
}

bool CCharDataMgr::CheckFlushingTrunk(ETrunkType _eTrunkType, uint32 _nTrunkIndex) {
    SCharDataUseInfo &CharDataInfo = m_CharDataInfo[_eTrunkType];
    SChunckTypeInfo &DesTrunkTypeInfo = m_pGameShareFile->m_aryChunkTypeInfo[_eTrunkType];
    tbyte *pDesInfoStart = (tbyte *)(&DesTrunkTypeInfo) + DesTrunkTypeInfo.m_nChunkOffset;
    uint32 nOffsetFromStart = DesTrunkTypeInfo.m_nChunkSize * _nTrunkIndex;
    SChunckHead *pDesTrunkHead = (SChunckHead *)(pDesInfoStart + nOffsetFromStart);
    if (pDesTrunkHead->m_nChunckID != INVALID_64BITID)
        return false;
    CIndex2ArrayID::iterator it = CharDataInfo.m_mapFlushingTrunks.find(_nTrunkIndex);
    uint64 nTrunkID = it->second;
    GammaAst(it != CharDataInfo.m_mapFlushingTrunks.end());
    CharDataInfo.m_mapUsingTrunks.erase(it->second);
    CharDataInfo.m_mapFlushingTrunks.erase(it);
    CharDataInfo.m_aryFreeTrunks.PushBack(CharDataInfo.m_aryCommitTrunk[_nTrunkIndex]);

    GammaLog << "[CCharDataMgr] DB FreeCharDBData ok, index=" << _nTrunkIndex
             << ",used=" << CharDataInfo.m_mapUsingTrunks.size() << ",id=" << nTrunkID << endl;
    if (_eTrunkType != eTrunkType_Char)
        return true;
    // g_pApp->GetScript()->RunFunction( nullptr, "OnPlayerDataCommited", (int64)nTrunkID );
    return true;
}

uint16 CCharDataMgr::CopyPropBlock(SBlockData *_pBlockDataDes, SBlockData *_pBlockDataSrc,
                                   uint32 _nMaxBlockSize, int32 _nDiffInfo[INVALID_16BITID]) {
    char *pBufDes = (char *)(_pBlockDataDes + 1);
    char *pBufSrc = (char *)(_pBlockDataSrc + 1);
    int32 nCopyCount = (int32)(_nMaxBlockSize - sizeof(SBlockData));
    int32 nCopyInt32 = nCopyCount / sizeof(uint32);

    int32 *pBufDes32 = (int32 *)(pBufDes);
    int32 *pBufSrc32 = (int32 *)(pBufSrc);

    bool bSame = *pBufDes32 == *pBufSrc32;
    uint16 nCount = 0;
    uint16 nDiffCount = 0;

    for (int32 i = 0; i < nCopyInt32; i++) {
        bool bCurSame = pBufDes32[i] == pBufSrc32[i];
        if (!bCurSame)
            pBufDes32[i] = pBufSrc32[i];

        if (bCurSame == bSame) {
            nCount += 4;
            continue;
        }

        _nDiffInfo[nDiffCount++] = bSame ? -nCount : nCount;
        nCount = 4;
        bSame = !bSame;
    }

    for (int32 i = nCopyInt32 * sizeof(uint32); i < nCopyCount; i++) {
        bool bCurSame = pBufDes[i] == pBufSrc[i];
        if (!bCurSame)
            pBufDes[i] = pBufSrc[i];

        if (bCurSame == bSame) {
            nCount++;
            continue;
        }

        _nDiffInfo[nDiffCount++] = bSame ? -nCount : nCount;
        nCount = 1;
        bSame = !bSame;
    }

    _nDiffInfo[nDiffCount++] = bSame ? -nCount : nCount;
    return nDiffCount;
}

int32 CCharDataMgr::GetFreeTrunk(ETrunkType _eType, uint64 _nTrunkID) {
    SCharDataUseInfo &CharDataInfo = m_CharDataInfo[_eType];

    map<uint64, uint32>::iterator it = CharDataInfo.m_mapUsingTrunks.find(_nTrunkID);
    if (it != CharDataInfo.m_mapUsingTrunks.end() && !CheckFlushingTrunk(_eType, it->second))
        return ePPM_Exist;

    CTrunkCommitInfo *pFree = CharDataInfo.m_aryFreeTrunks.GetFirst();
    if (!pFree) {
        CIndex2ArrayID::iterator it = CharDataInfo.m_mapFlushingTrunks.begin();
        while (it != CharDataInfo.m_mapFlushingTrunks.end()) {
            uint32 nTrunkIndex = (it++)->first;
            if (!CheckFlushingTrunk(_eType, nTrunkIndex))
                continue;
            return nTrunkIndex;
        }
        return ePPM_NoSpace;
    }

    return (int32)(pFree - &CharDataInfo.m_aryCommitTrunk[0]);
}

/* 将db数据拷贝的shm中 */
SChunckHead *CCharDataMgr::AllocDBData(ETrunkType _eTrunkType, uint64 _nTrunkID,
                                       const SChunckHead *_pData) {
    int32 nFreeIndex = GetFreeTrunk(_eTrunkType, _nTrunkID);
    if (nFreeIndex == ePPM_Exist || nFreeIndex == ePPM_NoSpace) {
        GammaLog << "[CCharDataMgr] error:AllocError," << (uint32)_eTrunkType << ","
                 << "," << _nTrunkID << "," << nFreeIndex << endl;
        return nullptr;
    }

    SCharDataUseInfo &CharDataInfo = m_CharDataInfo[_eTrunkType];

    CTrunkCommitInfo &Free = CharDataInfo.m_aryCommitTrunk[nFreeIndex];
    CharDataInfo.m_mapUsingTrunks[_nTrunkID] = nFreeIndex;
    GammaAst(Free.IsInList());
    Free.Remove();

    SChunckTypeInfo &SrcTrunkTypeInfo = m_GameShareFile.m_aryChunkTypeInfo[_eTrunkType];
    SChunckTypeInfo &DesTrunkTypeInfo = m_pGameShareFile->m_aryChunkTypeInfo[_eTrunkType];

    tbyte *pSrcInfoStart = (tbyte *)(&SrcTrunkTypeInfo) + SrcTrunkTypeInfo.m_nChunkOffset;
    tbyte *pDesInfoStart = (tbyte *)(&DesTrunkTypeInfo) + DesTrunkTypeInfo.m_nChunkOffset;

    SChunckHead *pSrcTrunkHead =
        (SChunckHead *)(pSrcInfoStart + nFreeIndex * SrcTrunkTypeInfo.m_nChunkSize);
    SChunckHead *pDesTrunkHead =
        (SChunckHead *)(pDesInfoStart + nFreeIndex * DesTrunkTypeInfo.m_nChunkSize);
    const SChunckHead *pInitTrunkHead = (const SChunckHead *)_pData;
    for (uint16 i = 0; i < pSrcTrunkHead->m_nBlockCount; i++) {
        SBlockInfo *pBlockInfoSrc =
            (SBlockInfo *)(((tbyte *)pSrcTrunkHead) + pSrcTrunkHead->m_nChunkHeadSize) + i;
        SBlockData *pBlockDataSrc =
            (SBlockData *)(((tbyte *)pBlockInfoSrc) + pBlockInfoSrc->m_nDataOffset);

        const SBlockInfo *pBlockInfoInit =
            (SBlockInfo *)(((tbyte *)pInitTrunkHead) + pInitTrunkHead->m_nChunkHeadSize) + i;
        const SBlockData *pBlockDataInit =
            (SBlockData *)(((tbyte *)pBlockInfoInit) + pBlockInfoInit->m_nDataOffset);
        //		memset(pBlockDataSrc, 0, pBlockInfoSrc->m_nMaxBlockSize);
        memcpy(pBlockDataSrc, pBlockDataInit, pBlockInfoInit->m_nMaxBlockSize);

        SBlockInfo *pBlockInfoDes =
            (SBlockInfo *)(((tbyte *)pDesTrunkHead) + pDesTrunkHead->m_nChunkHeadSize) + i;
        SBlockData *pBlockDataDes =
            (SBlockData *)(((tbyte *)pBlockInfoDes) + pBlockInfoDes->m_nDataOffset);
        memcpy(pBlockDataDes, pBlockDataInit, pBlockInfoInit->m_nMaxBlockSize);

        pBlockDataDes->m_nQueryDataVersion = pBlockDataDes->m_nAnswerDataVersion = 0;
        pBlockDataSrc->m_nQueryDataVersion = pBlockDataSrc->m_nAnswerDataVersion = 0;

        pBlockDataSrc->m_nBlockInfoIndex = i;
        pBlockDataSrc->m_nRegistForCommit = INVALID_32BITID;

        // 用于同步数据回调的PlayerServer指针
        pBlockInfoSrc->m_pContextData = NULL;
    }
    pSrcTrunkHead->m_nDataState = pDesTrunkHead->m_nDataState = eSCT_UpdateBlock;
    pSrcTrunkHead->m_nDataVersion = pDesTrunkHead->m_nDataVersion = 0;

    pSrcTrunkHead->m_nChunckID = pDesTrunkHead->m_nChunckID = _nTrunkID;
    pSrcTrunkHead->m_nCommitIDFirst = pDesTrunkHead->m_nCommitIDFirst = 0;
    pSrcTrunkHead->m_nCommitIDLast = pDesTrunkHead->m_nCommitIDLast = 0;
    pSrcTrunkHead->m_nQueryCheck = pDesTrunkHead->m_nQueryCheck = 0;
    return pSrcTrunkHead;
}

bool CCharDataMgr::CommitTrunk(ETrunkType _eType, SChunckHead *_pTrunk) {
    SChunckTypeInfo &SrcTrunkTypeInfo = m_GameShareFile.m_aryChunkTypeInfo[_eType];
    tbyte *pSrcInfoStart = (tbyte *)(&SrcTrunkTypeInfo) + SrcTrunkTypeInfo.m_nChunkOffset;
    uint32 nTrunkIndex =
        (uint32)(((tbyte *)(_pTrunk)-pSrcInfoStart) / SrcTrunkTypeInfo.m_nChunkSize);
    GammaAst(nTrunkIndex < SrcTrunkTypeInfo.m_nChunkCount);
    GammaAst(_pTrunk->m_nDataState == eSCT_UpdateBlock);
    _pTrunk->m_nDataState = eSCT_UpdateTrunk;

    SCharDataUseInfo &CharDataInfo = m_CharDataInfo[_eType];
    CharDataInfo.m_mapFlushingTrunks[nTrunkIndex] = _pTrunk->m_nChunckID;
    CTrunkCommitInfo &CommitTrunk = CharDataInfo.m_aryCommitTrunk[nTrunkIndex];
    if (CommitTrunk.IsInList())
        return false;
    CharDataInfo.m_aryPendingTrunks.PushBack(CommitTrunk);
    CommitData(CharDataInfo.m_aryPendingTrunks, _eType);
    return true;
}

// 将db中查询到的数据写到shm
// 过程为：GetPlayerDataInDB -> OnPlayerDataResult -> AllocalPlayerData
CPlayerDataDefineShm *CCharDataMgr::AllocalPlayerData(uint64 _nOjbectID,
                                                      const CPlayerDataDefineShm *pInitData) {
    CPlayerDataDefineShm *pData =
        (CPlayerDataDefineShm *)AllocDBData(eTrunkType_Char, _nOjbectID, pInitData);
    SCharDataUseInfo &CharDataInfo = m_CharDataInfo[eTrunkType_Char];
    GammaLog << "[CCharDataMgr] DB AllocalPlayerData," << CharDataInfo.m_mapUsingTrunks.size()
             << "," << _nOjbectID << "," << pData << endl;
    return pData;
}

CPlayerDataDefineShm *CCharDataMgr::GetPlayerData(uint64 _nCharID) {
    SChunckTypeInfo &TrunkTypeInfo = m_GameShareFile.m_aryChunkTypeInfo[eTrunkType_Char];
    tbyte *pTrunckStart = (tbyte *)(&TrunkTypeInfo) + TrunkTypeInfo.m_nChunkOffset;

    for (uint32 i = 0; i < TrunkTypeInfo.m_nChunkCount; ++i) {
        SChunckHead *pTrunk = (SChunckHead *)(pTrunckStart + i * TrunkTypeInfo.m_nChunkSize);
        if (pTrunk->m_nChunckID == _nCharID)
            return (CPlayerDataDefineShm *)pTrunk;
    }
    return nullptr;
}

void CCharDataMgr::FreeCharDBData(uint64 nCharID, const void *pCharDB) {
    CPlayerDataDefineShm *pPlayerProp = (CPlayerDataDefineShm *)pCharDB;
    bool bRet = CommitTrunk(eTrunkType_Char, pPlayerProp);
    GammaLog << "[CCharDataMgr] DB FreeCharDBData begin," << nCharID << "," << (uint32)bRet << ","
             << pPlayerProp << endl;
}

void CCharDataMgr::OnDataCommit(uint16 nBlockIndex, uint16 nCount, const void *szMaxBuffer,
                                uint32 nSize) {
    GammaLog << "[CCharDataMgr] DB OnDataCommit," << nBlockIndex << "," << nCount << endl;
}

void CCharDataMgr::QueryCharInfo(uint64 _nOjbectID, uint64 _nBitArray, void *pLuaContext) {
    // auto pDBConsumer = g_pApp->GetModuleConsummer<GameDBConsummer>();
    // pDBConsumer->QueryCharInfo(_nOjbectID, _nBitArray, pLuaContext );
}

void CCharDataMgr::OnPlayerDataResult(bool _bOk, int64 _uuid, const void *_pCharData) {
    // bool bEnable = _bOk && EnableAllocData(eTrunkType_Char, _uuid);
    // auto pData = bEnable ? (CPlayerDataDefineShm*)_pCharData : nullptr;
    // g_pApp->GetScript()->RunFunction(nullptr, "OnPlayerDataResult", _uuid, pData);
}

void CCharDataMgr::UpdateShmData(const CDataContainer *pContainter, void *pData, uint32 nOffset) {
    if (pContainter && eDDT_Player == pContainter->GetDataDefineType())
        RegistCommitData(((tbyte *)pData) + nOffset, false);
}

/* shm 启动/重启/提交相关逻辑 */
template <>
void CCharDataMgr::OnShellMsg(const CGameRebortResult *_pCmd, size_t _nSize) {
    IShareMemoryMgr *pShareMemMgr = GetShmMgr();
    if (!pShareMemMgr)
        return;
    pShareMemMgr->OnRebootFinish(!!_pCmd->bFailed);
}

template <>
void CCharDataMgr::OnShellMsg(const CGameModifyCharNameResult *_pCmd, size_t _nSize) {
    // g_pApp->GetScript()->RunFunction( nullptr, "OnPlayerRenameResult",
    // 	_pCmd->nCharID, _pCmd->nResult, _pCmd->szCharName, _pCmd->fromType );
}

template <>
void CCharDataMgr::OnShellMsg(const CGameShmCommitBlocksInfoResult *_pCmd, size_t _nSize) {
    IShareMemoryMgr *pShareMemMgr = GetShmMgr();
    if (!pShareMemMgr)
        return;
    SCommitBlocksInfo Info = *(const SCommitBlocksInfo *)(_pCmd + 1);
    Info.m_nMainThreadFinishTime = GetProcessTime();
    pShareMemMgr->OnCommitBlocksFinish(&Info, !!_pCmd->bFailed);
}

template <>
void CCharDataMgr::OnShellMsg(const CGameShmCommitFlagResult *_pCmd, size_t _nSize) {
    IShareMemoryMgr *pShareMemMgr = GetShmMgr();
    if (!pShareMemMgr)
        return;
    SCommitFlag Info = *((const SCommitFlag *)(_pCmd + 1));
    Info.m_nMainThreadFinishTime = GetProcessTime();
    pShareMemMgr->OnCommitFlagFinish(&Info, !!_pCmd->bFailed);
}