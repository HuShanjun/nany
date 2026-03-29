#include "GammaCommon/CDynamicObject.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaDatabase/IDatabase.h"
#include "GammaMTDbs/IDbsThreadMgr.h"
#include "GammaMTDbs/IDbsQuery.h"
#include "GammaMTDbs/IResultHolder.h"
#include "GammaMTDbs/CBaseDbsConn.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

using namespace Gamma;

// =============================================
// 命令协议定义
// =============================================
enum EDbCmd : uint8_t
{
	eDbCmd_QueryPlayer = 1,
	eDbCmd_InsertPlayer = 2,
};

#pragma pack(push, 1)
struct SCmdQueryPlayer
{
	EDbCmd eCmd;
	int32_t nMinLevel;
};

struct SCmdInsertPlayer
{
	EDbCmd eCmd;
	char szName[64];
	int32_t nLevel;
	int64_t nGold;
};

struct SResultPlayerRow
{
	uint32_t nId;
	char szName[64];
	int32_t nLevel;
	int64_t nGold;
};
#pragma pack(pop)

// =============================================
// CBaseDbsConn 子类: 在 DB 线程中处理命令
// =============================================
class CSampleDbsConn : public CBaseDbsConn
{
	DECLARE_DYNAMIC_CLASS(CSampleDbsConn);

	IDbStatement* m_pStmtQuery;
	IDbStatement* m_pStmtInsert;

public:
	CSampleDbsConn()
		: m_pStmtQuery(nullptr)
		, m_pStmtInsert(nullptr)
	{
	}

	void OnConnected(uint32_t nThreadIndex, IDbConnection** aryDbsConn,
		uint8_t nConnCount, bool bIsRecord) override
	{
		IDbConnection* pConn = aryDbsConn[0];
		std::cout << "[DbThread " << nThreadIndex << "] Connected, DBId="
			<< pConn->GetDBId() << std::endl;

		pConn->Execute(
			"CREATE TABLE IF NOT EXISTS t_player ("
			"  id       INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,"
			"  name     VARCHAR(64) NOT NULL DEFAULT '',"
			"  level    INT NOT NULL DEFAULT 1,"
			"  gold     BIGINT NOT NULL DEFAULT 0"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
		);

		m_pStmtQuery = pConn->CreateDynamicStatement(
			"SELECT id, name, level, gold FROM t_player WHERE level >= ?");
		m_pStmtInsert = pConn->CreateDynamicStatement(
			"INSERT INTO t_player(name, level, gold) VALUES(?, ?, ?)");
	}

	void OnDisConnect(IDbConnection** aryDbsConn, uint8_t nConnCount) override
	{
		std::cout << "[DbThread] Disconnected." << std::endl;
		if (m_pStmtQuery) { m_pStmtQuery->Release(); m_pStmtQuery = nullptr; }
		if (m_pStmtInsert) { m_pStmtInsert->Release(); m_pStmtInsert = nullptr; }
	}

	void OnShellMsg(IDbConnection** aryDbsConn, uint8_t nConnCount,
		IResultHolder* pRH, const void* pCmd, size_t nSize) override
	{
		EDbCmd eCmd = *(const EDbCmd*)pCmd;

		switch (eCmd)
		{
		case eDbCmd_QueryPlayer:
			HandleQueryPlayer(pRH, pCmd, nSize);
			break;
		case eDbCmd_InsertPlayer:
			HandleInsertPlayer(pRH, pCmd, nSize);
			break;
		default:
			std::cerr << "[DbThread] Unknown cmd: " << (int)eCmd << std::endl;
			break;
		}
	}

private:
	void HandleQueryPlayer(IResultHolder* pRH, const void* pCmd, size_t nSize)
	{
		auto* pReq = (const SCmdQueryPlayer*)pCmd;

		int32_t nMinLevel = pReq->nMinLevel;
		m_pStmtQuery->SetParamInt32(&nMinLevel, 0);

		SResultPlayerRow row = {};
		ulong nameLen = 0;
		bool isNull[4] = {};
		bool isError[4] = {};

		m_pStmtQuery->SetResultUint32(&row.nId, &isNull[0], &isError[0], 0);
		m_pStmtQuery->SetResultText(row.szName, sizeof(row.szName), &nameLen, &isNull[1], &isError[1], 1);
		m_pStmtQuery->SetResultInt32(&row.nLevel, &isNull[2], &isError[2], 2);
		m_pStmtQuery->SetResultInt64(&row.nGold, &isNull[3], &isError[3], 3);

		m_pStmtQuery->Execute();

		uint32_t nRows = m_pStmtQuery->GetResultRowNum();
		for (uint32_t i = 0; i < nRows; ++i)
		{
			memset(&row, 0, sizeof(row));
			m_pStmtQuery->FetchResultRow(i);
			pRH->Write(&row, sizeof(row));
			pRH->Segment();
		}
	}

