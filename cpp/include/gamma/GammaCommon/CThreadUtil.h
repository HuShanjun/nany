#pragma once
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#endif
#include <time.h>

// 线程

class CThread
{
private:
#ifdef _WIN32
	typedef void (__cdecl* LOOP_FUNC)(void*);
	typedef bool (__cdecl* INIT_FUNC)(void*);
	typedef bool (__cdecl* SHUT_FUNC)(void*);
	
	// 线程函数
	static void __cdecl WorkerProc(void* lpParameter)
#else
	typedef void(* LOOP_FUNC)(void*);
	typedef bool(* INIT_FUNC)(void*);
	typedef bool(* SHUT_FUNC)(void*);

	// 线程函数
	static void* WorkerProc(void* lpParameter)
#endif
	{
		CThread* pthis = (CThread*)lpParameter;
		LOOP_FUNC loop_func = pthis->m_LoopFunc;
		void* context = pthis->m_pContext;
		int sleep_ms = pthis->m_nSleep;
		
		if (pthis->m_InitFunc)
		{
			pthis->m_InitFunc(context);
		}

		::srand((unsigned int)time(nullptr));

		while (!pthis->m_bQuit)
		{
			loop_func(context);
			
			if (sleep_ms > 0)
			{
#ifdef _WIN32
				Sleep(sleep_ms);
#else
				struct timespec ts;
				ts.tv_sec = 0;
				ts.tv_nsec = sleep_ms * 1000000;
				nanosleep(&ts, NULL);
#endif
			}
			else if (sleep_ms == 0)
			{
#ifdef _WIN32
				Sleep(sleep_ms);
#else
				sched_yield();
#endif
			}
		}
		
		if (pthis->m_ShutFunc)
		{
			pthis->m_ShutFunc(context);
		}
#ifndef _WIN32
		return nullptr;
#endif // !_WIN32
	}
	
public:
	CThread(LOOP_FUNC loop_func, INIT_FUNC init_func, SHUT_FUNC shut_func, 
		void* context, int sleep_ms, int stack_size)
	{
		m_LoopFunc = loop_func;
		m_InitFunc = init_func;
		m_ShutFunc = shut_func;
		m_pContext = context;
		m_nSleep = sleep_ms;		
		m_nStackSize = stack_size;
		m_bQuit = false;
#ifdef _WIN32
		m_hThread = NULL;
#else
		m_hThread = -1;
#endif
	}
	
	bool IsValid() const
	{
#ifdef _WIN32
		return m_hThread != NULL;
#else
		return m_hThread != -1;
#endif
	}

	~CThread()
	{
	}
	
	// 是否已退出线程
	void SetQuit(bool value) { m_bQuit = value; }
	bool GetQuit() const { return m_bQuit; }
	
	// 启动线程
	bool Start()
	{
		m_bQuit = false;
#ifdef _WIN32
		m_hThread = (HANDLE)_beginthread(WorkerProc, m_nStackSize, this);
		return true;
#else
		int res = pthread_create(&m_hThread, NULL, WorkerProc, this);
		return (0 == res);
#endif
	}

	// 停止线程
	bool Stop()
	{
		if (m_bQuit == true)
		{
			return true;
		}
		m_bQuit = true;
	
		if (IsValid())
		{
#ifdef _WIN32
			WaitThreadExit(m_hThread);
			m_hThread = NULL;
#else
			pthread_join(m_hThread, NULL);
			m_hThread = -1;
#endif
		}
	
		return true;
	}
	
private:
	CThread();
	CThread(const CThread&);
	CThread& operator=(const CThread&);
	
#ifdef _WIN32
	// 等待线程结束
	bool WaitThreadExit(HANDLE handle)
	{
		for (;;)
		{
			DWORD res = WaitForSingleObject(handle, 1000);
			
			if (res == WAIT_OBJECT_0)
			{
				return true;
			}
			else if (res == WAIT_TIMEOUT)
			{
				DWORD exit_code;
				
				if (GetExitCodeThread(handle, &exit_code) == FALSE)
				{
					return false;
				}
				
				if (exit_code != STILL_ACTIVE)
				{
					break;
				}
			}
			else
			{
				return false;
			}
		}
		
		return true;
	}
#endif

private:
	LOOP_FUNC m_LoopFunc;
	INIT_FUNC m_InitFunc;
	SHUT_FUNC m_ShutFunc;
	void* m_pContext;
	int m_nSleep;
	int m_nStackSize;
	bool m_bQuit;
#ifdef _WIN32
	HANDLE m_hThread;
#else
	pthread_t m_hThread;
#endif
};

// 线程等待

class CThreadWaiter
{
public:
	CThreadWaiter()
	{
#ifdef _WIN32
		m_hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
#else
		pthread_cond_init(&m_cond, NULL);
		pthread_mutex_init(&m_mutex, NULL);
#endif
	}

	~CThreadWaiter()
	{
#ifdef _WIN32
		CloseHandle(m_hEvent);
#else
		pthread_cond_destroy(&m_cond);
		pthread_mutex_destroy(&m_mutex);
#endif
	}
	
	// 等待线程唤醒
	bool Wait(int ms)
	{
		bool result = false;

		if (ms < 0)
		{
#ifdef _WIN32
			// 无限时间等待
			WaitForSingleObject(m_hEvent, INFINITE);
			result = true;
#else
			// 无限时间等待
			pthread_mutex_lock(&m_mutex);
			int res = pthread_cond_wait(&m_cond, &m_mutex);
			if (0 == res)
			{
				result = true;
			}
			pthread_mutex_unlock(&m_mutex);
#endif
		}
		else
		{
#ifdef _WIN32
			DWORD res = WaitForSingleObject(m_hEvent, ms);
			
			if (WAIT_TIMEOUT == res)
			{
				result = false;
			}
#else
			struct timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_nsec += ms * 1000000;
			if (ts.tv_nsec > 999999999)
			{
				ts.tv_sec += 1;
				ts.tv_nsec -= 1000000000;
			}
			pthread_mutex_lock(&m_mutex);
			int res = pthread_cond_timedwait(&m_cond, &m_mutex, &ts);
			if (0 == res)
			{
				result = true;
			}
			pthread_mutex_unlock(&m_mutex);
#endif
		}
		
		return result;
	}
	
	// 唤醒
	bool Signal()
	{
#ifdef _WIN32
		return SetEvent(m_hEvent) == TRUE;
#else
		return pthread_cond_signal(&m_cond);
#endif
	}

	void Reset(void)
	{
#ifdef _WIN32
		ResetEvent(m_hEvent);
#endif
	}
	
private:
#ifdef _WIN32
	HANDLE m_hEvent;
#else
	pthread_cond_t m_cond;
	pthread_mutex_t m_mutex;
#endif
};

