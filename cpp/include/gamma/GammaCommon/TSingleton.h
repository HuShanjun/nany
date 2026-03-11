#ifndef _SINGLETON_H_
#define _SINGLETON_H_

/*
单例类实现，所有需要实现单例模式的类，都需要继承或使用此类。
*/
template <class T>
class CSingleton
{
private :
	static T* m_pInstance;
protected :
	/**
	 * 构造函数
	 *
	 */
	CSingleton() {}


	/**
	 * 析构函数
	 *
	 */
	virtual ~CSingleton() {}
public :
	
	/**
	 * 获取单例对象
	 * @return T
	 */
	static T* Instance()
	{
		if ( m_pInstance == NULL )
		{
			m_pInstance = new T();
		}
		return m_pInstance;
	}

	/**
	 * 强制释放单例资源
	 */
	static void Destroy()
	{
		if ( m_pInstance )
		{
			delete m_pInstance;
			m_pInstance = NULL;
		}
	}
};

template<class T>
T* CSingleton<T>::m_pInstance = NULL;

#endif
