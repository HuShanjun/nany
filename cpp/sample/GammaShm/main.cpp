#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include "GammaCommon/GammaHelp.h"
#include "GammaShm/CBaseShareMemory.h"
#include "GammaShm/IShareMemoryMgr.h"

using namespace Gamma;

//=====================================================
// 示例：演示 GammaShm 模块的基本使用方法
//
// 本示例定义了一个简单的共享内存布局：
// - 1 种 ChunkType（玩家数据）
// - 每个 Chunk 包含 1 个 BlockInfo（基础属性）
//=====================================================

// 玩家基础属性（对应一个 SBlockInfo 的数据区）
struct SPlayerBaseInfo
{
	SBlockData	m_BlockData;	// 必须以 SBlockData 开头
	uint64_t	m_nPlayerID;
	char		m_szName[32];
	uint32_t	m_nLevel;
	uint32_t	m_nExp;
	int32_t		m_nHP;
	int32_t		m_nMP;
};

// 定义布局参数
static const uint32_t MAX_PLAYER_COUNT = 10;
static const uint32_t BLOCK_COUNT_PER_PLAYER = 1;

// 计算单个 Chunk 大小 = SChunckHead + SBlockInfo[] + 数据区[]
static const uint32_t PLAYER_CHUNK_SIZE =
	sizeof(SChunckHead) +
	BLOCK_COUNT_PER_PLAYER * sizeof(SBlockInfo) +
	sizeof(SPlayerBaseInfo);

// 计算共享内存总大小
static const uint32_t SHM_TOTAL_SIZE =
	sizeof(SShareCommonHead) +
	sizeof(SChunckTypeInfo) +		// 1 种 ChunkType
	MAX_PLAYER_COUNT * PLAYER_CHUNK_SIZE;

//=====================================================
// 实现 IShareMemoryHandler 回调
//=====================================================
class CSampleShmHandler : public IShareMemoryHandler
{
public:
	CSampleShmHandler() : m_pMgr(nullptr) {}
	void SetMgr(IShareMemoryMgr* pMgr) { m_pMgr = pMgr; }

	void OnReboot() override
	{
		std::cout << "[Handler] OnReboot - 从共享内存恢复数据" << std::endl;
		// 实际项目中在此从 shm 恢复游戏数据
		// 完成后必须调用 OnRebootFinish
		m_pMgr->OnRebootFinish(false);
	}

	void OnCommitBlocksInfo(const SCommitBlocksInfo* pInfo, size_t nSize) override
	{
		std::cout << "[Handler] OnCommitBlocksInfo - TrunkID=" << pInfo->m_nTrunkID
			<< " BlockCount=" << pInfo->m_nBlockCount << std::endl;
		// 实际项目中在此将数据块写入数据库
		// 完成后必须调用 OnCommitBlocksFinish
		m_pMgr->OnCommitBlocksFinish(pInfo, false);
	}

