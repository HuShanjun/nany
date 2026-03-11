#pragma once
#include "GammaCommon/GammaHelp.h"
#include "GammaShm/CBaseShareMemory.h"
#include <string>

using namespace std;
using namespace Gamma;

namespace Gamma
{	
	enum ECmdType
	{
		eQueryType_ShmReboot,
		eQueryType_CommitBlock,
		eQueryType_CommitFlag,
		eQueryType_CommitAllBlocksOK,
		eQueryType_Count,
	};
	
	struct SShmCmdHead
	{
		uint8_t	m_nType;
		bool	m_bFailed;
	};

	struct SCommitData
	{
		SCommitData() 
			: m_bDataInited( false )
			, m_nBlockCommitID( 0 ){}
		bool			m_bDataInited;
		uint32_t			m_nBlockCommitID;
	};

	struct SLogBuffChange
	{
		uint32_t			m_nCommitID;
		uint32_t			m_nReadOffset;
	};

	struct SLogName
	{
		SLogName():m_nSerialNum(0),m_nCreateTime(0),m_nGasTime(0) {};
		string			m_strFileName;
		uint32_t			m_nCreateTime;
		uint32_t			m_nSerialNum;
		uint32_t			m_nGasTime;
	};

	struct SModInfo
	{
		uint16_t			m_nDBGasID;
		uint64_t			m_nID;
		uint32_t			m_nQueryChannel;
	};

	typedef map<void*, uint32_t> CCommitMap;
	typedef map<SChunckHead*, SModInfo> CModFlagMap;

	struct STrunkTypeData
	{
		STrunkTypeData() : m_nCommitID(0){}
		uint32_t			m_nCommitID;
		CCommitMap		m_mapCommitData;
		uint32_t			m_nDBCost;
	};
}