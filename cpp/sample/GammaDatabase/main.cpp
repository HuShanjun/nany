#include "GammaDatabase/IDatabase.h"
#include <iostream>
#include <cstring>

using namespace Gamma;

int main(int argc, char* argv[])
{
	IDatabase* pDB = GetDatabase();
	IDbConnection* pConn = nullptr;

	try
	{
		pConn = pDB->CreateConnection(
			"127.0.0.1",   // host
			3306,           // port
			"root",         // user
			"123456",       // password
			"test_db",      // database
			1,              // nDBId
			true,           // bFoundAsUpdateRow
			false,          // bMultiStatements
			true            // bPingReconnect
		);
		std::cout << "[OK] Connected, DBId=" << pConn->GetDBId() << std::endl;
	}
	catch (std::string& err)
	{
		std::cerr << "[FAIL] " << err << std::endl;
		return -1;
	}

	// =========================================
	// 1. 直接执行 SQL + IDbTextResult
	// =========================================
	try
	{
		pConn->Execute(
			"CREATE TABLE IF NOT EXISTS t_player ("
			"  id       INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,"
			"  name     VARCHAR(64) NOT NULL DEFAULT '',"
			"  level    INT NOT NULL DEFAULT 1,"
			"  gold     BIGINT NOT NULL DEFAULT 0"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
		);
		std::cout << "[OK] Table t_player ensured." << std::endl;

		pConn->Execute("INSERT IGNORE INTO t_player(name, level, gold) VALUES('Alice', 10, 5000)");
		pConn->Execute("INSERT IGNORE INTO t_player(name, level, gold) VALUES('Bob', 20, 12000)");
		std::cout << "[OK] Test data inserted. LastInsertId=" << pConn->LastInsertId() << std::endl;

		IDbTextResult* pResult = pConn->Execute("SELECT id, name, level, gold FROM t_player");
		if (pResult)
		{
			uint32_t nRows = pResult->GetRowNum();
			uint32_t nCols = pResult->GetColNum();
			std::cout << "[Query] rows=" << nRows << " cols=" << nCols << std::endl;

			for (uint32_t r = 0; r < nRows; ++r)
			{
				pResult->Locate(r);
				std::cout << "  row " << r << ": ";
				for (uint32_t c = 0; c < nCols; ++c)
				{
					const char* val = pResult->GetData(c);
					std::cout << (val ? val : "NULL") << "\t";
				}
				std::cout << std::endl;
			}
			pResult->Release();
		}
	}
	catch (std::string& err)
	{
		std::cerr << "[FAIL] Execute: " << err << std::endl;
	}

	// =========================================
	// 2. 预编译语句 (CreateStatement)
	// =========================================
	try
	{
		// INSERT
		IDbStatement* pInsert = pConn->CreateStatement(
			"INSERT INTO t_player(name, level, gold) VALUES(?, ?, ?)");

		char szName[64] = "Charlie";
		ulong nameLen = (ulong)strlen(szName);
		int32_t nLevel = 30;
		int64_t nGold = 99999;

		pInsert->SetParamText(szName, sizeof(szName), &nameLen, 0);
		pInsert->SetParamInt32(&nLevel, 1);
		pInsert->SetParamInt64(&nGold, 2);
		pInsert->Execute();

		std::cout << "[Prepared INSERT] InsertId=" << pInsert->GetInsertID()
			<< " AffectedRows=" << pInsert->GetAffectRowNum() << std::endl;
		pInsert->Release();

		// SELECT with result binding
		IDbStatement* pSelect = pConn->CreateStatement(
			"SELECT id, name, level, gold FROM t_player WHERE level >= ?");

		int32_t nMinLevel = 15;
		pSelect->SetParamInt32(&nMinLevel, 0);

		uint32_t resId = 0;
		char     resName[64] = {};
		ulong    resNameLen = 0;
		int32_t  resLevel = 0;
		int64_t  resGold = 0;
		bool     isNull[4] = {};
		bool     isError[4] = {};

		pSelect->SetResultUint32(&resId, &isNull[0], &isError[0], 0);
		pSelect->SetResultText(resName, sizeof(resName), &resNameLen, &isNull[1], &isError[1], 1);
		pSelect->SetResultInt32(&resLevel, &isNull[2], &isError[2], 2);
		pSelect->SetResultInt64(&resGold, &isNull[3], &isError[3], 3);

		pSelect->Execute();

		uint32_t nRows = pSelect->GetResultRowNum();
		std::cout << "[Prepared SELECT] rows=" << nRows << std::endl;

		for (uint32_t i = 0; i < nRows; ++i)
		{
			pSelect->FetchResultRow(i);
			std::cout << "  id=" << resId
				<< " name=" << resName
				<< " level=" << resLevel
				<< " gold=" << resGold << std::endl;
		}
		pSelect->Release();
	}
	catch (std::string& err)
	{
		std::cerr << "[FAIL] Prepared Statement: " << err << std::endl;
	}

	// =========================================
	// 3. 动态语句 (CreateDynamicStatement)
	// =========================================
	try
	{
		IDbStatement* pDynSelect = pConn->CreateDynamicStatement(
			"SELECT id, name FROM t_player WHERE gold > ?");

		int64_t nMinGold = 10000;
		pDynSelect->SetParamInt64(&nMinGold, 0);

		uint32_t dynId = 0;
		char     dynName[64] = {};
		ulong    dynNameLen = 0;
		bool     dynNull[2] = {};
		bool     dynErr[2] = {};

		pDynSelect->SetResultUint32(&dynId, &dynNull[0], &dynErr[0], 0);
		pDynSelect->SetResultText(dynName, sizeof(dynName), &dynNameLen, &dynNull[1], &dynErr[1], 1);

		pDynSelect->Execute();

		uint32_t nRows = pDynSelect->GetResultRowNum();
		std::cout << "[Dynamic SELECT] rows=" << nRows << std::endl;

		for (uint32_t i = 0; i < nRows; ++i)
		{
			pDynSelect->FetchResultRow(i);
			std::cout << "  id=" << dynId << " name=" << dynName << std::endl;
		}
		pDynSelect->Release();
	}
	catch (std::string& err)
	{
		std::cerr << "[FAIL] Dynamic Statement: " << err << std::endl;
	}

	// =========================================
	// 4. 事务
	// =========================================
	try
	{
		pConn->StartTran();
		pConn->Execute("UPDATE t_player SET gold = gold + 1000 WHERE name = 'Alice'");
		pConn->Execute("UPDATE t_player SET gold = gold - 1000 WHERE name = 'Bob'");
		pConn->EndTran();
		std::cout << "[OK] Transaction committed." << std::endl;
	}
	catch (std::string& err)
	{
		pConn->CancelTran();
		std::cerr << "[FAIL] Transaction rolled back: " << err << std::endl;
	}

	// =========================================
	// 5. ExecuteWithoutException + FormatCode
	// =========================================
	{
		IDbStatement* pStmt = pConn->CreateStatement(
			"INSERT INTO t_player(name, level, gold) VALUES(?, ?, ?)");

		char szName[64] = "Alice";
		ulong nameLen = (ulong)strlen(szName);
		int32_t nLevel = 10;
		int64_t nGold = 5000;

		pStmt->SetParamText(szName, sizeof(szName), &nameLen, 0);
		pStmt->SetParamInt32(&nLevel, 1);
		pStmt->SetParamInt64(&nGold, 2);

		uint32_t nErrCode = pStmt->ExecuteWithoutException();
		if (nErrCode != 0)
		{
			EErrorCodes eCode = pDB->FormatCode(nErrCode);
			if (eCode == eEC_DupEntry)
				std::cout << "[Warn] Duplicate entry, skipped." << std::endl;
			else
				std::cerr << "[Error] code=" << nErrCode
					<< " msg=" << pStmt->GetExecuteError() << std::endl;
		}
		else
		{
			std::cout << "[OK] Insert without exception succeeded." << std::endl;
		}
		pStmt->Release();
	}

	// =========================================
	// 6. 工具 API
	// =========================================
	{
		std::cout << "[Ping] " << (pConn->Ping() ? "alive" : "dead") << std::endl;

		const char* szRaw = "it's a test";
		char szEscaped[128] = {};
		uint32_t len = pConn->EscapeString(szEscaped, szRaw, (uint32_t)strlen(szRaw));
		std::cout << "[Escape] \"" << szRaw << "\" -> \"" << szEscaped
			<< "\" (len=" << len << ")" << std::endl;

		std::cout << "[Info] TotalStatement=" << pConn->GetTotalStatement() << std::endl;
	}

	pConn->Release();
	std::cout << "[OK] Connection released." << std::endl;
	return 0;
}
