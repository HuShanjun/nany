#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

// 共享资源互斥访问锁

class CLockUtil
{
public:
	CLockUtil() 
	{
#ifdef _WIN32
		InitializeCriticalSection(&m_csHandle); 
#else
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&m_Handle, &attr);
#endif
	}

	~CLockUtil() 
	{
#ifdef _WIN32
		DeleteCriticalSection(&m_csHandle); 
#else
		pthread_mutex_destroy(&m_Handle);
#endif
	}

	// 锁定
	void Lock() 
	{
#ifdef _WIN32
		EnterCriticalSection(&m_csHandle); 
#else
		pthread_mutex_lock(&m_Handle);
#endif
	}

	bool TryLock()
	{
#ifdef _WIN32
		return TryEnterCriticalSection(&m_csHandle) != 0;
#else	
		return pthread_mutex_trylock(&m_Handle);
#endif
	}

	// 释放
	void Unlock() 
	{
#ifdef _WIN32
		LeaveCriticalSection(&m_csHandle); 
#else
		pthread_mutex_unlock(&m_Handle);
#endif
	}

private:
#ifdef _WIN32
	CRITICAL_SECTION m_csHandle;
#else
	pthread_mutex_t m_Handle;
	pthread_mutexattr_t attr;
#endif
};

class CAutoLock
{
public:
	CAutoLock(CLockUtil* plock)
	{
		m_pLock = plock;
		if (nullptr == m_pLock)
		{
			return;
		}
		m_pLock->Lock();
	}

	~CAutoLock()
	{
		if (nullptr == m_pLock)
		{
			return;
		}
		m_pLock->Unlock();
	}

private:
	CLockUtil* m_pLock;
};
