#pragma once
#include "GammaCommon/GammaTime.h"
#include "GammaCommon/CRefshare.h"
#define tmax(a,b)  (((a) > (b)) ? (a) : (b))
#define tmin(a,b)  (((a) < (b)) ? (a) : (b))

enum
{
	TIMER_SECOND,		//秒为单位
	TIMER_MILLISECOND,//毫秒为单位
};
class CTinyTimer : public CRefShare
{
public:
	CTinyTimer(int nSecType = TIMER_SECOND, time_t nInterval = 0)
		: m_nSecType(nSecType), m_tInterval(nInterval), m_tCurrent(0), m_nCount(0) {}
	~CTinyTimer() {}

public://接口
	void Update() { m_tCurrent = this->GetCurrentTime(); }//设置时间标签
	void Start(time_t tSecs) { m_tInterval = tSecs; this->Update(); }//设置时间片并开启定时器
	void AddCount() { ++m_nCount; }
	int GetCount() const { return m_nCount; }
	void Clear() { m_tCurrent = 0; m_tInterval = 0; }
	bool Passed() { return this->GetCurrentTime() >= (m_tCurrent + m_tInterval); }//是否超时
	bool Passed(time_t tSecs) { return this->GetCurrentTime() >= (m_tCurrent + tSecs); }//是否超时指定时间片
	bool Actived() { return m_tCurrent != 0; }//是否激活
	bool TimerOver() { if (this->Actived() && this->Passed()) return this->Clear(), true; return false; }//检查触发一次的定时超时
	bool ToNextTime(time_t tSecs) { if (this->Passed(tSecs)) return m_tCurrent += tSecs, true; return false; }//变长超时事件
	bool ToNextTime() { if (this->Passed()) return m_tCurrent += m_tInterval, true; return false; }//变长超时事件
	bool ToNextTimeReset(time_t tSecs) { if (this->Passed()) return m_tCurrent = this->GetCurrentTime() + tSecs, true; return false; }//变长超时事件(重置为当前时间开始)
	void setType(int nSecType) { m_nSecType = nSecType; }
	void setCurrent(time_t tCurrent) { m_tCurrent = tCurrent; }
public:
	time_t GetInterval() { return m_tInterval; }
	time_t GetCurrent() { return m_tCurrent; }
	time_t GetRemain() { return m_tCurrent ? tmin(tmax(m_tInterval - (this->GetCurrentTime() - m_tCurrent), 0), m_tInterval) : 0; }
	void IncInterval(time_t nSec, time_t nLimit = 0xFFFFFFF) { m_tInterval = tmin(m_tInterval + nSec, nLimit); }
	void DecInterval(time_t nSec) { m_tInterval = tmax(m_tInterval - nSec, 0); }

private:
	//按照type获取当前时间(秒/毫秒)
	time_t GetCurrentTime()
	{
		if (m_nSecType == TIMER_MILLISECOND)
			return Gamma::GetGammaTime();
		return Gamma::GetGammaTime() / 1000;
	}

private:
	time_t m_tInterval;//时间片
	time_t m_tCurrent;//当前时间标签
	int	m_nSecType;
	int m_nCount;
};
typedef CRefObject<CTinyTimer> CTinyTimerPtr;
