#pragma once
#include "GammaCommon/GammaCommonType.h"
#include <set>

namespace Gamma
{
#pragma pack(push,1)
	/**
	除了数据状态以外，其他状态直接影响子数据
	*/
	enum EShareState
	{
		eSCT_Invalid,		//初始化数据
		eSCT_UpdateBlock,	//需要更新块数据
		eSCT_UpdateTrunk,	//需要全部提交
		eSCT_UpdateFlg,		//需要更新flg
		eSCT_WaitingFlg,	//最后等待
		eSCT_Count,
	};

	// 大数据块类型信息
	// 一个sharememory文件可能会有多种数据块类型，
	// 如角色，帮会等，由游戏定义
	struct SChunckTypeInfo
	{
		uint32_t			m_bCommitFlagRequire;
		uint32_t			m_nChunkOffset;
		uint32_t			m_nChunkSize;
		uint32_t			m_nChunkCount;
	};

	// 大数据块头部结构
	struct SChunckHead
	{
		uint64_t			m_nChunckID;
		uint32_t			m_nChunkHeadSize;
		uint32_t			m_nBlockCount;
		uint16_t			m_nDBGasID;
		uint16_t			m_nDataState;
		uint32_t			m_nDataVersion;

		uint32_t			m_nCommitIDFirst;	// 当前数据提交ID
		uint32_t			m_nCommitIDLast;	// 当前数据提交ID
		
		uint16_t			m_nQueryCheck;		//	提交检查
	};

	// 小数据块类型信息
	// 一个大数据块可能会有多种小数据块类型，
	// 如角色的技能、道具
	struct SBlockInfo
	{
		uint32_t			m_nType;
		uint32_t			m_nDataOffset;
		uint32_t			m_nMaxBlockSize;
		uint32_t			m_nArrayStartOff;	// 数组结构起始位置
		uint32_t			m_nArrayElemSize;	// 数组元素大小
		union
		{
			uint64_t		m_ptPoint;
			void*		m_pContextData;
		};
	};

	// 用于计算数组大小
	struct SSizeType
	{
		SSizeType() : m_nSize(0){}
		uint32_t m_nSize;
	};


	class CDataContainer
	{
	protected:
		bool			m_bCloned;
		int64_t			m_UUID;
		std::set<int64_t> m_AllObserverID;

	public:
		CDataContainer( uint64_t uuid = 0 )
			: m_bCloned( false )
			, m_UUID(uuid){}
		virtual ~CDataContainer() {}
		virtual uint8_t GetDataDefineType() const = 0;
		virtual void  UpdateDB( uint8_t _eCTType, uint16_t _nTableID, int64_t nDataUUID, uint16_t _nFieldIndex) = 0;
		virtual void  OnDBResult( uint8_t _eCTType, uint16_t _nTableID, int64_t nDataUUID ) = 0;
		void SetUUID(int64_t nID) { m_UUID = nID; }
		int64_t GetUUID() const { return m_UUID; }
		bool AddObserver(int64_t nID)
		{
			if (m_AllObserverID.find(nID) != m_AllObserverID.end())
				return false;
			m_AllObserverID.insert(nID);
			return true;
		}
		void DelObserver(int64_t nID)
		{
			if (-1 == nID)
				m_AllObserverID.clear();
			else
				m_AllObserverID.erase(nID);
		}
		const std::set<int64_t>& GetAllObserverID() const
		{
			return m_AllObserverID;
		}
		void SetCloned(bool bValue) { m_bCloned = bValue; }
		bool IsCloned() const { return m_bCloned; }
	};


	// 一个BlockData只能有一个用于diff的数组结构，
	// 如果有多个数组结构需要diff，那么需要拆开成两个BlockData，
	// 如果有需要diff的数组结构，必定需要以SSizeType作为数组的开始，表示数组元素的个数。
	// 当有需要diff的数组结构的时候m_nArrayElemSize为数组元素的大小，
	// 当没有需要diff的数组结构的时候，m_nArrayElemSize为0
	struct SBlockData
	{
		uint32_t			m_nQueryDataVersion;	//请求数据版本
		uint32_t			m_nAnswerDataVersion;	//应答数据版本
		uint32_t			m_nBlockInfoIndex;	// SBlockInfo
		uint32_t			m_nRegistForCommit;	// 是否已经注册提交,INVALID_32BITID 表示不用提交，否则表示数据块的索引，用于服务器创建对象不成功的时候从commit列表删除
		uint32_t			m_nCommitIDFirst;	// 当前数据提交ID
		uint32_t			m_nCommitIDLast;	// 当前数据提交ID

		//观察者
		union
		{
			uint64_t				m_ptPoint;
			CDataContainer*	m_pDataContainter;
		};

		bool			m_bSaveDB;
		uint8_t			m_ShmStateMask;	//通用表 shm中，本块数据在shm中的状态
		//db操作使用
		uint16_t			m_nDataTableID;
		uint32_t			m_nDataSize;
		int64_t			m_UUID;	//必须为最后一个变量
		SBlockData( uint16_t nDataTableID = 0, uint32_t nDataSize = 0, bool bSaveDB = false, uint64_t uuid = 0  )
			: m_ptPoint(0)
			, m_nDataTableID(nDataTableID)
			, m_nDataSize(nDataSize) 
			, m_UUID(uuid)
			, m_nQueryDataVersion(0)
			, m_nAnswerDataVersion(0)
			, m_nBlockInfoIndex(0)
			, m_nRegistForCommit(0)
			, m_nCommitIDFirst(0)
			, m_nCommitIDLast(0)
			, m_bSaveDB(bSaveDB)
			, m_ShmStateMask( 0 ){}

