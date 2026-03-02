#include "GammaCommon/CFileUtil.h"

#include <sys/stat.h>

/*

   Class	CFileUtil

*/
int32 CFileUtil::GetLastModifiedTime(const char* pFileName,time_t& LastModifiedTime)
{
	struct stat filestat;
	if ( 0 != stat(pFileName,&filestat) )
	{
#ifdef WIN32
		return GetLastError();
#else
		return errno;
#endif
	}
	LastModifiedTime = filestat.st_mtime;
	return 0;
}
