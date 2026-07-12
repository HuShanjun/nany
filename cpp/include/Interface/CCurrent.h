#pragma once
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/CRefshare.h"
#include "GammaConnects/CBaseConn.h"
#include "GammaConnects/TBasePrtlMsg.h"

typedef Gamma::TBasePrtlMsg<uint16> CShellPrt;
#define IS_STRUCT_MSG(T) (((T)&0x8000) > 0)
#define IS_PROTO_MSG(T) (((T)&0x8000) == 0)

enum  ECurrentValue
{
	ECurrentValue_ServerID = 1,//服务器id
	ECurrentValue_PlayerID = 2,//玩家id
};

struct ST_CURRENT_VALUE
{
	enum
	{
		VALUE_INT32,
		VALUE_INT64,
		VALUE_PTR
	}m_Type;
	union
	{
		int32 nNum;
		int64 nBigNum;
	}m_Value;
	CRefSharePtr m_pPtr;
	ST_CURRENT_VALUE()
	{
		m_pPtr = nullptr;
	}

	ST_CURRENT_VALUE(const struct ST_CURRENT_VALUE& Other)
	{
		m_Type = Other.m_Type;
		m_Value.nNum = Other.m_Value.nNum;
		m_Value.nBigNum = Other.m_Value.nBigNum;
		m_pPtr = Other.m_pPtr;
	}

	ST_CURRENT_VALUE& operator= (const struct ST_CURRENT_VALUE& Other)
	{
		m_Type = Other.m_Type;
		m_Value.nNum = Other.m_Value.nNum;
		m_Value.nBigNum = Other.m_Value.nBigNum;
		m_pPtr = Other.m_pPtr;
		return *this;
	}

	~ST_CURRENT_VALUE()
	{
		if (m_Type == VALUE_PTR && m_pPtr != nullptr)
			m_pPtr = nullptr;
	}
};

class CCurrent : public Gamma::CBaseConn
{
private:
	//内部保存
	std::map<int, struct ST_CURRENT_VALUE> m_Values;
public :
	virtual ~CCurrent() {};
	virtual uint32 GetConnectID() = 0;
	virtual uint32 GetServerID() { return GetConnectID(); }
	virtual void SetServerID(uint32 nServerID) {}
	virtual void SetContextID(int64 nValue ) = 0;
	virtual int64 GetContextID() = 0;

	void SetInt(int Name, int32 Value)
	{
		ST_CURRENT_VALUE value;
		value.m_Type = ST_CURRENT_VALUE::VALUE_INT32;
		value.m_Value.nNum = Value;
		m_Values[Name] = value;
	}
	void SetInt64(int Name, int64 Value)
	{
		ST_CURRENT_VALUE value;
		value.m_Type = ST_CURRENT_VALUE::VALUE_INT64;
		value.m_Value.nBigNum = Value;
		m_Values[Name] = value;
	}

	void SetPtr(int Name, CRefSharePtr Value)
	{
		ST_CURRENT_VALUE value;
		value.m_Type = ST_CURRENT_VALUE::VALUE_PTR;
		value.m_pPtr = Value;
		m_Values[Name] = value;
	}

	void SetPtr(int Name, CRefShare* Value)
	{
		ST_CURRENT_VALUE value;
		value.m_Type = ST_CURRENT_VALUE::VALUE_PTR;
		value.m_pPtr = Value;
		m_Values[Name] = value;
	}

	bool IsExist(int Name)
	{
		auto i = m_Values.find(Name);
		if (i != m_Values.end())
			return true;
		return false;
	}

	int32 GetInt(int Name, int32 nDefault = 0)
	{
		auto i = m_Values.find(Name);
		if (i == m_Values.end())
		{
			GammaLog << "Current::GetInt error -- key [" << Name << "] not exists";
			return nDefault;
		}
		if (i->second.m_Type != ST_CURRENT_VALUE::VALUE_INT32)
		{
			GammaLog << "Current::GetInt error type[" << i->second.m_Type << "] -- key [" << Name << "] not exists";
			return nDefault;
		}
		return i->second.m_Value.nNum;
	}

	int64 GetInt64(int Name, int64 nDefault = 0)
	{
		auto i = m_Values.find(Name);
		if (i == m_Values.end())
		{
			GammaLog << "Current::GetInt64 error -- key [" << Name << "] not exists";
			return nDefault;
		}
		if (i->second.m_Type != ST_CURRENT_VALUE::VALUE_INT64)
		{
			GammaLog << "Current::GetInt error type[" << i->second.m_Type << "] -- key [" << Name << "] not exists";
			return nDefault;
		}
		return i->second.m_Value.nBigNum;
	}

	CRefSharePtr GetPtr(int Name, CRefSharePtr pDefault = nullptr)
	{
		auto i = m_Values.find(Name);
		if (i == m_Values.end())
		{
			GammaLog << "Current::GetPtr error -- key [" << Name << "] not exists";
			return pDefault;
		}
		if (i->second.m_Type != ST_CURRENT_VALUE::VALUE_PTR)
		{
			GammaLog << "Current::GetInt error type[" << i->second.m_Type << "] -- key [" << Name << "] not exists";
			return pDefault;
		}
		return i->second.m_pPtr;
	}

	CRefShare* GetPtr(int Name, CRefShare* pDefault = nullptr)
	{
		auto i = m_Values.find(Name);
		if (i == m_Values.end())
		{
			GammaLog << "Current::GetPtr error -- key [" << Name << "] not exists";
			return pDefault;
		}
		if (i->second.m_Type != ST_CURRENT_VALUE::VALUE_PTR)
		{
			GammaLog << "Current::GetInt error type[" << i->second.m_Type << "] -- key [" << Name << "] not exists";
			return pDefault;
		}
		return i->second.m_pPtr.get();
	}

	bool Remove(int Name)
	{
		auto i = m_Values.find(Name);
		if (i == m_Values.end())
			return false;
		m_Values.erase(Name);
		return true;
	}

	void Reset()
	{
		m_Values.clear();
	}
};
