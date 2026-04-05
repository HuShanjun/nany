#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include "GammaCommon/GammaHelp.h"
#include "GammaShm/IShareMemoryMgr.h"
#include "CharData.h"

using namespace Gamma;

//=====================================================
// GammaShm 示例：共享内存布局与初始化全部由 CharData.h 提供
// （整段 shm → 每名玩家一个 SCharTrunk → 每项业务一个 Block）
//=====================================================

class CSampleShmHandler : public IShareMemoryHandler
{
public:
	explicit CSampleShmHandler(IShareMemoryMgr* pMgr = nullptr) : m_pMgr(pMgr) {}
	void SetMgr(IShareMemoryMgr* pMgr) { m_pMgr = pMgr; }

	void OnReboot() override
	{
		std::cout << "[Handler] OnReboot - 从共享内存恢复数据" << std::endl;
		m_pMgr->OnRebootFinish(false);
	}

	void OnCommitBlocksInfo(const SCommitBlocksInfo* pInfo, size_t nSize) override
	{
		std::cout << "[Handler] OnCommitBlocksInfo - TrunkID=" << pInfo->m_nTrunkID
			<< " BlockCount=" << pInfo->m_nBlockCount << std::endl;
		m_pMgr->OnCommitBlocksFinish(pInfo, false);
	}

	void OnCommitFlag(const SCommitFlag* pInfo, size_t nSize) override
	{
		std::cout << "[Handler] OnCommitFlag - TrunkID=" << pInfo->m_nTrunkID << std::endl;
		m_pMgr->OnCommitFlagFinish(pInfo, false);
	}

	void OnCommitAllBlocksOK(const SCommitFlag* pInfo, size_t nSize) override
	{
		std::cout << "[Handler] OnCommitAllBlocksOK - TrunkID=" << pInfo->m_nTrunkID
			<< " 所有数据块已提交完成" << std::endl;
		m_pMgr->OnCommitAllBlocksOKFinish(pInfo, false);
	}

private:
	IShareMemoryMgr* m_pMgr;
};

/// Gas 侧：在一个玩家 Trunk 内更新多个 Block，共用 Trunk 的 CommitID 对
static void SimulatePlayerLogin(SCharShareMemory* pShm, uint32_t nSlotIndex,
	uint64_t nPlayerID, const char* szName, uint32_t nLevel)
{
	SCharTrunk* pTrunk = CharShm_TrunkAt(pShm, nSlotIndex);
	if (!pTrunk)
		return;

	SCharBaseBlock* pBase = CharShm_BaseBlock(pTrunk);
	SCharItemBlock* pItem = CharShm_ItemBlock(pTrunk);

	pTrunk->m_nCommitIDFirst++;

	pTrunk->m_nChunckID = nPlayerID;
	pTrunk->m_nDBGasID = 1;
	pTrunk->m_nDataState = eSCT_UpdateTrunk;

	// Block：eCharBlock_Base
	pBase->m_nQueryDataVersion++;
	pBase->m_nPlayerID = nPlayerID;
	strncpy(pBase->m_szName, szName, sizeof(pBase->m_szName) - 1);
	pBase->m_szName[sizeof(pBase->m_szName) - 1] = '\0';
	pBase->m_nLevel = nLevel;
	pBase->m_nExp = nLevel * 1000;
	pBase->m_nHP = 100 + static_cast<int32_t>(nLevel) * 10;
	pBase->m_nMP = 50 + static_cast<int32_t>(nLevel) * 5;

	// Block：eCharBlock_Item
	pItem->m_nQueryDataVersion++;
	pItem->m_ItemCount.m_nSize = 2;
	pItem->m_Items[0].m_nItemID = 1001;
	pItem->m_Items[0].m_nCount = 3;
	pItem->m_Items[1].m_nItemID = 2002;
	pItem->m_Items[1].m_nCount = 1;

	pTrunk->m_nCommitIDLast++;

	std::cout << "[Gas] 玩家登录 slot=" << nSlotIndex
		<< " ID=" << nPlayerID
		<< " Name=" << szName
		<< " Level=" << nLevel
		<< " Items=" << pItem->m_ItemCount.m_nSize << std::endl;
}

int main(int argc, const char* argv[])
{
	SetConsoleOutputUTF8();
	std::cout << "=== GammaShm 示例（CharData.h 布局）===" << std::endl;
	std::cout << "共享内存镜像大小 CHAR_SHM_TOTAL_SIZE: " << CHAR_SHM_TOTAL_SIZE << " 字节" << std::endl;
	std::cout << "玩家槽位数 CHAR_SHM_MAX_PLAYER_COUNT: " << CHAR_SHM_MAX_PLAYER_COUNT << std::endl;
	std::cout << "单玩家 Trunk CHAR_SHM_PLAYER_CHUNK_SIZE: " << CHAR_SHM_PLAYER_CHUNK_SIZE << " 字节" << std::endl;
	std::cout << "每 Trunk 内 Block 数 CHAR_SHM_BLOCK_COUNT: " << CHAR_SHM_BLOCK_COUNT
		<< " (eCharBlock_Count=" << eCharBlock_Count << ")" << std::endl;
	std::cout << "SChunckTypeInfo 数组长度 eCharChunk_Count: " << eCharChunk_Count << std::endl;
	std::cout << std::endl;

	auto* pSrc = new SCharShareMemory();
	CharShm_InitShareMemory(pSrc);
	std::cout << "[Init] CharShm_InitShareMemory 完成，m_nChunkTypeCount="
		<< pSrc->m_nChunkTypeCount << std::endl;

	CSampleShmHandler handler;
	IShareMemoryMgr* pMgr = CreateShareMemoryMgr(
		1,
		reinterpret_cast<SShareCommonHead*>(pSrc),
		"./shm_sample.dat",
		&handler,
		5000,
		true,
		INVALID_32BITID
	);
	handler.SetMgr(pMgr);
	std::cout << "[Init] ShareMemoryMgr 创建完成" << std::endl;

	pMgr->Start();
	std::cout << "[Init] ShareMemoryMgr 已启动" << std::endl << std::endl;

	SimulatePlayerLogin(pSrc, 0, 10001, "Alice", 10);
	SimulatePlayerLogin(pSrc, 1, 10002, "Bob", 25);

	std::cout << std::endl << "[Main] 进入主循环，等待数据提交..." << std::endl;
	for (int i = 0; i < 100; i++)
	{
		pMgr->Check();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		if (pMgr->IsReady() && i > 10)
		{
			std::cout << "[Main] 所有数据已提交完毕" << std::endl;
			break;
		}
	}

	std::cout << std::endl << "[Main] 通知关闭..." << std::endl;
	pMgr->NotifyAppClose();

	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	pMgr->Release();
	delete pSrc;

	std::cout << "[Main] 示例结束" << std::endl;
	return 0;
}
