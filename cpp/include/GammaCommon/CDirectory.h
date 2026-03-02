#pragma once
#include "GammaCommon/GammaCommonType.h"
#include <string>


#ifdef WIN32
//#include <winbase.h>
#include <windows.h>
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#endif
#pragma warning (disable:4996)

#ifdef WIN32
#define DIR_HANDLE HANDLE
#define STAT_DATA WIN32_FIND_DATA
#else
#define DIR_HANDLE DIR*
#define STAT_DATA struct stat
#endif

#ifndef ulong64
typedef struct _tULong64
{
	unsigned long HighFileSize;
	unsigned long LowFileSize;
}ulong64;
#endif

enum EDIRTYPE
{
	DIR_INEXISTENCE,
	DIR_FILE,
	DIR_DIRECTORY,
};

//操作目录的类，用于实现查询某个目录下的文件名
class GAMMA_COMMON_API CDirectory
{
private :
	bool bOpened;
	bool bNext;
	std::string strDirectory;
	std::string strFileName;
	DIR_HANDLE dp;
	STAT_DATA fp;

public :
	CDirectory();
	~CDirectory();
	//打开指定的目录
	bool OpenDirectory(const char* pathname);
	//查询是否还有文件
	bool HasNextFile();
	//获取下一个文件的名字－不包含路径
	const char* NextFile();
	//返回当前文件是否是目录
	bool IsDirectory();
	//返回当前文件是否是文件
	bool IsFile();
	//返回当前文件所在目录
	const char* GetDirectory();
	//获取当前文件的大小
	ulong64 GetFileSize();
	//判断类型
	static int GetNodeType(const char* pathname);
	//递归创建目录
	static void MakeDirectory(const char *pathname);
	void Close();
};

class GAMMA_COMMON_API CFileFilter
{
private :
	std::string filematchname;

public :
	CFileFilter(){}
	~CFileFilter();
	void SetFilter(const char* filter);
	bool IsMatch(const char* filename);
};

