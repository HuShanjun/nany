#pragma once
/************************************************************************/
/* 游戏db数据管理器，进程单一实例                                          */
/************************************************************************/
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaTime.h"
#include "GammaCommon/TGammaList.h"
#include "GammaShm/CBaseShareMemory.h"
#include "GammaShm/IShareMemoryMgr.h"
#include "Help/ShmHelp.h"
#include "CharDataDefine.h"


class CCharDataMgr
	: public IShareMemoryHandler
	, public CTick
{
public:
	CCharDataMgr();
	virtual ~CCharDataMgr();

	void OnInit();
	void Start();
	
private:
	enum
	{
		ePPM_NoSpace = -1,
		ePPM_Exist = -2
	};

	IShareMemoryMgr* m_pShmMgr;

	SGameShareFile	m_GameShareFile;
	SGameShareFile* m_pGameShareFile;


	//角色数据池使用标记
	typedef std::map<uint64, uint32> CID2ArrayIndex;
	typedef std::map<uint32, uint64> CIndex2ArrayID;
	struct SCharDataUseInfo
	{
		// 所有trunk的提交信息，每个TrunkData对应一个CTrunkCommitInfo
		// 约定：m_aryCommitTrunk[i] 与 SGameShareFile.m_PlayersData[i] 一一对应——下标 i 就是 Shm 槽位号。
		vector<CTrunkCommitInfo>		m_aryCommitTrunk; 
		// 待提交的trunk列表
		TGammaList<CTrunkCommitInfo>	m_aryPendingTrunks;
		// 空闲的trunk列表
		TGammaList<CTrunkCommitInfo>	m_aryFreeTrunks;
		// 使用中的trunk列表 
		// key: 角色ID，value: trunk索引
		CID2ArrayIndex					m_mapUsingTrunks;
		// 正在刷新的trunk列表
		// key: trunk索引，value: 角色ID
		CIndex2ArrayID					m_mapFlushingTrunks;
	};

	SCharDataUseInfo m_CharDataInfo[eTrunkType_Count];


	bool	IsDataReady();


	//定时器触发
	void OnTick();

	//IShareMemoryHandler overlap
	virtual void	OnReboot();
	virtual void	OnCommitBlocksInfo(const SCommitBlocksInfo* _pInfo, size_t _nSize);
	virtual void	OnCommitFlag(const SCommitFlag* _pInfo, size_t _nSize);
	virtual void	OnCommitAllBlocksOK(const SCommitFlag* _pInfo, size_t _nSize);


	//通过数据块计算得到数据类型及其索引
	pair<ETrunkType, uint32> CalculateTrunkInfo(const tbyte* _pOffset, SBlockData** _ppBlockData, SChunckHead*& _pTrunckHead);

	//提交数据
	void	CommitData(TGammaList<CTrunkCommitInfo>& _listTrunk, uint32 _nTrunkType);
	bool	CheckFlushingTrunk(ETrunkType _eType, uint32 _nTrunkIndex);
	uint16	CopyPropBlock(SBlockData* _pBlockDataDes, SBlockData* _pBlockDataSrc,
					uint32 _nMaxBlockSize, int32 _nDiffInfo[INVALID_16BITID]);


	//分配使用
	int32		GetFreeTrunk(ETrunkType _eTrunkType, uint64 _nTrunkID);
	SChunckHead* AllocDBData(ETrunkType _eTrunkType, uint64 _nTrunkID, const SChunckHead* _pData );
	bool		CommitTrunk(ETrunkType eType, SChunckHead* pTrunk);


public:
	IShareMemoryMgr* GetShmMgr() { return m_pShmMgr; }
	void	DumpPlayerData();
	bool	OnServerStop();

	//是否可以分配角色数据
	bool	EnableAllocData(ETrunkType _eTrunkType, uint64 _nTrunkID);

	//数据有修改时调用
	bool	RegistCommitData(tbyte* _pData, bool _bCheckDataStart);

	CPlayerDataDefineShm* AllocalPlayerData(uint64 _nOjbectID, const CPlayerDataDefineShm* pInitData);
	CPlayerDataDefineShm* GetPlayerData(uint64 _nCharID);

	void FreeCharDBData(uint64 nCharID, const void* pCharDB);

	void UpdateShmData( const CDataContainer* pContainter, void* pData, uint32 nOffset );

	void QueryCharInfo(uint64 _nOjbectID, uint64 _nBitArray, void* pLuaContext);

	void OnDataCommit(uint16 nBlockIndex, uint16 nCount, const void* szMaxBuffer, uint32 nSize);
	void OnPlayerDataResult(bool _bOk, int64 __uuid, const void* _pCharData);

private:
	template<typename DBResultMsg>
	void OnShellMsg( const DBResultMsg* _pCmd, size_t _nSize );
};