		void SetDataContainter(CDataContainer* pDataObserverID)
		{
			m_pDataContainter = pDataObserverID;
		}
		CDataContainer* GetDataContainter() const
		{
			return m_pDataContainter;
		}
		uint16_t GetDataTableID() { return m_nDataTableID; }
		uint32_t GetDataSize() { return m_nDataSize; }
		int64_t	GetUUID() const
		{
			return m_UUID;
		}
		//返回值用于反射用，表示该表中第几个字段
		int32_t	SetUUID(int64_t uuid, bool bSimple = false )
		{
			m_UUID = uuid;
			return 0;
		}
		void SetSaveDB(bool bValue) { m_bSaveDB = bValue; }
		bool IsSaveDB() { return m_bSaveDB; }

		void SetShmState( uint8_t nState, bool bSet )
		{
			if (bSet)
				m_ShmStateMask |= (1 << nState);
			else
				m_ShmStateMask &= ~(1 << nState);
		}
		bool HasShmState( uint8_t nState )
		{
			return ( m_ShmStateMask & (1 << nState) ) == (1 << nState);
		}
	};

	// 所有的共享内存必须是以下的内存头
	// 实际上Gamma层以外访问的SShareCommonHead结构是来源于堆上
	// 而core层里面除了堆上有一个SShareCommonHead结构以外，
	// 同时在共享文件的映射内存上构造了一个同样结构的对象
	struct SShareCommonHead
	{
		uint32_t			m_nVersion;
		uint32_t			m_nSizeOfShareFlie;

		// Trunk信息
		uint32_t			m_nChunkTypeCount;

		/* 后面紧跟TrunkType信息
		SChunckTypeInfo	m_aryChunkTypeInfo[0];
		*/
	};

/**
     --------------------------------------------------------------------------------------------------
	| 	状态				| 生成 |               Gas可以改变状态为               | Shm可以改变状态为 |
	|-----------------------|------|-----------------------------------------------|-------------------|
	| eSCT_Invalid			|  Shm |              eSCT_UpdateBlock                 |                   |
	|-----------------------|------|-----------------------------------------------|-------------------|
	| eSCT_UpdateBlock      |  Gas |              eSCT_UpdateTrunk                 |                   |
	|-----------------------|------|-----------------------------------------------|-------------------|
	| eSCT_UpdateTrunk      |  Gas |                                               |  eSCT_UpdateFlg   |
	|-----------------------|------|-----------------------------------------------|-------------------|
	| eSCT_UpdateFlg        |  Shm |                                               |  eSCT_WaitingFlg  |
	|-----------------------|------|-----------------------------------------------|-------------------|
	| eSCT_WaitingFlg       |  Shm |                                               |  eSCT_Invalid     |
	 ---------------------------------------------------------------------------------------------------
	
		
	 			     								结构描述
	      					  ---------------------------------------------------
							 |                 SShareCommonHead                  |
							 |---------------------------------------------------|
							 |                   SPlayerInfo 1                   |
							 |---------------------------------------------------|
							 |                    SBlockInfo 1                   |
							 |---------------------------------------------------|
							 |                    SBlockInfo 2                   |
							 |---------------------------------------------------|
							 |                    SBlockInfo 3                   |
							 |---------------------------------------------------|
							 |                    SBlockInfo n                   |
							 |---------------------------------------------------|
							 |               Data for SBlockInfo 1               |
							 |---------------------------------------------------|
							 |               Data for SBlockInfo 2               |
							 |---------------------------------------------------|
							 |               Data for SBlockInfo 3               |
							 |---------------------------------------------------|
							 |               Data for SBlockInfo n               |
							 |---------------------------------------------------|
							 |                   SPlayerInfo 2                   |
							 |---------------------------------------------------|
							 |                    SBlockInfo 1                   |
							 |---------------------------------------------------|
							 |                    SBlockInfo 2                   |
							 |---------------------------------------------------|
							 |                    SBlockInfo 3                   |
							 |---------------------------------------------------|
							 |                    SBlockInfo n                   |
							 |---------------------------------------------------|
							 |               Data for SBlockInfo 1               |
							 |---------------------------------------------------|
							 |               Data for SBlockInfo 2               |
							 |---------------------------------------------------|
							 |               Data for SBlockInfo 3               |
							 |---------------------------------------------------|
							 |               Data for SBlockInfo n               |
							 |---------------------------------------------------|
							 |     .........................................     |
							 |---------------------------------------------------|

@ 关于SBlockInfo
	m_nType 完全由数据定义方（例如游戏逻辑，也就是GameServer定义）定义其含义，
		主要用来标识数据块类型，例如m_nType作为玩家ID
	m_nOffsetFromPlayerInfo表示数据块相对于SPlayerInfo数据块的偏移，例如上图中
		Data for SBlockInfo 1相对于SPlayerInfo 1的偏移
	m_nSizeOfBlocks 当前数据块大小
	*/
	struct SDBDebug
	{
		uint32_t			m_nQueryTime;	//发起
		uint32_t			m_nMainThreadQueryTime;
		uint32_t			m_nDBStartTime;	//
		uint32_t			m_nDBResultTime;
		uint32_t			m_nMainThreadFinishTime;
		uint32_t			m_nDBCost;
	};

	struct SDBBlockInfo : SDBDebug
	{
		uint32_t			m_nTrunkType;
		uint32_t			m_nTrunkIndex;
		uint16_t			m_nDBGasID;		
		uint64_t			m_nTrunkID;
	};

	struct SCommitBlocksInfo : SDBBlockInfo
	{
		uint32_t			m_nCommitID;
		uint32_t			m_nCurDataVersion;
		uint32_t			m_nBlockCount;
	};

	struct SCommitFlag : SDBBlockInfo
	{
	};
#pragma pack(pop)
}
