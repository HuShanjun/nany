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
		uint8	m_nType;
		bool	m_bFailed;
	};

	struct SCommitData
	{
		SCommitData() 
			: m_bDataInited( false )
			, m_nBlockCommitID( 0 ){}
		bool			m_bDataInited;
		uint32			m_nBlockCommitID;
	};

	struct SLogBuffChange
	{
		uint32			m_nCommitID;
		uint32			m_nReadOffset;
	};

	struct SLogName
	{
		SLogName():m_nSerialNum(0),m_nCreateTime(0),m_nGasTime(0) {};
		string			m_strFileName;
		uint32			m_nCreateTime;
		uint32			m_nSerialNum;
		uint32			m_nGasTime;
	};

	struct SModInfo
	{
		uint16			m_nDBGasID;
		uint64			m_nID;
		uint32			m_nQueryChannel;
	};

	typedef map<void*, uint32> CCommitMap;
	typedef map<SChunckHead*, SModInfo> CModFlagMap;

	struct STrunkTypeData
	{
		STrunkTypeData() : m_nCommitID(0){}
		uint32			m_nCommitID;
		CCommitMap		m_mapCommitData;
		uint32			m_nDBCost;
	};
}