	void HandleInsertPlayer(IResultHolder* pRH, const void* pCmd, size_t nSize)
	{
		auto* pReq = (const SCmdInsertPlayer*)pCmd;

		char szName[64] = {};
		strncpy(szName, pReq->szName, sizeof(szName) - 1);
		ulong nameLen = (ulong)strlen(szName);
		int32_t nLevel = pReq->nLevel;
		int64_t nGold = pReq->nGold;

		m_pStmtInsert->SetParamText(szName, sizeof(szName), &nameLen, 0);
		m_pStmtInsert->SetParamInt32(&nLevel, 1);
		m_pStmtInsert->SetParamInt64(&nGold, 2);
		m_pStmtInsert->Execute();

		uint32_t nAffected = m_pStmtInsert->GetAffectRowNum();
		pRH->Write(&nAffected, sizeof(nAffected));
	}
};

DEFINE_DYNAMIC_CLASS(CSampleDbsConn, 4, GET_CLASS_ID(CSampleDbsConn));

// =============================================
// IDbsQueryHandler 实现: 在主线程接收结果
// =============================================
class CSampleQueryHandler : public IDbsQueryHandler
{
	EDbCmd m_eLastCmd;
public:
	CSampleQueryHandler() : m_eLastCmd(eDbCmd_QueryPlayer) {}
	void SetLastCmd(EDbCmd eCmd) { m_eLastCmd = eCmd; }

	size_t OnQueryResult(const void* pResult, size_t nSize) override
	{
		switch (m_eLastCmd)
		{
		case eDbCmd_QueryPlayer:
		{
			if (nSize >= sizeof(SResultPlayerRow))
			{
				auto* pRow = (const SResultPlayerRow*)pResult;
				std::cout << "  [Result] id=" << pRow->nId
					<< " name=" << pRow->szName
					<< " level=" << pRow->nLevel
					<< " gold=" << pRow->nGold << std::endl;
			}
			break;
		}
		case eDbCmd_InsertPlayer:
		{
			if (nSize >= sizeof(uint32_t))
			{
				uint32_t nAffected = *(const uint32_t*)pResult;
				std::cout << "  [Result] Insert affected=" << nAffected << std::endl;
			}
			break;
		}
		default:
			break;
		}
		return nSize;
	}
};

// =============================================
// main
// =============================================
int main(int argc, char* argv[])
{
	// --- 1. 创建 IDbsThreadMgr (简化版工厂) ---
	IDbsThreadMgr* pMgr = CreateDbsThreadMgr(
		2,                                  // nThreadCount: 2 个工作线程
		CSampleDbsConn::GetID(),            // nDbsConnClassID
		"127.0.0.1",                        // host
		3306,                               // port
		"test_db",                          // database
		"root",                             // user
		"123456",                           // password
		true                                // bFoundAsUpdateRow
	);
	std::cout << "[OK] DbsThreadMgr created with 2 threads." << std::endl;

	// --- 2. 创建 IDbsQuery ---
	CSampleQueryHandler handler;
	IDbsQuery* pQuery = pMgr->CreateDbsQuery(&handler);
	std::cout << "[OK] DbsQuery created." << std::endl;

	// --- 3. 发送 Insert 命令 ---
	{
		handler.SetLastCmd(eDbCmd_InsertPlayer);

		SCmdInsertPlayer cmd = {};
		cmd.eCmd = eDbCmd_InsertPlayer;
		strncpy(cmd.szName, "Alice", sizeof(cmd.szName) - 1);
		cmd.nLevel = 10;
		cmd.nGold = 5000;
		pQuery->Query(0, &cmd, sizeof(cmd));
		std::cout << "[Send] InsertPlayer 'Alice' on channel 0" << std::endl;

		cmd = {};
		cmd.eCmd = eDbCmd_InsertPlayer;
		strncpy(cmd.szName, "Bob", sizeof(cmd.szName) - 1);
		cmd.nLevel = 20;
		cmd.nGold = 12000;
		pQuery->Query(1, &cmd, sizeof(cmd));
		std::cout << "[Send] InsertPlayer 'Bob' on channel 1" << std::endl;
	}

	// 等待 DB 线程处理并轮询结果
	for (int i = 0; i < 20; ++i)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		pMgr->Check();
		if (pMgr->GetAllCmdWaitingCount() == 0 && pMgr->GetAllResultWaitingCount() == 0)
			break;
	}
	std::cout << std::endl;

	// --- 4. 发送 Query 命令 ---
	{
		handler.SetLastCmd(eDbCmd_QueryPlayer);

		SCmdQueryPlayer cmd = {};
		cmd.eCmd = eDbCmd_QueryPlayer;
		cmd.nMinLevel = 1;
		pQuery->Query(0, &cmd, sizeof(cmd));
		std::cout << "[Send] QueryPlayer (level>=1) on channel 0" << std::endl;
	}

	// 轮询等待查询结果
	for (int i = 0; i < 20; ++i)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		pMgr->Check();
		if (pMgr->GetAllCmdWaitingCount() == 0 && pMgr->GetAllResultWaitingCount() == 0)
			break;
	}

	// --- 5. 查看队列状态 ---
	std::cout << "\n[Info] AllCmdWaiting=" << pMgr->GetAllCmdWaitingCount()
		<< " AllResultWaiting=" << pMgr->GetAllResultWaitingCount() << std::endl;

	// --- 6. 清理 ---
	pQuery->Release();
	pMgr->Release();
	std::cout << "[OK] Cleaned up." << std::endl;

	return 0;
}
