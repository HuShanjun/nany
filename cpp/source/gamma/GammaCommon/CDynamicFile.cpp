#include "GammaCommon/CDynamicFile.h"

/*

		Class	CDynamicFile

*/
CDynamicFile::CDynamicFile()
{
	m_hDll = NULL;
	m_strFileName = "";
}

CDynamicFile::~CDynamicFile()
{
	close();
}

bool CDynamicFile::open(const char* pFileName)
{
	m_strFileName = pFileName;
#ifdef WIN32
	m_hDll = LoadLibrary(pFileName);
	if ( m_hDll == NULL )
		return false;
#else
	m_hDll = dlopen(pFileName,RTLD_NOW);
	if ( m_hDll == NULL )
	{
		fprintf(stderr, "file:%s error:%s\n", pFileName, dlerror());
		return false;
	}
#endif
	return true;
}

void CDynamicFile::close()
{
	if ( m_hDll != NULL )
	{
#ifdef WIN32
		FreeLibrary(m_hDll);
#else
		dlclose(m_hDll);
#endif
	}
}

void* CDynamicFile::getFuncAddr(const char *pFuncName)
{
	if ( m_hDll == NULL )
		return NULL;
#ifdef WIN32
	return GetProcAddress(m_hDll,pFuncName);
#else
	return dlsym(m_hDll,pFuncName);
#endif
}

const char* CDynamicFile::getError()
{
#ifdef WIN32
	sprintf(m_ErrMsg,"error code[%d]",GetLastError());
#else
	sprintf(m_ErrMsg,"%s",dlerror());
#endif
	return m_ErrMsg;
}

const char* CDynamicFile::getFileName()
{
	return m_strFileName.c_str();
}