#pragma once
#include "GammaConnects/TBasePrtlMsg.h"

typedef Gamma::TBasePrtlMsg<uint16> CShellPrt;
/************************************************************************/
/* shm 与 db 之间的通讯定义		 						               */
/************************************************************************/

#pragma pack(push,1)

#undef  Cmd_DBBegin
#undef  Cmd_DBEnd

#define TABLE_NAME_LEN 50

//字段名最长
#define FIELD_NAME_LEN 50


class CDbPrt : public CShellPrt
{
protected:
	CDbPrt( uint16 CommandId )
		: CShellPrt( CommandId ){};
};

#define Cmd_DBBegin(CommandName,CommandId) \
class CommandName : public CDbPrt\
{\
public:\
	CommandName() : CDbPrt( (uint16)CommandId ){}\
	static uint16 GetIdByType()		{ return (uint16)CommandId; }\
	static size_t GetHeaderSize()	{ return sizeof(CommandName); }\
	static const char* GetName()	{ return #CommandName; }

#define Cmd_DBEnd };

enum ECommonDBQuery
{
	eCommonDBQ_WrapQuery,
	eCommonDBQ_CommonQuery,
	eCommonDBQ_CharShmQuery,
	eCommonDBQ_Count,
};

Cmd_DBBegin(CCommonDBQ_CommonQuery, eCommonDBQ_CommonQuery)
	uint8	nDBOpType;	
	uint8	nHandlerType;
	uint16	nTableID;
	uint32  nHandlerSerialID;
	uint32	nQueryTime;
	char	szKeyFieldName[FIELD_NAME_LEN+1];
Cmd_DBEnd

Cmd_DBBegin(CCommonDBQ_CharShmQuery, eCommonDBQ_CharShmQuery)
	int64 nCharID;
	int64 nBitArray;
	uint64 pContext;
Cmd_DBEnd

Cmd_DBBegin(CCommonDBQ_WrapQuery, eCommonDBQ_WrapQuery)
	uint32 nThreadID;
	uint64 nQuerySerialID;
Cmd_DBEnd


enum ECommonDBAnswer
{
	eCommonDBA_WrapAnswer,
	eCommonDBA_CommonAnswer,
	eCommonDBA_CharShmAnswer,
	eCommonDBA_Log,
	eCommonDBA_Count,
};

Cmd_DBBegin(CCommonDBA_CommonAnswer, eCommonDBA_CommonAnswer)
	uint8	nDBOpType;
	uint8	nHandlerType;
	uint16	nTableID;
	uint32  nHandlerSerialID;
	int32	nResult;
	uint32	nVersion;
	int64	nUUID;
	uint32	nQueryTime;
Cmd_DBEnd

Cmd_DBBegin(CCommonDBA_CharShmAnswer, eCommonDBA_CharShmAnswer)
	uint8	bSucceed;
	int64	nCharID;
	uint32	nBufferLen;
	uint64  nLuaContxt;
	Cmd_DBEnd

Cmd_DBBegin( CCommonDBA_WrapAnswer, eCommonDBA_WrapAnswer )
	uint32 nThreadID;
	uint64 nQuerySerialID;
Cmd_DBEnd

Cmd_DBBegin( CCommonDBA_Log, eCommonDBA_Log )
	uint16 nLogLen;
Cmd_DBEnd
#pragma pack(pop)