	void OnCommitFlag(const SCommitFlag* pInfo, size_t nSize) override
	{
		std::cout << "[Handler] OnCommitFlag - TrunkID=" << pInfo->m_nTrunkID << std::endl;
		// 实际项目中在此提交标志到数据库
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

//=====================================================
// 在堆上构造共享内存源数据布局
//=====================================================
static uint8_t* BuildShareMemoryLayout()
{
	uint8_t* pMemory = new uint8_t[SHM_TOTAL_SIZE];
	memset(pMemory, 0, SHM_TOTAL_SIZE);

	// 1. 填写 SShareCommonHead
	SShareCommonHead* pHead = (SShareCommonHead*)pMemory;
	pHead->m_nVersion = 1;
	pHead->m_nSizeOfShareFlie = SHM_TOTAL_SIZE;
	pHead->m_nChunkTypeCount = 1;

	// 2. 填写 SChunckTypeInfo（紧跟在 Head 后面）
	SChunckTypeInfo* pTypeInfo = (SChunckTypeInfo*)(pHead + 1);
	pTypeInfo->m_bCommitFlagRequire = 0;	// 不需要提交标志到数据库
	pTypeInfo->m_nChunkOffset = (uint32_t)((uint8_t*)(pTypeInfo + 1) - (uint8_t*)pTypeInfo);
	pTypeInfo->m_nChunkSize = PLAYER_CHUNK_SIZE;
	pTypeInfo->m_nChunkCount = MAX_PLAYER_COUNT;

	// 3. 初始化每个 Chunk
	uint8_t* pChunkStart = (uint8_t*)(pTypeInfo + 1);
	for (uint32_t i = 0; i < MAX_PLAYER_COUNT; i++)
	{
		SChunckHead* pChunk = (SChunckHead*)(pChunkStart + i * PLAYER_CHUNK_SIZE);
		pChunk->m_nChunckID = INVALID_64BITID;	// 无效 ID 表示空槽位
		pChunk->m_nChunkHeadSize = sizeof(SChunckHead);
		pChunk->m_nBlockCount = BLOCK_COUNT_PER_PLAYER;
		pChunk->m_nDataState = eSCT_Invalid;
		pChunk->m_nDataVersion = 0;
		pChunk->m_nCommitIDFirst = 0;
		pChunk->m_nCommitIDLast = 0;

		// 填写 SBlockInfo
		SBlockInfo* pBlock = (SBlockInfo*)((uint8_t*)pChunk + sizeof(SChunckHead));
		pBlock->m_nType = 0;	// 类型由游戏定义
		pBlock->m_nDataOffset = (uint32_t)((uint8_t*)(pBlock + 1) - (uint8_t*)pBlock);
		pBlock->m_nMaxBlockSize = sizeof(SPlayerBaseInfo);
		pBlock->m_nArrayStartOff = 0;	// 无数组结构
		pBlock->m_nArrayElemSize = 0;	// 无数组结构
		pBlock->m_ptPoint = 0;

		// 初始化 SBlockData
		SPlayerBaseInfo* pPlayerInfo = (SPlayerBaseInfo*)(pBlock + 1);
		memset(pPlayerInfo, 0, sizeof(SPlayerBaseInfo));
	}

	return pMemory;
}

//=====================================================
// 模拟 Gas 写入玩家数据到共享内存
//=====================================================
static void SimulatePlayerLogin(uint8_t* pMemory, uint32_t nSlotIndex,
	uint64_t nPlayerID, const char* szName, uint32_t nLevel)
{
	SShareCommonHead* pHead = (SShareCommonHead*)pMemory;
	SChunckTypeInfo* pTypeInfo = (SChunckTypeInfo*)(pHead + 1);
	uint8_t* pChunkStart = (uint8_t*)(pTypeInfo + 1);

	SChunckHead* pChunk = (SChunckHead*)(pChunkStart + nSlotIndex * PLAYER_CHUNK_SIZE);
	SBlockInfo* pBlock = (SBlockInfo*)((uint8_t*)pChunk + sizeof(SChunckHead));
	SPlayerBaseInfo* pPlayer = (SPlayerBaseInfo*)(pBlock + 1);

	// 模拟 Gas 写入（使用 CommitID 保护）
	pChunk->m_nCommitIDFirst++;

	pChunk->m_nChunckID = nPlayerID;
	pChunk->m_nDBGasID = 1;
	pChunk->m_nDataState = eSCT_UpdateTrunk;

	pPlayer->m_BlockData.m_nQueryDataVersion++;
	pPlayer->m_nPlayerID = nPlayerID;
	strncpy(pPlayer->m_szName, szName, sizeof(pPlayer->m_szName) - 1);
	pPlayer->m_nLevel = nLevel;
	pPlayer->m_nExp = nLevel * 1000;
	pPlayer->m_nHP = 100 + nLevel * 10;
	pPlayer->m_nMP = 50 + nLevel * 5;

	pChunk->m_nCommitIDLast++;

	std::cout << "[Gas] 玩家登录: ID=" << nPlayerID
		<< " Name=" << szName
		<< " Level=" << nLevel << std::endl;
}

//=====================================================
// 主函数
//=====================================================
int main(int argc, const char* argv[])
{
	SetConsoleOutputUTF8();
	std::cout << "=== GammaShm 示例 ===" << std::endl;
	std::cout << "共享内存大小: " << SHM_TOTAL_SIZE << " 字节" << std::endl;
	std::cout << "最大玩家数: " << MAX_PLAYER_COUNT << std::endl;
	std::cout << "单个 Chunk 大小: " << PLAYER_CHUNK_SIZE << " 字节" << std::endl;
	std::cout << std::endl;

	// 1. 构造共享内存布局
	uint8_t* pSrcMemory = BuildShareMemoryLayout();
	std::cout << "[Init] 共享内存布局构造完成" << std::endl;

	// 2. 创建回调处理器和管理器
	CSampleShmHandler handler;
	IShareMemoryMgr* pMgr = CreateShareMemoryMgr(
		1,								// GasID
		(SShareCommonHead*)pSrcMemory,	// 源布局
		"./shm_sample.dat",				// 映射文件路径
		&handler,						// 回调处理器
		5000,							// 提交间隔 5 秒
		true,							// 关闭时不保留 shm 文件
		INVALID_32BITID					// 不启用 Linux 刷盘线程
	);
	handler.SetMgr(pMgr);
	std::cout << "[Init] ShareMemoryMgr 创建完成" << std::endl;

	// 3. 启动
	pMgr->Start();
	std::cout << "[Init] ShareMemoryMgr 已启动" << std::endl;
	std::cout << std::endl;
   


	// 4. 模拟玩家登录（Gas 写入数据）
	SimulatePlayerLogin(pSrcMemory, 0, 10001, "Alice", 10);
	SimulatePlayerLogin(pSrcMemory, 1, 10002, "Bob", 25);

	// 5. 主循环：定期调用 Check() 处理回调
	std::cout << std::endl << "[Main] 进入主循环，等待数据提交..." << std::endl;
	for (int i = 0; i < 100; i++)
	{
		pMgr->Check();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		// 检查是否所有数据已处理完毕
		if (pMgr->IsReady() && i > 10)
		{
			std::cout << "[Main] 所有数据已提交完毕" << std::endl;
			break;
		}
	}

	// 6. 关闭
	std::cout << std::endl << "[Main] 通知关闭..." << std::endl;
	pMgr->NotifyAppClose();

	// 等待后台线程退出
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	pMgr->Release();
	delete[] pSrcMemory;

	std::cout << "[Main] 示例结束" << std::endl;
	return 0;
}
