#pragma once

#include "GammaCommon/GammaHelp.h"

#ifdef WIN32
typedef HINSTANCE DLLHANDLE;
#else
#include <dlfcn.h>
typedef void* DLLHANDLE;
#endif

class GAMMA_COMMON_API CDynamicFile
{
private :
	DLLHANDLE m_hDll;
	char m_ErrMsg[100];
	std::string m_strFileName;
public :
	/**
	 * 构造函数
	 */
	CDynamicFile();
	/**
	 * 析构函数
	 */
	~CDynamicFile();
	/**
	 * 打开动态库
	 * @param pFileName 动态库的文件名
	 * @return bool 打开是否成功，如果失败，详见getError()
	 */
	bool open(const char* pFileName);
	/**
	 * 获取dll函数的指针
	 * @param pFuncName 函数名
	 * @return void* 函数指针，如果失败，则返回NULL
	 */
	void* getFuncAddr(const char* pFuncName);
	/**
	 * 关闭动态库
	 */
	void close();
	/**
	 * 获取错误信息
	 */
	const char* getError();

	const char* getFileName();
};

