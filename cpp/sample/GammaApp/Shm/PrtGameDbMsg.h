#pragma once
#include "Help/PrtDbMsg.h"
/************************************************************************/
/* shm 与 db 之间的通讯定义		 						               */
/************************************************************************/

#pragma pack(push,1)

//============================================
// 数据库查询命令ID。
//============================================
enum EGameOperate
{
	eGameOperate_Begin = 35000,
	eGameRebort = eGameOperate_Begin,
	eGameShmCommitBlocksInfo,
	eGameShmCommitFlag,
	eGameModifyCharName,
	eGameOperate_End,
};
	
Cmd_DBBegin(CGameRebort, eGameRebort)
Cmd_DBEnd

Cmd_DBBegin(CGameShmCommitBlocksInfo, eGameShmCommitBlocksInfo)
	uint32 nTime;
Cmd_DBEnd

Cmd_DBBegin(CGameShmCommitFlag, eGameShmCommitFlag)
	uint16 nLoginGasID;
	uint32 nTime;
Cmd_DBEnd

Cmd_DBBegin(CGameModifyCharName, eGameModifyCharName)
	int64 nCharID;
	char szName[22];
	int8 fromType;
Cmd_DBEnd

//============================================
// 数据库查询回应ID。
//============================================
enum EGameOperateResult
{
	eGameOperateResult_Begin = 53000,
	eGameRebortResult = eGameOperateResult_Begin,
	eGameShmCommitBlocksInfoResult,
	eGameShmCommitFlagResult,
	eGameModifyCharNameResult,
	eGameOperateResult_End,
};

Cmd_DBBegin(CGameRebortResult, eGameRebortResult)
	uint8 bFailed;
Cmd_DBEnd

Cmd_DBBegin(CGameShmCommitBlocksInfoResult, eGameShmCommitBlocksInfoResult)
	uint8 bFailed;
Cmd_DBEnd

Cmd_DBBegin(CGameShmCommitFlagResult, eGameShmCommitFlagResult)
	uint8 bFailed;
Cmd_DBEnd

Cmd_DBBegin(CGameModifyCharNameResult, eGameModifyCharNameResult)
	int64 nCharID;
	int8 nResult;
	char szCharName[22];
	int8 fromType;
Cmd_DBEnd


#pragma pack(pop)

