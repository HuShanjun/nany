#ifndef _FILEUTIL_H_
#define _FILEUTIL_H_
#include "GammaCommon/GammaHelp.h"
/*
基础文件处理的类
*/

class GAMMA_COMMON_API CFileUtil
{
public :
	/**
	 * 获取文件最后的修改时间
	 * @param LastModifiedTime 输出参数，记录文件的最后修改时间
	 * @return 成功则返回0，如果文件不存在，则返回对应的错误码
	 */
	static int32 GetLastModifiedTime(const char* pFileName,time_t& LastModifiedTime);
};

#endif

