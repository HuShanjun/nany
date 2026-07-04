#pragma once
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/TGammaList.h"
#include "GammaShm/CBaseShareMemory.h"
#include "Help/ShmHelp.h"
#include "TableData.gen/DataDefine.h"
#include "TableData.gen/TableCommonDefine.h"
#include <vector>

using namespace Gamma;
using namespace std;

enum EShareMemVersion
{
	eSMV_Org = 0,
	eSMV_Count,
	eSMV_Cur = eSMV_Count - 1,
};

#pragma pack(push,1)


#define SetTrunkHead( nIndex, Chunks, bCommitFlagRequire ) \
{ \
	m_aryChunkTypeInfo[nIndex].m_bCommitFlagRequire = bCommitFlagRequire;\
	m_aryChunkTypeInfo[nIndex].m_nChunkOffset = (uint32)(((tbyte*)&Chunks[0]) - ((tbyte*)&m_aryChunkTypeInfo[nIndex]));\
	m_aryChunkTypeInfo[nIndex].m_nChunkSize = (uint32)sizeof( Chunks[0] );\
	m_aryChunkTypeInfo[nIndex].m_nChunkCount = ELEM_COUNT( Chunks );\
}

struct SGameShareFile
{
	SShareCommonHead	m_ShareFileHead;
	SChunckTypeInfo		m_aryChunkTypeInfo[eTrunkType_Count];
	CPlayerDataDefineShm	m_PlayersData[MAX_PLAYER_COUNT];
	SGameShareFile()
	{
		m_ShareFileHead.m_nSizeOfShareFlie = sizeof(SGameShareFile);
		m_ShareFileHead.m_nChunkTypeCount = ELEM_COUNT(m_aryChunkTypeInfo);
		m_ShareFileHead.m_nVersion = eSMV_Cur;
		SetTrunkHead(eTrunkType_Char, m_PlayersData, true);
	}
};

class CTrunkCommitInfo : public TGammaList<CTrunkCommitInfo>::CGammaListNode
{
public:
	// 每一个trunk需要提交的block数据指针
	vector<SBlockData*>	m_aryCommitBlockData;
	uint32				m_nCurCommitBlockCount;
	CTrunkCommitInfo() : m_nCurCommitBlockCount(0) {}
};

#pragma pack(pop)
